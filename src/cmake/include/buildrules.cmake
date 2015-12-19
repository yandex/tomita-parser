#This file contains macros which prepare variables and define macros used later in cmake-file

CMAKE_POLICY(SET CMP0026 OLD)
CMAKE_POLICY(SET CMP0045 OLD)
CMAKE_POLICY(SET CMP0054 OLD)

IF (NOT ARCADIA_ROOT)
    # At this stage it is absolutely mandatory to have this variable defined
    MESSAGE(FATAL_ERROR "ARCADIA_ROOT is not defined")
ENDIF ()

INCLUDE(${ARCADIA_ROOT}/cmake/include/InternalHelpers.cmake)
INCLUDE(${ARCADIA_ROOT}/cmake/include/dtmk.cmake)
INCLUDE(${ARCADIA_ROOT}/cmake/include/buildlibs.cmake)

IF (WIN32)
    SET(TOUCH_CMD ${ARCADIA_ROOT}/devtools/ymake/buildrules/scripts/touch.py)
ELSE ()
    SET(TOUCH_CMD touch)
ENDIF ()

# We need the file to exist, not be recent; if it doesn't exist or has a fresh
# mtime after this rule, all objects would be recompiled.
# However, if the file is kept old, touch will be called every single time; it
# is typically very fast, but for windows might interfere with virus checking
# and slow down drastically so recompiling is faster since it is rare.
IF (WIN32)
    MACRO (GET_BARRIER_TOUCH_CMD var file)
        SET(${var} copy /b/y nul ${file} > nul)
    ENDMACRO ()

ELSEIF (DEFINED USE_BARRIER_RECOMPILE)
    MACRO (GET_BARRIER_TOUCH_CMD var file)
        SET(${var} touch ${file})
    ENDMACRO ()

ELSE ()
    MACRO (GET_BARRIER_TOUCH_CMD var file)
        SET(${var} touch -t 200001010000 ${file} || true)
    ENDMACRO ()

ENDIF ()

# =============================================================================
# RUN_PROVE()
#
MACRO (RUN_PROVE)
    IF (USE_PERL)
        ADD_CUSTOM_TARGET(
            "${PROJECT_NAME}RunProve" ALL
            COMMAND ${PERL} -e \"use App::Prove\; \\\$$app=App::Prove->new\; \\\$$app->process_args('-v') \; exit(\\\$$app->run?0:1) \"
            DEPENDS "${PROJECT_NAME}"
            COMMENT "Running prove..."
        )
    # " <- for syntax highlighting
    ELSE ()
        MESSAGE_EX("You should only use RUN_PROVE() with USE_PERL option enabled" )
        MESSAGE(FATAL_ERROR)
    ENDIF ()
ENDMACRO ()

MACRO (ANALYZE_PEERDIRS __peerdirs_var_)
    SET(__need_proto_deps_)
    SET(__need_ev_deps_)
    FOREACH(__dir_ ${${__peerdirs_var_}})
        PROP_DIR_GET(__has_proto_ ${ARCADIA_ROOT}/${__dir_} PROJECT_WITH_PROTO)
        PROP_DIR_GET(__has_ev_ ${ARCADIA_ROOT}/${__dir_} PROJECT_WITH_EV)
        PROP_DIR_GET(__has_codegen_ ${ARCADIA_ROOT}/${__dir_} PROJECT_WITH_CODEGEN)
        IF (__has_proto_ OR __has_ev_ OR __has_codegen_)
            BUILDAFTER(${__dir_})
            IF (__has_ev_)
                ENABLE(__need_ev_deps_)
            ENDIF ()
            IF (__has_proto_)
                ENABLE(__need_proto_deps_)
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    FILE(RELATIVE_PATH __from_dir_ ${ARCADIA_ROOT} ${CURDIR})
    IF (__need_proto_deps_)
        ADDINCL(contrib/libs/protobuf)
        SET_APPEND(${__peerdirs_var_} contrib/libs/protobuf)
        ADD_DIRS(PEERDIR contrib/libs/protobuf)
    ENDIF ()
    IF (__need_ev_deps_)
        BUILDAFTER(library/eventlog)
        ADDINCL(library/eventlog)
        SET_APPEND(${__peerdirs_var_} library/eventlog)
        ADD_DIRS(PEERDIR library/eventlog)
    ENDIF ()
ENDMACRO ()

# =============================================================================
# PEERDIR(dirs)
#
MACRO (PEERDIR)
    SET(__peerdirs_)
    SET(__next_addincl_)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "ADDINCL")
            SET(__next_addincl_ yes)
        ELSEIF ("${__item_}" STREQUAL "AS")
            SET(__next_as_ yes)
        ELSE ()
            SET(__realdir_ ${ARCADIA_ROOT}/${__item_})
            IF (__next_as_)
                IF (NOT ${SAVEDTYPE} STREQUAL "PACKAGE")
                    MESSAGE_EX("PEERDIR: keyword AS can be used only in PACKAGE")
                ELSE()
                    MESSAGE_EX("PEERDIR: keyword AS inside PACKAGE macro is not supported yet")
                ENDIF ()
                SET(__next_as_)
            ELSEIF (NOT EXISTS ${__realdir_})
                MESSAGE_EX("PEERDIR ${__item_} that does not exist")
            ELSEIF (NOT EXISTS ${__realdir_}/CMakeLists.txt AND NOT EXISTS ${__realdir_}/${METALIBRARY_NAME})
                MESSAGE_EX("PEERDIR to ${__item_}: CMakeLists.txt does not exist")
            ELSE ()
                SET_APPEND(__peerdirs_ ${__item_})
                IF (__next_addincl_)
                    ADDINCL(${__item_})
                ENDIF ()
                DEBUGMESSAGE(3 "PEERDIR( ${__item_} ) is found (${__realdir_})")
            ENDIF ()

            IF (__next_addincl_)
                SET(__next_addincl_)
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()
    ADD_DIRS(PEERDIR ${__peerdirs_})
    ANALYZE_PEERDIRS(__peerdirs_)

    FOREACH(__dir_ ${__peerdirs_})
        IS_METALIBRARY(__is_meta_ ${__dir_})
        PROP_DIR_GET(__dir_type_ ${ARCADIA_ROOT}/${__dir_} PROJECT_TYPE)
        PROP_DIR_GET(__is_shared_lib_ ${ARCADIA_ROOT}/${__dir_} IS_SHARED_LIB)
        IF("${SAVEDTYPE}" STREQUAL "PACKAGE")
            PROP_DIR_GET(__prj_target_name_ ${ARCADIA_ROOT}/${__dir_} PROJECT_TARGET_NAME)
            SET_APPEND(PACKAGE_PEERDIR_DEPS ${__prj_target_name_})
        ELSEIF (${__is_shared_lib_})
            MESSAGE_EX("PEERDIR to DLL - ${__dir_}")
        ENDIF ()
        IF (NOT __is_meta_)
            GET_GLOBAL_KEYWORD_PROPERTY("ExplicitDepsForProgs" __prog_deps_ ${__dir_})
            IF (NOT "X${__prog_deps_}X" STREQUAL "XX")
                ADD_GLOBAL_KEYWORD_PROPERTY("ExplicitDepsForProgs" ${__prog_deps_})
            ENDIF ()
            GET_GLOBAL_KEYWORD_PROPERTY("ExplicitDepsForSo" __so_deps_ ${__dir_})
            IF (NOT "X${__so_deps_}X" STREQUAL "XX")
                ADD_GLOBAL_KEYWORD_PROPERTY("ExplicitDepsForSo" ${__so_deps_})
            ENDIF ()
            GET_GLOBAL_KEYWORD_PROPERTY("ExtraLibs" __extra_libs_ ${__dir_})
            IF (NOT "X${__extra_libs_}X" STREQUAL "XX")
                ADD_GLOBAL_KEYWORD_PROPERTY("ExtraLibs" ${__extra_libs_})
            ENDIF ()
            GET_GLOBAL_KEYWORD_PROPERTY("AddinclGlobal" __global_addincls_ ${__dir_})
            IF (NOT "X${__global_addincls_}X" STREQUAL "XX")
                ADD_GLOBAL_KEYWORD_PROPERTY("AddinclGlobal" ${__global_addincls_})
            ENDIF ()
            GET_GLOBAL_KEYWORD_PROPERTY("CFlagsGlobal" __global_cflags_ ${__dir_})
            IF (NOT "X${__global_cflags_}X" STREQUAL "XX")
                ADD_GLOBAL_KEYWORD_PROPERTY("CFlagsGlobal" ${__global_cflags_})
            ENDIF ()
            GET_GLOBAL_KEYWORD_PROPERTY("BuildAfterGlobal" __global_build_after_ ${__dir_})
            IF (NOT "X${__global_build_after_}X" STREQUAL "XX")
                ADD_GLOBAL_KEYWORD_PROPERTY("BuildAfterGlobal" ${__global_build_after_})
            ENDIF ()
        ENDIF ()

        IF(${NORUNTIME})
            PROP_DIR_GET(__has_noruntime_ ${ARCADIA_ROOT}/${__dir_} NORUNTIME)
            IF(NOT __has_noruntime_)
                MESSAGE_EX("PEERDIR from project with NORUNTIME to project without NORUNTIME - ${__dir_}")
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

SET (__allocators_ GOOGLE J LF B LOCKLESS)
MACRO (ALLOCATOR alloc)
    IF("${SAVEDTYPE}" STREQUAL "LIB" AND NOT MAKE_ONLY_SHARED_LIB)
        MESSAGE_EX("ALLOCATOR can not be used inside LIBRARY")
        MESSAGE(SEND_ERROR)
    ELSE ()
        FOREACH (__alloc_ ${__allocators_})
            SET(USE_${__alloc_}_ALLOCATOR no)
        ENDFOREACH ()
        SET(USE_${alloc}_ALLOCATOR yes)
    ENDIF ()
ENDMACRO ()

# =============================================================================
# TOOLDIR_EX(pairs of <dir varname> )
#
MACRO (TOOLDIR_EX)
    SET(__isdir_ yes)
    SET(__dirs_)
    SET(__vars_)
    FOREACH(__item_ ${ARGN})
        IF (__isdir_)
            SET(__lastdir_ "${__item_}")
            SET(__isdir_ no)
        ELSE ()
            SET_APPEND(__dirs_ "${__lastdir_}")
            SET(__lastdir_)
            SET_APPEND(__vars_ "${__item_}")
            SET(__isdir_ yes)
        ENDIF ()
    ENDFOREACH ()
    IF (__lastdir_)
        MESSAGE_EX("TOOLDIR_EX has odd number of parameters (last is [${__lastdir_}])")
        MESSAGE(SEND_ERROR)
    ENDIF ()

    ADD_DIRS(TOOLDIR ${__dirs_})

    FOREACH(__item_ ${__dirs_})
        LIST(GET __vars_ 0 __var_)
        LIST(REMOVE_AT __vars_ 0)
        GET_GLOBAL_DIRNAME(__dir_ ${ARCADIA_ROOT}/${__item_})
        IF (NOT "X${${__dir_}_DEPENDNAME_PROG}X" STREQUAL "XX")
            GETTARGETNAME(${__var_} ${${__dir_}_DEPENDNAME_PROG})
            DEBUGMESSAGE(2 "TOOLDIR in ${__item_} is [${${__dir_}_DEPENDNAME_PROG}]")
        ELSE ()
            SET(${__var_} NOTFOUND)
            MESSAGE_EX("TOOLDIR in ${__item_} is not defined")
            MESSAGE(SEND_ERROR)
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

# =============================================================================
# TOOLDIR(dirs)
#
MACRO (TOOLDIR)
    ADD_DIRS(TOOLDIR ${ARGN})
ENDMACRO ()

# =============================================================================
# SRCDIR(dirs)
#
MACRO (SRCDIR)
    SET(__dirs_)
    PREPARE_DIR_ARGS(__dirs_ ${ARGN})
    SET_APPEND(SRCDIR ${__dirs_})
ENDMACRO ()

MACRO (EXTRADIR)
ENDMACRO ()

# =============================================================================
# ADDINCL(dirs) - adds directories to -I switches
#
MACRO (ADDINCL)
    SET(__dirs_)
    PREPARE_DIR_ARGS(__dirs_ ${ARGN})

    SET(__global_addincls_)
    SET(__next_global_)
    FOREACH(__item_ ${__dirs_})
        IF ("${__item_}" STREQUAL "GLOBAL")
            SET(__next_global_ yes)
        ELSE ()
            IF (__next_global_)
                SET_APPEND(__global_addincls_ ${__item_})
                SET(__next_global_)
            ENDIF ()
            IF (IS_ABSOLUTE ${__item_})
                SET_APPEND(DTMK_I ${__item_})
            ELSE ()
                SET_APPEND(DTMK_I ${ARCADIA_ROOT}/${__item_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    IF (NOT "X${__global_addincls_}X" STREQUAL "XX")
        ADD_GLOBAL_KEYWORD_PROPERTY("AddinclGlobal" ${__global_addincls_})
    ENDIF ()
ENDMACRO ()

# =============================================================================
# CFLAGS(flags) and CXXFLAGS(flags)
#
MACRO (CFLAGS)
    SET(__global_cflags_)
    SET(__next_global_)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "GLOBAL")
            SET(__next_global_ yes)
        ELSE ()
            IF (__next_global_)
                SET_APPEND(__global_cflags_ ${__item_})
                SET(__next_global_)
            ENDIF ()
            SET_APPEND(CFLAGS ${__item_})
        ENDIF ()
    ENDFOREACH ()

    IF (NOT "X${__global_cflags_}X" STREQUAL "XX")
        ADD_GLOBAL_KEYWORD_PROPERTY("CFlagsGlobal" ${__global_cflags_})
    ENDIF ()
ENDMACRO ()

MACRO (CXXFLAGS)
    SET_APPEND(CXXFLAGS ${ARGN})
ENDMACRO ()

MACRO (OBJADDE)
    SET_APPEND(OBJADDE ${ARGN})
ENDMACRO ()

MACRO (EXTRALIBS)
    IF (NOT "X${ARGN}X" STREQUAL "XX")
        ADD_GLOBAL_KEYWORD_PROPERTY("ExtraLibs" ${ARGN})
        OBJADDE(${ARGN})
    ENDIF ()
ENDMACRO ()

# =============================================================================
# CUDA_NVCC_FLAGS(flags)
#
MACRO (CUDA_NVCC_FLAGS)
    SET_APPEND(CUDA_NVCC_FLAGS ${ARGN})
ENDMACRO ()

MACRO(NORUNTIME)
    NOUTIL()
    ENABLE(NORUNTIME)
    PROP_CURDIR_SET(NORUNTIME yes)
ENDMACRO ()

# =============================================================================
# SRCS(prjname)
#
MACRO (SRCS)
    SET(__next_global_src_)
    SET(__next_prop_)
    SET(__cursrc_)
    SET(__item_global_)
    SET(__compilable_ NO)
    SET(__flagnames_ COMPILE_FLAGS OBJECT_DEPENDS DEPENDS PROTO_RELATIVE)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "GLOBAL")
            SET(__next_global_src_ yes)
        ELSEIF (__next_prop_)
            IF (NOT __cursrc_)
                MESSAGE_EX("No source file name found to set ${__next_prop_} property")
                MESSAGE(FATAL_ERROR)
            ENDIF ()
            # Set property for a given file
            IF (NOT DEFINED __item_global_)
                GET_GLOBAL_DIRNAME(__item_global_ "${__cursrc_}")
                IF (NOT __compilable_)
                    SPLIT_FILENAME(__file_namewe_ __file_ext_ ${__cursrc_})
                    IS_COMPILABLE_TYPE(__compilable_ ${__file_ext_})
                    IF (__compilable_)
                        PROP_CURDIR_SET(DIR_HAS_PROPS YES)
                    ENDIF ()
                ENDIF ()
            ENDIF ()
            SET_APPEND(${__item_global_}_PROPS "${__next_prop_};${__item_}")
            DEBUGMESSAGE(1 "${__item_global_}_PROPS[${${__item_global_}_PROPS}]")
            SET(__next_prop_)
        ELSE ()
            SET(__is_flag_)
            FOREACH(__flagname_ ${__flagnames_})
                IF (__item_ STREQUAL "${__flagname_}")
                    SET(__is_flag_ yes)
                ENDIF ()
            ENDFOREACH ()

            IF (__is_flag_)
                SET(__next_prop_ "${__item_}")
            ELSE ()
                IF (__next_global_src_)
                    ADD_OBJ_FOR_LINK(${__item_})
                    SET(__next_global_src_)
                ENDIF ()
                SET_APPEND(SRCS "${__item_}")
                SET(__cursrc_ "${__item_}")
                SET(__item_global_)
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

MACRO (PREPARE_DIR_ARGS __dirs_var)
    SET(__prefix_)
    SET(__dir_args_)
    FOREACH (__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "LOCAL")
            FILE(RELATIVE_PATH __prefix_ "${ARCADIA_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
        ELSE ()
            SET_APPEND(__dir_args_ ${__item_})
        ENDIF ()
    ENDFOREACH ()

    IF (__prefix_)
        FOREACH (__item_ ${__dir_args_})
            SET_APPEND(${__dirs_var} ${__prefix_}/${__item_})
        ENDFOREACH ()
    ELSE ()
        SET(${__dirs_var} ${ARGN})
    ENDIF ()
ENDMACRO ()

MACRO (RECURSE_ROOT_RELATIVE)
    ENTER_PROJECT()
    IF (NOT "X${ARGN}X" STREQUAL "XX")
        ADD_DIRS(#SUBDIR
            ${ARGN}
        )
    ELSE ()
        MESSAGE_EX("RECURSE_ROOT_RELATIVE without args")
    ENDIF ()
    LEAVE_PROJECT()
ENDMACRO ()

MACRO (RECURSE)
    IF (NOT "X${ARGN}X" STREQUAL "XX")
        SET(__dirs_)
        FILE(RELATIVE_PATH __prefix_ "${ARCADIA_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
        IF (NOT "X${__prefix_}X" STREQUAL "XX")
            FOREACH (__item_ ${ARGN})
                SET_APPEND(__dirs_ ${__prefix_}/${__item_})
            ENDFOREACH ()
        ELSE ()
            SET(__dirs_ ${ARGN})
        ENDIF ()
        RECURSE_ROOT_RELATIVE(${__dirs_})
    ELSE ()
        MESSAGE_EX("RECURSE without args")
    ENDIF ()
ENDMACRO ()

MACRO (ARCHIVE)
    SET(__afiles)
    SET(__aparams)
    SET(__next_name)
    SET(__dont_compress)
    SET(__name "archive.inc")

    FOREACH (__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "NAME")
            SET(__next_name yes)
        ELSEIF ("${__item_}" STREQUAL "DONTCOMPRESS")
            SET(__dont_compress yes)
        ELSE ()
            IF (__next_name)
                SET(__name "${__item_}")
                SET(__next_name)
            ELSE ()
                GET_SRC_ABS_PATH(__afile_ ${__item_})

                SET_APPEND(__afiles ${__afile_})
                SET_APPEND(__aparams "${__afile_}:")
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    SET(__archiver_flags -x -q)
    IF (__dont_compress)
      SET_APPEND(__archiver_flags -p)
    ENDIF ()

    RUN_PROGRAM(tools/archiver ${__archiver_flags} ${__aparams} STDOUT ${__name} CWD ${CMAKE_CURRENT_BINARY_DIR} IN ${__afiles})

    SPLIT_FILENAME(__namewe __ext "${__name}")
    IF (NOT "${__ext}" STREQUAL ".inc")
        DIR_ADD_GENERATED_INC(${CMAKE_CURRENT_BINARY_DIR}/${__name})
    ENDIF ()

    SET(__afiles)
    SET(__aparams)
    SET(__next_name)
    SET(__name)
ENDMACRO ()

# =============================================================================
# BUILD_ONLY_IF(variables_list...) and NO_BUILD_IF(variables_list...)
# BUILD_ONLY_IF and NO_BUILD_IF macroses only writes warning messages.
#   Use BUILD_ONLY_IF macro when you need to create a project only when 1+ of listed
# variables is positive.
#   Use NO_BUILD_IF macro when you don't want a project to be created if any of
# listed variables is positive.

MACRO(BUILD_ONLY_IF)
    SET(__send_warning_ yes)
    FOREACH(__item_ ${ARGN})
        IF (${__item_})
            SET(__send_warning_ no)
        ENDIF ()
    ENDFOREACH ()

    IF (${__send_warning_})
        MESSAGE_EX("should not be built, because it is build only if one of '${ARGN}' is true")
    ENDIF ()
ENDMACRO ()

MACRO(NO_BUILD_IF)
    FOREACH(__item_ ${ARGN})
        IF (${__item_})
            MESSAGE_EX("should not be built, because '${item}' is true")
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

MACRO(EXCLUDE_FROM_ALL)
    SET(LOCAL_EXCLUDE_FROM_ALL yes)
ENDMACRO ()

# =============================================================================
# BUILDAFTER(dirs ...)
#
MACRO (BUILDAFTER)
    IF (NOT "X${ARGN}X" STREQUAL "XX")
        ADD_GLOBAL_KEYWORD_PROPERTY("BuildAfterGlobal" ${ARGN})

        ADD_DIRS(${ARGN})
        FOREACH(__item_ ${ARGN})
            GET_GLOBAL_DIRNAME(__dir_ ${ARCADIA_ROOT}/${__item_})
            IF (NOT ${ARCADIA_ROOT}/${__item_} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
                ENABLE(__invalid_)
                FOREACH(__depend_ ${__dir_}_DEPENDNAME_PROG ${__dir_}_DEPENDNAME_LIB)
                    IF (NOT "X${${__depend_}}X" STREQUAL "XX")
                        LIST(APPEND THISPROJECTDEPENDS ${${__depend_}})
                        LIST(APPEND THISPROJECTDEPENDSDIRSNAMES ${__item_})
                        DEBUGMESSAGE(2 "BUILDAFTER @ ${CURDIR}: ${__item_} is [${${__depend_}}]")
                        DISABLE(__invalid_)
                    ENDIF ()
                ENDFOREACH ()
                IF (__invalid_)
                    MESSAGE_EX("BUILDAFTER: not found a target in ${__item_}")
                    MESSAGE(SEND_ERROR)
                ENDIF ()
            ENDIF ()
        ENDFOREACH ()
    ENDIF ()
ENDMACRO ()

# =============================================================================
# ENTER_PROJECT(prjname)
#
MACRO (ENTER_PROJECT)
    IF (PRINTCURPROJSTACK)
        IF (ARGN)
            SET(__prjname_ ${ARGN})
        ELSE ()
            FILE(RELATIVE_PATH __var_ "${ARCADIA_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
            GET_FILENAME_COMPONENT(__prjname_ ${__var_} PATH)
        ENDIF ()
        SET(CURPROJSTACK ${__prjname_} ${CURPROJSTACK})
        DEBUGMESSAGE(2 "Enter. Current project stack: [${CURPROJSTACK}]")
    ELSE ()
        DEBUGMESSAGE(2 "PRINTCURPROJSTACK is \"${PRINTCURPROJSTACK}\"")
    ENDIF ()
ENDMACRO ()

# =============================================================================
# LEAVE_PROJECT()
#
MACRO (LEAVE_PROJECT)
    IF (PRINTCURPROJSTACK)
        DEBUGMESSAGE(2 "Exit.  Current project stack: [${CURPROJSTACK}]")
        IF (CURPROJSTACK)
            LIST(REMOVE_AT CURPROJSTACK 0)
        ENDIF ()
    ENDIF ()
ENDMACRO ()


#IF (NOT DEFINED ARCADIA_BUILD_ROOT OR NOT EXISTS ${ARCADIA_BUILD_ROOT})
#   IF (PATH_TO_ROOT)
#       SET(ARCADIA_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${PATH_TO_ROOT}) # Assume that first cmake ran in
#       GET_FILENAME_COMPONENT(ARCADIA_BUILD_ROOT ${ARCADIA_BUILD_ROOT} ABSOLUTE)
#   ELSE ()
#       SET(ARCADIA_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR}) # Assume that first cmake ran in
#       GET_FILENAME_COMPONENT(ARCADIA_BUILD_ROOT ${ARCADIA_BUILD_ROOT} ABSOLUTE)
#       SET(ARCADIA_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "")
#   ENDIF ()
#ENDIF ()

# This macro checks whether a directory contains metalibrary structure
MACRO (IS_METALIBRARY result_varname path)
    SET(${result_varname} no)
    IF (1 AND EXISTS ${ARCADIA_ROOT}/${path}/${METALIBRARY_NAME} AND NOT EXISTS ${ARCADIA_ROOT}/${path}/CMakeLists.txt)
        SET(${result_varname} yes)
    ENDIF ()
ENDMACRO ()

# =============================================================================
# ADD_ARC_DIRS(var_to_remove dirtype ...)
#
#
MACRO (ADD_ARC_DIRS var_to_remove __dirtype_)
    DEBUGMESSAGE(2 "${CURDIR}: PEERDIR[${PEERDIR}], TOOLDIR[${TOOLDIR}], SUBDIR[${SUBDIR}], ARGN[${ARGN}]")

    SET(__peerdir_ ${PEERDIR})
    SET(__subdir_ ${SUBDIR})
    SET(__tooldir_ ${TOOLDIR})
    SET(__prj_def_ ${PROJECT_DEFINED})
    SET(PEERDIR)
    SET(SUBDIR)
    SET(TOOLDIR)
    SET(PROJECT_DEFINED)
    SET(ADD_PEERDIR)
    # stupid MACRO arguments are neither strings nor variables; however,
    # stupid IF expands arguments as variables so can't use either directly
    IF ("${__dirtype_}" STREQUAL "PEERDIR")
        ENABLE(__is_peerdir_)
    ELSE ()
        DISABLE(__is_peerdir_)
    ENDIF ()
    FOREACH(__item_ ${ARGN})
        SET(__is_meta_ ${__is_peerdir_})
        IF (__is_meta_)
            IS_METALIBRARY(__is_meta_ ${__item_})
        ENDIF ()
        IF (__is_meta_)
            SET_APPEND(ADD_PEERDIR ${__item_})
            DEBUGMESSAGE(1 "Metalib in ${__item_}")
        ELSE ()
            ADD_SUBDIRECTORY_EX(${ARCADIA_ROOT}/${__item_} ${ARCADIA_BUILD_ROOT}/${__item_})
        ENDIF ()
    ENDFOREACH ()
    SET(PEERDIR ${__peerdir_})
    SET(SUBDIR ${__subdir_})
    SET(TOOLDIR ${__tooldir_})
    SET(PROJECT_DEFINED ${__prj_def_})

    PROP_CURDIR_GET(__dep_dirs DEPENDENT_DIRECTORIES)
    IF(NOT "X${__dep_dirs}X" STREQUAL "XX")
        LIST(REMOVE_DUPLICATES __dep_dirs)
        PROP_CURDIR_SET(DEPENDENT_DIRECTORIES ${__dep_dirs})
    ENDIF ()

    IF (ADD_PEERDIR)
        DEBUGMESSAGE(1 "ADD_PEERDIR[${ADD_PEERDIR}]")
        SET(${SAVEDNAME}_ADD_PEERDIR ${ADD_PEERDIR})
    ENDIF ()

    # Now include all collected metalib files
    FOREACH(__item_ ${ADD_PEERDIR})
        SET(__metadir_ ${ARCADIA_ROOT}/${__item_})
        SET(__metaname_ ${__metadir_}/${METALIBRARY_NAME})
        SAVE_VARIABLES(SAVEDNAME PROJECT_DEFINED)
        DEBUGMESSAGE(2 "METALIBRARY: INCLUDE(${__metaname_} @ ${CURDIR}")
        INCLUDE(${__metaname_})
        CHECK_SAVED_VARIABLES(__changed_ SAVEDNAME PROJECT_DEFINED)
        IF (__changed_)
            MESSAGE_EX("buildrules.cmake: ${__metaname_} modified following variables: [${__changed_}]. PROJECT_DEFINED: ${__saved_${PROJECT_DEFINED}} -> ${PROJECT_DEFINED}. SAVEDNAME: ${__saved_${SAVEDNAME}} -> ${SAVEDNAME}")
            MESSAGE_EX("buildrules.cmake: Possibly it defines a target and it is prohibited for .lib-files.")
            MESSAGE(SEND_ERROR)
        ENDIF ()
        SRCDIR(${__item_})
        SET_APPEND(DTMK_I ${__metadir_})
    ENDFOREACH ()

    IF (${SAVEDNAME}_ADD_PEERDIR)
        SET(${var_to_remove} ${${SAVEDNAME}_ADD_PEERDIR})
        DEBUGMESSAGE(1 "ADD_PEERDIR to remove: ${${var_to_remove}}")
    ENDIF ()
ENDMACRO ()

# =============================================================================
# ADD_DIRS( (PEERDIR | SUBDIR | TOOLDIR) ...)
#
#
MACRO (ADD_DIRS varname)
    SET(__to_remove_)
    SET(varname ${varname}) # need this since IF(macro_argument) fails
    IF (varname MATCHES ".DIR$")
        ADD_ARC_DIRS(__to_remove_ ${ARGV})
        SET_APPEND(${ARGV})
        DEBUGMESSAGE(2 "varname:${varname}=[${${varname}}]")
#        DEBUGMESSAGE(1 "__to_remove_[${__to_remove_}]")
        IF (__to_remove_)
            DEBUGMESSAGE(1 "Removing dirs from ${varname} [${__to_remove_}]")
            LIST(REMOVE_ITEM ${varname} ${__to_remove_})
            DEBUGMESSAGE(1 "The rest is [${${varname}}]")
        ENDIF ()
    ELSE ()
        ADD_ARC_DIRS(__to_remove_ "" ${ARGV})
    ENDIF ()
ENDMACRO ()

# =============================================================================
# ADD_SUBDIRECTORY_EX(srcdir bindir)
#
#
FUNCTION (ADD_SUBDIRECTORY_EX_IMPL var srcdir bindir)
    SET(${var} PARENT_SCOPE)

    FILE(RELATIVE_PATH subdir ${ARCADIA_ROOT} ${srcdir})
    IF (subdir MATCHES "^\\.\\.")
        FILE(RELATIVE_PATH subdir ${ARCADIA_BUILD_ROOT} ${srcdir})
        IF (subdir MATCHES "^\\.\\.")
            MESSAGE_EX("Directory ${subdir} is not contained within ARCADIA_ROOT")
            MESSAGE(SEND_ERROR)
            RETURN()
        ENDIF ()
        UNSET(subdir)
    ENDIF ()

    PROP_GET(__processed_ GLOBAL PROCESSED_DIRS)
    LIST(FIND __processed_ "${srcdir}" idx)
    IF (NOT idx LESS 0) # found
        DEBUGMESSAGE(2 "======= Directory [${srcdir}] is already processed. Skipping")
        RETURN()
    ENDIF ()

    IF (NOT EXISTS ${srcdir}/CMakeLists.txt)
        DEBUGMESSAGE(2 "=====!! File [${srcdir}/CMakeLists.txt] does not exist. Skipping")
        RETURN()
    ENDIF ()

    IF (DEFINED subdir)
        # check if subdir is excluded due to cmake errors
        LIST(FIND EXCLUDE_PROJECTS "${subdir}" idx)
        IF (NOT idx LESS 0) # found, i.e. excluded
            DEBUGMESSAGE(0 "project ${subdir} is manualy excluded from build")
            RETURN()
        ENDIF ()

        # this property contains all subdirectories
        PROP_CURDIR_ADD(ADDED_SUBDIRECTORIES "${subdir}")
        # this property contains only those yet to be checked yet
        PROP_CURDIR_ADD(CHECK_SUBDIRECTORIES "${subdir}")
    ENDIF ()

    PROP_ADD(GLOBAL PROCESSED_DIRS "${srcdir}")
    FILE(APPEND ${PROCESSED_DIRS_FILE} "X${srcdir}X;")
    SET(${var} TRUE PARENT_SCOPE)
ENDFUNCTION()

MACRO (ADD_SUBDIRECTORY_EX srcdir bindir)
    ADD_SUBDIRECTORY_EX_IMPL(__is_ok_ ${srcdir} ${bindir})
    IF (DEFINED __is_ok_)
        ADD_SUBDIRECTORY(${srcdir} ${bindir})
    ENDIF ()

    FILE(RELATIVE_PATH subdir ${ARCADIA_ROOT} ${srcdir})
    LIST(FIND EXCLUDE_PROJECTS "${subdir}" idx)
    IF (EXISTS ${srcdir}/CMakeLists.txt AND ${idx} EQUAL -1)
        #__is_meta_ - don't processing metalibs here (they come here from RECURSE macro)
        PROP_DIR_GET(__dep_dirs_ ${srcdir} DEPENDENT_DIRECTORIES)
        PROP_CURDIR_ADD(DEPENDENT_DIRECTORIES "${subdir}" ${__dep_dirs_})
        PROP_CURDIR_ADD(ALL_SUBDIRECTORIES "${subdir}")
    ELSE ()
        IS_METALIBRARY(__is_meta_ ${subdir})
        IF (NOT EXISTS ${srcdir})
            MESSAGE_EX("${subdir} does not exist")
        ELSEIF(__is_meta_)
            # silently? RECURSE metalibrary directory
        ELSEIF (${idx} EQUAL 0)
            # silently
        ELSE ()
            MESSAGE_EX("${subdir}/CMakeLists.txt does not exist")
        ENDIF ()
    ENDIF ()
ENDMACRO ()

# =============================================================================
# CHECK_DEPENDENT_DIRS([ERROR] [ALLOW_ONLY] dirs_for_chack...)
#  This macro check all dependent dirs and send warning message
#   if current dir has disallowed dependancies.
#   ERROR - send fatal error message insteat warning message
#   ALLOW_ONLY - check that current dir has only directory dependancies from dirs_for_chack...
#
MACRO (CHECK_DEPENDENT_DIRS)
    SET(__dirs_for_check_)
    SET(__is_verbose_ no)
    SET(__is_allow_)
    SET(__dont_check_programs_)
    SET(__message_type)
    FOREACH(__arg_ ${ARGN})
        IF ("${__arg_}" STREQUAL "ERROR")
            SET(__message_type FATAL_ERROR)
        ELSEIF ("${__arg_}" STREQUAL "ALLOW_ONLY")
            SET(__is_allow_ yes)
            SET(__dont_check_programs_ yes)
        ELSEIF ("${__arg_}" STREQUAL "DENY")
            SET(__is_allow_ no)
        ELSEIF ("${__arg_}" STREQUAL "VERBOSE")
            SET(__is_verbose_ yes)
        ELSE ()
            SET_APPEND(__dirs_for_check_ ${__arg_})
        ENDIF ()
    ENDFOREACH ()

    IF ("X${__is_allow_}X" STREQUAL "XX")
        MESSAGE_EX("Check type does not define. Possible types: ALLOW_ONLY, DENY.")
        MESSAGE(SEND_ERROR)
    ENDIF ()

    SET(__baddirs_)
    PROP_CURDIR_GET(__dep_dirs_ DEPENDENT_DIRECTORIES)
    FOREACH (__dep_dir_ ${__dep_dirs_})
        SET(__is_matches_ no)
        FOREACH(__dir_for_check_ ${__dirs_for_check_})
            IF (__dep_dir_ MATCHES "^${__dir_for_check_}")
                SET(__is_matches_ yes)
                BREAK()
            ENDIF ()
        ENDFOREACH ()
        IF((__is_matches_ AND NOT __is_allow_) OR (NOT __is_matches_ AND __is_allow_))
            IF (__dont_check_programs_)
                PROP_DIR_GET(__proj_dir_type_ ${ARCADIA_ROOT}/${__dep_dir_} PROJECT_TYPE)
                IF (NOT ${__proj_dir_type_} STREQUAL "PROG")
                    SET_APPEND(__baddirs_ ${__dep_dir_})
                ENDIF ()
            ELSE ()
                SET_APPEND(__baddirs_ ${__dep_dir_})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    IF (NOT "X${__baddirs_}X" STREQUAL "XX")
        JOIN("${__baddirs_}" " " __baddirs_)
        SET(__verbose_msg)
        IF (__is_verbose_)
            PREPARE_CHECK_DEPENDENT_DIRS_MESSAGE(__verbose_msg)
        ENDIF ()
        MESSAGE_EX("Disallowed dependencies: ${__baddirs_} ${__verbose_msg}")
        IF (__message_type)
            MESSAGE(${__message_type})
        ENDIF()
    ENDIF ()
ENDMACRO ()

MACRO(PREPARE_CHECK_DEPENDENT_DIRS_MESSAGE verbose_msg)
    FILE(RELATIVE_PATH cdir ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
    FOREACH (__dep_dir_ ${__dep_dirs_} ${cdir})
        PROP_DIR_GET(__subdirs_ ${ARCADIA_ROOT}/${__dep_dir_} ALL_SUBDIRECTORIES)
        FOREACH(__subdir_ ${__subdirs_})
            GET_GLOBAL_DIRNAME(__dir_name ${__subdir_})
            LIST(FIND __dirs_dependent_from_${__dir_name} "${__dep_dir_}" idx)
            IF (${idx} EQUAL -1)
                SET_APPEND(__dirs_dependent_from_${__dir_name} ${__dep_dir_})
            ENDIF ()
        ENDFOREACH ()
    ENDFOREACH ()

    FOREACH(__baddir_ ${__baddirs_})
        SET_APPEND(${verbose_msg} "Backtrace for ${__baddir_}:")
        SET(__layer_number 0)
        SET(__cur_layer ${__baddir_})

        SET(__already_processed)
        SET(__next_layer)
        WHILE (NOT "X${__cur_layer}X" STREQUAL "XX")
            LIST(GET __cur_layer 0 __dir_in_layer)
            LIST(FIND __already_processed "${__dir_in_layer}" idx)
            IF (${idx} EQUAL -1)
                GET_GLOBAL_DIRNAME(__dir_name ${__dir_in_layer})
                SET_APPEND(__next_layer ${__dirs_dependent_from_${__dir_name}})
                FOREACH(__dep ${__dirs_dependent_from_${__dir_name}})
                    PROP_DIR_GET(__dir_in_layer_type ${ARCADIA_ROOT}/${__dir_in_layer} PROJECT_TYPE)
                    SET_APPEND(${verbose_msg} "${__layer_number}: ${__dep} -> ${__dir_in_layer} (${__dir_in_layer_type})")
                ENDFOREACH ()
                SET_APPEND(__already_processed ${__dir_in_layer})
            ENDIF ()

            LIST(REMOVE_AT __cur_layer 0)
            IF ("X${__cur_layer}X" STREQUAL "XX")
                MATH(EXPR __layer_number "${__layer_number} + 1")
                SET(__cur_layer ${__next_layer})
                SET(__next_layer)
            ENDIF ()
        ENDWHILE ()
    ENDFOREACH ()
    JOIN("${${verbose_msg}}" "\n" ${verbose_msg})
    IF(NOT "X${${verbose_msg}}X" STREQUAL "XX")
        SET(${verbose_msg} "\nMore verbose:\n${${verbose_msg}}")
    ENDIF ()
ENDMACRO ()

# =============================================================================
# ADD_CUSTOM_COMMAND_EX()
#  This macro adds dependency to the current custom rule definition.
#
MACRO (ADD_CUSTOM_COMMAND_EX)
    SET(__rule_filename_)
    SET(__outfile_found_ no)
    FOREACH(__item_ ${ARGN})
        IF (NOT __outfile_found_ AND NOT "${__rule_filename_}" STREQUAL "OUTPUT")
            SET(__rule_filename_ ${__item_})
        ELSEIF (NOT __outfile_found_ )
            SET(__rule_filename_ ${__item_})
            SET(__outfile_found_ yes)
        ENDIF ()
    ENDFOREACH ()
    SET(__new_cmd_ "${ARGN}")
    IF (__outfile_found_)
        SET(__rule_suffix_ ".custom_build_rule")

        IF (NOT IS_ABSOLUTE ${__rule_filename_})
            SET(__rule_filename_ "${BINDIR}/${__rule_filename_}")
        ENDIF ()
        SET(__rule_filename_ "${__rule_filename_}${__rule_suffix_}")

        SET(__old_rule_)
        IF (EXISTS "${__rule_filename_}")
            FILE(READ "${__rule_filename_}" __old_rule_)
        ENDIF ()
        IF (NOT "${__old_rule_}" STREQUAL "${ARGN}")
            FILE(WRITE "${__rule_filename_}" "${ARGN}")
        ENDIF ()

        IF ("${ARGN}" MATCHES ";DEPENDS;")
            STRING(REPLACE ";DEPENDS;" ";DEPENDS;${__rule_filename_};" __new_cmd_ "${ARGN}")
        ELSE ()
            SET(__new_cmd_ "${ARGN};DEPENDS;${__rule_filename_}")
        ENDIF ()

        DEBUGMESSAGE (4 "${CURDIR}: ADD_CUSTOM_COMMAND[${__new_cmd_}]")
        ADD_CUSTOM_COMMAND(
            OUTPUT ${__rule_filename_}
            COMMAND echo "\"Somebody removed this file after cmake's run\"" >${__rule_filename_}
            COMMENT "Buildrule checkfile[${__rule_filename_}] is lost. Restoring."
        )
    ENDIF ()

    ADD_CUSTOM_COMMAND(${__new_cmd_})
ENDMACRO ()


# =============================================================================
# PREPARE_FLAGS
#  this macro resets all should-be-local variables that could be used by dtmk.cmake or user
#
MACRO (PREPARE_FLAGS)
    IF (NOT FILE_IS_INCLUDED)
        SET(__local_vars_ SRCS PACKAGE_FILES SRCDIR PEERLIBS PEERDIR PEERLDADD PEERDEPENDS OBJADD OBJADDE THIRDPARTY_OBJADD ADDSRC ADDINCL TOOLDEPDIR PROG LIB
            CURTARGET_EXCLUDED CREATEPROG ROOM PUBROOM
            TARGET_PATH LOCAL_EXECUTABLE_SUFFIX LOCAL_SHARED_LIBRARY_PREFIX LOCAL_SHARED_LIBRARY_SUFFIX
            CFLAGS CFLAGS_DEBUG CFLAGS_RELEASE CFLAGS_PROFILE CFLAGS_COVERAGE
            CUDA_NVCC_FLAGS
            OBJDIRPREFIX OPTIMIZE NOOPTIMIZE CXXFLAGS NO_COMPILER_WARNINGS NO_OPTIMIZE
            OPTIMIZE NOOPTIMIZE DTMK_D DTMK_I DTMK_F DTMK_L DTMK_CFLAGS DTMK_LIBTYPE DTMK_CXXFLAGS DTMK_CLEANFILES
            WERROR MAKE_ONLY_SHARED_LIB MAKE_ONLY_STATIC_LIB NO_SHARED_LIB NO_STATIC_LIB PROFFLAG USE_MPROF USEMPROF USE_LIBBIND
            USE_MEMGUARD USEMEMGUARD USE_BERKELEY USE_GNUTLS USE_BOOST USE_LUA USE_PYTHON USE_PERL PERLXS PERLXSCPP LINK_STATIC_PERL
            NOUTIL ALLOCATOR NOCHECK NOPEER SHLIB_MINOR SHLIB_MAJOR THISPROJECTDEPENDS THISPROJECTDEPENDSDIRSNAMES
            IDE_FOLDER USE_GOOGLE_ALLOCATOR USE_LF_ALLOCATOR USE_B_ALLOCATOR USE_J_ALLOCATOR USE_SYSTEM_ALLOCATOR USE_LOCKLESS_ALLOCATOR USE_FAKE_ALLOCATOR _DELAY_YTEST NORUNTIME
            END_MACRO_PRESENT UNIT_MACRO_PRESENT PACKAGE_PEERDIR_DEPS
            DLL_FOR TGZ_FOR DEB_FOR RPM_FOR PY_PROTOS_FOR
            PY_PROTOS
            )
        FOREACH(__item_ ${__local_vars_})
            SET(${__item_})
        ENDFOREACH ()
    ENDIF ()
ENDMACRO ()

MACRO (META_PROJECT prjname)
    INIT_CONFIG()

    STRING(REGEX MATCH "PROG" __prjnamePROG_ ${prjname})
    STRING(REGEX MATCH "LIB" __prjnameLIB_ ${prjname})

    IF (NOT "X${__prjnamePROG_}${__prjnameLIB_}X" MATCHES "XX")
        DEFAULT(${prjname} ${ARGN})
        PROJECT_EX(${ARGN})
    ELSE ()
        PROJECT_EX(${prjname})
    ENDIF ()
    SET_APPEND(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})
ENDMACRO ()

# =============================================================================
# PROJECT_EX2(prjname)
#
#
MACRO (PROJECT_EX2)
    PREPARE_FLAGS()
    META_PROJECT(${ARGN})
ENDMACRO ()

MACRO(GETTARGETNAME varname)
    IF (NOT "X${ARGN}X" STREQUAL "XX")
        SET(__prjname_ "${ARGV1}")
    ELSE ()
        SET(__prjname_ "${LASTPRJNAME}")
    ENDIF ()
    GET_TARGET_PROPERTY(${varname} ${__prjname_} LOCATION)
ENDMACRO ()

# =============================================================================
# PROJECT_EX(prjname)
#  just a proxy
#
MACRO (PROJECT_EX prjname)
    SET_APPEND(PROJECT_DEFINED ${prjname})
    PROJECT(${prjname} ${ARGN})
    SET(CURDIR ${CMAKE_CURRENT_SOURCE_DIR})
    SET(BINDIR ${CMAKE_CURRENT_BINARY_DIR})
#    DEFAULT(TARGET_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    SET(TARGET_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

    GET_GLOBAL_DIRNAME(GLOBAL_TARGET_NAME "${TARGET_PATH}")
ENDMACRO ()

MACRO (OWNER)
    IF (USE_OWNERS)
        FILE(RELATIVE_PATH rp ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
        FOREACH(name ${ARGN})
            FILE (APPEND ${PROCESSED_OWNERS_FILE} "${rp}\t${name}\n")
        ENDFOREACH ()
        SET(rp)
    ENDIF ()
ENDMACRO ()

FUNCTION(EOF_HOOK variable access)
    IF(${variable} STREQUAL CMAKE_BACKWARDS_COMPATIBILITY AND
        (${access} STREQUAL UNKNOWN_READ_ACCESS OR ${access} STREQUAL READ_ACCESS))
        IF (UNIT_MACRO_PRESENT AND NOT END_MACRO_PRESENT)
            # It is not possible to use END macro here, because CMAKE_BACKWARDS_COMPATIBILITY watches as last step before 'Configure' step finishes
            MESSAGE_EX("CMakeLists.txt does not contain END macro")
            MESSAGE(FATAL_ERROR)
        ENDIF ()
    ENDIF ()
ENDFUNCTION()

# Some magic to detect projects with missing END macro
# For more information: http://stackoverflow.com/questions/15760580/execute-command-or-macro-in-cmake-as-last-step-before-configure-step-finishes
FUNCTION(SETUP_EOF_HOOK)
    VARIABLE_WATCH(CMAKE_BACKWARDS_COMPATIBILITY EOF_HOOK)
ENDFUNCTION()

MACRO(GET_DEFAULT_UNIT_NAME result_var unit_type)
    IF ("${unit_type}" STREQUAL "PACKAGE")
        GET_UNIQ_NAME(__target_name_ "package_from" TO_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR})
        PROP_CURDIR_SET(PACKAGE_TARGET_NAME ${__target_name_})
        SET(${result_var} ${__target_name_})
    ELSE ()
        FILE(RELATIVE_PATH path "${ARCADIA_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
        STRING(REGEX REPLACE ".*/" "" ${result_var} ${path})
        SET(path)

        IF ("${unit_type}" STREQUAL "LIB")
            FILE(RELATIVE_PATH __dir ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
            GET_FILENAME_COMPONENT(__dir ${__dir} PATH)
            IF (NOT "${__dir}" STREQUAL "")
                GET_FILENAME_COMPONENT(__lib_prefix_ ${__dir} NAME)
                SET(${result_var} "${__lib_prefix_}-${${result_var}}")
            ENDIF ()
            IF (NOT "${__dir}" STREQUAL "")
                GET_FILENAME_COMPONENT(__dir ${__dir} PATH)
                IF (NOT "${__dir}" STREQUAL "")
                    GET_FILENAME_COMPONENT(__lib_prefix_ ${__dir} NAME)
                    SET(${result_var} "${__lib_prefix_}-${${result_var}}")
                ENDIF ()
            ENDIF ()
        ENDIF ()
    ENDIF ()
ENDMACRO()

MACRO (UNIT type)
    IF (NOT ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} MATCHES 2.8.12) # hook causes infinite loop in cmake 2.8.12
        SETUP_EOF_HOOK()
    ENDIF ()

    SET(q ${ARGN})
    LIST(LENGTH q len)

    IF (len)
        LIST(GET q 0 name)
    ELSE ()
        GET_DEFAULT_UNIT_NAME(name ${type})
    ENDIF ()

    SET(q)
    SET(len)

    CHECK_DUPLICATE_TARGET(name)
    ENTER_PROJECT()

    PROJECT_EX2(${type} ${name})
    ENABLE(UNIT_MACRO_PRESENT)
    SET(SAVEDNAME ${name})
    SET(LASTPRJNAME ${name})
    SET(REALPRJNAME ${name})
    SET(SAVEDTYPE ${type})
    PROP_CURDIR_SET(PROJECT_TYPE ${type})
    PROP_CURDIR_SET(PROJECT_NAME ${name})

    SET(name)

    IF (PY_PROTOS_FOR_DIR)
        CREATE_INIT_PY_STRUCTURE(${PY_PROTOS_FOR_DIR})
        # SRCDIR(${ARCADIA_ROOT})
        SRCDIR(${PY_PROTOS_FOR_DIR})
        ADDINCL(${PY_PROTOS_FOR_DIR})
        ENABLE(PY_PROTOS_FOR)
        SET(PY_PROTOS_FOR_DIR)
        SET(PY_PROTOS_FOR_ARGS)
    ENDIF ()

    # cmake does not recognize -iquote include flag. This causes its dependency
    # scanner to miss dependencies in generated C++ source files. Which leads
    # to broken incremental builds. To fix this, we can tell cmake that
    # ${CMAKE_CURRENT_SOURCE_DIR} is a compiler's implicit include directory.
    # The documented way to do so is to set CMAKE_*_IMPLICIT_INCLUDE_DIRECTORIES.
    # But it leads to an undocumented side effect of cmake wrongfully altering
    # compiler's -I flags. So we have to use an undocumented way of setting
    # several variables (unused in arcadia) which does not have that side effect.
    # These variables are checked in cmake's cmLocalGenerator.cxx.
    SET(VTK_SOURCE_DIR /)
    SET(VTK_MAJOR_VERSION 4)
    SET(VTK_MINOR_VERSION 0)
ENDMACRO ()

MACRO (STRIP)
    IF (NOT NO_STRIP AND NOT WIN32)
        SET_APPEND(LDFLAGS -s)
    ENDIF ()
ENDMACRO ()

MACRO (TOOL)
    PROGRAM(${ARGN})
    STRIP()
ENDMACRO ()

MACRO (PROGRAM)
    UNIT(PROG ${ARGN})
    SET(CURPROJTYPES PROGRAM ${CURPROJTYPES})
ENDMACRO ()

MACRO (UNITTEST)
    SET(__ut_name)
    SET(args)
    FOREACH (__item_ ${ARGN})
        IF ("X${__ut_name}X" STREQUAL "XX")
            SET(__ut_name ${__item_})
        ELSE ()
            SET_APPEND(args ${__item_})
        ENDIF ()
    ENDFOREACH ()

    IF ("X${__ut_name}X" STREQUAL "XX")
        FILE(RELATIVE_PATH __dir ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
        STRING(REGEX REPLACE "/" "-" __ut_name ${__dir})
    ENDIF ()

    PROP_CURDIR_SET(_DELAY_YTEST ${__ut_name})

    PROGRAM(${__ut_name} ${args})

    IF (UT_SKIP_EXCEPTIONS)
        CFLAGS(-DUT_SKIP_EXCEPTIONS)
    ENDIF ()

    PEERDIR(
        library/unittest
    )
ENDMACRO ()

MACRO (PACKAGE)
    SET(PACKAGE_TYPE)
    SET(PACKAGE_FOR_DIR)
    IF (TGZ_FOR_DIR)
        SET(PACKAGE_TYPE TGZ)
        SET(PACKAGE_FOR_DIR ${TGZ_FOR_DIR})
        ENABLE(TGZ_FOR)
        # reset flags immediately to not influence peers
        SET(TGZ_FOR_DIR)
        SET(TGZ_FOR_ARGS)
    ELSEIF (RPM_FOR_DIR)
        SET(PACKAGE_TYPE RPM)
        SET(PACKAGE_FOR_DIR ${RPM_FOR_DIR})
        ENABLE(RPM_FOR)
        # reset flags immediately to not influence peers
        SET(RPM_FOR_DIR)
        SET(RPM_FOR_ARGS)
    ELSEIF (DEB_FOR_DIR)
        SET(PACKAGE_TYPE DEB)
        SET(PACKAGE_FOR_DIR ${DEB_FOR_DIR})
        ENABLE(DEB_FOR)
        # reset flags immediately to not influence peers
        SET(DEB_FOR_DIR)
        SET(DEB_FOR_ARGS)
    ENDIF()

    UNIT(PACKAGE ${ARGN})
    SET(CURPROJTYPES PACKAGE ${CURPROJTYPES})
    # IF (PACKAGE_FOR_DIR)
        # SRCDIR(${PACKAGE_FOR_DIR})
        # ADDINCL(${PACKAGE_FOR_DIR})
    # ENDIF ()
ENDMACRO ()

MACRO (LIBRARY_IMPL)
    UNIT(LIB ${ARGN})
    SET(CURPROJTYPES LIBRARY ${CURPROJTYPES})
ENDMACRO ()

MACRO (METALIBRARY)
    SET(CURPROJTYPES METALIBRARY ${CURPROJTYPES})
    DEBUGMESSAGE(3 "METALIBRARY. ARGN[${ARGN}]")
# Do not uncomment the following line
#    UNIT(METALIB ${ARGN})
ENDMACRO ()

MACRO (METAQUERY)
    SET(CURPROJTYPES METAQUERY ${CURPROJTYPES})
ENDMACRO ()

MACRO (METAQUERYFILES)
# TODO
ENDMACRO ()


MACRO (APPLY_GLOBAL_TO_CURDIR)
    GET_GLOBAL_KEYWORD_PROPERTY("AddinclGlobal" __global_addincls_ ${CURDIR})
    ADDINCL(${__global_addincls_})
    IF(NOT "X${DTMK_I}X" STREQUAL "XX")
        LIST(REMOVE_DUPLICATES DTMK_I)
    ENDIF ()
    GET_GLOBAL_KEYWORD_PROPERTY("CFlagsGlobal" __global_cflags_ ${CURDIR})
    CFLAGS(${__global_cflags_})
    IF(NOT "X${CFLAGS}X" STREQUAL "XX")
        LIST(REMOVE_DUPLICATES CFLAGS)
    ENDIF ()
    GET_GLOBAL_KEYWORD_PROPERTY("ExtraLibs" __extra_libs_ ${CURDIR})
    OBJADDE(${__extra_libs_})
    IF(NOT "X${OBJADDE}X" STREQUAL "XX")
        LIST(REMOVE_DUPLICATES OBJADDE)
    ENDIF ()
    GET_GLOBAL_KEYWORD_PROPERTY("BuildAfterGlobal" __global_build_after_ ${CURDIR})
    BUILDAFTER(${__global_build_after_})
    # Use explicit linker deps inside programs and .so
    IF (${SAVEDTYPE} STREQUAL "PROG")
        GET_GLOBAL_KEYWORD_PROPERTY("ExplicitDepsForProgs" deps ${CURDIR})
        OBJADDE(${deps})
    ELSEIF (MAKE_ONLY_SHARED_LIB AND NOT NO_SHARED_LIB)
        GET_GLOBAL_KEYWORD_PROPERTY("ExplicitDepsForSo" deps ${CURDIR})
        OBJADDE(${deps})
    ENDIF ()
ENDMACRO ()

MACRO (END)
    ENABLE(END_MACRO_PRESENT)
    LIST(GET CURPROJTYPES 0 __curtype_)
    LIST(REMOVE_AT CURPROJTYPES 0)
    IF (__curtype_ STREQUAL "METALIBRARY")
        DEBUGMESSAGE(3 "METALIBRARY end @ ${CURDIR}, SAVEDNAME[${SAVEDNAME}]")
    ELSE ()

        IF (SAVEDNAME)
            IF (${SAVEDTYPE} STREQUAL "LIB")
                ADD_LIBRARY_EX(${SAVEDNAME} ${SRCS})
            ELSEIF (${SAVEDTYPE} STREQUAL "PROG")
                ADD_EXECUTABLE_EX(${SAVEDNAME} ${__local_exclude_from_all_} ${SRCS})
            ELSEIF (${SAVEDTYPE} STREQUAL "PACKAGE")
                GENERATE_PACKAGE()
            ELSE ()
                MESSAGE_EX("Unknown SAVEDTYPE: ${SAVEDTYPE}")
                MESSAGE(FATAL_ERROR)
            ENDIF ()
        ENDIF ()

        IF (PY_PROTOS)
            GET_UNIQ_NAME(__target_name_ TO_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "_gen_pb2_pys")
            ADD_CUSTOM_TARGET("${__target_name_}" ALL DEPENDS ${PY_PROTOS})
        ENDIF ()

        SET(SAVEDNAME)
        SET(SAVEDTYPE)

        LEAVE_PROJECT()

        SET(__delay_ytest)
        PROP_CURDIR_GET(__delay_ytest _DELAY_YTEST)

        IF (__delay_ytest)
            ADD_YTEST(${__delay_ytest} unittest.py)
        ENDIF ()
    ENDIF ()
ENDMACRO ()

# =============================================================================
# FIND_FILE_IN_DIRS
#
MACRO (FIND_FILE_IN_DIRS found_var filename)
    SET(__file_ ${filename}) # in case found_var points to filename
    TRY_FIND_FILE_IN_DIRS(${found_var} ${filename} ${ARGN})
    IF (NOT DEFINED ${found_var})
        MESSAGE_EX("Cannot locate ${__file_} in ${ARGN}")
        MESSAGE(SEND_ERROR)
    ENDIF ()
ENDMACRO ()

FUNCTION (TRY_FIND_FILE_IN_DIRS found_var filename)
    IF (IS_ABSOLUTE ${filename})
        SET(${found_var} ${filename} PARENT_SCOPE)
        RETURN()
    ENDIF ()

    # undefine it
    SET(${found_var} PARENT_SCOPE)

    FOREACH (__dir_ ${ARGN})
        SET(__path_ ${__dir_}/${filename})
        DEBUGMESSAGE(3 "-- trying ${__path_}")
        GET_FILENAME_COMPONENT(__path_ ${__path_} ABSOLUTE)

        IF (EXISTS ${__path_})
            DEBUGMESSAGE(3 "--- is ${__path_}")
            SET(${found_var} ${__path_} PARENT_SCOPE)
            RETURN()
        ENDIF ()

        SET_APPEND(__tried_ ${__path_})
    ENDFOREACH ()

    DEBUGMESSAGE(2 "${CURDIR}: cannot locate [${filename}]. Generated?")
    DEBUGMESSAGE(2 "\tTried names [${__tried_}]")
ENDFUNCTION (TRY_FIND_FILE_IN_DIRS)

# =============================================================================
# IS_COMPILABLE_TYPE
# This macro checks if this file could be built by CMake itself
#
MACRO(IS_COMPILABLE_TYPE res_var file_ext)
    SET(${res_var} no)

    # TODO: use ${CMAKE_CXX_SOURCE_FILE_EXTENSIONS} ${CMAKE_C_SOURCE_FILE_EXTENSIONS}
    # Note that CXX_... contains c++ (invalid regular expression)

    FOREACH(__cpp_ext_ ${COMPILABLE_FILETYPES})
        IF (NOT ${res_var})
            # this doesn't work because file_exc is the longest extension (.byk.cpp, for example):
            IF ("${file_ext}" STREQUAL ".${__cpp_ext_}")
                SET(${res_var} yes)
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

# =============================================================================
# SPLIT_FILENAME
#   This macro exists because "The longest file extension is always considered"
#     in GET_FILENAME_COMPONENT macro. We do need the longest name and shortest
#     file extension.
MACRO (SPLIT_FILENAME namewe_var ext_var)
    # Get name and extention
    GET_FILENAME_COMPONENT(__file_name_ "${ARGN}" NAME)
    STRING(REGEX REPLACE ".*\\." "." ${ext_var} "${__file_name_}")
    STRING(REGEX REPLACE "\\.[^.]+$" "" ${namewe_var} "${__file_name_}")
    IF ("${ext_var}" STREQUAL "${__file_name_}")
        SET(${namewe_var} "${${ext_var}}")
        SET(${ext_var} "")
    ENDIF ()

    GET_FILENAME_COMPONENT(__file_dir_ "${ARGN}" PATH)
    IF (NOT "X${__file_dir_}X" STREQUAL "XX")
        SET(${namewe_var} "${__file_dir_}/${${namewe_var}}")
    ENDIF ()

    DEBUGMESSAGE(3 "Splitted [${ARGN}] to [${${namewe_var}}][${${ext_var}}]")
ENDMACRO ()

# =============================================================================
# TRY_PREPARE_SRC_BUILDRULE
#
MACRO (TRY_PREPARE_SRC_BUILDRULE filename_var srcdir_var)
    SPLIT_FILENAME(__file_pathwe_ __file_ext_ ${${filename_var}})
    IS_COMPILABLE_TYPE(__compilable_ ${__file_ext_})

    IF (__compilable_)
        SET(__srcfound_)
        FOREACH(__new_ext_ ${SUFFIXES})
            TRY_FIND_FILE_IN_DIRS(__srcfound_
                "${__file_pathwe_}.${__new_ext_}" ${${srcdir_var}})
            IF (DEFINED __srcfound_)
                # If ${filename_var} is a relative path then add a BINARY_DIR-prefix
                IF (NOT IS_ABSOLUTE "${${filename_var}}")
                    SET(${filename_var} "${BINDIR}/${${filename_var}}")
                ENDIF ()

                ADD_SRC_BUILDRULE(.${__new_ext_} ${__srcfound_} ${filename_var})
                BREAK()
            ENDIF ()
        ENDFOREACH ()
        IF (NOT DEFINED __srcfound_)
            DEBUGMESSAGE(1 "Can't find a source file to build ${${filename_var}} (@ ${CMAKE_CURRENT_SOURCE_DIR})")
            MESSAGE_EX("Cannot find source file: ${${filename_var}}")
            SET(${filename_var})
        ENDIF ()
    ELSE ()
        # This file is not cpp AND not exists.
        FILE(RELATIVE_PATH rp ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
        MESSAGE_EX("Cannot find source file: ${${filename_var}}")
        SET(${filename_var})
    ENDIF ()
ENDMACRO ()


# =============================================================================
# PREPARE_SRC_BUILDRULE
#
MACRO(PREPARE_SRC_BUILDRULE srcfile_var)
    SET(__srcfile_ "${${srcfile_var}}")
    GET_FILENAME_COMPONENT(__file_name_ "${__srcfile_}" NAME)
    GET_FILENAME_COMPONENT(__file_path_ "${__srcfile_}" PATH)

    DEBUGMESSAGE(3 "-- ${srcfile_var} = ${__srcfile_} (${__file_path_} / ${__file_namewe_} . ${__file_ext_})")
    SPLIT_FILENAME(__file_namewe_ __file_ext_ ${__file_name_})

    IS_COMPILABLE_TYPE(__compilable_ ${__file_ext_})
    IF (__compilable_)
        # Nothing to do
        DEBUGMESSAGE(2 "----------- ${__srcfile_} is buildable directly (extension is [${__file_ext_}])")
        PROP_CURDIR_GET(__has_props_ DIR_HAS_PROPS)
        IF (__has_props_)
            # Just to apply ..._PROPS
            ADD_SRC_BUILDRULE(".cpp" "${__srcfile_}" ${srcfile_var} ${ARGN})
        ENDIF ()

    # Check if this file extension is in SUFFIXES
    ELSEIF (DEFINED IS_SUFFIX${__file_ext_})
        DEBUGMESSAGE(2 "----------- ${__srcfile_} is buildable indirectly (extension is [${__file_ext_}])")
        SET(${srcfile_var} "${BINDIR}/${__file_namewe_}.cpp")
        ADD_SRC_BUILDRULE(${__file_ext_} "${__srcfile_}" ${srcfile_var} ${ARGN})
    ELSE ()
        DEBUGMESSAGE(2 "----------- ${__srcfile_} is not buildable")
        SET(${srcfile_var} ${__srcfile_}.rule-not-found.cpp)
    ENDIF ()
ENDMACRO ()


#
# The source barrier will cause compilation of any file to start only after all
# source files have been updated. Use sources barrier when some of the source
# files are generated and are used by each other or non-generated files; for
# instance, when using .proto files when they can import each other or in the
# same directory as non-generated source files which might be using them.
#

MACRO (SET_NEED_SRC_BARRIER)
    PROP_CURDIR_SET(NEED_BARRIER_SOURCES TRUE)
ENDMACRO ()

MACRO (GET_NEED_SRC_BARRIER var)
    PROP_CURDIR_GET(${var} NEED_BARRIER_SOURCES)
ENDMACRO ()


#
# Helper macros to propagate dependencies on directories containing generated
# files (mainly headers). The result will be used in dtmk.cmake's PEERDIR
# processing, when all peer libraries are known and can be checked for having
# generated files themselves or, in turn, depending on other such libraries.
# These generated files must be used or included externally (e.g., header
# files); standalone source files do not need to be included in these lists.
#

# these files are not going to be used by anyone
FUNCTION (DIR_ADD_GENERATED_SRC)
    IF ("0" LESS "${ARGC}")
        SOURCE_GROUP("Generated" FILES ${ARGV})
    ENDIF ()
ENDFUNCTION ()

# these, however, might be included by other files
FUNCTION (DIR_ADD_GENERATED_INC)
    IF ("0" LESS "${ARGC}")
        SET_NEED_SRC_BARRIER()
        DIR_ADD_GENERATED_SRC(${ARGV}) # in case some are source files
        PROP_CURDIR_ADD(DIR_GENERATED_INCLUDES ${ARGV})
    ENDIF ()
ENDFUNCTION ()

MACRO (DIR_GET_GENERATED_INC var)
    PROP_CURDIR_GET(${var} DIR_GENERATED_INCLUDES)
ENDMACRO ()

MACRO (GET_DIR_HAS_GENERATED_INC var dir)
    PROP_DIR_GET(${var} ${dir} DIR_GENERATED_INCLUDES SET)
ENDMACRO ()

MACRO (SET_DIR_HAS_GENERATED)
    MESSAGE_EX("Don't use SET_DIR_HAS_GENERATED(); instead,"
        " specify generated files (which MUST be used or included by others)"
        " via DIR_ADD_GENERATED_INC(); these files, of course, have to be"
        " OUTPUT by an appropriate ADD_CUSTOM_COMMAND().")
    MESSAGE(SEND_ERROR)
ENDMACRO ()

MACRO (ADD_DIR_DEP_GENERATED)
    PROP_CURDIR_ADD(DIR_DEP_GENERATED ${ARGN})
ENDMACRO ()

FUNCTION (GET_DIR_DEP_GENERATED var)
    IF ("${ARGC}" GREATER "1")
        UNSET(__set)
        FOREACH (__dir ${ARGN})
            PROP_DIR_GET(__val ${__dir} DIR_DEP_GENERATED)
            IF (DEFINED __val)
                SET_APPEND(__set ${__val})
            ENDIF ()
        ENDFOREACH ()
    ELSE ()
        PROP_CURDIR_GET(__set DIR_DEP_GENERATED)
    ENDIF ()
    IF (DEFINED __set)
        LIST(REMOVE_DUPLICATES __set)
        SET(${var} ${__set} PARENT_SCOPE)
    ELSE ()
        SET(${var} PARENT_SCOPE)
    ENDIF ()
ENDFUNCTION()

FUNCTION (IMP_DIR_DEP_GENERATED dir)
    GET_DIR_DEP_GENERATED(__dir_dep_generated ${dir} ${ARGN})
    IF (__dir_dep_generated)
        ADD_DIR_DEP_GENERATED(${__dir_dep_generated})
    ENDIF ()
ENDFUNCTION ()


FUNCTION (SET_BARRIER name_barrier path_barrier)
    GET_FILENAME_COMPONENT(__name_barrier ${name_barrier} NAME)
    SET(__path_barrier ${BINDIR}/${__name_barrier})
    SET(${path_barrier} ${__path_barrier} PARENT_SCOPE)
    GET_BARRIER_TOUCH_CMD(__cmd_ ${__name_barrier})
    ADD_CUSTOM_COMMAND(
        OUTPUT    ${__path_barrier}
        COMMAND   ${__cmd_}
        DEPENDS   ${ARGN}
        WORKING_DIRECTORY ${BINDIR}
        COMMENT   ""
    )
ENDFUNCTION ()

MACRO (SET_BARRIER_OBJ __name_barrier_ __path_barrier_ __deps_ __srcs_)
    IF (NOT "" STREQUAL "${${__deps_}}")
        SET_BARRIER(${__name_barrier_} ${__path_barrier_} ${${__deps_}})
        FOREACH (__item_ ${${__srcs_}})
            SET_PROPERTY(SOURCE ${__item_}
                APPEND PROPERTY OBJECT_DEPENDS ${${__path_barrier_}})
        ENDFOREACH ()
    ENDIF ()
ENDMACRO ()

MACRO (SET_BARRIER_BETWEEN __name_barrier_ __path_barrier_ __deps_ __tgts_)
    IF (NOT "" STREQUAL "${${__deps_}}")
        SET_BARRIER(${__name_barrier_} ${__path_barrier_} ${${__deps_}})
        ADD_CUSTOM_COMMAND(
            OUTPUT    ${${__tgts_}}
            DEPENDS   ${${__path_barrier_}}
            APPEND
        )
    ENDIF ()
ENDMACRO ()

MACRO (PREPARE_SRC_FILES_BASE new_srcs)
    FOREACH (__item_ ${ARGN})
        IF (EXISTS ${__item_} OR IS_ABSOLUTE ${__item_})
            # If file with given name exists or filename is full path.
            # Probably not compilable by c/c++ compiler.
            PREPARE_SRC_BUILDRULE(__item_)
        ELSE ()
            SET(__path_)
            TRY_FIND_FILE_IN_DIRS(__path_ ${__item_} ${__srcdir_full_})
            IF (DEFINED __path_)
                # Found file with given name.
                # Probably not compilable by c/c++ compiler.
                PREPARE_SRC_BUILDRULE(__path_ ${__item_})
                SET(__item_ ${__path_})
            ELSE ()
                # File has not been found
                # Trying to find source file from gperf, l, rl, etc.
                DEBUGMESSAGE(2 "----------- ${__item_} not found")
                TRY_PREPARE_SRC_BUILDRULE(__item_ __srcdir_full_)
            ENDIF ()
        ENDIF ()

        IF (DEFINED __item_)
            SET_APPEND(${new_srcs} ${__item_})
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

# =============================================================================
# PREPARE_SRC_FILES
#
MACRO (PREPARE_SRC_FILES srcs srcdir srcdirbase)
    DEBUGMESSAGE(2 "srcdirbase[${srcdirbase}]; SRCDIR[${${srcdir}}] @ ${CMAKE_CURRENT_SOURCE_DIR}")
    IF (${srcdir})
        # Prepare full directory names
        SET(__srcdir_full_)
        FOREACH (__diritem_ ${${srcdir}})
            IF (IS_ABSOLUTE ${__diritem_})
                SET_APPEND(__srcdir_full_ ${__diritem_})
            ELSEIF ("${__diritem_}" STREQUAL ".")
                SET_APPEND(__srcdir_full_ ${CURDIR})
            ELSE ()
                SET_APPEND(__srcdir_full_ ${srcdirbase}/${__diritem_})
            ENDIF ()
        ENDFOREACH ()

        SET(__new_srcs)
        LIST(LENGTH ${srcs} __new_srcs_l)
        PREPARE_SRC_FILES_BASE(__new_srcs ${${srcs}})
        LIST(LENGTH SRCS __srcs_l)
        IF (NOT "X${SRCS}X" STREQUAL "XX")
            WHILE (NOT ${__new_srcs_l} EQUAL ${__srcs_l})
                SET(__diff_srcs)
                SET(__diff_i 0)
                MATH(EXPR __srcs_iter "${__new_srcs_l}")
                WHILE (${__srcs_iter} LESS ${__srcs_l})
                    LIST(GET SRCS ${__srcs_iter} __src_)
                    SET_APPEND(__diff_srcs ${__src_})
                    MATH(EXPR __srcs_iter "${__srcs_iter} + 1")
                ENDWHILE ()

                PREPARE_SRC_FILES_BASE(__new_srcs ${__diff_srcs})

                LIST(LENGTH __diff_srcs __diff_srcs_l)
                MATH(EXPR __new_srcs_l "${__new_srcs_l} + ${__diff_srcs_l}")
                LIST(LENGTH SRCS __srcs_l)
            ENDWHILE ()
        ENDIF ()

        GET_NEED_SRC_BARRIER(__need_src_barrier)
        IF (__need_src_barrier)
            DIR_GET_GENERATED_INC(__gen_inc_files_)
            SET_BARRIER_OBJ("_barrier_sources"
                __path_src_barrier __gen_inc_files_ __new_srcs)
        ENDIF ()

        SET(${srcs} ${__new_srcs})
        SET(__headers_)
        IF ((WIN32 OR XCODE) AND NOT NO_ADD_HEADERS)
            FOREACH(__item_ ${${srcs}})
                IF (IS_ABSOLUTE ${__item_})
                    IF (${__item_} MATCHES ".cpp$")
                        STRING(REPLACE ".cpp" ".h" __h_file_ ${__item_})
                    ELSEIF (${__item_} MATCHES ".c$")
                        STRING(REPLACE ".c" ".h" __h_file_ ${__item_})
                    ELSEIF (${__item_} MATCHES ".cc$")
                        STRING(REPLACE ".cc" ".h" __h_file_ ${__item_})
                    ENDIF ()
                    IF (EXISTS ${__h_file_})
                        SET_APPEND(__headers_ ${__h_file_})
                    ENDIF ()
                ENDIF ()
            ENDFOREACH ()
            SET_APPEND(${srcs} ${__headers_})
            DEBUGMESSAGE(2 "Headers added to project [" ${__headers_} "]")
        ENDIF ()
    ENDIF ()
ENDMACRO ()

# =============================================================================
# SAVE_VARIABLES. Saves variables. Use CHECK_SAVED_VARIABLES to check if they
#   have changed
#
MACRO (SAVE_VARIABLES)
    FOREACH(__item_ ${ARGN})
        SET(__saved_${__item_} "${${__item_}}")
    ENDFOREACH ()
ENDMACRO ()

MACRO (CHECK_SAVED_VARIABLES outvar)
    SET(${outvar} "")
    FOREACH(__item_ ${ARGN})
        IF (NOT "${__saved_${__item_}}" STREQUAL "${${__item_}}")
            SET_APPEND(${outvar} ${__item_})
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

# =============================================================================
# LIST_REMOVE_DUPLICATES(listname)
#
MACRO (LIST_REMOVE_DUPLICATES listname)
    SET(__newlist_)
    SET(__remove_only_ ".*")
    IF (NOT "X${ARGN}X" STREQUAL "XX")
        SET(__remove_only_ "${ARGV1}")
        DEBUGMESSAGE(2 "Removing only '${__remove_only_}'")
    ENDIF ()
    WHILE (${listname})
        LIST(GET ${listname} 0 __head_)
        LIST(APPEND __newlist_ ${__head_})
        IF ("${__head_}" MATCHES "${__remove_only_}")
            LIST(REMOVE_ITEM ${listname} ${__head_})
            DEBUGMESSAGE(2 "Removing: from ${listname} '${__head_}'")
        ELSE ()
            LIST(REMOVE_AT ${listname} 0)
            DEBUGMESSAGE(2 "Removing: from ${listname} 0'th element ('${__head_}')")
        ENDIF ()
    ENDWHILE (${listname})
    SET(${listname} ${__newlist_})
ENDMACRO ()

# =============================================================================
# Internal Macros to apply collected DTMK_D, DTMK_I etc. to srcs
#
MACRO(_APPLY_DEFS srcs)
    SET(__sources_ ${srcs} ${ARGN})
    SET(__LOCAL_DTMK_I)
    INCLUDE_DIRECTORIES(${DTMK_I})
    DEBUGMESSAGE(2 "${CMAKE_CURRENT_SOURCE_DIR} DTMK_I=${DTMK_I}\n")

#   FOREACH(__item_ ${DTMK_I})
#       IF (WIN32)
#           SET_APPEND(__LOCAL_DTMK_I "/I \"${__item_}\"")
#       ELSE ()
#           SET_APPEND(__LOCAL_DTMK_I -I${__item_})
#       ENDIF ()
#   ENDFOREACH ()

    ADD_DEFINITIONS(${DTMK_D})
    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: DTMK_D=${DTMK_D}\n")
#   SET(__LOCAL_COMPILE_FLAGS ${DTMK_CFLAGS} ${DTMK_CXXFLAGS}) #${__LOCAL_DTMK_I}
#   SEPARATE_ARGUMENTS_SPACE(__LOCAL_COMPILE_FLAGS)

    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: __LOCAL_COMPILE_FLAGS=${__LOCAL_COMPILE_FLAGS}\n__sources_=${__sources_}\n")

IF (USE_PRECOMPILED)
    SET(__have_stdafxcpp_)
    STRING(TOLOWER "${__sources_}" srclist)
    IF ("${srclist}" MATCHES "stdafx.cpp")
        ENABLE(__have_stdafxcpp_)
    ENDIF ()

    IF (WIN32 AND __have_stdafxcpp_)
        FOREACH(__item_ ${__sources_})
            STRING(TOLOWER "${__item_}" __name_lwrcase_)
            IF (${__name_lwrcase_} MATCHES "stdafx.cpp\$")
                SET_SOURCE_FILES_PROPERTIES(${__item_} PROPERTIES COMPILE_FLAGS "/Yc\"stdafx.h\" /Fp\"${BINDIR}/${CMAKE_CFG_INTDIR}/${PROJECTNAME}.pch\"")
            ELSEIF (${__name_lwrcase_} MATCHES ".*h[p]*\$")
                # It's not good way to compile headers
            ELSE ()
                SET_SOURCE_FILES_PROPERTIES(${__item_} PROPERTIES COMPILE_FLAGS "/Yu\"stdafx.h\" /Fp\"${BINDIR}/${CMAKE_CFG_INTDIR}/${PROJECTNAME}.pch\"")
            ENDIF ()
        ENDFOREACH ()
    ENDIF ()
ENDIF ()
ENDMACRO ()

# =============================================================================
# lorder_for_peerlibs
#   (internal macro)
MACRO(lorder_for_peerlibs prjname peerlibs_var peerdepends_var sources_var)
    IF (PEERDEPENDS)
        IF (USE_LORDER)
            ADD_CUSTOM_COMMAND(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp
                COMMAND echo "${prjname}: current libraries order:" > ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp
                COMMAND echo ${${peerlibs_var}} >> ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp
                COMMAND echo "Good libraries order:" && lorder ${${peerlibs_var}} | tsort 2>/dev/null 1>${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp || true
                COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp
                COMMAND ${RM} ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp
                COMMAND cp ${ARCADIA_ROOT}/cmake/include/_cmake_fake_src.cpp ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp
                DEPENDS ${${peerdepends_var}}
                COMMENT "lorder+tsort for peerlibs of ${prjname}"
            )
            LIST(APPEND ${sources_var} ${CMAKE_CURRENT_BINARY_DIR}/_cmake_fake_src.cpp)
        ENDIF ()
    ENDIF ()
ENDMACRO ()

MACRO (ON_TARGET_FINISH)
    IF (DTMK_CLEANFILES)
        SET_DIRECTORY_PROPERTIES(PROPERTIES
            ADDITIONAL_MAKE_CLEAN_FILES "${DTMK_CLEANFILES}"
        )
    ENDIF ()
    IF (COMMAND ON_TARGET_FINISH_HOOK)
        ON_TARGET_FINISH_HOOK()
    ENDIF ()
    IF (CALC_CMAKE_DEPS)
        CMAKE_DEPS_TARGET()
    ENDIF ()
ENDMACRO ()

MACRO (CHECK_PROJECT_DEFINED prjname prjtype)
    SET(__nobuild_)
    LIST(LENGTH PROJECT_DEFINED __len_)
    IF (NOT PROJECT_DEFINED)
        MESSAGE_EX("buildrules.cmake warning: ADD_${prjtype}_EX without corresponding PROJECT_EX(2) in ${CURDIR}. Project[${prjname}]")
        SET(__nobuild_ yes)
    ELSEIF (NOT __len_ EQUAL 1)
        MESSAGE_EX("buildrules.cmake warning: nested PROJECT_EX(2) in ${CURDIR}. Projects[${PROJECT_DEFINED}]")
        SET(__nobuild_ yes)
    ENDIF ()
    CDR(PROJECT_DEFINED ${PROJECT_DEFINED})

    IF (__nobuild_)
        MESSAGE_EX("buildrules.cmake warning: ADD_${prjtype}_EX, target [${prjname}] not created")
    ENDIF ()

    SET(${prjname}_EXCLUDED ${__nobuild_})
    SET(CURTARGET_EXCLUDED ${__nobuild_})
ENDMACRO ()

# check duplicate target and rename target if target with similar name is already exists
MACRO (CHECK_DUPLICATE_TARGET prjname)
    GET_TARGET_PROPERTY(__target_type_ "${${prjname}}" TYPE)
    IF (__target_type_)
        GET_TARGET_PROPERTY(__target_location_ "${${prjname}}" LOCATION)
        SET(__old_prjname ${${prjname}})
        GET_UNIQ_NAME(${prjname} TO_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${${prjname}})
        FILE(RELATIVE_PATH __rp_existed ${ARCADIA_BUILD_ROOT} ${__target_location_})
        MESSAGE_EX("Rename target to \"${${prjname}}\", because target \"${__old_prjname}\" of type ${__target_type_} is already defined, its binary located at ${__rp_existed}")
    ENDIF ()
ENDMACRO ()

MACRO(SET_IDE_FOLDER target)
    IF (DONT_USE_FOLDERS)
        SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS OFF)
    ELSE ()
        SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
    ENDIF ()
    IF (MSVC OR XCODE)
        IF (NOT ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} LESS 2.8)
            SET(q ${IDE_FOLDER})
            LIST(LENGTH q len)
            IF (len)
                SET_PROPERTY(TARGET "${target}" PROPERTY FOLDER "${IDE_FOLDER}")
            ELSE ()
                FILE(RELATIVE_PATH rp ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
                STRING(REGEX REPLACE "(.*)/.*" "\\1" rp ${rp})
                SET_PROPERTY(TARGET "${target}" PROPERTY FOLDER "${rp}")
            ENDIF ()
        ENDIF ()
    ENDIF ()
ENDMACRO ()

# =============================================================================
# ADD_EXECUTABLE_EX(exename sources)
#
MACRO (ADD_EXECUTABLE_EX exename)
    CHECK_PROJECT_DEFINED(${exename} EXECUTABLE)
    CHECK_SRCS(${ARGN})
    IF (NOT __nobuild_)
        SET(PROJECTNAME ${exename})
        EXEC_DTMK()
        IF (LOCAL_EXCLUDE_FROM_ALL)
            SET_APPEND(__keywords_ EXCLUDE_FROM_ALL)
        ENDIF ()

        SET(__sources_1 ${ARGN})
        IF (__sources_1)
            PREPARE_SRC_FILES(__sources_1 SRCDIR ${ARCADIA_ROOT})

            _APPLY_DEFS(${__sources_1})

            lorder_for_peerlibs(${exename} PEERLIBS PEERDEPENDS __sources_1)

            # DEBUGMESSAGE(2 "\nCMAKE_CXX_LINK_EXECUTABLE 1: ${CMAKE_CXX_LINK_EXECUTABLE}")
            # Set default flags. Don't use CMAKE_CXX_LINK_EXECUTABLE and CMAKE_C_LINK_EXECUTABLE explicitly
            IF("X${DEFAULT_CMAKE_CXX_LINK_EXECUTABLE}${DEFAULT_CMAKE_C_LINK_EXECUTABLE}X" STREQUAL "XX")
                SET(DEFAULT_CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")
                SET(DEFAULT_CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE}")
            ENDIF ()

            IF (THIRDPARTY_OBJADD)
                SEPARATE_ARGUMENTS_SPACE(THIRDPARTY_OBJADD)
                SET(CMAKE_CXX_LINK_EXECUTABLE "${DEFAULT_CMAKE_CXX_LINK_EXECUTABLE} ${THIRDPARTY_OBJADD} ")
                SET(CMAKE_C_LINK_EXECUTABLE "${DEFAULT_CMAKE_C_LINK_EXECUTABLE} ${THIRDPARTY_OBJADD} ")
                DEBUGMESSAGE(2 "  added 3rd objs: ${THIRDPARTY_OBJADD}")
            ENDIF ()
            # DEBUGMESSAGE(2 "CMAKE_CXX_LINK_EXECUTABLE 2: ${CMAKE_CXX_LINK_EXECUTABLE}\n")

            SET(tmp_CMAKE_EXECUTABLE_SUFFIX ${CMAKE_EXECUTABLE_SUFFIX})
            IF (LOCAL_EXECUTABLE_SUFFIX)
                SET(CMAKE_EXECUTABLE_SUFFIX ${LOCAL_EXECUTABLE_SUFFIX})
            ENDIF ()

            ADD_EXECUTABLE(${exename} ${__keywords_} ${__sources_1})
            SET_IDE_FOLDER(${exename})

            APPLY_BUILDAFTER(${exename} ${__sources_1})
            APPLY_PEERDIR_SUBDIR(${exename})
            GETTARGETNAME(__this_ ${exename})

            SET(${GLOBAL_TARGET_NAME}_DEPENDNAME_PROG ${exename} CACHE INTERNAL "" FORCE)
            DEBUGMESSAGE(2 "${exename}: TOOLDIR in CURDIR will set, ${GLOBAL_TARGET_NAME}_DEPENDNAME_PROG is [${exename}]")

            IF (CREATEPROG AND PLATFORM_SUPPORTS_SYMLINKS)
                IF ("${CURDIR}" STREQUAL "${BINDIR}")
                     # Do not make symlink to executable (issue related to per-project unittests)
                     ENABLE(__nolink_local_)
                ENDIF ()
                IF (NOT NOLINK AND NOT __nolink_local_)
                    ADD_CUSTOM_COMMAND(TARGET ${exename}
                        POST_BUILD
                        COMMAND ln -sf ${__this_} ${CMAKE_CURRENT_SOURCE_DIR}/${CREATEPROG}${CMAKE_EXECUTABLE_SUFFIX}
                        COMMENT "Making a symbolic link from ${__this_} to ${CMAKE_CURRENT_SOURCE_DIR}/${CREATEPROG}${CMAKE_EXECUTABLE_SUFFIX}"
                        )
                    SET_APPEND(DTMK_CLEANFILES ${CMAKE_CURRENT_SOURCE_DIR}/${CREATEPROG}${CMAKE_EXECUTABLE_SUFFIX})
                ENDIF ()

                IF (EXESYMLINK_DIR)
                    ADD_CUSTOM_COMMAND(TARGET ${exename}
                        POST_BUILD
                        COMMAND mkdir -p "${EXESYMLINK_DIR}"
                        COMMAND ln -sf ${__this_} ${EXESYMLINK_DIR}/${CREATEPROG}${CMAKE_EXECUTABLE_SUFFIX}
                        COMMENT "Making a symbolic link from ${__this_} to ${EXESYMLINK_DIR}/${CREATEPROG}${CMAKE_EXECUTABLE_SUFFIX}"
                        )
                    SET_APPEND(DTMK_CLEANFILES ${EXESYMLINK_DIR}/${CREATEPROG}${CMAKE_EXECUTABLE_SUFFIX})
                ENDIF ()
            ENDIF ()

            SET(CMAKE_EXECUTABLE_SUFFIX ${tmp_CMAKE_EXECUTABLE_SUFFIX})

            IF (NOT "X${TARGET_SHLIB_MAJOR}${TARGET_SHLIB_MINOR}X" MATCHES "XX")
                SET_TARGET_PROPERTIES(${exename} PROPERTIES
                    VERSION "${TARGET_SHLIB_MAJOR}" #.${TARGET_SHLIB_MINOR}"
                )
            ENDIF ()

        ELSE ()
            DEBUGMESSAGE(1 "ADD_EXECUTABLE_EX(${exename}) @ ${CMAKE_CURRENT_SOURCE_DIR}: no sources found. No executables will be added")
        ENDIF ()

        DEBUGMESSAGE(1 "ADD_EXECUTABLE_EX(${exename}) @ ${CMAKE_CURRENT_SOURCE_DIR} succeeded")
    ELSE ()
        DEBUGMESSAGE(1 "ADD_EXECUTABLE_EX(${exename}) @ ${CMAKE_CURRENT_SOURCE_DIR} skipped (see '${ARCADIA_BUILD_ROOT}/excl_target.list' for details)")
    ENDIF ()

    PROP_CURDIR_SET(PROJECT_TARGET_NAME ${exename})
    ON_TARGET_FINISH()
ENDMACRO ()

# =============================================================================
# ADD_LIBRARY_EX(libname)
#
MACRO (ADD_LIBRARY_EX libname)
    CHECK_PROJECT_DEFINED(${libname} LIBRARY)
    CHECK_SRCS(${ARGN})
    IF (NOT __nobuild_)
        SET(PROJECTNAME ${libname})
        EXEC_DTMK()

        IF (LOCAL_EXCLUDE_FROM_ALL)
            SET_APPEND(__keywords_ EXCLUDE_FROM_ALL)
        ENDIF ()

        SET(__sources_1 ${ARGN})
        IF (NOT __sources_1 AND PEERDIR)
            # Adding
            SET_APPEND(__sources_1 "${ARCADIA_ROOT}/cmake/include/_cmake_fake_src.cpp")
            DEBUGMESSAGE(2 "Adding fictive source file to library ${PROJECTNAME} to inherit PEERDIRs")
        ENDIF ()
        IF (__sources_1)
            PREPARE_SRC_FILES(__sources_1 SRCDIR ${ARCADIA_ROOT})

            _APPLY_DEFS(${__sources_1})

            IF (NOT "X${TARGET_SHLIB_MAJOR}${TARGET_SHLIB_MINOR}X" MATCHES "XX" AND MAKE_ONLY_SHARED_LIB)
                SET(DTMK_LIBTYPE SHARED)
            ENDIF ()

            IF ("${DTMK_LIBTYPE}" MATCHES "SHARED")
                lorder_for_peerlibs(${libname} PEERLIBS PEERDEPENDS __sources_1)
            ELSE ()
                # No need for PEERDEPENDS in STATIC libraries
                #SET(PEERDEPENDS)
            ENDIF ()

            IF ("${DTMK_LIBTYPE}" MATCHES "SHARED")
                IF (NOT EXCLUDE_SHARED_LIBRARIES_FROM_ALL)
                    ADD_LIBRARY(${libname} ${DTMK_LIBTYPE} ${__keywords_} ${__sources_1})
                ELSE ()
                    ADD_LIBRARY(${libname} ${DTMK_LIBTYPE} EXCLUDE_FROM_ALL ${__keywords_} ${__sources_1})
                ENDIF ()
            ELSE ()
                ADD_LIBRARY(${libname} ${DTMK_LIBTYPE} ${__keywords_} ${__sources_1})
                IF (NOT WIN32 AND NOT PIC_FOR_ALL_TARGETS)
                    ADD_LIBRARY("${libname}_pic" ${DTMK_LIBTYPE} EXCLUDE_FROM_ALL ${__keywords_} ${__sources_1})
                    SET_TARGET_PROPERTIES("${libname}_pic" PROPERTIES COMPILE_FLAGS "-fPIC")
                    SET_TARGET_PROPERTIES("${libname}_pic" PROPERTIES LABELS "")
                ENDIF ()
            ENDIF ()
            SET_IDE_FOLDER(${libname})

            IF ("${DTMK_LIBTYPE}" MATCHES "SHARED")
                IF (LOCAL_SHARED_LIBRARY_SUFFIX)
                    SET_TARGET_PROPERTIES(${libname} PROPERTIES
                        SUFFIX "${LOCAL_SHARED_LIBRARY_SUFFIX}"
                    )
                    DEBUGMESSAGE(2 "Changed SUFFIX[${LOCAL_SHARED_LIBRARY_SUFFIX}] for ${libname}")
                ENDIF ()
                IF (DEFINED LOCAL_SHARED_LIBRARY_PREFIX)
                    SET_TARGET_PROPERTIES(${libname} PROPERTIES
                        PREFIX "${LOCAL_SHARED_LIBRARY_PREFIX}"
                    )
                    DEBUGMESSAGE(2 "Changed PREFIX[${LOCAL_SHARED_LIBRARY_PREFIX}] for ${libname}")
                ENDIF ()
            ENDIF ()

            GET_TARGET_PROPERTY(__realname_ ${libname} LOCATION)
            DEBUGMESSAGE(3 "Realname for ${libname} is [${__realname_}]")

#           SET_TARGET_PROPERTIES(${libname} PROPERTIES
#                   COMPILE_FLAGS "${__LOCAL_COMPILE_FLAGS}")

            IF (NOT "X${TARGET_SHLIB_MAJOR}${TARGET_SHLIB_MINOR}X" MATCHES "XX")
                SET_TARGET_PROPERTIES(${libname} PROPERTIES
                    VERSION "${TARGET_SHLIB_MAJOR}" #.${TARGET_SHLIB_MINOR}"
                )
            ENDIF ()

            APPLY_BUILDAFTER(${libname} ${__sources_1})
            APPLY_PEERDIR_SUBDIR(${libname})
            IF (NOT "${DTMK_LIBTYPE}" MATCHES "SHARED" AND NOT WIN32 AND NOT PIC_FOR_ALL_TARGETS)
                APPLY_BUILDAFTER(${libname}_pic ${sources_1})
                APPLY_PEERDIR_SUBDIR(${libname}_pic)
            ENDIF ()
            GETTARGETNAME(__this_ ${libname})

            SET(${GLOBAL_TARGET_NAME}_DEPENDNAME_LIB ${libname} CACHE INTERNAL "" FORCE)
            DEBUGMESSAGE(2 "${libname}: PEERDIR in CURDIR will set, ${GLOBAL_TARGET_NAME}_DEPENDNAME_LIB is [${libname}]")

            IF ("${DTMK_LIBTYPE}" MATCHES "SHARED")
                SET(VERSION_SUFFIX)
                IF (NOT "X${TARGET_SHLIB_MAJOR}X" STREQUAL "XX")
                    SET(VERSION_SUFFIX .${TARGET_SHLIB_MAJOR})
                ENDIF ()

                IF ("${CURDIR}" STREQUAL "${BINDIR}")
                     # Do not make symlink to library
                     ENABLE(__nolink_local_)
                ENDIF ()
                IF (NOT NOLINK AND NOT __nolink_local_ AND PLATFORM_SUPPORTS_SYMLINKS)
                    GET_TARGET_PROPERTY(__realname_ ${libname} LOCATION)
                    GET_FILENAME_COMPONENT(__realname_ ${__realname_} NAME)
                    ADD_CUSTOM_COMMAND(TARGET ${libname}
                        POST_BUILD
                        COMMAND ln -sf ${__this_} ${CMAKE_CURRENT_SOURCE_DIR}/${__realname_}
                        COMMENT "Making a symbolic link from ${__this_} to ${CMAKE_CURRENT_SOURCE_DIR}/${__realname_}"
                        )
                    SET_APPEND(DTMK_CLEANFILES ${CMAKE_CURRENT_SOURCE_DIR}/${__realname_})
                ENDIF ()

                IF (PLATFORM_SUPPORTS_SYMLINKS)
                    SET(SHLIBNAME_LINK)
                    # Make symlinks:
                    #   LIBSYMLINK_DIR/libname.major -> ${__this_}
                    #   LIBSYMLINK_DIR/libname -> libname.major
                    # Vars:
                    #   SHLIBNAME - lib name with major version number
                    #   SHLIBNAME_LINK - lib name without version numbers (just an .so-extension)
                    IF (NOT "${CMAKE_BUILD_TYPE}" MATCHES "Profile")

                        GET_FILENAME_COMPONENT(SHLIBNAME_LINK ${__this_} NAME)
                        DEBUGMESSAGE(3 "SHLIBNAME_LINK=[${SHLIBNAME_LINK}]")


                        ADD_CUSTOM_COMMAND(TARGET ${libname} POST_BUILD
                            COMMAND mkdir -p "${LIBSYMLINK_DIR}"
                            COMMENT "mkdir -p '${LIBSYMLINK_DIR}'"
                            VERBATIM
                        )

                        IF (DEFINED TARGET_SHLIB_MAJOR)
                            GET_FILENAME_COMPONENT(SHLIBNAME      ${__this_} NAME_WE)
                            SET(SHLIBNAME ${SHLIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}.${TARGET_SHLIB_MAJOR})
                            DEBUGMESSAGE(3 "SHLIBNAME=[${SHLIBNAME}]")
                            ADD_CUSTOM_COMMAND(TARGET ${libname} POST_BUILD
                                COMMAND ln -sf ${__this_}${VERSION_SUFFIX} ${SHLIBNAME}
                                WORKING_DIRECTORY ${LIBSYMLINK_DIR}
                                COMMENT "Making symlinks in ${LIBSYMLINK_DIR} for ${SHLIBNAME_LINK}"
                                VERBATIM
                            )
                            ADD_CUSTOM_COMMAND(TARGET ${libname} POST_BUILD
                                COMMAND ln -sf ${SHLIBNAME} ${SHLIBNAME_LINK}
                                WORKING_DIRECTORY ${LIBSYMLINK_DIR}
                                COMMENT "Making symlinks in ${LIBSYMLINK_DIR} for ${SHLIBNAME}"
                                VERBATIM
                            )
                        ELSE ()
                            ADD_CUSTOM_COMMAND(TARGET ${libname} POST_BUILD
                                COMMAND ln -sf ${__this_} ${SHLIBNAME_LINK}
                                WORKING_DIRECTORY ${LIBSYMLINK_DIR}
                                COMMENT "Making symlinks in ${LIBSYMLINK_DIR} for ${SHLIBNAME_LINK}"
                                VERBATIM
                            )
                        ENDIF ()


                        SET_APPEND(DTMK_CLEANFILES ${LIBSYMLINK_DIR}/${SHLIBNAME_LINK} ${LIBSYMLINK_DIR}/${SHLIBNAME})
                    ENDIF ()
                ENDIF ()

            ENDIF ()

            DEBUGMESSAGE(1 "ADD_LIBRARY_EX(${libname} ${DTMK_LIBTYPE}) @ ${CMAKE_CURRENT_SOURCE_DIR} succeeded")
        ELSE ()
            DEBUGMESSAGE(1 "ADD_LIBRARY_EX(${libname} ${DTMK_LIBTYPE}) @ ${CMAKE_CURRENT_SOURCE_DIR}: no sources found. No libraries will be added")
        ENDIF ()
    ELSE ()
        DEBUGMESSAGE(1 "ADD_LIBRARY_EX(${libname} ${DTMK_LIBTYPE}) @ ${CMAKE_CURRENT_SOURCE_DIR} skipped (see '${ARCADIA_BUILD_ROOT}/excl_target.list' for details)")
    ENDIF ()

    PROP_CURDIR_SET(PROJECT_TARGET_NAME ${libname})
    ON_TARGET_FINISH()
ENDMACRO ()

# =============================================================================
# APPLY_PEERDIR_SUBDIR(prjname)
#
MACRO(APPLY_PEERDIR_SUBDIR prjname)
    FOREACH(__itemprop_ ${CHECK_TARGETPROPERTIES})
        SET_TARGET_PROPERTIES(${prjname} PROPERTIES
            ${__itemprop_} "${${__itemprop_}}")
    ENDFOREACH ()

    # Подвязываем список библиотек
    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: PEERDIR=[${PEERDIR}]")
    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: PEERLIBS=[${PEERLIBS}]")
    SET(__peernames_)
    SET(__prop_error_overall_)
    FOREACH(__item_ ${PEERDEPENDS})
        GET_TARGET_PROPERTY(__prop_ ${__item_} TYPE)
        SET_APPEND(__peernames_ "${__item_}:${__prop_}")

        FOREACH(__itemprop_ ${CHECK_TARGETPROPERTIES})
            GET_TARGET_PROPERTY(__prop_ ${__item_} ${__itemprop_})
            SET(__prop_error_ no)
            IF (__prop_ AND NOT ${__itemprop_})
                SET(__prop_error_ yes)
            ELSEIF (NOT __prop_ AND ${__itemprop_})
                SET(__prop_error_ yes)
            ENDIF ()
            IF (__prop_error_)
                SET_APPEND(__prop_error_overall_ COMMAND echo "Targets ${prjname} and ${__item_} have different ${__itemprop_} property (${${__itemprop_}}/${__prop_})")
                MESSAGE_EX("Targets ${prjname} and ${__item_} have different ${__itemprop_} property (${${__itemprop_}}/${__prop_})")
            ENDIF ()
        ENDFOREACH ()
    ENDFOREACH ()
    IF (__prop_error_overall_)
        ADD_CUSTOM_COMMAND(TARGET ${prjname}
            PRE_BUILD
            ${__prop_error_overall_}
            COMMAND false
            VERBATIM
        )
    ENDIF ()
    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: PEERDEPENDS=[${__peernames_}]")
    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: OBJADDE=[${OBJADDE}]")

    IF (NOT USE_WEAK_DEPENDS)
        #GET_TARGET_PROPERTY(__prop_ ${prjname} TYPE)

        TARGET_LINK_LIBRARIES(${prjname} ${OBJADDE})

        IF (PEERDEPENDS)# AND NOT "${__prop_}" STREQUAL "STATIC_LIBRARY")
            IF (NOT WIN32 OR NOT ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} LESS 2.8)
                TARGET_LINK_LIBRARIES(${prjname} ${PEERDEPENDS})
            ENDIF ()

            ADD_DEPENDENCIES(${prjname} ${PEERDEPENDS})
        ENDIF ()

    ELSE ()

        SET(PEERDEPENDS_NEW)
        SET(OBJADDE_NEW)
        FOREACH (__peer_ ${PEERDEPENDS})
            GET_TARGET_PROPERTY(__peer_value_ ${__peer_} PEERDEPENDS)
            IF (NOT __peer_value_)
                SET( __peer_value_)
            ENDIF ()
            GET_TARGET_PROPERTY(__obj_value_ ${__peer_} OBJADDE)
            IF (NOT __obj_value_)
                SET( __obj_value_)
            ENDIF ()
            SET(PEERDEPENDS_NEW ${PEERDEPENDS_NEW} ${__peer_value_})
            SET(OBJADDE_NEW ${OBJADDE_NEW} ${__obj_value_})
        ENDFOREACH ()

        DEBUGMESSAGE(2 "${prjname}'s inherited PEERDEPENDS[${PEERDEPENDS_NEW}]")
        DEBUGMESSAGE(2 "${prjname}'s PEERDEPENDS[${PEERDEPENDS}]")
        DEBUGMESSAGE(2 "${prjname}'s inherited OBJADDE[${OBJADDE_NEW}]")
        DEBUGMESSAGE(2 "${prjname}'s OBJADDE[${OBJADDE}]")

        SET(__objadde_ ${OBJADDE_NEW} ${OBJADDE})
        LIST_REMOVE_DUPLICATES(__objadde_ "^-\([lL].*\)|^-\(Wl,[-]E\)|\(kthread\)|\(rdynamic\)")

        SET(__peers_ ${PEERDEPENDS_NEW} ${PEERDEPENDS})
        LIST_REMOVE_DUPLICATES(__peers_)

        SET(__peers2_)
        SET(__peers_pic_)
        FOREACH (__p1_ ${__peers_})
            SET_APPEND(__peers2_ ${__p1_})
            IF (NOT "${__p1_}X" MATCHES "_picX$")
                GET_TARGET_PROPERTY(__p1_prop_ ${__p1_} TYPE)
                IF ("${__p1_prop_}" STREQUAL "SHARED_LIBRARY")
                    SET_APPEND(__peers_pic_ ${__p1_})
                ELSE ()
                    SET_APPEND(__peers_pic_ ${__p1_}_pic)
                ENDIF ()
            ELSE ()
                SET_APPEND(__peers_pic_ ${__p1_})
            ENDIF ()
        ENDFOREACH ()
        SET(__peers_ ${__peers2_})

        GET_TARGET_PROPERTY(__prop_ ${prjname} TYPE)

        IF (NOT "${__prop_}" STREQUAL "STATIC_LIBRARY")
            TARGET_LINK_LIBRARIES(${prjname} ${__objadde_})
            IF (NOT "X${__peers_}X" STREQUAL "XX")
                IF ("${__prop_}" STREQUAL "SHARED_LIBRARY" AND NOT WIN32 AND NOT PIC_FOR_ALL_TARGETS)
                    FOREACH (__p1_ ${__peers_pic_})
                        GET_TARGET_PROPERTY(__p1_type_ ${__p1_} TYPE)
                        IF ("X${__p1_type_}X" STREQUAL "XSTATIC_LIBRARYX")
                            GET_TARGET_PROPERTY(__labels_ "${__p1_}" LABELS)
                            LIST(FIND __labels_ "PIC_REQUIRED" __pic_required_)
                            LIST(FIND __labels_ "NON_PIC_REQUIRED" __non_pic_required_)
                            IF (${__pic_required_} LESS 0)
                                STRING(REGEX REPLACE "_pic$" "" __p2_ ${__p1_})
                                IF (${__non_pic_required_} LESS 0)
                                    DEBUGMESSAGE(2 "Exclude ${__p2_} from all.")
                                    SET_TARGET_PROPERTIES(${__p2_} PROPERTIES EXCLUDE_FROM_ALL "yes")
                                    SET_TARGET_PROPERTIES(${__p1_} PROPERTIES EXCLUDE_FROM_ALL "no")
                                    DEBUGMESSAGE(2 "Make ${__p2_} dependent from ${__p1_}.")
                                    ADD_DEPENDENCIES(${__p2_} ${__p1_})
                                ELSE ()
                                    DEBUGMESSAGE(2 "Return ${__p1_} to all.")
                                    SET_TARGET_PROPERTIES(${__p1_} PROPERTIES EXCLUDE_FROM_ALL "no")
                                ENDIF ()
                                LIST(APPEND __labels_ "PIC_REQUIRED")
                                SET_TARGET_PROPERTIES(${__p1_} PROPERTIES LABELS "${__labels_}")
                            ENDIF ()
                        ENDIF ()
                    ENDFOREACH ()
                    ADD_DEPENDENCIES(${prjname} ${__peers_pic_})
                ELSE ()
                    IF (NOT PIC_FOR_ALL_TARGETS)
                        FOREACH (__p1_ ${__peers_})
                            GET_TARGET_PROPERTY(__p1_type_ ${__p1_} TYPE)
                            IF ("X${__p1_type_}X" STREQUAL "XSTATIC_LIBRARYX" AND NOT "${prjname}X" MATCHES "_picX$" AND NOT "${prjname}X" MATCHES "_utX$")
                                GET_TARGET_PROPERTY(__labels_ "${__p1_}_pic" LABELS)
                                LIST(FIND __labels_ "PIC_REQUIRED" __pic_required_)
                                LIST(FIND __labels_ "NON_PIC_REQUIRED" __non_pic_required_)
                                IF (${__non_pic_required_} LESS 0)
                                    IF (${__pic_required_} LESS 0)
                                        DEBUGMESSAGE(2 "Make ${__p1_}_pic dependent from ${__p1_}.")
                                        ADD_DEPENDENCIES("${__p1_}_pic" ${__p1_})
                                    ELSE ()
                                        DEBUGMESSAGE(2 "Return ${__p1_} to all.")
                                        SET_TARGET_PROPERTIES(${__p1_} PROPERTIES EXCLUDE_FROM_ALL "no")
                                    ENDIF ()
                                    LIST(APPEND __labels_ "NON_PIC_REQUIRED")
                                    SET_TARGET_PROPERTIES("${__p1_}_pic" PROPERTIES LABELS "${__labels_}")
                                ENDIF ()
                            ENDIF ()
                        ENDFOREACH ()
                    ENDIF ()
                    ADD_DEPENDENCIES(${prjname} ${__peers_})
                ENDIF ()
                SET(${prjname}_PEERLIBS ${__peers_})
                IF (NOT WIN32 OR NOT ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} LESS 2.8)
                    IF ("${__prop_}" STREQUAL "SHARED_LIBRARY" AND NOT PIC_FOR_ALL_TARGETS)
                        TARGET_LINK_LIBRARIES(${prjname} ${__peers_pic_})
                    ELSE ()
                        TARGET_LINK_LIBRARIES(${prjname} ${__peers_})
                    ENDIF ()
                ENDIF ()
            ENDIF ()
            DEBUGMESSAGE(2 "${prjname} use PEERDEPENDS[${__peers_}]")
            DEBUGMESSAGE(2 "${prjname} use OBJADDE[${__objadde_}]")
        ELSE ()
            IF (__peers_)
                SET(${prjname}_PEERLIBS ${__peers_})
                SET_TARGET_PROPERTIES(${prjname} PROPERTIES
                    PEERDEPENDS "${__peers_}"
                )
            ENDIF ()

            DEBUGMESSAGE(2 "${prjname} is a static library. Will save PEERDEPENDS[${PEERDEPENDS}]")
            DEBUGMESSAGE(2 "${prjname} is a static library. Will save OBJADDE[${OBJADDE}]")
        ENDIF ()

    ENDIF ()

    SET(__dtmk_l_ ${DTMK_L})
    SEPARATE_ARGUMENTS_SPACE(__dtmk_l_)
    DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: DTMK_L=[${DTMK_L}]")
    SET_TARGET_PROPERTIES(${prjname} PROPERTIES
        LINK_FLAGS "${__dtmk_l_}")

    IF (USE_GOOGLE_ALLOCATOR)
        SET_TARGET_PROPERTIES(${prjname}
            PROPERTIES LINKER_LANGUAGE CXX)
    ENDIF ()

    IF (USE_J_ALLOCATOR)
        SET_TARGET_PROPERTIES(${prjname}
            PROPERTIES LINKER_LANGUAGE CXX)
    ENDIF ()

    IF (USE_LF_ALLOCATOR)
        SET_TARGET_PROPERTIES(${prjname}
            PROPERTIES LINKER_LANGUAGE CXX)
    ENDIF ()

    IF (USE_LOCKLESS_ALLOCATOR)
        SET_TARGET_PROPERTIES(${prjname}
            PROPERTIES LINKER_LANGUAGE CXX)
    ENDIF ()

    GET_TARGET_PROPERTY(__linker_lang_ ${prjname} LINKER_LANGUAGE)
    IF (NOT __linker_lang_)
        SET_TARGET_PROPERTIES(${prjname}
            PROPERTIES LINKER_LANGUAGE CXX)
        DEBUGMESSAGE(3 "${CMAKE_CURRENT_SOURCE_DIR}: LINKER_LANGUAGE for ${prjname} set to CXX")
    ENDIF ()

    IF (${prjname}_OUTPUTNAME)
        SET_TARGET_PROPERTIES(
            ${prjname} PROPERTIES
                OUTPUT_NAME ${${prjname}_OUTPUTNAME}
        )
        DEBUGMESSAGE(3 "${CURDIR}: OUTPUT_NAME=[${${prjname}_OUTPUTNAME}]")
    ENDIF ()

    GETTARGETNAME(__location_ ${prjname})
    FILE(APPEND ${TARGET_LIST_FILENAME} "${prjname} ${CURDIR} ${__location_} ${BINDIR}\n")
ENDMACRO ()

# =============================================================================
# INIT_CONFIG path_to_root
# This macro should be just before the PROJECT_EX directive
#
MACRO (INIT_CONFIG)
    FILE(RELATIVE_PATH __var_ "${CMAKE_CURRENT_SOURCE_DIR}" "${ARCADIA_ROOT}/CMakeLists.txt")
    GET_FILENAME_COMPONENT(__var_ ${__var_} PATH)
    DEBUGMESSAGE(4 "PathToRoot[${__var_}] for ${CMAKE_CURRENT_SOURCE_DIR}")

    SET(PATH_TO_ROOT ${__var_})

    INCLUDE_FROM(local.cmake ${ARCADIA_ROOT}/.. ${ARCADIA_ROOT} ${ARCADIA_BUILD_ROOT}/.. ${ARCADIA_BUILD_ROOT})
ENDMACRO ()

MACRO (ADD_YTEST test_name script_rel_path)
    GETTARGETNAME(__binary_path_)
    GET_FILENAME_COMPONENT(__project_filename ${__binary_path_} NAME)
    ADD_YTEST_BASE(${test_name} ${script_rel_path} ${__binary_path_} ${__project_filename} ${ARGN})
ENDMACRO ()

MACRO (ADD_FLEUR_TEST_DIR)
# TODO
ENDMACRO ()

MACRO (RESET_YTEST_BASE_FLAGS)
    SET(__next_data_)
    SET(__next_deps_)
ENDMACRO ()

# =============================================================================
# ADD_YTEST_BASE
# This macro check targetname before ADD_TEST_BASE and implement some common tasks
#
MACRO (ADD_YTEST_BASE test_name script_rel_path binary_path project_filename)
    RESET_YTEST_BASE_FLAGS()
    SET(__test_data_)
    SET(__test_deps_)
    FOREACH(__item_ ${ARGN})
        IF("${__item_}" STREQUAL "ignored")
            SET(__test_ignored_ yes)
        ELSEIF("${__item_}" STREQUAL "DATA")
            RESET_YTEST_BASE_FLAGS()
            SET(__next_data_ yes)
        ELSEIF("${__item_}" STREQUAL "DEPENDS")
            RESET_YTEST_BASE_FLAGS()
            SET(__next_deps_ yes)
        ELSE ()
            IF (${__next_data_})
                SET_APPEND(__test_data_ ${__item_})
            ELSEIF (${__next_deps_})
                SET_APPEND(__test_deps_ ${__item_})
            ELSE ()
                MESSAGE_EX("Bad arg inside ADD_YTEST_BASE: ${__item_}")
                MESSAGE(SEND_ERROR)
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    DEBUGMESSAGE(1, "Test ${test_name} ${script_rel_path} ${binary_path}")
    IF (LASTPRJNAME AND NOT SAVEDNAME)
        IF ("X${__test_deps_}X" STREQUAL "XX")
            SET(YTEST_CUSTOM_DEPENDS "null")
        ELSE ()
            SET(YTEST_CUSTOM_DEPENDS ${__test_deps_})
            RECURSE_ROOT_RELATIVE(${__test_deps_})
        ENDIF ()

        FILE (APPEND ${TEST_DART_TMP_FILENAME} "TEST-NAME: ${test_name}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "SCRIPT-REL-PATH: ${script_rel_path}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "TESTED-PROJECT-NAME: ${LASTPRJNAME}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "TESTED-PROJECT-FILENAME: ${project_filename}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "SOURCE-FOLDER-PATH: ${CMAKE_CURRENT_SOURCE_DIR}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "BUILD-FOLDER-PATH: ${CMAKE_CURRENT_BINARY_DIR}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "BINARY-PATH: ${binary_path}\n")
        JOIN("${YTEST_CUSTOM_DEPENDS}" " " __spaced_deps_)
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "CUSTOM-DEPENDENCIES: ${__spaced_deps_}\n")
        JOIN("${__test_data_}" " " __spaced_test_data_)
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "TEST-DATA: ${__spaced_test_data_}\n")
        FILE (APPEND ${TEST_DART_TMP_FILENAME} "=============================================================\n")

        FILE(APPEND ${TEST_LIST_FILENAME} "${LASTPRJNAME} ${CURDIR} ${binary_path} ${test_name}\n")
    ELSE ()
        MESSAGE_EX("Unknown targetname, move macro ADD_TEST_EX after END()")
        MESSAGE(SEND_ERROR)
    ENDIF ()
ENDMACRO ()

# =============================================================================
# OPTION_NEEDS
# If option1 is set and option2 is not, then send an error.
#
MACRO(OPTION_NEEDS option1 option2)
    IF (${option1} AND NOT ${option2})
        MESSAGE_EX("Option ${option1} needs ${option2}")
        MESSAGE(SEND_ERROR)
    ENDIF ()
ENDMACRO ()

ENABLE(USE_OPTIMIZATION)

MACRO (GENERATE_ENUM_SERIALIZATION)
    SET(__first yes)
    FOREACH(__item ${ARGN})
        IF (__first)
            SET(file_for_parsing "${__item}")
            SET(__first no)
        ELSE ()
            SET(ns "${__item}")
        ENDIF ()
    ENDFOREACH ()

    GET_FILENAME_COMPONENT(name ${file_for_parsing} NAME_WE)
    SET(FILE_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/${name}_serialized.cpp)

    LUA(${ARCADIA_ROOT}/devtools/ymake/buildrules/scripts/parseenum.lua ${file_for_parsing} ${FILE_GENERATED} ${ns} IN ${file_for_parsing} OUT ${FILE_GENERATED})

    DIR_ADD_GENERATED_SRC(${FILE_GENERATED})
ENDMACRO ()

MACRO (SET_GITREVISION)
    EXECUTE_PROCESS(
        COMMAND git describe --tags --dirty
        OUTPUT_VARIABLE __git_describe
        WORKING_DIRECTORY ${ARCADIA_ROOT}
        TIMEOUT 1
        RESULT_VARIABLE __git_result
    )
    IF (__git_result EQUAL 0)
        STRING(REGEX MATCH "[^\r\n]+" __git_describe "${__git_describe}")
        ADD_DEFINITIONS(-DGIT_TAG="\\"${__git_describe}\\"")
    ENDIF ()
ENDMACRO ()

MACRO (SET_SVNREVISION)
    IF (NOT NO_UTIL_SVN_DEPEND)
        EXECUTE_PROCESS(
            COMMAND svn info
            OUTPUT_VARIABLE __svn_info
            WORKING_DIRECTORY ${ARCADIA_ROOT}
            TIMEOUT 1
            RESULT_VARIABLE __svn_result
        )
        IF (__svn_result EQUAL 0)
            STRING(REGEX MATCH   "Revision: *[0-9]+"  __svn_revision "${__svn_info}")
            STRING(REGEX REPLACE "Revision: *" ""     __svn_revision "${__svn_revision}")
            STRING(REGEX MATCH   "URL: *[^\r\n ]+"  __svn_arcroot "${__svn_info}")
            STRING(REGEX REPLACE "URL: *" ""        __svn_arcroot "${__svn_arcroot}")
            STRING(REGEX MATCH   "Last Changed Date: *[^\r\n(]+" __svn_time "${__svn_info}")
            STRING(REGEX REPLACE "Last Changed Date: *" ""       __svn_time "${__svn_time}")
            DEBUGMESSAGE(1 "rev (cmd): ${__svn_revision} from ${__svn_arcroot} at ${__svn_time}")
        ELSE ()
            # this works for text-based older svn client metadata only
            IF (EXISTS "${ARCADIA_ROOT}/.svn/entries")
                FILE(STRINGS ${ARCADIA_ROOT}/.svn/entries __svn_entries LIMIT_COUNT 10)
                DEBUGMESSAGE(2 "entries: ${__svn_entries}")
                LIST(LENGTH __svn_entries __len)
                IF (${__len} GREATER 9)
                    LIST(GET __svn_entries 3 __svn_revision)
                    LIST(GET __svn_entries 4 __svn_arcroot)
                    LIST(GET __svn_entries 9 __svn_time)
                    DEBUGMESSAGE(1 "rev (txt): ${__svn_revision}")
                ENDIF ()
            ENDIF ()
        ENDIF ()

        IF (__svn_revision)
            ADD_DEFINITIONS(-DSVN_REVISION="\\"${__svn_revision}\\"")
            ADD_DEFINITIONS(-DSVN_ARCROOT="\\"${__svn_arcroot}\\"")
            ADD_DEFINITIONS(-DSVN_TIME="\\"${__svn_time}\\"")
        ENDIF ()
    ENDIF ()
ENDMACRO ()

MACRO (NO_OPTIMIZE)
    ENABLE(NO_OPTIMIZE)
ENDMACRO ()

MACRO (NOUTIL)
    ENABLE(NOUTIL)
ENDMACRO ()

MACRO (NO_COMPILER_WARNINGS)
    ENABLE(NO_COMPILER_WARNINGS)
ENDMACRO ()

MACRO (WERROR)
    ENABLE(WERROR)
ENDMACRO ()

MACRO (NOLIBC)
    NORUNTIME()
    ENABLE(NOLIBC)
ENDMACRO ()

MACRO (NOPLATFORM)
    NOLIBC()
    ENABLE(NOPLATFORM)
ENDMACRO ()

MACRO (NOJOINSRC)
    ENABLE(NOJOINSRC)
ENDMACRO ()

MACRO (JOINSRC)
    ENABLE(JOINSRC)
ENDMACRO ()

MACRO (ADDINCLSELF)
    ENABLE(ADDINCLSELF)
ENDMACRO ()

MACRO (USE_MKL_RT)
    IF (USE_MKL AND NOT MIC_ARCH AND NOT LINK_STATICALLY)
        EXTRALIBS(-lmkl_rt -lpthread -lm)
    ENDIF ()
ENDMACRO ()

IF (CMAKE_GENERATOR STREQUAL "Ninja")

MACRO(APPLY_BUILDAFTER prjname)
    IF (THISPROJECTDEPENDSDIRSNAMES)
        SET(__generated_includes)
        FOREACH(__dir ${THISPROJECTDEPENDSDIRSNAMES})
            SET(__peerpath ${ARCADIA_ROOT}/${__dir})
            SET(__gen_incs)
            PROP_DIR_GET(__gen_incs ${__peerpath} DIR_GENERATED_INCLUDES)
            SET_APPEND(__generated_includes ${__gen_incs})
        ENDFOREACH ()
        IF (__generated_includes)
            LIST(REMOVE_DUPLICATES __generated_includes)
            FOREACH(__item_ ${ARGN})
                SET_PROPERTY(SOURCE ${__item_} APPEND
                    PROPERTY OBJECT_DEPENDS ${__generated_includes})
            ENDFOREACH ()
        ENDIF ()
    ENDIF ()
ENDMACRO ()

ELSE ()

MACRO(APPLY_BUILDAFTER prjname)
    IF (THISPROJECTDEPENDS)
        DEBUGMESSAGE(2 "${prjname} depends on [${THISPROJECTDEPENDS}] (set in BUILDAFTER)")
        ADD_DEPENDENCIES(${prjname} ${THISPROJECTDEPENDS})
    ENDIF ()
ENDMACRO ()

ENDIF ()
