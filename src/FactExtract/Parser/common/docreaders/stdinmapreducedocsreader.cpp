#include "stdinmapreducedocsreader.h"
#include <iostream>

#ifdef _win_
#include <fcntl.h>
#include <io.h>
#endif //_win_

CStdinMapReduceDocsReader::CStdinMapReduceDocsReader(ECharset encoding, Stroka subkey)
: Encoding(encoding)
, DocLen(0)
, CurrentDocNumber(0)
, Subkey(subkey)
, LastDocNumber(Max<size_t>())
{
#ifdef _win_
    setmode(fileno(stdin), O_BINARY);
#endif
}

CStdinMapReduceDocsReader::~CStdinMapReduceDocsReader()
{
#ifdef _win_
    setmode(fileno(stdin), O_TEXT);
#endif
}

// read length-value (throws yexception or TLoadEOF)
void CStdinMapReduceDocsReader::ReadLV(TInputStream& is, Stroka& value)
{
    ::Load(&is, value);
    if (value.size() == 0 || value.size() > (1 << 27))
        ythrow yexception() << "bad data size wile reading MapReduce length-value output (" << value.size() << " bytes)";
}

bool CStdinMapReduceDocsReader::GetNextDocInfo(Stroka& /*strFilePath*/, SDocumentAttribtes& attrs)
{
    try {
        attrs.Reset();

        Stroka key;
        Stroka subKey;
        Stroka value;

        try { // let's catch exception at EOF

            if (Subkey == "") // no subkey used
                ReadLV(Cin, key);
            else {
                while (true) {
                    ReadLV(Cin, key);
                    ReadLV(Cin, subKey);
                    if (subKey == Subkey)
                        break; // right subkey found
                    else
                        ReadLV(Cin, value);
                }
            }
            DocLen = LoadSize(&Cin);

        } catch (yexception& ) {
            return false;
        }

        attrs.m_iDocID = ++CurrentDocNumber;
        attrs.m_strUrl = key;
        size_t pos = attrs.m_strUrl.find('/');
        attrs.m_strSource = (pos == Stroka::npos) ? attrs.m_strUrl : attrs.m_strUrl.substr(0, pos);

        ++CurrentDocNumber;
        if (CurrentDocNumber > LastDocNumber)
            return false;

        GetAgNameByDoc(attrs.m_iDocID, attrs.m_iSourceID, attrs.m_strSource);

        if (attrs.m_strSource.empty())
            attrs.m_strSource = attrs.m_strUrl;

    } catch (yexception& e) {
        Cerr << e.what() << Endl;
    } catch (...) {
        Cerr << "Error in \"CStdinMapReduceDocsReader::GetNextDocInfo\" proccessing DocN " << attrs.m_iDocID << Endl;
    }

    return true;
}

bool CStdinMapReduceDocsReader::GetDocBodyByAdress(const Stroka& /*strAddr*/, CDocBodyBase& body)
{
    return body.FillBody(*this, Encoding);
}

bool CStdinMapReduceDocsReader::ReadInBuffer(ui8* p_buffer)
{
    ::LoadArray(&Cin, p_buffer, DocLen);
    return true;
}

long CStdinMapReduceDocsReader::GetBufferLen()
{
    return DocLen;
}

int CStdinMapReduceDocsReader::GetCharset()
{
    return 2;
}
