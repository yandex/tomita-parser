MACRO (GENERATE_PACKAGE)
    # Don't process SRCS inside PACKAGE?
    # CHECK_SRCS(${SRCS})
    # PREPARE_SRC_FILES(SRCS SRCDIR ${ARCADIA_ROOT})
    IF("X${PACKAGE_TYPE}X" STREQUAL "XX")
        SET(__output_ ${PACKAGE_FILES})
    ELSEIF ("${PACKAGE_TYPE}" STREQUAL "TGZ")
        SET(__output_ ${CMAKE_CURRENT_BINARY_DIR}/${REALPRJNAME}.tgz)
        ADD_CUSTOM_COMMAND(
            OUTPUT ${__output_}
            COMMAND cmake -E tar czf "${__output_}" ${PACKAGE_FILES}
            DEPENDS ${PACKAGE_FILES}
        )
    ELSE ()
        MESSAGE(FATAL_ERROR "PACKAGE_TYPE ${PACKAGE_TYPE} not implemented yet")
    ENDIF ()

    PROP_CURDIR_GET(__pkg_target_name_ PACKAGE_TARGET_NAME)
    ADD_CUSTOM_TARGET(
        ${__pkg_target_name_} ALL DEPENDS ${__output_} ${PACKAGE_PEERDIR_DEPS}
    )
ENDMACRO ()

MACRO (FILES)
    IF (NOT ${SAVEDTYPE} STREQUAL "PACKAGE")
        MESSAGE_EX("FILES macro can be used only inside PACKAGE unit")
    ELSE ()
        FOREACH(__item_ ${ARGN})
            IF ("${__item_}" STREQUAL "AS")
                SET(__next_where_ yes)
            ELSE ()
                IF (__next_where_)
                    SET(__next_where_)
                    MESSAGE_EX("FILES macro: AS keyword not implemented yet")
                ELSE ()
                    SET_APPEND(PACKAGE_FILES ${__item_})
                ENDIF ()
            ENDIF ()
        ENDFOREACH ()
    ENDIF ()
ENDMACRO ()

MACRO (TGZ_FOR)
    FOR_BASE(TGZ ${ARGN})
ENDMACRO ()

MACRO (DEB_FOR)
    FOR_BASE(DEB ${ARGN})
ENDMACRO ()

MACRO (RPM_FOR)
    FOR_BASE(RPM ${ARGN})
ENDMACRO ()
