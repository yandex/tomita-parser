#This file contains helpers used to simplify the Makefile-to-CMakeList process

#
# Substitution of ?=
#
MACRO (DEFAULT variable)
    IF (NOT DEFINED ${variable})
        SET(${variable} ${ARGN})
    ENDIF ()
ENDMACRO ()

#
# Substitution of +=
# SET_APPEND(variable value)
#
MACRO (SET_APPEND variable )
    SET(${variable} ${${variable}} ${ARGN})
ENDMACRO ()

# Substitution of ismake(targetnames )
# TODO

#
# Substitution of !=
#
MACRO (SET_EXEC variable command)
    EXECUTE_PROCESS(COMMAND ${command}
    OUTPUT_VARIABLE ${variable})
ENDMACRO ()

# =============================================================================
# LIST_ADDPREFIX
#  this macro adds prefix to each item of listname_var
#
MACRO (LIST_ADDPREFIX listname_var prefix)
    SET(__list_addprefix_newlist_)
    FOREACH(__list_addprefix_item_ ${${listname_var}})
        SET_APPEND(__list_addprefix_newlist_ ${prefix}${__list_addprefix_item_})
    ENDFOREACH ()
    SET(${listname_var} ${__list_addprefix_newlist_})
ENDMACRO ()

#
# DEBUGMESSAGE: message is printed only when level <= DEBUG_MESSAGE_LEVEL
#
MACRO (DEBUGMESSAGE level)
    IF (NOT ${DEBUG_MESSAGE_LEVEL} LESS ${level})
        MESSAGE("${ARGN}")
    ENDIF ()
ENDMACRO ()

# Setting default value of 0 right here
DEFAULT(DEBUG_MESSAGE_LEVEL 0)


#
# Some helper macros
# (taken from http://www.vtk.org/Wiki/CMakeMacroParseArguments)
#

MACRO(CAR var val)
  SET(${var} ${ARGV1})
ENDMACRO ()

MACRO(CDR var junk)
  SET(${var} ${ARGN})
ENDMACRO ()

MACRO(LIST_LENGTH var)
  SET(entries)
  FOREACH(e ${ARGN})
    SET(entries "${entries}.")
  ENDFOREACH ()
  STRING(LENGTH "${entries}" ${var})
ENDMACRO ()

MACRO(LIST_INDEX var index)
  SET(list . ${ARGN})
  FOREACH(i RANGE 1 ${index})
    CDR(list ${list})
  ENDFOREACH ()
  CAR(${var} ${list})
ENDMACRO ()

MACRO(LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET(${var} TRUE)
    ENDIF ()
  ENDFOREACH ()
ENDMACRO ()

# LIST_FILTER(<list> <regexp_var> [<regexp_var> ...]
#              [OUTPUT_VARIABLE <variable>])
# Removes items from <list> which do not match any of the specified
# regular expressions. An optional argument OUTPUT_VARIABLE
# specifies a variable in which to store the matched items instead of
# updating <list>
# As regular expressions can not be given to macros (see bug #5389), we pass
# variable names whose content is the regular expressions.
# Note that this macro requires PARSE_ARGUMENTS macro, available here:
# http://www.cmake.org/Wiki/CMakeMacroParseArguments
MACRO(LIST_FILTER)
  PARSE_ARGUMENTS(LIST_FILTER "OUTPUT_VARIABLE" "" ${ARGV})
  # Check arguments.
  LIST(LENGTH LIST_FILTER_DEFAULT_ARGS LIST_FILTER_default_length)
  IF(${LIST_FILTER_default_length} EQUAL 0)
    MESSAGE(FATAL_ERROR "LIST_FILTER: missing list variable.")
  ELSEIF(${LIST_FILTER_default_length} EQUAL 1)
    MESSAGE(FATAL_ERROR "LIST_FILTER: missing regular expression variable.")
  ENDIF ()
  # Reset output variable
  IF(NOT LIST_FILTER_OUTPUT_VARIABLE)
    SET(LIST_FILTER_OUTPUT_VARIABLE "LIST_FILTER_internal_output")
  ENDIF ()
  SET(${LIST_FILTER_OUTPUT_VARIABLE})
  # Extract input list from arguments
  LIST(GET LIST_FILTER_DEFAULT_ARGS 0 LIST_FILTER_input_list)
  LIST(REMOVE_AT LIST_FILTER_DEFAULT_ARGS 0)
  FOREACH(LIST_FILTER_item ${${LIST_FILTER_input_list}})
    FOREACH(LIST_FILTER_regexp_var ${LIST_FILTER_DEFAULT_ARGS})
      FOREACH(LIST_FILTER_regexp ${${LIST_FILTER_regexp_var}})
        IF(${LIST_FILTER_item} MATCHES ${LIST_FILTER_regexp})
          LIST(APPEND ${LIST_FILTER_OUTPUT_VARIABLE} ${LIST_FILTER_item})
        ENDIF ()
      ENDFOREACH ()
    ENDFOREACH ()
  ENDFOREACH ()
  # If OUTPUT_VARIABLE is not specified, overwrite the input list.
  IF(${LIST_FILTER_OUTPUT_VARIABLE} STREQUAL "LIST_FILTER_internal_output")
    SET(${LIST_FILTER_input_list} ${${LIST_FILTER_OUTPUT_VARIABLE}})
  ENDIF ()
ENDMACRO ()

#
# PARSE_ARGUMENTS(prefix arg_names option_names)
# (taken from http://www.vtk.org/Wiki/CMakeMacroParseArguments)
#
MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH ()
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH ()

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    LIST_CONTAINS(is_arg_name ${arg} ${arg_names})
    IF (is_arg_name)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE ()
      LIST_CONTAINS(is_option ${arg} ${option_names})
      IF (is_option)
        SET(${prefix}_${arg} TRUE)
      ELSE ()
        SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF ()
    ENDIF ()
  ENDFOREACH ()
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO ()

#
# Substitution of target1 .. targetN: deptarget1 .. deptargetM
# Usage: ADD_DEPENDENCIES_MULTI(target1 .. targetN DEPENDS deptarget1 .. deptargetM)
#
MACRO (ADD_DEPENDENCIES_MULTI dep1)
    SET(__adm_deptarget)
    SET(__adm_targets)
    SET(__depchain ${dep1} ${ARGN})
    FOREACH(__adm_item ${depchain})
        IF (NOT DEFINED __adm_deptarget)
            IF (NOT "${__adm_item}" MATCHES "DEPENDS")
                SET_APPEND(__adm_targets ${__adm_item})
            ELSE ()
                SET(__adm_deptarget "")
            ENDIF ()
        ELSE ()
            SET_APPEND(__adm_deptarget ${__adm_item})
        ENDIF ()
    ENDFOREACH ()

    FOREACH(__adm_item ${__adm_targets})
        ADD_DEPENDENCIES(${__adm_item} ${__adm_deptarget})
    ENDFOREACH ()
ENDMACRO ()

#
# Substitution of target1 .. targetN: deptarget1 .. deptargetM
# Usage: ADD_DEPENDENCIES_MULTI(target1 .. targetN DEPENDS deptarget1 .. deptargetM)
#
MACRO (ADD_DEPENDENCIES_ONEOF curtarget)
    SET(__adm_deptarget)
    SET(__adm_targets)
    SET(__depchain ${ARGN})
    FOREACH(__adm_item ${__depchain})
        IF (NOT DEFINED __adm_deptarget)
            IF (NOT "${__adm_item}" MATCHES "DEPENDS")
                SET_APPEND(__adm_targets ${__adm_item})
            ELSE ()
                SET(__adm_deptarget "")
            ENDIF ()
        ELSE ()
            SET_APPEND(__adm_deptarget ${__adm_item})
        ENDIF ()
    ENDFOREACH ()

    FOREACH(__adm_item ${__adm_targets})
        IF ("${curtarget}" MATCHES "${__adm_item}")
            ADD_DEPENDENCIES(${__adm_item} ${__adm_deptarget})
        ENDIF ()
    ENDFOREACH ()
ENDMACRO ()

#
# Transform to space-delimited list (SEPARATE_ARGUMENTS delimits them with semicolon)
#
MACRO (SEPARATE_ARGUMENTS_SPACE variable)
    SET(__dst "")
    FOREACH(__item_ ${${variable}})
        SET(__dst "${__dst} ${__item_}")
    ENDFOREACH ()
    SET(${variable} "${__dst}")
ENDMACRO ()

#
# Simple macros to make positive/negative value
#
MACRO (ENABLE name)
    SET(${name} yes)
ENDMACRO ()

MACRO (DISABLE name)
    SET(${name} no)
ENDMACRO ()

# Macro for setting all variables in list to the same value
MACRO(SET_ALL)
    SET(__tmplist_ ${ARGN})
    LIST(LENGTH __tmplist_ __tmplist_len_)
    MATH(EXPR __tmplist_len_ "${__tmplist_len_}-1")
    LIST(GET __tmplist_ ${__tmplist_len_} __tmp_value_)
    LIST(REMOVE_AT __tmplist_ ${__tmplist_len_})
    FOREACH(__tmplist_item_ ${__tmplist_})
        SET(${__tmplist_item_} "${__tmp_value_}")
    ENDFOREACH ()
ENDMACRO ()

MACRO(INCLUDE_FROM filename)
    FOREACH(__dir_ ${ARGN})
        INCLUDE(${__dir_}/${filename} OPTIONAL)
    ENDFOREACH ()
ENDMACRO ()
