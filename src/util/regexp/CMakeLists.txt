LIBRARY()

PEERDIR(
    contrib/libs/pcre
)

SRCS(
    regexp.cpp
)

IF (WIN32)
    SRCS(
        glob.cpp
    )
ENDIF ()

END()
