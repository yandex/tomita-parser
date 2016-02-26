#include "parseroptions.h"

#include "prettysitwriter.h"

TXmlNodePtrBase CPrettySitWriter::AddSentence(CSentenceRusProcessor* pSent, TXmlNodePtrBase piPar, int iSent)
{
    TXmlNodePtrBase piSent = CPrettyWriter::AddSentence(pSent, piPar, iSent);

    const SKWDumpOptions& options = ParserOptions.m_KWDumpOptions;

    for (yset<SArtPointer>::const_iterator it = options.SitToFind.begin(); it != options.SitToFind.end(); ++it) {
        const yvector<SValenciesValues>* pValValues = pSent->GetFoundVals(*it);
        if (pValValues != NULL) {
            for (size_t i = 0; i < pValValues->size(); i++) {
                const SValenciesValues& val = (*pValValues)[i];
                AddSituationTable(piSent, pSent, val);
            }
        }
    }
    return piSent;
}

void CPrettySitWriter::AddSituationTable(TXmlNodePtrBase piSent, CSentence* pSent, const SValenciesValues& val)
{
    TXmlNodePtr piSit("situation");
    Stroka strSitname;

    if (val.m_NodeWH.IsValid())
        strSitname = Substitute("$0.$1 ($2)", WideToUTF8(val.m_pArt->get_title()),
                                WideToUTF8(val.m_pArt->get_subordination(val.m_iSubord).m_str_subbord_name),
                                WideToUTF8(pSent->m_Words.GetWord(val.m_NodeWH).GetOriginalText()));
    else
        strSitname = Substitute("$0.$1 ", WideToUTF8(val.m_pArt->get_title()),
                                WideToUTF8(val.m_pArt->get_subordination(val.m_iSubord).m_str_subbord_name));

    piSit.AddAttr("sit_name", strSitname);
    for (size_t i = 0; i < val.m_values.size(); i++) {
        const SValencyValue v = val.m_values[i];
        TXmlNodePtr piVal("val");
        piVal.AddAttr("val_name", v.m_strVal);
        Wtroka strWord = pSent->m_Words.GetWord(v.m_Value).GetOriginalText();
        SubstGlobal(strWord, ASCIIToWide("_"), ASCIIToWide(" "));
        piVal.AddAttr("val_value", strWord);
        piSit.AddChild(piVal);
    }
    piSent.AddChild(piSit.Release());
}

