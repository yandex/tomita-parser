LIBRARY()

IF (WIN32)
    CFLAGS(
        -DLIBEXSLT_STATIC
        -DLIBXSLT_STATIC
        -DLIBXML_STATIC
    )
ENDIF ()

NOUTIL()
NO_COMPILER_WARNINGS()

SRCS(
    common.c
    crypto.c
    date.c
    dynamic.c
    exslt.c
    functions.c
    math.c
    saxon.c
    sets.c
    strings.c
)

PEERDIR(
    contrib/libs/libxml
    contrib/libs/libxslt
)

END()
