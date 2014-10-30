#include <algorithm>
#include <util/string/cast.h>
#include <util/charset/unidata.h>
#include <FactExtract/Parser/common/gramstr.h>
#include "dateprocessor.h"
#include "wordvector.h"


CDateProcessor::~CDateProcessor(void)
{
}

void CDateProcessor::Run()
{
    for (size_t i=0; i < m_Words.OriginalWordsCount();) {
        CDateGroup date;
        int iLast = i;
        if (BuildDate(i, date)) {
            date.PutWSType(DateTimeWS);
            m_Dates.push_back(date);
            iLast = date.LastWord();

        }
        date.Reset();
        if (BuildTime(i, date)) {
            date.PutWSType(DateTimeWS);
            m_Dates.push_back(date);
            if (iLast <  date.LastWord())
                iLast = date.LastWord();
        }
        i = iLast + 1;
    }

    std::sort(m_Dates.begin(), m_Dates.end());
    UniteDates();
    BuildDatesIntervals();
    AddToWordSequences();
}

bool CDateProcessor::OnlyOneDigit(size_t w)
{
    if ((m_Words[w].m_typ == Digit) &&
        (m_Words.OriginalWordsCount() == 1))
        return true;
    if (m_Words[w].m_typ == Digit) {
        size_t i = 0;
        for (; i < m_Words.OriginalWordsCount(); i++) {
            if (((size_t)i != w) && (m_Words[i].m_typ != Punct))
                break;
        }

        if (i >= m_Words.OriginalWordsCount())
            return true;

        return false;
    }
    return false;
}

static inline bool IsYearLetter(wchar16 ch)
{
    return ch == 0x0433;      // г
}

template <typename TInt>
inline bool ParseInt(const TWtringBuf& text, TInt& res) {
    if (text.empty())
        return false;

    try {
        res = FromString<TInt>(text);
        return true;
    } catch (yexception&) {
        return false;
    }
}

template <typename TInt>
inline bool ParseRestrictedInt(const TWtringBuf& text, TInt& res, TInt min, TInt max) {
    return ParseInt<TInt>(text, res) && res >= min && res <= max;
}

bool CDateProcessor::BuildDigitDate(size_t w, CDateGroup& date)
{
    if (w >= m_Words.OriginalWordsCount())
        return false;

    const size_t len1 = strlen("xx.xx.xx"),
                 len2 = strlen("xx.xx.xxxx"),
                 len3 = strlen("xx.xx.xxxxx"); // для случая "24.03.2006г"

    size_t len = m_Words[w].GetText().size();
    if (len != len1 && len != len2 && len != len3)
        return false;

    TWtrokaTokenizer tokenizer(m_Words[w].GetText(), ".\\/-");
    unsigned int val = 0;
    len = 0;

    if (!tokenizer.NextAsNumber(val, len))
        return false;

    if (len != 2)
        return false;

    bool bFirstDay = true;
    if (val > 31) {
        if (val >= 1900 && val <= 3000) {
            bFirstDay = false;
            date.m_iYear.push_back(val);
        } else
            return false;
    } else {
        if (val < 1 || val > 31)
            return false;
        date.m_iDay.push_back(val);
    }

    if (!tokenizer.NextAsNumber(val,len))
        return false;

    if (len == 2 && val <= 12 && val >= 1)
        date.m_iMonth.push_back(val);
    else
        return false;

    if (m_Words[w].m_typ == ComplexDigit)
        return false;

    TWtringBuf sVal;
    if (!tokenizer.NextAsString(sVal))
        return false;

    len = sVal.size();
    if (len < 5 || (len == 5 && IsYearLetter(sVal[4])))
        if (!ParseRestrictedInt<unsigned int>(sVal.SubStr(0, 4), val, 1, 3000))
            return false;

    if (bFirstDay) {
        if ((len != 2 && len != 4 && len != 5) || (val > 99 && val < 1800))
            return false;

        if (val <= 99) {
            if (val < 30)
                date.m_iYear.push_back(2000 + val);
            else if (val >= 30 && val <= 99)
                date.m_iYear.push_back(1900 + val);
        } else
            date.m_iYear.push_back(val);

    } else if (val > 31)
            return false;
        else
            date.m_iDay.push_back(val);

    // если ещё что-то осталось
    if (tokenizer.NextAsNumber(val, len))
        return false;

    date.SetPair(w,w);
    date.m_iMainWord = w;

    return true;
}

int CDateProcessor::FindNeighbourDate(int iD)
{
    int i = iD + 1;
    for (; i < (int)m_Dates.size(); i++) {
        if (m_Dates[i].FirstWord() !=  m_Dates[i-1].LastWord() + 1)
            break;
    }
    return i - 1;
}

void CDateProcessor::AddToWordSequences()
{
    for (size_t i = 0; i < m_Dates.size(); ++i) {
        int iD = i;
        THolder<CDateGroup> pWS(new CDateGroup(m_Dates[i]));
        pWS->SetPair(m_Dates[iD].FirstWord(), m_Dates[i].LastWord());
        for (size_t j = iD; j < i; j++)
            pWS->m_strDateLemmas.insert(m_Dates[j].m_strDateLemmas.begin(), m_Dates[j].m_strDateLemmas.end());

        Wtroka lemma = m_Words.ToString(*pWS);
        pWS->SetCommonLemmaString(lemma);

        pWS->SetMainWord(pWS->LastWord(),m_Words[pWS->LastWord()].IterHomonyms().GetID());
        pWS->PutWSType(DateTimeWS);
        m_wordSequences.push_back(pWS.Release());
    }
    CWordSequence::Sort(m_wordSequences);
}

int CDateProcessor::GetMohth(const Wtroka& str)
{
    size_t index = TRusMonths::IndexOf(str);
    if (index == TRusMonths::Invalid)
        return -1;
    else
        return index + 1;
}

bool CDateProcessor::IsMonth(size_t w, CDateGroup& date)
{
    const CWord& pW = m_Words[w];
    for (size_t i = 0; i < pW.GetHomonymsCount(); ++i) {
        int val = GetMohth(pW.GetRusHomonym(i).GetLemma());
        if (val != -1) {
            date.m_iMonth.push_back(val);
            for (size_t j = 0; j < pW.m_variant.size(); j++) {
                val = GetMohth(pW.m_variant[j]->GetLemma());
                if (val != -1)
                    date.m_iMonth.push_back(val);
            }
            return true;
        }
    }
    return false;
}

// number of digits from the beginning of @str to first non-digit char
static inline size_t DigitCount(const TWtringBuf& str) {
    size_t res = 0;
    for (; res < str.size(); ++res)
        if (!::IsCommonDigit(str[res]))
            break;
    return res;
}

bool CDateProcessor::IsDay(size_t w, CDateGroup& date)
{
    if (m_Words[w].m_typ == Digit) {
        ui64 val;
        if (!ParseRestrictedInt<ui64>(m_Words[w].GetText(), val, 1, 31))
            return false;
        date.m_iDay.push_back(static_cast<int>(val));
        return true;
    } else if (m_Words[w].m_typ == DigHyp) {
        TWtrokaTokenizer tokenizer(m_Words[w].GetText(), "-");
        ui64 val = 0;
        while (tokenizer.NextAsNumber(val)) {
            if (val <= 0 || val > 31)
                return false;
            date.m_iDay.push_back(static_cast<int>(val));
        }
        return true;
    } else if (m_Words[w].m_typ == Hyphen || m_Words[w].m_typ == Complex) {
        if (!m_Words[w].HasPOS(gAdjNumeral))
            return false;
        ui64 val = 0;
        if (m_Words[w].m_typ == Hyphen) {
            TWtrokaTokenizer tokenizer(m_Words[w].GetText(), "-");
            if (!tokenizer.NextAsNumber(val))
                return false;
        }
        TWtringBuf tmp = m_Words[w].GetText();
        if (m_Words[w].m_typ == Complex && !ParseInt(tmp.SubStr(0, DigitCount(tmp)), val))
            return false;

        if (val <= 0 || val > 31)
            return false;
        date.m_iDay.push_back(static_cast<int>(val));
        return true;
    }
    return false;
}

bool CDateProcessor::IsYearNumber(ui64& val)
{
    if ((val > 99 && val < 1000) || val > 3000)
        return false;

    if (val <= 99) {
        if (val < 30) {
            val += 2000;
            return true;
        } else if (val >= 30 && val <= 99) {
            val += 1900;
            return true;
        }
    }
    return true;
}

struct TYearAdjFlexes: public TStaticNames<TYearAdjFlexes> {
    Wtroka M, E, H, GO, J, MU;

    TYearAdjFlexes() {
        M = Add("м");
        E = Add("е");
        H = Add("х");
        GO = Add("го");
        J = Add("й");
        MU = Add("му");
    }
};

DECLARE_STATIC_NAMES(TYearWordforms, "годе годом году года год гг г");

bool CDateProcessor::IsYear(size_t w, CDateGroup& date, bool& bHasYearWord, Wtroka& strPostfix, int& iOriginalYear)
{
    ui64 MAX_YEAR_LEN = 6;

    bHasYearWord = false;
    ui64 val = 0;
    ui64 val2 = 0; // если "в 1996-1998 годах "
    if (m_Words[w].m_typ == Digit) {
        if (m_Words[w].GetText().size() > MAX_YEAR_LEN || !ParseInt(m_Words[w].GetText(), val))
            return false;
    } else if (m_Words[w].m_typ == Hyphen) {
        TWtrokaTokenizer strTok(m_Words[w].GetText(), "-");
        if (!strTok.NextAsNumber(val))
            return false;
        Wtroka strItem;
        if (!strTok.NextAsString(strItem))
            return false;
        TMorph::ToLower(strItem);
        if (!TYearAdjFlexes::Has(strItem))
            return false;
        strPostfix = strItem;
    } else if (m_Words[w].m_typ == Complex) // специальная заточка на случай 2002года или 2002г
    {
        if (m_Words[w].m_variant.size() != 2)
            return false;

        int iGodVar = 1;
        int iYearVar = 0;

        Wtroka strLemma = m_Words[w].m_variant[iYearVar]->GetLemma();
        if (DigitCount(strLemma) == strLemma.size())
            iGodVar = 1;
        else {
            iYearVar = 1;
            strLemma = m_Words[w].m_variant[iYearVar]->GetLemma();
            if (DigitCount(strLemma) == strLemma.size())
                iGodVar = 0;
            else
                return false;
        }

        if (strLemma.size() > MAX_YEAR_LEN || !ParseInt(strLemma, val) || !IsYearNumber(val))
            return false;

        strLemma = m_Words[w].m_variant[iGodVar]->GetLemma();
        if (!TYearWordforms::Has(strLemma))
            return false;
        bHasYearWord = true;
    } else if (m_Words[w].m_typ == DigHyp) // если "в 1996-1998 годах "
    {
        TWtrokaTokenizer strTok(m_Words[w].GetText(), "-");
        if (!strTok.NextAsNumber(val) || !strTok.NextAsNumber(val2))
            return false;
    } else
        return false;

    iOriginalYear = static_cast<int>(val);
    if (!IsYearNumber(val))
        return false;
    if (val2 != 0 && !IsYearNumber(val2))
        return false;
    date.m_iYear.push_back(static_cast<int>(val));
    if (val2 != 0)
        date.m_iYear.push_back(static_cast<int>(val2));

    if (strPostfix == TYearAdjFlexes::Static()->M ||
        strPostfix == TYearAdjFlexes::Static()->E ||
        strPostfix == TYearAdjFlexes::Static()->H)   // м е х
        date.m_bUncertainYear = true;
    return true;
}

bool CDateProcessor::IsYearWord(size_t w)
{
    DECLARE_STATIC_RUS_WORD(kGPoint, "г.");
    DECLARE_STATIC_RUS_WORD(kGod, "год");
    DECLARE_STATIC_RUS_WORD(kLet, "лет");
    DECLARE_STATIC_RUS_WORD(kG, "г");
    DECLARE_STATIC_RUS_WORD(kGG, "гг.");

    return m_Words[w].HasHomonym(kGPoint) ||
        (m_Words[w].HasHomonym(kGod) && !m_Words[w].HasHomonym(kLet)) ||
        m_Words[w].HasHomonym(kG) ||
        m_Words[w].HasHomonym(kGG);
}

bool CDateProcessor::BuildDMYDate(size_t from, CDateGroup& date)
{
    if (from + 2 >= m_Words.OriginalWordsCount())
        return false;

    if (!IsDay(from, date))
        return false;

    if (!IsMonth(from + 1, date))
        return false;

    bool bHasYearWord;
    Wtroka strPostfix;
    int iOriginalYear;
    if (!IsYear(from + 2, date, bHasYearWord, strPostfix, iOriginalYear))
        return false;

    int iLastWord = from + 2;

    if (!bHasYearWord && from + 3 < m_Words.OriginalWordsCount() && IsYearWord(from + 3))
        iLastWord = from + 3;

    date.SetPair(from, iLastWord);
    date.m_iMainWord = from+2;
    return true;
}

// даты типа 24.11, их объявляем квазидатами, полными они станут только в грамматике в нужном контексте
bool CDateProcessor::BuildDigitalDMDate(size_t w, CDateGroup& date)
{
    if (w >= m_Words.OriginalWordsCount())
        return false;

    const size_t len1 = strlen("xx:xx"), len2 = strlen("x:xx");
    size_t len = m_Words[w].GetText().size();
    if (len != len1 && len != len2)
        return false;

    TWtrokaTokenizer tokenizer(m_Words[w].GetText(), ".");
    unsigned int val = 0;

    if (!tokenizer.NextAsNumber(val, len) || (val > 31 || val < 1))
        return false;
    date.m_iDay.push_back(val);
    if (!tokenizer.NextAsNumber(val, len) || len != 2 || val > 12 || val < 1)
        return false;
    date.m_iMonth.push_back(val);
    date.SetPair(w,w);
    return true;
}

bool CDateProcessor::BuildMYDate(size_t from, CDateGroup& date)
{
    if (from + 1 >= m_Words.OriginalWordsCount() || !IsMonth(from, date))
        return false;

    bool bHasYearWord;
    Wtroka strPostfix;
    int iOriginalYear;
    if (!IsYear(from + 1, date, bHasYearWord,strPostfix, iOriginalYear))
        return false;

    if (!bHasYearWord) {
        bool bNeedYear = true;
        if (!strPostfix.empty())
            bNeedYear = false;

        if (from + 2 < m_Words.OriginalWordsCount()) {
            if (!IsYearWord(from + 2)) {
                if (bNeedYear)
                    return false;
                else
                    bHasYearWord = true;
            }
        } else {
            if (bNeedYear)
                return false;
            else
                bHasYearWord = true;
        }
    }

    if (bHasYearWord)
        date.SetPair(from, from + 1);
    else
        date.SetPair(from, from + 2);

    date.m_iMainWord = from + 1;
    return true;
}

static bool HasMorphAdjWithGrammems(const CWord& word, const TGramBitSet& grammems)
{
    for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it)
        if (it->IsMorphAdj() && it->Grammems.HasAll(grammems))
            return true;
    return false;
}

DECLARE_STATIC_NAMES(TYearPrefixes_GO, "с до зима весна лето осень начало середина конец половина");
DECLARE_STATIC_NAMES(TYearPrefixes_H,   "с до в начало середина конец половина");

DECLARE_STATIC_RUS_WORD(PREP_PO, "по");
DECLARE_STATIC_RUS_WORD(PREP_NAD, "над");
DECLARE_STATIC_RUS_WORD(PREP_K, "к");
DECLARE_STATIC_RUS_WORD(PREP_V, "в");
DECLARE_STATIC_RUS_WORD(PREP_S, "с");
DECLARE_STATIC_RUS_WORD(PREP_DO, "до");

bool CDateProcessor::CheckYearWithoutYearWord(size_t from, CDateGroup& date, Wtroka strPostfix)
{
    if (from == 0)
        return false;

    bool bRes = false;

    static const TGramBitSet mask_subj(gSubstantive, gAdjective, gParticiple);
    static const TGramBitSet mask_nominative(gSingular, gNominative, gAccusative);
    static const TGramBitSet mask_dative(gSingular, gDative);
    static const TGramBitSet mask_vocative(gSingular, gVocative);
    static const TGramBitSet mask_genitive(gSingular, gGenitive);

    if (strPostfix == TYearAdjFlexes::Static()->J)  // й
        for (;;) {
            if (!m_Words[from - 1].HasHomonym(PREP_PO) && !m_Words[from - 1].HasHomonym(PREP_NAD))
                break;
            if (from < m_Words.OriginalWordsCount() - 1 && m_Words[from + 1].HasAnyOfPOS(mask_subj))
                if (m_Words[from + 1].HasMorphNounWithGrammems(mask_nominative) ||
                    HasMorphAdjWithGrammems(m_Words[from + 1], mask_nominative))
                        break;
            bRes =  true;
            break;
        } else if (strPostfix == TYearAdjFlexes::Static()->MU) // му
        for (;;) {
            if (!m_Words[from - 1].HasHomonym(PREP_K))
                break;
            if (from < m_Words.OriginalWordsCount() - 1 && m_Words[from + 1].HasAnyOfPOS(mask_subj))
                if (m_Words[from + 1].HasMorphNounWithGrammems(mask_dative) ||
                    HasMorphAdjWithGrammems(m_Words[from + 1], mask_dative))
                        break;
            bRes =  true;
            break;
        } else if (strPostfix == TYearAdjFlexes::Static()->M) // м
        for (;;) {
            if (!m_Words[from - 1].HasHomonym(PREP_V))
                break;
            if (from < m_Words.OriginalWordsCount() - 1 && m_Words[from + 1].HasAnyOfPOS(mask_subj))
                if (m_Words[from + 1].HasMorphNounWithGrammems(mask_vocative) ||
                    HasMorphAdjWithGrammems(m_Words[from + 1], mask_vocative))
                        break;
            bRes =  true;
            break;
        } else if (strPostfix == TYearAdjFlexes::Static()->GO) // го
        for (;;) {
            if (!m_Words[from - 1].HasHomonymFromList<TYearPrefixes_GO>())
                break;
            if (from < m_Words.OriginalWordsCount() - 1 && m_Words[from + 1].HasAnyOfPOS(mask_subj))
                if (m_Words[from + 1].HasMorphNounWithGrammems(mask_genitive) ||
                    HasMorphAdjWithGrammems(m_Words[from + 1], mask_genitive))
                        break;
            bRes =  true;
            break;
        }

    if (!bRes && date.m_iYear[0] % 10 == 0) {
        if (strPostfix == TYearAdjFlexes::Static()->E) // е
        {
            if (!m_Words[from - 1].HasHomonym(PREP_PO) && !m_Words[from - 1].HasHomonym(PREP_V))
                return false;
            bRes =  true;
        } else if (strPostfix == TYearAdjFlexes::Static()->H) // х
        {
            if (!m_Words[from - 1].HasHomonymFromList<TYearPrefixes_H>())
                return false;
            bRes =  true;
        } else if (strPostfix == TYearAdjFlexes::Static()->M) // м
        {
            if (!m_Words[from-1].HasHomonym(PREP_K))
                return false;
            bRes =  true;
        }
    }
    return bRes;
}

bool CDateProcessor::HasTomuNazad(size_t from)
{
    if (from >= m_Words.OriginalWordsCount())
        return false;
    if (!NStr::EqualCiRus(m_Words[from].GetText(), "года"))
        return false;
    if (from + 1 >= m_Words.OriginalWordsCount())
        return false;
    if (NStr::EqualCiRus(m_Words[from + 1].GetText(), "назад"))
        return true;
    if (!NStr::EqualCiRus(m_Words[from + 1].GetText(), "тому"))
        return false;
    if (from + 2 >= m_Words.OriginalWordsCount())
        return false;
    if (NStr::EqualCiRus(m_Words[from + 2].GetText(), "назад"))
        return true;
    return false;
}

bool CDateProcessor::BuildYDate(size_t from, CDateGroup& date)
{
    if (from >= m_Words.OriginalWordsCount())
        return false;

    bool bHasYearWord;
    Wtroka strPostfix;
    int iOriginalYear;
    if (!IsYear(from, date, bHasYearWord, strPostfix, iOriginalYear))
        return false;

    if (!bHasYearWord) {
        bool bNeedYear = true;
        if (!strPostfix.empty())
            if (iOriginalYear > 0 && iOriginalYear < 10)
                bNeedYear = true;
            else if (iOriginalYear > 9 && iOriginalYear < 100)
                bNeedYear = !CheckYearWithoutYearWord(from, date, strPostfix);
            else
                bNeedYear = false;

        if (from + 1 < m_Words.OriginalWordsCount()) {
            if (!IsYearWord(from + 1)) {
                if (bNeedYear)
                    return false;
                else
                    bHasYearWord = true;
            } else if (HasTomuNazad(from + 1))
                return false;
        } else if (bNeedYear)
            return false;
        else
            bHasYearWord = true;
    }

    if (bHasYearWord)
        date.SetPair(from, from);
    else
        date.SetPair(from, from + 1);
    date.m_iMainWord = from;
    return true;
}

bool CDateProcessor::BuildDMDate(size_t from, CDateGroup& date)
{
    if (from + 1 >= m_Words.OriginalWordsCount())
        return false;

    if (!IsDay(from, date))
        return false;

    if (!IsMonth(from + 1, date))
        return false;

    date.SetPair(from, from + 1);
    date.m_iMainWord = from + 1;
    return true;
}

bool CDateProcessor::HasStorableYearNumber(CDateGroup& date)
{
    for (size_t i = 0; i < date.m_iYear.size(); ++i)
        if (date.m_iYear[i] < 1753 || date.m_iYear[i] > 3000)
            return false;
    return true;
}

bool CDateProcessor::BuildTime(size_t w, CDateGroup& date)
{
    if (w >= m_Words.OriginalWordsCount())
        return false;

    const size_t len1 = strlen("xx:xx"), len2 = strlen("x:xx");
    size_t len = m_Words[w].GetText().size();
    if (len != len1 && len != len2)
        return false;

    TWtrokaTokenizer tokenizer(m_Words[w].GetText(), ":.-");
    unsigned int val = 0;

    if (!tokenizer.NextAsNumber(val, len) || val > 23)
        return false;

    date.m_iHour.push_back(val);
    if (!tokenizer.NextAsNumber(val, len) || len != 2 || val > 59)
        return false;

    date.m_iMinute.push_back(val);
    date.SetPair(w,w);
    return true;
}

bool CDateProcessor::BuildDate(size_t from, CDateGroup& date)
{
    if (BuildDigitDate(from, date))
        return true;

    date.Reset();
    if (BuildDMYDate(from, date))
        return true;

    date.Reset();
    if (BuildMYDate(from, date))
        return true;

    date.Reset();
    if (BuildDigitalDMDate(from, date))
        return true;

    date.Reset();
    if (BuildYDate(from, date))
        return true;

    return false;
}

void CDateProcessor::FindPrevMDates(size_t iDate)
{
    CDateGroup& date = m_Dates[iDate];
    for (int w = date.FirstWord() - 1; w >= 1; w -= 2) {
        if (!m_Words[w].IsComma() && !m_Words[w].IsAndConj())
            return;
        CDateGroup new_date;
        if (!IsMonth(w - 1, new_date))
            return;
        date.m_iMonth.insert(date.m_iMonth.end(), new_date.m_iMonth.begin(), new_date.m_iMonth.end());
    }
}

bool CDateProcessor::FindPrevDMDates(size_t ii)
{
    int iDate = ii;
    CDateGroup& main_date = m_Dates[iDate];
    bool bFound = false;
    while (--iDate >= 0) {
        FindPrevDDates(iDate);
        CDateGroup& prev_date = m_Dates[iDate + 1];
        int w = prev_date.FirstWord() - 1;
        if (w < 2 || (!m_Words[w].IsComma() && !m_Words[w].IsAndConj()))
            break;
        CDateGroup& date = m_Dates[iDate];
        if (date.LastWord() + 1 != w || !(date.m_iYear.size() == 0 && date.m_iDay.size() != 0 && date.m_iMonth.size() != 0))
            break;
        date.m_iYear.insert(date.m_iYear.end(), main_date.m_iYear.begin(), main_date.m_iYear.end());
        date.m_iMainWord = prev_date.m_iMainWord;
    }
    return bFound;
}

void CDateProcessor::FindPrevDDates(size_t iDate)
{
    CDateGroup& date = m_Dates[iDate];
    for (int w = date.FirstWord() - 1; w >= 1; w -= 2) {
        if (!m_Words[w].IsComma() && !m_Words[w].IsAndConj())
            return;
        CDateGroup new_date;
        if (!IsDay(w-1, new_date))
            return;
        date.m_iDay.insert(date.m_iDay.end(), new_date.m_iDay.begin(), new_date.m_iDay.end());
        date.SetFirstWord(w - 1);
    }
}

void CDateProcessor::UniteDates()
{
    size_t size = m_Dates.size();
    //yvector<CDateGroup> new_dates;
    for (int i = size - 1; i >=0; --i) {
        CDateGroup& date = m_Dates[i];
        if (date.IsMH())
            continue;
        if (date.m_iYear.size() != 0 && date.m_iMonth.size() != 0) {
            if (date.m_iDay.size() == 0)
                FindPrevMDates(i);
            else {
                FindPrevDDates(i);
                FindPrevDMDates(i);
            }
        } else if (date.m_iMonth.size() != 0 && date.m_iDay.size() != 0)
            FindPrevDDates(i);
    }
    //m_Dates.insert(m_Dates.end(), new_dates.begin() , new_dates.end());
}

bool CDateProcessor::TryToBuildDMYDateInterval(size_t i)
{
    if (i == 0)
        return false;
    CDateGroup& d1 = m_Dates[i - 1];
    CDateGroup& d2 = m_Dates[i];
    if (!d1.IsDMY() || !d2.IsDMY())
        return false;
    if (d1.FirstWord() == 0)
        return false;
    if (d1.LastWord() != d2.FirstWord() - 2)
        return false;

    if (!m_Words[d1.FirstWord() - 1].HasHomonym(PREP_S))
        return false;
    if (!m_Words[d1.LastWord() + 1].HasHomonym(PREP_DO) &&
        !m_Words[d1.LastWord() + 1].HasHomonym(PREP_PO))
        return false;
    d2.m_strIntervalDate = d1.BuildDMYDate();
    return true;
}

bool CDateProcessor::TryToBuildMDateInterval(yvector<CDateGroup>& new_dates, size_t i)
{
    CDateGroup& date = m_Dates[i];
    if (date.FirstWord() < 3)
        return false;

    CDateGroup new_date;
    if (!IsMonth(date.FirstWord() - 2, new_date))
        return false;
    if (!m_Words[date.FirstWord() - 3].HasHomonym(PREP_S))
        return false;
    if (!m_Words[date.FirstWord() - 1].HasHomonym(PREP_DO) &&
        !m_Words[date.FirstWord() - 1].HasHomonym(PREP_PO))
        return false;
    new_date.m_iYear = date.m_iYear;
    new_date.m_iMainWord = date.m_iMainWord;
    new_date.SetPair(date.FirstWord() - 3, date.FirstWord() - 1);
    date.m_strIntervalDate = new_date.BuildMYDate();
    new_dates.push_back(new_date);
    return true;
}

bool CDateProcessor::TryToBuildDDateInterval(yvector<CDateGroup>& new_dates, size_t i)
{
    CDateGroup& date = m_Dates[i];
    if (date.FirstWord() < 3)
        return false;

    CDateGroup new_date;
    if (!IsDay(date.FirstWord() - 2, new_date))
        return false;
    if (!m_Words[date.FirstWord() - 3].HasHomonym(PREP_S))
        return false;
    if (!m_Words[date.FirstWord() - 1].HasHomonym(PREP_DO) &&
        !m_Words[date.FirstWord() - 1].HasHomonym(PREP_PO))
        return false;

    new_date.m_iMonth = date.m_iMonth;
    new_date.m_iYear = date.m_iYear;
    new_date.m_iMainWord = date.m_iMainWord;
    new_date.SetPair(date.FirstWord() - 3, date.FirstWord() - 1);
    if (new_date.IsDMY())
        date.m_strIntervalDate = new_date.BuildDMYDate();
    else if (date.IsDM())
        date.m_strIntervalDate = new_date.BuildDMDate();
    new_dates.push_back(new_date);
    return true;
}

bool CDateProcessor::TryToBuildYDateInterval(yvector<CDateGroup>& new_dates, size_t i)
{
    CDateGroup& date = m_Dates[i];
    if (date.FirstWord() < 3)
        return false;
    CDateGroup new_date;
    bool bHasYearWord;
    Wtroka strPostfix;
    int iOriginalYear;
    if (!IsYear(date.FirstWord() - 2, new_date, bHasYearWord, strPostfix, iOriginalYear))
        return false;
    if (!m_Words[date.FirstWord() - 3].HasHomonym(PREP_S))
        return false;
    if (!m_Words[date.FirstWord() - 1].HasHomonym(PREP_DO) &&
        !m_Words[date.FirstWord() - 1].HasHomonym(PREP_PO))
        return false;
    date.m_strIntervalDate = new_date.BuildYDate();
    new_date.m_iMainWord = date.FirstWord() - 2;
    new_date.SetPair(date.FirstWord() - 3, date.FirstWord() - 1);
    new_dates.push_back(new_date);
    return true;
}

bool CDateProcessor::TryToBuildDMDateInterval(size_t i)
{
    if (i == 0)
        return false;
    CDateGroup& d1 = m_Dates[i - 1];
    CDateGroup& d2 = m_Dates[i];

    if (d1.FirstWord() == 0 || d1.LastWord() != d2.FirstWord() - 2)
        return false;
    if (!m_Words[d1.FirstWord() - 1].HasHomonym(PREP_S))
        return false;
    if (!m_Words[d1.LastWord() + 1].HasHomonym(PREP_DO) &&
        !m_Words[d1.LastWord() + 1].HasHomonym(PREP_PO))
        return false;

    d1.m_iYear = d2.m_iYear;
    if (d1.IsDMY()) {
        d2.m_strIntervalDate = d1.BuildDMYDate();
        d1.m_iMainWord = d2.m_iMainWord;
    } else if (d1.IsDM())
        d2.m_strIntervalDate = d1.BuildDMDate();

    return true;
}

void CDateProcessor::BuildDatesIntervals()
{
    yvector<CDateGroup> new_dates;
    size_t size = m_Dates.size();
    for (int i = size - 1; i >=0; --i) {
        CDateGroup& date = m_Dates[i];

        if (date.IsMY())
            TryToBuildMDateInterval(new_dates, i);
        else if (date.IsY())
            TryToBuildYDateInterval(new_dates, i);
        else if (date.IsDMY() && !TryToBuildDMYDateInterval(i) && !TryToBuildDMDateInterval(i))
            TryToBuildDDateInterval(new_dates, i);
        else if (date.IsDM() && !TryToBuildDMDateInterval(i))
            TryToBuildDDateInterval(new_dates, i);
    }
    m_Dates.insert(m_Dates.end(), new_dates.begin() , new_dates.end());
    std::sort(m_Dates.begin(), m_Dates.end());
}

//////////////////////////////////////////////////////////////////////////////

CDateGroup::CDateGroup()
{
    m_iMainWord = -1;
    m_bDateField = false;
    m_bUncertainYear = false;
}

Wtroka CDateGroup::BuildMYDate()
{
    if (!IsMY())
        return Wtroka();
    return CharToWide(Sprintf("!%d%02d", m_iYear[0], m_iMonth[0]));
}

Wtroka CDateGroup::BuildDMDate()
{
    if (!IsDM())
        return Wtroka();
    return CharToWide(Sprintf("!0000%02d%02d", m_iMonth[0], m_iDay[0]));
}

Wtroka CDateGroup::BuildYDate()
{
    if (!IsY())
        return Wtroka();
    return CharToWide(Sprintf("!%d", m_iYear[0]));
}

Wtroka CDateGroup::BuildDMYDate()
{
    if (!IsDMY())
        return Wtroka();

    Stroka strYear = Sprintf("%d", m_iYear[0]);
    if (strYear.size() != 4)
        return Wtroka();

    return CharToWide("!" + strYear + Sprintf("%02d%02d", m_iMonth[0], m_iDay[0]));
}

//проверяем, что год укладывается в интервал datetime sql-я
bool CDateGroup::HasStorebaleYear() const
{
    return  (m_SpanInterps.begin()->GetRealYear() >= 1753) && (m_SpanInterps.begin()->GetRealYear() < 2050) &&
            (m_SpanInterps.rbegin()->GetRealYear() >= 1753) && (m_SpanInterps.rbegin()->GetRealYear() < 2050);
}

Wtroka CDateGroup::GetLemma() const
{
    return ToString();
}

Wtroka CDateGroup::GetCapitalizedLemma() const
{
    return ToString();
}

Wtroka CDateGroup::BuildMaxLemma(CTime &timeFromDateField)
{
    Wtroka strRes;
    int year = 1980;
    if (m_iYear.size() > 0 && m_iYear[0] >= 1980)
        year = m_iYear[0];

    if (IsDMY()) {
        strRes = BuildDMYDate();
        timeFromDateField = CTime(year, m_iMonth[0], m_iDay[0],0,0,0);
    } else if (IsMY()) {
        strRes = BuildMYDate();
        timeFromDateField = CTime(year, m_iMonth[0],1,0,0,0);
    } else if (IsDM()) {
        strRes = BuildDMDate();
        timeFromDateField = CTime(year, m_iMonth[0], m_iDay[0],0,0,0);
    } else if (IsY()) {
        strRes = BuildYDate();
        timeFromDateField = CTime(year,1,1,0,0,0);
    }

    if (!strRes.empty())
        strRes = strRes.substr(1);
    return strRes;
}

Wtroka CDateGroup::ToString() const
{
    Stroka strRes;
    if (m_SpanInterps.size() == 1)
        strRes = m_SpanInterps.begin()->GetDateString();
    else if (m_SpanInterps.size() > 1) {
        strRes += m_SpanInterps.begin()->GetDateString();
        strRes += " - ";
        strRes += m_SpanInterps.rbegin()->GetDateString();
    } else
        return CWordSequence::GetLemma();

    return CharToWide(strRes);
}

bool CDateGroup::BuildLemmas(bool isFateField)
{
    Stroka strYear, strMonth, strDay;

    Stroka spec("!");
    bool bZeroYear = false;
    for (size_t i = 0; (i < m_iYear.size()) || bZeroYear; i++) {
        bZeroYear = false;
        if (m_iYear.size())
            strYear = Sprintf("%d", m_iYear[i]);
        else
            strYear = "0000";

        if (strYear.size() != 4)
            continue;

        if (m_iYear.size())
            m_strDateLemmas.insert(CharToWide(spec + strYear));

        for (size_t j = 0; j < m_iMonth.size(); ++j) {
            if (m_iMonth[j] != 0) {
                strMonth = Sprintf("%02d", m_iMonth[j]);
                m_strDateLemmas.insert(CharToWide(spec + strYear + strMonth));
            }

            for (size_t k = 0; k < m_iDay.size(); k++) {
                if (m_iDay[k] != 0)
                    strDay = Sprintf("%02d", m_iDay[k]);

                if (m_iDay[k] != 0 && m_iMonth[j] != 0)
                    m_strDateLemmas.insert(CharToWide(spec + strYear + strMonth + strDay));
            }
        }
    }

    if (!m_strIntervalDate.empty())
        if (IsDMY())
            m_strDateLemmas.insert(m_strIntervalDate + BuildDMYDate());
        else if (IsMY())
            m_strDateLemmas.insert(m_strIntervalDate + BuildMYDate());
        else if (IsDM())
            m_strDateLemmas.insert(m_strIntervalDate + BuildDMDate());
        else if (IsY())
            m_strDateLemmas.insert(m_strIntervalDate + BuildYDate());

    if (isFateField) {
        yset<Wtroka> copy = m_strDateLemmas;
        yset<Wtroka>::iterator it = copy.begin();
        Wtroka wspec = CharToWide(spec);
        for (; it != copy.end(); ++it)
            m_strDateLemmas.insert(wspec + *it);
    }
    m_strDateLemmas.insert(g_strDateNonTerminal);
    return true;
}

bool CDateGroup::IsY() const
{
    return (m_iYear.size() != 0) && (m_iMonth.size() == 0) && (m_iDay.size() == 0);
}

bool CDateGroup::IsMY() const
{
    return (m_iYear.size() != 0) && (m_iMonth.size() != 0) && (m_iDay.size() == 0);
}

bool CDateGroup::IsDMY() const
{
    return (m_iYear.size() != 0) && (m_iMonth.size() != 0) && (m_iDay.size() != 0);
}

bool CDateGroup::IsDM() const
{
    return (m_iYear.size() == 0) && (m_iMonth.size() != 0) && (m_iDay.size() != 0);
}

bool CDateGroup::IsMH() const
{
    return (m_iHour.size() != 0) && (m_iMinute.size() != 0);
}

void CDateGroup::Reset()
{
    m_iDay.clear();
    m_iMonth.clear();
    m_iYear.clear();
    m_iHour.clear();
    m_iMinute.clear();
    m_strDateLemmas.clear();
}

CTimeLong::CTimeLong(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nYDif) :
            CTime(nYear, nMonth, nDay, nHour, nMin, nSec)
{
    m_iYDif = nYDif;
}

int CTimeLong::GetRealYear() const
{
    return GetYear() + m_iYDif;
}

int CTimeLong::GetRealDayOfWeek() const
{
    return (0 == m_iYDif) ? GetDayOfWeek() : -1;
}

Stroka CTimeLong::GetDateString() const
{
    Stroka sDate(::ToString(GetRealYear()));
    sDate += Format("-%m-%d %H:%M:%S");
    return sDate;
}

Stroka CTimeLong::GetSQLFormat() const
{
    Stroka sDate(::ToString(GetRealYear()));
    sDate += Format("-%m-%d");
    return sDate;
}

int CTimeLong::CalculateYDif(int iYear, int& iYDif)
{
    if (low_date > iYear) iYDif = iYear - low_date;
    if (high_date < iYear) iYDif = iYear - high_date;

    return iYear - iYDif;
}
