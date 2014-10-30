LIBRARY()

PEERDIR(
    library/token
    library/unicode_normalization
)

SRCS(
    abc.h
    alphaux.h
    abc_data.cpp
    alphabet.cpp
    directory.cpp
    normalizer.cpp
)

END()

# vim: set ft=cmake:
