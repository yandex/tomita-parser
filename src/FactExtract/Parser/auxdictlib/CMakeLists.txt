LIBRARY()

PEERDIR(
    FactExtract/Parser/afdocparser/afdocruspureparserlib # for TMorph
    FactExtract/Parser/common # for pathhelper.cpp
    FactExtract/Parser/tomalanglib
    kernel/gazetteer/common
    library/lemmer/dictlib
)

SRCS(
    articles_syn_lex.l
    articles_syn_yacc.y
    articles_parser.cpp
    dictionary.cpp
    dictindex.cpp
)

END()
