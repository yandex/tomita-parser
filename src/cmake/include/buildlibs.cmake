MACRO (DLL)
    SET(__dll_name_)
    SET(__major_ver_)
    SET(__minor_ver_)
    SET(__next_exports_)
    SET(__next_prefix_)
    SET(USER_PREFIX)
    FOREACH(__item_ ${ARGN})
        IF ("${__item_}" STREQUAL "EXPORTS")
            SET(__next_exports_ yes)
        ELSEIF ("${__item_}" STREQUAL "PREFIX")
            SET(__next_prefix_ yes)
        ELSE ()
            IF (__next_exports_)
                SET(__export_file_ ${__item_})
                SET(__next_exports_)
            ELSEIF (__next_prefix_)
                SET(USER_PREFIX ${__item_})
                SET(__next_prefix_)
            ELSE ()
                IF ("X${__dll_name_}X" STREQUAL "XX")
                    SET(__dll_name_ ${__item_})
                ELSEIF ("X${__major_ver_}X" STREQUAL "XX")
                    SET(__major_ver_ ${__item_})
                ELSEIF ("X${__minor_ver_}X" STREQUAL "XX")
                    SET(__minor_ver_ ${__item_})
                ELSE ()
                    MESSAGE(SEND_ERROR "Unknown DLL argument - ${item}. DLL name - ${__dll_name_}")
                ENDIF ()
            ENDIF ()
        ENDIF ()
    ENDFOREACH ()

    LIBRARY_IMPL(${__dll_name_})
    SET(LOCAL_SHARED_LIBRARY_PREFIX ${USER_PREFIX})

    IF (NOT "${__major_ver_}" STREQUAL "")
        SET(SHLIB_MAJOR ${__major_ver_})

        IF (NOT "${__minor_ver_}" STREQUAL "")
            SET(SHLIB_MINOR ${__minor_ver_})
        ENDIF ()
    ENDIF ()

    ENABLE(MAKE_ONLY_SHARED_LIB)
    PROP_CURDIR_SET(IS_SHARED_LIB yes)

    IF (NOT "X${__export_file_}X" STREQUAL "XX")
        GET_SRC_ABS_PATH(__export_file_full_ ${__export_file_})
        IF (NOT WIN32)
            OBJADDE("-Wl,--version-script=${__export_file_full_}")
        ENDIF ()
    ENDIF ()
ENDMACRO ()

MACRO (PLUGIN)
    DLL(${ARGN})

    IF(NOT WIN32 AND NOT DARWIN)
        # This is needed to force symbol lookup in the .so itself
        SET_APPEND(LDFLAGS "-Wl,-Bsymbolic")
    ENDIF ()

    # These flags are needed for correct exception handling
    IF(FREEBSD)
        CFLAGS(-static-libgcc)
        OBJADDE(-lsupc++)
    ENDIF ()
ENDMACRO ()

# =============================================================================
# KwCalc UDF
#
MACRO (UDF)
    PLUGIN(${ARGN})

    ALLOCATOR(SYSTEM)

    # Always link udflib
    PEERDIR(yweb/robot/kiwi/kwcalc/udflib)
ENDMACRO ()

MACRO (LIBRARY)
    IF (DLL_FOR_DIR)
        # LOG("LIBRARY ${CMAKE_CURRENT_SOURCE_DIR} dynamic for: ${DLL_FOR_DIR}, args: ${DLL_FOR_ARGS} / ${ARGN}")
        DLL(${DLL_FOR_ARGS})
        SRCDIR(${DLL_FOR_DIR})
        ADDINCL(${DLL_FOR_DIR})
        # make it possibale to use IF(DLL_FOR) inside CMakeLists.txt with LIBRARY
        ENABLE(DLL_FOR)
        # reset flags immediately to not influence peers
        SET(DLL_FOR_DIR)
        SET(DLL_FOR_ARGS)
    ELSE ()
        # LOG("LIBRARY as usual: ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN}")
        LIBRARY_IMPL(${ARGN})
    ENDIF ()
ENDMACRO ()

MACRO(FOR_BASE var proj)
    SET(${var}_FOR_DIR ${proj})
    SET(${var}_FOR_ARGS ${ARGN})
    INCLUDE(${ARCADIA_ROOT}/${proj}/CMakeLists.txt)
    SET(${var}_FOR_DIR)
    SET(${var}_FOR_ARGS)
ENDMACRO()

MACRO (DLL_FOR)
    FOR_BASE(DLL ${ARGN})
ENDMACRO ()

MACRO (PY_PROTOS_FOR dir)
    FOR_BASE(PY_PROTOS ${dir})

    IF (NOT "${dir}" STREQUAL "contrib/libs/protobuf/python")
        RECURSE_ROOT_RELATIVE(contrib/libs/protobuf/python)
    ENDIF ()
ENDMACRO ()
