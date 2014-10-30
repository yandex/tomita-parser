LIBRARY(yutil)

NOUTIL()

PEERDIR(
    util/charset
    util/datetime
    util/digest
    util/folder
    util/generic
    util/lambda
    util/memory
    util/network
    util/random
    util/stream
    util/string
    util/system
    util/thread
)

IF (NOT WIN32)
    WERROR()
ENDIF ()

SRCS(
    ffb.cpp
    fileptr.cpp
    mbitmap.cpp
    str_hash.cpp
    ysafeptr.cpp
    ysaveload.cpp
)

CHECK_DEPENDENT_DIRS(ALLOW_ONLY util contrib library/malloc)

END()
