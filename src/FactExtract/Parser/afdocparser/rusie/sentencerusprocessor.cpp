#include <FactExtract/Parser/common/libxmlutil.h>
#include "sentencerusprocessor.h"
#include "fragmentationrunner.h"
#include <FactExtract/Parser/afdocparser/rus/namefinder.h>
#include "filtercheck.h"
#include "situationssearcher.h"
#include "anaphoraresolution.h"

CSentenceRusProcessor::CSentenceRusProcessor()
    : CSentence()
    , m_MultiWordCreator(m_Words, GetWordSequences())
    , m_clausesStructure(m_Words)
{
}

CSentenceRusProcessor::~CSentenceRusProcessor(void)
{
    FreeData();
}



void CSentenceRusProcessor::FreeData()
{
    CSentence::FreeData();
    m_clausesStructure.Reset();
}

CMultiWordCreator& CSentenceRusProcessor::GetMultiWordCreator()
{
    return m_MultiWordCreator;
}

bool CSentenceRusProcessor::RunFragmentation()
{
    GetClauseStructureOrBuilt();
    return true;
}

const CClauseVariant& CSentenceRusProcessor::GetClauseStructureOrBuilt()
{
    if (m_clausesStructure.IsEmpty()) {
        CFragmentationRunner fragmentator(this);
        fragmentator.Run(FragmOptions);
        fragmentator.GetFirstBestVar().Clone(m_clausesStructure);
        fragmentator.freeFragmResults();
    }

    return m_clausesStructure;
}

void CSentenceRusProcessor::PutReferenceSearcher(CReferenceSearcher* pRefSeracher)
{
    m_MultiWordCreator.PutReferenceSearcher(pRefSeracher);
}

bool CSentenceRusProcessor::SearchingOfSitWasPerformed(const Wtroka& strArt) const
{
    return m_SitFoundByArticles.find(strArt) != m_SitFoundByArticles.end();
}

const yvector<SValenciesValues>* CSentenceRusProcessor::GetFoundVals(const SArtPointer& artPointer)
{
    FindSituations(artPointer);
    if (artPointer.HasKWType()) {
        yhash<TKeyWordType, TValValuesPtr>::const_iterator ptr_it = m_KwType2FoundVals.find(artPointer.GetKWType());
        if (ptr_it != m_KwType2FoundVals.end() && ptr_it->second.Get() != NULL && ptr_it->second->size() > 0)
            return ptr_it->second.Get();
    } else {
        if (artPointer.HasStrType()) {
            ymap<Wtroka, TValValuesPtr>::const_iterator it = m_article2FoundVals.find(artPointer.GetStrType());
            if (it != m_article2FoundVals.end())
                return it->second.Get();
        }
    }
    return NULL; //empty
}

void CSentenceRusProcessor::AddToAlreadySearchedSit(const SArtPointer& artPointer)
{
    if (artPointer.HasKWType()) {
        TKeyWordType kwType = artPointer.GetKWType();
        const yvector<int>* artIndexes = GlobalDictsHolder->GetDict(KW_DICT).GetAuxArticles(kwType);
        YASSERT(artIndexes != NULL);
        for (size_t i = 0; i < artIndexes->size(); ++i) {
            const Wtroka& strArt = GlobalDictsHolder->GetAuxArticle(SDictIndex(KW_DICT, (*artIndexes)[i]))->get_title();
            m_SitFoundByArticles.insert(strArt);
        }
    } else if (artPointer.HasStrType())
        m_SitFoundByArticles.insert(artPointer.GetStrType());
}

void CSentenceRusProcessor::FindSituationsInner(const SArtPointer& artPointer)
{
    bool bRun = true;

    CFilterCheck filterCheck(m_MultiWordCreator);
    if (artPointer.HasKWType() && !filterCheck.CheckFiltersWithoutOrder(GlobalDictsHolder->GetFilter(artPointer.GetKWType())))
       bRun = false;

    if (bRun) {
        m_MultiWordCreator.m_bProcessedByGrammar = true;
        CSituationsSearcher SituationsSearcher(this);
        SituationsSearcher.Run(artPointer);
        const yvector<SValenciesValues>& valValues = SituationsSearcher.GetFoundVals();
        for (size_t i = 0; i < valValues.size(); ++i)
            AddFoundVals(valValues[i]);

        if (valValues.size() > 0 && artPointer.HasKWType())
            m_KwType2FoundVals[artPointer.GetKWType()] = new yvector<SValenciesValues>(valValues);
    }
    AddToAlreadySearchedSit(artPointer);
}

void CSentenceRusProcessor::AddFoundVals(const SValenciesValues& valenciesValues)
{
    const Wtroka& strArt = valenciesValues.m_pArt->get_title();
    ymap<Wtroka, TValValuesPtr>::iterator it = m_article2FoundVals.find(strArt);
    if (it == m_article2FoundVals.end()) {
        TValValuesPtr& ptr = m_article2FoundVals[strArt];
        ptr.Reset(new yvector<SValenciesValues>);
        ptr->push_back(valenciesValues);
    } else {
        if (it->second.Get() == NULL)
            it->second.Reset(new yvector<SValenciesValues>);
        it->second->push_back(valenciesValues);
    }
}

void CSentenceRusProcessor::FindSituations(const SArtPointer& artPointer)
{
    if (artPointer.HasKWType()) {
        TKeyWordType kwType = artPointer.GetKWType();
        const yvector<int>* artIndexes = GlobalDictsHolder->GetDict(KW_DICT).GetAuxArticles(kwType);
        YASSERT(artIndexes!= NULL);
        yvector<SArtPointer> artToFind; // запихиваем сюда те статьи, которые
                                        // не были найдены
        bool bAllNotFound = true;
        for (size_t i = 0; i < artIndexes->size(); i++) {
            const Wtroka& strArt = GlobalDictsHolder->GetAuxArticle(SDictIndex(KW_DICT, (*artIndexes)[i]))->get_title();
            if (!SearchingOfSitWasPerformed(strArt))
                artToFind.push_back(SArtPointer(strArt));
            else
                bAllNotFound = false;
        }
        if (bAllNotFound)
            FindSituationsInner(artPointer);
        else
            for (size_t i = 0; i < artToFind.size(); ++i)
                FindSituationsInner(artToFind[i]);
    } else if (artPointer.HasStrType() && !SearchingOfSitWasPerformed(artPointer.GetStrType()))
            FindSituationsInner(artPointer);
}

void CSentenceRusProcessor::GetFoundMWInLinks(yvector<SMWAddressWithLink>& mwFounds, const SArtPointer& artPointer) const
{
    if (!artPointer.IsForLink())
        return;
    if (m_SentMarkUps.size() == 0)
        return;

    yset<SSentMarkUp>::const_iterator it = m_SentMarkUps.begin();
    for (; it != m_SentMarkUps.end(); it++) {
        const SSentMarkUp& markUp = *it;
        yvector<SWordHomonymNum> whs;
        m_MultiWordCreator.GetFoundWordsInPeriod(artPointer, whs, markUp);
        for (size_t i = 0; i < whs.size(); i++) {
            mwFounds.push_back(SMWAddressWithLink(-1, whs[i]));
            mwFounds.back().m_Link = markUp;
        }
    }
}

void CSentenceRusProcessor::FindMultiWordsInLink(const SArtPointer& artPointer)
{
    yset<SSentMarkUp>::const_iterator it = m_SentMarkUps.begin();
    for (; it != m_SentMarkUps.end(); it++) {
        const SSentMarkUp& markUp = *it;
        m_MultiWordCreator.FindWordsInPeriod(artPointer, markUp);
    }
}

void CSentenceRusProcessor::FindMultiWords(const SArtPointer& artPointer)
{
    try {
        if (artPointer.IsForLink()) {
            if (m_SentMarkUps.size() > 0)
                FindMultiWordsInLink(artPointer);
        } else
            m_MultiWordCreator.FindKWWords(artPointer, KW_DICT);

    } catch (yexception& e) {
        PrintError("Error in CSentenceRusProcessor::FindMultiWords()", &e);
        throw;
    } catch (...) {
        PrintError("Error in CSentenceRusProcessor::FindMultiWords()", NULL);
        throw;
    }
}

void CSentenceRusProcessor::FindMultiWordsByArticle(const Wtroka& strArtTitle, TKeyWordType kwType)
{
    try {
        SArtPointer artPointer;
        if (kwType != NULL) {
            artPointer.PutKWType(kwType);
        }

        if (!strArtTitle.empty()) {
            artPointer.PutStrType(strArtTitle);
        }
        FindMultiWords(artPointer);
    } catch (yexception& e) {
        PrintError("Error in CSentenceRusProcessor::FindMultiWordsByArticle(), ", &e);
        throw;
    } catch (...) {
        PrintError("Error in CSentenceRusProcessor::FindMultiWordsByArticle(), ", NULL);
        throw;
    }
}

const CFactFields&  CSentenceRusProcessor::GetFact(const SFactAddress& factAddr) const
{
    CHomonym& h = const_cast<CHomonym&>(m_Words[factAddr]);
    CWordSequence* pWS = h.GetSourceWordSequence();
    if (pWS == NULL)
        ythrow yexception() << "Bad fact address in \"CSentenceRusProcessor::GetFact\"";
    CFactsWS* pFactsWS = dynamic_cast<CFactsWS*>(pWS);
    if (pFactsWS == NULL)
        ythrow yexception() << "Bad CWordSequence subclass in \"CSentenceRusProcessor::GetFact\"";
    return pFactsWS->GetFact(factAddr.m_iFactNum);
}

void CSentenceRusProcessor::FillFactAddresses(yset<SFactAddress>& factAddresses, SFactAddress& templateFactAddress)
{
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        const CWord& w = m_Words.GetAnyWord(i);
        templateFactAddress.m_WordNum = i;
        if (templateFactAddress.m_WordNum >= (int)m_Words.OriginalWordsCount())
            templateFactAddress.m_WordNum -= m_Words.OriginalWordsCount();
        if (w.IsMultiWord())
            templateFactAddress.m_bOriginalWord = false;
        else
            templateFactAddress.m_bOriginalWord = true;
        for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it) {
            CHomonym& h = (CHomonym&)(*it);
            CWordSequence* pWS = h.GetSourceWordSequence();
            CFactsWS* pFactsWS = dynamic_cast<CFactsWS*>(pWS);
            if (pFactsWS == NULL)
                continue;
            templateFactAddress.m_HomNum = it.GetID();
            for (int j = 0; j < pFactsWS->GetFactsCount(); j++) {
                SFactAddress factAddress;
                factAddress = templateFactAddress;
                factAddress.m_iFactNum = j;
                factAddresses.insert(factAddress);
            }
        }
    }
}

Wtroka CSentenceRusProcessor::GetCapitalizedLemma(const CWord& w, int, Wtroka strLemma)
{
    return CNormalization::GetCapitalizedLemma(w, strLemma);
}

void CSentenceRusProcessor::FindNames(yvector<CFioWordSequence>& foundNames,
                                      yvector<int>& potentialSurnames)
{
#ifdef NDEBUG
    try {
#endif
        InitNameTypes();
        CNameFinder nameFinder(m_Words);
        nameFinder.Run(foundNames, potentialSurnames);

        for (size_t i = 0; i < foundNames.size(); i++)
            ChangeFirstName(foundNames[i].GetFio(), foundNames[i]);

        yvector<SWordHomonymNum> foundWords;
        DECLARE_STATIC_RUS_WORD(kMWFio, "многословное_фио_");
        yvector<SWordHomonymNum>* pFoundWords = m_MultiWordCreator.GetFoundWords(SArtPointer(kMWFio), KW_DICT);
        yvector<COccurrence> multiWordsOccurrences;
        if (pFoundWords == NULL)
            return;
        foundWords = *pFoundWords;

        yvector<CWordsPair> fiosToDelete;

        for (size_t i = 0; i < foundWords.size(); i++) {
            const CWord& pW = m_Words.GetWord(foundWords[i]);

            COccurrence o(pW.GetSourcePair().FirstWord(), pW.GetSourcePair().LastWord() + 1, i);
            multiWordsOccurrences.push_back(o);
            if (IsBadMWNFio(m_Words[foundWords[i]].GetSourceWordSequence()))
                fiosToDelete.push_back(pW.GetSourcePair());
        }

        std::multiset<CWordsPair, SSimplePeriodOrder> manuallyFoundFios;
        for (size_t i = 0; i < foundNames.size(); i++) {
            const CWordSequence& ws = foundNames[i];
            COccurrence o(ws.FirstWord(), ws.LastWord() + 1, 0xFFFF);
            manuallyFoundFios.insert(ws);
            multiWordsOccurrences.push_back(o);
        }

        SolveAmbiguity(multiWordsOccurrences);

        for (size_t i = 0; i < multiWordsOccurrences.size(); i++) {
            COccurrence& occ = multiWordsOccurrences[i];
            CWordsPair wp(occ.first, occ.second-1);
            std::multiset<CWordsPair, SPeriodOrder>::iterator it = manuallyFoundFios.find(wp);
            if (it != manuallyFoundFios.end()) {
                const CWordsPair& manualFio = *it;
                size_t k;
                for (k = 0; k < fiosToDelete.size(); k++)
                    if (fiosToDelete[k] == manualFio)
                        break;
                if (k >= fiosToDelete.size()) {
                    do
                    {
                        manuallyFoundFios.erase(it);
                        it = manuallyFoundFios.find(wp);
                    }
                    while (it != manuallyFoundFios.end());
                }
            } else {
                SWordHomonymNum wh = foundWords[occ.m_GrammarRuleNo];
                if (occ.m_GrammarRuleNo == 0xFFFF) {
                    std::cout << "Errors!" << std::endl;
                    continue; //не должно случиться
                }
                const CHomonym& h = m_Words[wh];

                CFioWordSequence fioOcc;
                if (!ConvertMWFioFactToFioWS(h.GetSourceWordSequence(), fioOcc.GetFio(), fioOcc))
                    continue;
                fioOcc.m_NameMembers[Surname] = wh;
                foundNames.push_back(fioOcc);
            }
        }
        // удалим ФИО, которые построены руками, но не добавлены
        if (manuallyFoundFios.size() > 0) {
            for (int i = (int)foundNames.size() - 1; i >= 0; i--) {
                std::multiset<CWordsPair, SPeriodOrder>::iterator it = manuallyFoundFios.find(foundNames[i]);
                if (it != manuallyFoundFios.end()) {
                    foundNames.erase(foundNames.begin()+i);
                    manuallyFoundFios.erase(it);
                }
            }
        }
#ifdef NDEBUG
    } catch (yexception& e) {
        PrintError("Error in CSentenceRusProcessor::FindNames()", &e);
        throw;
    } catch (...) {
        PrintError("Error in CSentenceRusProcessor::FindNames()", NULL);
        throw;
    }
#endif
}

// predefined MWN field names
static const Stroka
    kMWN = "MWN",
    kDeleteManualFio = "DeleteManualFio",
    k_fname = "fname",
    k_mname = "mname",
    k_lname = "lname",
    k_bNominative = "bNominative",
    k_bKnownLName = "bKnownLName";

bool CSentenceRusProcessor::ConvertMWFioFactToFioWS(CWordSequence* pWS, SFullFIO& fullFio, CFIOOccurence& fioOccurence)
{
    CFactsWS* pFactsWS = dynamic_cast<CFactsWS*>(pWS);
    if (pFactsWS == NULL)
        return false;

    int i;
    for (i = 0; i < pFactsWS->GetFactsCount(); i++) {
        if (pFactsWS->GetFact(i).GetFactName() == kMWN)
            break;
    }
    if (i >= pFactsWS->GetFactsCount())
        return false;

    CFactFields& fact = pFactsWS->GetFact(i);
    if (fact.GetBoolValue(kDeleteManualFio))
        return false;
    CTextWS* textWS = fact.GetTextValue(k_fname);
    if (textWS)
        fullFio.m_strName = textWS->GetCapitalizedLemma();
    textWS = fact.GetTextValue(k_mname);
    if (textWS)
        fullFio.m_strPatronomyc = textWS->GetCapitalizedLemma();
    textWS = fact.GetTextValue(k_lname);
    if (textWS)
        fullFio.m_strSurname = textWS ->GetCapitalizedLemma();
    fullFio.m_bSurnameLemma = fact.GetBoolValue(k_bNominative);
    fullFio.m_bFoundSurname = fact.GetBoolValue(k_bKnownLName);

    (CWordSequence&)(fioOccurence) = (CWordSequence&)(*pFactsWS);
    return true;
}

bool CSentenceRusProcessor::IsBadMWNFio(CWordSequence* pWS)
{
    CFactsWS* pFactsWS = dynamic_cast<CFactsWS*>(pWS);
    if (pFactsWS == NULL)
        return false;

    for (int i = 0; i < pFactsWS->GetFactsCount(); ++i)
        if (pFactsWS->GetFact(i).GetFactName() == kMWN) {
            CFactFields& fact = pFactsWS->GetFact(i);
            return fact.GetBoolValue(kDeleteManualFio);
        }

    return false;
}

void CSentenceRusProcessor::SetFragmOptions(const SFragmOptions* fragmOptions)
{
    FragmOptions = fragmOptions;
}

// у собранных вручную фио поменяем, если нужно, имя на лемму, указнную в статьях, где ТИП_КС = tl_variant
// (у фио, собранных грамматикой, имя заменяется автоматически)
void CSentenceRusProcessor::ChangeFirstName(SFullFIO& fullFio, CFIOOccurence& fioOccurence)
{
        TKeyWordType kwtype_tl_variant = GlobalDictsHolder->BuiltinKWTypes().TlVariant;

        if (fioOccurence.m_NameMembers[FirstName].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[FirstName]);
            int iH = w.HasArticle_i(SArtPointer(kwtype_tl_variant));
            if (iH != -1) {
                const CHomonym& kw_h = w.GetRusHomonym(iH);
                if (kw_h.GetLemma() == fullFio.m_strName) {
                    Wtroka lemma_text = GlobalDictsHolder->GetUnquotedDictionaryLemma(kw_h, KW_DICT);
                    if (!lemma_text.empty())
                        fullFio.m_strName = lemma_text;
                }
            }
        } else if (fioOccurence.m_NameMembers[InitialName].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[InitialName]);
            if (w.IsName(InitialName)) {
                // TODO: gazetteer
                const CIndexedDictionary& dict = GlobalDictsHolder->GetDict(KW_DICT);
                yvector<CContentsCluster*> foundClusters;
                if (dict.m_Contents.FindCluster(fullFio.m_strName, CompByLemma, foundClusters))
                    for (size_t i = 0; i < foundClusters.size(); ++i) {
                        const CContentsCluster* cl = foundClusters[i];
                        if (cl->size() == 1)
                            for (size_t j = 0; j < cl->m_IDs.size(); ++j) {
                                const article_t* pArt = dict[cl->m_IDs[j]];
                                if (pArt->get_kw_type() == kwtype_tl_variant)
                                    fullFio.m_strName = pArt->get_lemma();
                            }
                    }
            }
      }
}

