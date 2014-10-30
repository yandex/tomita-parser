LIBRARY()

PEERDIR(
    FactExtract/Parser/afdocparser/afdocrusparserielib
    FactExtract/Parser/common
    FactExtract/Parser/common/docreaders
    FactExtract/Parser/simpletextminerlib
    FactExtract/Parser/lemmerlib
    kernel/gazetteer
    util/regexp
)

SRCS(
    processor.cpp
)

END()
