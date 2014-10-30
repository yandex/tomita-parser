#pragma once

#include "memory_traits.h"

namespace NAccessors {
namespace NPrivate {

template <typename Ta>
struct TMemoryAccessorBase {
    enum {
        SimpleMemory = TMemoryTraits<Ta>::SimpleMemory,
        ContinuousMemory = TMemoryTraits<Ta>::ContinuousMemory,
    };

    struct TBadAccessor;
};

template <typename Ta>
struct TBegin : public TMemoryAccessorBase<Ta> {
    typedef typename TMemoryTraits<Ta>::TElementType TElementType;

    template <typename Tb>
    struct TNoMemoryIndirectionBegin {
        static const TElementType* Get(const Tb& b) {
            return (const TElementType*)&b;
        }
    };

    template <typename Tb>
    struct TIndirectMemoryRegionBegin {
        HAS_MEMBER(Begin)
        HAS_MEMBER(begin)

        template <typename Tc>
        struct TByBegin {
            static const TElementType* Get(const Tc& b) {
                return (const TElementType*)b.Begin();
            }
        };

        template <typename Tc>
        struct TBybegin {
            static const TElementType* Get(const Tc& b) {
                return (const TElementType*)b.begin();
            }
        };

        typedef typename TSelectType<THasBegin<Tb>::Result,
                        TByBegin<Tb>, TBybegin<Tb> >::TResult TGet;

        static const TElementType* Get(const Tb& b) {
            return TGet::Get(b);
        }
    };

    typedef typename TSelectType<
                    TMemoryAccessorBase<Ta>::SimpleMemory,
                    TNoMemoryIndirectionBegin<Ta>,
                    typename TSelectType<
                                TMemoryAccessorBase<Ta>::ContinuousMemory,
                                TIndirectMemoryRegionBegin<Ta>,
                                typename TMemoryAccessorBase<Ta>::TBadAccessor
                                >::TResult
                    >::TResult TGet;

    static const TElementType* Get(const Ta& b) {
        return TGet::Get(b);
    }
};

template <typename Ta>
struct TEnd : public TMemoryAccessorBase<Ta> {
    typedef typename TMemoryTraits<Ta>::TElementType TElementType;

    template <typename Tb>
    struct TNoMemoryIndirectionEnd {
        static const TElementType* Get(const Tb& b) {
            return (const TElementType*)(&b + 1);
        }
    };

    template <typename Tb>
    struct TIndirectMemoryRegionEnd {
        HAS_MEMBER(End)
        HAS_MEMBER(end)

        template <typename Tc>
        struct TByEnd {
            static const TElementType* Get(const Tc& b) {
                return (const TElementType*)b.End();
            }
        };

        template <typename Tc>
        struct TByend {
            static const TElementType* Get(const Tc& b) {
                return (const TElementType*)b.end();
            }
        };

        typedef typename TSelectType<THasEnd<Tb>::Result,
                        TByEnd<Tb>, TByend<Tb> >::TResult TGet;

        static const TElementType* Get(const Tb& b) {
            return TGet::Get(b);
        }
    };

    typedef typename TSelectType<
                    TMemoryAccessorBase<Ta>::SimpleMemory,
                    TNoMemoryIndirectionEnd<Ta>,
                    typename TSelectType<
                                TMemoryAccessorBase<Ta>::ContinuousMemory,
                                TIndirectMemoryRegionEnd<Ta>,
                                typename TMemoryAccessorBase<Ta>::TBadAccessor
                                >::TResult
                    >::TResult TGet;

    static const TElementType* Get(const Ta& b) {
        return TGet::Get(b);
    }
};

template <typename Ta, bool Init>
struct TClear : public TMemoryAccessorBase<Ta> {
    template <typename Tb>
    struct TNoMemoryIndirectionClear {
        static void Do(Tb& b) {
            Zero(b);
        }
    };

    template <typename Tb>
    struct TIndirectMemoryRegionClear {
        HAS_MEMBER(Clear)
        HAS_MEMBER(clear)

        template <typename Tc>
        struct TByClear {
            static void Do(Tc& b) {
                b.Clear();
            }
        };

        template <typename Tc>
        struct TByclear {
            static void Do(Tc& b) {
                b.clear();
            }
        };

        template <typename Tc>
        struct TByNone {
            static void Do(Tc& b) {
                if (!Init)
                    b = Tc();
            }
        };

        typedef typename TSelectType<
                        THasClear<Tb>::Result,
                        TByClear<Tb>,
                        typename TSelectType<
                                THasclear<Tb>::Result,
                                TByclear<Tb>,
                                TByNone<Tb>
                                >::TResult
                        >::TResult TDo;

        static void Do(Tb& b) {
            TDo::Do(b);
        }

    };

    typedef typename TSelectType<
                    TMemoryAccessorBase<Ta>::SimpleMemory,
                    TNoMemoryIndirectionClear<Ta>,
                    TIndirectMemoryRegionClear<Ta>
            >::TResult TDo;

    static void Do(Ta& b) {
        TDo::Do(b);
    }
};

template <typename Tb>
struct TReserve {
    HAS_MEMBER(Reserve)
    HAS_MEMBER(reserve)

    template <typename Tc>
    struct TByReserve {
        static void Do(Tc& b, size_t sz) {
            b.Reserve(sz);
        }
    };

    template <typename Tc>
    struct TByreserve {
        static void Do(Tc& b, size_t sz) {
            b.reserve(sz);
        }
    };

    template <typename Tc>
    struct TByNone {
        static void Do(Tc&, size_t) {
        }
    };

    typedef typename TSelectType<
                    THasReserve<Tb>::Result,
                    TByReserve<Tb>,
                    typename TSelectType<
                            THasreserve<Tb>::Result,
                            TByreserve<Tb>,
                            TByNone<Tb>
                            >::TResult
                    >::TResult TDo;

    static void Do(Tb& b, size_t sz) {
        return TDo::Do(b, sz);
    }
};

template <typename Tb>
struct TResize {
    HAS_MEMBER(Resize)
    HAS_MEMBER(resize)

    template <typename Tc>
    struct TByResize {
        static void Do(Tc& b, size_t sz) {
            b.Resize(sz);
        }
    };

    template <typename Tc>
    struct TByresize {
        static void Do(Tc& b, size_t sz) {
            b.resize(sz);
        }
    };

    typedef typename TSelectType<THasResize<Tb>::Result,
                    TByResize<Tb>, TByresize<Tb> >::TResult TDo;

    static void Do(Tb& b, size_t sz) {
        return TDo::Do(b, sz);
    }
};

template <typename Tb>
struct TAppend {
    HAS_MEMBER(Append)
    HAS_MEMBER(append)
    HAS_MEMBER(push_back)

    template <typename Tc>
    struct TByAppend {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType& val) {
            b.Append(val);
        }
    };

    template <typename Tc>
    struct TByappend {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType& val) {
            b.append(val);
        }
    };

    template <typename Tc>
    struct TBypush_back {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType& val) {
            b.push_back(val);
        }
    };

    typedef typename TSelectType<
                    THasAppend<Tb>::Result,
                    TByAppend<Tb>,
                    typename TSelectType<
                            THasappend<Tb>::Result,
                            TByappend<Tb>,
                            TBypush_back<Tb>
                            >::TResult
                    >::TResult TDo;

    typedef typename TMemoryTraits<Tb>::TElementType TElementType;

    static void Do(Tb& b, const TElementType& val) {
        return TDo::Do(b, val);
    }
};

template <typename Tb>
struct TAppendRegion {
    HAS_MEMBER(Append)
    HAS_MEMBER(append)
    HAS_MEMBER(insert)

    template <typename Tc>
    struct TByAppend {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
            b.Append(beg, end);
        }
    };

    template <typename Tc>
    struct TByappend {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
            b.append(beg, end);
        }
    };

    template <typename Tc>
    struct TByinsert {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
            b.insert(b.end(), beg, end);
        }
    };

    template <typename Tc>
    struct TByNone {
        typedef typename TMemoryTraits<Tc>::TElementType TElementType;

        static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
            for (const TElementType* it = beg; it != end; ++it)
                TAppend<Tc>::Do(b, *it);
        }
    };

    typedef typename TSelectType<
                    THasAppend<Tb>::Result,
                    TByAppend<Tb>,
                    typename TSelectType<
                            THasappend<Tb>::Result,
                            TByappend<Tb>,
                            typename TSelectType<
                                    THasinsert<Tb>::Result,
                                    TByinsert<Tb>,
                                    TByNone<Tb>
                            >::TResult
                     >::TResult
             >::TResult TDo;

    typedef typename TMemoryTraits<Tb>::TElementType TElementType;

    static void Do(Tb& b, const TElementType* beg, const TElementType* end) {
        return TDo::Do(b, beg, end);
    }
};

template <typename Ta>
struct TAssign : public TMemoryAccessorBase<Ta> {
    typedef typename TMemoryTraits<Ta>::TElementType TElementType;

    template <typename Tb>
    struct TNoMemoryIndirectionAssign {
        static void Do(Tb& b, const TElementType* beg, const TElementType* end) {
            if (sizeof(Tb) == sizeof(TElementType))
                b = *beg;
            else
                memcpy(&b, beg, Min<size_t>(end - beg, sizeof(b)));
        }
    };

    template <typename Tb>
    struct TIndirectMemoryRegionAssign {
        HAS_MEMBER(Assign)
        HAS_MEMBER(assign)

        template <typename Tc>
        struct TByAssign {
            static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
                b.Assign(beg, end);
            }
        };

        template <typename Tc>
        struct TByassign {
            static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
                b.assign(beg, end);
            }
        };

        template <typename Tc>
        struct TByClearAppend {
            static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
                TClear<Tc, false>::Do(b);
                TAppendRegion<Tc>::Do(b, beg, end);
            }
        };

        template <typename Tc>
        struct TByConstruction {
            static void Do(Tc& b, const TElementType* beg, const TElementType* end) {
                b = Tc(beg, end);
            }
        };

        typedef typename TSelectType<
                        THasAssign<Tb>::Result,
                        TByAssign<Tb>,
                        typename TSelectType<
                                THasassign<Tb>::Result,
                                TByassign<Tb>,
                                typename TSelectType<
                                        TMemoryTraits<Tb>::OwnsMemory,
                                        TByClearAppend<Tb>,
                                        TByConstruction<Tb>
                                >::TResult
                        >::TResult
                >::TResult TDo;

        static void Do(Tb& b, const TElementType* beg, const TElementType* end) {
            TDo::Do(b, beg, end);
        }
    };

    typedef typename TSelectType<
                    TMemoryAccessorBase<Ta>::SimpleMemory,
                    TNoMemoryIndirectionAssign<Ta>,
                    TIndirectMemoryRegionAssign<Ta>
            >::TResult TDo;

    static void Do(Ta& b, const TElementType* beg, const TElementType* end) {
        TDo::Do(b, beg, end);
    }
};

}
}
