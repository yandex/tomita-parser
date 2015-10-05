#include "morph.h"
#include "namefinder.h"

#include <FactExtract/Parser/common/utilit.h>

SNameTemplate g_FirmedNameTemplates[g_FirmedNameTemplatesCount] =
{
    { {FirstName, Patronomyc}, 2, FIOname, true},
    { {InitialName, InitialPatronomyc}, 2, FIOname, true},
    { {FirstName, InitialPatronomyc }, 2, FIOname, true},
    { {FirstName, InitialName, InitialPatronomyc}, 3, FIOname, false}, //Мамай В.И.
    { {FirstName, Patronomyc}, 2, IOname, false},
    { {FirstName, InitialPatronomyc}, 2, IOnameIn, false},
    { {InitialName, InitialPatronomyc}, 2, IOnameInIn, false},
    { {FirstName }, 1, FIname, true },
    { {InitialName }, 1, IFnameIn, true},
    { {FirstName, InitialName}, 2, FInameIn, false},    //Мамай В.
    { {FirstName }, 1, Iname, false},
    { {Surname}, 1, Fname, false},
    { {InitialName }, 1, InameIn, false}
};

CNameFinder::CNameFinder(CWordVector&  words)
    : m_bPriorityToTheRightSurname(true)
    , m_Words(words)
{
}

static void GetAgreedHomonymForms(const CHomonymBase& hom, const TGramBitSet& grammems, NGleiche::TGleicheFunc agr,
                                  TGrammarBunch& res)
{
    const TGrammarBunch& forms = hom.Grammems.Forms();
    for (TGrammarBunch::const_iterator it = forms.begin(); it != forms.end(); ++it)
        if (agr(*it, grammems).any())
            res.insert(*it);
}

//вычисляем полноценные грамеммы для построенного фио на основе
//граммем, вычисленных в CheckGrammInfo
void CNameFinder::AssignGrammems(CFioWordSequence& foundName)
{
    SFullFIO& fio = foundName.m_Fio;

    const SWordHomonymNum& fname_hnum = foundName.m_NameMembers[FirstName];
    const SWordHomonymNum& surname_hnum = foundName.m_NameMembers[Surname];
    TGrammarBunch fname_grammems, surname_grammems;

    if (fname_hnum.IsValid())
        GetAgreedHomonymForms(m_Words[fname_hnum], foundName.GetGrammems().All(), NGleiche::GenderNumberCaseCheck, fname_grammems);

    if (fio.m_bFoundSurname || (surname_hnum.IsValid() && m_Words.GetWord(surname_hnum).HasHomonymWithGrammems(TGramBitSet(gSurname)))) {
        const CHomonym& h = m_Words[surname_hnum];
        if (fio.m_bFoundSurname || h.HasGrammem(gSurname) || h.IsFullAdjective() || h.HasPOS(gAdjNumeral))
            GetAgreedHomonymForms(h, foundName.GetGrammems().All(), NGleiche::GenderNumberCaseCheck, surname_grammems);
    }

    bool bUseFirstName = true;  //prefer first-name by default
    if (fname_grammems.empty())
        bUseFirstName = false;
    else if (!surname_grammems.empty()) {
        bool ind_fname = m_Words[fname_hnum].Grammems.HasAllCases();     //consider indeclinable homonyms
        bool ind_surname = m_Words[surname_hnum].Grammems.HasAllCases();
        if (ind_fname != ind_surname)           //if only one is indeclinable use non-indeclinable
            bUseFirstName = ind_surname;
    }

    foundName.SetGrammems(*(bUseFirstName ? &fname_grammems : &surname_grammems));
    AdjustGrammemsForFIO(foundName.GetGrammems(), fio);
}

//нормализуем женскую форму фамилии если можно
//(не могли сделать раньше, так как хранили лемму фамилии, а она мужская)
//проставляем главное слово и граммемки
void CNameFinder::NormalizeFio(CFioWordSequence& foundName)
{
    if (!foundName.m_Fio.m_bFoundSurname) {
        const TGramBitSet grammems(gNominative, gSingular);
        if (!foundName.m_Fio.m_bSurnameLemma)
            foundName.m_Fio.m_bSurnameLemma = foundName.GetGrammems().HasAll(grammems);
    }

    if (foundName.GetGrammems().Has(gFeminine) &&
        !foundName.GetGrammems().Has(gMasculine) &&
        foundName.m_NameMembers[Surname].IsValid() &&
        m_Words.GetWord(foundName.m_NameMembers[Surname]).HasHomonymWithGrammems(TGramBitSet(gSurname))) {
        //если не сможем предсказать ж.р , то лучше сохранить, то что было в оригинале, чем
        //приписать предсказанную форму, которая всегда м.р (эта делается во время поиска несловарной фамилии,
        //в ф-ции GleichePredictedSurname)
        foundName.m_Fio.m_strSurname = m_Words.GetWord(foundName.m_NameMembers[Surname]).GetText();
        TMorph::ToLower(foundName.m_Fio.m_strSurname);
        Wtroka s = foundName.m_Fio.GetFemaleSurname();
        if (!s.empty())
            foundName.m_Fio.m_strSurname = s;
    }

    if (foundName.GetGrammems().Empty()) {
        if (foundName.m_Fio.m_Genders.Has(gMasculine))
            foundName.GetGrammems().Reset(TGramBitSet(gMasculine, gSingular) | NSpike::AllCases);

        if (foundName.m_Fio.m_Genders.Has(gFeminine))
            foundName.GetGrammems().Reset(TGramBitSet(gFeminine, gSingular) | NSpike::AllCases);
    }

    if (foundName.m_NameMembers[Surname].IsValid())
        foundName.SetMainWord(foundName.m_NameMembers[Surname]);
    else if (foundName.m_NameMembers[FirstName].IsValid())
            foundName.SetMainWord(foundName.m_NameMembers[FirstName]);
    else if (foundName.m_NameMembers[InitialName].IsValid())
            foundName.SetMainWord(foundName.m_NameMembers[InitialName]);

    AssignGrammems(foundName);
}

void CNameFinder::Run(yvector<CFioWordSequence>& foundNames, yvector<int>& potentialSurnames)
{
    int curWord = 0;
    int iLastWordInFIO = -1;

    for (; curWord < (int)m_Words.OriginalWordsCount();) {
        if (!m_Words[curWord].m_bUp || (m_Words[curWord].m_typ == AltWord)) {
            curWord++;
            continue;
        }

        bool bFound = false;
        if (m_Words[curWord].IsNameFoundInDic()) {

            bool bAddedOnce = false;
            for (int i = 0; i < g_FirmedNameTemplatesCount; i++) {
                //считаем, что в обычном тексте не встретится Алесандр А. Петров
                if (i == 5)
                    continue;
                yvector<CFioWordSequence> foundNameVariants;

                int iW = curWord;

                //так как мы ищем сначала фио без фамилии,
                //а фамилию ищем и слева и справа, а потом выбираем лучшую (при прочих равных - правую)
                //то мы должны пропустить фамлилию, так двигаемся по предложению слева направо
                //но если есть омоним имени, то не пропускаем
                if (m_Words[curWord].IsName(Surname) && g_FirmedNameTemplates[i].m_bCheckSurnameFromDic) {
                    int iH = m_Words[curWord].IsName_i(FirstName);
                    if (iH == -1)
                        iH = m_Words[curWord].IsName_i(InitialName);
                    if (iH == -1)
                        iW++;
                    else {
                        CHomonym& pH = m_Words[curWord].GetRusHomonym(iH);
                        if (pH.HasGrammem(gPlural) && !pH.Grammems.IsIndeclinable())        //dont't skip indeclinable first-names too (?)
                            iW++;
                    }
                }

                if (!FindFirmedName(g_FirmedNameTemplates[i], foundNameVariants, iW, iLastWordInFIO))
                    continue;
                bool bBreak = true;
                yvector<CFioWordSequence> addedFioVariants;
                for (size_t k = 0; k < foundNameVariants.size(); k++) {
                    bool bAdd = true;
                    CFioWordSequence& foundName = foundNameVariants[k];
                    bool bWithSurname = false;
                    //раширяем где нужно несловарными фамилиями фио
                    switch (g_FirmedNameTemplates[i].FIOType) {
                        case FIOname:
                        {
                            bWithSurname = true;
                            break;
                        }
                        case IOname:
                        {
                            if (!EnlargeIOname(foundName, iLastWordInFIO))
                                if (bAddedOnce)
                                    bAdd = false;
                            break;
                        }
                        case FIname:
                        {
                            bWithSurname = true;
                            break;
                        }
                        case Fname:
                        {
                            break;
                        }
                        case Iname:
                        {
                            EnlargeIname(foundName, iLastWordInFIO);
                            break;
                        }
                        case IOnameIn:
                        {
                            if (!EnlargeIOnameIn(foundName)) {
                                bBreak = false;
                                bAdd = false;
                                continue;
                            }
                            break;
                        }
                        case IOnameInIn:
                        {
                            EnlargeIOnameInIn(foundName, iLastWordInFIO);
                            if (foundName.Size() == 2)
                                bAdd = false;
                            break;
                        }

                        case IFnameIn:
                        {
                            bWithSurname = true;
                            break;
                        }
                        case InameIn:
                        {
                            EnlargeInameIn(foundName, iLastWordInFIO);
                            if (foundName.Size() == 1)
                                bAdd = false;
                            break;
                        }
                        default:
                            break;
                    }

                    if (!bAdd)
                        continue;

                    iLastWordInFIO = foundName.LastWord();

                    if (bWithSurname) {
                        if ((foundName.FirstWord() == curWord + 1) && (curWord + 1 == iW))
                        //для ситуации ...Петрову Александру Иванову жалко ...
                        //Петрову - пропустили (см. выше), анализируя окружение
                        //Александру выбрали Иванову, но и про Петрову не нужно забывать - добавим это как фио
                        //то что перед Петрову нет имени или инициала, гарантирует порядок просмотра предложения слева направо
                            AddSingleSurname(curWord, foundNames);
                    }

                    NormalizeFio(foundName);
                    if (AddFoundFio(addedFioVariants,foundName))
                        bAddedOnce = true;

                    curWord = foundName.LastWord();
                    bFound = true;
                    if (potentialSurnames.size() != 0)
                        if (potentialSurnames[potentialSurnames.size() - 1] >= foundName.FirstWord())
                            potentialSurnames.erase(potentialSurnames.begin() + (potentialSurnames.size() - 1));

                }
                ChooseVariants(addedFioVariants, foundNames);
                if (bBreak)
                    break;
            }
        }
        if (!bFound && (m_Words[curWord].m_typ != Initial))
            potentialSurnames.push_back(curWord);

        curWord++;
    }

    //отдельно рассмотрим последовательности двух подряд идущих IName и IOName
    //IName может оказаться фамилией типа  Мамай Павел Александрович
    TrateNamesAsSurnames(foundNames);
}

//выберем среди конечных вариантов ФИО вокруг одной фамилии те
//у которых фамилия стоит как можно правее,
//например для ...Союза Валерия Новодворская... построится два
//варианта "Союза Валерий" и "Валерия Новодворская", нужно оставить только "Валерия Новодворская"
void CNameFinder::ChooseVariants(yvector<CFioWordSequence>& addedFioVariants, yvector<CFioWordSequence>& foundNames)
{
    if (addedFioVariants.size() == 0)
        return;
    if (addedFioVariants.size() == 1) {
        foundNames.push_back(addedFioVariants[0]);
        return;
    }

    int iMaxRightWord = 0;
    yvector<bool> gleichedFios;
    gleichedFios.resize(addedFioVariants.size(), false);
    bool bAllNotGleiched = true;
    size_t i = 0;

    bool bAllWithoutSurname = true;

    for (; i < addedFioVariants.size(); i++) {
        CFioWordSequence& oc = addedFioVariants[i];
        if (oc.m_NameMembers[Surname].IsValid()) {
            bAllWithoutSurname = false;
            int iW = m_Words.GetWord(oc.m_NameMembers[Surname]).GetSourcePair().FirstWord();
            if (iMaxRightWord < iW)
                iMaxRightWord = iW;
            if (oc.m_NameMembers[FirstName].IsValid()) {
                gleichedFios[i] = NGleiche::Gleiche(m_Words[oc.m_NameMembers[Surname]], m_Words[oc.m_NameMembers[FirstName]],
                                                    NGleiche::GenderNumberCaseCheck);
                if (gleichedFios[i])
                    bAllNotGleiched = false;
            }
        }
    }

    for (i = 0; i < addedFioVariants.size(); i++) {
        CFioWordSequence& oc = addedFioVariants[i];
        if (oc.m_NameMembers[Surname].IsValid() /*&& !bEqualFioPos*/) {
            int iW = m_Words.GetWord(oc.m_NameMembers[Surname]).GetSourcePair().FirstWord();
            if (iMaxRightWord == iW && (bAllNotGleiched || gleichedFios[i]))
                foundNames.push_back(addedFioVariants[i]);
        } else if (bAllWithoutSurname)
                foundNames.push_back(addedFioVariants[i]);
    }
}

bool CNameFinder::AddFoundFio(yvector<CFioWordSequence>& foundNames, CFioWordSequence& foundName)
{
    CWord* pW1 = &m_Words[foundName.FirstWord()];
    CWord* pW2 = &m_Words[foundName.LastWord()];
    bool bQuote1 = false;
    bool bQuote2 = false;
    if (!pW1->HasOpenQuote()) {
        if (foundName.FirstWord() > 0)
            if (m_Words[foundName.FirstWord()-1].IsQuote())
                bQuote1 = true;
    } else
        bQuote1 = true;

    if (!pW2->HasCloseQuote()) {
        if (foundName.LastWord() < (int)m_Words.OriginalWordsCount() - 1)
            if (m_Words[foundName.LastWord() + 1].IsQuote())
                bQuote2 = true;
    } else
        bQuote2 = true;

    bool bQuoteBetween = (foundName.Size() > 1) && (pW1->HasCloseQuote() || pW2->HasOpenQuote());
    if (!(bQuote1 && bQuote2) && !bQuoteBetween) {
        bool bAdd = true;
        if (foundNames.size() > 0) {
            CFioWordSequence& prevFio = foundNames[foundNames.size() - 1];
            if ((((CFIOOccurence&)prevFio) == (CFIOOccurence&)foundName) &&
                (prevFio.GetGrammems() == foundName.GetGrammems())) {
                bAdd = false;
                prevFio.m_Fio.m_Genders |= foundName.m_Fio.m_Genders;
            }
        }

        if (bAdd)
            foundNames.push_back(foundName);
        return true;
    }
    return false;
}

void CNameFinder::AddSingleSurname(int iW, yvector<CFioWordSequence>& foundNames)
{
    int iH = m_Words[iW].IsName_i(Surname);
    if (iH == -1)
        return;
    CFioWordSequence foundName;
    foundName.SetPair(iW, iW);
    foundName.m_NameMembers[Surname] = SWordHomonymNum(iW, iH);
    foundName.m_Fio.m_strSurname = m_Words[iW].GetHomonym(iH)->GetLemma();
    foundName.m_Fio.m_bFoundSurname = true;
    yvector<CFioWordSequence> nameVariants;
    CheckGrammInfo(foundName, nameVariants);
    for (size_t i = 0; i < nameVariants.size(); i++) {
        NormalizeFio(nameVariants[i]);
        AddFoundFio(foundNames, nameVariants[i]);
    }
}

void CNameFinder::TrateNamesAsSurnames(yvector<CFioWordSequence>& foundNames)
{
    yvector<int> to_del;
    int i = 0;
    for (; i < (int)foundNames.size();) {
        i = TrateNameAsSurnames(i, foundNames, to_del);
    }
    for (i = (int)to_del.size() - 1; i >= 0; i--)
        foundNames.erase(foundNames.begin() + to_del[i]);

    for (i = (int)foundNames.size() - 1; i >= 0; i--) {
        if (((foundNames[i].m_Fio.GetType() == IOname) ||
              (foundNames[i].m_Fio.GetType() == Iname)) &&
            foundNames[i].m_Fio.m_bInitialName)
            foundNames.erase(foundNames.begin() + i);
    }
}

int CNameFinder::TrateNameAsSurnames(int iFoundName, yvector<CFioWordSequence>& foundNames, yvector<int>& to_del)
{
    int ret = iFoundName + 1;
    SFullFIO& fio1 = foundNames[iFoundName].m_Fio;
    CFioWordSequence& fioWordsPair1 = foundNames[iFoundName];
    if (fio1.GetType() != Iname)
        return ret;
    if (fio1.m_bInitialName)
        return ret;
    bool bFromRight = false;
    bool bFromLeft = false;
    const CHomonym& firstname1 = m_Words[fioWordsPair1.m_NameMembers[FirstName]];

    int iLastDel = -1;
    if (to_del.size())
        iLastDel = to_del[(int)to_del.size() - 1];

    if (iFoundName < (int)foundNames.size() - 1) {
        CFioWordSequence& fioWS = foundNames[iFoundName + 1];
        for (;;) {
            if (fioWS.FirstWord() != fioWordsPair1.LastWord() + 1)
                break;
            if (fioWS.m_NameMembers[Surname].IsValid())
                break;
            if (fioWS.m_NameMembers[FirstName].IsValid()) {
                const CHomonym& firstname1 = m_Words[fioWordsPair1.m_NameMembers[FirstName]];
                const CHomonym& firstname2 = m_Words[fioWS.m_NameMembers[FirstName]];
                if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::CaseNumberCheck)) {
                    CWord* pW = &m_Words[fioWordsPair1.m_NameMembers[FirstName].m_WordNum];
                    int iFirstNameH = pW->IsName_i(FirstName, fioWordsPair1.m_NameMembers[FirstName].m_HomNum + 1);
                    if (iFirstNameH == - 1)
                        break;
                    {
                        CHomonym& firstname1 = pW->GetRusHomonym(iFirstNameH);
                        if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::CaseNumberCheck))
                            break;
                    }
                }
            }
            bFromRight = true;
            break;
        }
    }
    if (iFoundName > 0) {
        CFioWordSequence& fioWS= foundNames[iFoundName - 1];
        for (;;) {
            if (iLastDel == (iFoundName - 1))
                break;
            if (fioWS.LastWord() != fioWordsPair1.FirstWord() - 1)
                break;
            if (fioWS.m_NameMembers[Surname].IsValid())
                break;
            if (fioWS.m_NameMembers[FirstName].IsValid()) {
                CWord* pW = &m_Words[fioWS.m_NameMembers[FirstName].m_WordNum];
                const CHomonym& firstname2 = m_Words[fioWS.m_NameMembers[FirstName]];
                if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::CaseNumberCheck)) {
                    int iFirstNameH = pW->IsName_i(FirstName, fioWS.m_NameMembers[FirstName].m_HomNum + 1);
                    if (iFirstNameH == - 1)
                        break;
                    {
                    CHomonym& firstname2 = pW->GetRusHomonym(iFirstNameH);
                    if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::CaseNumberCheck))
                        break;
                    }
                }
            }
            bFromLeft = true;
            break;
        }
    }
    if (bFromLeft && (m_bPriorityToTheRightSurname || !bFromRight)) {
        CFioWordSequence& fioWS = foundNames[iFoundName - 1];
        fioWS.m_Fio.m_strSurname = fio1.m_strName;
        fioWS.m_NameMembers[Surname] = fioWordsPair1.m_NameMembers[FirstName];
        fioWS.SetLastWord(fioWordsPair1.LastWord());
        to_del.push_back(iFoundName);
    }

    if (bFromRight && !m_bPriorityToTheRightSurname) {
        CFioWordSequence& fioWS = foundNames[iFoundName + 1];
        fioWS.m_Fio.m_strSurname = fio1.m_strName;
        fioWS.m_NameMembers[Surname] = fioWordsPair1.m_NameMembers[FirstName];
        fioWS.SetFirstWord(fioWordsPair1.FirstWord());
        to_del.push_back(iFoundName);
        ret = iFoundName + 2;
    }
    if (bFromRight && m_bPriorityToTheRightSurname) {
        CFioWordSequence& fioWS = foundNames[iFoundName + 1];
        if (fioWS.m_NameMembers[Patronomyc].IsValid() ||
            fioWS.m_NameMembers[InitialPatronomyc].IsValid() ||
            fioWS.m_NameMembers[InitialName].IsValid()) {
            fioWS.m_Fio.m_strSurname = fio1.m_strName;
            fioWS.m_NameMembers[Surname] = fioWordsPair1.m_NameMembers[FirstName];
            fioWS.SetFirstWord(fioWordsPair1.FirstWord());
            to_del.push_back(iFoundName);
            ret = iFoundName + 2;
        }
    }
    //это для случая ГЕРМАНА Дмитрия Леонидовича
    //здесь мы сначала  взяли ГЕРМАНА как имя (есть такое в словаре)
    //а Дмитрия Леонидовича - как ИмяОтчество
    //теперь соединив их, нужно посмореть, а вдруг это было такое дурацкое слово
    //у которого было омоним и имени и фамилии

    if (bFromLeft) {
        CFioWordSequence& fioWS = foundNames[iFoundName - 1];
        if (fioWS.m_NameMembers[Surname].IsValid()) {
            CWord& w = m_Words.GetWord(fioWS.m_NameMembers[Surname]);
            int iH = w.HasMorphNounWithGrammems_i(TGramBitSet(gSurname));
            if (iH != -1)
                fioWS.m_NameMembers[Surname].m_HomNum = iH;
        }
    }

    return ret;
}

bool CNameFinder::NameCanBeSurnameFromTheRight(CFioWordSequence& foundName, int iW, int& iSurnameHFromRight)
{
    CWord* pWord = &m_Words[iW];
    if (!pWord->m_bUp)
        return false;
    int iFirstNameH = pWord->IsName_i(FirstName);
    if (iFirstNameH == -1)
        return false;
    if (iW < (int)m_Words.OriginalWordsCount() - 1) {
        CWord* pNextWord = &m_Words[iW+1];
        if (pNextWord->IsName(Surname) ||
            pNextWord->IsName(Patronomyc))
            return false;
        if (CanBeSurnameFromTheRight(iW+1))
            return false;
    }
    if (foundName.m_NameMembers[FirstName].IsValid()) {
        SWordHomonymNum& firstname_ind = foundName.m_NameMembers[FirstName];
        const CHomonym& firstname1 = m_Words[firstname_ind];
        CHomonym& firstname2 = m_Words[iW].GetRusHomonym(iFirstNameH);
        if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::CaseNumberCheck)) {
            iFirstNameH = pWord->IsName_i(FirstName, iFirstNameH + 1);
            if (iFirstNameH == - 1)
                return false;
            CHomonym& firstname2 = m_Words[iW].GetRusHomonym(iFirstNameH);
            if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::CaseNumberCheck))
                return false;
        }
    }
    iSurnameHFromRight = iFirstNameH;
    return true;
}

bool CNameFinder::NameCanBeSurnameFromTheLeft(CFioWordSequence& foundName, int iW)
{
    CWord* pWord = &m_Words[iW];
    if (!pWord->m_bUp)
        return false;
    int iFirstNameH = pWord->IsName_i(FirstName);
    if (iFirstNameH == -1)
        return false;

    if (foundName.m_NameMembers[FirstName].IsValid()) {
        SWordHomonymNum& firstname_ind = foundName.m_NameMembers[FirstName];
        const CHomonym& firstname1 = m_Words[firstname_ind];
        CHomonym& firstname2 = m_Words[iW].GetRusHomonym(iFirstNameH);
        if (!NGleiche::Gleiche(firstname1, firstname2, NGleiche::GenderNumberCaseCheck))
            return false;
    }
    return true;
}

bool CNameFinder::EnlargeBySurname(CFioWordSequence& foundName, int lastWordInFIO, int nextWord, int prevWord)
{
    bool bFromRight = false;
    bool bFromLeft = false;

    int iSurnameHFromRight = -1;
    int iSurnameHFromLeft = -1;

    TGramBitSet iCommonGrammemsFromRight;
    TGramBitSet iCommonGrammemsFromLeft;

    if (nextWord < (int)m_Words.OriginalWordsCount())
        if (!m_Words[nextWord - 1].HasCloseQuote()) {
            if (CanBeSurnameFromTheRight(nextWord) &&
                GleichePredictedSurname(foundName, nextWord, iSurnameHFromRight, iCommonGrammemsFromRight))
                bFromRight = true;
            else if (m_bPriorityToTheRightSurname && NameCanBeSurnameFromTheRight(foundName, nextWord, iSurnameHFromRight))
                    bFromRight = true;
        }

    if ((prevWord >= 0) && (lastWordInFIO < prevWord))
        if (!m_Words[prevWord + 1].HasOpenQuote()) {
            bool bAtTheEndOfSentence = (foundName.LastWord() == (int)m_Words.OriginalWordsCount() - 1);
            if (!bAtTheEndOfSentence)
                bAtTheEndOfSentence = m_Words[foundName.LastWord()+1].IsPunct() || m_Words[foundName.LastWord()+1].HasSimConjHomonym();
            // allow Puttin Vladimir , but not PUTTIN Vladimir for non dictionary surnames
            bool bDifferentCase = false;
            if (foundName.m_NameMembers[FirstName].IsValid())
                bDifferentCase = (m_Words[foundName.m_NameMembers[FirstName].m_WordNum].HasAtLeastTwoUpper() != m_Words[prevWord].HasAtLeastTwoUpper());
            if (CanBeSurnameFromTheLeft(prevWord, bAtTheEndOfSentence) &&
                GleichePredictedSurname(foundName, prevWord, iSurnameHFromLeft, iCommonGrammemsFromLeft) && !bDifferentCase)
                bFromLeft = true;
        }

    bool bPriorityToTheRightSurname = m_bPriorityToTheRightSurname;
    if ((iCommonGrammemsFromRight.any()) && (iCommonGrammemsFromLeft.none()))
        bPriorityToTheRightSurname = true;

    if ((iCommonGrammemsFromRight.none()) && (iCommonGrammemsFromLeft.any()))
        bPriorityToTheRightSurname = false;

    if (bFromRight && (bPriorityToTheRightSurname || !bFromLeft)) {
        CWord* pNextWord = &m_Words[nextWord];
        foundName.SetLastWord(nextWord);
        foundName.m_NameMembers[Surname] = SWordHomonymNum(nextWord, 0);
        if (iSurnameHFromRight != -1) {
            foundName.m_NameMembers[Surname].m_HomNum = iSurnameHFromRight;
            const CHomonym& hSurname = m_Words[foundName.m_NameMembers[Surname]];
            foundName.m_Fio.m_strSurname = hSurname.GetLemma();
            foundName.m_Fio.m_bSurnameLemma = true;
        } else
            foundName.m_Fio.m_strSurname = pNextWord->GetLowerText();
        if (iCommonGrammemsFromRight.any())
            foundName.SetGrammems(iCommonGrammemsFromRight);
        return true;
    }

    if (bFromLeft && (!bPriorityToTheRightSurname || !bFromRight)) {
        foundName.SetFirstWord(prevWord);
        foundName.m_NameMembers[Surname] = SWordHomonymNum(prevWord, 0);
        if (iSurnameHFromLeft != -1) {
            foundName.m_NameMembers[Surname].m_HomNum = iSurnameHFromLeft;
            const CHomonym& hSurname = m_Words[foundName.m_NameMembers[Surname]];
            foundName.m_Fio.m_strSurname = hSurname.GetLemma();
            foundName.m_Fio.m_bSurnameLemma = true;
        } else
            foundName.m_Fio.m_strSurname = m_Words[prevWord].GetLowerText();
        if (iCommonGrammemsFromLeft.any())
            foundName.SetGrammems(iCommonGrammemsFromLeft);
        return true;
    }

    return false;
}

//ищем несловарную фамилию для одиночного инициала
void CNameFinder::EnlargeInameIn(CFioWordSequence& foundName, int lastWordInFIO)
{
    if (!foundName.m_NameMembers[InitialName].IsValid())
        return;

    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord);
}

//ищем несловарную фамилию для двух инициалов
void CNameFinder::EnlargeIOnameInIn(CFioWordSequence& foundName, int lastWordInFIO)
{
    if (!foundName.m_NameMembers[InitialName].IsValid() || !foundName.m_NameMembers[InitialPatronomyc].IsValid())
        return;
    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord);
}

//ищем несловарную фамилию для имени и инициала
bool CNameFinder::EnlargeIOnameIn(CFioWordSequence& foundName)
{
    if (!foundName.m_NameMembers[FirstName].IsValid() || !foundName.m_NameMembers[InitialPatronomyc].IsValid())
        return false;
    int nextWord = foundName.LastWord() + 1;
    //int prevWord = foundName.FirstWord() - 1;
    if (nextWord < (int)m_Words.OriginalWordsCount())
        if (CanBeSurnameFromTheRight(nextWord)) {
            CWord* pNextWord = &m_Words[nextWord];
            foundName.SetLastWord(nextWord);
            foundName.m_NameMembers[Surname] = SWordHomonymNum(nextWord, 0);
            foundName.m_Fio.m_strSurname = pNextWord->GetText();
            return true;
        }

    return false;
}

//ищем несловарную фамилию для имени
bool CNameFinder::EnlargeIname(CFioWordSequence& foundName, int lastWordInFIO)
{
    if (!foundName.m_NameMembers[FirstName].IsValid())
        return false;
    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    return EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord);
}

bool CNameFinder::HasRightPrimGroupType(CWord* pWord)
{
    if (pWord->m_typ == Word)
        return true;
    if ((pWord->m_typ == Hyphen) || (pWord->m_typ == DivWord) || (pWord->m_typ == HypDiv)) {
        const Wtroka& w = pWord->GetText();
        for (const wchar16* ch = w.begin(); ch != w.end(); ++ch)
            if (::IsCommonDigit(*ch))
                return false;
        return true;
    }
    return false;
}

Wtroka CNameFinder::RusConsonants = UTF8ToWide(Stroka("йцкнгшщзхфвпрлджчсмтбЙЦКНГШЩЗХФВПРЛДЖЧСМТБ"));

//хотя бы одна гласная и одна согласная должна быть
bool CNameFinder::TextCanBeSurname(const TWtringBuf& s)
{
    const NLemmer::TAlphabetWordNormalizer* awn = NLemmer::GetAlphaRules(TMorph::GetMainLanguage());
    bool bHasVowel = false;
    bool bHasConsonants = false;

    for (size_t i = 0; i < s.size(); ++i)
    {
        if (awn->GetAlphabet()->CharClass(s[i]) & NLemmer::TAlphabet::CharAlphaRequired)
             bHasVowel = true;
        if (RusConsonants.find(s[i]) != Wtroka::npos)
            bHasConsonants = true;
        if(bHasVowel & bHasConsonants)
            break;
    }
    return bHasVowel & bHasConsonants;
}

//если это анкоды, где неразделимы м.р. и ж.р. (вА, вБ, вВ)
//искусственно их разделяем, анкод херим - нафиг он нам не нужен тогда
void CNameFinder::AdjustGrammemsForFIO(THomonymGrammems& grammems, const SFullFIO& fio) const
{
    const TGramBitSet mask = TGramBitSet(gFeminine, gMasculine);
    if (grammems.HasAll(mask) || grammems.Has(gMasFem)) {
        grammems.Delete(gPlural);
        grammems.Replace(NSpike::AllGenders, fio.m_Genders);
    }
}

//предсказываем одниночную несловарную фамилию (ее мы определили в качестве фамилии в CNameClusterBuilder
// на основе сравнения с другими найденными фио )
//еще и проверяем, совпадает ли предсказанная лемма с леммой из SFullFIO& fio
//если не совпадает (или род не тот), то возвращает false
//(если предсказать не смог - возвращаем true)

bool CNameFinder::PredictSingleSurname(CFIOOccurence& fioWP, const SFullFIO& fio)
{
    if (fio.m_bFoundSurname)
        return true;
    if (fioWP.m_NameMembers[FirstName].IsValid() || fioWP.m_NameMembers[InitialName].IsValid())
        return true;
    if (!fioWP.m_NameMembers[Surname].IsValid())
        return true;

    if (!PredictSurname(fioWP,fioWP.m_NameMembers[Surname].m_WordNum))
        return true;

    //для выбора женской формы фамилии
    //к сожалению, пока эта ф-ция реализована в этом классе
    SFullFIO fioForFemSurname;

    CWord& wSurname = m_Words.GetWord(fioWP.m_NameMembers[Surname]);
    CWord::SHomIt it = wSurname.IterHomonyms();
    int iHSurname = -1;
    //выберем среди предсказанных фамилий ту, которая совпадает с фамилией из SFullFIO& fio
    for (; it.Ok(); ++it) {
        const CHomonym& h = *it;
        if (!h.HasGrammem(gSurname))
            continue;

        //так как женская форма не совпадает с леммой, то нужно ее породить
        if (fio.m_Genders.Has(gFeminine) && !fio.m_Genders.Has(gMasculine) && h.HasGrammem(gFeminine)) {
            fioForFemSurname.m_strSurname = h.GetLemma();
            if (NStr::EqualCi(fio.m_strSurname, fioForFemSurname.GetFemaleSurname()))
                break;
        } else if (fio.m_Genders.Has(gMasculine) && h.HasGrammem(gMasculine))
            if (NStr::EqualCi(fio.m_strSurname, h.GetLemma()))
                break;
    }
    if (it.Ok())
        iHSurname = it.GetID();

    if (iHSurname != -1) {
        const CHomonym& h = wSurname.GetRusHomonym(iHSurname);
        TGramBitSet grm = h.Grammems.All();
        grm &=  ~TGramBitSet(gPlural);
        grm &= ~NSpike::AllGenders | fio.m_Genders;

        TGrammarBunch forms;
        GetAgreedHomonymForms(h, grm, NGleiche::GenderNumberCaseCheck, forms);

        fioWP.SetGrammems(forms);
        AdjustGrammemsForFIO(fioWP.GetGrammems(), fio);

        fioWP.m_NameMembers[Surname].m_HomNum = iHSurname;
        fioWP.SetMainWord(fioWP.m_NameMembers[Surname]);
        return true;
    } else
        return false;
}

bool CNameFinder::PredictSurname(CFIOOccurence& fioWP, int iW)
{
    SWordHomonymNum resWH;
    //int newH = -1;
    SWordHomonymNum whFirstName = fioWP.m_NameMembers[FirstName];
    //для имени в именительном падеже не предсказываем
    if (whFirstName.IsValid()) {
        const CHomonym& h = m_Words[whFirstName];
        if ((m_Words.GetWord(whFirstName).GetRusHomonymsCount() == 1) && h.Grammems.HasNominativeOnly())
            return false;
    }

    CWord& w = m_Words[iW];
    bool bPredicted = false;
    bPredicted = w.PredictAsSurname();
    if (!bPredicted) {
        int iH = w.HasPOSi(gAdjective);
        if (iH != -1) {
            w.GetRusHomonym(iH).Grammems.Add(gSurname);
            bPredicted = true;
        }
    }
    if (!bPredicted) {
        int iH = w.HasPOSi(gAdjNumeral);
        if (iH != -1) {
            w.GetRusHomonym(iH).Grammems.Add(gSurname);
            bPredicted = true;
        }
    }

    return bPredicted;
}

static TGramBitSet GetGrammemsContaining(const CHomonym& hom, const TGramBitSet& required) {
    TGramBitSet res;
    for (TGrammarBunch::const_iterator it = hom.Grammems.Forms().begin(); it != hom.Grammems.Forms().end(); ++it)
        if (it->HasAll(required))
            res |= *it;
    return res;
}

//согласование для предсказанной фамилии
bool CNameFinder::GleichePredictedSurname(CFioWordSequence& foundName, int iW, int& newH, TGramBitSet& commonGrammems)
{
    if (!PredictSurname(foundName, iW))
        return true;
    SWordHomonymNum whFirstName = foundName.m_NameMembers[FirstName];
    CWord& wSurname = m_Words.GetOriginalWord(iW);

    bool bRes = false;
    //предсказанных фамилий может быть несколько
    //например, Шиховой - как Шихов или как Шиховой
    //ищем согласованные с именем
    for (CWord::SHomIt it = wSurname.IterHomonyms(); it.Ok(); ++it) {
        const CHomonym& hSurname = *it;
        if (!hSurname.HasGrammem(gSurname))
            continue;

        newH = it.GetID();
        if (whFirstName.IsValid()) {
            const CHomonym& hFirstName = m_Words[whFirstName];
            commonGrammems = NGleiche::GleicheGrammems(hSurname, hFirstName, NGleiche::GenderNumberCaseCheck);
            if (commonGrammems.any()) {
                bRes = true;
                break;
            }
        } else {
            commonGrammems = GetGrammemsContaining(hSurname, (foundName.m_Fio.m_Genders & NSpike::AllGenders) | TGramBitSet(gSingular));
            if (commonGrammems.any()) {
                bRes = true;
                break;
            }
        }
    }

    //если согласование не прошло, а имя неизменяемое или фамилия несловарное слово, то считаем, что имя иностранное,
    //а предсказание случайное, <strike>херим все предсказанные омонимы</strike> теперь не херим!
    //и говорим, что все зашибись (типа Катрин Денев)
    if (!bRes && whFirstName.IsValid())
        if (m_Words[whFirstName].Grammems.HasAllCases() || !wSurname.IsDictionary()) {
            static const TGramBitSet predicted_grammems(gSubstantive, gSurname);
            /*Не херим, т.к. если у имени две интерпретации (Евгения = муж. Евгений + жен. Евгения)
              то можем по ошибке удалить предсказанную фамилию до того, как дело дойдет до проверки ее согласованности
              с правильным вариантом имени (Евгения Балуевского - если сначала проверять Евгению (жен), то она не согласуется с Балуевского
              и правильно предсказанная фамилия Балуевский будет удалена)
              Удаление же лишнего фио (в данном случае "Евгении Балуевского") будет выполнено в ChooseVariants

            wSurname.DeleteHomonymWithGrammems(predicted_grammems, true);
            if (wSurname.GetRusHomonymsCount() == 0)
                wSurname.CreateHomonyms(false);
            */

            newH = -1;
            //check that non-predicted surname homonym exists, otherwise create it
            for (CWord::SHomIt it = wSurname.IterHomonyms(); it.Ok(); ++it)
                if (!it->Grammems.HasAll(predicted_grammems)) {
                    newH = it.GetID();
                    break;
                }
            if (newH == -1)
                wSurname.CreateHomonyms(false);

            return true;
        }
    return bRes;
}

bool CNameFinder::CanBeSurnameFromTheRight(int iW)
{
    CWord* pWord = &m_Words[iW];
    if (!HasRightPrimGroupType(pWord))
        return false;
    bool bCan = pWord->m_bUp &&
                TextCanBeSurname(pWord->GetText()) &&
                !pWord->HasOpenQuote() &&
                !pWord->IsName(Surname) &&
                !pWord->GetHasAltWordPart() &&
                !pWord->IsName(FirstName)&&
                (pWord->m_typ != Initial) &&
                (pWord->GetText().size() > 1) &&
                ((pWord->m_typ == Word) ||
                    (pWord->m_typ == Hyphen) ||
                    (pWord->m_typ == DivWord));
    if (!bCan)
        return false;

    return true;
}

bool CNameFinder::CanBeSurnameFromTheLeft(int iW, bool bAtTheEndOfSent)
{
    CWord* pWord = &m_Words[iW];
    if (!HasRightPrimGroupType(pWord))
        return false;

    const TGramBitSet noun_adj_mask(gSubstantive, gAdjective);
    return  pWord->m_bUp &&
            TextCanBeSurname(pWord->GetText()) &&
            !pWord->HasCloseQuote() &&
            !pWord->IsName(FirstName) &&
            !pWord->IsName(Surname) &&
            !pWord->GetHasAltWordPart() &&
            !(pWord->HasGeoHom() &&
                m_bPriorityToTheRightSurname &&
                pWord->GetHomonymsCount() == 1 &&
                pWord->HasPOS(gSubstantive)) &&
            (!pWord->IsDictionary() ||
                (iW > 0 && pWord->HasOnlyPOSes(noun_adj_mask)) ||
                !m_bPriorityToTheRightSurname  ||
                (bAtTheEndOfSent && iW==0 && pWord->HasOnlyPOSes(noun_adj_mask)))&&
            (pWord->m_typ != Initial) &&
            (pWord->GetText().size() > 1) &&
            ((pWord->m_typ == Word) ||
                (pWord->m_typ == Hyphen) ||
                (pWord->m_typ == DivWord)
            );
}
//ищем несловарную фамилию для имени и отчества
bool CNameFinder::EnlargeIOname(CFioWordSequence& foundName, int lastWordInFIO)
{
    if (!foundName.m_NameMembers[FirstName].IsValid() || !foundName.m_NameMembers[Patronomyc].IsValid())
        return false;

    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    if (EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord))
        return true;

    return false;
}

//проверяет согласование имени и word_ind (фамилии или отчества)
//если у имени есть еще один омоним, то и его проверяем
TGramBitSet CNameFinder::FirstNameAgreement(const SWordHomonymNum& word_ind, SWordHomonymNum& first_name_ind) const
{
    const CHomonym& word = m_Words[word_ind];
    const CHomonym& firstName = m_Words[first_name_ind];
    TGramBitSet grammems = NGleiche::GleicheGrammems(word, firstName, NGleiche::GenderNumberCaseCheck);
    if (grammems.Has(gPlural) && !grammems.Has(gSingular))
        return TGramBitSet();
    return grammems;
}

//выбираем нужные граммемки от фамилии, разбивая их на женские и мужские
TGramBitSet CNameFinder::GetGrammemsForSingleSurname(const CHomonym& surname, TGramBitSet& femGrammems, TGramBitSet& muscGrammems)
{
    for (CHomonym::TFormIter it = surname.Grammems.IterForms(); it.Ok(); ++it) {
        //для неизменяемых фамилий типа Бойко
        if (it->Has(gPlural) && !it->Has(gSingular))
            continue;
        if (it->Has(gFeminine))
            femGrammems |= *it;
        if (it->Has(gMasculine))
            muscGrammems |= *it;
    }
    return surname.Grammems.All();
}
//вычисляем нужные граммемы из имени
//если есть еще один омоним с именем (условно женский), то его сохраняем в secondFirstNameInd
//если граммемы первого омонима (first_name_ind) плохие  (нет ед.ч.), то ищем альтернативный омоним
//и заменяем им first_name_ind
//также если это для поиска (обрабатывается \фио), то плохими считаются граммемы, где нет им.п.
TGramBitSet CNameFinder::GetGrammemsForSingleFirstName(SWordHomonymNum& first_name_ind,
                                                       TGramBitSet& femGrammems, TGramBitSet& muscGrammems,
                                                       SWordHomonymNum& secondFirstNameInd)
{
    const CHomonym& first_name = m_Words[first_name_ind];
    bool bQuestionableGrammems = false;
    TGramBitSet grammems = GetGrammemsContaining(first_name, TGramBitSet(gSingular));
    if (!grammems.any())
        bQuestionableGrammems = true;
    muscGrammems = grammems;
    //попытаемся еще найти омоним для женского имени

    int iHom = m_Words[first_name_ind.m_WordNum].IsName_i(FirstName, first_name_ind.m_HomNum + 1);
    if (iHom != -1) {
        secondFirstNameInd = first_name_ind;
        secondFirstNameInd.m_HomNum = iHom;
        const CHomonym& firstName_fem = m_Words[secondFirstNameInd];
        TGramBitSet grammems_fem = GetGrammemsContaining(firstName_fem, TGramBitSet(gSingular));
        femGrammems = grammems_fem;

        if (bQuestionableGrammems) {
            for (;;) {
                if (!muscGrammems.any() && femGrammems.Has(gSingular)) {
                    muscGrammems.Reset();
                    grammems = grammems_fem;
                    break;
                }

                {
                    grammems |= grammems_fem;
                    break;
                }
/*
                if (!grammems.any())            //??? repeated code
                {
                    grammems = grammems_fem;
                    muscGrammems.Reset();
                }
                else
                    grammems |= grammems_fem;
                break;
*/          }
        } else
            grammems |= grammems_fem;

        if (!muscGrammems.any()) {
            first_name_ind = secondFirstNameInd;
            secondFirstNameInd.Reset();
        }
    } else if (grammems.Has(gFeminine)) {
        femGrammems = grammems;
        muscGrammems.Reset();
    }
    return grammems;
}

bool CNameFinder::GetNameHomonyms(ENameType type, SWordHomonymNum whTemplate, yvector<SWordHomonymNum>& homs) const
{
    const CWord& w = m_Words.GetWord(whTemplate);
    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it)
        if (it->HasNameType(type)) {
            SWordHomonymNum newWH = whTemplate;
            newWH.m_HomNum = it.GetID();
            homs.push_back(newWH);
        }
    return homs.size() > 0;
}

bool CNameFinder::CheckAgreements(CFioWordSequence& foundName) const
{
    TGramBitSet commonFirstNameSurnameGrammems, commonPatronymicSurnameGrammems, resGrammems;

    //согласование между именем и фамилией
    if (foundName.m_NameMembers[Surname].IsValid() &&
        foundName.m_NameMembers[FirstName].IsValid()) {
        commonFirstNameSurnameGrammems = FirstNameAgreement(foundName.m_NameMembers[Surname], foundName.m_NameMembers[FirstName]);
        resGrammems = commonFirstNameSurnameGrammems;
        if (commonFirstNameSurnameGrammems.none())
            return false;
    }
    //согласование между именем и отчеством
    if (foundName.m_NameMembers[Patronomyc].IsValid() &&
        foundName.m_NameMembers[FirstName].IsValid()) {
        commonPatronymicSurnameGrammems = FirstNameAgreement(foundName.m_NameMembers[Patronomyc], foundName.m_NameMembers[FirstName]);
        resGrammems = commonPatronymicSurnameGrammems;
        if (commonPatronymicSurnameGrammems.none())
            return false;
    }

    //пересечение граммем (именем и фамилией) и (именем и отчеством)
    if (commonPatronymicSurnameGrammems.any() && commonFirstNameSurnameGrammems.any()) {
        resGrammems = commonFirstNameSurnameGrammems & commonPatronymicSurnameGrammems
                      & (NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers);
        if (resGrammems.none())
            return false;
    }

    //возьмем нужные граммемы от фамилии, когда либо вообще нет имени, либо только инициал, с которого ничего не возьмешь
    if (foundName.m_NameMembers[Surname].IsValid() && !foundName.m_NameMembers[FirstName].IsValid())
        resGrammems = m_Words[foundName.m_NameMembers[Surname]].Grammems.All();

    //возьмем нужные граммемы от имени, когда ничего больше нет
    if  (foundName.m_NameMembers[FirstName].IsValid() &&
         !foundName.m_NameMembers[InitialName].IsValid() && !foundName.m_NameMembers[Patronomyc].IsValid() &&
         !foundName.m_NameMembers[InitialPatronomyc].IsValid() && !foundName.m_NameMembers[Surname].IsValid())

        resGrammems = m_Words[foundName.m_NameMembers[FirstName]].Grammems.All();

    //когда граммемы взять неоткуда и неизвестен даже род,
    //присваиваем все возможные значения
    if (foundName.m_NameMembers[InitialName].IsValid() && !foundName.m_NameMembers[Surname].IsValid())
        resGrammems =  TGramBitSet(gMasculine, gFeminine, gSingular, gSurname) | NSpike::AllCases;

    //only allow FIO with gSingular
    if (!resGrammems.Has(gSingular))
       return false;
    if (!resGrammems.HasAll(NSpike::AllMajorCases))
        resGrammems.Reset(gPlural);     //do not allow plural FIO (only indeclinable)

    foundName.SetGrammems(resGrammems);
    foundName.m_Fio.m_Genders = resGrammems & NSpike::AllGenders;

    return resGrammems.any();
}

//проверяет все возможные согласования между именем, фамилией и отчеством
//приписывает граммемки CFIOOccurence
//добаляет варианты для женского (или мужского) варианта
//на входе функции в foundNames всегда находится один вариант
bool CNameFinder::CheckGrammInfo(const CFioWordSequence& nameTempalte, yvector<CFioWordSequence>& foundNames)
{
    YASSERT(foundNames.size() == 0);

    yvector<SWordHomonymNum> firstNameHoms;
    if (nameTempalte.m_NameMembers[FirstName].IsValid())
        GetNameHomonyms(FirstName, nameTempalte.m_NameMembers[FirstName],  firstNameHoms);

    yvector<SWordHomonymNum> surnameHoms;
    if (nameTempalte.m_NameMembers[Surname].IsValid())
        GetNameHomonyms(Surname, nameTempalte.m_NameMembers[Surname],  surnameHoms);

    if (firstNameHoms.size() > 0 && surnameHoms.size() > 0) {
        for (size_t i = 0; i < firstNameHoms.size(); i++)
            for (size_t j = 0; j < surnameHoms.size(); j++) {
                CFioWordSequence newName = nameTempalte;
                newName.m_NameMembers[FirstName] = firstNameHoms[i];
                newName.m_NameMembers[Surname] = surnameHoms[j];
                if (CheckAgreements(newName)) {
                    newName.m_Fio.m_strSurname = m_Words[newName.m_NameMembers[Surname]].GetLemma();
                    newName.m_Fio.m_strName= m_Words[newName.m_NameMembers[FirstName]].GetLemma();
                    foundNames.push_back(newName);
                }
            }
    } else if (firstNameHoms.size() > 0) {
            for (size_t j = 0; j < firstNameHoms.size(); j++) {
                CFioWordSequence newName = nameTempalte;
                newName.m_NameMembers[FirstName] = firstNameHoms[j];
                if (CheckAgreements(newName)) {
                    newName.m_Fio.m_strName = m_Words[newName.m_NameMembers[FirstName]].GetLemma();
                    foundNames.push_back(newName);
                }
            }
        } else if (surnameHoms.size() > 0) {
            for (size_t i = 0; i < surnameHoms.size(); i++) {
                CFioWordSequence newName = nameTempalte;
                newName.m_NameMembers[Surname] = surnameHoms[i];
                if (CheckAgreements(newName)) {
                    newName.m_Fio.m_strSurname = m_Words[newName.m_NameMembers[Surname]].GetLemma();
                    foundNames.push_back(newName);
                }
            }
        } else //инициалы какие-нибудь
     {
        foundNames.push_back(nameTempalte);
     }

    return foundNames.size() > 0;
}

bool CNameFinder::ApplyTemplate(SNameTemplate& nameTemplate, yvector<CFioWordSequence>& foundNames, int& curWord, int iLastWordInFIO)
{
    if (curWord + nameTemplate.count > (int)m_Words.OriginalWordsCount())
        return false;
    SFullFIO fullFio;
    CFioWordSequence foundName(fullFio);
    for (int i = 0; i < nameTemplate.count; i++) {
        int iHom;
        if ((iHom = m_Words[curWord + i].IsName_i(nameTemplate.name[i])) == -1)
            return false;
        if (m_Words[curWord + i].HasOpenQuote() && m_Words[curWord + i].HasCloseQuote())
            return false;
        foundName.m_NameMembers[nameTemplate.name[i]] = SWordHomonymNum(curWord + i, iHom);
    }

    foundName.SetPair(curWord, curWord + nameTemplate.count - 1);

    if (!nameTemplate.m_bCheckSurnameFromDic) {
        if (!CheckGrammInfo(foundName, foundNames))
            return false;
        else
            return true;
    }

    bool bFoundFromLeft = false;
    bool bFoundFromRight = false;

    int iSurnameFromRight = curWord + nameTemplate.count;
    int iSurnameFromLeft = curWord - 1;

    CFioWordSequence foundFioFromRight = foundName;
    CFioWordSequence foundFioFromLeft = foundName;

    yvector<CFioWordSequence> namesFoundFromRight;
    yvector<CFioWordSequence> namesFoundFromLeft;

    bool fname_uppercase = m_Words[curWord].HasAtLeastTwoUpper();

    //попытаемся справа найти
    if (iSurnameFromRight < (int)m_Words.OriginalWordsCount()) {
        // allow Vladimir Putin, Vladimir PUTIN and VLADIMIR PUTIN, but not VLADIMIR Putin (spike :( )
        if (m_Words[iSurnameFromRight].IsName(Surname) &&
            (!fname_uppercase || m_Words[iSurnameFromRight].HasAtLeastTwoUpper())) {
            //так как может быть много омонимов-фамилий(например, Шиховой)
            for (CWord::SHomIt it = m_Words[iSurnameFromRight].IterHomonyms(); it.Ok(); ++it) {
                const CHomonym& h = *it;
                if (!h.HasNameType(Surname))
                    continue;
                foundFioFromRight.m_NameMembers[Surname] = SWordHomonymNum(iSurnameFromRight, it.GetID());
                foundFioFromRight.SetLastWord(iSurnameFromRight);
                //проверяем согласования и заполняем граммемы для WordSequence
                //ф-ция может размножить варианты фио для женских и мужских омонимов
                if (CheckGrammInfo(foundFioFromRight, namesFoundFromRight)) {
                    bFoundFromRight = true;
                    break;
                }
            }
        }
    }

    //попытаемся слева найти
    if ((iSurnameFromLeft >= 0) && (iSurnameFromLeft > iLastWordInFIO)) {
        // allow Putin Vladimir, PUTIN Vladimir and PUTIN VLADIMIR, but not Putin VLADIMIR (spike :( )
        //also check for closing quote separating Surname from FIO (which is forbidden)
        if (m_Words[iSurnameFromLeft].IsName(Surname) && !m_Words[iSurnameFromLeft].HasCloseQuote() &&
            (!fname_uppercase || m_Words[iSurnameFromLeft].HasAtLeastTwoUpper())) {
            for (CWord::SHomIt it = m_Words[iSurnameFromLeft].IterHomonyms(); it.Ok(); ++it) {
                const CHomonym& h = *it;
                if (!h.HasGrammem(gSurname))
                    continue;
                foundFioFromLeft.m_NameMembers[Surname] = SWordHomonymNum(iSurnameFromLeft, it.GetID());
                foundFioFromLeft.SetFirstWord(iSurnameFromLeft);
                if (CheckGrammInfo(foundFioFromLeft, namesFoundFromLeft)) {
                    bFoundFromLeft = true;
                    break;
                }
            }
        }
    }

    if (bFoundFromRight && (m_bPriorityToTheRightSurname || !bFoundFromLeft)) {
        foundNames = namesFoundFromRight;
        return true;
    }

    if (bFoundFromLeft && (!m_bPriorityToTheRightSurname || !bFoundFromRight)) {
        foundNames = namesFoundFromLeft;
        return true;
    }

    return false;
}

//ищем цепочку, описанную в SNameTemplate
bool CNameFinder::FindFirmedName(SNameTemplate& nameTemplate, yvector<CFioWordSequence>& foundNames, int& curWord, int iLastWordInFIO)
{
    if (!ApplyTemplate(nameTemplate, foundNames, curWord, iLastWordInFIO))
        return false;

    for (size_t i = 0; i < foundNames.size(); i++) {
        CFioWordSequence& foundName = foundNames[i];

        if (foundName.m_NameMembers[Surname].IsValid()) {
            foundName.m_Fio.m_strSurname = m_Words[foundName.m_NameMembers[Surname]].GetLemma();
            foundName.m_Fio.m_bFoundSurname = true;
        }

        //для случая Мамай В.И.
        if (foundName.m_NameMembers[FirstName].IsValid() && foundName.m_NameMembers[InitialName].IsValid()) {
            foundName.m_Fio.m_strSurname = m_Words[foundName.m_NameMembers[FirstName].m_WordNum].GetLowerText();
            foundName.m_NameMembers[Surname] = foundName.m_NameMembers[FirstName];
            foundName.m_NameMembers[FirstName].Reset();
            int iH = m_Words[foundName.m_NameMembers[Surname].m_WordNum].HasMorphNounWithGrammems_i(TGramBitSet(gSurname));
            if (iH != -1) {
                foundName.m_Fio.m_bFoundSurname = true;
                foundName.m_NameMembers[Surname].m_HomNum = iH;
            }
        }

        if (foundName.m_NameMembers[FirstName].IsValid())
            foundName.m_Fio.m_strName = m_Words[foundName.m_NameMembers[FirstName]].GetLemma();
        if (foundName.m_NameMembers[Patronomyc].IsValid())
            foundName.m_Fio.m_strPatronomyc = m_Words[foundName.m_NameMembers[Patronomyc]].GetLemma();
        if (foundName.m_NameMembers[InitialName].IsValid()) {
            foundName.m_Fio.m_strName = m_Words[foundName.m_NameMembers[InitialName]].GetLemma();
            if (foundName.m_Fio.m_strName.back() == '.')
                foundName.m_Fio.m_strName  = foundName.m_Fio.m_strName.substr(0, foundName.m_Fio.m_strName.size() - 1);
            foundName.m_Fio.m_bInitialName = true;
        }
        if (foundName.m_NameMembers[InitialPatronomyc].IsValid()) {
            foundName.m_Fio.m_strPatronomyc = m_Words[foundName.m_NameMembers[InitialPatronomyc]].GetLemma();
            if (foundName.m_Fio.m_strPatronomyc.back() == '.')
                foundName.m_Fio.m_strPatronomyc = foundName.m_Fio.m_strPatronomyc.substr(0, foundName.m_Fio.m_strPatronomyc.size() - 1);

            foundName.m_Fio.m_bInitialPatronomyc = true;
        }
    }

    return true;
}
