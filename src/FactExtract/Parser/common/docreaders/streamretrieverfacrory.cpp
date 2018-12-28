#include <util/stream/file.h>
#include <util/string/strip.h>
#include "streamretrieverfacrory.h"
#include "tararchivereader.h"
#include "memoryreader.h"


CStreamRetrieverFactory::CStreamRetrieverFactory()
    : m_sourceType(UndefSource)
{
}

CStreamRetrieverFactory::~CStreamRetrieverFactory()
{
}

EDocSourceType CStreamRetrieverFactory::GetSourceType()
{
    return m_sourceType;
}

void CStreamRetrieverFactory::ReadUnloadDocNums(Stroka strFileName, yset<int>& unloadDocNums)
{
    if (!isexist(strFileName.c_str()))
        ythrow yexception() << "Can't open \"unloaddocnums\" file.";
    TBufferedFileInput file(strFileName);

    Stroka ss;
    while (file.ReadLine(ss))
        unloadDocNums.insert(FromString<int>(StripString(TStringBuf(ss))));
}

TInputStream* CStreamRetrieverFactory::GetInputStream() {
    if (NULL != m_pInputFile.Get())
        return m_pInputFile.Get();
    else
        return &Cin;
}

bool CStreamRetrieverFactory::CreateTarStreamRetriever(const CCommonParm& parm) {
    if (parm.GetInputFileName().empty())
        ythrow yexception() << "Can't read TAR archive from STDIN. Input file isn't specified";

    m_pTarArchiveReader.Reset(new CTarArchiveReader(parm.GetInputEncoding()));
    m_sourceType = TarArchive;

    return m_pTarArchiveReader->Init(parm.GetInputFileName(), parm.m_strDoc2AgencyFileName, parm.GetSourceFormat());
}

bool CStreamRetrieverFactory::CreateSomStreamRetriever(const CCommonParm& parm) {
    m_pSomReader.Reset(new CSomReader());
    m_sourceType = SomSource;
    m_pSomReader->m_Date = parm.GetDate();
    m_pSomReader->Init(parm.GetInputFileName());

    return true;
}

bool CStreamRetrieverFactory::CreateDplStreamRetriever(const CCommonParm& parm) {
    m_pStdinDocPerLineReader.Reset(new CStdinDocPerLineReader(*GetInputStream(), parm.GetInputEncoding()));

    if (parm.GetLastUnloadDocNum() >= 0)
        m_pStdinDocPerLineReader->LastDocNumber = parm.GetLastUnloadDocNum();

    m_pStdinDocPerLineReader->m_Date = parm.GetDate();
    m_sourceType = StdinDocPerLineSource;
    return true;
}

bool CStreamRetrieverFactory::CreateOneDocStreamRetriever(const CCommonParm& parm) {
    m_pOneDocReader.Reset(new COneDocReader(*GetInputStream(), parm.GetInputEncoding()));

    if (parm.GetLastUnloadDocNum() >= 0)
        m_pOneDocReader->LastDocNumber = parm.GetLastUnloadDocNum();

    m_pOneDocReader->m_Date = parm.GetDate();
    m_sourceType = OneDocSource;
    return true;
}

bool CStreamRetrieverFactory::CreateMapreduceStreamRetriever(const CCommonParm& parm) {
    m_pStdinMapReduceDocsReader.Reset(new CStdinMapReduceDocsReader(parm.GetInputEncoding(), parm.GetMapReduceSubkey()));
    if (parm.GetLastUnloadDocNum() >= 0)
        m_pStdinMapReduceDocsReader->LastDocNumber = parm.GetLastUnloadDocNum();
    m_sourceType = StdinMapReduceSource;
    m_pStdinMapReduceDocsReader->Init(parm.m_strDoc2AgencyFileName);

    return true;
}

bool CStreamRetrieverFactory::CreateDirStreamRetriever(const CCommonParm& parm) {
    m_DocStreamDisc.Reset(new CDocStreamDisc(parm));
    m_pDocListRetrieverFromDisc.Reset(new CDocListRetrieverFromDisc());
    m_pDocListRetrieverFromDisc->m_Date = parm.GetDate();
    m_sourceType = SrcFilesSource;
    return m_pDocListRetrieverFromDisc->Init(parm.GetDocDir());
}

bool CStreamRetrieverFactory::CreateMemoryStreamRetriever(const CCommonParm& parm) {
    m_memoryReader.Reset(new CMemoryReader());
    m_sourceType = Memory;
    m_memoryReader->m_Date = parm.GetDate();
    return true;
}

bool CStreamRetrieverFactory::CreateStreamRetriever(const CCommonParm& parm)
{
    yset<int> unloadDocNums;

    if (!parm.GetInputFileName().empty()) {
        if (("tar" == parm.GetSourceType() || "som" == parm.GetSourceType()
            || "arcview" == parm.GetSourceType() || "dpl" == parm.GetSourceType()
            || "mapreduce" == parm.GetSourceType())
            && !PathHelper::Exists(parm.GetInputFileName()))
            ythrow yexception() << "File \"" << parm.GetInputFileName() << "\" doesn't exists.";

        if (!PathHelper::IsDir(parm.GetInputFileName())
            && "tar" != parm.GetSourceType() && "yarchive" != parm.GetSourceType()
            && "som" != parm.GetSourceType() && "dir" != parm.GetSourceType()) {

            m_pInputFile.Reset(new TBufferedFileInput(parm.GetInputFileName()));
        }
    }

    if (!parm.m_strUnloadDocNumsFile.empty())
        ReadUnloadDocNums(parm.m_strUnloadDocNumsFile, unloadDocNums);

    if ("tar" == parm.GetSourceType())
        return CreateTarStreamRetriever(parm);

    else if ("som" == parm.GetSourceType())
        return CreateSomStreamRetriever(parm);

    else if (("doc_per_line" == parm.GetSourceType()) || ("dpl" == parm.GetSourceType()))
        return CreateDplStreamRetriever(parm);

    else if ("no" == parm.GetSourceType())
        return CreateOneDocStreamRetriever(parm);

    else if ("mapreduce" == parm.GetSourceType())
        return CreateMapreduceStreamRetriever(parm);

    else if ("dir" == parm.GetSourceType())
        return CreateDirStreamRetriever(parm);

    else if ("memory" == parm.GetSourceType())
        return CreateMemoryStreamRetriever(parm);

    else
        ythrow yexception() << "Unknown source type \"" << parm.GetSourceType() << "\".";
}

CDocListRetrieverBase& CStreamRetrieverFactory::GetDocRetriever()
{
    switch (m_sourceType) {
        case SrcFilesSource:
            return    *m_pDocListRetrieverFromDisc;

        case StdinDocPerLineSource:
            return *m_pStdinDocPerLineReader;

        case StdinMapReduceSource:
            return *m_pStdinMapReduceDocsReader;

        case TarArchive:
            return *m_pTarArchiveReader;

        case SomSource:
            return *m_pSomReader;

        case OneDocSource:
            return *m_pOneDocReader;

        case Memory:
            return *m_memoryReader;

        //case UndefSource:
        default:
            ythrow yexception() << " GetDocRetriever error: maybe CreateStreamRetriever is not called.";
    }
}

CDocStreamBase& CStreamRetrieverFactory::GetDocStream()
{
    switch (m_sourceType) {
        case XMLFileSource:
        case SrcFilesSource:
            return *m_DocStreamDisc;

        case StdinDocPerLineSource:
            return *m_pStdinDocPerLineReader;

        case StdinMapReduceSource:
            return *m_pStdinMapReduceDocsReader;

        case TarArchive:
            return *m_pTarArchiveReader;

        case SomSource:
            return *m_pSomReader;

        case OneDocSource:
            return *m_pOneDocReader;

        case Memory:
            return *m_memoryReader;

        //case UndefSource:
        default:
            ythrow yexception() << " GetDocStream error: maybe CreateStreamRetriever is not called.";
    }
}
