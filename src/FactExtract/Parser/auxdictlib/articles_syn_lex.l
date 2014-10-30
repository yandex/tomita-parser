%{

#include "articles_parser.h"
#include "articles_syn_yacc.h"

using std::cin;
using std::cout;
using std::cerr;


%}

%option c++
%option noyywrap
%option yylineno
%option yyclass="articles_parser_t"
%option prefix="axd"


ws			[ \t\n]+
dig			[0-9]
number		({dig})+
rus_alpha_up [ЁЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЮБЬТИМСЧЯ-]
rus_alpha_low [ёйцукенгшщзхъфывапролджэячсмитьбю-]
eng_alpha_low [qwertyuiopasdfghjklzxcvbnm-]
eng_alpha_up  [QWERTYUIOPASDFGHJKLZXCVBNM-]
rus_alpha	  (rus_alpha_up|rus_alpha_low)
eng_alpha     ({eng_alpha_up}|{eng_alpha_low})
title_kw		("ЗГЛ")
content_kw		("СОСТАВ")
pos_kw			("ЧР")
new_pos			("Н_ЧР")
del_hom_kw		("УД_ОМОНИМ")
dir_kw			("УПР"({dig}|{rus_alpha_up}|"_")+)
dir_single_kw	("УПР")
posit_kw		("ПОЗ")
m_i_kw			("МИ")
sin_r_kw		("СИН_О")
conj_dir_kw		("СОЮЗ_УПР")
conj_pos_kw		("СОЮЗ_ЧР")
fr_dir_kw		("УПР_ФР")
sup_kw			("ГЛАВ")
follow_kw		("СЛЕД")
type_kw			("ТИП")
type_kw_kw		("ТИП_КС")
conj_kw			("СОЮЗ")
coord_kw		("СОГЛ")
resend_kw		("ОТС")
modal_kw		("МОД")
main_word		("ГС")
fr_type_kw		("ТИП_ФР")
prefix_kw		("ПРЕФИКС")
end_comma_kw	("КОНЕЦ_ЗПТ")
order			("ПОРЯДОК")
comp_by_lemma_kw	("СРАВН_ЛЕММА")
corr_kw			("КОРР")
ambig_kw		("AMBIGUITY")
geo_alg			("ГЕО_АЛГ")
geo_part		("ГЕО_ЧАСТЬ")
poulation		("НАСЕЛЕНИЕ")
geo_lemma		("ГЕО_ЛЕММА")
y_geo_id		("Я_ГЕО_ИД")
ant_kw			("АНТ")
comma_kw		("ЗПТ") 
light_kw		("ЛЕГКОЕ_КС") 
s0_kw   		("С0") 
file			("file")
tomita			("tomita")
alg				("alg")

fact_type				("fact_type")
fact_type_nolead		("fact_type_nolead")
fact_type_temp			("fact_type_temp")
fact_interp				("interp")
fact_field_name			({eng_alpha_low}|{eng_alpha_up}|"-"|"_"|{dig})+
fact_var_value			("$"{eng_alpha_up}+)

include			("#include")
valencie_name	({rus_alpha_up}|"_")+
file_name		(({eng_alpha_low}|{eng_alpha_up}|{dig}|"-"|"_"|"\\")+)"."(({eng_alpha_low}|{dig}|{eng_alpha_up}|"_")+)
field_value		({rus_alpha_low}|"_"|{number}|{eng_alpha_low})+
rus_word		({eng_alpha_low})*("-")?((({rus_alpha_low})+)((\.)?))((({rus_alpha_low})+)((\.)?))?|(",")|{number}|{eng_alpha_low}+


%x	COMMENT RUS_WORD_ST FACT_TYPE_DESCR FACT_TYPE_DESCR_COMMENT WORDS_IN_QUOTES_FACT_TYPE WORDS_IN_QUOTES

%%


<INITIAL,RUS_WORD_ST>{include}			{
							
							//printf("INCLUDE_KW\n" );
							BEGIN(RUS_WORD_ST);
							return INCLUDE_KW;
			
}


<INITIAL,RUS_WORD_ST>{fact_interp}			{
							
							//printf("FACT_INTERP_KW\n" );
							BEGIN(FACT_TYPE_DESCR);
							return FACT_INTERP_KW;
			
}

<INITIAL,RUS_WORD_ST>{fact_type}			{
							
							//printf("FACT_TYPE_KW\n" );
							BEGIN(FACT_TYPE_DESCR);
							return FACT_TYPE_KW;
			
}

<INITIAL,RUS_WORD_ST>{fact_type_nolead}			{
							
							//printf("FACT_TYPE_NOLOEAD_KW\n" );
							BEGIN(FACT_TYPE_DESCR);
							return FACT_TYPE_NOLEAD_KW;
			
}

<INITIAL,RUS_WORD_ST>{fact_type_temp}			{
							
							//printf("FACT_TYPE_TEMP_KW\n" );
							BEGIN(FACT_TYPE_DESCR);
							return FACT_TYPE_TEMP_KW;
			
}

<FACT_TYPE_DESCR>"}"			{
							
							//printf("'%c'\n", YYText()[0]);
							BEGIN(INITIAL);
							return ((int)YYText()[0]);
			
}

<FACT_TYPE_DESCR>")"			{
							
							//printf("'%c'\n", YYText()[0]);
							BEGIN(INITIAL);
							return ((int)YYText()[0]);
			
}

<FACT_TYPE_DESCR>("true"|"false")	{
							
							//printf("FACT_FIELD_NAME_KW\n" );							
							return TRUE_OR_FALSE_KW;
			
}


<FACT_TYPE_DESCR>("$"({eng_alpha})+)	{  
							
							//printf("VAR_FACT_VAL\n" );							
							return VAR_FACT_VAL;
			
}


<FACT_TYPE_DESCR>("from")	{							
							//printf("FROM_OPERATOR\n" );							
							return FROM_OPERATOR;
			
}

<WORDS_IN_QUOTES>[^\n\"\'}]*	{
										//printf("FIELD_VALUE\n" );							
										return FIELD_VALUE;
									}				


<WORDS_IN_QUOTES_FACT_TYPE>[^\n\"\'}]*	{
										//printf("FIELD_VALUE\n" );							
										return FIELD_VALUE;
									}				

<FACT_TYPE_DESCR>{fact_field_name}	{
							
							//printf("FACT_FIELD_NAME_KW\n" );							
							return FACT_FIELD_NAME_KW;
			
}

<FACT_TYPE_DESCR>("\""|"\'")		{
						
						BEGIN(WORDS_IN_QUOTES_FACT_TYPE);									
						//printf("'%c' - state FACT_TYPE_DESCR\n", YYText()[0]);
						return ((int)YYText()[0]);
					}


<INITIAL,RUS_WORD_ST>("\""|"\'")		{
						
						BEGIN(WORDS_IN_QUOTES);									
						//printf("'%c' - state FACT_TYPE_DESCR\n", YYText()[0]);
						return ((int)YYText()[0]);
					}

<WORDS_IN_QUOTES_FACT_TYPE>("\""|"\'")		{
						BEGIN(FACT_TYPE_DESCR);									
						//printf("'%c' - state WORDS_IN_QUOTES_FACT_TYPE\n", YYText()[0]);
						return ((int)YYText()[0]);
					}


<WORDS_IN_QUOTES>("\""|"\'")		{
						BEGIN(INITIAL);
						//printf("'%c' - state WORDS_IN_QUOTES_FACT_TYPE\n", YYText()[0]);
						return ((int)YYText()[0]);
					}


<INITIAL,RUS_WORD_ST>{title_kw}			{
						//printf("TITLE_KW\n" );
						BEGIN(INITIAL);				
						return TITLE_KW;
					}

<RUS_WORD_ST>"="	{
						//printf("'%c'\n", YYText()[0]);
						return ((int)YYText()[0]);
					}


<RUS_WORD_ST>{file}	{
						//printf("'%s'(file)\n", YYText());
						return FILE_KW;						
					}
					
<RUS_WORD_ST>{tomita}	{
						//printf("TOMITA_KW\n");						
						return TOMITA_KW;						
					}

<RUS_WORD_ST>{alg}	{
						//printf("ALG_KW\n");		
						BEGIN(INITIAL);				
						return ALG_KW;						
					}

					
<RUS_WORD_ST>{file_name}	{
						//printf("'%s'(FILE_NAME)\n", YYText());
						BEGIN(INITIAL);
						return FILE_NAME;						
					}

<RUS_WORD_ST>{rus_word}	{
						//printf("'%s'(rus_word)\n", YYText());
						return RUS_WORD;						
					}

<INITIAL,RUS_WORD_ST>{content_kw}		{
						//printf("CONTENT_KW\n" );
						BEGIN(RUS_WORD_ST);
						return CONTENT_KW;
					}

<INITIAL,RUS_WORD_ST>{pos_kw}			{
						//printf("POS_KW\n" );
						BEGIN(INITIAL);
						return POS_KW;
					}

<INITIAL,RUS_WORD_ST>{new_pos}			{
						//printf("NEW_POS_KW\n" );
						BEGIN(INITIAL);
						return NEW_POS_KW;
					}

<INITIAL,RUS_WORD_ST>{del_hom_kw}		{
						//printf("DEL_HOM_KW\n" );
						BEGIN(INITIAL);
						return DEL_HOM_KW;
					}

<INITIAL,RUS_WORD_ST>{dir_kw}			{
						//printf("DIR_KW\n" );
						BEGIN(INITIAL);
						return DIR_KW;
					}
					
<INITIAL,RUS_WORD_ST>{dir_single_kw}		{
						//printf("DIR_KW\n" );
						BEGIN(INITIAL);
						return DIR_SINGLE_KW;
					}


<INITIAL,RUS_WORD_ST>{type_kw}			{
						//printf("TYPE_KW\n" );
						BEGIN(INITIAL);
						return TYPE_KW;
					}
				
<INITIAL,RUS_WORD_ST>{type_kw_kw}			{
						//printf("TYPE_KW\n" );
						BEGIN(INITIAL);
						return TYPE_KW_KW;
					}
					

<INITIAL,RUS_WORD_ST>{geo_alg} {					
						//printf("GEO_ALG_KW\n" );
						BEGIN(INITIAL);
						return GEO_ALG_KW;
					}
					
<INITIAL,RUS_WORD_ST>{geo_part} {					
						//printf("GEO_PART_KW\n" );
						BEGIN(INITIAL);
						return GEO_PART_KW;
					}		

<INITIAL,RUS_WORD_ST>"ЦЕЛЬ" {					
						//printf("GEO_PART_KW\n" );
						BEGIN(INITIAL);
						return GOAL_KW;
					}		

					
<INITIAL,RUS_WORD_ST>{poulation} {					
						//printf("POPULATION_KW\n" );
						BEGIN(INITIAL);
						return POPULATION_KW;
					}		
			


<INITIAL,RUS_WORD_ST>{y_geo_id}		{					
						//printf("Y_GEO_ID\n" );
						BEGIN(INITIAL);
						return Y_GEO_ID;
					}
			
<INITIAL,RUS_WORD_ST>{geo_lemma}		{					
						//printf("GEO_LEMMA_KW\n" );
						BEGIN(INITIAL);
						return LEMMA_KW;
					}

<INITIAL,RUS_WORD_ST>"ЛЕММА"		{					
						//printf("GEO_LEMMA_KW\n" );
						BEGIN(INITIAL);
						return LEMMA_KW;
					}
					
<INITIAL,RUS_WORD_ST>"ЦЕНТР"		{					
						//printf("GEO_CENTER_KW\n" );
						BEGIN(INITIAL);
						return GEO_CENTER_KW;
					}
										
<INITIAL,RUS_WORD_ST>"НЕИЗМ"		{					
						//printf("INCHANGEABLE_KW\n" );
						BEGIN(INITIAL);
						return INCHANGEABLE_KW;
					}

<INITIAL,RUS_WORD_ST>"ВСЕГДА"		{					
						//printf("ALWAYS_KW\n" );
						BEGIN(INITIAL);
						return ALWAYS_KW;
					}


<INITIAL,RUS_WORD_ST>{fr_dir_kw}		{
						//printf("POSIT_KW\n" );
						BEGIN(INITIAL);
						return FR_DIR_KW;
					}

<INITIAL,RUS_WORD_ST>{posit_kw}			{
						//printf("POSIT_KW\n" );
						BEGIN(INITIAL);
						return POSIT_KW;
					}

<INITIAL,RUS_WORD_ST>{m_i_kw}			{
						//printf("MORPH_I_KW\n" );
						BEGIN(INITIAL);
						return MORPH_I_KW;
					}
					
<INITIAL,RUS_WORD_ST>{sin_r_kw}			{
						//printf("SYN_R_KW\n" );
						BEGIN(INITIAL);
						return SYN_R_KW;
					}

<INITIAL,RUS_WORD_ST>{conj_dir_kw}		{
						//printf("CONJ_DIR_KW\n" );
						BEGIN(INITIAL);
						return CONJ_DIR_KW;
					}

<INITIAL,RUS_WORD_ST>{sup_kw}			{
						//printf("SUP_KW\n" );
						BEGIN(INITIAL);
						return SUP_KW;
					}

<INITIAL,RUS_WORD_ST>{conj_kw}			{
						BEGIN(INITIAL);
						return CONJ_KW;
					}

<INITIAL,RUS_WORD_ST>{resend_kw}	{
						//printf("RESEND_KW\n" );
						BEGIN(INITIAL);
						return RESEND_KW;
					}

<INITIAL,RUS_WORD_ST>{end_comma_kw}	{
						//printf("END_COMMA_KW\n" );
						BEGIN(INITIAL);
						return END_COMMA_KW;
					}					
					
<INITIAL,RUS_WORD_ST>{s0_kw}	{
						//printf("S0_KW\n" );
						BEGIN(INITIAL);
						return S0_KW;
					}

<INITIAL,RUS_WORD_ST>{light_kw}	{
						//printf("END_COMMA_KW\n" );
						BEGIN(INITIAL);
						return LIGHT_KW;
					}
					
<INITIAL,RUS_WORD_ST>{comp_by_lemma_kw}	{
						//printf("COMP_BY_LEMMA_KW\n" );
						BEGIN(INITIAL);
						return COMP_BY_LEMMA_KW;
					}

<INITIAL,RUS_WORD_ST>{order}	{
						//printf("ORDER_KW\n" );
						BEGIN(INITIAL);
						return ORDER_KW;					
					}

<INITIAL,RUS_WORD_ST>{conj_pos_kw}	{
						//printf("CONJ_POS_KW\n" );
						BEGIN(INITIAL);
						return CONJ_POS_KW;					
					}

<INITIAL,RUS_WORD_ST>{fr_type_kw}	{
						//printf("FR_TYPE_KW\n" );
						BEGIN(INITIAL);
						return FR_TYPE_KW;					
					}
					
<INITIAL,RUS_WORD_ST>{main_word}	{					
						//printf("MAIN_WORD_KW\n" );
						BEGIN(INITIAL);
						return MAIN_WORD_KW;					
					}
					
					

<INITIAL,RUS_WORD_ST>{coord_kw} {
						//printf("COORD_FUNC_KW\n" );
						BEGIN(INITIAL);
						return COORD_FUNC_KW;					
					}
					
<INITIAL,RUS_WORD_ST>{prefix_kw} {
						//printf("PREFIX_KW\n" );
						BEGIN(INITIAL);
						return PREFIX_KW;					
					}


<INITIAL,RUS_WORD_ST>{corr_kw} {
						//printf("CORR_KW\n" );
						BEGIN(INITIAL);
						return CORR_KW;					
					}


<INITIAL,RUS_WORD_ST>{ambig_kw} {
						//printf("AMBIG_KW\n" );
						BEGIN(INITIAL);
						return AMBIG_KW;					
					}


<INITIAL,RUS_WORD_ST>{ant_kw} {
						//printf("ANT_KW\n" );
						BEGIN(INITIAL);
						return ANT_KW;					
					}

<INITIAL,RUS_WORD_ST>{comma_kw} {
						//printf("COMMA_KW\n" );
						BEGIN(INITIAL);
						return COMMA_KW;					
					}

<INITIAL,RUS_WORD_ST>{modal_kw}	{
						//printf("MODAL_KW\n" );
						BEGIN(INITIAL);
						return MODAL_KW;					
					}
					
<INITIAL,RUS_WORD_ST>{follow_kw}	{
						//printf("FOLLOW_KW\n" );
						BEGIN(INITIAL);
						return FOLLOW_KW;					
					}


<INITIAL,RUS_WORD_ST>{valencie_name}			{
						//printf("valencie_name\n" );
						BEGIN(INITIAL);
						return VALENCIE_NAME_KW;
					}					

{field_value}		{
						//printf("FIELD_VALUE\n");						
						return 	FIELD_VALUE;
					}
					


<INITIAL,RUS_WORD_ST>"#"    BEGIN(COMMENT);
<INITIAL,RUS_WORD_ST>"//"   BEGIN(COMMENT);

<COMMENT>[^\n]*				

<COMMENT>\n				BEGIN(INITIAL);							


<FACT_TYPE_DESCR>"#"    BEGIN(FACT_TYPE_DESCR_COMMENT);
<FACT_TYPE_DESCR>"//"   BEGIN(FACT_TYPE_DESCR_COMMENT);

<FACT_TYPE_DESCR_COMMENT>[^\n]*				

<FACT_TYPE_DESCR_COMMENT>\n				BEGIN(FACT_TYPE_DESCR);							


<INITIAL,RUS_WORD_ST,FACT_TYPE_DESCR>{ws}   

 
<INITIAL,RUS_WORD_ST,FACT_TYPE_DESCR>.  {						
						//printf("'%c'\n", YYText()[0]);
						return ((int)YYText()[0]);
					}


