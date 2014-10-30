#include "textprocessor.h"
#include "interview_fact_creator.h"
#include "normalization.h"
#include "referencesearcher.h"

CInterviewFactCreator::CInterviewFactCreator(yvector<CSentence*>& vecSentence, const TArticleRef& article)
    : m_vecSentence(vecSentence)
    , m_MainArticle(article)
{
}

bool CInterviewFactCreator::IsAlreadyCreated()
{
    for (size_t iSent = 0; iSent < m_vecSentence.size(); iSent++) {
        CSentenceRusProcessor* pSent = (CSentenceRusProcessor*)m_vecSentence[iSent];
        DECLARE_STATIC_RUS_WORD(kINTERVIEW, "_интервью");
        if (pSent->GetMultiWordCreator().GetFoundWords(SArtPointer(kINTERVIEW), KW_DICT, false))
            return true;
    }
    return false;
}

bool EqualFio(const SFullFIO& fio1, const SFullFIO& fio2)
{
    return fio1.equal_by_i(fio2) && TMorph::SimilarSurnSing(fio1.m_strSurname, fio2.m_strSurname);

}

void CInterviewFactCreator::HasFioInInterviewFact(SFullFIO& interviewFio, int iSent, int& iGoodCount, int& iBadCount, int& iMaybeCount)
{
    CSentenceRusProcessor* pSent = (CSentenceRusProcessor*)m_vecSentence[iSent];
    pSent->GetMultiWordCreator().GetReferenceSearcher()->SetCurSentence(iSent);
    DECLARE_STATIC_RUS_WORD(kHERO, "_герой_интервью");
    yvector<SWordHomonymNum>* pFoundWords = pSent->GetMultiWordCreator().GetFoundWords(SArtPointer(kHERO), KW_DICT);
    if (!pFoundWords)
        return;
    for (size_t i = 0; i < pFoundWords->size(); ++i) {
        SWordHomonymNum wh = pFoundWords->at(i);
        const CHomonym& h = pSent->m_Words[wh];
        CFactsWS* pFactsWS =  dynamic_cast<CFactsWS*>(h.GetSourceWordSequence());
        if (!pFactsWS)
            continue;
        for (int j = 0; j < pFactsWS->GetFactsCount(); ++j) {
            const CFactFields& fact = pFactsWS->GetFact(j);
            if (fact.GetFactName() == "HeroGood") {
                const CFioWS* pFio = fact.GetFioValue(CFactFields::Fio);
                if (EqualFio(pFio->m_Fio, interviewFio))
                    iGoodCount++;
            }
            if (fact.GetFactName() == "HeroBad") {
                const CFioWS* pFio = fact.GetFioValue(CFactFields::Fio);
                if (EqualFio(pFio->m_Fio, interviewFio))
                    iBadCount++;
            }
            if (fact.GetFactName() == "HeroMaybe") {
                const CFioWS* pFio = fact.GetFioValue(CFactFields::Fio);
                if (EqualFio(pFio->m_Fio, interviewFio))
                    iMaybeCount++;
            }
        }
    }
}

bool CInterviewFactCreator::CreateFact(Wtroka strFio, int iLastTitleSent)
{
    if (strFio.empty())
        return false;

    if (IsAlreadyCreated())
        return false;

    if (m_vecSentence.size() == 0)
        return false;
    CSentenceRusProcessor* pFirstSent = (CSentenceRusProcessor*)m_vecSentence[0];

    int iCurSent = pFirstSent->GetMultiWordCreator().GetReferenceSearcher()->GetCurSentence();

    strFio = StripString(strFio);
    static const Wtroka delims = CharToWide(" \t");
    size_t ii = strFio.find_first_of(delims);
    if (ii == Wtroka::npos)
        return false;
    Wtroka strSurname = StripString(strFio.substr(0, ii));
    Wtroka strName = StripString(strFio.substr(ii + 1));

    SFullFIO interviewFio;
    interviewFio.m_strName = strName;
    interviewFio.m_strSurname = strSurname;
    interviewFio.MakeUpper();

    SWordHomonymNum whFio;
    bool bFoundFio = false;

    size_t iSentWithFio = 0;

    for (size_t iSent = 0; iSent < m_vecSentence.size(); iSent++) {

        if (!bFoundFio) {
            CSentenceRusProcessor* pSent = (CSentenceRusProcessor*)m_vecSentence[iSent];
            TKeyWordType fio_kw = GlobalDictsHolder->BuiltinKWTypes().Fio;
            yvector<SWordHomonymNum>* pFoundWords = pSent->GetMultiWordCreator().GetFoundWords(SArtPointer(fio_kw),KW_DICT);
            if (!pFoundWords)
                continue;
            for (size_t j = 0; j < pFoundWords->size(); j++) {
                SWordHomonymNum whFioTemp = pFoundWords->at(j);
                const CHomonym& h = pSent->m_Words[whFioTemp];
                CFioWordSequence* pFioWS =  dynamic_cast<CFioWordSequence*>(h.GetSourceWordSequence());
                if (!pFioWS)
                    continue;

                if (EqualFio(pFioWS->m_Fio, interviewFio)) {
                    bFoundFio = true;
                    whFio = whFioTemp;
                    iSentWithFio = iSent;
                    break;
                }
            }
        }
    }

    pFirstSent->GetMultiWordCreator().GetReferenceSearcher()->SetCurSentence(iCurSent);

    if (!bFoundFio || !whFio.IsValid())
        return false;

    CFactFields factFields("Interview");
    factFields.m_LeadInfo.m_iFirstSent = iLastTitleSent + 1;
    //if(iSentWithFio > iLastTitleSent)
    //    factFields.m_LeadInfo.m_iLastSent = (iSentWithFio == m_vecSentence.size()-1)? iSentWithFio : iSentWithFio+1;
    //else
    if (iLastTitleSent + 4 < (int)m_vecSentence.size() - 1)
        factFields.m_LeadInfo.m_iLastSent = iLastTitleSent + 4;
    else
        factFields.m_LeadInfo.m_iLastSent = m_vecSentence.size()-1;

    CSentenceRusProcessor* pSent = (CSentenceRusProcessor*)m_vecSentence[iSentWithFio];
    const CHomonym& h = pSent->m_Words[whFio];
    CFioWordSequence* pFioWS =  dynamic_cast<CFioWordSequence*>(h.GetSourceWordSequence());

    CFioWS newFioWS(*pFioWS);
    newFioWS.SetMainWord(whFio);
    factFields.AddValue(CFactFields::Fio, newFioWS);

    CFactsWS* factWS = new CFactsWS();
    factWS->SetPair(*pFioWS);
    factWS->PutArticle(m_MainArticle);
    factWS->AddFact(factFields);
    factWS->SetMainWord(whFio);
    pSent->GetMultiWordCreator().TakeMultiWord(factWS);
    return true;
}
