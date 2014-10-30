LIBRARY()

NOUTIL()

PEERDIR(
    contrib/libs/lz4
    contrib/libs/zlib

    util/network
)

IF (NOT WIN32)
    WERROR()
ENDIF ()

SRCS(
    buffer.cpp
    buffered.cpp
    chunk.cpp
    debug.cpp    # util/thread/queue.cpp
    file.cpp
    helpers.cpp
    input.cpp
    length.cpp
    lz.cpp
    mem.cpp
    null.cpp
    output.cpp
    pipe.cpp
    stack_file.cpp
    str.cpp
    tee.cpp
    zlib.cpp
    printf.cpp
    format.cpp
    factory.cpp     # kernel/gazetteer
)

END()
