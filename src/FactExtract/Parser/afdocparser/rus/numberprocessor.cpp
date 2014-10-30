
#include "numberprocessor.h"
#include <math.h>
#include <util/string/cast.h>

const ui64 g_Exp1000[g_Exp1000Count] = {1000000000000ULL, 1000000000ULL, 1000000ULL, 1000ULL};

CNumber::CNumber()
{
    m_iMainWord = -1;
    m_bSingle = false;
}

Stroka CNumber::BuildSingleString(double num)
{
    double integer, fractional;
    fractional = modf(num, &integer);
    Stroka str = Sprintf(g_strNumberOld, integer, fractional*100);
    str = str.substr(0, 20);
    return str;
}

Stroka CNumber::ConvertToString(double num, Stroka pref)
{
    double integer, fractional;
    double m = (num == 0) ? 0 : log10(num);
    m = floor(m) + 51;
    fractional = modf(num, &integer);
    Stroka strM = Sprintf("%03.0f#", m);

    size_t i = 0;

    if (fractional > 0) {
        if (m < 50)
            i = 50 - (int)m;
        if (i < 8)
            i = 8;
    }

    num *= pow((double)10, (int)i);

    if (num > 10) {
        while (((i64)num % 10) == 0)
            num /= 10;
    }

    Stroka s = Sprintf("%.f", num);

    // Осторожно, double
    while ((i = s.rfind('0')) != Stroka::npos)
        if (i + 1 == s.size())
            s = s.substr(1);
        else
            break;
    return pref + strM + s;

}

Stroka CNumber::BuildNumberLemma(double num)
{
    return ConvertToString(num, g_strNumberLemma);
}

Stroka CNumber::BuildNumberForm(double num)
{
    double integer;
    double m = (num == 0) ? 0 : log10(num);
    m = floor(m) + 1;

    if (m > 2) {
        m -= 2;
    } else
        m = 0;

    modf(num/pow((double)10, m), &integer);
    integer *= pow((double)10, m);

    if (integer == 0)
        integer = num; // если num < 0

    return ConvertToString(integer, g_strNumberForm);
}

void CNumber::BuildString(yvector<Stroka>& strings, ENumberOptions opt)
{
    if (opt == DoNotBuildNumbers)
        return;
    for (size_t i = 0; i < m_Numbers.size(); i++) {
        if (opt == BuildAllNumbers)
            strings.push_back(BuildNumberLemma(m_Numbers[i]));
        strings.push_back(BuildNumberForm(m_Numbers[i]));
    }
    if (opt != Only3Bucksnf)
        strings.push_back(g_strNumberNonTerminal);
}

bool CNumberProcessor::IsFree(int iW)
{
    for (size_t i = 0; i < m_wordSequences.size(); i++) {
        if (m_wordSequences[i]->Contains(iW))
            return false;
        if (m_wordSequences[i]->FirstWord() > iW)
            return true;
    }
    return true;
}

void CNumberProcessor::Run(void)
{
    // разбирает последовательности типа "семь с половиной"
    for (size_t i = 0; i < m_numbers.size(); i++)
        i = EnlargeSingleNumberWithHalf(i);

    // объединяем последовательности типа "двадцать пять"
    for (size_t i = 0; i < m_numbers.size(); i++) {
        size_t r = UniteWordsDigitSequence(i);
        if (r > i)
            m_numbers.erase(m_numbers.begin() + i + 1, m_numbers.begin() + r);
    }

    // объединяем последовательности типа "(двадцать пять) тысяч"
    for (size_t i = 0; i < m_numbers.size(); i++) {
        size_t r = UniteDigitThousandSequence(i);
        if (r > i)
            m_numbers.erase(m_numbers.begin() + i + 1, m_numbers.begin() + r);
    }

    // объединяем последовательности типа "((тридцать три) миллиона) ((двадцать пять) тысяч)"
    for (size_t i = 0; i < m_numbers.size(); i++) {
        size_t r = UniteComplexDigitSequence(i);
        if (r > i)
            m_numbers.erase(m_numbers.begin() + i + 1, m_numbers.begin() + r);
    }

    // разбирает последовательности типа "5 или 6 миллионов"
    for (size_t i = 0; i < m_numbers.size(); i++)
        i = EnlargeSingleNumberWithSimilar(i);

    AddAsVariants();

    AddToWordSequences();

}

void CNumberProcessor::AddToWordSequences()
{
    for (size_t i = 0; i < m_numbers.size(); i++) {
        THolder<CNumber> num(new CNumber(m_numbers[i]));
        num->SetMainWord(num->LastWord(),0);
        num->PutWSType(NumberWS);
        m_wordSequences.push_back(num.Release());
    }
    CWordSequence::Sort(m_wordSequences);
}

void CNumberProcessor::AddAsVariants()
{
    for (size_t i = 0; i < m_numbers.size(); i++) {
        CNumber& num = m_numbers[i];
        CWord* pWord = GetFirstWord(num);

        yvector<Stroka> strs;
        num.BuildString(strs, BuildAllNumbers);

        for (size_t j = 0; j < strs.size(); j++) {
            CHomonym* pHom = new CHomonym(TMorph::GetMainLanguage());
            pHom->InitText(UTF8ToWide(strs[j]), true);
            pWord->AddVariant(pHom);
        }

    }
}

CWord* CNumberProcessor::GetWord(int num)
{
    YASSERT(num >= 0 && (size_t)num < m_words.size());
    return m_words[num].Get();
}

CWord* CNumberProcessor::GetFirstWord(CNumber& num)
{
    return GetWord(num.FirstWord());
}

CWord* CNumberProcessor::GetLastWord(CNumber& num)
{
    return GetWord(num.LastWord());
}

size_t CNumberProcessor::UniteWordsDigitSequence(size_t ii)
{
    double prev = 0;
    size_t i = ii;
    for (; (i <  m_numbers.size()) && (i < ii + 3); i++) {
        CWord* pWord = GetFirstWord(m_numbers[i]);
        if (pWord->m_typ != Word)
            break;
        if (m_numbers[i].m_Numbers.size() != 1)
            break;
        if (m_numbers[i].Size() != 1)
            break;

        if (i > ii)
            if (m_numbers[i].FirstWord() != (m_numbers[i-1].LastWord() + 1))
                break;

        double num = m_numbers[i].m_Numbers[0];
        if (num >= 1000)
            break;

        if ((num >= 100) && (num < 1000)) {
            if (((ui64)num % 100) != 0)
                break;
            if (i != ii)
                break;
        }

        if ((num >= 10) && (num < 20)) {
            if (((ui64)prev % 100) != 0)
                break;
        }

        if ((num >= 20) && (num < 100)) {
            if (((ui64)num % 10) != 0)
                break;
            if (((ui64)prev % 100) != 0)
                break;
        }
        if ((num > 0) && (num < 10))
            if (((ui64)prev % 10) != 0)
                break;
        prev += num;
    }
    if (i > ii) {
        m_numbers[ii].m_Numbers[0] = prev;
        m_numbers[ii].SetLastWord(m_numbers[i - 1].LastWord());
    }
    return i;
}

size_t  CNumberProcessor::UniteDigitThousandSequence(size_t ii)
{
    if (ii + 1 >= m_numbers.size())
        return ii;
    if (m_numbers[ii].m_bSingle)
        return  ii;
    if (m_numbers[ii + 1].m_bSingle)
        return  ii;
    if (m_numbers[ii].LastWord() != m_numbers[ii + 1].FirstWord() -1)
        return ii;

    CWord* pWord = GetFirstWord(m_numbers[ii + 1]);
    if ((pWord->m_typ != Word) && (pWord->m_typ != Abbrev) && (pWord->m_typ != AbbDig) && (pWord->m_typ != AbbEos))
        return ii;

    DECLARE_STATIC_RUS_WORD(sto, "сотня");
    if (!m_numbers[ii + 1].Is1000Exp() && !pWord->FindLemma(sto))
        return ii;
    for (size_t i = 0; i < m_numbers[ii].m_Numbers.size(); i++)
        m_numbers[ii].m_Numbers[i] *= m_numbers[ii + 1].m_Numbers[0];

    m_numbers[ii].SetLastWord(m_numbers[ii + 1].LastWord());
    return ii+2;
}

size_t CNumberProcessor::EnlargeSingleNumberWithHalf(size_t ii)
{

    if (m_numbers[ii].m_Numbers.size() != 1)
        return ii;
    if (m_numbers[ii].LastWord() + 2 >= (int)m_words.size())
        return ii;
    CWord& word1 = *m_words[m_numbers[ii].LastWord() + 1];
    CWord& word2 = *m_words[m_numbers[ii].LastWord() + 2];

    DECLARE_STATIC_RUS_WORD(kS, "с");
    if (word1.GetText() != kS)
        return ii;

    DECLARE_STATIC_RUS_WORD(kPolovina, "половина");
    if (!word2.HasHomonym(kPolovina))
        return ii;

    m_numbers[ii].m_Numbers[0] += 0.5;
    m_numbers[ii].SetLastWord(m_numbers[ii].LastWord() + 2);
    return ii;
}

DECLARE_STATIC_NAMES(TNumberProcOt, "с от");
DECLARE_STATIC_NAMES(TNumberProcConj, "и или");
DECLARE_STATIC_NAMES(TNumberProcDo, "до");

size_t CNumberProcessor::EnlargeSingleNumberWithSimilar(size_t ii)
{
    bool bHasOt = false;
    if (m_numbers[ii].FirstWord() > 0)
        bHasOt = TNumberProcOt::Has(m_words[m_numbers[ii].FirstWord() - 1]->GetText());

    size_t i = ii;
    for (; i <  m_numbers.size(); i++) {
        if (i > ii) {
            if (m_numbers[i].FirstWord() != m_numbers[i-1].LastWord() + 2)
                break;
            const CWord& word = *m_words[m_numbers[i].FirstWord() - 1];
            if (!word.IsComma() && !TNumberProcConj::Has(word.GetText()) &&
                !(bHasOt && TNumberProcDo::Has(word.GetText())))
                    break;
        }
    }
    if (i <= ii+1)
        return ii;
    size_t last = i-1;
    if (last >= m_numbers.size())
        return last;

    if (m_numbers[last].Size() == 1)
        return last;

    if (bHasOt && (last != ii+1))
        return last;

    CWord& word = *m_words[m_numbers[last].LastWord()];
    if (word.m_typ != Word && word.m_typ != Abbrev && word.m_typ != AbbDig && word.m_typ != AbbEos)
        return last;
    ui64 integer = 0;
    int fractional = 0;
    for (i = 0; i < word.GetHomonymsCount(); i++)
        if (GlobalGramInfo->FindCardinalNumber(word.GetHomonym(i)->GetLemma(), integer, fractional))
            break;
    if (i >= word.GetHomonymsCount())
        return last;
    if (fractional != 0)
        return last;
    int kk = 0;
    for (; kk < g_Exp1000Count; kk++)
        if (integer == g_Exp1000[kk])
            break;

    if (kk >= g_Exp1000Count)
        return last;

    int exp = (int)floor(log10(m_numbers[ii].m_Numbers[0]));

    for (size_t k = ii + 1; k <= last; k++) {
        int exp1;
        if (k == last)
            exp1 = (int)floor(log10(m_numbers[k].m_Numbers[0]/integer));
        else
            exp1 = (int)floor(log10(m_numbers[k].m_Numbers[0]));

        if (!((exp1 >= exp-1) && (exp1 <= exp+1)))
            return last;
    }

    for (size_t k = ii; k < last; k++)
        for (size_t j = 0; j < m_numbers[k].m_Numbers.size(); j++)
            m_numbers[k].m_Numbers[j] *= integer;
    return last;
}

size_t CNumberProcessor::UniteComplexDigitSequence(size_t ii)
{
    i64 prev = 0;
    yvector<double> nums;
    size_t i = ii;
    for (; ((i <  m_numbers.size()) && (i < ii + 5)); i++) {
        GetFirstWord(m_numbers[i]);       //has side-effect!
        if (m_numbers[i].m_Numbers.size() != 1 && (i > ii))
            break;

        if (i > ii)
            if (m_numbers[i - 1].LastWord() + 1 != (m_numbers[i].FirstWord()))
                break;

        i64 num = (i64)m_numbers[i].m_Numbers[0];

        int k = 0;
        for (; k < g_Exp1000Count; k++)
            if ((num % (i64)(g_Exp1000[k])) == 0)
                break;
        if (k > 0) {
            if (i > ii)
                if ((prev % (i64)g_Exp1000[k-1]) != 0)
                    break;
        } else if (i != ii)
                break;

        if (i == ii) {
            nums = m_numbers[i].m_Numbers;
            prev = (i64)m_numbers[i].m_Numbers[0];
        } else {
            for (size_t k = 0; k < nums.size(); k++)
                nums[k] += num;
            prev = (i64)nums[0];
        }
        if (k >=  g_Exp1000Count) {
            i++;
            break;
        }
    }

    if (i > ii) {
        m_numbers[ii].m_Numbers = nums;
        m_numbers[ii].SetLastWord(m_numbers[i - 1].LastWord());
    }

    return i;
}
