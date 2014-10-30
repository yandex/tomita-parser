#include "tls.h"
#include "mutex.h"
#include "thread.h"

#include <util/generic/set.h>
#include <util/generic/hash.h>
#include <util/generic/intrlist.h>
#include <util/generic/singleton.h>

#if defined(_unix_)
    #include <pthread.h>
#endif

using namespace NTls;

namespace {
static inline TAtomicBase AcquireKey() {
    static TAtomic cur;

    return AtomicIncrement(cur) - (TAtomicBase)1;
}

class TGenericTlsBase {
public:
    typedef size_t TSmallKey;

    class TPerThreadStorage {
    public:
        struct TKey: public TNonCopyable {
            inline TKey(TDtor dtor)
                : Key(AcquireKey())
                , Dtor(dtor)
            {
            }

            TSmallKey Key;
            TDtor Dtor;
        };

        class TStoredValue: public TIntrusiveListItem<TStoredValue> {
        public:
            inline TStoredValue(const TKey* key)
                : Data_(0)
                , Dtor_(key->Dtor)
            {
            }

            inline ~TStoredValue() throw () {
                if (Dtor_ && Data_) {
                    Dtor_(Data_);
                }
            }

            inline void Set(void* ptr) throw () {
                Data_ = ptr;
            }

            inline void* Get() const throw () {
                return Data_;
            }

        private:
            void* Data_;
            TDtor Dtor_;
        };

        inline TStoredValue* Value(const TKey* key) {
            const size_t idx = (size_t)key->Key;

            if (idx >= Values_.size()) {
                Values_.resize(idx + 1);
            }

            TStoredValue*& ret = Values_[idx];

            if (!ret) {
                THolder<TStoredValue> sv(new TStoredValue(key));

                Storage_.PushFront(sv.Get());
                ret = sv.Release();
            }

            return ret;
        }

    private:
        yvector<TStoredValue*> Values_;
        TIntrusiveListWithAutoDelete<TStoredValue, TDelete> Storage_;
    };

    inline TPerThreadStorage* MyStorage() {
#if defined(HAVE_FAST_POD_TLS)
        POD_STATIC_THREAD(TPerThreadStorage*) my(0);

        if (!my) {
            my = MyStorageSlow();
        }

        return my;
#else
        return MyStorageSlow();
#endif
    }

    virtual TPerThreadStorage* MyStorageSlow() = 0;
};
}

#if defined(_unix_)
namespace {
    class TMasterTls: public TGenericTlsBase {
    public:
        inline TMasterTls() {
            VERIFY(!pthread_key_create(&Key_, Dtor), "pthread_key_create failed");
        }

        inline ~TMasterTls() throw () {
            //explicitly call dtor for main thread
            Dtor(pthread_getspecific(Key_));

            VERIFY(!pthread_key_delete(Key_), "pthread_key_delete failed");
        }

        static inline TMasterTls* Instance() {
            return SingletonWithPriority<TMasterTls, 1>();
        }

    private:
        virtual TPerThreadStorage* MyStorageSlow() {
            void* ret = pthread_getspecific(Key_);

            if (!ret) {
                ret = new TPerThreadStorage();

                VERIFY(!pthread_setspecific(Key_, ret), "pthread_setspecific failed");
            }

            return (TPerThreadStorage*)ret;
        }

        static void Dtor(void* ptr) {
            delete (TPerThreadStorage*)ptr;
        }

    private:
        pthread_key_t Key_;
    };

    typedef TMasterTls::TPerThreadStorage::TKey TKeyDescriptor;
}

class TKey::TImpl: public TKeyDescriptor {
public:
    inline TImpl(TDtor dtor)
        : TKeyDescriptor(dtor)
    {
    }

    inline void* Get() const {
        return TMasterTls::Instance()->MyStorage()->Value(this)->Get();
    }

    inline void Set(void* val) const {
        TMasterTls::Instance()->MyStorage()->Value(this)->Set(val);
    }

    static inline void Cleanup() {
    }
};
#else
namespace {
    class TGenericTls: public TGenericTlsBase {
    public:
        virtual TPerThreadStorage* MyStorageSlow() {
            TGuard<TMutex> lock(Lock_);

            {
                TPTSRef& ret = Datas_[TThread::CurrentThreadId()];

                if (!ret) {
                    ret.Reset(new TPerThreadStorage());
                }

                return ret.Get();
            }
        }

        inline void Cleanup() throw () {
            TGuard<TMutex> lock(Lock_);

            Datas_.erase(TThread::CurrentThreadId());
        }

        static inline TGenericTls* Instance() {
            return SingletonWithPriority<TGenericTls, 1>();
        }

    private:
        typedef TAutoPtr<TPerThreadStorage> TPTSRef;
        TMutex Lock_;
        yhash<TThread::TId, TPTSRef> Datas_;
    };
}

class TKey::TImpl {
public:
    inline TImpl(TDtor dtor)
        : Key_(dtor)
    {
    }

    inline void* Get() {
        return TGenericTls::Instance()->MyStorage()->Value(&Key_)->Get();
    }

    inline void Set(void* ptr) {
        TGenericTls::Instance()->MyStorage()->Value(&Key_)->Set(ptr);
    }

    static inline void Cleanup() {
        TGenericTls::Instance()->Cleanup();
    }

private:
    TGenericTls::TPerThreadStorage::TKey Key_;
};
#endif

TKey::TKey(TDtor dtor)
    : Impl_(new TImpl(dtor))
{
}

TKey::~TKey() throw () {
}

void* TKey::Get() const {
    return Impl_->Get();
}

void TKey::Set(void* ptr) const {
    Impl_->Set(ptr);
}

void TKey::Cleanup() throw () {
    TImpl::Cleanup();
}
