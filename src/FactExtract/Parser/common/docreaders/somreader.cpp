#include "somreader.h"
#include <iostream>

using namespace std;

#ifdef _win_
    #include <fcntl.h>
    #include <io.h>
#endif //_win_

CSomReader::CSomReader():
    m_Date(0),
    CurrentDocNumber(-1)
{

}

CSomReader::~CSomReader(void)
{
}

void CSomReader::Init(const Stroka& fileName)
{
    TXmlDocPtr piXmlDoc = TXmlDocPtr::ParseFromFile(fileName);
    if (piXmlDoc.Get() == NULL)
        ythrow yexception() << "Cannot load som-file " << fileName << ".";

    TXmlNodePtrBase piList = piXmlDoc->children;
    for (; piList.Get() != NULL; piList = piList->next) {
        if (piList.HasName("textset"))
            break;
    }
    piList = piList->children;
    Wtroka tmpbuf;
    for (; piList.Get() != NULL; piList = piList->next) {
        CSomReader::SSomDoc somDoc;
        if (!piList.HasName("text") || piList->type != XML_ELEMENT_NODE)
            continue;
        somDoc.Url = piList.GetAttr("id");
        somDoc.Text = WideToChar(UTF8ToWide(piList.GetNodeContent(), tmpbuf));
        Docs.push_back(somDoc);
    }
}

bool CSomReader::GetNextDocInfo(Stroka& /*strFilePath*/, SDocumentAttribtes& attrs)
{
    try {
        attrs.Reset();
        ++CurrentDocNumber;

        if(CurrentDocNumber >= Docs.ysize())
            return false;

        attrs.m_iDocID = CurrentDocNumber;
        attrs.m_strSource = "som";
        attrs.m_strUrl = Docs[CurrentDocNumber].Url;

        if (m_Date != 0)
            attrs.m_Date = m_Date;
        else
            attrs.m_Date = time(0);
    } catch (...) {
        cerr << "Error in \"CSomReader::GetNextDocInfo\" proccessing DocN " << attrs.m_iDocID << "\n";
    }

    return true;
}

bool CSomReader::GetDocBodyByAdress(const Stroka& /*strAddr*/, CDocBodyBase& body)
{
    return body.FillBody(*this, CODES_UTF8);
}

bool CSomReader::ReadInBuffer(ui8* p_buffer)
{
    memcpy(p_buffer, Docs[CurrentDocNumber].Text.c_str(), Docs[CurrentDocNumber].Text.size());
    return true;
}

long CSomReader::GetBufferLen()
{
    return Docs[CurrentDocNumber].Text.size();
}

int CSomReader::GetCharset()
{
    return 2;
}
