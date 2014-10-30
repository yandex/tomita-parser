TOOL(pire_inline)

CFLAGS(
    -DPIRE_NO_CONFIG
)

PEERDIR(
    ADDINCL library/pire
)

SRCDIR(
    ADDINCL contrib/libs/pire/pire
)

SRCS(
    inline.l
)

END()
