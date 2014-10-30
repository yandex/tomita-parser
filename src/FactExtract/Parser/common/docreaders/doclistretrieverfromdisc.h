#pragma once

#include "doclistretrieverbase.h"

#include <FactExtract/Parser/common/smartfilefind.h>

#include <util/generic/stroka.h>


class CDocListRetrieverFromDisc : public CDocListRetrieverBase
{
public:
    CDocListRetrieverFromDisc() : m_iCurPath(0) {};
    ~CDocListRetrieverFromDisc() {};

    virtual bool Init(const Stroka& strSearchDir);
    virtual bool GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs);
    virtual void Close();

    void PutLTMType(Stroka s) {m_strLTM = s;};
    void PutStartTime(Stroka s) {m_strStartTime = s;};
    time_t  m_Date;

protected:
    void FillDocInfo(SDocumentAttribtes& attrs);
    void TransformBackSlash(Stroka& str);

protected:
    Stroka m_strSearchDir;
    int    m_iCurPath;
    Stroka            m_strLTM;
    Stroka            m_strStartTime;
    CSmartFileFind m_SmartFileFind;

};
