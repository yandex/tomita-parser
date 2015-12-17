#
# Generate python bindings for protobuf files
#
MACRO(GENERATE_PY_PROTOS)
    # TODO: fix IDE folders
    # MESSAGE_EX("Generating Python protobuf and package files...")
    BUILDAFTER(contrib/tools/protoc)

    # set ${CMAKE_CURRENT_BINARY_DIR} by default?
    SET(__dest_dir_ ${ARCADIA_BUILD_ROOT})
    SET(__next_dest_)
    SET(__next_result_)
    SET(__result_ "")
    SET(__py_protos_)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "DESTINATION")
            SET(__next_dest_ yes)
        ELSEIF ("${__item_}" STREQUAL "RESULT")
            SET(__next_result_ yes)
        ELSE ()
            IF (__next_dest_)
                SET(__dest_dir_ ${__item_})
                SET(__next_dest_)
            ELSEIF (__next_result_)
                SET(__result_ ${__item_})
                SET(__next_result_)
            ELSE ()
                SET_APPEND(__py_protos_ ${__item_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    # Ensure that destination exists.
    EXECUTE_PROCESS(
        COMMAND mkdir -p ${__dest_dir_}
    )

    # Generate python protobuf files.
    FOREACH(proto ${__py_protos_})
        STRING(REPLACE ".proto" "_pb2.py" pyfile "${proto}")
        SET(fullproto "${ARCADIA_ROOT}/${proto}")
        SET(pyfile "${__dest_dir_}/${pyfile}")
        PY_PROTO(${fullproto} ${pyfile} ${__dest_dir_})
        # We need a list with absolute file names later.
        SET_APPEND(PY_PROTOS ${pyfile})
    ENDFOREACH ()

    IF (NOT "x${__result_}x" STREQUAL "xx")
        SET(${__result_} ${PY_PROTOS})
    ENDIF ()
ENDMACRO ()

MACRO (PY_PROTO full_proto output dest_dir)
    RUN_PROGRAM(contrib/tools/protoc "-I=${ARCADIA_ROOT}/" "--python_out=${dest_dir}" "${full_proto}" IN "${full_proto}" OUT ${output})
ENDMACRO ()
#
# Create __init__.py files to make Python treat the directories as containing packages.
# This is a bad macros. Try don't use this macro.
#
MACRO(CREATE_INIT_PY)
    SET(__next_dest_)
    SET(__next_result_)
    SET(__result_ "")
    SET(__inc_dest_)
    SET(__dest_dir_ ${ARCADIA_BUILD_ROOT})
    SET(__py_protos_)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "DESTINATION")
            SET(__next_dest_ yes)
        ELSEIF ("${__item_}" STREQUAL "RESULT")
            SET(__next_result_ yes)
        ELSEIF ("${__item_}" STREQUAL "INCLUDING_DEST_DIR")
            SET(__inc_dest_ yes)
        ELSE ()
            IF (__next_dest_)
                SET(__dest_dir_ ${__item_})
                SET(__next_dest_)
            ELSEIF (__next_result_)
                SET(__result_ ${__item_})
                SET(__next_result_)
            ELSE ()
                SET_APPEND(__py_protos_ ${__item_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    IF (__inc_dest_)
        FILE(WRITE "${__dest_dir_}/__init__.py" "")
        SET_APPEND(generated ${initpy})
    ENDIF ()

    SET(pkgs)
    SET(generated)
    FOREACH (protofile ${__py_protos_})
        GET_FILENAME_COMPONENT(apath "${__dest_dir_}/${protofile}" PATH)
        WHILE (NOT ${apath} STREQUAL ${__dest_dir_})
            LIST(FIND pkgs ${apath} finded)
            IF (${finded} EQUAL -1)
                SET(initpy "${apath}/__init__.py")
                FILE(WRITE ${initpy} "")

                SET_APPEND(generated ${initpy})
                SET_APPEND(pkgs ${apath})
            ENDIF ()
            GET_FILENAME_COMPONENT(apath ${apath} PATH)
        ENDWHILE()
    ENDFOREACH ()

    #GET_TARGET_UNIQ_NAME(__target_name_ TO_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "_init_pys")
    #ADD_CUSTOM_TARGET(${__target_name_} ALL DEPENDS ${generated})

    IF (NOT "x${__result_}x" STREQUAL "xx")
        SET(${__result_} ${generated})
    ENDIF ()
ENDMACRO ()

MACRO(CREATE_INIT_PY_STRUCTURE dir)
    SET(apath ${dir})
    FILE(WRITE "${ARCADIA_BUILD_ROOT}/__init__.py" "")
    WHILE(NOT ${apath})
        SET(initpy "${ARCADIA_BUILD_ROOT}/${apath}/__init__.py")
        FILE(WRITE ${initpy} "")
        GET_FILENAME_COMPONENT(apath ${apath} PATH)
    ENDWHILE ()
ENDMACRO()
