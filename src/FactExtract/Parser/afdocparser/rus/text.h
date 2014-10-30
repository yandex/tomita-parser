#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/common/textbase.h>
#include "sentence.h"
#include <FactExtract/Parser/afdocparser/common/graminfo.h>

class CReferenceSearcher;
class CParserOptions;

struct SSurnameBegining
{
    Wtroka m_str2LetterBegining;
    mutable yvector<Wtroka> m_MaxLenFlexBegining;

    SSurnameBegining() {}
    SSurnameBegining(const Wtroka& s)
        : m_str2LetterBegining(s)
    {
    }

    bool operator < (const SSurnameBegining& SurnameBegining) const
    {
        return m_str2LetterBegining < SurnameBegining.m_str2LetterBegining;
    }
};

class CText : public CTextBase
{
public:
    CText(const CParserOptions* )
        : m_pReferenceSearcher(NULL)
    {
    }

    virtual ~CText() {
        FreeData();
    }

    virtual void FreeData();

public: // static language data
    void Process(const Wtroka& text, const yset<SLinkRefMarkUp>& markUps);

public: // overwrited base pure virtual
    void loadCharTypeTable();
    void markLastSentParEnd();
    virtual void analyzeSentences();
    void findNames();

    bool IsSurname(const Wtroka& pText);
    bool IsPOSAndGrammems(const Wtroka& w, TGrammar POS, const TGramBitSet& gramems);
    void AddSentence(CSentenceBase* pSent);
    //virtual void setLangData(CLangData* pData, bool bAutoDelete = false);
    virtual CSentenceBase* CreateSentence();
    virtual CWordBase* createWord(CPrimGroup &group, const Wtroka& txt);
    virtual docLanguage GetLang() const { return TMorph::GetMainLanguage(); }

    void processAsNumber(CWordBase* pWord, CPrimGroup& group, CNumber& number);

    bool processHyphenOrComplexGroupAsNumberAndFlex(CWord* pWord, CPrimGroup& group, CNumber& number);
    void processHyphenOrComplexGroupAsNumber(CWord* pWord, CPrimGroup& group, CNumber& number);
    bool ProcessTurkishNumberWithPeriod(CWord* pWord, CPrimGroup& group, CNumber& number);

    void ReplaceSimilarLetters(CPrimGroup &group);
    bool ReplaceSimilarLetters(CPrimGroup &group, ETypeOfPrim from, ETypeOfPrim to, docLanguage lang);

    bool FindNamesInFioField(CNameClusterBuilder& NameClusterBuilder, int iSent);
    virtual void RunFragmentation() { }
    bool ShouldRunFragmentation(CSentence& sent);

    virtual size_t GetSentenceCount() const;
    virtual CSentenceBase* GetSentence(size_t i);

    virtual Wtroka GetText(const CPrimitive& prim) const;
    virtual Wtroka GetText(CPrimGroup& group) const;
    virtual Wtroka GetClearText(const CPrimGroup& group) const;

    virtual void SetCurSent(int iSent);

    CSentence* GetSentenceRus(size_t i);

protected:
    static const char s_strRusAltLetters[100];

    void JoinSentencesAndAddArtificialPunct(int sent1);
    void CreateReferenceSearcher();
    bool isUnusefulPostfix(Wtroka text);
    bool HasSuspiciousPatronymic(CFioWordSequence& fio, CSentence* pSent);

    void PredictSurname(CFIOOccurence& fioWP, SFullFIO& fio, CSentence* pSent);
    bool PredictSurname(CFIOOccurence& fioWP, CSentence* pSent);
    void AddToSurnameBeginings(yset<SSurnameBegining>& surnameBeginings, const Wtroka& s);

    virtual bool HasToResolveContradictionsFios() { return false; }

    CSentence* BuildOneSentence(const Wtroka& pText);

    virtual size_t GetMaxNamesCount() { return 1000; }

public: // temporary - for debug
    yvector<CSentence*> m_vecSentence;
    CReferenceSearcher* m_pReferenceSearcher;
};

