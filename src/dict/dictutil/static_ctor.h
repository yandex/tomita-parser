#pragma once

#define DECLARE_STATIC_CTOR \
    private: \
        static void StaticCtor(); \
        struct TStaticCtor { \
            TStaticCtor() { StaticCtor(); } \
        }; \
        static const TStaticCtor staticCtor;

#define DECLARE_STATIC_CTOR_AND_DTOR \
    private: \
        static void StaticCtor(); \
        static void StaticDtor(); \
        struct TStaticCtor { \
            TStaticCtor() { StaticCtor(); } \
            ~TStaticCtor() { StaticDtor(); } \
        }; \
        static const TStaticCtor staticCtor;

#define DEFINE_STATIC_CTOR(className) const className::TStaticCtor className::staticCtor
