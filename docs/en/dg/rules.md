# Rules

Grammars for Tomita parser consist of rules. Each rule has a left and right part, divided by the `—>` symbol. On the left side, there is a single nonterminal (`S` in the example below). On the right side, there is a list of terminals or nonterminals (`S1 ... Sn`), followed by conditions (`Q`) that are applied to all the rules as a whole. Conditions may be omitted. A rule ends with a semicolon (`;`).

## Overview of rules in grammars

`S —> S1 ... Sn { Q };`

For any nonterminal mentioned on the right side of the rule, there must be a rule in which this nonterminal is located to the left of `—>`. The order for listing rules in the grammar does not matter, meaning it does not affect how this grammar is applied to a text.

If the input text contains a chain that matches the right side, the rule is <q>triggered</q> and the grammar uses this chain as the value of the `LeftPart` symbol.

> ```no-highlight
> MoscowWord —> "moskva"<h—reg1>;
> MoscowGroup —> Adj<gnc—agr[1]>* MoscowWord<rt,gnc—agr[1]>;
> ```
> 
> Takes the text `"Krasnaya Moskva"` ("Red Moscow"; singular feminine nominative adjective with the singular feminine nominative city name).
> 
> The symbols S  in the right part of the rule are separated by "or" operators `|` or spaces, and can also include the unary operators *, + and ().


## Operators <a name="operators"></a>

### | operator

The `|` operator is used for shortening the entry of rules with the same left part. The space has higher priority than `|`. The following example illustrates the use of the `|` operator.

> ## Example without the `|` operator
> 
> ```no-highlight
> NP —> Noun;                              // rule 1: a nominal group can consist                                         //            of a single noun
> NP —> Adj<gnc—agr[1]> Noun<gnc—agr[1]>;  // rule 2: a nominal group can include
>                                          //            an adjective and noun in agreement
> NP —> Noun Noun<gram="gn">;              // rule 3: a nominal group can consist of two
>                                          //            nouns, and the second is in the genitive case
> ```

> ## Example with the `|` operator
> 
> `NP —> Noun | Adj<gnc—agr[1]> Noun<gnc—agr[1]> | Noun Noun<gram="gn">;`

### \* operator

The `*` operator (asterisk) after a (non)terminal indicates that the symbol is repeated zero or more times. This means that the right part of the rule must always have a (non)terminal without the * operator, since rules with an empty right half are forbidden in Tomita.

> ```no-highlight
> NounGroup —> Noun Noun<gram="gen">*;
> //The parser's preprocessor transforms this rule like this:
> ARTIFICIAL_Noun —> Noun<gram="gen">;
> ARTIFICIAL_Noun —> ARTIFICIAL_Noun  Noun<gram="gen">
> NounGroup —> Noun ARTIFICIAL_Noun<gram="gen">;
> NounGroup —> Noun;
> ```

The `*` operator is used for defining agreement between copies of a (non)terminal. Agreement is written after the `*` operator in square brackets `[]`.

> `NounGroup —> Noun Noun<gram="gen">*[gn—agr];`

### + operator

The `+` operator after a (non)terminal indicates that the symbol is repeated one or more times. The `+` operator also allows to define agreement between copies of a (non)terminal.

> ## Example
> 
> `Adjectives —> Adj+[gnc—agr];`
> 
> This rule is the same as the following:
> 
> `Adjectives —> Adj<gnc—agr[1]> Adj<gnc—agr[1]>+;`

### Symbols in the right part of a rule

The `Si` symbols in the right part consist of three parts: `N <P1,…,Pn> interp (I1;…;In)`, where `N` is the name of the terminal or nonterminal, `Pi` are [constraint tags](labels-limits.md) on properties of the terminal/nonterminal `N`, and `Ii` is the name of the field and fact where, during [interpretation](interpretation.md), the word chain is written that is appropriate for the nonterminal `N`. The only mandatory part is the name of the terminal/nonterminal `N`. Names of nonterminals can consist of Latin letters, numbers and underscores `_`, must start with a letter, and cannot coincide with reserved names of [terminals](terminals-list.md).

Each `N` symbol corresponds to a single word (if it is a terminal) or group of words (if it is a nonterminal), which have a large number of different properties: grammatical characteristics, register, belonging to a pre-defined word set, and so on. The `Pi` constraints are for managing these properties, by narrowing the word sets that the N symbol can match.

See [Constraint tags for terminals and nonterminals](all-labels-list.md).

### Operations with rules

Tag | Semantics | Usage example
----- | ----- | -----
`outgram` | The field of the `outgram` tag has an array of grammemes that is appended to the symbol in the left part of a rule. | ```no-highlight ForeignWord —> Word<lat>                {outgram = 'nom,acc,gen,loc,dat,ins'}; ```
`count` | The field of the `count` tag is used for entering the maximum number of terminals that can include the symbol in the left part of a rule. The number of terminals must be fewer than the specified value. | ```no-highlight S —> NP {count = 10}; ```
`weight` | The field of the `weight` tag is used for entering the weight (a number with a value from 0 to 1) that is assigned to the symbol in the left part of a rule. The usage of weighting for rules is described in detail in ($). | ```no-highlight NP —> Adj Noun {weight = 0.7}; ```
`trim` | The `trim` tag indicates that for an extracted chain, any terminals that do not occur in the facts that were assembled during interpretation are thrown out. | ```no-highlight S —> NP {trim}; ```
`not_hreg_fact` | If a group that was assembled by a rule with the constraint `not_hreg_fact` contains only uppercase words for the values of the filled-in fact fields, analysis of such a tree is declared unsuccessful. | ```no-highlight S —> NP {trim, not_hreg_fact}; ```


### Quotation marks in rules

Double and single quotation marks, `" "` and `' '`, terminate strings in the right part of rules. A word in quotation marks must be a lemma. If a word must be restricted to an oblique form, the `gram` tag can be used.

In addition, articles with Cyrillic names must be enclosed in double quotes `" "`.

> ## Example
> 
> ```no-highlight
> CatSlangWord —> "котэ";
> CatSlangWord —> Word<kwtype="котэ">;
> ```

