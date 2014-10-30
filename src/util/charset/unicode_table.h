#pragma once

#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>
#include <util/generic/static_assert.h>

namespace NUnicodeTable {
    template <class Value>
    struct TValueSelector;

    template <class Value>
    struct TValueSelector {
        typedef const Value TStored;
        typedef const Value& TValueRef;
        typedef const Value* TValuePtr;

        static inline TValueRef Get(TValuePtr val) {
            return *val;
        }
    };

    template <class Value>
    struct TValueSelector<const Value*> {
        typedef const Value TStored[];
        typedef const Value* TValueRef;
        typedef const Value* TValuePtr;

        static inline TValueRef Get(TValuePtr val) {
            return val;
        }
    };

    template <class Value>
    struct TValues {
        typedef TValueSelector<Value> TSelector;

        typedef typename TSelector::TStored TStored;
        typedef typename TSelector::TValueRef TValueRef;
        typedef typename TSelector::TValuePtr TValuePtr;

        typedef const TValuePtr* TData;

        static inline TValuePtr Get(TData table, size_t index) {
            STATIC_ASSERT(TTypeTraits<TData>::IsPointer);
            return table[index];
        }

        static inline TValueRef Get(TValuePtr val) {
            return TSelector::Get(val);
        }
    };

    template <int Shift, class TChild>
    struct TSubtable {
        typedef typename TChild::TStored TStored;
        typedef typename TChild::TValueRef TValueRef;
        typedef typename TChild::TValuePtr TValuePtr;
        typedef const typename TChild::TData* TData;

        static inline TValuePtr Get(TData table, size_t key) {
            STATIC_ASSERT(TTypeTraits<TData>::IsPointer);
            return TChild::Get(table[key >> Shift], key & ((1 << Shift) - 1));
        }

        static inline TValueRef Get(TValuePtr val) {
            return TChild::Get(val);
        }
    };

    template <class T>
    class TTable {
    private:
        typedef T TImpl;
        typedef typename TImpl::TData TData;

        const TData Data;
        const size_t MSize;

    public:
        typedef typename TImpl::TStored TStored;
        typedef typename TImpl::TValueRef TValueRef;
        typedef typename TImpl::TValuePtr TValuePtr;

    private:
        inline TValueRef GetImpl(size_t key) const {
            TValuePtr val = TImpl::Get(Data, key);

            return TImpl::Get(val);
        }

    public:
        TTable(TData data, size_t size)
            : Data(data)
            , MSize(size)
        {
            STATIC_ASSERT(TTypeTraits<TData>::IsPointer);
        }

        inline TValueRef Get(size_t key) const {
            if (key >= Size())
                ythrow yexception() << "Invalid index";

            return GetImpl(key);
        }

        inline TValueRef Get(size_t key, TValueRef value) const {
            if (key >= Size())
                return value;

            return GetImpl(key);
        }

        inline TValueRef Get(size_t key, size_t defaultKey) const {
            if (key >= Size())
                return Get(defaultKey);

            return GetImpl(key);
        }

        inline size_t Size() const {
            return MSize;
        }
    };

    const size_t UNICODE_TABLE_SHIFT = 5;
}; // namespace NUnicodeTable
