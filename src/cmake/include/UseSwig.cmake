MACRO (SWIG_PERL_INIT)
    SET(USE_PERL)
    CHECK_PERL_VARS()
    INCLUDE_DIRECTORIES(${PERLLIB_PATH})
    SET_APPEND(SWIG_FLAGS "-shadow")
ENDMACRO ()

MACRO (SWIG_PYTHON_INIT)
    PYTHON_ADDINCL()
    GET_FILENAME_COMPONENT(_interface_ ${LOCAL_SHARED_LIBRARY_PREFIX}${SWIG_MODULE}${CMAKE_SHARED_LIBRARY_SUFFIX} NAME_WE)
    SET_APPEND(SWIG_FLAGS -interface ${_interface_})
ENDMACRO ()

MACRO (SWIG_JAVA_INIT)
    FIND_PACKAGE(JNI)
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH})
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH2})
ENDMACRO ()

MACRO (SWIG_INIT)
    TOOLDIR_EX(
        contrib/tools/swig SWIG
    )
    DEFAULT(SWIG_FLAGS "-c++")
    DEFAULT(SWIG_LANG "python")
    DEFAULT(SWIG_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    STRING(TOLOWER "${SWIG_LANG}" SWIG_LANG)
    DEFAULT(SWIG_MODULE ${REALPRJNAME})
    IF (SWIG_MODULE)
        SET_APPEND(SWIG_FLAGS -module ${SWIG_MODULE})
    ENDIF ()
    SET_APPEND(SWIG_FLAGS -cpperraswarn)

    IF (${SWIG_LANG} STREQUAL "perl")
        SWIG_PERL_INIT()
    ELSEIF (${SWIG_LANG} STREQUAL "python")
        SWIG_PYTHON_INIT()
    ELSEIF (${SWIG_LANG} STREQUAL "java")
        SWIG_JAVA_INIT()
    ENDIF ()
    SET(SWIG_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    FILE(MAKE_DIRECTORY ${SWIG_OUT_DIR})
ENDMACRO ()

MACRO (BUILDWITH_SWIG srcfile dstfile)
    SWIG_INIT()
    ADD_CUSTOM_COMMAND(
        OUTPUT ${dstfile}
        COMMAND "${SWIG}"
        ARGS    ${SWIG_FLAGS}
                -${SWIG_LANG}
                -o "${dstfile}"
                -outdir ${SWIG_OUT_DIR}
                -I${ARCADIA_ROOT}
                -I${ARCADIA_BUILD_ROOT}
                -I${ARCADIA_ROOT}/bindings/swiglib
                ${srcfile}
        MAIN_DEPENDENCY "${srcfile}"
        DEPENDS "${srcfile}" ${SWIG} ${ARGN}
        COMMENT "Building \"${srcfile}\" with swig"
    )
    SOURCE_GROUP("Custom Builds" FILES ${srcfile})
    SOURCE_GROUP("Generated" FILES ${dstfile} )
    SRCS(${dstfile})
ENDMACRO ()
