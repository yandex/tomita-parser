#include "stdinyandexdocsreader.h"

#include <util/charset/codepage.h>

#include <iostream>

using namespace std;

CStdinYandexDocsReader::CStdinYandexDocsReader(TInputStream& stream, ECharset encoding)
    : Encoding(encoding)
    , file(stream)
    , CurrentDocNumber(0)
    , LastDocNumber(Max<size_t>())
{
}

CStdinYandexDocsReader::~CStdinYandexDocsReader()
{
}

const char DocsDelim[] = "~~~~~~ Document ";
const char DocTitleTag[] = "TITLE=";
const char DocUrlTag[] = "URL=";
const char DocTimeTag[] = "TIME=";
const char DocMimeType[] = "MIMETYPE=";
const char DocCharset[] = "CHARSET=";
const char DocLang[] = "lang=";

bool CStdinYandexDocsReader::GetNextDocInfo(Stroka& /*strFilePath*/, SDocumentAttribtes& attrs)
{
    try {
        attrs.Reset();
        bool bSuccess = true;

        while (bSuccess && CurrentLine.find(DocsDelim) != 0)
            bSuccess = file.ReadLine(CurrentLine);

        if (!bSuccess)
            return false;

        attrs.m_iDocID = atoi(CurrentLine.c_str() + strlen(DocsDelim));

        bSuccess = file.ReadLine(CurrentLine);
        bSuccess = file.ReadLine(CurrentLine);
        while (bSuccess && (attrs.m_strUrl == "" || CurrentLine != "")) {
            if (CurrentLine.find(DocTitleTag) == 0)
                attrs.m_strTitle = NStr::Decode(CurrentLine.c_str() + strlen(DocTitleTag), Encoding);
            else if (CurrentLine.find(DocUrlTag) == 0) {
                attrs.m_strUrl = CurrentLine.c_str() + strlen(DocUrlTag);
                size_t pos = attrs.m_strUrl.find('/');
                attrs.m_strSource = (pos == Stroka::npos) ? attrs.m_strUrl : attrs.m_strUrl.substr(0, pos);
            } else if (CurrentLine.find(DocTimeTag) == 0) {
                struct tm tmDate;
                DateParser.ParseDate(CurrentLine.c_str() + strlen(DocTimeTag), tmDate);
                attrs.m_Date = mktime(&tmDate);
            } else if (CurrentLine.find(DocMimeType) == 0) {
                attrs.m_MimeType = mimeByStr(CurrentLine.c_str() + strlen(DocMimeType));
            } else if (CurrentLine.find(DocCharset) == 0) {
                attrs.m_Charset = CharsetByName(CurrentLine.c_str() + strlen(DocCharset));
            } else if (CurrentLine.find(DocLang) == 0) {
                attrs.m_Language = LanguageByName(CurrentLine.c_str() + strlen(DocLang));
            }
            bSuccess = file.ReadLine(CurrentLine);
        }

        ++CurrentDocNumber;
        if (CurrentDocNumber > LastDocNumber)
            return false;

        GetAgNameByDoc(attrs.m_iDocID, attrs.m_iSourceID, attrs.m_strSource);

        if (attrs.m_strSource.empty())
            attrs.m_strSource = attrs.m_strUrl;
    } catch (yexception& e) {
        cerr << e.what() << "\n";
    } catch (...) {
        cerr << "Error in \"CStdinYandexDocsReader::GetNextDocInfo\" proccessing DocN " << attrs.m_iDocID << "\n";
    }

    return true;
}

bool CStdinYandexDocsReader::GetDocBodyByAdress(const Stroka& /*strAddr*/, CDocBodyBase& body)
{
    Body.clear();
    bool bSuccess = true;
    while (bSuccess && CurrentLine.find(DocsDelim) != 0) {
        Body += CurrentLine + "\n";
        bSuccess = file.ReadLine(CurrentLine);
    }
    return body.FillBody(*this, Encoding);
}

bool CStdinYandexDocsReader::ReadInBuffer(ui8* p_buffer)
{
    memcpy(p_buffer, Body.c_str(), Body.size());
    return true;
}

long CStdinYandexDocsReader::GetBufferLen()
{
    return Body.size();
}

int CStdinYandexDocsReader::GetCharset()
{
    return 2;
}
