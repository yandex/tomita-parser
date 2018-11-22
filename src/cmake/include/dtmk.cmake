MACRO(EXEC_DTMK)

#This file was made from dt.mk

# ============================================================================ #
# Here are global build variables (you can override them in your local.cmake)

# End of global build variables list
# ============================================================================ #

# Creating symlink to sources
IF (PLATFORM_SUPPORTS_SYMLINKS AND NOT EXISTS "${BINDIR}/SRC" AND NOT NO_SRCLINK)
    DEBUGMESSAGE(3 "Creating symlink ${BINDIR}/SRC to ${CURDIR}")
    EXECUTE_PROCESS(
        COMMAND ln -sf ${CURDIR} SRC
        WORKING_DIRECTORY ${BINDIR}
        OUTPUT_QUIET
    )
ENDIF ()

# =========================================================================== #

SET(OBJDIRPREFIX    "/OBJDIRPREFIX-is-deprecated")
SET(OBJDIR_ARCADIA  "/OBJDIR_ARCADIA-is-deprecated")
SET(CANONICALOBJDIR "/CANONICALOBJDIR-is-deprecated")

DEFAULT(PEERLIBS "") # peer libs for current target
DEFAULT(PEERLDADD "")
DEFAULT(OBJADD "")
DEFAULT(MPROFLIB "") #note

# TODO: translate it when building unittests
#.if defined(LIB) && make(unittests) && $(LIB) != "yutil"
#PEERDIR += util
#.endif

IF (PROG AND NOT CREATEPROG)
    SET(CREATEPROG ${PROG})
ENDIF ()

SET(DTMK_CFLAGS "${COMMON_CFLAGS}")
SET(DTMK_CXXFLAGS "${COMMON_CXXFLAGS}")

SET_APPEND(DTMK_I ${CMAKE_CURRENT_BINARY_DIR})

SET(LINK_WLBSTATIC_BEGIN)
SET(LINK_WLBSTATIC_END)
IF (LINK_STATIC_LIBS)
    SET(LINK_WLBSTATIC_BEGIN "-Wl,-Bstatic")
    SET(LINK_WLBSTATIC_END "-Wl,-Bdynamic")
ENDIF ()

IF (NOT WIN32)
    DEFAULT(USE_STDCXX11 yes)
    IF (USE_STDCXX11)
        SET_APPEND(DTMK_CXXFLAGS -std=c++11 -D__LONG_LONG_SUPPORTED)
    ENDIF ()
ENDIF ()

IF (WIN32)
    IF (NOT NOSSE)
        IF ("${HARDWARE_TYPE}" MATCHES "x86_64")
            SET_APPEND(DTMK_CFLAGS -DSSE_ENABLED=1 -DSSE2_ENABLED=1 -DSSE3_ENABLED=1)
        ELSE ()
            IF (IS_MSSE2_SUPPORTED)
                SET_APPEND(DTMK_CFLAGS /arch:SSE2 -DSSE_ENABLED=1 -DSSE2_ENABLED=1)
            ELSEIF (IS_MSSE_SUPPORTED)
                SET_APPEND(DTMK_CFLAGS /arch:SSE -DSSE_ENABLED=1)
            ENDIF ()
        ENDIF ()
    ENDIF ()
ELSE ()
    SET_APPEND(DTMK_CFLAGS -fexceptions)
    IF (NOSSE)
        IF (IS_MNOSSE_SUPPORTED)
            SET_APPEND(DTMK_CFLAGS -mno-sse)
        ENDIF ()
    ELSE ()
        IF (IS_MSSE_SUPPORTED)
            SET_APPEND(DTMK_CFLAGS -msse -DSSE_ENABLED=1)
        ENDIF ()

        IF (IS_MSSE2_SUPPORTED)
            SET_APPEND(DTMK_CFLAGS -msse2 -DSSE2_ENABLED=1)
        ENDIF ()

        IF (IS_MSSE3_SUPPORTED)
            SET_APPEND(DTMK_CFLAGS -msse3 -DSSE3_ENABLED=1)
        ENDIF ()

        IF (IS_MSSSE3_SUPPORTED)
            SET_APPEND(DTMK_CFLAGS -mssse3 -DSSSE3_ENABLED=1)
        ENDIF ()
    ENDIF ()
ENDIF ()


# c-flags-always
IF (WIN32)
    SET(CMAKE_CONFIGURATION_TYPES Debug Release)

    # _WIN32_WINNT values taken from
    # http://msdn.microsoft.com/en-us/library/aa383745%28v=VS.85%29.aspx

    IF (CMAKE_SYSTEM_VERSION MATCHES "5.2")
        # Windows Server 2003, assume SP2
        DEFAULT(NTFLAG "_WIN32_WINNT=0x0502")
    ENDIF ()
    IF (CMAKE_SYSTEM_VERSION MATCHES "6.0")
        # Windows Vista, assume SP1,
        # or Windows Server 2008
        DEFAULT(NTFLAG "_WIN32_WINNT=0x0600")
    ENDIF ()
    IF (CMAKE_SYSTEM_VERSION MATCHES "6.1")
        # Windows 7
        DEFAULT(NTFLAG "_WIN32_WINNT=0x0601")
    ENDIF ()
    IF (CMAKE_SYSTEM_VERSION MATCHES "6.2")
        # Windows 8.1
        DEFAULT(NTFLAG "_WIN32_WINNT=0x0603")
    ENDIF ()

    # Default to XP SP3.
    DEFAULT(NTFLAG "_WIN32_WINNT=0x0501")

    SET(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /DSTRICT /D_MBCS /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /D_USE_MATH_DEFINES /D__STDC_CONSTANT_MACROS /D__STDC_FORMAT_MACROS /D${NTFLAG} /Zm1000 /GR /nologo /bigobj /Zi /FD /FC /EHsc /nologo /errorReport:prompt")
    IF (NOT (("${CMAKE_GENERATOR}" MATCHES "Makefiles") AND "${CMAKE_CL_64}"))
        SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /c)
    ENDIF ()

    DEFAULT(WFLAG "/W4 /w34018 /w34265 /w34296 /w34431 /wd4267 /wd4244") #/w34640 /w34365
    IF (NO_COMPILER_WARNINGS)
        SET(WFLAG "/W0")
    ENDIF ()
    SET(CMAKE_CXX_FLAGS_DEBUG   "/MTd /Ob0 /Od /D_DEBUG  /RTC1 ${WFLAG} ${CFLAGS_DEBUG}")
    SET(CMAKE_CXX_FLAGS_RELEASE "/MT /DNDEBUG /GF /Gy ${WFLAG} ${CFLAGS_RELEASE}")
    SET(OPTIMIZE "/Ox /Oi /Ob2")

    SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

    # Add "/DEBUG" for linker in Release build (i.e., Release = RelWithDebInfo)
    FOREACH(__item_ CMAKE_EXE_LINKER_FLAGS_RELEASE CMAKE_SHARED_LINKER_FLAGS_RELEASE)
        IF (NOT "${${__item_}}" MATCHES "/DEBUG")
            SET(${__item_} "${${__item_}} /DEBUG")
        ENDIF ()
    ENDFOREACH ()

ELSE ()
    # No configuration types exist in make-environment
    # SET(CMAKE_CONFIGURATION_TYPES Debug Release)

    IF (NOT NO_DEBUGINFO)
        SET(__debug_info_flag_ "-g")
    ELSE ()
        SET(__debug_info_flag_ "")
    ENDIF ()

    IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0)
        SET(__pie_gcc_flag_ "-fno-pie")
    ELSE()
        SET(__pie_gcc_flag_ "")
    ENDIF()

    SET(CMAKE_C_FLAGS            "-pipe ${__debug_info_flag_} -Wall ${__pie_gcc_flag_} -W -Wno-parentheses -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DGNU")
    SET(CMAKE_CXX_FLAGS          "${CMAKE_C_FLAGS} -Woverloaded-virtual")

    SET(CMAKE_CXX_FLAGS_RELEASE  "-O2 -DNDEBUG ${CFLAGS_RELEASE}")
    IF (NOT IS_PATHSCALE AND NOT CMAKE_CXX_COMPILER MATCHES ".*clang.*")
        IF (NOT NOGCCSTACKCHECK)
            SET(CMAKE_CXX_FLAGS_DEBUG    "-fstack-check -fstack-protector ${CFLAGS_DEBUG}")
        ELSE ()
            SET(CMAKE_CXX_FLAGS_DEBUG    "-fstack-protector ${CFLAGS_DEBUG}")
        ENDIF ()
    ENDIF ()
    SET(CMAKE_CXX_FLAGS_VALGRIND "${CFLAGS_VALGRIND}")
    SET(CMAKE_CXX_FLAGS_PROFILE  "-O2 -DNDEBUG -pg ${CFLAGS_PROFILE}")
    SET(CMAKE_CXX_FLAGS_COVERAGE "-fprofile-arcs -ftest-coverage ${CFLAGS_COVERAGE}")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -UNDEBUG")

    SET(CMAKE_C_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE}")
    SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    SET(CMAKE_C_FLAGS_DEBUG      "${CMAKE_CXX_FLAGS_DEBUG}")
    SET(CMAKE_C_FLAGS_VALGRIND   "${CMAKE_CXX_FLAGS_VALGRIND}")
    SET(CMAKE_C_FLAGS_PROFILE    "${CMAKE_CXX_FLAGS_PROFILE}")
    SET(CMAKE_C_FLAGS_COVERAGE   "${CMAKE_CXX_FLAGS_COVERAGE}")

    # SET(CMAKE_EXE_LINKER_FLAGS_PROFILE    "-static -static-libgcc")
    # SET(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-static -static-libgcc")
    IF (NO_RDYNAMIC)
        SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
        SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    ENDIF ()

    # Issue: YMAKE-169 (workaround for FreeBSD-10)
    # Issue: https://github.com/yandex/tomita-parser/issues/48 (workaround for Ubuntu 16.04 and Fedora 24)
    IF (CMAKE_SYSTEM MATCHES "FreeBSD-10.0" OR ${GCC_VERSION} VERSION_GREATER "5.3")
        SET_APPEND(CMAKE_CXX_FLAGS -D _GLIBCXX_USE_C99_FP_MACROS_DYNAMIC)
    ENDIF ()

    # These instructions are important when you cross-compile your code,
    #   for example using distcc (from 32bit to 64bit and vice versa)
    IF (NOT SUN AND NOT "${HARDWARE_TYPE}" MATCHES "mips" AND NOT "${HARDWARE_TYPE}" MATCHES "ia64" AND NOT "${HARDWARE_TYPE}" MATCHES "x86_64")
        SET_APPEND(DTMK_CFLAGS -m32)
    ELSEIF ("${HARDWARE_TYPE}" MATCHES "ia64" OR "${HARDWARE_TYPE}" MATCHES "x86_64")
        SET_APPEND(DTMK_CFLAGS -m64)
    ENDIF ()
ENDIF ()

IF (${CCVERS} GREATER 30000)
    SET_APPEND(DTMK_CXXFLAGS -Wno-deprecated)
    #SET_APPEND(MDFLAGS -MP) # no need for gcc depends
ENDIF ()

IF (NOT IS_PATHSCALE)
    IF (NOT 30400 GREATER ${CCVERS})
        SET_APPEND(DTMK_CXXFLAGS -Wno-invalid-offsetof)
    ENDIF ()
ENDIF ()

IF (${CCVERS} GREATER 40100 AND ${CCVERS} LESS 40200)
    SET_APPEND(DTMK_CXXFLAGS -Wno-strict-aliasing)
ENDIF ()

IF (${CCVERS} GREATER 40400 AND ${CCVERS} LESS 40500)
    # Suppress a spurious warning: util/generic/yexception.h:37: warning: dereferencing pointer '<anonymous>' does break strict-aliasing rules
    # It appears to be an instance of the following gcc 4.4 only bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=42488
    SET_APPEND(DTMK_CXXFLAGS -Wno-strict-aliasing)
ENDIF ()

IF (NOT NO_COMPILER_WARNINGS AND NOT NO_WERROR_FOR_ALL)
    IF (WERROR OR WERROR_FOR_ALL)
        IF (WIN32)
            SET_APPEND(DTMK_CFLAGS "-WX")
        ELSE ()
            SET_APPEND(DTMK_CFLAGS "-Werror")
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (NOT WIN32)
    IF (NO_COMPILER_WARNINGS)
        SET_APPEND(DTMK_CFLAGS "-w")
    ENDIF ()
ENDIF ()

SET_APPEND(DTMK_CFLAGS "${RKEYS}")

IF (NOT OPTIMIZE)
    IF (WIN32)
        SET_APPEND(OPTIMIZE /O2 /Ob2 /Ot)
    ELSE ()
        IF (NOT SUN AND NOT "${HARDWARE_TYPE}" MATCHES "ia64" AND NOT "${HARDWARE_TYPE}" MATCHES "x86_64")
            IF (${CCVERS} GREATER 30400)
                SET_APPEND(OPTIMIZE -march=pentiumpro -mtune=pentiumpro)
            ELSE ()
                SET_APPEND(OPTIMIZE -march=pentiumpro -mcpu=pentiumpro)
            ENDIF ()
        ENDIF ()
        IF (${CCVERS} LESS 40000)
            SET(OPTIMIZE ${OPTIMIZE} -O3 -DNDEBUG) #-finline-limit=40)
            IF (SPARC)
                SET_APPEND(OPTIMIZE -mcpu=ultrasparc)
            ENDIF ()
        ELSE ()
            SET_APPEND(OPTIMIZE -O2 -DNDEBUG)
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (NO_OPTIMIZE)
    IF (WIN32)
        SET(OPTIMIZE /Od)
    ELSE ()
        SET(OPTIMIZE -O0)
    ENDIF ()
ENDIF ()

IF (NOT DEFINED NOOPTIMIZE)
    IF (WIN32)
        SET_APPEND(NOOPTIMIZE /Od)
    ELSE ()
        SET_APPEND(NOOPTIMIZE -O0)
    ENDIF ()
ENDIF ()

IF (SAVE_TEMPS AND NOT WIN32)
    SET_APPEND(DTMK_CFLAGS -save-temps)
ENDIF ()

# =================================
# define Coverage and Profile build types

STRING(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)

IF ("${CMAKE_BUILD_TYPE}" MATCHES "Coverage" OR MAKE_COVERAGE)
    SET_APPEND(OBJADDE -lgcov)
ENDIF ()

IF (DEFINED USEMPROF OR DEFINED USE_MPROF)
    IF (DEFINED SUN)
    ELSEIF (DEFINED LINUX)
        SET_APPEND(MPROFLIB -ldmalloc)
    ELSE ()
        SET_APPEND(MPROFLIB -L/usr/local/lib -lc_mp)
        SET_APPEND(DTMK_D -DUSE_MPROF)
    ENDIF ()
ENDIF ()

IF (USE_LIBBIND AND NOT NO_LIBBIND)
    IF (FREEBSD_VER)
        DEFAULT(BINDDIR /usr/local/bind-f4)
    ENDIF ()
    IF (DEFINED BINDDIR AND NOT BINDDIR)
        SET(BINDDIR)
    ENDIF ()
    IF (BINDDIR)
        IF (NOT EXISTS ${BINDDIR}/include/bind/resolv.h)
            MESSAGE("dtmk.cmake warning: bind not found at ${BINDDIR}, use ENABLE(NO_LIBBIND) in local.cmake if libbind is not needed")
        ELSE ()
            SET_APPEND(OBJADDE ${LINK_WLBSTATIC_BEGIN} -L${BINDDIR}/lib -lbind_r ${LINK_WLBSTATIC_END})
            SET_APPEND(DTMK_D -DBIND_LIB)
            SET_APPEND(DTMK_I ${BINDDIR}/include/bind)
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (USE_LIBBIND AND NO_LIBBIND)
    MESSAGE(STATUS "dtmk.cmake warning: NO_LIBBIND and USE_LIBBIND are positive. LIBBIND functionality is disabled since now. Please fix your project (${CURDIR})")
ENDIF ()

IF ("${CMAKE_BUILD_TYPE}" STREQUAL "valgrind" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Valgrind")
    SET (WITH_VALGRIND 1)
ENDIF ()

IF (WITH_VALGRIND)
    SET_APPEND(DTMK_D -DWITH_VALGRIND=1)
    ENABLE(NO_WERROR_FOR_ALL)
    IF (EXISTS /usr/include/valgrind/valgrind.h AND EXISTS /usr/include/valgrind/memcheck.h)
        SET_APPEND(DTMK_D -DHAVE_VALGRIND=1)
    ENDIF ()
ENDIF ()

IF (USEMEMGUARD OR USE_MEMGUARD)
    IF (SUN)
        SET_APPEND(OBJADDE /usr/local/lib/libefence.a)
    ELSE ()
        SET_APPEND(OBJADDE ${LINK_WLBSTATIC_BEGIN} -L/usr/local/lib -lefence ${LINK_WLBSTATIC_END})
    ENDIF ()
ENDIF ()

IF (NOT NOUTIL)
    DEBUGMESSAGE(3 "Appending util to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(util)
ENDIF ()

IF (USE_B_ALLOCATOR)
    IF (NOT WIN32 AND NOT WITH_VALGRIND)
        SET_APPEND(DTMK_CFLAGS -DUSE_B_ALLOCATOR)
    ELSE ()
        SET(USE_B_ALLOCATOR no)
    ENDIF ()
ENDIF ()

SET(__allocator "")

IF (NOT "X${PROG}${CREATEPROG}X" STREQUAL "XX")
    # default
    SET(__allocator "system")

    IF (NOT WITH_VALGRIND)
        IF (USE_FAKE_ALLOCATOR)
            SET(__allocator "")
        ENDIF ()

        IF (USE_B_ALLOCATOR)
            SET(USE_GOOGLE_ALLOCATOR no)
            SET(USE_J_ALLOCATOR no)
            SET(USE_LOCKLESS_ALLOCATOR no)
            SET(USE_LF_ALLOCATOR no)
            SET(__allocator "b")
        ENDIF ()

        IF (USE_LF_ALLOCATOR)
            SET(USE_GOOGLE_ALLOCATOR no)
            SET(USE_J_ALLOCATOR no)
            SET(USE_LOCKLESS_ALLOCATOR no)
            SET(USE_B_ALLOCATOR no)
            SET(__allocator "lf")
        ENDIF ()

        IF (USE_J_ALLOCATOR AND NOT WIN32)
            SET(USE_GOOGLE_ALLOCATOR no)
            SET(USE_LOCKLESS_ALLOCATOR no)
            SET(USE_B_ALLOCATOR no)
            SET(__allocator "j")
        ENDIF ()

        IF (USE_GOOGLE_ALLOCATOR)
            SET(USE_LOCKLESS_ALLOCATOR no)
            SET(USE_B_ALLOCATOR no)
            SET(__allocator "google")
        ENDIF ()

        IF (USE_LOCKLESS_ALLOCATOR)
            SET(USE_B_ALLOCATOR no)
            SET(__allocator "lockless")
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (__allocator STREQUAL "")
    # no allocator (not a program)
ELSEIF (__allocator STREQUAL "system")
    DEBUGMESSAGE(3 "Appending library/malloc/system to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(library/malloc/system)
ELSEIF (__allocator STREQUAL "b")
    DEBUGMESSAGE(3 "Appending library/balloc to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(library/balloc)
ELSEIF (__allocator STREQUAL "lf")
    DEBUGMESSAGE(3 "Appending library/lfalloc to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(library/lfalloc)
ELSEIF (__allocator STREQUAL "j")
    DEBUGMESSAGE(3 "Appending library/malloc/jemalloc to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(library/malloc/jemalloc)
ELSEIF (__allocator STREQUAL "google")
    DEBUGMESSAGE(3 "Appending library/malloc/galloc to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(library/malloc/galloc)
ELSEIF (__allocator STREQUAL "lockless")
    DEBUGMESSAGE(3 "Appending library/malloc/lockless to PEERDIR (in ${CMAKE_CURRENT_SOURCE_DIR})")
    PEERDIR(library/malloc/lockless)
ELSE ()
    MESSAGE(SEND_ERROR "internal error: unknown allocator: ${__allocator}")
ENDIF ()

DEFAULT(USE_ASMLIB no)
DEFAULT(USE_INTERNAL_STL yes)
IF (NORUNTIME)
    SET(__saved_USE_INTERNAL_STL ${USE_INTERNAL_STL})
    SET(USE_INTERNAL_STL no)
    SET(USE_ARCADIA_LIBM no)
ELSEIF (${SAVEDTYPE} STREQUAL "PROG" AND NOT DARWIN AND NOT WIN32 AND NOT WITH_VALGRIND AND USE_ASMLIB)
#    PEERDIR(contrib/libs/asmlib)
ENDIF ()

IF (NOT PROJECTNAME STREQUAL "contrib-libs-stlport")
    IF (USE_INTERNAL_STL)
        PEERDIR(
            contrib/libs/stlport
        )
    ELSE ()
        SET_APPEND(LDFLAGS -lstdc++)
    ENDIF ()
ENDIF ()

DEFAULT(USE_ARCADIA_LIBM no)

IF (USE_ARCADIA_LIBM)
    IF (NOT "${PROJECTNAME}" STREQUAL "contrib-libs-libm")
        PEERDIR(
            contrib/libs/libm
        )
    ENDIF ()
ENDIF ()

IF (NORUNTIME)
    SET(USE_INTERNAL_STL ${__saved_USE_INTERNAL_STL})
ENDIF ()

IF (NOT WIN32)
    SET_APPEND(DTMK_D -D_THREAD_SAFE -D_PTHREADS -D_REENTRANT)
ENDIF ()

IF (USE_BERKELEY OR USE_BERKELEYDB)
    DEFAULT(BERKELEYDB /usr/local/db3.3)
    DEFAULT(BERKELEYDB_INCLUDE ${BERKELEYDB}/include)
    DEFAULT(BERKELEYDB_LIB -L${BERKELEYDB}/lib -ldb)
    SET_APPEND(DTMK_I ${BERKELEYDB_INCLUDE})
    SET_APPEND(THIRDPARTY_OBJADD ${BERKELEYDB_LIB})
ENDIF ()

IF (USE_GNUTLS)
    SET_APPEND(DTMK_D -DUSE_GNUTLS)
    SET_APPEND(LIBS -L/usr/lib -lgnutls)
ENDIF ()

IF (USE_MKL)
    SET_APPEND(DTMK_D -DNN_USE_MKL)
ENDIF ()

IF (USE_MIC_OPTS)
    SET_APPEND(DTMK_CFLAGS "-mmic")
ENDIF()

IF (USE_BOOST)
    SET(BOOST_VER 1_31)
    DEFAULT(BOOSTDIR /usr/local/boost)
    DEFAULT(BOOSTINC ${BOOSTDIR}/include/boost-${BOOST_VER})
    SET_APPEND(DTMK_I ${BOOSTINC})
    SET_APPEND(OBJADDE -L${BOOSTDIR}/lib)
ENDIF ()

IF (USE_LUA)
    SET_APPEND(DTMK_D -DUSE_LUA)
ENDIF ()

IF (USE_PYTHON)
    SET_APPEND(DTMK_D -DUSE_PYTHON)
#   DEFAULT(PYTHON_VERSION 2.4)
ENDIF ()

IF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
    ENABLE(PROFILE)
ELSE ()
    DISABLE(PROFILE)
ENDIF ()

IF (PERLXS OR PERLXSCPP OR USE_PERL)
    SET_APPEND(DTMK_D -DUSE_PERL)

    IF (WIN32)
        IF (NOT DEFINED LD_PERLLIB)
            EXECUTE_PROCESS(OUTPUT_VARIABLE LD_PERLLIB
                COMMAND ${PERL} -V:libperl
                RESULT_VARIABLE __perl_exec_res_
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            STRING(REGEX REPLACE "libperl='(.*)';" "\\1" LD_PERLLIB "${LD_PERLLIB}")
            FILE(TO_CMAKE_PATH ${PERLLIB_PATH}/${LD_PERLLIB} LD_PERLLIB)
            # fallback just in case
            IF (NOT EXISTS ${LD_PERLLIB})
                SET(LD_PERLLIB ${PERLLIB_PATH}/perl512.lib)
            ENDIF ()
            IF (NOT EXISTS ${LD_PERLLIB})
                SET(LD_PERLLIB ${PERLLIB_PATH}/perl510.lib)
            ENDIF ()
            IF (NOT EXISTS ${LD_PERLLIB})
                SET(LD_PERLLIB ${PERLLIB_PATH}/perl58.lib)
            ENDIF ()
        ENDIF ()
        SET_APPEND(OBJADDE ${LD_PERLLIB})
    ELSE ()
        DEFAULT(PERLLIB_BIN "-lperl")
        SET(LD_PERLLIB -L${PERLLIB_PATH} ${PERLLIB_BIN})
        IF (DARWIN)
            SET_APPEND(OBJADDE ${LD_PERLLIB})
        ELSEIF (PROFILE)
            SET_APPEND(OBJADDE -L${PERLLIB_PATH}/CORE -lperl_p)
            SET_APPEND(OBJADDE -lcrypt_p)
        ELSEIF (SUN)
            SET_APPEND(OBJADDE
                -Wl,-Bstatic ${LD_PERLLIB} -Wl,-Bdynamic -lcrypt)
        ELSEIF (LINK_STATIC_PERL)
            SET_APPEND(OBJADDE
                -Wl,-Bstatic ${LD_PERLLIB} -lcrypt -Wl,-Bdynamic)
        ELSE ()
            SET_APPEND(OBJADDE ${LD_PERLLIB} -lcrypt -Wl,-E)
        ENDIF ()

        IF (50809 GREATER ${PERL_VERSION})
            SET_APPEND(THIRDPARTY_OBJADD ${PERLLIB}/auto/DynaLoader/DynaLoader.a)
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (NOT WIN32)
        #SET_APPEND(OBJADDE ${LINK_WLBSTATIC_BEGIN} ${THREADLIB} ${LINK_WLBSTATIC_END})
        SET_APPEND(ST_OBJADDE ${THREADLIB})
ENDIF ()

IF (SUN)
    SET_APPEND(DTMK_D -D_REENTRANT) # for errno
    SET_APPEND(DTMK_D -DMUST_ALIGN -DHAVE_PARAM_H)
    SET_APPEND(LIBS -lnsl -lsocket -ldl -lresolv)
ELSEIF (LINUX)
    SET_APPEND(ST_OBJADDE -ldl -lrt)
    SET_APPEND(LIBS -ldl -lrt)
ENDIF ()

IF (ALLOCATOR)
    SET_APPEND(DTMK_D -D${ALLOCATOR})
ENDIF ()

MACRO(add_positive_define varname value)
    IF (NEED_${varname})
        ADD_DEFINITIONS(-DUSE_${varname}=${value})
    ENDIF ()
ENDMACRO ()

add_positive_define(PERL_GENERATE 1)
add_positive_define(PERL_GOODWORDS 1)
add_positive_define(PERL_REQSTAT 1)
add_positive_define(PERL_HILITESTR 1)
add_positive_define(PERL_URL2CACHE 1)
add_positive_define(PERL_GEOTARGET 1)
add_positive_define(PERL_FILTER 1)
add_positive_define(PERL_MIRROR 1)

# ============================================================================ #

SET(VARI "")

# ============================================================================ #

# Collecting additional include directories

IF (WIN32 OR ADDINCLSELF)
    SET(ADDINCL ${CMAKE_CURRENT_SOURCE_DIR})
ELSE ()
    SET_APPEND(DTMK_CFLAGS "-iquote" "\"${CMAKE_CURRENT_SOURCE_DIR}\"")
ENDIF ()

FOREACH(DIR ${ADDINCL})
    SET_APPEND(DTMK_I ${DIR})
ENDFOREACH ()

# TODO: another strange line
#DEPDIR ?=
SET(DEPDIR "")

IF (NOT NOPEER)
    IF (NOT LIB OR SHLIB_MAJOR)
        FOREACH(DIR ${PEERDIR})
            IF (EXISTS ${DIR})
                SET_APPEND(DEPDIR ${DIR})
            ENDIF ()
        ENDFOREACH ()
    ENDIF ()

    FOREACH (DIR ${TOOLDIR})
        IF (EXISTS ${DIR})
            SET_APPEND(TOOLDEPDIR ${DIR})
        ENDIF ()
    ENDFOREACH ()
ENDIF ()


IF (DEFINED SHLIB_MAJOR)
    SET(TARGET_SHLIB_MAJOR ${SHLIB_MAJOR})
ENDIF ()
IF (DEFINED SHLIB_MINOR)
    SET(TARGET_SHLIB_MINOR ${SHLIB_MINOR})
ENDIF ()


IF (NOT WIN32)
    IF (DEFINED TARGET_SHLIB_MAJOR)
        IF ("${HARDWARE_TYPE}" MATCHES "ia64" OR SUN)
            SET_APPEND(PICFLAGS -fPIC)
        ENDIF ()
        IF ("${HARDWARE_TYPE}" MATCHES "x86_64")
            SET_APPEND(PICFLAGS -fPIC)
        ENDIF ()
    ENDIF ()

    IF (PIC_FOR_ALL_TARGETS)
        IF ("${HARDWARE_TYPE}" MATCHES "x86_64")
            SET_APPEND(PICFLAGS -fPIC)
        ENDIF ()
    ENDIF ()
ENDIF ()

IF (MAKE_ONLY_SHARED_LIB AND NOT NO_SHARED_LIB)
    SET(MAKE_ONLY_SHARED_LIB0 yes)
    SET(DTMK_LIBTYPE SHARED)
ENDIF ()

IF (MAKE_ONLY_STATIC_LIB AND NOT NO_STATIC_LIB)
    SET(MAKE_ONLY_STATIC_LIB0 yes)
    SET(DTMK_LIBTYPE STATIC)
ENDIF ()

IF (MAKE_ONLY_STATIC_LIB0 AND MAKE_ONLY_SHARED_LIB0)
    MESSAGE(SEND_ERROR "MAKE_ONLY_SHARED_LIB and MAKE_ONLY_STATIC_LIB should not be used simultaneously (@ ${CMAKE_CURRENT_SOURCE_DIR})")
ENDIF ()

# =============================================================================================================================== #
# Здесь добавляются для каждого рума
#    - корневая директория аркадии
#    - директория объектников аркадии.
#
#FOREACH (CR ${ROOMS})
#   SET_APPEND(DTMK_I ${CR}/arcadia)
#   SET_APPEND(DTMK_I ${OBJDIRPREFIX}/${CR}/arcadia)
#ENDFOREACH ()

# Заменяем это всё на:
SET_APPEND(DTMK_I ${ARCADIA_ROOT} ${ARCADIA_BUILD_ROOT})

IF (PEERDIR)
    LIST_LENGTH(PEERDIR_COUNT ${PEERDIR})

    DEFAULT(LIBORDER_FILE "${ARCADIA_ROOT}/cmake/build/staticlibs.lorder")

    DEBUGMESSAGE(2 "PEERDIR[${PEERDIR}] in ${CURDIR}")
    FOREACH (DIR ${PEERDIR})
        SET(PEERPATH ${ARCADIA_ROOT}/${DIR})
        SET(MAKE_ONLY_SHARED_LIB)
        SET(SHLIB_MAJOR)
        SET(SHLIB_MINOR)
        SET(CDEP "NOTFOUND")

        IF (NOT CDEP AND EXISTS ${PEERPATH})
            SET(CDEP ${PEERPATH})
        ENDIF ()

        FOREACH (INC ${ADDINCL})
            # TODO: Check if it is ok
            SET_APPEND(DTMK_I ${INC})
        ENDFOREACH ()

        GET_GLOBAL_DIRNAME(__dir_ ${PEERPATH})

        # This check is a simple solution for METALIB-peerdir'ing-METALIB situations
        IS_METALIBRARY(__is_meta_ "${DIR}")
        IF (NOT __is_meta_)
            IF ("X${${__dir_}_DEPENDNAME_LIB}X" STREQUAL "XX")
                MESSAGE_EX("PEERDIR LIBRARY ${DIR} not found")
            ELSE ()
                GETTARGETNAME(__cur_lib_ ${${__dir_}_DEPENDNAME_LIB})
                SET_APPEND(PEERLIBS "${__cur_lib_}")
                SET_APPEND(PEERDEPENDS ${${__dir_}_DEPENDNAME_LIB})
                GET_DIR_HAS_GENERATED_INC(__dir_has_gen_inc ${PEERPATH})
                IF (__dir_has_gen_inc)
                    ADD_DIR_DEP_GENERATED(${DIR})
                ENDIF ()
                IMP_DIR_DEP_GENERATED(${PEERPATH})
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    SET(__dir_dep_generated)
    GET_DIR_DEP_GENERATED(__dir_dep_generated)
    IF (__dir_dep_generated)
        LIST(LENGTH __dir_dep_generated __dir_dep_generated_len)
        IF ("0" LESS "${__dir_dep_generated_len}")
            BUILDAFTER(${__dir_dep_generated})
        ENDIF ()
    ENDIF ()

    DEBUGMESSAGE(3 "PEERLDADD ${PEERLDADD} in ${CMAKE_CURRENT_SOURCE_DIR}")
ENDIF ()

IF (SRCDIR)
    FOREACH (DIR ${SRCDIR})
        SET(__srcpath_ ${ARCADIA_ROOT}/${DIR})
    ENDFOREACH ()
ENDIF ()

FOREACH (DIR ${SUBDIR})
    IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${DIR})
        SET_APPEND(DEPDIR ${CMAKE_CURRENT_SOURCE_DIR}/${DIR})
        SET_APPEND(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR}/${DIR})
    ELSE ()
        DEBUGMESSAGE(1 "${CMAKE_CURRENT_SOURCE_DIR}/${DIR} listed in \"${CMAKE_CURRENT_SOURCE_DIR}\"'s SUBDIR doesn't exist. Ignoring")
    ENDIF ()
ENDFOREACH ()

SEPARATE_ARGUMENTS_SPACE(PEERLDADD)

IF (SUN)
    SET(PEERLDADD "-z rescan ${PEERLDADD}")
ELSEIF (DARWIN)
    SET(PEERLDADD "${PEERLDADD} ${PEERLDADD}")
ELSEIF (WIN32)
    # PEERLDADD is already well-formatted
ELSE ()
    IF (PEERLIBS AND NOT "${CMAKE_CXX_CREATE_SHARED_LIBRARY}" MATCHES "start-group <LINK_LIBRARIES>")
        STRING(REPLACE "<LINK_LIBRARIES>"   "-Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group"   CMAKE_C_LINK_EXECUTABLE   "${CMAKE_C_LINK_EXECUTABLE}")
        STRING(REPLACE "<LINK_LIBRARIES>"   "-Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group"   CMAKE_CXX_LINK_EXECUTABLE   "${CMAKE_CXX_LINK_EXECUTABLE}")

        STRING(REPLACE "<LINK_LIBRARIES>"   "-Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group"   CMAKE_C_CREATE_SHARED_LIBRARY   "${CMAKE_C_CREATE_SHARED_LIBRARY}")
        STRING(REPLACE "<LINK_LIBRARIES>"   "-Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group"   CMAKE_CXX_CREATE_SHARED_LIBRARY   "${CMAKE_CXX_CREATE_SHARED_LIBRARY}")
    ENDIF ()
ENDIF ()

IF (SRCS)
    IF ("${CMAKE_BUILD_TYPE}" MATCHES "Coverage")
        DEFAULT(GCOV_OUT_FNAME /dev/stdout)
        FOREACH (src_file ${SRCS})
            GET_FILENAME_COMPONENT(src_file_path ${src_file} PATH)
            GET_FILENAME_COMPONENT(src_file_name ${src_file} NAME_WE)
            GET_FILENAME_COMPONENT(src_file_ext ${src_file} EXT)

            # guarantee we have a relative path
            IF (src_file_path)
                STRING(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} "" src_file_path ${src_file_path})
            ENDIF ()

            SET_APPEND(ALLGCOV ${CMAKE_CURRENT_BINARY_DIR}/${src_file_path}/@@${src_file_name}${src_file_ext}.gcov)
            IF (EXISTS ${src_file_path}/${src_file_name}_c.gcno)
                ADD_CUSTOM_COMMAND_EX(
                    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${src_file_path}/@@${src_file_name}${src_file_ext}.gcov
                    PRE_BUILD
                    COMMAND echo "=====> SOURCE: " ${src_file} >>${GCOV_OUT_FNAME}
                    COMMAND echo "=====> GCOV OUT DIR: " ${CMAKE_CURRENT_BINARY_DIR} >>${GCOV_OUT_FNAME}

                    COMMAND ${GCOV} ${GCOV_OPTIONS} -o ${src_file_path}/${src_file_name}_c.o ${src_file} >>${GCOV_OUT_FNAME} 2>&1
                    DEPENDS ${src_file_path}/${src_file_name}_c.gcno ${src_file_path}/${src_file_name}_c.gcda
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT ${GCOV} ${GCOV_OPTIONS} -o ${src_file_path}/${src_file_name}_c.o ${src_file}
                    )
            ELSE ()
                ADD_CUSTOM_COMMAND_EX(
                    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${src_file_path}/@@${src_file_name}${src_file_ext}.gcov
                    PRE_BUILD
                    COMMAND echo Warning: ${src_file_path}/${src_file_name}_c.gcno not found
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    )
            ENDIF ()
        ENDFOREACH ()

        ADD_CUSTOM_TARGET (gcov
            DEPENDS ${ALLGCOV}
        )
    ENDIF ()
ENDIF ()

IF ("${CMAKE_BUILD_TYPE}" MATCHES "Release")
    IF (DEFINED PKG_FOR_FREEBSD_4.1)
        SET(PKG_FOR_FREEBSD_41_FLAGS -Wl,-u__ti9exception -lgcc)
    ELSE ()
        SET(PKG_FOR_FREEBSD_41_FLAGS "")
    ENDIF ()
ENDIF ()

MACRO (EXTRACT_INC_DEF var)
    SET(__var_)
    SET(__prevopt_)

    IF (WIN32)
        SET(__dash_ "[-/]")
    ELSE ()
        SET(__dash_ "-")
    ENDIF ()

    FOREACH(__item_ ${${var}})
        IF (NOT DEFINED __prevopt_)
            IF (__item_ MATCHES "^${__dash_}([ID])(.*)")
                SET(__prevopt_ ${CMAKE_MATCH_1})
                SET(__item_ ${CMAKE_MATCH_2})

            ELSE ()
                SET_APPEND(__var_ ${__item_})

            ENDIF ()
        ENDIF ()

        IF (DEFINED __prevopt_ AND NOT "${__item_}" STREQUAL "")
            IF ("${__prevopt_}" STREQUAL "I")
                SET_APPEND(DTMK_I ${__item_})

            ELSEIF ("${__prevopt_}" STREQUAL "D")
                SET_APPEND(DTMK_D -D${__item_})

            ENDIF ()
            SET(__prevopt_)

        ENDIF ()
    ENDFOREACH ()

    SET(${var} ${__var_})
ENDMACRO ()

# ============================================================================ #
# restore before PEERDIR
SET(MAKE_ONLY_SHARED_LIB MAKE_ONLY_SHARED_LIB0)
# Now apply global keyword deps (after all peerdirs apply and before CFLAGS processing)
APPLY_GLOBAL_TO_CURDIR()
SET(MAKE_ONLY_SHARED_LIB)

# ============================================================================ #
# Now parse CFLAGS and extract -I and -D from there
EXTRACT_INC_DEF(DTMK_CFLAGS)
EXTRACT_INC_DEF(CFLAGS)

# ============================================================================ #

# Now set cmake's C/C++ compiler flags
# === C Flags ===
SET_APPEND(DTMK_CFLAGS ${PICFLAGS} ${CFLAGS})
SET_APPEND(CMAKE_C_FLAGS ${DTMK_CFLAGS})

SET_APPEND(DTMK_CXXFLAGS ${CXXFLAGS})
SEPARATE_ARGUMENTS_SPACE(CMAKE_C_FLAGS)
SET_APPEND(CMAKE_C_FLAGS_RELEASE ${OPTIMIZE})
SEPARATE_ARGUMENTS_SPACE(CMAKE_C_FLAGS_RELEASE)
DEBUGMESSAGE(3 "${CURDIR}: C_FLAGS=${CMAKE_C_FLAGS}")
DEBUGMESSAGE(3 "${CURDIR}: C_FLAGS_RELEASE=${CMAKE_C_FLAGS_RELEASE}")

# === C++ Flags ===
SET_APPEND(CMAKE_CXX_FLAGS "${DTMK_CFLAGS} ${DTMK_CXXFLAGS}")
SET_APPEND(CMAKE_CXX_FLAGS_RELEASE ${OPTIMIZE})
SEPARATE_ARGUMENTS_SPACE(CMAKE_CXX_FLAGS)
SEPARATE_ARGUMENTS_SPACE(CMAKE_CXX_FLAGS_RELEASE)
DEBUGMESSAGE(3 "${CURDIR}: CXX_FLAGS=${CMAKE_CXX_FLAGS}")
DEBUGMESSAGE(3 "${CURDIR}: CXX_FLAGS_RELEASE=${CMAKE_CXX_FLAGS_RELEASE}")

IF (LINUX)
    SET(LDHACK -Wl,--no-as-needed)
ENDIF ()

# === Link Flags ===
SET(DTMK_L "${MPROFLIB} ${LDFLAGS} ${LDHACK} ${OBJADD} ${VAROBJS} ${LIBS} ${PKG_FOR_FREEBSD_41_FLAGS}") # ${PEERLDADD}
SET(CMAKE_EXE_LINKER_FLAGS "${PROFFLAG}")
IF (MSVC)
    SET_APPEND(CMAKE_EXE_LINKER_FLAGS " /ignore:4042") # ignore warning: object specified more than once; extras ignored
ENDIF ()
IF(MSVC AND NOT USE_MANIFEST)
    SET_APPEND(CMAKE_EXE_LINKER_FLAGS " /MANIFEST:NO")
ENDIF ()
SEPARATE_ARGUMENTS_SPACE(CMAKE_EXE_LINKER_FLAGS)


# === Static C++ runtime ===
IF (USE_STATIC_CPP_RUNTIME)
    IF (NOT "X${ST_OBJADDE}${ST_LDFLAGS}X" MATCHES "XX")
        IF (NOT "${CMAKE_C_LINK_EXECUTABLE}" MATCHES "${ST_LDFLAGS}")
            STRING(REPLACE "<CMAKE_C_COMPILER>"   "<CMAKE_C_COMPILER> ${ST_LDFLAGS}"   CMAKE_C_LINK_EXECUTABLE   "${CMAKE_C_LINK_EXECUTABLE}")
            STRING(REPLACE "<CMAKE_CXX_COMPILER>" "<CMAKE_CXX_COMPILER> ${ST_LDFLAGS}" CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")
        ENDIF ()

        DEBUGMESSAGE(2 "${CURDIR}: CMAKE_C_LINK_EXECUTABLE=${CMAKE_C_LINK_EXECUTABLE}")
        DEBUGMESSAGE(2 "${CURDIR}: CMAKE_CXX_LINK_EXECUTABLE=${CMAKE_CXX_LINK_EXECUTABLE}")

        # Avoid replacement if run for the second time (in CREATE_SHARED_LIBRARY())"
        IF (NOT "${CMAKE_C_CREATE_SHARED_LIBRARY}" MATCHES "${ST_LDFLAGS}" AND USE_DL_STATIC_RUNTIME)
            STRING(REPLACE "<CMAKE_C_COMPILER>"   "<CMAKE_C_COMPILER> ${ST_LDFLAGS}"   CMAKE_C_CREATE_SHARED_LIBRARY   "${CMAKE_C_CREATE_SHARED_LIBRARY}")
            STRING(REPLACE "<CMAKE_CXX_COMPILER>" "<CMAKE_CXX_COMPILER> ${ST_LDFLAGS}" CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY}")
        ENDIF ()

        SET_APPEND(THIRDPARTY_OBJADD ${ST_OBJADDE})
    ENDIF ()
ENDIF ()

IF (NO_MAPREDUCE)
    ADD_DEFINITIONS(-DNO_MAPREDUCE)
ENDIF ()

ENDMACRO ()
