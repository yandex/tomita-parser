#pragma once

#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include "wordvector.h"
#include "namecluster.h"


struct SNameTemplate
{
    ENameType name[3];
    long      count;
    EFIOType  FIOType;
    bool      m_bCheckSurnameFromDic;//должны ли мы пытаться искать словарную фамилию слева или справа
};

const long g_FirmedNameTemplatesCount = 13;

class CNameFinder
{
public:
    CNameFinder(CWordVector& words);

    void Run(yvector<CFioWordSequence>& foundNames, yvector<int>& potentialSurnames);
    bool CheckGrammInfo(const CFioWordSequence& nameTempalte, yvector<CFioWordSequence>& foundNames);
    void SetPriorityToTheRightSurname(bool val)
    {
        m_bPriorityToTheRightSurname = val;
    }
    //не только предсказывает одиночную фамилию, но еще и проверяет
    //совпадает ли предсказанная лемма с леммой из SFullFIO& fio
    //если не совпадает (или род не тот), то возвращает false
    //(если предсказать не смог - возвращаем true)
    bool PredictSingleSurname(CFIOOccurence& fioWP, const SFullFIO& fio);

    static Wtroka RusConsonants;

protected:
    bool ApplyTemplate(SNameTemplate& nameTemplate, yvector<CFioWordSequence>& foundName, int& curWord, int iLastWordInFIO);
    bool FindFirmedName(SNameTemplate& nameTemplate, yvector<CFioWordSequence>& foundName, int& curWord, int iLastWordInFIO);
    TGramBitSet FirstNameAgreement(const SWordHomonymNum& surname_ind, SWordHomonymNum& first_name_ind) const;
    bool EnlargeIOname(CFioWordSequence& foundName, int lastWordInFIO);
    bool EnlargeIname(CFioWordSequence& foundName, int lastWordInFIO);
    bool EnlargeIOnameIn(CFioWordSequence& foundName);
    void EnlargeIOnameInIn(CFioWordSequence& foundName, int lastWordInFIO);
    void EnlargeInameIn(CFioWordSequence& foundName, int lastWordInFIO);
    bool CanBeSurnameFromTheLeft(int iW, bool bAtTheEndOfSent);
    bool CanBeSurnameFromTheRight(int iW);
    bool TextCanBeSurname(const TWtringBuf& s);
    bool NameCanBeSurnameFromTheRight(CFioWordSequence& foundName, int iW, int& iSurnameHFromRight);
    bool NameCanBeSurnameFromTheLeft(CFioWordSequence& foundName, int iW);
    bool EnlargeBySurname(CFioWordSequence& foundName, int lastWordInFIO, int nextWord, int prevWord);
    TGramBitSet GetGrammemsForSingleSurname(const CHomonym& surname,
                                            TGramBitSet& femGrammems, TGramBitSet& muscGrammems);
    TGramBitSet GetGrammemsForSingleFirstName(SWordHomonymNum& first_name_ind,
                                              TGramBitSet& femGrammems, TGramBitSet& muscGrammems, SWordHomonymNum& second_first_name_ind);
    void TrateNamesAsSurnames(yvector<CFioWordSequence>& foundNames);
    int TrateNameAsSurnames(int iFoundName, yvector<CFioWordSequence>& foundNames, yvector<int>& to_del);
    bool HasRightPrimGroupType(CWord* pWord);
    void AddSingleSurname(int iW, yvector<CFioWordSequence>& foundNames);
    bool AddFoundFio(yvector<CFioWordSequence>& foundNames, CFioWordSequence& foundName);
    bool PredictSurname(CFIOOccurence& fioWP, int iW);
    bool GleichePredictedSurname(CFioWordSequence& foundName, int iW, int& newH, TGramBitSet& iCommonGrammemsFromRight);
    void NormalizeFio(CFioWordSequence& foundName);
    void AssignGrammems(CFioWordSequence& foundName);
    void AdjustGrammemsForFIO(THomonymGrammems& grammems, const SFullFIO& fio) const;
    void ChooseVariants(yvector<CFioWordSequence>& addedFioVariants, yvector<CFioWordSequence>& foundNames);
    bool GetNameHomonyms(ENameType type, SWordHomonymNum whTemplate, yvector<SWordHomonymNum>& homs) const;
    bool CheckAgreements(CFioWordSequence& foundName) const;

protected:
    bool m_bPriorityToTheRightSurname;
    CWordVector& m_Words;
};
