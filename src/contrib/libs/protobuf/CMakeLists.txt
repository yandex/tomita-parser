LIBRARY()

JOINSRC()
NO_COMPILER_WARNINGS()
ADDINCLSELF()

ADDINCL(
    contrib/libs/protobuf/compiler
    contrib/libs/protobuf/io
    contrib/libs/protobuf/stubs
)
SRCDIR(
    contrib/libs/protobuf/compiler
    contrib/libs/protobuf/io
    contrib/libs/protobuf/stubs
)

SRCS(
    common.cc
    hash.cc
    map-util.cc
    once.cc
    structurally_valid.cc
    stl_util-inl.cc
    substitute.cc
    strutil.cc

    descriptor.cc
    descriptor.pb.cc
    descriptor_database.cc
    dynamic_message.cc
    extension_set.cc
    extension_set_heavy.cc
    generated_message_reflection.cc
    generated_message_util.cc
    message.cc
    message_lite.cc
    messagext.cc
    reflection_ops.cc
    repeated_field.cc
    service.cc
    text_format.cc
    unknown_field_set.cc
    wire_format.cc
    wire_format_lite.cc

    coded_stream.cc
    printer.cc
    tokenizer.cc
    zero_copy_stream.cc
    zero_copy_stream_impl.cc
    zero_copy_stream_impl_lite.cc
    importer.cc
    parser.cc
)

END()
