#pragma once

#include "typelist.h"
#include "typehelpers.h"

#include <util/system/defaults.h>

template <bool, class = void>
struct TEnableIf {
};

template <class T>
struct TEnableIf<true, T> {
    typedef T TResult;
};

template <class T>
struct TPodTraits {
    enum {
        IsPod = false
    };
};

template <class T>
struct TToUnsignedInt {
    typedef T TResult;
};

template <class T>
struct TToSignedInt {
    typedef T TResult;
};

template <class T>
struct TIsClassOrUnion {
    typedef char TYes;
    typedef char (&TNo)[2];

    template <class U>
    static TYes Tester(int U::*arg);

    template <class U>
    static TNo Tester(...);

    enum {
        Result = (sizeof(Tester<T>(0)) == sizeof(TYes))
    };
};

template <class T>
class TTypeTraitsBase {
        /*
         * some helpers
         */
        template <class C>
        struct TPointerTraits {
            enum {
                IsPointer = false
            };

            typedef C TPointTo;
        };

        template <class C>
        struct TPointerTraits<C*> {
            enum {
                IsPointer = true
            };

            typedef C TPointTo;
        };

        template <class C>
        struct TReferenceTraits {
            enum {
                IsReference = false,
                IsLvalueReference = false,
                IsRvalueReference = false
            };

            typedef C TReferenceTo;
        };

        template <class C>
        struct TReferenceTraits<C&> {
            enum {
                IsReference = true,
                IsLvalueReference = true,
                IsRvalueReference = false
            };

            typedef C TReferenceTo;
        };

        template <class C>
        struct TReferenceTraits<C&&> {
            enum {
                IsReference = true,
                IsLvalueReference = false,
                IsRvalueReference = true
            };

            typedef C TReferenceTo;
        };

        template <class C>
        struct TConstTraits {
            enum {
                IsConstant = false
            };

            typedef C TResult;
        };

        template <class C>
        struct TConstTraits<const C> {
            enum {
                IsConstant = true
            };

            typedef C TResult;
        };

        template <class C>
        struct TVolatileTraits {
            enum {
                IsVolatile = false
            };

            typedef C TResult;
        };

        template <class C>
        struct TVolatileTraits<volatile C> {
            enum {
                IsVolatile = true
            };

            typedef C TResult;
        };

        template <class C>
        struct TArrayTraits {
            enum {
                IsArray = false
            };

            typedef C TResult;
        };

        template <class C, size_t n>
        struct TArrayTraits<C[n]> {
            enum {
                IsArray = true
            };

            typedef C TResult;
        };

        template <class C>
        struct TPointerToMemberTraits {
            enum {
                IsPointerToMember = false
            };
        };

        template <class C, class M>
        struct TPointerToMemberTraits<C M::*> {
            enum {
                IsPointerToMember = true
            };
        };

        template <class C, bool isConst, bool isVolatile>
        struct TApplyQualifiers {
            typedef C TResult;
        };

        template <class C>
        struct TApplyQualifiers<C, false, true> {
            typedef volatile C TResult;
        };

        template <class C>
        struct TApplyQualifiers<C, true, false> {
            typedef const C TResult;
        };

        template <class C>
        struct TApplyQualifiers<C, true, true> {
            typedef const volatile C TResult;
        };

    public:
        /*
         * qualifier traits
         */
        enum {
            IsConstant = TConstTraits<T>::IsConstant
        };

        enum {
            IsVolatile = TVolatileTraits<T>::IsVolatile
        };

        enum {
            IsPointer = TPointerTraits<T>::IsPointer
        };

        enum {
            IsPointerToMember = TPointerToMemberTraits<T>::IsPointerToMember
        };

        enum {
            IsReference = TReferenceTraits<T>::IsReference
        };

        enum {
            IsLvalueReference = TReferenceTraits<T>::IsLvalueReference
        };

        enum {
            IsRvalueReference = TReferenceTraits<T>::IsRvalueReference
        };

        /*
         * T = [const, volatile] TReferenceTo&
         */
        typedef typename TReferenceTraits<T>::TReferenceTo TReferenceTo;

        enum {
            IsConstReference = IsReference && TConstTraits<TReferenceTo>::IsConstant
        };

        enum {
            IsArray = TArrayTraits<T>::IsArray
        };

        /*
         * type without 'const' qualifier
         */
        typedef typename TConstTraits<T>::TResult TNonConst;

        /*
         * type without 'volatile' qualifier
         */
        typedef typename TVolatileTraits<T>::TResult TNonVolatile;

        /*
         * type without qualifiers
         */
        typedef typename TConstTraits<TNonVolatile>::TResult TNonQualified;

        /*
         * T = [const, volatile] TPointTo*
         */
        typedef typename TPointerTraits<T>::TPointTo TPointTo;

        /*
         * int traits
         */
        enum {
            IsSignedInt = TSignedInts::THave<TNonQualified>::Result
        };

        enum {
            IsUnsignedInt = TUnsignedInts::THave<TNonQualified>::Result
        };

        /*
         * float traits
         */
        enum {
            IsFloat = TFloats::THave<TNonQualified>::Result
        };

        /*
         * numerical traits
         */
        enum {
            IsIntegral = IsSignedInt || IsUnsignedInt
        };

        enum {
            IsArithmetic = IsIntegral || IsFloat
        };

        /*
         * traits too
         */
        enum {
            IsVoid = TSameType<TNonQualified, void>::Result
        };

        enum {
            IsFundamental = IsArithmetic || IsVoid
        };

        enum {
            IsClassType = TIsClassOrUnion<TNonQualified>::Result
        };

        enum {
            IsPrimitive = !IsClassType
        };

        enum {
            IsEnum = IsPrimitive
                && !(IsArithmetic || IsPointer || IsPointerToMember || IsVoid || IsReference || IsArray)
        };

        enum {
            IsPod = TPodTraits<TNonQualified>::IsPod || IsPrimitive
        };

        /*
         * signed type to unsigned type conversion
         */
        typedef typename TToUnsignedInt<TNonQualified>::TResult TUnsignedTypeHelper;
        typedef typename TApplyQualifiers<TUnsignedTypeHelper, IsConstant, IsVolatile>::TResult TUnsigned;

        /*
         * unsigned type to signed type conversion
         */
        typedef typename TToSignedInt<TNonQualified>::TResult TSignedTypeHelper;
        typedef typename TApplyQualifiers<TSignedTypeHelper, IsConstant, IsVolatile>::TResult TSigned;
};

template <class T>
class TTypeTraits: public TTypeTraitsBase<T> {
    typedef TTypeTraitsBase<T> TBase;
public:
    /*
     * can be effectively passed to function as value
     */
    enum EIsValueType {
        //TODO - fix all Out<enum> cases
        IsValueType = (TBase::IsPrimitive || TBase::IsReference || (TBase::IsPod && (sizeof(T) <= sizeof(void*)))) && !TBase::IsEnum
    };

    /*
     * can be used in function templates for effective parameters passing
     */
    typedef typename TSelectType<IsValueType, T, const typename TBase::TReferenceTo&>::TResult TFuncParam;
};

template <>
class TTypeTraits<void>: public TTypeTraitsBase<void> {
};

template <class T>
struct TRemoveReference {
    typedef typename TTypeTraits<T>::TReferenceTo TType;
};

template <typename T>
struct TAddRValueReference {
    typedef T&& TType;
};

template <>
struct TAddRValueReference<void> {
    typedef void TType;
};

template <typename T>
typename TAddRValueReference<T>::TType DeclVal();

#define DECLARE_PODTYPE(type) \
    template <>\
    struct TPodTraits<type> {\
        enum {\
            IsPod = true\
        };\
    };

#define DEF_INT_CVT(a)\
    template <>\
    struct TToUnsignedInt<a> {\
        typedef unsigned a TResult;\
    };\
    \
    template <>\
    struct TToSignedInt<unsigned a> {\
        typedef a TResult;\
    }

DEF_INT_CVT(char);
DEF_INT_CVT(short);
DEF_INT_CVT(int);
DEF_INT_CVT(long);
DEF_INT_CVT(long long);

#undef DEF_INT_CVT

#define HAS_MEMBER_NAMED(method, name) \
    template <class T>\
    struct TClassHas ## name {\
        struct TBase {\
            void method();\
        };\
        class THelper: public T, public TBase {\
            public:\
                template <class T1>\
                inline THelper(const T1& = T1()) {\
                }\
        };\
        template <class T1, T1 val>\
        class TChecker {};\
        struct TNo {\
            char ch;\
        };\
        struct TYes {\
            char arr[2];\
        };\
        template <class T1>\
        static TNo CheckMember(T1*, TChecker<void (TBase::*)(), &T1::method>* = 0);\
        static TYes CheckMember(...);\
        enum {\
            Result = (sizeof(TYes) == sizeof(CheckMember((THelper*)0)))\
        };\
    };\
    template <class T, bool isClassType>\
    struct TBaseHas ## name {\
        enum {\
            Result = false\
        };\
    };\
    template <class T>\
    struct TBaseHas ## name<T, true>: public TClassHas ## name<T> {\
    };\
    template <class T>\
    struct THas ## name: public TBaseHas ## name<T, TTypeTraits<T>::IsClassType> {\
    };

#define HAS_MEMBER(name) HAS_MEMBER_NAMED(name, name)
