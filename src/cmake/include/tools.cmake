DEFAULT(MD5LIB -lmd)

SET(INVALID_TOOLS)

# typicaly this should be set in compilerinit.cmake, but if not, set it now
IF (NOT ORIGINAL_CXX_COMPILER)
    SET(ORIGINAL_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE FILEPATH "Original C++ compiler")
ENDIF ()

IF (USE_CLANG)
    SET(CLDRV "${__cmake_incdir_}/../scripts/cldriver")

    SET(AR ${CLDRV})
    SET(CMAKE_AR ${AR})

    SET(RANLIB ${CLDRV})
    SET(CMAKE_RANLIB ${RANLIB})

    SET(USE_STATIC_CPP_RUNTIME no)
ENDIF ()

IF (LINUX)
    EXECUTE_PROCESS(COMMAND uname -r
        OUTPUT_VARIABLE LINUX_KERNEL
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "\(...\).*" LINUX_KERNEL "\\1" ${LINUX_KERNEL})

    DEFAULT(PKG_CREATE rpm)
    DEFAULT(RPMBUILD rpm)
    SET(MD5LIB -lcrypt)
    SET(LIBRT -lrt)
    SET(LIBRESOLV -lresolv)
    DEFAULT(APACHE            /usr/include/apache-1.3)
    DEFAULT(APACHE_INCLUDE    /usr/include/apache-1.3)
ELSEIF (SUN)
    #DEFAULT(PUBROOM /opt/arcadia/room)
    DEFAULT(PKG_CREATE pkgmk)
    DEFAULT(THREADLIB -lpthread)
    IF (SUN)
        SET(INSTALL     /usr/ucb/install)
        DEFAULT(AWK   /usr/bin/awk)
        DEFAULT(TAIL  /usr/xpg4/bin/tail)
        DEFAULT(BISON /usr/local/bin/bison)
    ENDIF ()
ELSEIF (CYGWIN)
    DEFAULT(THREADLIB -lpthread)
ELSEIF (DARWIN)
    DEFAULT(THREADLIB -lpthread)
    SET(LIBRESOLV -lresolv)
ELSEIF (WIN32)
    # TODO: Add necessary defs
    DEFAULT(RM del)
    DEFAULT(RM_FORCEFLAG "/F/Q")
    DEFAULT(RM_RECURFLAG "/S")
    DEFAULT(CAT type)
    DEFAULT(MV move)
    DEFAULT(MV_FORCEFLAG "/Y")
ELSEIF (FREEBSD)
    EXECUTE_PROCESS(COMMAND uname -r
        OUTPUT_VARIABLE FREEBSD_VER
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "^([0-9]+).*" "\\1" FREEBSD_VER ${FREEBSD_VER})

    EXECUTE_PROCESS(COMMAND uname -r
        OUTPUT_VARIABLE FREEBSD_VER_MINOR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" FREEBSD_VER_MINOR ${FREEBSD_VER_MINOR})

    DEFAULT(PKG_CREATE pkg_create)
    SET(LDFLAGS "${LDFLAGS} -Wl,-E")
    DEFAULT(VALGRIND_DIR /usr/local/valgrind/)
    DEFAULT(APACHE /large/old/usr/place/apache-nl/apache_1.3.29rusPL30.19/src)
    IF (DEFINED FREEBSD_VER AND "${FREEBSD_VER}" MATCHES "4")
        DEFAULT(GCOV gcov34)
    ENDIF ()
    SET(CTAGS_PRG exctags)
ENDIF ()

IF (SUN)
    SET(SONAME -h)
    SET(TSORT_FLAGS)
ELSEIF (CYGWIN)
    SET(SONAME -h)
    SET(TSORT_FLAGS)
ELSE () #linux/freebsd
    SET(SONAME -soname)
    SET(TSORT_FLAGS -q)
ENDIF ()

DEFAULT(APACHE_INCLUDE ${APACHE}/include)

DEFAULT(PERL      perl)

IF (PERL)
    IF (NOT PERL_VERSION)
        EXECUTE_PROCESS(COMMAND ${PERL} -V:version
            OUTPUT_VARIABLE PERL_VERSION
            ERROR_VARIABLE PERL_VERSION_ERR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        IF (PERL_VERSION)
            STRING(REPLACE "\n" " " PERL_VERSION ${PERL_VERSION})
            STRING(REGEX MATCH "version='[0-9\\.]*'" PERL_VERSION ${PERL_VERSION})
            STRING(REPLACE "version=" "" PERL_VERSION ${PERL_VERSION})
            STRING(REPLACE "'" "" PERL_VERSION ${PERL_VERSION})
            STRING(REPLACE "." ";" PERL_VERSION ${PERL_VERSION})
            SET(__perl_ver_ 0)
            FOREACH(__item_ ${PERL_VERSION})
                MATH(EXPR __perl_ver_ "${__perl_ver_}*100+${__item_}")
            ENDFOREACH ()
            SET(PERL_VERSION ${__perl_ver_})
        ELSE ()
            SET(PERL_VERSION NOTFOUND)
        ENDIF ()
        DEBUGMESSAGE(1 "PERL_VERSION is [${PERL_VERSION}]")
    ENDIF ()
ENDIF ()

DEFAULT(BISON     bison)
DEFAULT(GCOV      gcov)
DEFAULT(GCOV_OPTIONS  --long-file-names)
DEFAULT(RM rm)
DEFAULT(RM_FORCEFLAG "-f")
DEFAULT(RM_RECURFLAG "-r")
DEFAULT(CAT cat)
DEFAULT(MV mv)
DEFAULT(MV_FORCEFLAG "-f")
DEFAULT(CTAGS_PRG ctags)

DEFAULT(AR ar)
DEFAULT(RANLIB ranlib)

DEFAULT(PYTHON python)
IF (NOT PYTHON_VERSION)
    IF (WIN32)
        SET(PYTHON_VERSION 2.5)
    ELSE ()
        EXECUTE_PROCESS(COMMAND ${PYTHON} -V
        OUTPUT_VARIABLE PYTHON_VERSION
        ERROR_VARIABLE PYTHON_VERSION_ERR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        )
        SET(PYTHON_VERSION "${PYTHON_VERSION}${PYTHON_VERSION_ERR}")
        IF (PYTHON_VERSION)
            STRING(REGEX REPLACE "^[^ ]* \([^.]*.[^.]*\).*" "\\1" PYTHON_VERSION "${PYTHON_VERSION}")
        ELSE ()
            MESSAGE(STATUS "\${PYTHON_VERSION} is empty. Please check if \${PYTHON} variable is set properly")
        ENDIF ()
    ENDIF ()
ENDIF ()
IF (WIN32)
	DEFAULT(PYTHON_INCLUDE C:/Python/include)
ELSEIF (LINUX OR DARWIN)
	DEFAULT(PYTHON_INCLUDE /usr/include/python${PYTHON_VERSION})
ELSE () # sun/freebsd
	DEFAULT(PYTHON_INCLUDE /usr/local/include/python${PYTHON_VERSION})
ENDIF ()

IF (WIN32)
        DEFAULT(PYTHON_EXECUTABLE C:/Python/${PYTHON}.exe)
ELSE ()
        EXECUTE_PROCESS(
            COMMAND which ${PYTHON}
            OUTPUT_VARIABLE PYTHON_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
ENDIF ()

IF (NOT CCVERS)
    IF (WIN32)
        SET(CCVERS 0)
    ELSE ()
        EXECUTE_PROCESS(COMMAND ${ORIGINAL_CXX_COMPILER} -dumpversion
        COMMAND awk -F. "{print $1*10000+$2*100+$3}"
        OUTPUT_VARIABLE CCVERS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF ()
ENDIF ()

INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(__PATHSCALE__ "" IS_PATHSCALE)

INCLUDE(TestCXXAcceptsFlag)

IF (BUILD_FOR_PRODUCTION)
    # Build production binaries with preset CPU features.
    MESSAGE(STATUS "BUILD_FOR_PRODUCTION enabled")
    SET(IS_MNOSSE_SUPPORTED yes)
    SET(IS_MSSE_SUPPORTED yes)
    SET(IS_MSSE2_SUPPORTED yes)
    SET(IS_MSSE3_SUPPORTED yes)
    SET(IS_MSSSE3_SUPPORTED no)
    SET(IS_MPOPCNT_SUPPORTED yes)
ELSE ()
    IF (IS_PATHSCALE)
        # pathscale rejects these sse flags without a proper -march
        CHECK_CXX_ACCEPTS_FLAG("-mpopcnt -march=core" IS_MPOPCNT_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-mssse3 -march=core" IS_MSSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse3 -march=core" IS_MSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse2 -march=core" IS_MSSE2_SUPPORTED)
    ELSE ()
        CHECK_CXX_ACCEPTS_FLAG("-mpopcnt" IS_MPOPCNT_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-mssse3" IS_MSSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse3" IS_MSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse2" IS_MSSE2_SUPPORTED)
    ENDIF ()
    CHECK_CXX_ACCEPTS_FLAG("-msse" IS_MSSE_SUPPORTED)
    CHECK_CXX_ACCEPTS_FLAG("-mno-sse" IS_MNOSSE_SUPPORTED)
ENDIF ()

#
# For USE_STATIC_CPP_RUNTIME
#
IF (NOT WIN32)
    SET(__OPTS)
    IF ("${HARDWARE_TYPE}" MATCHES "x86_64")
        SET(__OPTS -m64)
    ENDIF ()
    IF (NOT DEFINED GCC_FULLLIBDIR)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} ${__OPTS} "-print-file-name=libsupc++.a"
            OUTPUT_VARIABLE GCC_FULLLIBDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLLIBDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLLIBDIR "${GCC_FULLLIBDIR}" PATH)
        ENDIF ()
    ENDIF ()

    IF (NOT DEFINED GCC_FULLLIBGCCDIR)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} ${__OPTS} "-print-file-name=libgcc.a"
            OUTPUT_VARIABLE GCC_FULLLIBGCCDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLLIBGCCDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLLIBGCCDIR "${GCC_FULLLIBGCCDIR}" PATH)
        ENDIF ()
    ENDIF ()

    IF (NOT DEFINED GCC_FULLCRTOBJDIR)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} ${__OPTS} "-print-file-name=crtbegin.o"
            OUTPUT_VARIABLE GCC_FULLCRTOBJDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLCRTOBJDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLCRTOBJDIR "${GCC_FULLCRTOBJDIR}" PATH)
        ELSE ()
            SET(GCC_FULLCRTOBJDIR)
        ENDIF ()
    ENDIF ()

    IF (NOT DEFINED GCC_FULLCRTOBJDIR)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} ${__OPTS} "-print-file-name=crt3.o"
            OUTPUT_VARIABLE GCC_FULLCRTOBJDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLCRTOBJDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLCRTOBJDIR "${GCC_FULLCRTOBJDIR}" PATH)
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (NOT GCC_FULLLIBDIR AND NOT WIN32)
    IF (LINUX)
        DEFAULT(GCC_LIBDIR /usr/lib)
    ELSEIF (NOT GCC_LIBDIR AND FREEBSD)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} -v
            ERROR_VARIABLE GCC_LIBDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        STRING(REGEX MATCH "--libdir=[^ ]* " GCC_LIBDIR "${GCC_LIBDIR}")
        STRING(REGEX REPLACE "--libdir=" "" GCC_LIBDIR "${GCC_LIBDIR}")
        STRING(REGEX REPLACE " " "" GCC_LIBDIR "${GCC_LIBDIR}")
        STRING(REGEX REPLACE "\n" "" GCC_LIBDIR "${GCC_LIBDIR}")
    ENDIF ()

    IF (NOT DEFINED GCC_TARGET)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} -dumpmachine
            OUTPUT_VARIABLE GCC_TARGET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF ()

    IF (NOT GCC_VERSION)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} -dumpversion
            OUTPUT_VARIABLE GCC_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF ()

    SET(GCC_FULLLIBDIR    ${GCC_LIBDIR}/gcc/${GCC_TARGET}/${GCC_VERSION})
    SET(GCC_FULLCRTOBJDIR ${GCC_FULLLIBDIR})
    SET(GCC_FULLLIBGCCDIR ${GCC_FULLLIBDIR})

    DEBUGMESSAGE(1 "Can't get GCC_FULLLIBDIR from -print-file-name=libsupc++.a. Euristic method gave path[${GCC_FULLLIBDIR}]")
ENDIF ()

IF (NOT DEFINED NATIVE_INCLUDE_PATH)
    IF (NOT WIN32)
        EXECUTE_PROCESS(
            COMMAND ${ORIGINAL_CXX_COMPILER} -E -v -x c++ /dev/null
            OUTPUT_QUIET
            ERROR_VARIABLE GCC_OUT
            ERROR_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )

        STRING(REGEX REPLACE "\n" " " GCC_OUT "${GCC_OUT}")
        STRING(REGEX REPLACE ".*search starts here:[ \t]*" "" GCC_OUT "${GCC_OUT}")
        STRING(REGEX REPLACE "End of search list.*" "" GCC_OUT "${GCC_OUT}")
        SEPARATE_ARGUMENTS(GCC_OUT)

        FOREACH (__item__ ${GCC_OUT})
            IF (NOT DEFINED __ret__ AND EXISTS "${__item__}/string")
                SET(__ret__ "${__item__}")
            ENDIF ()
        ENDFOREACH ()

        IF (NOT DEFINED __ret__)
            MESSAGE(SEND_ERROR "tools.cmake: Can't get gcc native include path")
        ENDIF ()

        SET(NATIVE_INCLUDE_PATH "${__ret__}")
    ENDIF ()
ENDIF ()

IF (DEFINED NATIVE_INCLUDE_PATH)
    SET(COMMON_CXXFLAGS "-DNATIVE_INCLUDE_PATH=\"${NATIVE_INCLUDE_PATH}\"")
ENDIF ()

DEBUGMESSAGE(1 "GCC_FULLLIBDIR[${GCC_FULLLIBDIR}]")

IF (GCC_FULLLIBDIR)
    IF (EXISTS "${GCC_FULLLIBDIR}/libsupc++.a")
#       SET(ST_OBJADDE -Wl,-Bstatic)
        SET_APPEND(ST_OBJADDE ${GCC_FULLLIBDIR}/libsupc++.a)
        SET_APPEND(ST_OBJADDE ${GCC_FULLLIBGCCDIR}/libgcc.a)

        IF (EXISTS ${GCC_FULLLIBGCCDIR}/libgcc_eh.a)
            SET_APPEND(ST_OBJADDE ${GCC_FULLLIBGCCDIR}/libgcc_eh.a)
        ENDIF ()

        IF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
            SET_APPEND(ST_OBJADDE -lc_p)
        ELSE ()
            SET_APPEND(ST_OBJADDE -lc)
        ENDIF ()

        SET_APPEND(ST_OBJADDE
            ${THREADLIB}
        )

        IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0)
            SET_APPEND(ST_OBJADDE -no-pie)
        ENDIF()

        IF(NOT USE_ARCADIA_LIBM)
            SET_APPEND(ST_OBJADDE -lm)
        ENDIF ()

        IF (EXISTS ${GCC_FULLCRTOBJDIR}/crtend.o)
            SET_APPEND(ST_OBJADDE ${GCC_FULLCRTOBJDIR}/crtend.o)
        ENDIF ()

    #   SET_APPEND(ST_OBJADDE -Wl,-Bdynamic)
        IF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
            SET(CRT1_NAME gcrt1.o)
        ELSE ()
            SET(CRT1_NAME crt1.o)
        ENDIF ()

        DEFAULT(CRT1_PATH /usr/lib)

        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "x86_64")
            SET(CRT1_PATH /usr/lib/x86_64-linux-gnu)
        ENDIF ()

        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "x86_64")
            SET(CRT1_PATH /usr/lib64)
        ENDIF ()

        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "i686")
            SET(CRT1_PATH /usr/lib/i386-linux-gnu)
        ENDIF ()

        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME})
            DEBUGMESSAGE(SEND_ERROR "cannot find ${CRT1_NAME} at /usr/lib and /usr/lib64 and ${CRT1_PATH}. Set CRT1_PATH in your local configuration. HARDWARE_TYPE: ${HARDWARE_TYPE}")
        ENDIF ()

        IF (EXISTS ${CRT1_PATH}/crtn.o)
            SET_APPEND(ST_OBJADDE ${CRT1_PATH}/crtn.o)
        ENDIF ()

        SET(ST_LDFLAGS
            -nostdlib
            ${CRT1_PATH}/${CRT1_NAME}
        )

        IF (EXISTS ${CRT1_PATH}/crti.o)
            SET_APPEND(ST_LDFLAGS ${CRT1_PATH}/crti.o)
        ENDIF ()

        IF (EXISTS ${GCC_FULLCRTOBJDIR}/crtbegin.o)
            SET_APPEND(ST_LDFLAGS ${GCC_FULLCRTOBJDIR}/crtbegin.o)
        ENDIF ()

        IF (EXISTS ${GCC_FULLCRTOBJDIR}/crt3.o)
            SET_APPEND(ST_LDFLAGS ${GCC_FULLCRTOBJDIR}/crt3.o)
        ENDIF ()
    ELSE ()
        IF (USE_STATIC_CPP_RUNTIME)
            MESSAGE(SEND_ERROR "tools.cmake: ${GCC_FULLLIBDIR}/libsupc++.a doesn't exist (with positive USE_STATIC_CPP_RUNTIME)")
        ENDIF ()
    ENDIF ()
ELSE ()
    IF (NOT WIN32)
        IF (USE_STATIC_CPP_RUNTIME)
            MESSAGE(SEND_ERROR "tools.cmake: Can't get GCC_FULLLIBDIR (with positive USE_STATIC_CPP_RUNTIME)")
        ENDIF ()
    ENDIF ()
ENDIF ()

SEPARATE_ARGUMENTS_SPACE(ST_LDFLAGS)
SEPARATE_ARGUMENTS_SPACE(ST_OBJADDE)
#
# End of USE_STATIC_CPP_RUNTIME)
#

DEFAULT(AWK       awk)
DEFAULT(TAIL      tail)
DEFAULT(ECHODIR       echo)
DEFAULT(ECHO      echo)

IF (NOT "5" GREATER "0${FREEBSD_VER}")
    DEFAULT(THREADLIB     -lthr)
ELSEIF (LINUX)
    DEFAULT(THREADLIB     -lpthread)
ELSEIF (WIN32)
    SET(THREADLIB)
ELSE ()
    DEFAULT(THREADLIB     -pthread)
ENDIF ()

DEFAULT(TOUCH     touch)
DEFAULT(DBC2CPP       "echo --- Define dependency to dbc2cpp explicitly in your project's CMakeLists.txt ---")
#DEFAULT(DBC2CPP      ${ARCADIA_BUILD_ROOT}/yweb/webutil/dbc2cpp/dbc2cpp) # used only in check/mk_cvs_tag.sh

DEFAULT(INSTALLFLAGS  "-c -m 755")
DEFAULT(INSTFLAGS_755 "-c -m 755")
DEFAULT(INSTFLAGS_644 "-c -m 644")



#PEERDIR    ?=  # peer dir for current

IF (NOT IGNORE_INVALID_TOOLS AND INVALID_TOOLS)
    MESSAGE(FATAL_ERROR "tools.cmake, fatal: one or more tools are missing or have invalid version (${INVALID_TOOLS}). Build failed")
ENDIF ()
