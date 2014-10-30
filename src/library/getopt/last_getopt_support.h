#pragma once

#include <util/string/cast.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>

namespace NLastGetopt {

class TOpt;
class TOpts;
class TOptsParser;

/// base of all getopt exceptions
class TException: public yexception {
};

class TConfException: public TException {
};

class TUsageException: public TException {
};

struct IOptHandler {
    virtual void HandleOpt(const TOptsParser* parser) = 0;
    virtual ~IOptHandler() { }
};

namespace NPrivate {

    template <typename TpFunc>
    class THandlerFunctor0
        : public IOptHandler
    {
        TpFunc Func_;
    public:
        THandlerFunctor0(TpFunc func)
            : Func_(func)
        {}
        virtual void HandleOpt(const TOptsParser*) {
            Func_();
        }
    };

    template <typename TpFunc, typename TpArg = const TOptsParser*>
    class THandlerFunctor1
        : public IOptHandler
    {
        TpFunc Func_;
        const TpArg Def_;
        const bool HasDef_;
    public:
        THandlerFunctor1(TpFunc func)
            : Func_(func)
            , Def_()
            , HasDef_(false)
        {}
        template <typename T>
        THandlerFunctor1(const TpFunc &func, const T &def)
            : Func_(func)
            , Def_(def)
            , HasDef_(true)
        {}
        virtual void HandleOpt(const TOptsParser* parser);
    };

    template <typename TpFunc>
    class THandlerFunctor1<TpFunc, const TOptsParser*>
        : public IOptHandler
    {
        TpFunc Func_;
    public:
        THandlerFunctor1(TpFunc func)
            : Func_(func)
        {}
        virtual void HandleOpt(const TOptsParser* parser) {
            Func_(parser);
        }
    };

    template <typename T, typename TpVal = T>
    class TStoreResultFunctor
    {
    private:
        T* Target_;
    public:
        TStoreResultFunctor(T* target)
            : Target_(target)
        {}
        void operator()(const TpVal& val) {
            *Target_ = val;
        }
    };

    template <typename TpTarget, typename TpFunc, typename TpVal = TpTarget>
    class TStoreMappedResultFunctor
    {
    private:
        TpTarget* Target_;
        const TpFunc Func_;
    public:
        TStoreMappedResultFunctor(TpTarget* target, const TpFunc& func)
            : Target_(target)
            , Func_(func)
        {}
        void operator()(const TpVal& val) {
            *Target_ = Func_(val);
        }
    };

    template <typename T, typename TpVal = T>
    class TStoreValueFunctor
    {
        T* Target;
        const TpVal Value;

    public:
        template <typename TpArg>
        TStoreValueFunctor(T* target, const TpArg& value)
            : Target(target)
            , Value(value)
        {}
        void operator()(const TOptsParser*) {
            *Target = Value;
        }
    };

    Stroka OptToString(char c);
    Stroka OptToString(const Stroka& longOption);
    Stroka OptToString(const TOpt* opt);

    // wrapper of FromString<T> that prints nice message about option used
    template <typename T, typename TSomeOpt>
    T OptFromString(const TStringBuf& value, const TSomeOpt opt);

} // NPrivate
} // NLastGetopt
