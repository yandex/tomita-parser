#include "textbase.h"

#include <util/charset/unidata.h>
#include <util/charset/unidata.h>

static const ui32 MAX_WORD_LEN_N = 128;

// u - unknown
// Z - space (' ','\t')
// N - newline ('\n')
// R - return ('\r')
// P - punctuation
// S - symbol
// D - digit
// T t - text uppercase / lowercase
// A a - alternate text uppercase / lowercase

TTextCharTable::TTextCharTable()
    : Table(1 << 16, 'u')
{
}

void TTextCharTable::Reset(docLanguage landId)
{
    // from old RusCharTypeTable
    //      P:    !"'(),-.:;?`|
    //      S:    #$%&*+/<=>@[\]^_{}~

    const NLemmer::TAlphabetWordNormalizer* awn = NLemmer::GetAlphaRules(landId);
    YASSERT(awn != NULL);

    for (size_t i = 0; i < Table.size(); ++i) {
        wchar16 ch = static_cast<wchar16>(i);
        if (::IsLineSep(ch) || ::IsParaSep(ch))
            Table[i] = 'N';
        else if (::IsSpace(ch))
            Table[i] = 'Z';
        else if (::IsTerminal(ch) || ::IsQuotation(ch) || ::IsHyphen(ch) || ::IsDash(ch))
            Table[i] = 'P';
        // all other punctuation treat as symbols
        else if (::IsPunct(ch) || ::IsSymbol(ch))
            Table[i] = 'S';
        else if (::IsCommonDigit(ch))
            Table[i] = 'D';
        else if (::IsDigit(ch))     // consider uncommon exotic digits as symbols
            Table[i] = 'S';
        else if (::IsAlpha(ch))
            if (awn->GetAlphabet()->CharClass(ch) & NLemmer::TAlphabet::CharAlphaNormal)
                Table[i] = ::IsUpper(ch) ? 'T' : 't';
            else
                Table[i] = ::IsUpper(ch) ? 'A' : 'a';
        else
            Table[i] = 'u';
    }

    // corrections:
    Table[CTEXT_REM_SYMBOL] = 'R';              //CTEXT_REM_SYMBOL (0x01) is used as HTML eraser
    Table['\r'] = 'R';
    Table['\n'] = 'N';

    Table['('] = 'P';
    Table[')'] = 'P';
    Table['`'] = 'P';
    Table['|'] = 'P';
    Table['/'] = 'P';

    //Table[static_cast<ui8>('№')] = 'u';     //skip
}

const int g_ComplexNumberSchemesCount = 35;

const Wtroka g_strDashWord = CharToWide("$$$dashw");

SPrimScheme g_ComplexNumberSchemes[g_ComplexNumberSchemesCount] =
{
    { { {Digit, 0},  { Symbol, '%'} }, 2, ComplexDigit },
    { { {Digit, 0},  { Punct, ','}, {Digit, 0}, { Symbol, '%'} }, 4, ComplexDigit },
    { { {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Symbol, '%'} }, 4, ComplexDigit },
    { { {Digit, 0},  {Punct, ','}, {Digit, 0} }, 3, ComplexDigit },
    { { {Digit, 0},  {Punct, '.'}, {Digit, 0} }, 3, ComplexDigit },
    { { {Symbol, '$'}, {Digit, 0},  {Punct, ','}, {Digit, 0} }, 4, ComplexDigit },
    { { {Symbol, '$'}, {Digit, 0},  {Punct, '.'}, {Digit, 0} }, 4, ComplexDigit },
    { { {Digit, 0},  { Punct, ','}, {Digit, 0}, { Punct, '-'}, {Digit, 0},  { Punct, ','}, {Digit, 0} }, 7, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Punct, '-'}, {Digit, 0},  { Punct, '.'}, {Digit, 0} }, 7, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, '-'}, {Digit, 0},  { Punct, ','}, {Digit, 0} }, 5, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, '-'}, {Digit, 0},  { Punct, '.'}, {Digit, 0} }, 5, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, ','}, {Digit, 0}, { Punct, '-'}, {Digit, 0} }, 5, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Punct, '-'}, {Digit, 0} }, 5, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, ','}, {Digit, 0}, { Punct, '-'}, {Symbol, '$'}, {Digit, 0},  { Punct, ','}, {Digit, 0} }, 9, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Punct, '-'}, {Symbol, '$'}, {Digit, 0},  { Punct, '.'}, {Digit, 0} }, 9, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '-'}, {Symbol, '$'}, {Digit, 0},  { Punct, ','}, {Digit, 0} }, 7, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '-'}, {Symbol, '$'}, {Digit, 0},  { Punct, '.'}, {Digit, 0} }, 7, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, ','}, {Digit, 0}, { Punct, '-'}, {Symbol, '$'}, {Digit, 0} }, 7, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Punct, '-'}, {Symbol, '$'}, {Digit, 0} }, 7, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, ','}, {Digit, 0}, { Punct, '-'},  {Digit, 0},  { Punct, ','}, {Digit, 0} }, 8, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Punct, '-'},  {Digit, 0},  { Punct, '.'}, {Digit, 0} }, 8, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '-'},  {Digit, 0},  { Punct, ','}, {Digit, 0} }, 6, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '-'},  {Digit, 0},  { Punct, '.'}, {Digit, 0} }, 6, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, ','}, {Digit, 0}, { Punct, '-'},  {Digit, 0} }, 6, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '.'}, {Digit, 0}, { Punct, '-'},  {Digit, 0} }, 6, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, '-'},  {Digit, 0} }, 3, DigHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '-'},  {Symbol, '$'},  {Digit, 0} }, 5, ComplexDigitHyp },
    { { {Digit, 0},  { Punct, '-'}, {Digit, 0}, { Symbol, '%'} }, 4, ComplexDigitHyp },
    { { {Symbol, '$'}, {Digit, 0},  { Punct, '-'},  {Digit, 0} }, 4, ComplexDigitHyp },
    { { {Symbol, '#'}, {Digit, 0} }, 2, ComplexDigit },
    { { {Word, 'N'}, {Digit, 0} }, 2, ComplexDigit },
    { { {AltWord, 'N'}, {Digit, 0} }, 2, ComplexDigit },
//    { { {Symbol, '№'}, {Digit, 0} }, 2, ComplexDigit }, // TODO: needs to be rewritten for UTF8
    { { {Digit, 0}, {Symbol, '%'}, { Punct, '-'}, {Word, 0} }, 4, Hyphen}, //25%-РіРѕ
    { { {Digit, 0}, { Punct, '-'}, {Word, 0} }, 3, Hyphen} //47-Рµ
    /*{ { {Symbol, '$'}, {Digit, 0}, { Punct, '.'} }, 3, ComplexDigit },
    { { {Symbol, '$'}, {Digit, 0}, { Punct, ','} }, 3, ComplexDigit } */
};

CTextBase::CTextBase(void)
{
    m_strText = NULL;
    m_strFrom = 0;
    m_strSize = 0;

    m_bSourceIsHtml = false;

    m_ProcessingTarget = ForDataBase;

    m_pErrorStream = NULL;
    m_iBadSymbolsCount = 0;
    m_iAllLettersCount = 0;
    m_iUpperLettersCount = 0;
}

CTextBase::~CTextBase(void)
{
    FreeData();
}

//////////////////////////////////////////////////////////////////////////////
// FreeData

void CTextBase::analyzeSentences()
{
    for (size_t i = 0; i < GetSentenceCount(); ++i) {
        Wtroka str = GetSentence(i)->BuildDates();
        if (!str.empty())
            m_strDateFromDateField = str;
    }
}

void CTextBase::ResetPrimitives(void)
{
    m_strText = NULL;
    m_strFrom = 0;
    m_strSize = 0;
    m_iBadSymbolsCount = 0;

    m_vecPrimitive.clear();
    m_vecPrimGroup.clear();
}

void CTextBase::FreeData(void)
{
    m_iAllLettersCount = 0;
    m_iUpperLettersCount = 0;
    m_docAttrs.Reset();
};

//////////////////////////////////////////////////////////////////////////////
// Proceed
void CTextBase::PutAttributes(const SDocumentAttribtes& docAttrs)
{
    m_docAttrs = docAttrs;
    m_DateFromDateFiled = CTime(docAttrs.m_Date);
}

const SDocumentAttribtes& CTextBase::GetAttributes() const
{
    return m_docAttrs;
}

SDocumentAttribtes& CTextBase::GetAttributes()
{
    return m_docAttrs;
}

void CTextBase::Proceed(const Wtroka& pText, int iFrom, int iSize)
{
    ResetPrimitives();

    m_strText = pText;
    m_strFrom = iFrom;
    if (iSize>=0) {
        m_strSize = iSize;
    } else {
        if ((size_t)iFrom <= pText.size())
            m_strSize = pText.size() - iFrom;
        else
            m_strSize = 0;
    }

    createPrimitives();
    if (m_iBadSymbolsCount > (m_strSize * 0.2) && m_strSize > 100)
        ythrow yexception() << Substitute("Too many bad symbols (more then 20%, m_iBadSymbolsCount = $0, m_strSize = $1).",
                                            m_iBadSymbolsCount, m_strSize);

    createPrimGroups();
    if (!m_bSourceIsHtml)
        proceedNewLineBreaks();
    proceedSentEndBreaks();
    createSentences();
}

//////////////////////////////////////////////////////////////////////////////
// Proceed

void CTextBase::ProceedPrimGroups(const Wtroka& pText, int iFrom, int iSize)
{
    FreeData();

    m_strText = pText;
    m_strFrom = iFrom;
    if (iSize>=0) {
        m_strSize = iSize;
    } else {
        if ((size_t)iFrom <= pText.size())
            m_strSize = pText.size() - iFrom;
        else
            m_strSize = 0;
    }

    createPrimitives();
    createPrimGroups();
}

//////////////////////////////////////////////////////////////////////////////
// GetText

Wtroka CTextBase::GetText(const CPrimitive &prim) const
{
    Wtroka str(m_strText.c_str() + prim.m_pos, prim.m_len);
    NStr::RemoveChar(str, CTEXT_REM_SYMBOL);
    str = str.substr(0, MAX_WORD_LEN_N - 2);
    return str;
}

void CTextBase::BuildPrimGroupText(const CPrimGroup &group, Wtroka& str) const
{
    for (size_t i=0; i<group.m_prim.size(); i++) {
        Wtroka tmp(m_strText.c_str() + group.m_prim[i].m_pos, group.m_prim[i].m_len);
        NStr::RemoveChar(tmp, CTEXT_REM_SYMBOL);
        str += tmp;
    }

// refine HTML Escape Sequences
// we should proceed only ETypeOfPrim == 'Punct'
// because 'Space' and 'Symbol' not affect anything
    if (m_bSourceIsHtml && group.m_str[0]=='&') {
        if (NStr::IsEqual(str, "&quot;"))
            NStr::Assign(str, "\"");
        else if (NStr::IsEqual(str, "&apos;"))
            NStr::Assign(str, "'");
        else if (NStr::IsEqual(str, "&shy;"))
            NStr::Assign(str, "-");
        else if (NStr::IsEqual(str, "&laquo;"))
            NStr::Assign(str, "\"");
        else if (NStr::IsEqual(str, "&raquo;"))
            NStr::Assign(str, "\"");
    }

    static const Wtroka shy = CharToWide("&shy;");
    NStr::ReplaceSubstr(str, shy, Wtroka());

    if (group.m_gtyp==Punct && group.m_break==WordBreak)
        NStr::Assign(str, " ");

    if (str.size() >= MAX_WORD_LEN_N)
        str = group.m_str.substr(0, MAX_WORD_LEN_N - 2);
}

Wtroka CTextBase::GetText(CPrimGroup& group) const
{
    if (!group.m_str.empty())
        return group.m_str;
    BuildPrimGroupText(group, group.m_str);
    return group.m_str;
}

CWordBase* CTextBase::CreateWordForLemmatization()
{
    if (m_vecPrimGroup.size() != 1)
        return NULL;
    CPrimGroup &group = m_vecPrimGroup[0];
    CWordBase* pWord = NULL;
    if (group.isForMorph()) {
        pWord = createWord(group, GetText(group));
        CNumber num;
        applyMorphToGroup(pWord, group, num);
    } else if (group.m_gtyp == Complex || group.m_gtyp == DigHyp || group.m_gtyp == ComplexDigitHyp || group.m_gtyp == ComplexDigit) {
        pWord = createWord(group, GetText(group));
        processComplexGroup(pWord, group);
    }
    return pWord;
}

//////////////////////////////////////////////////////////////////////////////
// createPrimitives

void CTextBase::createPrimitives()
{
    m_vecPrimitive.reserve(m_strSize);
    size_t last = m_strFrom + m_strSize;

    for (size_t pos = m_strFrom; pos < last;) {
        size_t end;
        switch (m_charTypeTable.Check(m_strText[pos])) {
            case 'R': // remove
                if (pos + 1 < last && m_charTypeTable.Check(m_strText[pos + 1]) != 'N') {
                    CPrimitive prim(NewLine, pos, 1);
                    m_vecPrimitive.push_back(prim);
                }
                ++pos;
                continue;

            case 'u': // group
            {
                if (static_cast<ui16>(m_strText[pos]) > CTEXT_CTR_SYMBOL)
                    ++m_iBadSymbolsCount;
                for (end = pos + 1; end < last && m_charTypeTable.Check(m_strText[end]) == 'u'; ++end)
                    if (static_cast<ui16>(m_strText[end]) > CTEXT_CTR_SYMBOL)
                        ++m_iBadSymbolsCount;

                CPrimitive prim(UnknownPrim, pos, end - pos);
                m_vecPrimitive.push_back(prim);
                pos = end;
                continue;
            }
            case 'N': // single
            {
                CPrimitive prim(NewLine, pos, 1);
                m_vecPrimitive.push_back(prim);
                ++pos;
                continue;
            }
            case 'Z': // group
            {
                end = pos + 1;
                while (end < last && m_charTypeTable.Check(m_strText[end]) == 'Z')
                    ++end;
                CPrimitive prim(Space, pos, end - pos);
                m_vecPrimitive.push_back(prim);
                pos = end;
                continue;
            }
            case 'P': // single
            {
                CPrimitive prim(Punct, pos, 1);
                m_vecPrimitive.push_back(prim);
                ++pos;
                continue;
            }
            case 'S': // group
            {
                end = pos + 1;
                while (end < last && m_charTypeTable.Check(m_strText[end]) == 'S' && m_strText[end] != '&')
                    ++end;
                while (m_charTypeTable.Check(m_strText[end - 1]) == 'R')
                    --end;
                CPrimitive prim(Symbol, pos, end - pos);
                m_vecPrimitive.push_back(prim);
                pos = end;
                continue;
            }
            case 'D': // group
            {
                end = pos + 1;
                while (end < last && m_charTypeTable.Check(m_strText[end]) == 'D')
                    end++;
                CPrimitive prim(Digit, pos, end - pos);
                m_vecPrimitive.push_back(prim);
                pos = end;
                continue;
            }
            case 'T':
            case 't':
            {
                end = pos + 1;
                while (end < last && (m_charTypeTable.Check(m_strText[end]) == 'T' || m_charTypeTable.Check(m_strText[end]) == 't')) {
                    if (m_charTypeTable.Check(m_strText[end]) == 'T')
                        m_iUpperLettersCount++;
                    m_iAllLettersCount++;
                    end++;
                }
                CPrimitive prim(Word, pos, end - pos);
                m_vecPrimitive.push_back(prim);
                pos = end;
                continue;
            }
            case 'A':
            case 'a':
            {
                end = pos + 1;
                while (end < last && (m_charTypeTable.Check(m_strText[end]) == 'A' || m_charTypeTable.Check(m_strText[end]) == 'a')) {
                    if (m_charTypeTable.Check(m_strText[end]) == 'A')
                        m_iUpperLettersCount++;
                    m_iAllLettersCount++;
                    end++;
                }
                CPrimitive prim(AltWord, pos, end - pos);
                m_vecPrimitive.push_back(prim);
                pos = end;
                continue;
            }
            default:
            {
                YASSERT(false);
                PrintError("Incorrect CharTypeTable");
                pos++;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// createPrimGroups

void CTextBase::createPrimGroups()
{
        size_t num = m_vecPrimitive.size();
        if (num==0)
            return;
        m_vecPrimGroup.reserve(num);
        size_t no;

        for (size_t i=0; i<num;) {
            CPrimGroup group(m_vecPrimitive[i]);

    // last primitive
            if (i+1==num) {
                m_vecPrimGroup.push_back(group);
                break;
            }

    // proceed HTML Control Sequences
            if (m_bSourceIsHtml) {
                no = queryHtmlCtrSeq(group,i+1);
                if (no>0) {
                    //i++;
                    m_vecPrimGroup.push_back(group);
                    i += no;
                    continue;
                }
            }

    // proceed HTML Escape Sequences
            if (m_bSourceIsHtml) {
                no = queryHtmlEscSeq(group,i+1);
                if (no>0) {
                    i++;
                    m_vecPrimGroup.push_back(group);
                    i += no;
                    continue;
                }
            }

            no = queryLongComplexDigit(group,i); // 53 567 000
            if (no>0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

            no = QueryTurkishOrdinalNumberGroup(group, i); // 53.
            if (no > 0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

            //strState = "queryComplexDigit";
            no = queryComplexDigit(group,i); // "14,5" "14,5-15,5" "14-15,5" "14,5-15"
            if (no>0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

            //strState = "queryBucksGoup";
            no = queryBucksGoup(group,i); // $100 000
            if (no>0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

            no = querySlashDigitGroup(group, i);     //  36/38
            if (no > 0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

    // we should proceed only
            if (!m_vecPrimitive[i].isDWA() || m_vecPrimitive[i+1].isNZ()) {
                m_vecPrimGroup.push_back(group);
                i++;
                continue;
            }
            i++;

    // Initial
            no = queryInitialGroup(group,i);
            if (no>0)
                i += no;

    // Abbrev
            no = queryAbbrevGroup(group,i);
            if (no>0) {
                i += no;
                Wtroka str = this->GetText(group);
            }

    // Complex
            //strState = "queryComplexGroup";
            no = queryComplexGroup(group,i);
            if (no>0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

     // Apostrophe(s)
            no = queryApostropheGroup(group,i);
            if (no>0) {
                m_vecPrimGroup.push_back(group);
                i += no;
                continue;
            }

    // Hyphen(s)
            //strState = "queryHyphenGroup";
            no = queryHyphenGroup(group,i);
            if (no>0)
                i += no;

    // Division
            //strState = "queryDivWordGroup";
            no = queryDivWordGroup(group,i);
            if (no>0)
                i += no;

    // WordShy
            //strState = "queryWordShyGroup";
            no = queryWordShyGroup(group,i);
            if (no>0)
                i += no;

    // Ok
            m_vecPrimGroup.push_back(group);
        }
}

//////////////////////////////////////////////////////////////////////////////
// queryHtmlCtrSeq

size_t CTextBase::queryHtmlCtrSeq(CPrimGroup &group,size_t from)
{
    if (m_strText[group.m_pos]!=CTEXT_CTR_SYMBOL)
        return 0;
    size_t num = m_vecPrimitive.size();
    if (from+1>=num || m_strText[m_vecPrimitive[from+1].m_pos]!=CTEXT_CTR_SYMBOL)
        return 0;

// HtmlWB
    if (m_strText[m_vecPrimitive[from].m_pos]=='W') {
        group.m_gtyp = Punct;
        appendPrimitivesToPrimGroup(group,from,from+2);
        group.m_break = WordBreak;
        return 2;
    }
// HtmlSB
    if (m_strText[m_vecPrimitive[from].m_pos]=='S') {
        group.m_gtyp = Space;
        appendPrimitivesToPrimGroup(group,from,from+2);
        group.m_break = SentBreak;
        return 2;
    }
// HtmlPB
    if (m_strText[m_vecPrimitive[from].m_pos]=='P') {
        group.m_gtyp = Space;
        appendPrimitivesToPrimGroup(group,from,from+2);
        group.m_break = ParaBreak;
        return 2;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// queryHtmlEscSeq

size_t CTextBase::queryHtmlEscSeq(CPrimGroup &group,size_t from)
{
    if (m_strText[group.m_pos] != '&')
        return 0;
    size_t num = m_vecPrimitive.size();
    if (from + 1 >= num || m_vecPrimitive[from + 1].m_typ != Punct || m_strText[m_vecPrimitive[from + 1].m_pos] != ';')
        return 0;

    if (!m_vecPrimitive[from].isDWA())
        return 0;

    Wtroka tmp = GetText(m_vecPrimitive[from]);
    if (NStr::IsEqual(tmp, "nbsp")  || NStr::IsEqual(tmp, "160")) {
        group.m_gtyp = Space;
        appendPrimitivesToPrimGroup(group, from, from + 2);
        return 2;
    } else if (NStr::IsEqual(tmp, "shy")) {
        group.m_gtyp = Punct;
        appendPrimitivesToPrimGroup(group, from, from + 2);
        return 2;
    } else if (NStr::IsEqual(tmp, "apos") || NStr::IsEqual(tmp, "quot")) {
        group.m_gtyp = Punct;
        appendPrimitivesToPrimGroup(group, from, from + 2);
        return 2;
    } else if (NStr::IsEqual(tmp, "laquo") || NStr::IsEqual(tmp, "raquo")) {
        group.m_gtyp = Punct;
        appendPrimitivesToPrimGroup(group, from, from + 2);
        return 2;
    } else {
        group.m_gtyp = Symbol;
        appendPrimitivesToPrimGroup(group, from, from + 2);
        return 2;
    }
}

//////////////////////////////////////////////////////////////////////////////
// queryInitialGroup T. Tt. A. Aa.

DECLARE_STATIC_NAMES(TDoubleCharInitials, "Ал Ан Вл Вч Вс Дм Ев Ел Ст Дж");

size_t CTextBase::queryInitialGroup(CPrimGroup &group, size_t from)
{
    if (from >= m_vecPrimitive.size())
        return 0;

    ETypeOfPrim type = Initial;
    if (!group.isWA() || m_vecPrimitive[from].m_typ != Punct || m_strText[m_vecPrimitive[from].m_pos] != '.')
        return 0;

    Wtroka text = GetText(group);

    if (m_charTypeTable.Check(text.at(0)) == 'A')
        type = AltInitial;

    bool bFirstIsUpper = ::IsUpper(m_charTypeTable.Check(text.at(0)));
    if( IgnoreUpperCase() )
        bFirstIsUpper = true;

    if (group.m_len == 1 && bFirstIsUpper )
    {
        group.m_gtyp = type;
        appendPrimitivesToPrimGroup(group, from, from + 1);
        return 1;
    }

    if (group.m_len == 2 && ::IsUpper(m_charTypeTable.Check(text.at(0))) &&
                            ::IsLower(m_charTypeTable.Check(text.at(1)))) {
        if (!TDoubleCharInitials::Has(text))
            return 0;

        if (m_charTypeTable.Check(text.at(1)) == 'A')
            type = AltInitial;

        group.m_gtyp = type;
        appendPrimitivesToPrimGroup(group, from, from + 1);
        return 1;
    }
    return 0;
}

static bool RecognizeAbbrev(Wtroka& text, CPrimGroup& group, bool altLang) {
    TMorph::ToLower(text);
    ETypeOfAbbrev abbrType;
    if (!GlobalLangData->HasAbbrev(text, abbrType))
        return false;

    switch (abbrType) {
        case Standard: group.m_gtyp = altLang ? AbbEosAlt : AbbEos; break;
        case NewerEOS: group.m_gtyp = altLang ? AbbAlt : Abbrev; break;
        case DigitEOS: group.m_gtyp = altLang ? AbbDigAlt : AbbDig; break;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// queryAbbrevGroup [t]. | [t].[t] | [t]. [t]. | [t].-[t].

size_t CTextBase::queryAbbrevGroup(CPrimGroup& group, size_t from)
{
    if (from >= m_vecPrimitive.size())
        return 0;

    const CPrimitive& punct = m_vecPrimitive[from];
    if (!group.isWA() || (!isPunctDot(punct) && !isPunctHyphen(punct)))
        return 0;

    size_t num = m_vecPrimitive.size();
    Wtroka tmp = GetText(group);
    tmp += m_strText[punct.m_pos];

    if (group.m_gtyp == Word) {
        Wtroka tmp2 = tmp;
        size_t no = 0;

        if (from + 2 < num &&  m_vecPrimitive[from + 1].m_typ == Word && isPunctDot(m_vecPrimitive[from + 2])) {
            no = 2;
        } else if (from + 3 < num && m_vecPrimitive[from + 1].m_typ == Space && m_vecPrimitive[from + 1].m_len < 2
                                && m_vecPrimitive[from + 2].m_typ == Word
                                && isPunctDot(m_vecPrimitive[from + 3])) {
            no = 3;
        } else if (from + 3 < num && isPunctHyphen(m_vecPrimitive[from + 1])
                                && m_vecPrimitive[from + 2].m_typ == Word
                                && isPunctDot(m_vecPrimitive[from + 3])) {
            no = 3;
            tmp2 += '-';
        }

        if (no > 0) {
            tmp2 += GetText(m_vecPrimitive[from + no - 1]);
            tmp2 += '.';
            if (RecognizeAbbrev(tmp2, group, false)) {
                appendPrimitivesToPrimGroup(group, from, from + 1 + no);
                return no + 1;
            }
        } // tmp2

        if (RecognizeAbbrev(tmp, group, false)) {
            appendPrimitivesToPrimGroup(group, from, from + 1);
            return 1;
        }
    } // Word

    if (group.m_gtyp == AltWord) {
        //РїРѕРїС‹С‚Р°РµРјСЃСЏ РµС‰Рµ РїРѕРёСЃРєР°С‚СЊ РІ СЂРѕРґРЅРѕРј m_mapAbbrev, С‚Р°Рј С‚РѕРґРµРјРѕРіСѓС‚ Р±С‹С‚СЊ alt-СЃРѕРєСЂР°С‰РµРЅРёСЏ
        if (RecognizeAbbrev(tmp, group, true)) {
            appendPrimitivesToPrimGroup(group, from, from + 1);
            return 1;
        }
    }  // AltWord

    return 0;
}

//РїС‹С‚Р°РµРјСЃСЏ РёСЃРїСЂР°РІР»СЏС‚СЊ РѕС€РёР±РєРё, РєРѕРіРґР° РїРѕСЃР»Рµ РєРѕРЅС†Р° РїСЂРµРґР»РѕР¶РµРЅРёСЏ Р·Р°Р±С‹Р»Рё РїРѕСЃС‚Р°РІРёС‚СЊ РїСЂРѕР±РµР»
//РІ СЌС‚РёС… СЃР»СѓС‡Р°СЏС… РЅРµ СЃРєР»РµРёРІР°РµРј СЃР»РѕРІР°, РЅРѕ РёС… РЅСѓР¶РЅРѕ РѕС‚Р»РёС‡Р°С‚СЊ РѕС‚ РЅР°Р·РІР°РЅРёР№ С‚РёРїР° "РЇРЅРґРµРєСЃ.Р”РµРЅСЊРіРё"
bool CTextBase::CanIncludePointBetweenWords(size_t from)
{
    size_t num = m_vecPrimitive.size();
    if (from + 3 >= num)
        return true;
    ETypeOfPrim typCur = m_vecPrimitive[from].m_typ;
    if (typCur != Word)
        return true;

    size_t posPoint = m_vecPrimitive[from+1].m_pos;
    if (m_strText[posPoint] != '.')
        return true;
    if (m_vecPrimitive[from+2].m_typ != Word)
        return true;
    size_t posLast = m_vecPrimitive[from+2].m_pos;
    if (m_charTypeTable.Check(m_strText[posLast]) != 'T' &&
        m_charTypeTable.Check(m_strText[posLast]) != 't')
        return true;

    if (!m_vecPrimitive[from+3].isNZ() &&
        !(m_strText[m_vecPrimitive[from+3].m_pos] == ',') &&
        !(m_strText[m_vecPrimitive[from+3].m_pos] == ':'))
        return true;

    Wtroka strFirstPart = GetText(m_vecPrimitive[from]);
    TMorph::ToLower(strFirstPart);
    Wtroka strLastpart = GetText(m_vecPrimitive[from+2]);
    TMorph::ToLower(strLastpart);
    Wtroka strWhole = strFirstPart + '.' + strLastpart;

    if (strFirstPart.size() <= 2)
        return true;

    if (GlobalLangData->HasWordWithPoint(strFirstPart + '.') ||
        GlobalLangData->HasWordWithPoint(Wtroka((wchar16)'.') + strLastpart) ||
        GlobalLangData->HasWordWithPoint(strWhole))
        return true;

    return false;
}

//////////////////////////////////////////////////////////////////////////////
// queryComplexGroup

size_t CTextBase::queryComplexGroup(CPrimGroup &group,size_t from)
{
    if (!group.isDWA())
        return 0;
    //ETypeOfPrim typBeg = group.m_gtyp;
    ETypeOfPrim typLst = group.m_gtyp;
    bool bComplex = false;

    size_t num = m_vecPrimitive.size();
    size_t no=from;
    for (; no<num;) {
        ETypeOfPrim typCur = m_vecPrimitive[no].m_typ;

// Directly joined as 28B etc.
        if (typCur==Digit || typCur==Word || typCur==AltWord) {
            if (typLst!=Punct && typLst!=Symbol && typCur!=typLst)
                bComplex = true;
            typLst = typCur;
            no++;
            continue;
        }

// Indirectly joined
        size_t nNxt = no+1;
        if (nNxt>=num)
            break;
        ETypeOfPrim typNxt = m_vecPrimitive[nNxt].m_typ;
        if (typNxt!=Digit && typNxt!=Word && typNxt!=AltWord)
            break;

// Any + [P] ('.' or '-' or ':')
// Digit + ',' + Digit
// or single S (not | not &)

        bool bChkJoin = false;
        size_t pos = m_vecPrimitive[no].m_pos;
        if (typCur==Punct && m_strText[pos]=='.') {
            bComplex = true;
            if (!CanIncludePointBetweenWords(no-1))
                bComplex = false;
            bChkJoin = true;
        }
        if (typCur==Punct && m_strText[pos]==':') {
            bComplex = true;
            bChkJoin = true;
        }
        if (typCur==Punct && m_strText[pos]=='-')
            bChkJoin = true;

        //if( typLst==Digit && typCur==Punct && m_strText[pos]==',' && typNxt==Digit )
        //{
        //  bComplex = true;
        //  bChkJoin = true;
        //}

        if (typCur==Symbol && m_vecPrimitive[no].m_len==1 && m_strText[pos]!='|' && m_strText[pos]!='&') {
            bComplex = true;
            bChkJoin = true;
        }
        if (!bChkJoin)
            break;
        typLst = typCur;

// Next
// if single P ('.' or '-' or ':') can't mix words
        if (typCur==Punct && ((typLst==Word && typNxt==AltWord) || (typLst==AltWord && typNxt==Word)))
            break;

        no++;
    }

    if (no==from || !bComplex)
        return 0;

    group.m_gtyp = Complex;
    appendPrimitivesToPrimGroup(group,from,no);

    //РїРѕРїС‹С‚Р°РµРјСЃСЏ СЂРµС€РёС‚СЊ РїСЂРѕР±Р»РµРјСѓ СЃ Р»Р°С‚РёРЅСЃРєРёРјРё Р±СѓРєРІР°РјРё РІ СЂСѓСЃСЃРєРѕРј СЃР»РѕРІРµ Рё РЅР°РѕР±РѕСЂРѕС‚
    ReplaceSimilarLetters(group);

    return(no-from);
}

//////////////////////////////////////////////////////////////////////////////
// queryApostropheGroup t't, a'a etc.

size_t CTextBase::queryApostropheGroup(CPrimGroup &group,size_t from)
{
    ETypeOfPrim typ = group.m_gtyp;
    size_t no = from;
    size_t num = m_vecPrimitive.size();
    while (no<num) {
        if (m_vecPrimitive[no].m_typ != Punct || m_vecPrimitive[no].m_len != 1)
            break;

        wchar16 ch = m_strText[m_vecPrimitive[no].m_pos];
        // also allow double quotation mark to be a primitive separator in a word (for Turkish only)
        if (!::IsSingleQuotation(ch) && !(::IsQuotation(ch) && TMorph::GetMainLanguage() == LANG_TUR))
            break;
        no++;

        if (no>=num || m_vecPrimitive[no].m_typ!=typ)
            return 0;
        no++;
    }

    if (no==from)
        return 0;

    appendPrimitivesToPrimGroup(group,from,no);
    return(no-from);
}

size_t CTextBase::query2DigHyphenGroup(CPrimGroup &group,size_t from)
{
    size_t no = from;
    //size_t num = m_vecPrimitive.size();

    if ((m_vecPrimitive[no].m_typ == Symbol) &&
        (m_strText[m_vecPrimitive[no].m_pos] == '$')
       )
       no++;

    if ((no + 2) >= m_vecPrimitive.size())
        return 0;

    if (m_vecPrimitive[no].m_typ != Digit)
        return 0;

    if (!((m_vecPrimitive[no+1].m_typ == Punct) &&
        (m_strText[m_vecPrimitive[no+1].m_pos] == '-'))
       )
       return 0;

    if ((m_vecPrimitive[no+2].m_typ == Symbol) &&
        (m_strText[m_vecPrimitive[no+2].m_pos] == '$')
       )
       no++;

    if ((no + 2) >= m_vecPrimitive.size())
        return 0;

    if (m_vecPrimitive[no+2].m_typ != Digit)
        return 0;
    if (!isGroupBorder(no+3))
        return 0;

    group.m_gtyp = ComplexDigitHyp;
    appendPrimitivesToPrimGroup(group,from +1 ,no + 3);
    return(no-from + 3);
}

//////////////////////////////////////////////////////////////////////////////
// queryHyphenGroup t-t, a-a, D-t, a-t, etc.

size_t CTextBase::queryHyphenGroup(CPrimGroup &group,size_t from)
{
    if (from >= m_vecPrimitive.size())
        return 0;

    size_t no = from;
    size_t num = m_vecPrimitive.size();
    bool bDigHyp = (group.m_gtyp==Digit);

    while (no+1<num) {
        if (m_vecPrimitive[no].m_typ!=Punct || m_strText[m_vecPrimitive[no].m_pos]!='-'
        || !m_vecPrimitive[no+1].isDWA()
        )
            break;

        if (m_vecPrimitive[no+1].m_typ!=Digit)
            bDigHyp = false;

        no += 2;
    }

    if (no==from)
        return 0;

    if (bDigHyp)
        group.m_gtyp = DigHyp;
    else
        group.m_gtyp = Hyphen;
    appendPrimitivesToPrimGroup(group,from,no);
    return(no-from);
}
//////////////////////////////////////////////////////////////////////////////
// query3DigitChain

bool CTextBase::is3DigitChainDelimeter(size_t from)
{
    if (from >= m_vecPrimitive.size())
        return false;
    return  (m_vecPrimitive[from].isNZ() && (m_vecPrimitive[from].m_len == 1)) ||
            ((m_vecPrimitive[from].m_typ == Punct) && (m_strText[m_vecPrimitive[from].m_pos] == '.'));
}

size_t CTextBase::query3DigitChain(size_t from) //С†РµРїРѕС‡РєР° РёР· Digit , РїРѕ 3 С€С‚СѓРєРµ РІ РєР°Р¶РґРѕР№ , СЂР°Р·РґРµР»РµРЅРЅС‹С… РѕРґРЅРёРј(!) РїСЂРѕР±РµР»РѕРј
{
    if (from >= m_vecPrimitive.size())
        return from;
    bool bPrevIsSpace = false;
    if (is3DigitChainDelimeter(from)) {
        bPrevIsSpace = true;
        from++;
    } else
        return from;

    for (/*size_t i = from*/; from < m_vecPrimitive.size(); from++) {
        if (bPrevIsSpace) {
            if (!((m_vecPrimitive[from].m_typ == Digit) && (m_vecPrimitive[from].m_len == 3)))
                return from - 1;        // potentially dangerous
            else
                bPrevIsSpace = false;
        } else {
            if (!is3DigitChainDelimeter(from))
                return from;
            else
                bPrevIsSpace = true;
        }
    }
    if (bPrevIsSpace)
        return from - 1;    // potentially dangerous
    else
        return from;
}

//////////////////////////////////////////////////////////////////////////////
// queryLongComplexDigit

size_t CTextBase::queryLongComplexDigit(CPrimGroup &group,size_t from)
{
    if (m_vecPrimitive[from].m_typ != Digit)
        return 0;
    size_t kk = query3DigitChain(from + 1);
    if (kk > from + 1) {
        if (!isGroupBorder(kk))
            return 0;
        appendPrimitivesToPrimGroup(group,from + 1,kk);
        group.m_gtyp = ComplexDigit;
        return kk - from;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// queryComplexDigit

size_t CTextBase::queryComplexDigit(CPrimGroup &group,size_t from)
{
    if (from > 0)
        if  (!(m_vecPrimitive[from-1].isNZ() ||
                    ((m_vecPrimitive[from-1].m_typ == Punct) &&
                      (m_strText[m_vecPrimitive[from - 1].m_pos] != '-') &&
                      (m_strText[m_vecPrimitive[from - 1].m_pos] != '.') &&
                      (m_strText[m_vecPrimitive[from - 1].m_pos] != ',')
                     )
                )
            )
            return 0;

    if (!((m_vecPrimitive[from].m_typ == Symbol) ||
            (m_vecPrimitive[from].m_typ == Digit) ||
            ((from < m_vecPrimitive.size() - 1) &&
                (
                    (m_vecPrimitive[from].m_typ == AltWord) ||
                    (m_vecPrimitive[from].m_typ == Word)
                )&&
                (m_vecPrimitive[from].m_len == 1) &&
                (m_vecPrimitive[from+1].m_typ == Digit)
            )
         )
      ) {
        return 0;
    }

    for (int i = 0; i < g_ComplexNumberSchemesCount; i++) {

        if (CompareToScheme(g_ComplexNumberSchemes[i], from)) {
            appendPrimitivesToPrimGroup(group, from + 1, from + g_ComplexNumberSchemes[i].m_Len);
            group.m_gtyp = g_ComplexNumberSchemes[i].m_ResType;
            return g_ComplexNumberSchemes[i].m_Len;
        }
    }
    return 0;
}

bool CTextBase::CompareToScheme(SPrimScheme& primScheme, size_t from)
{
    size_t i = from;
    for (; (i < m_vecPrimitive.size()) && ((int)(i - from) < primScheme.m_Len); i++) {
        if (m_vecPrimitive[i].m_typ != primScheme.m_Prims[i - from].m_Type)
            return false;
        if (primScheme.m_Prims[i - from].m_Symbol != 0)
            if (m_strText[m_vecPrimitive[i].m_pos]!= primScheme.m_Prims[i - from].m_Symbol)
                return false;
    }
    if ((int)(i - from) != primScheme.m_Len)
        return false;
    if (!isGroupBorder(i))
        return false;

    return true;
}

bool CTextBase::isGroupBorder(size_t no)
{
    if (no >= m_vecPrimitive.size())
        return true;

    CPrimitive& cur = m_vecPrimitive[no];
    if (cur.isNZ())
        return true;

    if (isPunctSentEnd(cur) || isPunctComma(cur) || isPunctColon(cur) || isPunctSemicolon(cur) ||  isPunctQuote(cur) ||
        isPunctLeftBracket(cur) || isPunctRightBracket(cur) || IsPunctChar(cur, '/')) {
        if (no + 1 >= m_vecPrimitive.size())
            return true;
        CPrimitive& next = m_vecPrimitive[no + 1];
        if (next.isNZ())
            return true;
        wchar16 c = m_strText[next.m_pos];
        if (m_charTypeTable.IsUpper(c))
            return true;

        if (isPunctRightBracket(cur) &&
            (isPunctSentEnd(next) || isPunctComma(next) || isPunctColon(next) || isPunctSemicolon(next) || isPunctQuote(next)))
            return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////
// queryBucksGoup

size_t CTextBase::queryBucksGoup(CPrimGroup &group,size_t from)
{
    if (from > 0)
        if (!m_vecPrimitive[from-1].isNZ() && (m_vecPrimitive[from-1].m_typ != Punct))
            return 0;
    if (from >= m_vecPrimitive.size() - 1)
        return 0;
    if (m_vecPrimitive[from].m_typ != Symbol)
        return 0;
    Wtroka str = GetText(m_vecPrimitive[from]);
    if (str.size() != 1)
        return 0;
    if (str[0] != '$')
        return 0;
    if (m_vecPrimitive[from + 1].m_typ != Digit)
        return 0;
    if (from + 2 < m_vecPrimitive.size())
        if (!m_vecPrimitive[from+2].isNZ() &&
            !((m_vecPrimitive[from+2].m_typ == Punct) &&
                ((m_strText[m_vecPrimitive[from+2].m_pos] == '.') ||
                 (m_strText[m_vecPrimitive[from+2].m_pos] == '!') ||
                 (m_strText[m_vecPrimitive[from+2].m_pos] == '?') ||
                (m_strText[m_vecPrimitive[from+2].m_pos] == ','))))
            return 0;
    size_t kk = query3DigitChain(from + 2);
    if (!isGroupBorder(kk))
        return 0;
    appendPrimitivesToPrimGroup(group,from + 1,kk);
    group.m_gtyp = ComplexDigit;
    return kk - from;
}

size_t CTextBase::querySlashDigitGroup(CPrimGroup& group, size_t start)
{
    if (start > 0) {
        CPrimitive& prev = m_vecPrimitive[start - 1];
        if (!prev.isNZ() && prev.m_typ != Punct)
            return 0;
    }

    if (m_vecPrimitive[start].m_typ == Digit) {
        size_t stop = start + 1;
        for (; stop < m_vecPrimitive.size() && IsPunctChar(m_vecPrimitive[stop], '/'); ++stop) {
            if (stop + 1 < m_vecPrimitive.size() && m_vecPrimitive[stop + 1].m_typ == Digit)
                stop += 1;
        }

        size_t groupLen = stop - start;
        if (groupLen > 1 && isGroupBorder(stop)) {
            appendPrimitivesToPrimGroup(group, start + 1, stop);
            group.m_gtyp = ComplexDigit;
            return groupLen;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// queryPercentGoup

size_t CTextBase::queryPercentGoup(CPrimGroup &group,size_t from)
{
    if (from > 0)
        if (!m_vecPrimitive[from-1].isNZ() && (m_vecPrimitive[from-1].m_typ != Punct))
            return 0;
    if (from >= m_vecPrimitive.size() - 1)
        return 0;
    if (m_vecPrimitive[from].m_typ != Digit)
        return 0;
    if (m_vecPrimitive[from + 1].m_typ != Symbol)
        return 0;
    Wtroka str = GetText(m_vecPrimitive[from + 1]);
    if (str.size() != 1)
        return 0;
    if (str[0] != '%')
        return 0;
    if (!isGroupBorder(from + 2))
        return 0;
    appendPrimitivesToPrimGroup(group,from + 1,from + 2);
    group.m_gtyp = ComplexDigit;
    return 2;
}

// A number with adjacent period after, meaning an ordinal number in Trukish. E.g. "76. Sokak" -> "76."
size_t CTextBase::QueryTurkishOrdinalNumberGroup(CPrimGroup& group, size_t start) {

    if (TMorph::GetMainLanguage() != LANG_TUR)
        return 0;

    if (m_vecPrimitive[start].m_typ != Digit)
        return 0;

    if (start + 1 >= m_vecPrimitive.size() || !isPunctDot(m_vecPrimitive[start + 1]))
        return 0;

    // next token should be a space
    if (start + 2 >= m_vecPrimitive.size() || !m_vecPrimitive[start + 2].isNZ())
        return 0;

    // next token should be a word
    if (start + 3 >= m_vecPrimitive.size() || !m_vecPrimitive[start + 3].isWA())
        return 0;

    if (start > 0) {
        if (!m_vecPrimitive[start - 1].isNZ())
            return 0;
        // do not allow "11 222." for such group
        if (start > 1 && m_vecPrimitive[start - 2].m_typ == Digit)
            return 0;
    }

    appendPrimitivesToPrimGroup(group, start + 1, start + 2);
    group.m_gtyp = Complex;
    return 2;
}

//////////////////////////////////////////////////////////////////////////////
// queryDivWordGroup t-[S]N[S]t, a-[S]N[S]a

size_t CTextBase::queryDivWordGroup(CPrimGroup &group,size_t from)
{
    size_t no = from;
    if (no >= m_vecPrimitive.size())
        return 0;

    if (m_vecPrimitive[no].m_typ!=Punct || m_vecPrimitive[no].m_len!=1 || m_strText[m_vecPrimitive[no].m_pos]!='-')
        return 0;

    size_t num = m_vecPrimitive.size();
    no++;
    if (no>=num)
        return 0;
    if (m_vecPrimitive[no].m_typ==Space)
        no++;
    if (no>=num)
        return 0;
    if (m_vecPrimitive[no].m_typ!=NewLine)
        return 0;
    no++;
    if (no>=num)
        return 0;
    if (m_vecPrimitive[no].m_typ==Space)
        no++;
    if (no>=num)
        return 0;
    if (!m_vecPrimitive[no].isDWA())
        return 0;

    if (group.m_gtyp == Hyphen)
        group.m_gtyp = HypDiv;
    else
        group.m_gtyp = DivWord;

    no++;
    appendPrimitivesToPrimGroup(group,from,no);

    return(no-from);
}

//////////////////////////////////////////////////////////////////////////////
// queryWordShy РёР·РІРµ&shy;СЃС‚РЅРѕ

size_t CTextBase::queryWordShyGroup(CPrimGroup &group,size_t from)
{
    if (from >= m_vecPrimitive.size())
        return 0;

    if (from + 3 < m_vecPrimitive.size() &&
        m_strText[m_vecPrimitive[from].m_pos] == '&' &&
        m_strText[m_vecPrimitive[from + 2].m_pos] == ';' &&
        NStr::IsEqual(GetText(m_vecPrimitive[from + 1]), "shy")) {
        group.m_gtyp = WordShy;
        appendPrimitivesToPrimGroup(group, from, from + 4);
        return 4;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// appendPrimitivesToPrimGroup

void CTextBase::appendPrimitivesToPrimGroup(CPrimGroup &group,size_t noBegPrim,size_t noEndPrim)
{
    for (size_t j=noBegPrim; j<noEndPrim; j++)
        group.m_prim.push_back(m_vecPrimitive[j]);

    group.m_len = (group.m_prim[group.m_prim.size()-1].m_pos + group.m_prim[group.m_prim.size()-1].m_len)
                    - group.m_pos;

// lazy
    group.m_str.clear();
}

//////////////////////////////////////////////////////////////////////////////
// proceedNewLineBreaks

void CTextBase::proceedNewLineBreaks()
{
    size_t num = m_vecPrimGroup.size();
    if (num==0)
        return;
    size_t endLine1=0,begLine2=0,endLine2=0;

    size_t p1beg=0,p1end=0,p2beg=0,p2end=0,s1left=0,s2left=0;
    CPrimGroup *g1beg=0,*g1end=0,*g2beg=0,*g2end=0;

    while (endLine2<num && !m_vecPrimGroup[endLine2].isNewLine())
        endLine2++;
    fetchLineInfo(begLine2,endLine2,p2beg,p2end,&g2beg,&g2end,s2left);

    while (endLine2<num) {
        endLine1 = endLine2;
        p1beg = p2beg;
        p1end = p2end;
        g1beg = g2beg;
        g1end = g2end;
        s1left = s2left;

        begLine2 = endLine1+1;
        endLine2 = begLine2;
        while (endLine2<num && !m_vecPrimGroup[endLine2].isNewLine())
            endLine2++;
        fetchLineInfo(begLine2,endLine2,p2beg,p2end,&g2beg,&g2end,s2left);

// empty line
        if (g1beg==NULL)
            continue;

// empty line after non-empty line -> ParaBreak
        if (g2beg==NULL) {
            g1end->m_break = ParaBreak;
            continue;
        }
// from here all is valid

// pure graphic line (as table "======" etc) -> ParaBreak + next SentBreak
        bool bPS = true;
        for (size_t i=p1beg; i<=p1end; i++) {
            if (!m_vecPrimGroup[i].isPS() && !m_vecPrimGroup[i].isSpace()) {
                bPS = false;
                break;
            }
        }
        if (bPS) {
            if (p1beg>0)
                m_vecPrimGroup[p1beg-1].m_break = ParaBreak;
            g1end->m_break = SentBreak;
            continue;
        }

// line starts with D. or D). -> SentBreak
        if (p2beg<p2end
        &&  m_vecPrimGroup[p2beg].m_gtyp==Digit
        &&  isPunctDot(m_vecPrimGroup[p2beg+1])
        ) {
            g1end->m_break = SentBreak;
        }
        if (p2beg+1<p2end
        &&  m_vecPrimGroup[p2beg].m_gtyp==Digit
        &&  m_vecPrimGroup[p2beg+1].m_gtyp==Punct && GetText(m_vecPrimGroup[p2beg+1]).at(0)==')'
        &&  isPunctDot(m_vecPrimGroup[p2beg+2])
        ) {
            g1end->m_break = SentBreak;
        }

// prev line ends with ',' ';' ':'
// next line begs with '-' '*' -> SentBreak (ParaBreak?)
        wchar16 cend = GetText(*g1end).back();
        wchar16 cbeg = GetText(*g2beg).at(0);

        if ((cend == ',' || cend == ';' || cend == ':') && (cbeg == '-' || cbeg == '*')) {
            g1end->m_break = SentBreak;
            continue;
        }

        if (s1left >= s2left)
            continue;
// from here we have shift right

// new line begs with [D] '"' '-'
        if (g2beg->hasFirstD()
        ||  GetText(*g2beg).at(0)=='"'
        ||  GetText(*g2beg).at(0)=='-'
        ) {
            g1end->m_break = ParaBreak;
            continue;
        }
// new line begs with [T] [A]
        if (g2beg->hasFirstWA() && hasFirstUpper(*g2beg)) {
            g1end->m_break = ParaBreak;
            continue;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// fetchLineInfo

void CTextBase::fetchLineInfo(size_t beg,size_t end,
        size_t &pbeg,size_t &pend,CPrimGroup **g1,CPrimGroup **g2,size_t &sleft)
{
    *g1 = NULL;
    *g2 = NULL;
    sleft = 0;

    if (beg>=end)
        return;

    for (pbeg=beg; pbeg<end; pbeg++) {
        if (!m_vecPrimGroup[pbeg].isSpace()) {
            *g1 = &(m_vecPrimGroup[pbeg]);
            break;
        }
    }

    for (pend=end-1; pend>=pbeg; pend--) {
        if (!m_vecPrimGroup[pend].isSpace()) {
            *g2 = &(m_vecPrimGroup[pend]);
            break;
        }
    }

    if (pbeg<end) {
        for (size_t i=beg; i<pbeg; i++)
            sleft += GetText(m_vecPrimGroup[i]).size();
    }
}

//////////////////////////////////////////////////////////////////////////////
// proceedSentEndBreaks

void CTextBase::proceedSentEndBreaks()
{
    int num = (int)m_vecPrimGroup.size();
    if (num<=0) // !required - later we assume num>0
        return;

    for (int i=0;;) {
        while (i<num && !isPunctSentEnd(m_vecPrimGroup[i]) && !m_vecPrimGroup[i].isEOS())
            i++;
        if (i>=num)
            break;

// append [S]
        int b=i+1;
        while (b<num && m_vecPrimGroup[b].isNZ())
            b++;
        if (b>=num)
            break;

// DigEos (Leva)
        if (m_vecPrimGroup[i].m_gtyp==AbbDig || m_vecPrimGroup[i].m_gtyp==AbbDigAlt)
            if ((m_vecPrimGroup[b].m_gtyp==Digit) ||
                //РґР»СЏ РІСЃСЏРєРёС… РЅРѕРјРµСЂРѕРІ РґРѕРјРѕРІ С‚РёРїР° 32\21
                (m_vecPrimGroup[b].m_gtyp==DigHyp) ||
                ((m_vecPrimGroup[b].m_gtyp == Complex) &&
                    (m_vecPrimGroup[b].hasFirstDigits()) //РµСЃР»Рё РЅР°С‡РёРЅР°РµС‚СЃСЏ СЃ С†РёС„СЂ
                )
               ) {
                i = b+1;
                continue;
            }

// move to )
        if (isPunctRightBracket(m_vecPrimGroup[b]))
            i = b; // move here

// append single P not ','';'':''-''('
        int p=i+1;
        while (p<num && m_vecPrimGroup[p].m_gtyp==Punct) {
            if (isPunctComma(m_vecPrimGroup[p]) || isPunctSemicolon(m_vecPrimGroup[p])
            ||  isPunctColon(m_vecPrimGroup[p]) || isPunctHyphen(m_vecPrimGroup[p])
            ||  isPunctLeftBracket(m_vecPrimGroup[p]))
                break;
            p++;
        }
        if (p>=num)
            break;

// exclude when last is single P ','';'':'
        if (isPunctComma(m_vecPrimGroup[p]) ||
            isPunctSemicolon(m_vecPrimGroup[p]) ||
            isPunctColon(m_vecPrimGroup[p])) {
            i = p+1;
            continue;
        }
        i = p-1; // move here

// find what after
        size_t k = i + 1;
        while ((int)k < num && m_vecPrimGroup[k].isNZ())
            k++;
        if ((int)k >= num)
            break;

        size_t kSave = k;
        if (isDirectPartOfSpeechQuote(k)) {
            i = k;
            continue;
        } else
            k = kSave;
// k is on next group
        CPrimGroup gr = m_vecPrimGroup[k];
        if (hasFirstLower(gr) || isPunctComma(gr) || isPunctLeftBracket(gr)) {
            i = k;
            continue;
        }

        if (m_vecPrimGroup[i].m_break != ParaBreak)
            m_vecPrimGroup[i].m_break = SentBreak;
        i = k;
    }
}

//СЃС‚РѕРёРј РЅР° СЃРёРјРІРѕР»Рµ РїРѕСЃР»Рµ "." РёР»Рё "!"  РёР»Рё "?" (Рё РїРѕСЃР»Рµ РїСЂРѕР±РµР»РѕРІ)
//РїС‹С‚Р°РµРјСЃСЏ РїРѕРЅСЏС‚СЊ РјРѕР¶РµС‚ Р»Рё СЌС‚Рѕ Р±С‹С‚СЊ РїСЂСЏРјРѕР№ СЂРµС‡СЊСЋ
bool CTextBase::isDirectPartOfSpeechQuote(size_t& k)
{
    size_t num = m_vecPrimGroup.size();
    if (!isPunctQuote(m_vecPrimGroup[k]))
        return false;

    k++;

    while (k < num && m_vecPrimGroup[k].isNZ())
        k++;
    if (k >= num)
        return true;
    if (!(isPunctComma(m_vecPrimGroup[k]) || isPunctSemicolon(m_vecPrimGroup[k]) || isPunctColon(m_vecPrimGroup[k])))
        return false;

        k++;

    while (k < num && m_vecPrimGroup[k].isNZ())
        k++;
    if (k >= num)
        return true;

    if (hasFirstLower(m_vecPrimGroup[k]) || isPunctHyphen(m_vecPrimGroup[k]) || isPunctComma(m_vecPrimGroup[k]) || isPunctLeftBracket(m_vecPrimGroup[k])) {
        return true;
    }
    return false;

}

//////////////////////////////////////////////////////////////////////////////
// createSentences

void CTextBase::createSentences()
{
    size_t num = m_vecPrimGroup.size();
    size_t from = 0;
    int pCnt = 0;

    while (from<num) {
        size_t size=0;
        size_t pos = m_vecPrimGroup[from].m_pos;

        while (from < num && (m_vecPrimGroup[from].m_gtyp==Space || m_vecPrimGroup[from+size].m_gtyp==UnknownPrim)) {
            from++;
        }

        if (from >= num)
            break;

        while (from+size < num &&
            (m_vecPrimGroup[from+size].m_break==NoBreak || m_vecPrimGroup[from+size].m_break==WordBreak) &&
            size < (MAX_WORD_IN_SENT/5) &&
            (m_vecPrimGroup[from+size].m_pos + m_vecPrimGroup[from+size].m_len - pos) < 0xFFFF
          ) {
            //if( size == 5040 )
            //  int vvv = 0;
            size++;
        }
        //РґР»СЏ РЅРµРІРµСЂРѕСЏС‚РЅРѕ РґР»РёРЅРЅС‹С… СЃР»РѕРІ
        if ((size == 0) && (m_vecPrimGroup[from+size].m_len >= 0xFFFF)) {
            from++;
            continue;
        }

        if ((from+size<num) && (m_vecPrimGroup[from+size].m_pos + m_vecPrimGroup[from+size].m_len - pos) < 0xFFFF)
            size++;

        bool bEmpty = true;
        for (size_t i=0; i<size; i++) {
            if (!m_vecPrimGroup[from+i].isNZ() && !m_vecPrimGroup[from+i].isPS()) {
                bEmpty = false;
                break;
            }
        }
        if (bEmpty) {
            if (m_vecPrimGroup[from+size-1].m_break==ParaBreak)
                markLastSentParEnd();
            from += size;
            continue;
        }

        CSentenceBase *pSent = createSentence(from,size);

// ParaBreak
        if (pSent) {
            if (m_vecPrimGroup[from+size-1].m_break==ParaBreak || (pCnt++ >= (MAX_SENT_IN_PARAGRAPH - 1))) {
                pSent->m_bPBreak = true;
                pCnt = 0;
            }

            AddSentence(pSent);
        }

        from += size;
    }
    markLastSentParEnd();
}

//////////////////////////////////////////////////////////////////////////////
// FreeExternOutput

void CTextBase::FreeExternOutput(yvector<CSentenceBase *> &vecSentences)
{
    for (size_t i=0; i<vecSentences.size(); i++) {
        vecSentences[i]->FreeData();
        delete vecSentences[i];
        vecSentences[i] = NULL;
    }
    vecSentences.clear();
}

bool CTextBase::CanAddCloseQuote(const CWordBase* pWord) const
{
    if (pWord->IsPunct()) {
        if (pWord->IsCloseBracket()  ||
            pWord->IsPoint() ||
            pWord->IsExclamationMark() ||
            pWord->IsInterrogationMark())
            return true;
        else
            return false;
    }

    return true;
}

CSentenceBase* CTextBase::createSentence(size_t from,size_t size)
{
    THolder<CSentenceBase> pSent;

        pSent.Reset(CreateSentence());
        pSent->SetErrorStream(m_pErrorStream);

        int iPrevIsQuote = -1;
        bool bPrevSpace = false;
        CWordBase *pPrevWord = NULL;
        for (size_t i=0; i<size; i++) {
            CPrimGroup &group = m_vecPrimGroup[from+i];
            if (group.isNZ()) {
                bPrevSpace = true;
                continue;
            }
            if (group.m_gtyp == UnknownPrim) {
                bPrevSpace = false;
                continue;
            }

            //СЂРµС€РёР»Рё РІСЃСЏРєРёРµ СЃРёРјРІРѕР»С‹ РЅРµ С…РµСЂРёС‚СЊ
            if (group.m_gtyp == Symbol) {
                bPrevSpace = false;
            }

            Wtroka strTxt = GetText(group);
            if (CWordBase::IsQuote(strTxt)) {
                if (iPrevIsQuote != -1) {
                    CWordBase *pQuoteWord = createWord(m_vecPrimGroup[iPrevIsQuote], GetClearText(m_vecPrimGroup[iPrevIsQuote]));
                    addPunctHomonym(m_vecPrimGroup[iPrevIsQuote], pQuoteWord);
                    pSent->AddWord(pQuoteWord);
                    iPrevIsQuote = from+i;
                } else if (!bPrevSpace && pPrevWord && CanAddCloseQuote(pPrevWord)) {

                        pPrevWord->PutIsCloseQuote(CWordBase::IsSingleQuote(strTxt));
                        iPrevIsQuote = -1;
                    } else
                        iPrevIsQuote = from+i;
                bPrevSpace = false;
                continue;
            }
            CNumber number;

            //CWord *pWord = new CWord(group, GetText(group), m_pGrammInfo);
            CWordBase *pWord;
            if ((group.m_gtyp == Word) || (group.m_gtyp == Hyphen) || (group.m_gtyp == WordShy))
                pWord = createWord(group, GetText(group));
            else
                pWord = createWord(group, GetClearText(group));
            pWord->PutIsUp(hasFirstUpper(group));

            if (iPrevIsQuote != -1) {
                if (!bPrevSpace && !pWord->IsPunct())
                    pWord->PutIsOpenQuote(CWordBase::IsSingleQuote(GetText(m_vecPrimGroup[iPrevIsQuote])));
                else {
                    CWordBase *pQuoteWord = createWord(m_vecPrimGroup[iPrevIsQuote], GetClearText(m_vecPrimGroup[iPrevIsQuote]));
                    addPunctHomonym(m_vecPrimGroup[iPrevIsQuote], pQuoteWord);
                    pSent->AddWord(pQuoteWord);
                }

                iPrevIsQuote = -1;
            }

            bPrevSpace = false;

            if (group.isForMorph())
                applyMorphToGroup(pWord, group, number);
            else if (group.m_gtyp == Digit) {
                    addStupidHomonym(group, pWord);
                    processAsNumber(group, number);
            } else if (group.m_gtyp == Complex) {
                    processComplexGroup(pWord, group);
                    processAsNumber(group, number);
                    processAsNumber(pWord,group, number);
            } else if ((group.m_gtyp == DigHyp) || (group.m_gtyp == ComplexDigitHyp)) {
                    processComplexGroup(pWord, group);
                    processDigHypAsNumber(group, number);
            } else if (group.m_gtyp == ComplexDigit) {
                    processComplexGroup(pWord, group);
                    processAsNumber(group, number);
            } else if ((group.m_gtyp == Initial) ||
                      (group.m_gtyp == AltInitial) ||
                      (group.m_gtyp == Abbrev) ||
                      (group.m_gtyp == AbbEos) ||
                      (group.m_gtyp == AbbDig) ||
                      (group.m_gtyp == AbbAlt) ||
                      (group.m_gtyp == AbbEosAlt) ||
                      (group.m_gtyp == AbbDigAlt)
                    ) {
                    processInitialAndAbbrev(pWord, group);
                    processAsNumber(pWord, group, number);
            } else if (group.m_gtyp == Punct)
                    addPunctHomonym(group, pWord);
            else
                addStupidHomonym(group, pWord);
/*
            if( TMorph::GetParserOptions().m_bFDOCompanyNameFieldOpt )
                addWordPartsCount(group, pWord);
*/
            pSent->AddWord(pWord);
            pPrevWord = pWord;
            if (number.m_Numbers.size() > 0) {
                number.SetPair(pSent->getWordsCount() - 1, pSent->getWordsCount() - 1);
                number.m_iMainWord = number.FirstWord();
                pSent->AddNumber(number);
            }
        }

        if (iPrevIsQuote != -1) {
            CWordBase *pQuoteWord = createWord(m_vecPrimGroup[iPrevIsQuote], GetClearText(m_vecPrimGroup[iPrevIsQuote]));
            addPunctHomonym(m_vecPrimGroup[iPrevIsQuote], pQuoteWord);
            pSent->AddWord(pQuoteWord);
        }

        if (pSent->getWordsCount()==0) {
            pSent->FreeData();
            //delete pSent;
            if (m_vecPrimGroup[from+size-1].m_break==ParaBreak)
                markLastSentParEnd();
            return NULL;
        }

// Set pos & len
        size_t i1=0;
        for (; i1<size; i1++) {
            CPrimGroup &group = m_vecPrimGroup[from+i1];
            if (!group.isNZ())
                break;
        }
        pSent->m_pos = m_vecPrimGroup[from+i1].m_pos;
        size_t i2=size-1;
        for (; i2>i1; i2--) {
            CPrimGroup &group = m_vecPrimGroup[from+i2];
            if (!group.isNZ())
                break;
        }
        pSent->m_len = m_vecPrimGroup[from+i2].m_pos + m_vecPrimGroup[from+i2].m_len -
                            pSent->m_pos;

        return pSent.Release();
}

void CTextBase::processDigHypAsNumber(CPrimGroup &group, CNumber& number)
{
    if ((group.m_gtyp != DigHyp) && (group.m_gtyp != ComplexDigitHyp))
        return;
    bool bInteger = true;

    Wtroka strInteger;
    Wtroka strFractional;

    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if (group.m_prim[i].m_typ == Symbol) {
            continue;
        }

        if ((group.m_prim[i].m_typ == Punct) && (m_strText[group.m_prim[i].m_pos] == ',')) {
            bInteger = false;
            continue;
        }
        if ((group.m_prim[i].m_typ == Punct) && (m_strText[group.m_prim[i].m_pos] == '-')) {
            if (strInteger.empty())
                return;
            if (number.m_Numbers.size() >= 2) {
                number.m_Numbers.clear();
                return;
            }
            number.m_Numbers.push_back(CNumberProcessor::Atod(strInteger, strFractional));
            strFractional.clear();
            strInteger.clear();
            bInteger = true;
            continue;
        }

        if (group.m_prim[i].m_typ != Digit)
            continue;
        if (bInteger)
            strInteger += GetText(group.m_prim[i]);
        else
            strFractional += GetText(group.m_prim[i]);

    }

    if (strInteger.empty())
        return;

    if (number.m_Numbers.size() >= 2) {
        number.m_Numbers.clear();
        return;
    }

    number.m_Numbers.push_back(CNumberProcessor::Atod(strInteger, strFractional));
}

void CTextBase::processAsNumber(CPrimGroup &group, CNumber& number)
{
    if ((group.m_gtyp != Digit) && (group.m_gtyp != ComplexDigit))
        return;
    bool bInteger = true;

    Wtroka strInteger;
    Wtroka strFractional;

    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if ((group.m_prim[i].m_typ == Punct) &&
            ((m_strText[group.m_prim[i].m_pos] == ',') ||
                (m_strText[group.m_prim[i].m_pos] == '.'))) {
            bInteger = false;
            continue;
        }
        if (group.m_prim[i].m_typ != Digit)
            continue;
        if (bInteger)
            strInteger += GetText(group.m_prim[i]);
        else
            strFractional += GetText(group.m_prim[i]);
    }
    if (strInteger.empty())
        return;

    //if( !strFractional.empty() && bHasPoint && ((strFractional.size() % 3) == 0) )
    //{
    //  strInteger += strFractional;
    //  strFractional.clear();
    //}

    //С‡С‚РѕР±С‹ РІ double РІР»РµР·Р»Рѕ
    if (strInteger.size() +  strFractional.size() > 15)
        return;

    double res = CNumberProcessor::Atod(strInteger, strFractional);//atof(strInteger + "," + strFractional);
    number.m_Numbers.push_back(res);
}

void CTextBase::applyMorphToGroup(CWordBase* pWord, CPrimGroup& group, CNumber& number)
{
    switch (group.m_gtyp) {
    case Word:
        applyMorph(GetText(group), pWord, true);
        break;

    case AltWord:
        applyAltMorph(GetText(group), pWord, true);
        addStupidHomonym(group, pWord);
        break;

    case Hyphen:
        processHyphenGroup(pWord, group);
        break;

    case DivWord:
        processHypDivGroup(group, pWord);
        break;

    case HypDiv:
        processHypDivGroup(group, pWord);
        break;

    case WordShy:
        applyMorph(GetText(group), pWord, true);
        pWord->m_typ = Word;
        break;

    default:
        break;
    }

    //РЅР° РІСЃСЏРєРёР№ СЃР»СѓС‡Р°Р№
    if (pWord->GetHomonymsCount() == 0)
        addStupidHomonym(group, pWord);

    processAsNumber(pWord, group, number);
}

void CTextBase::AddVariants(THomonymBaseVector& homs, CWordBase *pWord)
{
    for (size_t i = 0; i < homs.size(); ++i)
        pWord->AddVariant(homs[i]);
}

bool CTextBase::applyMorphAndAddAsVariants(const Wtroka& strWord, CWordBase *pWord)
{
    THomonymBaseVector homonyms;
    bool bFound = TMorph::GetHomonyms(strWord, homonyms, false);

    if (bFound)
        AddVariants(homonyms, pWord);

    for (size_t i = 0; i < m_langAltInfo.size(); ++i)
        if (m_langAltInfo[i].bUseAlways || (m_langAltInfo[i].bUseAsPrime && !bFound))
            if (applyAltMorph(strWord, m_langAltInfo[i].lang, pWord))
                bFound = true;

    return bFound;
}

bool CTextBase::applyMorph(const Wtroka& strWord, CWordBase *pWord, bool bUsePrediction)
{
    THomonymBaseVector homonyms, predicted;
    bool bFound = TMorph::GetHomonyms(strWord, homonyms, bUsePrediction, predicted);

    bool bAllowIgnore = GetShouldCheckHomonym();

    for (size_t i = 0; i < homonyms.size(); ++i)
        pWord->AddHomonym(homonyms[i], bAllowIgnore);

    for (size_t i = 0; i < predicted.size(); ++i)
        pWord->AddPredictedHomonym(predicted[i], bAllowIgnore);

    for (size_t i = 0; i < m_langAltInfo.size(); ++i)
        if (m_langAltInfo[i].bUseAlways || (m_langAltInfo[i].bUseAsPrime && !bFound))
            if (applyAltMorph(strWord, m_langAltInfo[i].lang, pWord))
                bFound = true;

    return bFound;
}

bool CTextBase::applyAltMorph(const Wtroka& strWord, CWordBase *pWord, bool bUsePrediction)
{
    bool bFound = false;
    for (size_t i = 0; i < m_langAltInfo.size(); ++i)
        if (!m_langAltInfo[i].bUseAsPrime && (!bFound || m_langAltInfo[i].bUseAlways))
            bFound = applyAltMorph(strWord, m_langAltInfo[i].lang, pWord, bUsePrediction);

    return bFound;
}

bool CTextBase::applyAltMorph(const Wtroka& strWord, docLanguage /*lang*/, CWordBase *pWord, bool bUsePrediction)
{
    THomonymBaseVector homonyms;
    bool bFound = TMorph::GetHomonyms(strWord, homonyms, bUsePrediction);
    if (bFound)
        AddVariants(homonyms, pWord);

    return bFound;
}

void CTextBase::addStupidHomonym(Wtroka& strText, CWordBase *pWord)
{
    THomonymBasePtr pHom = TMorph::CreateHomonym();
    pHom->InitText(strText);
    pWord->AddHomonym(pHom, GetShouldCheckHomonym());
}

Wtroka CTextBase::GetClearText(const CPrimGroup& group) const
{
    if (group.m_gtyp == Punct && group.m_break == WordBreak)
        return Wtroka((wchar16)' ');
    Wtroka strText;
    for (size_t i = 0; i < group.m_prim.size(); ++i)
        if (group.m_prim[i].m_typ != UnknownPrim && group.m_prim[i].m_typ != NewLine && group.m_prim[i].m_typ != Space)
            strText += GetText(group.m_prim[i]);

    if (strText.size() <= MAX_WORD_LEN_N - 2)
        return strText;
    else
        return strText.substr(0, MAX_WORD_LEN_N - 2);
}

void CTextBase::addStupidHomonym(CPrimGroup &group, CWordBase *pWord)
{
    THomonymBasePtr pHom = TMorph::CreateHomonym();

    Wtroka strText;
    if (group.m_gtyp == Word || group.m_gtyp == AltWord)
        strText = GetText(group);
    else
        for (size_t i = 0; i < group.m_prim.size(); ++i) {
            if (group.m_prim[i].m_typ == UnknownPrim || group.m_prim[i].m_typ == NewLine || group.m_prim[i].m_typ == Space)
                continue;
            strText += GetText(group.m_prim[i]);
        }

    pHom->InitText(strText);
    if (group.m_gtyp == Abbrev)
        pHom->Grammems.ResetAbbrev();

    pWord->AddHomonym(pHom, GetShouldCheckHomonym());
}

docLanguage CTextBase::GetLangByPrimType(CPrimitive &element)
{
    if (element.m_typ == Word)
        return GetLang();
    if (element.m_typ == AltWord) {
        for (size_t i = 0; i < m_langAltInfo.size(); i++) {
            if (!m_langAltInfo[i].bUseAsPrime && !m_langAltInfo[i].bUseAlways)
                return m_langAltInfo[i].lang;
        }
    }
    return GetLang();
}

void CTextBase::addStupidVariant(CPrimitive &element, CWordBase *pWord, bool asPrefixes)
{
    THomonymBasePtr pHom = TMorph::CreateHomonym(/*GetLangByPrimType(element)*/);
    pHom->InitText(GetText(element));
    if (asPrefixes) {
        THomonymBaseVector homs;
        homs.push_back(pHom);
        AddVariants(homs, pWord);
    } else
        pWord->AddVariant(pHom);
}

void CTextBase::processHyphenGroup(CWordBase *pWord, CPrimGroup &group)
{
    SetHasAltWordPart(group, pWord);
    bool bHasUnusefulPostfix = isUnusefulPostfix(GetText(group.m_prim[group.m_prim.size()- 1]));
    processHyphenPrimtives(pWord, group.m_prim, bHasUnusefulPostfix);
}

void CTextBase::processHyphenPrimtives(CWordBase *pWord, yvector<CPrimitive> primitives, bool bHasUnusefulPostfix)
{
    if (tryToProcessHyphenPrimtivesAsOneWord(pWord, primitives))
        return;

    if (bHasUnusefulPostfix && primitives.size() >= 3) {
        primitives.erase(primitives.begin() + primitives.size() - 1);
        primitives.erase(primitives.begin() + primitives.size() - 1);
        processHyphenPrimtives(pWord, primitives, false);
        pWord->m_bHasUnusefulPostfix = true;
        return;
    }

    if ((m_ProcessingTarget != ForExactWordSearching)) {

        for (int i = (int)primitives.size() - 1; i >= 0; i--) {
            if ((primitives[i].m_typ != Word) && (primitives[i].m_typ != AltWord) && (primitives[i].m_typ != Digit))
                continue;

            bool bFound = false;

            Wtroka strText = GetText(primitives[i]);
            if (i > 0)
                if ((primitives[i-1].m_typ == Word) || (primitives[i-1].m_typ == AltWord))
                    strText = GetText(primitives[--i]) + strText;

            if (primitives[i].m_typ == Word)
                    bFound = applyMorphToHyphenPart(strText, pWord);

            if (primitives[i].m_typ == AltWord)
                bFound = applyAltMorph(strText, pWord);

            if (!bFound)
                addStupidVariant(primitives[i], pWord, true);
        }
    }

    if ((m_ProcessingTarget == ForDataBase) || (m_ProcessingTarget == ForExactWordSearching)) {
        addComplexLemma(primitives, pWord);
        if (m_ProcessingTarget == ForDataBase) {
            THomonymBasePtr pH = TMorph::CreateHomonym();
            pH->InitText(g_strDashWord, true);
            pWord->AddVariant(pH);
        }
    }

}

bool CTextBase::applyMorphToHyphenPart(Wtroka& strWord, CWordBase *pWord)
{
    THomonymBaseVector homonyms;
    bool bFound = TMorph::GetHomonyms(strWord, homonyms, false);
    if (bFound)
        AddVariants(homonyms, pWord);

    for (size_t i = 0; i < m_langAltInfo.size(); ++i)
        if (m_langAltInfo[i].bUseAlways || (m_langAltInfo[i].bUseAsPrime && !bFound))
            if (applyAltMorph(strWord, m_langAltInfo[i].lang, pWord))
                bFound = true;

    return bFound;
}

bool CTextBase::GetShouldCheckHomonym() const
{
    return (m_ProcessingTarget != ForSearchingWithBigParad) &&
           (m_ProcessingTarget != ForSearching);
}

bool CTextBase::tryToProcessHyphenPrimtivesAsOneWord(CWordBase *pWord, yvector<CPrimitive> primitives/*, bool up*/)
{
    Wtroka strText;
    bool bAlt = true, bPrim = true;

    for (size_t i = 0; i < primitives.size(); i++) {
        strText += GetText(primitives[i]);
        if (primitives[i].m_typ == Word)
            bAlt = false;
        else if (primitives[i].m_typ == AltWord)
            bPrim = false;
    }

    if (bPrim)
        if (applyMorph(strText, /*up,*/ pWord, false))
            return true;

    if (bAlt)
        if (applyAltMorph(strText, /*up,*/ pWord))
            return true;

    return false;
}

void CTextBase::addComplexLemma(yvector<CPrimitive>& primitives, CWordBase* pWord)
{
    THomonymBaseVector homonyms_of_last_part;

    size_t pcount = primitives.size();
    CPrimitive& plast = primitives[pcount-1];

    Wtroka lastWord = GetText(plast);
    if (pcount >= 2 && (primitives[pcount-2].m_typ == Word || primitives[pcount-2].m_typ == AltWord))
        lastWord = GetText(primitives[pcount-2]) + lastWord;

    //create homonym based on last primitive part
    if (plast.m_typ == Word)
        TMorph::GetHomonymsWithPrefixs(lastWord, homonyms_of_last_part);
    else if (plast.m_typ == AltWord) {
        bool bFound = false;
        for (size_t i = 0; i < m_langAltInfo.size(); ++i)
            if (!m_langAltInfo[i].bUseAsPrime && (!bFound || m_langAltInfo[i].bUseAlways))
                bFound = TMorph::GetHomonymsWithPrefixs(/*m_langAltInfo[i].lang,*/ lastWord, homonyms_of_last_part);
    }
    if (homonyms_of_last_part.size() == 0) {
        THomonymBasePtr pHom = TMorph::CreateHomonym();
        pHom->InitText(GetText(plast));
        homonyms_of_last_part.push_back(pHom);
    }

    //append all rest primitives to our new homonyms
    for (int i = primitives.size()-2; i >= 0; --i) {
        if (primitives[i].m_typ == UnknownPrim || primitives[i].m_typ == NewLine || primitives[i].m_typ == Space)
            continue;
        Wtroka primitive_text = GetText(primitives[i]);
        for (size_t j = 0; j < homonyms_of_last_part.size(); ++j) {
            //now check agreements - if current primitive homonym has same grammems as our last primitive current homonym
            THomonymBasePtr pH = homonyms_of_last_part[j];
            if (primitives[i].m_typ != Word)
                pH->SetLemma(primitive_text + pH->CHomonymBase::GetLemma());
            else
                pH->AddLemmaPrefix(primitive_text);
        }
    }

    for (size_t j = 0; j < homonyms_of_last_part.size(); ++j) {
        THomonymBasePtr hom = homonyms_of_last_part[j];
        TMorph::ToLower(hom->Lemma);
        if (!hom->IsDictionary() || hom->Lang != GetLang())
            pWord->AddVariant(hom);
        else
            pWord->AddHomonym(hom, GetShouldCheckHomonym());
    }
}

void CTextBase::getMiddleLemmas(yvector<yvector<Wtroka> >& lemmas_variants, int n, const Wtroka& str, yvector<Wtroka>& res)
{
    if (n >= (int)lemmas_variants.size())
        return;
    Wtroka tail(str);
    for (size_t i = 0; i < lemmas_variants[n].size(); i++) {
        if (n == (int)lemmas_variants[n].size() - 1)
            res.push_back(tail + lemmas_variants[n][i]);
        else
            getMiddleLemmas(lemmas_variants, n + 1, tail + lemmas_variants[n][i] + '-',  res);
    }
}

void CTextBase::SetHasAltWordPart(CPrimGroup& group, CWordBase* pWord)
{
    for (size_t i = 0; i < group.m_prim.size(); i++)
        if (group.m_prim[i].m_typ == AltWord) {
            pWord->SetHasAltWordPart(true);
            break;
        }
}

void CTextBase::processHypDivGroup(CPrimGroup& group, CWordBase* pWord)
{
    SetHasAltWordPart(group, pWord);
    int iBeforeDiv = -1, iAfterDiv = -1, iDiv = -1, iNewLine = -1;
    for (size_t i = 0; i < group.m_prim.size(); i++)
        if (group.m_prim[i].m_typ == NewLine)
            iNewLine = (int)i;

    if (iNewLine != -1) {
        int i = iNewLine - 1;
        for (; i >= 0; i--) {
            if (group.m_prim[i].m_typ == Punct) {
                iDiv = (int)i;
                break;
            }
        }

        for (i = iNewLine - 1; i >= 0; i--) {
            if ((group.m_prim[i].m_typ == Word) ||
                (group.m_prim[i].m_typ == Digit) ||
                (group.m_prim[i].m_typ == AltWord)) {
                iBeforeDiv = (int)i;
                break;
            }
        }
        for (i = iNewLine + 1; i < (int)group.m_prim.size(); i++) {
            if ((group.m_prim[i].m_typ == Word) ||
                (group.m_prim[i].m_typ == Digit) ||
                (group.m_prim[i].m_typ == AltWord)) {
                iAfterDiv = (int)i;
                break;
            }
        }

    }

    yvector<CPrimitive> primitives;
    if ((iAfterDiv == -1) || (iBeforeDiv == -1) || (iDiv == -1)) {
        for (size_t i = 0; i < group.m_prim.size(); i++) {
            if ((group.m_prim[i].m_typ == Word) ||
                (group.m_prim[i].m_typ == AltWord) ||
                (group.m_prim[i].m_typ == Digit) ||
                (group.m_prim[i].m_typ == Punct))
                primitives.push_back(group.m_prim[i]);

        }
        bool bHasUnusefulPostfix = isUnusefulPostfix(GetText(primitives[primitives.size()- 1]));
        processHyphenPrimtives(pWord, primitives, /*hasFirstUpper(group),*/ bHasUnusefulPostfix);
        return;
    }

    primitives.clear();
    bool bAsOneWord = true;
    //СЃРЅР°С‡Р°Р»Р° РїРѕРїСЂРѕР±СѓРµРј СѓРґР°Р»РёС‚СЊ РїРµСЂРµРЅРѕСЃ Рё СЃРѕР±СЂР°С‚СЊ РІСЃРµ РІРјРµСЃС‚Рµ
    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if ((int)i == iBeforeDiv) {
            primitives.push_back(group.m_prim[iBeforeDiv]);
            primitives.push_back(group.m_prim[iAfterDiv]);
            if (hasFirstLower(group.m_prim[iAfterDiv]) != hasLastLower(group.m_prim[iBeforeDiv]))
                bAsOneWord = false;
            i = iAfterDiv;
            continue;
        }
        primitives.push_back(group.m_prim[i]);
    }

    if (bAsOneWord && tryToProcessHyphenPrimtivesAsOneWord(pWord, primitives/*, hasFirstUpper(group)*/))
        return;

    primitives.clear();
    //РїРѕРїСЂРѕР±СѓРµРј СЃРґРµР»Р°С‚СЊ РїРµСЂРµРЅРѕСЃ РѕР±С„С‡РЅС‹Рј РґРµС„РёСЃРѕРј Рё РѕРїСЏС‚СЊ СЃРѕР±СЂР°С‚СЊ РІСЃРµ РІРјРµСЃС‚Рµ
    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if ((int)i == iBeforeDiv) {
            primitives.push_back(group.m_prim[iBeforeDiv]);
            primitives.push_back(group.m_prim[iDiv]);
            primitives.push_back(group.m_prim[iAfterDiv]);
            i = iAfterDiv;
            continue;
        }
        primitives.push_back(group.m_prim[i]);
    }

    if (tryToProcessHyphenPrimtivesAsOneWord(pWord, primitives/*, hasFirstUpper(group)*/))
        return;

    primitives.clear();
    yvector<CHomonymBase*> homonyms;
    bool bFound = false;
    if (bAsOneWord)
        bFound = TMorph::FindWord(GetText(group.m_prim[iBeforeDiv]) + GetText(group.m_prim[iAfterDiv]));

    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if ((int)i == iBeforeDiv) {
            primitives.push_back(group.m_prim[iBeforeDiv]);
            if (!bFound)
                primitives.push_back(group.m_prim[iDiv]);
            primitives.push_back(group.m_prim[iAfterDiv]);
            i = iAfterDiv;
            continue;
        }
        primitives.push_back(group.m_prim[i]);
    }

    bool bHasUnusefulPostfix = isUnusefulPostfix(GetText(primitives[primitives.size()- 1]));
    processHyphenPrimtives(pWord, primitives, /*hasFirstUpper(group),*/ bHasUnusefulPostfix);
}

void CTextBase::processComplexGroup(CWordBase *pWord, CPrimGroup &group)
{
    Wtroka strWholeStr;
    bool bHasPunct = false;
    for (int i = (int)group.m_prim.size() - 1; i >= 0; i--) {
        if ((group.m_prim[i].m_typ == UnknownPrim) ||
            (group.m_prim[i].m_typ == NewLine) ||
            (group.m_prim[i].m_typ == Space)) {
            bHasPunct = true;
            continue;
        }

        strWholeStr = GetText(group.m_prim[i]) + strWholeStr;

        if ((group.m_prim[i].m_typ == Punct) ||
            ((group.m_prim[i].m_typ == Symbol) &&
                (m_strText[group.m_prim[i].m_pos] != '$') &&
                (m_strText[group.m_prim[i].m_pos] != '%')
            )) {
            bHasPunct = true;
            continue;
        }

        bool bFounded = false;

        if (group.m_prim[i].m_typ == Word) {
            bFounded = applyMorphAndAddAsVariants(GetText(group.m_prim[i]), /*hasUpper(group.m_prim[i]),*/ pWord);
            if (!pWord->GetIsUp())
                pWord->PutIsUp(hasUpper(group.m_prim[i]));
        }

        if (group.m_prim[i].m_typ == AltWord) {
            bFounded = applyAltMorph(GetText(group.m_prim[i]), pWord/*, true*/);
            if (!pWord->GetIsUp())
                pWord->PutIsUp(hasUpper(group.m_prim[i]));
        }

        if (!bFounded)
            addStupidVariant(group.m_prim[i], pWord,true);
    }

    strWholeStr = strWholeStr.substr(0, MAX_WORD_LEN_N - 2);
    THomonymBasePtr pHom = TMorph::CreateHomonym();
    pHom->InitText(strWholeStr);

    if (pWord->GetHomonymsCount() == 0)
        pWord->AddHomonym(pHom, GetShouldCheckHomonym());
    else if (!bHasPunct)
        pWord->AddVariant(pHom);
}

void CTextBase::processInitialAndAbbrev(CWordBase *pWord, CPrimGroup &group)
{
    addStupidHomonym(group, pWord);
    for (size_t i = 0; i < group.m_prim.size(); i++) {
        bool bAdded = false;
        if (group.m_prim[i].m_typ == Word)
            bAdded = applyMorphAndAddAsVariants(GetText(group.m_prim[i]), pWord);

        if (group.m_prim[i].m_typ == AltWord)
            bAdded = applyAltMorph(GetText(group.m_prim[i]), pWord);

        if (!bAdded)
            if (group.m_prim[i].m_typ == Word || group.m_prim[i].m_typ == AltWord)
                addStupidVariant(group.m_prim[i], pWord);
    }
}
void CTextBase::addPunctHomonym(CPrimGroup &group, CWordBase *pWord)
{
    THomonymBasePtr pHom = TMorph::CreateHomonym();
    Wtroka str = GetText(group);

    //str.Trim(CTEXT_CTR_SYMBOL);
    TWtringBuf trimmed(str);
    while (!trimmed.empty() && trimmed[0] == CTEXT_CTR_SYMBOL)
        trimmed.Skip(1);
    while (!trimmed.empty() && trimmed.back() == CTEXT_CTR_SYMBOL)
        trimmed.Chop(1);

    pHom->InitText(ToWtring(trimmed));
    pWord->AddHomonym(pHom, GetShouldCheckHomonym());
}

int CTextBase::GetWordPartsCount(CPrimGroup& group)
{
    if ((group.m_gtyp == Word) ||
        (group.m_gtyp == AltWord))
        return 1;
    int pc = 0;
    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if ((group.m_prim[i].m_typ == Word) ||
            (group.m_prim[i].m_typ == AltWord) ||
            (group.m_prim[i].m_typ == Digit))
            pc++;
    }
    return pc;

}
void CTextBase::addWordPartsCount(CPrimGroup& group, CWordBase* pWord)
{
    int pc = GetWordPartsCount(group);
    if (pc <= 1)
        return;
    Wtroka w = CharToWide("$$$pc" + ToString(pc));
    THomonymBasePtr pHom = TMorph::CreateHomonym();
    pHom->InitText(w, true);
    pWord->AddVariant(pHom);
}

int CTextBase::GetWordPartsCount(const Wtroka& pText)
{
    ProceedPrimGroups(pText, 0, pText.size());
    int pc = 0;
    for (size_t i = 0; i < m_vecPrimGroup.size(); i++) {
        CPrimGroup &group = m_vecPrimGroup[i];
        pc += GetWordPartsCount(group);
    }
    return pc;
}

void CTextBase::ProcessAttributes()
{
    int iSentCountBefore = GetSentenceCount();
    Proceed(m_docAttrs.m_strTitle, 0, -1);

    m_docAttrs.m_strTitle.clear();

    m_docAttrs.m_TitleSents.first = iSentCountBefore;
    m_docAttrs.m_TitleSents.second = GetSentenceCount()-1;
    for (size_t i = iSentCountBefore; i < GetSentenceCount(); i++) {
        m_docAttrs.m_strTitle += GetSentence(i)->ToString();
        if (i < GetSentenceCount() - 1)
            m_docAttrs.m_strTitle += ' ';
    }
}

