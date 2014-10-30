#pragma once

#include <util/generic/stroka.h>
#include <util/generic/pair.h>
#include <util/stream/file.h>

class CAgencyInfoRetriver
{
public:
    bool Init(const Stroka& strDoc2AgFile);

protected:
    bool GetAgNameByDoc(int iDocId, int& agId, Stroka& agName);
    TFile m_DocId2AgNameFile;

    struct TDoc2Ag
    {
        int DocId;
        int AgId;
        Stroka AgName;

        TDoc2Ag()
            : DocId(-1)
            , AgId(-1)
            , AgName("")
        {
        }
    };

    TDoc2Ag    m_iLast_d2a;
};

