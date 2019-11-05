# Basic concepts

The Tomita parser makes it possible to use custom user patterns ([CFGs, or context-free grammars](http://ru.wikipedia.org/wiki/%D0%9A%D0%BE%D0%BD%D1%82%D0%B5%D0%BA%D1%81%D1%82%D0%BD%D0%BE-%D1%81%D0%B2%D0%BE%D0%B1%D0%BE%D0%B4%D0%BD%D0%B0%D1%8F_%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D0%B0%D1%82%D0%B8%D0%BA%D0%B0)) to extract chains of words or facts from a text and separate them into fields. For example, you can write patterns for extracting addresses. An address is a fact, and its fields are <q>city name</q>, <q>street name</q>, <q>house number</q>, and so on.

The parser includes three standard linguistic processors: a tokenizer (separates text into words), a segmenter (divides text into sentences), and a morphologic analyzer ([mystem](http://h.yandex.net/?http%3A%2F%2Fcompany.yandex.ru%2Ftechnologies%2Fmystem%2F)).

The main parser components are the [gazetteer](gazetteer-syntax.md), [a set of CFGs](syntax-grammars.md), and a set of [fact type descriptions](interpretation.md) that are created from the grammars as the result of the [interpretation](interpretation.md) process.

The **gazetteer** is a dictionary of keywords that are used for analysis with the CFGs. Each entry in this dictionary defines a set of words and phrases that share a common characteristic. For example, we could have the entry <q>all Russian cities</q>. Then the property <q>is a Russian city</q> could be used in a grammar. Words and phrases can be defined either using an explicit list, or <q>functionally</q>, by indicating a grammar that describes the appropriate chains. For example, a chain of <q>address</q> keywords is described by a corresponding grammar and can be used in a grammar to locate city events. This is discussed in more detail in the description of the cascading mechanism.

A **grammar** is a set of rules in the CFG language that describe the syntactic structure of chains to extract. The grammatical parser is always run on a single sentence. Before the parser is launched, the grammar's terminals are mapped to the words in the sentence, or to phrases (these are explained separately later). A single word can correspond to multiple terminal symbols. As a result, the parser input is a succession of various terminal symbols. For example, let's say we have a grammar with just the two terminals `Verb` and `Noun`, and the Russian input sentence `(literally "mama washed windows")`. In this case, the parser gets this sequence as input: `{Noun}, {Verb, Noun}, {Verb, Noun}`. The resulting output is chains of words that were detected using this grammar.

**Facts** are tables with columns, which are called fact fields. Facts are filled in while the parser is analyzing a sentence. Each individual grammar specifies how and when to fill in the fact fields. This is called [interpretation](interpretation.md). Fact types are described in a special language in a separate file.

* [How it works](overview.md)
* [Grammar syntax](syntax-grammars.md)
* [Rules](rules.md)
* [Syntactic tags](syntax-labels.md)
* [Constraint tags](labels-limits.md)
* [List of all tags](all-labels-list.md)
* [List of terminals](terminals-list.md)
* [Grammeme values](grammemes-values.md)
* [Interpretation and normalization](interpretation.md)
* [Gazetteer syntax](gazetteer-syntax.md)
* [Relationship between the gazetteer and grammar](link.md)
* [Useful solutions for common problems](unobvious-solutions.md)
* [Debugging grammars](debug-grammar.md)
* [Example of how the parser works](example.md)
* [Launching the parser and the configuration file](run-parser.md)