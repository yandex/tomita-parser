
#include "graminfo.h"
#include <FactExtract/Parser/common/string_tokenizer.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/common/utilit.h>
#include <util/stream/file.h>
#include <util/string/strip.h>


const Wtroka g_strNameStubShort                 = UTF8ToWide("#n#");
const Wtroka g_strPatronomycStubShort           = UTF8ToWide("#p#");
const Wtroka g_strInitialPatronomycStubShort    = UTF8ToWide("#ip#");
const Wtroka g_strInitialNameStubShort          = UTF8ToWide("#in#");

const Wtroka g_strFioFormPrefix                 = UTF8ToWide("$$$ff");
const Wtroka g_strFioLemmaPrefix                = UTF8ToWide("$$$fl");

const Wtroka g_strFIONonTerminal                = UTF8ToWide("@фио");
const Wtroka g_strDateNonTerminal               = UTF8ToWide("@дата");

struct SNumeral2Number
{
    Wtroka m_Cardinal; // количественное числительное  "два"
    Wtroka m_Ordinal;  // порядковое числительное      "второй"
    Wtroka m_Adverb;   // наречие                      "вдвоём"
    Wtroka m_Noun;     // существительное              "двое"
    ui64 m_Integer;    // числовое представление       2
    int m_Fractional;

    SNumeral2Number(const char* cardinal, const char* ordinal, const char* adverb, const char* noun, ui64 integer, int fract)
        : m_Cardinal(UTF8ToWide(cardinal))
        , m_Ordinal(UTF8ToWide(ordinal))
        , m_Adverb(UTF8ToWide(adverb))
        , m_Noun(UTF8ToWide(noun))
        , m_Integer(integer)
        , m_Fractional(fract)
    {
    }
};

class TNumeral2Numbers
{
    DECLARE_SINGLETON_FRIEND(TNumeral2Numbers)
private:
    yvector<SNumeral2Number> List;

    typedef ymap<Wtroka, size_t, ::TLess<TWtringBuf> > TIndex;
    TIndex CardinalIndex;
    TIndex OrdinalIndex;

    void Add(const char* cardinal, const char* ordinal, const char* adverb, const char* noun, ui64 integer, int fract = 0) {
        SNumeral2Number item(cardinal, ordinal, adverb, noun, integer, fract);
        CardinalIndex[item.m_Cardinal] = List.size();
        OrdinalIndex[item.m_Ordinal] = List.size();
        List.push_back(item);
    }

    TNumeral2Numbers()
    {
        Add("один", "первый", "", "", 1);
        Add("полтора", "", "", "", 1, 5);
        Add("два", "второй", "вдвоем", "двое", 2);
        Add("три", "третий", "втроем", "трое", 3);
        Add("четыре", "четвертый", "вчетвером", "четверо", 4);
        Add("пять", "пятый", "впятером", "пятеро", 5);
        Add("шесть", "шестой", "вшестером", "шестеро", 6);
        Add("семь", "седьмой", "всемером", "семеро", 7);
        Add("восемь", "восьмой", "ввосьмером", "восьмеро", 8);
        Add("девять", "девятый", "вдевятером", "девятеро", 9);
        Add("десять", "десятый", "вдесятером", "десятеро", 10);
        Add("одиннадцать", "одиннадцатый", "водиннадцатером", "", 11);
        Add("двенадцать", "двенадцатый", "вдвенадцатером", "", 12);
        Add("тринадцать", "тринадцатый", "втринадцатером", "", 13);
        Add("четырнадцать", "четырнадцатый", "вчетырнадцатером", "", 14);
        Add("пятнадцать", "пятнадцатый", "впятнадцатером", "", 15);
        Add("шестнадцать", "шестнадцатый", "вшестнадцатером", "", 16);
        Add("семнадцать", "семнадцатый", "всемнадцатером", "", 17);
        Add("восемнадцать", "восемнадцатый", "ввосемнадцатером", "", 18);
        Add("девятнадцать", "девятнадцатый", "вдевятнадцатером", "", 19);
        Add("двадцать", "двадцатый", "вдвадцатером", "", 20);
        Add("тридцать", "тридцатый", "втридцатером", "", 30);
        Add("сорок", "сороковой", "", "", 40);
        Add("пятьдесят", "пятидесятый", "впятидесятером", "", 50);
        Add("шестьдесят", "шестидесятый", "вшестидесятером", "", 60);
        Add("семьдесят", "семидесятый", "всемидесятером", "", 70);
        Add("восемьдесят", "восьмидесятый", "ввосьмидесятером", "", 80);
        Add("девяносто", "девяностый", "", "", 90);
        Add("сто", "сотый", "", "", 100);
        Add("сотня", "", "", "", 100);
        Add("двести", "двухсотый", "", "", 200);
        Add("триста", "трехсотый", "", "", 300);
        Add("четыреста", "четырехсотый", "", "", 400);
        Add("пятьсот", "пятисотый", "", "", 500);
        Add("шестьсот", "шестисотый", "", "", 600);
        Add("семьсот", "семисотый", "", "", 700);
        Add("восемьсот", "восьмисотый", "", "", 800);
        Add("девятьсот", "девятисотый", "", "", 900);
        Add("тысяча", "тысячный", "", "", 1000);
        Add("миллион", "миллионный", "", "", 1000000ULL);
        Add("млн", "", "", "", 1000000ULL);
        Add("млн.", "", "", "", 1000000ULL);
        Add("тыс", "", "", "", 1000);
        Add("тыс.", "", "", "", 1000);
        Add("млрд", "", "", "", 1000000000ULL);
        Add("млрд.", "", "", "", 1000000000ULL);
        Add("миллиард", "миллиардный", "", "", 1000000000ULL);
        Add("триллион", "триллионный", "", "", 1000000000000ULL);
        Add("трлн.", "", "", "", 1000000000000ULL);
        Add("трлн", "", "", "", 1000000000000ULL);
    }

    static const TNumeral2Numbers* Static() {
        return Singleton<TNumeral2Numbers>();
    }

public:
    static const SNumeral2Number* FindByCardinal(const TWtringBuf& cardinal) {
        TIndex::const_iterator it = Static()->CardinalIndex.find(cardinal);
        return (it != Static()->CardinalIndex.end()) ? &(Static()->List[it->second]) : NULL;
    }

    static const SNumeral2Number* FindByOrdinal(const TWtringBuf& ordinal) {
        TIndex::const_iterator it = Static()->OrdinalIndex.find(ordinal);
        return (it != Static()->OrdinalIndex.end()) ? &(Static()->List[it->second]) : NULL;
    }
};

/*----------------------------------class CGramInfo------------------------------*/

bool CGramInfo::s_bRunFragmentationWithoutXML=false;
bool CGramInfo::s_bLoadLemmaFreq = false;
bool CGramInfo::s_bRunCompanyAndPostAnalyzer = false;
bool CGramInfo::s_bRunResignationAppointment = false;
bool CGramInfo::s_bRunSituationSearcher = false;
bool CGramInfo::s_bForceRecompile = false;
size_t CGramInfo::s_maxFactsCount = 64;

EBastardMode CGramInfo::s_BastardMode = EBastardMode::No;

bool CGramInfo::s_bDebugProtobuf = false;
TOutputStream* CGramInfo::s_PrintRulesStream = NULL;
TOutputStream* CGramInfo::s_PrintGLRLogStream = NULL;
THolder<TOutputStream> CGramInfo::s_PrintRulesStreamHolder;
THolder<TOutputStream> CGramInfo::s_PrintGLRLogStreamHolder;
bool CGramInfo::s_bNeedAuxKwDict = true;

ECharset CGramInfo::s_DebugEncoding = CODES_UTF8;

void CGramInfo::ParseNamesString(const Wtroka& str, Wtroka& field, yvector<Wtroka>& names) // static
{
    TWtrokaTokenizer tokenizer(str, "\t\n =,");
    Wtroka item;
    if (!tokenizer.NextAsString(field))
        return;

    field = StripString(field);
    while (tokenizer.NextAsString(item)) {
        item = StripString(item);
        item.to_lower();
        names.push_back(item);
    }
}

Wtroka CGramInfo::Modality2Str(EModality modal) const
{
    return ::Modality2Str(modal);
}

Wtroka CGramInfo::ClauseType2Str(EClauseType type) const
{
    try {
        return ::ClauseType2Str(type);
    } catch (...) {
        return Wtroka();
    }
}

Wtroka CGramInfo::GetFullFirstName(Wtroka strName) const
{
    strName.to_lower();
    if (m_FirstNames.Get() != NULL) {
        TNameIndex::const_iterator it = m_FirstNames->find(strName);
        if (it != m_FirstNames->end()) {
            if (it->second.m_bMainVar)
                return strName;
            else if (it->second.m_NameVariants.size() == 1)
                return it->second.m_NameVariants[0];
        }
    }
    return strName;
}

const yvector<Wtroka>* CGramInfo::GetFirstNameVariants(Wtroka strName) const
{
    strName.to_lower();
    if (m_FirstNames.Get() != NULL) {
        TNameIndex::const_iterator it = m_FirstNames->find(strName);
        if (it != m_FirstNames->end())
            return &(it->second.m_NameVariants);
    }
    return NULL;
}

bool CGramInfo::FindCardinalNumber(const Wtroka& strNumber, ui64& integer, int& fractional) const
{
    const SNumeral2Number* num = TNumeral2Numbers::FindByCardinal(strNumber);
    if (num != NULL) {
        integer = num->m_Integer;
        fractional = num->m_Fractional;
        return true;
    } else
        return false;

}

bool CGramInfo::FindOrdinalNumber(const Wtroka& strNumber, ui64& integer, int& fractional) const
{
    const SNumeral2Number* num = TNumeral2Numbers::FindByOrdinal(strNumber);
    if (num != NULL) {
        integer = num->m_Integer;
        fractional = num->m_Fractional;
        return true;
    } else
        return false;
}

void CGramInfo::InsertNames(const Wtroka& field, yvector<Wtroka>& names, Wtroka& curArticle)
{
    // the guard is not required - this method is protected

    static const Wtroka ZGL = UTF8ToWide("ЗГЛ");
    static const Wtroka UM = UTF8ToWide("УМ");
    static const Wtroka OT_M = UTF8ToWide("ОТ_М");
    static const Wtroka OT_F = UTF8ToWide("ОТ_Ж");

    if (field == ZGL)
        curArticle.clear();

    if (names.size() <= 0)
        return;

    if (field == ZGL) {
        curArticle = names[0];
        (*m_FirstNames)[curArticle] = SName();
        return;
    }

    if (field == UM)
        for (size_t i = 0; i < names.size(); ++i) {
            SName Name(false);
            Wtroka aname = names[i];
            if (!curArticle.empty()) {
                TNameIndex::iterator it = m_FirstNames->find(curArticle);
                if (it != m_FirstNames->end())
                    it->second.m_NameVariants.push_back(aname);
                Name.m_NameVariants.push_back(curArticle);
            }
            (*m_FirstNames)[aname] = Name;
        }
}

void CGramInfo::InitNames(const Stroka& strNameFile, ECharset encoding)
{
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    if (m_FirstNames.Get() == NULL) {

        m_FirstNames.Reset(new TNameIndex);

        if (!PathHelper::Exists(strNameFile))
            ythrow yexception() << "Cannot open names dictionary: " << strNameFile << " not found.";

        TIFStream file(strNameFile);

        Wtroka strPrevName;
        Stroka str;
        for (size_t linenum = 1; file.ReadLine(str); ++linenum) {
            str = StripString(str);
            if (str.empty())
                continue;

            Wtroka wstr = NStr::DecodeUserInput(str, encoding, strNameFile, linenum);

            Wtroka field;
            yvector<Wtroka> names;
            ParseNamesString(wstr, field, names);
            InsertNames(field, names, strPrevName);
            str.clear();
        }
    }
}

bool CGramInfo::IsNounGroup(int /*iTerminal*/)
{
    return true;
}

bool CGramInfo::IsNounGroupForSubject(int /*iTerminal*/)
{
    return true;
}

bool CGramInfo::IsVerbGroup(int /*iTerminal*/)
{
    return false;
}
