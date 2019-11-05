# List of terminals

A terminal can be the lemma of a word enclosed in single quotes, or one of the reserved terminal names from the table below. Note that terminals describe not only separate words, but also multiword entities (that are already assembled by the grammar or using another method), whose heads satisfy the definition of a terminal. Most terminals coincide with well-known parts of speech, but the match is often incomplete. For example, the terminal Adj does not recognize short forms of adjectives.

Some terminals recognize punctuation, such as Comma and LBracket. Punctuation marks that do not have a corresponding terminal can be described by combining the terminal AnyWord with the wff tag (see the [list of tags](all-labels-list.md)), for example: `Anyword<wff="!">;`

Terminal | **Value**
----- | -----
'...' | Morphologic lemma. Entered in single quotes.
AnyWord | Any sequence of symbols without spaces. Be cautious with the construction `AnyWord*`, because this will make the parser build a large number of variants, which will slow it down considerably.
Word | Any word consisting of letters in the Russian or Latin alphabets. Words with dashes are also allowed. This definition does not allow chains containing punctuation marks (other than dashes), special ASCII symbols, or chains of numbers.
Noun | Noun (a word with the <q>S</q> grammeme). This does not include first names, surnames, or patronymics.
Adj | Adjective, participle, ordinal number, or pronominal adjective: a word with the grammeme <q>A</q>, <q>partcp</q>,<q> ANUM</q> or <q>APRO</q>. This does not include short-form adjectives.
OrdinalNumeral | Ordinal number: a word with the grammeme <q>ANUM</q>.
Adv | Adverb: a word with the grammeme <q>ADV</q>.
Participle | Participle: a word with the grammeme <q>partcp</q>.
Verb | Verb: a word with the grammeme <q>V</q>.
Prep | Preposition: a word with the grammeme <q>PR</q>.
UnknownPOS | A word that is unrecognized morphologically. This nonterminal does not include non-lemmatized words that are described in the gazetteer. The morphologic component builds a paradigm for these words, and they are lemmatized if the appropriate articles are mentioned in this grammar.
SimConjAnd | The word <q>and</q>.
QuoteDbl | Double quotation marks.
QuoteSng | Single quotation marks.
LBracket | Opening parenthesis.
RBracket | Closing parenthesis.
Hyphen | Hyphen.
Punct | Period (dot).
Comma | Comma.
Colon | Colon.
Percent | A chain of symbols including the % symbol.
Dollar | A chain of symbols including the $ symbol.
PlusSign | The plus sign +.
EOSent | Symbol for the end of a sentence.


