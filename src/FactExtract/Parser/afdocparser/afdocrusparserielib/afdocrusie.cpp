#include "afdocrusie.h"

class CParserOptions;

CDocIE::CDocIE(docLanguage lang)
    : CDocSingleLanguage(lang)
{
    if (!GlobalDataInitialized())
        ythrow yexception() << "Global data is not initialized.";
}

CTextBase* CDocIE::CreateSingleTextClass(const CParserOptions* parserOptions) const
{
    return new CTextProcessor(parserOptions);
}
