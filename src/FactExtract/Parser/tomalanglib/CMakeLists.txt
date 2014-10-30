LIBRARY()

PEERDIR(
    FactExtract/Parser/afglrparserlib
    FactExtract/Parser/common
    kernel/gazetteer/common
    library/lemmer/dictlib
    library/token
)

SRCS(
    tomasyntax.proto
    tomaparser.cpp
    tomacompiler.cpp
    postfix.cpp
)

END()
