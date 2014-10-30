LIBRARY()

PEERDIR(
    contrib/libs/libxslt
    contrib/libs/libexslt
)

SRCS(
    xsltransform.cpp
    catxml.cpp
    datefun.cpp
)

END()

