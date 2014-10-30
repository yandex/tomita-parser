LIBRARY()

NOUTIL()

OWNER(umnov)

SRCS(
    generated/composition.cpp
    generated/decomposition.cpp

    decomposition_table.h
    normalization.cpp

    custom_encoder.cpp
)

PEERDIR(
    util/charset
)

END()
