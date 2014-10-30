#include "agencyinforetriever.h"

#include <FactExtract/Parser/common/string_tokenizer.h>

#include <util/folder/dirut.h>
#include <util/string/strip.h>

bool CAgencyInfoRetriver::Init(const Stroka& strDoc2AgFile)
{
    if (!strDoc2AgFile.empty() && isexist(strDoc2AgFile.c_str()))
        m_DocId2AgNameFile = TFile(strDoc2AgFile.c_str(), OpenExisting | RdOnly);

    return true;
}

bool CAgencyInfoRetriver::GetAgNameByDoc(int iDoc, int& agId, Stroka& agName)
{
    if (!m_DocId2AgNameFile.IsOpen())
        return false;
    if (m_iLast_d2a.DocId != -1) {
        if (iDoc == m_iLast_d2a.DocId) {
            agId = m_iLast_d2a.AgId;
            agName = m_iLast_d2a.AgName;
            m_iLast_d2a.DocId = -1;
            return true;
        } else {
            return false;
        }
    }

    TDoc2Ag d2a;
    Stroka s;
    bool bFound = false;
    TBufferedFileInput file(m_DocId2AgNameFile);
    while (file.ReadLine(s)) {
        TStrokaTokenizer tokenizer(s, "\t");

        if (!tokenizer.NextAsNumber(d2a.DocId) || !tokenizer.NextAsNumber(d2a.AgId) || !tokenizer.NextAsString(d2a.AgName))
            ythrow yexception() << "Tab delimited DocId, AgencyId, AgencyName are expected in d2a file";

        d2a.AgName = ToString(StripString(d2a.AgName));
        if (d2a.AgName.length() <= 1)
            ythrow yexception() << "Emtpy AgencyName for AgencyId = " << d2a.AgId << " was found in d2a file";

        if (d2a.DocId > iDoc) {
            m_iLast_d2a = d2a;
            break;
        } else if (d2a.DocId == iDoc) {
            bFound = true;
            break;
        }
    }

    if (bFound) {
        agId = d2a.AgId;
        agName = d2a.AgName;
        return true;
    } else
        return false;
}

