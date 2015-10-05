#include "clauses.h"
#include <FactExtract/Parser/afdocparser/rus/gleiche.h>

bool SConjType::IsSubConj()
{
    return m_Type == gSubConj || m_Type == gPronounConj;
}

bool SConjType::IsSimConj()
{
    return m_Type == gSimConj || IsSimConjAnd();
}

bool SConjType::IsSimConjAnd()
{
    return m_bSimConjAnd;
}

bool SConjType::IsCorr()
{
    return m_Type == gCorrelateConj;
}

bool SConjType::IsInPair(CWordsPair& words_pair, const CWordVector& words)
{
    const CWord& conj = words.GetWord(m_WordNum);
    return words_pair.Includes(conj.GetSourcePair());
}

SClauseType SClauseType::Clone()
{
    SClauseType ClauseType;
    ClauseType = *this;
    return ClauseType;
}

int SClauseType::HasSubjVal_i() const
{
    for (size_t i = 0; i < m_Vals.size(); i++)
        if (m_Vals[i].IsSubjVal() && m_Vals[i].IsValid())
            return i;

    return -1;
}

bool SClauseType::HasSubjVal()
{
    return (HasSubjVal_i() != -1);
}

bool SClauseType::AdjustByPair(CWordsPair& words_pair, const CWordVector& words)
{
    if (m_NodeNum.IsValid()) {
        const CWord& node = words.GetWord(m_NodeNum);
        if (!words_pair.Includes(node.GetSourcePair()))
            return false;
    }

    for (size_t i = 0; i < m_Vals.size(); i++)
        if (m_Vals[i].m_ActantType == WordActant)
            if (m_Vals[i].m_Actant.IsValid()) {
                const CWord& actant = words.GetWord(m_Vals[i].m_Actant);
                if (!words_pair.Includes(actant.GetSourcePair())) {
                    m_Vals[i].m_Actant.m_HomNum = -1;
                    m_Vals[i].m_Actant.m_WordNum = -1;
                    m_Type = WantsVal;
                }
            }

    return true;
}

/*------------------------------------class CClause ------------------------------------*/

CClause::CClause(CWordVector& Words)
    : m_Words(Words),
      m_ID(-1),
      m_BadWeight(0),
      m_RuleBadWeight(0),
      m_bCalculatedBadWeight(false)
{
}

void CClause::PutID(long ID)
{
    m_ID = ID;
}

long CClause::GetID()
{
    return m_ID;
}

void CClause::Copy(const CClause& clause)
{
    m_ClauseWords = clause.m_ClauseWords;
    m_Conjs = clause.m_Conjs;
    m_Types = clause.m_Types;
    m_BadWeight = clause.m_BadWeight;
    m_BadCommas = clause.m_BadCommas;
    SetPair(clause.FirstWord(), clause.LastWord());
}

void CClause::AddValsToType(yvector<SValency> vals, int iType)
{
    if (vals.size() == 0)
        return;

    for (size_t i = 0; i < vals.size(); i++)
        AddValToType(vals[i], iType);
}

int CClause::GetWordsCount() const
{
    return m_ClauseWords.size();
}

int CClause::GetTypesCount() const
{
    return m_Types.size();
}

bool CClause::HasConjs() const
{
    return GetConjsCount() > 0;
}

int CClause::GetConjsCount() const
{
    return m_Conjs.size();
}

SClauseType& CClause::GetType(int i)
{
    if (i < 0 || (size_t)i >= m_Types.size())
        ythrow yexception() << "Bad iType in CClause::GetType.";
    return m_Types[i];
}

const SClauseType& CClause::GetType(int i) const
{
    if (i < 0 || (size_t)i >= m_Types.size())
        ythrow yexception() << "Bad iType in CClause::GetType.";
    return m_Types[i];
}

void CClause::AddValToType(const SValency& val, int iType)
{
    SClauseType& type = m_Types[iType];
    type.m_Vals[val.m_ValNum] = val;
    CheckTypeByVals(type);
}

void CClause::DelAllTypesAndAdd(SClauseType type)
{
    m_Types.clear();
    m_Types.push_back(type);
}

void CClause::DelTypesExcept(int iType)
{
    YASSERT(iType >= 0 && (size_t)iType < m_Types.size());
    for (int i = (int)m_Types.size() - 1; i >= 0; --i)
        if (i != iType)
            DeleteType(i);
}

int CClause::GetFirstClauseWordWithType()
{
    for (size_t i = 0; i < m_ClauseWords.size(); i++)
        for (size_t j = 0; j < m_Types.size(); j++)
            if (m_ClauseWords[i].EqualByWord(m_Types[j].m_NodeNum))
                return i;

    return -1;
}

CClause* CClause::Clone()
{
    CClause* newClause = CClause::Create(m_Words);
    newClause->Copy(*this);
    return newClause;
}

bool CClause::FindFirmWord(const TGramBitSet& POS, int& iW)
{
    iW = -1;
    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);
        if (word.HasOnlyPOS(POS)) {
            iW = i;
            return true;
        }
    }
    return false;
}

void CClause::DeleteHomWithPOS(const TGramBitSet& POS)
{
    for (size_t i = 0; i < m_ClauseWords.size(); ++i)
        m_Words.GetWord(m_ClauseWords[i]).DeleteHomonymWithPOS(POS);
}

DECLARE_STATIC_RUS_WORD(kByt, "быть");
DECLARE_STATIC_RUS_WORD(kStat, "стать");

void CClause::DeleteVerbHomExceptBytStat(int iGoodNode)
{
    for (int i = 0; (size_t)i < m_ClauseWords.size(); i++) {
        if (i == iGoodNode)
            continue;
        CWord& word = m_Words.GetWord(m_ClauseWords[i]);
        yvector<int> del_hom;
        for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
            Wtroka strLemma = it->GetLemma();
            if (it->IsPersonalVerb() && strLemma != kByt && strLemma != kStat)
                del_hom.push_back(it.GetID());
        }

        if (del_hom.size() < word.GetRusHomonymsCount())
            for (size_t j = 0; j < del_hom.size(); j++)
                word.DeleteHomonym(del_hom[j]);
    }

}

//two predications rule
void CClause::DeleteHomPredic()
{
    const TGramBitSet AdjectiveShort(gAdjective, gShort);
    const TGramBitSet ParticipleShort(gParticiple, gShort);
    const TGramBitSet VerbMask(gVerb);
    const TGramBitSet PraedicMask(gPraedic);

    TGramBitSet PredicPOSes[6] = { VerbMask, AdjectiveShort, ParticipleShort, PraedicMask, TGramBitSet(gGerund), TGramBitSet(gImperative)   };

    const TGramBitSet pos_mask(gVerb, gPraedic, gGerund, gImperative);

    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);
        CWord::SHomIt it = word.IterHomonyms();
        //looking for first word having only homonyms with special POSes
        for (; it.Ok(); ++it)
            if (!it->IsShortAdjectiveOrParticiple() && !it->HasAnyOfPOS(pos_mask))
                break;

        if (!it.Ok())     // found such a word
        {
            size_t iGoodNode = i;
            bool IsVerb = word.HasPOS(gVerb);
            bool IsBytStat = IsVerb && (word.HasHomonym(kByt) || word.HasHomonym(kStat));
            bool IsPred = (word.HasPOS(AdjectiveShort) || word.HasPOS(ParticipleShort) || word.HasPOS(gPraedic)) &&
                          !IsVerb && !word.HasPOS(gGerund) && !word.HasPOS(gImperative);

            for (int j = 0; j < 6; j++) {
                //do not drop homonyms with some POS if head of clause is verbs "byt" or "stat" [translit]
                if (IsBytStat && (PredicPOSes[j] == AdjectiveShort || PredicPOSes[j] == ParticipleShort || PredicPOSes[j] == PraedicMask))
                    continue;

                if (IsPred && PredicPOSes[j] == VerbMask) {
                    DeleteVerbHomExceptBytStat(iGoodNode);
                    continue;
                }

                for (size_t k = 0; k < m_ClauseWords.size(); ++k)
                    if (k != iGoodNode)
                        m_Words.GetWord(m_ClauseWords[k]).DeleteHomonymWithPOS(PredicPOSes[j]);
            }
        }
    }
}

Wtroka CClause::ToString(bool bAllWords) const
{
    Wtroka res;
    if (bAllWords)
        for (int i = FirstWord(); i <= LastWord(); ++i) {
            const CWord& w = m_Words.GetOriginalWord(i);
            res += w.GetOriginalText();
            if (i < LastWord())
                res += ' ';
        }
    else
        for (size_t i = 0; i < m_ClauseWords.size(); ++i) {
            const CWord& w = m_Words.GetWord(m_ClauseWords[i]);
            res += w.GetOriginalText();
            if (i + 1 < m_ClauseWords.size())
                res += ' ';
        }
    return res;
}

int CClause::FindSubjVal(int iType) //finds valency for this type
{
    if (iType < 0 || (size_t)iType >= m_Types.size())
        return -1;

    SClauseType& type = m_Types[iType];

    if (!type.m_NodeNum.IsValid())
        return -1;

    const article_t* piArt = GlobalDictsHolder->GetAuxArticle(m_Words[type.m_NodeNum], PREDIC_DICT);
    for (size_t i = 0; i < type.m_Vals.size(); i++) {
        const or_valencie_list_t& piValVar = piArt->get_valencies().at(i);

        size_t j;
        for (j = 0; j < piValVar.size(); j++) {
            const valencie_t& piVal = piValVar[j];
            if ((piVal.m_syn_rel == SubjDat) ||
                (piVal.m_syn_rel == Subj)   ||
                (piVal.m_syn_rel == SubjGen))
                break;
        }

        if (j < piValVar.size())
            return i;
    }

    return -1;
}

bool CClause::IsValid() const
{
    return !m_ClauseWords.empty();

}
void CClause::Init(bool bBuildAnalyticForm /*= true*/)
{
    if (m_ClauseWords.size() == 0)
        InitClauseWords();

    //for cloned clauses CClause::Init runs one more time
    //BuildAnalyticWordForms may not be called after
    if (bBuildAnalyticForm)
        BuildAnalyticWordForms();
    //two predications rule
    DeleteHomPredic();
    RunSynAn();
    InitClauseInfo();
}

void CClause::BuildAnalyticWordForms()
{
    if (HasType(Brackets))
        return;
    CAnalyticFormBuilder AnalyticFormBuilder((CWordVector&)m_Words, m_ClauseWords);
    AnalyticFormBuilder.Run();
}

void CClause::InitClauseWords(CClause& clause)
{
    m_ClauseWords = clause.m_ClauseWords;
}

void CClause::InitClauseWords()
{
    for (int i = FirstWord(); i <= LastWord(); i++)
        m_ClauseWords.push_back(SWordHomonymNum(i, -1));
}

void CClause::InitClauseInfo()
{
    InitConjs();
    InitNodes();
    InitClauseTypes();
}

void CClause::AdjustTypesAndConjs()
{
    int i;
    for (i = m_Types.size() - 1; i >= 0; i--)
        if (!m_Types[i].AdjustByPair(*this, m_Words))
            m_Types.erase(m_Types.begin() + i);

    for (i = m_Conjs.size() - 1; i >= 0; i--)
        if (!m_Conjs[i].IsInPair(*this, m_Words))
            m_Conjs.erase(m_Conjs.begin() + i);
}

void CClause::SubstituteByMultiWord(SWordHomonymNum& WordHomonymNum)
{
    const CWord& multi_word = m_Words.GetWord(WordHomonymNum);

    // do not extend the clause to the right:
    if (multi_word.GetSourcePair().LastWord() > LastWord())
        return;

    //if the first word of conjunction expands over left border
    if (multi_word.GetSourcePair().FirstWord() < FirstWord()) {
        SetFirstWord(multi_word.GetSourcePair().FirstWord());
        for (size_t i = 0; i < m_ClauseWords.size(); i++) {
            const CWord& w = m_Words.GetWord(m_ClauseWords[i]);
            if (w.GetSourcePair().FirstWord() == multi_word.GetSourcePair().LastWord()) {
                m_ClauseWords.erase(m_ClauseWords.begin(), m_ClauseWords.begin() + i + 1);
                break;
            }
        }
        m_ClauseWords.insert(m_ClauseWords.begin(), WordHomonymNum);
    } else {
        int k1 = -1, k2 = -1;
        for (size_t i = 0; i < m_ClauseWords.size(); i++) {
            const CWord& w = m_Words.GetWord(m_ClauseWords[i]);
            if (w.IsOriginalWord() && w.GetSourcePair().FirstWord() == multi_word.GetSourcePair().FirstWord())
                k1 = i;

            if (k1 != -1) {
                if (!w.IsOriginalWord())
                    break;

                if (w.GetSourcePair().FirstWord() == multi_word.GetSourcePair().LastWord()) {
                    k2 = i;
                    break;
                }
            }
        }

        if (k1 != -1 && k2 != -1)
            m_ClauseWords.erase(m_ClauseWords.begin() + k1, m_ClauseWords.begin() + k2 + 1);

        if (k1 != -1)
            m_ClauseWords.insert(m_ClauseWords.begin() + k1, WordHomonymNum);
    }
}

bool CClause::HasNounWithOnlyNominative()
{
    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        SWordHomonymNum& WordHomonymNum = m_ClauseWords[i];
        const CWord& word = m_Words.GetWord(WordHomonymNum);
        if (!word.HasOnlyPOS(gSubstantive))
            continue;
        CWord::SHomIt it = word.IterHomonyms();
        const CHomonym& hom = *(it);
        if (hom.Grammems.HasNominativeOnly())
            return true;
    }
    return false;
}

SWordHomonymNum CClause::FindNounNominative()
{
    SWordHomonymNum res;
    bool  bFirst = true;
    int iBadWeightRes = 0;
    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        SWordHomonymNum& WordHomonymNum = m_ClauseWords[i];
        const CWord& word = m_Words.GetWord(WordHomonymNum);

        for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
            const CHomonym& hom = *(it);
            int iBadWeight = 0;
            if (CanBeSubject(hom, iBadWeight)) {
                if (word.GetRusHomonymsCount() > 1)
                    iBadWeight += 10;

                if (word.HasPOS(gAdjPronoun))
                    iBadWeight += 10;

                if (bFirst || iBadWeight < iBadWeightRes) {
                    res = WordHomonymNum;
                    res.m_HomNum = it.GetID();
                    iBadWeightRes = iBadWeight;
                    bFirst = false;
                }
            }
        }
    }
    return res;
}

CClause* CClause::CloneBySubConj()
{
    for (size_t i = 0; i < m_Conjs.size(); i++) {
        if (m_Conjs[i].IsSimConj())
            continue;
        if (i > 0) {
            const CWord& w1 = m_Words.GetWord(m_Conjs[i-1].m_WordNum);
            const CWord& w2 = m_Words.GetWord(m_Conjs[i].m_WordNum);
            if ((w1.GetSourcePair().LastWord() + 1) != w2.GetSourcePair().FirstWord())
                return NULL;

            const CHomonym& h1 = m_Words[m_Conjs[i-1].m_WordNum];
            const CHomonym& h2 = m_Words[m_Conjs[i].m_WordNum];

            if (!h1.IsSimConj() && !h2.IsSimConj()) {
                CClause* newClause = CClause::Create(m_Words);
                const CWord& first_word = m_Words.GetWord(m_ClauseWords[0]);
                SetFirstWord(w2.GetSourcePair().FirstWord());
                newClause->SetPair(first_word.GetSourcePair().FirstWord(), w1.GetSourcePair().LastWord());
                UnlockHomonym(h1);
                newClause->InitClauseWords();
                newClause->TryToFindMultiWordConjs();
                newClause->Init(false);
                return newClause;
            }
        }
    }
    return NULL;
}

CClause* CClause::Create(CWordVector& Words)
{
    return /*CGramInfo::s_bRunSynAn ? new CClauseSyn(Words) :*/ new CClause(Words);
}

bool CClause::EqualByConj(CClause& clause)
{
    if (FirstWord() != clause.FirstWord() || LastWord() != clause.LastWord())
        return false;
    for (size_t i = 0; i < m_Conjs.size(); i++) {
        size_t j;
        for (j = 0; j < clause.m_Conjs.size(); j++) {
            if (m_Conjs[i].Equal(clause.m_Conjs[j]))
                break;
        }
        if (j >= clause.m_Conjs.size())
            return false;
    }
    return true;
}

void CClause::FindAllMultiWordsConj(yvector<SWordHomonymNum>& multiWords)
{
    for (size_t i = 0; i < m_Words.GetMultiWordsCount(); ++i) {
        const CWord& multiWord = m_Words.GetMultiWord(i);

        if (!multiWord.HasConjHomonym())
            continue;

        if (multiWord.GetSourcePair().LastWord() < FirstWord())
            continue;

        if (multiWord.GetSourcePair().LastWord() > LastWord())
            break;

        multiWords.push_back(SWordHomonymNum(i,-1, false));
    }

}

void CClause::FindMultiWords()
{
    yvector<SWordHomonymNum> multiWords;

    for (size_t i = 0; i < m_Words.GetMultiWordsCount(); ++i) {
        const CWord& multiWord = m_Words.GetMultiWord(i);

        if (multiWord.HasConjHomonym())
            continue;

        if (!this->Includes(multiWord.GetSourcePair()))
            continue;

        multiWords.push_back(SWordHomonymNum(i,-1, false));
    }
    for (size_t i = 0; i < multiWords.size(); i++)
        SubstituteByMultiWord(multiWords[i]);
}

bool CClause::TryToFindMultiWordConjs()
{
    bool bNewClause = false;
    yvector<SWordHomonymNum> multiWords;
    FindAllMultiWordsConj(multiWords);
    //int iCorr = -1;
    const CHomonym* pCorrH = NULL;
    const CWord*    pCorrW = NULL;
    for (size_t i = 0; i < multiWords.size(); i++) {
        const CWord& word = m_Words.GetWord(multiWords[i]);

        CWord::SHomIt it = word.IterHomonyms();
        for (; it.Ok(); ++it)
            if (it->IsConj() && !IsLockedHomonym(*it))
                break;

        if (!it.Ok())
            continue;

        const CHomonym& conj = *(it);

        bool bBegPos = true;
        int iW = 0;

        for (size_t j = 0; j < m_ClauseWords.size(); j++) {
            const CWord& w = m_Words.GetWord(m_ClauseWords[j]);

            if (w.GetSourcePair().FirstWord() == word.GetSourcePair().FirstWord())
                iW = j;

            if (w.GetSourcePair().LastWord() >= word.GetSourcePair().FirstWord())
                break;
            if (w.HasConjHomonym())
                bBegPos = false;
        }

        if (!CheckConjPosition(conj, iW, bBegPos))
            continue;

        if (!GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT)->get_del_homonym()) {
            bNewClause = true;
            if (conj.HasGrammem(gCorrelateConj)) {
                pCorrW = &word;
                pCorrH = &conj;
            }
        }

        SubstituteByMultiWord(multiWords[i]);
    }

    if (pCorrW && pCorrH) {
        const CWord& w = *pCorrW;
        int iPos = w.GetSourcePair().LastWord();
        const CHomonym& h = *pCorrH;
        size_t i = 0;
        for (; i < m_Words.GetAllWordsCount(); ++i) {
            const CWord& word = m_Words.GetAnyWord(i);
            if (word.GetSourcePair().FirstWord() <= iPos)
                continue;
            int iH = word.HasConjHomonym_i();
            if (iH == -1)
                continue;
            const CHomonym& hom = word.GetRusHomonym(iH);
            const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, CONJ_DICT);
            int ic = piArt->get_corrs_count();
            int j;
            for (j = 0; j < ic; j++) {
                if (h.GetLemma() == piArt->get_corr(j))
                    break;
            }
            if (j < ic)
                break;
        }
        if (i < m_Words.GetAllWordsCount())
            bNewClause = false;
    }

    return bNewClause;
}

//Finds next conjunction and locks it using LockHomonym()
//On next call looks for unlocked homonyms and returns true if any
bool CClause::InitConjs()
{
    if (m_Conjs.size() > 0)
        return false;
    //looking for adjacent conjunctions with GetPosition() == OnTheBeginning
    //as well as with GetPosition() == AnyPosition or AfterWeb (for "li" [translit])
    bool bPrevConjs = true;
    bool bFound = false;
    bool bFoundNotLocked = false;

    const TGramBitSet conj_mask(gPronounConj, gSubConj);

    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);

        bool bOneConj = word.HasOneConjHomonym();
        CWord::SHomIt it = word.IterHomonyms();
        for (; it.Ok(); ++it) {
            const CHomonym& hom = word.GetRusHomonym(it.GetID());

            if (!hom.IsConj())
                continue;

            if ((!hom.IsConj() || IsLockedHomonym(hom)) && !bOneConj)
                continue;

            if (!CheckConjPosition(hom, i, bPrevConjs))
                continue;

            if ((hom.IsSimConj() && i > 0) || hom.HasGrammem(gCorrelateConj))
                continue;

            if (!bOneConj)
                bFoundNotLocked = true;

            SConjType conjType;
            conjType.m_Type = SConjType::GetConjType(hom);
            conjType.m_bSimConjAnd = SConjType::IsSimConjAnd(hom);
            conjType.m_WordNum = m_ClauseWords[i];
            conjType.m_WordNum.m_HomNum = it.GetID();
            LockHomonym(hom);
            if (bFoundNotLocked)
                bFound = true;
            m_Conjs.push_back(conjType);
            if (conjType.m_Type == gPronounConj || conjType.m_Type == gSubConj) // чтобы не плодить постоянно варианты для "что - мест_союз" и "что - подч_союз"
            {
                CWord::SHomIt it1 = it;
                ++it1;
                for (; it1.Ok(); ++it1)
                    if (it1->Grammems.HasAny(conj_mask))
                        LockHomonym(*it1);
            }
            break;
        }

        if (!it.Ok()) {
            bPrevConjs = false;
            continue;
        }

    }

    return bFound && bFoundNotLocked;
}

bool CClause::CheckConjPosition(const CHomonym& hom, int iW, bool bBegPOS)
{
    const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, CONJ_DICT);

    const TGramBitSet VerbInfinitivePraedic(gVerb, gInfinitive, gPraedic);
    const TGramBitSet AdjectiveShort(gAdjective, gShort);
    const TGramBitSet ParticipleShort(gParticiple, gShort);

    for (int i = 0; i < piArt->get_positions_count(); i++) {
        EPosition position = piArt->get_position(i);

        switch (position) {
            case AnyPosition:
                return true;

            case OnTheBeginning:
                {
                    if (bBegPOS)
                        return true;
                    if (!hom.IsSimConj()) {
                        if (iW > 0) {
                            const CWord& word = m_Words.GetWord(m_ClauseWords[iW - 1]);
                            if (word.HasPOS(gCorrelateConj))
                                return true;
                        }
                    }
                    break;
                }

            case AfterVerb:
                {
                    if (iW > 0) {
                        const CWord& word = m_Words.GetWord(m_ClauseWords[iW - 1]);
                        if (word.HasAnyOfPOS(VerbInfinitivePraedic) || word.HasPOS(AdjectiveShort) || word.HasPOS(ParticipleShort))
                            return true;
                    }
                    break;
                }
            case AfterPrep:
                {
                    if (iW > 0) {
                        const CWord& word = m_Words.GetWord(m_ClauseWords[iW - 1]);
                        if (word.HasPOS(gPreposition))
                            return true;
                    }
                    break;
                }

            default:
                break;
        }
    }

    return false;
}

bool CClause::InitBracketsType()
{
    if (m_ClauseWords.size() == 0)
        return false;

    if (m_Words.GetWord(m_ClauseWords[0]).IsOpenBracket()) {
        int j;
        for (j = m_ClauseWords.size()-1; j >= 0; j--) {
            const CWord& word = m_Words.GetWord(m_ClauseWords[j]);
            if (word.IsCloseBracket())
                break;
            if (!word.IsPunct())
                return false;
        }
        if (j < 0)
            return false;

        m_Types.clear();
        SClauseType type;
        type.m_Type = Brackets;
        m_Types.push_back(type);
        return true;
    }

    return false;
}

bool CClause::InitParenthType()
{
    if (m_ClauseWords.size() == 0)
        return false;

    const CWord& word = m_Words.GetWord(m_ClauseWords[0]);
    const CWord& last_word = m_Words.GetWord(m_ClauseWords[m_ClauseWords.size() - 1]);
    const CWord* parenthWord = NULL;
    const article_t* piArt = NULL;

    SWordHomonymNum new_wh;
    int iH = word.HasArticle_i(SArtPointer(GlobalDictsHolder->BuiltinKWTypes().Parenthesis));
    if (iH != -1) {
        piArt = GlobalDictsHolder->GetAuxArticle(word.GetRusHomonym(iH), KW_DICT);
        parenthWord = &word;
    } else {
        for (size_t i = 0; i < m_Words.GetMultiWordsCount(); ++i) {
            const CWord& mw = m_Words.GetMultiWord(i);
            if (!(mw.GetSourcePair().FirstWord() == FirstWord() &&
                    this->Includes(mw.GetSourcePair())))
                    continue;

            int iH = mw.HasArticle_i(SArtPointer(GlobalDictsHolder->BuiltinKWTypes().Parenthesis));
            if (iH != -1) {
                piArt = GlobalDictsHolder->GetAuxArticle(mw.GetRusHomonym(iH), KW_DICT);
                new_wh.m_bOriginalWord = false;
                new_wh.m_HomNum = iH;
                new_wh.m_WordNum = i;
                parenthWord = &mw;
                break;
            }
            if (piArt != NULL)
                break;
        }
    }

    if ((piArt == NULL) || (parenthWord == NULL))
        return false;

    if (piArt->get_end_comma()) {
        if (m_ClauseWords.size() < 2)
            return false;
        if (!((parenthWord->GetSourcePair().LastWord() == (LastWord() -1)) && last_word.IsPunct()))
            return false;

        SClauseType type;
        type.m_Type = Parenth;
        m_Types.clear();
        m_Types.push_back(type);
        if (new_wh.IsValid()) {
            SubstituteByMultiWord(new_wh);
            //иначе координаты всех групп съедут
            RunSynAn();
        }
        return true;
    } else {
        int iEmptyType = HasType_i(Empty);

        if (((parenthWord->GetSourcePair().LastWord() == (LastWord() -1)) && last_word.IsPunct()))
            return false;

        if (iEmptyType == -1) {
            SClauseType ClauseType;
            ClauseType.m_Type =  Parenth;
            m_Types.push_back(ClauseType);
        } else
            m_Types[iEmptyType].m_Type = Parenth;

        //drop types, which are in this multiWord
        if (m_Types.size() > 1) {
            for (int i = m_Types.size() -1; i >= 0; i--) {
                SWordHomonymNum whNode = m_Types[i].m_NodeNum;
                if (!whNode.IsValid())
                    return false;
                if (m_Words.GetWord(whNode).GetSourcePair().Intersects(m_Words.GetWord(new_wh).GetSourcePair()))
                    m_Types.erase(m_Types.begin() + i);
            }
        }

        if (new_wh.IsValid()) {
            SubstituteByMultiWord(new_wh);
            //иначе координаты всех групп съедут
            RunSynAn();
        }

        int iAdjShortType = HasTypeWithPOS_i(TGramBitSet(gAdjective, gShort));
        if ((iAdjShortType != -1) && (m_Types.size() == 2)) //только пустыха и кр. прил.
            DeleteType(iAdjShortType);

        return false;
    }

}

bool CClause::HasTypeWithSubj()
{
    for (size_t j = 0; j < m_Types.size(); j++) {
        if (!m_Types[j].m_NodeNum.IsValid())
            continue;

        if (m_Types[j].HasSubjVal_i() != -1)
            return true;
    }
    return false;
}

void CClause::InitClauseTypes()
{
    if (InitBracketsType())
        return;

    if (InitParenthType())
        return;

    yvector<SClauseType> new_types;
    yvector<int> del_types;

    for (size_t i = 0; i < m_Types.size(); i++) {
        new_types.clear();
        if (!InitClauseType(m_Types[i], new_types))
            del_types.push_back(i);

        if (new_types.size()) {
            m_Types.insert(m_Types.begin() + i,new_types.begin(), new_types.end());
            i += new_types.size();
        }
    }

    if (del_types.size()) {
        if (m_Types.size() > del_types.size()) {
            for (int i = del_types.size() - 1; i >= 0; i--)
                m_Types.erase(m_Types.begin() + del_types[i]);
            TryToAddEmptyType();
        } else {
            m_Types.clear();
            SClauseType ClauseType;
            ClauseType.m_Type =  Empty;
            m_Types.push_back(ClauseType);
        }
    }
}

int CClause::FindLemma(const Wtroka& strLemma, EDirection direction)
{
    int i = (direction == RightToLeft) ? m_ClauseWords.size() -1 : 0;

    for (;  ((direction == RightToLeft) ? (i >= 0) : (i < (int)m_ClauseWords.size()));
            ((direction == RightToLeft) ? i-- : i++)) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);

        if (word.FindLemma(strLemma))
            return i;
    }

    return -1;
}

bool  CClause::FindAntecedent(const Wtroka& strLemma)
{
    int iClWord = FindLemma(strLemma, RightToLeft);

    if (iClWord == -1)
        return false;

    if (iClWord < GetFirstClauseWordWithType())
        return false;

    return true;
}

long CClause::GetTypeBadWeight(int iType)
{
    long iBadWeight = 0;
    SClauseType& type = m_Types[iType];

    if (type.m_Type == NeedNode)
        iBadWeight += 5;

    if (type.HasPOS(gInfinitive))
        if (!HasSubConj())
            iBadWeight += 2;
        else
            iBadWeight++;

    if (type.m_Type == Empty) {
        iBadWeight++;
        if (HasNounWithOnlyNominative())
            iBadWeight += 2;
    }

    if (type.HasPOS(gImperative))
        iBadWeight++;

    if (type.m_NodeNum.IsValid() && (type.m_Type== WantsVal) && (type.m_Modality == Necessary)) {
        const CHomonym& hom = m_Words[type.m_NodeNum];
        int iW = FindClauseWord(type.m_NodeNum);
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, PREDIC_DICT);
        DECLARE_STATIC_RUS_WORD(kBadTitle1, "_глагол_безличн");
        if (piArt->get_title() == kBadTitle1)
            iBadWeight += 2;
        else
            iBadWeight += 3;

        for (size_t i = 0; i < type.m_Vals.size(); i++) {
            SValency& val = type.m_Vals[i];

            if (val.m_Actant.IsValid() && (val.m_ActantType == WordActant)) {
                int iWA = FindClauseWord(val.m_Actant);
                const or_valencie_list_t& piValVar = piArt->get_valencies().at(val.m_ValNum);

                size_t j;
                for (j = 0; j < piValVar.size(); j++) {
                    const valencie_t& piVal = piValVar[j];
                    if (piVal.m_syn_rel == val.m_Rel)
                        break;
                }

                if (j < piValVar.size()) {
                    const valencie_t& piVal = piValVar[j];
                    if ((piVal.m_position == OnTheRight) && (iWA < iW))
                        iBadWeight++;

                    if ((piVal.m_position == OnTheLeft) && (iWA > iW))
                        iBadWeight++;
                }
            }
        }
    }

    return iBadWeight;
}

long CClause::CalculateTypesBadWeight()
{
    long minBadWeight = 0;
    for (size_t i = 0; i < m_Types.size(); i++) {
        long weight = GetTypeBadWeight(i);
        if ((weight < minBadWeight) || (i == 0))
            minBadWeight = weight;
    }

    if (HasSimConj()) {
        if (m_ClauseWords.size() > 0) {
            if (FirstWord() > 0) {
                const CWord& w = m_Words[FirstWord() - 1];
                if (!w.IsPunct())
                    minBadWeight += 2;
            }
        }
    }

    return minBadWeight;
}

long CClause::GetBadWeightForCommas()
{
    long res = 0;
    for (int i = 0; i < (int)m_BadCommas.size(); i++)
        res += m_BadCommas[i].m_iBadWeight;
    return res;
}

long CClause::CalculateBadWeight()
{
    if (m_bCalculatedBadWeight)
        return m_BadWeight;
    m_RuleBadWeight = m_BadWeight;
    m_BadWeight += CalculateTypesBadWeight();
    m_BadWeight += GetBadWeightForCommas();
    m_bCalculatedBadWeight = true;
    return m_BadWeight;
}

const SConjType& CClause::GetConjType(int iConj) const
{
    return m_Conjs[iConj];
}

const CHomonym& CClause::GetConj(int iConj) const
{
    return m_Words[m_Conjs[iConj].m_WordNum];
}

void CClause::MultiplyBadCommaWeight(CWordsPair& pair, int coef)
{
    if (!pair.IsValid() || (coef == 1))
        return;

    for (int i = 0; i < (int)m_BadCommas.size(); i++) {
        if ((m_BadCommas[i].m_iNum > pair.FirstWord()) && (m_BadCommas[i].m_iNum < pair.LastWord()))
            m_BadCommas[i].m_iBadWeight *= coef;
    }
}

void CClause::AddBadComma(int num, int w, CWordsPair& pair, int coef)
{
    AddBadComma(num, w);
    MultiplyBadCommaWeight(pair, coef);
}

void CClause::AddBadComma(int num, int w)
{
    if (w > 0)
        m_BadCommas.push_back(SBadComma(num, w));
}

bool CClause::CanBeSubject(const CHomonymBase& hom, int& iBadWeight) {
    static const TGramBitSet mask(gSubstantive, gSubstPronoun, gPronounConj, gNumeral);
    if (!(hom.Grammems.HasAny(mask) || (hom.HasGrammem(gAdjPronoun) && hom.Grammems.HasNominativeOnly())))
        return false;

    if (!hom.HasGrammem(gNominative))
        return false;

    if (hom.HasGrammem(gNumeral))
        iBadWeight += 6;

    if (hom.HasGrammem(gPronounConj))
        iBadWeight++;

    if (!hom.Grammems.HasNominativeOnly()) {
        if ((hom.Grammems.All() & NSpike::AllCases) == TGramBitSet(gNominative, gAccusative))
            iBadWeight += 4;
        else if (hom.Grammems.HasAllCases())
            iBadWeight += 6;
        else
            iBadWeight += 5;
    }
    return true;
}

bool CClause::FindSubject(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc)
{
    //try
    {
        int iBadWeightRes = 0xFFFF;
        bool bBefore = true;
        subj.Reset();
        const CHomonym& hPred = m_Words[pred];
        const TGramBitSet nominative(gNominative);
        for (size_t i = 0; i < m_ClauseWords.size(); i++) {
            SWordHomonymNum& WordHomonymNum = m_ClauseWords[i];
            if ((WordHomonymNum.m_WordNum == pred.m_WordNum) && (WordHomonymNum.m_bOriginalWord == pred.m_bOriginalWord)) {
                bBefore = false;
                continue;
            }
            int iBadWeight = 0;
            SWordHomonymNum word_num;
            word_num = WordHomonymNum;

            const CWord& word = m_Words.GetWord(word_num);

            if (word.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Date))
                continue;

            for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
                const CHomonym& hom = *it;
                word_num.m_HomNum = it.GetID();
                if (!CanBeSubject(hom, iBadWeight))
                    continue;

                if (!NGleiche::Gleiche(hom, hPred, CoordFunc))
                    continue;

                bool bHasPrimitve = false;
                if (HasSynInfo()) {
                    yvector<CGroup*> groups;
                    GetAllGroupsByMainWordWithGrammems(groups, i, it.GetID(), nominative);

                    if (groups.size() == 0)
                        continue;

                    bool bGoodNounGroup = false;
                    for (size_t j = 0; j < groups.size(); j++) {
                        CGroup* pGroup = groups[j];
                        if (pGroup->IsPrimitive()) {
                            bHasPrimitve = true;
                            bGoodNounGroup = true;
                        } else {
                            const TGramBitSet& gr_grammems = pGroup->GetGrammems().All();
                            if (CGramInfo::IsNounGroupForSubject(pGroup->GetActiveSymbol())) {
                                bGoodNounGroup = true;
                                if (!gr_grammems.HasAny(NSpike::AllCases & ~nominative)) {
                                    iBadWeight = 0;
                                    break;
                                }
                            }
                        }
                    }

                    if (!bGoodNounGroup)
                        continue;
                } else
                    bHasPrimitve = true;

                if (bHasPrimitve) {
                    bool hasNominativeOnly = true;
                    for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it)
                        if (!it->Grammems.HasNominativeOnly()) {
                            hasNominativeOnly = false;
                            break;
                        }

                    if (word.GetRusHomonymsCount() > 1 && !hasNominativeOnly) {
                        if (GlobalDictsHolder->HasKWType(hom, GlobalDictsHolder->BuiltinKWTypes().Fio) ||
                            GlobalDictsHolder->HasKWType(hom, GlobalDictsHolder->BuiltinKWTypes().FioChain))
                            iBadWeight += 2;
                        else
                            iBadWeight += 10;
                    } else {
                        //если нашли такое замечательное фио (обязательно из 2 или более слов) с одним им. п., то счиатем, что оно уж точно подл.(грубовато...)
                        if ((GlobalDictsHolder->HasKWType(hom, GlobalDictsHolder->BuiltinKWTypes().Fio) ||
                            GlobalDictsHolder->HasKWType(hom, GlobalDictsHolder->BuiltinKWTypes().FioChain))
                            && word.GetSourcePair().Size() > 1) {
                            if (hom.Grammems.HasNominativeOnly()) {
                                iBadWeightRes = 0;
                                subj = word_num;
                                i = m_ClauseWords.size();
                                break;
                            }
                        }
                    }

                    //for quoted company name with all the cases - increase bad weight
                    if (GlobalDictsHolder->HasKWType(hom, GlobalDictsHolder->BuiltinKWTypes().CompanyInQuotes) && hom.Grammems.HasAllCases())
                        iBadWeight += 10;

                    if (word.HasPOS(gAdjPronoun))
                        iBadWeight += 10;

                    //типа "его"
                    if (hom.HasPOS(gAdjPronoun) && word.HasPOS(gSubstPronoun))
                        continue;

                }

                if (!bBefore)
                    iBadWeight += 2;

                if (iBadWeight < iBadWeightRes) {
                    iBadWeightRes = iBadWeight;
                    subj = word_num;
                }
            }
        }

        if (!subj.IsValid())
            return FindNounGroupAsSubject(pred, subj, CoordFunc);
        return subj.IsValid();
    }
    /*catch(...)
    {
        ythrow yexception() << "Error un \"CClause::FindSubject\"";
    }*/
}

bool CClause::FindNounGroupAsSubject(SWordHomonymNum&, SWordHomonymNum&, ECoordFunc)
{
    return false;
}

bool CClause::FindSubjectOld(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc)
{
    int iBadWeightRes = 0;
    bool  bFirst = true;
    bool bBefore = true;
    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        SWordHomonymNum& WordHomonymNum = m_ClauseWords[i];
        if ((WordHomonymNum.m_WordNum == pred.m_WordNum) && (WordHomonymNum.m_bOriginalWord == pred.m_bOriginalWord)) {
            bBefore = false;
            continue;
        }
        int iBadWeight = 0;
        SWordHomonymNum word_num;
        word_num = WordHomonymNum;

        const CWord& word = m_Words.GetWord(word_num);

        for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
            const CHomonym& hom = *it;
            word_num.m_HomNum = it.GetID();
            if (CanBeSubject(hom, iBadWeight) &&
                NGleiche::Gleiche(m_Words[word_num], m_Words[pred], CoordFunc)) {
                TKeyWordType kwType = GlobalDictsHolder->GetKWType(hom, KW_DICT);

                if (word.GetRusHomonymsCount() > 1) {
                    if (kwType == GlobalDictsHolder->BuiltinKWTypes().Fio || kwType == GlobalDictsHolder->BuiltinKWTypes().FioChain)
                        iBadWeight += 3;
                    else
                        iBadWeight += 10;
                }

                if (word.HasPOS(gAdjPronoun))
                    iBadWeight += 10;

                //типа "его"
                if (hom.HasPOS(gAdjPronoun) && word.HasPOS(gSubstPronoun))
                    continue;

                if ((kwType == GlobalDictsHolder->BuiltinKWTypes().Fio || kwType == GlobalDictsHolder->BuiltinKWTypes().FioChain)
                    && word.GetSourcePair().Size() > 1 && word.GetRusHomonymsCount() == 1) {
                    if (hom.Grammems.HasNominativeOnly()) {
                        iBadWeightRes = 0;
                        subj = word_num;
                        i = m_ClauseWords.size();
                        break;
                    }
                }

                if (!bBefore)
                    iBadWeight++;

                if (bFirst || iBadWeight < iBadWeightRes) {
                    iBadWeightRes = iBadWeight;
                    subj = word_num;
                    bFirst = false;
                }
            }
        }
    }

    return subj.IsValid();
}

bool CClause::HasWordsGroupSimConj()
{
    int iSimConj = HasSimConj_i();
    if (iSimConj == -1)
        return false;
    const CHomonym& sim_conj = m_Words[m_Conjs[iSimConj].m_WordNum];
    const article_t* piArt =  GlobalDictsHolder->GetAuxArticle(sim_conj, CONJ_DICT);
    if (piArt->get_conj_valencies().size() == 0)
        return true;
    for (size_t i = 0; i < piArt->get_conj_valencies().size(); i++) {
        const or_valencie_list_t& piValVars = piArt->get_conj_valencies().at(i);
        for (size_t j = 0; j < piValVars.size(); j++)
            if (piValVars[j].m_actant_type == WordActant)
                return true;
    }
    return false;
}

bool CClause::ConjValsHasTypeWord(const or_valencie_list_t& piValVars, EClauseType& type)
{
    valencie_t piVal;
    size_t i;
    for (i = 0; i < piValVars.size(); i++) {
        piVal = piValVars[i];
        if (piVal.m_actant_type == WordActant)
            break;
    }
    if (i < piValVars.size()) {
        if ((m_ClauseWords.size() == 1) ||
             !HasPunctAtTheEnd()) {
            type = SubConjEmptyNeedEnd;
            return true;
        }

        for (i = 0; i < m_ClauseWords.size(); i++) {
            const CWord& word = m_Words.GetWord(m_ClauseWords[i]);
            if (word.HasConjHomonym())
                continue;
            if (word.HasAnyOfPOS(piVal.m_POSes)) {
                type = SubConjEmpty;
                return true;
            }
        }
    }
    return false;
}

int  CClause::HasCorr_i()
{
    for (size_t i = 0; i < m_Conjs.size(); i++)
        if (m_Conjs[i].IsCorr())
            return i;
    return -1;
}

bool CClause::HasSimConjWithCorr()
{
    return (HasSimConjWithCorr_i() != -1);
}

int CClause::HasSimConjWithCorr_i()
{
    for (size_t i = 0; i < m_Conjs.size(); i++) {
        if (m_Conjs[i].IsSimConj()) {
            const CHomonym& sim_conj = GetConj(i);
            if (GlobalDictsHolder->IsSimConjWithCorr(sim_conj))
                return i;
        }
    }

    return -1;
}

int CClause::HasSimConj_i()
{
    for (size_t i = 0; i < m_Conjs.size(); i++)
        if (m_Conjs[i].IsSimConj())
            return i;

    return -1;
}

bool CClause::HasSimConj()
{
    return (HasSimConj_i() != -1);
}

bool CClause::HasSubConj()
{
    return (HasSubConj_i() != -1);
}

int  CClause::HasSubConj_i()
{
    for (size_t i = 0; i < m_Conjs.size(); i++)
        if (m_Conjs[i].IsSubConj())
            return i;

    return -1;
}

void CClause::InitClauseTypesByConj()
{

    size_t i;
    for (i = 0; i < m_Conjs.size(); i++)
        if (m_Conjs[i].IsSubConj())
            break;
    if (i >= m_Conjs.size())
        return;

    int iSubConj = i;

    const CHomonym& conj = m_Words[m_Conjs[iSubConj].m_WordNum];
    const article_t* piArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);
    if (piArt->get_conj_valencies().size() != 1)
        return;
    const or_valencie_list_t& piValVars = piArt->get_conj_valencies().at(0);

    yvector<int> delTypes;
    bool bHasEmptyType = false;
    for (i = 0; i < m_Types.size(); i++) {
        if (m_Types[i].m_NodePOS.any() && !m_Types[i].HasPOS(gSubstantive)) {
            bool bRes = CompTypeWithConjVals(m_Types[i], piValVars);
            if (bRes) {
                m_Conjs[iSubConj].m_Val.m_Actant = m_Types[i].m_NodeNum;
                m_Conjs[iSubConj].m_Val.m_ActantType = WordActant;
                m_Conjs[iSubConj].m_Val.m_ValNum = 0;
            }

            if (!bRes && !m_Types[i].HasPOS(gInfinitive) && !m_Types[i].HasPOS(gComparative) &&
                m_Types[i].m_Type != Empty)

                delTypes.push_back(i);

            if (m_Types[i].HasPOS(gInfinitive) && !bRes)
                m_Types[i].m_Type = NeedNode;
        } else {
            bHasEmptyType = true;
            if (m_Types[i].m_Type == Empty)
                m_Types[i].m_Type = NeedNode;
        }
    }

    if (bHasEmptyType) {
        EClauseType cl_type;
        if (ConjValsHasTypeWord(piValVars, cl_type)) {
            SClauseType newType;
            newType.m_Type = cl_type;
            m_Types.push_back(newType);
        }
    }

    if (delTypes.size() < m_Types.size()) {
        for (int j = (int)delTypes.size() - 1; j >= 0; j--)
            m_Types.erase(m_Types.begin() + delTypes[j]);
    }

}

int CClause::CompValencyWithWord(const valencie_t& piVal, const CWord& word, EPosition pos)
{
    for (CWord::SHomIt hom_it = word.IterHomonyms(); hom_it.Ok(); ++hom_it)
        if (CompValencyWithHomonym(piVal, *hom_it, pos))
            return hom_it.GetID();

    return -1;
}

bool CClause::CompValencyWithWord(const valencie_t& piVal, const CWord& word , EPosition pos, yvector<int>& homs)
{
    for (CWord::SHomIt hom_it = word.IterHomonyms(); hom_it.Ok(); ++hom_it)
        if (CompValencyWithHomonym(piVal, *hom_it, pos))
            homs.push_back(hom_it.GetID());

    return (homs.size() > 0);
}

bool CClause::CompValencyWithHomonym(const valencie_t& piVal, const CHomonym& hom, EPosition pos)
{
    if (!hom.HasAnyOfPOS(piVal.m_POSes))
        return false;

    const TGrammarBunch& grammems = piVal.m_morph_scale;
    bool has_grammems = grammems.empty();
    for (TGrammarBunch::const_iterator it = grammems.begin(); it != grammems.end(); ++it)
        if (hom.Grammems.HasAll(*it)) {
            has_grammems = true;
            break;
        }
    if (!has_grammems)
        return false;

    if (pos == AnyPosition)
        return true;

    if ((piVal.m_position != AnyPosition) && (pos != piVal.m_position))
        return false;

    return true;
}

bool CClause::CompTypeWithConjVals(SClauseType& type, const or_valencie_list_t& piValVars)
{
    if (!type.m_NodeNum.IsValid())
        return false;

    const CHomonym& node = m_Words[type.m_NodeNum];

    for (size_t i = 0; i < piValVars.size(); i++) {
        const valencie_t& piVal = piValVars[i];
        if (piVal.m_actant_type == WordActant || !node.HasAnyOfPOS(piVal.m_POSes))
            continue;

        if (piVal.m_morph_scale.empty())
            return true;
        else
            for (TGrammarBunch::const_iterator it = piVal.m_morph_scale.begin(); it != piVal.m_morph_scale.end(); ++it)
                if (node.Grammems.HasAll(*it))
                    return true;
    }
    return piValVars.empty();       //true for empty piValVars, otherwise false
}

bool CClause::FindActantInWordSpan(int iW1, int iW2, SWordHomonymNum& pred, const valencie_t& piVal,
                                   SWordHomonymNum& actant, EDirection direction)
{
    if (iW1 < 0)
        return false;
    if (iW2 >= (int)m_ClauseWords.size())
        return false;

    int i;
    for (i = (direction == LeftToRight) ? iW1 : iW2;
                (direction == LeftToRight) ? (i <= iW2) : (i >= iW1);
                i =  (direction == LeftToRight) ? (i + 1) : (i -1)) {

        yvector<int> homs;
        if (!CompValencyWithWord(piVal, m_Words.GetWord(m_ClauseWords[i]), piVal.m_position, homs))
            continue;

        for (size_t j = 0; j < homs.size(); j++) {
            actant = m_ClauseWords[i];
            actant.m_HomNum = homs[j];
            const CHomonym& pred_hom = m_Words[pred];
            const CHomonym& actant_hom = m_Words[actant];
            ECoordFunc CoordFunc = piVal.m_coord_func;
            if (NGleiche::Gleiche(actant_hom, pred_hom, CoordFunc))
                return true;
        }
    }
    return false;
}

bool CClause::FindActant(SWordHomonymNum& pred, const valencie_t& piVal, SWordHomonymNum& actant, EDirection direction)
{
    int iPredk = FindClauseWord(pred);

    int iW1, iW2;

    if (iPredk == -1) //if @pred from another clause
    {
        iW1 = 0;
        iW2 = m_ClauseWords.size() - 1;
    } else {
        if (piVal.m_position == OnTheLeft) {
            iW1 = 0;
            iW2 = iPredk-1;
        } else {
            iW1 = iPredk + 1;
            iW2 = m_ClauseWords.size() - 1;
        }
    }

    if (FindActantInWordSpan(iW1, iW2, pred, piVal, actant, direction))
        return true;

    if (iPredk == -1)
        return false;

    if (piVal.m_position == OnTheLeft) {
        iW1 = iPredk + 1;
        iW2 = m_ClauseWords.size() - 1;
    } else {
        iW1 = 0;
        iW2 = iPredk-1;
    }

    return FindActantInWordSpan(iW1, iW2, pred, piVal, actant, direction);
}

bool CClause::FillValency(SWordHomonymNum& pred, const valencie_t& piVal, SWordHomonymNum& actant)
{
    if (piVal.m_actant_type != WordActant)
        return false;
    ECoordFunc CoordFunc = piVal.m_coord_func;
    YASSERT(CoordFunc >= 0 && CoordFunc < CoordFuncCount);
    switch (CoordFunc) {
        case SubjVerb:
        case SubjAdjShort:
            return FindSubject(pred, actant, CoordFunc);
        default:
            return FindActant(pred, piVal, actant);
    }
}

void CClause::RefreshValencies()
{
    for (size_t i = 0; i < m_Types.size(); i++)
        FillValencies(m_Types[i], true);
}

void  CClause::ClearConjs()
{
    for (size_t i = 0; i < m_Conjs.size(); i++) {
        const CHomonym& hom = m_Words[m_Conjs[i].m_WordNum];
        UnlockHomonym(hom);
    }

    m_Conjs.clear();
}

void  CClause::ClearTypes()
{
    for (size_t i = 0; i < m_Types.size(); i++) {
        if (m_Types[i].m_NodeNum.IsValid()) {
            const CHomonym& hom = m_Words[m_Types[i].m_NodeNum];
            UnlockHomonym(hom);
        }
    }

    m_Types.clear();
}

void  CClause::RefreshClauseInfo()
{
    ClearConjs();
    ClearTypes();
    RunSynAn();
    InitConjs();
    InitNodes();
    InitClauseTypes();
    InitClauseTypesByConj();
}

void  CClause::RefreshConjInfo()
{
    ClearConjs();
    InitConjs();
    InitClauseTypesByConj();
}

bool CClause::FillValencies(SClauseType& Type, bool bRefresh /*= false*/)
{
    //try
    {
        if (!Type.m_NodeNum.IsValid())
            return false;
        const CHomonym& hom = m_Words[Type.m_NodeNum];
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, PREDIC_DICT);
        const valencie_list_t& piValencies = piArt->get_valencies();

        bool bAllValsFilled = true;

        if (piValencies.size() != Type.m_Vals.size())
            Type.m_Vals.resize(piValencies.size());

        for (size_t i = 0; i < piValencies.size(); i++) {
            Type.m_Vals[i].m_ValNum = i;
            const or_valencie_list_t& piValVars = piValencies[i];

            size_t j;
            for (j = 0; j <  piValVars.size(); j++) {

                const valencie_t& piVal = piValVars[j];

                if (bRefresh)
                    if (piVal.m_syn_rel != Subj)
                        continue;

                SWordHomonymNum actant;
                if (FillValency(Type.m_NodeNum, piVal, actant)) {
                    Type.m_Vals[i].m_Actant = actant;
                    Type.m_Vals[i].m_Rel = piVal.m_syn_rel;
                    LockHomonym(m_Words[actant]);
                    Type.m_Vals[i].m_ActantType = WordActant;
                    break;
                }
            }
            if (j == piValVars.size())
                bAllValsFilled = false;
        }
        return bAllValsFilled;
    }
    /*catch(yexception& e)
    {
        throw e;
    }
    catch(...)
    {
        ythrow yexception() << "Error in \"CClause::FillValencies\"";
    }*/
}

void CClause::CheckTypeByVals(SClauseType& Type)
{
    if (!Type.m_NodeNum.IsValid())
        return;

    Type.m_Type = WantsNothing;
    Type.m_Modality = Possibly;
    const valencie_list_t& piVals = GlobalDictsHolder->GetAuxArticle(m_Words[Type.m_NodeNum], PREDIC_DICT)->get_valencies();
    for (size_t i = 0; i < Type.m_Vals.size(); i++) {
        if (!Type.m_Vals[i].IsValid()) {
            Type.m_Type = WantsVal;
            const or_valencie_list_t& piValVars = piVals[i];
            if (piValVars.get_modality() == Necessary)
                Type.m_Modality = Necessary;
        }
    }
}

bool CClause::InitClauseType(SClauseType& Type, yvector<SClauseType>& /*new_types*/)
{
    if (!Type.m_NodeNum.IsValid())
        return true;

    const CHomonym& hom = m_Words[Type.m_NodeNum];

    FillValencies(Type);
    CheckTypeByVals(Type);

    //trying to find required word after a participle
    if (Type.HasPOS(gParticiple)) {
        if (FirstWord() == 0) //a participial clause cannot be at the beginning of a sentence
            return false;

        if (HasSimConj() && m_Words[FirstWord() - 1].IsPunct())
            return false;

        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, PREDIC_DICT);
        if (piArt->get_sup_valencies().size() != 1)
            return true;
        const or_valencie_list_t& piValVars = piArt->get_sup_valencies().at(0);

        for (size_t i = 0; i < piValVars.size(); i++) {
            const valencie_t& piVal = piValVars[i];
            if (piVal.m_actant_type != WordActant)
                continue;

            SWordHomonymNum actant;
            int iW1 = FindClauseWord(Type.m_NodeNum);
            if (FindActantInWordSpan(iW1 + 1, (int)m_ClauseWords.size() - 1, Type.m_NodeNum, piVal , actant, LeftToRight)) {
                size_t j;
                for (j = 0; j < m_Types.size(); j++)
                    if (m_Types[j].m_Type == Empty)
                        break;
                if (j >= m_Types.size()) {
                    SClauseType ClauseType;
                    ClauseType.m_Type =  Empty;
                    m_Types.push_back(ClauseType);
                }

                break;
            }
        }
    }

    return true;
}

//looking for nodes in order of importance
void CClause::InitNodes()
{
    yvector<size_t> homs_count;
    homs_count.resize(m_ClauseWords.size(), 0);

    if (TryToInitNode(TGramBitSet(gVerb), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gImperative), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gAdjective, gShort), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gParticiple, gShort), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gPraedic), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gGerund), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gParticiple), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gComparative), homs_count))
        return;

    if (TryToInitNode(TGramBitSet(gInfinitive), homs_count)) //инфинитив вроде как пустыха
        return;

    TryToAddEmptyType();
}

// пытаемся добавить пустыху, если вдруг у какого-нибудь слова все омонимы распределены по типам,
// то добавить пустыху не можем
void CClause::TryToAddEmptyType()
{
    yvector<int> homs_count;
    homs_count.resize(m_ClauseWords.size(), 0);

    for (size_t i = 0; i < m_Types.size(); i++) {
        if ((m_Types[i].m_Type == Empty) || (m_Types[i].m_Type == Parenth))
            return;
        if (!m_Types[i].m_NodeNum.IsValid())
            continue;
        int ii = FindClauseWord(m_Types[i].m_NodeNum);
        if ((ii >= 0) || (ii < (int)homs_count.size())) {
            homs_count[ii]++;
            if (homs_count[ii] == (int)(m_Words.GetWord(m_Types[i].m_NodeNum).GetRusHomonymsCount()))
                return;
        }
    }

    SClauseType ClauseType;
    ClauseType.m_Type =  Empty;
    m_Types.push_back(ClauseType);

}

int CClause::FindClauseWord(SWordHomonymNum& word)
{
    for (size_t i = 0; i < m_ClauseWords.size(); i++)
        if ((m_ClauseWords[i].m_WordNum == word.m_WordNum) &&
            (m_ClauseWords[i].m_bOriginalWord == word.m_bOriginalWord))
            return i;

    return -1;
}

bool CClause::CanBeNode(int, int)
{
    return true;
}

bool CClause::TryToInitNode(const TGramBitSet& POS, yvector<size_t>& homs_count)
{
    for (size_t i = 0; i < m_ClauseWords.size(); ++i) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);
        int iH = word.HasPOSi(POS);
        if (iH == -1 || !CanBeNode(i, iH))
            continue;

        for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it)
            if (it->HasPOS(POS)) {
                SWordHomonymNum WordHomonymNum = m_ClauseWords[i];
                WordHomonymNum.m_HomNum = it.GetID();
                LockHomonym(m_Words[WordHomonymNum]);
                AddNode(WordHomonymNum, POS);

                homs_count[i] += 1;
                if (homs_count[i] == m_Words.GetWord(WordHomonymNum).GetRusHomonymsCount())
                    return true;
            }
    }
    return false;
}

void CClause::SetFirstWord(int first)
{
    bool bChangeTypesAndConjs = false;
    if (first > First)
        bChangeTypesAndConjs = true;

    if (m_ClauseWords.size() > 0) {
        for (size_t i = 0; i + 1 < m_ClauseWords.size();) {
            const CWord& w = m_Words.GetWord(m_ClauseWords[i]);
            if (w.GetSourcePair().LastWord() < first)
                m_ClauseWords.erase(m_ClauseWords.begin() + i);
            else
                break;
        }
    }

    CWordsPair::SetFirstWord(first);
    if (bChangeTypesAndConjs)
        AdjustTypesAndConjs();
}

void CClause::SetLastWord(int last)
{
    bool bChangeTypesAndConjs = false;
    if (last < Last)
        bChangeTypesAndConjs = true;

    for (int i = (int)m_ClauseWords.size() - 1; i >= 0; --i) {
        const CWord& w = m_Words.GetWord(m_ClauseWords[i]);
        if (w.GetSourcePair().FirstWord() > last)
            m_ClauseWords.erase(m_ClauseWords.begin() + i);
        else
            break;
    }

    CWordsPair::SetLastWord(last);
    if (bChangeTypesAndConjs) {
        AdjustTypesAndConjs();
        RunSynAn();
    }
}

void CClause::AddNode(SWordHomonymNum WordHomonymNum, const TGramBitSet& POS)
{

    SClauseType ClauseType;
    ClauseType.m_NodeNum = WordHomonymNum;
    ClauseType.m_NodePOS = POS;

    // lazyfrog: looking for "ne" [translit] before the word-node
    // remember this fact if any
    if (WordHomonymNum.m_WordNum > First) {
        // checking makes sense only if there is at least one word before the node
        DECLARE_STATIC_RUS_WORD(kNe, "не");
        if (m_Words[ WordHomonymNum.m_WordNum - 1 ].GetLowerText() == kNe)
            ClauseType.m_bNegation = true;
    }

    // cannot be two equal types !!!
    if (std::find(m_Types.begin(), m_Types.end(), ClauseType) != m_Types.end()) {
        //ASSERT(false);
        return;
    }
    m_Types.push_back(ClauseType);
}

void CClause::PrintNodeInfo(TOutputStream& stream, ECharset encoding) const
{
    for (size_t i = 0; i < m_Types.size(); i++) {
        stream << "  ";
        if (m_Types[i].m_NodeNum.IsValid())
            stream << "node: \"" << WideToChar(m_Words[m_Types[i].m_NodeNum].GetLemma(), encoding) << "\" ";

        stream << "POS: \"";
        CHomonym::PrintGrammems(m_Types[i].m_NodePOS, stream, encoding);
        stream << "\" type: \"" << WideToChar(GlobalGramInfo->ClauseType2Str(m_Types[i].m_Type), encoding) << "\" ";

        if (m_Types[i].m_Type == WantsVal)
            stream << "(" << WideToChar(GlobalGramInfo->Modality2Str(m_Types[i].m_Modality), encoding) << ")";

        stream << Endl;
        for (size_t j = 0; j < m_Types[i].m_Vals.size(); j++) {
            if (m_Types[i].m_Vals[j].m_Actant.IsValid()) {
                const CHomonym& hom = m_Words[m_Types[i].m_Vals[j].m_Actant];
                stream << "             valency: \"" << WideToChar(hom.GetLemma(), encoding) << " \"" << Endl;
            }
        }
    }

}

void CClause::PrintConjsInfo(TOutputStream& stream, ECharset encoding) const
{
    DECLARE_STATIC_RUS_WORD(Header, " Союзы:");
    if (m_Conjs.size()) {
        stream << WideToChar(Header, encoding) << Endl;
        for (size_t i = 0; i < m_Conjs.size(); i++) {
            const CHomonym& h = m_Words[m_Conjs[i].m_WordNum];
            stream << "  ";
            stream << WideToChar(h.GetLemma(), encoding) << "\" POS: \"";
            CHomonym::PrintGrammems(MakeMask(m_Conjs[i].m_Type), stream, encoding);
            stream << "\"" << Endl;
        }
    }
}

void CClause::Print(TOutputStream& stream, ECharset encoding) const
{
    for (size_t i = 0; i < m_Types.size(); i++) {
        stream << "  ";
        if (m_Types[i].m_NodeNum.IsValid())
            stream << "node: \"" << WideToChar(m_Words[m_Types[i].m_NodeNum].GetLemma(), encoding) << "\" ";

        stream << "POS: \"";
        CHomonym::PrintGrammems(m_Types[i].m_NodePOS, stream, encoding);
        stream << "\" type: \"" << WideToChar(GlobalGramInfo->ClauseType2Str(m_Types[i].m_Type), encoding) << "\" ";

        if (m_Types[i].m_Type == WantsVal)
            stream << "(" << WideToChar(GlobalGramInfo->Modality2Str(m_Types[i].m_Modality), encoding) << ")";

        stream << Endl;

        for (size_t j = 0; j < m_Types[i].m_Vals.size(); ++j)
            if (m_Types[i].m_Vals[j].m_Actant.IsValid()) {
                const CHomonym& hom = m_Words[m_Types[i].m_Vals[j].m_Actant];
                stream << "     valency: \"" << WideToChar(hom.GetLemma(), encoding) << " \"" << Endl;
            }
    }

    DECLARE_STATIC_RUS_WORD(Text1, " Союзы:");
    DECLARE_STATIC_RUS_WORD(Text2, "     Союз: \"");

    if (m_Conjs.size()) {
        stream << WideToChar(Text1, encoding) << Endl;
        for (size_t i = 0; i < m_Conjs.size(); i++) {
            const CHomonym& h = m_Words[m_Conjs[i].m_WordNum];
            stream << "  ";
            stream << WideToChar(Text2, encoding) << WideToChar(h.GetLemma(), encoding) << "\" POS: \"";
            if (m_Conjs[i].m_Type == gInvalid)
                stream << "gInvalid";
                    // Союзы "если", "якобы" не имеют граммем, отличных от gConjunction
                    // В результате GetConjType проставляет им gInvalid.
                    // Чтобы не менять остальную логику, меняю только печать.
            else
                CHomonym::PrintGrammems(TGramBitSet(m_Conjs[i].m_Type), stream, encoding);
            stream << "\"" << Endl;
        }
    }
    stream << Endl << Endl;
}

bool CClause::HasType(EClauseType Type) const
{
    return (HasType_i(Type) != -1);
}

bool CClause::HasTypeWithPOS(const TGramBitSet& POS)
{
        return HasTypeWithPOS_i(POS) != -1;
}

int CClause::HasTypeWithPOS_i(const TGramBitSet& POS)
{
    for (size_t i = 0; i < m_Types.size(); ++i)
        if (m_Types[i].HasPOS(POS))
            return i;
    return -1;
}

bool CClause::HasTypeWithPOS(TGrammar POS)
{
        return HasTypeWithPOS_i(POS) != -1;
}

int CClause::HasTypeWithPOS_i(TGrammar POS)
{
    for (size_t i = 0; i < m_Types.size(); ++i)
        if (m_Types[i].HasPOS(POS))
            return i;
    return -1;
}

bool CClause::HasOneTypeWithPOSNot(const TGramBitSet& POS)
{
    return HasOneTypeWithPOSNot_i(POS) != -1;
}

int CClause::HasOneTypeWithPOSNot_i(const TGramBitSet& POS)
{
    for (size_t i = 0; i < m_Types.size(); ++i)
        if (!m_Types[i].HasPOS(POS))
            return i;
    return -1;
}

bool CClause::HasOneTypeNot(EClauseType Type)
{
    return HasOneTypeNot_i(Type) != -1;
}

int CClause::HasOneTypeNot_i(EClauseType Type)
{
    for (size_t i = 0; i < m_Types.size(); i++) {
        if (m_Types[i].m_Type != Type)
            return i;
    }
    return -1;
}

int CClause::HasEmptyType()
{
    int iType;

    if ((iType = HasType_i(Empty)) != -1)
        return iType;

    if ((iType = HasTypeWithPOS_i(gInfinitive)) != -1) {
        if (HasSubConj()) {
            if (HasType_i(NeedNode) != -1)
                return iType;
        } else
            return iType;
    }

    return -1;
}

int CClause::HasType_i(EClauseType Type) const
{
    for (size_t i = 0; i < m_Types.size(); i++) {
        if (m_Types[i].m_Type == Type)
            return i;
    }
    return -1;
}

bool CClause::IsSubClause()
{
    if (HasSubConj())
        return true;

    if (HasTypeWithPOS_i(gParticiple) != -1)
        return true;

    if (HasTypeWithPOS_i(gGerund) != -1)
        return true;

    return false;

}

void CClause::DeleteType(int iType)
{
    YASSERT(iType >= 0 && (size_t)iType < m_Types.size());

    if (m_Types.size() == 1) {
        SClauseType empty_type;
        empty_type.m_Type = Empty;
        m_Types.push_back(empty_type);
    }

    m_Types.erase(m_Types.begin() + iType);
}

bool CClause::HasColon()
{
    const CWord& word = m_Words[LastWord()];
    return word.IsColon();
}

bool CClause::HasDash()
{
    const CWord& word = m_Words[LastWord()];
    return word.IsDash();
}

bool CClause::HasPunctAtTheEnd()
{
    if (m_ClauseWords.size() == 0)
        return false;
    const CWord& word = m_Words.GetWord(m_ClauseWords[m_ClauseWords.size() - 1]);
    return word.IsPunct();
}

bool CClause::HasPunctAtTheVeryEnd()
{
    const CWord& word = m_Words[LastWord()];
    return word.IsPunct();
}

void  CClause::PutLastWordPunct()
{
    if (Size() == 0)
        return;
    const CWord& word = m_Words[LastWord()];
    word.PutIsPunct(true);
}

void CClause::AddClauseWords(CClause& clause, bool bAtTheEnd)
{
    if (bAtTheEnd) {
        m_ClauseWords.insert(m_ClauseWords.end(), clause.m_ClauseWords.begin(), clause.m_ClauseWords.end());
        SetLastWord(clause.LastWord());
    } else {
        m_ClauseWords.insert(m_ClauseWords.begin(), clause.m_ClauseWords.begin(), clause.m_ClauseWords.end());
        SetFirstWord(clause.FirstWord());
    }
}

void CClause::PrintClauseWord(Wtroka& strClauseWords)
{
    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        strClauseWords += m_Words.GetWord(m_ClauseWords[i]).GetOriginalText();
        strClauseWords += ' ';
    }
}

bool CClause::IsDisrupted()
{
    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);
        if (i > 0) {
            const CWord& prev_word = m_Words.GetWord(m_ClauseWords[i-1]);
            if (prev_word.GetSourcePair().LastWord() != (word.GetSourcePair().FirstWord() - 1))
                return true;
        }
    }

    return false;
}

Wtroka CClause::PrintTypeForJava(int iType)
{
    Wtroka res;
    res += '[';
    res += GlobalGramInfo->ClauseType2Str(m_Types[iType].m_Type);
    res += ']';
    if (m_Types[iType].m_NodeNum.IsValid()) {
        res += '(';
        res += m_Words[m_Types[iType].m_NodeNum].GetLemma();
        res += ')';
    }
    return res;
}

void CClause::PrintType(TOutputStream& stream, int iType, ECharset encoding)
{

    stream << "(Bad weight - " << m_BadWeight<< "(" << m_RuleBadWeight << ";" << GetBadWeightForCommas()<< ")" << ")";

    SClauseType& clType = m_Types[iType];

    if (clType.m_NodeNum.IsValid())
        stream << "node: \"" << WideToChar(m_Words[clType.m_NodeNum].GetLemma(), encoding) << "\" ";

    stream << "POS: \"";
    CHomonym::PrintGrammems(clType.m_NodePOS, stream, encoding);
    stream << "\" type: \"" << WideToChar(GlobalGramInfo->ClauseType2Str(clType.m_Type), encoding) << "\" ";

    if (clType.m_Type == WantsVal)
        stream << "(" << WideToChar(GlobalGramInfo->Modality2Str(clType.m_Modality), encoding) << ")";
    stream << Endl;

    for (size_t j = 0; j < clType.m_Vals.size(); j++) {
        if (clType.m_Vals[j].m_Actant.IsValid()) {
            const CHomonym& hom = m_Words[clType.m_Vals[j].m_Actant];
            stream << "     valency: \"" << WideToChar(hom.GetLemma(), encoding) << " \"" << Endl;
        }
    }

    for (size_t k = 0; k < m_Conjs.size(); k++) {
        SConjType& conj = m_Conjs[k];
        stream << "Conj: " << "\"" << WideToChar(m_Words[conj.m_WordNum].GetLemma(), encoding) << "\" ";
        stream << "ConjPOS: " << "\"";
        CHomonym::PrintGrammems(TGramBitSet(conj.m_Type), stream, encoding);
        stream << "\"";
    }
}

void CClause::PrintDisruptedWords(yvector< TPair<Wtroka, TPair<int, int> > >& words2pairs)
{
    if (m_ClauseWords.size() == 0)
        return;

    Wtroka strClauseWords;

    int left_border = m_Words.GetWord(m_ClauseWords[0]).GetSourcePair().FirstWord();

    for (size_t i = 0; i < m_ClauseWords.size(); i++) {
        const CWord& word = m_Words.GetWord(m_ClauseWords[i]);
        if (i > 0) {
            const CWord& prev_word = m_Words.GetWord(m_ClauseWords[i-1]);
            if (prev_word.GetSourcePair().LastWord() != (word.GetSourcePair().FirstWord() -1)) {
                TPair<Wtroka, TPair<int, int> > word2pair;
                word2pair.first = strClauseWords;
                word2pair.second.first = left_border;
                word2pair.second.second = prev_word.GetSourcePair().LastWord();
                strClauseWords.clear();
                words2pairs.push_back(word2pair);
                left_border = word.GetSourcePair().FirstWord();
            }
        }

        strClauseWords += m_Words.GetWord(m_ClauseWords[i]).GetOriginalText();
        strClauseWords += ' ';
    }

    TPair<Wtroka, TPair<int, int> > word2pair;
    word2pair.first = strClauseWords;
    word2pair.second.first = left_border;
    word2pair.second.second = m_Words.GetWord(m_ClauseWords[(int)m_ClauseWords.size() - 1]).GetSourcePair().LastWord();
    words2pairs.push_back(word2pair);
}

int _FindClauseWordNum(yvector<CWord*>& sentWords, const CWordsPair& wp)
{
    for (size_t i = 0; i < sentWords.size(); i++)
        if (sentWords[i]->GetSourcePair() == wp)
            return i;
    return -1;
}

CClauseSyn::CClauseSyn(CWordVector& Words)
    : CClause(Words)
    , m_SynChains(m_Words, GlobalDictsHolder->m_SynAnGrammar, m_ClauseWords)
{

}
CClauseSyn::~CClauseSyn()
{
    m_SynChains.Free();
}

void CClauseSyn::GetAllGroupsByMainWordWithGrammems(yvector<CGroup*>& groups, int iClauseWord, int iH, const TGramBitSet& grammems)
{
    for (size_t i = 0; i < m_SynChains.m_SynVariants.size(); i++) {
        CSynChainVariant& synVar = m_SynChains.m_SynVariants[i];
        for (size_t j = 0; j < synVar.m_SynItems.size(); j++) {
            if (synVar.m_SynItems[j]->Contains(iClauseWord)) {
                CGroup* pMainGroup = synVar.m_SynItems[j]->GetMainGroup();
                if (pMainGroup->IsPrimitive())
                    groups.push_back(pMainGroup);
                else {
                    CPrimitiveGroup* pPrimGroup = pMainGroup->GetMainPrimitiveGroup();
                    if ((pPrimGroup->FirstWord() == iClauseWord) &&
                        pPrimGroup->ContainsHomonym(iH)) {
                        if (pMainGroup->GetGrammems().HasAll(grammems))
                            groups.push_back(pMainGroup);
                    }
                }
            }
        }
    }
}

void CClauseSyn::RunSynAn()
{
    m_SynChains.Free();
    m_SynChains.Run();
}

void CClauseSyn::Copy(const CClause& _clause)
{
    CClause::Copy(_clause);
    CClauseSyn& clause = (CClauseSyn&)_clause;
    m_SynChains.CopyFrom(clause.m_SynChains);

}

bool CClauseSyn::CanBeNode(int iClauseWord, int iH)
{
    for (size_t i = 0; i < m_SynChains.m_SynVariants.size(); i++) {
        CSynChainVariant& synVar = m_SynChains.m_SynVariants[i];
        for (size_t j = 0; j < synVar.m_SynItems.size(); j++) {
            if (synVar.m_SynItems[j]->Contains(iClauseWord)) {
                CGroup* pMainGroup = synVar.m_SynItems[j]->GetMainGroup();
                if (pMainGroup->IsPrimitive())
                    return true;
                else {
                    if (CGramInfo::IsNounGroup(pMainGroup->GetActiveSymbol()))
                        continue;
                    if (CGramInfo::IsVerbGroup(pMainGroup->GetActiveSymbol())) {
                        CPrimitiveGroup* pPrimGroup = pMainGroup->GetMainPrimitiveGroup();
                        if ((pPrimGroup->FirstWord() == iClauseWord) &&
                            (pPrimGroup->ContainsHomonym(iH))) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool CClauseSyn::FindNounGroupAsSubject(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc)
{
    subj.Reset();

    const CHomonym& predHom  = m_Words[pred];

    int iPredClauseWord = FindClauseWord(pred);

    int iBadWeightRes = 0xFFFF;

    for (size_t i = 0; i < m_SynChains.m_SynVariants.size(); i++) {
        CSynChainVariant& synVar = m_SynChains.m_SynVariants[i];
        for (size_t j = 0; j < synVar.m_SynItems.size(); j++) {
            CGroup* pMainGroup = synVar.m_SynItems[j]->GetMainGroup();
            if (!pMainGroup->IsPrimitive() && CGramInfo::IsNounGroupForSubject(pMainGroup->GetActiveSymbol())) {
                int iBadWeight = 0;
                const TGramBitSet& gr_grammems = pMainGroup->GetGrammems().All();
                if (gr_grammems.Has(gNominative) &&
                    NGleiche::Gleiche(gr_grammems, predHom.Grammems.All(), CoordFunc)) {
                    if (iPredClauseWord < pMainGroup->FirstWord())
                        iBadWeight++;
                    if (gr_grammems.HasAny(NSpike::AllCases & ~TGramBitSet(gNominative)))
                        iBadWeight++;
                    if (iBadWeight <= iBadWeightRes) {
                        int iClauseWord, iH;
                        pMainGroup->GetMainClauseWord(iClauseWord, iH);
                        subj = m_ClauseWords[iClauseWord];
                        subj.m_HomNum = iH;
                    }
                }
            }
        }
    }
    return subj.IsValid();
}

