#include <cstdlib>

#include "jconfig.h"
#include "jparameters.h"

/*
    Поскольку объект у нас глобальный, то есть будет жить за пределами стека jni-вызова,
    то важно полностью скопировать jni-буфера, ибо они будут освобождены после текущего jni-вызова
*/
void JConfig::SetParameters(JStringArray& params)
{
    int argc = params.m_length + 1;
    char** argv = new char*[argc]; // Массив, с которого конфигурируется tomita

    argv[0] = "./tomita-parser"; // эмулируем первый элемент

    printf("Parser parameters (%d count):\n", params.m_length);

    for (size_t i = 0; i < params.m_length; i++) {
        Stroka param = (*params)[i];
        char* tmp = (char*) malloc((*params)[i].size() + 1);
        memcpy(tmp, (char*) param.c_str(), (*params)[i].size());
        tmp[(*params)[i].size()] = 0;
        argv[i+1] = tmp;
        printf("%s\n", argv[i+1]);
    }

    m_argc = argc;
    m_argv = argv;
}

/*
    Копируем буфер полностью (см. документацию к SetParameters)
*/
void JConfig::SetConfigurationDirectory(JString& confDir)
{
    char* tmp = (char*) malloc(confDir.m_length + 1);
    memcpy(tmp, *confDir, confDir.m_length);
    tmp[confDir.m_length] = 0;
    m_confDir = tmp;

    printf("Config directory: %s\n", m_confDir);
}
