#include "doclistretrieverfromdisc.h"

#include <FactExtract/Parser/common/timehelper.h>
#include <FactExtract/Parser/common/pathhelper.h>

bool CDocListRetrieverFromDisc::Init(const Stroka& strSearchDir)
{
    m_iCurPath = 0;
    m_strSearchDir = strSearchDir;
    m_SmartFileFind.FindFiles(strSearchDir, true);
    return true;
}

void CDocListRetrieverFromDisc::TransformBackSlash(Stroka& str)
{
    PathHelper::CorrectSep(str);
}

void CDocListRetrieverFromDisc::FillDocInfo(SDocumentAttribtes& attrs)
{
    Stroka strFilePath = m_SmartFileFind.GetFoundFilePath(m_iCurPath);

    Stroka strURL;
    if (strFilePath == m_strSearchDir) {
        TStringBuf left, right;
        PathHelper::Split(strFilePath, left, right);
        strURL = ToString(right);
    } else
        strURL = strFilePath.substr(m_strSearchDir.size());

    if (strURL.empty())
        ythrow yexception() << "Can't build url for file \"" << strFilePath
                              << "\" with searchdir \"" << m_strSearchDir << "\".";

    TransformBackSlash(strURL);

    attrs.m_strUrl = strURL;

    Stroka strTime;
    if (stroka(m_strLTM) == "file") {
        CTime lw_time = m_SmartFileFind.GetFoundFileInfo(m_iCurPath).m_LastWriteTime;
        strTime = lw_time.Format("%d.%m.%Y %H:%M:%S");
    }

    if (strTime.empty())
        strTime = m_strStartTime;

    attrs.m_strTitle = CharToWide(strTime);
    attrs.m_strSource = strURL;
    attrs.m_strTitle = CharToWide(attrs.m_strSource);     // ??? rewriting
}

bool CDocListRetrieverFromDisc::GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs)
{
    attrs.Reset();

    if (m_strSearchDir.empty())
        return false;

    if (m_iCurPath >= (int)m_SmartFileFind.GetFoundFilesCount())
        return false;

    strFilePath = m_SmartFileFind.GetFoundFilePath(m_iCurPath);
    FillDocInfo(attrs);

    m_iCurPath++;

    attrs.m_Date = m_Date;

    return true;
}

void CDocListRetrieverFromDisc::Close()
{
    m_SmartFileFind.Reset();
}
