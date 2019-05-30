# Relationship between the gazetteer and grammar

## Link from a grammar to the gazetteer <a name="ssylkaizgrammatikivgazettir"></a>

If we want to restrict a terminal or the head of a nonterminal using a phrase or set of words, we use kwtype (see [information on kwtype](labels-limits.md)) and kwset (see [information on kwset](labels-limits.md)). For example:

`Noun<kwtype=name_of_type_or__entry>`

or

`NP<kwset=[name_of_type_or_entry1 , ..., name_of_type_or_entry2 ]`

For nonterminals [kwset](labels-limits.md) is better, since it will work faster.

If the entry type is specified, this is the same as explicitly listing the names of all the entries that have this type. Sometimes it is necessary to give the parser input of pre-assembled chains, called multiwords. For example, we are writing a grammar to extract names, and we want to extract the name from the chain `"g. Vladimir"` ("g" is an abbreviation for "gorod", meaning "city" and used before city names in Russian). So we can write an entry in the gazetteer that explicitly sets a list of such cities with the words "g" or "gorod" (`"g. Kirov"`, `"gorod Vladimir"`). Next, using the [#GRAMMAR_KWSET](syntax-grammars.md#GRAMMAR_KWSET) directive, we link to this entry in the grammar for names. Before the parser runs the grammar on the sentence `"V g. Vladimire"` ("In the city of Vladimir"), it assembles the multiword `"g_Vladimir"` as an input symbol, so the parser will not even see the separate word `"Vladimir"`. You can find more information about this technique [here](unobvious-solutions.md).


## Link from the gazetteer to a grammar <a name="ssylkaizgazettiravgrammatiku"></a>

If we want to write a gazetteer entry with a [key](gazetteer-syntax.md) that includes all the chains extracted by the grammar from the `grammar.cxx` file, we enter it like this:

`key = { "tomita:grammar.cxx" type=CUSTOM }`

There is no way to make the parser recognize, compile and run a grammar, other than to include an entry in the gazetteer with a link to it.


## Using the gazetter during normalization <a name="ispolzovaniegazettiraprinormalizacii"></a>

When populating fact fields, the parser tries to normalize the extracted chain. But it doesn't just lemmatize the head and its agreeing words. If it finds a word that was assigned a gazetteer entry with a `lemma` field, instead of the original word, the contents of this field will be entered.

For example, we have the grammar:

```no-highlight
Country -> Word<kwtype= geo_country >;
S-> Country interp(Geo.Country);
```

And an entry it links to:

```no-highlight
geo_country "geo_russia"
{
    key = "russia"| "rf"
    lemma = "russia"
}
```

Then the grammar will run on the phrase `"president RF"` and in the `Country` field for the `Geo` fact it will enter the word `"russia"`.


## Grammatical attributes of a multiword <a name="grammaticheskiepriznakimultivorda"></a>

If a gazetteer entry's key consists of more than one word and there is a link to this entry from a grammar, the grammar will see this key as a single word. When processing an input sentence, a multiword is built from these words. In addition, the grammatical characteristics that are checked by the `gram` or `GU<link>` tags are inherited from the head. It is set in the `mainword` field (see [[!TITLE gazetteer-syntax.md]](gazetteer-syntax.md)).

For example, a grammar that links to the `heads` entry:

```no-highlight
N -> Word<kwtype=heads>;
Post -> N<gram='anim'>;
```

A gazetteer that has this entry in it:

```no-highlight
heads "gen_dir"
{ 
    key = "general director"
}
```

For the phrase `"General director Petrov"`, this grammar will not work. The `"<gen_dir>"` entry does not specify the main word, which by default is the first word. So the multiword `"general_director"` takes its grammatical characteristics from the word `"general"`, which does not have an animate category. The correct entry is:

```no-highlight
heads "gen_dir"
{ 
    key = "general director"
    mainword = 2
}
```

