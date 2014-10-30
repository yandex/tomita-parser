#pragma once

#include "doclistretrieverbase.h"
#include "docstreambase.h"

#include "doclistretrieverfromdisc.h"
#include "stdindocperlinereader.h"

#include "stdinmapreducedocsreader.h"
#include "docstreamdisc.h"
#include "somreader.h"
#include "onedocreader.h"

#include <FactExtract/Parser/common/commonparm.h>

#include <util/generic/stroka.h>

class CTarArchiveReader;

enum EDocSourceType { SrcFilesSource, XMLFileSource, MDFFileSource, MDFXMLFileSource, YandexArchive, OldBaseSource, StdinYandexDocsSource, TarArchive, StdinDocPerLineSource, StdinMapReduceSource, SomSource, OneDocSource, UndefSource };

class CStreamRetrieverFactory
{
public:
    CStreamRetrieverFactory();
    ~CStreamRetrieverFactory();
    bool CreateStreamRetriever(const CCommonParm& parm);
    void ReadUnloadDocNums(Stroka strFileName, yset<int>& unloadDocNums);

    CDocListRetrieverBase& GetDocRetriever();
    CDocStreamBase& GetDocStream();

    inline EDocSourceType GetSourceType();
private:
    DECLARE_NOCOPY(CStreamRetrieverFactory);

    EDocSourceType m_sourceType;

    THolder<CDocStreamDisc>    m_DocStreamDisc;
    THolder<CDocListRetrieverFromDisc> m_pDocListRetrieverFromDisc;

    THolder<CStdinDocPerLineReader> m_pStdinDocPerLineReader;
    THolder<COneDocReader> m_pOneDocReader;
    THolder<CStdinMapReduceDocsReader> m_pStdinMapReduceDocsReader;
    THolder<CTarArchiveReader> m_pTarArchiveReader;
    THolder<CSomReader> m_pSomReader;

    THolder<TBufferedFileInput> m_pInputFile;

    TInputStream* GetInputStream();

    bool CreateTarStreamRetriever(const CCommonParm& parm);
    bool CreateSomStreamRetriever(const CCommonParm& parm);

    bool CreateDplStreamRetriever(const CCommonParm& parm);
    bool CreateOneDocStreamRetriever(const CCommonParm& parm);
    bool CreateMapreduceStreamRetriever(const CCommonParm& parm);
    bool CreateDirStreamRetriever(const CCommonParm& parm);

};

