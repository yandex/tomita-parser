LIBRARY()

OWNER(mowgli)

PEERDIR(
    contrib/libs/protobuf
    library/binsaver
)

SRCS(
    pb_utils.h
    pb_io.cpp
    repeated_field_utils.h
    is_equal.cpp
)

END()
