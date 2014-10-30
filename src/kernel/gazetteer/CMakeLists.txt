LIBRARY()

OWNER(mowgli)

PEERDIR(
    kernel/gazetteer/common
    library/comptrie
    library/lemmer/alpha
    library/lemmer/dictlib
    library/protobuf/util
    library/token
    library/iter
    library/unicode_normalization
    FactExtract/Parser/lemmerlib
)

INDUCED_DEPS(kernel/gazetteer/base.proto)

SRCS(
    protoparser.cpp
    gztparser.cpp
    builtin.cpp
    sourcetree.cpp
    descriptors.cpp
    protopool.cpp
    articlepool.cpp
    gzttrie.cpp
    tokenize.cpp
    gazetteer.cpp
    generator.cpp
    worditerator.cpp
    articlefilter.cpp
    filterdata.cpp
    gztarticle.cpp
    syntax.proto # description of *.gzt format (textual)
    binary.proto # description of *.gzt.bin format (binary)
    base.proto # base gazetteer definitions
)

END()
