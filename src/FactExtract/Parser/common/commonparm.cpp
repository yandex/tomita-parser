#include "commonparm.h"
#include "pathhelper.h"

#include <util/string/strip.h>
#include <util/string/cast.h>

#include <library/getopt/last_getopt.h>
#include <library/svnversion/svnversion.h>

#include <kernel/gazetteer/config/protoconf.h>

#include <FactExtract/Parser/afdocparser/rusie/parseroptions.h>

#include "textminerconfig.pb.h"

#define OPT_BINARY_DIR "binary-dir"

NLastGetopt::TEasySetup MakeTomitaParserOpts() {
    NLastGetopt::TEasySetup opts;

    opts('b', OPT_BINARY_DIR,  " ", "Alternative directory for compiled grammars.", false)
        ('v', "version", &PrintSvnVersionAndExit0, "Print version information");

    opts.SetFreeArgsNum(1, 1);
    opts.SetFreeArgTitle(0, "config.proto", "parser configuration file name");

    opts.SetTitle(Stroka("Yandex Tomita-parser ") + GetProgramVersionTag() + ". Open-source edition.\nhttp://api.yandex.ru/tomita");

    return opts; 
}

CCommonParm::CCommonParm() {
    m_strSourceType = "no";
}

CCommonParm::~CCommonParm() {
    ParserOptions.Release();
}

bool CCommonParm::InitParameters() {
    return true;
}

bool CCommonParm::AnalizeParameters(int argc, char *argv[]) {
    try {
        NLastGetopt::TOpts opts = MakeTomitaParserOpts();
        NLastGetopt::TOptsParseResult r(&opts, argc, argv);

        if (r.Has(OPT_BINARY_DIR))
            m_args[OPT_BINARY_DIR] = r.Get(OPT_BINARY_DIR);

        if (r.GetFreeArgCount() > 0) {
            m_strConfig = r.GetFreeArgs()[0];
            ParseConfig(m_strConfig);
        } else
            return false;

        return true;
    } catch (yexception& e) {
        m_strError += e.what();
        return false;
    } catch (...) {
        m_strError += "Unknown error in \"CParmBase::AnalizeParameters\"";
        return false;
    }
}

void CCommonParm::PrintParameters() {
    MakeTomitaParserOpts().PrintUsage("tomita-parser", Cerr);
}

bool CCommonParm::Init(int argc, char *argv[]) {
    if (!AnalizeParameters(argc, argv))
        return false;

    if (!CheckParameters())
        return false;

    if (!InitParameters())
        return false;

    return true;
}

ECharset CCommonParm::ParseEncoding(const Stroka& encodingName, ECharset& res) {
    ECharset enc = CharsetByName(encodingName.c_str());
    if (enc == CODES_UNKNOWN)
        m_strError += "\nUnkown encoding: \"" + encodingName + "\"";
    else
        res = enc;

    return enc;
}

ECharset CCommonParm::ParseEncoding(const Stroka& encodingName) const {
    ECharset enc = CharsetByName(encodingName.c_str());
    if (enc == CODES_UNKNOWN)
        ythrow yexception() << "Unknown encoding: \"" << encodingName << "\"";

    return enc;
}

void CCommonParm::WriteToLog(const TStringBuf& str) {
    Clog << str << Endl;
    if (m_LogFile.IsOpen()) {
        m_LogFile.Write(~str, +str);
        m_LogFile.Write("\n", 1);
    }
}

void CCommonParm::AnalizeParameter(const TStringBuf&) {

}

bool CCommonParm::CheckParameters() {
    if (m_strDocDir.empty() && GetSourceType().empty()) {
        m_strError += "No sources";
        return false;
    }

    if ("yarchive" == GetSourceType() && GetSourceFormat() != "html" && GetSourceFormat() != "text")
        ythrow yexception() << "Error: /treat-as must be either \"html\" or \"text\".";

    if ("yarchive" == GetSourceType()) {
        if (GetLastUnloadDocNum() != -1)
            if (GetFirstUnloadDocNum() > GetLastUnloadDocNum() && GetLastUnloadDocNum() != -1) {
                m_strError += "Bad FirstUnloadDocNum is greater than LastUnloadDocNum.";
                return false;
            }
    }

    return true;
}

Stroka CCommonParm::GetGramRoot() const {
    if (NULL != Config.Get() && Config->has_dictionary())
        return Config->dictionary();
    return Stroka("");
}

Stroka CCommonParm::GetDicDir() const {
    if (GetGramRoot().length() > 0)
        return PathHelper::DirNameWithSlash(GetGramRoot());
    return Stroka("");
}

Stroka CCommonParm::GetBinaryDir() const {
    if (NULL != Config.Get() && Config->has_binarydir() && Config->binarydir().length() > 0)
        return PathHelper::DirNameWithSlash(Config->binarydir());
    return Stroka("");
}

ECharset CCommonParm::GetInputEncoding() const {
    if (NULL != Config.Get() && Config->has_input() && Config->input().has_encoding())
        return ParseEncoding(Config->input().encoding());

    return CODES_UTF8;
}

ECharset CCommonParm::GetOutputEncoding() const {
    if (NULL != Config.Get() && Config->has_output() && Config->output().has_encoding())
        return ParseEncoding(Config->output().encoding());

    return CODES_UTF8;
}

ECharset CCommonParm::GetLangDataEncoding() const {
    return CODES_WIN;
}

Stroka CCommonParm::GetMapReduceSubkey() const {
    if (NULL != Config.Get() && Config->has_input() && Config->input().has_subkey())
        return Config->input().GetSubKey();

    return Stroka("");
}

docLanguage CCommonParm::GetLanguage() const {
    if (NULL != Config.Get() && Config->has_language())
        return LanguageByName(Config->language());

    return LANG_RUS;
}

Stroka CCommonParm::GetInputFileName() const {
    return m_strInputFileName;
}

Stroka CCommonParm::GetOutputFileName() const {
    if (NULL != Config.Get() && Config->has_output() && Config->output().has_file()) {
        Stroka fn = Config->output().GetFile();
        fn.to_lower();
        if ("stdout" == fn)
            return "-";
        return Config->output().GetFile();
    }

    return Stroka("-");
}

Stroka CCommonParm::GetSourceType() const {
    return m_strSourceType;
}

Stroka CCommonParm::GetDocDir() const {
    return m_strDocDir;
}

size_t CCommonParm::GetJobCount() const {
    if (NULL != Config.Get() && Config->has_numthreads())
        return Config->numthreads();

    return 1;
}

Stroka CCommonParm::GetSourceFormat() const {
    if (NULL != Config.Get() && Config->input().has_format())
        switch (Config->input().GetFormat()) {
            case TTextMinerConfig::TInputParams::plain:
                return Stroka("text");

            case TTextMinerConfig::TInputParams::html:
                return Stroka("html");

            default:
                ythrow yexception() << "Source format unknown.";
        }

   return Stroka("text");
}

Stroka CCommonParm::GetOutputFormat() const {
    if (NULL != Config.Get() && Config->output().has_format())
        switch (Config->output().GetFormat()) {
            case TTextMinerConfig::TOutputParams::text:
                return Stroka("text");

            case TTextMinerConfig::TOutputParams::xml:
                return Stroka("xml");

            case TTextMinerConfig::TOutputParams::proto:
                return Stroka("proto");

            case TTextMinerConfig::TOutputParams::json:
                return Stroka("json");

            case TTextMinerConfig::TOutputParams::mapreduce:
                return Stroka("mapreduce");

            default:
                ythrow yexception() << "This type of input isn't supported";
        }

   return Stroka("xml");
}

CCommonParm::EBastardMode CCommonParm::GetBastardMode() const {
    if (NULL != Config.Get() && Config->has_bastardmode())
        switch (Config->GetBastardMode()) {
            case TTextMinerConfig::no:
                return CCommonParm::EBastardMode::no;

            case TTextMinerConfig::outOfDict:
                return CCommonParm::EBastardMode::outOfDict;

            case TTextMinerConfig::always:
                return CCommonParm::EBastardMode::always;

            default:
                ythrow yexception() << "This bastard mode isn't supported";
        }

    return CCommonParm::EBastardMode::no;  
}

int CCommonParm::GetFirstUnloadDocNum() const {
    if (NULL != Config.Get() && Config->has_input() && Config->input().has_firstdocnum())
        return Config->input().GetFirstDocNum();

    return -1;
}

int CCommonParm::GetLastUnloadDocNum() const {
    if (NULL != Config.Get() && Config->has_input() && Config->input().has_lastdocnum())
        return Config->input().GetLastDocNum();

    return -1;
}

Stroka CCommonParm::GetPrettyOutputFileName() const {
    if (NULL != Config.Get() && Config->has_prettyoutput()) {
        Stroka fn = Config->GetPrettyOutput();
        fn.to_lower();
        if ("stdout" == fn || "stderr" == fn)
            return fn;
        if ("-" == fn)
            return "stdout";
        if (fn.size() > 0)
            return Config->GetPrettyOutput();
    }

    return Stroka("");
}

Stroka CCommonParm::GetDebugTreeFileName() const {
    if (NULL != Config.Get() && Config->has_printtree())
        return Config->GetPrintTree();

    return Stroka("");
}

Stroka CCommonParm::GetDebugRulesFileName() const {
    if (NULL != Config.Get() && Config->has_printrules())
        return Config->GetPrintRules();

    return Stroka("");
}

bool CCommonParm::IsAppendFdo() const {
    if (NULL != Config.Get() && Config->output().has_mode())
        switch (Config->output().GetMode()) {
            case TTextMinerConfig::TOutputParams::append:
                return true;

            case TTextMinerConfig::TOutputParams::overwrite:
                return false;

            default:
                ythrow yexception() << "This output mode isn't supported";
        }

    return false;
}

bool CCommonParm::IsWriteLeads() const {
    if (NULL != Config.Get() && Config->has_output() && Config->output().has_saveleads())
        return Config->output().GetSaveLeads();

    return true;
}

bool CCommonParm::IsCollectEqualFacts() const {
    if (NULL != Config.Get() && Config->has_output() && Config->output().has_saveequals())
        return Config->output().GetSaveEquals();

    return false;
}

bool CCommonParm::NeedAuxKwDict() const {
    if (NULL != Config.Get() && Config->has_auxkwdict())
        return Config->auxkwdict();

    return false;
}

bool CCommonParm::GetForceRecompile() const {
    if (NULL != Config.Get() && Config->has_forcerecompile())
        return Config->forcerecompile();

    return false;
}

bool CCommonParm::UseOldLemmer() const {
    if (NULL != Config.Get() && Config->has_useoldlemmer())
        return Config->useoldlemmer();

    return false;
}

size_t CCommonParm::GetMaxFactsCountPerSentence() const {
    if (NULL != Config.Get() && Config->has_maxfactscountpersentence())
        return Config->maxfactscountpersentence();

    return 64;
}

time_t CCommonParm::GetDate() const {
    if (NULL != Config.Get() && Config->has_input() && Config->input().has_date()) {
        struct tm tmDate;
        CDateParser DateParser;
        DateParser.ParseDate(Config->input().GetDate(), tmDate);
        return mktime(&tmDate);
    }

    return 0;
}

Stroka CCommonParm::GetInterviewUrl2FioFileName() const {
    if (NULL != Config.Get() && Config->has_input() && Config->input().has_url2fio())
        return Config->input().GetUrl2Fio();

    return Stroka("");
}

bool CCommonParm::ParseConfig(const Stroka& fn) {
    Config.Reset(NProtoConf::LoadFromFile<TTextMinerConfig>(fn));
    if (!Config)
        ythrow yexception() << "Cannot read the config from \"" << fn << "\".";

    if (m_args.has(OPT_BINARY_DIR)) {
        if (Config->has_binarydir() && Config->binarydir().length() > 0)
            ythrow yexception() << "Both \"--" << OPT_BINARY_DIR << "\" command line argument and \"BinaryDir\" config parameter specified";

        Config->set_binarydir(m_args[OPT_BINARY_DIR]);
    }

    if (Config->has_input()) {
        TTextMinerConfig_TInputParams inputParams = Config->input();

        if (inputParams.has_file() && !inputParams.file().empty()
            && inputParams.has_dir() && !inputParams.dir().empty())
            ythrow yexception() << "Input\\File and Input\\Dir options are meaningless together";

        Stroka fn = inputParams.file();
        fn.to_lower();
        if (fn.empty() || "stdin" == fn || "-" == fn)
            m_strInputFileName = "";
        else
            m_strInputFileName = inputParams.file();

        if (inputParams.has_dir()) {
            if (inputParams.has_type())
                ythrow yexception() << "Input\\Type field is meaningless for directory processing";

            m_strSourceType = "dir";
            m_strInputFileName = inputParams.dir();
            m_strDocDir = inputParams.dir();
            if (!PathHelper::IsDir(m_strDocDir))
                ythrow yexception() << "\"" << m_strDocDir << "\" isn't a directory";
        } else {
            if (inputParams.has_type()) {
                switch (inputParams.type()) {
                    case TTextMinerConfig::TInputParams::no:
                        m_strSourceType = "no";
                        break;

                    case TTextMinerConfig::TInputParams::dpl:
                        m_strSourceType = "dpl";
                        break;

                    case TTextMinerConfig::TInputParams::arcview:
                        m_strSourceType = "arcview";
                        break;

                    case TTextMinerConfig::TInputParams::mapreduce:
                        m_strSourceType = "mapreduce";
                        break;

                    case TTextMinerConfig::TInputParams::tar:
                        if (m_strInputFileName.empty())
                            ythrow yexception() << "Please specify Input\\File field in configuration file in order to use .tar archive.";
                        m_strSourceType = "tar";
                        break;

                    case TTextMinerConfig::TInputParams::som:
                        if (m_strInputFileName.empty())
                            ythrow yexception() << "Please specify Input\\File field in configuration file in order to read SOM data.";
                        m_strSourceType = "som";
                        break;

                    case TTextMinerConfig::TInputParams::yarchive:
                        if (m_strInputFileName.empty())
                            ythrow yexception() << "Please specify Input\\File field in configuration file in order to read Yandex archive.";
                        m_strSourceType = "yarchive";
                        break;

                    default:
                        ythrow yexception() << "This type of input isn't supported";
                }
            } else
                m_strSourceType = "no";
        }
    }

    if (NULL == ParserOptions.Get())
        ParserOptions.Reset(new CParserOptions);

    ParserOptions->InitFromConfigObject(*Config.Get());

    return true;
}

void CCommonParm::AnalizeParameter(const TStringBuf& name, const TStringBuf& value) {
    Stroka paramName = ToString(StripString(name));
    Stroka paramValue = ToString(StripString(value));

    paramName.to_lower();
}

