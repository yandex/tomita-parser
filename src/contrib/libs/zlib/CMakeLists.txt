LIBRARY()

NOUTIL()
NO_COMPILER_WARNINGS()
NOJOINSRC()

CFLAGS(
    -DBUILD_ZLIB
)

SRCS(
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)

END()
