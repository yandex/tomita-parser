#include "afdocbase.h"


CDocBase::CDocBase()
{
    m_dtyp = DocumentText;
    m_dnum = 0;
    m_pErrorStream = NULL;
    m_iTagsSize = 0;
}

CDocBase::~CDocBase(void)
{
    FreeData();
    RemoveDocLangAltInfo();
}

void CDocBase::FreeData()
{
    m_iTagsSize = 0;
    m_Source.clear();
    if (m_pText.Get() != NULL)
        m_pText->FreeData();
}

/*
void CDocBase::SetDocLang(docLanguage lang)
{
    Language = lang;
}
*/
void CDocBase::AddDocLangAltInfo(docLanguage lang, bool bUseAsPrime, bool bUseAlways)
{
    typeLangAltInfo info;
    info.lang = lang;
    info.bUseAsPrime = bUseAsPrime;
    info.bUseAlways = bUseAlways;
    AddDocLangAltInfo(info);
}

void CDocBase::AddDocLangAltInfo(const typeLangAltInfo &langAltInfo)
{
    m_vecLangAltInfo.push_back(langAltInfo);
}

void CDocBase::SetDocType(ETypeOfDocument type)
{
    m_dtyp = type;
}

void CDocBase::SetDocNum(ui32 num)
{
    m_dnum = num;
}

//////////////////////////////////////////////////////////////////////////////
// Proceed

bool CDocBase::Proceed(const TWtringBuf& src, const CParserOptions* parserOptions)
{
    SDocumentAttribtes docAttrs;
    return Proceed(src, docAttrs, parserOptions);
}

bool CDocBase::ProceedOnlyMorph(const TWtringBuf& src, const SDocumentAttribtes& docAttrs)
{
    return ProceedInternal(src, docAttrs, NULL, false);
}

bool CDocBase::Proceed(const TWtringBuf& src, const SDocumentAttribtes& docAttrs, const CParserOptions* parserOptions)
{
    return ProceedInternal(src, docAttrs, parserOptions, true);
}

bool CDocBase::ProceedInternal(const TWtringBuf& src, const SDocumentAttribtes& docAttrs, const CParserOptions* parserOptions, bool bAnalyze)
{

    // if caller forgot this...
        FreeData();
    // this really allocate memory only on first call

        //лениво создаем m_pText
        CreateTextClass(parserOptions);

        GetText()->PutAttributes(docAttrs);
        GetText()->SetParserOptions(parserOptions);
        const size_t MAX_SRC_SIZE = 2*1024*1024;
        m_Source = Wtroka(~src, (src.size() < MAX_SRC_SIZE) ? src.size() : MAX_SRC_SIZE);

        switch (m_dtyp) {
            case DocumentHtml:
                proceedHtml();
                break;
            default:
                proceedText();
        }
        if (bAnalyze) {
            if( parserOptions )
                m_pText->analyzeSentences();
            else
                ythrow yexception() << "ParserOptions are not initialized.";

        }
    return true;
}

int CDocBase::GetPlainTextSize()
{
    return m_Source.size() - m_iTagsSize;
}

