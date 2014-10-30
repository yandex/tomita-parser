# First, remove per-session cached variables

GET_CMAKE_PROPERTY(vars CACHE_VARIABLES)
FOREACH(item ${vars})
    IF (item MATCHES "_DEPENDNAME_(LIB|PROG)")
        SET(${item} "" CACHE INTERNAL "" FORCE)
    ENDIF ()
ENDFOREACH ()

# Some variables are set here (please find details at http://www.cmake.org/Wiki/CMake_Useful_Variables)
SET(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES yes)
SET(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES yes)
SET(CMAKE_SKIP_BUILD_RPATH yes)

cmake_minimum_required(VERSION 2.8.0)

IF (COMMAND cmake_policy)
    cmake_policy(SET CMP0002 OLD)
    cmake_policy(SET CMP0000 OLD)
    cmake_policy(SET CMP0005 OLD)
    cmake_policy(SET CMP0003 NEW)
ENDIF ()

IF (NOT DEFINED DEBUG_MESSAGE_LEVEL AND DEFINED DML)
    SET(DEBUG_MESSAGE_LEVEL ${DML})
ENDIF ()

# Saving ARCADIA_ROOT and ARCADIA_BUILD_ROOT into the cache

IF (LINUX)
    SET(REALPATH readlink)
    SET(REALPATH_FLAGS -f)
ELSEIF (FREEBSD)
    SET(REALPATH realpath)
    SET(REALPATH_FLAGS)
ENDIF ()

SET(SAVE_TEMPS $ENV{SAVE_TEMPS})

IF (NOT ARCADIA_ROOT)
    IF (0) #REALPATH)
        EXECUTE_PROCESS(COMMAND ${REALPATH} ${REALPATH_FLAGS} "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE ARCADIA_ROOT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        SET(CMAKE_CURRENT_SOURCE_DIR ${ARCADIA_ROOT})
        SET(CMAKE_SOURCE_DIR ${ARCADIA_ROOT})
    ELSE ()
        SET(ARCADIA_ROOT "${CMAKE_SOURCE_DIR}")
    ENDIF ()
    SET(ARCADIA_ROOT "${ARCADIA_ROOT}" CACHE PATH "Arcadia sources root dir" FORCE)
ENDIF ()

IF (NOT ARCADIA_BUILD_ROOT)
    IF (0) #REALPATH)
        EXECUTE_PROCESS(COMMAND ${REALPATH} ${REALPATH_FLAGS} "${CMAKE_BINARY_DIR}"
            OUTPUT_VARIABLE ARCADIA_BUILD_ROOT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        SET(CMAKE_CURRENT_BINARY_DIR ${ARCADIA_BUILD_ROOT})
        SET(CMAKE_BINARY_DIR ${ARCADIA_BUILD_ROOT})
    ELSE ()
        SET(ARCADIA_BUILD_ROOT "${CMAKE_BINARY_DIR}")
    ENDIF ()
    SET(ARCADIA_BUILD_ROOT "${ARCADIA_BUILD_ROOT}" CACHE PATH "Arcadia binary root dir" FORCE)
ENDIF ()

IF (WIN32)
    IF (NOT "X${YTEST_TARGET}X" STREQUAL "X${CMAKE_CURRENT_SOURCE_DIR}X")
        ADD_CUSTOM_TARGET(ytest python ${ARCADIA_ROOT}/check/run_test.py
            WORKING_DIRECTORY ${ARCADIA_BUILD_ROOT})
        SET_PROPERTY(TARGET "ytest" PROPERTY FOLDER "_CMakeTargets")
        SET (YTEST_TARGET ${CMAKE_CURRENT_SOURCE_DIR})
    ENDIF ()
ENDIF ()

GET_FILENAME_COMPONENT(__cmake_incdir_ "${CMAKE_CURRENT_LIST_FILE}" PATH)

# Here - only macros like SET_APPEND
INCLUDE(${__cmake_incdir_}/MakefileHelpers.cmake)
# Set OS_NAME and OS-specific variables
INCLUDE(${__cmake_incdir_}/config.cmake)

IF (DONT_USE_FOLDERS)
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS OFF)
ELSE ()
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
ENDIF ()

SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakeTargets")

SET(TEST_SCRIPTS_DIR ${ARCADIA_ROOT}/test)

IF (NOT WIN32)
    SET(PLATFORM_SUPPORTS_SYMLINKS yes)
    ENABLE(USE_WEAK_DEPENDS) # It should be enabled in build platforms with many-projects-at-once building capability
ENDIF ()
IF (WIN32 OR XCODE)
    # Force vcproj generator not to include CMakeLists.txt and its custom rules into the project
    DEFAULT(CMAKE_SUPPRESS_REGENERATION yes)
    # To set IDE folder for an arbitrary project please define <project name>_IDE_FOLDER variable.
    # For example, SET(gperf_IDE_FOLDER "tools/build")
    DEFAULT(CMAKE_DEFAULT_IDE_FOLDER "<curdir>")
ENDIF ()

INCLUDE(${__cmake_incdir_}/scripts.cmake)
INCLUDE(${__cmake_incdir_}/global_keyword.cmake)
INCLUDE(${__cmake_incdir_}/buildrules.cmake)
INCLUDE_FROM(local.cmake ${ARCADIA_ROOT}/.. ${ARCADIA_ROOT} ${ARCADIA_BUILD_ROOT}/.. ${ARCADIA_BUILD_ROOT})
INCLUDE(${__cmake_incdir_}/deps.cmake)
DEFAULT (CALC_CMAKE_DEPS true)

MACRO (ON_CMAKE_START)
    IF (COMMAND ON_CMAKE_START_HOOK)
        ON_CMAKE_START_HOOK()
    ENDIF ()
    IF (CALC_CMAKE_DEPS)
        CMAKE_DEPS_INIT()
    ENDIF ()
ENDMACRO ()
# for symmetry
ON_CMAKE_START()

MACRO (ON_CMAKE_FINISH)
    IF (COMMAND ON_CMAKE_FINISH_HOOK)
        ON_CMAKE_FINISH_HOOK()
    ENDIF ()
    IF (CALC_CMAKE_DEPS)
        CMAKE_DEPS_FINISH()
    ENDIF ()
ENDMACRO ()


# ============================================================================ #
# Here are global build variables (you can override them in your local.cmake)

DEFAULT(USE_J_ALLOCATOR yes)

IF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
    DISABLE(USE_STATIC_CPP_RUNTIME)
ENDIF ()

DEFAULT(USE_STATIC_CPP_RUNTIME yes)
DEFAULT(LINK_STATIC_LIBS yes)
DEFAULT(CHECK_TARGETPROPERTIES
#    USE_GOOGLE_ALLOCATOR
)
ENABLE(NO_LIBBIND)

SET(__USE_GENERATED_BYK_ $ENV{USE_GENERATED_BYK})
IF ("X${__USE_GENERATED_BYK_}X" STREQUAL "XX")
    DEFAULT(USE_GENERATED_BYK yes)
ELSE ()
    DEFAULT(USE_GENERATED_BYK $ENV{USE_GENERATED_BYK})
ENDIF ()

SET($ENV{LC_ALL} "C")

IF (WIN32)
    SET(EXECUTABLE_OUTPUT_PATH ${ARCADIA_BUILD_ROOT}/bin)
    SET(LIBRARY_OUTPUT_PATH    ${EXECUTABLE_OUTPUT_PATH})
ELSE ()
    SET(EXESYMLINK_DIR ${ARCADIA_BUILD_ROOT}/bin)
    SET(LIBSYMLINK_DIR ${ARCADIA_BUILD_ROOT}/lib)
ENDIF ()

# End of global build variables list

INCLUDE(${__cmake_incdir_}/tools.cmake)
INCLUDE(${__cmake_incdir_}/suffixes.cmake)

# Macro - get varname from ENV
MACRO(GETVAR_FROM_ENV)
    FOREACH (varname ${ARGN})
        IF (NOT DEFINED ${varname})
            SET(${varname} $ENV{${varname}})
        ENDIF ()
        DEBUGMESSAGE(1, "${varname}=${${varname}}")
    ENDFOREACH ()
ENDMACRO ()

# Get some vars from ENV
GETVAR_FROM_ENV(MAKE_ONLY USE_TIME COMPILER_PREFIX NO_STRIP)

IF (MAKE_RELEASE OR DEFINED release)
    SET(CMAKE_BUILD_TYPE Release)
ELSEIF (MAKE_COVERAGE OR DEFINED coverage)
    SET(CMAKE_BUILD_TYPE Coverage)
ELSEIF (MAKE_COVERAGE OR DEFINED profile)
    SET(CMAKE_BUILD_TYPE Profile)
ELSEIF (MAKE_VALGRIND OR DEFINED valgrind)
    SET(CMAKE_BUILD_TYPE Valgrind)
ELSEIF (DEFINED debug)
    SET(CMAKE_BUILD_TYPE Debug)
ELSE ()
    # Leaving CMAKE_BUILD_TYPE intact
ENDIF ()

# Default build type - DEBUG
IF (NOT WIN32 AND NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF ()

IF (NOT "X${CMAKE_BUILD_TYPE}X" STREQUAL "XX")
    # Enable MAKE_<config> to simplify checks
    STRING(TOUPPER "${CMAKE_BUILD_TYPE}" __upcase_)
    ENABLE(MAKE_${__upcase_})
ENDIF ()

MACRO (CACHE_VAR __cached_varname_ __argn_varname_ varname type descr)
    IF (DEFINED ${varname})
        SET(${__cached_varname_} "${${__cached_varname_}} ${varname}[${${varname}}]")
        SET(${varname} "${${varname}}" CACHE ${type} "${descr}" FORCE)
        DEBUGMESSAGE(1, "Caching ${varname}[${${varname}}]")
    ENDIF ()
    SET(${__argn_varname_} ${ARGN})
ENDMACRO ()

MACRO (CACHE_VARS)
    SET(__cached_)
    SET(__vars_ ${ARGN})
    WHILE(NOT "X${__vars_}X" STREQUAL "XX")
        CACHE_VAR(__cached_ __vars_ ${__vars_})
    ENDWHILE(NOT "X${__vars_}X" STREQUAL "XX")
    IF (__cached_)
        MESSAGE(STATUS "Cached:${__cached_}")
    ENDIF ()
ENDMACRO ()

SET(METALIBRARY_NAME CMakeLists.lib)
SET(METALIB_LIST "")

# Save CMAKE_BUILD_TYPE etc.
CACHE_VARS(
    MAKE_ONLY STRING "Used to strip buildtree. May contain zero or more paths relative to ARCADIA_ROOT"
    CMAKE_BUILD_TYPE STRING "Build type (Release, Debug, Valgrind, Profile, Coverage)"
    USE_TIME BOOL "Adds 'time' prefix to c/c++ compiler line"
    COMPILER_PREFIX STRING "Adds arbitrary prefix to c/c++ compiler line"
)

# processed_dirs.txt holds all processed directories
SET(PROCESSED_DIRS_FILE ${ARCADIA_BUILD_ROOT}/processed_dirs.txt)
FILE(WRITE "${PROCESSED_DIRS_FILE}" "Empty\n")
SET(PROCESSED_TARGETS_FILE ${ARCADIA_BUILD_ROOT}/processed_targets.txt)
FILE(WRITE "${PROCESSED_TARGETS_FILE}" "Empty\n")

# init some files
# The first file holds <target name> <source path> <target file name> for all targets
SET(__filenames_
    TARGET_LIST_FILENAME target.list
    TEST_LIST_FILENAME test.list
    TEST_DART_TMP_FILENAME __test.dart.tmp
    TEST_DART_FILENAME test.dart
)

DEFAULT(USE_OWNERS yes)
IF (USE_OWNERS)
    SET_APPEND(__filenames_ PROCESSED_OWNERS_FILE owners.list)
ENDIF ()

SET(__filename_)
FOREACH (__item_ ${__filenames_})
    IF (DEFINED __filename_)
        DEFAULT(${__filename_} "${ARCADIA_BUILD_ROOT}/${__item_}")
        FILE(WRITE "${${__filename_}}" "")
        SET(__filename_)
    ELSE ()
        SET(__filename_ "${__item_}")
    ENDIF ()
ENDFOREACH ()

FILE(WRITE "${TEST_DART_TMP_FILENAME}"
    "=============================================================\n")


# c/c++ flags, compilers, etc.


IF (CMAKE_COMPILER_IS_GNUCC)
    # some gcc builds do not print the micro-version in their --dumpversion
    EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} --version
                    OUTPUT_VARIABLE GCC_VERSION)
    STRING(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+" GCC_VERSION "${GCC_VERSION}")
    IF ((GCC_VERSION VERSION_GREATER 4.5 OR GCC_VERSION VERSION_EQUAL 4.5) AND GCC_VERSION VERSION_LESS 4.6)
        MESSAGE(FATAL_ERROR "GCC version 4.5 is not supported")
    ENDIF ()
    IF ((GCC_VERSION VERSION_GREATER 4.7.0 OR GCC_VERSION VERSION_EQUAL 4.7.0) AND GCC_VERSION VERSION_LESS 4.7.2)
        MESSAGE(FATAL_ERROR "GCC versions 4.7.0 and 4.7.1 are not supported")
    ENDIF ()

    SET(COMPILER_ID "GNU")
ELSEIF ("${ORIGINAL_CXX_COMPILER}" MATCHES "clang[^/]*$")
    SET(COMPILER_ID "CLANG")
ELSEIF ("${ORIGINAL_CXX_COMPILER}" MATCHES "icpc")
    SET(COMPILER_ID "ICC")
ELSEIF (MSVC)
    SET(COMPILER_ID "MSVC")
ELSE ()
    SET(COMPILER_ID "UNKNOWN")
ENDIF ()

IF (COMPILER_PREFIX)
    SET(CMAKE_CXX_COMPILE_OBJECT "${COMPILER_PREFIX} ${CMAKE_CXX_COMPILE_OBJECT}")
    SET(CMAKE_C_COMPILE_OBJECT "${COMPILER_PREFIX} ${CMAKE_C_COMPILE_OBJECT}")
    DEBUGMESSAGE(1, "COMPILER_PREFIX=${COMPILER_PREFIX}")
ENDIF ()

DEFAULT(USE_MIC_OPTS no)
IF (MIC_ARCH)
    IF (COMPILER_ID STREQUAL "ICC")
        ENABLE(USE_MIC_OPTS)
    ELSE()
        MESSAGE(FATAL_ERROR "USE_MIC_OPTS is valid only for icc. Won't be aplied.")
    ENDIF()
ENDIF()

IF (USE_TIME)
    SET(__time_compile_logfile_ ${ARCADIA_BUILD_ROOT}/time.compile.log)
    SET(__time_link_logfile_    ${ARCADIA_BUILD_ROOT}/time.link.log)
    DEFAULT(TIME "/usr/bin/time")
    FOREACH(__i_ CMAKE_C_COMPILE_OBJECT CMAKE_CXX_COMPILE_OBJECT)
        SET(${__i_} "${TIME} -ap -o ${__time_compile_logfile_} ${${__i_}}")
#       SET(${__i_} "echo 'file <SOURCE>' >> ${__time_compile_logfile_}; ${TIME} -ap -o ${__time_compile_logfile_} ${${__i_}}")
    ENDFOREACH ()
    FOREACH(__i_ CMAKE_C_LINK_EXECUTABLE CMAKE_CXX_LINK_EXECUTABLE CMAKE_C_CREATE_SHARED_LIBRARY CMAKE_CXX_CREATE_SHARED_LIBRARY)
        SET(${__i_} "${TIME} -ap -o ${__time_link_logfile_} ${${__i_}}")
#       SET(${__i_} "echo 'target <TARGET>' >> ${__time_link_logfile_}; ${TIME} -ap -o ${__time_link_logfile_} ${${__i_}}")
    ENDFOREACH ()
    DEBUGMESSAGE(1, "USE_TIME=${USE_TIME}")
ENDIF ()

INCLUDE(${__cmake_incdir_}/codgen.cmake)
SET(__use_perl_ ${USE_PERL})
ENABLE(USE_PERL)
INCLUDE(${__cmake_incdir_}/FindPerl2.cmake)
SET(USE_PERL ${__use_perl_})
INCLUDE(${__cmake_incdir_}/UseSwig.cmake)
INCLUDE(${__cmake_incdir_}/UsePython.cmake)
INCLUDE(${__cmake_incdir_}/UsePerl.cmake)

IF (NOT DEFINED PRINTCURPROJSTACK)
    IF ("${DEBUG_MESSAGE_LEVEL}" GREATER "0")
        SET(PRINTCURPROJSTACK yes CACHE INTERNAL "")
    ELSE ()
        SET(PRINTCURPROJSTACK no)
    ENDIF ()
ENDIF ()

DEFAULT(DICTIONARIES_ROOT ${ARCADIA_ROOT}/../dictionaries)

ADD_CUSTOM_TARGET(all_ut)
SET_PROPERTY(TARGET "all_ut" PROPERTY FOLDER "_CMakeTargets")

# Use cmake exclusion list, optional
DEFAULT(EXCLUDE_PROJECTS "")
IF (EXISTS ${ARCADIA_BUILD_ROOT}/exclude.cmake)
    FILE(STRINGS ${ARCADIA_BUILD_ROOT}/exclude.cmake EXCLUDE_PROJECTS)
ENDIF ()
DEBUGMESSAGE(1 "exclude list: ${EXCLUDE_PROJECTS}")

INCLUDE(${__cmake_incdir_}/check_formula_md5.cmake)
INCLUDE(${__cmake_incdir_}/fwd_compat.cmake)
INCLUDE(${__cmake_incdir_}/GeneratePythonProtos.cmake)
INCLUDE(${__cmake_incdir_}/test_macroses.cmake)
INCLUDE(${__cmake_incdir_}/packages.cmake)
INCLUDE(${__cmake_incdir_}/genscheeme.cmake)
INCLUDE(${__cmake_incdir_}/gazetteer.cmake)
INCLUDE(${__cmake_incdir_}/pire_inline.cmake)
INCLUDE(${__cmake_incdir_}/att_udf.cmake)
INCLUDE(${__cmake_incdir_}/debugtools.cmake)

