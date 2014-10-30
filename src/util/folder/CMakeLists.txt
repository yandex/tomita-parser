LIBRARY()

NOUTIL()

SRCS(
    filelist.cpp
    fts.cpp
    dirut.cpp
    path.cpp
    pathsplit.cpp  # util/folder/path.cpp
    iterator.cpp
)

IF (WIN32)
    SRCS(
        lstat_win.c
        dirent_win.c
    )
ENDIF ()

END()
