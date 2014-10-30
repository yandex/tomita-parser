#include "extractedfieldsearcher.h"
#include "textprocessor.h"

void CExtractedFieldSearcher::SearchExtractedNamesInText(int iSentNum, yvector<CWordSequence*>& rFoundFPCs)
{
    if (!m_pReferenceSearcher->IsArtificialFactProcessed(m_eArtAlg)) {
        m_pReferenceSearcher->SetArtificialFactProcessed(m_eArtAlg);
        StartGrammarsContainingExtractedField();
        m_pReferenceSearcher->BuildTextIndex();
        CollectUniqueNamesFromText();
        ProvideExtNameSearchByDocIndex();
    }

    m_pReferenceSearcher->GetArtificialFacts(iSentNum, rFoundFPCs);
}

void CExtractedFieldSearcher::StartGrammarsContainingExtractedField()
{
    const TBuiltinKWTypes& kwtypes = GlobalDictsHolder->BuiltinKWTypes();
    CTextProcessor& text = m_pReferenceSearcher->TextProcessor;
    switch (m_eArtAlg) {
    case CompInTextAlg:
        text.FindMultiWords(SArtPointer(kwtypes.FdoChain));
        text.FindMultiWords(SArtPointer(kwtypes.CompanyChain));
        text.FindMultiWords(SArtPointer(kwtypes.StockCompany));
        break;

    case GeoInTextAlg:
        text.FindMultiWords(SArtPointer(kwtypes.Geo));
        break;

    default:
        break;
    }
/*
    int iCurSentSave = m_pReferenceSearcher->GetCurSentence();
    for (size_t i = 0; i < m_pReferenceSearcher->m_vecSentence.size(); i++) {
        const CDictsHolder* dicts = GlobalDictsHolder;
        m_pReferenceSearcher->SetCurSentence(i);
        switch (m_eArtAlg) {
        case CompInTextAlg:
            m_pReferenceSearcher->GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), dicts->BuiltinKWTypes().FdoChain);
            m_pReferenceSearcher->GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), dicts->BuiltinKWTypes().CompanyChain);
            m_pReferenceSearcher->GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), dicts->BuiltinKWTypes().StockCompany);
            break;

        case GeoInTextAlg:
            m_pReferenceSearcher->GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), dicts->BuiltinKWTypes().Geo);
            break;

        default:
            break;
        }
    }
    m_pReferenceSearcher->SetCurSentence(iCurSentSave);
*/
}

void CExtractedFieldSearcher::CollectUniqueNamesFromText()
{
    yset<int> v_ProcessedCompanies;

    LoadGrammarExtractedFacts();

    BuildCompaniesIndex();

    for (size_t i = 0; i < m_DocGramExtVals.size(); i++) {
        if (v_ProcessedCompanies.find(i) != v_ProcessedCompanies.end())
            continue;

        v_ProcessedCompanies.insert(i);
        m_UniqueGramExtVals.push_back(i);

        for (size_t j = i + 1; j < m_DocGramExtVals.size(); ++j)
            if (v_ProcessedCompanies.find(j) == v_ProcessedCompanies.end() && IsEqualExtNameIndex(i, j) && IsEqualExtNameWords(i, j))
                v_ProcessedCompanies.insert(j);

    }
}

void CExtractedFieldSearcher::LoadGrammarExtractedFacts()
{
    for (size_t i = 0; i < m_pReferenceSearcher->m_vecSentence.size(); i++) {
        CSentenceRusProcessor *ptrSentProcessor = m_pReferenceSearcher->GetSentenceProcessor(i);
        const CWordSequence::TSharedVector& rSequences = ptrSentProcessor->GetWordSequences();
        for (size_t j = 0; j < rSequences.size(); j++) {
            const CFactsWS* pFacts = dynamic_cast<CFactsWS*> (rSequences[j].Get());
            if (!pFacts) continue;
            for (int k = 0; k < pFacts->GetFactsCount(); k++) {
                switch (m_eArtAlg) {
                case CompInTextAlg: {
                    const CTextWS* pComp = pFacts->GetFact(k).GetTextValue(CFactFields::CompanyName);
                    if (pComp != NULL
                        && !pComp ->HasAnaphoraOrEllipsis()
                        && "GEOOrgAsCompName" != pComp ->GetNonTerminalSem()
                        && "MinOrgName" != pComp ->GetNonTerminalSem()
                        && "PostMinBlock" != pComp ->GetNonTerminalSem()
                        )
                        m_DocGramExtVals.push_back(SFieldToSent(i, pComp));

                    pComp = pFacts->GetFact(k).GetTextValue(CFactFields::CompanyShortName);
                    if (pComp != NULL)
                        m_DocGramExtVals.push_back(SFieldToSent(i, pComp));

                    break;
                                    }
                case GeoInTextAlg: {
                    const CTextWS* pGeo = pFacts->GetFact(k).GetTextValue(CFactFields::Settlement);
                    if (pGeo != NULL)
                        m_DocGramExtVals.push_back(SFieldToSent(i, pGeo));

                    break;
                                   }
                default: return;
                }
            }
        }
    }
}

void CExtractedFieldSearcher::BuildCompaniesIndex()
{
    for (size_t i = 0; i < m_DocGramExtVals.size(); i++) {
        CSentenceRusProcessor *ptrSentenceRus = m_pReferenceSearcher->GetSentenceProcessor(m_DocGramExtVals[i].m_iSent);

        m_FirstWrdGramExtValIndex.insert(TPair<int, yvector< yset<long> > >(i, yvector< yset<long> >(0)));
        m_FirstWrdGramExtValIndex.find(i)->second.resize(m_DocGramExtVals[i].m_pExtField->Size());

        const CWordVector& rRusWords = ptrSentenceRus->GetRusWords();

        for (int j = m_DocGramExtVals[i].m_pExtField->FirstWord(); j <= m_DocGramExtVals[i].m_pExtField->LastWord(); j++) {
            const CWord& w = rRusWords.GetOriginalWord(j);
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it) {
                Wtroka s_lem = it->GetLemma();
                if (s_lem.empty())
                    continue;

                long l_ind = m_pReferenceSearcher->GetThreeLetterIndex(s_lem);
                m_FirstWrdGramExtValIndex.find(i)->second[j - m_DocGramExtVals[i].m_pExtField->FirstWord()].insert(l_ind);
            }
        }
    }
}

bool CExtractedFieldSearcher::IsEqualExtNameIndex(int iCompF, int iCompS) const
{
    ymap< int, yvector< yset<long> > >::const_iterator it_f = m_FirstWrdGramExtValIndex.find(iCompF);
    ymap< int, yvector< yset<long> > >::const_iterator it_s = m_FirstWrdGramExtValIndex.find(iCompS);

    if (it_f->second.size() != it_s->second.size()) return false;

    for (size_t i = 0; i < it_f->second.size(); i++) {
        yset<long>::const_iterator it_hom_ind = it_f->second[i].begin();
        for (; it_hom_ind != it_f->second[i].end(); it_hom_ind++)
            if (it_s->second[i].find(*it_hom_ind) != it_s->second[i].end())
                break;
        if (it_hom_ind == it_f->second[i].end()) return false;
    }

    return true;
}

bool CExtractedFieldSearcher::IsEqualExtNameWords(int iCompF, int iCompS) const
{
    const CTextWS& m_CompF = *(m_DocGramExtVals[iCompF].m_pExtField);
    const CTextWS& m_CompS = *(m_DocGramExtVals[iCompS].m_pExtField);

    if (m_CompF.Size() != m_CompS.Size()) return false;

    CSentenceRusProcessor *ptrSentenceRusF = m_pReferenceSearcher->GetSentenceProcessor(m_DocGramExtVals[iCompF].m_iSent);
    CSentenceRusProcessor *ptrSentenceRusS = m_pReferenceSearcher->GetSentenceProcessor(m_DocGramExtVals[iCompS].m_iSent);

    SArtPointer cmp_art(GlobalDictsHolder->BuiltinKWTypes().CompanyInQuotes);

    const CWordVector& rRusWordsF = ptrSentenceRusF->GetRusWords();
    const CWordVector& rRusWordsS = ptrSentenceRusS->GetRusWords();

    for (size_t i = 0; i < m_CompF.Size(); ++i) {
        const CWord& WF = rRusWordsF.GetOriginalWord(i + m_CompF.FirstWord());
        const CWord& WS = rRusWordsS.GetOriginalWord(i + m_CompS.FirstWord());

        if (!IsEqualWords(WF, WS, cmp_art))
            return false;
    }

    return true;
}

void CExtractedFieldSearcher::ProvideExtNameSearchByDocIndex()
{
    yset< TPair<int, int> > v_CompCandidates; //кандидаты на заполнение имени компании, полученные пересечением трехбуквенного индекса
                                            //пара[номер предложения, номер первого слова]
    for (size_t i = 0; i < m_UniqueGramExtVals.size(); i++) //цикл по уникальным компаниям-образцам, выделенными грамматиками
    {
        v_CompCandidates.clear();
        const_val_ind_it CompIndIt = m_FirstWrdGramExtValIndex.find(m_UniqueGramExtVals[i]);
        for (size_t j = 0; j < CompIndIt->second.size(); j++) //цикл по словам, из которых состоит компания-образец
        {
            //цикл по омонимам слова, которое входит в компанию-образец
            for (yset<long>::const_iterator HomIndIt = CompIndIt->second[j].begin(); HomIndIt != CompIndIt->second[j].end(); HomIndIt++) {
                _text_ind_const_it TextIndIt = m_pReferenceSearcher->m_TextIndex.find(*HomIndIt);
                assert(TextIndIt != m_pReferenceSearcher->m_TextIndex.end());

                if (0 == j)
                    v_CompCandidates.insert(TextIndIt->second.begin(), TextIndIt->second.end());
                else {
                    yset< TPair<int, int> > v_CandidatesIntersect;
                    for (yset< TPair<int, int> >::const_iterator it_candidate = v_CompCandidates.begin(); it_candidate != v_CompCandidates.end(); it_candidate++)
                        for (yset< TPair<int, int> >::const_iterator it_word_text = TextIndIt->second.begin(); it_word_text != TextIndIt->second.end(); it_word_text++)
                            if (it_candidate->first == it_word_text->first
                                && it_candidate->second + (int)j == it_word_text->second
                                )
                                v_CandidatesIntersect.insert(*it_candidate);

                    v_CompCandidates.clear();
                    v_CompCandidates.insert(v_CandidatesIntersect.begin(), v_CandidatesIntersect.end());
                }
            }
            if (0 == v_CompCandidates.size()) break;
        }
        if (0 == v_CompCandidates.size())
            continue;
        //проверить выделенные компании, чтобы не строить одну компанию дважды: m_DocGramExtVals
        CompareCandidatesWithExtractedPatterns(v_CompCandidates);
        //сравнить каждого кандидата с образцом по-словно: HasCommonHomonyms(...)
        CompareCandidatesWithExtractedPatternWordByWord(v_CompCandidates, m_UniqueGramExtVals[i]);
        //породить факт "CompanyDescr" для успешных кандидатов по компании-образцу: m_pReferenceSearcher->m_DocNonGrammarFacts.m_DocArtificialExtractedFacts
        BuildArtificialCandidates(v_CompCandidates, m_UniqueGramExtVals[i]);
    }
}

void CExtractedFieldSearcher::CompareCandidatesWithExtractedPatterns(yset< TPair<int, int> >& rArtExtCandidates)
{
    for (size_t i = 0; i < m_DocGramExtVals.size(); i++) {
        if (0 == rArtExtCandidates.size()) break;

        for (yset< TPair<int, int> >::iterator it_candidate = rArtExtCandidates.begin();
              it_candidate != rArtExtCandidates.end(); it_candidate++)
            if (m_DocGramExtVals[i].m_iSent == it_candidate->first
                 && m_DocGramExtVals[i].m_pExtField->FirstWord() == it_candidate->second) {
                rArtExtCandidates.erase(it_candidate);
                break;
            }
    }
}

void CExtractedFieldSearcher::CompareCandidatesWithExtractedPatternWordByWord(yset< TPair<int, int> >& rArtExtCandidates,
                                                                              int iPatternID)
{
    const CTextWS& rPattern = *(m_DocGramExtVals[iPatternID].m_pExtField);

    CSentenceRusProcessor *ptrSentPattern = m_pReferenceSearcher->GetSentenceProcessor(m_DocGramExtVals[iPatternID].m_iSent);
    const CWordVector& rRusWordsPattern = ptrSentPattern->GetRusWords();

    SArtPointer cmp_art(GlobalDictsHolder->BuiltinKWTypes().CompanyInQuotes);

    yvector< TPair<int, int> > v_EraseCandidate;
    for (size_t i = 0; i < rPattern.Size(); ++i) {
        const CWord& WPatt = rRusWordsPattern.GetOriginalWord(rPattern.FirstWord()+i);
        for (yset< TPair<int, int> >::iterator it_candidate = rArtExtCandidates.begin();
              it_candidate != rArtExtCandidates.end(); it_candidate++) {
            CSentenceRusProcessor *ptrSentCandidate = m_pReferenceSearcher->GetSentenceProcessor(it_candidate->first);
            const CWordVector& rRusWordsCandidate = ptrSentCandidate->GetRusWords();
            const CWord& WCand = rRusWordsCandidate.GetOriginalWord(it_candidate->second + i);

            if (!IsEqualWords(WPatt, WCand, cmp_art))
                v_EraseCandidate.push_back(*it_candidate);
        }
        for (size_t j = 0; j < v_EraseCandidate.size(); j++)
            rArtExtCandidates.erase(v_EraseCandidate[j]);
        if (0 == rArtExtCandidates.size())
            break;
        v_EraseCandidate.clear();
    }
}

bool CExtractedFieldSearcher::IsEqualWords(const CWord& rWPatt, const CWord& rWCand, const SArtPointer& art) const
{
    return rWPatt.m_bUp == rWCand.m_bUp
           && rWPatt.HasAtLeastTwoUpper() == rWCand.HasAtLeastTwoUpper()
           && rWPatt.HasArticle(art) == rWCand.HasArticle(art)
           && rWPatt.HasCommonHomonyms(rWCand);
}

void CExtractedFieldSearcher::BuildArtificialCandidates(yset< TPair<int, int> >& rArtExtCandidates, int iPatternID)
{
    if (0 == rArtExtCandidates.size()) return;

    const CTextWS& rPattern = *(m_DocGramExtVals[iPatternID].m_pExtField);
    CSentenceRusProcessor *ptrSentPattern = m_pReferenceSearcher->GetSentenceProcessor(m_DocGramExtVals[iPatternID].m_iSent);
    const CWordVector& rRusWordsPattern = ptrSentPattern->GetRusWords();

    const SWordHomonymNum& rMainWordPatt = rPattern.GetMainWord();
    const CWord& rMainPatt = rRusWordsPattern.GetWord(rMainWordPatt);
    const CHomonym& rMainHomPatt = rMainPatt.GetRusHomonym(rMainWordPatt.m_HomNum);
    TKeyWordType kw = GlobalDictsHolder->GetKWType(rMainHomPatt, KW_DICT);

    const TGramBitSet mask = NSpike::AllCases | NSpike::AllGenders | TGramBitSet(gSingular);

    for (yset< TPair<int, int> >::iterator it_candidate = rArtExtCandidates.begin();
          it_candidate != rArtExtCandidates.end(); it_candidate++) {
        CSentenceRusProcessor *ptrSentCandidate = m_pReferenceSearcher->GetSentenceProcessor(it_candidate->first);
        const CWordVector& rRusWordsCandidate = ptrSentCandidate->GetRusWords();

        CFactsWS* pFactWS = new CFactsWS();
        pFactWS->SetPair(it_candidate->second, (it_candidate->second + rPattern.Size()-1));

        CFactFields fact(GetExtractedFactName());
        CTextWS CompWS;
        CompWS.SetArtificialPair(*pFactWS);
        static const Wtroka EMPTY_WS = CharToWide("empty");
        CompWS.AddLemma(SWordSequenceLemma(EMPTY_WS)); //можно сделать идеологично и брать значение поля по умолчанию из fact_field_descr_t

        if (CompInTextAlg == m_eArtAlg) {
            fact.AddValue(CFactFields::CompanyIsLemma, CBoolWS(*pFactWS, false));
            fact.AddValue(CFactFields::CompanyDescr, CompWS);
        }

        CompWS.ClearLemmas();
        CompWS.SetArtificialPair(false);
        Wtroka sLemm;
        ptrSentPattern->GetWSLemmaString(sLemm, rPattern, false);
        CompWS.AddLemma(SWordSequenceLemma(sLemm));

        //ищем вершину (главное слово) для новой группы имени компании
        SWordHomonymNum mainWHN;
        if (kw != NULL) {
            yvector<SWordHomonymNum>* rKWMultiWords = ptrSentCandidate->GetMultiWordCreator().GetFoundWords(kw, KW_DICT);
            if (rKWMultiWords != NULL) {
                for (size_t i = 0; i < rKWMultiWords->size(); i++) {
                    const CWord& rWrdCandidate = rRusWordsCandidate.GetWord(rKWMultiWords->at(i));
                    if (CompWS.Includes(rWrdCandidate.GetSourcePair())) {
                        mainWHN = rKWMultiWords->at(i); break;
                    }
                }
            }
        } else if (rMainWordPatt.m_bOriginalWord) {
            mainWHN.m_WordNum = CompWS.FirstWord()+(rMainWordPatt.m_WordNum - rPattern.FirstWord());
            CWord& rWrdCandidate = rRusWordsCandidate.GetOriginalWord(mainWHN.m_WordNum);
            for (CWord::SHomIt hom_for_main_it = rWrdCandidate.IterHomonyms(); hom_for_main_it.Ok(); ++hom_for_main_it)
                if (rMainHomPatt.HasSamePOS(*hom_for_main_it) &&
                    rMainHomPatt.GetLemma() == hom_for_main_it->GetLemma()) {
                    mainWHN.m_HomNum = hom_for_main_it.GetID();
                    break;
                }
        }

        if (-1 == mainWHN.m_HomNum) {
            mainWHN.m_WordNum = it_candidate->second;
            CWord& rWrdCandidate = rRusWordsCandidate.GetOriginalWord(mainWHN.m_WordNum);
            CWord::SHomIt hom_for_main_it = rWrdCandidate.IterHomonyms();
            mainWHN.m_HomNum = hom_for_main_it.GetID();
        }
        //закончили поиск главного слова для новой группы

        const CWord& rWrdCandidate = rRusWordsCandidate.GetWord(mainWHN);
        const CHomonym& rHomCandidate = rWrdCandidate.GetRusHomonym(mainWHN.m_HomNum);

        pFactWS->SetMainWord(mainWHN);
        if (!rHomCandidate.HasUnknownPOS())
            pFactWS->SetGrammems(rHomCandidate.Grammems);
        else
            pFactWS->SetGrammems(mask);

        pFactWS->PutArticleOfHomonym(rHomCandidate, KW_DICT);

        CompWS.SetMainWord(mainWHN);
        if (!rHomCandidate.HasUnknownPOS())
            CompWS.SetGrammems(rHomCandidate.Grammems);
        else
            CompWS.SetGrammems(mask);

        CompWS.PutArticleOfHomonym(rHomCandidate, KW_DICT);

        fact.AddValue(GetExtractedFieldName(), CompWS);
        pFactWS->AddFact(fact);
        PushArtificialFacts(it_candidate->first, pFactWS);
    }

}

void CExtractedFieldSearcher::PushArtificialFacts(int iSentNum, CFactsWS* pFactWS)
{
    m_pReferenceSearcher->PushArtificialFacts(iSentNum, pFactWS);
}

Stroka CExtractedFieldSearcher::GetExtractedFactName() const
{
    switch (m_eArtAlg) {
    case CompInTextAlg:
        return "CompanyDescr";

    case GeoInTextAlg:
        return "ComplexGeo";

    default:
        return Stroka();
    }
}

Stroka CExtractedFieldSearcher::GetExtractedFieldName() const
{
    switch (m_eArtAlg) {
    case CompInTextAlg:
        return "CompanyName";

    case GeoInTextAlg:
        return "Settlement";

    default:
        return Stroka();
    }
}
