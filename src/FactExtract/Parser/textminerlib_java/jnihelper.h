#pragma once

#include "generated/com_srg_tomita_TomitaParserInternal.h"
#include "jbuffer.h"

class JNIHelper
{
public:
    JNIHelper() { }
    virtual ~JNIHelper() { }

    void SetJniEnv(JNIEnv* env);

    void Init();

    void Destroy();

    jint throwException(const char *message);

    JByteArray ByteArray();
    JIntArray IntArray();
    JString String();
    JStringArray StringArray();

private:
    JNIEnv* m_Env;

    // Класс java исключения
    jclass m_javaExceptionClass;
};
