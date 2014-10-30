LIBRARY()

WERROR()

OWNER(pg nga)

IF (YMAKE)
    CFLAGS(-DYMAKE=1)

    PEERDIR(
        library/svnversion
    )
ELSE ()
    SET_SVNREVISION()
ENDIF ()

SRCS(
    last_getopt.cpp
    last_getopt_support.h
    opt.cpp
    opt2.cpp
    posix_getopt.cpp
    ygetopt.cpp
)

END()
