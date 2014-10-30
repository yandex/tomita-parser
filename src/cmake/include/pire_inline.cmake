ADDINCL(contrib/libs/pire/pire)

MACRO (PIRE_INLINE)
    FOREACH (__src ${ARGN})
        IF (IS_ABSOLUTE ${__src})
            SET(__src_path "${__src}")
            GET_FILENAME_COMPONENT(__src_name ${__src_path} NAME)
            SET(__dst_path "${BINDIR}/${__src_name}")
        ELSE ()
            SET(__src_path "${CURDIR}/${__src}")
            SET(__dst_path "${BINDIR}/${__src}")
        ENDIF ()

        RUN_PROGRAM(
            library/pire/inline -o ${__dst_path} ${__src_path}
                IN  ${__src_path}
                OUT ${__dst_path}
                CWD ${CURDIR}
        )
    ENDFOREACH ()
ENDMACRO ()
