TOOL(gztcompiler)

#use lfalloc explicitly
ALLOCATOR(LF)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/common
    library/getopt
)

SRCS(
    main.cpp
)

END()
