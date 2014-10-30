#include <FactExtract/Parser/common/libxmlutil.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/common/utilit.h>
#include "textprocessor.h"
#include "referencesearcher.h"
#include "docinsidestatuscluster.h"
#include "parseroptions.h"

#include <FactExtract/Parser/afdocparser/builtins/articles_base.pb.h>

#ifndef TOMITA_EXTERNAL
#include <FactExtract/Parser/tag_lib/tomita/tagger.h>
#endif

CTextProcessor::CTextProcessor(const CParserOptions* parserOptions)
    : CText(parserOptions)
    , m_iTouchedSentencesCount(0)
    , m_ParserOptions(parserOptions)
    , m_pErrorStream(NULL)
    , m_FactsCollection(this)
{
}

CTextProcessor::~CTextProcessor() {
    FreeData();
}


void CTextProcessor::FreeData()
{
    CText::FreeData();
    m_iTouchedSentencesCount = 0;
    m_FactsCollection.Reset();

}

bool CTextProcessor::IgnoreUpperCase() const
{
    if(m_ParserOptions)
        return m_ParserOptions->GetIgnoreUpperCase();
    return false;
}


void CTextProcessor::FindMultiWords(const SArtPointer& art) {
    int sentBackup = m_pReferenceSearcher->GetCurSentence();
    for (size_t i = 0; i < m_vecSentence.size(); ++i) {
        m_pReferenceSearcher->SetCurSentence(i);
        GetSentenceProcessor(i)->FindMultiWords(art);
    }
    m_pReferenceSearcher->SetCurSentence(sentBackup);
}


bool CTextProcessor::RunTagger(const TGztArticle& article) {
    bool run = false;
    if (article.IsInstance<TTaggerArticle>()) {

        if (!UsedTaggerIds.insert(article.GetId()).second)
            return false;     // already run

        const CDictsHolder& dicts = *GlobalDictsHolder;

        // find all dependency kw-types
        const TTaggerArticle* taggerArt = article.As<TTaggerArticle>();
        for (size_t i = 0; i < taggerArt->KernelSize(); ++i)
            FindMultiWords(SArtPointer(dicts.FindKWType(taggerArt->GetKernel(i))));
        for (size_t i = 0; i < taggerArt->UsedKwtypeSize(); ++i)
            FindMultiWords(SArtPointer(dicts.FindKWType(taggerArt->GetUsedKwtype(i))));

#ifndef TOMITA_EXTERNAL
        for (NGzt::TCustomKeyIterator it(article, ::GetTaggerPrefix()); it.Ok(); ++it) {
            const TTagger& tagger = dicts.GetTaggerOrRegister(*it, article);
            tagger.Classify(*this);
            run = true;
        }
#endif
    }

    return run;
}

bool CTextProcessor::ShouldRunFramentation(CSentence& sent)
{
    int iDashCount = 0;
    int iBadPunctCount = 0;
    for (size_t i = 0; i < sent.m_words.size(); i++) {
        CWord& w = *sent.m_words[i];
        if (w.m_typ == Punct) {
            if (NStr::IsEqual(w.GetText(), "|"))
                iBadPunctCount++;
            if (w.GetText()[0] == 0x00A6)
                iBadPunctCount++;

            if (w.IsPunct()) {
                if (w.IsDash()) {
                    iDashCount++;
                    if (iDashCount == 4)
                        return false;
                }
            }
            if (!w.IsDash())
                iDashCount = 0;
        } else
            iDashCount = 0;
    }
    if (iBadPunctCount > 5)
        return false;
    if (sent.m_words.size() > 100)
        return false;
    return true;
}

void CTextProcessor::RunFragmentationWithoutXml()
{

    for (size_t i = 0; i < m_vecSentence.size(); i++) {
        if (!ShouldRunFramentation(*GetSentence(i)))
            continue;

        m_pReferenceSearcher->SetCurSentence(i);
        GetSentenceProcessor(i)->RunFragmentation();
    }
}

bool CTextProcessor::FindSituatiations(int iSent, const SArtPointer& artP)
{
    if (!ShouldRunFramentation(*m_vecSentence[iSent]))
        return false;
    m_pReferenceSearcher->SetCurSentence(iSent);
    return GetSentenceProcessor(iSent)->GetFoundVals(artP) != NULL;
}

void CTextProcessor::SetCurSent(int iSent)
{
    m_pReferenceSearcher->SetCurSentence(iSent);
}

void CTextProcessor::PrintError(const Stroka& msg, const yexception* error)
{
    if (m_pErrorStream == NULL)
        return;

    Stroka strSent;
    ToString(strSent);
    (*m_pErrorStream) << msg;
    if (error != NULL)
        (*m_pErrorStream) << ":\n\t" << error->what();
    (*m_pErrorStream) << "\nin sentence:\n\t" << strSent << "\n";
    m_pErrorStream->Flush();
}

#ifdef NDEBUG
    #define RELEASE_TRY  try
    #define RELEASE_CATCH(step) \
        catch (yexception& e) {\
            PrintError("Error in CTextProcessor::analyzeSentences() - " step, &e);\
            throw;\
        }\
        catch (...) {\
            PrintError("Error in CTextProcessor::analyzeSentences() - " step, NULL);\
            throw;\
        }
#else
    #define RELEASE_TRY
    #define RELEASE_CATCH(step)
#endif

void CTextProcessor::RunFragmentation() {
    CreateReferenceSearcher();
    CText::analyzeSentences();

    if(!m_ParserOptions)
        throw yexception() << "No parser options in \"CTextProcessor::analyzeSentences\"";

    const TBuiltinKWTypes& kwtypes = GlobalDictsHolder->BuiltinKWTypes();

    for (size_t i = 0; i < m_vecSentence.size(); i++) {
        GetSentenceProcessor(i)->SetFragmOptions(m_ParserOptions->m_FragmOptions.Get());
        m_pReferenceSearcher->SetCurSentence(i);
        GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), kwtypes.Date);
        GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), kwtypes.Fio);
        GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), kwtypes.FioWithoutSurname);
        GetSentenceProcessor(i)->FindMultiWordsByArticle(Wtroka(), kwtypes.Number);
    }

    RELEASE_TRY {
        if (CGramInfo::s_bRunFragmentationWithoutXML)
            RunFragmentationWithoutXml();
    } RELEASE_CATCH("fragmentation")
}

void CTextProcessor::analyzeSentences()
{
    RunFragmentation();

    const yset<SArtPointer>& mwToFind = m_ParserOptions->m_KWDumpOptions.MultiWordsToFind;
    const yset<SArtPointer>& sitToFind = m_ParserOptions->m_KWDumpOptions.SitToFind;

    for (size_t i = 0; i < m_vecSentence.size(); i++) {
        m_pReferenceSearcher->SetCurSentence(i);
        for (yset<SArtPointer>::const_iterator it = mwToFind.begin(); it != mwToFind.end(); ++it)
            GetSentenceProcessor(i)->FindMultiWords(*it);

        RELEASE_TRY {
            for (yset<SArtPointer>::const_iterator it = sitToFind.begin(); it != sitToFind.end(); ++it)
                FindSituatiations(i, *it);
        } RELEASE_CATCH("FindSituatiations")

        if (GetSentenceProcessor(i)->GetMultiWordCreator().m_bProcessedByGrammar)
            m_iTouchedSentencesCount++;

        if (NULL != CGramInfo::s_PrintRulesStream)
            GetSentenceProcessor(i)->GetMultiWordCreator().PrintWordsFeatures(*CGramInfo::s_PrintRulesStream, CGramInfo::s_DebugEncoding);
    }

    RELEASE_TRY {
        m_FactsCollection.Init();
        m_FactsCollection.GroupByCompanies();
        m_FactsCollection.GroupByFios();
        m_FactsCollection.GroupByQuotes();

    } RELEASE_CATCH("m_FactsCollection")
}

#undef RELEASE_TRY
#undef RELEASE_CATCH

//void CTextProcessor::analyzeSentences()
//{
//    analyzeSentences(GlobalDictsHolder->m_ParserOptions.m_KWDumpOptions);
//}

void CTextProcessor::GetFoundMWInLinks(yvector<SMWAddressWithLink>& mwFounds, const yset<SArtPointer>& artToFind) const
{
    yset<SArtPointer>::const_iterator it = artToFind.begin();
    for (; it != artToFind.end(); ++it)
        GetFoundMWInLinks(mwFounds, *it);
}

void CTextProcessor::GetFoundMWInLinks(yvector<SMWAddressWithLink>& mwFounds, const SArtPointer& artPointer) const
{
    for (size_t i = 0; i < GetSentenceCount(); i++) {
        int iBefore = mwFounds.size();
        GetSentenceProcessor(i)->GetFoundMWInLinks(mwFounds, artPointer);
        for (size_t k = iBefore; k < mwFounds.size(); k++)
            mwFounds[k].m_iSentNum = i;
    }
}

void CTextProcessor::CreateReferenceSearcher()
{
    GztCache.Reset(new NGzt::TCachingGazetteer(*GlobalDictsHolder->Gazetteer()));
    m_pReferenceSearcher.Reset(new CReferenceSearcher(*GztCache, *this, m_DateFromDateFiled));
    m_pReferenceSearcher->m_DocumentAttribtes = m_docAttrs;
    for (size_t i=0; i < m_vecSentence.size(); i++)
        GetSentenceProcessor(i)->PutReferenceSearcher(m_pReferenceSearcher.Get());
}

const CTime& CTextProcessor::GetDateFromDateField() const
{
    return m_DateFromDateFiled;
}

CFactFields& CTextProcessor::GetFact(const SFactAddress& factsAddr)
{
    return (CFactFields&)GetSentenceProcessor(factsAddr.m_iSentNum)->GetFact(factsAddr);
}

const CFactFields& CTextProcessor::GetFact(const SFactAddress& factsAddr) const
{
    return GetSentenceProcessor(factsAddr.m_iSentNum)->GetFact(factsAddr);
}

void CTextProcessor::FillFactAddresses(yset<SFactAddress>& factAddresses)
{
    SFactAddress templateFactAddress;
    for (size_t i=0; i < m_vecSentence.size(); i++) {
        templateFactAddress.m_iSentNum = i;
        GetSentenceProcessor(i)->FillFactAddresses(factAddresses, templateFactAddress);
    }
}

CSentenceBase* CTextProcessor::CreateSentence()
{
    return new CSentenceRusProcessor;
}

bool CTextProcessor::HasToResolveContradictionsFios()
{
    return true;    //!GlobalDictsHolder->m_ParserOptions.m_bLogClusterBuilder;
}

void UpdateFirstLastWord(int& iFirstWord, int& iLastWord, const CWordSequence* ws)
{
    if (!ws)
        return;
    if (ws->IsArtificialPair())
        return;
    if (!ws->IsValid())
        return;

    if ((iFirstWord == -1) || (iFirstWord > ws->FirstWord()))
        iFirstWord = ws->FirstWord();

    if ((iLastWord == -1) || (iLastWord < ws->LastWord()))
        iLastWord = ws->LastWord();
}


static Wtroka GetRawFioLemma(const CSentenceRusProcessor* pSent, const CFioWS* fioWS)
{
    Wtroka res;
    if(fioWS->m_NameMembers[Surname].IsValid())
    {
        res += pSent->GetRusWords().GetWord(fioWS->m_NameMembers[Surname]).GetText();
        res += ' ';
    }
    if(fioWS->m_NameMembers[FirstName].IsValid())
    {
        res += pSent->GetRusWords().GetWord(fioWS->m_NameMembers[FirstName]).GetText();
        res += ' ';
    }
    else
        if(fioWS->m_NameMembers[InitialName].IsValid())
        {
            res += pSent->GetRusWords().GetWord(fioWS->m_NameMembers[InitialName]).GetText();
            res += ' ';
        }

    if(fioWS->m_NameMembers[Patronomyc].IsValid())
    {
        res += pSent->GetRusWords().GetWord(fioWS->m_NameMembers[Patronomyc]).GetText();
        res += ' ';
    }
    else
        if(fioWS->m_NameMembers[InitialPatronomyc].IsValid())
        {
            res += pSent->GetRusWords().GetWord(fioWS->m_NameMembers[InitialPatronomyc]).GetText();
            res += ' ';
        }

    return StripString(res);
}

void CTextProcessor::SerializeFacts(yvector<TPair<Stroka, ymap<Stroka, Wtroka> > >& out_facts, yvector<TPair<int, int> >& facts_positions)
{
    CFactsCollection::SetOfFactsConstIt factsIt = m_FactsCollection.GetFactsItBegin();
    for (; factsIt != m_FactsCollection.GetFactsItEnd(); factsIt++) {
        const SFactAddress& factAddress = *factsIt;
        const CFactFields& fact  = GetFact(factAddress);

        const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(fact.GetFactName());

        if (!m_ParserOptions->m_ParserOutputOptions.HasFactToShow(fact.GetFactName()))
            continue;
        TPair<Stroka, ymap<Stroka, Wtroka> > newFact;
        newFact.first = pFactType->m_strFactTypeName;
        int iFirstWord = -1;
        int iLastWord = -1;
        const CSentenceRusProcessor* pSent = GetSentenceProcessor(factAddress.m_iSentNum);
        for (size_t i= 0; i < pFactType->m_Fields.size(); i++) {
            const fact_field_descr_t& fieldDescr = pFactType->m_Fields[i];

            switch (fieldDescr.m_Field_type) {
                case FioField:
                    {
                        const CFioWS* fioWS = fact.GetFioValue(fieldDescr.m_strFieldName);
                        UpdateFirstLastWord(iFirstWord, iLastWord, fioWS);
                        if (fioWS)
                        {
                            newFact.second[fieldDescr.m_strFieldName] = fioWS->GetCapitalizedLemma();
                            newFact.second[fieldDescr.m_strFieldName+"_raw"] = GetRawFioLemma(pSent,fioWS);
                        }
                        break;
                    }
                case TextField:
                    {
                        const CTextWS* textWS = fact.GetTextValue(fieldDescr.m_strFieldName);
                        UpdateFirstLastWord(iFirstWord, iLastWord, textWS);
                        if (textWS)
                            newFact.second[fieldDescr.m_strFieldName] = textWS ->GetCapitalizedLemma();
                        break;
                    }
                case DateField:
                    {
                        const CDateWS* pDateWS = fact.GetDateValue(fieldDescr.m_strFieldName);
                        UpdateFirstLastWord(iFirstWord, iLastWord, pDateWS);
                        if (pDateWS)
                            newFact.second[fieldDescr.m_strFieldName] = pDateWS->ToString();
                        break;
                    }
                case BoolField:
                    {
                        if (fact.GetBoolValue(fieldDescr.m_strFieldName))
                            NStr::Assign(newFact.second[fieldDescr.m_strFieldName], "1");
                        else
                            NStr::Assign(newFact.second[fieldDescr.m_strFieldName], "0");
                        break;
                    }

                default:
                    break;
            }
        }

        if (iFirstWord != -1 && iLastWord != -1 &&
            iFirstWord < (int)pSent->getWordsCount() &&
            iLastWord < (int)pSent->getWordsCount()) {
            CWordBase* pW1 = pSent->getWord(iFirstWord);
            CWordBase* pW2 = pSent->getWord(iLastWord);
            TPair<int, int> factPos(pW1->m_pos, pW2->m_pos + pW2->m_len);
            facts_positions.push_back(factPos);
            out_facts.push_back(newFact);
        }
    }
}



void CTextProcessor::GetFactsPositions(yvector<TSimpleFactPosition>* factsPositions)
{
    CFactsCollection::SetOfFactsConstIt factsIt = m_FactsCollection.GetFactsItBegin();
    for (; factsIt != m_FactsCollection.GetFactsItEnd(); factsIt++) {
        const SFactAddress& factAddress = *factsIt;
        const CFactFields& fact = GetFact(factAddress);
        const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(fact.GetFactName());
        if (!m_ParserOptions->m_ParserOutputOptions.HasFactToShow(fact.GetFactName()))
            continue;
        int iFirstWord = -1;
        int iLastWord = -1;
        for (size_t i= 0; i < pFactType->m_Fields.size(); i++) {
            const fact_field_descr_t& fieldDescr = pFactType->m_Fields[i];
            switch (fieldDescr.m_Field_type) {
                case FioField:
                    {
                        const CFioWS* fioWS = fact.GetFioValue(fieldDescr.m_strFieldName);
                        UpdateFirstLastWord(iFirstWord, iLastWord, fioWS);
                        break;
                    }
                case TextField:
                    {
                        const CTextWS* textWS = fact.GetTextValue(fieldDescr.m_strFieldName);
                        UpdateFirstLastWord(iFirstWord, iLastWord, textWS);
                        break;
                    }
                case DateField:
                    {
                        const CDateWS* pDateWS = fact.GetDateValue(fieldDescr.m_strFieldName);
                        UpdateFirstLastWord(iFirstWord, iLastWord, pDateWS);
                        break;
                    }
                default:
                    break;
            }
        }
        TSimpleFactPosition factPosition;
        factPosition.firstWord = iFirstWord;
        factPosition.lastWord = iLastWord;
        factPosition.sentenceNo = factAddress.m_iSentNum;

        const CSentenceRusProcessor* pSent = GetSentenceProcessor(factAddress.m_iSentNum);
        if (iFirstWord != -1 && iLastWord != -1 &&
            iFirstWord < (int)pSent->getWordsCount() &&
            iLastWord < (int)pSent->getWordsCount()) {
            CWordBase* pW1 = pSent->getWord(iFirstWord);
            CWordBase* pW2 = pSent->getWord(iLastWord);
            factPosition.byteStart = pW1->m_pos;
            factPosition.byteEnd = pW2->m_pos + pW2->m_len;
            factsPositions->push_back(factPosition);
        }
    }
}
