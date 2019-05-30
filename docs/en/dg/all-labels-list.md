# List of all tags

**Tag** | **Semantics** | **Terminal** | **Nonterminal** | **Filter** | **Negation** | **Syntax**
----- | ----- | ----- | ----- | ----- | ----- | -----
**Entries with dictionary references**
kwtype | The symbol is restricted by the entry or type of entry specified in the kwtype field. | + | + | + | + | kwtype="article1" kwtype="article1" kwtype= type1
kwset | The symbol is restricted by one of the entries or types of entries specified in the kwtype field. | + | + | - | + | kwset=[type1,"article1"] kwset=[type1,"article1"]
kwsetf | The same constraint as kwset, but applies to the first word in a group (not the main word). | + | + | - | + | kwsetf=[type1,"article1"]
label | The symbol is restricted by a list from the entry specified in the label field. | + | + | - | + | label="article1"
gztweight | Adds weight to the total weight of a nonterminal in the left part of a rule. The weight to be added is located in the dictionary entry in the field that is named in the field for the gztweight tag. The gztweight tag can only be used with the kwtype tag. | + | + | - | - | kwtype="type1", gztweight=" type1weight"
**Grammatical constraints**
gram | Checks the values of grammatical characteristics separately for each homonym. | + | + | - | + | gram="sg,pl"
GU | Checks the values of grammatical characteristics separately for each homonym, or for all homonyms simultaneously. | - | + | - | + | GU=[nom,sg] GU=[sg] GU=&[nom,acc]
**Agreement**
gnc-agr | Agreement by gender, number, and case. | + | + | - | + | Adj<gnc-agr[1]> Noun<gnc-agr[1]>
nc-agr | Agreement by number and case. | + | + | - | + | N1<nc-agr[2]> N2<nc-agr[2]>
c-agr | Agreement by case. | + | + | - | + | Noun<c-agr[3]> 'and' Noun<c-agr[3]>
gn-agr | Agreement by gender and number. | + | + | - | + | 
gc-agr | Agreement by gender and case. | + | + | - | + | 
fem-c-agr | An extension of the gnc-agr agreement that allows for non-agreement by gender if one of the agreement constituents has the grammemes "fem,famn". | + | + | - | + | `Noun<fem-c-agr[1]> Noun<fem-c-agr[1]>` It works like this: + `vrach Anna` ("doctor Anna"; masculine noun, feminine name) - `vrach Mikhail` ("doctor Michael"; masculine noun, masculine name)
after-num-agr | Agreement of an adjective-noun pair after a numeral in Russian, such as <q>5 amerikanskih prezidentOV</q>, but <q>2 amerikanskih prezidentA</q>. | + | + | - |  | 
sp-agr | Subject-predicate agreement. | + | + | - | + | Noun<sp-agr[4]> Verb<sp-agr[4]>
izf-agr | Turkish izafet agreement. | + | + | - |  | 
fio-agr | Agreement in format between two "fio" type objects. | + | + | - | + | 
geo-agr | Agreement between two objects in the geographical thesaurus by their belonging to the same branch of the geothesaurus. | + | + | - |  | 
**Regular expressions**
wfm | A regular expression is applied to a wordform that is the head of a syntactic group. | + | + | + | + | Word<wfm=".*банк(|a|у|е|ом)/">
wff | A regular expression is applied to the first wordform in a syntactic group. | + | + | + | + | Word<wff="им\\.">
wfl | A regular expression is applied to the last wordform in a syntactic group. | + | + | + | + | AnyWord<wfl="[0-9]{3}-[0-9]{2}-[0-9]{2}">; // phone number
**Case of letters in a word**
h-reg1 | The first letter in the word is in uppercase. The tag is applied to the beginning of a phrase, and not to the head. | + | + | + |  | Noun<h-reg1>
h-reg2 | The first letter of a word and at least one other letter in the word are in uppercase, such as in the word <q>MosStroy</q>. | + | + | + |  | 
h-reg | Synonym of h-reg2 |  |  |  |  | 
l-reg | All letters in the word are in lowercase. | + | + | + |  | 
**Quotation marks**
quoted | A word or group of words in quotation marks. | + | + | - | + | SomeQuote<quoted> SomeName<quoted>
l-quoted | A word or group of words with an opening quote before the first symbol and without a closing quote after the last symbol. | + | + | - | + | 
r-quoted | A word or group of words with a closing quote after the last symbol and without an opening quote before the first symbol. | + | + | - | + | 
**Special**
fw | The very first word of a symbol must be the first word of the sentence. | + | - | - | + | Lead_in<fw> ProperName<fw>
mw | Multiword. | + | - | - | + | Noun<mw> SimpleWord<mw>
lat | The word consists of letters in the Latin alphabet. | + | + | + | - | Word<lat>
no_hom | The symbol must consist of homonyms with the same part of speech. | + | + | - | - | Word<no_hom>
cut | The word or syntactic group is excluded from interpretation. | + | + | - | - | MainWords Context<cut>
rt | Designates the head of the resulting syntactic group. Strictly speaking, "rt" is not a constraint, but belongs to the syntactic operators described earlier. | + | + | - | - | NP -> Adj Noun<rt>;
**Dictionary entry**
dict | The word must be in the morphological dictionary. | + | - | + | + | 



## Details on quotation marks <a name="quotes"></a>

 | Vnukovo | "Vnukovo" | "Vnukovo | Vnukovo" | ."
quoted | no | **yes** | no | no | no
~quoted | **yes** | no | **yes** | **yes** | **yes**
l-quoted | no | no | **yes** | no | no
~l-quoted | **yes** | **yes** | no | **yes** | **yes**
r-quoted | no | no | no | **yes** | **yes**
~r-quoted | **yes** | **yes** | **yes** | no | no


