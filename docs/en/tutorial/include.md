# Including grammars (kwtypes)

Finding dates is a common task when extracting facts from a text. To avoid writing the same rules over again every time, we can include a previously written grammar in other grammars.

Let's say that our poem about the sparrow doesn't just talk about who he dined with, but also specifies when it happened:

> **(13)** — At the zoo, with the animals.
> 
> — Sparrow, where did you dine?
> 
> First I had lunch
> 
> With the lion, behind bars.
> 
> Had a snack with the fox on August 14th.
> 
> Had a drink with the walrus on September 1st.
> 
> Ate a carrot with the elephant on 29 December 2011.
> 
> Ate some wheat with the crane.
> 
> and so on.

We remember that we already have a grammar that extracts exact dates, and a grammar that extracts information about who dined with the sparrow. To find out when and with whom the sparrow dined, we just need to include one grammar in the other. There are two ways to do this:

1. Using the "include" directive.
    ```no-highlight
    #encoding "utf-8"
    #include <date.cxx>
    Animal -> Noun<kwtype=animal>; 
    WithWho -> 'at' (Adj<gnc-agr[1]>) Animal<gram='gen', rt, gnc-agr[1]> interp (Sparrow.Who);
    WithWho -> 'with' (Adj<gnc-agr[1]>) Animal<gram='ins', rt, gnc-agr[1]> interp (Sparrow.Who);
    S -> Date interp (Sparrow.When) WithWho;
    S -> WithWho Date interp (Sparrow.When);
    ```
    The "include" directive includes the text from the specified file in its place. So the parser <q>sees</q> this text:
    ```no-highlight
    #encoding "utf-8"
    // grammar for extracting dates from date.cxx
    DayOfWeek -> Noun<kwtype="week_day">;
    Day -> AnyWord<wff=/([1-2]?[0-9])|(3[0-1])/>;
    Month -> Noun<kwtype="month">;
    YearDescr -> "year" | "г. ";
    Year -> AnyWord<wff=/[1-2]?[0-9]{1,3}г?\.?/>;
    Year -> Year YearDescr;
    Date -> DayOfWeek interp (Date.DayOfWeek) (Comma) Day interp (Date.Day) Month interp (Date.Month) (Year interp (Date.Year)); 
    Date -> Day interp (Date.Day) Month interp (Date.Month) (Year interp (Date.Year));
    Date -> Month interp (Date.Month) Year interp (Date.Year);
    //main grammar
    Animal -> Noun<kwtype=animal>; 
    WithWho -> 'at' (Adj<gnc-agr[1]>) Animal<gram='gen', rt, gnc-agr[1]> interp (Sparrow.Who);
    WithWho -> 'with' (Adj<gnc-agr[1]>) Animal<gram='ins', rt, gnc-agr[1]> interp (Sparrow.Who);
    S -> Date interp (Sparrow.When) WithWho;
    S -> WithWho Date interp (Sparrow.When);
    ```
    So the include directive is the same as copying the text from one grammar and putting it in the other one.
1. Using gazetteer kwtypes.
    Recall that each grammar has a corresponding entry in the root dictionary. In essence, this kind of entry contains all the chains that the given grammar builds, which means we can use one grammar inside another with the help of the kwtype tag. It will look like this:
    ```no-highlight
    #encoding "utf-8"
    Date -> AnyWord<kwtype='**date**'>;  // using the "date" entry from the dictionary
    Animal -> Noun<kwtype=animal>; 
    WithWho -> 'at' (Adj<gnc-agr[1]>)
               Animal<gram='gen', rt, gnc-agr[1]> interp (Sparrow.Who);
    WithWho -> 'with' (Adj<gnc-agr[1]>)
               Animal<gram='ins', rt, gnc-agr[1]> interp (Sparrow.Who);
    S -> Date interp (Sparrow.When) WithWho;
    S -> WithWho Date interp (Sparrow.When);
    ```
    In this example, the Date nonterminal matches any chain built by the date.cxx grammar (which is located in the "date" entry in the root dictionary), such as 20 August 2012.


The result will be the same in both the first case and the second case.

The difference between the two methods of including the grammar is that in the second case, first the chains that are tagged with kwtypes are built, and only then the chains are built that are defined in the rules of the grammar itself. This means that in the second case, the grammar gets the following text as input:

> **(14)** — Sparrow, where did you dine?
> 
> — At the zoo, with the animals.
> 
> First I had lunch
> 
> With the lion, behind bars.
> 
> Had a snack with the fox on August_14th.
> 
> Had a drink with the walrus on September_1st.
> 
> Ate a carrot with the elephant on 29_December_2011.
> 
> Ate some wheat with the crane.

August_14th, September_1st, and 29_December_2011 are now indivisible entities that function like a single word in the text. The grammar no longer distinguishes the separate words that they are made up of. These units inherit the morphological characteristics of the main word in the chain that they were made from.

This is true not only for kwtypes that are based on a grammar, but for all others as well. For example, if a kwtype references an entry with multiword keys, first these multiword keys are combined into an indivisible unit, and then the grammar rules are applied. So if we have an entry like this in the dictionary:

```no-highlight
animal "elephant"
{
    key = "african elephant" 
}
```

And the following line is given as input:

> **(15)** Ate a carrot with the african elephant on 29 December 2011.

After applying kwtypes, our grammar will get the following text:

> **(16)** Ate a carrot with the african_elephant on 29_December_2011.

And this is the text that the parser will process using the rules.


## Source files for the tutorial5 project <a name="isxodnyefajjlyproektatutorial5"></a>

* `tutorial5/config.proto` — parser configuration file.
* `tutorial5/kwtypes.proto` — declaration of the `animals` kw type.
* `tutorial5/facttypes.proto` — fact type descriptions.
* `tutorial5/mydic.gzt` — root dictionary.
* `tutorial5/animals_dict.gzt` — dictionary with animal names.
* `tutorial5/mammals.txt` — list of mammals.
* `tutorial5/main.cxx` — main grammar.
* `tutorial5/date.cxx` — grammar for dates.
* `tutorial5/test.txt` — the text <q>Where the sparrow dined ...</q> with dates.


