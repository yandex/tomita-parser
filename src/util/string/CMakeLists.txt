LIBRARY()

NOUTIL()

SRCS(
    base64.cpp
    builder.cpp
    cast.cc
    encodexml.cpp
    escape.cpp
    util.cpp
    vector.cpp
    split_iterator.cpp
    split.cpp
    url.cpp
    quote.cpp
    ascii.cpp
    printf.cpp
    type.cpp
    strip.cpp
    pcdata.cpp
    hex.cpp
    kmp.cpp             # used from TSubstringSplitDelimiter::TSubstringSplitDelimiter(Stroka const&)
)

END()
