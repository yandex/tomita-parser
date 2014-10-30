LIBRARY()

JOINSRC()
NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/libs/protobuf
)

ADDINCL(
    contrib/libs/protobuf
)
SRCDIR(
    contrib/libs/protobuf
    contrib/libs/protobuf/compiler
    contrib/libs/protobuf/compiler/cpp
    contrib/libs/protobuf/compiler/java
    contrib/libs/protobuf/compiler/perlxs
    contrib/libs/protobuf/compiler/python
    contrib/libs/protobuf/io
    contrib/libs/protobuf/stub
)

SRCS(
    code_generator.cc
    command_line_interface.cc
    subprocess.cc
    zip_writer.cc
    plugin.cc
    plugin.pb.cc
    cpp_enum.cc
    cpp_enum_field.cc
    cpp_extension.cc
    cpp_field.cc
    cpp_file.cc
    cpp_generator.cc
    cpp_helpers.cc
    cpp_message.cc
    cpp_message_field.cc
    cpp_primitive_field.cc
    cpp_service.cc
    cpp_string_field.cc
    java_enum.cc
    java_enum_field.cc
    java_extension.cc
    java_field.cc
    java_file.cc
    java_generator.cc
    java_helpers.cc
    java_message.cc
    java_message_field.cc
    java_primitive_field.cc
    java_service.cc
    perlxs_generator.cc
    perlxs_helpers.cc
    python_generator.cc
)

SET(IDE_FOLDER "contrib/libs")

END()
