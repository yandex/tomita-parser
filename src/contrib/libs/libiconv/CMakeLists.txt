LIBRARY()

NOUTIL()
NO_COMPILER_WARNINGS()
NOJOINSRC()

SRCS(
    genaliases.c
    genaliases2.c
    #genflags.c
    #gentranslit.c
    iconv.c
    #relocatable.c
)

END()
