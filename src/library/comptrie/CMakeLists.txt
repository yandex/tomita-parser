LIBRARY()

SRCS(
    array_with_size.h
    chunked_helpers_trie.h
    compact_vector.h
    comptrie.h
    comptrie_bool.h
    comptrie_builder.h
    comptrie_builder_bool.h
    comptrie_packers.h
    comptrie_trie.h
    comptrie_trie_bool.h
    key_selector.h
    leaf_skipper.h
    set.h

    comptrie.cpp
    comptrie_impl.cpp
    make_fast_layout.cpp
    minimize.cpp
    node.cpp
    write_trie_backwards.cpp
    writeable_node.cpp
)

PEERDIR(
    library/chunks
    util/draft
)

END()
