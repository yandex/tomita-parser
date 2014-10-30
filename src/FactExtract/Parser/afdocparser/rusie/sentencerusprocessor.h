#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/ptr.h>

#include <FactExtract/Parser/afdocparser/rus/sentence.h>
#include "dictsholder.h"
#include "multiwordcreator.h"
#include "valencyvalues.h"
#include "topclauses.h"
#include "factscollection.h"

struct SFragmOptions;

struct SMWAddressWithLink: public SMWAddress
{
    SMWAddressWithLink(int iSentNum, int WordNum, int HomNum, bool bOriginalWord = true):
        SMWAddress(iSentNum, WordNum, HomNum, bOriginalWord)
    {
    }

    SMWAddressWithLink(int iSentNum, const SWordHomonymNum& WordHomonymNum):
        SMWAddress(iSentNum, WordHomonymNum)
    {
    }

    SSentMarkUp m_Link;
};

class CSentenceRusProcessor: public CSentence
{
    friend class CFragmentationRunner;
    friend class CTextProcessor;

public:
    CSentenceRusProcessor();
    virtual ~CSentenceRusProcessor(void);
    void FreeData();

    const yvector<SValenciesValues>* GetFoundVals(const SArtPointer& artPointer);
    CMultiWordCreator& GetMultiWordCreator();
    void FindSituations(const SArtPointer& artPointer);
    void FindMultiWords(const SArtPointer& artPointer);
    void FindMultiWordsInLink(const SArtPointer& artPointer);

    const CClauseVariant& GetClauseStructureOrBuilt();
    bool  RunFragmentation();
    void PutReferenceSearcher(CReferenceSearcher* pRefSeracher);

    void FindMultiWordsByArticle(const Wtroka& strArtTitle, TKeyWordType kwType);
    void SetFragmOptions(const SFragmOptions* fragmOptions);

    const CFactFields&  GetFact(const SFactAddress& factAddr) const;
    void FillFactAddresses(yset<SFactAddress>& factAddresses, SFactAddress& templateFactAddress);
    bool SearchingOfSitWasPerformed(const Wtroka& strArt) const;
    void FindSituationsInner(const SArtPointer& artPointer);
    void AddFoundVals(const SValenciesValues& valenciesValues);
    void AddToAlreadySearchedSit(const SArtPointer& artPointer);
    virtual Wtroka GetCapitalizedLemma(const CWord& w, int iH, Wtroka strLemma);
    void GetFoundMWInLinks(yvector<SMWAddressWithLink>& mwFounds, const SArtPointer& artPointer) const;

    virtual void FindNames(yvector<CFioWordSequence>& foundNames, yvector<int>& potentialSurnames);
    bool ConvertMWFioFactToFioWS(CWordSequence* pWS, SFullFIO& fullFio, CFIOOccurence& fioOccurence);
    bool IsBadMWNFio(CWordSequence* pWS);
    void ChangeFirstName(SFullFIO& fullFio, CFIOOccurence& fioOccurence);

protected:
    CMultiWordCreator m_MultiWordCreator;
    CClauseVariant m_clausesStructure;
    yset<Wtroka> m_SitFoundByArticles; //статьи тех ситуаций, которые мы уже пытались искать в этом предложении

    typedef TSharedPtr< yvector<SValenciesValues> > TValValuesPtr;
    ymap<Wtroka, TValValuesPtr> m_article2FoundVals; //найденные валентности ситуаций
    yhash<TKeyWordType, TValValuesPtr>  m_KwType2FoundVals;  //найденные валентности ситуаций
    const SFragmOptions* FragmOptions; //настройки из файла настроек для фрагментации

};
