#include "anaphoraresolution.h"

#include "textprocessor.h"
#include <FactExtract/Parser/common/utilit.h>


static const size_t MaxSentenceDistance = 2;

Stroka CAntecWordInfo::GetParamStr() const {
    return Substitute("$0;$1;$2", m_ClauseNestedness, m_SentenceRole, m_bMorphAnimative ? 1:0);
}

Stroka CAntecedentInfo::GetParamStr() const
{
    return Substitute("$0;$1;$2;$3;$4;$5;$6", m_SentenceDistance, m_TopClauseDistance, m_TokenDistance,  // dummy
                      m_Antecedent.GetParamStr(), m_Anaphor.m_ClauseNestedness, m_Anaphor.m_SentenceRole, m_bTheSameClause ? 1 : 0);
}

CSentenceRusProcessor* CAnaphoraResolution::GetSentPrc(size_t SentNo) const
{
    return CheckedCast<CSentenceRusProcessor*>(m_pReferenceSearcher->m_vecSentence[SentNo]);
}

int CAnaphoraResolution::GetSubjWordNo(const CClause* pClause) const
{
    for (int i =0; i < pClause->GetTypesCount(); i++) {
        const SClauseType& Type =  pClause->GetType(i);
        int SubjVali = Type.HasSubjVal_i();
        if (SubjVali != -1)
        return Type.m_Vals[SubjVali].m_Actant.m_WordNum;
    }
    return -1;
}

int CAnaphoraResolution::GetPredicateWordNo(const CClause* pClause) const
{
    if (pClause->GetTypesCount() == 0)
        return -1;
    else
        return pClause->GetType(0).m_NodeNum.m_WordNum;
}

const CHomonym* CAnaphoraResolution::GetHomonym (const SMWAddress& a) const
{
    const CSentenceRusProcessor* S = GetSentPrc(a.m_iSentNum);
    const CWord* W  = S->getWordRus(a.m_WordNum);
    return &W->GetRusHomonym(a.m_HomNum);
}

size_t CAnaphoraResolution::GetTextPosition(const SMWAddress& Addr) const
{
    return GetSentPrc(Addr.m_iSentNum)->getWordRus(Addr.m_WordNum)->m_pos;
}

const CClauseVariant& CAnaphoraResolution::GetClauseStructureOrBuilt(int SentNo) const
{
    CSentenceRusProcessor* Sent = GetSentPrc(SentNo);
    int iCurSentSave = m_pReferenceSearcher->GetCurSentence();
    ((CReferenceSearcher*)m_pReferenceSearcher)->SetCurSentence(SentNo);
    const CClauseVariant& Var  = Sent->GetClauseStructureOrBuilt();
    ((CReferenceSearcher*)m_pReferenceSearcher)->SetCurSentence(iCurSentSave);
    return Var;
}

bool CAnaphoraResolution::BuildAntecedentInfo(CAntecedentInfo& Info, const SMWAddress& AnapAddr)
{

    // расстояние в предложениях
    Info.m_SentenceDistance = AnapAddr.m_iSentNum - Info.m_Address.m_iSentNum;

    // расстояние в клаузах
    const_clause_it MinClauseIt1,MinClauseIt2;
    CSentenceRusProcessor* Sent1 = GetSentPrc(Info.m_Address.m_iSentNum);
    {
        const CClauseVariant& Var1  = GetClauseStructureOrBuilt(Info.m_Address.m_iSentNum);
        const CClauseVariant& Var2  = GetClauseStructureOrBuilt(AnapAddr.m_iSentNum);

        MinClauseIt1 = Var1.GetMinimalClauseThatContains(CWordsPair(Info.m_Address.m_WordNum));
        if (MinClauseIt1 == Var1.GetEndClause()) {
            assert (MinClauseIt1 != Var1.GetEndClause());
            return false;
        }
        MinClauseIt2 = Var2.GetMinimalClauseThatContains(CWordsPair(AnapAddr.m_WordNum));
        if (MinClauseIt2 == Var2.GetEndClause()) {
            assert (MinClauseIt2 != Var2.GetEndClause());
            return false;
        }
        Info.m_bTheSameClause = (AnapAddr.m_iSentNum == Info.m_Address.m_iSentNum) && (MinClauseIt1 ==  MinClauseIt2);

        const_clause_it TopClause1 = Var1.GetTopClause(MinClauseIt1, Info.m_Antecedent.m_ClauseNestedness);
        const_clause_it TopClause2 = Var2.GetTopClause(MinClauseIt2, Info.m_Anaphor.m_ClauseNestedness);

        if (Info.m_Address.m_iSentNum != AnapAddr.m_iSentNum) {
            Info.m_TopClauseDistance =  1;
            // число топовых клауз от антецедента до конца предложения
            const_clause_it it;
            while (Var1.GetNeighbourClauseR (TopClause1, it)) {
                TopClause1 = it;
                Info.m_TopClauseDistance++;
            }
            // число топовых клауз в предложениях между антецедентом и анафорой

            /*int iSentSave = m_pReferenceSearcher->GetCurSentence();*/        //unused ?

            for (int SentNo = Info.m_Address.m_iSentNum+1;  SentNo < AnapAddr.m_iSentNum; SentNo++) {
                const CClauseVariant& V = GetClauseStructureOrBuilt(SentNo);
                for (it = V.GetFirstClauseConst(LeftToRight); it != V.GetEndClause(); it = V.GetNextClause(LeftToRight, it))
                    Info.m_TopClauseDistance++;
            }
            // число топовых клауз от начала предложения до анафоры
            while (Var2.GetNeighbourClauseL (TopClause2, it)) {
                TopClause2 = it;
                Info.m_TopClauseDistance++;
            }
        } else {
            // одно и то же предложение
            Info.m_TopClauseDistance =  0;
            while (TopClause1 != TopClause2) {
                const_clause_it it;
                Var1.GetNeighbourClauseR (TopClause1, it);
                TopClause1 = it;
                Info.m_TopClauseDistance++;
            }

        }

    }

    // подлежащее
    {

        Info.m_Antecedent.m_SentenceRole = (GetSubjWordNo(*MinClauseIt1) == Info.m_Address.m_WordNum) ? 1 : 0;
        Info.m_Anaphor.m_SentenceRole = (GetSubjWordNo(*MinClauseIt2) == AnapAddr.m_WordNum) ? 1 : 0;
    }

    // Одушевленность
    {
        Info.m_Antecedent.m_bMorphAnimative = GetHomonym(Info.m_Address)->HasGrammem(gAnimated);
        Info.m_Anaphor.m_bMorphAnimative = GetHomonym(AnapAddr)->HasGrammem(gAnimated);

    }
    // положение по отношению предиката
    Info.m_bAntecedentIsBeforePredicate =  GetPredicateWordNo(*MinClauseIt1) > Info.m_Address.m_WordNum;

    // расстояние в токенах
    if (Info.m_SentenceDistance == 0) {
        Info.m_TokenDistance = AnapAddr.m_WordNum - Info.m_Address.m_WordNum;
    } else {
        Info.m_TokenDistance = AnapAddr.m_WordNum + Sent1->getWordsCount() - Info.m_Address.m_WordNum;
        for (int i=AnapAddr.m_iSentNum+1; i < Info.m_Address.m_iSentNum; i++)
            Info.m_TokenDistance += GetSentPrc(i)->getWordsCount();
    }

    Info.m_AntecedentWordStr = Sent1->getWordRus(Info.m_Address.m_WordNum)->GetText();
    if (Info.m_AntecedentWordStr.empty())
        return false;

    return true;

}

bool  CAnaphoraResolution::IsIsolatedParticiple (int SentNo, int WordNo, const CHomonym& PartHom) const
{
    if (!PartHom.IsFullParticiple()) return false;

    CSentenceRusProcessor* pSent = GetSentPrc(SentNo);
    const CClauseVariant& Var  = GetClauseStructureOrBuilt(SentNo);
    const_clause_it MinClauseIt = Var.GetMinimalClauseThatContains(CWordsPair(WordNo));
    if (MinClauseIt == Var.GetEndClause()) {
        assert (MinClauseIt != Var.GetEndClause());
        return false;
    }

    if (GetPredicateWordNo(*MinClauseIt) == WordNo) return false; // cannot be the root of a participle clause

    if (WordNo+1 == (int)pSent->getWordsCount()) return  true; // the last word
    const CWord* pNextW = pSent->getWordRus(WordNo+1);

    for (CWord::SHomIt it1 = pNextW->IterHomonyms(); it1.Ok(); ++it1) {
        const CHomonym& NounOrAdjHom = *it1;
        if (NGleiche::Gleiche(NounOrAdjHom, PartHom, NGleiche::GenderNumberCaseCheck))
            return false;
    }

    return true;
}

void CAnaphoraResolution::ResolveOneAnaphora(const SMWAddress& AnaAddr)
{
    const CHomonym* AnaHom = GetHomonym(AnaAddr);
    const TGramBitSet& AnaGrammems = AnaHom->Grammems.All();
    for (int SentNo=AnaAddr.m_iSentNum; SentNo >= 0; SentNo--) {
        if (AnaAddr.m_iSentNum - SentNo  > (int)MaxSentenceDistance) break;
        int WordNo = (AnaAddr.m_iSentNum == SentNo) ? AnaAddr.m_WordNum - 1 :
                                                      (int)m_pReferenceSearcher->m_vecSentence[SentNo]->getWordsCount() - 1;
        for (; WordNo >=0;  WordNo--) {
            const CWord* pW = m_pReferenceSearcher->m_vecSentence[SentNo]->getWordRus(WordNo);
            for (CWord::SHomIt it = pW->IterHomonyms(); it.Ok(); ++it) {
                const CHomonym& NounHom = *it;
                static const TGramBitSet subst_pronoun_3(gSubstPronoun, gPerson3);
                if (NounHom.HasPOS(gSubstantive) ||
                    NounHom.Grammems.HasAll(subst_pronoun_3) ||
                    IsIsolatedParticiple(SentNo, WordNo, NounHom)) {
                    if ((AnaHom->HasGrammem(gPlural) && NounHom.HasGrammem(gPlural)) ||
                        (AnaHom->HasGrammem(gSingular)&& NounHom.HasGrammem(gSingular) &&
                         (NounHom.Grammems.All() & AnaGrammems).HasAny(NSpike::AllGenders))) {
                        CAntecedentInfo Info;
                        Info.m_Address.m_iSentNum = SentNo;
                        Info.m_Address.m_WordNum = WordNo;
                        Info.m_Address.m_HomNum = it.GetID();
                        Info.m_Address.m_HomNum = it.GetID();
                        if (BuildAntecedentInfo (Info, AnaAddr)) {
                            Info.m_AntecedentPosition = GetTextPosition(Info.m_Address);
                            YASSERT(Info.m_AntecedentPosition < GetTextPosition(AnaAddr));
                            if (Info.m_AntecedentPosition < GetTextPosition(AnaAddr) && Info.m_TokenDistance > 1)
                                m_PossibleAntecedents[AnaHom].push_back(Info);
                        } else
                            Cerr << "Some error! Cannot build antecedent info, SentNo= " << AnaAddr.m_iSentNum << Endl;
                    }

                }

            }
        }

    }

}

void CAnaphoraResolution::ResolveAnaphora()
{
    assert (m_pReferenceSearcher->GetCurSentence() < (int)m_pReferenceSearcher->m_vecSentence.size());
    int CurrSent = m_pReferenceSearcher->GetCurSentence();

    // Цикл по анафорам  текущего предложения
    for (size_t AnaWordNo = 0; AnaWordNo < m_pReferenceSearcher->m_vecSentence[CurrSent]->getWordsCount();  AnaWordNo++) {
        const CWord* pAnapWord = m_pReferenceSearcher->m_vecSentence[CurrSent]->getWordRus(AnaWordNo);
        // Цикл по омонимам текущего слова
        for (CWord::SHomIt ana_it = pAnapWord->IterHomonyms(); ana_it.Ok(); ++ana_it) {
            const CHomonym& AnaHom = *ana_it;
            static const TGramBitSet subst_pronoun_3(gSubstPronoun, gPerson3);
            if (AnaHom.Grammems.HasAll(subst_pronoun_3)) // он, они, она, оно
            {
                SMWAddress AnaAddr;
                AnaAddr.m_iSentNum = CurrSent;
                AnaAddr.m_WordNum = AnaWordNo;
                AnaAddr.m_HomNum  = ana_it.GetID();

                ResolveOneAnaphora(AnaAddr);
            }
        }

    }

}

static int GetScoreBaseLine (const CAntecedentInfo& I)
{
    return      2*I.m_SentenceDistance
            +   I.m_TopClauseDistance
            +   I.m_Antecedent.m_ClauseNestedness
            -   2*I.m_Antecedent.m_SentenceRole  // пока только 0 или 1, 1 - подлежащее
            -   (I.m_Antecedent.m_bMorphAnimative  ? 2 : 0)
            +   (I.m_bTheSameClause ? 1 : 0);
}

static bool IsApplicableBaseLine (const ymap<int, int>& Score2Index, const yvector<CAntecedentInfo>& Infos)
{
    //if (Score2Index.empty()) return false;
    //return true;

    ymap<int, int>::const_iterator it;
    for (it = Score2Index.begin(); it != Score2Index.end(); it++) {
        if (Infos[it->second].m_AntecedentWordStr != Infos[Score2Index.begin()->second].m_AntecedentWordStr)
            break;
    }
    if (it == Score2Index.end()) // all hypots are the same
          return  true;
    else
        //// the most accurate decision
        ////return it->first > Score2Index.begin()->first+1;
        //  a not very accurate decision
        return it->first > Score2Index.begin()->first;

}

bool CAnaphoraResolution::GetBestAntecedent(const CWord* pAnapWord, CAntecedentInfo& Info) const
{
    yvector<CAntecedentInfo> PossibleAntecedents;
    for (CWord::SHomIt ana_it = pAnapWord->IterHomonyms(); ana_it.Ok(); ++ana_it) {
        const CHomonym& AnaHom = *ana_it;
        yhash<const CHomonym*, yvector<CAntecedentInfo> >::const_iterator it = m_PossibleAntecedents.find(&AnaHom);
        if (it != m_PossibleAntecedents.end())
            PossibleAntecedents.insert(PossibleAntecedents.begin(), it->second.begin(), it->second.end());
    }
    if (PossibleAntecedents.empty())
        return false;

    ymap<int, int> Score2Index;
    for (size_t i = 0; i < PossibleAntecedents.size(); ++i)
        Score2Index[GetScoreBaseLine(PossibleAntecedents[i])] = i;

    if (!IsApplicableBaseLine(Score2Index, PossibleAntecedents))
        return false;
    Info = PossibleAntecedents[Score2Index.begin()->second];
    return true;
}

/*
struct CAnaphor
{
    Stroka     m_AntecedentStr;
    Stroka     m_AnaphorStr;
    size_t      m_AntecedentPosition;

    CAnaphor() {};
    CAnaphor( const Stroka& AnaphorStr, const Stroka& AntecedentStr, size_t AntecedentPosition)
    {
        m_AntecedentStr = AntecedentStr;
        m_AnaphorStr = AnaphorStr;
        m_AntecedentPosition = AntecedentPosition;
    }
};
*/
