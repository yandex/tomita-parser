MACRO (CPP_PREPROCESS srcfile dstfile)
    IF (WIN32)
        SET(__cpp_flags /E /TP) #/C #${CMAKE_CXX_FLAGS} is not working  #that is terrible wrong!!!
    ELSE ()
         IF (CMAKE_GENERATOR STREQUAL "Ninja")
             SET(__cpp_flags ${COMMON_CXXFLAGS} -E -x c++) # $(CXX_FLAGS) works for make only, this is a hack to make it work for ninja
         ELSE ()
             SET(__cpp_flags $(CXX_DEFINES) $(CXX_FLAGS) -E -x c++) # /C #${CMAKE_CXX_FLAGS} is not working
         ENDIF ()
    ENDIF ()

    ADD_CUSTOM_COMMAND(
        OUTPUT
            ${dstfile}
        COMMAND
            ${CMAKE_CXX_COMPILER} ${__cpp_flags} -I ${ARCADIA_ROOT} -I ${ARCADIA_BUILD_ROOT} -I ${ARCADIA_ROOT}/contrib/libs/stlport/stlport-${CURRENT_STLPORT_VERSION} -I ${ARCADIA_ROOT}/contrib/libs/stlport/stlport-${CURRENT_STLPORT_VERSION}/stlport ${srcfile} > ${dstfile}
        DEPENDS
            ${srcfile} ${ARGN}
        COMMENT
            "Building ${dstfile} with cpp preprocessor"
    )
ENDMACRO ()

MACRO (EXTRACT_STRUCT_INFO srcfile dstfile)
    RUN_PROGRAM(tools/structparser -o ${dstfile} ${EXTRACT_STRUCT_INFO_FLAGS} ${srcfile} STDOUT ${dstfile}.log IN ${srcfile} ${ARGN} OUT ${dstfile})
ENDMACRO ()

MACRO (GEN_SCHEEME2 scheeme_name fromfile)
    SET(__fromfile_ "${CMAKE_CURRENT_SOURCE_DIR}/${fromfile}")
    CPP_PREPROCESS(${__fromfile_} ${BINDIR}/${scheeme_name}_dbrecords.cph ${ARGN})

    #    --gcc44_no_typename  is conformance to C++03, but in c++11 typename is allowed outside template,
    # main reason for typename is flock, pthread_once, sigaction, sigstack, sigvec, stat, timezone, both c function and structures i.e. struct a; void a(); valid C++ code
    # and so for referencing a better to point out that we means typename  and not a function.
    # visual studio 2008 allows typename in template parameters (outside template!), but not in typedef

    SET(EXTRACT_STRUCT_INFO_FLAGS -f \"const static ui32 RecordSig\" -u \"RecordSig\" -n N${scheeme_name}SchemeInfo --gcc44_no_typename --no_complex_overloaded_func_export ${DATAWORK_SCHEEME_EXPORT_FLAGS})

    EXTRACT_STRUCT_INFO(${BINDIR}/${scheeme_name}_dbrecords.cph ${BINDIR}/${scheeme_name}.inc ${ARGN})
    SOURCE_GROUP("Custom Builds" FILES ${__fromfile_} )
    SOURCE_GROUP("Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/${scheeme_name}.inc )
ENDMACRO ()
