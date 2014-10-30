#include "tempbuf.h"
#include "addstorage.h"

#include <util/system/tls.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/generic/intrlist.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/thread/singleton.h>

#ifndef TMP_BUF_LEN
    #define TMP_BUF_LEN (64 * 1024)
#endif

class TTempBuf::TImpl: public TRefCounted<TImpl, TSimpleCounter, TImpl> {
    public:
        inline TImpl(void* data, size_t size) throw ()
            : Data_(data)
            , Size_(size)
            , Offset_(0)
        {
        }

        /*
         * We do not really need 'virtual' here
         * but for compiler happiness...
         */
        virtual ~TImpl() throw () {
        }

        inline void* Data() throw () {
            return Data_;
        }

        const void* Data() const throw () {
            return Data_;
        }

        inline size_t Size() const throw () {
            return Size_;
        }

        inline size_t Filled() const throw () {
            return Offset_;
        }

        inline void Reset() throw () {
            Offset_ = 0;
        }

        inline size_t Left() const throw () {
            return Size() - Filled();
        }

        void SetPos(size_t off) {
            YASSERT(off <= Size());
            Offset_ = off;
        }

        inline void Proceed(size_t off) {
            YASSERT(off <= Left());

            Offset_ += off;
        }

        static inline void Destroy(TImpl* This) throw () {
            This->Dispose();
        }

    protected:
        virtual void Dispose() throw () = 0;

    private:
        void*  Data_;
        size_t Size_;
        size_t Offset_;
};

namespace {
    class TTempBufManager;

    class TAllocedBuf: public TTempBuf::TImpl, public TAdditionalStorage<TAllocedBuf> {
    public:
        inline TAllocedBuf()
            : TImpl(AdditionalData(), AdditionalDataLength())
        {
        }

        inline ~TAllocedBuf() throw () {
        }

    private:
        virtual void Dispose() throw () {
            delete this;
        }
    };

    class TPerThreadedBuf: public TTempBuf::TImpl, public TIntrusiveSListItem<TPerThreadedBuf> {
        friend class TTempBufManager;
    public:
        inline TPerThreadedBuf(TTempBufManager* manager) throw ()
            : TImpl(Data_, sizeof(Data_))
            , Manager_(manager)
        {
        }

        inline ~TPerThreadedBuf() throw () {
        }

    private:
        virtual void Dispose() throw ();

    private:
        char Data_[TMP_BUF_LEN];
        TTempBufManager* Manager_;
    };

    class TTempBufManager {
        struct TDelete {
            inline void operator() (TPerThreadedBuf* p) throw () {
                delete p;
            }
        };

    public:
        inline TTempBufManager() throw () {
        }

        inline ~TTempBufManager() throw () {
            TDelete deleter;

            Unused_.ForEach(deleter);
        }

        inline TPerThreadedBuf* Aquire() {
            if (!Unused_.Empty()) {
                return Unused_.PopFront();
            }

            return new TPerThreadedBuf(this);
        }

        inline void Return(TPerThreadedBuf* buf) throw () {
            buf->Reset();
            Unused_.PushFront(buf);
        }

    private:
        TIntrusiveSList<TPerThreadedBuf> Unused_;
    };
}

static inline TTempBufManager* TempBufManager() {
    return FastTlsSingletonWithPriority<TTempBufManager, 2>();
}

void TPerThreadedBuf::Dispose() throw () {
    if (Manager_ == TempBufManager()) {
        Manager_->Return(this);
    } else {
        delete this;
    }
}

TTempBuf::TTempBuf()
    : Impl_(TempBufManager()->Aquire())
{
}

/*
 * all magick is here:
 * if len <= TMP_BUF_LEN. then we get prealloced per threaded buffer
 * else allocate one in heap
 */
static inline TTempBuf::TImpl* ConstructImpl(size_t len) {
    if (len <= TMP_BUF_LEN) {
        return TempBufManager()->Aquire();
    }

    return new (len) TAllocedBuf();
}

TTempBuf::TTempBuf(size_t len)
    : Impl_(ConstructImpl(len))
{
}

TTempBuf::TTempBuf(const TTempBuf& b) throw ()
    : Impl_(b.Impl_)
{
}

TTempBuf::~TTempBuf() throw () {
}

TTempBuf& TTempBuf::operator= (const TTempBuf& b) throw () {
    if (this != &b) {
        Impl_ = b.Impl_;
    }

    return *this;
}

char* TTempBuf::Data() throw () {
    return (char*)Impl_->Data();
}

const char* TTempBuf::Data() const throw () {
    return static_cast<const char *>(Impl_->Data());
}

size_t TTempBuf::Size() const throw () {
    return Impl_->Size();
}

char* TTempBuf::Current() throw () {
    return Data() + Filled();
}

const char* TTempBuf::Current() const throw () {
    return Data() + Filled();
}

size_t TTempBuf::Filled() const throw () {
    return Impl_->Filled();
}

size_t TTempBuf::Left() const throw () {
    return Impl_->Left();
}

void TTempBuf::Reset() throw () {
    Impl_->Reset();
}

void TTempBuf::SetPos(size_t off) {
    Impl_->SetPos(off);
}

void TTempBuf::Proceed(size_t off) {
    Impl_->Proceed(off);
}

void TTempBuf::Append(const void* data, size_t len) {
    if (len > Left()) {
        ythrow yexception() << "temp buf exhausted(" << Left() << ", " << len << ")";
    }

    memcpy(Current(), data, len);
    Proceed(len);
}

#if 0
#include "cputimer.h"

#define LEN (1024)

void* allocaFunc() {
    return alloca(LEN);
}

int main() {
    const size_t num = 10000000;
    size_t tmp = 0;

    {
        CTimer t("alloca");

        for (size_t i = 0; i < num; ++i) {
            tmp += (size_t)allocaFunc();
        }
    }

    {
        CTimer t("log buffer");

        for (size_t i = 0; i < num; ++i) {
            TTempBuf buf(LEN);

            tmp += (size_t)buf.Data();
        }
    }

    {
        CTimer t("malloc");

        for (size_t i = 0; i < num; ++i) {
            void* ptr = malloc(LEN);

            tmp += (size_t)ptr;

            free(ptr);
        }
    }

    return 0;
}
#endif
