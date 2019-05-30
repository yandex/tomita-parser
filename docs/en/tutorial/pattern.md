# Regular expressions

Sometimes when working with text it is helpful to use templates, or [regular expressions](http://ru.wikipedia.org/wiki/%D0%A0%D0%B5%D0%B3%D1%83%D0%BB%D1%8F%D1%80%D0%BD%D1%8B%D0%B5_%D0%B2%D1%8B%D1%80%D0%B0%D0%B6%D0%B5%D0%BD%D0%B8%D1%8F). For example, you can use templates to learn how to extract a character's birthdate from text. Texts often use the pattern "X was born in YEAR" to express this information, where X is the name of the biographical character. We already know how to extract person names, and the word "born" can be set as a lemma with the appropriate grammatical information, but we will need to write a regular expression to set the date:

```no-highlight
#encoding "utf-8"
ProperName -> Word<h-reg1, gram='persn'>
              Word<h-reg1, gram='patrn'>
              Word<h-reg1, gram='famn'>;
S -> ProperName 'born'<gram='praet'> 'in'
     AnyWord<wff=/[1-2]?[0-9]{1,3}г?\.?/> ('year' <gram='sg, dat'>);
```

Regular expressions are usually written in the "wff" field (for details, see the [list of tags](../dg/all-labels-list.md)), and the syntax is almost the same as the syntax for regular expressions in Perl.

Using the rule we have written, we can take the text

> **(10)** Ivan Ivanovich Ivanov was born in the year 1875 on his father's estate in a small town.

and easily get the details on Ivan Ivanovich's biography.


## Source files for the tutorial3 project <a name="isxodnyefajjlyproektatutorial3"></a>

`tutorial3/config.proto` | Parser configuration file
`tutorial3/mydic.gzt` | Root dictionary
`tutorial3/fioborn.cxx` | Grammar
`tutorial3/test.txt` | Text


The config file specifies that the text to analyze is in the file test.txt, so the project runs in the normal way.

