# How it works

## Algorithm for parsing a single sentence with a single grammar <a name="algoritm"></a>

1. The parser searches for all occurrences of all the keys from the gazetteer. If a key consists of multiple words (such as the city name "Nizhny Novgorod"), a new artificial word is created that we call a "multiword".
1. From all the gazetteer keys found, it selects only the ones that are mentioned in the grammar (see the [kwtype tag](labels-limits.md)).
1. Some of the selected keys may be multiwords that overlap each other or include single keywords as their parts. The parser tries to maximally cover the sentence with non-intersecting keywords, so that as many tokens from the sentence as possible are covered by them.
1. A linear chain of words and multiwords is input to the GLR parser. The grammar's terminals are mapped to the input words and multiwords.
1. The GLR parser constructs all the possible variants for the set of terminals. From all the constructed variants, it selects the ones with maximal coverage for the sentence.
1. Then the parser executes the [interpretation](interpretation.md) procedure on the assembled syntactic tree. It filters out specially marked subnodes, and tokens that match them are entered in the fact fields generated from the grammar.



## Source files for a simple project <a name="source-files"></a>

To run the Tomita parser, you must create the files listed in the following table.

**Content** | **Format** | **Notes**
----- | ----- | -----
**config.proto** — Parser configuration file. It tells the parser where to find all the other files, how to interpret them, and what to do. | Protobuf | Always required.
**dic.gzt** — Root dictionary. Contains a list of all the dictionaries and grammars used in the project. | Protobuf / Gazetteer | Always required.
**mygram.cxx** — Grammar. | Grammar description language. | Necessary if grammars are used in the project. There can be several of these files.
**facttypes.proto** — Description of fact types. | Protobuf | Necessary if facts are generated in the project. The parser will run without it, but there will not be any facts.
**kwtypes.proto** — Description of keyword types. | Protobuf | Necessary if new keyword types are created in the project.


So the minimal set of files for running the parser includes just the configuration file and the root dictionary. However, grammars and custom keyword types will not be used, and facts will not be generated. The typical parser usage scenario assumes that all five types of files are available.

### Cascading

The results of other grammars can be used as keywords. At step 2 of the previous algorithm, the parser checks which keywords are set by specific grammars, instead of by a word list (see [special types of keys](gazetteer-syntax.md)). If such keys are present, it executes the same algorithm recursively on all the grammars that are referenced in entries for these keywords. The chains extracted by these grammars become multiwords and are mixed in with the rest of the keywords. This means that entire trees of parsers can be created by submitting the results of one of them as input symbols for another one.

### Addressing

The primary element for the parser is a [gazetteer entry](gazetteer-syntax.md). The entry can be thought of as a function on sentences that always results in an extracted subchain. So the [entry name](gazetteer-syntax.md) is the name of this function, and the [entry type](gazetteer-syntax.md) is a set of functions applied in random order. Inside this entry-function there is an explicit definition of how to extract the necessary chain: either using a grammar, or searching for specified words and phrases. Interaction between grammars occurs via gazetteer entries, in which the key [specifies a grammar instead of words](link.md). The names of gazetteer entries are also specified in the [parser parameters](run-parser.md), meaning it invokes functions that extract subchains.

### Weighting

The parser can generate many variations for the same chain, which differ only in their analysis tree. Different trees can generate different results for populating fact fields. For this purpose, a rule can be assigned a weight (see the description of the `weight` tag in the section [Operations with rules](rules.md#weight)). By default, all rules have a weight equal to 1. But this weight can be artificially reduced so the parser will select a variation with a heavier weight, if one has been assembled.

