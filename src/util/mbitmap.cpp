#include "fput.h"
#include "fgood.h"
#include "mbitmap.h"
#include "autoarray.h"

#include <cstdio>
#include <cerrno>

void bitmap_2::LoadDeletedDocuments(FILE *fportion, const ui8 val) {
    char buf[20];
    int i = 0;

    while (fgets(buf, sizeof(buf), fportion)) {
        i++;
        char *end;
        long handle = strtol(buf, &end, 0);
        if (handle < 0 || handle == LONG_MAX || (*end != '\n' && *end != '\r'))
            ythrow yexception() << "bad line from docdel: " <<  i;
        set(handle, val);
    }
    if (ferror(fportion))
        ythrow yexception() << "Can't read docdel: " << LastSystemErrorText();
};

ui32 bitmap_1::count(ui32 page) const {
    ui32* v = vect[page];
    if (!v) return 0;
    ui32 n, k;
    for (n = 0, k = 0; n < (ui32)bitmap_pagesize; n++) {
        ui32 u = v[n];
        u = (u >> 1 & 0x55555555) + (u & 0x55555555);
        u = (u >> 2 & 0x33333333) + (u & 0x33333333);
        u = (u >> 4 & 0x0f0f0f0f) + (u & 0x0f0f0f0f);
        u = (u >> 8 & 0x00ff00ff) + (u & 0x00ff00ff);
        k += (u >> 16) + (u & 0xffff);
    }
    return k;
}

ui32 bitmap_1::count() const {
    ui32 cnt = 0, n;
    for (n = 0; n < numpages(); n++)
        cnt += count(n);
    return cnt;
}

bool bitmap_1::set(ui32 id) {
    ui32 mask;
    mask = 1 << (id & 31);
    ui32 elh = id / (bitmap_pagesize * 32);
    id = id / 32 & (bitmap_pagesize - 1);
    if (elh >= vect.size()) { //resize
        vect.resize(elh + 2, (ui32*)0);
    }
    if (!vect[elh]) {
        vect[elh] = new ui32[bitmap_pagesize];
        memset(vect[elh], 0, bitmap_pagesize * sizeof(ui32));
    }
    ui32 *v = vect[elh];
    if (v[id] & mask) return false;
    v[id] |= mask;
    return true;
}

bool bitmap_1::reset(ui32 id) {
    ui32 mask;
    mask = 1 << (id & 31);
    ui32 elh = id / (bitmap_pagesize * 32);
    id = id / 32 & (bitmap_pagesize - 1);
    if (elh >= vect.size() || !vect[elh])
        return false;
    ui32 *v = vect[elh];
    if (!(v[id] & mask)) return false;
    v[id] &= ~mask;
    return true;
}

void bitmap_1::clear() {
    for (ui32 n = 0; n < vect.size(); n++) delete[] vect[n];
    vect.clear();
}

void bitmap_1::save(FILE *f) const throw() {
    ui32 SIG = 0x42744D70, SubSIG = 1, Ver = 0;
    fput(f, SIG); fput(f, SubSIG); fput(f, Ver);
    ui32 n, numfpages = 0, mp = 0, Count = 0;
    autoarray<char> v(numpages(), (char)0);
    for (n = 0; n < numpages(); n++) {
        ui32 cnt = count(n);
        if (cnt) mp = n + 1, numfpages++, Count += cnt, v[n] = 1;
    }
    n = (mp + 3) & ~3;
    fput(f, n); fput(f, numfpages); fput(f, mp); fput(f, Count);
    for (n = 0; n < mp; n++) fput(f, v[n]);
    char p = 0;
    for (; n & 3; n++) fput(f, p);
    for (n = 0; n < numpages(); n++) if (v[n]) {
        size_t written = fwrite(vect[n], sizeof(ui32), bitmap_pagesize, f);
        (void)written;
    }
}

void bitmap_1::load(FILE *f, const char *dbg_fname) {
    ui32 SIG, SubSIG, Ver;
    ui32 bsz, numfpages, mp, Count, numfpages_chk = 0;
    fget(f, SIG); fget(f, SubSIG); fget(f, Ver);
    fget(f, bsz); fget(f, numfpages); fget(f, mp); fget(f, Count);
    if (SIG != 0x42744D70 || SubSIG != 1 || Ver != 0)
        ythrow yexception() <<  dbg_fname << ": unexpected data in bitmap file header";
    clear();
    autoarray<char> v(bsz);
    if (bsz != 0 && bsz != fread(&v[0], 1, bsz, f))
        ythrow yexception() << "fread failed";
    if (vect.size() < mp) vect.resize(mp, (ui32*)0);
    for (ui32 n = 0; n < mp; n++) if (v[n]) {
        if (!vect[n]) vect[n] = new ui32[bitmap_pagesize];
        if ((int)fread(vect[n], sizeof(ui32), bitmap_pagesize, f) != bitmap_pagesize)
            ythrow yexception() <<  dbg_fname << ": bitmap file too small";
        numfpages_chk++;
    }
    if (numfpages_chk != numfpages)
        ythrow yexception() <<  dbg_fname << ": bitmap file damaged";
}

void bitmap_1::save(const char *fname) const {
    TFILEPtr f(fname, "wb");
    save(f);
}
void bitmap_1::load(const char *fname) {
    TFILEPtr f(fname, "rb");
    load(f, fname);
}

bitmap_1_iterator::bitmap_1_iterator(const bitmap_1 &v_) : v(v_) {
    doc = 0;
    cur_page_end = 0;
    cur_page = (size_t)-1;
    cur_page_ptr = 0;
    pptr = eptr = 0;
    num_pages = v.numpages();
    if (!next_page())
        return;
    if (!(cur_mask & 1))
        operator++();
}

bool bitmap_1_iterator::next_page() {
    cur_page++;
    while (true) {
        while (cur_page < num_pages && !v.vect[cur_page])
            cur_page++;
        if (cur_page == num_pages)
            return false;
        cur_page_ptr = v.vect[cur_page];
        pptr = cur_page_ptr, eptr = pptr + bitmap_pagesize;
        for (; pptr < eptr && !*pptr; pptr++) {}
        if (pptr < eptr) {
            doc = cur_page * bitmap_1::elems_per_page + (pptr - cur_page_ptr) * 32;
            cur_mask = *pptr;
            break;
        }
        cur_page++;
    }
    return true;
}
