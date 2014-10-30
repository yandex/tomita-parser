LIBRARY()

NOUTIL()

IF (NOT WIN32)
    WERROR()
ENDIF ()

SRCS(
    bitutils.cpp                  # comptrie
    delim_file_iter.cpp           # util/draft
    format.cpp                    # kernel/gazetteer
)

PEERDIR(
    library/svnversion
)


END()
