#include <FactExtract/Parser/textminerlib/processor.h>

#include <stdexcept>

#include "jtextminer.h"
#include "jparameters.h"
#include "jconfig.h"

JTextMiner::JTextMiner()
    : m_output_buffer(OUTPUT_XML_BUFFER_LENGTH)
    , m_output(*m_output_buffer, m_output_buffer.m_length)
{

}

JTextMiner::~JTextMiner()
{
    m_context.ConfigureThreadLocals();
}

void JTextMiner::Configure()
{
    m_context.ConfigureThreadLocals();

    m_processor.m_Parm.setConfigDir(m_config.GetConfigurationDirectory());
    m_processor.m_OutStream = &m_output;

    if (!m_processor.Init(m_config.GetArgc(), m_config.GetArgv())) {
        printf("Configuration problem\n");
        m_processor.m_Parm.PrintParameters();
        throw std::runtime_error(
            std::string("Configuration problem: ") + (const char*) m_processor.m_Parm.m_strError.c_str());
    }

    // Если m_memoryReader отсутствует, значит конфиги неправильные
    if (!m_processor.Factory.m_memoryReader) {
        printf("The parser is not configured to read input data from memory!\n");
        throw std::runtime_error("The parser is not configured to read input data from memory!");
    }
}

void JTextMiner::Parse(JByteArray& _inputBuffer, JIntArray& _inputLengths, CBuffer& outputBuffer)
{
    m_context.ConfigureThreadLocals();

    m_output.Reset(*m_output_buffer, m_output_buffer.m_length);

    m_processor.Factory.m_memoryReader->Reset();
    m_processor.Factory.m_memoryReader->SetInputLengthsBuffer(*_inputLengths);
    m_processor.Factory.m_memoryReader->SetInputLengthsBufferSize(_inputLengths.m_length*sizeof(int));
    m_processor.Factory.m_memoryReader->SetInputBuffer((char*) *_inputBuffer);
    m_processor.Factory.m_memoryReader->SetInputBufferSize(_inputBuffer.m_length);

    if (!m_processor.Run()) {
        printf("Failed to parse document\n");
        throw std::runtime_error("Failed to parse document");
    }

    size_t bufferLen = m_output.Buf() - *m_output_buffer;
    outputBuffer.SetLength(bufferLen);
    outputBuffer.WriteFrom(m_output_buffer, 0, outputBuffer.m_length);
}
