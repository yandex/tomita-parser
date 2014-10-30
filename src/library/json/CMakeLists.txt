LIBRARY()

SRCS(
    json_writer.cpp
    json_reader.cpp
    json_prettifier.cpp
)

PEERDIR(
    contrib/libs/yajl
    library/json/writer
    util/draft
)

END()
