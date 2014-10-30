#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

#include <FactExtract/Parser/common/serializers.h>

#include "dictionary.h"
#include <util/generic/map.h>


enum ECompType { CompByLemma = 0, CompByWord, CompByLemmaSimilar, CompTypeCount };

struct SContent2ArticleID
{
    yvector<Wtroka> m_Content;
    yvector<long> m_IDs;

    SContent2ArticleID()
    {
    }

    SContent2ArticleID(yvector<Wtroka>& content, int id)
        : m_Content(content)
        , m_IDs(1, id)
    {
    }

    bool operator<(const SContent2ArticleID& cont) const
    {
        return m_Content.size() > cont.m_Content.size();
    }

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

};

//все поля СОСТАВ , у которых первые слова одинаковы
//причем все они упорядочены по количеству слов
class CContentsCluster : public yvector<SContent2ArticleID>
{
private:
    typedef yvector<SContent2ArticleID> TBaseClass;

public:

    inline CContentsCluster()
    {
    }

    explicit inline CContentsCluster(const Wtroka& str)
        : m_Repr(str)
    {
    }

    bool operator < (const CContentsCluster& clust)
    {
        return m_Repr < clust.m_Repr;
    }

    inline const Wtroka& GetRepr() const
    {
        return m_Repr;
    }

    void AddContent(yvector<Wtroka>& new_content, int artID);

    inline void Sort()
    {
        std::sort(begin(), end());
    }

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

private:
    //представитель этого кластера
    Wtroka m_Repr;

public:
    yvector<long> m_IDs; //номер статьи, для слова m_Repr (если такой статьи нет, то -1)
};

//хранятся поля СОСТАВ в удобном для поиска виде
const int s_iRussianLowerLettersCount = RUS_LOWER_YA - RUS_LOWER_A + 1;

//+1 для тех слов, у которых первая буква не кирилица
const int s_iClusterIndexSize = s_iRussianLowerLettersCount + 1;
class CArticleContents
{

    friend class CGramInfo;

public:
    bool Init(dictionary_t& dict);
    void Clear();
    void SortClustersContent();
    CContentsCluster* FindCluster(const Wtroka& word, ECompType CompType) const;
    bool FindCluster(const Wtroka& word, ECompType CompType, yvector<CContentsCluster*>& foundClusters) const;
    void AddContent(yvector<Wtroka>& content, int artID, ECompType CompType);

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

private:
    // Different maps for searching articles by lemma or by wordform
    // Note that each key data belongs to corresponding CContentsCluster object, so we should be a bit careful with keys
    // But this allow lookups of sub-strings without memory copying.
    typedef ymap<TWtringBuf, TSharedPtr<CContentsCluster> > TClusterMap;
    TClusterMap m_Clusters[CompTypeCount][s_iClusterIndexSize];

private:
    bool IsForMrph(const Wtroka& w) const;

    bool FindCluster(const Wtroka& word, ECompType CompType, TClusterMap::const_iterator& it) const;
    bool FindClusterForNonDicWord(const Wtroka& word, const TClusterMap& clusters,
                                  yvector<CContentsCluster*>& foundClusters) const;
    ui8 GetPositionByLetter(const Wtroka& word, ECompType CompType) const;

};

struct SGrammemsTemplate
{
    SGrammemsTemplate() : m_iArtID(0)
    {}

    SGrammemsTemplate(int iArtID)
    {m_iArtID = iArtID;}

    int m_iArtID;
    TGrammarBunch m_Grammems;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

};

struct SKWTypeInfo
{
    SKWTypeInfo()
    {
        m_bOnlyTextContent = true;
    }

    yvector<int> m_Articles;
    bool m_bOnlyTextContent;//все статьи с этим типом имеюют только текстовое поле состав

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

};

struct CIndexedDictionary : public dictionary_t
{

    CArticleContents  m_Contents;
    ~CIndexedDictionary();
    void Clear() {
        m_Contents.Clear();
    }

    inline bool IsEmpty() const {
        return size() == 0;
    }

    bool Init() {
        return m_Contents.Init(*this);
    }
    bool InitGrammemsTemplates();
    void InitKWIndex();
    const article_t* GetArticle(size_t iArt) const {
        return (*this)[iArt];
    }

    typedef ymap<TGramBitSet, yvector<SGrammemsTemplate> > TGrammemTemplates;
    TGrammemTemplates m_GrammemsTemplates; //POS index

    yhash<TKeyWordType, SKWTypeInfo> m_KWType2Articles; //индекс по полю ТИП_КС

    inline const yvector<int>* GetAuxArticles(TKeyWordType kwtype) const
    {
        yhash<TKeyWordType, SKWTypeInfo>::const_iterator kwtype_info = m_KWType2Articles.find(kwtype);
        if (kwtype_info == m_KWType2Articles.end())
            return NULL;
        else
            return &kwtype_info->second.m_Articles;
    }

    virtual void Save(TOutputStream* buffer) const;
    virtual void Load(TInputStream* buffer);
};
