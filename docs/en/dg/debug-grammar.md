# Debugging grammars

It is not possible to track each step in applying grammars, but you can use the parameters `PrintTree` (see [[!TITLE run-parser.md]](run-parser.md)) and `PrintRules` (see [[!TITLE run-parser.md]](run-parser.md)) in the configuration file. Use these parameters to view the syntactic tree that was built (`PrintTree`) and find out which rules were or were not applied (`PrintRules`).

You can also create a fact with the name `Test` and a few optional fields.

```no-highlight
message Test: NFactType.TFact
{
  optional string A = 1;
  optional string B = 2;
  optional string C = 3;
  optional string D = 4;
  optional string E = 5;
}
```

Then you can insert the interpretation of this fact in all the desired places in a grammar (just remember to set this fact in the [configuration file](run-parser.md) so it will be printed). This is similar to debugging using "print" in Perl and "printf" in C++.

