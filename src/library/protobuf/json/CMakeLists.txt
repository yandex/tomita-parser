LIBRARY()

OWNER(familom)

SRCS(
    proto2json.h
    proto2json.cpp
    json2proto.h
    json2proto.cpp
    string_transform.h
    string_transform.cpp
    inline.cpp
)

PEERDIR(
    contrib/libs/protobuf
    library/json
    library/protobuf/util
    util/draft
)

END()
