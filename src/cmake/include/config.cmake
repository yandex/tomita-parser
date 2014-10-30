#This file was made from config.mk
#
# OS_RELEASE
# OS_NAME
# OS_MAJOR
# HARDWARE_TYPE
#
# WIN32, SUN, BSDI, HPUX, CYGWIN, DARWIN
# ALPHA, SPARC
#

# guarding "ifdef"
IF (NOT CONFIG_MK)
SET(CONFIG_MK yes)

# avoid CMake Warning: Manually-specified variables were not used by the project: MAKE_CHECK
# this variable manually-specified in om-tool
SET(MAKE_CHECK unused)
SET(CURRENT_STLPORT_VERSION 5.1.4)

DEFAULT(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${PATH_TO_ROOT}/cmake/include)
DEFAULT(USE_PRECOMPILED yes)

IF (WIN32)
    # Windows. Set all values manually

    SET(OS_RELEASE 5.0) # TODO: Fix it
    SET(OS_NAME WIN32)
    SET(OS_MAJOR 5) # Fix it, too
    IF (CMAKE_CL_64 OR ("${CMAKE_GENERATOR}" MATCHES "Win64"))
        SET(HARDWARE_TYPE x86_64)
    ELSE ()
        SET(HARDWARE_TYPE i386)
    ENDIF ()
ELSE ()
    # Any unix-like environment
    EXECUTE_PROCESS(COMMAND hostname
    OUTPUT_VARIABLE HOSTNAME)

    EXECUTE_PROCESS(COMMAND uname -r
    COMMAND tr /[:lower:]/ /[:upper:]/
    OUTPUT_VARIABLE OS_RELEASE)

    STRING(REGEX REPLACE "\n" "" OS_RELEASE ${OS_RELEASE})

    EXECUTE_PROCESS(COMMAND uname -s
    COMMAND tr /[:lower:]/ /[:upper:]/
    OUTPUT_VARIABLE OS_NAME)

    STRING(REGEX REPLACE "\n" "" OS_NAME ${OS_NAME})

    EXECUTE_PROCESS(COMMAND uname -r
    OUTPUT_VARIABLE tmp_var)
    STRING(REGEX REPLACE "\([0-9][0-9]*\).*" "\\1" OS_MAJOR "${tmp_var}" )

    STRING(REGEX REPLACE "\n" "" OS_MAJOR ${OS_MAJOR})

    IF (NOT HARDWARE_TYPE)
        EXECUTE_PROCESS(
            COMMAND uname -m
            OUTPUT_VARIABLE HARDWARE_TYPE)
            STRING (REGEX REPLACE "[^_A-Za-z0-9]" "" HARDWARE_TYPE ${HARDWARE_TYPE}
            )
    ENDIF ()

    IF (NOT "X$ENV{SANDBOX_COMPILER_NO_WERROR}X" STREQUAL "XX")
        IF (NOT NO_WERROR_FOR_ALL)
            SET(NO_WERROR_FOR_ALL $ENV{SANDBOX_COMPILER_NO_WERROR})
        ENDIF ()
    ENDIF ()
    DEFAULT(NO_WERROR_FOR_ALL yes)

    IF ("${OS_NAME}" MATCHES "LINUX")
        DEFAULT(LINUX yes)
        DEFAULT(NOGCCSTACKCHECK yes)
    ELSEIF ("${OS_NAME}" MATCHES "SUNOS")
        DEFAULT(SUN yes)
    ELSEIF ("${OS_NAME}" MATCHES "BSDI")
        DEFAULT(BSDI yes)
    ELSEIF ("${OS_NAME}" MATCHES "HPUX")
        DEFAULT(HPUX yes)
    ELSEIF ("${OS_NAME}" MATCHES "CYGWIN_NT-5.1")
        DEFAULT(CYGWIN yes)
    ELSEIF ("${OS_NAME}" MATCHES "DARWIN")
        DEFAULT(DARWIN yes)
        DEFAULT(NOGCCSTACKCHECK yes)
    IF (${CMAKE_GENERATOR} MATCHES "Xcode")
        SET(XCODE yes)
        SET(PIC_FOR_ALL_TARGETS yes)
    ENDIF ()
    ELSEIF ("${OS_NAME}" MATCHES "FREEBSD")
        DEFAULT(FREEBSD yes)
        DEFAULT(NOGCCSTACKCHECK yes)
    ENDIF ()

    IF ("${HARDWARE_TYPE}" MATCHES "alpha")
        DEFAULT(ALPHA yes)
        DEFAULT(NO_WERROR_FOR_ALL yes)
    ELSEIF ("${HARDWARE_TYPE}" MATCHES "amd64")
        SET(HARDWARE_TYPE x86_64)
    ELSEIF ("${HARDWARE_TYPE}" MATCHES "sun4u")
        DEFAULT(SPARC yes)
    ELSEIF ("${HARDWARE_TYPE}" MATCHES "mips")
        DEFAULT(MIPS yes)
    ENDIF ()

    IF ("${HARDWARE_TYPE}" MATCHES "i386")
       SET(HARDWARE_TYPE i686)
    ENDIF ()

    SET(PROFFLAG -pg)
ENDIF ()

IF ("${HARDWARE_TYPE}" MATCHES "i686" OR "${HARDWARE_TYPE}" MATCHES "i386")
    SET(HARDWARE_ARCH 32)
ELSE ()
    SET(HARDWARE_ARCH 64)
ENDIF ()

ENDIF ()
