LIBRARY()

PEERDIR(
    contrib/libs/protobuf
    kernel/gazetteer
)

SRCS(
    builtins.cpp
    articles_base.gztproto
    facttypes_base.gztproto
    kwtypes_base.gztproto
)

END()
