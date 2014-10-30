#include "situationssearcherinclause.h"
#include "keywordsfinder.h"

#include <FactExtract/Parser/common/string_tokenizer.h>


//проверяет,что один вариант заполнения валентностей включает в себя другой
//то есть, у них все значения валентностей включаются друг в друга
bool SValenciesValues::Includes(const SValenciesValues& valenciesValues, const CWordVector& allWords) const
{
    if (valenciesValues.m_pArt != m_pArt)
        return false;

    if (!(valenciesValues.m_DictIndex == m_DictIndex))
        return false;

    if (m_values.size() != valenciesValues.m_values.size())
        return false;
    if (m_iSubord != valenciesValues.m_iSubord)
        return false;

    for (size_t i = 0; i < m_values.size(); i++) {
        const CWord& w1 = allWords.GetWord(m_values[i].m_Value);
        const CWord& w2 = allWords.GetWord(valenciesValues.m_values[i].m_Value);
        if (!w1.GetSourcePair().Includes(w2.GetSourcePair()))
            return false;
    }

    return true;
}

CSituationsSearcherInClause::CSituationsSearcherInClause(CMultiWordCreator& multiWordCreator, const CClause& clause,
                                                         const CClause* pMainClause, yvector<const CClause*>& subClauses)
    : m_multiWordCreator(multiWordCreator)
    , m_Clause(clause)
    , m_pMainClause(pMainClause)
    , m_wordSequences(multiWordCreator.m_wordSequences)
    , m_Words(multiWordCreator.m_Words)
    , m_subClauses(subClauses)
{
    m_pCurWNode = NULL;
    m_pCurHNode = NULL;
}

CSituationsSearcherInClause::~CSituationsSearcherInClause(void)
{
}

const yvector<SValenciesValues>& CSituationsSearcherInClause::GetFoundVals() const
{
    return m_ValValues;
}

//пытаемя поискать "валентности" статьей с пустым полем состав
//(здесь может быть много статей, если в SArtPointer есть KwType
void CSituationsSearcherInClause::SearchForArticleWithEmptySOSTAV(const SArtPointer& artPointer)
{
    if (artPointer.HasKWType()) {
        const yvector<int>* arts = GlobalDictsHolder->GetDict(KW_DICT).GetAuxArticles(artPointer.GetKWType());
        YASSERT(arts != NULL);
        for (size_t i = 0; i < arts->size(); i++)
            SearchForArticleWithEmptySOSTAV(SDictIndex(KW_DICT, (*arts)[i]));
    } else if (artPointer.HasStrType()) {
        SDictIndex dicIndex(KW_DICT, GlobalDictsHolder->GetDict(KW_DICT).get_article_index(artPointer.GetStrType()));
        SearchForArticleWithEmptySOSTAV(dicIndex);
    }
    ChooseLargestVariants(m_ValValues);
    Interpret();
}

void CSituationsSearcherInClause::SearchForArticleWithEmptySOSTAV(const SDictIndex& dicIndex)
{
    if (!dicIndex.IsValid())
        return;
    const article_t* pArt = GlobalDictsHolder->GetAuxArticle(dicIndex);
    if (pArt->get_clause_type() != ClauseTypesCount)
        if (!m_Clause.HasType(pArt->get_clause_type()))
            return;
    if (!pArt->has_contents_str())
        FindNodeSituation(SWordHomonymNum(), -1, dicIndex);
}

void CSituationsSearcherInClause::Run(const SArtPointer& artPointer)
{
    yvector<SWordHomonymNum>* foundWords;
    foundWords = m_multiWordCreator.GetFoundWords(artPointer, KW_DICT);
    if (foundWords == NULL) {
        SearchForArticleWithEmptySOSTAV(artPointer);
        return;
    }

    for (size_t i = 0; i < foundWords->size(); i++) {
        SWordHomonymNum& nodeWH = foundWords->at(i);
        CWord& w = m_Words.GetWord(nodeWH);
        if(!w.HasAuxArticle(KW_DICT))
            continue;
        int typesCount = m_Clause.GetTypesCount();
        for (int j = 0; j < typesCount; j++) {
            // lazyfrog: здесь надо проверить, нету ли перед вершиной клаузы
            // отрицания. Если есть, то пропускаем.
            const SClauseType& clause_type = m_Clause.GetType(j);
            if (clause_type.m_bNegation)
                continue;
            const SWordHomonymNum& wh = m_Clause.GetType(j).m_NodeNum;
            if (wh.IsValid()) {
                CWord& wNode = m_Words.GetWord(wh);
                if (w.GetSourcePair().Includes(wNode.GetSourcePair())) {
                    if (FindNodeSituation(nodeWH, j, SDictIndex(KW_DICT, m_Words[nodeWH].GetAuxArticleIndex(KW_DICT))))
                        break;
                }
            }
        }
    }
    ChooseLargestVariants(m_ValValues);
    Interpret();
}

void CSituationsSearcherInClause::Interpret()
{
    for (size_t i = 0; i < m_ValValues.size(); i++) {
        yvector<CWordSequence*> tempFacts;
        CSitFactInterpretation sitFactInterpretation(m_multiWordCreator.m_Words, tempFacts);
        TIntrusivePtr<CFactsWS> pFactWS = sitFactInterpretation.Interpret(m_ValValues[i]);
        if (pFactWS.Get() != NULL) {
            m_wordSequences.push_back(pFactWS.Get());
            pFactWS->SetPair(m_Clause);
            m_multiWordCreator.AddMultiWord(pFactWS.Release());
        }
    }
}

void CSituationsSearcherInClause::Run(const Wtroka& strArt)
{
    Run(SArtPointer(strArt));
}

void CSituationsSearcherInClause::Run(TKeyWordType type)
{
    Run(SArtPointer(type));
}

void CSituationsSearcherInClause::SetCurNode(const SWordHomonymNum& nodeWH)
{
    if (nodeWH.IsValid()) {
        m_pCurHNode = &m_Words[nodeWH];
        m_pCurWNode = &m_Words.GetWord(nodeWH);
    } else {
        m_pCurHNode = NULL;
        m_pCurWNode = NULL;
    }

}

bool CSituationsSearcherInClause::FindNodeSituation(const SWordHomonymNum& nodeWH, int iClauseType, const SDictIndex& dicIndex)
{
    SetCurNode(nodeWH);
    const article_t* pArt = GlobalDictsHolder->GetAuxArticle(dicIndex);
    if (pArt->get_subordination_count() == 0)
        return false;

    for (int i = 0; i < pArt->get_subordination_count(); i++) {
        if (CheckSubbordination(nodeWH, iClauseType, i, dicIndex))
            return true;
    }
    return false;
}

bool CSituationsSearcherInClause::AddValencyValue(SValenciesValues& valenciesValues, SValencyValue& last,
                                                  const valencie_list_t& subord, bool& bCanBeOmited)
{
    bCanBeOmited = false;
    CWord& w = m_Words.GetWord(last.m_Value);
    for (int i = valenciesValues.m_values.size() - 1; i >= 0; i--) {
        SValencyValue& val = valenciesValues.m_values[i];
        if (w.GetSourcePair().Intersects(m_Words.GetWord(val.m_Value).GetSourcePair())) {
            if (subord[val.m_iVal].get_modality() == Possibly) {
                valenciesValues.m_values.erase(valenciesValues.m_values.begin() + i);
                valenciesValues.m_values.push_back(last);
                return true;
            } else if (subord[last.m_iVal].get_modality() == Possibly) {
                bCanBeOmited = true;
                return true;
            }

            if ((subord[val.m_iVal].get_modality() == Necessary) &&
                (subord[last.m_iVal].get_modality() == Necessary)) {
                return false;
            }
        }
    }

    valenciesValues.m_values.push_back(last);
    return true;
}

//у одинаковых моделей управлений пытаемя сплющить те значения, которые отличаются только необязательными валентностями
void CSituationsSearcherInClause::InspectPossibleValValues(yvector<SValenciesValues>& valValues, int iCurSub,
                                                           yvector<SValenciesValues>& new_valValues)
{
    if (valValues.size() == 0)
        return;
    const valencie_list_t& subord = valValues[0].m_pArt->get_subordination(iCurSub); //статья для всех одна и та же
    yset<int> differentPossibleValues;
    //нащупываем те валентности, у которых разные значения
    size_t j;
    for (j = 0; j < subord.size(); j++) {
        int iVal = j;
        const or_valencie_list_t& val_var = subord[j];
        SWordHomonymNum prevVal;
        size_t k;
        for (k = 0; k < valValues.size(); k++) {
            if (valValues[k].m_iSubord != iCurSub)
                continue;
            SWordHomonymNum val = valValues[k].GetValue(iVal);
            if (k == 0)
                prevVal = val;
            else {
                if (!(prevVal == val))
                    break;
                else
                    prevVal = val;
            }
        }
        //значит нашли не совпадающие значения у валентности iVal
        if (k < valValues.size()) {
            if (val_var.get_modality() == Possibly)
                differentPossibleValues.insert(iVal);
            else {
                break;
            }
        }
    }

    bool bMerge = (j >=  subord.size());

    for (j = 0; j < valValues.size(); j++) {
        if (valValues[j].m_iSubord != iCurSub)
            continue;
        if (!bMerge)
            new_valValues.push_back(valValues[j]);
        else {
            SValenciesValues valValue = valValues[j];
            SValenciesValues newValValue = valValue;
            newValValue.m_values.clear();
            for (size_t k = 0; k < valValue.m_values.size(); k++)
                if (differentPossibleValues.find(valValue.m_values[k].m_iVal) == differentPossibleValues.end())
                    newValValue.m_values.push_back(valValue.m_values[k]);
            new_valValues.push_back(newValValue);
            break;
        }
    }
}

inline bool CmpValenciesValues (const SValenciesValues& a1, const SValenciesValues& a2) {
    return a1.LessByValuesCount(a2);
}

//оставляем вариант с максимальным количеством заполненных валентностей
void CSituationsSearcherInClause::ChooseValenciesWithMaximumValues(yvector<SValenciesValues>& valValues)
{
    if (valValues.size() <= 1)
        return;
    std::sort(valValues.begin(), valValues.end(), CmpValenciesValues);

    size_t iMax = valValues[valValues.size() - 1].m_values.size();
    int i;
    for (i = valValues.size() - 2; i >= 0; i--) {
        if (valValues[i].m_values.size() < iMax)
            break;
    }

    if (i >=  0)
        valValues.erase(valValues.begin(), valValues.begin() + i + 1);
}

//оставляет только максимальные по вложению  ( SValenciesValues::Includes)
void CSituationsSearcherInClause::ChooseLargestVariants(yvector<SValenciesValues>& valValues)
{
    if (valValues.size() <= 1)
        return;

    yvector<SValenciesValues> all_new_valValues;
    yset<int> usedSubbords;
    for (size_t i = 0; i < valValues.size(); i++) {
        //значит уже рассматривали
        if (valValues[i].m_pArt == NULL)
            continue;

        size_t j;
        for (j = i + 1; j < valValues.size(); j++) {
            //значит уже рассматривали
            if (valValues[j].m_pArt == NULL)
                continue;
            if (valValues[i].Includes(valValues[j], m_Words))
                valValues[j].m_pArt = NULL;
            else if (valValues[j].Includes(valValues[i], m_Words)) {
                //значит i точно не может быть максимальным по вложению
                //выходим из цикла,  j может стать максимальным
                break;
            }
        }
        if (j >= valValues.size())
            all_new_valValues.push_back(valValues[i]);
        valValues[i].m_pArt = NULL;
    }

    valValues.clear();
    valValues.insert(valValues.begin(),all_new_valValues.begin(), all_new_valValues.end());

}

void CSituationsSearcherInClause::InspectPossibleValValues(yvector<SValenciesValues>& valValues)
{
    yvector<SValenciesValues> all_new_valValues;
    yset<int> usedSubbords;
    for (size_t i = 0; i < valValues.size(); i++) {
        SValenciesValues& valValue = valValues[i];
        int iCurSub = valValue.m_iSubord;
        if (usedSubbords.find(iCurSub) == usedSubbords.end())
            usedSubbords.insert(iCurSub);
        else
            continue;

        InspectPossibleValValues(valValues, iCurSub, all_new_valValues);
    }

    valValues.clear();
    valValues.insert(valValues.begin(),all_new_valValues.begin(), all_new_valValues.end());
}

void CSituationsSearcherInClause::AddValencyVariants(yvector<SValencyValue>& valency_variants, yvector<SValenciesValues>& valValues,
                                                     const valencie_list_t& subord)
{
    if (valency_variants.size() == 1) {
        for (int j = valValues.size() - 1; j >=0; j--) {
            bool bCanBeOmited;
            if (!AddValencyValue(valValues[j], valency_variants[0], subord, bCanBeOmited))
                valValues.erase(valValues.begin() + j);
        }
    } else if (valency_variants.size() > 1) {
        yvector<SValenciesValues> copy_valValues = valValues;
        valValues.clear();
        bool bCopyIfEmpty = false;
        for (size_t k = 0; k < valency_variants.size(); k++) {
            for (size_t j = 0; j < copy_valValues.size(); j++) {
                SValenciesValues ValenciesValues = copy_valValues[j];
                bool bCanBeOmited;
                if (AddValencyValue(ValenciesValues, valency_variants[k], subord, bCanBeOmited))
                    if (!bCanBeOmited)
                        valValues.push_back(ValenciesValues);
                    else
                        bCopyIfEmpty = true;

            }
        }
        if ((valValues.size() == 0) && bCopyIfEmpty)
            valValues = copy_valValues;
    }

}

bool CSituationsSearcherInClause::CheckSubbordination(const SWordHomonymNum& nodeWH, int iClauseType,
                                                      int iSubordination, const SDictIndex& dicIndex)
{
    const article_t* pArt = GlobalDictsHolder->GetAuxArticle(dicIndex);
    const valencie_list_t& subord = pArt->get_subordination(iSubordination);
    yvector<SValenciesValues> valValues;
    valValues.push_back(SValenciesValues(nodeWH, iSubordination, pArt, dicIndex));

    bool bAlreadyHadNecesasaryVal = false;

    for (size_t i = 0; i < subord.size(); i++) {
        yvector<SValencyValue> valency_variants;
        const or_valencie_list_t& valency_alternatives = subord[i];
        FindValencyVariants(subord, i, iClauseType, valency_variants);
        if (valency_variants.size() == 0 && (valency_alternatives.get_modality() == Necessary))
            return false;
        if (valency_alternatives.get_modality() == Necessary)
            bAlreadyHadNecesasaryVal = true;

        AddValencyVariants(valency_variants, valValues, subord);

        if (valValues.size() == 0)
            if (bAlreadyHadNecesasaryVal)
                return false;
            else {
                yvector<SValenciesValues> valValues;
                valValues.push_back(SValenciesValues(nodeWH, iSubordination, pArt, dicIndex));
            }
    }

    for (int j = valValues.size() - 1; j >= 0; j--) {
        SValenciesValues& oneValValues = valValues[j];
        if (!CheckValsOrderConditions(oneValValues, subord))
            valValues.erase(valValues.begin() + j);

    }

    if (valValues.size() <= 0)
        return false;

    //оставляем те, у кго больше валентностей заполнено
    ChooseValenciesWithMaximumValues(valValues);
    //оставляет только максимальные по вложению
    ChooseLargestVariants(valValues);
    //сплющиваем те, которые отличаются только необязательными валентностями
    //(эти необязательные удаляем)
    InspectPossibleValValues(valValues);

    if (valValues.size() > 1)
        return false;

    m_ValValues.insert(m_ValValues.begin(), valValues.begin(), valValues.end());

    return true;
}

bool CSituationsSearcherInClause::CheckValsOrderConditions(const SValenciesValues& valValues, const valencie_list_t& subord)
{
    if (subord.m_actant_condions.size() == 0)
        return true;
    size_t i;
    for (i = 0; i < subord.m_actant_condions.size(); i++) {
        const actant_contions_t& ac = subord.m_actant_condions[i];
        size_t j;
        for (j = 0; j < ac.m_conditions.size(); j++) {
            const single_actant_contion_t& sac = ac.m_conditions[j];
            if (!CheckValsSingleConditions(valValues, sac, subord))
                break;
        }
        if (j >= ac.m_conditions.size())
            break;
    }

    if (i < subord.m_actant_condions.size())
        return true;

    return false;
}

bool CSituationsSearcherInClause::CheckValsSingleConditions(const SValenciesValues& valValues, const single_actant_contion_t& sac,
                                                            const valencie_list_t& subord)
{
    switch (sac.m_type) {
        case FollowACType: return CheckValsSingleOrderConditions(valValues, sac, subord);
        case AgrACType: return CheckValsSingleAgrConditions(valValues, sac, subord);
        default: return true;
    }
}

bool CSituationsSearcherInClause::CheckValsSingleAgrConditions(const SValenciesValues& valValues, const single_actant_contion_t& sac,
                                                               const valencie_list_t& /* subord */)
{
    if (sac.m_vals.size() != 2)
        return true;
    SWordHomonymNum v1 = valValues.GetValue(sac.m_vals[0]);
    SWordHomonymNum v2 = valValues.GetValue(sac.m_vals[1]);
    if (!v1.IsValid() || !v2.IsValid())
        return true;
    return GleicheHomonyms(m_Words[v1], m_Words[v2], sac.m_CoordFunc);
}

bool CSituationsSearcherInClause::CheckValsSingleOrderConditions(const SValenciesValues& valValues, const single_actant_contion_t& sac,
                                                                 const valencie_list_t& subord)
{
    CWordsPair wp_prev;
    for (size_t i = 0; i < sac.m_vals.size(); i++) {
        if (!sac.is_THIS_label(i)) {
            size_t j;
            for (j = 0; j < valValues.m_values.size(); j++) {
                if (sac.m_vals[i] == subord[valValues.m_values[j].m_iVal].m_str_name)
                    break;
            }

            if (j < valValues.m_values.size()) {
                if (sac.m_type == FollowACType) {
                    if (i > 0) {
                        if (!wp_prev.FromLeft(m_Words.GetWord(valValues.m_values[j].m_Value).GetSourcePair()))
                            return false;
                    }
                }

                wp_prev = m_Words.GetWord(valValues.m_values[j].m_Value).GetSourcePair();
            }
        } else {
            if (!valValues.m_NodeWH.IsValid())
                return false;
            if (sac.m_type == FollowACType) {
                if (i > 0) {
                    if (!wp_prev.FromLeft(m_Words.GetWord(valValues.m_NodeWH).GetSourcePair()))
                        return false;
                }
            }
            wp_prev = m_Words.GetWord(valValues.m_NodeWH).GetSourcePair();
        }
    }
    return true;
}

void CSituationsSearcherInClause::FindValencyVariants(const valencie_list_t& subord , int iSub, int iClauseType, yvector<SValencyValue>& valency_variants)
{
    const or_valencie_list_t& valency_alternatives = subord[iSub];

    size_t i;
    for (i = 0; i < valency_alternatives.size(); i++) {
        CheckValency(valency_alternatives[i], iClauseType, valency_variants);
    }
    for (i =  0; i < valency_variants.size(); i++) {
        valency_variants[i].m_iVal = iSub;
        valency_variants[i].m_strVal = subord[iSub].m_str_name;
    }

}

CWord* CSituationsSearcherInClause::GetWordForMultiWord(int iFirst, int iLast, SWordHomonymNum& newWH,
                                                           const SWordHomonymNum& mainWord)
{
    Wtroka str;
    CWord* pNewWord;
    if (iFirst == iLast) {
        pNewWord = &m_Words.GetOriginalWord(iFirst);
        newWH.m_bOriginalWord = true;
        newWH.m_WordNum = iFirst;
        newWH.m_HomNum = mainWord.m_HomNum;
    } else {
        for (int i = iFirst; i <= iLast; i++) {
            str += m_Words.GetOriginalWord(i).GetOriginalText();
            if (i < iLast)
                str += '_';
        }

        CWordSequence* pWS = new CWordSequence();
        pWS->SetPair(iFirst, iLast);
        pNewWord = new CMultiWord(str, false);
        pNewWord->SetSourcePair(iFirst, iLast);
        pNewWord->m_bUp = m_Words.GetOriginalWord(iFirst).m_bUp;
        m_Words.AddMultiWord(pNewWord);
        newWH.m_bOriginalWord = false;
        newWH.m_WordNum = m_Words.GetMultiWordsCount() - 1;
        CHomonym* pNewHom = new CHomonym(TMorph::GetMainLanguage(), str);
        pNewHom->SetSourceWordSequence(pWS);
        if (mainWord.IsValid())
            pNewHom->SetGrammems(m_Words[mainWord].Grammems);

        pNewWord->AddRusHomonym(pNewHom);
        newWH.m_HomNum = 0;
    }
    return pNewWord;
}

bool CSituationsSearcherInClause::GleicheHomonyms(const CHomonym& h1, const CHomonym& h2, ECoordFunc f)
{
    return NGleiche::Gleiche(h1, h2, f);
}

bool CSituationsSearcherInClause::CheckGramConditions(const valencie_t& valency,  int /* iClauseType */, SValencyValue& valency_val)
{
    const CHomonym& h = m_Words[valency_val.m_Value];
    bool found = valency.m_morph_scale.empty();
    for (THomonymGrammems::TFormIter it(valency.m_morph_scale); it.Ok(); ++it)
        if (h.Grammems.HasFormWith(*it)) {
            found = true;
            break;
        }
    if (!found)
        return false;

    switch (valency.m_coord_func) {
        case CheckLettForAbbrevs:
        case CoordFuncCount:
        case AnyFunc:
            break;

        case SubjAdjShort:
        case SubjVerb:
        default:
            {
                if (GetCurHNode() == NULL)
                    return false;
                if (!GleicheHomonyms(h, *GetCurHNode(), valency.m_coord_func))
                    return false;
            }
    }
    return true;
}

void CSituationsSearcherInClause::FindWordsByValencyContent(yvector<SWordHomonymNum>& SWVector,
                                                            const  valencie_t& valency , int iW1, int iW2)
{
    CArticleContents articleContents;
    for (size_t i = 0; i < valency.m_contents_str.size(); i++) {
        yvector<Wtroka> content;
        GenarateContent(valency.m_contents_str[i], content);
        articleContents.AddContent(content, i, CompByLemma);
    }
    articleContents.SortClustersContent();

    for (int i = iW1; i <= iW2; i++) {
        if (i == GetCurNodeSourcePair().FirstWord()) {
            i = GetCurNodeSourcePair().LastWord();
            continue;
        }

        SWordHomonymNum mainWord;
        int iLast;
        ymap<int, yset<int> > mapHom2Art;

        CKeyWordsFinder keyWordsFinder(m_multiWordCreator);
        bool bRes = keyWordsFinder.FindAuxWordSequenceByLemma(i, articleContents, iLast, mainWord, mapHom2Art, DICTYPE_COUNT);
        if (bRes) {
            ymap<int, yset<int> >::iterator it = mapHom2Art.begin();
            for (; it != mapHom2Art.end(); it++) {
                yset<int>::iterator art_it = it->second.begin();
                for (; art_it != it->second.end(); art_it++) {
                    SWordHomonymNum wh;
                    if (!mainWord.IsValid()) {
                        mainWord.m_HomNum = it->first;
                        mainWord.m_WordNum = i;
                    }
                    GetWordForMultiWord(i, iLast, wh, mainWord);
                    SWVector.push_back(wh);
                }
            }
        }
    }
    articleContents.Clear();
}

void CSituationsSearcherInClause::CheckValencyContent(const valencie_t& valency, int iClauseType,
                                                      yvector<SValencyValue>& valency_variants_res)
{
    yvector<SWordHomonymNum> SWVector;

    FindWordsByValencyContent(SWVector, valency, m_Clause.FirstWord(), m_Clause.LastWord());

    if (SWVector.size() == 0)
        return;

    CheckValencyInsideClause(&SWVector, valency,  iClauseType, valency_variants_res);
}

void CSituationsSearcherInClause::CheckValencyInsideClause(yvector<SWordHomonymNum>* pSWVector, const valencie_t& /* valency */,
                                                           int /* iClauseType */, yvector<SValencyValue>& valency_variants_res)
{
    if (pSWVector == NULL)
        return;
    for (size_t i = 0; i < pSWVector->size(); i++) {
        CWord& w = m_Words.GetWord(pSWVector->at(i));
        if (!m_Clause.Includes(w.GetSourcePair()))
            continue;
        else {
            size_t j;
            for (j = 0; j < m_subClauses.size(); j++) {
                //игнорируем скобки
                if (m_subClauses[j]->HasType(Brackets))
                    continue;
                if (m_subClauses[j]->Includes(w.GetSourcePair()) ||
                    (m_subClauses[j]->Intersects(w.GetSourcePair()) && !w.GetSourcePair().Includes(*m_subClauses[j])))
                    break;
            }
            if (j < m_subClauses.size())
                continue;
        }
        if (w.GetSourcePair().Intersects(GetCurNodeSourcePair()))
            continue;
        valency_variants_res.push_back(SValencyValue(pSWVector->at(i)));
    }
}

void CSituationsSearcherInClause::CheckValencyKWType(const valencie_t& valency, int iClauseType,
                                                     yvector<SValencyValue>& valency_variants_res)
{
    CheckValencyInsideClause(m_multiWordCreator.GetFoundWords(valency.m_kw_type, KW_DICT),
                             valency,  iClauseType, valency_variants_res);
}

void CSituationsSearcherInClause::CheckValencyTitle(const valencie_t& valency, int iClauseType,
                                                    yvector<SValencyValue>& valency_variants_res)
{
    CheckValencyInsideClause(m_multiWordCreator.GetFoundWords(valency.m_str_title, KW_DICT),
                             valency, iClauseType, valency_variants_res);
}

bool CSituationsSearcherInClause::CheckPrefixByArticle(yvector<SWordHomonymNum>* pSWVector, SValencyValue& valency_val)
{
    if (pSWVector == NULL)
        return false;
    CWord& wVal = m_Words.GetWord(valency_val.m_Value);
    for (size_t i = 0; i < pSWVector->size(); i++) {
        CWord& w = m_Words.GetWord(pSWVector->at(i));
        if (!m_Clause.Includes(w.GetSourcePair()))
            continue;
        if (w.GetSourcePair().LastWord() == wVal.GetSourcePair().FirstWord() - 1)
            return true;
    }

    return false;
}

void CSituationsSearcherInClause::GenarateContent(const Wtroka& s, yvector<Wtroka>& content)
{
    TWtrokaTokenizer tokenizer(s, " ");
    Wtroka str;
    while (tokenizer.NextAsString(str)) {
        TMorph::ToLower(str);
        content.push_back(str);
    }
}

bool CSituationsSearcherInClause::CompWordsAsPrefix(const Wtroka& s, SValencyValue& valency_val)
{
    CWord& w = m_Words.GetWord(valency_val.m_Value);
    yvector<Wtroka> content;
    GenarateContent(s, content);
    if (w.GetSourcePair().FirstWord() - (int)content.size() < m_Clause.FirstWord())
        return false;
    for (int i = w.GetSourcePair().FirstWord() - content.size(); i < w.GetSourcePair().FirstWord(); i++)
        if (!m_Words.GetOriginalWord(i).HasHomonym(content[i - (w.GetSourcePair().FirstWord() - content.size())]))
            return false;

    return true;
}

bool CSituationsSearcherInClause::CheckPrefix(const reference_to_wordchain_t& ref2wch, SValencyValue& valency_val)
{
    if (ref2wch.m_kw_type != NULL)
        return CheckPrefixByArticle(m_multiWordCreator.GetFoundWords(ref2wch.m_kw_type, KW_DICT), valency_val);
    else if (ref2wch.m_title.length() > 0)
        return CheckPrefixByArticle(m_multiWordCreator.GetFoundWords(ref2wch.m_title, KW_DICT), valency_val);
    else if (ref2wch.m_contents_str.size() > 0)
        for (size_t i = 0; i < ref2wch.m_contents_str.size(); ++i)
            if (CompWordsAsPrefix(ref2wch.m_contents_str[i], valency_val))
                return true;

    return false;
}

bool CSituationsSearcherInClause::CheckAntecedent(const reference_to_wordchain_t& ref2wch, SValencyValue& valency_val)
{
    yvector<SWordHomonymNum>* pSWVector = NULL;
    CWord& valWord = m_Words.GetWord(valency_val.m_Value);
    const CClause* pClause = &m_Clause;
    if (!m_Clause.Includes(valWord.GetSourcePair()) && (m_pMainClause != NULL))
        pClause = m_pMainClause;

    //пока ищем только у союзов
    for (int i = 0; i < pClause->GetConjsCount(); i++) {
        const SConjType type = pClause->GetConjType(i);
        if (!type.m_SupVal.IsValid())
            continue;
        if (!m_Words.GetWord(valency_val.m_Value).GetSourcePair().Includes(m_Words.GetWord(type.m_WordNum).GetSourcePair()))
            continue;

        if (pSWVector == NULL) {
            if (ref2wch.m_kw_type != NULL)
                pSWVector = m_multiWordCreator.GetFoundWords(ref2wch.m_kw_type, KW_DICT);
            if (ref2wch.m_title.length() > 0)
                pSWVector = m_multiWordCreator.GetFoundWords(ref2wch.m_title, KW_DICT);
            if (!pSWVector)
                return false;
        }
        for (size_t j = 0; j < pSWVector->size(); j++) {
            if (m_Words.GetWord(pSWVector->at(j)).GetSourcePair().Includes(m_Words.GetWord(type.m_SupVal.m_Actant).GetSourcePair())) {
                valency_val.m_Value = pSWVector->at(j);//навреное, временно, т.к. это уже дело интерпретации
                return true;
            }
        }
    }
    return false;
}

void CSituationsSearcherInClause::CheckNodeSynRel(const valencie_t& valency, const SClauseType& clauseType,
                                                  yvector<SValencyValue>& valency_variants_res)
{
    SWordHomonymNum actant;
    if (clauseType.m_SupVal.IsValid() && clauseType.m_SupVal.m_Rel == valency.m_syn_rel)
        actant = clauseType.m_SupVal.m_Actant;
    else
        for (size_t i = 0; i < clauseType.m_Vals.size(); i++) {
            if (clauseType.m_Vals[i].m_Rel == valency.m_syn_rel) {
                actant = clauseType.m_Vals[i].m_Actant;
                break;
            }
        }

    if (!actant.IsValid())
        return;

    yvector<SWordHomonymNum> SWVector;
    yvector<SWordHomonymNum>* pSWVector = NULL;
    if (valency.m_contents_str.size() > 0) {
        FindWordsByValencyContent(SWVector, valency, 0, m_Words.OriginalWordsCount() - 1);
        if (SWVector.size() > 0)
            pSWVector = &SWVector;
    } else if (valency.m_kw_type != NULL)
        pSWVector = m_multiWordCreator.GetFoundWords(valency.m_kw_type, KW_DICT);
    else if (valency.m_str_title.length() > 0)
        pSWVector = m_multiWordCreator.GetFoundWords(valency.m_str_title, KW_DICT);

    if (pSWVector == NULL)
        return;

    CWord& wActant = m_Words.GetWord(actant);
    for (size_t i = 0; i < pSWVector->size(); i++) {
        CWord& w = m_Words.GetWord(pSWVector->at(i));
        if (w.GetSourcePair().Intersects(GetCurNodeSourcePair()))
            continue;
        if (!w.GetSourcePair().Includes(wActant.GetSourcePair()))
            continue;
        valency_variants_res.push_back(SValencyValue(pSWVector->at(i)));
    }

}

void CSituationsSearcherInClause::CheckNodeSynRel(const valencie_t& valency, int iClauseType,
                                                  yvector<SValencyValue>& valency_variants_res)
{
    if (iClauseType != -1) {
        const SClauseType& clauseType = m_Clause.GetType(iClauseType);
        CheckNodeSynRel(valency,  clauseType, valency_variants_res);
    } else //если было пустое поле СОСТАВ, то син_о проверяем просто на какую-нибудь вершину клаузы
    {
        for (int i = 0; i < m_Clause.GetTypesCount(); i++) {
            const SClauseType& clauseType = m_Clause.GetType(i);
            CheckNodeSynRel(valency,  clauseType, valency_variants_res);
        }
    }
}

//добавляем только уникальные
void CSituationsSearcherInClause::AddValencyValue(yvector<SValencyValue>& valency_variants_res, SValencyValue& val)
{
    const CWord& w_NewVal = m_Words.GetWord(val.m_Value);
    size_t i;
    for (i = 0; i < valency_variants_res.size(); i++) {
        const CWord& w_Val = m_Words.GetWord(valency_variants_res[i].m_Value);
        if (w_Val.GetSourcePair() == w_NewVal.GetSourcePair())
            break;
    }
    if (i >= valency_variants_res.size())
        valency_variants_res.push_back(val);

}

void CSituationsSearcherInClause::CheckValency(const valencie_t& valency, int iClauseType, yvector<SValencyValue>& valency_variants_res)
{
    yvector<SValencyValue> valency_variants;

    if (valency.m_syn_rel !=  AnyRel)  //у валентности указывается син_о, которое должно уже быть приписано к вершине клаузы
                                        //нужно проверить лишь его актанты
    {
        CheckNodeSynRel(valency,  iClauseType, valency_variants);
    } else {
        if (valency.m_contents_str.size() > 0)
            CheckValencyContent(valency,  iClauseType, valency_variants);
        else if (valency.m_kw_type != NULL)
            CheckValencyKWType(valency,  iClauseType, valency_variants);
        else if (valency.m_str_title.length() > 0)
            CheckValencyTitle(valency,  iClauseType, valency_variants);
    }

    if (valency_variants.size() == 0)
        return;
    for (int i = valency_variants.size() - 1; i >= 0; i--) {
        if (!CheckGramConditions(valency,  iClauseType, valency_variants[i]))
            continue;
        if (valency.m_prefix.is_valid())
            if (!CheckPrefix(valency.m_prefix, valency_variants[i]))
                continue;
        //здесь valency_variants[i].m_Value можнт поменяться
        //например, для "который"  valency_variants[i].m_Value станет его антецедентом
        if (valency.m_ant.is_valid())
            if (!CheckAntecedent(valency.m_ant, valency_variants[i]))
                continue;

        AddValencyValue(valency_variants_res, valency_variants[i]);
    }
}

CWordsPair CSituationsSearcherInClause::GetCurNodeSourcePair()  const
{
    if (m_pCurWNode == NULL)
        return CWordsPair(-1,-1);
    else
        return m_pCurWNode->GetSourcePair();
}
