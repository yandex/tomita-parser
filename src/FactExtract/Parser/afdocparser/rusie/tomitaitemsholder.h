#pragma once

#include <util/generic/cast.h>
#include <util/generic/vector.h>
#include <util/generic/stroka.h>


#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include <FactExtract/Parser/afglrparserlib/inputsequence.h>
#include "primitivegroup.h"
#include <FactExtract/Parser/afglrparserlib/inputitemfactory.h>
#include "syngroup.h"
#include "solveperiodambiguity.h"

#include <FactExtract/Parser/afdocparser/common/datetime.h>


class CReferenceSearcher;
class CWeightSynGroup;

struct SReduceConstraints {
    const CRuleAgreement& Agreements;
    const yvector<CInputItem*>& Children;

    // used in commongraminterp only
    yset<size_t>* TrimTree;
    yset<CWordsPair>* LeftPriorityOrder;

    THomonymGrammems Grammems;
    yvector<ui32> ChildHomonyms;

    size_t SymbolNo;

    size_t FirstItemIndex;
    size_t MainItemIndex;
    size_t ChainSize;

    size_t CurrentConstraintIndex;

    // KWSets, checked explicitly (and found) in child items
    ymap<size_t, SArtPointer> CheckedKWSet;


    SReduceConstraints(const CRuleAgreement& agreements,  const yvector<CInputItem*>& children,
                       size_t symbolNo, size_t firstItem, size_t mainItem, size_t chainSize);

    const SRuleExternalInformationAndAgreements& Get() const {
        return Agreements[CurrentConstraintIndex];
    }

    const CGroup* GetChildGroup(size_t i) const {
        return CheckedCast<const CGroup*>(Children[i]);
    }

    CGroup* GetChildGroup(size_t i) {
        return CheckedCast<CGroup*>(Children[i]);
    }
};

class TTomitaGeoHierarchy;

class CTomitaItemsHolder :
    public CInputItemFactory
{

protected:

    void        RunParser(CInputSequenceGLR& InputSequenceGLR, yvector< COccurrence >& occurrences, bool bAllowAmbiguity);
    void        FreeParser(CInputSequenceGLR& InputSequenceGLR);

    bool CheckItemGrammems(const TGramBitSet& grammemsToCheck, bool allow, THomonymGrammems& resGrammems) const;
    bool CheckItemPositiveNegativeGrammems(const TGramBitSet& PositGrammems, const TGramBitSet& NegGrammems, THomonymGrammems& resGrammems) const;

    bool CheckItemPositiveNegativeUnitedGrammems(const TGramBitSet& PositGrammems, const TGramBitSet& NegGrammems, int iChildNum,
                                                 const yvector<CInputItem*>& children) const;

    void GetFormsWithGrammems(const TGramBitSet& grammems, const TGrammarBunch& formGrammems, TGrammarBunch& res) const;

    const CWord*  GetFirstAgrWord(const SReduceConstraints* pReduceConstraints, const SAgr& agr) const;
    const CWord*  GetLastAgrWord(const SReduceConstraints* pReduceConstraints, const SAgr& agr) const;

    void InitKwtypesCount(CWeightSynGroup* pNewGroup, const yvector<CInputItem*>& children,
                          const SRuleExternalInformationAndAgreements& ruleInfo) const;
    void InitUserWeight(CWeightSynGroup* pNewGroup, const yvector<CInputItem*>& children,
                        const SRuleExternalInformationAndAgreements& ruleInfo) const;

public:
    struct CGramInterp
    {
        THomonymGrammems Grammems;
        CGramInterp() {
        }

        explicit CGramInterp(const CGroup* group) { //copy constructor from CGroup
            Grammems = group->GetGrammems();
        }

        bool HasEqualGrammems(const CGroup* group) const {
            return Grammems == group->GetGrammems();
        }

        bool IsEmpty() const { return Grammems.Empty(); }

    };

    CTomitaItemsHolder(const CWordVector& Words, const CWorkGrammar& gram, const yvector<SWordHomonymNum>& clauseWords);
    virtual ~CTomitaItemsHolder();

    virtual CInputItem* CreateNewItem(size_t SymbolNo, const CRuleAgreement& agr,
                                        size_t iFirstItemNo, size_t iLastItemNo,
                                        size_t iSynMainItemNo, yvector<CInputItem*>& children);

    virtual CInputItem* CreateNewItem(size_t SymbolNo, size_t iFirstItemNo);
    virtual bool EqualNodes(const CInputItem* item1, const CInputItem* item2) const;

    Stroka m_GramFileName;

protected:
    const CWordVector& m_Words;
    const CWorkGrammar& m_GLRgrammar;
    const yvector<SWordHomonymNum>& m_FDOChainWords;

    THolder<TTomitaGeoHierarchy> GeoHierarchy;

    bool    HasOnlyOneHomId(ui32 iHomIDs) const;
    ui32    GetNewHomIDsByGrammems(const CGramInterp& gramInterp, const CWord& w, ui32 iOldHomIDs) const;
    void    BuildFinalCommonGroup(const COccurrence& occur, CWordSequence* pWordSeq);
    void    AssignOutputGrammems(THomonymGrammems& resGrammems, const TGramBitSet& outGrammems) const;

    //ф-ции проверки согласования и ограничений на Reduce
    bool    ReduceCheckConstraintSet(SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckGroupAgreement(SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckOutputGrammems(SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckOutputGroupSize(const SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckKWSets(SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckWFSets(const SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckFirstWord(const SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckTrim(SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckLeftDominantRecessive(SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckNotHRegFact(const SReduceConstraints* pReduceConstraints) const;
    bool    ReduceCheckGrammemUnion(const SReduceConstraints* pReduceConstraints, yvector<CGramInterp>&  ChildGramInterps) const;
    bool    ReduceCheckQuotedPositions(const SReduceConstraints* pReduceConstraints, const SAgr& agr) const;
    bool    ReduceCheckGeoAgreement(SReduceConstraints* pReduceConstraints) const;

    bool GetGztWeight(const SRuleExternalInformationAndAgreements& ruleInfo,
                      const CGroup* group, size_t index, double& gztWeight) const;

    void    AdjustChildHomonyms(const yvector<CGramInterp>&  ChildGramInterps, SReduceConstraints* pReduceConstraints) const;
    bool    IsWSUpperCase(const CWordsPair& seq) const;
    bool    CheckFioAgreement(const CGroup* pGr1, const CGroup* pGr2) const;
    virtual Stroka GetDumpOfWords(const CInputItem* item1, ECharset encoding)  const;

    virtual void PrintOccurrences(TOutputStream &stream,
                                  const yvector<COccurrence>& occurrences,
                                  const yvector<COccurrence>& dropped_occurrences = yvector<COccurrence>()) const;
    void PrintOccurrence(TOutputStream &stream, const COccurrence& occurrence, bool dropped) const;
    void PrintFlatRule(TOutputStream &stream, const CGroup* group) const;
    void PrintFlatRule(TOutputStream &stream, size_t SymbolNo, size_t iSynMainItemNo, const yvector<CInputItem*>& children) const;
    void PrintFlatRuleChild(TOutputStream &stream, const CGroup* child, const CGroup* maingroup = NULL, bool verbose_primitives = false) const;
    void PrintRulesTree(TOutputStream &stream, const CGroup* group, const Stroka& indent1 = Stroka(), const Stroka& indent2 = Stroka()) const;
    void PrintRuleTreeBranches(TOutputStream &stream, const yvector<CInputItem*>& children, const Stroka& indent = Stroka()) const;
};

class CFPCInterpretations
{
public:
    CFPCInterpretations(yvector<CWordSequence*>& FoundFPCs, CReferenceSearcher* ReferenceSearcher):
        m_FoundFPCs(FoundFPCs)
    {
        m_pReferenceSearcher = ReferenceSearcher;
    };

    yvector<CWordSequence*>& m_FoundFPCs;
    CReferenceSearcher* m_pReferenceSearcher;
};
