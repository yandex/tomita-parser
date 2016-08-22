#pragma once
#include <util/system/oldfile.h>
#include "parmbase.h"
#include "smartfilefind.h"
#include "dateparser.h"


class TTextMinerConfig;
class CParserOptions;

class CCommonParm :
    public CParmBase
{
protected:
    virtual void AnalizeParameter(const TStringBuf& paramName);

public:
    CCommonParm();
    ~CCommonParm();

    virtual bool AnalizeParameters(int argc, char *argv[]);
    virtual void AnalizeParameter(const TStringBuf& paramName, const TStringBuf& paramValue);
    virtual bool InitParameters();
    virtual void PrintParameters();

    virtual bool CheckParameters();

    bool Init(int argc, char *argv[]);

    // Нынешний формат конфига не позволяет установить эти параметры
    Stroka m_strUnloadBase;
    Stroka m_strUnloadFileMask;
    Stroka m_strUnloadDocNumsFile;
    Stroka m_strDoc2AgencyFileName;

    virtual time_t GetDate() const;
    virtual Stroka GetInterviewUrl2FioFileName() const;

    virtual Stroka GetMapReduceSubkey() const;

    virtual ECharset GetInputEncoding() const;
    virtual ECharset GetOutputEncoding() const;
    virtual ECharset GetLangDataEncoding() const;

    virtual docLanguage GetLanguage() const;

    virtual Stroka GetInputFileName() const;
    virtual Stroka GetOutputFileName() const;
    virtual Stroka GetSourceType() const;
    virtual Stroka GetDocDir() const;
    virtual size_t GetJobCount() const;
    virtual Stroka GetSourceFormat() const;
    virtual Stroka GetOutputFormat() const;
    virtual int GetFirstUnloadDocNum() const;
    virtual int GetLastUnloadDocNum() const;
    virtual Stroka GetPrettyOutputFileName() const;
    virtual Stroka GetDebugTreeFileName() const;
    virtual Stroka GetDebugRulesFileName() const;
    virtual bool IsAppendFdo() const;
    virtual bool IsWriteLeads() const;
    virtual bool IsCollectEqualFacts() const;
    virtual bool NeedAuxKwDict() const;
    virtual bool GetForceRecompile() const;
    virtual size_t GetMaxFactsCountPerSentence() const;

    virtual void WriteToLog(const TStringBuf& msg);

    virtual Stroka GetGramRoot() const;
    virtual Stroka GetDicDir() const;
    virtual Stroka GetBinaryDir() const;
    virtual bool UseOldLemmer() const; // Использовать ли старый Яндекс-леммер (BYK) или новый

    virtual CParserOptions* GetParserOptions() {
        return ParserOptions.Get();
    }

    enum EBastardMode { no = 0, outOfDict = 1, always = 2 };
    virtual EBastardMode GetBastardMode() const;

protected:
    Stroka m_strInputFileName;
    Stroka m_strSourceType;
    Stroka m_strDocDir;
    ymap<Stroka, Stroka> m_args;

    ECharset ParseEncoding(const Stroka& encodingName, ECharset& res);
    ECharset ParseEncoding(const Stroka& encodingName) const;

    TOldOsFile  m_LogFile;

    THolder<CParserOptions> ParserOptions;
    THolder<TTextMinerConfig> Config;

    Stroka m_strConfig;

    bool ParseConfig(const Stroka& fn);
};
