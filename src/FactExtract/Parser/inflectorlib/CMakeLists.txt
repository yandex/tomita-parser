LIBRARY()

OWNER(mowgli)

IF (NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    FactExtract/Parser/common
    FactExtract/Parser/lemmerlib
    kernel/gazetteer/common
    library/lemmer/dictlib
    library/token
)

SRCS(
    gramfeatures.cpp
    ylemma.cpp
    complexword.cpp
    numeral.cpp
)

END()
