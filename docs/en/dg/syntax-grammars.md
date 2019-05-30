# Grammar syntax

The grammar file consists of two parts: a section with directives that affect how the grammar works in general, and a set of rules.


## Directives <a name="directives"></a>

Directives start with the `#` symbol and end with a line break. Some directives are identical in meaning with similar directives in the C++ preprocessor.

### #encoding

Indicates encoding for this grammar file. Default encoding: `utf-8`. Encoding is set in quotation marks.

> `#encoding “windows-1251”;`

### #include

Includes the text of a different grammar in this grammar. The grammar filters are combined, and the root of the included grammar (see #grammar_root) is ignored. The name of the grammar to include is set in quotation marks.

> `#include “small_grammar.cxx” ;`

### #GRAMMAR_ROOT

The `#GRAMMAR_ROOT` directive specifies the nonterminal that is the root of this grammar. The grammar root does not have to be explicitly defined if the grammar only has one nonterminal, which is never found on the right side of the grammar rules.

> `#GRAMMAR_ROOT MainRule;`

### #GRAMMAR_KWSET <a name="GRAMMAR_KWSET"></a>

The `#GRAMMAR_KWSET` directive is for explicitly defining names or types of gazetteer entries, whose keys that are found in sentences should be passed to the parser as terminals. From the perspective of the current grammar, these chains become multiwords (see step 2 in the parser [algorithm](overview.md)). In particular, the `#GRAMMAR_KWSET` directive can be used for more precisely defining the value of the [kwtype=none](labels-limits.md#kwtype-none) tag. See the usage example [here](unobvious-solutions.md).

The syntax for describing entries is the same as the syntax for the `kwset` tag: a comma-separated list inside square brackets `[ ]`.

> `#GRAMMAR_KWSET [“kitties”, cat_types];`

### #filter

Filters can make grammars work faster. If an input sentence does not match any of the declared filters, the grammar is not run on them.

A filter is set as a sequence of terminals. A sentence passes through the filter if it contains the words described by these terminals, in the specified order. The distance between the words does not matter.

The ampersand symbol `&` is placed before the names of terminals in filters. Terminals can have their normal tags: `kwtype`, `wff` and others. Operators (`+`, `*` and others) cannot be used in filters. The maximum allowed distance between terminals can be set in square brackets `[]`.

If another grammar is included in this grammar using the `#include` directive, the filters in the included grammar are also taken into account.

> `#filter &amp;Word<kwtype=fio> &amp;AnyWord<wff=".*\d.*",h-reg1> [10] &amp;Hyphen;`

### Declaring substitution: #define, #undef <a name="ad-substitution"></a>

The `#define` directive is also used for implementing substitution. In this case, the directive has two arguments: the substitution name and value. The substitution name can be any sequence of letters, numbers and underscore symbols, but the first character of a substitution name cannot be a number. The substitution value can be any complete expression in the syntax that is allowed on the right side of the grammar's rules: a list of grammemes, a chain of nonterminals with constraints and interpretations, and so on. The linebreak symbol indicates the end of the substitution value.

After it is declared, the substitution name can be used in curly brackets with the dollar sign before it: `${ … }`. Spaces are not allowed between the `$` sign, the curly brackets, and the substitution name.

Keep in mind that the substitution is not literally "substituted" (like in C++, for example), but is in itself a complex token in the grammar's text. Therefore, substitution cannot be used inside a string literal (it just won't be recognized), or inside identifiers or other long tokens.

> ```no-highlight
> #define ALL_CASES [nom,acc,gen,dat,ins,loc]
> BetterStatus -> PostStatusCoord<gnc-agr[1]> 
>                 FIO<rt,gnc-agr[1],GU=${ALL_CASES}>;
> BetterStatus -> PostStatusCoord<gram='nom'>
>                 FIO<rt,GU=&amp;${ALL_CASES}>;
> #undef ALL_CASES
> ```

> ```no-highlight
> #define HERO_WITH_INTERP Hero<rt,gram='sg,nom'> interp (HeroMaybe.Fio from Fio)
> Maybe -> ${HERO_WITH_INTERP} 
>          TellVerb<gram='3p,sg'> ToSomeone Word;
> Maybe -> ${HERO_WITH_INTERP} 
>          ToSomeone TellVerb<gram='3p,sg'> Word;
> #undef HERO_WITH_INTERP
> ```

All names that are defined using `#define` apply to the rest of the grammar's text, even for all files that are included later. Substitution can be canceled by specifying its name after the `#undef` directive. We recommend always explicitly canceling substitutions, to avoid any unexpected effects from including one grammar in another one.

### #NO_INTERPRETATION

Prohibits interpretation within the framework of the current grammar. All `interp` operators stop working.

