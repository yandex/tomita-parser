# Syntactic tags

## rt tag <a name="pometart"></a>

A terminal or nonterminal with the `rt` tag (root) becomes the syntactic head of the subtree that corresponds to the given rule. This means that the nonterminal in the left part of the rule gets the grammatical and other characteristics of the (non)terminal with the `rt` tag. There cannot be more than one (non)terminal with the `rt` tag in the right part of a rule. By default, the `rt` tag is assigned to the leftmost (non)terminal in the right part of the rule.


## Value of the ${ } substitution <a name="znacheniepodstanovki"></a>

The combination `${ }` with a substitution name in curly brackets tells the preprocessor to replace the substitution name with its value. For more details about substitution, see [Declaring substitutions: #define and #undef](syntax-grammars.md#ad-substitution).


## Comments <a name="kommentarii"></a>

Single-line and multi-line comments can be used, like in C++.

Single-line comments are preceded by double slashes `//` and end at the end of the line. They can start at the end of any grammar text or at the beginning of a line. Comments cannot be put inside meaningful grammar text.

> ## Example
> 
> `// This is a single-line comment`

The start of a multi-line comment is preceded by the `/*` sign and ends with the `*/` sign.

> ## Example
> 
> ```no-highlight
> /* This is a multi-line
>      comment
>   */
> ```

