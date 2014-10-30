#if defined(HARD)
#define STATIC
struct TParserClosure {
#else
#define STATIC static
#endif
 struct Bigint {
    struct Bigint* next;
    int k, maxwds, sign, wds;
    ULong x[1];
 };

 typedef struct Bigint Bigint;

 inline Bigint* Balloc(int k) {
#if defined(HARD)
   int x = 1 << k;
   Bigint* rv = (Bigint*)Allocate(sizeof(Bigint) + (x-1)*sizeof(ULong));
   rv->sign = rv->wds = 0;
   rv->k = k;
   rv->maxwds = x;
   rv->next = 0;
   return rv;
#else
   (void)k;
   abort();
   return 0;
#endif
 }

 inline void Bfree(Bigint*) {
 }
#if defined(HARD)
 struct TResHelper {
     class TAlloc: public IAllocator {
         public:
             TAlloc()
                 : isAllocated(false)
             {
             }

             virtual TBlock Allocate(size_t) {
                 if (isAllocated) {
                     abort();
                 }
                 TBlock ret = {mBuf.Data(), mBuf.Size()};

                 isAllocated = true;
                 return ret;
             }

             virtual void Release(const TBlock&) {
             }

       private:
             TTempBuf mBuf;
             bool isAllocated;
     };

     inline TResHelper()
         : MyPool(1, TMemoryPool::TExpGrow::Instance(), &Alloc)
         , Pool(&MyPool)
     {
     }

     inline void Set(TMemoryPool* pool) throw () {
         Pool = pool;
     }

     static inline TMemoryPool* GlobalPool() {
         static TMemoryPool pool(8192);

         return &pool;
     }

     inline void SetGlobal() {
         Set(GlobalPool());
     }

     inline void SetLocal() {
         Set(&MyPool);
     }

     TAlloc Alloc;
     TMemoryPool MyPool;
     TMemoryPool* Pool;
 };

 char mTmpData[sizeof(TResHelper) + PLATFORM_DATA_ALIGN];
 THolder<TResHelper, TDestructor> mRes;

 inline TResHelper* Res() {
     if (!mRes) {
         mRes.Reset(new (AlignUp(mTmpData)) TResHelper);
     }

     return mRes.Get();
 }

 inline void* Allocate(size_t len) {
     return Res()->Pool->Allocate(len);
 }

 inline void SetGlobal() {
     Res()->SetGlobal();
 }

 inline void SetLocal() {
     Res()->SetLocal();
 }
#else
 static inline void SetGlobal() {
 }

 static inline void SetLocal() {
 }
#endif

#define Bcopy(x,y) memcpy((char *)&x->sign, (char *)&y->sign, \
y->wds*sizeof(Long) + 2*sizeof(int))

STATIC inline Bigint* multadd(Bigint *b, int m, int a)    /* multiply by m and add a */
{
    int i, wds;
#ifdef ULLong
    ULong *x;
    ULLong carry, y;
#else
    ULong carry, *x, y;
#ifdef Pack_32
    ULong xi, z;
#endif
#endif
    Bigint *b1;

    wds = b->wds;
    x = b->x;
    i = 0;
    carry = a;
    do {
#ifdef ULLong
        y = *x * (ULLong)m + carry;
        carry = y >> 32;
        *x++ = y & FFFFFFFF;
#else
#ifdef Pack_32
        xi = *x;
        y = (xi & 0xffff) * m + carry;
        z = (xi >> 16) * m + (y >> 16);
        carry = z >> 16;
        *x++ = (z << 16) + (y & 0xffff);
#else
        y = *x * m + carry;
        carry = y >> 16;
        *x++ = y & 0xffff;
#endif
#endif
        }
        while(++i < wds);
    if (carry) {
        if (wds >= b->maxwds) {
            b1 = Balloc(b->k+1);
            Bcopy(b1, b);
            Bfree(b);
            b = b1;
            }
        b->x[wds++] = carry;
        b->wds = wds;
        }
    return b;
    }

STATIC inline Bigint* s2b(TBufIt s, int nd0, int nd, ULong y9)
{
    Bigint *b;
    int i, k;
    Long x, y;

    x = (nd + 8) / 9;
    for(k = 0, y = 1; x > y; y <<= 1, k++) ;
#ifdef Pack_32
    b = Balloc(k);
    b->x[0] = y9;
    b->wds = 1;
#else
    b = Balloc(k+1);
    b->x[0] = y9 & 0xffff;
    b->wds = (b->x[1] = y9 >> 16) ? 2 : 1;
#endif

    i = 9;
    if (9 < nd0) {
        s += 9;
        do b = multadd(b, 10, *s++ - '0');
            while(++i < nd0);
        s++;
        }
    else
        s += 10;
    for(; i < nd; i++)
        b = multadd(b, 10, *s++ - '0');
    return b;
    }

static inline int hi0bits(register ULong x)
{
    register int k = 0;

    if (!(x & 0xffff0000)) {
        k = 16;
        x <<= 16;
        }
    if (!(x & 0xff000000)) {
        k += 8;
        x <<= 8;
        }
    if (!(x & 0xf0000000)) {
        k += 4;
        x <<= 4;
        }
    if (!(x & 0xc0000000)) {
        k += 2;
        x <<= 2;
        }
    if (!(x & 0x80000000)) {
        k++;
        if (!(x & 0x40000000))
            return 32;
        }
    return k;
    }

static inline int lo0bits(ULong *y)
{
    register int k;
    register ULong x = *y;

    if (x & 7) {
        if (x & 1)
            return 0;
        if (x & 2) {
            *y = x >> 1;
            return 1;
            }
        *y = x >> 2;
        return 2;
        }
    k = 0;
    if (!(x & 0xffff)) {
        k = 16;
        x >>= 16;
        }
    if (!(x & 0xff)) {
        k += 8;
        x >>= 8;
        }
    if (!(x & 0xf)) {
        k += 4;
        x >>= 4;
        }
    if (!(x & 0x3)) {
        k += 2;
        x >>= 2;
        }
    if (!(x & 1)) {
        k++;
        x >>= 1;
        if (!x)
            return 32;
        }
    *y = x;
    return k;
    }

STATIC inline Bigint* i2b(int i)
{
    Bigint *b;

    b = Balloc(1);
    b->x[0] = i;
    b->wds = 1;
    return b;
    }

STATIC inline Bigint* mult(Bigint *a, Bigint *b)
{
    Bigint *c;
    int k, wa, wb, wc;
    ULong *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    ULong y;
#ifdef ULLong
    ULLong carry, z;
#else
    ULong carry, z;
#ifdef Pack_32
    ULong z2;
#endif
#endif

    if (a->wds < b->wds) {
        c = a;
        a = b;
        b = c;
        }
    k = a->k;
    wa = a->wds;
    wb = b->wds;
    wc = wa + wb;
    if (wc > a->maxwds)
        k++;
    c = Balloc(k);
    for(x = c->x, xa = x + wc; x < xa; x++)
        *x = 0;
    xa = a->x;
    xae = xa + wa;
    xb = b->x;
    xbe = xb + wb;
    xc0 = c->x;
#ifdef ULLong
    for(; xb < xbe; xc0++) {
        if ((y = *xb++)) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = *x++ * (ULLong)y + *xc + carry;
                carry = z >> 32;
                *xc++ = z & FFFFFFFF;
                }
                while(x < xae);
            *xc = carry;
            }
        }
#else
#ifdef Pack_32
    for(; xb < xbe; xb++, xc0++) {
        if (y = *xb & 0xffff) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
                carry = z >> 16;
                z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
                carry = z2 >> 16;
                Storeinc(xc, z2, z);
                }
                while(x < xae);
            *xc = carry;
            }
        if (y = *xb >> 16) {
            x = xa;
            xc = xc0;
            carry = 0;
            z2 = *xc;
            do {
                z = (*x & 0xffff) * y + (*xc >> 16) + carry;
                carry = z >> 16;
                Storeinc(xc, z, z2);
                z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
                carry = z2 >> 16;
                }
                while(x < xae);
            *xc = z2;
            }
        }
#else
    for(; xb < xbe; xc0++) {
        if (y = *xb++) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = *x++ * y + *xc + carry;
                carry = z >> 16;
                *xc++ = z & 0xffff;
                }
                while(x < xae);
            *xc = carry;
            }
        }
#endif
#endif
    for(xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) ;
    c->wds = wc;
    return c;
    }

    STATIC inline Bigint* NextPower(Bigint* p) {
        if (p) {
            if (!p->next) {
                TGuard<TAtomic> guard(POWER_LOCK);
                volatile Bigint* pSafe = p;
                if (!pSafe->next) {
                    SetGlobal();
                    pSafe->next = mult(p, p);
                    SetLocal();
                }
            }

            return p->next;
        } else {
            static Bigint* volatile ret = 0;

            if (!ret) {
                TGuard<TAtomic> guard(POWER_LOCK);
                if (!ret) {
                    SetGlobal();
                    ret = i2b(625);
                    SetLocal();
                }
            }

            return ret;
        }
    }

STATIC inline Bigint* pow5mult(Bigint *b, int k)
{
    Bigint *b1, *p5;
    int i;
    static int p05[3] = { 5, 25, 125 };

    if ((i = k & 3))
        b = multadd(b, p05[i-1], 0);

    if (!(k >>= 2))
        return b;
    p5 = NextPower(0);

    for(;;) {
        if (k & 1) {
            b1 = mult(b, p5);
            Bfree(b);
            b = b1;
            }
        if (!(k >>= 1))
            break;
        p5 = NextPower(p5);
        }
    return b;
    }

STATIC inline Bigint* lshift(Bigint *b, int k)
{
    int i, k1, n, n1;
    Bigint *b1;
    ULong *x, *x1, *xe, z;

#ifdef Pack_32
    n = k >> 5;
#else
    n = k >> 4;
#endif
    k1 = b->k;
    n1 = n + b->wds + 1;
    for(i = b->maxwds; n1 > i; i <<= 1)
        k1++;
    b1 = Balloc(k1);
    x1 = b1->x;
    for(i = 0; i < n; i++)
        *x1++ = 0;
    x = b->x;
    xe = x + b->wds;
#ifdef Pack_32
    if (k &= 0x1f) {
        k1 = 32 - k;
        z = 0;
        do {
            *x1++ = *x << k | z;
            z = *x++ >> k1;
            }
            while(x < xe);
        if ((*x1 = z))
            ++n1;
        }
#else
    if (k &= 0xf) {
        k1 = 16 - k;
        z = 0;
        do {
            *x1++ = *x << k  & 0xffff | z;
            z = *x++ >> k1;
            }
            while(x < xe);
        if (*x1 = z)
            ++n1;
        }
#endif
    else do
        *x1++ = *x++;
        while(x < xe);
    b1->wds = n1 - 1;
    Bfree(b);
    return b1;
    }

static inline int cmp(Bigint *a, Bigint *b)
{
    ULong *xa, *xa0, *xb, *xb0;
    int i, j;

    i = a->wds;
    j = b->wds;
#ifdef DEBUG
    if (i > 1 && !a->x[i-1])
        Bug("cmp called with a->x[a->wds-1] == 0");
    if (j > 1 && !b->x[j-1])
        Bug("cmp called with b->x[b->wds-1] == 0");
#endif
    if (i -= j)
        return i;
    xa0 = a->x;
    xa = xa0 + j;
    xb0 = b->x;
    xb = xb0 + j;
    for(;;) {
        if (*--xa != *--xb)
            return *xa < *xb ? -1 : 1;
        if (xa <= xa0)
            break;
        }
    return 0;
    }

STATIC inline Bigint* diff(Bigint *a, Bigint *b)
{
    Bigint *c;
    int i, wa, wb;
    ULong *xa, *xae, *xb, *xbe, *xc;
#ifdef ULLong
    ULLong borrow, y;
#else
    ULong borrow, y;
#ifdef Pack_32
    ULong z;
#endif
#endif

    i = cmp(a,b);
    if (!i) {
        c = Balloc(0);
        c->wds = 1;
        c->x[0] = 0;
        return c;
        }
    if (i < 0) {
        c = a;
        a = b;
        b = c;
        i = 1;
        }
    else
        i = 0;
    c = Balloc(a->k);
    c->sign = i;
    wa = a->wds;
    xa = a->x;
    xae = xa + wa;
    wb = b->wds;
    xb = b->x;
    xbe = xb + wb;
    xc = c->x;
    borrow = 0;
#ifdef ULLong
    do {
        y = (ULLong)*xa++ - *xb++ - borrow;
        borrow = y >> 32 & (ULong)1;
        *xc++ = y & FFFFFFFF;
        }
        while(xb < xbe);
    while(xa < xae) {
        y = *xa++ - borrow;
        borrow = y >> 32 & (ULong)1;
        *xc++ = y & FFFFFFFF;
        }
#else
#ifdef Pack_32
    do {
        y = (*xa & 0xffff) - (*xb & 0xffff) - borrow;
        borrow = (y & 0x10000) >> 16;
        z = (*xa++ >> 16) - (*xb++ >> 16) - borrow;
        borrow = (z & 0x10000) >> 16;
        Storeinc(xc, z, y);
        }
        while(xb < xbe);
    while(xa < xae) {
        y = (*xa & 0xffff) - borrow;
        borrow = (y & 0x10000) >> 16;
        z = (*xa++ >> 16) - borrow;
        borrow = (z & 0x10000) >> 16;
        Storeinc(xc, z, y);
        }
#else
    do {
        y = *xa++ - *xb++ - borrow;
        borrow = (y & 0x10000) >> 16;
        *xc++ = y & 0xffff;
        }
        while(xb < xbe);
    while(xa < xae) {
        y = *xa++ - borrow;
        borrow = (y & 0x10000) >> 16;
        *xc++ = y & 0xffff;
        }
#endif
#endif
    while(!*--xc)
        wa--;
    c->wds = wa;
    return c;
    }

static inline double ulp(double xx)
{
    register Long L;
    U a, x;
    dval(x) = xx;

    L = (word0(x) & Exp_mask) - (P-1)*Exp_msk1;
#ifndef Avoid_Underflow
#ifndef Sudden_Underflow
    if (L > 0) {
#endif
#endif
#ifdef IBM
        L |= Exp_msk1 >> 4;
#endif
        word0(a) = L;
        word1(a) = 0;
#ifndef Avoid_Underflow
#ifndef Sudden_Underflow
        }
    else {
        L = -L >> Exp_shift;
        if (L < Exp_shift) {
            word0(a) = 0x80000 >> L;
            word1(a) = 0;
            }
        else {
            word0(a) = 0;
            L -= Exp_shift;
            word1(a) = L >= 31 ? 1 : 1 << 31 - L;
            }
        }
#endif
#endif
    return dval(a);
    }

static inline double b2d(Bigint *a, int *e)
{
    ULong *xa, *xa0, w, y, z;
    int k;
    U d;
#ifdef VAX
    ULong d0, d1;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

    xa0 = a->x;
    xa = xa0 + a->wds;
    y = *--xa;
#ifdef DEBUG
    if (!y) Bug("zero y in b2d");
#endif
    k = hi0bits(y);
    *e = 32 - k;
#ifdef Pack_32
    if (k < Ebits) {
        d0 = Exp_1 | y >> Ebits - k;
        w = xa > xa0 ? *--xa : 0;
        d1 = y << (32-Ebits) + k | w >> Ebits - k;
        goto ret_d;
        }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits) {
        d0 = Exp_1 | y << k | z >> 32 - k;
        y = xa > xa0 ? *--xa : 0;
        d1 = z << k | y >> 32 - k;
        }
    else {
        d0 = Exp_1 | y;
        d1 = z;
        }
#else
    if (k < Ebits + 16) {
        z = xa > xa0 ? *--xa : 0;
        d0 = Exp_1 | y << k - Ebits | z >> Ebits + 16 - k;
        w = xa > xa0 ? *--xa : 0;
        y = xa > xa0 ? *--xa : 0;
        d1 = z << k + 16 - Ebits | w << k - Ebits | y >> 16 + Ebits - k;
        goto ret_d;
        }
    z = xa > xa0 ? *--xa : 0;
    w = xa > xa0 ? *--xa : 0;
    k -= Ebits + 16;
    d0 = Exp_1 | y << k + 16 | z << k | w >> 16 - k;
    y = xa > xa0 ? *--xa : 0;
    d1 = w << k + 16 | y << k;
#endif
 ret_d:
#ifdef VAX
    word0(d) = d0 >> 16 | d0 << 16;
    word1(d) = d1 >> 16 | d1 << 16;
#else
#undef d0
#undef d1
#endif
    return dval(d);
    }

STATIC inline Bigint* d2b(double dd, int *e, int *bits)
{
    Bigint *b;
    int de, k;
    ULong *x, y, z;
    U d;
    dval(d) = dd;
#ifndef Sudden_Underflow
    int i;
#endif
#ifdef VAX
    ULong d0, d1;
    d0 = word0(d) >> 16 | word0(d) << 16;
    d1 = word1(d) >> 16 | word1(d) << 16;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

#ifdef Pack_32
    b = Balloc(1);
#else
    b = Balloc(2);
#endif
    x = b->x;

    z = d0 & Frac_mask;
    d0 &= 0x7fffffff;    /* clear sign bit, which we ignore */
#ifdef Sudden_Underflow
    de = (int)(d0 >> Exp_shift);
#ifndef IBM
    z |= Exp_msk11;
#endif
#else
    if ((de = (int)(d0 >> Exp_shift)))
        z |= Exp_msk1;
#endif
#ifdef Pack_32
    if ((y = d1)) {
        if ((k = lo0bits(&y))) {
            x[0] = y | z << 32 - k;
            z >>= k;
            }
        else
            x[0] = y;
#ifndef Sudden_Underflow
        i =
#endif
            b->wds = (x[1] = z) ? 2 : 1;
        }
    else {
#ifdef DEBUG
        if (!z)
            Bug("Zero passed to d2b");
#endif
        k = lo0bits(&z);
        x[0] = z;
#ifndef Sudden_Underflow
        i =
#endif
            b->wds = 1;
        k += 32;
        }
#else
    if (y = d1) {
        if (k = lo0bits(&y))
            if (k >= 16) {
                x[0] = y | z << 32 - k & 0xffff;
                x[1] = z >> k - 16 & 0xffff;
                x[2] = z >> k;
                i = 2;
                }
            else {
                x[0] = y & 0xffff;
                x[1] = y >> 16 | z << 16 - k & 0xffff;
                x[2] = z >> k & 0xffff;
                x[3] = z >> k+16;
                i = 3;
                }
        else {
            x[0] = y & 0xffff;
            x[1] = y >> 16;
            x[2] = z & 0xffff;
            x[3] = z >> 16;
            i = 3;
            }
        }
    else {
#ifdef DEBUG
        if (!z)
            Bug("Zero passed to d2b");
#endif
        k = lo0bits(&z);
        if (k >= 16) {
            x[0] = z;
            i = 0;
            }
        else {
            x[0] = z & 0xffff;
            x[1] = z >> 16;
            i = 1;
            }
        k += 32;
        }
    while(!x[i])
        --i;
    b->wds = i + 1;
#endif
#ifndef Sudden_Underflow
    if (de) {
#endif
#ifdef IBM
        *e = (de - Bias - (P-1) << 2) + k;
        *bits = 4*P + 8 - k - hi0bits(word0(d) & Frac_mask);
#else
        *e = de - Bias - (P-1) + k;
        *bits = P - k;
#endif
#ifndef Sudden_Underflow
        }
    else {
        *e = de - Bias - (P-1) + 1 + k;
#ifdef Pack_32
        *bits = 32*i - hi0bits(x[i-1]);
#else
        *bits = (i+2)*16 - hi0bits(x[i]);
#endif
        }
#endif
    return b;
    }
#undef d0
#undef d1

static inline double ratio(Bigint *a, Bigint *b)
{
    U da, db;
    int k, ka, kb;

    dval(da) = b2d(a, &ka);
    dval(db) = b2d(b, &kb);
#ifdef Pack_32
    k = ka - kb + 32*(a->wds - b->wds);
#else
    k = ka - kb + 16*(a->wds - b->wds);
#endif
#ifdef IBM
    if (k > 0) {
        word0(da) += (k >> 2)*Exp_msk1;
        if (k &= 3)
            dval(da) *= 1 << k;
        }
    else {
        k = -k;
        word0(db) += (k >> 2)*Exp_msk1;
        if (k &= 3)
            dval(db) *= 1 << k;
        }
#else
    if (k > 0)
        word0(da) += k*Exp_msk1;
    else {
        k = -k;
        word0(db) += k*Exp_msk1;
        }
#endif
    return dval(da) / dval(db);
    }

#ifdef INFNAN_CHECK

#ifndef NAN_WORD0
#define NAN_WORD0 0x7ff80000
#endif

#ifndef NAN_WORD1
#define NAN_WORD1 0
#endif

static inline int match(CONST char **sp, char *t)
{
    int c, d;
    CONST char *s = *sp;

    while(d = *t++) {
        if ((c = *++s) >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        if (c != d)
            return 0;
        }
    *sp = s + 1;
    return 1;
    }

#ifndef No_Hex_NaN
static inline void hexnan(double *rvp, CONST char **sp)
{
    ULong c, x[2];
    CONST char *s;
    int havedig, udx0, xshift;

    x[0] = x[1] = 0;
    havedig = xshift = 0;
    udx0 = 1;
    s = *sp;
    /* allow optional initial 0x or 0X */
    while((c = *(CONST unsigned char*)(s+1)) && c <= ' ')
        ++s;
    if (s[1] == '0' && (s[2] == 'x' || s[2] == 'X'))
        s += 2;
    while(c = *(CONST unsigned char*)++s) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
            c += 10 - 'a';
        else if (c >= 'A' && c <= 'F')
            c += 10 - 'A';
        else if (c <= ' ') {
            if (udx0 && havedig) {
                udx0 = 0;
                xshift = 1;
                }
            continue;
            }
#ifdef GDTOA_NON_PEDANTIC_NANCHECK
        else if (/*(*/ c == ')' && havedig) {
            *sp = s + 1;
            break;
            }
        else
            return;    /* invalid form: don't change *sp */
#else
        else {
            do {
                if (/*(*/ c == ')') {
                    *sp = s + 1;
                    break;
                    }
                } while(c = *++s);
            break;
            }
#endif
        havedig = 1;
        if (xshift) {
            xshift = 0;
            x[0] = x[1];
            x[1] = 0;
            }
        if (udx0)
            x[0] = (x[0] << 4) | (x[1] >> 28);
        x[1] = (x[1] << 4) | c;
        }
    if ((x[0] &= 0xfffff) || x[1]) {
        word0(*rvp) = Exp_mask | x[0];
        word1(*rvp) = x[1];
        }
    }
#endif /*No_Hex_NaN*/
#endif /* INFNAN_CHECK */

STATIC inline double strtod(TBufIt s00, char** se)
{
#if !defined(HARD)
    TBufIt copy00(s00);
#endif

#ifdef Avoid_Underflow
    int scale;
#endif
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign,
         e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
    TBufIt s, s0, s1;
    double aadj, adj;
    U rv, rv0, aadj1;
    Long L;
    ULong y, z;
    Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;

#if !defined(HARD)
    bb = 0;
    bd = 0;
    bs = 0;
    delta = 0;
#endif

#ifdef SET_INEXACT
    int inexact, oldinexact;
#endif
#ifdef Honor_FLT_ROUNDS /*{*/
    int Rounding;
#ifdef Trust_FLT_ROUNDS /*{{ only define this if FLT_ROUNDS really works! */
    Rounding = Flt_Rounds;
#else /*}{*/
    Rounding = 1;
    switch(fegetround()) {
      case FE_TOWARDZERO:    Rounding = 0; break;
      case FE_UPWARD:    Rounding = 2; break;
      case FE_DOWNWARD:    Rounding = 3;
      }
#endif /*}}*/
#endif /*}*/
#ifdef USE_LOCALE
    CONST char *s2;
#endif

    sign = nz0 = nz = 0;
    dval(rv) = 0.;
    for(s = s00;;s++) switch(*s) {
        case '-':
            sign = 1;
            /* no break */
        case '+':
            if (*++s)
                goto break2;
            /* no break */
        case 0:
            goto ret0;
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
        case ' ':
            continue;
        default:
            goto break2;
        }
 break2:
    if (*s == '0') {
        nz0 = 1;
        while(*++s == '0') ;
        if (!*s)
            goto ret;
        }
    s0 = s;
    y = z = 0;
    for(nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
        if (nd < 9)
            y = 10*y + c - '0';
        else if (nd < 16)
            z = 10*z + c - '0';
    nd0 = nd;
#ifdef USE_LOCALE
    s1 = localeconv()->decimal_point;
    if (c == *s1) {
        c = '.';
        if (*++s1) {
            s2 = s;
            for(;;) {
                if (*++s2 != *s1) {
                    c = 0;
                    break;
                    }
                if (!*++s1) {
                    s = s2;
                    break;
                    }
                }
            }
        }
#endif
    if (c == '.') {
        c = *++s;
        if (!nd) {
            for(; c == '0'; c = *++s)
                nz++;
            if (c > '0' && c <= '9') {
                s0 = s;
                nf += nz;
                nz = 0;
                goto have_dig;
                }
            goto dig_done;
            }
        for(; c >= '0' && c <= '9'; c = *++s) {
 have_dig:
            nz++;
            if (c -= '0') {
                nf += nz;
                for(i = 1; i < nz; i++)
                    if (nd++ < 9)
                        y *= 10;
                    else if (nd <= DBL_DIG + 1)
                        z *= 10;
                if (nd++ < 9)
                    y = 10*y + c;
                else if (nd <= DBL_DIG + 1)
                    z = 10*z + c;
                nz = 0;
                }
            }
        }
 dig_done:
    e = 0;
    if (c == 'e' || c == 'E') {
        if (!nd && !nz && !nz0) {
            goto ret0;
            }
        s00 = s;
        esign = 0;
        switch(c = *++s) {
            case '-':
                esign = 1;
            case '+':
                c = *++s;
            }
        if (c >= '0' && c <= '9') {
            while(c == '0')
                c = *++s;
            if (c > '0' && c <= '9') {
                L = c - '0';
                s1 = s;
                while((c = *++s) >= '0' && c <= '9')
                    L = 10*L + c - '0';
                if (s - s1 > 8 || L > 19999)
                    /* Avoid confusion from exponents
                     * so large that e might overflow.
                     */
                    e = 19999; /* safe for 16 bit ints */
                else
                    e = (int)L;
                if (esign)
                    e = -e;
                }
            else
                e = 0;
            }
        else
            s = s00;
        }
    if (!nd) {
        if (!nz && !nz0) {
#ifdef INFNAN_CHECK
            /* Check for Nan and Infinity */
            switch(c) {
              case 'i':
              case 'I':
                if (match(&s,"nf")) {
                    --s;
                    if (!match(&s,"inity"))
                        ++s;
                    word0(rv) = 0x7ff00000;
                    word1(rv) = 0;
                    goto ret;
                    }
                break;
              case 'n':
              case 'N':
                if (match(&s, "an")) {
                    word0(rv) = NAN_WORD0;
                    word1(rv) = NAN_WORD1;
#ifndef No_Hex_NaN
                    if (*s == '(') /*)*/
                        hexnan(&rv, &s);
#endif
                    goto ret;
                    }
              }
#endif /* INFNAN_CHECK */
 ret0:
            s = s00;
            sign = 0;
            }
        goto ret;
        }
    e1 = e -= nf;

    /* Now we have nd0 digits, starting at s0, followed by a
     * decimal point, followed by nd-nd0 digits.  The number we're
     * after is the integer represented by those digits times
     * 10**e */

    if (!nd0)
        nd0 = nd;
    k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
    dval(rv) = y;
    if (k > 9) {
#ifdef SET_INEXACT
        if (k > DBL_DIG)
            oldinexact = get_inexact();
#endif
        dval(rv) = tens[k - 9] * dval(rv) + z;
        }
    bd0 = 0;
    if (nd <= DBL_DIG
#ifndef RND_PRODQUOT
#ifndef Honor_FLT_ROUNDS
        && Flt_Rounds == 1
#endif
#endif
            ) {
        if (!e)
            goto ret;
        if (e > 0) {
            if (e <= Ten_pmax) {
#ifdef VAX
                goto vax_ovfl_check;
#else
#ifdef Honor_FLT_ROUNDS
                /* round correctly FLT_ROUNDS = 2 or 3 */
                if (sign) {
                    dval(rv) = -dval(rv);
                    sign = 0;
                    }
#endif
                /* rv = */ rounded_product(dval(rv), tens[e]);
                goto ret;
#endif
                }
            i = DBL_DIG - nd;
            if (e <= Ten_pmax + i) {
                /* A fancier test would sometimes let us do
                 * this for larger i values.
                 */
#ifdef Honor_FLT_ROUNDS
                /* round correctly FLT_ROUNDS = 2 or 3 */
                if (sign) {
                    dval(rv) = -dval(rv);
                    sign = 0;
                    }
#endif
                e -= i;
                dval(rv) *= tens[i];
#ifdef VAX
                /* VAX exponent range is so narrow we must
                 * worry about overflow here...
                 */
 vax_ovfl_check:
                word0(rv) -= P*Exp_msk1;
                /* rv = */ rounded_product(dval(rv), tens[e]);
                if ((word0(rv) & Exp_mask)
                 > Exp_msk1*(DBL_MAX_EXP+Bias-1-P))
                    goto ovfl;
                word0(rv) += P*Exp_msk1;
#else
                /* rv = */ rounded_product(dval(rv), tens[e]);
#endif
                goto ret;
                }
            }
#ifndef Inaccurate_Divide
        else if (e >= -Ten_pmax) {
#ifdef Honor_FLT_ROUNDS
            /* round correctly FLT_ROUNDS = 2 or 3 */
            if (sign) {
                rv = -rv;
                sign = 0;
                }
#endif
            /* rv = */ rounded_quotient(dval(rv), tens[-e]);
            goto ret;
            }
#endif
        }
    e1 += nd - k;

#ifdef IEEE_Arith
#ifdef SET_INEXACT
    inexact = 1;
    if (k <= DBL_DIG)
        oldinexact = get_inexact();
#endif
#ifdef Avoid_Underflow
    scale = 0;
#endif
#ifdef Honor_FLT_ROUNDS
    if (Rounding >= 2) {
        if (sign)
            Rounding = Rounding == 2 ? 0 : 2;
        else
            if (Rounding != 2)
                Rounding = 0;
        }
#endif
#endif /*IEEE_Arith*/

    /* Get starting approximation = rv * 10**e1 */

    if (e1 > 0) {
        if ((i = e1 & 15))
            dval(rv) *= tens[i];
        if ((e1 &= ~15)) {
            if (e1 > DBL_MAX_10_EXP) {
 ovfl:
#ifndef NO_ERRNO
                errno = ERANGE;
#endif
                /* Can't trust HUGE_VAL */
#ifdef IEEE_Arith
#ifdef Honor_FLT_ROUNDS
                switch(Rounding) {
                  case 0: /* toward 0 */
                  case 3: /* toward -infinity */
                    word0(rv) = Big0;
                    word1(rv) = Big1;
                    break;
                  default:
                    word0(rv) = Exp_mask;
                    word1(rv) = 0;
                  }
#else /*Honor_FLT_ROUNDS*/
                word0(rv) = Exp_mask;
                word1(rv) = 0;
#endif /*Honor_FLT_ROUNDS*/
#ifdef SET_INEXACT
                /* set overflow bit */
                dval(rv0) = 1e300;
                dval(rv0) *= dval(rv0);
#endif
#else /*IEEE_Arith*/
                word0(rv) = Big0;
                word1(rv) = Big1;
#endif /*IEEE_Arith*/
                if (bd0)
                    goto retfree;
                goto ret;
                }
            e1 >>= 4;
            for(j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    dval(rv) *= bigtens[j];
        /* The last multiplication could overflow. */
            word0(rv) -= P*Exp_msk1;
            dval(rv) *= bigtens[j];
            if ((z = word0(rv) & Exp_mask)
             > Exp_msk1*(DBL_MAX_EXP+Bias-P))
                goto ovfl;
            if (z > Exp_msk1*(DBL_MAX_EXP+Bias-1-P)) {
                /* set to largest number */
                /* (Can't trust DBL_MAX) */
                word0(rv) = Big0;
                word1(rv) = Big1;
                }
            else
                word0(rv) += P*Exp_msk1;
            }
        }
    else if (e1 < 0) {
        e1 = -e1;
        if ((i = e1 & 15))
            dval(rv) /= tens[i];
        if (e1 >>= 4) {
            if (e1 >= 1 << n_bigtens)
                goto undfl;
#ifdef Avoid_Underflow
            if (e1 & Scale_Bit)
                scale = 2*P;
            for(j = 0; e1 > 0; j++, e1 >>= 1)
                if (e1 & 1)
                    dval(rv) *= tinytens[j];
            if (scale && (j = 2*P + 1 - ((word0(rv) & Exp_mask)
                        >> Exp_shift)) > 0) {
                /* scaled rv is denormal; clear j low bits */
                if (j >= 32) {
                    word1(rv) = 0;
                    if (j >= 53)
                     word0(rv) = (P+2)*Exp_msk1;
                    else
                     word0(rv) &= 0xffffffff << j-32;
                    }
                else
                    word1(rv) &= 0xffffffff << j;
                }
#else
            for(j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    dval(rv) *= tinytens[j];
            /* The last multiplication could underflow. */
            dval(rv0) = dval(rv);
            dval(rv) *= tinytens[j];
            if (!dval(rv)) {
                dval(rv) = 2.*dval(rv0);
                dval(rv) *= tinytens[j];
#endif
                if (!dval(rv)) {
 undfl:
                    dval(rv) = 0.;
#ifndef NO_ERRNO
                    errno = ERANGE;
#endif
                    if (bd0)
                        goto retfree;
                    goto ret;
                    }
#ifndef Avoid_Underflow
                word0(rv) = Tiny0;
                word1(rv) = Tiny1;
                /* The refinement below will clean
                 * this approximation up.
                 */
                }
#endif
            }
        }

#if !defined(HARD)
    {
        TParserClosure parser;

        return parser.StrToDHard(copy00, se);
    }
#endif

    /* Now the hard part -- adjusting rv to the correct value.*/
    /* Put digits into bd: true value = bd * 10^e */
    bd0 = s2b(s0, nd0, nd, y);

    for(;;) {
        bd = Balloc(bd0->k);
        Bcopy(bd, bd0);
        bb = d2b(dval(rv), &bbe, &bbbits);    /* rv = bb * 2^bbe */
        bs = i2b(1);

        if (e >= 0) {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
            }
        else {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
            }
        if (bbe >= 0)
            bb2 += bbe;
        else
            bd2 -= bbe;
        bs2 = bb2;
#ifdef Honor_FLT_ROUNDS
        if (Rounding != 1)
            bs2++;
#endif
#ifdef Avoid_Underflow
        j = bbe - scale;
        i = j + bbbits - 1;    /* logb(rv) */
        if (i < Emin)    /* denormal */
            j += P - Emin;
        else
            j = P + 1 - bbbits;
#else /*Avoid_Underflow*/
#ifdef Sudden_Underflow
#ifdef IBM
        j = 1 + 4*P - 3 - bbbits + ((bbe + bbbits - 1) & 3);
#else
        j = P + 1 - bbbits;
#endif
#else /*Sudden_Underflow*/
        j = bbe;
        i = j + bbbits - 1;    /* logb(rv) */
        if (i < Emin)    /* denormal */
            j += P - Emin;
        else
            j = P + 1 - bbbits;
#endif /*Sudden_Underflow*/
#endif /*Avoid_Underflow*/
        bb2 += j;
        bd2 += j;
#ifdef Avoid_Underflow
        bd2 += scale;
#endif
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
            i = bs2;
        if (i > 0) {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
            }
        if (bb5 > 0) {
            bs = pow5mult(bs, bb5);
            bb1 = mult(bs, bb);
            Bfree(bb);
            bb = bb1;
            }
        if (bb2 > 0)
            bb = lshift(bb, bb2);
        if (bd5 > 0)
            bd = pow5mult(bd, bd5);
        if (bd2 > 0)
            bd = lshift(bd, bd2);
        if (bs2 > 0)
            bs = lshift(bs, bs2);
        delta = diff(bb, bd);
        dsign = delta->sign;
        delta->sign = 0;
        i = cmp(delta, bs);
#ifdef Honor_FLT_ROUNDS
        if (Rounding != 1) {
            if (i < 0) {
                /* Error is less than an ulp */
                if (!delta->x[0] && delta->wds <= 1) {
                    /* exact */
#ifdef SET_INEXACT
                    inexact = 0;
#endif
                    break;
                    }
                if (Rounding) {
                    if (dsign) {
                        adj = 1.;
                        goto apply_adj;
                        }
                    }
                else if (!dsign) {
                    adj = -1.;
                    if (!word1(rv)
                     && !(word0(rv) & Frac_mask)) {
                        y = word0(rv) & Exp_mask;
#ifdef Avoid_Underflow
                        if (!scale || y > 2*P*Exp_msk1)
#else
                        if (y)
#endif
                          {
                          delta = lshift(delta,Log2P);
                          if (cmp(delta, bs) <= 0)
                            adj = -0.5;
                          }
                        }
 apply_adj:
#ifdef Avoid_Underflow
                    if (scale && (y = word0(rv) & Exp_mask)
                        <= 2*P*Exp_msk1)
                      word0(adj) += (2*P+1)*Exp_msk1 - y;
#else
#ifdef Sudden_Underflow
                    if ((word0(rv) & Exp_mask) <=
                            P*Exp_msk1) {
                        word0(rv) += P*Exp_msk1;
                        dval(rv) += adj*ulp(dval(rv));
                        word0(rv) -= P*Exp_msk1;
                        }
                    else
#endif /*Sudden_Underflow*/
#endif /*Avoid_Underflow*/
                    dval(rv) += adj*ulp(dval(rv));
                    }
                break;
                }
            adj = ratio(delta, bs);
            if (adj < 1.)
                adj = 1.;
            if (adj <= 0x7ffffffe) {
                /* adj = rounding ? ceil(adj) : floor(adj); */
                y = adj;
                if (y != adj) {
                    if (!((Rounding>>1) ^ dsign))
                        y++;
                    adj = y;
                    }
                }
#ifdef Avoid_Underflow
            if (scale && (y = word0(rv) & Exp_mask) <= 2*P*Exp_msk1)
                word0(adj) += (2*P+1)*Exp_msk1 - y;
#else
#ifdef Sudden_Underflow
            if ((word0(rv) & Exp_mask) <= P*Exp_msk1) {
                word0(rv) += P*Exp_msk1;
                adj *= ulp(dval(rv));
                if (dsign)
                    dval(rv) += adj;
                else
                    dval(rv) -= adj;
                word0(rv) -= P*Exp_msk1;
                goto cont;
                }
#endif /*Sudden_Underflow*/
#endif /*Avoid_Underflow*/
            adj *= ulp(dval(rv));
            if (dsign) {
                if (word0(rv) == Big0 && word1(rv) == Big1)
                    goto ovfl;
                dval(rv) += adj;
                }
            else
                dval(rv) -= adj;
            goto cont;
            }
#endif /*Honor_FLT_ROUNDS*/

        if (i < 0) {
            /* Error is less than half an ulp -- check for
             * special case of mantissa a power of two.
             */
            if (dsign || word1(rv) || word0(rv) & Bndry_mask
#ifdef IEEE_Arith
#ifdef Avoid_Underflow
             || (word0(rv) & Exp_mask) <= (2*P+1)*Exp_msk1
#else
             || (word0(rv) & Exp_mask) <= Exp_msk1
#endif
#endif
                ) {
#ifdef SET_INEXACT
                if (!delta->x[0] && delta->wds <= 1)
                    inexact = 0;
#endif
                break;
                }
            if (!delta->x[0] && delta->wds <= 1) {
                /* exact result */
#ifdef SET_INEXACT
                inexact = 0;
#endif
                break;
                }
            delta = lshift(delta,Log2P);
            if (cmp(delta, bs) > 0)
                goto drop_down;
            break;
            }
        if (i == 0) {
            /* exactly half-way between */
            if (dsign) {
                if ((word0(rv) & Bndry_mask1) == Bndry_mask1
                 &&  word1(rv) == (
#ifdef Avoid_Underflow
            (scale && (y = word0(rv) & Exp_mask) <= 2*P*Exp_msk1)
        ? (0xffffffff & (0xffffffff << (2*P+1-(y>>Exp_shift)))) :
#endif
                           0xffffffff)) {
                    /*boundary case -- increment exponent*/
                    word0(rv) = (word0(rv) & Exp_mask)
                        + Exp_msk1
#ifdef IBM
                        | Exp_msk1 >> 4
#endif
                        ;
                    word1(rv) = 0;
#ifdef Avoid_Underflow
                    dsign = 0;
#endif
                    break;
                    }
                }
            else if (!(word0(rv) & Bndry_mask) && !word1(rv)) {
 drop_down:
                /* boundary case -- decrement exponent */
#ifdef Sudden_Underflow /*{{*/
                L = word0(rv) & Exp_mask;
#ifdef IBM
                if (L <  Exp_msk1)
#else
#ifdef Avoid_Underflow
                if (L <= (scale ? (2*P+1)*Exp_msk1 : Exp_msk1))
#else
                if (L <= Exp_msk1)
#endif /*Avoid_Underflow*/
#endif /*IBM*/
                    goto undfl;
                L -= Exp_msk1;
#else /*Sudden_Underflow}{*/
#ifdef Avoid_Underflow
                if (scale) {
                    L = word0(rv) & Exp_mask;
                    if (L <= (2*P+1)*Exp_msk1) {
                        if (L > (P+2)*Exp_msk1)
                            /* round even ==> */
                            /* accept rv */
                            break;
                        /* rv = smallest denormal */
                        goto undfl;
                        }
                    }
#endif /*Avoid_Underflow*/
                L = (word0(rv) & Exp_mask) - Exp_msk1;
#endif /*Sudden_Underflow}}*/
                word0(rv) = L | Bndry_mask1;
                word1(rv) = 0xffffffff;
#ifdef IBM
                goto cont;
#else
                break;
#endif
                }
#ifndef ROUND_BIASED
            if (!(word1(rv) & LSB))
                break;
#endif
            if (dsign)
                dval(rv) += ulp(dval(rv));
#ifndef ROUND_BIASED
            else {
                dval(rv) -= ulp(dval(rv));
#ifndef Sudden_Underflow
                if (!dval(rv))
                    goto undfl;
#endif
                }
#ifdef Avoid_Underflow
            dsign = 1 - dsign;
#endif
#endif
            break;
            }
        if ((aadj = ratio(delta, bs)) <= 2.) {
            if (dsign)
                aadj = dval(aadj1) = 1.;
            else if (word1(rv) || word0(rv) & Bndry_mask) {
#ifndef Sudden_Underflow
                if (word1(rv) == Tiny1 && !word0(rv))
                    goto undfl;
#endif
                aadj = 1.;
                dval(aadj1) = -1.;
                }
            else {
                /* special case -- power of FLT_RADIX to be */
                /* rounded down... */

                if (aadj < 2./FLT_RADIX)
                    aadj = 1./FLT_RADIX;
                else
                    aadj *= 0.5;
                dval(aadj1) = -aadj;
                }
            }
        else {
            aadj *= 0.5;
            dval(aadj1) = dsign ? aadj : -aadj;
#ifdef Check_FLT_ROUNDS
            switch(Rounding) {
                case 2: /* towards +infinity */
                    dval(aadj1) -= 0.5;
                    break;
                case 0: /* towards 0 */
                case 3: /* towards -infinity */
                    dval(aadj1) += 0.5;
                }
#else
            if (Flt_Rounds == 0)
                dval(aadj1) += 0.5;
#endif /*Check_FLT_ROUNDS*/
            }
        y = word0(rv) & Exp_mask;

        /* Check for overflow */

        if (y == Exp_msk1*(DBL_MAX_EXP+Bias-1)) {
            dval(rv0) = dval(rv);
            word0(rv) -= P*Exp_msk1;
            adj = dval(aadj1) * ulp(dval(rv));
            dval(rv) += adj;
            if ((word0(rv) & Exp_mask) >=
                    Exp_msk1*(DBL_MAX_EXP+Bias-P)) {
                if (word0(rv0) == Big0 && word1(rv0) == Big1)
                    goto ovfl;
                word0(rv) = Big0;
                word1(rv) = Big1;
                goto cont;
                }
            else
                word0(rv) += P*Exp_msk1;
            }
        else {
#ifdef Avoid_Underflow
            if (scale && y <= 2*P*Exp_msk1) {
                if (aadj <= 0x7fffffff) {
                    if ((z = (ULong)aadj) <= 0)
                        z = 1;
                    aadj = z;
                    dval(aadj1) = dsign ? aadj : -aadj;
                    }
                word0(aadj1) += (2*P+1)*Exp_msk1 - y;
                }
            adj = dval(aadj1) * ulp(dval(rv));
            dval(rv) += adj;
#else
#ifdef Sudden_Underflow
            if ((word0(rv) & Exp_mask) <= P*Exp_msk1) {
                dval(rv0) = dval(rv);
                word0(rv) += P*Exp_msk1;
                adj = dval(aadj1) * ulp(dval(rv));
                dval(rv) += adj;
#ifdef IBM
                if ((word0(rv) & Exp_mask) <  P*Exp_msk1)
#else
                if ((word0(rv) & Exp_mask) <= P*Exp_msk1)
#endif
                    {
                    if (word0(rv0) == Tiny0
                     && word1(rv0) == Tiny1)
                        goto undfl;
                    word0(rv) = Tiny0;
                    word1(rv) = Tiny1;
                    goto cont;
                    }
                else
                    word0(rv) -= P*Exp_msk1;
                }
            else {
                adj = dval(aadj1) * ulp(dval(rv));
                dval(rv) += adj;
                }
#else /*Sudden_Underflow*/
            /* Compute adj so that the IEEE rounding rules will
             * correctly round rv + adj in some half-way cases.
             * If rv * ulp(rv) is denormalized (i.e.,
             * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
             * trouble from bits lost to denormalization;
             * example: 1.2e-307 .
             */
            if (y <= (P-1)*Exp_msk1 && aadj > 1.) {
                dval(aadj1) = (double)(int)(aadj + 0.5);
                if (!dsign)
                    dval(aadj1) = -dval(aadj1);
                }
            adj = dval(aadj1) * ulp(dval(rv));
            dval(rv) += adj;
#endif /*Sudden_Underflow*/
#endif /*Avoid_Underflow*/
            }
        z = word0(rv) & Exp_mask;
#ifndef SET_INEXACT
#ifdef Avoid_Underflow
        if (!scale)
#endif
        if (y == z) {
            /* Can we stop now? */
            L = (Long)aadj;
            aadj -= L;
            /* The tolerances below are conservative. */
            if (dsign || word1(rv) || word0(rv) & Bndry_mask) {
                if (aadj < .4999999 || aadj > .5000001)
                    break;
                }
            else if (aadj < .4999999/FLT_RADIX)
                break;
            }
#endif
 cont:
        Bfree(bb);
        Bfree(bd);
        Bfree(bs);
        Bfree(delta);
        }
#ifdef SET_INEXACT
    if (inexact) {
        if (!oldinexact) {
            word0(rv0) = Exp_1 + (70 << Exp_shift);
            word1(rv0) = 0;
            dval(rv0) += 1.;
            }
        }
    else if (!oldinexact)
        clear_inexact();
#endif
#ifdef Avoid_Underflow
    if (scale) {
        word0(rv0) = Exp_1 - 2*P*Exp_msk1;
        word1(rv0) = 0;
        dval(rv) *= dval(rv0);
#ifndef NO_ERRNO
        /* try to avoid the bug of testing an 8087 register value */
        if (word0(rv) == 0 && word1(rv) == 0)
            errno = ERANGE;
#endif
        }
#endif /* Avoid_Underflow */
#ifdef SET_INEXACT
    if (inexact && !(word0(rv) & Exp_mask)) {
        /* set underflow bit */
        dval(rv0) = 1e-300;
        dval(rv0) *= dval(rv0);
        }
#endif
 retfree:
    Bfree(bb);
    Bfree(bd);
    Bfree(bs);
    Bfree(bd0);
    Bfree(delta);
 ret:
    if (se)
        *se = (char *)s.C_;
    return sign ? -dval(rv) : dval(rv);
    }

inline int quorem(Bigint *b, Bigint *S)
{
    int n;
    ULong *bx, *bxe, q, *sx, *sxe;
#ifdef ULLong
    ULLong borrow, carry, y, ys;
#else
    ULong borrow, carry, y, ys;
#ifdef Pack_32
    ULong si, z, zs;
#endif
#endif

    n = S->wds;
#ifdef DEBUG
    /*debug*/ if (b->wds > n)
    /*debug*/    Bug("oversize b in quorem");
#endif
    if (b->wds < n)
        return 0;
    sx = S->x;
    sxe = sx + --n;
    bx = b->x;
    bxe = bx + n;
    q = *bxe / (*sxe + 1);    /* ensure q <= true quotient */
#ifdef DEBUG
    /*debug*/ if (q > 9)
    /*debug*/    Bug("oversized quotient in quorem");
#endif
    if (q) {
        borrow = 0;
        carry = 0;
        do {
#ifdef ULLong
            ys = *sx++ * (ULLong)q + carry;
            carry = ys >> 32;
            y = *bx - (ys & FFFFFFFF) - borrow;
            borrow = y >> 32 & (ULong)1;
            *bx++ = y & FFFFFFFF;
#else
#ifdef Pack_32
            si = *sx++;
            ys = (si & 0xffff) * q + carry;
            zs = (si >> 16) * q + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) - borrow;
            borrow = (y & 0x10000) >> 16;
            z = (*bx >> 16) - (zs & 0xffff) - borrow;
            borrow = (z & 0x10000) >> 16;
            Storeinc(bx, z, y);
#else
            ys = *sx++ * q + carry;
            carry = ys >> 16;
            y = *bx - (ys & 0xffff) - borrow;
            borrow = (y & 0x10000) >> 16;
            *bx++ = y & 0xffff;
#endif
#endif
            }
            while(sx <= sxe);
        if (!*bxe) {
            bx = b->x;
            while(--bxe > bx && !*bxe)
                --n;
            b->wds = n;
            }
        }
    if (cmp(b, S) >= 0) {
        q++;
        borrow = 0;
        carry = 0;
        bx = b->x;
        sx = S->x;
        do {
#ifdef ULLong
            ys = *sx++ + carry;
            carry = ys >> 32;
            y = *bx - (ys & FFFFFFFF) - borrow;
            borrow = y >> 32 & (ULong)1;
            *bx++ = y & FFFFFFFF;
#else
#ifdef Pack_32
            si = *sx++;
            ys = (si & 0xffff) + carry;
            zs = (si >> 16) + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) - borrow;
            borrow = (y & 0x10000) >> 16;
            z = (*bx >> 16) - (zs & 0xffff) - borrow;
            borrow = (z & 0x10000) >> 16;
            Storeinc(bx, z, y);
#else
            ys = *sx++ + carry;
            carry = ys >> 16;
            y = *bx - (ys & 0xffff) - borrow;
            borrow = (y & 0x10000) >> 16;
            *bx++ = y & 0xffff;
#endif
#endif
            }
            while(sx <= sxe);
        bx = b->x;
        bxe = bx + n;
        if (!*bxe) {
            while(--bxe > bx && !*bxe)
                --n;
            b->wds = n;
            }
        }
    return q;
    }

#ifndef MULTIPLE_THREADS
 static char *dtoa_result;
#endif

char* rv_alloc(int i)
{
    int j, k, *r;

    j = sizeof(ULong);
    for(k = 0;
        (int)sizeof(Bigint) - (int)sizeof(ULong) - (int)sizeof(int) + j <= i;
        j <<= 1)
            k++;
    r = (int*)Balloc(k);
    *r = k;
    return
#ifndef MULTIPLE_THREADS
    dtoa_result =
#endif
        (char *)(r+1);
    }

char* nrv_alloc(const char *s, char **rve, int n)
{
    char *rv, *t;

    t = rv = rv_alloc(n);
    while((*t = *s++)) t++;
    if (rve)
        *rve = t;
    return rv;
    }

/* freedtoa(s) must be used to free values s returned by dtoa
 * when MULTIPLE_THREADS is #defined.  It should be used in all cases,
 * but for consistency with earlier versions of dtoa, it is optional
 * when MULTIPLE_THREADS is not defined.
 */

void freedtoa(char *s)
{
    Bigint *b = (Bigint *)((int *)s - 1);
    b->maxwds = 1 << (b->k = *(int*)b);
    Bfree(b);
#ifndef MULTIPLE_THREADS
    if (s == dtoa_result)
        dtoa_result = 0;
#endif
    }

/* dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
 *
 * Inspired by "How to Print Floating-Point Numbers Accurately" by
 * Guy L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90, pp. 112-126].
 *
 * Modifications:
 *    1. Rather than iterating, we use a simple numeric overestimate
 *       to determine k = floor(log10(d)).  We scale relevant
 *       quantities using O(log2(k)) rather than O(k) multiplications.
 *    2. For some modes > 2 (corresponding to ecvt and fcvt), we don't
 *       try to generate digits strictly left to right.  Instead, we
 *       compute with fewer bits and propagate the carry if necessary
 *       when rounding the final digit up.  This is often faster.
 *    3. Under the assumption that input will be rounded nearest,
 *       mode 0 renders 1e23 as 1e23 rather than 9.999999999999999e22.
 *       That is, we allow equality in stopping tests when the
 *       round-nearest rule will give the same floating-point value
 *       as would satisfaction of the stopping test with strict
 *       inequality.
 *    4. We remove common factors of powers of 2 from relevant
 *       quantities.
 *    5. When converting floating-point integers less than 1e16,
 *       we use floating-point arithmetic rather than resorting
 *       to multiple-precision integers.
 *    6. When asked to produce fewer than 15 digits, we first try
 *       to get by with floating-point arithmetic; we resort to
 *       multiple-precision integer arithmetic only if we cannot
 *       guarantee that the floating-point calculation has given
 *       the correctly rounded result.  For k requested digits and
 *       "uniformly" distributed input, the probability is
 *       something like 10^(k-15) that we must resort to the Long
 *       calculation.
 */

STATIC inline char* dtoa(double dd, int mode, int ndigits, int *decpt, int *sign, char **rve)
{
 /*    Arguments ndigits, decpt, sign are similar to those
    of ecvt and fcvt; trailing zeros are suppressed from
    the returned string.  If not null, *rve is set to point
    to the end of the return value.  If d is +-Infinity or NaN,
    then *decpt is set to 9999.

    mode:
        0 ==> shortest string that yields d when read in
            and rounded to nearest.
        1 ==> like 0, but with Steele & White stopping rule;
            e.g. with IEEE P754 arithmetic , mode 0 gives
            1e23 whereas mode 1 gives 9.999999999999999e22.
        2 ==> max(1,ndigits) significant digits.  This gives a
            return value similar to that of ecvt, except
            that trailing zeros are suppressed.
        3 ==> through ndigits past the decimal point.  This
            gives a return value similar to that from fcvt,
            except that trailing zeros are suppressed, and
            ndigits can be negative.
        4,5 ==> similar to 2 and 3, respectively, but (in
            round-nearest mode) with the tests of mode 0 to
            possibly return a shorter string that rounds to d.
            With IEEE arithmetic and compilation with
            -DHonor_FLT_ROUNDS, modes 4 and 5 behave the same
            as modes 2 and 3 when FLT_ROUNDS != 1.
        6-9 ==> Debugging modes similar to mode - 4:  don't try
            fast floating-point estimate (if applicable).

        Values of mode other than 0-9 are treated as mode 0.

        Sufficient space is allocated to the return value
        to hold the suppressed trailing zeros.
    */
    U d, d2, eps;
    int bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1,
        j, j1, k, k0, k_check, leftright, m2, m5, s2, s5,
        spec_case, try_quick;
    ilim = 0;
    ilim1 = 0;
    Long L;
#ifndef Sudden_Underflow
    int denorm;
    ULong x;
#endif
    Bigint *b, *b1, *delta, *mlo, *mhi, *S;
    double ds;
    char *s, *s0;
#ifdef SET_INEXACT
    int inexact, oldinexact;
#endif
#ifdef Honor_FLT_ROUNDS /*{*/
    int Rounding;
#ifdef Trust_FLT_ROUNDS /*{{ only define this if FLT_ROUNDS really works! */
    Rounding = Flt_Rounds;
#else /*}{*/
    Rounding = 1;
    switch(fegetround()) {
      case FE_TOWARDZERO:    Rounding = 0; break;
      case FE_UPWARD:    Rounding = 2; break;
      case FE_DOWNWARD:    Rounding = 3;
      }
#endif /*}}*/
#endif /*}*/
    dval(d) = dd;
#ifndef MULTIPLE_THREADS
    if (dtoa_result) {
        freedtoa(dtoa_result);
        dtoa_result = 0;
        }
#endif

    if (word0(d) & Sign_bit) {
        /* set sign for everything, including 0's and NaNs */
        *sign = 1;
        word0(d) &= ~Sign_bit;    /* clear sign bit */
        }
    else
        *sign = 0;

#if defined(IEEE_Arith) + defined(VAX)
#ifdef IEEE_Arith
    if ((word0(d) & Exp_mask) == Exp_mask)
#else
    if (word0(d)  == 0x8000)
#endif
        {
        /* Infinity or NaN */
        *decpt = 9999;
#ifdef IEEE_Arith
        if (!word1(d) && !(word0(d) & 0xfffff))
            return nrv_alloc("inf", rve, 8);
#endif
        return nrv_alloc("nan", rve, 3);
        }
#endif
#ifdef IBM
    dval(d) += 0; /* normalize */
#endif
    if (!dval(d)) {
        *decpt = 1;
        return nrv_alloc("0", rve, 1);
        }

#ifdef SET_INEXACT
    try_quick = oldinexact = get_inexact();
    inexact = 1;
#endif
#ifdef Honor_FLT_ROUNDS
    if (Rounding >= 2) {
        if (*sign)
            Rounding = Rounding == 2 ? 0 : 2;
        else
            if (Rounding != 2)
                Rounding = 0;
        }
#endif

    b = d2b(dval(d), &be, &bbits);
#ifdef Sudden_Underflow
    i = (int)(word0(d) >> Exp_shift1 & (Exp_mask>>Exp_shift1));
#else
    if ((i = (int)(word0(d) >> Exp_shift1 & (Exp_mask>>Exp_shift1)))) {
#endif
        dval(d2) = dval(d);
        word0(d2) &= Frac_mask1;
        word0(d2) |= Exp_11;
#ifdef IBM
        if (j = 11 - hi0bits(word0(d2) & Frac_mask))
            dval(d2) /= 1 << j;
#endif

        /* log(x)    ~=~ log(1.5) + (x-1.5)/1.5
         * log10(x)     =  log(x) / log(10)
         *        ~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
         * log10(d) = (i-Bias)*log(2)/log(10) + log10(d2)
         *
         * This suggests computing an approximation k to log10(d) by
         *
         * k = (i - Bias)*0.301029995663981
         *    + ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
         *
         * We want k to be too large rather than too small.
         * The error in the first-order Taylor series approximation
         * is in our favor, so we just round up the constant enough
         * to compensate for any error in the multiplication of
         * (i - Bias) by 0.301029995663981; since |i - Bias| <= 1077,
         * and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
         * adding 1e-13 to the constant term more than suffices.
         * Hence we adjust the constant term to 0.1760912590558.
         * (We could get a more accurate k by invoking log10,
         *  but this is probably not worthwhile.)
         */

        i -= Bias;
#ifdef IBM
        i <<= 2;
        i += j;
#endif
#ifndef Sudden_Underflow
        denorm = 0;
        }
    else {
        /* d is denormalized */

        i = bbits + be + (Bias + (P-1) - 1);
        x = i > 32  ? word0(d) << 64 - i | word1(d) >> i - 32
                : word1(d) << 32 - i;
        dval(d2) = x;
        word0(d2) -= 31*Exp_msk1; /* adjust exponent */
        i -= (Bias + (P-1) - 1) + 1;
        denorm = 1;
        }
#endif
    ds = (dval(d2)-1.5)*0.289529654602168 + 0.1760912590558 + i*0.301029995663981;
    k = (int)ds;
    if (ds < 0. && ds != k)
        k--;    /* want k = floor(ds) */
    k_check = 1;
    if (k >= 0 && k <= Ten_pmax) {
        if (dval(d) < tens[k])
            k--;
        k_check = 0;
        }
    j = bbits - i - 1;
    if (j >= 0) {
        b2 = 0;
        s2 = j;
        }
    else {
        b2 = -j;
        s2 = 0;
        }
    if (k >= 0) {
        b5 = 0;
        s5 = k;
        s2 += k;
        }
    else {
        b2 -= k;
        b5 = -k;
        s5 = 0;
        }
    if (mode < 0 || mode > 9)
        mode = 0;

#ifndef SET_INEXACT
#ifdef Check_FLT_ROUNDS
    try_quick = Rounding == 1;
#else
    try_quick = 1;
#endif
#endif /*SET_INEXACT*/

    if (mode > 5) {
        mode -= 4;
        try_quick = 0;
        }
    leftright = 1;
    switch(mode) {
        case 0:
        case 1:
            ilim = ilim1 = -1;
            i = 18;
            ndigits = 0;
            break;
        case 2:
            leftright = 0;
            /* no break */
        case 4:
            if (ndigits <= 0)
                ndigits = 1;
            ilim = ilim1 = i = ndigits;
            break;
        case 3:
            leftright = 0;
            /* no break */
        case 5:
            i = ndigits + k + 1;
            ilim = i;
            ilim1 = i - 1;
            if (i <= 0)
                i = 1;
        }
    s = s0 = rv_alloc(i);

#ifdef Honor_FLT_ROUNDS
    if (mode > 1 && Rounding != 1)
        leftright = 0;
#endif

    if (ilim >= 0 && ilim <= Quick_max && try_quick) {

        /* Try to get by with floating-point arithmetic. */

        i = 0;
        dval(d2) = dval(d);
        k0 = k;
        ilim0 = ilim;
        ieps = 2; /* conservative */
        if (k > 0) {
            ds = tens[k&0xf];
            j = k >> 4;
            if (j & Bletch) {
                /* prevent overflows */
                j &= Bletch - 1;
                dval(d) /= bigtens[n_bigtens-1];
                ieps++;
                }
            for(; j; j >>= 1, i++)
                if (j & 1) {
                    ieps++;
                    ds *= bigtens[i];
                    }
            dval(d) /= ds;
            }
        else if ((j1 = -k)) {
            dval(d) *= tens[j1 & 0xf];
            for(j = j1 >> 4; j; j >>= 1, i++)
                if (j & 1) {
                    ieps++;
                    dval(d) *= bigtens[i];
                    }
            }
        if (k_check && dval(d) < 1. && ilim > 0) {
            if (ilim1 <= 0)
                goto fast_failed;
            ilim = ilim1;
            k--;
            dval(d) *= 10.;
            ieps++;
            }
        dval(eps) = ieps*dval(d) + 7.;
        word0(eps) -= (P-1)*Exp_msk1;
        if (ilim == 0) {
            S = mhi = 0;
            dval(d) -= 5.;
            if (dval(d) > dval(eps))
                goto one_digit;
            if (dval(d) < -dval(eps))
                goto no_digits;
            goto fast_failed;
            }
#ifndef No_leftright
        if (leftright) {
            /* Use Steele & White method of only
             * generating digits needed.
             */
            dval(eps) = 0.5/tens[ilim-1] - dval(eps);
            for(i = 0;;) {
                L = (Long)dval(d);
                dval(d) -= L;
                *s++ = '0' + (int)L;
                if (dval(d) < dval(eps))
                    goto ret1;
                if (1. - dval(d) < dval(eps))
                    goto bump_up;
                if (++i >= ilim)
                    break;
                dval(eps) *= 10.;
                dval(d) *= 10.;
                }
            }
        else {
#endif
            /* Generate ilim digits, then fix them up. */
            dval(eps) *= tens[ilim-1];
            for(i = 1;; i++, dval(d) *= 10.) {
                L = (Long)(dval(d));
                if (!(dval(d) -= L))
                    ilim = i;
                *s++ = '0' + (int)L;
                if (i == ilim) {
                    if (dval(d) > 0.5 + dval(eps))
                        goto bump_up;
                    else if (dval(d) < 0.5 - dval(eps)) {
                        while(*--s == '0') {
                        }
                        s++;
                        goto ret1;
                        }
                    break;
                    }
                }
#ifndef No_leftright
            }
#endif
 fast_failed:
        s = s0;
        dval(d) = dval(d2);
        k = k0;
        ilim = ilim0;
        }

    /* Do we have a "small" integer? */

    if (be >= 0 && k <= Int_max) {
        /* Yes. */
        ds = tens[k];
        if (ndigits < 0 && ilim <= 0) {
            S = mhi = 0;
            if (ilim < 0 || dval(d) <= 5*ds)
                goto no_digits;
            goto one_digit;
            }
        for(i = 1;; i++, dval(d) *= 10.) {
            L = (Long)(dval(d) / ds);
            dval(d) -= L*ds;
#ifdef Check_FLT_ROUNDS
            /* If FLT_ROUNDS == 2, L will usually be high by 1 */
            if (dval(d) < 0) {
                L--;
                dval(d) += ds;
                }
#endif
            *s++ = '0' + (int)L;
            if (!dval(d)) {
#ifdef SET_INEXACT
                inexact = 0;
#endif
                break;
                }
            if (i == ilim) {
#ifdef Honor_FLT_ROUNDS
                if (mode > 1)
                switch(Rounding) {
                  case 0: goto ret1;
                  case 2: goto bump_up;
                  }
#endif
                dval(d) += dval(d);
                if (dval(d) > ds || dval(d) == ds && L & 1) {
 bump_up:
                    while(*--s == '9')
                        if (s == s0) {
                            k++;
                            *s = '0';
                            break;
                            }
                    ++*s++;
                    }
                break;
                }
            }
        goto ret1;
        }

    m2 = b2;
    m5 = b5;
    mhi = mlo = 0;
    if (leftright) {
        i =
#ifndef Sudden_Underflow
            denorm ? be + (Bias + (P-1) - 1 + 1) :
#endif
#ifdef IBM
            1 + 4*P - 3 - bbits + ((bbits + be - 1) & 3);
#else
            1 + P - bbits;
#endif
        b2 += i;
        s2 += i;
        mhi = i2b(1);
        }
    if (m2 > 0 && s2 > 0) {
        i = m2 < s2 ? m2 : s2;
        b2 -= i;
        m2 -= i;
        s2 -= i;
        }
    if (b5 > 0) {
        if (leftright) {
            if (m5 > 0) {
                mhi = pow5mult(mhi, m5);
                b1 = mult(mhi, b);
                Bfree(b);
                b = b1;
                }
            if ((j = b5 - m5))
                b = pow5mult(b, j);
            }
        else
            b = pow5mult(b, b5);
        }
    S = i2b(1);
    if (s5 > 0)
        S = pow5mult(S, s5);

    /* Check for special case that d is a normalized power of 2. */

    spec_case = 0;
    if ((mode < 2 || leftright)
#ifdef Honor_FLT_ROUNDS
            && Rounding == 1
#endif
                ) {
        if (!word1(d) && !(word0(d) & Bndry_mask)
#ifndef Sudden_Underflow
         && word0(d) & (Exp_mask & ~Exp_msk1)
#endif
                ) {
            /* The special case */
            b2 += Log2P;
            s2 += Log2P;
            spec_case = 1;
            }
        }

    /* Arrange for convenient computation of quotients:
     * shift left if necessary so divisor has 4 leading 0 bits.
     *
     * Perhaps we should just compute leading 28 bits of S once
     * and for all and pass them and a shift to quorem, so it
     * can do shifts and ors to compute the numerator for q.
     */
#ifdef Pack_32
    if ((i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0x1f))
        i = 32 - i;
#else
    if (i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0xf)
        i = 16 - i;
#endif
    if (i > 4) {
        i -= 4;
        b2 += i;
        m2 += i;
        s2 += i;
        }
    else if (i < 4) {
        i += 28;
        b2 += i;
        m2 += i;
        s2 += i;
        }
    if (b2 > 0)
        b = lshift(b, b2);
    if (s2 > 0)
        S = lshift(S, s2);
    if (k_check) {
        if (cmp(b,S) < 0) {
            k--;
            b = multadd(b, 10, 0);    /* we botched the k estimate */
            if (leftright)
                mhi = multadd(mhi, 10, 0);
            ilim = ilim1;
            }
        }
    if (ilim <= 0 && (mode == 3 || mode == 5)) {
        if (ilim < 0 || cmp(b,S = multadd(S,5,0)) <= 0) {
            /* no digits, fcvt style */
 no_digits:
            k = -1 - ndigits;
            goto ret;
            }
 one_digit:
        *s++ = '1';
        k++;
        goto ret;
        }
    if (leftright) {
        if (m2 > 0)
            mhi = lshift(mhi, m2);

        /* Compute mlo -- check for special case
         * that d is a normalized power of 2.
         */

        mlo = mhi;
        if (spec_case) {
            mhi = Balloc(mhi->k);
            Bcopy(mhi, mlo);
            mhi = lshift(mhi, Log2P);
            }

        for(i = 1;;i++) {
            dig = quorem(b,S) + '0';
            /* Do we yet have the shortest decimal string
             * that will round to d?
             */
            j = cmp(b, mlo);
            delta = diff(S, mhi);
            j1 = delta->sign ? 1 : cmp(b, delta);
            Bfree(delta);
#ifndef ROUND_BIASED
            if (j1 == 0 && mode != 1 && !(word1(d) & 1)
#ifdef Honor_FLT_ROUNDS
                && Rounding >= 1
#endif
                                   ) {
                if (dig == '9')
                    goto round_9_up;
                if (j > 0)
                    dig++;
#ifdef SET_INEXACT
                else if (!b->x[0] && b->wds <= 1)
                    inexact = 0;
#endif
                *s++ = dig;
                goto ret;
                }
#endif
            if (j < 0 || j == 0 && mode != 1
#ifndef ROUND_BIASED
                            && !(word1(d) & 1)
#endif
                    ) {
                if (!b->x[0] && b->wds <= 1) {
#ifdef SET_INEXACT
                    inexact = 0;
#endif
                    goto accept_dig;
                    }
#ifdef Honor_FLT_ROUNDS
                if (mode > 1)
                 switch(Rounding) {
                  case 0: goto accept_dig;
                  case 2: goto keep_dig;
                  }
#endif /*Honor_FLT_ROUNDS*/
                if (j1 > 0) {
                    b = lshift(b, 1);
                    j1 = cmp(b, S);
                    if ((j1 > 0 || j1 == 0 && dig & 1)
                    && dig++ == '9')
                        goto round_9_up;
                    }
 accept_dig:
                *s++ = dig;
                goto ret;
                }
            if (j1 > 0) {
#ifdef Honor_FLT_ROUNDS
                if (!Rounding)
                    goto accept_dig;
#endif
                if (dig == '9') { /* possible if i == 1 */
 round_9_up:
                    *s++ = '9';
                    goto roundoff;
                    }
                *s++ = dig + 1;
                goto ret;
                }
#ifdef Honor_FLT_ROUNDS
 keep_dig:
#endif
            *s++ = dig;
            if (i == ilim)
                break;
            b = multadd(b, 10, 0);
            if (mlo == mhi)
                mlo = mhi = multadd(mhi, 10, 0);
            else {
                mlo = multadd(mlo, 10, 0);
                mhi = multadd(mhi, 10, 0);
                }
            }
        }
    else
        for(i = 1;; i++) {
            *s++ = dig = quorem(b,S) + '0';
            if (!b->x[0] && b->wds <= 1) {
#ifdef SET_INEXACT
                inexact = 0;
#endif
                goto ret;
                }
            if (i >= ilim)
                break;
            b = multadd(b, 10, 0);
            }

    /* Round off last digit */

#ifdef Honor_FLT_ROUNDS
    switch(Rounding) {
      case 0: goto trimzeros;
      case 2: goto roundoff;
      }
#endif
    b = lshift(b, 1);
    j = cmp(b, S);
    if (j > 0 || j == 0 && dig & 1) {
 roundoff:
        while(*--s == '9')
            if (s == s0) {
                k++;
                *s++ = '1';
                goto ret;
                }
        ++*s++;
        }
    else {
#ifdef Honor_FLT_ROUNDS
 trimzeros:
#endif
        while(*--s == '0') {
        }
        s++;
        }
 ret:
    Bfree(S);
    if (mhi) {
        if (mlo && mlo != mhi)
            Bfree(mlo);
        Bfree(mhi);
        }
 ret1:
#ifdef SET_INEXACT
    if (inexact) {
        if (!oldinexact) {
            word0(d) = Exp_1 + (70 << Exp_shift);
            word1(d) = 0;
            dval(d) += 1.;
            }
        }
    else if (!oldinexact)
        clear_inexact();
#endif
    Bfree(b);
    *s = 0;
    *decpt = k + 1;
    if (rve)
        *rve = s;
    return s0;
    }
#if defined(HARD)
};
#endif
#undef STATIC
