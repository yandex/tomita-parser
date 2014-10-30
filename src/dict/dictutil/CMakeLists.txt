LIBRARY()

OWNER(aosipenko)

WERROR()

SRCS(
    ascii.cpp
    charset.cpp
    char_set.cpp
    encode.cpp
    exceptions.cpp
    lang_ids.cpp
    lang_utils.cpp
    map_str.cpp
    math.cpp
    prefsuff.cpp
    script.cpp
    split.cpp
    str.cpp
    str_metric.cpp
    wcs.cpp

    generated/zh_tables.cpp
)

PEERDIR(
    contrib/libs/libiconv
    library/unicode_normalization
    util/draft
)

END()
