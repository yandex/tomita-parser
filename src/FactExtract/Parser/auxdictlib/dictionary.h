#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/common_constants.h>
#include <FactExtract/Parser/common/gramstr.h>
#include <FactExtract/Parser/common/serializers.h>
#include <FactExtract/Parser/common/fact-types.h>

#include <library/lemmer/dictlib/grammarhelper.h>
#include <kernel/gazetteer/common/binaryguard.h>

struct SDefaultFieldValues
{
    SDefaultFieldValues()
        : m_ActantTypeVal(WordActant)
        , m_comp_by_lemma(false)
    {
    }

    EActantType    m_ActantTypeVal;
    bool m_comp_by_lemma;
};

struct reference_to_wordchain_t
{
    reference_to_wordchain_t();
    void reset()
    {
        m_contents_str.clear();
        m_content.clear();
        m_title.clear();
        m_kw_type = NULL;
    }

    bool is_valid() const
    {
        return (m_title.length() > 0) || (m_contents_str.size() > 0) || (m_kw_type != NULL);
    }

    yvector<Wtroka>    m_contents_str;        //все возможные варианты состава этой статьи в одной строчке,
                                                //в которой слова разделены символом '#'
    yvector<yvector<Wtroka> >    m_content;    //поле СОСТАВ (вектор возможных альтернатив)s
    TKeyWordType m_kw_type;                     //поле ТИП_КС
    Wtroka m_title;                        //поле ЗГЛ

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct reference_to_valency_t
{
    bool is_valid_for_valency() const { return (m_str_article_title.length() > 0) && (m_str_subbord_name.length() > 0) && (m_str_valency_name.length() > 0); }
    bool is_valid_for_subbord() const { return (m_str_article_title.length() > 0) && (m_str_subbord_name.length() > 0); }

    Wtroka m_str_article_title;
    Wtroka m_str_subbord_name;
    Wtroka m_str_valency_name;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct valencie_t
{
    valencie_t();
    void reset();

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    EPosition m_position;    //поле ПОЗ
    TGrammarBunch m_POSes;    //поле    ЧР (битовая шкала частей речи)
    TGrammarBunch m_morph_scale; //битовые шкалы с морф. инфо.
    EActantType        m_actant_type;    //поле    ТИП
    TKeyWordType            m_kw_type;        //поле    ТИП_КС
    yvector<Wtroka>    m_conjs;        //поле ЗГЛ нужных союзов
    TGrammarBunch m_conj_POSes;
    ESynRelation    m_syn_rel;        //поле СИН_О
    EClauseType        m_clause_type;
    ECoordFunc    m_coord_func;
    EModality    m_modality;
    yvector<Wtroka>    m_contents_str;        //все возможные варианты состава этой статьи в одной строчке,
                                        //в которой слова разделены символом '#'
    yvector<yvector<Wtroka> >    m_content;    //поле СОСТАВ (вектор возможных альтернатив)
    reference_to_wordchain_t m_ant; //поле АНТ:
    reference_to_wordchain_t m_prefix; //поле ПРЕФИКС:
    Wtroka m_str_title;
    //Wtroka    m_str_val_name;

    static SDefaultFieldValues s_DefaultValues;
};

//то, что написано в статье через "|"
struct or_valencie_list_t : public yvector<valencie_t>
{
    or_valencie_list_t& operator=(const yvector<valencie_t>& val)
    {
        yvector<valencie_t>::operator=(val);
        return *this;
    }

    void reset()
    {
        clear();
        m_str_name.clear();
    }

    EModality get_modality() const
    {
        for (int i = 0; i < (int)size(); i++) {
            if (at(i).m_modality == Necessary)
                return Necessary;
        }
        return Possibly;
    }

    or_valencie_list_t()
    {
    }
    or_valencie_list_t(const yvector<valencie_t>& val) : yvector<valencie_t>(val)
    {

    }
    Wtroka m_str_name;
    yvector<fact_field_reference_t> m_interpretations;
    reference_to_valency_t m_reference;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct single_actant_contion_t
{
    EActantCondinionType m_type;
    ECoordFunc m_CoordFunc;
    yvector<Wtroka> m_vals;

    single_actant_contion_t()
        : m_type(ActantCondinionTypeCount)
        , m_CoordFunc(AnyFunc)
    {
    }

    bool is_THIS_label(int i) const {
        return m_vals[i] == ACTANT_LABEL_SYMBOL;
    }

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct actant_contions_t
{
    yvector<single_actant_contion_t> m_conditions;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_conditions);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_conditions);
    }
};

//то, что написано в статье через пробел
struct valencie_list_t : public yvector<or_valencie_list_t>
{
    valencie_list_t& operator=(const yvector<or_valencie_list_t>& val)
    {
        yvector<or_valencie_list_t>::operator=(val);
        return *this;
    }

    void reset()
    {
        clear();
        m_actant_condions.clear();
    }

    yvector<actant_contions_t> m_actant_condions; //альтернативные условия на актанты
    Wtroka m_str_subbord_name;
    reference_to_valency_t m_reference;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct SAgreement
{
    SAgreement(ECoordFunc agr, int iW1, int iW2) : m_agr(agr), m_iW1(iW1), m_iW2(iW2)
    {
    }

    SAgreement() : m_agr(AnyFunc), m_iW1(-1), m_iW2(-1)
    {
    }

    ECoordFunc m_agr;
    int m_iW1;
    int m_iW2;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_agr);
        ::Save(buffer, m_iW1);
        ::Save(buffer, m_iW2);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_agr);
        ::Load(buffer, m_iW1);
        ::Load(buffer, m_iW2);
    }

};

class SArticleLemmaInfo
{
public:

    SArticleLemmaInfo()
        : m_iMainWord(0)
        , m_bIndeclinable(false)
        , m_bAlwaysReplace(false)
    {
    };

    void Reset()
    {
        m_Lemmas.clear();
        m_iMainWord = 0;
        m_bIndeclinable = false;
        m_bAlwaysReplace = false;
    }

    bool IsEmpty() const
    {
        return m_Lemmas.size()== 0;
    }

    yvector<Wtroka> m_Lemmas;
    int m_iMainWord;
    bool m_bIndeclinable;
    bool m_bAlwaysReplace;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_Lemmas);
        ::Save(buffer, m_iMainWord);
        ::Save(buffer, m_bIndeclinable);
        ::Save(buffer, m_bAlwaysReplace);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_Lemmas);
        ::Load(buffer, m_iMainWord);
        ::Load(buffer, m_bIndeclinable);
        ::Load(buffer, m_bAlwaysReplace);
    }
};

class articles_parser_t;

class article_t
{
    friend class dictionary_t;
    friend class articles_parser_t;
    friend int yyparse(articles_parser_t*);
public:
    article_t();
    void reset();
    void generate_contents();
    void copy_resend_article_info(const article_t& art); //копирует всё кроме ЗГЛ, ГС и СОСТАВ и полей, для которых определено значение

public:
    bool get_del_homonym() const;
    const Wtroka& get_title() const;
    const TGramBitSet& get_pos() const;
    const TGramBitSet& get_new_pos() const;
    const TGrammarBunch&  get_morph_info() const;
    const Stroka& get_gram_file_name() const;
    bool has_gram_file_name() const;
    const Stroka& get_alg() const;
    bool has_alg() const;
    bool is_light_kw() const;
    bool get_comp_by_lemma() const;
    const SArticleLemmaInfo& get_lemma_info() const;
    void put_comp_by_lemma(bool b);
    long get_goal_articles_count() const;
    Wtroka get_goal_article_title(long i) const;
    void get_geo_part_references(yvector<const article_t*>& articles) const;
    void get_geo_center_references(yvector<const article_t*>& articles) const;
    const valencie_list_t& get_valencies() const;
    const valencie_list_t& get_clause_valencies() const;
    const valencie_list_t& get_conj_valencies() const;
    const valencie_list_t& get_sup_valencies() const;
    bool has_text_content() const;
    long  get_contents_count() const;
    const Wtroka& get_content(int i) const;
    EPosition get_position(int i) const;
    long get_morph_info_count() const;
    bool get_end_comma() const;
    long get_corrs_count() const;
    Wtroka get_corr(int i) const;
    long get_ant_count() const;
    Wtroka get_ant(int i) const;
    bool get_has_comma_before() const;
    long get_positions_count() const;
    TKeyWordType get_kw_type() const;
    void put_kw_type(TKeyWordType v);
    int get_main_word() const;
    int get_subordination_count() const;
    bool get_subordination(valencie_list_t& sub, const Wtroka& str_name) const;
    bool get_valency_by_ref(or_valencie_list_t&  val, const reference_to_valency_t& ref) const;
    const valencie_list_t& get_subordination(int i) const;
    bool has_lemma() const;
    Wtroka get_lemma() const;
    bool allow_ambiguity() const;
    static void update_valencie(valencie_t& val);

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    bool has_contents_str() const;
    int  get_content_str_count() const;
    const Wtroka&  get_content_str(int i) const;

    bool has_agreements() const;
    int  get_agreements_count() const;
    const SAgreement&  get_agreement(int i) const;
    bool is_simple_article() const;
    EClauseType get_clause_type() const;
    int get_geo_population() const;
    int get_y_geo_id() const;

    long get_s0_count() const;
    Wtroka get_s0(long i) const;

protected:
/**/Wtroka m_title;                        //поле ЗГЛ
    Wtroka m_resend_article;            //поле ОТС
    Stroka m_file_with_words;            //файл с ключевыми словами ( в поле состав)
    Stroka m_file_with_grammar;            //файл с грамматикой ( в поле состав)
    Stroka m_str_alg;                    //файл с предопределенным алгоритмом ( в поле состав)
    yvector< yvector<Wtroka> >    m_content;    //поле СОСТАВ (вектор возможных альтернатив)
    TGramBitSet m_POS;                //поле ЧР
    TGramBitSet m_newPOS;                //поле Н_ЧР
/**/TKeyWordType    m_KWType;                    //поле ТИП_КС
    bool  m_del_hom;                    //поле УД_ОМОНИМ
    bool  m_end_comma;                    //поле КОНЕЦ_ЗПТ
    bool  m_is_light_kw;                    //поле КОНЕЦ_ЗПТ
/**/bool  m_comp_by_lemma;                //поле СРАВН_ЛЕММА
    bool  m_before_comma;                //поле ЗПТ
/**/Wtroka    m_strGeoArtRef;                //поле ГЕО_ЧАСТЬ
/**/Wtroka    m_strGeoCentreRef;                //поле ЦЕНТР
/**/const article_t* m_pGeoPartArtRef;
/**/const article_t* m_pGeoCentrRef;
/**/int  m_iPopulation;                    //поле НАСЕЛЕНИЕ
    int  m_iYGeoID;                    //поле Я_ГЕО_ИД
    /*mutable*/ SArticleLemmaInfo m_Lemma;                    //поле ЛЕММА
    TGrammarBunch m_morph_scale;        //битовые шкалы с морф. инфо.
    yvector<valencie_list_t>    m_subordinations; //m_valencies;        //поля УПРi
    valencie_list_t    m_fr_valencies;        //поле ФР_УПР
    valencie_list_t    m_single_valencies;    //поле УПР
/**/int    m_iMainWord;                    //главное слово (то, у которого проверяются поля МИ и ЧР статьи)
    valencie_list_t    m_conj_valencies;    //поле СОЮЗ_УПР
    valencie_list_t    m_sup_fr_valencies;    //поле ГЛАВ
    yvector<EPosition> m_positions;                //поле ПОЗ
/**/yvector<Wtroka>    m_contents_str;        //все возможные варианты состава этой статьи в одной строчке,
                                        //в которой слова разделены символом '#'
    yvector<Wtroka>    m_corrs;            //поле  КОРР
    yvector<Wtroka>    m_s0;               //поле С0 (ссылки на статьи, от которых образованы данные прилагательные)
    yvector<Wtroka>    m_GoalArticles;        //поле  ЦЕЛЬ
    yvector<Wtroka>    m_ant_words;        //поле  АНТ
    yvector<SAgreement>    m_agreements;    //поле СОГЛ
    EClauseType        m_clause_type;        //поле ТИП_ФР
    bool            m_allow_ambiguity;  //поле AMBIGUITY

public:

    bool generate_content_tail(int w, Wtroka& str);

    static SDefaultFieldValues s_DefaultValues;

};

// Interface for checking referred articles titles (used to ask gazetteer articles)
class IExternalArticles
{
public:
    virtual bool HasArticle(const Wtroka& title) const = 0;
};

// forward
class IFactTypeDictionary;

//article_t* - удаляется в деструктуре CCOMArticle
class dictionary_t : protected yvector<article_t*>
{
friend class articles_parser_t;
friend int yyparse(articles_parser_t*);

public:
    inline dictionary_t()
        : m_ExternalArticles(NULL)
        , m_empty_article(new article_t)
    {
    }

    virtual ~dictionary_t();

    void SetExternalArticles(const IExternalArticles* external_articles)
    {
        m_ExternalArticles = external_articles;
    }

    void SetExternalFactTypes(const IFactTypeDictionary* externalFactTypes) {
        m_ExternalFactTypes = externalFactTypes;
    }

    bool add_article(article_t*);
    article_t* operator[](int i) const;
    const article_t* operator[](const Wtroka& title) const; //статью по полю ЗГЛ
    article_t* operator[](const Wtroka& title); //статью по полю ЗГЛ
    const article_t* get_empty_article() const { return m_empty_article; }
    size_t size() const;
    void erase(Wtroka& title);

    inline bool has_article(const Wtroka& title) const
    {
        return m_title2article.find(title) != m_title2article.end();
    }

    inline bool has_external_article(const Wtroka& title) const
    {
        return m_ExternalArticles != NULL && m_ExternalArticles->HasArticle(title);
    }

    int get_article_index(const Wtroka& title) const;
    const article_t*  get_article(const Wtroka& title) const;
    article_t* get_article(const Wtroka& title);

    inline bool has_art_with_kw_type(TKeyWordType t) const
    {
        return m_FoundTypes.find(t) != m_FoundTypes.end();
    }

//Fact types are separated into special class: TFactTypeHolder (see afdocparser/common/facttypeholder.h)
private:
    const fact_type_t&    get_fact_type(int i) const;
    const fact_type_t* get_fact_type(const Stroka& type_name) const;
    long get_fact_type_count();
public:
    // This methods is only for transferring fact-types from aux-dic to TFactTypeHolder.
    const yvector<fact_type_t>& GetFactTypes() const {
        return m_FactTypes;
    }

    void    fill_geo_references();

    bool check_geo_references(article_t* art);
    bool check_goal_references(article_t* art) const;
    bool check_references(const yvector<Wtroka>& titles, const Wtroka& strFieldName, article_t* art) const;
    bool check_s0_references(article_t* art) const;

    void SaveHeader(TOutputStream* buffer) const;
    bool LoadHeader(TInputStream* buffer);
    void VerifyHeader(TInputStream* buffer);    // load and fail if header is bad

    virtual void Save(TOutputStream* buffer) const;
    virtual void Load(TInputStream* buffer);

    bool SaveToFile(const Stroka& file_name);
    bool LoadFromFile(const Stroka& file_name);

    bool CompilationRequired(const Stroka& srcFile, const Stroka& binFile, TBinaryGuard::ECompileMode mode);
public:
    ymap<Stroka, Stroka> SourceFiles;       // source files -> checksum (md5)

protected:
    const IExternalArticles* m_ExternalArticles;
    const IFactTypeDictionary* m_ExternalFactTypes;

    yset<TKeyWordType> m_FoundTypes;
    yvector<fact_type_t> m_FactTypes;

protected:
    ymap<Wtroka, long>    m_title2article;
    article_t* m_empty_article;

};
