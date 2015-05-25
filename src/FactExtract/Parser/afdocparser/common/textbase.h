#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "langdata.h"
#include "primitive.h"
#include "sentencebase.h"
#include <FactExtract/Parser/common/sdocattributes.h>
#include <FactExtract/Parser/afdocparser/rus/namecluster.h>
#include <FactExtract/Parser/afdocparser/rus/numberprocessor.h>

class TMorph;
class CParserOptions;

#define CTEXT_REM_SYMBOL 0x01 // is used as HTML eraser
#define CTEXT_CTR_SYMBOL 0x02 // is used as HTML control
#define MAX_WORD_IN_SENT 32000
#define MAX_SENT_IN_PARAGRAPH 64000
//////////////////////////////////////////////////////////////////////////////
// abstract base class

struct SPrim
{
    ETypeOfPrim m_Type;
    char m_Symbol;
};

struct SPrimScheme
{
    SPrim m_Prims[10];
    int   m_Len;
    ETypeOfPrim m_ResType;
};

enum EProcessingTarget { ForDataBase, ForSearching, ForSearchingWithBigParad, ForExactWordSearching };

extern const Wtroka g_strDashWord; // "$$$dashw"

class TTextCharTable
{
public:
    TTextCharTable();
    void Reset(docLanguage lang);

    char Check(wchar16 ch) const {
        size_t index = static_cast<ui16>(ch);
        YASSERT(index < Table.size());
        return Table[index];
    }

    bool IsUpper(wchar16 ch) const {
        return Check(ch) == 'T' || Check(ch) == 'A';
    }

    bool IsLower(wchar16 ch) const {
        return Check(ch) == 't' || Check(ch) == 'a';
    }
private:
    yvector<char> Table;
};

class CTextBase
{
public:
    CTextBase(void);
    virtual ~CTextBase(void);

    virtual void Proceed(const Wtroka& pText, int iFrom, int iSize);
    virtual void ProceedPrimGroups(const Wtroka& pText, int iFrom, int iSize);
    virtual void ResetPrimitives(void);
    virtual void FreeData(void);

    Wtroka GetRawText() const {
        return m_strText;
    }

    virtual Wtroka GetText(const CPrimitive &prim) const;
    virtual Wtroka GetText(CPrimGroup &group) const;
    void BuildPrimGroupText(const CPrimGroup &group, Wtroka& str) const;
    virtual Wtroka GetClearText(const CPrimGroup& group) const;
    void PutAttributes(const SDocumentAttribtes& docAttrs);
    const SDocumentAttribtes& GetAttributes() const;
    SDocumentAttribtes& GetAttributes();

    CWordBase* CreateWordForLemmatization();        //used in morphlib

    void ProcessAttributes();

    Wtroka      m_strDateFromDateField;
    Wtroka      m_strIssueString;
    CTime       m_DateFromDateFiled;

    bool CanAddCloseQuote(const CWordBase* pWord) const;
// Data
protected:
// Text reference
    Wtroka      m_strText;
    size_t      m_strFrom;
    size_t      m_strSize;
    int         m_iBadSymbolsCount;

    int         m_iAllLettersCount; //общее количество букв
    int         m_iUpperLettersCount;//количество букв в верхнем регистре
    Wtroka      BuildIssueString();

// Char Type Table
    TTextCharTable m_charTypeTable;

// Primitives and Groups
    yvector<CPrimitive> m_vecPrimitive;
    yvector<CPrimGroup> m_vecPrimGroup;

// Language Info/Data
    yvector<typeLangAltInfo> m_langAltInfo;

// Control parameters
    bool m_bSourceIsHtml;

    TOutputStream* m_pErrorStream;

    EProcessingTarget m_ProcessingTarget;
    //проверяет, нужно ли при добавлении омонима проверять
    //регистр для аббр, 'о' и т.д. (при поиске не нужно!)
    bool GetShouldCheckHomonym() const;

// Code
public: // pure virtual
    virtual void loadCharTypeTable() = 0;
    virtual void markLastSentParEnd() = 0;
    virtual void analyzeSentences();
    virtual void AddSentence(CSentenceBase* pSent) = 0;
    virtual CSentenceBase* CreateSentence() = 0;
    virtual docLanguage GetLang() const = 0;
    virtual CWordBase* createWord(CPrimGroup &group, const Wtroka& txt) = 0;
    virtual void SetParserOptions(const CParserOptions* ) {};

    virtual size_t GetSentenceCount() const = 0;
    virtual CSentenceBase* GetSentence(size_t i) = 0;
    virtual bool isDependentPrefix(const Wtroka&) { return false; }
    virtual bool isUnusefulPostfix(Wtroka) { return false; };
    virtual bool IgnoreUpperCase() const {return false; };

    virtual void processAsNumber(CWordBase*, CPrimGroup&, CNumber&) {};

public: // Language Info/Data
    void appendLangAltInfo(typeLangAltInfo &info) { m_langAltInfo.push_back(info); }

    bool isDirectPartOfSpeechQuote(size_t& k);
    void applyMorphToGroup(CWordBase *pWord, CPrimGroup &group, CNumber& number);
    CSentenceBase* createSentence(size_t from,size_t size);
    bool applyMorph(const Wtroka& strWord,  CWordBase *pWord, bool bUsePrediction);
    bool applyMorphAndAddAsVariants(const Wtroka& strWord, CWordBase *pWord);
    bool applyAltMorph(const Wtroka& strWord, docLanguage lang, CWordBase *pWordRus, bool bUsePrediction = false);
    bool applyAltMorph(const Wtroka& strWord, CWordBase *pWordRus, bool bUsePrediction = false);
    void addStupidHomonym(CPrimGroup &group, CWordBase *pWord);
    void addStupidHomonym(Wtroka& strText, CWordBase *pWord);
    void addStupidVariant(CPrimitive &element, CWordBase *pWordRus, bool asPrefixes = false);
    void processHyphenGroup(CWordBase *pWord, CPrimGroup &group);
    void processHyphenPrimtives(CWordBase *pWord, yvector<CPrimitive> primitives, bool bHasUnusefulPostfix);
    bool applyMorphToHyphenPart(Wtroka& strWord, CWordBase *pWord);
    bool tryToProcessHyphenPrimtivesAsOneWord(CWordBase *pWord, yvector<CPrimitive> primitives);
    void processHypDivGroup(CPrimGroup& group, CWordBase* pWord);
    void processComplexGroup(CWordBase *pWord, CPrimGroup &group);
    void processInitialAndAbbrev(CWordBase *pWord, CPrimGroup &group);
    void addComplexLemma(yvector<CPrimitive>& primitives, CWordBase* pWord);
    void addPunctHomonym(CPrimGroup &group, CWordBase *pWord);
    void getMiddleLemmas(yvector<yvector<Wtroka> >& lemmas_variants, int n, const Wtroka& str, yvector<Wtroka>& res);

    void processAsNumber(CPrimGroup &group, CNumber& number);
    void processDigHypAsNumber(CPrimGroup &group, CNumber& number);
    void AddVariants(THomonymBaseVector& homs, CWordBase *pWord);
    void SetHasAltWordPart(CPrimGroup& group, CWordBase* pWord);
    void addWordPartsCount(CPrimGroup& group, CWordBase* pWord);
    int GetWordPartsCount(CPrimGroup& group);
    int GetWordPartsCount(const Wtroka& pText);
    docLanguage GetLangByPrimType(CPrimitive &element);

    virtual bool BuildNames(const yvector<Wtroka>& /*words*/, bool& /*bIndexed*/, yset<Wtroka>& /*names*/, bool /*bFullName*/, bool /*bFullPatronymic*/, bool /*bAnyPatronymic*/) { return false; };

public: // Html
    void setHtmlSource(bool val) { m_bSourceIsHtml = val; }

public:
    void SetErrorStream(TOutputStream* pErrStream) { m_pErrorStream = pErrStream;   }
    void PrintError(const char* msg) { if (m_pErrorStream) (*m_pErrorStream) << msg << Endl; }
    int  GetBadSymbolsCount() { return m_iBadSymbolsCount; }

private:
    bool IsPunctChar(const CPrimGroup& gr, wchar16 ch) const {
        return gr.m_gtyp == Punct && gr.m_len == 1 && m_strText[gr.m_prim[0].m_pos] == ch;
    }
    bool IsPunctChar(const CPrimitive& pr, wchar16 ch) const {
        return pr.m_typ == Punct && m_strText[pr.m_pos] == ch;
    }
    bool IsPunctCharByText(CPrimGroup& gr, wchar16 ch) const {
        if (gr.m_gtyp != Punct)
            return false;
        Wtroka text = GetText(gr);
        return text.size() == 1 && text[0] == ch;
    }
    bool HasUpper(const Wtroka& text) const {
        for (size_t i = 0; i < text.size(); ++i)
            if (m_charTypeTable.IsUpper(text[i]))
                return true;
        return false;
    }
    bool HasLower(const Wtroka& text) const {
        for (size_t i = 0; i < text.size(); ++i)
            if (m_charTypeTable.IsLower(text[i]))
                return true;
        return false;
    }

protected:
    bool isPunctDot(const CPrimGroup& gr) const { return IsPunctChar(gr, '.'); }
    bool isPunctDot(const CPrimitive& pr) const { return IsPunctChar(pr, '.'); }

    bool isPunctComma(const CPrimGroup& gr) const { return IsPunctChar(gr, ','); }
    bool isPunctComma(const CPrimitive& pr) const { return IsPunctChar(pr, ','); }

    bool isPunctColon(const CPrimGroup& gr) const { return IsPunctChar(gr, ':'); }
    bool isPunctColon(const CPrimitive& pr) const { return IsPunctChar(pr, ':'); }

    bool isPunctSemicolon(const CPrimGroup& gr) const { return IsPunctChar(gr, ';'); }
    bool isPunctSemicolon(const CPrimitive& pr) const { return IsPunctChar(pr, ';'); }

    bool isPunctHyphen(const CPrimGroup& gr) const { return IsPunctChar(gr, '-'); }
    bool isPunctHyphen(const CPrimitive& pr) const { return IsPunctChar(pr, '-'); }

    bool isPunctLeftBracket(const CPrimGroup& gr) const { return IsPunctChar(gr, '('); }
    bool isPunctLeftBracket(const CPrimitive& pr) const { return IsPunctChar(pr, '('); }

    bool isPunctRightBracket(const CPrimGroup& gr) const { return IsPunctChar(gr, ')'); }
    bool isPunctRightBracket(const CPrimitive& pr) const { return IsPunctChar(pr, ')'); }

    bool isPunctApostrophe(CPrimGroup& gr) const { return IsPunctCharByText(gr, '\''); }
    bool isPunctApostrophe(const CPrimitive& pr) const { return IsPunctChar      (pr, '\''); }

    bool isPunctQuote(CPrimGroup& gr) const { return IsPunctCharByText(gr, '"'); }
    bool isPunctQuote(const CPrimitive& pr) const { return IsPunctChar      (pr, '"'); }

    bool isPunctSentEnd(const CPrimGroup& gr) const {
        if (gr.m_gtyp != Punct || gr.m_len != 1)
            return false;
        wchar16 ch = m_strText[gr.m_prim[0].m_pos];
        return ch == '.' || ch == '?' || ch == '!';
    }
    bool isPunctSentEnd(const CPrimitive& pr) const {
        if (pr.m_typ != Punct)
            return false;
        wchar16 ch = m_strText[pr.m_pos];
        return ch == '.' || ch == '?' || ch == '!';
    }

    bool isSymbolAsterisk(const CPrimGroup& gr) const {
        return gr.m_gtyp == Symbol && gr.m_len == 1 && m_strText[gr.m_prim[0].m_pos] == '*';
    }

    bool isGroupBorder(size_t no);
    bool CompareToScheme(SPrimScheme& primScheme, size_t from);

    bool hasUpper(const CPrimitive& pr) const { return HasUpper(GetText(pr)); }
    bool hasUpper(CPrimGroup& gr) const { return HasUpper(GetText(gr)); }

    bool hasLower(const CPrimitive& pr) const { return HasLower(GetText(pr)); }
    bool hasLower(CPrimGroup& gr) const { return HasLower(GetText(gr)); }

    bool hasFirstUpper(const CPrimitive &pr) const { Wtroka tmp = GetText(pr); return tmp.size() > 0 && m_charTypeTable.IsUpper(tmp[0]); }
    bool hasFirstUpper(CPrimGroup &gr) const { Wtroka tmp = GetText(gr); return tmp.size() > 0 && m_charTypeTable.IsUpper(tmp[0]); }

    bool hasFirstLower(const CPrimitive &pr) const { Wtroka tmp = GetText(pr); return tmp.size() > 0 && m_charTypeTable.IsLower(tmp[0]); }
    bool hasFirstLower(CPrimGroup &gr) const { Wtroka tmp = GetText(gr); return tmp.size() > 0 && m_charTypeTable.IsLower(tmp[0]); }

    bool hasLastLower(const CPrimitive &pr) const { Wtroka tmp = GetText(pr); return tmp.size() > 0 && m_charTypeTable.IsLower(tmp.back()); }

    void createPrimitives();
    void createPrimGroups();

    size_t queryHtmlCtrSeq(CPrimGroup &group,size_t from);
    size_t queryHtmlEscSeq(CPrimGroup &group,size_t from);
    size_t queryInitialGroup(CPrimGroup &group,size_t from);
    size_t queryAbbrevGroup(CPrimGroup &group,size_t from);
    size_t queryComplexGroup(CPrimGroup &group,size_t from);
    size_t queryApostropheGroup(CPrimGroup &group,size_t from);
    size_t queryHyphenGroup(CPrimGroup &group,size_t from);
    size_t queryDivWordGroup(CPrimGroup &group,size_t from);
    size_t queryBucksGoup(CPrimGroup &group,size_t from);
    size_t queryPercentGoup(CPrimGroup &group,size_t from);
    size_t query3DigitChain(size_t from);
    size_t queryLongComplexDigit(CPrimGroup &group,size_t from);
    size_t queryComplexDigit(CPrimGroup &group,size_t from);
    size_t query2DigHyphenGroup(CPrimGroup &group,size_t from);
    size_t queryWordShyGroup(CPrimGroup &group,size_t from);
    size_t querySlashDigitGroup(CPrimGroup &group,size_t from);
    size_t QueryTurkishOrdinalNumberGroup(CPrimGroup &group, size_t from); // "76. Sokak" in Turkish
    virtual void ReplaceSimilarLetters(CPrimGroup &/*group*/) {};
    bool CanIncludePointBetweenWords(size_t from);
    bool is3DigitChainDelimeter(size_t from);

    TWtringBuf GetPrimitiveText(const CPrimitive& p) const {
        return TWtringBuf(m_strText).SubString(p.m_pos, p.m_len);
    }

    TWtringBuf GetPrimitiveText(size_t index) const {
        return GetPrimitiveText(m_vecPrimitive[index]);
    }

    void appendPrimitivesToPrimGroup(CPrimGroup &group,size_t noBegPrim,size_t noEndPrim);

    void proceedNewLineBreaks();
    void fetchLineInfo(size_t begLine,size_t endLine,
        size_t &pbeg,size_t &pend,CPrimGroup **g1,CPrimGroup **g2,size_t &sleft);

    void proceedSentEndBreaks();

    void createSentences();

    SDocumentAttribtes  m_docAttrs;

public:
    static void FreeExternOutput(yvector<CSentenceBase*>& vecSentences);
};
