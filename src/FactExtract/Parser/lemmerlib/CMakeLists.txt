LIBRARY()

OWNER(lazyfrog)

IF (NOT WIN32)
    WERROR()
ENDIF ()

PEERDIR(
    library/lemmer/dictlib
)

SRCS(
    lemma.cpp
    extlemmer.cpp
)

END()
