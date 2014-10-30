LIBRARY()

NOUTIL()
NO_COMPILER_WARNINGS()
JOINSRC()
NORUNTIME()

IF (LINUX)
    EXTRALIBS(-lpthread)
ENDIF ()

IF (FREEBSD)
    EXTRALIBS(-lthr)
ENDIF ()

SRCDIR(
    contrib/libs/stlport/stlport-5.1.4/src
)

ADDINCL(
    GLOBAL contrib/libs/stlport/stlport-5.1.4
    GLOBAL contrib/libs/stlport/stlport-5.1.4/stlport
)

CFLAGS(
    GLOBAL -DUSE_INTERNAL_STL
)

EXTRADIR(
    stlport-5.1.4/src/c_locale_dummy
    stlport-5.1.4/src/c_locale_glibc
    stlport-5.1.4/src/c_locale_win32
    stlport-5.1.4/stlport/stl
    stlport-5.1.4/stlport/stl/config
    stlport-5.1.4/stlport/stl/debug
    stlport-5.1.4/stlport/stl/pointers
    stlport-5.1.4/stlport/using
    stlport-5.1.4/stlport/using/h
)

SRCS(
     #allocators.cpp
    bitset.cpp
    cxa.c
    string.cpp
    ctype.cpp
    num_put_float.cpp
    locale_catalog.cpp
    numpunct.cpp
    c_locale.c
    dll_main.cpp
    locale_impl.cpp
    ostream.cpp
    codecvt.cpp
    facets_byname.cpp
    messages.cpp
    sstream.cpp
    collate.cpp
    fstream.cpp
    monetary.cpp
    stdio_streambuf.cpp
    complex.cpp
    ios.cpp
    num_get.cpp
    complex_io.cpp
    iostream.cpp
    num_get_float.cpp
    strstream.cpp
    complex_trig.cpp
    istream.cpp
    num_put.cpp
    time_facets.cpp
)

SET(IDE_FOLDER "util")

END()
