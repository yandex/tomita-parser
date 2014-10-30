# - Find Perl
# This module finds if Perl is installed and determines where the
# executables are. This code sets the following variables:
#
#  PERL:                path to the Perl compiler
#
# This code defines the following macros:
#
#  BUILDWITH_PERL(script destfile sourcefiles)
#

IF (NOT YMAKE)
IF (WIN32)
    FIND_PACKAGE ( Perl )
    IF ( PERL_FOUND )
        MESSAGE ( STATUS "  PERL_EXECUTABLE:        " ${PERL_EXECUTABLE} )
        DEFAULT(PERL  ${PERL_EXECUTABLE})
    ELSE ()
        MESSAGE ( STATUS "Fallback to use just 'perl.exe' and hope, it's OK." )
        DEFAULT(PERL  perl.exe)
    ENDIF ()
ELSE ()
	DEFAULT(PERL		perl)
ENDIF ()

MACRO(IS_PERL_OK __res_)
    IF (NOT DEFINED IS_PERL_OK_RESULT)
        EXECUTE_PROCESS(OUTPUT_VARIABLE __tmp_
            COMMAND ${PERL} -V
            RESULT_VARIABLE __perl_exec_res_
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (__perl_exec_res_ EQUAL 0)
            IS_VALID_PERL(__is_valid_perl_res_ __tmp_)
            IF (__is_valid_perl_res_)
                ENABLE(IS_PERL_OK_RESULT)
            ELSE ()
                DISABLE(IS_PERL_OK_RESULT)
                MESSAGE("Warning: Arcadia build needs Perl compiled with -DMULTIPLICITY")
            ENDIF ()
        ELSE ()
            DISABLE(IS_PERL_OK_RESULT)
            IF (${ARGC} GREATER 1)
                MESSAGE("${ARGN}")
            ENDIF ()
        ENDIF ()
        SET(IS_PERL_OK_RESULT ${IS_PERL_OK_RESULT} CACHE BOOL "IS_PERL_OK result")
    ENDIF ()
    SET(${__res_} ${IS_PERL_OK_RESULT})
ENDMACRO ()

MACRO(IS_VALID_PERL __res1_ __perloutputvar_)
    STRING(REGEX REPLACE "\n" " " __perloutput_ "${${__perloutputvar_}}")
    STRING(REGEX MATCH "Compile-time options(.*)MULTIPLICITY" __re_result_ "${__perloutput_}")
    IF (__re_result_)
        ENABLE(${__res1_})
    ELSE ()
        DISABLE(${__res1_})
    ENDIF ()
ENDMACRO ()

MACRO(CHECK_PERL_VARS)
    IF (PERLXS OR PERLXSCPP OR USE_PERL) # || make(paths)
        IS_PERL_OK(__perl_exec_res_)
        IF (__perl_exec_res_)
            IF (NOT DEFINED PERLLIB)
                EXECUTE_PROCESS(OUTPUT_VARIABLE PERLLIB
                    COMMAND ${PERL} -V:archlib
                    RESULT_VARIABLE __perl_exec_res_
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                STRING(REGEX REPLACE "archlib='(.*)';" "\\1" PERLLIB "${PERLLIB}")
                FILE(TO_CMAKE_PATH ${PERLLIB} PERLLIB)
            ENDIF ()
            SET_APPEND(DTMK_I ${PERLLIB}/CORE)

            IF (NOT DEFINED PERLLIB_PATH)
                SET(PERLLIB_PATH ${PERLLIB}/CORE)
                IF (NOT EXISTS ${PERLLIB_PATH}/libperl.a)
#                       SET(PERLLIB_PATH /usr/lib)
#                       ENDIF ()
                ENDIF ()
                SET(PERLLIB_PATH ${PERLLIB_PATH} CACHE STRING "PERLLIB_PATH")
            ENDIF ()

            IF (NOT DEFINED EXTUT)
                EXECUTE_PROCESS(OUTPUT_VARIABLE EXTUT
                    COMMAND ${PERL} -V:privlibexp
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                STRING(REGEX REPLACE "privlibexp='(.*)';" "\\1" EXTUT "${EXTUT}")
                SET(EXTUT "${EXTUT}/ExtUtils")
                FILE(TO_CMAKE_PATH ${EXTUT} EXTUT)
            ENDIF ()

            IF (NOT DEFINED PERLSITEARCHEXP)
                EXECUTE_PROCESS(OUTPUT_VARIABLE PERLSITEARCHEXP
                    COMMAND ${PERL} -V:sitearchexp
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                STRING(REGEX REPLACE "sitearchexp='(.*)';" "\\1" PERLSITEARCHEXP "${PERLSITEARCHEXP}")
                FILE(TO_CMAKE_PATH ${PERLSITEARCHEXP} PERLSITEARCHEXP)
            ENDIF ()

            IF (NOT DEFINED PERLINSTALLSITEARCH)
                EXECUTE_PROCESS(OUTPUT_VARIABLE PERLINSTALLSITEARCH
                    COMMAND ${PERL} -V:installsitearch
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                STRING(REGEX REPLACE "installsitearch='(.*)';" "\\1" PERLINSTALLSITEARCH "${PERLINSTALLSITEARCH}")
                FILE(TO_CMAKE_PATH "${PERLINSTALLSITEARCH}" PERLINSTALLSITEARCH)
                SET(PERLINSTALLSITEARCH "${PERLINSTALLSITEARCH}" CACHE STRING "perl -V:installsitearch")
            ENDIF ()

            IF (DEFINED USE_PERL_5_6)
                SET_APPEND(DTMK_D -DPERL_POLLUTE)
            ENDIF ()
        ELSE ()
            DEBUGMESSAGE(1 "FindPerl2.cmake: perl executable is not found")
        ENDIF ()
    ENDIF ()
ENDMACRO ()

CHECK_PERL_VARS()


#
# BUILDWITH_PERL script dstfile commandargs
#   Tip: place NODEPEND keyword before an argument to remove it from DEPENDS section
#        (used for non-srcfile perlscript arguments)
#
MACRO (BUILDWITH_PERL script dstfile commandargs)
    CHECK_PERL_VARS()

    IS_PERL_OK(__perl_exec_res_)
    IF (NOT __perl_exec_res_)
        MESSAGE(SEND_ERROR "BUILDWITH_PERL macro: perl not found")
    ENDIF ()

    SET(__cmdargs ${commandargs} ${ARGN})
    SET(__srcfiles)
    SET(__depfiles)
    SET(__next_nodepend "no")
    SET(__next_depend "no")
    FOREACH(__item_ ${__cmdargs})
	    IF (__next_nodepend)
		    SET_APPEND(__srcfiles ${__item_})
		    SET(__next_nodepend "no")
	    ELSEIF (__next_depend)
		    SET_APPEND(__depfiles ${__item_})
		    SET(__next_depend "no")
	    ELSE ()
		    IF ("${__item_}" MATCHES "NODEPEND")
			    SET(__next_nodepend "yes")
		    ELSEIF ("${__item_}" MATCHES "DEPEND")
			    SET(__next_depend "yes")
		    ELSE ()
			    SET_APPEND(__srcfiles ${__item_})
			    SET_APPEND(__depfiles ${__item_})
		    ENDIF ()
	    ENDIF ()
    ENDFOREACH ()
    #SEPARATE_ARGUMENTS_SPACE(__srcfiles)
    ADD_CUSTOM_COMMAND(
	    OUTPUT "${dstfile}"
	    PRE_BUILD
	    COMMAND ${PERL} ${script} ${__srcfiles} > ${dstfile} || ${RM} ${RM_FORCEFLAG} ${dstfile}
	    MAIN_DEPENDENCY "${script}"
	    DEPENDS ${script} ${__depfiles}
	    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	    COMMENT "Building ${dstfile} with perl"
	)
    SOURCE_GROUP("Custom Builds" FILES ${script})
    SOURCE_GROUP("Generated" FILES ${dstfile})
ENDMACRO ()

#
#
#
MACRO (BUILDWITH_PERLXSCPP srcfile dstfile)
    ENABLE(PERLXSCPP)
    CHECK_PERL_VARS()

    IS_PERL_OK(__perl_exec_res_)
    IF (NOT __perl_exec_res_)
        MESSAGE_EX("BUILDWITH_PERLXSCPP macro: perl not found")
        MESSAGE(SEND_ERROR)
    ENDIF ()

#	IF (PERLXSCPP)
		ADD_CUSTOM_COMMAND(
			OUTPUT ${dstfile}
			PRE_BUILD
			COMMAND ${PERL} ${EXTUT}/xsubpp -typemap ${EXTUT}/typemap -csuffix .cpp ${XSUBPPFLAGS} ${srcfile} > ${dstfile} || ${RM} ${RM_FORCEFLAG} ${dstfile}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			MAIN_DEPENDENCY "${srcfile}"
			DEPENDS ${srcfile} ${ARGN}
			COMMENT "Building ${dstfile} with perl (.xs.cpp)"
			)
#	ENDIF ()
	SOURCE_GROUP("Custom Builds" FILES ${srcfile})
	SOURCE_GROUP("Generated" FILES ${dstfile})
    IF (NOT WIN32)
        SET_SOURCE_FILES_PROPERTIES(${dstfile} PROPERTIES
            COMPILE_FLAGS -Wno-unused
        )
    ENDIF ()
ENDMACRO ()

MACRO (BUILDWITH_PERLXS srcfile dstfile)
    ENABLE(PERLXS)
    CHECK_PERL_VARS()

    IS_PERL_OK(__perl_exec_res_)
    IF (NOT __perl_exec_res_)
        MESSAGE(SEND_ERROR "BUILDWITH_PERLXS macro: perl not found")
    ENDIF ()

#	IF (PERLXS)
		ADD_CUSTOM_COMMAND(
			OUTPUT ${dstfile}
			PRE_BUILD
			COMMAND ${PERL} ${EXTUT}/xsubpp -typemap ${EXTUT}/typemap ${XSUBPPFLAGS} ${srcfile} > ${dstfile} || ${RM} ${RM_FORCEFLAG} ${dstfile}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			MAIN_DEPENDENCY "${srcfile}"
			DEPENDS ${srcfile} ${ARGN}
			COMMENT "Building ${dstfile} with perl (.xs.c)"
			)
#	ENDIF ()
	SOURCE_GROUP("Custom Builds" FILES ${srcfile})
	SOURCE_GROUP("Generated" FILES ${dstfile})
ENDMACRO ()
ENDIF ()
