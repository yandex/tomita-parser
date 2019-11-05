# Constraint tags

Multiple constraints can be applied to a set of chains that is described by a terminal or nonterminal. To do this, comma-separated constraint tags are put in angle brackets `< >` after the (non)terminal, thus more precisely defining the properties of the (non)terminal. A list of these constraint tags is provided in the table at the end of this section.

Constraints can only be applied to (non)terminals that are on the right side of the rules or in filters. For non-terminals, the constraint applies to the syntactic head (main word) of the group that this nonterminal describes. Most of the tags can take the negation operator `~`, which means <q>any constraint values are allowed except the following</q>. The negation operator is placed to the left of the tag.

Constraints can be diverse in their structure. Some of them are unary operators, while some have a field that can be filled in with various values. The <q>agreement</q> constraint is defined for a pair of (non)terminals.

Some constraints are special: some of them cannot be applied to nonterminals, and some are not used with negation. All of these characterisitics are also described in the table. A list of tags that require detailed documentation is provided below.


## Type of keyword (kwtype tag) <a name="kwtype"></a>

The `kwtype` tag (keyword type) specifies that the word (or head of a multiword entity) that this (non)terminal corresponds to must be an object of the given type. The value of the `kwtype` field takes the name of the dictionary entry or the name of the type of dictionary entry. Information about entries and their types can be found in one of the gzt dictionaries.

Each dictionary entry serves as instructions for constructing an object. In turn, the object may be a keyword from a specific dictionary, or a syntactic group extracted by a different grammar. The kwtype field plays a key role in constructing grammar rules, since it allows the rules to use the results of other grammars as atomic objects.

> `Animal -> Noun<kwtype="animals_of_central_africa">;`

The `kwtype` tag also has the special value `none`. It indicates that the given symbol cannot be an object corresponding to an entry. When checking homonyms of a word for the presence of the `kwtype=none` tag, objects are ignored if they are not defined in the rules of this grammar. For example, if a word is the name of a company, but the <q>company_name</q> entry describing it is not mentioned in the rules for this grammar, this word will be considered a basic entity and will satisfy the constraint `kwtype=none`.

> [!IMPORTANT]
> 
> This constraint is applied differently for the `Word` terminal and for all other symbols. The `Word<kwtype=none>` symbol is only applied to words for which the conditions mentioned above are met for all homonyms. For the rest of the symbols, this tag is applied to words that have at least one homonym that these conditions are met for.
> 

After `kwtype` is applied, the symbol is left with only one of the homonyms that has this tag. Consequently, it is pointless to apply another `kwtype` constraint to the same symbol higher up in the syntactic tree: only one homonym is assigned to this symbol and a rule that requires the presence of a different homonym will not be applied. This is how the `kwtype` tag differs from the `kwset` tag.

> [!NOTE]
> 
> We do not recommend using the `kwtype` tag on nonterminals placed at high levels in the syntactic tree, since this may slow down the parser.
> 


## Set of keyword types (kwset tag) <a name="kwset"></a>

The `kwset` constraint (keyword set) performs the same function as `kwtype`. The difference between these constraints is that `kwset` is applied to all of a word's homonyms. First of all, this means that a symbol obtained as a result of applying `kwset` may contain other homonyms if there are other dictionary entries that also describe this symbol. Secondly, the `kwset` tag can list several entry names or types at once.

The entry `kwset=["entry_1","entry_2",…,"entry_n"]` means that the word must have at least one homonym that is described by one of the listed entries or types. A negative entry `kwset=~["entry_1","entry_2",…,"entry_n"]` means that if at least one of the word's homonyms is described by one of the listed entries or types, the rule is not applied.

Using slash `/` and asterisk `*` symbols in the `kwset` field works the same way as in the `kwtype` field.


## Grammatical characteristics (gram tag) <a name="gram"></a>

The `gram` tag restricts a symbol to acceptable values of grammatical characteristics. For example, the entry `gram="nom,pl"` means that the word (or the main word in a multiword entity) must have the grammemes `nom` (nominative case) and `pl` (plural number). The list of grammemes can specify any grammemes, including parts of speech, so the terminal `NOUN` can be entered as `Word<gram="S">`. A list of grammemes used and their values is provided in the table [Grammeme values](grammemes-values.md).

If a word has two homonyms, the gram tag checks the values of the grammatical characteristics of each of them in turn. For example, the Russian word <q>lesa</q> ("forest") has a homonym with the grammemes `nom` and `pl` (nominative case and plural number) and a homonym with the grammemes `gen` and `sg` (genative case and singular number). This word will match a rule with the tag `gram="gen,sg"` or `gram="pl"`, but it will not match a rule with the tag `gram="gen,pl"`.

Negation of a grammeme in the `gram` field is immediately applied to all homonyms. For example, the word <q>lesa</q> does not satisfy the tag `gram="~sg"`, even though it has a plural homonym. Grammemes with and without negation can be specified simultaneously in the same `gram` tag.


## Combining grammatical characteristics (GU tag) <a name="obedineniegrammaticheskixxarakteristikpometagu"></a>

The `GU` (grammar union) tag provides a broader scope for using grammemes in grammars. In its simplest form `GU=["nom,pl"]`, this tag is the same as the `gram` tag: it checks grammatical characteristics for each homonym (in the example above, <q>nominative case, plural number</q>) and if a homonym is found that satisfies this condition, the rule is applied.

Grammemes are comma-separated inside square brackets `[ ]`. Negating individual grammemes within this entry is not allowed. Negation `~` before the square brackets indicates that the intersection of the listed grammemes with each homonym's grammemes is empty; in other words, if the interpretation of at least one of the homonyms matches the set of grammemes in the square brackets, the rule with this constraint is not applied.

> ## Example without negation
> 
> `S -> Noun<GU=[sg,acc], rt>;`
> 
> It is applied like this:
> 
> ```no-highlight
> - taburetka // "stool", nominative case
> + taburetku // "stool", accusative case
> + stol        // "table", nominative or accusative case
> - stola      // "table", genitive case
> ```

> ## Example with negation
> 
> `S -> Noun<GU=~[sg,acc], rt>;`
> 
> It is applied like this:
> 
> ```no-highlight
> + taburetka // "stool", nominative case
> - taburetku // "stool", accusative case
> - stol        // "table", nominative or accusative case
> + stola      // "table", genitive case
> ```

The ampersand `&` before the square brackets indicates that the parser will analyze the grammemes simultaneously for the combined grammatical attributes of all the homonyms, and not for each homonym separately.

> ## Example with ampersand
> 
> `S -> Noun<GU=&amp;[sg,acc,nom], rt>;`
> 
> It is applied like this:
> 
> ```no-highlight
> - taburetka // "stool", nominative case
> - taburetku // "stool", accusative case
> + stol        // "table", nominative or accusative case
> - stola      // "table", genitive case
> ```

In addition, you can use the `GU` tag to enter the disjunction of several of these conditions. The pipe symbol `|` (<q>or</q>) can be used to separate several different lists of grammemes, so the rule is applied correctly when the word satisfies at least one of the listed conditions.

> `GU=[sg,ins]|&[nom,acc,gen,dat,ins]`

In this entry, one of the following conditions must be fulfilled

- the nonterminal has a homonym in the instrumental case, singular number
- the combination of all the grammemes of the nonterminal includes all the case grammemes (the word does not change by case)


```no-highlight
- stol ("table", singular nominative)
+ stolom ("table", singular instrumental)
+ pal'to ("coat", indeclinable)
```


## Agreement in grammars <a name="soglasovanievgrammatikax"></a>

Tomita implements an agreement mechanism that detects whether two symbols have matching values for one or several attributes. For example, case agreement (the `c-agr` tag) in the following example applies for the phrase <q>chelovek i koshka</q> ("person and cat", with both nouns in the nominative case) but will not work for the phrase <q>cheloveku i koshka</q> (the same phrase, but the first noun is in the dative case and the second noun is in the nominative case).

`A -> Noun<c-agr[1]> 'and' Noun<c-agr[1]>`

Agreement is a binary relationship, which is entered for each of two symbols on the right side of a rule. To differentiate a single type of agreement inside a single rule, the pair of symbols is assigned an ID (an integer). If the check for agreement was completed successfully and one of the agreement constituents is the head of a syntactic group, it is assigned an array of grammemes derived from the overlapping grammemes of agreement constituents.

Negated agreement means that two symbols cannot have identical values for the attributes set by the agreement. However, some of the attribute values may be the same for these symbols.

One symbol may simultaneously be a part of multiple agreement relationships with different constituents. For example, a noun can simultaneously agree with a determiner and a predicate, or with two adjectives. Below you will find several examples of agreement and examples of parsable chains (denoted by a plus at the start of the line) and non-parsable chains (denoted by a minus at the start of the line):

`S -> Adj<gnc-agr[1]> Noun<gnc-agr[1], gram=’nom’, sp-agr[2]> Adv Verb<rt, sp-agr[2]>;`

```no-highlight
+ nasha Masha gromko plachet ("our Masha is crying loudly"; singular feminine nominative adjective, singular feminine nominative noun, adverb, singular present verb)
- nashei Masha gromko plachet (same phrase as the first, but the adjective is in dative case)
- nasha Masha gromko plachut (same phrase as the first, but the verb is plural)
```

`S -> Adj<gnc-agr[1]> Adj<gnc-agr[2]> Noun<rt,gnc-agr[1],gnc-agr[2] >;`

```no-highlight
+ noviy estonskiy prem'er-ministr ("new Estonian Prime Minister"; singular masculine nominative adjective, singular masculine nominative adjective, singular masculine nominative noun)
- noviye estonskiy prem'er-ministr (same phrase as the first, but the first adjective is plural)
- noviy estonskaya prem'er-ministr (same phrase as the first, but the second adjective is feminine)
```

`S -> Participle<gnc-agr[2]> Adj<gnc-agr[1]> Noun<gnc-agr[1], gram=’ins’> Noun<gnc-agr[2], gram=’nom’, rt>;`

```no-highlight
+ obozhaemiy mestnim naseleniem napitok ("beloved drink of the local population"; singular masculine nominative adjective, singular masculine instrumental adjective modifying singular masculine instrumental noun, singular masculine nominative noun)
- obozhaemaya mestnim naseleniem napitok (same as the first phrase, but the first adjective is feminine)
- obozhaemiy mestnimi naseleniem napitok (same as the first phrase, but the second adjective is plural)
```

`S -> Noun<~gnc-agr[1], rt> Adj<~gnc-agr[1]>;`

```no-highlight
+ platforma Severniy ("North platform"; singular feminine nominative noun, singular male nominative adjective)
- platforma Severnaya (same as the first phrase, but the adjective is feminine)
```

Now we will describe special types of agreement that are useful when writing grammars for Russian texts.


### fem-c-agr agreement <a name="soglasovaniefem-c-agr"></a>

The agreement `fem-c-agr` is an extension of the `gnc-agr` agreement; it allows for non-agreement by gender if one of the agreement constituents has the grammemes `fem` and `surn`. This agreement is for female first names and surnames, since they can be used together with male nouns in Russian, such as `poet Ahmatova` (a masculine noun before a female surname). But the chain `poetessa Mayakovskiy` (a feminine noun before a masculine surname) will not be accepted.


### after-num-agr agreement <a name="soglasovanieafter-num-agr"></a>

The agreement of an "adjective + noun" pair after a numeral in Russian.

`S -> Adj<after-num-agr[1]> Noun<after-num-agr[1], rt>;`

```no-highlight
pyat' amerikanskih prezidentov ("five American presidents"; genitive case is used for nouns after the numbers five and higher) 
dva amerikanskih prezidenta ("two American presidents"; accusative case is used for nouns after the numbers two through four)
```


### fio-agr agreement <a name="soglasovaniefio-agr"></a>

The `fio-agr` agreement is made between two `fio` objects. This agreement was created to verify that full names that are constituents of a single sentence are written in the same format. The `fio-agr` agreement is implemented by the following rules:

1. If one of the names consists of only the surname, but another contains the first name or initial, verification fails.
1. If both the names contain initials or surnames, but they are positioned differently relative to the surname (to the right or left of it), verification fails.
1. In all other cases, agreement is successful.



## Regular expressions (wfm, wff, and wfl tags) <a name="reguljarnyevyrazhenijapometywfmwffwfl"></a>

A symbol is checked against a regular expression specified in the `wfm`, `wff` or `wfl` field. The `wfm` tag applies a regular expression to the head of the syntactic group that corresponds to this nonterminal. The `wff` and `wfl` tags are applied to the first and last words in the chain, respectively. If the symbol corresponds to a single word in the text, the results of applying these three tags to it are all the same.

Regular expressions are parsed using the freely distributed library [Perl Compatible Regular Expressions](http://h.yandex.net/?http%3A%2F%2Fwww.pcre.org). You can find the syntax of Perl regular expressions in the documentation: [perlre](http://h.yandex.net/?http%3A%2F%2Fperldoc.perl.org%2Fperlre.html). A small but significant difference from the documented syntax for regular expressions is that the preprocessor additionally appends all the expressions from the `wfm`, `wff` and `wfl` fields with the symbols for the beginning and end of the chain, `^` and `$`. In practice, this is usually more convenient than not. If it is inconvenient, this feature can be modified by adding the sequence <q>dot-asterisk</q>`.*` on both sides of the regular expression.

Setting a regular expression in quotation marks requires additional escape symbols compared to the standard Perl syntax for regular expressions. Since the backslash `\` is already an escape symbol for strings, it must be repeated twice in order to get a single backslash in the regular expression. So in order to recognize the `\` sign itself using a regular expression, the `wfm` field must use the entry  (compare with `/\\/` in Perl).

The traditional format for regular expressions with two slashes `/ /` can also be used instead of quotation marks `" "`. This entry format interprets escape symbols as is normal for Perl.

> `S -> Word<wfm=/[A-Z-]{3,10}>; // only uppercase letters and dashes, from 3 to 10 characters long`


## no_hom tag <a name="pometanohom"></a>

The `no_hom` tag requires that the symbol consists of homonyms with a single part of speech. For example, the rule `S->Word<no_hom>;` applies to the word `stol` ("table") but does not apply to the word `stolovaya` ("dining room"), since the latter is listed in the dictionary both as an adjective and as a noun.

If the symbol is assigned a <q>geographical</q>`kwtype` (i.e. any `kwtype` whose type starts with the prefix `geo_`), the `no_hom` tag checks that all homonyms belong to geographical dictionaries.

