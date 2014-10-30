#include "afdocsinglelanguage.h"

CTextBase* CDocSingleLanguage::CreateExternalTextClass(const CParserOptions* parserOptions)
{
    YASSERT(GlobalDataInitialized());

    CTextBase* pText = CreateSingleTextClass(parserOptions);
    pText->loadCharTypeTable();
    pText->SetErrorStream(m_pErrorStream);
    return pText;
}

CTextBase* CDocSingleLanguage::CreateExternalTextClass(TOutputStream* pErrorStream, const CParserOptions* parserOptions) const
{
    YASSERT(GlobalDataInitialized());

    CTextBase* pText = CreateSingleTextClass(parserOptions);
    pText->loadCharTypeTable();
    pText->SetErrorStream(pErrorStream);
    return pText;
}

void CDocSingleLanguage::CreateTextClass(const CParserOptions* parserOptions)
{
  if (m_pText.Get() == NULL)
      m_pText.Reset(CreateExternalTextClass(parserOptions));
}
