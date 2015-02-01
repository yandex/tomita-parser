#pragma once

#if !defined(FROM_IMPL)
#define PROGRAM_VERSION GetProgramSvnVersion()
#define ARCADIA_SOURCE_PATH GetArcadiaSourcePath()
#define PRINT_VERSION PrintSvnVersionAndExit(argc, (char**)argv)
#endif

#if defined(__cplusplus)
extern "C" {
#endif
    const char* GetProgramSvnVersion();
    const char* GetProgramVersionTag();
    void PrintProgramSvnVersion();
    const char* GetArcadiaSourcePath();
    const char* GetArcadiaSourceUrl();
    const char* GetArcadiaLastChange();
    const char* GetArcadiaLastAuthor();
    int GetProgramSvnRevision();
    void PrintSvnVersionAndExit(int argc, char *argv[]);
    void PrintSvnVersionAndExit0();
    const char* GetProgramScmData();
    const char* GetProgramBuildUser();
    const char* GetProgramBuildHost();
    const char* GetProgramBuildDate();
#if defined(__cplusplus)
}
#endif
