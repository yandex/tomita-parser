# Source files for a simple project

To run the Tomita parser, you need to create the files listed in the following table.

**Content** | **Format** | **Notes**
----- | ----- | -----
**config.proto** — Parser configuration file. It tells the parser where to find all the other files, how to interpret them, and what to do. | Protobuf | Always required.
**dic.gzt** — Root dictionary. Contains a list of all the dictionaries and grammars used in the project. | Protobuf / Gazetteer | Always required.
**mygram.cxx** — Grammar. | Grammar description language. | Necessary if grammars are used in the project. There can be several of these files.
**facttypes.proto** — Description of fact types. | Protobuf | Necessary if facts are generated in the project. The parser will run without it, but there will not be any facts.
**kwtypes.proto** — Description of keyword types. | Protobuf | Necessary if new keyword types are created in the project.


So the minimal set of files for running the parser includes just the configuration file and the root dictionary (see the **minimal** example). However, grammars and custom keyword types will not be used in this case, and facts will not be generated. The typical parser usage scenario assumes that all five types of files are available.

**Source files for the sample projects in this guide**

Name | **Configuration file** | **Root dictionary** | **Grammar** | Fact types | **Keyword types** | **Texts**
----- | ----- | ----- | ----- | ----- | ----- | -----
**minimal** | config.proto | mydic.gzt |  |  |  | 
**tutorial1** | config.proto | mydic.gzt | first.cxx |  |  | test.txt
**tutorial2** | config.proto | mydic.gzt | adjperson.cxx |  |  | test1.txt, test2.txt, test3.txt
**tutorial3** | config.proto | mydic.gzt | fioborn.cxx |  |  | test.txt
**tutorial4** | config.proto | mydic.gzt | date.cxx | facttypes.proto |  | test.txt
**tutorial5** | config.proto | mydic.gzt, animals_dict.gzt, mammals.txt | main.cxx, date.cxx | facttypes.proto | kwtypes.proto | test.txt


