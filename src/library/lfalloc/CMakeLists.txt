LIBRARY()

NOUTIL()
NO_COMPILER_WARNINGS()

IF ("${YMAKE}" MATCHES "devtools")
    CFLAGS(-DYMAKE=1)
ENDIF ()

SRCS(
    lf_allocX64.cpp
    malloc-info.cpp
)

PEERDIR(
    library/malloc/api
)

SET(IDE_FOLDER "util")

END()
