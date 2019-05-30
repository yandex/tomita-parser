# Launching the parser and the configuration file

The name of the configuration file is the only argument for the "tomitaparser" program. I.e., it should be launched like this:

```no-highlight
In Linux, FreeBSD and other *nix systems:
./tomitaparser config.proto
In Windows:
tomitaparser.exe config.proto
```

The configuration file specifies:

- Where to find texts to process.
- The path to the root dictionary.
- Which entries from the root dictionary to run.
- Which facts to save and in which format.
- Where to save facts.
- Several auxiliary parameters.



## Description of the config file format <a name="opisanieformatakonfiguracionnogofajjla"></a>

```no-highlight
message TTextMinerConfig {
  message TInputParams {
    enum SourceFormat {
      plain = 0;
      html = 1;
    }
    enum SourceType {
      no = 0;
      dpl = 1;
      mapreduce = 2;
      arcview = 3;
      tar = 4;
      som = 5;
      yarchive = 6;
    }
    optional string File = 1 [default = ""];            // If File is skipped, then STDIN
    optional string Dir = 2 [default = ""];             // If reading files from a folder
    optional SourceFormat Format = 3 [default = plain]; // If Format is skipped, then plain

    optional SourceType Type = 4;                       // If Type is skipped, then read the normal file (the same as "no")
    optional string Encoding = 5 [default = "utf8"];    // Input data encoding (if omitted, then utf8)
    optional string SubKey = 6;                         // For mapreduce
    optional int32 FirstDocNum = 7 [default = -1];      // For arcview: the first document to analyze
                                                        // (if omitted, start from the beginning of the archive)
    optional int32 LastDocDum = 8 [default = -1];       // For arcview: the last document to analyze
                                                        // (if omitted, to the end of the archive)
    optional string Url2Fio = 9;                        // Table url2fio for interview
    optional string Date = 10;                          // For formats other than arcview
  }
  message TOutputParams {
    enum OutputMode {
      append = 0;
      overwrite = 1;
    }
    enum OutputFormat {
      xml = 0;
      text = 1;
      proto = 2;
      mapreduce = 3;
    }
    optional string File = 1 [default = ""];            // If omitted, then STDOUT
    optional OutputMode Mode = 2 [default = append];    // If not set explicitly, but the file exists, then error
    optional OutputFormat Format = 3 [default = xml];   // If omitted, then xml
    optional string Encoding = 4 [default = "utf8"];    // Output data encoding, by default utf8
    optional bool SaveLeads = 5 [default = false];      // Whether to save leads
    optional bool SaveEquals = 6 [default = false];     // Whether to save equals facts
  }
  message TArticleRef {
    required string Name = 1;
  }
  message TFactTypeRef {
    required string Name = 1;
    optional bool NonEquality = 2 [default = false];
  }
  required string Dictionary = 1;                       // May be a folder, then the parser will work
                                                        // in compatibility  mode with old paths
  optional string PrettyOutput = 2;                     // Name of the file where the debugging HTML file will be written
  optional uint32 NumThreads = 3 [default = 1];         // Number of threads (by default, one)
  optional string Language = 4 [default = "ru"];        // Language (by default, Russian)
  optional string PrintTree = 5;                        // see parsedoc
  optional string PrintRules = 6;                       // see parsedoc
  optional TInputParams Input = 7;
  optional TOutputParams Output = 8;
  repeated TArticleRef Articles = 9;
  repeated TFactTypeRef Facts = 10;
  optional bool SavePartialFacts = 11 [default = true];                                         
  repeated TArticleRef ArticlesBeforeFragmentation = 12;
}
```


## Debugging output <a name="otladochnyjjvyvod"></a>

The parser can output detailed information about the progress of text analysis to the screen or a file. The output format of this debugging information is designed for viewing convenience, and is not intended for automated processing. The `PrintTree` parameter specifies the file where the syntactic analysis trees will be written. The `PrintRules` parameter specifies rules to apply. For the filename, either `STDOUT` or `STDERR` can be specified to output debugging information to the appropriate thread.

The `PrintTree` and `PrintRules` parameters can be used simultaneously.


### Syntactic analysis trees (PrintTree parameter) <a name="derevjasintaksicheskogorazboraparametrprinttree"></a>

The `PrintTree` parameter lets you specify the file for outputting syntactic trees that the parser constructs during analysis.

**Example**

```no-highlight
// grammar
S -> Adj Noun;
// text
Hard work enobles a good man.
// config file fragment
PrintTree="tree.txt";
```

As a result, the parser will generate the file `tree.txt`, which will contain all the trees constructed by the parser.

**tree.txt**

```no-highlight
=====================first.cxx======================
coverage: 2, weight: 0.36666666 
S  ->  Adj[*Hard]  Noun[work] :: 0.43333333
|
|__ Adj  ->  Hard :: 1
|
|__ Noun  ->  work :: 1
coverage: 2, weight: 0.34
S  ->  Adj[*good]  Noun[man] :: 0.43333333
| 
|__ Adj  ->  good :: 1
|
|__ Noun  ->  man :: 1
======================multiwords======================
Hard_work: TAuxDicArticle, 
              our_first_grammar,
good_man: TAuxDicArticle,
              our_first_grammar,
```


### Applying rules (PrintRules parameter) <a name="srabatyvaniepravilparametrprintrules"></a>

The `PrintRules` parameter lets you specify the file for outputting a list of rules that were applied.

**Example**

```no-highlight
// grammar
S -> Adj Noun;
// text
Hard work enobles a good man.
// config file fragment
PrintRules="rules.txt";
```

**rules.txt**

```no-highlight
============================processing first.cxx=============================


S  ->  Adj[*Hard]  Noun[work] ::


S  ->  Adj[*good]  Noun[man] ::


****************************finished first.cxx*******************************
```


## Samples <a name="primery"></a>

Important: do not forget to specify encoding for the config itself

```no-highlight
encoding "utf8";
TTextMinerConfig {
  Dictionary = "../tomita-work/simple.gzt";
  Articles = [
    { Name = "_entry" }
  ]
  Facts = [
    { Name = "TFdo" }
  ]
}
```

Reads the console text as a single document in utf8 encoding. Runs the <q>_entry</q> entry from the root gzt file <q>../tomita-work/simple.gzt</q>. Prints XML with TFdo-type facts to the console.

```no-highlight
encoding "utf8";
TTextMinerConfig {
  Dictionary = "../tomita-work/simple.gzt";
  Input = {
    File = "fdo.txt";
    Encoding = "windows-1251";
  }
  Articles = [
    { Name = "_entry" }
  ]
  Facts = [
    { Name = "TFdo" }
  ]
}
```

It will read the fdo.txt file as a single document in Windows-1251 encoding (this must be specified explicitly, since the default is utf8). Runs the <q>_entry</q> entry from the root gzt file <q>../tomita-work/simple.gzt</q>. Prints XML with TFdo-type facts to the console.

```no-highlight
encoding "utf8";
TTextMinerConfig {
  Dictionary = "../tomita-work/simple.gzt";
  Input = {
    File = "fdo.txt";
    Encoding = "windows-1251";
  }
  Output = {
    File = "facts.proto";
    Format = proto;
  }
  Articles = [
    { Name = "_entry" }
  ]
  Facts = [
    { Name = "TFdo" }
  ]
}
```

Saves a file with TFdo-type facts in facts.proto.

