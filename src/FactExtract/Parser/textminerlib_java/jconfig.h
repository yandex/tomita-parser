#pragma once

#include <string>

#include "jbuffer.h"

class JConfig
{
public:
    JConfig() { }

    ~JConfig()
    {
        for (int i = 1; i < m_argc; i++) {
            delete[] m_argv[i];
        }
        delete[] m_argv;
        delete[] m_confDir;
    }

    char** GetArgv()
    {
        return m_argv;
    }

    size_t GetArgc()
    {
        return m_argc;
    }

    char* GetConfigurationDirectory()
    {
        return m_confDir;
    }

    void SetParameters(JStringArray& params);

    void SetConfigurationDirectory(JString& confDir);

private:
    char** m_argv;
    size_t m_argc;
    char* m_confDir;
};
