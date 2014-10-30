#include "dictionary.h"

#include <util/stream/file.h>
#include <util/stream/lz.h>
#include <util/stream/buffered.h>
#include <util/string/util.h>

#include <kernel/gazetteer/common/serialize.h>
#include <FactExtract/Parser/common/pathhelper.h>

#include <FactExtract/Parser/tomalanglib/abstract.h>


//  static const ui32 AUXDIC_BINARY_VERSION = 1;
//  static const ui32 AUXDIC_BINARY_VERSION = 2;        //  aux-dic does not store kwtype index
    static const ui32 AUXDIC_BINARY_VERSION = 3;        //  MiniLzo -> Lz4 (better license)

bool generate_content_tail(int w, const Wtroka& str, yvector<yvector<Wtroka> >& content, yvector<Wtroka>& contents_str)
{
    Wtroka queue = str;

    for (size_t i = 0; i < content[w].size(); i++) {
        if (w == (int)content.size() - 1)
            contents_str.push_back(queue + content[w][i]);
        else {
            if (content[w][i].empty())
                generate_content_tail(w + 1, queue, content, contents_str);
            else {
                generate_content_tail(w + 1, queue + content[w][i] + ' ', content, contents_str);
            }
        }
    }
    return true;
}

void generate_contents(yvector<yvector<Wtroka> >& content, yvector<Wtroka>& contents_str, bool bAdd___)
{
    //для статей-заглушек, у которых нет поля состав
    if (content.size() == 0) {
        static const Wtroka kUnderlines = CharToWide("___");
        if (bAdd___)
            contents_str.push_back(kUnderlines);
        return;
    }

    Wtroka str;
    generate_content_tail(0, str, content, contents_str);
}

/*----------------------------------struct single_actant_contion_t-----------------------------*/

void single_actant_contion_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_type);
    ::Save(buffer, m_CoordFunc);
    ::Save(buffer, m_vals);
}

void single_actant_contion_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_type);
    ::Load(buffer, m_CoordFunc);
    ::Load(buffer, m_vals);
}

/*----------------------------------struct reference_to_wordchain_t-----------------------------*/
reference_to_wordchain_t::reference_to_wordchain_t()
{
    reset();
}

void reference_to_wordchain_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_contents_str);
    ::Save(buffer, m_kw_type);
    ::Save(buffer, m_title);
}

void reference_to_wordchain_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_contents_str);
    ::Load(buffer, m_kw_type);
    ::Load(buffer, m_title);
}

/*----------------------------------struct reference_to_valency_t-----------------------------*/

void reference_to_valency_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_str_article_title);
    ::Save(buffer, m_str_subbord_name);
    ::Save(buffer, m_str_valency_name);
}

void reference_to_valency_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_str_article_title);
    ::Load(buffer, m_str_subbord_name);
    ::Load(buffer, m_str_valency_name);
}

/*----------------------------------struct valencie_t-----------------------------*/

SDefaultFieldValues valencie_t::s_DefaultValues = SDefaultFieldValues();

valencie_t::valencie_t()
{
    reset();
}

void valencie_t::reset()
{
    m_position = AnyPosition;
    m_clause_type = AnyClauseType;
    m_str_title.clear();
    m_POSes.clear();
    m_conj_POSes.clear();
    m_actant_type = s_DefaultValues.m_ActantTypeVal;
    m_syn_rel = AnyRel;
    m_coord_func = AnyFunc;
    m_modality = Necessary;
    m_conjs.clear();
    m_morph_scale.clear();
    m_kw_type = NULL;
    m_content.clear();
    m_contents_str.clear();
    m_ant.reset();
    m_prefix.reset();
}

void valencie_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_position);
    ::Save(buffer, m_POSes);
    ::Save(buffer, m_morph_scale);
    ::Save(buffer, m_actant_type);
    ::Save(buffer, m_kw_type);
    ::Save(buffer, m_conjs);
    ::Save(buffer, m_conj_POSes);
    ::Save(buffer, m_syn_rel);
    ::Save(buffer, m_clause_type);
    ::Save(buffer, m_coord_func);
    ::Save(buffer, m_modality);
    ::Save(buffer, m_contents_str);
    ::Save(buffer, m_ant);
    ::Save(buffer, m_prefix);
    ::Save(buffer, m_str_title);
}

void valencie_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_position);
    ::Load(buffer, m_POSes);
    ::Load(buffer, m_morph_scale);
    ::Load(buffer, m_actant_type);
    ::Load(buffer, m_kw_type);
    ::Load(buffer, m_conjs);
    ::Load(buffer, m_conj_POSes);
    ::Load(buffer, m_syn_rel);
    ::Load(buffer, m_clause_type);
    ::Load(buffer, m_coord_func);
    ::Load(buffer, m_modality);
    ::Load(buffer, m_contents_str);
    ::Load(buffer, m_ant);
    ::Load(buffer, m_prefix);
    ::Load(buffer, m_str_title);
}

//---------------------------------class or_valencie_list_t-----------------------------------------------

void or_valencie_list_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, (const yvector<valencie_t>&)(*this));
    ::Save(buffer, m_str_name);
    ::Save(buffer, m_interpretations);
    ::Save(buffer, m_reference);
}

void or_valencie_list_t::Load(TInputStream* buffer)
{
    ::Load(buffer, (yvector<valencie_t>&)(*this));
    ::Load(buffer, m_str_name);
    ::Load(buffer, m_interpretations);
    ::Load(buffer, m_reference);
}

//---------------------------------calss valencie_list_t-----------------------------------------------

void valencie_list_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, (const yvector<or_valencie_list_t>&)(*this));
    ::Save(buffer, m_actant_condions);
    ::Save(buffer, m_str_subbord_name);
    ::Save(buffer, m_reference);
}

void valencie_list_t::Load(TInputStream* buffer)
{
    ::Load(buffer, (yvector<or_valencie_list_t>&)(*this));
    ::Load(buffer, m_actant_condions);
    ::Load(buffer, m_str_subbord_name);
    ::Load(buffer, m_reference);
}

/*----------------------------------class article_t-----------------------------*/

SDefaultFieldValues article_t::s_DefaultValues = SDefaultFieldValues();

article_t::article_t()
{
    reset();
}

void article_t::reset()
{
    m_Lemma.Reset();
    m_iMainWord = 0;
    m_KWType = NULL;
    m_POS.Reset();
    m_newPOS.Reset();
    m_strGeoArtRef.clear();
    m_strGeoCentreRef.clear();
    m_iPopulation = 0;
    m_iYGeoID = 0;
    m_del_hom = false;
    m_end_comma = true;
    m_comp_by_lemma = s_DefaultValues.m_comp_by_lemma;
    m_before_comma = false;
    m_is_light_kw = false;
    m_title = Wtroka();
    m_resend_article = Wtroka();
    m_content.clear();
    m_morph_scale.clear();
    m_single_valencies.clear();
    m_subordinations.clear();
    m_fr_valencies.clear();
    m_conj_valencies.clear();
    m_sup_fr_valencies.clear();
    m_corrs.clear();
    m_GoalArticles.clear();
    m_s0.clear();
    m_file_with_words.clear();
    m_file_with_grammar.clear();
    m_str_alg.clear();
    m_positions.clear();
    m_positions.push_back(OnTheBeginning);
    m_agreements.clear();
    m_pGeoPartArtRef = NULL;
    m_pGeoCentrRef = NULL;
    m_clause_type = ClauseTypesCount;
    m_allow_ambiguity = false;
}

bool article_t::get_del_homonym() const
{
    return m_del_hom;
};

const Wtroka& article_t::get_title() const
{
    return m_title;
}

const TGramBitSet& article_t::get_pos() const
{
    return m_POS;
}

const Stroka& article_t::get_gram_file_name() const
{
    return m_file_with_grammar;
}

bool article_t::has_gram_file_name() const
{
    return !m_file_with_grammar.empty();
}

const Stroka& article_t::get_alg() const
{
    return m_str_alg;
}

bool article_t::has_contents_str() const
{
    return m_contents_str.size() > 0;
}

EClauseType article_t::get_clause_type() const
{
    return m_clause_type;
}

int  article_t::get_content_str_count() const
{
    return m_contents_str.size();
}

const Wtroka& article_t::get_content_str(int i) const
{
    return m_contents_str[i];
}

bool article_t::has_agreements() const
{
    return m_agreements.size() > 0;
}

int  article_t::get_agreements_count() const
{
    return m_agreements.size();
}

const SAgreement& article_t::get_agreement(int i) const
{
    if ((i < 0) || ((size_t)i >= m_contents_str.size()))
        ythrow yexception() << "Bad index in \"article_t::get_agreement\".";
    return m_agreements[i];

}

bool article_t::has_alg() const
{
    return !m_str_alg.empty();
}

bool article_t::is_light_kw() const
{
    return m_is_light_kw;
}

bool article_t::get_comp_by_lemma() const
{
    return m_comp_by_lemma;
}

void article_t::put_comp_by_lemma(bool b)
{
    m_comp_by_lemma = b;
}

const TGramBitSet& article_t::get_new_pos() const
{
    return m_newPOS;
}

const TGrammarBunch& article_t::get_morph_info() const
{
    return m_morph_scale;
}

const valencie_list_t& article_t::get_valencies() const
{
    return m_single_valencies;
}

const valencie_list_t& article_t::get_clause_valencies() const
{
    return m_fr_valencies;
};

const valencie_list_t& article_t::get_conj_valencies() const
{
    return m_conj_valencies;
};

const valencie_list_t& article_t::get_sup_valencies() const
{
    return m_sup_fr_valencies;
};

bool article_t::has_text_content() const
{
    return get_contents_count() > 0;
}

long  article_t::get_contents_count() const
{
    return m_contents_str.size();
};

const Wtroka& article_t::get_content(int i) const
{
    return m_contents_str[i];
};

long article_t::get_goal_articles_count() const
{
    return m_GoalArticles.size();
}

long article_t::get_s0_count() const
{
    return m_s0.size();
}

Wtroka article_t::get_s0(long i) const
{
    return m_s0[i];
}

Wtroka article_t::get_goal_article_title(long i) const
{
    return m_GoalArticles[i];
}

EPosition article_t::get_position(int i) const
{
    return m_positions[i];
};

long article_t::get_morph_info_count() const
{
    return m_morph_scale.size();
};

bool article_t::get_end_comma() const
{
    return m_end_comma;
};

long article_t::get_corrs_count() const
{
    return m_corrs.size();
};

Wtroka article_t::get_corr(int i) const
{
    return m_corrs[i];
};

const SArticleLemmaInfo& article_t::get_lemma_info() const
{
    return m_Lemma;
}

Wtroka article_t::get_lemma() const
{
    if (m_Lemma.m_Lemmas.empty())
        return Wtroka();

    Wtroka s = m_Lemma.m_Lemmas[0];
    for (size_t i = 1; i < m_Lemma.m_Lemmas.size(); ++i) {
        s += ' ';
        s += m_Lemma.m_Lemmas[i];
    }
    return StripString(s);
}

bool article_t::has_lemma() const
{
    return !m_Lemma.IsEmpty();
}

bool article_t::allow_ambiguity() const
{
    return m_allow_ambiguity;
}

long article_t::get_ant_count() const
{
    return m_ant_words.size();
};

Wtroka article_t::get_ant(int i) const
{
    return m_ant_words[i];
};

bool article_t::get_has_comma_before() const
{
    return m_before_comma;
};

long article_t::get_positions_count() const
{
    return m_positions.size();
};

TKeyWordType article_t::get_kw_type() const
{
    return m_KWType;
}

void article_t::put_kw_type(TKeyWordType v)
{
    m_KWType = v;
}

int article_t::get_main_word() const
{
    return m_iMainWord;
}

bool article_t::is_simple_article() const
{
    return  m_file_with_grammar.empty() &&
            m_str_alg.empty() &&
            m_POS.none() &&
            m_newPOS.none() &&
            (m_del_hom == false) &&
            (m_end_comma == true) &&
            (m_is_light_kw == false) &&
            (m_before_comma == false) &&
            (m_morph_scale.size() == 0) &&
            (m_subordinations.size() == 0) &&
            (m_fr_valencies.size() == 0) &&
            (m_single_valencies.size() == 0) &&
            (m_conj_valencies.size() == 0) &&
            (m_sup_fr_valencies.size() == 0) &&
            ((m_positions.size() == 1) && (m_positions[0] == OnTheBeginning)) &&
            (m_corrs.size() == 0) &&
            (m_GoalArticles.size() == 0) &&
            (m_ant_words.size() == 0) &&
            (m_agreements.size() == 0);
}

int article_t::get_subordination_count() const
{
    return m_subordinations.size();
}

const valencie_list_t& article_t::get_subordination(int i) const
{
    return  m_subordinations[i];
}

void article_t::copy_resend_article_info(const article_t& art)
{
    if (m_POS.none())
        m_POS = art.m_POS;
    m_newPOS = art.m_newPOS;
    m_del_hom = art.m_del_hom;
    m_end_comma = art.m_end_comma;
    m_comp_by_lemma = art.m_comp_by_lemma;
    m_file_with_words = art.m_file_with_words;
    m_clause_type = art.m_clause_type;
    m_KWType = art.m_KWType;
    m_before_comma = art.m_before_comma;
    if (m_morph_scale.size() == 0)
        m_morph_scale = art.m_morph_scale;
    m_single_valencies = art.m_single_valencies;
    m_subordinations = art.m_subordinations;
    m_fr_valencies = art.m_fr_valencies;
    m_conj_valencies = art.m_conj_valencies;
    m_sup_fr_valencies = art.m_sup_fr_valencies;
    m_positions = art.m_positions;
    m_corrs = art.m_corrs;
    m_GoalArticles = art.m_GoalArticles;
    m_s0 = art.m_s0;
    m_ant_words = art.m_ant_words;
    m_agreements = art.m_agreements;
}

void article_t::generate_contents()
{
    ::generate_contents(m_content, m_contents_str, false);

    for (size_t i = 0; i < m_contents_str.size(); i++)
        m_contents_str[i].to_lower();

    for (size_t i = 0; i < m_subordinations.size(); i++) {
        for (size_t j = 0; j < m_subordinations[i].size(); j++) {
            for (size_t k = 0; k < m_subordinations[i][j].size(); k++)
                update_valencie(m_subordinations[i][j][k]);
        }
    }
}

bool article_t::get_valency_by_ref(or_valencie_list_t&  val, const reference_to_valency_t& ref) const
{
    valencie_list_t sub;
    if (!get_subordination(sub, ref.m_str_subbord_name))
        return false;
    for (size_t i = 0; i < sub.size(); i++) {
        if (sub[i].m_str_name == ref.m_str_valency_name) {
            Wtroka s = val.m_str_name;
            yvector<fact_field_reference_t> saveInterpretations = val.m_interpretations;
            val = sub[i];
            val.m_str_name = s;
            val.m_interpretations.insert(val.m_interpretations.begin(), saveInterpretations.begin(), saveInterpretations.end());
            return true;
        }
    }
    return false;
}

bool article_t::get_subordination(valencie_list_t& sub, const Wtroka& str_name) const
{
    for (size_t i = 0; i < m_subordinations.size(); i++)
        if (m_subordinations[i].m_str_subbord_name == str_name) {
            Wtroka s = sub.m_str_subbord_name;
            sub = m_subordinations[i];
            sub.m_str_subbord_name = s;
            return true;
        }
    return false;
}

void article_t::get_geo_part_references(yvector<const article_t*>& articles) const
{
    const article_t* art_ref = m_pGeoPartArtRef;
    while (art_ref != NULL) {
        articles.push_back(art_ref);
        art_ref = art_ref->m_pGeoPartArtRef;
    }
}

void article_t::get_geo_center_references(yvector<const article_t*>& articles) const
{
    const article_t* art_ref = m_pGeoCentrRef;
    while (art_ref != NULL) {
        articles.push_back(art_ref);
        art_ref = art_ref->m_pGeoCentrRef;
    }
}

int article_t::get_geo_population() const
{
    return m_iPopulation;
}

int article_t::get_y_geo_id() const
{
    return m_iYGeoID;
}

void article_t::update_valencie(valencie_t& val)
{
    ::generate_contents(val.m_ant.m_content, val.m_ant.m_contents_str, false);
    ::generate_contents(val.m_prefix.m_content, val.m_prefix.m_contents_str, false);
    ::generate_contents(val.m_content, val.m_contents_str, false);
}

void article_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_title);
    ::Save(buffer, m_file_with_grammar);
    ::Save(buffer, m_str_alg);
    ::Save(buffer, m_POS);
    ::Save(buffer, m_newPOS);
    ::Save(buffer, m_KWType);
    ::Save(buffer, m_del_hom);
    ::Save(buffer, m_end_comma);
    ::Save(buffer, m_is_light_kw);
    ::Save(buffer, m_comp_by_lemma);
    ::Save(buffer, m_before_comma);
    ::Save(buffer, m_strGeoArtRef);
    ::Save(buffer, m_strGeoCentreRef);
    ::Save(buffer, m_iPopulation);
    ::Save(buffer, m_iYGeoID);
    ::Save(buffer, m_Lemma);
    ::Save(buffer, m_morph_scale);
    ::Save(buffer, m_subordinations);
    ::Save(buffer, m_fr_valencies);
    ::Save(buffer, m_single_valencies);
    ::Save(buffer, m_iMainWord);
    ::Save(buffer, m_conj_valencies);
    ::Save(buffer, m_sup_fr_valencies);
    ::Save(buffer, m_positions);
    ::Save(buffer, m_contents_str);
    ::Save(buffer, m_corrs);
    ::Save(buffer, m_GoalArticles);
    ::Save(buffer, m_s0);
    ::Save(buffer, m_ant_words);
    ::Save(buffer, m_agreements);
    ::Save(buffer, m_clause_type);  // was explicit (int)
    ::Save(buffer, m_allow_ambiguity);
}

void article_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_title);
    ::Load(buffer, m_file_with_grammar);
    ::Load(buffer, m_str_alg);
    ::Load(buffer, m_POS);
    ::Load(buffer, m_newPOS);
    ::Load(buffer, m_KWType);
    ::Load(buffer, m_del_hom);
    ::Load(buffer, m_end_comma);
    ::Load(buffer, m_is_light_kw);
    ::Load(buffer, m_comp_by_lemma);
    ::Load(buffer, m_before_comma);
    ::Load(buffer, m_strGeoArtRef);
    ::Load(buffer, m_strGeoCentreRef);
    ::Load(buffer, m_iPopulation);
    ::Load(buffer, m_iYGeoID);
    ::Load(buffer, m_Lemma);
    ::Load(buffer, m_morph_scale);
    ::Load(buffer, m_subordinations);
    ::Load(buffer, m_fr_valencies);
    ::Load(buffer, m_single_valencies);
    ::Load(buffer, m_iMainWord);
    ::Load(buffer, m_conj_valencies);
    ::Load(buffer, m_sup_fr_valencies);
    ::Load(buffer, m_positions);
    ::Load(buffer, m_contents_str);
    ::Load(buffer, m_corrs);
    ::Load(buffer, m_GoalArticles);
    ::Load(buffer, m_s0);
    ::Load(buffer, m_ant_words);
    ::Load(buffer, m_agreements);
    ::Load(buffer, m_clause_type);
    ::Load(buffer, m_allow_ambiguity);
}

/*----------------------------------class dictionary_t-----------------------------*/

dictionary_t::~dictionary_t()
{
    delete m_empty_article;
    for (size_t i = 0; i < size(); ++i) {
        delete at(i);
    };
    clear();
    m_title2article.clear();
}

bool dictionary_t::add_article(article_t* article)
{
    TPair<ymap<Wtroka, long>::iterator, bool> res =
        m_title2article.insert(ymap<Wtroka, long>::value_type(article->m_title, yvector<article_t*>::size()));
    if (!res.second) {
        return false;
    } else {
        article->generate_contents();
        push_back(article);
        if (article->m_KWType != NULL)
            m_FoundTypes.insert(article->m_KWType);
    }
    return true;
}

long dictionary_t::get_fact_type_count()
{
    return m_FactTypes.size();
}

const fact_type_t& dictionary_t::get_fact_type(int i) const
{
    if ((i < 0) || ((size_t)i >= m_FactTypes.size()))
        ythrow yexception() << "Bad index in \"dictionary_t::get_fact_type\".";
    return m_FactTypes[i];
}

const fact_type_t* dictionary_t::get_fact_type(const Stroka& type_name) const
{
    for (size_t i = 0; i < m_FactTypes.size(); ++i)
        if (m_FactTypes[i].m_strFactTypeName == type_name)
            return &m_FactTypes[i];
    return m_ExternalFactTypes != NULL ? m_ExternalFactTypes->GetFactType(type_name) : NULL;
}

int  dictionary_t::get_article_index(const Wtroka& title) const
{
    ymap<Wtroka, long>::const_iterator it = m_title2article.find(title);
    if (it == m_title2article.end())
        return -1;
    else
        return it->second;
}

article_t* dictionary_t::operator[](int i) const
{
    return at(i);
}

article_t* dictionary_t::operator[](const Wtroka& title)
{
    return get_article(title);
}

void    dictionary_t::fill_geo_references()
{
    for (size_t i = 0; i < size(); ++i)
        check_geo_references(at(i));
}

bool dictionary_t::check_goal_references(article_t* art) const
{
    DECLARE_STATIC_RUS_WORD(mGoal, "ЦЕЛЬ");
    return check_references(art->m_GoalArticles, mGoal, art);
}

bool    dictionary_t::check_s0_references(article_t* art) const
{
    DECLARE_STATIC_RUS_WORD(mS0, "С0");
    return check_references(art->m_s0, mS0, art);
}

bool    dictionary_t::check_references(const yvector<Wtroka>& titles, const Wtroka& strFieldName, article_t* art) const
{
    bool bRes = true;
    for (size_t i = 0; i < titles.size(); ++i)
        if (!get_article(titles[i])) {
            Cerr << "Article \"" << art->m_title << "\" has bad value field \""
                      << strFieldName << "\" (can't find article \""<< titles[i] <<"\")!" << Endl;
            bRes = false;
        }
    return bRes;
}

bool    dictionary_t::check_geo_references(article_t* art)
{
    if (art->m_strGeoArtRef.length() > 0) {
        const article_t* pRefArt =  get_article(art->m_strGeoArtRef);
        if (!pRefArt) {
            Cerr << "Article \"" << art->m_title << "\" has bad value field \"ГЕО_ЧАСТЬ\" (can't find article \""
                      << art->m_strGeoArtRef <<"\")!" << Endl;
            return false;
        }
        art->m_pGeoPartArtRef = pRefArt;
    }

    if (art->m_strGeoCentreRef.length() > 0) {
        const article_t* pRefArt =  get_article(art->m_strGeoCentreRef);
        if (!pRefArt) {
            Cerr << "Article \"" << art->m_title << "\" has bad value field \"ЦЕНТР\" (can't find article \""
                      << art->m_strGeoCentreRef <<"\")!" << Endl;
            return false;
        }
        art->m_pGeoCentrRef = pRefArt;
    }
    return true;
}

article_t*  dictionary_t::get_article(const Wtroka& title)
{
    ymap<Wtroka, long>::const_iterator it;
    it = m_title2article.find(title);
    if (it == m_title2article.end())
        return NULL;
    return at(it->second);
}

const article_t*  dictionary_t::get_article(const Wtroka& title) const
{
    ymap<Wtroka, long>::const_iterator it;
    it = m_title2article.find(title);
    if (it == m_title2article.end())
        return NULL;
    return at(it->second);

}

const article_t* dictionary_t::operator[](const Wtroka& title) const//статью по полю ЗГЛ
{
    return get_article(title);
}

void dictionary_t::erase(Wtroka& title)
{
    ymap<Wtroka, long>::iterator it;
    it = m_title2article.find(title);
    if (it != m_title2article.end())
        m_title2article.erase(it);
}

size_t dictionary_t::size() const
{
    return yvector<article_t*>::size();
}

void dictionary_t::Save(TOutputStream* buffer) const
{
    //пишем статьи
    ::SaveProtected(buffer, (const yvector<article_t*>&)(*this));
    //пишем fact_types
    ::SaveProtected(buffer, m_FactTypes);
}

void dictionary_t::Load(TInputStream* buffer)
{
    //читаем статьи
    ::LoadProtected(buffer, (yvector<article_t*>&)(*this));
    size_t count = this->size();
    for (size_t i = 0; i < count; ++i) {
        article_t* pArt = this->at(i);
        if (pArt->m_KWType != NULL)
            m_FoundTypes.insert(pArt->m_KWType);
        m_title2article[pArt->get_title()] = i;
    }
    //читаем fact_types
    ::LoadProtected(buffer, m_FactTypes);

    fill_geo_references();
}

bool dictionary_t::SaveToFile(const Stroka& file_name)
{
    TOFStream f(file_name);

    // binary protection
    SaveHeader(&f);

    const ui16 block_size = 1 << 15;
    TLz4Compress z(&f, block_size);
    TBufferedOutput b(&z, block_size);

    // in order to collect kw-types table (to save it before all other content), serialize dictionary to tmp-buffer
    TBufferOutput tmpbuf;
    this->Save(&tmpbuf); // virtual

    // now save collected kw-types
    GetKWTypePool().Save(&b);
    // and write already serialized dictionary content after
    ::SaveProtectedSize(&b, tmpbuf.Buffer().Size());
    ::SaveArray(&b, tmpbuf.Buffer().Data(), tmpbuf.Buffer().Size());

    return true;
}

bool dictionary_t::LoadFromFile(const Stroka& file_name)
{
    TIFStream f(file_name);

    // binary protection
    VerifyHeader(&f);

    TLz4Decompress z(&f);

    // first load kw-types table
    GetMutableKWTypePool().Load(&z);
    // then load the dictionary itself
    ::LoadProtectedSize(&z);
    this->Load(&z); // virtual

    return true;
}

void dictionary_t::SaveHeader(TOutputStream* output) const
{
    SaveProtectedSize(output, AUXDIC_BINARY_VERSION);
    SaveProtected(output, SourceFiles);
}

bool dictionary_t::LoadHeader(TInputStream* input)
{
    try {
        size_t version = 0;
        if (!LoadProtectedSize(input, version) || version != AUXDIC_BINARY_VERSION)
            return false;
        LoadProtected(input, SourceFiles);
        return true;
    } catch (yexception&) {
        return false;
    }
}

void dictionary_t::VerifyHeader(TInputStream* input)
{
    if (!LoadHeader(input)) {
        Stroka msg = "Failed to load aux-dictionary: the binary is incompatible with your program. "
                     "You should either re-compiler your .bin, or rebuild your program from corresponding version of sources.\n";
        Cerr << msg << Endl;
        ythrow yexception() << msg;
    }
}

// TODO: make common base class with TTomitaDependencies (almost the same)
class TAuxDicDependencies: public TBinaryGuard {
public:
    TAuxDicDependencies(const Stroka& srcFile)
        : BaseDir(PathHelper::DirName(srcFile))
    {
    }

    virtual bool LoadDependencies(const Stroka& binFile, yvector<TBinaryGuard::TDependency>& res) const {
        dictionary_t tmpDict;
        {
            TIFStream f(binFile);
            if (!tmpDict.LoadHeader(&f))
                return false;
        }

        for (ymap<Stroka, Stroka>::const_iterator it = tmpDict.SourceFiles.begin(); it != tmpDict.SourceFiles.end(); ++it)
            res.push_back(TBinaryGuard::TDependency(it->first, it->second, false));
        return true;
    }

    virtual Stroka GetDiskFileName(const TBinaryGuard::TDependency& dep) const {
        return PathHelper::Join(BaseDir, dep.Name);
    }

private:
    Stroka BaseDir;
};

bool dictionary_t::CompilationRequired(const Stroka& srcFile, const Stroka& binFile, TBinaryGuard::ECompileMode mode) {
    return TAuxDicDependencies(srcFile).RebuildRequired(srcFile, binFile, mode);
}

