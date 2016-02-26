#include <util/stream/file.h>
#include <util/string/strip.h>

#include "tararchivereader.h"
#include "doclistretrieverbase.h"
#include "docstreambase.h"
#include "rmltar.h"

#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/common/string_tokenizer.h>




CTarArchiveReader::CTarArchiveReader(ECharset encoding)
    : Encoding(encoding)
{
    m_iLast_d2a.first = -1;
    m_iFirstDocNum = 0;
    m_iLastDocNum = -1;
    m_iCurDocument = 0;
    //m_Tempuffer = NULL;
    m_bFirstTime = true;

    // Tar archive reader implementation
    p_TAR = NULL;
    i_format = 4;
}

CTarArchiveReader::~CTarArchiveReader(void)
{
}

bool  CTarArchiveReader::Init(const Stroka& archiveName, const Stroka& _strDoc2AgFile, const Stroka& str_treat_as)
{
    stroka str_treat_as_ci(str_treat_as);   // case-insensitive
    try {
        Stroka strDoc2AgFile = _strDoc2AgFile;
        if (strDoc2AgFile.empty())
            strDoc2AgFile = archiveName + ".d2ag";

        CAgencyInfoRetriver::Init(strDoc2AgFile);

        if (str_treat_as_ci == "html")
            i_format = 4;
        else if (str_treat_as_ci == "text")
            i_format = 2;
        else
            ythrow yexception() << "unknown \"treat-as\" value: " << str_treat_as;

        tar_open(&p_TAR, archiveName.c_str());
        return true;
    } catch (yexception& e) {
        ythrow yexception() << "Error in \"CYndexArchiveReader::Init\" (" << e.what() << ")";
    }
}

bool CTarArchiveReader::GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs)
{
    bool b_error = false;
    bool b_results = tar_get_next_file(p_TAR, s_curr_filename, v_curr_content, b_error);

    if (! b_results || b_error) {
        s_curr_filename = "";
        v_curr_content.clear();
        return false;
    }

    strFilePath = s_curr_filename.c_str();

    attrs.Reset();

    attrs.m_iDocID = 0;
    attrs.m_strSource = "undefined";
    attrs.m_strTitle = CharToWide("undefined");
    attrs.m_strUrl = strFilePath; //= "undefined";

    return true;
}

bool  CTarArchiveReader::GetDocBodyByAdress(const Stroka&, CDocBodyBase& body)
{
    if (0 == v_curr_content.size())
        return false;

    return body.FillBody(*this, Encoding);
}

bool CTarArchiveReader::ReadInBuffer(ui8* p_buffer)
{
    if (v_curr_content.size() > 0) {
        memcpy(p_buffer, (char*)&v_curr_content[0], GetBufferLen());
        return true;
    } else
        return false;
}

long CTarArchiveReader::GetBufferLen()
{
    return v_curr_content.size() + 1;
}

int  CTarArchiveReader::GetCharset()
{
    return i_format;
}
