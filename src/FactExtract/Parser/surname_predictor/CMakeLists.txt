LIBRARY()

IF(NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    library/lemmer/dictlib
)

SRCS(
    surname_predictor.cpp
)

END()

