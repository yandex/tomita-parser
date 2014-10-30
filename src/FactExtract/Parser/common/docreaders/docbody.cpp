#include "docbody.h"
#include "docstreambase.h"

CDocBody::CDocBody()
{
    m_unloadDocCheckSum = 0;
}

CDocBody::~CDocBody()
{

}

ui32  CDocBody::GetUnloadDocCheckSum() const
{
    return m_unloadDocCheckSum;
}

void CDocBody::PutUnloadDocCheckSum(ui32 unloadDocCheckSum)
{
    m_unloadDocCheckSum = unloadDocCheckSum;
}

void CDocBody::Reset()
{
    CDocBodyBase::Reset();
    m_unloadDocCheckSum = 0;
}

//bool CDocBody::ConvertToHTML(CString& strConverter)
//{
//    CConvertHTML converter;

//    converter.SetAllocator(&m_Allocator, s_MemID);
//    char* newBody;
//    long iNewLen = m_iBodyLen;
//    if( !converter.MainDeal(strConverter, (char*)m_pBody, iNewLen, &newBody) )
//        return false;

//    if( newBody == NULL)
//        return false;

//    m_iBodyLen = iNewLen;
//    m_Allocator.Free(s_MemID, m_pBody);
//    m_pBody = (BYTE*)newBody;
//    if( converter.GetNewCode() != -1 )
//        m_iCharset = converter.GetNewCode();
//    return true;
//}
