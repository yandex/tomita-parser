#include "sentencebase.h"

CSentenceBase::CSentenceBase()
{
    m_pos = 0;
    m_len = 0;
    m_bPBreak = false;
    m_2sum = false;
    m_vn = 0;
    m_pErrorStream = NULL;
}

CSentenceBase::~CSentenceBase(void)
{
}

void CSentenceBase::PrintError(const Stroka& msg, const yexception* error)
{
    if (m_pErrorStream == NULL)
        return;

    Wtroka strSent;
    ToString(strSent);

    (*m_pErrorStream) << msg;
    if (error != NULL)
        (*m_pErrorStream) << ":\n\t" << error->what();
    (*m_pErrorStream) << "\nin sentence:\n\t" << NStr::Encode(strSent, CGramInfo::s_DebugEncoding) << "\n";
    m_pErrorStream->Flush();
}

