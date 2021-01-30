#include "jnihelper.h"
#include "jparameters.h"

void JNIHelper::SetJniEnv(JNIEnv* env)
{
    m_Env = env;
}

void JNIHelper::Init()
{
    jclass exceptionClassLocal = m_Env->FindClass(JAVA_EXCEPTION_CLASS);
    m_javaExceptionClass = (jclass) m_Env->NewGlobalRef(exceptionClassLocal);
    m_Env->DeleteLocalRef(exceptionClassLocal);
}

void JNIHelper::Destroy()
{
    m_Env->DeleteGlobalRef(m_javaExceptionClass);
}

jint JNIHelper::throwException(const char *message)
{
    return m_Env->ThrowNew(m_javaExceptionClass, message);
}

JByteArray JNIHelper::ByteArray()
{
    JByteArray buf(m_Env);
    return buf;
}

JIntArray JNIHelper::IntArray()
{
    JIntArray buf(m_Env);
    return buf;
}

JString JNIHelper::String()
{
    JString buf(m_Env);
    return buf;
}

JStringArray JNIHelper::StringArray()
{
    JStringArray buf(m_Env);
    return buf;
}
