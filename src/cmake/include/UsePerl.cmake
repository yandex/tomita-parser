MACRO (PERL_ADDINCL)
    INCLUDE(${ARCADIA_ROOT}/cmake/include/FindPerl2.cmake)

    CFLAGS(
        -I${PERLLIB_PATH}
    )
ENDMACRO ()

MACRO (USE_PERL_LIB)
    PERL_ADDINCL()

    SET_APPEND(OBJADDE
        -lperl
    )
ENDMACRO ()

MACRO (ADD_PERL_MODULE dir module)
    PEERDIR(${dir})
    SET_APPEND(modules ${module})
ENDMACRO ()
