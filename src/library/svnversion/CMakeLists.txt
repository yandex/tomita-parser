LIBRARY()

CREATE_SVNVERSION_FOR(svnversion.cpp svnversion_data.h)

SET_GITREVISION()

SRCS(
    svnversion.cpp
)

END()
