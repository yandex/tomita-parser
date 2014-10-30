LIBRARY()

PEERDIR(
    contrib/libs/protobuf
    library/protobuf/json
    library/protobuf/util
    library/iter
    util/draft
)

SRCS(
    tools.cpp
    protohelpers.cpp
    binaryguard.cpp
    parserbase.cpp
)

END()
