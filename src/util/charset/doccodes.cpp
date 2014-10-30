#include "doccodes.h"

#include <util/system/defaults.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

#include <cctype>

/*
 * define language by docLanguage
 */

namespace {
    struct TLanguageNameAndEnum {
        docLanguage Language;
        EScript Script;
        const char* EnglishName;
        const char* BiblioName;
        const char* IsoName;
        const char* Synonyms;
    };

    static const TLanguageNameAndEnum LanguageNameAndEnum[] = {
        {LANG_UNK,       SCRIPT_OTHER,    "Unknown",    "unk", "mis", NULL},
        {LANG_RUS,       SCRIPT_CYRILLIC, "Russian",    "rus", "ru", "ru-RU"},
        {LANG_ENG,       SCRIPT_LATIN,    "English",    "eng", "en", "en-US, en-GB, en-CA, en-NZ, en-AU"},
        {LANG_POL,       SCRIPT_LATIN,    "Polish",     "pol", "pl", NULL},
        {LANG_HUN,       SCRIPT_LATIN,    "Hungarian",  "hun", "hu", NULL},
        {LANG_UKR,       SCRIPT_CYRILLIC, "Ukrainian",  "ukr", "uk", "uk-UA"},
        {LANG_GER,       SCRIPT_LATIN,    "German",     "ger", "de", "deu"},
        {LANG_FRN,       SCRIPT_LATIN,    "French",     "fre", "fr", "fra, fr-FR, fr-CA"},
        {LANG_TAT,       SCRIPT_CYRILLIC, "Tatar",      "tat", "tt", NULL},
        {LANG_BLR,       SCRIPT_CYRILLIC, "Belarusian", "blr", "be", "Belorussian"},
        {LANG_KAZ,       SCRIPT_CYRILLIC, "Kazakh",     "kaz", "kk", NULL},
        {LANG_ALB,       SCRIPT_LATIN,    "Albanian",   "alb", "sq", NULL},
        {LANG_SPA,       SCRIPT_LATIN,    "Spanish",    "spa", "es", NULL},
        {LANG_ITA,       SCRIPT_LATIN,    "Italian",    "ita", "it", NULL},
        {LANG_ARM,       SCRIPT_ARMENIAN, "Armenian",   "arm", "hy", "hye"},
        {LANG_DAN,       SCRIPT_LATIN,    "Danish",     "dan", "da", NULL},
        {LANG_POR,       SCRIPT_LATIN,    "Portuguese", "por", "pt", NULL},
        {LANG_ISL,       SCRIPT_LATIN,    "Icelandic",  "isl", "is", "ice"},
        {LANG_SLO,       SCRIPT_LATIN,    "Slovak",     "slo", "sk", "slk"},
        {LANG_SLV,       SCRIPT_LATIN,    "Slovene",    "slv", "sl", NULL},
        {LANG_DUT,       SCRIPT_LATIN,    "Dutch",      "dut", "nl", "nld"},
        {LANG_BUL,       SCRIPT_CYRILLIC, "Bulgarian",  "bul", "bg", NULL},
        {LANG_CAT,       SCRIPT_LATIN,    "Catalan",    "cat", "ca", NULL},
        {LANG_HRV,       SCRIPT_LATIN,    "Croatian",   "hrv", "hr", "scr"},
        {LANG_CZE,       SCRIPT_LATIN,    "Czech",      "cze", "cs", "ces"},
        {LANG_GRE,       SCRIPT_GREEK,    "Greek",      "gre", "el", "ell"},
        {LANG_HEB,       SCRIPT_HEBREW,   "Hebrew",     "heb", "he", NULL},
        {LANG_NOR,       SCRIPT_LATIN,    "Norwegian",  "nor", "no", NULL},
        {LANG_MAC,       SCRIPT_CYRILLIC, "Macedonian", "mac", "mk", NULL},
        {LANG_SWE,       SCRIPT_LATIN,    "Swedish",    "swe", "sv", NULL},
        {LANG_KOR,       SCRIPT_HANGUL,   "Korean",     "kor", "ko", NULL},
        {LANG_LAT,       SCRIPT_LATIN,    "Latin",      "lat", "la", NULL},
        {LANG_BASIC_RUS, SCRIPT_CYRILLIC, "Basic Russian", "basic-rus", "bas-ru", NULL},
        {LANG_BOS,       SCRIPT_LATIN,    "Bosnian",    "bos", "bs", NULL},
        {LANG_MLT,       SCRIPT_LATIN,    "Maltese",    "mlt", "mt", NULL},

        {LANG_EMPTY, SCRIPT_OTHER,        "Empty",      "empty", NULL, NULL},
        {LANG_UNK_LAT, SCRIPT_LATIN,      "Unknown Latin", "unklat", NULL, NULL},
        {LANG_UNK_CYR, SCRIPT_CYRILLIC,   "Unknown Cyrillic", "unkcyr", NULL, NULL},
        {LANG_UNK_ALPHA, SCRIPT_OTHER,    "Unknown Alpha", "unkalpha", NULL, NULL},

        {LANG_FIN, SCRIPT_LATIN,          "Finnish",    "fin", "fi", NULL},
        {LANG_EST, SCRIPT_LATIN,          "Estonian",   "est", "et", NULL},
        {LANG_LAV, SCRIPT_LATIN,          "Latvian",    "lav", "lv", NULL},
        {LANG_LIT, SCRIPT_LATIN,          "Lithuanian", "lit", "lt", NULL},
        {LANG_BAK, SCRIPT_CYRILLIC,       "Bashkir",    "bak", "ba", NULL},
        {LANG_TUR, SCRIPT_LATIN,          "Turkish",    "tur", "tr", NULL},
        {LANG_RUM, SCRIPT_LATIN,          "Romanian",   "rum", "ro", "ron"},
        {LANG_MON, SCRIPT_CYRILLIC,       "Mongolian",  "mon", "mn", NULL},
        {LANG_UZB, SCRIPT_CYRILLIC,       "Uzbek",      "uzb", "uz", NULL},
        {LANG_KIR, SCRIPT_CYRILLIC,       "Kirghiz",    "kir", "ky", NULL},
        {LANG_TGK, SCRIPT_CYRILLIC,       "Tajik",      "tgk", "tg", NULL},
        {LANG_TUK, SCRIPT_LATIN,          "Turkmen",    "tuk", "tk", NULL},
        {LANG_SRP, SCRIPT_CYRILLIC,       "Serbian",    "srp", "sr", NULL},
        {LANG_AZE, SCRIPT_LATIN,          "Azerbaijani", "aze", "az", NULL},
        {LANG_BASIC_ENG, SCRIPT_LATIN,    "Basic English", "basic-eng", "bas-en", NULL},
        {LANG_GEO, SCRIPT_GEORGIAN,       "Georgian",   "geo", "ka", "kat"},
        {LANG_ARA, SCRIPT_ARABIC,         "Arabic",     "ara", "ar", NULL},
        {LANG_PER, SCRIPT_ARABIC,         "Persian",    "per", "fa", "fas"},
        {LANG_CHU, SCRIPT_CYRILLIC,       "Church Slavonic", "chu", "cu", NULL},
        {LANG_CHI, SCRIPT_HAN,            "Chinese",    "chi", "zh", "zho"},
        {LANG_JPN, SCRIPT_HIRAGANA,       "Japanese",   "jpn", "ja", NULL},
        {LANG_IND, SCRIPT_LATIN,          "Indonesian", "ind", "id", NULL},
        {LANG_MSA, SCRIPT_LATIN,          "Malay",      "msa", "ms", "may"},
        {LANG_THA, SCRIPT_THAI,           "Thai",       "tha", "th", NULL},
        {LANG_VIE, SCRIPT_LATIN,          "Vietnamese", "vie", "vi", NULL}
    };

    class TLanguagesMap {
    private:
        static const char* const EMPTY_NAME;

        typedef yhash<TStringBuf, docLanguage, ci_hash, ci_equal_to> TNamesHash;
        TNamesHash Hash;

        typedef yvector<const char*> TNamesVector;
        TNamesVector BiblioNames;
        TNamesVector IsoNames;
        TNamesVector FullNames;

        typedef yvector<EScript> TScripts;
        TScripts Scripts;

    private:
        void AddNameToHash(const TStringBuf& name, docLanguage language) {
            if (Hash.find(name) != Hash.end()) {
                YASSERT(Hash.find(name)->second == language);
                return;
            }

            Hash[name] = language;
        }

        void AddName(const char* name, docLanguage language, TNamesVector& names) {
            if (name == NULL || strlen(name) == 0)
                return;

            YASSERT(names[language] == EMPTY_NAME);
            names[language] = name;

            AddNameToHash(name, language);
        }

        void AddSynonyms(const char* syn, docLanguage language) {
            static const char* del = " ,;";
            if (!syn)
                return;
            while (*syn) {
                size_t len = strcspn(syn, del);
                AddNameToHash(TStringBuf(syn, len), language);
                syn += len;
                while (*syn && strchr(del, *syn))
                    ++syn;
            }
        }

    public:
        TLanguagesMap() {
            BiblioNames.resize(LANG_MAX, EMPTY_NAME);
            IsoNames.resize(LANG_MAX, EMPTY_NAME);
            FullNames.resize(LANG_MAX, EMPTY_NAME);
            Scripts.resize(LANG_MAX, SCRIPT_OTHER);

            for (size_t i = 0; i != ARRAY_SIZE(LanguageNameAndEnum); ++i) {
                const TLanguageNameAndEnum& val = LanguageNameAndEnum[i];

                docLanguage language = val.Language;

                AddName(val.BiblioName, language, BiblioNames);
                AddName(val.IsoName, language, IsoNames);
                AddName(val.EnglishName, language, FullNames);
                AddSynonyms(val.Synonyms, language);

                if (Scripts[language] == SCRIPT_OTHER) {
                    Scripts[language] = val.Script;
                }
            }
        }

    public:
        inline docLanguage LanguageByName(const TStringBuf& name, docLanguage def) const {
            if (!name)
                return def;

            TNamesHash::const_iterator i = Hash.find(name);
            if (i == Hash.end())
                return def;

            return i->second;
        }

        inline const char* FullNameByLanguage(docLanguage language) const {
            if (language < 0 || static_cast<size_t>(language) >= FullNames.size())
                return NULL;

            return FullNames[language];
        }
        inline const char* BiblioNameByLanguage(docLanguage language) const {
            if (language < 0 || static_cast<size_t>(language) >= BiblioNames.size())
                return NULL;

            return BiblioNames[language];
        }
        inline const char* IsoNameByLanguage(docLanguage language) const {
            if (language < 0 || static_cast<size_t>(language) >= IsoNames.size())
                return NULL;

            return IsoNames[language];
        }

        inline EScript Script(docLanguage language) const {
            return Scripts[language];
        }
    };
}

const char* const TLanguagesMap::EMPTY_NAME = "";

const char* FullNameByLanguage(docLanguage language) {
    return Singleton<TLanguagesMap>()->FullNameByLanguage(language);
}
const char* NameByLanguage(docLanguage language) {
    return Singleton<TLanguagesMap>()->BiblioNameByLanguage(language);
}
const char* IsoNameByLanguage(docLanguage language) {
    return Singleton<TLanguagesMap>()->IsoNameByLanguage(language);
}

docLanguage LanguageByNameStrict(const TStringBuf& name) {
    return Singleton<TLanguagesMap>()->LanguageByName(name, LANG_MAX);
}

docLanguage LanguageByNameOrDie(const TStringBuf &name) {
    docLanguage result = LanguageByNameStrict(name);
    if (result == LANG_MAX)
        throw yexception() << "LanguageByNameOrDie: invalid language '" << name << "'";
    return result;
}

docLanguage LanguageByName(const TStringBuf& name) {
    return Singleton<TLanguagesMap>()->LanguageByName(name, LANG_UNK);
}

EScript ScriptByLanguage(docLanguage language) {
    return Singleton<TLanguagesMap>()->Script(language);
}

namespace {
    static const size_t MAX_GLYPH = 0x10000;
    class TScriptGlyphIndex {
    public:
        TScriptGlyphIndex() {
            NCharsetInternal::InitScriptData(Data, MAX_GLYPH);
        }

        EScript GetGlyphScript(wchar32 glyph) const {
            if (glyph >= MAX_GLYPH)
                return SCRIPT_UNKNOWN;
            return (EScript)Data[glyph];
        }

    private:
        ui8 Data[MAX_GLYPH];
    };
}

EScript ScriptByGlyph(wchar32 glyph) {
    return Singleton<TScriptGlyphIndex>()->GetGlyphScript(glyph);
}

/*
 * MIME types
 */

class TMimeTypes {
// Constructor
public:
    TMimeTypes();

// Methods
public:
    const char* StrByExt(const char* ext) const;
    MimeTypes   MimeByStr(const char* str) const;
    const char* StrByMime(MimeTypes mime) const;

// Constants
public:
    static const size_t MAX_EXT_LEN = 5; // max length of supported extensions

// Helper methods
private:
    void SetContentTypes();
    void SetExt();

// Types
private:
    struct TRecord {
        MimeTypes Mime;
        const char* ContentType;
        const char* Ext;
    };

    typedef yhash<const char*, int> TRecordHash;

// Fields
private:
    static const TRecord Records[];
    TRecordHash ContentTypes;
    TRecordHash Ext;
};

const TMimeTypes::TRecord TMimeTypes::Records[] = {
    { MIME_UNKNOWN, 0, 0 },
    { MIME_TEXT, "text/plain\0", "asc\0txt\0" },
    { MIME_HTML, "text/html\0", "html\0htm\0shtml\0" },
    { MIME_PDF, "application/pdf\0", "pdf\0" },
    { MIME_RTF, "text/rtf\0application/rtf\0", "rtf\0" },
    { MIME_DOC, "application/msword\0", "doc\0" },
    { MIME_MPEG, "audio/mpeg\0", "mp3\0mpa\0m2a\0mp2\0mpg\0mpga\0" },
    { MIME_XML, "text/xml\0", "xml\0" },
    { MIME_WML, "text/vnd.wap.wml\0", "wml\0" },
    { MIME_SWF, "application/x-shockwave-flash\0", "swf\0" },
    { MIME_XLS, "application/vnd.ms-excel\0", "xls\0" },
    { MIME_PPT, "application/vnd.ms-powerpoint\0", "ppt\0" },
    { MIME_IMAGE_JPG, "image/jpeg\0image/jpg\0", "jpeg\0jpg\0" },
    { MIME_IMAGE_PJPG, "image/pjpeg\0", "pjpeg\0" },
    { MIME_IMAGE_PNG, "image/png\0", "png\0" },
    { MIME_IMAGE_GIF, "image/gif\0", "gif\0" },
    { MIME_DOCX, "application/vnd.openxmlformats-officedocument.wordprocessingml.document\0", "docx\0" },
    { MIME_ODT, "application/vnd.oasis.opendocument.text\0", "odt\0" },
    { MIME_ODP, "application/vnd.oasis.opendocument.presentation\0", "odp\0" },
    { MIME_ODS, "application/vnd.oasis.opendocument.spreadsheet\0", "ods\0" },
    { MIME_UNKNOWN, 0, 0 },
    { MIME_IMAGE_BMP, "image/bmp\0image/x-ms-bmp\0image/x-windows-bmp\0", "bmp\0" },
    { MIME_WAV, "audio/x-wav\0", "wav\0" },
    { MIME_ARCHIVE, "application/x-archive\0", 0 },
    { MIME_EXE, "application/exe\0application/octet-stream\0application/x-dosexec\0application/x-msdownload\0", "exe\0" },
    { MIME_ODG, "application/vnd.oasis.opendocument.graphics\0", "odg\0" },
    { MIME_GZIP, "application/x-gzip\0", "gz\0gzip\0" },
    { MIME_XLSX, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet\0", "xlsx\0" },
    { MIME_PPTX, "application/vnd.openxmlformats-officedocument.presentationml.presentation\0", "pptx\0" },
    { MIME_JAVASCRIPT, "application/javascript\0text/javascript\0", "js\0" },
    { MIME_EPUB, "application/epub+zip\0", "epub\0" },
    { MIME_TEX, "application/tex\0", "tex\0" },
    { MIME_MAX, 0, 0 },

    // Additional records
    { MIME_HTML, "application/xhtml+xml\0", "xhtml\0" },
    { MIME_UNKNOWN, "text/css\0", "css\0" },
    { MIME_UNKNOWN, "image/x-icon\0", "ico\0" },
    { MIME_UNKNOWN, "application/zip\0", "zip\0" },
};

TMimeTypes::TMimeTypes()
  : ContentTypes()
  , Ext()
{
    SetContentTypes();
    SetExt();
}

void TMimeTypes::SetContentTypes() {
    for (int i = 0; i < (int)ARRAY_SIZE(Records); ++i) {
        const TRecord& record(Records[i]);
        assert(i == record.Mime || i > MIME_MAX || record.Mime == MIME_UNKNOWN);
        if (!record.ContentType)
            continue;
        for (const char* type = record.ContentType; *type; type += strlen(type) + 1) {
            assert(ContentTypes.find(type) == ContentTypes.end());
            ContentTypes[type] = i;
        }
    }
}

void TMimeTypes::SetExt() {
    for (int i = 0; i < (int)ARRAY_SIZE(Records); ++i) {
        const TRecord& record(Records[i]);
        if (!record.Ext)
            continue;
        for (const char* ext = record.Ext; *ext; ext += strlen(ext) + 1) {
            assert(strlen(ext) <= MAX_EXT_LEN);
            assert(Ext.find(ext) == Ext.end());
            Ext[ext] = i;
        }
    }
}

const char* TMimeTypes::StrByExt(const char* ext) const {
    TRecordHash::const_iterator it = Ext.find(ext);
    if (it == Ext.end())
        return 0;
    return Records[it->second].ContentType;
}

MimeTypes TMimeTypes::MimeByStr(const char* str) const {
    TRecordHash::const_iterator it = ContentTypes.find(str);
    if (it == ContentTypes.end())
        return MIME_UNKNOWN;
    return Records[it->second].Mime;
}

const char* TMimeTypes::StrByMime(MimeTypes mime) const {
    return Records[mime].ContentType;
}

const char* mimetypeByExt(const char *fname, const char* check_ext) {
    const char* ext_p;
    if (fname == 0 || *fname == 0 ||
        (ext_p=strrchr(fname, '.')) == 0 || strlen(ext_p)-1 > TMimeTypes::MAX_EXT_LEN) {
        return 0;
    }

    char ext[TMimeTypes::MAX_EXT_LEN + 1];
    size_t i;
    ext_p++;
    for (i=0; i < TMimeTypes::MAX_EXT_LEN && ext_p[i]; i++)
        ext[i] = (char)tolower(ext_p[i]);
    ext[i] = 0;

    if (check_ext != 0) {
        if (strcmp(ext, check_ext) == 0)
            return check_ext;
        else
            return 0;
    }

    return Singleton<TMimeTypes>()->StrByExt(ext);
}

MimeTypes mimeByStr(const char* mimeStr) {
    return Singleton<TMimeTypes>()->MimeByStr(mimeStr);
}

const char* strByMime(MimeTypes mime) {
    if (mime < 0 || mime > MIME_MAX)
        return 0; // index may contain documents with invalid MIME (ex. 255)
    return Singleton<TMimeTypes>()->StrByMime(mime);
}

const char *MimeNames[MIME_MAX] = {
    "unknown",      // MIME_UNKNOWN         //  0
    "text",         // MIME_TEXT            //  1
    "html",         // MIME_HTML            //  2
    "pdf",          // MIME_PDF             //  3
    "rtf",          // MIME_RTF             //  4
    "doc",          // MIME_DOC             //  5
    "mpeg",         // MIME_MPEG            //  6
    "xml",          // MIME_XML             //  7
    "wap",          // MIME_WML             //  8
    "swf",          // MIME_SWF             //  9
    "xls",          // MIME_XLS             // 10
    "ppt",          // MIME_PPT             // 11
    "jpeg",         // MIME_IMAGE_JPG       // 12
    "pjpeg",        // MIME_IMAGE_PJPG      // 13
    "png",          // MIME_IMAGE_PNG       // 14
    "gif",          // MIME_IMAGE_GIF       // 15
    "docx",         // MIME_DOCX            // 16
    "odt",          // MIME_ODT             // 17
    "odp",          // MIME_ODP             // 18
    "ods",          // MIME_ODS             // 19
    "xmlhtml",      // MIME_XHTMLXML        // 20
    "bmp",          // MIME_IMAGE_BMP       // 21
    "wav",          // MIME_WAV             // 22
    "archive",      // MIME_ARCHIVE         // 23
    "exe",          // MIME_EXE             // 24
    "odg",          // MIME_ODG             // 25
    "gzip",         // MIME_GZIP            // 26
    "xlsx",         // MIME_XLSX            // 27
    "pptx",         // MIME_PPTX            // 28
    "js",           // MIME_JAVASCRIPT      // 29
    "epub",         // MIME_EPUB            // 30
    "tex"          // MIME_TEX              // 31
};
