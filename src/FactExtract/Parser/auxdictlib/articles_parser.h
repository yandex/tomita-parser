#pragma once

#if !defined (yyFlexLexerOnce)
    #define yyFlexLexer axdFlexLexer
    #include <contrib/tools/flex-old/FlexLexer.h>
#endif


#include <stack>

#include "dictionary.h"
#include "simplelist.h"
#include <FactExtract/Parser/common/utilit.h>


struct SArtFileAddress
{
    SArtFileAddress(int i, const Stroka& s) : m_iLineNo(i), m_strFileName(s) {};
    SArtFileAddress() : m_iLineNo(-1){};
    bool IsValid() const { return !m_strFileName.empty() && (m_iLineNo != -1); }
    int m_iLineNo;
    Stroka m_strFileName;
};

class articles_parser_t : public yyFlexLexer
{
friend int yyparse(articles_parser_t*);
public:
    articles_parser_t(dictionary_t& dict, SDefaultFieldValues DefaultValues);
    int yylex();
    int GetToken (void* valp);
    void yyerror(const char *a, bool prit_line_no = true);
    void yyerror(const char *a, const char* sFile, int line);
    void yyerror(const char *a, const article_t* pArt);
    bool fill_resend_articles(); //делаем полноценные статьи ис отсылочных (с полем ОТС)
    bool fill_contents_from_file(); //заполняем m_contents_str из файла, указанного в поле СОСТАВ
    bool process(const Stroka& strFile, const Stroka& strText = NULL);
    bool process_one_file(const Stroka& strFile);
    bool process_from_stream(std::istream *is);
    void put_dic_folder(const Stroka& s);

    template <class TYPE>
    TYPE convert_field_value(TYPE old_value, char* str_value, const char* field, int size, const char* str_list)
    {
        if (!strcmp(field, "ЧР") || !strcmp(field, "Н_ЧР") || !strcmp(field, "СОЮЗ_ЧР")) {
            Wtroka tmp = UTF8ToWide(str_value);
            tmp.to_upper();
            Stroka tmp_utf8 = WideToUTF8(tmp);
            strcpy(str_value, tmp_utf8.c_str());
        }

        int res = IsInList_i(str_list, size, str_value);
        delete str_value;
        if (res == -1) {
            char msg[500];
            sprintf(msg, "convert_field_value: Field \"%s\" has wrong value.", field);
            yyerror(msg);
            return old_value;
        } else
            return (TYPE)res;
    }

    TKeyWordType ParseKWType(char* str_value);
    TGramBitSet ParsePOS(char* strPOS, const char* field);
    TGrammar ParseGrammem(char* strGrammem, const char* field);

    dictionary_t& m_dict;

    bool        m_error;
    valencie_t  m_cur_valencie;
    article_t   m_cur_article;
    //fact_type_t   m_cur_fact_type;
    SDefaultFieldValues s_DefaultValues;
    SArtFileAddress GetArtFileAddress(const Wtroka& title) const;
    ymap<Wtroka, SArtFileAddress> m_mapArtTitle2FileAddress;

protected:
    bool    check_agreements();
    bool    check_valency_references(article_t* art);
    //bool    check_kwtype_reference(const TKeyWordType& kwtype, article_t* art, const Wtroka& valency_name);
    bool    check_title_reference(const Wtroka& reference, article_t* art, const Wtroka& valency_name);
    bool    check_geo_references(article_t* art);
    bool    check_geo_cycles(const article_t* art);
    bool    check_references();
    bool    fill_resend_valencies(article_t* p_art);
    bool    check_interpretations();
    bool    add_article(article_t* p_art);
    Stroka m_strDicFolder;
    std::stack<Stroka> m_included_files;
    Stroka m_strCurFile;
    int m_iCurArticleTitleLineNo;
};
