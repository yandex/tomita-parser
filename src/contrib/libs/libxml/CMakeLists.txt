LIBRARY()

IF (WIN32 OR PROFILE)
    CFLAGS(
        -DLIBXML_STATIC
    )
ENDIF ()

NOJOINSRC()
NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/libs/libiconv
    util/charset
)

ADDINCL(GLOBAL contrib/libs/libxml/include)

IF (WIN32)
# Under Windows, use compiler-specific TLS: it seems broken the other way round.
    CFLAGS(
        GLOBAL -DHAVE_COMPILER_TLS
        GLOBAL -DLIBXML_STATIC
    )
ELSE ()
    CFLAGS (
        -DHAVE_DLOPEN
        -DHAVE_DLFCN_H
    )
ENDIF ()

SRCS(
    srcs/c14n.c
    srcs/catalog.c
    srcs/chvalid.c
    srcs/debugXML.c
    srcs/dict.c
    srcs/DOCBparser.c
    srcs/encoding.c
    srcs/entities.c
    srcs/error.c
    srcs/globals.c
    srcs/hash.c
    srcs/legacy.c
    srcs/list.c
    srcs/parser.c
    srcs/parserInternals.c
    srcs/pattern.c
    srcs/relaxng.c
    srcs/SAX.c
    srcs/SAX2.c
    srcs/schematron.c
    srcs/threads.c
    srcs/tree.c
    srcs/trio.c
    srcs/trionan.c
    srcs/triostr.c
    srcs/uri.c
    srcs/valid.c
    srcs/xinclude.c
    srcs/xlink.c
    srcs/xmlIO.c
    srcs/xmlmemory.c
    srcs/xmlmodule.c
    srcs/xmlreader.c
    srcs/xmlregexp.c
    srcs/xmlsave.c
    srcs/xmlschemas.c
    srcs/xmlschemastypes.c
    srcs/xmlstring.c
    srcs/xmlunicode.c
    srcs/xmlwriter.c
    srcs/xpath.c
    srcs/xpointer.c
    srcs/yencoding.cpp
)

END()
