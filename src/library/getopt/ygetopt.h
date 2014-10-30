#pragma once

#include <util/generic/ptr.h>

class Stroka;

class TGetOpt {
    public:
        class TIterator {
                friend class TGetOpt;
            public:
                char Key() const throw ();
                const char* Arg() const throw ();

                inline bool HaveArg() const throw () {
                    return Arg();
                }

                inline void operator++ () {
                    Next();
                }

                inline bool operator== (const TIterator& r) const throw () {
                    return AtEnd() == r.AtEnd();
                }

                inline bool operator!= (const TIterator& r) const throw () {
                    return !(*this == r);
                }

                inline TIterator& operator* () throw () {
                    return *this;
                }

                inline const TIterator& operator* () const throw () {
                    return *this;
                }

                inline TIterator* operator-> () throw () {
                    return this;
                }

                inline const TIterator* operator-> () const throw () {
                    return this;
                }

            private:
                TIterator() throw ();
                TIterator(const TGetOpt* parent);

                void Next();
                bool AtEnd() const throw ();

            private:
                class TIterImpl;
                TSimpleIntrusivePtr<TIterImpl> Impl_;
        };

        TGetOpt(int argc, const char* const* argv, const Stroka& format);

        inline TIterator Begin() const {
            return TIterator(this);
        }

        inline TIterator End() const throw () {
            return TIterator();
        }

    private:
        class TImpl;
        TSimpleIntrusivePtr<TImpl> Impl_;
};
