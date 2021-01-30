LIBRARY()

IF (NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    FactExtract/Parser/common
    library/getopt
)

SRCS(
    docbody.cpp
    docbodybase.cpp
    doclistretrieverfromdisc.cpp
    docstreambase.cpp
    docstreamdisc.cpp
    streamretrieverfacrory.cpp
    stdinyandexdocsreader.cpp
    stdindocperlinereader.cpp
    onedocreader.cpp
    tararchivereader.cpp
    rmltar.cpp
    agencyinforetriever.cpp
    stdinmapreducedocsreader.cpp
    somreader.cpp
    memoryreader.cpp
)

END()
