#include "opt.h"
#include "ygetopt.h"

#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

class TGetOpt::TImpl: public TRefCounted<TImpl> {
    public:
        inline TImpl(int argc, const char* const* argv, const Stroka& fmt)
            : args(argv, argv + argc)
            , format(fmt)
        {
            if (argc == 0) {
                ythrow yexception() << "zero argc";
            }
        }

        inline ~TImpl() throw () {
        }

        yvector<Stroka> args;
        const Stroka format;
};

class TGetOpt::TIterator::TIterImpl: public TRefCounted<TIterImpl> {
    public:
        inline TIterImpl(const TGetOpt* parent)
            : Args_(parent->Impl_->args)
            , ArgsPtrs_(new char*[Args_.size() + 1])
            , Format_(parent->Impl_->format)
            , OptLet_(0)
            , Arg_(0)
        {
            for (size_t i = 0; i < Args_.size(); ++i) {
                ArgsPtrs_.Get()[i] = Args_[i].begin();
            }

            ArgsPtrs_.Get()[Args_.size()] = 0;
            Opt_.Reset(new Opt((int)Args_.size(), ArgsPtrs_.Get(), ~Format_));
        }

        inline ~TIterImpl() throw () {
        }

        inline void Next() {
            OptLet_ = Opt_->Get();
            Arg_ = Opt_->Arg;
        }

        inline char Key() const throw () {
            return (char)OptLet_;
        }

        inline const char* Arg() const throw () {
            return Arg_;
        }

        inline bool AtEnd() const throw () {
            return OptLet_ == EOF;
        }

    private:
        yvector<Stroka> Args_;
        TArrayHolder<char*> ArgsPtrs_;
        const Stroka Format_;
        THolder<Opt> Opt_;
        int OptLet_;
        const char* Arg_;
};

TGetOpt::TIterator::TIterator() throw ()
    : Impl_(0)
{
}

TGetOpt::TIterator::TIterator(const TGetOpt* parent)
    : Impl_(new TIterImpl(parent))
{
    Next();
}

void TGetOpt::TIterator::Next() {
    Impl_->Next();
}

char TGetOpt::TIterator::Key() const throw () {
    return Impl_->Key();
}

bool TGetOpt::TIterator::AtEnd() const throw () {
    if (Impl_.Get()) {
        return Impl_->AtEnd();
    }

    return true;
}

const char* TGetOpt::TIterator::Arg() const throw () {
    if (Impl_.Get()) {
        return Impl_->Arg();
    }

    return 0;
}

TGetOpt::TGetOpt(int argc, const char* const* argv, const Stroka& format)
    : Impl_(new TImpl(argc, argv, format))
{
}
