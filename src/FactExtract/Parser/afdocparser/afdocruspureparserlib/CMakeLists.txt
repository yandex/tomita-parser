LIBRARY()

IF (NOT WIN32)
    WERROR()
ENDIF()

PEERDIR(
    FactExtract/Parser/afdocparser/builtins
    FactExtract/Parser/common
    FactExtract/Parser/surname_predictor
    FactExtract/Parser/lemmerlib
    kernel/gazetteer
    kernel/gazetteer/common
    library/lemmer/dictlib
    library/iter
)

SRCS(
    afdoc.cpp
    ../rus/dateprocessor.cpp
    ../rus/fiowordsequencee.cpp
    ../rus/gleiche.cpp
    ../rus/homonym.cpp
    ../rus/namecluster.cpp
    ../rus/namefinder.cpp
    ../rus/numberprocessor.cpp
    ../rus/wordvector.cpp
    ../rus/sentence.cpp
    ../rus/text.cpp
    ../rus/word.cpp
    ../common/afdocbase.cpp
    ../common/afdochtml.cpp
    ../common/afdocsinglelanguage.cpp
    ../common/afdoctext.cpp
    ../common/graminfo.cpp
    ../common/homonymbase.cpp
    ../common/langdata.cpp
    ../common/primitive.cpp
    ../common/sentencebase.cpp
    ../common/textbase.cpp
    ../common/wordbase.cpp
    ../common/wordsequence.cpp
    ../common/gztarticle.cpp
    ../common/wordspair.cpp
    ../common/facttypeholder.cpp
    ../rus/morph.cpp
)

END()
