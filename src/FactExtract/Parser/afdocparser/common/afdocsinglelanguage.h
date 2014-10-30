#pragma once

#include "afdocbase.h"
#include "textbase.h"

#include <util/generic/ptr.h>

class CParserOptions;

class CDocSingleLanguage: public CDocBase
{
public:
    CDocSingleLanguage(docLanguage lang)
        : Language(lang)
    {
    }

    virtual ~CDocSingleLanguage() {
    }

    virtual void CreateTextClass(const CParserOptions* parserOptions);
    virtual CTextBase* CreateExternalTextClass(const CParserOptions* parserOptions);
    virtual CTextBase* CreateExternalTextClass(TOutputStream* pErrorStream, const CParserOptions* parserOptions) const;

protected:
    virtual CTextBase* CreateSingleTextClass(const CParserOptions* parserOptions) const = 0;

    docLanguage Language;   // unused really
};
