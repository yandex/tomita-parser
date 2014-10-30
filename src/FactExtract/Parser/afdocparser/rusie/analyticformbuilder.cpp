#include "analyticformbuilder.h"

CAnalyticFormBuilder::CAnalyticFormBuilder(CWordVector& Words, yvector<SWordHomonymNum>& clauseWords)
    : m_Words(Words)
    , m_ClauseWords(clauseWords)
{
}

DECLARE_STATIC_RUS_WORD(kByt, "быть");
DECLARE_STATIC_RUS_WORD(kStat, "стать");

bool CAnalyticFormBuilder::HasAnalyticalBe(const CWord& _W) const
{
    //NB! в настоящем варианте парсера нет словаря оборотов, поэтому проверка слова на оборот временно отключена
    // если мы попали на оборот(например, "может быть"), тогда не будем строить здесь анал. форму.
    //if (_W.IsInOborot()) return false;

    // "быто" предсказывается как "быть"
    //if (_W.m_bPredicted) return false;
    //NB! пока предсказание отсутствует в парсере

    //if ( _W.HasPOS(UnknownPOS) ) return false;
    if (_W.HasUnknownPOS()) return false;

    for (CWord::SHomIt it = _W.IterHomonyms(); it.Ok(); ++it) {
        bool is_verb = it->IsPersonalVerb();
        if (is_verb && !it->IsPresentTense() && it->Lemma == kByt)
            return true;
        if ((is_verb || it->HasGrammem(gInfinitive)) && it->Lemma == kStat)
            return true;
    }
    return false;
}

DECLARE_STATIC_NAMES(TSynNounLemmas, "каждый один другой тот который");
static bool IsSynNoun(const CHomonymBase& hom) {
    return hom.IsMorphNoun() || (hom.HasGrammem(gAdjPronoun) && TSynNounLemmas::Has(hom.Lemma));
}

bool CAnalyticFormBuilder::HasDeclinableSynNounInInstrumentalis(const CWord& _W) const
{
    for (CWord::SHomIt it = _W.IterHomonyms(); it.Ok(); ++it)
        if (IsSynNoun(*it) && it->HasGrammem(gInstrumental) && !it->HasGrammem(gNominative))
            return true;
    return false;
}

bool CAnalyticFormBuilder::CheckComparativeZAPLATAForAnalyticalForm (long lWordNo) const
{
    for (long i = lWordNo+1; i < (long)m_ClauseWords.size(); i++)
        if (HasDeclinableSynNounInInstrumentalis(m_Words.GetWord(m_ClauseWords.at(i))))
          return true;

    return false;
};

bool CAnalyticFormBuilder::HasShortParticipleOrAdj(const CWord& _W) const
{
    for (CWord::SHomIt it = _W.IterHomonyms(); it.Ok(); ++it)
        if (it->IsShortAdjectiveOrParticiple())
            return true;
    return false;
}

/*
разбираем случай "они должны были быть потрачены",
когда словоформа "были" должна  будет подсоединиться к форме "должны",
а сначала "быть" должна подсоединиться к форме "потрачены", чтобы "потрачены"
не считалось предикатом, а стало инфинитивом. Последнее действие совершается
в ниженстоящей процедуре.  Здесь ищется инфинитив "быть", за которым сразу же или через несколько
слов должно стоять краткое прилагательное, например:
"они должны были быть быстро потрачены"

*/

void CAnalyticFormBuilder::BuildAnalyticalVerbFormsZaplata2()
{
    const TGramBitSet mask(gInfinitive, gIntransitive, gImperfect);
    int iBe = -1;
    for (int WordNo = 0; (size_t)WordNo < m_ClauseWords.size(); WordNo++) {
        CWord& _W = m_Words.GetWord(m_ClauseWords.at(WordNo));
        if (_W.GetRusHomonymsCount() > 0 && _W.IterHomonyms()->Grammems.HasAll(mask) && NStr::EqualCi(_W.GetText(), kByt)) {
            iBe = WordNo;
            continue;
        }
        if (iBe != -1 && HasShortParticipleOrAdj(_W) && CheckAnalyticalVerbForm(iBe, WordNo))
            break;
    }
}

bool CAnalyticFormBuilder::CheckAnalyticalVerbForm(int iVWrd, int iSWrd)
{
    int VerbHomNo;
    yvector<int> AnalyticHom;

    bool b_AddAuxVerbHom;
    if (!IsAnalyticalVerbForm(iVWrd, iSWrd, VerbHomNo, AnalyticHom, b_AddAuxVerbHom))
        return false;

    if (AnalyticHom.size() == 0)
        return false;

    CWord& _Be = m_Words.GetWord(m_ClauseWords.at(iVWrd));
    CWord& _Predik = m_Words.GetWord(m_ClauseWords.at(iSWrd));

    //nim: в версии Диалинга у потенциального предиката (второй компоненты аналитической формы)
    //удалялись все омонимы, которые не могли образовать настоящую аналитическую форму,
    //например, для "хорошо" удалялись омонимы "Н" и "ЧАСТ".
    //в текущей версии покамест оставляем все омонимы. :nim

    for (size_t j = 0; j < AnalyticHom.size(); j++) {
        ChangeGrammemsAsAnalyticForm(_Predik.GetRusHomonym(AnalyticHom[j]), _Be.GetRusHomonym(VerbHomNo));
    }

    if (b_AddAuxVerbHom) {
        THomonymPtr pAuxHom = _Be.GetRusHomonym(VerbHomNo).Clone();
        pAuxHom->Grammems.Add(gAuxVerb);
        _Be.AddRusHomonym(pAuxHom);
        VerbHomNo = _Be.HasPOSi(gAuxVerb);
    } else
        _Be.GetRusHomonym(VerbHomNo).Grammems.Add(gAuxVerb);

    //удаляются омонимы не AuxVerb
    //так для слф. "были" будет убит омоним "быль"
    //пример: "были спровоцированы скандалы"
    yvector<int> HomsToDel;
    for (CWord::SHomIt it = _Be.IterHomonyms(); it.Ok(); ++it)
        if (!it->HasGrammem(gAuxVerb))
            HomsToDel.push_back(it.GetID());

    YASSERT(HomsToDel.size() < _Be.GetRusHomonymsCount());
    for (size_t i = 0; i < HomsToDel.size(); ++i)
        _Be.DeleteHomonym(HomsToDel[i]);
    //так как если вызывается из фрагментации и с синтаксисом, то
    //уже все символы были построены, а мы удаляем омоним с приписанными терминалами
    _Be.UniteHomonymsTerminalSymbols();
    for (size_t j = 0; j < AnalyticHom.size(); j++)
        _Predik.GetRusHomonym(AnalyticHom[j]).SetAnalyticMainVerbHom(&(_Be.GetRusHomonym(VerbHomNo)));

    return true;
}

//check an agreement between aux. verbs and predicates of analitical form.
bool CAnalyticFormBuilder::IsAnalyticalVerbForm(int iVerbWrd, int iSWrd, int& VerbHomNo,
                                                yvector<int>& AnalyticHom, bool& b_AddAuxVerbHom)
{
    b_AddAuxVerbHom = false;
    AnalyticHom.clear();

    const CWord& _Be = m_Words.GetWord(m_ClauseWords.at(iVerbWrd));
    const CWord& _Predik = m_Words.GetWord(m_ClauseWords.at(iSWrd));

    //с предикативными значениями "много" и "мало" анал. форм не строить : DIALING
    //с предикативными значениями "много" и "мало" анал. форм строится, но клонируется омоним для вспомогательного глагола
    //if ( ( m_Words[iSWrd].FindLemma("МНОГО") || m_Words[iSWrd].FindLemma("МАЛО") ) && ... )
    if (_Predik.HasPOS(gNumeral) && _Predik.HasPOS(gPraedic))
            b_AddAuxVerbHom = true;

    //14.07.04: отсутствие синтаксиса порождает заплатку:
    //если у потенциального предиката есть омоним Noun, то добавить омоним AuxVerb для вспомогательного глагола,
    //в противном случае, просто изменить часть речи на AuxVerb.
    if (_Predik.HasMorphNoun())
        b_AddAuxVerbHom = true;

    //masks used later (to avoid constructing them repeatedly in loop)
    const TGramBitSet Verb_Infinitive(gVerb, gInfinitive),
                      Infinitive_Imperfect(gInfinitive, gImperfect),
                      Person3_Singular(gPerson3, gSingular),
                      Infinitive_Gerund(gInfinitive, gGerund);

    for (CWord::SHomIt it = _Be.IterHomonyms(); it.Ok(); ++it) {
        VerbHomNo = it.GetID();
        const CHomonym& VerbHom = _Be.GetRusHomonym(VerbHomNo);

        if (VerbHom.Lemma == kByt || (VerbHom.Lemma == kStat && VerbHom.HasAnyOfPOS(Verb_Infinitive))) {
            for (CWord::SHomIt it_predik = _Predik.IterHomonyms(); it_predik.Ok(); ++it_predik) {
                int j = it_predik.GetID();

                const CHomonym& ShortFormHom = _Predik.GetRusHomonym(j);
                if (ShortFormHom.Grammems.HasAll(Infinitive_Imperfect) && (VerbHom.IsFutureTense() || VerbHom.Lemma == kStat))
                    AnalyticHom.push_back(j);

                else if ((ShortFormHom.HasGrammem(gPraedic)) &&
                         (VerbHom.HasGrammem(gNeuter) || (VerbHom.IsFutureTense() && VerbHom.Grammems.HasAll(Person3_Singular))))
                    AnalyticHom.push_back(j);

                else if (ShortFormHom.HasGrammem(gComparative) && ShortFormHom.IsFullAdjective())
                    AnalyticHom.push_back(j);

                else if (ShortFormHom.IsShortAdjectiveOrParticiple()) {
                    // "была", "стал", "был", "было" ...
                    if ((VerbHom.HasGrammem(gSingular) && VerbHom.IsPastTense() &&
                         ShortFormHom.HasGrammem(gSingular) &&
                         (VerbHom.Grammems.All() & ShortFormHom.Grammems.All()).HasAny(NSpike::AllGenders))
                        || (VerbHom.HasGrammem(gPlural) && ShortFormHom.HasGrammem(gPlural))
                        || (!VerbHom.IsPastTense() && VerbHom.HasGrammem(gSingular) && ShortFormHom.HasGrammem(gSingular))
                        || VerbHom.Grammems.HasAny(Infinitive_Gerund))

                        AnalyticHom.push_back(j);
                }
            }
            return AnalyticHom.size() > 0;
        }
    }
    return false;
}

void CAnalyticFormBuilder::ChangeGrammemsAsAnalyticForm(CHomonym& H, const CHomonym& VerbHomonym)
{
    THomonymGrammems old_grammems;
    H.Grammems.Swap(old_grammems);

    for (THomonymGrammems::TFormIter old = old_grammems.IterForms(); old.Ok(); ++old)
        for (THomonymGrammems::TFormIter verb = VerbHomonym.Grammems.IterForms(); verb.Ok(); ++verb) {
            Stroka strPos;

            // auxiliary verb grammems
            const TGramBitSet& VerbGrammems = *verb;
            // meaningful part grammems
            TGramBitSet MainGrammems = *old;

            // final grammems to set
            TGramBitSet ResultedGrammems;

            if (MainGrammems.Has(gInfinitive)) {
                ResultedGrammems = MainGrammems & ~TMorph::AllPOS();
                if (VerbGrammems.Has(gImperative)) {
                    // analytical form for imperatives in singular number does not exist
                    if (VerbGrammems.Has(gSingular))
                        continue;
                    ResultedGrammems.Set(gImperative); // "будем же жить!"
                } else
                    ResultedGrammems |= VerbGrammems & NSpike::AllTimes; // "я стал пилить" или "стану писать"

                ResultedGrammems |= VerbGrammems & NSpike::AllPersons;
                ResultedGrammems |= VerbGrammems & NSpike::AllNumbers;
                ResultedGrammems |= VerbGrammems & NSpike::AllGenders;

                //copy all POS grammems from verb to main
                ResultedGrammems |= VerbGrammems & TMorph::AllPOS();

                H.PutAuxArticle(SDictIndex(TEMPLATE_DICT, VerbHomonym.GetAuxArticleIndex(TEMPLATE_DICT)));
                strPos = "Г";
            } else if (TMorph::IsShortParticiple(MainGrammems)) {
                // "*будем же взяты!"
                if (VerbGrammems.Has(gImperative))
                    continue;

                ResultedGrammems = MainGrammems & ~TMorph::AllPOS();
                // remove any time grammems from participle
                ResultedGrammems &= ~NSpike::AllTimes;

                ResultedGrammems |= VerbGrammems & NSpike::AllPersons;
                ResultedGrammems |= VerbGrammems & NSpike::AllTimes;

                if (VerbGrammems.Has(gImperative))  // ??? the same check second time, always false?
                    ResultedGrammems.Set(gImperative);

                strPos = "ПРИЧАСТИЕ";
                ResultedGrammems |= TGramBitSet(gParticiple, gShort);
            } else if (TMorph::IsShortAdjective(MainGrammems)) {
                if (VerbGrammems.Has(gImperative))
                    continue; // будем cчитать, что "будем же красивы!" - это плохо!
                              // на самом деле, просто не хочется вводить  ее кучу кодов.

                ResultedGrammems =  VerbGrammems;
                ResultedGrammems |= MainGrammems & (NSpike::AllNumbers | NSpike::AllGenders | TGramBitSet(gAnimated, gInanimated));
                ResultedGrammems &= ~TMorph::AllPOS();

                if (ResultedGrammems.Has(gActive))
                    ResultedGrammems &= ~TGramBitSet(gActive);

                ResultedGrammems |= TGramBitSet(gAdjective, gShort);
                strPos = "П";
            } else if (MainGrammems.Has(gPraedic))     // "мне было больно"
            {
                ResultedGrammems = VerbGrammems;
                ResultedGrammems |= NSpike::AllCases & MainGrammems;    //copied from PronounPredk code (commented below) - preserve cases if any
                ResultedGrammems &= ~TMorph::AllPOS();

                if (ResultedGrammems.Has(gActive))
                    ResultedGrammems.Reset(gActive);

                strPos = "ПРЕДК";
                ResultedGrammems |= MainGrammems & TMorph::AllPOS();
            } else if (MainGrammems.Has(gComparative))       // он был больше тебя
            {
                ResultedGrammems = (VerbGrammems & ~TMorph::AllPOS()) | TGramBitSet(gComparative);
                if (ResultedGrammems.Has(gActive))
                    ResultedGrammems.Reset(gActive);

                strPos = "П";
                ResultedGrammems |= MainGrammems & TMorph::AllPOS();
            } else if (TMorph::IsFullAdjective(MainGrammems))
                // resolve disambiguity of homonyms, because analytical forms with full adjectives do not exist.
                continue;

            // "стал писать" "стану писать" "стать писать" - совершенный вид
            if (VerbHomonym.Lemma == kStat)
                ResultedGrammems.Reset(gImperfect).Set(gPerfect);

            // if the auxiliary verb was an infinitive then it is all an infinitive
            //  "быть  лучше" или "должно быть принесено"
            if (VerbHomonym.HasGrammem(gInfinitive)) {
                ResultedGrammems &= ~TMorph::AllPOS();
                ResultedGrammems.Set(gInfinitive);
                strPos = "ИНФИНИТИВ";
            } else if (VerbHomonym.HasGrammem(gGerund))     //  "будучи лишней"
            {
                ResultedGrammems &= ~TMorph::AllPOS();
                ResultedGrammems.Set(gGerund);

                strPos = "ДЕЕПРИЧАСТИЕ";
            }

            if (strPos.empty())
                continue;

            /* do some corrections (code taken from RusGramTab.ProcessPOSAndGrammems) */
            if (ResultedGrammems.HasAll(NSpike::AllCases | TGramBitSet(gAdjPronoun)))
                ResultedGrammems |= NSpike::AllGenders | NSpike::AllNumbers;

            if (ResultedGrammems.Has(gMasFem))
                ResultedGrammems |= TGramBitSet(gMasculine, gFeminine);

            if (!ResultedGrammems.Has(gPraedic) && ResultedGrammems.HasAll(NSpike::AllCases) && !ResultedGrammems.Has(gSingular))
                ResultedGrammems |= TGramBitSet(gSingular, gPlural);

            H.Grammems.AddForm(ResultedGrammems);
        }
}

bool CAnalyticFormBuilder::HasCompar(const CWord& W)
{
    for (CWord::SHomIt it = W.IterHomonyms(); it.Ok(); ++it)
        if (it->IsFullAdjective() && it->HasGrammem(gComparative))
            return true;

    return false;
}

bool CAnalyticFormBuilder::HasPredik(const CWord& W)
{
    for (CWord::SHomIt it = W.IterHomonyms(); it.Ok(); ++it)
        if (!it->Grammems.IsIndeclinable()) //  "бух","спасибо" не строятся с анал. формами
            if (it->HasGrammem(gPraedic))
                return true;

    return false;
}

bool CAnalyticFormBuilder::HasInfinitive(const CWord& W)
{
    for (CWord::SHomIt it = W.IterHomonyms(); it.Ok(); ++it)
        if (it->HasGrammem(gInfinitive))
            return true;

    return false;
}

DECLARE_STATIC_NAMES(TAnalyticFormPredicates, "давать становиться оказаться стать");

bool CAnalyticFormBuilder::AllHomonymsArePredicates(const CWord& W) const
{
    const TGramBitSet VerbGerundPraedic(gVerb, gGerund, gPraedic);
    for (CWord::SHomIt it = W.IterHomonyms(); it.Ok(); ++it) {
        if (it->HasAnyOfPOS(VerbGerundPraedic) || it->IsShortAdjectiveOrParticiple())
             continue;
        //verbs that can not be predicates or an.f.
        if (!TAnalyticFormPredicates::Has(it->Lemma))
            return false;
     }
    return true;
}

/*
  building of verb analytical forms: auxiliary can be "byt'" (to be) or "stat'" (to become) [translit]
*/
void CAnalyticFormBuilder::BuildAnalyticalVerbForms()
{
    //auxiliary verb number in chain of wordforms - the first part of an.f.
    int iBe = -1;
    //vector of potential predicates for second part
    CAnalyticalFormVars v_AnalyticalFormVars;
    BuildAnalyticalVerbFormsZaplata2();

    for (size_t WordNo = 0; WordNo < m_ClauseWords.size(); WordNo++) {
        const CWord& _Form = m_Words.GetWord(m_ClauseWords.at(WordNo));
        if (!_Form.IsAnalyticVerbForm()) {
            if (!m_ClauseWords.at(WordNo).m_bOriginalWord)
                continue;
            const Wtroka& s_lem = _Form.IterHomonyms()->Lemma;
            //looking for verbs "to be" or "to become"
            if (HasAnalyticalBe(_Form) && iBe == -1) {
                iBe = WordNo;
                continue;
            }

            if (HasInfinitive(_Form)) {
                v_AnalyticalFormVars.push_back(SAnalyticalFormVariant(WordNo, _Form.GetRusHomonymsCount(), SAnalyticalFormVariant::Infinitive, s_lem, AllHomonymsArePredicates(_Form)));
                continue;
            }

            if (HasPredik(_Form)) {
                v_AnalyticalFormVars.push_back(SAnalyticalFormVariant(WordNo, _Form.GetRusHomonymsCount(), SAnalyticalFormVariant::Predikative, s_lem, AllHomonymsArePredicates(_Form)));
                continue;
            }

            if (HasShortParticipleOrAdj(_Form)) {
                v_AnalyticalFormVars.push_back(SAnalyticalFormVariant(WordNo, _Form.GetRusHomonymsCount(), SAnalyticalFormVariant::Short_Form, s_lem, AllHomonymsArePredicates(_Form)));
                continue;
            }
            //looking for comparative form
            if (HasCompar(_Form))
                v_AnalyticalFormVars.push_back(SAnalyticalFormVariant(WordNo, _Form.GetRusHomonymsCount(), SAnalyticalFormVariant::Comp_Adj, s_lem, AllHomonymsArePredicates(_Form)));
        }
    }

    if (iBe != -1 && v_AnalyticalFormVars.size() > 0) {
            {
                //правило для цепочки {"быть"(буд.), предикатив (омонимичный), инфинитив (нс)}, тогда строим ан.ф. с инфинитивом
                //"Мальчик будет горько плакать"
                int DummyHomNo;
                bool bDummyAuxVHom;
                yvector<int> dummyVector;
                if    ((2 == v_AnalyticalFormVars.size())
                        &&    (iBe < v_AnalyticalFormVars[0].iWordNum)
                        &&  !v_AnalyticalFormVars[0].bAllHomPredik
                        &&  SAnalyticalFormVariant::Predikative == v_AnalyticalFormVars[0].ePos
                        &&  SAnalyticalFormVariant::Infinitive == v_AnalyticalFormVars[1].ePos

                        &&  IsAnalyticalVerbForm(iBe, v_AnalyticalFormVars[0].iWordNum, DummyHomNo, dummyVector, bDummyAuxVHom)
                        &&  IsAnalyticalVerbForm(iBe, v_AnalyticalFormVars[1].iWordNum, DummyHomNo, dummyVector, bDummyAuxVHom)
                    ) {
                        bool bFoundChain = true;
                        for (int mm = 0; mm < iBe; mm++) {
                            const CWord& _Form = m_Words.GetWord(m_ClauseWords.at(mm));
                            for (CWord::SHomIt it = _Form.IterHomonyms(); it.Ok(); ++it)
                                if (it->HasGrammem(gDative))
                                    bFoundChain = false;
                        }

                        if (bFoundChain)
                            // удаляем вариант омонимичного предикатива, если есть вариант будущего времени
                            v_AnalyticalFormVars.erase(v_AnalyticalFormVars.begin());
                    }

                //  конец  подслучая "Мальчик будет горько плакать"
            };

            //проверить на возможное вхождение компоненты аналитической формы в синтаксическую группу:
            //"был о нем"
            //CheckGroupsForAnalyticalVerbForms(v_AnalyticalFormVars, PrCl); : DIALING
            //в текущей версии синтаксиса именных групп покамест нет

            if (0 == v_AnalyticalFormVars.size()) return;

            /*разбираем случай "он был больше учителем, чем шофером", где не надо строить
              аналитическую форму со сравн. степенью, хотя, в принципе, ей ничего не мешает.

              Формально:
              Если у нас только одна гипотеза ("сравн. степени"), если после нее стоит существительное
              в творительном падеже, тогда  анал. форму строит не надо.
               Сокирко. 3 мая 2001
            */
            if (v_AnalyticalFormVars.size() == 1)
                if (SAnalyticalFormVariant::Comp_Adj == v_AnalyticalFormVars[0].ePos)
                    if (CheckComparativeZAPLATAForAnalyticalForm(v_AnalyticalFormVars[0].iWordNum))
                        return;

            //установить порядок на гипотезах (см. operator<);
            std::sort(v_AnalyticalFormVars.begin(), v_AnalyticalFormVars.end());

            for (size_t k = 0; k < v_AnalyticalFormVars.size(); k++)
                if (CheckAnalyticalVerbForm(iBe, v_AnalyticalFormVars[k].iWordNum)) {
                    //Stroka n_str = m_Words[iBe].m_strWord;
                    //n_str = n_str + "-" + m_Words[v_AnalyticalFormVars[k].iWordNum].m_strWord;
                    //m_Words[v_AnalyticalFormVars[k].iWordNum].SetWordStr(n_str);
                    //m_Words[v_AnalyticalFormVars[k].iWordNum].m_bAnalyticalForm = true;

                    break;
                }
    }
}

void CAnalyticFormBuilder::Run()
{
    BuildAnalyticalVerbForms();
}
