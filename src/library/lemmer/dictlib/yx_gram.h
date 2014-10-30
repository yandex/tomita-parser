#pragma once

/*
 * (C)++ iseg
 */

#include "yx_gram_enum.h"
#include "tgrammar_processing.h"

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/string/traits.h>
#include <util/system/defaults.h>

/*
проверка 2 списков характеристик на несовместимость
ПРЕДУСЛОВИЕ: они должны быть отсортированы
*/
int incompatible(char *, char *);

bool grammar_byk_from_rus(const char* grammar, char* outbuf, int bufsize);
// 14-09-96 uniq
int uniqsort(char *str, int len);

const int MAX_FORMA_NUM = 16000;
const int MIN_STEM_LEN = 2;
const int MIN_BASTARDED_WORD_LEN = 3;
const int MIN_BASTARDS_NOTSUBST_WORD_LEN = 5;

int common_grammar(Stroka gram[], int size, char *outgram);

Stroka sprint_grammar(const char* grammar, TGramClass hidden_classes = 0, bool russ = false);

typedef int POS_SET;
inline POS_SET POS_BIT(unsigned char gPOS) {
    assert(gPOS > gBefore);
    assert(gPOS <= sizeof(POS_SET) * CHAR_BIT + gBefore);
    return 1 << (gPOS - (gBefore + 1));
}

extern const POS_SET Productive_PartsOfSpeech;
extern const POS_SET Preffixable_PartsOfSpeech;
extern const POS_SET ShortProductive_PartsOfSpeech;

// Mapping of grammeme codes to names and vice versa. The public interface
// includes only static methods for lookup and formatting of grammar info
class TGrammarIndex {
private:
    typedef yhash<TStringBuf, TGrammar> TStrokaGrammarMap;
    typedef yhash<TWtringBuf, TGrammar> TWtrokaGrammarMap;

    TStrokaGrammarMap CodeByNameIndex;
    TWtrokaGrammarMap WCodeByNameIndex;

    yvector<Stroka> YandexNameByCodeIndex;
    yvector<Stroka> Utf8NameByCodeIndex;
    yvector<Stroka> LatinNameByCodeIndex;

    yvector<Wtroka> WNameByCodeIndex;
    yvector<Wtroka> WLatinNameByCodeIndex;

    yvector<TGramClass> ClassByCodeIndex;

    typedef yhash<TGramClass, yvector<TGrammar> > TClassToCodes;
    TClassToCodes CodesByClassIndex;

    // Singleton object to hold all indexes
    static const TGrammarIndex TheIndex;

private:
    // See constructor in ccl.cpp
    TGrammarIndex();
    void RegisterGrammeme(TGrammar code, const char* name, const char* latin_name, TGramClass grClass);

    template<typename TChr>
    inline static const TStringBufImpl<TChr> GetNameFromIndex (TGrammar code, const yvector<typename TCharStringTraits<TChr>::TString>& index) {
        static const TChr empty[] = {0};
        size_t n = static_cast<size_t> (code);
        if (n >= index.size())
            return TStringBufImpl<TChr>(empty, size_t(0));
        return index[n];
    }

    template<typename TChr>
    inline static TGrammar GetCodeImpl(const TStringBufImpl<TChr>& name, const yhash<TStringBufImpl<TChr>, TGrammar>& index) {
        if (name.empty())
            return gInvalid;

        typename yhash<TStringBufImpl<TChr>, TGrammar>::const_iterator it = index.find(name);
        if (it == index.end())
            return gInvalid;
        return (*it).second;
    }

public:
    inline static TGrammar GetCode(const TWtringBuf& name) {
        return GetCodeImpl(name, TheIndex.WCodeByNameIndex);
    }

    inline static TGrammar GetCode(const TStringBuf& name) {
        return GetCodeImpl(name, TheIndex.CodeByNameIndex);
    }

    inline static const char* GetName(TGrammar code) {
        return GetNameFromIndex<char>(code, TheIndex.YandexNameByCodeIndex).c_str();
    }

    inline static const char* GetUtf8Name(TGrammar code) {
        return GetNameFromIndex<char>(code, TheIndex.Utf8NameByCodeIndex).c_str();
    }

    inline static const char* GetLatinName(TGrammar code) {
        return GetNameFromIndex<char>(code, TheIndex.LatinNameByCodeIndex).c_str();
    }

    inline static const TChar* GetWName(TGrammar code) {
        return GetNameFromIndex<TChar>(code, TheIndex.WNameByCodeIndex).c_str();
    }

    inline static const TChar* GetWLatinName(TGrammar code) {
        return GetNameFromIndex<TChar>(code, TheIndex.WLatinNameByCodeIndex).c_str();
    }


    inline static TGramClass GetClass(TGrammar code) {
        size_t n = static_cast<size_t> (code);
        const yvector<TGramClass>& index = TheIndex.ClassByCodeIndex;

        if (n >= index.size())
            return 0;
        return index[n];
    }

    static Stroka& Format(Stroka& s, const TGrammar* grammar, const char* separator = ",") {
        s.remove();
        while (*grammar) {
            s += GetName(*grammar);
            grammar++;
            if (*grammar)
                s += separator;
        }
        return s;
    }

    static const yvector<TGrammar>& GetCodes(TGramClass grclass) {
        static yvector<TGrammar> empty;
        TClassToCodes::const_iterator i = TheIndex.CodesByClassIndex.find(grclass);
        if (i == TheIndex.CodesByClassIndex.end())
            return empty;
        return i->second;
    }
};

inline TGramClass GetClass(char c) {return TGrammarIndex::GetClass(NTGrammarProcessing::ch2tg(c));}
inline void SkipClass(const char* &p, TGramClass cl)  {
    while (GetClass(*p)&cl)
        p++;
}
