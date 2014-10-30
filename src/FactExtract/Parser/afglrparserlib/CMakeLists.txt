LIBRARY()

IF (NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    FactExtract/Parser/common
    FactExtract/Parser/lemmerlib
    kernel/gazetteer/common
    library/lemmer/dictlib
    library/pire
)

SRCS(
    ahokorasick.cpp
    glr.cpp
    glrtable.cpp
    grammaritem.cpp
    inputitemfactory.cpp
    inputsequence.cpp
    loader.cpp
    lr-items.cpp
    lr0.cpp
    regexp.cpp
    sourcegrammarparser.cpp
)

END()
