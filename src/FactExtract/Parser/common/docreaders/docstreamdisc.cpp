#include "docstreamdisc.h"

int CDocStreamDisc::s_BufSize = 1000000;

CDocStreamDisc::CDocStreamDisc(const CCommonParm& parm)
    : m_Parm(parm)
{
    Encoding = parm.GetInputEncoding();
}

CDocStreamDisc::~CDocStreamDisc()
{
}

bool  CDocStreamDisc::Init(const Stroka&)
{
    return true;
}

bool  CDocStreamDisc::AddAddressHint(const Stroka&)
{
    return false;
}

bool  CDocStreamDisc::GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body)
{
    try {
        m_CurFilePath = strAddr;
        if (!m_CurFile.Open(strAddr.c_str()))
            ythrow yexception() << "Can't open document " << strAddr << ".";

        bool bRes = body.FillBody(*this, Encoding);
        m_CurFile.Close();
        return bRes;
    } catch (...) {
        if (m_CurFile.IsOpen())
            m_CurFile.Close();
        throw;
    }
}

bool CDocStreamDisc::ReadInBuffer(ui8* p_buffer)
{
    try {
        if (!m_CurFile.IsOpen())
            return false;
        m_CurFile.Read(p_buffer, (int)m_CurFile.Length());
        return true;
    } catch (...) {
        return false;
    }
}

long CDocStreamDisc::GetBufferLen()
{
    return (long)m_CurFile.Length();
}

int     CDocStreamDisc::GetCharset()
{
    Stroka ext(".");
    //  Проверим расширение прочитанного документа
    size_t k = m_CurFilePath.rfind('.');
    if (k != Stroka::npos)
        ext += m_CurFilePath.substr(k + 1);
    else
        return 2;

    stroka ext_ci(ext);
    if (ext_ci == ".htm" ||
        ext_ci == ".asp" ||
        ext_ci == ".htt" ||
        ext_ci == ".htw" ||
        ext_ci == ".htx" ||
        ext_ci == ".plg" ||
        ext_ci == ".stm" ||
        ext_ci == ".html")
        return 4;
    else
        return 2;
}

