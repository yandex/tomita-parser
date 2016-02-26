#include "text.h"
#include "morph.h"

#include <FactExtract/Parser/common/pathhelper.h>

#include <util/generic/cast.h>

void CText::FreeData()
{
    for (size_t i=0; i<m_vecSentence.size(); i++) {
        m_vecSentence[i]->FreeData();
        delete m_vecSentence[i];
        m_vecSentence[i] = NULL;
    }
    m_vecSentence.clear();
    CTextBase::FreeData();
    CTextBase::ResetPrimitives();
}

void CText::Process(const Wtroka& text, const yset<SLinkRefMarkUp>& markUps)
{
    size_t iSentCountBefore = GetSentenceCount();
    CText::Proceed(text, 0, -1);
    size_t iSentCount = GetSentenceCount();
    yset<SLinkRefMarkUp>::const_iterator it = markUps.begin();
    for (size_t i = iSentCountBefore; i < iSentCount; i++) {
        CSentence* pSent = GetSentenceRus(i);
        while (it != markUps.end() && pSent->AddMarkUp(*it))
            it++;
    }
}

void CText::ReplaceSimilarLetters(CPrimGroup &group)
{
    size_t iAltLetCount = 0;
    size_t iRusLetCount = 0;
    for (size_t i = 0; i < group.m_prim.size(); ++i) {
        if (group.m_prim[i].m_typ != Word && group.m_prim[i].m_typ != AltWord)
            return;
        if (group.m_prim[i].m_typ == Word)
            iRusLetCount += group.m_prim[i].m_len;
        if (group.m_prim[i].m_typ == AltWord)
            iAltLetCount += group.m_prim[i].m_len;
    }
    if (iAltLetCount == 0 || iRusLetCount == 0)
        return;
    if (!ReplaceSimilarLetters(group, AltWord, Word, LANG_RUS))
        ReplaceSimilarLetters(group, Word, AltWord, LANG_ENG);
}

bool CText::ReplaceSimilarLetters(CPrimGroup &group, ETypeOfPrim from, ETypeOfPrim to, docLanguage lang)
{
    Wtroka strText, derenyxed;
    bool res = false;
    for (size_t i = 0; i < group.m_prim.size(); ++i) {
        CPrimitive& prim = group.m_prim[i];
        Wtroka text = GetText(prim);
        if (prim.m_typ == from && NLemmer::GetAlphaRules(lang)->Derenyx(text).IsChanged.any())
            res = true;
        strText += text;
    }
    if (res) {
        group.m_str = strText;
        group.m_gtyp = to;
    }
    return res;
}

Wtroka CText::GetText(const CPrimitive &prim) const
{
    Wtroka res = CTextBase::GetText(prim);
    //TMorph::ReplaceYo(res);
    return res;
}

Wtroka CText::GetText(CPrimGroup& group) const
{
    if (!group.m_str.empty())
        return group.m_str;
    BuildPrimGroupText(group, group.m_str);
    //TMorph::ReplaceYo(group.m_str);
    return group.m_str;
}

Wtroka CText::GetClearText(const CPrimGroup &group) const
{
    Wtroka res = CTextBase::GetClearText(group);
    //TMorph::ReplaceYo(res);
    return res;
}

//////////////////////////////////////////////////////////////////////////////
// createSentence

void CText::AddSentence(CSentenceBase* pSent)
{
    CSentence *pSentRus = (CSentence*)pSent;
    if (pSentRus)
        m_vecSentence.push_back(pSentRus);
}
CWordBase* CText::createWord(CPrimGroup &group, const Wtroka& txt)
{
    CWord* res = new CWord(group, txt, IgnoreUpperCase());
    res->PutIsUp(hasFirstUpper(group));
    return res;
}

CSentenceBase* CText::CreateSentence()
{
    return new CSentence();
}

bool CText::isUnusefulPostfix(Wtroka text)
{
    if (text.size() != 2)
        return false;

    DECLARE_STATIC_RUS_WORD(KA, "ка");
    DECLARE_STATIC_RUS_WORD(TO, "то");
    TMorph::ToLower(text);
    return text == KA || text == TO;
}

void CText::markLastSentParEnd()
{
    if (m_vecSentence.size()>0)
        m_vecSentence[m_vecSentence.size()-1]->m_bPBreak = true;
}

void CText::analyzeSentences()
{
    for (size_t i = 0; i < m_vecSentence.size(); ++i)
        m_vecSentence[i]->BuildDates();

    findNames();

    for (size_t i = 0; i < m_vecSentence.size(); ++i)
        m_vecSentence[i]->BuildNumbers();
}


size_t CText::GetSentenceCount() const
{
    return m_vecSentence.size();
}

CSentence* CText::GetSentenceRus(size_t i)
{
    return m_vecSentence[i];
}

CSentenceBase* CText::GetSentence(size_t i)
{
    return m_vecSentence[i];
}

static bool AppendParsedNumber(const TWtringBuf& str, yvector<double>& numbers) {
    try {
        double val = FromString<double>(WideToChar(str));
        numbers.push_back(val);
        return true;
    } catch (yexception&) {
        return false;
    }
}

//words like "47-e mesto", "25%-go"
bool CText::processHyphenOrComplexGroupAsNumberAndFlex(CWord* pWord, CPrimGroup& group, CNumber& number)
{
    // first part should be a digit, last - a word.
    if (group.m_prim.size() < 2 || group.m_prim[0].m_typ != Digit || group.m_prim.back().m_typ != Word)
        return false;

    if (hasUpper(group.m_prim.back()))
        return false;

    if (group.m_prim.size() == 3) {
        // 12-iy
        if (!isPunctHyphen(group.m_prim[1]))
            return false;
    } else if (group.m_prim.size() == 4) {
        // 40%-niy
        if (!(group.m_prim[1].m_typ == Symbol && GetText(group.m_prim[1]) == Wtroka::FromAscii("%") &&
              isPunctHyphen(group.m_prim[2])))
            return false;
    } else if (group.m_prim.size() != 2 || group.m_gtyp != Complex) // 5ya
        return false;

    Wtroka strFlex = GetText(group.m_prim.back());
    Wtroka strDig = GetText(group.m_prim[0]);

    if (strDig.size() == 0 || strFlex.size() <= 0 || strFlex.size() > 4)
        return false;

    const TGrammarBunch* grammems = TMorph::GetNumeralGrammemsByFlex(strDig, strFlex);
    if (grammems == NULL)
        return false;
    DECLARE_STATIC_RUS_WORD(num_flex, "-й");
    pWord->GetRusHomonym(0).Init(strDig + num_flex, *grammems, false);

    if (!AppendParsedNumber(strDig, number.m_Numbers))
        return false;

    number.m_bSingle = true;
    return true;
}

bool CText::ProcessTurkishNumberWithPeriod(CWord* pWord, CPrimGroup& group, CNumber& number) {

    if (TMorph::GetMainLanguage() != LANG_TUR)
        return false;

    // [Digit,"."] pattern
    if (group.m_prim.size() != 2 || group.m_prim[0].m_typ != Digit || !isPunctDot(group.m_prim[1]))
        return false;

    Wtroka strDig = GetText(group.m_prim[0]);
    if (strDig.empty())
        return false;

    if (!AppendParsedNumber(strDig, number.m_Numbers))
        return false;
    number.m_bSingle = true;

    strDig.append('.');
    CHomonym& hom = pWord->GetRusHomonym(0);
    hom.InitText(strDig);
    hom.SetGrammems(TGramBitSet(gAdjNumeral));

    return true;
}

//words like "3-4-dnevniy"
void CText::processHyphenOrComplexGroupAsNumber(CWord* pWord, CPrimGroup& group, CNumber& number)
{
    if (processHyphenOrComplexGroupAsNumberAndFlex(pWord, group, number))
        return;
    if (ProcessTurkishNumberWithPeriod(pWord, group, number))
        return;

    bool bDigitExpected = true;
    for (size_t i = 0; i < group.m_prim.size(); i++) {
        if ((group.m_prim[i].m_typ == Punct) ||
            (group.m_prim[i].m_typ == NewLine))
            continue;
        if (bDigitExpected && (group.m_prim[i].m_typ != Digit) && (i == 0))
            return;
        if (group.m_prim[i].m_typ == Word)
            bDigitExpected = false;
        if (!bDigitExpected && group.m_prim[i].m_typ != Word)
            return;
    }
    yvector<double> nums;
    const Wtroka& txt = pWord->GetLowerText();
    for (size_t i = 0; i < pWord->m_variant.size(); i++) {
        Wtroka lemma = pWord->m_variant[i]->GetLemma();
        if (lemma == txt || lemma == g_strDashWord)
            continue;
        if (!AppendParsedNumber(lemma, nums))
            return;
    }
    if (nums.size() == 0)
        return;

    ui64 integer = 0;
    int fractional = 0;

    for (size_t i = 0; i < pWord->GetHomonymsCount(); ++i)
        if (GlobalGramInfo->FindOrdinalNumber(pWord->GetRusHomonym(i).GetLemma(), integer, fractional)) {
            if (fractional != 0)
                return;
            if ((integer != 1000) && (integer != 1000000ULL) &&
                (integer != 1000000000ULL) && (integer != 1000000000000ULL))
                return;
            break;
        }

    if (integer == 0)
        return;

    for (size_t i = 0; i < nums.size(); i++)
        nums[i] *= integer;

    number.m_Numbers = nums;
    number.m_bSingle = true;
}

void CText::processAsNumber(CWordBase* _pWord, CPrimGroup& group, CNumber& number)
{
    CWord* pWord = CheckedCast<CWord*>(_pWord);
    ui64 integer;
    int fractional;
    switch (group.m_gtyp) {
        case Word:
        case AbbDig:
        case AbbEos:
        case Abbrev:
            {
                for (size_t i = 0; i < pWord->GetHomonymsCount(); ++i)
                    if (GlobalGramInfo->FindCardinalNumber(pWord->GetRusHomonym(i).GetLemma(), integer, fractional)) {
                        number.m_Numbers.push_back((double)integer + (double)fractional/10.0);
                        return;
                    }
                break;
            }
        case Hyphen:
        case DivWord:
        case HypDiv:
        case Complex:
            {
                processHyphenOrComplexGroupAsNumber(pWord, group, number);
                break;
            }
        default:
            break;
    }
}

bool CText::IsPOSAndGrammems(const Wtroka& w, TGrammar POS, const TGramBitSet& grammems)
{
    Wtroka ss = w;
    TMorph::ToUpper(ss);
    ProceedPrimGroups(ss, 0, ss.size());
    if (m_vecPrimGroup.size() != 1)
        return false;
    m_ProcessingTarget = ForSearching;
    Wtroka tmp = GetText(m_vecPrimGroup[0]);

    THolder<CWord> pWord((CWord*)createWord(m_vecPrimGroup[0], tmp));
    CNumber num;
    applyMorphToGroup(pWord.Get(), m_vecPrimGroup[0], num);

    for (CWord::SHomIt it = pWord->IterHomonyms(); it.Ok(); ++it)
        if (it->HasPOS(POS) && it->Grammems.HasFormWith(grammems))
            return true;
    return false;
}

bool CText::IsSurname(const Wtroka& pText)
{
        Wtroka ss = pText;
        TMorph::ToUpper(ss);
        ProceedPrimGroups(ss, 0, ss.size());
        if (m_vecPrimGroup.size() != 1)
            return false;
        m_ProcessingTarget = ForSearching;
        Wtroka tmp = GetText(m_vecPrimGroup[0]);

        THolder<CWord> pWord((CWord*)createWord(m_vecPrimGroup[0], tmp));
        CNumber num;
        applyMorphToGroup(pWord.Get(), m_vecPrimGroup[0], num);
        pWord->m_bUp = true;
        pWord->InitNameType();
        int iH = pWord->IsName_i(Surname);
        if (iH != -1) {
            const CHomonym& h = pWord->GetRusHomonym(iH);
            static const TGramBitSet mask(gSingular, gNominative);
            if (h.Grammems.HasFormWith(mask))
                return true;
        }
        return false;
}

CSentence* CText::BuildOneSentence(const Wtroka& pText)
{
    Wtroka strText = pText;
    SubstGlobal(strText, ASCIIToWide("-"), ASCIIToWide(" "));

    ProceedPrimGroups(strText, 0, strText.size());
    CSentence* pRes = (CSentence*)createSentence(0, m_vecPrimGroup.size());
    ResetPrimitives();
    return pRes;
}

bool CText::HasSuspiciousPatronymic(CFioWordSequence& fio, CSentence* pSent)
{
    bool bSuspiciousPatronymic = false;
    const CWord& w = *(pSent->m_words[fio.m_NameMembers[Patronomyc].m_WordNum]);
    if (w.IsName(Surname)) {
        if (fio.m_NameMembers[Surname].m_WordNum != fio.LastWord()) {
            if (fio.m_NameMembers[Surname].m_WordNum == fio.FirstWord()) {
                bool bL= false, bR = false;
                if (fio.FirstWord() == 0)
                    bL = true;
                else {
                    CWord& wf = *(pSent->m_words[fio.FirstWord() - 1]);
                    if (wf.IsPunct() || wf.HasConjHomonym())
                        bL = true;
                }
                if (fio.LastWord() == (int)pSent->m_words.size() - 1)
                    bR = true;
                else {
                    CWord& wl = *(pSent->m_words[fio.LastWord() + 1]);
                    if (wl.IsPunct() || wl.HasConjHomonym())
                        bR = true;
                }
                if (!bR || !bL)
                    bSuspiciousPatronymic = true;
            } else
                bSuspiciousPatronymic = true;
        }
    } else if (!w.GetRusHomonym(fio.m_NameMembers[Patronomyc].m_HomNum).IsDictionary()) {
        fio.m_Fio.m_bFoundPatronymic = false;
        bSuspiciousPatronymic = true;
    }

    return bSuspiciousPatronymic;
}

void CText::AddToSurnameBeginings(yset<SSurnameBegining>& surnameBeginings, const Wtroka& s)
{
    if (s.size() >= 2) {
        int ii = TMorph::MaxLenFlex(s);
        yset<SSurnameBegining>::iterator it = surnameBeginings.find(s.substr(0, 2));
        if (it == surnameBeginings.end()) {
            SSurnameBegining SurnameBegining;
            SurnameBegining.m_str2LetterBegining = s.substr(0, 2);
            SurnameBegining.m_MaxLenFlexBegining.push_back(s.substr(0, s.size() - ii));
            surnameBeginings.insert(SurnameBegining);
        } else
            it->m_MaxLenFlexBegining.push_back(s.substr(0, s.size() - ii));
    }
}

void CText::SetCurSent(int)
{
}

void CText::findNames()
{
        yset<SSurnameBegining> surnameBeginings;

        CNameClusterBuilder NameClusterBuilder;
        NameClusterBuilder.m_bResolveContradictions = HasToResolveContradictionsFios();

        yvector< TPair<int,int> > potentialSurnames;

        size_t iFiosCount = 0;
        for (size_t i=0; i < m_vecSentence.size(); i++) {
            CSentence* ptr = (CSentence*)m_vecSentence[i];

            TPair<SFullFIO, CFIOOccurence> Name1 = MakePair(SFullFIO(), CFIOOccurence());
            yvector< CFioWordSequence> foundNames;
            yvector<int> potentialSurnamesInOnesent;

            SetCurSent(i);
            ptr->FindNames(foundNames, potentialSurnamesInOnesent);
            for (size_t j = 0; j < foundNames.size(); j++) {
                CFIOOccurenceInText group;
                (CFIOOccurence&)group = (CFIOOccurence&)foundNames[j];
                group.PutSentNum(i);
                bool bSuspiciousPatronymic = false;
                for (int l = 0; l < NameTypeCount; l++) {
                    if ((l == (ENameType)Patronomyc) && foundNames[j].m_NameMembers[l].IsValid() && !foundNames[j].m_Fio.m_bFoundSurname) {
                        bSuspiciousPatronymic = HasSuspiciousPatronymic(foundNames[j], ptr);
                    }
                    group.m_NameMembers[l] = foundNames[j].m_NameMembers[l];
                }

                const SFullFIO& fio = foundNames[j].m_Fio;

                if (!fio.m_strSurname.empty() && !fio.m_bFoundSurname)
                    AddToSurnameBeginings(surnameBeginings, fio.m_strSurname);
                //also add beginnings of patronymics which could possibly be surnames
                if (bSuspiciousPatronymic && !fio.m_strPatronomyc.empty())
                    AddToSurnameBeginings(surnameBeginings, fio.m_strPatronomyc);

                NameClusterBuilder.AddFIO(foundNames[j].m_Fio, group, bSuspiciousPatronymic, false, 0);
                iFiosCount++;
            }
            for (size_t j = 0; j < potentialSurnamesInOnesent.size(); j++)
                potentialSurnames.push_back(MakePair(i,potentialSurnamesInOnesent[j]));

        }

        if (surnameBeginings.size() > 0) {
            for (size_t j = 0; j < potentialSurnames.size(); j++) {
                CSentence *ptr = (CSentence *)m_vecSentence[potentialSurnames[j].first];

                SFullFIO fio;
                int iW = potentialSurnames[j].second;
                fio.m_strSurname = ptr->m_words[iW]->GetLowerText();
                if (fio.m_strSurname.size() >= 2) {
                    yset<SSurnameBegining>::iterator it = surnameBeginings.find(SSurnameBegining(fio.m_strSurname.substr(0, 2)));

                    if (it == surnameBeginings.end())
                        continue;
                    size_t i = 0;
                    for (; i < it->m_MaxLenFlexBegining.size(); i++) {
                        size_t iLen = it->m_MaxLenFlexBegining[i].size();
                        if (fio.m_strSurname.size() >= iLen)
                            if (TWtringBuf(fio.m_strSurname).SubStr(0, iLen) == TWtringBuf(it->m_MaxLenFlexBegining[i]))
                                break;
                    }
                    if (i >= it->m_MaxLenFlexBegining.size())
                        continue;
                } else
                    continue;

                fio.m_bFoundSurname = false;

                CFIOOccurenceInText group;
                group.SetPair(iW, iW);
                group.PutSentNum(potentialSurnames[j].first);
                group.m_NameMembers[Surname] = SWordHomonymNum(iW,0);
                group.SetMainWord(group.m_NameMembers[Surname]);

                NameClusterBuilder.AddPotentialSurname(fio, group);
            }
        }

        if (GetMaxNamesCount() > 0) { // we have to try name clustering
            if (iFiosCount < GetMaxNamesCount())
                NameClusterBuilder.Run();
            else
                Cerr << "Too many names found (" << iFiosCount << "), name clustering will not be performed.\n"
                     << "You may try to increase the current limit (" << GetMaxNamesCount() << ")." << Endl;

            for (int i = 0; i < FIOTypeCount; i++) {
                for (size_t j = 0; j < NameClusterBuilder.m_FIOClusterVectors[(EFIOType)i].size(); j++) {
                    CNameCluster& nameCluster = NameClusterBuilder.m_FIOClusterVectors[(EFIOType)i][j];
                    yset<Wtroka> fioStrings;
                    nameCluster.m_FullName.BuildFIOStringRepresentations(fioStrings);
                    yset<CFIOOccurenceInText>::iterator name_variants_it = nameCluster.m_NameVariants.begin();
                    for (; name_variants_it != nameCluster.m_NameVariants.end(); name_variants_it++) {
                        const CFIOOccurenceInText& wordsPairInText = *name_variants_it;
                        CSentence* pSent = m_vecSentence[wordsPairInText.GetSentNum()];
                        pSent->AddFIO(fioStrings, wordsPairInText, nameCluster.m_FullName.m_bFoundSurname);
                        pSent->AddFIOWS(wordsPairInText, nameCluster.GetFullName(), nameCluster.m_NameVariants.size() - 1);
                    }
                }
            }
        }
}

// moved from textrusdata.cpp

void CText::loadCharTypeTable()
{
    m_charTypeTable.Reset(TMorph::GetMainLanguage());
}

