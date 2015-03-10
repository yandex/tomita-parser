#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "word.h"
#include <FactExtract/Parser/afdocparser/common/wordsequence.h>

const int g_Exp1000Count = 4;
extern const ui64 g_Exp1000[g_Exp1000Count];

const char g_strNumberForm[100]  = "$$$nf";
const char g_strNumberLemma[100]  = "$$$nl";

const char g_strNumberOld[100]  = "%017.0f_%02.0f";

enum ENumberOptions { DoNotBuildNumbers = 0, BuildAllNumbers, Only3BucksnfAndAt, Only3Bucksnf };

class CNumber: public CWordSequence
{
public:
    CNumber();
    bool Is1000Exp()
    {
        if (m_Numbers.size() != 1)
            return false;
        return  (m_Numbers[0] == 1000) ||
                (m_Numbers[0] == 1000000) ||
                (m_Numbers[0] == 1000000000LL) ||
                (m_Numbers[0] == 1000000000000LL);
    }
    static Stroka  BuildNumberLemma(double num);
    static Stroka  BuildNumberForm(double num);
    void BuildString(yvector<Stroka>& strings, ENumberOptions opt = BuildAllNumbers);
    static Stroka BuildSingleString(double num);
    void BuildStringVariants(yvector<Stroka>& strings, double num);
    static Stroka ConvertToString(double num, Stroka pref);

public:
    yvector<double> m_Numbers;
    int m_iMainWord;
    bool m_bSingle; //то есть не может быть продолжено, типа 3-тысячный
};

class CNumberProcessor
{
public:
    CNumberProcessor(yvector<TSharedPtr<CWord> >&  words, yvector<CNumber>& numbers, CWordSequence::TSharedVector& wordSequences)
        : m_words(words), m_numbers(numbers), m_wordSequences(wordSequences)
    {
        CWordSequence::Sort(m_wordSequences);
    };

    void Run();

    template <typename TStr>
    static double Atod(const TStr& strInteger, const TStr& strFractional) {
        if (strFractional.empty())
            return static_cast<double>(::FromString<i64>(strInteger));

        typedef typename TStr::char_type TChar;
        TStringBufImpl<TChar> fract = TStringBufImpl<TChar>(strFractional).SubStr(0, 10);
        return ::FromString<i64>(strInteger)
            + ::FromString<i64>(fract)/pow((double)10, (int)fract.size());
    }

protected:
    size_t UniteWordsDigitSequence(size_t ii);
    size_t UniteDigitThousandSequence(size_t ii);
    size_t UniteComplexDigitSequence(size_t ii);
    size_t EnlargeSingleNumberWithSimilar(size_t ii);
    size_t EnlargeSingleNumberWithHalf(size_t ii);
    CWord* GetFirstWord(CNumber& num);
    CWord* GetLastWord(CNumber& num);
    CWord* GetWord(int num);
    void AddAsVariants();
    void AddToWordSequences();

    bool IsFree(int iW);

protected:
    yvector<TSharedPtr<CWord> >& m_words;
    yvector<CNumber>& m_numbers;
    CWordSequence::TSharedVector& m_wordSequences;
};
