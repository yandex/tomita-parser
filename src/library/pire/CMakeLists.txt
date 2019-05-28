LIBRARY()

CFLAGS(-DPIRE_NO_CONFIG)

SRCDIR(contrib/libs/pire/pire)

SRCS(
    pcre2pire.cpp
    classes.cpp
    encoding.cpp
    fsm.cpp
    scanner_io.cpp
    easy.cpp
    scanners/null.cpp
    extra/capture.cpp
    extra/count.cpp
    extra/glyphs.cpp
    extra/unicode_support.cpp
    re_lexer.cpp
    re_parser.y
    extraencodings.cpp
)

END()

RECURSE(
    inline
)
