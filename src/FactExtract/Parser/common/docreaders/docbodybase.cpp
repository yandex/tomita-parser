#include "docbodybase.h"
#include "docstreambase.h"

#include <FactExtract/Parser/common/utilit.h>


int  CDocBodyBase::s_MemID = 301;

CDocBodyBase::CDocBodyBase()
    : m_Body()
{
}

CDocBodyBase::~CDocBodyBase()
{
    Reset();
}

const Wtroka& CDocBodyBase::GetBody() const
{
    return m_Body;
}

int    CDocBodyBase::GetCharset() const
{
    return m_iCharset;
}

void CDocBodyBase::PutCharset(int iCharset)
{
    m_iCharset = iCharset;
}

void CDocBodyBase::Reset()
{
    m_Body.clear();
    m_iCharset = -1;
}

bool  CDocBodyBase::FillBody(CDocStreamBase& docStream, ECharset encoding)
{
    m_TmpBuffer.ReserveAndResize(docStream.GetBufferLen());
    m_iCharset = docStream.GetCharset();
    if (!docStream.ReadInBuffer(reinterpret_cast<ui8*>(m_TmpBuffer.begin())))
        return false;

    NStr::DecodeUserInput(m_TmpBuffer, m_Body, encoding);
    return true;
}
