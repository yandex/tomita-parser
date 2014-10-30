LIBRARY()

NOUTIL()

SRCS(
    generated/cp_data.cpp
    generated/encrec_data.cpp
    generated/unidata.cpp
    generated/uniscripts.cpp

    codepage.cpp
    cp_encrec.cpp
    doccodes.cpp
    utf.cpp
    wide.cpp
)

PEERDIR(
    contrib/libs/libiconv
)

END()
