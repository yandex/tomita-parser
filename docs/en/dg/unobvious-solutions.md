# Useful solutions for common problems

## Excluding bad triggers <a name="wrong-responce"></a>

There is often a problem when there is a grammar that extracts something like dates. The most obvious frequent exceptions must be eliminated, such as "March 8th Street".

These names can be listed in the gazetteer by adding them as

keys for the entry called "bad_dates".

```no-highlight
Some_type "bad_dates"
{ 
    key = "!march 8th street"
}
```

And in the date grammar, refer to this entry in the #GRAMMAR_KWSET

directive.

`#GRAMMAR_KWSET [“bad_dates”]`

In this case, before the parser runs the grammar on the text <q>house on March 8th Street</q>, it converts the subchain <q>March 8th Street</q> to a single multiword, and it will no longer see the separate words <q>8th</q> and <q>March</q>.

To solve the same problem without creating a hard list of all bad occurrences of dates in proper names, we can describe them in the grammar instead. Then the key for the <q>bad_dates</q> entry will already have a link to the grammar.

```no-highlight
Some_type "bad_dates"
{ 
    key = { "tomita:bad_dates.cxx" type=CUSTOM }
}
```


## Describing only variable nouns <a name="opisattolkoizmenjaemyesushhestvitelnye"></a>

To do this, use the `GU` tag to forbid nouns that have all the cases at once.

`Noun<GU=~[nom,acc,dat,instr,loc,gen]>`


## Describing classes of words with regular word formation <a name="opisatklassyslovsreguljarnymslovoobrazovaniem"></a>

Sometimes it's not convenient to list words in a dictionary when they are all formed in a standard way. For example, `,`, and so on ("in Spanish", "in French"; the "po-" prefix and "ski" ending are used the same way with all languages in Russian). Such cases are easy to describe using regular expressions.

`S -> Word<wfm=/po-.*ski>;`

