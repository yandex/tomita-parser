LIBRARY()

IF(NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    kernel/gazetteer
)

SRCS(
    geo_hierarchy.cpp
)



END()

