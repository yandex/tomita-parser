#include "leadgenerator.h"
#include <FactExtract/Parser/afdocparser/rusie/textprocessor.h>

size_t CLeadGenerator::s_iMaxLeadSentLength = 400;

CLeadGenerator::CLeadGenerator(const CTextProcessor& text, int iDocID, const Stroka& strUrl, ECharset encoding)
    : Encoding(encoding)
    , m_iMaxLeadID(0)
    , m_Text(text)
    , m_iDocID(iDocID)
    , m_strUrl(strUrl)
{
    m_piDoc = TXmlDocPtr("1.0");
    m_piRoot = m_piDoc.AddNewChild("l");
    m_piDoc.SetEncoding(Encoding);

    m_SentencesPeriods.resize(text.GetSentenceCount());
    GenerateHeader();
}

void CLeadGenerator::GenerateHeader()
{
    m_piHeader = m_piRoot.AddNewChild("h");
    m_piHeader.AddAttr("t", "ya");

    const SDocumentAttribtes& docAttrs = m_Text.GetAttributes();
    if (docAttrs.m_Date != 0) {
        CTime docTime(docAttrs.m_Date);
        m_piHeader.AddNewTextNodeChild("d", CharToWide(docTime.Format("%Y-%m-%d %H:%M")));
    }
    if (!docAttrs.m_strSource.empty())
        m_piHeader.AddNewTextNodeChild("s", CharToWide(docAttrs.m_strSource));

    if (!docAttrs.m_strTitle.empty())
        m_piHeader.AddNewTextNodeChild("t", docAttrs.m_strTitle);

    //m_piBody = xmlNewChild(m_piRoot.Get(), NULL, (xmlChar*)"b", NULL);
    m_piBody = m_piRoot.AddNewChild("b");

    m_piHeader.AddAttr("di", m_iDocID)
        .AddAttr("u", m_strUrl);

}

int CLeadGenerator::AddFact(const SFactAddress& factAddress, yvector<CAttrWP>& Attrs)
{
    const CFactFields& fact = m_Text.GetFact(factAddress);

    const CDictsHolder* pDictsHolder = GlobalDictsHolder;
    const fact_type_t& fact_type = pDictsHolder->RequireFactType(fact.GetFactName());

    yvector<SSentenceLead>& sentencePeriods = m_SentencesPeriods[factAddress.m_iSentNum];
    for (size_t i = 0; i < sentencePeriods.size(); ++i)
        if (AddToSentencePeriods(fact, fact_type, sentencePeriods[i], Attrs))
            return sentencePeriods[i].m_iLeadID;

    SSentenceLead newSentencePeriods;
    newSentencePeriods.m_iLeadID = m_iMaxLeadID++;
    newSentencePeriods.m_LeadInfo = fact.m_LeadInfo;
    AddToSentencePeriods(fact, fact_type, newSentencePeriods, Attrs);
    sentencePeriods.push_back(newSentencePeriods);
    return newSentencePeriods.m_iLeadID;
}

const yvector<CLeadGenerator::SSentenceLead>& CLeadGenerator::GetLeadsBySent(int iSent)
{
    yvector<CLeadGenerator::SSentenceLead>& leads = m_SentencesPeriods[iSent];
    for (size_t i = 0; i < leads.size(); ++i)
        if (leads[i].m_strLead.empty())
            GenerateLead(leads[i], iSent);
    return leads;
}

void CLeadGenerator::GenerateLead(SSentenceLead& sentenceLead, int iSent)
{
    TXmlNodePtr piSent = GenerateLeadSent(sentenceLead, iSent);
    m_piBody.AddChild(piSent.Release());

    if (m_Text.GetAttributes().m_TitleSents.first <= iSent && m_Text.GetAttributes().m_TitleSents.second >= iSent)
        m_piHeader.AddAttr("tl", iSent);
    sentenceLead.m_strLead = GetLeadString();

    if (sentenceLead.OutOfSent(iSent))
        sentenceLead.m_PureText = GeneratePureLeadText(sentenceLead, false);
    else
        sentenceLead.m_PureText = GenerateSpannedLead(sentenceLead, iSent);
}

Wtroka CLeadGenerator::GetLeadString()
{
    TStringStream ss;
    m_piRoot.SaveToStream(&ss, Encoding);
    Wtroka strRes = CharToWide(ss.Str(), Encoding);

    m_piBody.Unlink();
    m_piBody = m_piRoot.AddNewChild("b");

    m_piHeader.RemoveAttr("tl");
    return strRes;
}

Wtroka CLeadGenerator::GeneratePureLeadText(SSentenceLead& sentenceLead, bool trim)
{
    Wtroka strSent;
    for (int i = sentenceLead.m_LeadInfo.m_iFirstSent; i <= sentenceLead.m_LeadInfo.m_iLastSent; ++i) {
        strSent += m_Text.GetSentence(i)->ToString();
        strSent += ' ';
        if (trim && strSent.size() > s_iMaxLeadSentLength)
            break;
    }
    return strSent;
}

void CLeadGenerator::GeneratePureLead(SSentenceLead& sentenceLead, TXmlNodePtrBase piSent)
{
    piSent.AddNewTextNodeChild(GeneratePureLeadText(sentenceLead, true));
}

Wtroka CLeadGenerator::GenerateSpannedLead(SSentenceLead& sentenceLead, int iSent)
{
    const CSentence* pSent = m_Text.GetSentence(iSent);
    size_t wordCount = pSent->getWordsCount();

    // for sententce iSent make span map: from word to corresponding attr beginning or ending
    yvector< yvector<const CAttrWP*> > begs(wordCount), ends(wordCount);
    for (yset<CAttrWP>::const_iterator it = sentenceLead.m_Periods.begin(); it != sentenceLead.m_Periods.end(); ++it) {
        const CAttrWP& attrWP = *it;
        if (attrWP.FirstWord() < (int)wordCount)
            begs[attrWP.FirstWord()].push_back(&attrWP);
        if (attrWP.LastWord() < (int)wordCount)
            ends[attrWP.LastWord()].push_back(&attrWP);
    }

    Wtroka res;
    size_t utf8Len = 0;
    Stroka utf8Word;
    for (size_t w = 0; w < wordCount; ++w) {
        CWord* pW = pSent->getWordRus(w);
        const Wtroka& word = pW->GetOriginalText();

        if (w > 0 && RequiresSpace(pSent->getWordRus(w-1)->GetOriginalText(), word)) {
            res += ' ';
            utf8Len += 1;
        }

        for (size_t k = 0; k < begs[w].size(); ++k) {
            begs[w][k]->m_StartChar = res.length();
            begs[w][k]->m_StopChar = res.length();      // to guarantee m_StartChar <= m_StopChar
            begs[w][k]->m_StartByte = utf8Len;
            begs[w][k]->m_StopByte = utf8Len;
        }

        res += word;
        ::WideToUTF8(word, utf8Word);
        utf8Len += utf8Word.length();

        for (size_t k = 0; k < ends[w].size(); ++k) {
            ends[w][k]->m_StopChar = res.length();
            ends[w][k]->m_StopByte = utf8Len;
        }

        // do not trim
        //if (res.length() > s_iMaxLeadSentLength)
        //    break;
    }

    return res;
}

TXmlNodePtr CLeadGenerator::GenerateLeadSent(SSentenceLead& sentenceLead, int iSent)
{
    TXmlNodePtr piSent("s");
    if (sentenceLead.OutOfSent(iSent)) {
        GeneratePureLead(sentenceLead, piSent);
        return piSent;
    }

    Wtroka str;
    size_t iLeadTextLen = 0;
    const CSentence* pSent = m_Text.GetSentence(iSent);
    yset<CAttrWP>::iterator it = sentenceLead.m_Periods.begin();

    for (size_t i = 0; i < pSent->getWordsCount(); ++i) {
        CWord* pW = pSent->getWordRus(i);

        if (i > 0 && RequiresSpace(pSent->getWordRus(i-1)->GetOriginalText(), pW->GetOriginalText())) {
              str += ' ';
            iLeadTextLen++;
        }

        Wtroka sCurWord = pW->GetOriginalText();
        if (it != sentenceLead.m_Periods.end()) {
            const CAttrWP& attrWP = *it;
            if ((int)i == attrWP.FirstWord()) {
                piSent.AddNewTextNodeChild(str);
                str.clear();
            }

            //если лид уже слишком длинный, то закроем его и выйдем по-быстрому
            if ((int)i == attrWP.LastWord() || iLeadTextLen > s_iMaxLeadSentLength) {
                TXmlNodePtr piFragm(attrWP.m_strTagName);
                piFragm.AddAttr(attrWP.GetAttrName().c_str(), "");
                piFragm.AddAttr("lemma", attrWP.m_strCapitalizedName);
                str += sCurWord;
                iLeadTextLen += sCurWord.size();
                piFragm.AddNewTextNodeChild(str);
                piSent.AddChild(piFragm);
                str.clear();
                sCurWord.clear();
                it++;
            }
        }
        str += sCurWord;
        if (iLeadTextLen + sCurWord.size() > s_iMaxLeadSentLength)
            break;
        iLeadTextLen += sCurWord.size();
    }

    if (!str.empty() && iLeadTextLen <= s_iMaxLeadSentLength) {
        piSent.AddNewTextNodeChild(str);
        str.clear();
    }

    if (sentenceLead.m_LeadInfo.m_iFirstSent != -1 && sentenceLead.m_LeadInfo.m_iLastSent != -1) {
        for (int i = sentenceLead.m_LeadInfo.m_iFirstSent; i <= sentenceLead.m_LeadInfo.m_iLastSent; ++i) {
            Wtroka strSent = m_Text.GetSentence(i)->ToString();
            strSent += ' ';
            if (iLeadTextLen + strSent.size() > s_iMaxLeadSentLength) {
                if (i < iSent) {
                    piSent.AddChildFront(TXmlNodePtr::NewTextNode(str));
                    str.clear();
                }
                break;
            }
            if (i < iSent) {
                str += strSent;
                if (i == iSent - 1) {
                    piSent.AddChildFront(TXmlNodePtr::NewTextNode(str));
                    str.clear();
                }
            } else if (i > iSent) {
                if (i == iSent + 1)
                    str += ' ';
                str += strSent;
            } else
                continue;
            iLeadTextLen += strSent.size();
        }
    }

    if (!str.empty() && iLeadTextLen <= s_iMaxLeadSentLength)
        piSent.AddNewTextNodeChild(str);

    return piSent;
}

bool CLeadGenerator::AddToSentencePeriods(const CFactFields& fact, const fact_type_t&  fact_type,
                                          SSentenceLead& sentencePeriods, yvector<CAttrWP>& attrInfo)
{
    if (!(fact.m_LeadInfo == sentencePeriods.m_LeadInfo))
        return false;
    yvector<CAttrWP> fieldPeriods;
    for (size_t i = 0; i < fact_type.m_Fields.size(); ++i) {
        const fact_field_descr_t& field_descr = fact_type.m_Fields[i];
        if (field_descr.m_Field_type == FioField || field_descr.m_Field_type == TextField) {
            const CWordSequence* pWS = dynamic_cast<const CWordSequence*>(fact.GetValue(field_descr.m_strFieldName));
            if (pWS == NULL || !pWS->IsValid() || pWS->IsArtificialPair())
                continue;
            CAttrWP newWp;
            (CWordsPair&)newWp = *((const CWordsPair*)pWS);

            if (field_descr.m_Field_type == TextField) {
                newWp.m_strCapitalizedName = pWS->GetCapitalizedLemma();
                NStr::ReplaceChar(newWp.m_strCapitalizedName, '\"', ' ');
                newWp.m_strCapitalizedName = StripString(newWp.m_strCapitalizedName);
            }

            //имя тега по первой букве названия поля
            newWp.m_strTagName = Stroka(field_descr.m_strFieldName[0]);

            //оставим только максимальные по вложению
            bool bAdd = true;
            for (size_t k = 0; k < fieldPeriods.size(); ++k) {
                if (fieldPeriods[k].Includes(newWp)) {
                    bAdd = false;
                    break;
                }

                if (newWp.Includes(fieldPeriods[k])) {
                    fieldPeriods.erase(fieldPeriods.begin() + k);
                    break;
                }
            }

            if (bAdd)
                fieldPeriods.push_back(newWp);
            else
                continue;

            //проверим, нет ли пересечения с полями из предыдущих фактов
            yset<CAttrWP>::iterator it = sentencePeriods.m_Periods.begin();
            for (; it != sentencePeriods.m_Periods.end(); ++it) {
                const CAttrWP& wp = *it;
                if (newWp.Intersects(wp) && !(newWp == wp))
                    return false;
            }
        }
    }

    //если добрались до этого места, то значит можно добавлять
    for (size_t i = 0; i < fieldPeriods.size(); ++i) {
        yset<CAttrWP>::const_iterator it = sentencePeriods.m_Periods.find(fieldPeriods[i]);
        if (it != sentencePeriods.m_Periods.end())
            attrInfo.push_back(*it);
        else {
            fieldPeriods[i].m_Index = sentencePeriods.m_Periods.size();
            sentencePeriods.m_Periods.insert(fieldPeriods[i]);
            attrInfo.push_back(fieldPeriods[i]);
        }
    }

    return true;
}

