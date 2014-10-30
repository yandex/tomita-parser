LIBRARY()

IF (NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    contrib/libs/libxml
    kernel/gazetteer/common
    kernel/gazetteer/config
    library/lemmer/alpha
    library/lemmer/dictlib
    library/token
    util/regexp
)

SRCS(
    dateparser.cpp
    gramstr.cpp
    inputitem.cpp
    libxmlutil.cpp
    parmbase.cpp
    simplexmlwriter.cpp
    smartfilefind.cpp
    pathhelper.cpp
    timehelper.cpp
    fact-types.cpp
    utilit.cpp
    toma_constants.cpp
    common_constants.cpp
    commonparm.cpp
    textminerconfig.proto
)

END()
