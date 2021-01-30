SET(ROOT_PROJECTS
        bindings
        contrib
        devtools
        dict
        FactExtract
        kernel
        library
        tools
        util
)

find_package(JNI)

IF (JNI_FOUND)
    MESSAGE ( STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}" )
    MESSAGE ( STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}" )
ELSE ()
    MESSAGE ( FATAL_ERROR "Failed to locate JNI headers. Check the jdk installation path." )
ENDIF ()

IF (YMAKE)
    IF (ARC)
        ARCPLUGINS(
            devtools/arc/rules
        )
    ENDIF ()

    RECURSE(
        ${ROOT_PROJECTS}
    )
ELSE ()
    CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
    CMAKE_POLICY(VERSION 2.6.0)

    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/include/global.cmake NO_POLICY_SCOPE)

    SET(NOLINK yes)
    SET(MAKE_ONLY "FactExtract/Parser/tomita-parser" CACHE STRING "Targets to make")

    IF (MAKE_ONLY)
        SET(__to_make_)
        FOREACH(__item_ ${MAKE_ONLY})
            IF ("${__item_}" STREQUAL "all")
                SET_APPEND(__to_make_ ${ROOT_PROJECTS})
            ELSE ()
                IF (NOT EXISTS ${ARCADIA_ROOT}/${__item_})
                    MESSAGE(SEND_ERROR "Directory \"${__item_}\" (listed in MAKE_ONLY) does not exist.")
                ELSE ()
                    SET_APPEND(__to_make_ ${__item_})
                ENDIF ()
            ENDIF ()
        ENDFOREACH()
        IF (__to_make_)
            RECURSE(
                ${__to_make_}
            )
        ENDIF ()
    ELSE ()
        RECURSE(
            ${ROOT_PROJECTS}
        )
    ENDIF ()

    PROJECT_EX(all_arcadia)
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/include/dtmk.cmake)

    # we have the old CMake in extlib!
    IF (NOT ROBOT_BUILD)
        ### This strings must locate only at the end of this file!!!
        FILE(READ ${TEST_DART_TMP_FILENAME} test_dart_content)
        FILE(WRITE ${TEST_DART_FILENAME} ${test_dart_content})
        FILE(REMOVE ${TEST_DART_TMP_FILENAME})
        ###
    ENDIF ()

    IF (COMMAND ON_CMAKE_FINISH_HOOK)
        ON_CMAKE_FINISH_HOOK()
    ENDIF ()
    # DBG_ALL_DEP_DIRS()
ENDIF()
