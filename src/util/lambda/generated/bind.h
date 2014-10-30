#pragma once

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 1>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 1> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() () {
        return TBase::mSelf(TBase::mParam);
    }

    inline TResult operator() () const {
        return TBase::mSelf(TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)();
    //STATIC_ASSERT(TT::Value == 0);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 2>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 2> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2) {
        return TBase::mSelf(TBase::mParam, p2);
    }

    inline TResult operator() (TParam2 p2) const {
        return TBase::mSelf(TBase::mParam, p2);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2);
    //STATIC_ASSERT(TT::Value == 1);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 2, 2>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 2, 2> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef TpArg TParam2;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1) {
        return TBase::mSelf(p1, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1) const {
        return TBase::mSelf(p1, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1);
    //STATIC_ASSERT(TT::Value == 1);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 3>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 3> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3) {
        return TBase::mSelf(TBase::mParam, p2, p3);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3) const {
        return TBase::mSelf(TBase::mParam, p2, p3);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3);
    //STATIC_ASSERT(TT::Value == 2);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 3, 3>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 3, 3> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef TpArg TParam3;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2) {
        return TBase::mSelf(p1, p2, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2) const {
        return TBase::mSelf(p1, p2, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2);
    //STATIC_ASSERT(TT::Value == 2);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 4>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 4> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4) {
        return TBase::mSelf(TBase::mParam, p2, p3, p4);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4) const {
        return TBase::mSelf(TBase::mParam, p2, p3, p4);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3, TParam4);
    //STATIC_ASSERT(TT::Value == 3);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 4, 4>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 4, 4> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef TpArg TParam4;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3) {
        return TBase::mSelf(p1, p2, p3, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3) const {
        return TBase::mSelf(p1, p2, p3, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2, TParam3);
    //STATIC_ASSERT(TT::Value == 3);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 5>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 5> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5) {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5) const {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3, TParam4, TParam5);
    //STATIC_ASSERT(TT::Value == 4);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 5, 5>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 5, 5> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef TpArg TParam5;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4) {
        return TBase::mSelf(p1, p2, p3, p4, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4) const {
        return TBase::mSelf(p1, p2, p3, p4, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2, TParam3, TParam4);
    //STATIC_ASSERT(TT::Value == 4);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 6>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 6> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6) {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6) const {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3, TParam4, TParam5, TParam6);
    //STATIC_ASSERT(TT::Value == 5);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 6, 6>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 6, 6> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef TpArg TParam6;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5) {
        return TBase::mSelf(p1, p2, p3, p4, p5, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5) const {
        return TBase::mSelf(p1, p2, p3, p4, p5, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2, TParam3, TParam4, TParam5);
    //STATIC_ASSERT(TT::Value == 5);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 7>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 7> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;
    typedef typename TFunctionParam<TpOp, 7>::TType TParam7;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7) {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6, p7);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7) const {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6, p7);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3, TParam4, TParam5, TParam6, TParam7);
    //STATIC_ASSERT(TT::Value == 6);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 7, 7>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 7, 7> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;
    typedef TpArg TParam7;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6) {
        return TBase::mSelf(p1, p2, p3, p4, p5, p6, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6) const {
        return TBase::mSelf(p1, p2, p3, p4, p5, p6, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2, TParam3, TParam4, TParam5, TParam6);
    //STATIC_ASSERT(TT::Value == 6);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 8>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 8> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;
    typedef typename TFunctionParam<TpOp, 7>::TType TParam7;
    typedef typename TFunctionParam<TpOp, 8>::TType TParam8;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7, TParam8 p8) {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6, p7, p8);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7, TParam8 p8) const {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6, p7, p8);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8);
    //STATIC_ASSERT(TT::Value == 7);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 8, 8>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 8, 8> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;
    typedef typename TFunctionParam<TpOp, 7>::TType TParam7;
    typedef TpArg TParam8;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7) {
        return TBase::mSelf(p1, p2, p3, p4, p5, p6, p7, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7) const {
        return TBase::mSelf(p1, p2, p3, p4, p5, p6, p7, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7);
    //STATIC_ASSERT(TT::Value == 7);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 1, 9>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 1, 9> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef TpArg TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;
    typedef typename TFunctionParam<TpOp, 7>::TType TParam7;
    typedef typename TFunctionParam<TpOp, 8>::TType TParam8;
    typedef typename TFunctionParam<TpOp, 9>::TType TParam9;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7, TParam8 p8, TParam9 p9) {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6, p7, p8, p9);
    }

    inline TResult operator() (TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7, TParam8 p8, TParam9 p9) const {
        return TBase::mSelf(TBase::mParam, p2, p3, p4, p5, p6, p7, p8, p9);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9);
    //STATIC_ASSERT(TT::Value == 8);
};

template <typename TpOp, typename TpArg>
struct TBinder<TpOp, TpArg, 9, 9>: public TBinderBase<TpOp, TpArg> {
    typedef TBinderBase<TpOp, TpArg> TBase;
    typedef TBinder<TpOp, TpArg, 9, 9> TSelf;

    typedef typename TBase::TResult TResult;
    typedef typename TBase::TOperationVar TOperationVar;

    typedef typename TFunctionParam<TpOp, 1>::TType TParam1;
    typedef typename TFunctionParam<TpOp, 2>::TType TParam2;
    typedef typename TFunctionParam<TpOp, 3>::TType TParam3;
    typedef typename TFunctionParam<TpOp, 4>::TType TParam4;
    typedef typename TFunctionParam<TpOp, 5>::TType TParam5;
    typedef typename TFunctionParam<TpOp, 6>::TType TParam6;
    typedef typename TFunctionParam<TpOp, 7>::TType TParam7;
    typedef typename TFunctionParam<TpOp, 8>::TType TParam8;
    typedef TpArg TParam9;

    inline TBinder(const TOperationVar& self, const TpArg& p)
        : TBase(self, p)
    {
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7, TParam8 p8) {
        return TBase::mSelf(p1, p2, p3, p4, p5, p6, p7, p8, TBase::mParam);
    }

    inline TResult operator() (TParam1 p1, TParam2 p2, TParam3 p3, TParam4 p4, TParam5 p5, TParam6 p6, TParam7 p7, TParam8 p8) const {
        return TBase::mSelf(p1, p2, p3, p4, p5, p6, p7, p8, TBase::mParam);
    }

    typedef TFunctionParamCount<TSelf> TT;
    typedef TResult (TType)(TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8);
    //STATIC_ASSERT(TT::Value == 8);
};

template <typename T, typename P1, typename P2>
static inline TBinder<TBinder<T, P1, 1>, P2, 1> BindFirst(const T& self, const P1& p1, const P2& p2) {
    return BindFirst<TBinder<T, P1, 1>, P2>(BindFirst<T, P1>(self, p1), p2);
}

template <typename T, typename P1, typename P2, typename P3>
static inline TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3) {
    return BindFirst<TBinder<TBinder<T, P1, 1>, P2, 1>, P3>(BindFirst<T, P1, P2>(self, p1, p2), p3);
}

template <typename T, typename P1, typename P2, typename P3, typename P4>
static inline TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
    return BindFirst<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4>(BindFirst<T, P1, P2, P3>(self, p1, p2, p3), p4);
}

template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5>
static inline TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
    return BindFirst<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5>(BindFirst<T, P1, P2, P3, P4>(self, p1, p2, p3, p4), p5);
}

template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
static inline TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
    return BindFirst<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6>(BindFirst<T, P1, P2, P3, P4, P5>(self, p1, p2, p3, p4, p5), p6);
}

template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
static inline TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1>, P7, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7) {
    return BindFirst<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1>, P7>(BindFirst<T, P1, P2, P3, P4, P5, P6>(self, p1, p2, p3, p4, p5, p6), p7);
}

template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
static inline TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1>, P7, 1>, P8, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8) {
    return BindFirst<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1>, P7, 1>, P8>(BindFirst<T, P1, P2, P3, P4, P5, P6, P7>(self, p1, p2, p3, p4, p5, p6, p7), p8);
}

template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
static inline TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1>, P7, 1>, P8, 1>, P9, 1> BindFirst(const T& self, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8, const P9& p9) {
    return BindFirst<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<TBinder<T, P1, 1>, P2, 1>, P3, 1>, P4, 1>, P5, 1>, P6, 1>, P7, 1>, P8, 1>, P9>(BindFirst<T, P1, P2, P3, P4, P5, P6, P7, P8>(self, p1, p2, p3, p4, p5, p6, p7, p8), p9);
}
