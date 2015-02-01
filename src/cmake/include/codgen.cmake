# - Find gperf and flex
# This module finds if gperf and flex are installed and determines where the
# executables are. This code sets the following variables:
#
#  LEX:                  path to the flex compiler
#  GPERF:                GPERF code generator
#  GP_FLAGS:             GPERF translator flags
#
# This code defines the following macros:
#
#  BUILDWITH_GPERF(sourcefile destfile)
#  BUILDWITH_LEX(sourcefile destfile)
#

# GET_FILENAME_COMPONENT(LEX_INCLUDE_DIR ${LEX} PATH) # sourcename?
SET(LEX_INCLUDE_DIR ${ARCADIA_ROOT}/contrib/tools/flex-old)

SET(LEX_FLAGS)

MACRO (BUILDWITH_LEX srcfile dstfile) # dstfile usually has .l extension
    TOOLDIR_EX(
        contrib/tools/flex-old LEX
    )

    IF (NOT LEX)
        MESSAGE(SEND_ERROR "LEX is not defined. Please check if contrib/tools/flex-old is checked out")
    ENDIF ()
    ADD_CUSTOM_COMMAND(
       OUTPUT ${dstfile}
       COMMAND ${LEX} ${LEX_FLAGS} -o${dstfile} ${srcfile}
       MAIN_DEPENDENCY "${srcfile}"
       DEPENDS ${srcfile} flex ${ARGN}
       COMMENT "Building \"${dstfile}\" with lex"
       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_SRC(${dstfile})
    INCLUDE_DIRECTORIES(${LEX_INCLUDE_DIR})
ENDMACRO ()

# flags for all cases
# -a         # ansi-prototypes (old version)
# -L ANSI-C  # ansi-prototypes (new version)
# -C         # const-strings
# -p         # return-pointer  (old version) ???
# -t         # user-type
# -T         # dont-print-type-definiton-use-h-file-instead

# usually we have a lot of keys
# -k*        # all-bytes-to-try
# -D         # disambiguate
# -S1        # generate-one-switch (m.b. not neccessary ?)

SET_APPEND(GP_FLAGS -CtTLANSI-C)

# usually we match string prefix to a key
# i.e. html-entities, word-preffixes are NOT nul-terminated
# -c         # use-strncmp

IF (NOT DEFINED GPERF_FEW_KEYS)
    SET_APPEND(GP_FLAGS -Dk*)
ENDIF ()

IF (NOT DEFINED GPERF_FULL_STRING_MATCH)
    SET_APPEND(GP_FLAGS -c)
ENDIF ()

MACRO (BUILDWITH_GPERF srcfile dstfile) # dstfile usually has .gperf.cpp extension
    IF (DEFINED GPERF_LOOKUP_FN)
        SET(LOCAL_GPERF_LOOKUP_FN ${GPERF_LOOKUP_FN})
    ELSE ()
        GET_FILENAME_COMPONENT(LOCAL_GPERF_LOOKUP_FN ${dstfile} NAME_WE)
        SET(LOCAL_GPERF_LOOKUP_FN in_${LOCAL_GPERF_LOOKUP_FN}_set)
    ENDIF ()

    TOOLDIR_EX(
        contrib/tools/gperf GPERF
    )

    IF (NOT GPERF)
        MESSAGE(SEND_ERROR "GPERF is not defined. Please check if contrib/tools/gperf is checked out")
    ENDIF ()

    ADD_CUSTOM_COMMAND(
        OUTPUT ${dstfile}
        PRE_BUILD
        COMMAND ${GPERF} ${GP_FLAGS} -N${LOCAL_GPERF_LOOKUP_FN} ${srcfile} >${dstfile} || \(${RM} ${RM_FORCEFLAG} ${dstfile} && exit 1\)
        MAIN_DEPENDENCY "${srcfile}"
        DEPENDS ${srcfile} gperf ${ARGN}
        COMMENT "Building ${dstfile} with gperf"
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_SRC(${dstfile})
ENDMACRO ()

MACRO (BUILDWITH_CYTHON_BASE src dst)
    SET(__options ${ARGN})
    GET_FILENAME_COMPONENT(_src ${src} NAME)
    GET_FILENAME_COMPONENT(_dst ${dst} NAME)

    PYTHON(${ARCADIA_ROOT}/contrib/tools/cython/cython.py ${__options} --cplus ${_src} -o ${_dst} IN ${_src} OUT ${_dst})

    SET(_src)
    SET(_dst)
ENDMACRO ()

MACRO (BUILDWITH_CYTHON src)
    GET_BIN_ABS_PATH(__dst ${src})
    BUILDWITH_CYTHON_BASE(${src} ${__dst} ${ARGN})
ENDMACRO ()

DEFAULT(YASM_OBJECT_FORMAT elf)
DEFAULT(YASM_PLATFORM "UNIX")

IF (DARWIN)
    SET(YASM_OBJECT_FORMAT macho)
    SET(YASM_PLATFORM "DARWIN")
ENDIF ()

IF (WIN32)
    SET(YASM_OBJECT_FORMAT win)
    SET(YASM_PLATFORM "WIN")
ENDIF ()

MACRO (BUILDWITH_YASM srcfile dstfile)
    TOOLDIR_EX(
        contrib/tools/yasm YASM
    )

    IF (NOT YASM)
        MESSAGE(SEND_ERROR "YASM is not defined. Please check if contrib/tools/yasm is checked out")
    ENDIF ()

    # produce a set of -I... flags
    SET(__incflags "")
    FOREACH(__incdir ${DTMK_I})
        SET_APPEND(__incflags "-I${__incdir}")
    ENDFOREACH ()

    ADD_CUSTOM_COMMAND(
        OUTPUT "${dstfile}"
        COMMAND "${YASM}" -f "${YASM_OBJECT_FORMAT}${HARDWARE_ARCH}" -D "${YASM_PLATFORM}" -D "_${HARDWARE_TYPE}_" -o "${dstfile}" ${__incflags} "${srcfile}"
        COMMENT "Building \"${dstfile}\" with yasm"
        MAIN_DEPENDENCY "${srcfile}"
        DEPENDS "${srcfile}" yasm ${ARGN}
        WORKING_DIRECTORY ${BINDIR}
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_SRC(${dstfile})
ENDMACRO ()

#TOOLDIR_EX(
#    contrib/tools/byacc BYACC
#)

DEFAULT(BYACC_FLAGS -v)

MACRO (BUILDWITH_BYACC srcfile dstfile)
    IF (NOT BYACC)
        MESSAGE(SEND_ERROR "BYACC is not defined. Please check if contrib/tools/bison/bison is checked out")
    ENDIF ()
        GET_FILENAME_COMPONENT(LOCAL_BYACC_DSTFILE_FILE ${dstfile} NAME_WE)
        GET_FILENAME_COMPONENT(LOCAL_BYACC_DSTFILE_PATH ${dstfile} PATH)
        SET(LOCAL_BYACC_DSTFILE_H "${LOCAL_BYACC_DSTFILE_PATH}/${LOCAL_BYACC_DSTFILE_FILE}.h")
    ADD_CUSTOM_COMMAND(
        OUTPUT ${dstfile} ${LOCAL_BYACC_DSTFILE_H}
        COMMAND "${BYACC}" ${BYACC_FLAGS} "${srcfile}"
        MAIN_DEPENDENCY "${srcfile}"
        DEPENDS "${srcfile}" byacc ${ARGN}
        COMMENT "Building \"${dstfile}\"+\"${LOCAL_BYACC_DSTFILE_H}\" with bison"
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_INC(${LOCAL_BYACC_DSTFILE_H})
    DIR_ADD_GENERATED_SRC(${dstfile})
ENDMACRO ()

# This macro generates source files from .proto files
#
#  BUILDWITH_PROTOC(srcfile var_dst_cc)

MACRO (BUILDWITH_PROTOC srcfile var_dst_cc)
    GET_PROTO_RELATIVE(${srcfile} __want_relative)
    BUILDWITH_PROTOC_IMPL(${srcfile} ${var_dst_cc} ${var_dst_hh} "${__want_relative}" ${ARGN})
ENDMACRO ()


FUNCTION (GET_RELATIVE_TO_TOP srcfile rel_var top_var)

    # determine the top of the hierarchy where srcfile
    # is located and its name relative to that top;
    # we only consider ARCADIA_ROOT and CMAKE_BINARY_DIR

    FOREACH (top ${CMAKE_BINARY_DIR} ${ARCADIA_ROOT})

        # see if srcfile is under ${top}
        FILE(RELATIVE_PATH rel ${top} ${srcfile})
        IF (NOT IS_ABSOLUTE ${rel} AND NOT rel MATCHES "^\\.\\./")
            SET(${top_var} ${top} PARENT_SCOPE)
            SET(${rel_var} ${rel} PARENT_SCOPE)
            return()
        ENDIF ()

    ENDFOREACH ()

    # don't know where it is, so don't use relative and don't modify top_var
    SET(${rel_var} ${srcfile} PARENT_SCOPE)

ENDFUNCTION ()


# This function generates source files from .proto files optionally using the
# path of the .proto file relative to either source or build root, as opposed
# to the file portion of the path name.
#
# If the path specified to protoc is relative, it will generate the code in a
# way that is consistent with importing the .proto using the same relative path
# rather than the file portion.
#
# For instance:
# * this can only be accessed via import "bar/baz.proto"
# $ protoc ... bar/baz.proto
# * this can only be accessed via import "baz.proto"
# $ protoc ... /foo/bar/baz.proto
#
#  BUILDWITH_PROTOC_IMPL(srcfile var_dst_cc var_dst_hh)

FUNCTION (BUILDWITH_PROTOC_IMPL srcfile var_dst_cc var_dst_hh)
    # contains the protoc
    TOOLDIR_EX(contrib/tools/protoc __protoc_)
    TOOLDIR_EX(contrib/tools/protoc/plugins/cpp_styleguide __cpp_styleguide_)

    # determine the working dir and relative path for srcfile
    SET(src_proto_dir "")
    GET_FILENAME_COMPONENT(src_proto_path ${srcfile} ABSOLUTE)
    GET_RELATIVE_TO_TOP(${src_proto_path} src_proto_rel src_proto_dir)

    # set the destination directory
    IF (src_proto_dir STREQUAL "")
        GET_FILENAME_COMPONENT(src_proto_dir ${src_proto_rel} PATH)
        GET_FILENAME_COMPONENT(src_proto_rel ${src_proto_rel} NAME)
        SET(dst_proto_dir ${BINDIR})
    ELSE ()
        SET(dst_proto_dir ${CMAKE_BINARY_DIR})
    ENDIF ()
    SET(dst_proto_path "${dst_proto_dir}/${src_proto_rel}")

    # determine the files which protoc will create; protoc removes .protodevel
    # and .proto extensions, if present, then appends .pb.{h,cc} for output
    STRING(REGEX REPLACE "\\.protodevel$" "" dst_proto_pref "${dst_proto_path}")
    IF (dst_proto_pref STREQUAL dst_proto_path)
        STRING(REGEX REPLACE "\\.proto$" "" dst_proto_pref "${dst_proto_path}")
    ENDIF ()
    SET(dst_proto_hh "${dst_proto_pref}.pb.h")
    SET(dst_proto_cc "${dst_proto_pref}.pb.cc")

    # return the name of generated source files to the caller
    SET(${var_dst_cc} ${dst_proto_cc} PARENT_SCOPE)
    SET(${var_dst_hh} ${dst_proto_hh} PARENT_SCOPE)

    IF (UNIX)
        SET(__cmd_mkdir mkdir -p ${dst_proto_dir})
    ELSE ()
        FILE(TO_NATIVE_PATH ${dst_proto_dir} __native)
        SET(__cmd_mkdir if not exist \"${__native}\" mkdir \"${__native}\")
    ENDIF ()
    ADD_CUSTOM_COMMAND(
        OUTPUT            ${dst_proto_cc} ${dst_proto_hh}
        COMMAND           ${__cmd_mkdir}
        COMMAND           ${__protoc_}
            -I=. -I=${ARCADIA_ROOT} -I=${CMAKE_BINARY_DIR}
            --cpp_out=${dst_proto_dir}
            --cpp_styleguide_out=${dst_proto_dir}
            --plugin=protoc-gen-cpp_styleguide=${__cpp_styleguide_}
            ${src_proto_rel}
        MAIN_DEPENDENCY   ${src_proto_path}
        DEPENDS           protoc cpp_styleguide ${ARGN}
        WORKING_DIRECTORY ${src_proto_dir}
        COMMENT           "Generating C++ source from ${srcfile}"
    )

    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_INC(${dst_proto_hh})
    DIR_ADD_GENERATED_SRC(${dst_proto_cc})

ENDFUNCTION ()


MACRO (SRCS_PROTO_REL)
    FOREACH(srcfile ${ARGN})
        SRCS(${srcfile} PROTO_RELATIVE yes)
    ENDFOREACH ()
ENDMACRO ()


MACRO (BUILDWITH_PROTOC_REL srcfile var_dst_cc var_dst_hh)
    BUILDWITH_PROTOC_IMPL(${srcfile} ${var_dst_cc} ${var_dst_hh} TRUE ${ARGN})
ENDMACRO ()


MACRO (SET_PROTO_RELATIVE)
    SET_SOURCE_FILES_PROPERTIES(${ARGN} PROPERTIES PROTO_RELATIVE TRUE)
ENDMACRO ()

MACRO (GET_PROTO_RELATIVE srcfile var)
    GET_SOURCE_FILE_PROPERTY(${var} ${srcfile} PROTO_RELATIVE)
    IF (${var} STREQUAL "NOTFOUND")
        SET(${var} "${PROTO_RELATIVE_DEFAULT}")
    ENDIF ()
ENDMACRO ()


MACRO (BUILDWITH_PROTOC_EVENTS srcfile dstfile)
    TOOLDIR_EX(contrib/tools/protoc __protoc_)
    TOOLDIR_EX(contrib/tools/protoc/plugins/cpp_styleguide __cpp_styleguide_)
    TOOLDIR_EX(tools/event2cpp __proto_events_)

    GET_FILENAME_COMPONENT(LOCAL_PROTOC_DSTFILE ${dstfile} NAME_WE)
    GET_FILENAME_COMPONENT(LOCAL_PROTOC_DSTPATH ${dstfile} PATH)
    GET_FILENAME_COMPONENT(SRC_PROTOC_PATH ${srcfile} PATH)
    GET_FILENAME_COMPONENT(SRC_PROTOC_NAME ${srcfile} NAME)

    SET(LOCAL_PROTOC_DSTFILE_H "${LOCAL_PROTOC_DSTPATH}/${SRC_PROTOC_NAME}.pb.h")

    ADD_CUSTOM_COMMAND(
        OUTPUT
            ${dstfile}
            ${LOCAL_PROTOC_DSTFILE_H}
        COMMAND
            ${__protoc_} "--cpp_out=${BINDIR}"
            "--plugin=protoc-gen-cpp_styleguide=${__cpp_styleguide_}"
            "--cpp_styleguide_out=${BINDIR}"
            "--plugin=protoc-gen-event2cpp=${__proto_events_}"
            "--event2cpp_out=${BINDIR}"
            "-I."
            "-I${ARCADIA_ROOT}"
            "-I${ARCADIA_ROOT}/library/eventlog"
            "${SRC_PROTOC_NAME}"
        MAIN_DEPENDENCY
            "${srcfile}"
        WORKING_DIRECTORY
            ${SRC_PROTOC_PATH}
        DEPENDS
            protoc cpp_styleguide event2cpp "${srcfile}" ${ARGN}
        COMMENT
            "Events ${srcfile}: Building ${dstfile} and ${LOCAL_PROTOC_DSTFILE_H}"
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_INC(${LOCAL_PROTOC_DSTFILE_H})
    DIR_ADD_GENERATED_SRC(${dstfile})
ENDMACRO ()

# transform XML -> whatever using XSLT
#
# BUILDWITH_XSLTPROC(srcfile xslfile dstfile)

MACRO (BUILDWITH_XSLTPROC srcfile xslfile dstfile)
    TOOLDIR_EX(
        contrib/tools/xsltproc __xsltproc_
    )
    ADD_CUSTOM_COMMAND(
        OUTPUT
            ${dstfile}
        COMMAND
            ${__xsltproc_} -o ${dstfile} ${XSLTPROC_FLAGS} ${xslfile} ${srcfile}
        DEPENDS
            xsltproc ${srcfile} ${xslfile} ${ARGN}
        COMMENT
            "Building ${dstfile} with xsltproc"
    )
ENDMACRO ()

# DO NOT use thic macro in your projects. Use PEERDIR(library/svnversion) instead and
# include <library/svnversion/svnversion.h> file to define PROGRAM_VERSION macro.
MACRO (CREATE_SVNVERSION_FOR srcfile svdfile)
    IF (NO_UTIL_SVN_DEPEND)
        FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${svdfile})
    ELSE ()
        IF (IGNORE_SVNVERSION)
            CFLAGS(-DIGNORE_SVNVERSION)
        ELSE ()
            IF (EXISTS "${ARCADIA_ROOT}/.svn/wc.db")
                SET(SVN_DEPENDS "${ARCADIA_ROOT}/.svn/wc.db")
            ELSEIF (EXISTS "${ARCADIA_ROOT}/.svn/entries")
                SET(SVN_DEPENDS "${ARCADIA_ROOT}/.svn/entries")
            ELSEIF (EXISTS "${ARCADIA_ROOT}/.git")
                SET(SVN_DEPENDS "${ARCADIA_ROOT}/.git")
            ENDIF ()

            LUA(${ARCADIA_ROOT}/cmake/include/svn_info.lua "${ARCADIA_ROOT}/../" ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${REALPRJNAME}.dir/flags.make ${ARCADIA_BUILD_ROOT}
                IN ${ARCADIA_ROOT}/cmake/include/svn_info.lua ${SVN_DEPENDS}
                CWD ${CMAKE_CURRENT_BINARY_DIR}
                STDOUT ${svdfile}
            )
        ENDIF ()
    ENDIF ()
ENDMACRO ()

# This macro generates source files from .asp files
#
#  BUILDWITH_HTML2CPP(srcfile dstfile)

MACRO (BUILDWITH_HTML2CPP srcfile dstfile)
    TOOLDIR_EX(
        tools/html2cpp __html2cpp_
    )

    GET_FILENAME_COMPONENT(SRC_HTML2CPP_PATH ${srcfile} PATH)
    GET_FILENAME_COMPONENT(SRC_HTML2CPP_NAME ${srcfile} NAME)

    ADD_CUSTOM_COMMAND(
        OUTPUT
            ${dstfile}
        COMMAND
            ${__html2cpp_} ${srcfile} ${dstfile}
        MAIN_DEPENDENCY
            "${srcfile}"
        WORKING_DIRECTORY
            ${SRC_HTML2CPP_PATH}
        DEPENDS
            html2cpp "${srcfile}" ${ARGN}
        COMMENT
            "Building ${dstfile} with html2cpp"
    )

    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_SRC(${dstfile})
ENDMACRO ()

# Generates .inc from .sfdl
MACRO (BUILD_SFDL srcfile dstfile)
    TOOLDIR_EX(tools/calcstaticopt __calcstaticopt)

    FILE(TO_NATIVE_PATH ${srcfile} __srcfile_native)
    FILE(TO_NATIVE_PATH ${dstfile} __dstfile_native)

    GET_FILENAME_COMPONENT(__dstfile_dir ${__dstfile_native} PATH)
    GET_FILENAME_COMPONENT(__dstfile_namewe ${__dstfile_native} NAME_WE)
    SET(__tmpfile ${__dstfile_dir}/${__dstfile_namewe}.i)

    IF (WIN32)
        SET(__cpp_flags /E /C)
    ELSE ()
        IF (CMAKE_GENERATOR STREQUAL "Ninja")
            SET(__cpp_flags ${COMMON_CXXFLAGS} -I ${ARCADIA_ROOT} -I ${ARCADIA_BUILD_ROOT} -I ${ARCADIA_ROOT}/contrib/libs/stlport/stlport-${CURRENT_STLPORT_VERSION} -E -C -x c++) # $(CXX_FLAGS) works for make only, this is a hack to make it work for ninja
        ELSE ()
            SET(__cpp_flags $(CXX_DEFINES) $(CXX_FLAGS) -E -C -x c++)
        ENDIF ()
    ENDIF ()

    ADD_CUSTOM_COMMAND(
        OUTPUT ${dstfile}
        COMMAND ${RM} ${RM_FORCEFLAG} ${__dstfile_native}.tmp ${__dstfile_native}
        COMMAND ${ORIGINAL_CXX_COMPILER} ${__cpp_flags} ${__srcfile_native} >${__tmpfile}
        COMMAND ${__calcstaticopt} -i ${__tmpfile} -a ${ARCADIA_ROOT} >${__dstfile_native}.tmp
        COMMAND ${MV} ${MV_FORCEFLAG} ${__dstfile_native}.tmp ${__dstfile_native}
        MAIN_DEPENDENCY ${srcfile}
        DEPENDS ${srcfile} ${__calcstaticopt} ${ARGN}
        COMMENT "Generating ${dstfile}"
    )

    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_INC(${dstfile})
ENDMACRO ()

# Reads $prefix.in, generates $prefix.cpp, $prefix.h in binary directory.
# Does not add $prefix.cpp to SRCS(), instead adds a dependency on it to
FUNCTION (BASE_CODEGEN tool_path prefix)
    SET(__srcdirs_ ${CURDIR} ${SRCDIR})
    LIST(REMOVE_DUPLICATES __srcdirs_)

    FIND_FILE_IN_DIRS(__srcpath_ ${prefix}.in ${__srcdirs_})

    GET_FILENAME_COMPONENT(__file_ ${prefix} NAME)
    SET(__dstpath_ ${BINDIR}/${__file_})
    RUN_PROGRAM(${tool_path} ${__srcpath_} ${__file_}.cpp ${__file_}.h ${ARGN} CWD ${BINDIR} IN ${__srcpath_} OUT ${__dstpath_}.cpp ${__dstpath_}.h)

    SOURCE_GROUP("Custom Builds" FILES ${__srcpath_})
    DIR_ADD_GENERATED_SRC(${__dstpath_}.cpp)
    PROP_CURDIR_SET(PROJECT_WITH_CODEGEN yes)
ENDFUNCTION ()

MACRO (STRUCT_CODEGEN prefix)
    PEERDIR(
        kernel/struct_codegen/metadata
        kernel/struct_codegen/reflection
    )
    BASE_CODEGEN(kernel/struct_codegen/codegen_tool ${prefix})
ENDMACRO ()

MACRO (DUMPERF_CODEGEN prefix)
    BASE_CODEGEN(extsearch/images/robot/tools/dumperf/codegen ${prefix})
ENDMACRO ()

MACRO (BUILD_MN)
    SET (__check_ no)
    SET (__ptr_ no)
    SET (__multi_ no)
    SET (__mninfo_)
    SET (__mnname_)
    FOREACH(__arg_ ${ARGN})
        IF ("${__arg_}" STREQUAL "CHECK")
            SET (__check_ yes)
        ELSEIF ("${__arg_}" STREQUAL "PTR")
            SET (__ptr_ yes)
        ELSEIF ("${__arg_}" STREQUAL "MULTI")
            SET (__multi_ yes)
        ELSEIF ("X${__mninfo_}X" STREQUAL "XX")
            SET (__mninfo_ ${__arg_})
        ELSEIF ("X${__mnname_}X" STREQUAL "XX")
            SET (__mnname_ ${__arg_})
        ELSE ()
            MESSAGE("Warning: BUILD_MN has too many args: BUILDMN(${ARGN})")
        ENDIF ()
    ENDFOREACH ()

    IF ("X${__mnname_}X" STREQUAL "XX" OR "X${__mninfo_}X" STREQUAL "XX")
        MESSAGE(SEND_ERROR "Macro invoked with incorrect arguments. Usage: BUILDMN([CHECK] [PTR] [MULTI] mninfo mnname)")
    ENDIF ()

    SET (mnpath "${CMAKE_CURRENT_SOURCE_DIR}/${__mninfo_}")
    IF ( __multi_ )
        SET(mncpp "mnmulti.${__mnname_}.cpp")
    ELSE ()
        SET(mncpp "mn.${__mnname_}.cpp")
    ENDIF ()

    SET(__tools_ tools/archiver)
    IF ( __check_ )
        SET_APPEND(__tools_ tools/relev_fml_unused)
    ENDIF ()

    LUA(${ARCADIA_ROOT}/cmake/include/buildmn.lua ${__mninfo_} ${__mnname_} ${__check_} ${__ptr_} ${__multi_}
        ${mnpath} ${ARCADIA_ROOT} ${__tools_}
        TOOL ${__tools_}
        IN ${mnpath}
        OUT ${mncpp}
    )

    SOURCE_GROUP("Custom Builds" FILES ${mnpath})
    DIR_ADD_GENERATED_SRC(${mncpp})
ENDMACRO ()

MACRO (BUILD_MNS)
    SET(__mninfos_)
    SET(__next_name_)
    SET(__check_ no)
    SET(__listname_)
    FOREACH (__arg_ ${ARGN})
        IF ("${__arg_}" STREQUAL "CHECK")
            SET( __check_ yes)
        ELSEIF ("${__arg_}" STREQUAL "NAME")
            SET(__next_name_ yes)
        ELSE ()
            IF (${__next_name_})
                SET(__listname_ ${__arg_})
                SET(__next_name_ no)
            ELSE ()
                SET_APPEND(__mninfos_ ${__arg_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    IF ("X${__listname_}X" STREQUAL "XX" OR "X${__mninfos_}X" STREQUAL "XX")
        MESSAGE(SEND_ERROR "Macro invoked with incorrect arguments. Usage: BUILDMNS([CHECK] NAME mnname mninfos...)")
    ENDIF ()


    IF ( __mninfos_ )
        LIST(REMOVE_DUPLICATES __mninfos_)
        LIST(SORT __mninfos_)
    ENDIF ()

    SET(__name_ "mnmodels")
    SET(__hdrfile_ "${__name_}.h")
    SET(__srcfile_ "${__name_}.cpp")

    SET(__hdr_ "${BINDIR}/${__hdrfile_}")
    SET(__src_ "${BINDIR}/${__srcfile_}")
    SET(__tmp_hdr_ "${BINDIR}/.tmp.${__hdrfile_}")
    SET(__tmp_src_ "${BINDIR}/.tmp.${__srcfile_}")


    SET(__autogen_ "// DO NOT EDIT THIS FILE DIRECTLY, AUTOGENERATED!\n")
    SET(__mndecl_ "const NMatrixnet::TMnSseInfo")
    SET(__mnlistname_ "${__listname_}")
    SET(__mnlistelem_ "const NMatrixnet::TMnSsePtr*")
    SET(__mnlisttype_ "ymap< Stroka, ${__mnlistelem_} >")
    SET(__mnlist_ "const ${__mnlisttype_} ${__mnlistname_}")

    SET(__mnmultidecl_ "const NMatrixnet::TMnMultiCateg")
    SET(__mnmultilistname_ "${__listname_}Multi")
    SET(__mnmultilistelem_ "const NMatrixnet::TMnMultiCategPtr*")
    SET(__mnmultilisttype_ "ymap< Stroka, ${__mnmultilistelem_} >")
    SET(__mnmultilist_ "const ${__mnmultilisttype_} ${__mnmultilistname_}")

    FILE(WRITE  ${__tmp_hdr_} ${__autogen_})
    FILE(APPEND ${__tmp_hdr_} "\#include <kernel/matrixnet/mn_sse.h>\n")
    FILE(APPEND ${__tmp_hdr_} "\#include <kernel/matrixnet/mn_multi_categ.h>\n\n")
    FILE(APPEND ${__tmp_hdr_} "extern ${__mnlist_};\n")
    FILE(APPEND ${__tmp_hdr_} "extern ${__mnmultilist_};\n")

    SET(__mnnames_)
    SET(__mnmultinames_)
    FOREACH (__item_ ${__mninfos_})
        STRING(REGEX REPLACE "^(.*/)?([^/]+)\\.[^/]*$" "\\2" mnfilename "${__item_}")
        STRING(REGEX REPLACE "[^-a-zA-Z0-9_]" "_" mnname "${mnfilename}")

        IF ("${__item_}" MATCHES "\\.info$")
            SET_APPEND(__mnnames_ ${mnfilename})
            SET(mnname "staticMn${mnname}Ptr")
            IF ( __check_ )
                BUILD_MN(PTR CHECK ${__item_} ${mnname} )
            ELSE ()
                BUILD_MN(PTR ${__item_} ${mnname} )
            ENDIF ()
            FILE(APPEND ${__tmp_hdr_} "extern const NMatrixnet::TMnSsePtr ${mnname};\n")
        ELSEIF ("${__item_}" MATCHES "\\.mnmc$")
            SET_APPEND(__mnmultinames_ ${mnfilename})
            SET(mnname "staticMnMulti${mnname}Ptr")
            BUILD_MN(MULTI PTR ${__item_} ${mnname} )
            FILE(APPEND ${__tmp_hdr_} "extern const NMatrixnet::TMnMultiCategPtr ${mnname};\n")
        ENDIF ()

    ENDFOREACH ()

    FILE(WRITE  ${__tmp_src_} ${__autogen_})
    FILE(APPEND ${__tmp_src_} "\#include \"${__hdrfile_}\"\n\n")

    IF ( __mnnames_ )
        SET(__mndata_ "${__mnlistname_}_data")
        FILE(APPEND ${__tmp_src_} "static const TPair< Stroka, ${__mnlistelem_} > ${__mndata_}[] = {\n")
        FOREACH (__item_ ${__mnnames_})
            STRING(REGEX REPLACE "[^-a-zA-Z0-9_]" "_" mnname "${__item_}")
            FILE(APPEND ${__tmp_src_} "    MakePair(Stroka(\"${__item_}\"), &staticMn${mnname}Ptr),\n")
        ENDFOREACH ()
        FILE(APPEND ${__tmp_src_} "};\n")
        FILE(APPEND ${__tmp_src_} "${__mnlist_}(${__mndata_},${__mndata_} + sizeof(${__mndata_}) / sizeof(${__mndata_}[0]));\n\n")
    ELSE ()
        FILE(APPEND ${__tmp_src_} "${__mnlist_};\n\n")
    ENDIF ()

    IF ( __mnmultinames_ )
        SET(__mnmultidata_ "${__mnmultilistname_}_data")
        FILE(APPEND ${__tmp_src_} "static const TPair< Stroka, ${__mnmultilistelem_} > ${__mnmultidata_}[] = {\n")
        FOREACH (__item_ ${__mnmultinames_})
            STRING(REGEX REPLACE "[^-a-zA-Z0-9_]" "_" mnname "${__item_}")
            FILE(APPEND ${__tmp_src_} "    MakePair(Stroka(\"${__item_}\"), &staticMnMulti${mnname}Ptr),\n")
        ENDFOREACH ()
        FILE(APPEND ${__tmp_src_} "};\n")
        FILE(APPEND ${__tmp_src_} "${__mnmultilist_}(${__mnmultidata_},${__mnmultidata_} + sizeof(${__mnmultidata_}) / sizeof(${__mnmultidata_}[0]));")
    ELSE ()
        FILE(APPEND ${__tmp_src_} "${__mnmultilist_};")
    ENDIF ()

    EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E rename ${__tmp_hdr_} ${__hdr_}
        RESULT_VARIABLE iserror
    )
    IF (iserror)
        MESSAGE(SEND_ERROR Failed to overwrite ${__hdr_})
    ENDIF ()
    EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E rename ${__tmp_src_} ${__src_}
        RESULT_VARIABLE iserror
    )
    IF (iserror)
        MESSAGE(SEND_ERROR Failed to overwrite ${__src_})
    ENDIF ()

    DIR_ADD_GENERATED_SRC(${__hdr_})
    SRCS(${__hdr_})
    DIR_ADD_GENERATED_SRC(${__src_})
    SRCS(${__src_})
ENDMACRO ()


MACRO (BUILD_PLN polynom cpp)
    TOOLDIR_EX(
        tools/relev_fml_codegen __relev_fml_codegen_
    )

    SET(polpath "${CMAKE_CURRENT_SOURCE_DIR}/${polynom}")

    SET(cpptmp "${cpp}.tmp")

    ADD_CUSTOM_COMMAND (
        OUTPUT ${cpp} ${hdr}
        COMMAND ${__relev_fml_codegen_} -b -o "${cpptmp}" -f "${polpath}"
        COMMAND ${CMAKE_COMMAND} -E rename "${cpptmp}" "${cpp}"
        DEPENDS relev_fml_codegen ${polpath}
        COMMENT "Generating cpp code for polynom ${polynom}"
        VERBATIM
    )

    SOURCE_GROUP("Custom Builds" FILES ${polynom})

    DIR_ADD_GENERATED_SRC(${cpp})
    SRCS(${cpp})
ENDMACRO ()

SET(__polynoms_)

MACRO (BUILD_PLNS)
    SET(__polynoms_ ${ARGN})
    LIST(SORT __polynoms_)
    LIST(REMOVE_DUPLICATES __polynoms_)

    FOREACH (__item_ ${__polynoms_})
        GET_FILENAME_COMPONENT(__pol_ ${__item_} NAME_WE)
        BUILD_PLN("${__item_}" "${BINDIR}/pln.${__pol_}.cpp")
    ENDFOREACH ()

        SET(__hdr_ "${BINDIR}/plnmodels.h")
        SET(__pln_header_script_ "${ARCADIA_ROOT}/devtools/ymake/buildrules/scripts/BuildPlnHeader.py")
        PYTHON(${__pln_header_script_} ${__hdr_} ${__polynoms_} IN ${__polynoms_} OUT ${__hdr_})

    DIR_ADD_GENERATED_INC("${__hdr_}")
    SRCS("${__hdr_}")
ENDMACRO ()

MACRO (RESOURCE)
    SET(__is_file_next_ yes)
    SET(__files_)
    FOREACH(__arg_ ${ARGN})
        IF (__is_file_next_)
            SET_APPEND(__files_ ${__arg_})
            SET(__is_file_next_ no)
        ELSE ()
            SET(__is_file_next_ yes)
        ENDIF ()
    ENDFOREACH ()

    #TODO: make the file name shorter
    GET_UNIQ_NAME(__generated_ "resource_from_" TO_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR})
    SET(__output_ ${CMAKE_CURRENT_BINARY_DIR}/${__generated_}.cpp)
    RUN_PROGRAM(tools/rescompiler ${__output_} ${ARGN} IN ${__files_} OUT ${__output_})

    PEERDIR(
        library/resource
    )
    SRCS(GLOBAL ${__output_})
ENDMACRO ()

MACRO (COPY)
    SET(__from_dir_ ${CMAKE_CURRENT_SOURCE_DIR})
    SET(__dest_dir_ ${CMAKE_CURRENT_BINARY_DIR})
    SET(__next_dest_)
    SET(__next_result_)
    SET(__keep_dir_struct_)
    SET(__result_ "")
    SET(__files_)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "DESTINATION")
            SET(__next_dest_ yes)
        ELSEIF ("${__item_}" STREQUAL "FROM")
            SET(__next_from_ yes)
        ELSEIF ("${__item_}" STREQUAL "KEEP_DIR_STRUCT")
            SET(__keep_dir_struct_ yes)
        ELSEIF ("${__item_}" STREQUAL "RESULT")
            SET(__next_result_ yes)
        ELSE ()
            IF (__next_dest_)
                SET(__dest_dir_ ${__item_})
                SET(__next_dest_)
            ELSEIF (__next_from_)
                SET(__from_dir_ ${__item_})
                SET(__next_from_)
            ELSEIF (__next_result_)
                SET(__result_ ${__item_})
                SET(__next_result_)
            ELSE ()
                SET_APPEND(__files_ ${__item_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    FOREACH(__file_ ${__files_})
        SET(__dst_file_dir_ "${__dest_dir_}")

        IF (__keep_dir_struct_)
            GET_FILENAME_COMPONENT(__file_relative_path_ ${__file_} PATH)
            IF ("X${__file_relative_path_}X" STREQUAL "XX")
                SET(__dst_file_dir_ ${__dest_dir_})
            ELSE ()
                SET(__dst_file_dir_ "${__dest_dir_}/${__file_relative_path_}")
            ENDIF ()
        ENDIF ()
        GET_FILENAME_COMPONENT(__file_name_ ${__file_} NAME)

        SET(__file_abspath_ ${__file_})
        IF (EXISTS ${__from_dir_}/${__file_})
            SET(__file_abspath_ ${__from_dir_}/${__file_})
        ENDIF ()

        IF (IS_DIRECTORY ${__file_abspath_})
            SET(__copy_command_ copy_directory)
        ELSE ()
            SET(__copy_command_ copy)
        ENDIF ()

        ADD_CUSTOM_COMMAND(
                OUTPUT "${__dst_file_dir_}/${__file_name_}"
                COMMENT "${__copy_command_} ${__file_abspath_} to ${__dst_file_dir_}/${__file_name_}"
                COMMAND cmake -E ${__copy_command_} "${__file_abspath_}"
                                        "${__dst_file_dir_}/${__file_name_}"
                DEPENDS "${__file_abspath_}"
        )

        #This command works badly with cmake + make
        #FILE(COPY ${__file_} DESTINATION ${__dst_file_dir_})
        LIST(APPEND files_dest "${__dst_file_dir_}/${__file_name_}")
    ENDFOREACH ()

    IF (NOT "x${__result_}x" STREQUAL "xx")
        SET(${__result_} ${files_dest})
    ENDIF ()

    GET_UNIQ_NAME(__target_name_ "copy_from" TO_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR})
    ADD_CUSTOM_TARGET(${__target_name_} ALL DEPENDS ${files_dest})
ENDMACRO ()

MACRO (SYMLINK from to)
    PYTHON(${ARCADIA_ROOT}/devtools/ymake/buildrules/scripts/symlink.py ${from} ${to}
        IN ${from}
        OUT ${to}
    )

    GET_UNIQ_NAME(__target_name_ "symlink_from" TO_RELATIVE ${from} "_to" TO_RELATIVE ${to})
    ADD_CUSTOM_TARGET(${__target_name_} ALL DEPENDS ${to})
    IF(${SAVEDTYPE} STREQUAL "PACKAGE")
        PROP_CURDIR_GET(__pkg_target_name_ PACKAGE_TARGET_NAME)
        ADD_DEPENDENCIES(${__target_name_} ${__pkg_target_name_})
    ENDIF ()
ENDMACRO ()

# This macro is used inside CMakeLists.txt but has no sense in cmake
MACRO (INDUCED_DEPS)
ENDMACRO ()

MACRO(GET_UNIQ_NAME result)
    SET(__next_abs_pathes_)
    SET(__strs_)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "TO_RELATIVE")
            SET(__next_abs_pathes_ yes)
        ELSE ()
            IF (__next_abs_pathes_)
                STRING(REGEX REPLACE "^${ARCADIA_ROOT}/" "" __rp_ ${__item_})
                STRING(REGEX REPLACE "^${ARCADIA_BUILD_ROOT}/" "" __rp_ ${__rp_})
                SET_APPEND(__strs_ ${__rp_})
                SET(__next_abs_pathes_)
            ELSE ()
                SET_APPEND(__strs_ ${__item_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    JOIN("${__strs_}" "_" __joined_args_)
    STRING(REPLACE "/" "_" __uniq_name_ "${__joined_args_}")

    IF ("X${${__uniq_name_}_uid_}X" STREQUAL "XX")
        SET(${__uniq_name_}_uid_ 0)
    ENDIF ()
    MATH(EXPR ${__uniq_name_}_uid_ "${${__uniq_name_}_uid_} + 1")
    SET(__uid_ ${${__uniq_name_}_uid_})

    SET(${result} "${__uniq_name_}_${__uid_}")
    SET(__next_abs_pathes_)
    SET(__strs_)
ENDMACRO ()

MACRO(JOIN values glue output)
    STRING (REPLACE ";" "${glue}" _tmp_str "${values}")
    SET (${output} "${_tmp_str}")
ENDMACRO ()


TOOLDIR_EX(
    contrib/tools/bison/bison BISON
    contrib/tools/bison/m4 M4
)

DEFAULT(BISON_FLAGS -v)

MACRO (BUILDWITH_BISON srcfile dstfile)
    IF (NOT BISON)
        MESSAGE(SEND_ERROR "BISON is not defined. Please check if contrib/tools/bison/bison is checked out")
    ENDIF ()
    IF (NOT M4)
        MESSAGE(SEND_ERROR "M4 is not defined. Please check if contrib/tools/bison/m4 is checked out")
    ENDIF ()
    GET_FILENAME_COMPONENT(LOCAL_BISON_DSTFILE_FILE ${dstfile} NAME_WE)
    GET_FILENAME_COMPONENT(LOCAL_BISON_DSTFILE_PATH ${dstfile} PATH)
    SET(LOCAL_BISON_DSTFILE_H "${LOCAL_BISON_DSTFILE_PATH}/${LOCAL_BISON_DSTFILE_FILE}.h")
    ADD_CUSTOM_COMMAND(
        OUTPUT ${dstfile} ${LOCAL_BISON_DSTFILE_H}
        COMMAND "${BISON}" ${BISON_FLAGS} --m4=${M4} -d -o "${dstfile}" "${srcfile}"
        MAIN_DEPENDENCY "${srcfile}"
        DEPENDS "${srcfile}" bison m4 ${ARGN}
        COMMENT "Building \"${dstfile}\"+\"${LOCAL_BISON_DSTFILE_H}\" with bison"
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    DIR_ADD_GENERATED_INC(${LOCAL_BISON_DSTFILE_H})
    DIR_ADD_GENERATED_SRC(${dstfile})
ENDMACRO ()
