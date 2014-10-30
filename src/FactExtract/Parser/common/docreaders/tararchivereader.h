#pragma once

#include "agencyinforetriever.h"
#include "doclistretrieverbase.h"
#include "docstreambase.h"

#include <library/getopt/opt.h>

#include <util/autoarray.h>
#include <util/charset/doccodes.h>
#include <util/generic/set.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/system/file.h>

struct TAR;

class CTarArchiveReader:
    public CDocListRetrieverBase,
    public CDocStreamBase,
    protected CAgencyInfoRetriver
{
public:
    CTarArchiveReader(ECharset encoding);
    virtual ~CTarArchiveReader(void);

public:
    /*CDocListRetrieverBase methods*/
    virtual bool GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs);

public:
    /*CDocStreamBase methods*/
    virtual bool  Init(const Stroka& archiveName, const Stroka& strDoc2AgFile, const Stroka& str_treat_as);
    void SetMask(Stroka& unloadFilesMask, Stroka& unloadFilesMaskNot) { (void)unloadFilesMask; (void)unloadFilesMaskNot; };
    virtual bool  GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body);
    virtual bool  AddAddressHint(const Stroka& strAddr) { (void)strAddr; return false; };

    i32 m_iFirstDocNum;
    i32 m_iLastDocNum;
    Stroka m_strUnloadDocNumsFile;

protected:

//    virtual bool  FillDocumentAttributes(SDocumentAttribtes& attrs, TDocArchive& DA, TDocDescr& DocD);
    Stroka GetAgNameByDoc(int iDoc);

    /*CDocStreamBase methods*/
    virtual bool ReadInBuffer(ui8* p_buffer);
    virtual long GetBufferLen();
    virtual int     GetCharset();
    bool m_bFirstTime;

    TFile m_DocId2AgNameFile;

    ECharset Encoding;

    i32 m_iCurDocument;
    TPair<int, Stroka> m_iLast_d2a;
    //char* m_Tempuffer;
    autoarray<ui8> m_Tempuffer;
    yset<int> m_setDocNums;

    // Tar archive reader implementation
    TAR *p_TAR;
    int i_format;

    Stroka s_curr_filename;
    yvector< char > v_curr_content;

};
