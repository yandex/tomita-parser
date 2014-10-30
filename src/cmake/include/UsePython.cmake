DEFAULT(USE_ARCADIA_PYTHON yes)

MACRO(PYTHON_ADDINCL)
    IF (USE_ARCADIA_PYTHON)
        SET(PYTHON_INCLUDE_PATH ${ARCADIA_ROOT}/contrib/tools/python/src/Include ${ARCADIA_ROOT}/contrib/tools/python/src)
    ELSE ()
        FIND_PACKAGE(PythonLibs)
        IF (NOT PYTHON_INCLUDE_PATH)
            MESSAGE("Python libs not found. Make sure Python development package (python-dev) is installed")
        ENDIF ()
    ENDIF ()
    INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
ENDMACRO ()

MACRO(ADD_PYTHON_CONFIG_FLAGS)
    EXECUTE_PROCESS(
        COMMAND python-config --cflags --ldflags
        OUTPUT_VARIABLE PYTHON_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    CFLAGS(${PYTHON_FLAGS})
ENDMACRO ()

MACRO(USE_PYTHON)
    PYTHON_ADDINCL()

    IF (USE_ARCADIA_PYTHON)
        PEERDIR(contrib/tools/python/lib)
    ELSE ()
        SET_APPEND(OBJADDE ${PYTHON_LIBRARIES})
        ADD_PYTHON_CONFIG_FLAGS()
    ENDIF ()
ENDMACRO ()

MACRO (PYMODULE)
    PLUGIN(${ARGV})
    IF (NOT USER_PREFIX)
        SET(LOCAL_SHARED_LIBRARY_PREFIX "")
    ENDIF ()

    PYTHON_ADDINCL()

    IF (WIN32)
        SET(LOCAL_SHARED_LIBRARY_PREFIX "lib")
        SET(LOCAL_SHARED_LIBRARY_SUFFIX ".pyd")
    ENDIF ()

ENDMACRO ()

MACRO (ADD_PY_TEST test_name script_rel_path)
    IF (USE_ARCADIA_PYTHON)
        TOOLDIR_EX(
            contrib/tools/python/bootstrap arcpython
        )
        SET(__binary_path_ ${arcpython})
    ELSE ()
        SET(__binary_path_ ${PYTHON_EXECUTABLE})
    ENDIF ()

    GETTARGETNAME(__project_bin_path_)
    GET_FILENAME_COMPONENT(__project_filename ${__project_bin_path_} NAME)
    ADD_YTEST_BASE(${test_name} ${script_rel_path} ${__binary_path_} ${__project_filename} ${ARGN})
ENDMACRO ()
