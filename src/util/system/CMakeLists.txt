LIBRARY()

NOUTIL()

IF (NOT WIN32)
    WERROR()
ENDIF ()

SRCS(
    atexit.cpp
    backtrace.cpp
    compat.cpp
    condvar.cpp
    datetime.cpp
    demangle.cpp   # util/system/backtrace.cpp
    dynlib.cpp
    env.cpp
    err.cpp
    error.cpp
    event.cpp
    execpath.cpp
    fasttime.cpp
    file.cpp
    filemap.cpp
    flock.cpp
    fs.cpp
    fstat.cpp
    hostname.cpp
    hp_timer.cpp
    info.cpp
    mlock.cpp
    mutex.cpp
    oldfile.cpp
    pipe.cpp
    progname.cpp
    rusage.cpp
    rwlock.cpp
    sem.cpp
    spinlock.cpp
    sysstat.cpp
    thread.cpp
    tls.cpp
    user.cpp
    yassert.cpp
    yield.cpp    # util/system/spinlock.cpp
    src_root.cpp
)

IF (MY_PLATFORM STREQUAL "arm")
    CFLAGS(-D_FORTIFY_SOURCE=0)
ELSE()
    IF (WIN32)
        SRCS(
            fs_win.cpp
        )
    ELSE()
        SRCS(
            context_x86.asm
        )
    ENDIF ()
ENDIF()

IF (LINUX)
    EXTRALIBS(-lrt -ldl)
ENDIF ()

IF (LINUX OR SUN OR CYGWIN OR WIN32)
    IF (NOT MUSL)
        SRCS(
            freeBSD_mktemp.cpp
        )
    ENDIF ()
ENDIF ()

IF (NOT MUSL)
    SRCS(
        strlcpy.c
    )
ENDIF ()

IF (MUSL)
    PEERDIR(
        contrib/libs/linuxvdso
    )
ENDIF ()

END()
