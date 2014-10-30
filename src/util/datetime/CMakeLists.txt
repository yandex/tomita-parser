LIBRARY()

NOUTIL()

IF (NOT WIN32)
    WERROR()
ENDIF ()

SRCS(
    base.cpp
    cputimer.cpp
    systime.cpp
)

IF (WIN32)
    SRCS(
        strptime.cpp
    )
ENDIF ()

END()
