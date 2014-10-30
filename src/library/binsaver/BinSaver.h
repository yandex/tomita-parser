#pragma once

#include "BufferedIO.h"
#include "BasicFactory.h"

#include <library/2darray/2d_array.h>

#include <util/generic/hash_set.h>
#include <util/generic/array.h>
#include <util/generic/buffer.h>
#include <util/generic/list.h>
#include <util/generic/maybe.h>
#include <util/generic/static_assert.h>
#include <util/memory/blob.h>
#include <util/draft/variant.h>

#include <stlport/list>
#include <stlport/string>
#include <stlport/bitset>

#define CStructureSaver IBinSaver

#define ZDATA_(a)
#define ZDATA
#define ZPARENT(a)
#define ZEND
#define ZSKIP
#define ZONSERIALIZE


#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif

//template <class T> class TArray2D;
template<int n> struct SInt2Type {};

enum ESaverMode
{
    SAVER_MODE_READ        = 1,
    SAVER_MODE_WRITE    = 2,
    SAVER_MODE_WRITE_COMPRESSED = 3,
};

//////////////////////////////////////////////////////////////////////////
struct IBinSaver
{
public:
    typedef unsigned char chunk_id;

private:
    template<class T>
        void CallObjectSerialize(T *p, ...) {
            p->T::operator&(*this);
        }
    template<class T>
        void CallObjectSerialize(T *p, SInt2Type<1> *) {
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
            // In MSVC __has_trivial_copy returns false to enums, primitive types and arrays.
            STATIC_CHECK(__has_trivial_copy(T), Class_Is_Non_Trivial_Copyable_You_Must_Define_OperatorAnd_See_JIRA_ARC_1228);
#endif
            DataChunk(p, sizeof(T));
        }
    //
    // vector
    template <class T, class TA> void DoVector(yvector<T,TA> &data) {
        int i, nSize;
        if (IsReading()) {
            data.clear();
            Add(2, &nSize);
            data.resize(nSize);
        } else {
            nSize = data.ysize();
            Add(2, &nSize);
        }
        for (i = 0; i < nSize; i++)
            Add(1, &data[i]);
    }
    template <class T, class TA> void DoDataVector(yvector<T,TA> &data) {
        int nSize = data.ysize();
        Add(1, &nSize);
        if (IsReading()) {
            data.clear();
            data.resize(nSize);
        }
        if (nSize > 0)
            DataChunk(&data[0], sizeof(T) * nSize);
    }
    // hash_map, yhash
    template <class T1,class T2,class T3> void DoHashMap(yhash_map<T1,T2,T3> &data) {
        if (IsReading()) {
            data.clear();
            int nSize, i;
            Add(3, &nSize);
            yvector<T1> indices;
            indices.resize(nSize);
            for (i = 0; i < nSize; ++i)
                Add(1, &indices[i]);
            for (i = 0; i < nSize; ++i)
                Add(2, &data[ indices[i] ]);
        } else {
            int nSize = data.ysize();
            Add(3, &nSize);

            yvector<T1> indices;
            indices.resize(nSize);
            int i = 1;
            for (typename yhash_map<T1,T2,T3>::iterator pos = data.begin(); pos != data.end(); ++pos, ++i)
                indices[ nSize - i ] = pos->first;
            for (i = 0; i < nSize; ++i)
                Add(1, &indices[i]);
            for (i = 0; i < nSize; ++i)
                Add(2, &data[ indices[i] ]);
        }
    }
    // hash_multimap
    template <class T1,class T2,class T3> void DoHashMultiMap(yhash_multimap<T1,T2,T3> &data) {
        if (IsReading()) {
            data.clear();
            int nSize, i;
            Add(3, &nSize);
            yvector<T1> indices;
            indices.resize(nSize);
            for (i = 0; i < nSize; ++i)
                Add(1, &indices[i]);
            for (i = 0; i < nSize; ++i) {
                std::pair<T1, T2> valToInsert;
                valToInsert.first = indices[i];
                Add(2, &valToInsert.second);
                data.insert(valToInsert);
            }
        } else {
            int nSize = data.ysize();
            Add(3, &nSize);
            for (typename yhash_map<T1,T2,T3>::iterator pos = data.begin(); pos != data.end(); ++pos)
                Add(1,(T1*)(&pos->first));
            for (typename yhash_map<T1,T2,T3>::iterator pos = data.begin(); pos != data.end(); ++pos)
                Add(2, &pos->second);
        }
    }
    // hash_set
    template <class T> void DoHashSet(T &data) {
        if (IsReading()) {
            data.clear();
            int nSize;
            Add(2, &nSize);
            for (int i = 0; i < nSize; ++i) {
                typename T::value_type member;
                Add(1, &member);
                data.insert(member);
            }
        } else {
            int nSize = (int)data.size();
            Add(2, &nSize);
            for (typename T::const_iterator pos = data.begin(); pos != data.end(); ++pos) {
                typename T::value_type member(*pos);
                Add(1, &member);
            }
        }
    }
    // 2D array
    template<class T> void Do2DArray(TArray2D<T> &a) {
        int nXSize = a.GetXSize(), nYSize = a.GetYSize();
        Add(1, &nXSize);
        Add(2, &nYSize);
        if (IsReading())
            a.SetSizes(nXSize, nYSize);
        for (int i = 0; i < nXSize * nYSize; i++)
            Add(3, &a[i/nXSize][i%nXSize]);
    }
    template<class T> void Do2DArrayData(TArray2D<T> &a) {
        int nXSize = a.GetXSize(), nYSize = a.GetYSize();
        Add(1, &nXSize);
        Add(2, &nYSize);
        if (IsReading())
            a.SetSizes(nXSize, nYSize);
        if (nXSize * nYSize > 0)
            DataChunk(&a[0][0], sizeof(T) * nXSize * nYSize);
    }
    // strings
    template<class TStringType> void DataChunkStr(TStringType &data, int elemSize)
    {
        if (bRead) {
            int nCount = 0;
            File.Read(&nCount, sizeof(int));
            data.resize(nCount);
            if (nCount)
                File.Read(data.begin(), nCount * elemSize);
        } else {
            int nCount = (int)data.size();
            File.Write(&nCount, sizeof(int));
            File.Write(data.c_str(), nCount * elemSize);
        }
    }
    void DataChunkString(std::string &data)
    {
        DataChunkStr(data, sizeof(char));
    }
    void DataChunkStroka(Stroka &data)
    {
        DataChunkStr(data, sizeof(Stroka::char_type));
    }
    void DataChunkWtroka(Wtroka &data)
    {
        DataChunkStr(data, sizeof(Wtroka::char_type));
    }

    void DataChunk(void *pData, size_t nSize) // sizeof(T) * int(size)
    {
        int chunkSize = 1 << 30;
        for (ui64 offset = 0; offset < nSize; offset += chunkSize) {
            void *ptr = (char *)pData + offset;
            int size = offset + chunkSize < nSize ? chunkSize : int(nSize - offset);
            if (bRead)
                File.Read(ptr, size);
            else
                File.Write(ptr, size);
        }
    }

    // storing/loading pointers to objects
    void StoreObject(TObjectBase *pObject);
    TObjectBase* LoadObject();

    bool bRead;
    TBufferedStream<> File;
    // maps objects addresses during save(first) to addresses during load(second) - during loading
    // or serves as a sign that some object has been already stored - during storing
    bool StableOutput;

    typedef yhash_map<void*, ui32, TPtrHash> PtrIdHash;
    TAutoPtr<PtrIdHash> PtrIds;

    typedef yhash_map<ui64, TPtr<TObjectBase> > CObjectsHash;
    TAutoPtr<CObjectsHash> Objects;

    yvector<TObjectBase*> ObjectQueue;
public:
    bool IsReading() { return bRead; }
    void AddRawData(const chunk_id, void *pData, int nSize) { DataChunk(pData, nSize); }

    // return value of Add() is used to detect specialized serializer
    template<class T>
        char Add(const chunk_id, T *p)
    {
        const int N_HAS_SERIALIZE_TEST = sizeof((*p)&(*this));
        SInt2Type<N_HAS_SERIALIZE_TEST> separator;
        CallObjectSerialize(p, &separator);
        return 0;
    }
    int Add(const chunk_id, std::string *pStr)
    {
        DataChunkString(*pStr);
        return 0;
    }
    int Add(const chunk_id, Stroka *pStr)
    {
        DataChunkStroka(*pStr);
        return 0;
    }
    int Add(const chunk_id, Wtroka *pStr)
    {
        DataChunkWtroka(*pStr);
        return 0;
    }
    int Add(const chunk_id, TBlob *blob)
    {
        if (bRead) {
            ui64 size = 0;
            File.Read(&size, sizeof(size));
            TBuffer buffer{size};
            if (size > 0)
                File.Read(buffer.Data(), buffer.Size());
            blob->FromBuffer(buffer);
        } else {
            const ui64 size = blob->Size();
            File.Write(&size, sizeof(size));
            File.Write(blob->Data(), blob->Size());
        }
        return 0;
    }
    template<class T1, class TA>
        int Add(const chunk_id, yvector<T1,TA> *pVec)
    {
        if (HasNonTrivialSerializer((T1*)0))
            DoVector(*pVec);
        else
            DoDataVector(*pVec);
        return 0;
    }
    template<class T1, class T2, class T3>
        int Add(const chunk_id, yhash_map<T1,T2,T3> *pHash)
    {
        DoHashMap(*pHash);
        return 0;
    }
    template<class T1, class T2, class T3>
        int Add(const chunk_id, yhash_multimap<T1,T2,T3> *pHash)
    {
        DoHashMultiMap(*pHash);
        return 0;
    }
    template<class T1, class T2, class T3, class T4>
        int Add(const chunk_id, yhash_set<T1,T2,T3,T4> *pHash)
    {
        DoHashSet(*pHash);
        return 0;
    }
    template<class T1, class T2, class T3, class T4, class T5>
        int Add(const chunk_id, yhash<T1,T2,T3,T4,T5> *pHash)
    {
        DoHashMap(*pHash);
        return 0;
    }

    template<class T1>
        int Add(const chunk_id, TArray2D<T1> *pArr)
    {
        if (HasNonTrivialSerializer((T1*)0))
            Do2DArray(*pArr);
        else
            Do2DArrayData(*pArr);
        return 0;
    }
    template<class T1>
        int Add(const chunk_id, ylist<T1> *pList)
    {
        ylist<T1> &data = *pList;
        if (IsReading())
        {
            int nSize;
            Add(2, &nSize);
            data.clear();
            data.insert(data.begin(), nSize, T1());
        }
        else
        {
            int nSize = data.size();
            Add(2, &nSize);
        }
        int i = 1;
        for (typename ylist<T1>::iterator k = data.begin(); k != data.end(); ++k, ++i)
            Add(i + 2, &(*k));
        return 0;
    }
    template <class T1, class T2>
        int Add(const chunk_id, std::pair<T1, T2> *pData)
    {
        Add(1, &(pData->first));
        Add(2, &(pData->second));
        return 0;
    }

    template <class T1, class T2>
        int Add(const chunk_id, TPair<T1, T2> *pData)
    {
        Add(1, &(pData->first));
        Add(2, &(pData->second));
        return 0;
    }

    template <class T1, size_t N>
    int Add(const chunk_id, yarray<T1, N> *pData)
    {
        if (HasNonTrivialSerializer((T1*)0)) {
            for (size_t i = 0; i < N; ++i)
                Add(1, &(*pData)[i]);
        } else {
            DataChunk((void*)pData->data(), pData->ysize()*sizeof(T1));
        }
        return 0;
    }

    template <size_t N>
    int Add(const chunk_id, std::bitset<N> *pData)
    {
        if (IsReading()) {
            std::string s;
            Add(1, &s);
            *pData = std::bitset<N>(s);
        } else {
            std::string s = pData->template to_string< char, std::char_traits<char>, std::allocator<char> >();
            Add(1, &s);
        }
        return 0;
    }

    template <class T1, class T2>
    int Add(const chunk_id, TVariant<T1, T2> *pData)
    {
        ui8 isSecond;
        if (!IsReading())
            isSecond = (ui8)pData->IsSecond();

        Add(1, &isSecond);

        if (IsReading()) {
            if (isSecond) {
                T2 t;
                Add(1, &t);
                *pData = TVariant<T1,T2>(t);
            } else {
                T1 t;
                Add(1, &t);
                *pData = TVariant<T1,T2>(t);
            }
        } else {
            if (isSecond)
                Add(1, pData->template Get<T2>());
            else
                Add(1, pData->template Get<T1>());
        }
        return 0;
    }

    void AddPolymorphicBase(chunk_id, TObjectBase *pObject) {
        (*pObject) & (*this);
    }
    template <class T1, class T2>
        void DoPtr(TPtrBase<T1,T2> *pData) {
        if (IsReading())
            pData->Set(CastToUserObject(LoadObject(), (T1*)0));
        else
            StoreObject(pData->GetBarePtr());
    }
    template <class T>
        int Add(const chunk_id, TMaybe<T> *pData)
    {
        TMaybe<T>& data = *pData;
        if (IsReading()) {
            bool defined;
            Add(1, &defined);
            if (defined) {
                data = T();
                Add(2, data.Get());
            }
        } else {
            bool defined = data.Defined();
            Add(1, &defined);
            if (defined) {
                Add(2, data.Get());
            }
        }
        return 0;
    }

    template<class T> static bool HasNonTrivialSerializer(T *ptr)
    {
        ptr = ptr;
        return sizeof(((IBinSaver*)0)->Add(0, ptr)) != 1 || sizeof((*ptr)&(*(IBinSaver*)0)) != 1;
    }

public:
    IBinSaver(IBinaryStream &stream, bool _bRead, bool stableOutput = false) : bRead(_bRead), File(_bRead, stream), StableOutput(stableOutput) {}
    virtual ~IBinSaver();
    bool IsValid() const { return File.IsValid(); }
};

template <class T>
inline char operator&(T&, IBinSaver&) {
    return 0;
}

// realisation of forward declared serialisation operator
template< class TUserObj, class TRef>
int TPtrBase<TUserObj,TRef>::operator&(IBinSaver &f) {
    f.DoPtr(this);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern TClassFactory<TObjectBase> *pSaverClasses;
void StartRegisterSaveload();

template<class TReg>
struct TRegisterSaveLoadType {
    TRegisterSaveLoadType(int num) {
        StartRegisterSaveload();
        pSaverClasses->RegisterType(num, TReg::NewSaveLoadNullItem, (TReg*)0);
    }
};

#define REGISTER_SAVELOAD_CLASS(N, name)\
    BASIC_REGISTER_CLASS(name)\
    static TRegisterSaveLoadType<name> init##name##N(N);

// using TObj/TRef on forward declared templ class will not work
// but multiple registration with same id is allowed
#define REGISTER_SAVELOAD_TEMPL1_CLASS(N, className, T)\
    static TRegisterSaveLoadType< className<T> > init##className##T##N(N);

#define REGISTER_SAVELOAD_TEMPL2_CLASS(N, className, T1, T2)\
    typedef className<T1,T2> temp##className##T1##_##T2##temp;\
    static TRegisterSaveLoadType< className<T1,T2> > init##className##T1##_##T2##N(N);

#define REGISTER_SAVELOAD_TEMPL3_CLASS(N, className, T1, T2, T3)\
    typedef className<T1,T2,T3> temp##className##T1##_##T2##_##T3##temp;\
    static TRegisterSaveLoadType< className<T1,T2,T3> > init##className##T1##_##T2##_##T3##N(N);

#define REGISTER_SAVELOAD_NM_CLASS(N, nmspace, className)\
    BASIC_REGISTER_CLASS(nmspace::className)\
    static TRegisterSaveLoadType<nmspace::className> init_##nmspace##_##name##N(N);


#define REGISTER_SAVELOAD_CLASS_NAME(N, cls, name) \
    BASIC_REGISTER_CLASS(cls) \
    static TRegisterSaveLoadType< cls > init ## name ## N(N)

#define REGISTER_SAVELOAD_CLASS_NS_PREF(N, cls, ns, pref) \
    REGISTER_SAVELOAD_CLASS_NAME(N, ns :: cls, _ ## pref ## _ ## cls)
