#pragma once

#include <util/generic/ptr.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

template<typename T>
void ComputePrefixFunction(const T* begin, const T* end, ssize_t** result) {
    if (begin == end)
        ythrow yexception() << "empty pattern";
    ssize_t len = end - begin;
    TArrayHolder<ssize_t> resultHolder(new ssize_t[len + 1]);
    ssize_t i = 0;
    ssize_t j = -1;
    resultHolder[0] = -1;
    while (i < len) {
        while ((j >= 0) && (begin[j] != begin[i]))
            j = resultHolder[j];
        ++i;
        ++j;
        YASSERT(i >= 0);
        YASSERT(j >= 0);
        YASSERT(j < len);
        if ((i < len) && (begin[i] == begin[j]))
            resultHolder[i] = resultHolder[j];
        else
            resultHolder[i] = j;
    }
    *result = resultHolder.Release();
}

class TKMPMatcher {
private:
    TArrayHolder<ssize_t> PrefixFunction;
    Stroka Pattern;

    void ComputePrefixFunction();

public:
    TKMPMatcher(const char* patternBegin, const char* patternEnd);
    TKMPMatcher(const Stroka& pattern);

    bool SubStr(const char* begin, const char* end, const char*& result) const {
        YASSERT(begin <= end);
        ssize_t m = +Pattern;
        ssize_t n = end - begin;
        ssize_t i, j;
        for (i = 0, j = 0; (i < n) && (j < m); ++i, ++j) {
            while ((j >= 0) && (Pattern[j] != begin[i]))
                j = PrefixFunction[j];
        }
        if (j == m) {
            result = begin + i - m;
            return true;
        } else {
            return false;
        }
    }
};

template<typename T>
class TKMPStreamMatcher {
public:
    class ICallback {
    public:
        virtual void OnMatch(const T* begin, const T* end) = 0;
        virtual ~ICallback() {}
    };

private:
    ICallback* Callback;
    TArrayHolder<ssize_t> PrefixFunction;
    typedef yvector<T> TTVector;
    TTVector Pattern;
    ssize_t State;
    TTVector Candidate;

public:
    TKMPStreamMatcher(const T* patternBegin, const T* patternEnd, ICallback* callback)
        : Callback(callback)
        , Pattern(patternBegin, patternEnd)
        , State(0)
        , Candidate(+Pattern)
    {
        ssize_t* pf;
        ComputePrefixFunction(patternBegin, patternEnd, &pf);
        PrefixFunction.Reset(pf);
    }

    void Push(const T& symbol) {
        while ((State >= 0) && (Pattern[State] != symbol)) {
            YASSERT(State <= (ssize_t)+Pattern);
            State = PrefixFunction[State];
            YASSERT(State <= (ssize_t)+Pattern);
        }
        if (State >= 0)
            Candidate[State] = symbol;
        ++State;
        if (State == (ssize_t)+Pattern) {
            Callback->OnMatch(Candidate.begin(), Candidate.end());
            State = 0;
        }
    }

    void Clear() {
        State = 0;
    }
};
