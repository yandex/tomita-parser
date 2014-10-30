#pragma once

#include <util/generic/vector.h>

#include "wordspair.h"
#include "graminfo.h"
#include "gztarticle.h"

#include "homonymbase.h"


struct SFIOLight
{
    SFIOLight()
        : m_bFoundSurname(false)
    {
        m_bFoundSurname = false;
    }

    EFIOType GetType() const;
    Wtroka m_strSurname;
    Wtroka m_strName;
    Wtroka m_strPatronomyc;
    bool  m_bFoundSurname;
};

struct SWordSequenceLemma
{
    Wtroka m_LemmaToShow;
    Wtroka m_CapitalizedLemma;

    SWordSequenceLemma(Wtroka Lemma, Wtroka capitalizedLemma)
        : m_LemmaToShow(Lemma)
        , m_CapitalizedLemma(capitalizedLemma)
    {
        TMorph::ToLower(m_LemmaToShow);
    }

    SWordSequenceLemma(Wtroka Lemma)
        : m_LemmaToShow(Lemma)
        , m_CapitalizedLemma(Lemma)
    {
        TMorph::ToLower(m_LemmaToShow);
    }

    bool operator== (const SWordSequenceLemma& _X) const {
        return m_LemmaToShow == _X.m_LemmaToShow;
    }

    bool HasLemma() const;
    Wtroka GetLemma(bool bCapitalized) const;
};

class CSentenceRusProcessor;
class CHomonym;

class CWordSequence: public CWordsPair, public TRefCounted<CWordSequence>
{
public:
    CWordSequence();

    virtual ~CWordSequence() {
    }

    EWordSequenceType GetWSType() const {
        return m_WSType;
    }

    virtual void reset();

    TKeyWordType GetKWType() const;
    Wtroka GetArticleTitle() const;

    const Stroka& GetNonTerminalSem() const;
    void PutWSType(EWordSequenceType type);
    void PutNonTerminalSem(const Stroka& sNonTerminalSem);
    const SWordHomonymNum& GetMainWord() const;
    void SetMainWord(int iW, int iH);
    void SetMainWord(const SWordHomonymNum& wh);

    void PutArticleOfHomonym(const CHomonym& homonym, EDicType dicType);

    void PutArticle(const TArticleRef& article);

    void PutAuxArticle(const SDictIndex& dicIndex);
    const SDictIndex& GetAuxArticleIndex() const;
    inline bool HasAuxArticle() const
    {
        return GetAuxArticleIndex().IsValid();
    }

    void PutGztArticle(const TGztArticle& gztArticle);
    const TGztArticle& GetGztArticle() const;
    inline bool HasGztArticle() const
    {
        return !GetGztArticle().Empty();
    }

    void SetGrammems(const THomonymGrammems& grammems) {
        Grammems = grammems;
    }

    const THomonymGrammems& GetGrammems() const {
        return Grammems;
    }

    THomonymGrammems& GetGrammems() {
        return Grammems;
    }


    const yvector<SWordSequenceLemma>& GetLemmas() const {
        return Lemmas;
    }

    bool HasLemmas() const {
        return !Lemmas.empty();
    }

    void AddLemma(const SWordSequenceLemma& lemma, bool normalized = true);
    void ClearLemmas();
    void AddLemmas(const yvector<SWordSequenceLemma>& lemmas, bool normalized);
    void ResetLemmas(const yvector<SWordSequenceLemma>& lemmas, bool normalized);
    void ResetLemmas(const yvector<Wtroka>& lemmas, bool normalized);
    void SetCommonLemmaString(const Wtroka& commonLemma);

    bool IsArtificialPair() const {
        return IsArtificialPair_;
    }

    bool IsConcatenationLemma() const {
        return IsConcatenationLemma_;
    }

    bool IsNormalizedLemma() const {
        return IsNormalizedLemma_;
    }

    void SetArtificialPair(const CWordsPair& wp);
    void SetArtificialPair(bool bArtificial = true);
    void SetConcatenationLemma(bool bConcat = true);

    virtual Wtroka GetCapitalizedLemma() const {
        return CapitalizedLemmaText;
    }

    virtual Wtroka GetLemma() const {
        return LemmaText;
    }

    Wtroka GetLemma(bool bCapitalized) const {
        return bCapitalized ? GetCapitalizedLemma() : GetLemma();
    }

    void CapitalizeAll();
    void CapitalizeFirstLetter1();
    void CapitalizeFirstLetter2();
    static void CapitalizeFirst1(Wtroka& s);
    static void CapitalizeFirst2(Wtroka& s);


    typedef yvector<TIntrusivePtr<CWordSequence> > TSharedVector;
    static inline void Sort(TSharedVector& vector);


private:
    EWordSequenceType m_WSType;

    Stroka m_NonTerminalSem;
    SWordHomonymNum m_MainWord;

    SDictIndex  m_DicIndex;
    TGztArticle m_GztArticle;

    THomonymGrammems Grammems;

    yvector<SWordSequenceLemma> Lemmas;
    Wtroka LemmaText, CapitalizedLemmaText;

    bool IsArtificialPair_;
    //конкатенация полей, т.е. координаты группы не соответствуют настоящему составу лемм
    bool IsConcatenationLemma_;

    bool IsNormalizedLemma_;
};

inline bool WSLessPtr(const TIntrusivePtr<CWordSequence>& ws1, const TIntrusivePtr<CWordSequence>& ws2) {
    return *ws1 < *ws2;
}

void CWordSequence::Sort(TSharedVector& vector) {
    ::StableSort(vector.begin(), vector.end(), WSLessPtr);
}

class CWordSequenceWithAntecedent: public CWordSequence
{
public:
    CWordSequenceWithAntecedent() { m_bIsAntecedent = false; m_EllipseType = eEllipseCount; };
    ~CWordSequenceWithAntecedent() {};

    enum EEllipses { eCompAbbrevEllipse = 0, eCompDescrEllipse, ePossessiveEllipse, eEmptyDescrEllipse, eEllipseCount };

    bool IsAntecedent() const { return m_bIsAntecedent; }
    void PutAntecedent(bool bVal) { m_bIsAntecedent = bVal; }
    void SetAntecedent() { m_bIsAntecedent = true; };
    void PutEllipseType(EEllipses eTyp) { m_EllipseType = eTyp; }
    EEllipses GetEllipseType() const { return m_EllipseType; }
    bool GetEllipseType(EEllipses eTyp) const { return (eTyp == m_EllipseType);  }
    //virtual bool is_vaid() const { return CWordsPair::is_vaid() || IsAntecedent(); };

protected:
    bool m_bIsAntecedent;
    EEllipses m_EllipseType;
};

struct SCompNameLemmaAttributives
{
    Wtroka m_sCompNameLemma;
    yset<Wtroka> m_CompDescrLemmas;
    yset<Wtroka> m_CompAbbrevLemmas;
    yset<Wtroka> m_CompShortNameLemmas;
    yset<Wtroka> m_CompGeoLemmas;
};

class CCompNameWordSequenceWithAntecedent: public CWordSequenceWithAntecedent
{
public:
    SCompNameLemmaAttributives m_CompNameLemmaAttributives;
};
