#include <stdexcept>

#include <util/system/progname.h>
#include <FactExtract/Parser/textminerlib/processor.h>
#include "generated/com_srg_tomita_TomitaParserInternal.h"
#include "jbuffer.h"
#include "jnihelper.h"
#include "jconfig.h"
#include "jtextminer.h"
#include "jparameters.h"
#include "jconfig.h"

static jint JNI_VERSION = JNI_VERSION_1_8;

static JNIHelper JNI_HELPER;
// static JTextMiner *textMiner;

/*
Эта функция будет вызвана JVM при загрузке динамической библиотеки.
Здесь мы создаем и инициализируем объект textMiner
*/
jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    printf("The text miner library is going to load...\n");

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION) != JNI_OK) {
        return JNI_ERR;
    }

    JNI_HELPER.SetJniEnv(env);
    JNI_HELPER.Init();

    printf("The text miner library has successfully loaded\n");

    return JNI_VERSION;
}

/*
Эта функция будет вызвана JVM при выгрузке динамической библиотеки. Здесь мы освообождаем ресурсы,
инициализированные при загрузке библиотеки.
*/
void JNI_OnUnload(JavaVM *vm, void* /*reserved*/)
{
    printf("The text miner library is going to unload...\n");

    JNIEnv* env;
    vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION);

    JNI_HELPER.SetJniEnv(env);
    JNI_HELPER.Destroy();

    printf("The text miner library has successfully unloaded\n");
}

JNIEXPORT jlong JNICALL Java_com_srg_tomita_TomitaParserInternal_create(
    JNIEnv *env,            // объект JNI, необходимый для работы с ява-ресурсами
    jclass /*clazz*/,
    jobjectArray params,    // параметры конфигурации - те же самые, что при вызове парсера с командной строки
    jstring confDir         // путь к директории с конфигами парсера, должен завершаться системным "/"
)
{
    printf("Trying to create the parser instance...\n");

    JNI_HELPER.SetJniEnv(env);

    JStringArray _params = JNI_HELPER.StringArray();
    _params.SetBuffer(params);

    JString _confDir = JNI_HELPER.String();
    _confDir.SetBuffer(confDir);

    JTextMiner *textMiner = new JTextMiner();

    textMiner->GetConfig().SetParameters(_params);
    textMiner->GetConfig().SetConfigurationDirectory(_confDir);

    try {
        textMiner->Configure();
    } catch (const std::runtime_error& error) {
        JNI_HELPER.throwException(error.what());
        return NULL;
    }

    printf("The parser instance %ld has successfully created\n", (jlong) textMiner);

    return (jlong) textMiner;
}

JNIEXPORT void JNICALL Java_com_srg_tomita_TomitaParserInternal_dispose
  (JNIEnv *env, jclass, jlong textMinerAddress)
{
    if (textMinerAddress == 0) {
        printf("The parser instance %ld is disposed already\n", textMinerAddress);
        return;
    }

    printf("The parser instance %ld is going to dispose...\n", textMinerAddress);

    JNI_HELPER.SetJniEnv(env);

    JTextMiner *textMiner = (JTextMiner*) textMinerAddress;
    delete textMiner;

    printf("The parser instance %ld has successfully disposed\n", textMinerAddress);
}

/*
Эта функция для вызова парсера. Вызывается из класса com.srg.tomita.TomitaParserInternal. Принимает
на вход параметры конфигурации парсера и данные для парсина. Возвращает результаты парсинга в метод
TomitaParserInternal.resultCallback (идентификатор этого метода закеширован в переменной
javaCallbackId при загрузке динамической библиотеки)
*/
JNIEXPORT jbyteArray JNICALL Java_com_srg_tomita_TomitaParserInternal_parse(
        JNIEnv *env,            // объект JNI, необходимый для работы с ява-ресурсами
        jclass /*clazz*/,
        jlong textMinerAddress,   // адрес парсера
        jbyteArray inputBuffer, // входные документы, которые будут парсится
        jintArray inputLengths  // длины входных документов - для нахождения границ между документами в inputBuffer
)
{
    printf("The parser instance %ld is going to parse input documents...\n", textMinerAddress);

    JNI_HELPER.SetJniEnv(env);

    JTextMiner *textMiner = (JTextMiner*) textMinerAddress;

    JByteArray _inputBuffer = JNI_HELPER.ByteArray();
    _inputBuffer.SetBuffer(inputBuffer);

    JIntArray _inputLengths = JNI_HELPER.IntArray();
    _inputLengths.SetBuffer(inputLengths);

    CBuffer result;
    try {
        textMiner->Parse(_inputBuffer, _inputLengths, result);
    } catch (const std::runtime_error& error) {
        printf("The parser instance %ld has failed to parse input documents\n", textMinerAddress);
        JNI_HELPER.throwException(error.what());
        return NULL;
    }

    printf("The parser instance %ld has successfully parse input documents\n", textMinerAddress);

    // Создаем сырой (необернутый в JByteArray) буфер jbyteArray - для того, чтобы RAII не собрал его со стека
    // (иначе данные не уйдут в jvm)
    jbyteArray ret = env->NewByteArray(result.m_length);
    env->SetByteArrayRegion(ret, 0, result.m_length, (const jbyte*) *result);
    return ret;
}
