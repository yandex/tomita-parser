#pragma once

#include <FactExtract/Parser/textminerlib/processor.h>

#include "generated/com_srg_tomita_TomitaParserInternal.h"
#include "jnihelper.h"
#include "jconfig.h"
#include "jcontext.h"

/*
Класс является мостом между java и Tomita-парсером, обеспечивая конфигурирование
и запуск парсера.

Парсер перед использованием должен быть сконфигурирован. Конфигурация заключатся
в установке требуемых параметров в объект JConfig, который можно получить методом GetConfig(),
и последующем вызове метода Configure().

Непосредственно парсинг заключается в вызове метода  Parse с передачей ему входных документов.
*/
class JTextMiner
{
public:
    JTextMiner();

    virtual ~JTextMiner();

    /*
        Возвращает конфиг
    */
    JConfig& GetConfig()
    {
        return m_config;
    }

    /*
        Конфигурация. Должна выполняться после установки
        небходимых параметров в конфиг.

        Возвращает true/false как признак успешной/неуспешной конфигурации.
    */
    void Configure();

    /*
        Разбор документов. Принимает буфер документов, следующими друг за другом без разделителя,
        и буфер с длинами документов.

        Параметры:
        inputBuffer буфер с документами
        inputLengths буфер с длинами документов
        return true/false как признак успешного/неуспешного разбора
    */
    void Parse(JByteArray& inputBuffer, JIntArray& inputLengths, CBuffer& outputBuffer);

private:
    JConfig m_config;
    CBuffer m_output_buffer;
    TMemoryOutput m_output;
    JContext m_context;
    CProcessor m_processor; // CProcessor должен идти самым последним!
};
