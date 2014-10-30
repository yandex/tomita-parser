#pragma once
#include <util/system/yassert.h>
#include <util/generic/algorithm.h>

#ifdef _DEBUG
template <class T>
struct TBoundCheck {
    T* Data;
    int Size;
    TBoundCheck(T* d, int s) {
        Data = d;
        Size = s;
    }
    T& operator[](int i) const {
        YASSERT(i >= 0 && i < Size);
        return Data[i];
    }
};
#endif

template <class T>
class TArray2D {
private:
    typedef T* PT;
    T* Data;
    T** PData;
    int XSize;
    int YSize;
private:
    void Copy(const TArray2D &a) {
        XSize = a.XSize;
        YSize = a.YSize;
        Create();
        for (int i = 0; i < XSize * YSize; i++)
            Data[i] = a.Data[i];
    }
    void Destroy() {
        delete[] Data;
        delete[] PData;
    }
    void Create() {
        Data = new T[XSize * YSize];
        PData = new PT[YSize];
        for (int i = 0; i < YSize; i++)
            PData[i] = Data + i * XSize;
    }
public:
    TArray2D(int xsize = 1, int ysize = 1) {
        XSize = xsize;
        YSize = ysize;
        Create();
    }
    TArray2D(const TArray2D &a) {
        Copy(a);
    }
    TArray2D& operator =(const TArray2D &a) {
        Destroy();
        Copy(a);
        return *this;
    }
    ~TArray2D() {
        Destroy();
    }
    void SetSizes(int xsize, int ysize) {
        if (XSize == xsize && YSize == ysize)
            return;
        Destroy();
        XSize = xsize;
        YSize = ysize;
        Create();
    }
    void Clear() {
        SetSizes(1,1);
    }
#ifdef _DEBUG
    TBoundCheck<T> operator[](int i) const {
        YASSERT(i >= 0 && i < YSize);
        return TBoundCheck<T>(PData[i], XSize);
    }
#else
    T* operator[](int i) const {
        YASSERT(i >= 0 && i < YSize);
        return PData[i];
    }
#endif
    int GetXSize() const {
        return XSize;
    }
    int GetYSize() const {
        return YSize;
    }
    void FillZero() {
        memset(Data, 0, sizeof(T) * XSize * YSize);
    }
    void FillEvery(const T& a) {
        for (int i = 0; i < XSize * YSize; i++)
            Data[i] = a;
    }
    void Swap(TArray2D& a) {
        std::swap(Data, a.Data);
        std::swap(PData, a.PData);
        std::swap(XSize, a.XSize);
        std::swap(YSize, a.YSize);
    }
};

template<class T>
inline bool operator ==(const TArray2D<T> &a, const TArray2D<T> &b) {
    if (a.GetXSize() != b.GetXSize() || a.GetYSize() != b.GetYSize())
        return false;
    for (int y = 0; y < a.GetYSize(); ++y) {
        for (int x = 0; x < a.GetXSize(); ++x)
            if (a[y][x] != b[y][x])
                return false;
    }
    return true;
}

template<class T>
inline bool operator !=(const TArray2D<T> &a, const TArray2D<T> &b) {
    return !(a == b);
}
