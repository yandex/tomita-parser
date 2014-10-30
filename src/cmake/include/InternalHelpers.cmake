IF (NOT DEFINED __INCLUDED__InternalHelpers__cmake__)

SET (__INCLUDED__InternalHelpers__cmake__ TRUE)

MACRO (PROP_SET type)
    set_property(${type} PROPERTY ${ARGN})
ENDMACRO ()

MACRO (PROP_OBJ_SET type obj)
    set_property(${type} ${obj} PROPERTY ${ARGN})
ENDMACRO ()


MACRO (PROP_ADD type)
    set_property(${type} APPEND PROPERTY ${ARGN})
ENDMACRO ()

MACRO (PROP_OBJ_ADD type obj)
    set_property(${type} ${obj} APPEND PROPERTY ${ARGN})
ENDMACRO ()


MACRO (PROP_GET var type)
    get_property(${var} ${type} PROPERTY ${ARGN})
ENDMACRO ()

MACRO (PROP_OBJ_GET var type obj)
    get_property(${var} ${type} ${obj} PROPERTY ${ARGN})
ENDMACRO ()


MACRO (PROP_CURDIR_SET)
    PROP_SET(DIRECTORY ${ARGN})
ENDMACRO ()

MACRO (PROP_CURDIR_ADD)
    PROP_ADD(DIRECTORY ${ARGN})
ENDMACRO ()

MACRO (PROP_CURDIR_GET var)
    PROP_GET(${var} DIRECTORY ${ARGN})
ENDMACRO ()

MACRO (PROP_DIR_SET dir)
    PROP_OBJ_SET(DIRECTORY ${dir} ${ARGN})
ENDMACRO ()

MACRO (PROP_DIR_ADD dir)
    PROP_OBJ_ADD(DIRECTORY ${dir} ${ARGN})
ENDMACRO ()

MACRO (PROP_DIR_GET var dir)
    PROP_OBJ_GET(${var} DIRECTORY ${dir} ${ARGN})
ENDMACRO ()


MACRO (PROP_SRC_SET src)
    PROP_OBJ_SET(SOURCE ${src} ${ARGN})
ENDMACRO ()

MACRO (PROP_SRC_ADD src)
    PROP_OBJ_ADD(SOURCE ${src} ${ARGN})
ENDMACRO ()

MACRO (PROPS_SRC_SET srcs)
    set_source_files_properties(${${srcs}} PROPERTIES ${ARGN})
ENDMACRO ()

MACRO (PROP_SRC_GET var src)
    PROP_OBJ_GET(${var} SOURCE ${src} ${ARGN})
ENDMACRO ()


FUNCTION (GET_GLOBAL_DIRNAME varname path)
    FILE(TO_CMAKE_PATH ${path} __vartmppath_)
    STRING(REGEX REPLACE "^./+" "" __vartmppath_ "${__vartmppath_}")
    STRING(REGEX REPLACE "/+" "__" __vartmppath_ "${__vartmppath_}")
    STRING(REGEX REPLACE "[^a-zA-Z0-9_]" "_" __vartmppath_ "${__vartmppath_}")
    SET(${varname} ${__vartmppath_} PARENT_SCOPE)
ENDFUNCTION()

MACRO(MESSAGE_EX)
    FILE(RELATIVE_PATH __rp_current ${ARCADIA_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
    IF (NOT __rp_current)
        SET(__rp_current ARCADIA_ROOT)
    ENDIF ()
    MESSAGE("${__rp_current}> ${ARGN}")
ENDMACRO()

ENDIF ()
