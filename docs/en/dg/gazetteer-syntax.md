# Gazetteer syntax

## Overview of a dictionary entry <a name="article"></a>

```no-highlight
EntryType “EntryName”
{
    Key = "..."
    Field1 = "..."
    ...
    FieldN = "...
}
```

A dictionary entry's definition consists of the type of entry, name, and contents. The contents are enclosed in curly brackets { }. The contents of an entry are the names of fields, followed by the equal sign (=) and their values. Each field is separated by a line break or curly brackets.

Curly brackets also separate internal sections in entries.

```no-highlight
object “Mountain”
{
    key = “Hoverla”
}
```


## Type of entry <a name="article-type"></a>

The entry's type is written before the name. Entry types can consist of Latin case-sensitive letters, numbers, and the underscore symbol (_). Numbers are not allowed at the beginning of an entry type. The gazetteer has a base type called `TAuxDicArticle`; all other types are derived from it and must be pre-declared. The entry's type determines its set of fields.

Examples of correct entry types: `TAuxDicArticle`, `funny_animal`, `object`.

Entry types are often used to group similar entries together, such as grouping all city names in the `city` type. To do this, define an entry type that is derived from `TAuxDicArticle` and does not contain any fields.

> `message city : TAuxDicArticle {}`

Entry types can be used in the constraints `kwtype` and `kwset`.


## Predefined types of entries <a name="predopredeljonnyetipystatejj"></a>

Several entry types are defined in the `kwtypes_base.proto` file that is built into the parser. These names cannot be redefined, but they can be used in dictionaries by creating entries with these types, and they can be referenced in grammars.

**Type name** | **Semantics**
`fio` | A C++ algorithm integrated in the parser for detecting a full name
`fio_without_surname` | Uses the same algorithm as for detecting a full name, but without the surname
`date` | Built-in parser algorithm for detecting a date (for example, <q>01.08.2012</q>, <q>1 January 2011</q>)
`number` | Built-in parser algorithm for detecting a number (for example, <q>1342 thousand</q>, <q>1,342 million</q>)



## Entry name <a name="article-name"></a>

The entry name is enclosed in quotation marks. It can consist of any case-sensitive letters of any alphabet, numbers, the underscore symbol (_), and the slash (/). Numbers are not allowed at the beginning of an entry name. Entry names must be unique. The entry type must be specified before the entry name.

Examples of correct entry names: `Гора`, `mountain/83`, `_dağ1`.


## Comments <a name="kommentarii"></a>

Comments are entered after double slashes (like in C++).

> `// comment`


## Key <a name="key"></a>

The key is the main field in an entry. The key specifies exactly how the chain is searched for. A single entry can have several keys. The text of the "key" field is enclosed in quotation marks. Words are separated by spaces. It is not case-sensitive.

> `key = "santa claus"`


### Key variations (|) <a name="variantykljucha"></a>

The | symbol (<q>or</q>) separates multiple variations of the key.

> `key = "someone" | "something"`


### Searching for the exact match (!) <a name="poiskpotochnojjforme"></a>

The `!` symbol (<q>exact match</q>) tells the gazetteer that the found word must match the exact form of the word specified in the key. The `!` symbol only applies to the word that it precedes.

> `key = { "!god !forbid" }`

The tag `morph = EXACT_FORM` is equivalent to placing <q>!</q> before every word in the key. For example:

> `key = { "god forbid" morph = EXACT_FORM }`


### Reference to another entry ($) <a name="ssylkanadrugujustatju"></a>

The entry that this key references must be described earlier. The entry name must be specified in full; you cannot reference a group of entries by putting their shared prefix before the slash symbol /. It is important to understand that information about main words is lost when an entry is passed to a key this way.

> `key = “wild $animal_name”`


### Register (Case = UPPER) <a name="registrcaseupper"></a>

This means that the value of the entry's key must be in uppercase letters. For example:

> `key = { "only in uppercase" Case = UPPER }`


### Entry head (mainword) <a name="glavnoeslovostatimainword"></a>

This tag specifies which of the words in the key is the main word (head) for the entry. Grammatical information from this word is appended to the chain that the entry is describing. Words are numbered starting from 1. For example:

> `key = { "wild dog" mainword = 2 }`

If "mainword" is defined outside of the key, it applies to all the keys for this entry. For example:

> ```no-highlight
> key = "wild dog"
> key = "wild cat"
> mainword = 2
> ```


### Chain substitution (lemma) <a name="lemma"></a>

The `lemma` field specifies a value to replace the chain defined by the entry. This substitution occurs in the resulting chain, as well as in the facts where this chain is used. For example:

In the `lemma` field we can set the parameters `always` and `indeclinable`:

\* If the value of `always = 1`, the chain defined by the entry is replaced with the value of the `lemma` field, even if the entry was not used for generating the chain that it is included in. By default, `always = 0`.

\* If the value of `indeclinable = 1`, during lemmatization, the value of the `lemma` field does not change. By default, `indeclinable = 1`.


## Grammatical tags in key descriptions <a name="grammaticheskiepometyvopisaniikljuchejj"></a>

### gram tag <a name="pometagram"></a>

The values of this field are grammemes that are applied to the key (see the complete list of grammemes in the section [Grammeme values](grammemes-values.md)). For example, gram=sg means that only singular forms of the key match the entry. For example:

> `key = { "table" gram=sg }`


#### word tag <a name="pometaword"></a>

The "word" tag indicates which word the "gram" tag applies to in a multiword key. If the "word" tag is omitted, the "gram" tag is applied to the entire key. For example:

> `key = { "pravo potrebitel'" gram = {"pl", word=1} gram = {"gen", word=2} }`


### Agreement (arg tag) <a name="soglasovaniepometaarg"></a>

This tag describes agreement between two words in a key. There are two possible types of agreement:

\* in gender, number, and case: `agr=gnc_agr` or `agr=GENDER+NUMBER+CASE`

\* in case: `agr=CASE`

> ```no-highlight
> key = { "avtonomniy oblast'" | "avtonomniy okrug" agr=gnc_agr } ("autonomous region" entered as masculine singular adjective, feminine singular noun and "autonomous district" entered as masculine singular adjective, masculine singular noun)
> key = { "glavniy redaktor" agr=GENDER+NUMBER+CASE } ("head editor" entered as masculine singular adjective, masculine singular noun)
> key = { "vooruzhenniy sila" gram="pl" agr=CASE } ("armed forces" entered as masculine singular adjective, plural noun)
> ```


## geopart tag (for internal use) <a name="pometageopart"></a>

Reference to the parent entry in the geothesaurus.

> ```no-highlight
> message "monte_carlo"
> {
>   key = { "monte-carlo" | "monte carlo" }
>   lemma = "monte carlo"
>   geopart = "geo_monaco"
> }
> ```


## Special key types <a name="specialnyetipykljuchejj"></a>

There are two special types of keys:

\* `type = CUSTOM` for integrating grammars and built-in parser algorithms in the glossary

\* `type = FILE` for files with lists of words and phrases

> \* `key = { "alg:fio" type=CUSTOM }` — References the C++ algorithm for detecting full names.
> 
> \* `key = { "tomita:geo/city.cxx" type=CUSTOM}` — References a grammar. Either a full path or relative path may be specified.
> 
> \* `key = { "animals.txt" type=FILE }` — References a file with a word list.

