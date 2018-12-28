OWNER(lazyfrog)

DLL()

include_directories(${JNI_INCLUDE_DIRS})

foreach(dir ${JNI_INCLUDE_DIRS})
    if(IS_DIRECTORY ${dir})
        SET(subdir "${dir}/linux")
        MESSAGE( STATUS "Additionally JNI includes: " ${subdir} )
        include_directories(${subdir})
    endif()
endforeach()

PEERDIR(
    FactExtract/Parser/textminerlib
)

SRCS(
    jnihelper.cpp
    jconfig.cpp
    jcontext.cpp
    jtextminer.cpp
    jtextminerlib.cpp
)

END()
