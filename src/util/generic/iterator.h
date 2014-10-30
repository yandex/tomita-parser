#pragma once

namespace NStlIterator {
    template <class T>
    struct TRefFromPtr;

    template <class T>
    struct TRefFromPtr<T*> {
        typedef T& TResult;
    };

    template <class T>
    struct TTraits {
        typedef typename T::TPtr TPtr;
        typedef typename TRefFromPtr<TPtr>::TResult TRef;

        static inline TPtr Ptr(const T& p) throw () {
            return T::Ptr(p);
        }
    };

    template <class T>
    struct TTraits<T*> {
        typedef T* TPtr;
        typedef T& TRef;

        static inline TPtr Ptr(TPtr p) throw () {
            return p;
        }
    };
}

template <class TSlave>
class TStlIterator {
    public:
        class TIterator {
            public:
                typedef typename TSlave::TRetVal TRetVal;
                typedef NStlIterator::TTraits<TRetVal> TValueTraits;
                typedef typename TValueTraits::TPtr TPtr;
                typedef typename TValueTraits::TRef TRef;

                inline TIterator() throw ()
                    : Slave_(0)
                    , Cur_()
                {
                }

                inline TIterator(TSlave* slave)
                    : Slave_(slave)
                    , Cur_(Slave_->Next())
                {
                }

                const TRetVal& Value() const throw() {
                    return Cur_;
                }

                inline bool operator== (const TIterator& it) const throw () {
                    return Cur_ == it.Cur_;
                }

                inline bool operator!= (const TIterator& it) const throw () {
                    return !(*this == it);
                }

                inline TPtr operator-> () const throw () {
                    return TValueTraits::Ptr(Cur_);
                }

                inline TRef operator* () const throw () {
                    return *TValueTraits::Ptr(Cur_);
                }

                inline TIterator& operator++ () throw () {
                    Cur_ = Slave_->Next();

                    return *this;
                }

            private:
                TSlave* Slave_;
                TRetVal Cur_;
        };

    public:
        inline TIterator Begin() const throw () {
            return TIterator(const_cast<TSlave*>(static_cast<const TSlave*>(this)));
        }

        inline TIterator End() const throw () {
            return TIterator();
        }

        //compat
        inline TIterator begin() const throw () {
            return this->Begin();
        }

        inline TIterator end() const throw () {
            return this->End();
        }
};
