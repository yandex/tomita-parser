#pragma once

#include <cstdlib>
#include <util/generic/stroka.h>
#include "generated/com_srg_tomita_TomitaParserInternal.h"


/*
Этот файл содержит классы для безопасной работы с ресурсами (RAII).
Классы оборачивают разного рода ресурсы (в основном JNI-буферы), освобождая их
при разрушении объектов.
*/

/*
Оборачивает обычный С-буфер
*/
class CBuffer
{
public:
    CBuffer()
    {
        m_bytes = NULL;
    }

    CBuffer(size_t length)
    {
        m_bytes = NULL;
        SetLength(length);
    }

    ~CBuffer()
    {
        if (m_bytes) {
            free(m_bytes);
        }
    }

    void SetLength(size_t length)
    {
        if (m_bytes) {
            free(m_bytes);
        }
        m_length = length;
        m_bytes = (char*) malloc(length);
    }

    void WriteFrom(CBuffer& buffer, size_t from, size_t length)
    {
        memcpy(m_bytes + from, buffer.m_bytes, length);
    }

    char* operator*() const {
        return m_bytes;
    }

    char* m_bytes;
    size_t m_length;
};

/*
Базовый класс для всех оберток JNI-буферов
*/
template <class BufType, class DataType>
class BaseBufferWrapper
{
public:
    BaseBufferWrapper()
    {
        m_buffer = NULL;
    }

    BaseBufferWrapper(JNIEnv *env)
    {
        m_env = env;
    }

    virtual ~BaseBufferWrapper()
    {
        if (m_buffer) {
            m_env->DeleteLocalRef(m_buffer);
        }
    }

    void SetBuffer(BufType buffer)
    {
        m_buffer = buffer;
    }

    DataType operator*() const {
        return m_bytes;
    }

    JNIEnv *m_env;
    BufType m_buffer;
    size_t m_length;
    DataType m_bytes;
};

/*
Обертка для jbyteArray
*/
class JByteArray : public BaseBufferWrapper<jbyteArray, jbyte*>
{
public:
    JByteArray(JNIEnv *env) : BaseBufferWrapper<jbyteArray, jbyte*>(env) { }

    ~JByteArray()
    {
        if (m_buffer) {
            m_env->ReleaseByteArrayElements(m_buffer, m_bytes, JNI_ABORT);
        }
    }

    void SetBuffer(jbyteArray buffer)
    {
        BaseBufferWrapper::SetBuffer(buffer);
        m_bytes = m_env->GetByteArrayElements(buffer, NULL);
        m_length = m_env->GetArrayLength(buffer);
    }

    void SetLength(size_t length)
    {
        SetBuffer(m_env->NewByteArray(length));
    }

    void WriteFrom(CBuffer& buffer, size_t from, size_t length)
    {
        m_env->SetByteArrayRegion(m_buffer, from, length, (const jbyte*) buffer.m_bytes);
    }
};

/*
Обертка для jintArray
*/
class JIntArray : public BaseBufferWrapper<jintArray, signed int *>
{
public:
    JIntArray(JNIEnv *env) : BaseBufferWrapper<jintArray, signed int *>(env) { }

    ~JIntArray() {
        if (m_buffer) {
            m_env->ReleaseIntArrayElements(m_buffer, m_bytes, JNI_ABORT);
        }
    }

    void SetBuffer(jintArray buffer)
    {
        BaseBufferWrapper::SetBuffer(buffer);
        m_bytes = m_env->GetIntArrayElements(buffer, NULL);
        m_length = m_env->GetArrayLength(buffer);
    }
};

/*
Обертка для jstring
*/
class JString : public BaseBufferWrapper<jstring, char *>
{
public:
    JString() : BaseBufferWrapper<jstring, char *>() { }

    JString(JNIEnv *env) : BaseBufferWrapper<jstring, char *>(env) { }

    ~JString() {
        if (m_buffer) {
            m_env->ReleaseStringUTFChars(m_buffer, m_bytes);
        }
    }

    void SetBuffer(jstring str)
    {
        BaseBufferWrapper::SetBuffer(str);
        m_bytes = (char*) m_env->GetStringUTFChars(str, 0);
        m_length = m_env->GetStringLength(str);
    }
};

/*
Обертка для jobjectArray, в котором лежат объекты типа jstring
*/
class JStringArray : public BaseBufferWrapper<jobjectArray, Stroka*>
{
public:
    JStringArray(JNIEnv *env) : BaseBufferWrapper<jobjectArray, Stroka*>(env) { }

    void SetBuffer(jobjectArray buffer)
    {
        BaseBufferWrapper::SetBuffer(buffer);

        m_length = m_env->GetArrayLength(buffer);
        m_bytes = new Stroka[m_length];

        for (size_t i = 0; i < m_length; i++) {
            jstring param = (jstring) m_env->GetObjectArrayElement(buffer, i);
            JString wrapper(m_env);
            wrapper.SetBuffer(param);
            m_bytes[i] = Stroka(wrapper.m_bytes);
        }
    }

    ~JStringArray()
    {
        if (m_buffer) {
            delete[] m_bytes;
        }
    }
};
