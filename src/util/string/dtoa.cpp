#include <util/memory/pool.h>
#include <util/system/spinlock.h>
#include <util/memory/tempbuf.h>
#include <util/generic/ptr.h>

static TAtomic POWER_LOCK;

namespace NDtoa {

class TBufIt {
    public:
        inline TBufIt() {
        }

        inline TBufIt(const char* b, const char* e)
            : C_(b)
            , E_(e)
        {
        }

        inline char operator* () throw () {
            if (C_ < E_) {
                return *C_;
            }

            return 0;
        }

        inline TBufIt& operator++ () throw () {
            Inc();
            return *this;
        }

        inline TBufIt& operator-- () throw () {
            Dec();
            return *this;
        }

        inline TBufIt operator++ (int) throw () {
            TBufIt ret(*this);
            Inc();
            return ret;
        }

        inline TBufIt operator-- (int) throw () {
            TBufIt ret(*this);
            Dec();
            return ret;
        }

        inline void operator+= (int n) throw () {
            C_ += n;
        }

        inline bool operator== (const TBufIt& r) const throw () {
            return C_ == r.C_;
        }

        inline bool operator!= (const TBufIt& r) const throw () {
            return C_ != r.C_;
        }

    private:
        inline void Inc() throw () {
            ++C_;
        }

        inline void Dec() throw () {
            --C_;
        }

    public:
        const char* C_;
        const char* E_;
};

inline size_t operator- (const TBufIt& l, const TBufIt& r) {
    return l.C_ - r.C_;
}

#if defined(_little_endian_)
    #define IEEE_8087
#else
    #define IEEE_MC68k
#endif

#define Omit_Private_Memory
#define NO_INFNAN_CHECK

#define Long i32
#define LLong i64
#define ULong ui32
#define ULLong ui64

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable:4244) /*conversion from '' to '', possible loss of data*/
#   pragma warning(disable:4701) /*potentially uninitialized local variable '' used*/
#   pragma warning(disable:4702) /*unreachable code*/
#endif

#include "dtoa.i"
#define HARD
#undef strtod
#define strtod StrToDHard
#include "dtoa_impl.cpp"
#undef HARD
#undef strtod
#define strtod StrToDFast
#include "dtoa_impl.cpp"
#undef strtod
}

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

STATIC_ASSERT(sizeof(NDtoa::U) == sizeof(double));

static inline double StrToDImpl(const char* b, const char* e, char** se) {
    using namespace NDtoa;

    return StrToDFast(TBufIt(b, e), se);
}
