#pragma once

#include <util/generic/pair.h>
#include <util/generic/stroka.h>
#include <util/charset/doccodes.h>


#include "utilit.h"

struct SDocumentAttribtes
{
    Stroka m_strUrl;
    Wtroka m_strTitle;
    Stroka m_strSource;
    int m_iSourceID;
    Wtroka m_strInterviewFio;
    int m_iDocID;
    time_t  m_Date;
    docLanguage m_Language;
    MimeTypes m_MimeType;
    ECharset m_Charset;
    TPair<int,int> m_TitleSents;//первое-последнее предложение, входящее в заголовок

    SDocumentAttribtes()
    {
        Reset();
        m_iSourceID = -1;
    };

    bool IsEmpty() const
    {
        return m_strUrl.empty() || m_strTitle.empty() || m_strSource.empty() || (m_Date == 0);
    }
    void Reset()
    {
        m_strUrl.clear();
        m_Date = 0;
        m_strTitle.clear();
        m_strSource.clear();
        m_iDocID = -1;
        m_TitleSents = MakePair(-1,-1);
        m_Language = LANG_UNK;
        m_MimeType = MIME_UNKNOWN;
        m_Charset = CODES_UNKNOWN;
    }

    Stroka DebugString() const {
        return Substitute("[URL=$0; DocId=$1; Title=\"$2\"]", m_strUrl, m_iDocID, NStr::DebugEncode(m_strTitle).substr(0, 50));
    }
};
