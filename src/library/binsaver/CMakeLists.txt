LIBRARY()

OWNER(gulin)

SRCS(
    BasicFactory.h
    BinSaver.cpp
    BlobIO.cpp
    BufferedIO.cpp
    UtilStreamIO.cpp
)

PEERDIR(
    util/draft
    library/2darray
)

END()
