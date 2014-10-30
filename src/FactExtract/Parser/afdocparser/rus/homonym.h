#pragma once

#include <FactExtract/Parser/afdocparser/common/homonymbase.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/morph.h>
#include <FactExtract/Parser/common/inputitem.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <library/lemmer/dictlib/yx_gram.h>
#include <library/lemmer/dictlib/grammarhelper.h>
#include <library/lemmer/dictlib/gleiche.h>

#include <FactExtract/Parser/afdocparser/common/gztarticle.h>


class CWordSequence;

class CHomonym : public CHomonymBase, public CInputItem
{

friend class CWord;
friend class CDictsHolder;

public:
    CHomonym(docLanguage lang, const Wtroka& sLemma = Wtroka());

private:
    virtual void CopyTo(CHomonymBase* pHom) const;
    void ResetGenderFromHomonym(const CHomonym& pH);

public:
    THomonymPtr Clone() const;

    bool InitNameType();
    void SetNameType(ENameType type)  { m_NameType.SafeSet(type); }
    bool IsSurname() const            { return m_NameType.Test(Surname); }
    bool IsFirstName() const          { return m_NameType.Test(FirstName); }
    bool IsPatronomyc() const         { return m_NameType.Test(Patronomyc); }
    bool HasNameType(ENameType type) const { return m_NameType.SafeTest(type); }

    void Delete();
    void Undelete();
    bool IsDeleted() const;

    Wtroka GetShortLemma() const;
    virtual Wtroka GetLemma() const;
    bool HasArtificialLemma() const;

    virtual void AddLemmaPrefix(const Wtroka& sPrefix);

    void Print(TOutputStream& stream, const Stroka& strKwType, ECharset encoding) const;
    void Print(TOutputStream& stream, ECharset encoding) const;
    void PrintFormGrammems(TOutputStream& stream, ECharset encoding) const;
    static void PrintGrammems(const TGramBitSet& grammems, TOutputStream& stream, ECharset encoding);

    bool ContainTerminal(TerminalSymbolType term) const;
    virtual CWordSequence* GetSourceWordSequence() const;
    void SetSourceWordSequence(CWordSequence* pSourceWordSequence);

    void AddLabel(const Wtroka& strLabel);
    bool HasLabel(const Wtroka& strLabel) const;
    Stroka GetLabelsString(ECharset encoding) const;
    Stroka GetStrKWType(ECharset encoding) const;

    bool HasGztArticle() const;
    bool HasAuxArticle(EDicType dicType) const;

    bool HasArticle(const SArtPointer& artP, EDicType dic) const;
    inline bool HasKWType(TKeyWordType t, EDicType dic) const {
        return HasArticle(SArtPointer(t), dic);
    }

    void PutAuxArticle(const SDictIndex& DictIndex);
    void PutGztArticle(const TGztArticle& gzt_article);
    inline void PutArticle(const TArticleRef& article) {
        if (article.AuxDic().IsValid())
            PutAuxArticle(article.AuxDic());
        else
            PutGztArticle(article.Gzt());
    }

    int GetAuxArticleIndex(EDicType dicType) const;
    const TGztArticle& GetGztArticle() const;

    typedef yhash<TKeyWordType, yvector<SDictIndex> > TKWType2Articles;
    inline const TKWType2Articles& GetKWType2Articles() const {
        return m_KWtype2Articles;
    }

    typedef yset<TGztArticle> TExtraGztArticles;
    inline const TExtraGztArticles& GetExtraGztArticles() const {
        return m_ExtraGztArticles;
    }

    bool HasTerminalSymbol(int iTerminal) const;
    const CHomonym* GetAnalyticMainVerbHom() const { return m_pAnalyticMainVerbHom; };
    void SetAnalyticMainVerbHom(const CHomonym* pVerbBeHom) { m_pAnalyticMainVerbHom = pVerbBeHom; };

private: // Data //

    typedef TEnumBitSet<ENameType, FirstName, NoName> TNameTypeBitSet;
    TNameTypeBitSet m_NameType;

    bool m_bDeleted;

    int    m_DicIndexes[DICTYPE_COUNT];
    const CHomonym* m_pAnalyticMainVerbHom;
    CWordSequence* m_pSourceWordSequence;

    yset<Wtroka> m_Labels;//заголовки тех статей, в которых ЛЕГКОЕ_КС = да

    TKWType2Articles m_KWtype2Articles;
    TGztArticle m_GztArticle;

    // Additional set of gazetteer articles, corresponding to this homonym
    TExtraGztArticles m_ExtraGztArticles;

};

//////////////////////////////////////////////////////////////////////////////

/*
inline
bool CHomonym::HasOnlyGrammems(const TGramBitSet& grammems, const TGramBitSet& grammem_type) const
{
    return (m_AllGrammems & grammem_type) == grammems;
}

inline
bool CHomonym::HasOnlyGrammem(const TGrammar grammem, const TGramBitSet& grammem_type) const
{
    return HasOnlyGrammems(TGramBitSet(grammem), grammem_type);
}
*/

