%pure-parser 
%{

#define yyFlexLexer axdFlexLexer
#include <contrib/tools/flex-old/FlexLexer.h>

#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/gramstr.h>
#include <FactExtract/Parser/common/string_tokenizer.h>
#include <FactExtract/Parser/auxdictlib/articles_parser.h>
#include <FactExtract/Parser/auxdictlib/dictionary.h>

#include <library/lemmer/dictlib/grammarhelper.h>

//#define YYDEBUG 1
//#define YYERROR_VERBOSE

void yyerror(articles_parser_t* toker, const char* str) {
    toker->yyerror(str);
}

inline int yylex(void *lval, articles_parser_t *toker) {
    YASSERT(toker);
    return (toker->GetToken(lval));
}

%}

%lex-param { articles_parser_t* parm }
%parse-param { articles_parser_t* parm }

%union
{
long     num;
char*    label;
TGramBitSet* grammems;
simple_list< TGramBitSet* >* grammems_list;
simple_list<valencie_t>* or_valencies;
simple_list<or_valencie_list_t>* all_vals;
or_valencie_list_t* full_val;
simple_list<Stroka>*    strings;
simple_list<fact_field_descr_t>*    fact_field_descr_list;
simple_list<EPosition>*    positions;
simple_list<yvector<Stroka> >* content;
valencie_t*        valency;
fact_field_descr_t* fact_field_descr;
article_t*        art;
single_actant_contion_t* single_actant_contion;
simple_list<single_actant_contion_t>* list_actant_contions;
Stroka* long_string;
reference_to_wordchain_t* ref_to_wordchain;
fact_field_reference_t* fact_type_reference_type;
simple_list<fact_field_reference_t>* list_of_fact_type_references_type;
simple_list<actant_contions_t>* actant_contions;
SAgreement* agr_type;
simple_list<SAgreement>* agr_type_list;
}

%token<num>    TITLE_KW INCLUDE_KW CONTENT_KW FILE_KW TOMITA_KW  GEO_ALG_KW GEO_PART_KW GOAL_KW GEO_CENTER_KW POPULATION_KW Y_GEO_ID LEMMA_KW ALG_KW POS_KW TYPE_KW_KW MAIN_WORD_KW NEW_POS_KW DEL_HOM_KW MORPH_I_KW CONJ_POS_KW FR_TYPE_KW COORD_FUNC_KW PREFIX_KW ORDER_KW FOLLOW_KW INCHANGEABLE_KW ALWAYS_KW AMBIG_KW
%token<num>     BOOL_TYPE_KW FIO_TYPE_KW DATE_TYPE_KW TEXT_TYPE_KW FACT_TYPE_KW FACT_TYPE_TEMP_KW FACT_TYPE_NOLEAD_KW
%token<label>    FIELD_VALUE NUMBER_VALUE RUS_WORD FILE_NAME VALENCIE_NAME_KW DIR_KW FACT_FIELD_NAME_KW TRUE_OR_FALSE_KW FACT_INTERP_KW FROM_OPERATOR VAR_FACT_VAL
%token<num>     DIR_SINGLE_KW POSIT_KW SYN_R_KW CONJ_DIR_KW SUP_KW TYPE_KW 
%token<num>    CONJ_KW RESEND_KW FR_DIR_KW MODAL_KW END_COMMA_KW COMP_BY_LEMMA_KW CORR_KW ANT_KW COMMA_KW LIGHT_KW S0_KW



%type<num> error
%type<num> list_of_articles  
%type<art> article
%type<label> default_type_field_value
%type<num> top
%type<grammems> morph_val 
%type<num> title_nt
%type<grammems_list> morph_info pos_field_values 
%type<valency> simple_valencie
%type<full_val> valencie
%type<full_val> valencie_reference
%type<fact_field_descr> type_and_name type_and_name_and_value type_field
%type<fact_field_descr_list> list_of_type_field
%type<all_vals> list_of_valencies  
%type<list_actant_contions> val_order_condition
%type<ref_to_wordchain> reference_to_words_chain
%type<or_valencies>     or_list_of_valencies
%type<num>    list_of_dir_fields fact_type_descr   fact_type_descr_end
%type<strings> or_content_words  content_word field_values_comma val_order_condition_single fact_field_options
%type<long_string> rus_words_with_space
%type<content> content_words
%type<single_actant_contion> val_order_condition_single_full
%type<agr_type_list> list_of_coord
%type<agr_type> coord
%type<fact_type_reference_type> fact_type_reference
%type<fact_type_reference_type> fact_field_address
%type<list_of_fact_type_references_type> list_of_fact_type_references  interpretation
%type<actant_contions> or_list_of_val_order
%type<num>    dir_field
%type<grammems> morph_field_values_comma 
%type<positions> positions_field_values_comm
%left<num> '(' 
%left<num> ')'
%left<num> '|'


%%
//-----------------------------------------------------------------------------------------------------------------
//                RULES PART
//-----------------------------------------------------------------------------------------------------------------

top:                list_of_articles { return 1; }
                    |    list_of_articles error  { return 0; }
                    |    error { return 0; }
                ;    

list_of_articles:         list_of_articles article 
                        {
                            if( !parm->add_article($2) )
                            {
                                char msg[500];
                                sprintf(msg, "Not unique value %s in field \"ЗГЛ\" ", ~NStr::DebugEncode($2->m_title));
                                yyerror(parm, msg);
                            }

                        }

                    |    list_of_articles include_file
                        {
                            
                        }
                    |    list_of_articles fact_type_descr
                        {
                            
                        }
                    |    list_of_articles error
                        {
                            parm->m_error = true;
                        }
                    |    include_file
                        {
                            
                        }
                    |    fact_type_descr
                        {
                            
                        }
                    |    article
                            {
                                if( !parm->add_article($1) )
                                {
                                    char msg[500];
                                    sprintf(msg, "Not unique value %s in field \"ЗГЛ\" ", ~NStr::DebugEncode($1->m_title));
                                    yyerror(parm, msg);
                                }

                            }
                ;

fact_type_descr: FACT_TYPE_KW    FACT_FIELD_NAME_KW '{' list_of_type_field fact_type_descr_end
                {
                    fact_type_t fact_type;
                    fact_type.m_strFactTypeName = $2;
                    fill_vector_and_free(fact_type.m_Fields, $4);
                    std::reverse(fact_type.m_Fields.begin(), fact_type.m_Fields.end());
                    parm->m_dict.m_FactTypes.push_back(fact_type);                    
                }
                |
                FACT_TYPE_NOLEAD_KW    FACT_FIELD_NAME_KW '{' list_of_type_field fact_type_descr_end
                {
                    fact_type_t fact_type;
                    fact_type.m_strFactTypeName = $2;
                    fill_vector_and_free(fact_type.m_Fields, $4);
                    std::reverse(fact_type.m_Fields.begin(), fact_type.m_Fields.end());
                    fact_type.m_bNoLead = true;
                    parm->m_dict.m_FactTypes.push_back(fact_type);                    
                }
                |
                FACT_TYPE_TEMP_KW    FACT_FIELD_NAME_KW '{' list_of_type_field fact_type_descr_end
                {
                    fact_type_t fact_type;
                    fact_type.m_strFactTypeName = $2;
                    fill_vector_and_free(fact_type.m_Fields, $4);
                    fact_type.m_bTemporary = true;
                    parm->m_dict.m_FactTypes.push_back(fact_type);                    
                }                
                ;
                
type_and_name: FACT_FIELD_NAME_KW FACT_FIELD_NAME_KW 
                {
                    int t = IsInList_i((const char*)FactFieldTypeStr,FactFieldTypesCount, $1);
                    
                    if( t == -1 )
                    {
                        yyerror(parm, "Field has wrong type.");                        
                    }
                        
                    $$ = new fact_field_descr_t($2, (EFactFieldType)t);
                    delete $1;
                    delete $2;                
                }
                | '~' FACT_FIELD_NAME_KW FACT_FIELD_NAME_KW 
                {
                    int t = IsInList_i((const char*)FactFieldTypeStr,FactFieldTypesCount, $2);
                    
                    if( t == -1 )
                    {
                        yyerror(parm, "Field has wrong type.");                        
                    }
                        
                    $$ = new fact_field_descr_t($3, (EFactFieldType)t);
                    $$->m_bNecessary = false;
                    delete $2;
                    delete $3;                
                }
                ;
        
type_field: type_and_name_and_value ';'
            {
                $$ = $1;
            }
            |
            type_and_name_and_value '[' fact_field_options ']' ';'
            {
                $$ = $1;
                yvector<Stroka> v;
                fill_vector_and_free(v, $3);
                for (size_t j = 0; j < v.size(); ++j)
                {
                    if( v[j] == "info"  )
                    {                        
                        $$->m_bSaveTextInfo = true;
                    }                
                    else if( v[j] == "h-reg1" )
                    {
                        $$->m_bCapitalizeFirstLetter1 = true;
                    }
                    else if( v[j] == "h-reg2" )
                    {
                        $$->m_bCapitalizeAll = true;
                    }
                    else if( v[j] == "h-reg3" )
                    {
                        $$->m_bCapitalizeFirstLetter2 = true;
                    }
                    else
                        $$->m_options.insert(v[j]);
                }
            }            
            
            
type_and_name_and_value:    type_and_name 
            {
                $$ = $1;
            }             
            | type_and_name '=' TRUE_OR_FALSE_KW 
            {
                
                Stroka s = $3;                
                delete $3;
                                
                if( s == "true" )
                    $1->m_bBoolValue = true;
                else
                    $1->m_bBoolValue = false;                
                    
                $$ = $1;
            }
            | type_and_name '=' VAR_FACT_VAL
            {
                $1->m_StringValue = NStr::DecodeAuxDic($3);
                delete $3;
                $$ = $1;
            }
            | type_and_name '=' default_type_field_value 
            {                
                $1->m_StringValue = NStr::DecodeAuxDic($3);
                delete $3;
                $$ = $1;
            }            
            ;
            
default_type_field_value: '\'' FIELD_VALUE '\''
                        {
                            $$ = $2;
                        }
                        | '\"' FIELD_VALUE '\"'
                        {
                            $$ = $2;
                        }
                        ;
                
list_of_type_field: list_of_type_field type_field
                    {
                        $$ = $1->add_new(*$2);
                        delete $2;                    
                    }
                |    type_field
                    {
                        simple_list<fact_field_descr_t>* el = new simple_list<fact_field_descr_t>;
                        el->next = NULL;
                        el->info = *$1;
                        delete $1;
                        $$ = el;
                    }
                    ;
                    

fact_type_descr_end :        '}' ';'
                        {
                        }
                        |    '}' 
                        {
                        }
                        ;
                
include_file:            INCLUDE_KW '\"'    FIELD_VALUE '\"'
                        {
                            parm->m_included_files.push(parm->m_strDicFolder + $3);    
                            delete $3;
                        }
                        ;
                        
title_nt:                TITLE_KW
                        {
                            parm->m_iCurArticleTitleLineNo = parm->lineno() + 1;
                        }
                        ;

article:                title_nt '=' FIELD_VALUE '{' list_of_fields '}'
                        {                             
                               parm->m_cur_article.m_title = NStr::DecodeAuxDic($3);
                               delete $3;
                            $$ = new article_t(parm->m_cur_article);
                            parm->m_cur_article.reset();
                        }
                        |
                        title_nt '=' FIELD_VALUE '{'  '}'
                        {         
                            
                            
                               parm->m_cur_article.m_title = NStr::DecodeAuxDic($3);
                               delete $3;
                            $$ = new article_t(parm->m_cur_article);
                            parm->m_cur_article.reset();
                        }                        
                ;

list_of_fields:            article_field list_of_fields
                |        article_field 
                ;


article_field:            CONTENT_KW '=' TOMITA_KW ':' FILE_NAME
                        {                            
                            parm->m_cur_article.m_file_with_grammar = $5;
                            delete $5;
                        }        
                |        CONTENT_KW '=' ALG_KW ':' FIELD_VALUE
                        {
                            parm->m_cur_article.m_str_alg = $5;
                            delete $5;
                        }        
                |        CONTENT_KW '=' FILE_KW ':' FILE_NAME
                        {
                            parm->m_cur_article.m_file_with_words = $5;
                            delete $5;
                        }        
                                                
                |    FR_TYPE_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_clause_type = parm->convert_field_value(parm->m_cur_article.m_clause_type, $3, "ТИП_ФР", ClauseTypesCount, (const char*)ClauseTypeNames);
                        }
                        
                        
                |        GEO_PART_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_strGeoArtRef = NStr::DecodeAuxDic($3);
                            delete $3;
                        }        
                        
                |        GOAL_KW '=' field_values_comma
                        {
                            fill_str_vector_and_free(parm->m_cur_article.m_GoalArticles, $3);
                        }        
                        
                        
                |        GEO_CENTER_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_strGeoCentreRef = NStr::DecodeAuxDic($3);
                            delete $3;
                        }                                
                |        Y_GEO_ID '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_iYGeoID = ::FromString<int>($3);
                            delete $3;
                            if (parm->m_cur_article.m_iYGeoID == 0)
                                yyerror(parm, "Field \"НАСЕЛЕНИЕ\" has wrong value.");                        
                        } 
                        
                |        POPULATION_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_iPopulation = ::FromString<int>($3);                            
                            delete $3;
                            if( parm->m_cur_article.m_iPopulation == 0 )
                                yyerror(parm, "Field \"НАСЕЛЕНИЕ\" has wrong value.");                        
                        }
                        
                /*|        GEO_ALG_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_GeoAlg = convert_field_value(parm->m_cur_article.m_GeoAlg, $3, "ГЕО_АЛГ", GeoCount, (const char*)GeoAlgStr);
                        }                        
                */
                        
                |        LEMMA_KW'.'ALWAYS_KW '=' FIELD_VALUE                        
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $5);
                            delete $5;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"ЛЕММА.ВСЕГДА\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_Lemma.m_bAlwaysReplace = (res == 1);
                        
                        }                                            
                        
                |        LEMMA_KW'.'INCHANGEABLE_KW '=' FIELD_VALUE                        
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $5);
                            delete $5;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"ЛЕММА.НЕИЗМ\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_Lemma.m_bIndeclinable = (res == 1);
                        
                        }            
                        
                |       S0_KW '=' field_values_comma
                        {
                            fill_str_vector_and_free(parm->m_cur_article.m_s0, $3);
                        }
                                                        
                        
                |        LEMMA_KW'.'MAIN_WORD_KW '=' FIELD_VALUE
                        {
                            if( strspn($5, "1234567890") == strlen($5) )
                            {
                                int ii = ::FromString<int>($5) - 1;
                                if( ii < 0 )
                                    yyerror(parm, "Field \"ЛЕММА.ГС\" has wrong value.");                                    
                                parm->m_cur_article.m_Lemma.m_iMainWord = ii;
                            }
                            else
                                yyerror(parm, "Field \"ЛЕММА.ГС\" has wrong value.");                                    
                            delete $5;                        
                        }                                                
                        
                |        LEMMA_KW '=' FIELD_VALUE
                        {                            
                            StringTokenizer<Stroka> tokenizer($3," _\t");
                            TStringBuf tok;
                            while (tokenizer.NextToken(tok))                            
                                parm->m_cur_article.m_Lemma.m_Lemmas.push_back(NStr::DecodeAuxDic(ToString(tok)));
                            delete $3;
                        }
                        
                |        LEMMA_KW '=' '\"' FIELD_VALUE '\"'
                        {                            
                            StringTokenizer<Stroka>    tokenizer($4," _\t");
                            TStringBuf tok;
                            while (tokenizer.NextToken(tok))
                                parm->m_cur_article.m_Lemma.m_Lemmas.push_back(NStr::DecodeAuxDic(ToString(tok)));
                            delete $4;
                        }
                |        LEMMA_KW '=' '\'' FIELD_VALUE '\''
                        {                            
                            StringTokenizer<Stroka> tokenizer($4," _\t");
                            TStringBuf tok;
                            while (tokenizer.NextToken(tok))
                                parm->m_cur_article.m_Lemma.m_Lemmas.push_back(NStr::DecodeAuxDic(ToString(tok)));
                            delete $4;
                        }
                |        CONTENT_KW '=' content_words
                        {
                            fill_str_vector_and_free(parm->m_cur_article.m_content, $3);
                        }        
                        
                |        MAIN_WORD_KW '=' FIELD_VALUE
                        {
                            if( strspn($3, "1234567890") == strlen($3) )
                            {
                                int ii = ::FromString<int>($3) - 1;
                                if( ii < 0 )
                                    yyerror(parm, "Field \"ГС\" has wrong value.");                                    
                                parm->m_cur_article.m_iMainWord = ii;
                            }
                            else
                                yyerror(parm, "Field \"ГС\" has wrong value.");                                    
                            delete $3;
                        }
                        
                |       TYPE_KW_KW '=' FIELD_VALUE
                        {
                            //parm->m_cur_article.m_KWType = parm->convert_field_value(parm->m_cur_article.m_KWType, $3, "ТИП_КС", EKWTypeCount, (const char*)KWTypwStr);
                            parm->m_cur_article.m_KWType = parm->ParseKWType($3);
                        }
        
                |        MORPH_I_KW '=' morph_info
                        {
                            fill_set_and_free(parm->m_cur_article.m_morph_scale, $3);
                        }

                |        NEW_POS_KW '=' FIELD_VALUE
                        {
                            //parm->m_cur_article.m_newPOS = parm->convert_field_value(parm->m_cur_article.m_newPOS, $3, "Н_ЧР", POSCount, (const char*)rPartOfSpeeches);
                            parm->m_cur_article.m_newPOS = parm->ParsePOS($3, "Н_ЧР");
                        }
                |        POS_KW '=' FIELD_VALUE
                        {                    
                            //parm->m_cur_article.m_POS = convert_field_value(parm->m_cur_article.m_POS, $3, "ЧР", POSCount, (const char*)rPartOfSpeeches);
                            parm->m_cur_article.m_POS = parm->ParsePOS($3, "ЧР");
                        }

                |        DEL_HOM_KW '=' FIELD_VALUE
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $3);
                            delete $3;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"УД_ОМОНИМ\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_del_hom = (res == 1);
                        }

                |        COMMA_KW '=' FIELD_VALUE
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $3);
                            delete $3;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"ЗПТ\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_before_comma = (res == 1);
                        }


                |        POSIT_KW '=' positions_field_values_comm                
                        {                            
                            fill_vector_and_free(parm->m_cur_article.m_positions, $3);                            
                        }
                        
                |        DIR_SINGLE_KW '=' list_of_valencies
                        {                            
                            if( parm->m_cur_article.m_single_valencies.size() > 0 )
                            {
                                yyerror(parm, "Field \"УПР\" can have only one value.");                        
                            }
                            fill_vector_and_free(parm->m_cur_article.m_single_valencies, $3);
                        }                                                

                |        DIR_KW '=' list_of_valencies
                        {
                            valencie_list_t  val;
                            fill_vector_and_free(val, $3);                            
                            val.m_str_subbord_name = NStr::DecodeAuxDic($1);
                            delete $1;
                            parm->m_cur_article.m_subordinations.push_back(val); // = convert_to_vector($3);
                        }
                        
                |        DIR_KW ':' FIELD_VALUE '.' DIR_KW ORDER_KW '(' val_order_condition ')'
                        {
                            valencie_list_t  val;
                            val.m_str_subbord_name = NStr::DecodeAuxDic($1);
                            delete $1;                            
                            val.m_reference.m_str_article_title = NStr::DecodeAuxDic($3);
                            delete $3;
                            val.m_reference.m_str_subbord_name = NStr::DecodeAuxDic($5);
                            delete $5;
                            actant_contions_t actant_contions;
                            fill_vector_and_free(actant_contions.m_conditions, $8);
                            val.m_actant_condions.push_back(actant_contions);                            
                            parm->m_cur_article.m_subordinations.push_back(val);
                            
                        }
                        
                |        DIR_KW ':' FIELD_VALUE '.' DIR_KW ORDER_KW '(' or_list_of_val_order ')'
                        {
                            valencie_list_t  val;
                            val.m_str_subbord_name = NStr::DecodeAuxDic($1);
                            delete $1;                            
                            val.m_reference.m_str_article_title = NStr::DecodeAuxDic($3);
                            delete $3;
                            val.m_reference.m_str_subbord_name = NStr::DecodeAuxDic($5);
                            delete $5;
                            fill_vector_and_free(val.m_actant_condions, $8);
                            parm->m_cur_article.m_subordinations.push_back(val);
                            
                        }
                |        DIR_KW ':' FIELD_VALUE '.' DIR_KW ORDER_KW 
                        {
                            valencie_list_t  val;
                            val.m_str_subbord_name = NStr::DecodeAuxDic($1);
                            delete $1;                            
                            val.m_reference.m_str_article_title = NStr::DecodeAuxDic($3);
                            delete $3;
                            val.m_reference.m_str_subbord_name = NStr::DecodeAuxDic($5);
                            delete $5;
                            parm->m_cur_article.m_subordinations.push_back(val);
                            
                        }
                |        DIR_KW '=' list_of_valencies ORDER_KW '(' or_list_of_val_order ')'
                        {
                            //parm->m_cur_article.m_valencies = convert_to_vector($3);
                            valencie_list_t  val;
                            fill_vector_and_free(val, $3);
                            val.m_str_subbord_name = NStr::DecodeAuxDic($1);
                            delete $1;                            
                            fill_vector_and_free(val.m_actant_condions, $6);
                            parm->m_cur_article.m_subordinations.push_back(val);    
                        }                        
                |        DIR_KW '=' list_of_valencies ORDER_KW '(' val_order_condition ')'
                        {                            
                            valencie_list_t  val;
                            fill_vector_and_free(val, $3);
                            val.m_str_subbord_name = NStr::DecodeAuxDic($1);
                            delete $1;                            
                            actant_contions_t actant_contions;
                            fill_vector_and_free(actant_contions.m_conditions, $6);
                            val.m_actant_condions.push_back(actant_contions);                            
                            parm->m_cur_article.m_subordinations.push_back(val);
                        }
                        
                |        FR_DIR_KW '=' list_of_valencies
                        {
                            fill_vector_and_free(parm->m_cur_article.m_fr_valencies, $3);                        
                            //parm->m_cur_article.m_fr_valencies = m_cur_subbordination;
                            //m_cur_subbordination.reset();
                        }
                |        CONJ_DIR_KW '='    list_of_valencies
                        {
                            fill_vector_and_free(parm->m_cur_article.m_conj_valencies, $3);
                            //parm->m_cur_article.m_conj_valencies = m_cur_subbordination;
                            //m_cur_subbordination.reset();
                        }
                |        SUP_KW '=' list_of_valencies
                        {
                            fill_vector_and_free(parm->m_cur_article.m_sup_fr_valencies, $3);
                            //parm->m_cur_article.m_sup_fr_valencies = m_cur_subbordination;
                            //m_cur_subbordination.reset();
                        }

                |        RESEND_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_article.m_resend_article = NStr::DecodeAuxDic($3);
                            delete $3;
                        }

                |        CORR_KW '=' field_values_comma
                        {
                            fill_str_vector_and_free(parm->m_cur_article.m_corrs, $3);
                        }

                |        ANT_KW '=' field_values_comma
                        {
                            fill_str_vector_and_free(parm->m_cur_article.m_ant_words, $3);
                        }
                                            
                |        LIGHT_KW '=' FIELD_VALUE
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $3);
                            delete $3;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"КОНЕЦ_ЗПТ\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_is_light_kw = (res == 1);                        
                        }

                |        END_COMMA_KW '=' FIELD_VALUE
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $3);
                            delete $3;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"КОНЕЦ_ЗПТ\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_end_comma = (res == 1);                        
                        }
                |        COMP_BY_LEMMA_KW '=' FIELD_VALUE
                        {
                            int res = IsInList_i((const char*)BoolStrVals,2, $3);
                            delete $3;
                            if( res == -1 )
                            {
                                yyerror(parm, "Field \"СРАВН_ЛЕММА\" has wrong value.");                        
                            }
                            else
                                parm->m_cur_article.m_comp_by_lemma = (res == 1);                        
                        }
                |    COORD_FUNC_KW '=' list_of_coord
                    {
                        fill_vector_and_free(parm->m_cur_article.m_agreements, $3);
                    }
                |        AMBIG_KW
                        {
                            parm->m_cur_article.m_allow_ambiguity = true;
                        }
                ;
                
                
coord:         FIELD_VALUE '(' FIELD_VALUE ',' FIELD_VALUE ')'
        {
            
            ECoordFunc f = parm->convert_field_value(parm->m_cur_valencie.m_coord_func, $1, "СОГЛ", CoordFuncCount, (const char*)CoordFuncsStr);
            int i1 = ::FromString<int>($3);
            int i2 = ::FromString<int>($5);
            delete $3;
            delete $5;
            if( i1 == 0 || i2 == 0 )
                yyerror(parm, "Field \"СОГЛ\" has wrong value.");
            $$ = new SAgreement(f, i1-1, i2-1);
        }
                
list_of_coord:    list_of_coord coord
                {
                    $$ = $1->add_new(*$2);
                    delete $2;                            
                }
                | coord
                {
                    $$ = new simple_list<SAgreement>;
                    $$->next = NULL;
                    $$->info = *$1;
                    delete $1;                                        
                }
                ;

list_of_dir_fields:    list_of_dir_fields  dir_field
                |        dir_field 
                ;

reference_to_words_chain:    TYPE_KW_KW '=' FIELD_VALUE
                            {
                                $$ = new reference_to_wordchain_t();
                                //$$->m_kw_type = convert_field_value($$->m_kw_type, $3, "ТИП_КС", EKWTypeCount, (const char*)KWTypwStr);
                                $$->m_kw_type = parm->ParseKWType($3);
                            }
                        |    TITLE_KW '=' FIELD_VALUE 
                            {
                                $$ = new reference_to_wordchain_t();
                                $$->m_title = NStr::DecodeAuxDic($3);
                                delete $3;
                            }
                        |    CONTENT_KW '=' content_words
                            {
                                $$ = new reference_to_wordchain_t();
                                fill_str_vector_and_free($$->m_content, $3);
                            }
                        ;    


dir_field:                MORPH_I_KW '=' morph_info
                        {
                            fill_set_and_free(parm->m_cur_valencie.m_morph_scale, $3);
                        }
                |        CONTENT_KW '=' content_words
                        {
                            fill_str_vector_and_free(parm->m_cur_valencie.m_content, $3);
                        }                        
                |        ANT_KW ':' reference_to_words_chain
                        {                
                            parm->m_cur_valencie.m_ant = *$3;
                            delete $3;
                        }                                                                        
                |        PREFIX_KW ':' reference_to_words_chain
                        {
                            parm->m_cur_valencie.m_prefix = *$3;
                            delete $3;
                        }
                |        SYN_R_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_valencie.m_syn_rel = parm->convert_field_value(parm->m_cur_valencie.m_syn_rel, $3, "СИН_О", SynRelationCount, (const char*)SynRelationsStr);
                        }
                        
                |        TYPE_KW_KW '=' FIELD_VALUE
                        {
                            //parm->m_cur_valencie.m_kw_type = parm->convert_field_value(parm->m_cur_valencie.m_kw_type, $3, "ТИП_КС", EKWTypeCount, (const char*)KWTypwStr);
                            parm->m_cur_valencie.m_kw_type = parm->ParseKWType($3);
                        }
                        
                |        TITLE_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_valencie.m_str_title = NStr::DecodeAuxDic($3);
                            delete $3;
                        }                        
                |        POSIT_KW '=' FIELD_VALUE                
                        {
                            parm->m_cur_valencie.m_position = parm->convert_field_value(parm->m_cur_valencie.m_position, $3, "ПОЗ", PositionsCount, (const char*)PositionsStr);
                        }
                |        COORD_FUNC_KW '=' FIELD_VALUE                
                        {
                            parm->m_cur_valencie.m_coord_func = parm->convert_field_value(parm->m_cur_valencie.m_coord_func, $3, "СОГЛ", CoordFuncCount, (const char*)CoordFuncsStr);
                        }

                |        POS_KW '=' pos_field_values
                        {
                            fill_set_and_free(parm->m_cur_valencie.m_POSes, $3);                            
                        }
                |        TYPE_KW '=' FIELD_VALUE        
                        {
                            parm->m_cur_valencie.m_actant_type = parm->convert_field_value(parm->m_cur_valencie.m_actant_type, $3, "ТИП", ActantTypeCount, (const char*)ActantTypesStr);
                        }

                |        CONJ_KW '=' field_values_comma
                        {
                            fill_str_vector_and_free(parm->m_cur_valencie.m_conjs, $3);
                        }

                |    FR_TYPE_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_valencie.m_clause_type = parm->convert_field_value(parm->m_cur_valencie.m_clause_type, $3, "ТИП_ФР", ClauseTypesCount, (const char*)ClauseTypeNames);
                        }

                |    MODAL_KW '=' FIELD_VALUE
                        {
                            parm->m_cur_valencie.m_modality = parm->convert_field_value(parm->m_cur_valencie.m_modality, $3, "МОД", ModalityCount, (const char*)ModalityStr);
                        }
                        

                |        CONJ_POS_KW '=' pos_field_values
                        {
                            fill_set_and_free(parm->m_cur_valencie.m_conj_POSes, $3);
                        }
                ;


positions_field_values_comm: positions_field_values_comm ',' FIELD_VALUE
                            {
                                $$ = $1->add_new(parm->convert_field_value(parm->m_cur_valencie.m_position, $3, "ПОЗ", PositionsCount, (const char*)PositionsStr));
                            }

                            |    FIELD_VALUE

                            {

                                simple_list<EPosition>* el = new simple_list<EPosition>;
                                el->next = NULL;
                                el->info = parm->convert_field_value(parm->m_cur_valencie.m_position, $1, "ПОЗ", PositionsCount, (const char*)PositionsStr);                                
                                $$ = el;                                                                    
                            }



fact_field_options:    fact_field_options ';'  FACT_FIELD_NAME_KW 
                    {                    
                        $$ = $1->add_new($3);
                        delete $3;
                    }                    
                    |
                    FACT_FIELD_NAME_KW
                    {
                        simple_list<Stroka>* el = new simple_list<Stroka>;
                        el->next = NULL;
                        el->info = $1;
                        delete $1;
                        $$ = el;                                    
                    }

field_values_comma:                field_values_comma ',' FIELD_VALUE
                                {
                                    $$ = $1->add_new($3);
                                    delete $3;
                                }
                                |    FIELD_VALUE
                                {
                                    simple_list<Stroka>* el = new simple_list<Stroka>;
                                    el->next = NULL;
                                    el->info = $1;
                                    delete $1;
                                    $$ = el;                                    
                                }

content_words:            content_word content_words
                        {
                            yvector<Stroka> v;
                            fill_vector_and_free(v, $1);
                            $$ = $2->add_new(v);                            
                        }

                |        content_word
                {
                    simple_list<yvector<Stroka> >* el = new simple_list<yvector<Stroka> >;
                    el->next = NULL;
                    fill_vector_and_free(el->info, $1);
                    $$ = el;                    
                }
                ;

content_word:            RUS_WORD
                        {
                            simple_list<Stroka>* el = new simple_list<Stroka>;
                            el->next = NULL;
                            el->info = $1;
                            delete $1;
                            $$ = el;
                        }
                |        '(' or_content_words ')'
                {
                    $$ = $2;
                }
                |        '(' or_content_words '|' ')'
                {
                    $$ = $2->add_new(Stroka(""));                    
                }
                |        '(' '|' or_content_words ')'                
                {
                    $$ = $3->add_new(Stroka(""));                    
                }
                ;

or_content_words:        or_content_words '|' rus_words_with_space
                        {
                            $$ = $1->add_new(*$3);
                            delete $3;
                        }

                |        rus_words_with_space
                {
                            simple_list<Stroka>* el = new simple_list<Stroka>;
                            el->next = NULL;
                            el->info = *$1;
                            delete $1;
                            $$ = el;                                        
                }
                ;

rus_words_with_space:    rus_words_with_space RUS_WORD
                        {
                            Stroka& ss = *$1;
                            ss += " ";
                            ss += Stroka($2);
                            $$ = &ss;    
                            delete $2;
                        }
                |        RUS_WORD
                {
                    Stroka* str = new Stroka($1);
                    delete $1;
                    $$ = str;
                }

list_of_valencies:        valencie list_of_valencies  
                        {
                            $$ = $2->add_new(*$1);
                            delete $1;    
                            //m_cur_subbordination.push_back(m_cur_or_valencie_list);                    
                            //m_cur_or_valencie_list.reset();
                        }

                |        valencie
                        {
                            simple_list<or_valencie_list_t>* el = new simple_list<or_valencie_list_t>;
                            el->next = NULL;
                            el->info = *$1;
                            delete $1;
                            $$ = el;                            
                            //m_cur_subbordination.push_back(m_cur_or_valencie_list);                    
                            //m_cur_or_valencie_list.reset();
                            
                        }
                ;
                
interpretation:    FACT_INTERP_KW '(' list_of_fact_type_references ')'
                {
                    $$ = $3;
                }
                ;
                
list_of_fact_type_references: fact_type_reference  list_of_fact_type_references
                    {
                        $$ = $2->add_new(*$1);
                        delete $1;
                    }
                    | fact_type_reference 
                    {
                        $$ = new simple_list<fact_field_reference_t>;
                        $$->next = NULL;
                        $$->info = *$1;
                        delete $1;                        
                    }
                    ;

fact_field_address: FACT_FIELD_NAME_KW '.' FACT_FIELD_NAME_KW 
                    {
                        $$ = new fact_field_reference_t;
                        $$->m_strFactTypeName = $1;
                        $$->m_strFieldName = $3;
                        delete $1;
                        delete $3;
                    }
                    ;
                    
fact_type_reference: fact_field_address ';'
                    {
                        $$ = $1;
                    }
                    | fact_field_address ':' ':' FACT_FIELD_NAME_KW ';'
                    {
                        $$ = $1;
                        EFactAlgorithm alg = Str2FactAlgorithm($4);
                        if( alg == eFactAlgCount )
                            yyerror(parm, "Bad algorithm.");
                        else
                            $$->m_eFactAlgorithm.Set(alg);
                        delete $4;                        
                            
                    }                    
                    | fact_field_address '=' TRUE_OR_FALSE_KW ';'
                    {
                        $$ = $1;
                        Stroka s = $3;                        
                        delete $3;
                                        
                        if( s == "true" )
                            $$->m_bBoolValue = true;
                        else
                            $$->m_bBoolValue = false;
                        $$->m_bHasValue = true;
                    }
                    | fact_field_address '=' default_type_field_value ';'
                    {
                        $$ = $1;
                        $$->m_StringValue = NStr::DecodeAuxDic($3);
                        delete $3;
                        $$->m_bHasValue = true;
                    }                    
                    | fact_field_address '+' '=' default_type_field_value ';'
                    {
                        $$ = $1;
                        $$->m_StringValue = NStr::DecodeAuxDic($4);
                        delete $4;
                        $$->m_bHasValue = true;
                        $$->m_bConcatenation = true;
                    }                                        
                    | fact_field_address FROM_OPERATOR FACT_FIELD_NAME_KW ';'
                    {
                        $$ = $1;
                        $$->m_strSourceFieldName = $3;
                        delete $3;                        
                    }
                    | fact_field_address FROM_OPERATOR FACT_FIELD_NAME_KW '.' FACT_FIELD_NAME_KW  ';'
                    {
                        $$ = $1;
                        $$->m_strSourceFieldName = $3;
                        $$->m_strSourceFactName = $5;
                        delete $3;                        
                        delete $5;
                    }
                    | fact_field_address '=' FACT_FIELD_NAME_KW ';'
                    {
                        $$ = $1;
                        $$->m_strSourceFieldName = $3;
                        delete $3;                        
                    }
                    | fact_field_address '+' '=' FACT_FIELD_NAME_KW ';'
                    {
                        $$ = $1;
                        $$->m_strSourceFieldName = $4;
                        delete $4;                        
                        $$->m_bConcatenation = true;
                    }
                    ;
                
or_list_of_valencies:    simple_valencie  '|' or_list_of_valencies
                        {
                            $$ = $3->add_new(*$1);
                            delete $1;            
                            //m_cur_or_valencie_list.push_back(parm->m_cur_valencie);
                            //parm->m_cur_valencie.reset();                                        
                        }
                |        simple_valencie     
                        {
                            simple_list<valencie_t>* el = new simple_list<valencie_t>;
                            el->next = NULL;
                            el->info = *$1;
                            delete $1;
                            $$ = el;                        
                            //m_cur_or_valencie_list.push_back(parm->m_cur_valencie);
                            //parm->m_cur_valencie.reset();
                        }
                ;


valencie_reference:  VALENCIE_NAME_KW ':' FIELD_VALUE '.' DIR_KW '.' VALENCIE_NAME_KW
                    {                    
                        $$ = new or_valencie_list_t();
                        $$->m_str_name = NStr::DecodeAuxDic($1);
                        delete $1;
                        $$->m_reference.m_str_article_title = NStr::DecodeAuxDic($3);
                        delete $3;
                        $$->m_reference.m_str_subbord_name = NStr::DecodeAuxDic($5);
                        delete $5;
                        $$->m_reference.m_str_valency_name = NStr::DecodeAuxDic($7);
                        delete $7;
                    }

valencie:                valencie_reference
                        {
                            $$ = $1;    
                        }                        
                |        valencie_reference interpretation
                        {
                            $$ = $1;    
                            fill_vector_and_free($$->m_interpretations, $2);
                        }                        
                |        VALENCIE_NAME_KW simple_valencie
                        {
                            $$ = new or_valencie_list_t();
                            $$->push_back(*$2);
                            $$->m_str_name = NStr::DecodeAuxDic($1);
                            delete $2;
                            delete $1;
                        }
                |        simple_valencie
                        {
                            
                            $$ = new or_valencie_list_t();
                            $$->push_back(*$1);                            
                            delete $1;
                        }                        

                |        '(' or_list_of_valencies  ')'
                        {
                            $$ = new or_valencie_list_t();
                            fill_vector_and_free(*$$, $2);
                        }

                |        VALENCIE_NAME_KW '(' or_list_of_valencies ')'
                        {
                            $$ = new or_valencie_list_t();
                            fill_vector_and_free(*$$, $3);
                            $$->m_str_name = NStr::DecodeAuxDic($1);
                            delete $1;                            
                        }
                |        VALENCIE_NAME_KW '(' or_list_of_valencies interpretation')'
                        {
                            $$ = new or_valencie_list_t();
                            fill_vector_and_free(*$$, $3);
                            $$->m_str_name = NStr::DecodeAuxDic($1);
                            fill_vector_and_free($$->m_interpretations, $4);
                            delete $1;                            
                        }
                    ;
        
        
                    
simple_valencie:        '(' list_of_dir_fields ')' 
                        {
                            $$ = new valencie_t(parm->m_cur_valencie);                            
                            parm->m_cur_valencie.reset();
                        }
                ;    


morph_info:                morph_info '|' morph_val
                        {
                            $$ = $1->add_new($3);
                        }
                |        morph_val
                        {
                            simple_list<TGramBitSet*>* el = new simple_list<TGramBitSet*>;
                            el->next = NULL;
                            el->info = $1;
                            $$ = el;
                        }
                ;


morph_val:                morph_field_values_comma
                        {
                            $$ = $1;
                        }
                |        '(' morph_field_values_comma ')'
                        {
                            $$ = $2;
                        }
                ;

morph_field_values_comma:        FIELD_VALUE ',' morph_field_values_comma
                                {                                    
                                    TGrammar res = parm->ParseGrammem($1, "МИ");
                                    $$ = $3;
                                    if (res != gInvalid)                                        
                                        *$$ |= TGramBitSet(res);
                                }

                |        FIELD_VALUE 
                {
                    TGrammar res = parm->ParseGrammem($1, "МИ");                    
                    if (res == gInvalid)
                        $$ = new TGramBitSet();
                    else
                        $$ = new TGramBitSet(res);
                }

                ;

pos_field_values:        FIELD_VALUE ',' pos_field_values
                        {
                            $$ = $3->add_new(new TGramBitSet(parm->ParsePOS($1, "ЧР")));
                        }
                |        FIELD_VALUE                 
                        {    
                            simple_list<TGramBitSet*>* el = new simple_list<TGramBitSet*>;
                            el->next = NULL;
                            el->info = new TGramBitSet(parm->ParsePOS($1, "ЧР"));
                            $$ = el;
                        }
                ;
val_order_condition_single:    VALENCIE_NAME_KW VALENCIE_NAME_KW
                            {                        
                                simple_list<Stroka>* el = new simple_list<Stroka>;
                                el->next = NULL;
                                el->info = $2;
                                delete $2;
                                el = el->add_new($1);
                                delete $1;
                                $$ = el;                                                                
                            }
                                                    
                        |   VALENCIE_NAME_KW val_order_condition_single 
                            {                        
                                $$ = $2->add_new($1);
                                delete $1;
                            }
                        ;
                        
val_order_condition_single_full:    FOLLOW_KW '(' val_order_condition_single ')'
                                    {
                                        single_actant_contion_t* el = new single_actant_contion_t();
                                        fill_str_vector_and_free(el->m_vals, $3);
                                        el->m_type = FollowACType;
                                        $$ = el;
                                    }
                                    | COORD_FUNC_KW '(' FIELD_VALUE VALENCIE_NAME_KW VALENCIE_NAME_KW ')'
                                    {
                                        single_actant_contion_t* el = new single_actant_contion_t();
                                        el->m_vals.push_back(NStr::DecodeAuxDic($4));
                                        delete $4;
                                        el->m_vals.push_back(NStr::DecodeAuxDic($5));
                                        delete $5;
                                        el->m_type = AgrACType;
                                        el->m_CoordFunc = parm->convert_field_value(el->m_CoordFunc, $3, "СОГЛ", CoordFuncCount, (const char*)CoordFuncsStr);
                                        $$ = el;
                                    }                                    
                            ;
                
val_order_condition:    val_order_condition_single_full
                        {
                            simple_list<single_actant_contion_t>* el = new simple_list<single_actant_contion_t>;
                            el->next = NULL;
                            el->info = *$1;
                            delete $1;
                            $$ = el;
                        }
                    |    val_order_condition val_order_condition_single_full
                        {
                            $$ = $1->add_new(*$2);
                            delete $2;
                        }
                    ;
                        
    
or_list_of_val_order:    '(' val_order_condition ')'
                        {
                            simple_list<actant_contions_t>* el = new simple_list<actant_contions_t>;
                            el->next = NULL;
                            fill_vector_and_free(el->info.m_conditions, $2);
                            $$ = el;
                        }
                    |    or_list_of_val_order '|'  '(' val_order_condition ')'
                        {                        
                            actant_contions_t actant_contions;
                            fill_vector_and_free(actant_contions.m_conditions, $4);
                            $$ = $1->add_new(actant_contions);
                        }
                    |    or_list_of_val_order '|'  val_order_condition_single_full
                        {                        
                            actant_contions_t actant_contions;
                            actant_contions.m_conditions.push_back(*$3);
                            $$ = $1->add_new(actant_contions);
                            delete $3;
                        }
                    ;
                        
                

%%

