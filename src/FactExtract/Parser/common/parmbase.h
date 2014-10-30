#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

class CParmBase
{
public:
    virtual ~CParmBase(void)
    {
    }

    virtual bool AnalizeParameters(int argc, char *argv[]);
    virtual void PrintParameters() = 0;

public:
    Stroka m_strError;

protected:
    bool GetParametersFromFile(const Stroka& strParmFileName, yvector<Stroka>& params);
    bool AnalizeParameters(const yvector<Stroka>& params);
    virtual void AnalizeParameter(const TStringBuf& paramName, const TStringBuf& paramValue) = 0;
    virtual void AnalizeParameter(const TStringBuf& paramName) = 0;
    virtual bool CheckParameters() = 0;

};
