IF (NOT YMAKE)
MACRO (GZT_PROTOC)

## compiling gzt proto-files with inheritance syntax.

    TOOLDIR_EX(dict/gazetteer/compiler __gztcompiler_)

    SET_BARRIER(_barrier_gzt_protos __path_gzt_proto_barrier)
    FOREACH(srcfile ${ARGN})

        SET(src_proto_dir "")
        GET_FILENAME_COMPONENT(src_proto_path ${srcfile} ABSOLUTE)
        GET_RELATIVE_TO_TOP(${src_proto_path} src_proto_rel src_proto_dir)
        IF (src_proto_dir STREQUAL "")
            SET(want_relative FALSE)
            GET_FILENAME_COMPONENT(src_proto_name ${srcfile} NAME)
            SET(dst_proto_path "${BINDIR}/${src_proto_name}")
        ELSE ()
            SET(want_relative TRUE)
            SET(dst_proto_path "${CMAKE_BINARY_DIR}/${src_proto_rel}")
        ENDIF ()

        SPLIT_FILENAME(__pathwe_ __ext_ "${dst_proto_path}")
        IF ("${__ext_}" STREQUAL ".gztproto")
            SET(dst_proto_path "${__pathwe_}.proto")
        ENDIF ()

        GET_FILENAME_COMPONENT(dst_proto_dir ${dst_proto_path} PATH)
        IF (UNIX)
            SET(__cmd_mkdir mkdir -p ${dst_proto_dir})
        ELSE ()
            FILE(TO_NATIVE_PATH ${dst_proto_dir} __native)
            SET(__cmd_mkdir if not exist \"${__native}\" mkdir \"${__native}\")
        ENDIF ()

        # convert to regular proto unless this thing is already generated
        IF (NOT src_proto_dir STREQUAL CMAKE_BINARY_DIR)
            ADD_CUSTOM_COMMAND(
                OUTPUT            ${dst_proto_path}
                COMMAND           ${__cmd_mkdir}
                COMMAND           ${__gztcompiler_} -p
                    -I ${ARCADIA_ROOT} ${src_proto_path} ${dst_proto_path}
                MAIN_DEPENDENCY   ${src_proto_path}
                DEPENDS           gztcompiler
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT           "Converting to proto format ${srcfile}"
            )
            # add barrier if proto is generated
            ADD_CUSTOM_COMMAND(
                OUTPUT  ${__path_gzt_proto_barrier}
                DEPENDS ${dst_proto_path}
                APPEND
            )
        ENDIF ()

        # compile the generated proto file
        BUILDWITH_PROTOC_IMPL(${dst_proto_path} dst_proto_cc dst_proto_hh)

        # add the barrier as a dependency regardless of whether the proto file
        # is generated, because a generated proto file might be in its imports
        ADD_CUSTOM_COMMAND(
            OUTPUT  ${dst_proto_cc} ${dst_proto_hh}
            DEPENDS ${__path_gzt_proto_barrier}
            APPEND
        )

        # add the generated source file to the list
        SRCS(${dst_proto_cc})

    ENDFOREACH ()
ENDMACRO ()
ENDIF ()
