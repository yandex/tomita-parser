# Keyword types (kwtype)

There is an old children's rhyme about a sparrow:

> **(11)** — Sparrow, where did you dine?
> 
> — At the zoo, with the animals.
> 
> First I had lunch
> 
> With the lion, behind bars.
> 
> Had a snack with the fox.
> 
> Had a drink with the walrus.
> 
> Ate a carrot with the elephant.
> 
> Ate some wheat with the crane.
> 
> Visited the rhinoceros,
> 
> Had some bran.
> 
> I went to a feast
> 
> With the long-tailed kangaroo.
> 
> I stopped by a party
> 
> With the shaggy bear, too.
> 
> But the toothy crocodile
> 
> Almost swallowed me in his smile.

From the first two lines, we know that the sparrow ate with some animals who live at the zoo. Let's try to write a simple rule system that we can use to find out exactly which animals the sparrow dined with.

For this grammar, we will need both grammatical and semantic information about the words from the text. We need to make a mini-dictionary.

We introduced the dictionary in the very beginning, when we created the root file `mydic.gzt`. Now we will examine this format in more detail.

In Tomita, a dictionary is called a gazetteer and is written in a separate file with the `gzt` extension. Like any dictionary, a gazetteer consists of entries. Let's create the file `animals_dict.gzt` and write a simple entry in it, where we list some animals:

```
encoding "utf8";
TAuxDicArticle “animals”
{
    key = "dog" | "cat" | "horse" | "cow" | "lion" | "elephant" | "wolf" | 
          "kangaroo" | "crocodile" 
}
```

In this example, `TAuxDicArticle` is the entry type, and <q>animals</q> is the entry name, while the `key` field lists all the words that are included in this entry.

The created dictionary must be imported to the root dictionary. To do this, add the following line to the `mydict.gzt` file:

`import "animals_dict.gzt";`

Now we can reference the created entry from the grammar. To do that, we use the `kwtype` and `kwset` tags with the entry type or name as the value. The `TAuxDicArticle` type is the default type and is used in many entries, so we reference a unique name:

```no-highlight
#encoding "utf-8"
S -> 'at' (Adj<gnc-agr[1]>) Noun<kwtype='animals', gram='gen', rt, gnc-agr[1]>;
S -> 'with' (Adj<gnc-agr[1]>) Noun<kwtype='animals', gram='ins', rt, gnc-agr[1]>;
```

If we run this grammar on the poem about the sparrow, we get the following subchains:

```no-highlight
у лев
у слон
у хвостатый кенгуру
```

Naturally, the completeness of the result directly depends on the completeness of the dictionary: in our example, only the animals that we included in the dictionary were extracted.

There are a lot of animal names, and listing them all in the dictionary is not very convenient. But in the gazetteer, an entry can reference a text file that lists all the words that should be included in this entry. Let's rewrite our dictionary to take advantage of this feature:

```no-highlight
TAuxDicArticle “animals”
{
    key = { "animals.txt" type=FILE }
}
```

The "key" field should specify the name of the text file and key type — FILE. The animals.txt file lists the names of animals:

```no-highlight
dog
cat
horse
cow
lion
elephant
wolf
kangaroo
crocodile
```

We can use the gazetteer to solve more difficult tasks, such as finding out what kind of birds the sparrows dine with. To do this, we need to separate the animals into various entries depending on their class:

```no-highlight
TAuxDicArticle "mammals"
{
    key = "dog" | "cat" | "horse" | "cow" | "lion" | "elephant" | "wolf" | 
          "kangaroo" | "walrus" | "fox" | "rhinoceros" | "bear" 
}
TAuxDicArticle "birds"
{
    key = "sparrow" | "crane" | "penguin"
}
TAuxDicArticle "reptiles"
{
    key = "turtle" | "crocodile"
}
```

To preserve the information that the mammals, birds, and reptiles are all animals, it makes sense to replace the standard entry type TAuxDicArticle with our own type, such as "animal". Entry types are defined in a special format in a separate file. We'll create the kwtypes.proto file and enter the following in it:

```no-highlight
import "base.proto";
import "articles_base.proto";
message animal : TAuxDicArticle {}
```

The first two lines import the built-in parser files `base.proto` and `article_base.proto` the same way as this is done in the root dictionary, and the `animal` type is defined in the last line as a derivative of the TAuxDicArticle basic type. All entry types must be derived from TAuxDicArticle. Creating custom entry types lets you combine multiple entries in a group, so that later you can use all of them by just specifying their type using the `kwtype` tag (for example, `kwtype=animal`), without listing each of them separately.

Now we can rewrite the gazetteer using the new class:

```no-highlight
encoding "utf-8";
import "kwtypes.proto"; //importing the file where we defined the entry types used in the dictionary
animal "mammals"
{
   key = "dog" | "cat" | "horse" | "cow" | "lion" | "elephant" | "wolf" | 
          "kangaroo" | "walrus" | "fox" | "rhinoceros" | "bear" 
}
animal "birds"
{
    key = "sparrow" | "crane" | "penguin"
}
animal "reptiles"
{
    key = "turtle" | "crocodile"
}
```

