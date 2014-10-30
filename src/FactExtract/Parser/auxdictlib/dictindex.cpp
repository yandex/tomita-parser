#include "dictindex.h"

#include <util/stream/file.h>

#include <util/stream/buffered.h>

#include <FactExtract/Parser/common/string_tokenizer.h>
#include <FactExtract/Parser/afdocparser/rus/morph.h>


/*----------------------------------class SGrammemsTemplate------------------------------*/

void SGrammemsTemplate::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_iArtID);
    ::Save(buffer, m_Grammems);
}

void SGrammemsTemplate::Load(TInputStream* buffer)
{
    ::Load(buffer, m_iArtID);
    ::Load(buffer, m_Grammems);
}

/*----------------------------------class SKWTypeInfo------------------------------*/

void SKWTypeInfo::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_Articles);
    ::Save(buffer, m_bOnlyTextContent);
}

void SKWTypeInfo::Load(TInputStream* buffer)
{
    ::Load(buffer, m_Articles);
    ::Load(buffer, m_bOnlyTextContent);
}

/*----------------------------------class SContent2ArticleID------------------------------*/

void SContent2ArticleID::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_Content);
    ::Save(buffer, m_IDs);
}

void SContent2ArticleID::Load(TInputStream* buffer)
{
    ::Load(buffer, m_Content);
    ::Load(buffer, m_IDs);
}

/*----------------------------------class CContentsCluster------------------------------*/

void CContentsCluster::AddContent(yvector<Wtroka>& new_content, int artID)
{
    bool found = false;
    for (size_t i = 0; i < size(); ++i) {
        SContent2ArticleID& cont = (*this)[i];
        if (cont.m_Content == new_content) {
            cont.m_IDs.push_back(artID);
            found = true;
            break;
        }
    }

    if (!found)
        push_back(SContent2ArticleID(new_content, artID));

    if (new_content.size() == 1)
        m_IDs.push_back(artID);
}

void CContentsCluster::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_IDs);
    ::Save(buffer, m_Repr);
    ::Save(buffer, (const TBaseClass&)(*this));
}

void CContentsCluster::Load(TInputStream* buffer)
{
    ::Load(buffer, m_IDs);
    ::Load(buffer, m_Repr);
    ::Load(buffer, (TBaseClass&)(*this));
}

/*----------------------------------class CArticleContents------------------------------*/

CContentsCluster* CArticleContents::FindCluster(const Wtroka& word, ECompType CompType) const
{
    TClusterMap::const_iterator it;
    return FindCluster(word, CompType, it) ? it->second.Get() : NULL;
}

bool CArticleContents::FindClusterForNonDicWord(const Wtroka& word, const TClusterMap& clusters,
                                                yvector<CContentsCluster*>& foundClusters) const
{
    int iFlexLen = TMorph::MaxLenFlex(word);
    TWtringBuf word_stem(~word, +word - iFlexLen);
    TClusterMap::const_iterator it = clusters.lower_bound(word_stem);

    for (; it != clusters.end(); ++it) {
        const TWtringBuf& key = it->first;
        if (+key.size() < word_stem.size())
            continue;
        TWtringBuf key_stem(~key, +word_stem);
        if (key_stem != word_stem)
            break;
        if (TMorph::Similar(word, key))
            foundClusters.push_back(it->second.Get());
    }
    return foundClusters.size() > 0;
}

static ui8 GetIndex(const Wtroka& word)
{
    YASSERT(!word.empty());
    static const Encoder& Encoder = EncoderByCharset(CODES_WIN);
    int index = static_cast<ui8>(Encoder.Tr(word[0])) - static_cast<ui8>(RUS_LOWER_A);
    if (index < 0 || index >= s_iRussianLowerLettersCount)
        index = s_iClusterIndexSize - 1;

    return static_cast<ui8>(index);
}

ui8 CArticleContents::GetPositionByLetter(const Wtroka& word, ECompType CompType) const
{
    ui8 c = GetIndex(word);
    if (m_Clusters[CompType][c].size() == 0)
        return s_iClusterIndexSize;
    return c;
}

bool CArticleContents::FindCluster(const Wtroka& word, ECompType CompType, yvector<CContentsCluster*>& foundClusters) const
{
    if (CompType == CompByLemmaSimilar && !IsForMrph(word))
        CompType = CompByLemma;

    ui8 c = GetPositionByLetter(word, CompType);
    if (c == s_iClusterIndexSize)
        return false;

    if (CompType == CompByLemmaSimilar)
        return FindClusterForNonDicWord(word, m_Clusters[CompType][c], foundClusters);
    else {
        TClusterMap::const_iterator it = m_Clusters[CompType][c].find(word);
        if (it == m_Clusters[CompType][c].end())
            return false;
        else {
            foundClusters.push_back(it->second.Get());
            return true;
        }
    }
}

bool CArticleContents::FindCluster(const Wtroka& word, ECompType CompType,
                                   TClusterMap::const_iterator& it) const
{
    ui8 c = GetPositionByLetter(word, CompType);
    if (c == s_iClusterIndexSize)
        return false;

    it = m_Clusters[CompType][c].find(word);
    return it != m_Clusters[CompType][c].end();
}

void CArticleContents::Clear()
{
    for (int i = 0; i < CompTypeCount; ++i)
        for (int j = 0; j < s_iClusterIndexSize; ++j)
            m_Clusters[i][j].clear();
}

bool CArticleContents::IsForMrph(const Wtroka& w) const
{
    DECLARE_STATIC_RUS_WORD(kRusLetters, "йцукенгшщзхъёфывапролджэячсмитьбю-");
    return w.find_first_not_of(kRusLetters) == Wtroka::npos && w.size() > 3;
}

bool CArticleContents::Init(dictionary_t& dict)
{
    for (size_t i = 0; i < dict.size(); ++i) {
        long size = dict[i]->get_contents_count();
        for (int j = 0; j < size; ++j) {
            Wtroka content_str = dict[i]->get_content(j);
            yvector<Wtroka> content;
            Wtroka str;
            for (TWtrokaTokenizer tokenizer(content_str, " "); tokenizer.NextAsString(str);) {
                str.to_lower();
                content.push_back(str);
            }
            if (content.size() == 0)
                continue;

            if (dict[i]->get_comp_by_lemma()) {
                if (IsForMrph(content[0]) && !TMorph::FindWord(content[0]))
                    AddContent(content, i, CompByLemmaSimilar);
                else
                    AddContent(content, i, CompByLemma);
            } else
                AddContent(content, i, CompByWord);
        }
    }

    //отсортируем содержимое каждого кластера
    SortClustersContent();

    return true;
}

void CArticleContents::SortClustersContent()
{
    for (int i = 0; i < CompTypeCount; ++i)
        for (int j = 0; j < s_iClusterIndexSize; ++j)
            for (TClusterMap::iterator it = m_Clusters[i][j].begin(); it != m_Clusters[i][j].end(); ++it)
                it->second->Sort();
}

void CArticleContents::AddContent(yvector<Wtroka>& content, int artID, ECompType CompType)
{
    if (content.empty())
        return;

    TClusterMap::const_iterator it;
    const Wtroka& key = content[0];
    if (FindCluster(key, CompType, it))
        it->second->AddContent(content, artID);
    else {
        ui8 c = GetIndex(key);
        TSharedPtr<CContentsCluster> cluster(new CContentsCluster(key));
        cluster->AddContent(content, artID);
        m_Clusters[CompType][c][key] = cluster;
    }
}

void CArticleContents::Save(TOutputStream* buffer) const
{
    for (int i = 0; i < CompTypeCount; ++i)
        for (int j = 0; j < s_iClusterIndexSize; ++j) {
            ::SaveSize(buffer, m_Clusters[i][j].size());
            // save only cluster (without key)
            for (TClusterMap::const_iterator it = m_Clusters[i][j].begin(); it != m_Clusters[i][j].end(); ++it)
                ::Save(buffer, *(it->second));
        }
}

void CArticleContents::Load(TInputStream* buffer)
{
    for (int i = 0; i < CompTypeCount; ++i)
        for (int j = 0; j < s_iClusterIndexSize; ++j) {
            size_t size = ::LoadSize(buffer);
            for (size_t k = 0; k < size; ++k) {
                TSharedPtr<CContentsCluster> cluster(new CContentsCluster());
                ::Load(buffer, *cluster);
                m_Clusters[i][j][cluster->GetRepr()] = cluster;
            }
        }
}

//---------------------------------class CIndexedDictionary--------------------------------------------------
CIndexedDictionary::~CIndexedDictionary()
{

}

void CIndexedDictionary::InitKWIndex()
{
    int artCount = size();
    for (int i = 0; i < artCount; i++) {
        article_t* piArt = at(i);
        TKeyWordType kwType = piArt->get_kw_type();
        if (kwType == NULL)
            continue;
        SKWTypeInfo& kwtype_info = m_KWType2Articles[kwType];
        kwtype_info.m_Articles.push_back(i);
        if (!piArt->has_text_content())
            kwtype_info.m_bOnlyTextContent = false;
    }
}

bool CIndexedDictionary::InitGrammemsTemplates()
{
    int artCount = size();
    for (int i = 0; i < artCount; i++) {
        article_t* piArt = at(i);
        const TGramBitSet& pos = piArt->get_pos();
        if (!pos.any())
            continue;

        yvector<SGrammemsTemplate>& templates = m_GrammemsTemplates[pos];
        templates.push_back(SGrammemsTemplate(i));
        templates.back().m_Grammems.insert(piArt->get_morph_info().begin(), piArt->get_morph_info().end());
    }
    return true;
}

void CIndexedDictionary::Save(TOutputStream* buffer) const
{
    dictionary_t::Save(buffer);
    ::Save(buffer, m_Contents);
    ::Save(buffer, m_GrammemsTemplates);
}

void CIndexedDictionary::Load(TInputStream* buffer)
{
    dictionary_t::Load(buffer);
    ::Load(buffer, m_Contents);
    ::Load(buffer, m_GrammemsTemplates);
    InitKWIndex();
}

