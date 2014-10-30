#include "parmbase.h"
#include <util/stream/file.h>
#include <util/folder/dirut.h>

bool CParmBase::AnalizeParameters(const yvector<Stroka>& params)
{
    for (size_t i = 0; i < params.size(); i++) {
        TStringBuf left, right;
        if (TStringBuf(params[i]).SplitImpl(':', left, right))
            AnalizeParameter(left, right);
        else
            AnalizeParameter(params[i]);
    }
    return CheckParameters();
}

bool CParmBase::GetParametersFromFile(const Stroka& strParmFileName, yvector<Stroka>& params)
{
    if (!isexist(strParmFileName.c_str())) {
        m_strError += "Can't open file " + ToString(strParmFileName);
        return false;
    }

    TBufferedFileInput file(strParmFileName);

    Stroka strParm;
    while (file.ReadLine(strParm))
        params.push_back(strParm);
    return true;
}

bool CParmBase::AnalizeParameters(int argc, char *argv[])
{
    try {
        Stroka strParmFileName;
        if (argc == 1)
            return false;

        if (argc == 2) {
            TStringBuf left, right;
            if (TStringBuf(argv[1]).SplitImpl(':', left, right) || left != "/parm")
                strParmFileName = ToString(right);
        }

        yvector<Stroka> params;
        if (!strParmFileName.empty()) {
            if (!GetParametersFromFile(strParmFileName, params))
                return false;
        } else
            for (int i = 1; i < argc; ++i)
                params.push_back(argv[i]);

        return AnalizeParameters(params);

    } catch (yexception& e) {
        m_strError += e.what();
        return false;
    } catch (...) {
        m_strError += "Unknown error in \"CParmBase::AnalizeParameters\"";
        return false;
    }
}
