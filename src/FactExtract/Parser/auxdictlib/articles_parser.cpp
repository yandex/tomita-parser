#include "articles_parser.h"
#include <FactExtract/Parser/auxdictlib/articles_syn_yacc.h>

#include <util/string/util.h>
#include <util/stream/str.h>
#include <util/stream/file.h>
#include <fstream>
#include <sstream>

#include <FactExtract/Parser/common/string_tokenizer.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/common/toma_constants.h>

#include <kernel/gazetteer/common/binaryguard.h>


articles_parser_t::articles_parser_t(dictionary_t& dict, SDefaultFieldValues DefaultValues)
    : m_dict(dict)
    , m_error(false)
{
    //m_DefaultValues = DefaultValues;
    valencie_t::s_DefaultValues = DefaultValues;
    article_t::s_DefaultValues = DefaultValues;
    m_cur_article.reset();
    m_cur_valencie.reset();
}

TKeyWordType articles_parser_t::ParseKWType(char* str_value)
{
    Stroka kwtype_name(str_value);
    delete str_value;
    if (kwtype_name.empty()) {
        yyerror("Empty kw-type is not allowed.");
        return NULL;
    }
    TKeyWordType res = GetKWTypePool().FindMessageTypeByName(kwtype_name);
    if (res == NULL) {
        Stroka msg = "Unknown kw-type \"" + kwtype_name + "\" found.";
        yyerror(msg.c_str());
        return NULL;
    }
    return res;
}

// mapping EPartOfSpeech -> TGrammar bitset
const TGramBitSet EPartOfSpeech2TGramBitSet[POSCount] =
{
      TGramBitSet(gSubstantive),
      TGramBitSet(gAdjective),
      TGramBitSet(gVerb),
      TGramBitSet(gSubstPronoun),
      TGramBitSet(gAdjPronoun),
      TGramBitSet(),              //PronounPredk = 5,
      TGramBitSet(gNumeral),
      TGramBitSet(gAdjNumeral),
      TGramBitSet(gAdverb),
      TGramBitSet(gPraedic),
      TGramBitSet(gPreposition),
      TGramBitSet(),              //POSL = 11,
      TGramBitSet(gConjunction),
      TGramBitSet(gInterjunction),
      TGramBitSet(gParenth),
      TGramBitSet(),              //_DUMMY = 15,
      TGramBitSet(),              //Phrase = 16,
      TGramBitSet(gParticle),
      TGramBitSet(),              //UnknownPOS = 18,
      TGramBitSet(),              //Noun_g  = 19,
      TGramBitSet(),              //Adj_g = 20,
      TGramBitSet(),              //Noun_n  = 21,
      TGramBitSet(gAdjective, gShort),
      TGramBitSet(gParticiple),
      TGramBitSet(gGerund),
      TGramBitSet(gParticiple, gShort),
      TGramBitSet(gInfinitive),
      TGramBitSet(),              //Noun_o  = 27,
      TGramBitSet(),              //Adj_o = 28,
      TGramBitSet(gComparative),
      TGramBitSet(gAdjective, gPossessive),
      TGramBitSet(gSubstPronoun),                   //Substitution for PersonalPronoun
      TGramBitSet(gSubConj),
      TGramBitSet(gSimConj),
      TGramBitSet(gSubConj),          //Substitution for AdvConj
      TGramBitSet(gPronounConj),
      TGramBitSet(),              //Number = 36,
      TGramBitSet(gImperative),
      TGramBitSet(gCorrelateConj),
      TGramBitSet(gSimConj),          //Substitution for SimConjAnd
      TGramBitSet(gAuxVerb)
};

TGramBitSet articles_parser_t::ParsePOS(char* strPOS, const char* field)
{
    TGrammar grammem = TGrammarIndex::GetCode(strPOS);
    if (TGramBitSet::IsValid(grammem)) {
        delete strPOS;
        return TGramBitSet(grammem);
    }
    // otherwise grammem name was not recognized by Yandex morph, so try afmorphrus names

    Stroka strPOS_copy(strPOS);  //preserve a copy of strPOS for message later (in case of error)
    // attention: strPOS will be deleted after next statement!
    EPartOfSpeech pos = convert_field_value(POSCount, strPOS, field, POSCount, (const char*)rPartOfSpeeches);

    TGramBitSet res;
    if (pos < POSCount)
        res = EPartOfSpeech2TGramBitSet[pos];

    if (res.none()) {
        Stroka msg = Stroka("Field \"") + Stroka(field) + Stroka("\" has wrong POS value ") + strPOS_copy;
        yyerror(msg.c_str());
    }
    return res;
}

TGrammar articles_parser_t::ParseGrammem(char* strGrammem, const char* field)
{
    TGrammar res = TGrammarIndex::GetCode(strGrammem);
    if (!TGramBitSet::IsValid(res)) {
        // grammem name was not recognized by Yandex morph, try afmorphrus names
        EGrammems grammem = Str2Grammem(strGrammem);
        if (grammem < GrammemsCount)
            res = EGrammem2TGrammar[grammem];

        //check once again
        if (!TGramBitSet::IsValid(res)) {
            Stroka msg = "articles_parser_t::ParseGrammem: Field \"" + Stroka(field) + "\" has wrong grammem value " + Stroka(strGrammem);
            yyerror(msg.c_str());
            res = gInvalid;
        }
    }
    delete strGrammem;
    return res;
}

SArtFileAddress articles_parser_t::GetArtFileAddress(const Wtroka& title) const
{
    SArtFileAddress res;
    ymap<Wtroka, SArtFileAddress>::const_iterator  it;
    it = m_mapArtTitle2FileAddress.find(title);
    if (it != m_mapArtTitle2FileAddress.end())
        res = it->second;
    return res;
}

void articles_parser_t::yyerror(const char *a, const article_t* pArt)
{
    SArtFileAddress ptr = GetArtFileAddress(pArt->get_title());
    if (!ptr.IsValid())
        yyerror(a, false);
    else
        yyerror(a, ptr.m_strFileName.c_str() , ptr.m_iLineNo);
}

void articles_parser_t::yyerror(const char *a, const char* sFile, int line)
{
    Cerr << "Error at " << sFile << "(" << line << "): " << a << Endl;
}

void articles_parser_t::yyerror(const char *a, bool prit_line_no)
{
    if (prit_line_no)
        yyerror(a, m_strCurFile.c_str(), yylineno);
    else
        yyerror(a, m_strCurFile.c_str(), 0);
}

bool    articles_parser_t::add_article(article_t* pArt)
{
    if (!m_dict.add_article(pArt))
        return false;
    m_mapArtTitle2FileAddress[pArt->get_title()] = SArtFileAddress(m_iCurArticleTitleLineNo, m_strCurFile);
    return true;
}

int articles_parser_t::GetToken (void* valp)
{
    int lex_res = yylex();
    YYSTYPE* arg  = (YYSTYPE*)valp;
    switch (lex_res) {
        case VALENCIE_NAME_KW:
        case RUS_WORD:
        case FILE_NAME:
        case DIR_KW:
        case NUMBER_VALUE:
        case FIELD_VALUE:
        case FACT_FIELD_NAME_KW:
        case VAR_FACT_VAL:
        case TRUE_OR_FALSE_KW:
        {
            arg->label = new char[strlen(YYText()) + 1];
            strcpy(arg->label, YYText());
            break;
        }
    }
    return lex_res;
}

void articles_parser_t::put_dic_folder(const Stroka& ss)
{
    m_strDicFolder = ss;
    PathHelper::AppendSlashInplace(m_strDicFolder);
}

//заполняем m_contents_str из файла, указанного в поле СОСТАВ
bool articles_parser_t::fill_contents_from_file()
{
    bool ok = true;
    for (size_t i = 0; i < m_dict.size(); ++i) {
        article_t* art = m_dict[i];
        if (art->m_file_with_words.empty())
            continue;

        Stroka fname = PathHelper::Join(m_strDicFolder, art->m_file_with_words);
        if (!PathHelper::Exists(fname)) {
            yyerror(Substitute("Can't open file \"$0\" in article \"$1\"", fname, NStr::DebugEncode(m_dict[i]->m_title)).c_str(), m_dict[i]);
            ok = false;
            continue;
        }

        TIFStream file(fname);
        Stroka line;
        art->m_contents_str.clear();
        while (file.ReadLine(line)) {
            Wtroka tmp = NStr::DecodeAuxDic(StripString(line));
            tmp.to_lower();
            art->m_contents_str.push_back(tmp);
        }
    }
    return ok;
}

//делаем полноценные статьи ис отсылочных (с полем ОТС),
//а также восстанавливаем ссылки валентностей
bool    articles_parser_t::fill_resend_articles()
{
    bool bRes = true;
    for (size_t i = 0; i < m_dict.size(); ++i) {
        if (!m_dict[i]->m_resend_article.empty()) {
            article_t* art = m_dict[m_dict[i]->m_resend_article];
            if (!art) {
                bRes = false;
                Stroka msg = Substitute("Bad value in field \"ОТС\" in article \"$0\"", NStr::DebugEncode(m_dict[i]->m_title));
                yyerror(msg.c_str(), m_dict[i]);
                //удалим эту дурацкую статью из map
                m_dict.erase(m_dict[i]->m_title);
            } else
                m_dict[i]->copy_resend_article_info(*art);
        }
        if (!fill_resend_valencies(m_dict[i]))
            bRes = false;
    }
    return bRes;
}

bool articles_parser_t::fill_resend_valencies(article_t* p_art)
{
    bool bRes = true;
    for (size_t i = 0; i < p_art->m_subordinations.size(); ++i) {
        if (p_art->m_subordinations[i].m_reference.is_valid_for_subbord()) {
            article_t* art = m_dict[p_art->m_subordinations[i].m_reference.m_str_article_title];
            if (!art) {
                Stroka msg = Substitute("Bad article reference in field \"$0\" in article \"$1\"",
                                        NStr::DebugEncode(p_art->m_subordinations[i].m_str_subbord_name),
                                        NStr::DebugEncode(p_art->m_title));
                yyerror(msg.c_str(), p_art);
                bRes = false;
                continue;
            }

            if (!art->get_subordination(p_art->m_subordinations[i], p_art->m_subordinations[i].m_reference.m_str_subbord_name)) {
                bRes = false;
                Stroka msg = Substitute("Bad subordination reference in field \"$0\" in article \"$1\"",
                                        NStr::DebugEncode(p_art->m_subordinations[i].m_str_subbord_name),
                                        NStr::DebugEncode(p_art->m_title));
                yyerror(msg.c_str(), p_art);
            }
        }

        for (size_t j = 0; j < p_art->m_subordinations[i].size(); ++j) {
            or_valencie_list_t&  val = p_art->m_subordinations[i][j];
            if (!val.m_reference.is_valid_for_valency())
                continue;
            article_t* art = m_dict[val.m_reference.m_str_article_title];

            if (!art) {
                Stroka msg = Substitute("Bad article reference in field \"$0.$1\" in article \"$2\"",
                                        NStr::DebugEncode(p_art->m_subordinations[i].m_str_subbord_name),
                                        NStr::DebugEncode(val.m_str_name), NStr::DebugEncode(p_art->m_title));
                yyerror(msg.c_str(), p_art);
                bRes = false;
                continue;
            }

            if (!art->get_valency_by_ref(val, val.m_reference)) {
                bRes = false;
                Stroka msg = Substitute("Bad valency reference in field \"$0.$1\" in article \"$2\"",
                                        NStr::DebugEncode(p_art->m_subordinations[i].m_str_subbord_name),
                                        NStr::DebugEncode(val.m_str_name), NStr::DebugEncode(p_art->m_title));
                yyerror(msg.c_str(), p_art);
            }
        }
    }
    return bRes;
}

bool    articles_parser_t::check_interpretations()
{
    bool bHasError = false;
    for (size_t i = 0; i < m_dict.size(); ++i) {
        article_t* art = m_dict[i];
        for (size_t j = 0; j < art->m_subordinations.size(); j++)
            for (size_t k = 0; k < art->m_subordinations[j].size(); k++) {

                or_valencie_list_t& valency = art->m_subordinations[j][k];

                for (size_t m = 0; m < valency.m_interpretations.size(); m++) {
                    fact_field_reference_t& fact_field_reference = valency.m_interpretations[m];
                    const fact_type_t* fact_type = m_dict.get_fact_type(fact_field_reference.m_strFactTypeName);
                    if (fact_type == NULL && !valency.m_reference.is_valid_for_valency()) {
                        TStringStream s;
                        s << "Article \"" << art->m_title << "\" has valency \"" << valency.m_str_name << "\" with bad interpretaion, " << " fact_type \"" << fact_field_reference.m_strFactTypeName << "\" is not found! " << Endl;
                        yyerror(s.c_str(), art);
                        bHasError = true;
                    } else {
                        const fact_field_descr_t* field = fact_type->get_field(fact_field_reference.m_strFieldName);
                        if (field == NULL && !valency.m_reference.is_valid_for_valency()) {
                            TStringStream s;
                            s << "Article \"" << art->m_title << "\" has valency \"" << valency.m_str_name << "\" with bad interpretaion, " << " field " << fact_field_reference.m_strFactTypeName << "." << fact_field_reference.m_strFieldName << " is not found! " << Endl;
                            yyerror(s.c_str(), art);
                            bHasError = true;
                        } else
                            fact_field_reference.m_Field_type = field->m_Field_type;

                    }

                }
            }
    }

    return !bHasError;
}

bool articles_parser_t::check_references()
{
    bool bHasError = false;
    for (size_t i = 0; i < m_dict.size(); ++i) {
        article_t* art = m_dict[i];
        bHasError = bHasError || !check_valency_references(art);
        bHasError = bHasError || !m_dict.check_geo_references(art);
        bHasError = bHasError || !m_dict.check_goal_references(art);
        bHasError = bHasError || !m_dict.check_s0_references(art);

    }
    for (size_t i = 0; i < m_dict.size(); ++i) {
        bHasError = bHasError || !check_geo_cycles(m_dict[i]);
    }
    return !bHasError;
}

bool    articles_parser_t::check_geo_cycles(const article_t* art)
{
    yset<const article_t*> arts;
    arts.insert(art);
    while (art->m_pGeoPartArtRef != 0) {
        art = art->m_pGeoPartArtRef;
        if (arts.find(art) != arts.end()) {
            TStringStream s;
            s << "Article \"" << art->m_title << "\" has cycled field  \"ГЕО_ЧАСТЬ\"! "<< Endl;
            yyerror(s.c_str(), art);
            return false;
        }
        arts.insert(art);
    }

    arts.clear();
    arts.insert(art);
    while (art->m_pGeoCentrRef != 0) {
        art = art->m_pGeoCentrRef;
        if (arts.find(art) != arts.end()) {
            TStringStream s;
            s << "Article \"" << art->m_title << "\" has cycled field  \"ЦЕНТР\"! "<< Endl;
            yyerror(s.c_str(), art);
            return false;
        }
        arts.insert(art);
    }
    return true;
}

bool articles_parser_t::check_title_reference(const Wtroka& title, article_t* art, const Wtroka& valency_name)
{
    if (!title.empty() && !(m_dict.has_article(title) || m_dict.has_external_article(title))) {
        yyerror(Substitute("Article \"$0\" has valency \"$1\" with undefined reference: \"$2\".",
                           NStr::DebugEncode(art->m_title), NStr::DebugEncode(valency_name), NStr::DebugEncode(title)).c_str(), art);
        return false;
    }
    return true;
}

bool    articles_parser_t::check_valency_references(article_t* art)
{
    bool bHasError = false;
    for (size_t j = 0; j < art->m_subordinations.size(); j++)
        for (size_t k = 0; k < art->m_subordinations[j].size(); k++) {

            for (size_t l = 0; l < art->m_subordinations[j][k].size(); l++) {
                valencie_t& val = art->m_subordinations[j][k][l];
                const Wtroka& valency_name = art->m_subordinations[j][k].m_str_name;

                if (!check_title_reference(val.m_prefix.m_title, art, valency_name))
                    bHasError = true;
                if (!check_title_reference(val.m_ant.m_title, art, valency_name))
                    bHasError = true;
                if (!check_title_reference(val.m_str_title, art, valency_name))
                    bHasError = true;
            }
        }
    return !bHasError;
}

bool articles_parser_t::process_one_file(const Stroka& path)
{
    std::ifstream file;
    file.open(path.c_str());
    if (!file.good())
        return false;
    if (!process_from_stream(&file))
        return false;
    file.close();
    return true;
}

bool articles_parser_t::process_from_stream(std::istream *stream)
{
    switch_streams(stream, 0);
    if (0 == yyparse(this))
        return false;
    return true;
}

bool    articles_parser_t::check_agreements()
{
    for (size_t i = 0; i < m_dict.size(); ++i) {
        article_t* art = m_dict[i];
        int count = 0;
        bool bEqualSize = true;
        for (int k = 0; k < art->get_contents_count(); k++) {
            const Wtroka& content_str = art->get_content(k);
            int ii = 0;
            StringTokenizer<Wtroka> tokenizer(content_str, ' ');
            while (tokenizer.SkipNext())
                ii++;

            if (k == 0)
                count = ii;
            else if (count != ii) {
                    bEqualSize = false;
                    break;
                }
        }

        if (count > 0 && art->m_iMainWord >= count) {
            yyerror("Bad value for \"ГС\"", art);
            return false;
        }

        if (art->m_Lemma.m_Lemmas.size() > 0 && art->m_Lemma.m_iMainWord >= (int)art->m_Lemma.m_Lemmas.size()) {
            yyerror("Bad value for \"ЛЕММА.ГС\"", art);
            return false;
        }

        if (art->m_agreements.size() == 0)
            continue;

        for (size_t j = 0; j < art->m_agreements.size(); ++j) {
            if (!bEqualSize)
                return false;
            SAgreement& agr = art->m_agreements[j];
            if (!(agr.m_iW1 < count && agr.m_iW2 < count)) {
                Stroka msg = Substitute("Bad word index in agreement function \"$0\"", CoordFuncsStr[agr.m_agr]);
                yyerror(msg.c_str(), art);
                return false;
            }
        }
    }
    return true;
}

bool    articles_parser_t::process(const Stroka& strFile, const Stroka& strText)
{
    if (strText.length() > 0) {
        std::istringstream stream;
        stream.str(strText);
        process_from_stream(&stream);
    } else {
        Stroka dirName = PathHelper::DirNameWithSlash(::RealPath(strFile));

        m_included_files.push(strFile);

        while (!m_included_files.empty()) {
            m_strCurFile = m_included_files.top();
            m_included_files.pop();

            if (!isexist(m_strCurFile.c_str())) {
                Cerr << "File \"" << m_strCurFile << "\" is not found." << Endl;
                return false;
            }

            if (!process_one_file(m_strCurFile)) {
                Cerr << "Error processing file \"" << m_strCurFile << "\"." << Endl;
                return false;
            }
            yylineno = 1;

            Stroka realFile = ::RealPath(m_strCurFile);
            if (realFile.has_prefix(dirName))
                realFile = realFile.substr(dirName.size());
            m_dict.SourceFiles[realFile] = NUtil::CalculateFileChecksum(m_strCurFile);
        }
    }

    if (!fill_contents_from_file())
        return false;
    if (!check_interpretations())
        return false;
    if (!fill_resend_articles())
        return false;
    if (!check_agreements())
        return false;
    return check_references();
}
