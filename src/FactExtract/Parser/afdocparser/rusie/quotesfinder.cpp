#include "textprocessor.h"
#include "quotesfinder.h"
#include "normalization.h"

static const Wtroka COLON = Wtroka::FromAscii(":");


CQuotesFinder::CQuotesFinder(yvector<CSentence*>& vecSentence, const TArticleRef& article)
    : m_vecSentence(vecSentence)
    , m_MainArticle(article)
{
    m_bCreateDBFact = getenv("CREATE_ALL_QUOTES") == 0;
}

CSentenceRusProcessor*  CQuotesFinder::GetSentPrc (int SentNo)
{
    return (CSentenceRusProcessor*)m_vecSentence[SentNo];
}

const CSentenceRusProcessor*  CQuotesFinder::GetSentPrc (int SentNo) const
{
    return (CSentenceRusProcessor*)m_vecSentence[SentNo];
}

bool CQuotesFinder::CreateQuoteValue(const Wtroka& quoteStr, CFactFields& factFields) const
{
    DECLARE_STATIC_RUS_WORD(kChto, "что ");
    DECLARE_STATIC_RUS_WORD(kChtoby, "чтобы ");

    Wtroka qstr = quoteStr;

    CTextWS QuoteWS;
    if (qstr.has_prefix(kChto))
        qstr.erase(0,4);
    if (qstr.has_prefix(kChtoby))
        qstr.erase(0,6);
    if (qstr.empty())
         return false;
    qstr = StripString(qstr);
    if (qstr.size() > 1 && qstr[0] =='"' && qstr.back() =='"')
        qstr = qstr.substr(1, qstr.size() - 2);
    qstr = StripString(qstr);

    QuoteWS.AddLemma(SWordSequenceLemma(qstr));
    QuoteWS.SetArtificialPair(CWordsPair(0,0));
    factFields.AddValue(CFactFields::QuoteValue, QuoteWS);
    return true;
}

bool CQuotesFinder::CreateQuoteSubjectAsFio(SWordHomonymNum Subj, CSentenceRusProcessor* pSent, CFactFields& factFields)
{
    if (!Subj.IsValid()) return false;
    const CHomonym& h = pSent->m_Words[Subj];
    const CFioWordSequence* pFioWS = dynamic_cast<const CFioWordSequence*>(h.GetSourceWordSequence());
    // если это не фио - пропускаем
    if (pFioWS != NULL) {
        CFioWS newFioWS(*pFioWS);
        newFioWS.SetMainWord(Subj);
        factFields.AddValue(CFactFields::Fio, newFioWS);
        return true;
    } else {
        const CFactsWS* pFactWS = dynamic_cast<const CFactsWS*>(h.GetSourceWordSequence());
        if (pFactWS)
            for (int i=0; i < pFactWS->GetFactsCount(); i++) {
                const CFactFields& f  = pFactWS->GetFact(i);
                const CFioWS* FioWS = f.GetFioValue(CFactFields::Fio);
                if (FioWS != 0) {
                    factFields.AddValue(CFactFields::Fio, *FioWS);
                    return true;
                }

            };
    }
    return false;
}

bool CQuotesFinder::AddQuoteDBFact(SWordHomonymNum Subj, CSentenceRusProcessor* pSent, const Wtroka& QuoteStr,
                                   SLeadInfo LeadInfo, bool bSubject)
{
    CFactFields factFields("Quote");
    factFields.m_LeadInfo = LeadInfo;

    if (!CreateQuoteSubjectAsFio(Subj, pSent, factFields))
        return false;

    if (!CreateQuoteValue(QuoteStr, factFields)) return false;

    CBoolWS bWs(bSubject);
    bWs.SetPair(0,0);
    factFields.AddValue(CFactFields::IsSubject, bWs);

    CFactsWS* factWS = new CFactsWS();
    if (!pSent->m_Words[Subj].GetSourceWordSequence())
        ythrow yexception() << "CQuotesFinder::AddQuoteDBFact: no pSent->m_Words[Subj].GetSourceWordSequence() ";
    factWS->SetPair(*pSent->m_Words[Subj].GetSourceWordSequence());
    factWS->PutArticle(m_MainArticle);
    factWS->AddFact(factFields);
    factWS->SetMainWord(Subj);
    pSent->GetMultiWordCreator().TakeMultiWord(factWS);
    return true;
}

void CQuotesFinder::CreateTextField(const Stroka& TextFieldName, CSentence* pSent, const SWordHomonymNum& WH,
                                    CFactFields& factFields, const Wtroka& Postfix)
{
    CNormalization InterpNorm(pSent->m_Words);
    const CHomonym& h = pSent->m_Words[WH];

    const CFioWordSequence* pFioWS = dynamic_cast<const CFioWordSequence*>(h.GetSourceWordSequence());
    if (pFioWS != NULL) {
        CFioWS newFioWS(*pFioWS);
        newFioWS.SetMainWord(WH);
        newFioWS.AddLemma(SWordSequenceLemma (InterpNorm.GetArtificialLemma(WH)));

        CTextWS newTextWS;
        newTextWS.SetPair(*pFioWS);
        newTextWS.SetMainWord(WH);
        newTextWS.ResetLemmas(newFioWS.GetLemmas(), true);
        if (!Postfix.empty())
            newTextWS.AddLemma(SWordSequenceLemma(Postfix));
        factFields.AddValue(TextFieldName, newTextWS);
    } else {
        const CFactsWS* pFactWS = dynamic_cast<const CFactsWS*>(h.GetSourceWordSequence());
        if (pFactWS) {
            CTextWS newTextWS;
            newTextWS.SetPair(*pFactWS);
            newTextWS.SetMainWord(WH);
            newTextWS.ResetLemmas(pFactWS->GetLemmas(), true);
            if (!Postfix.empty())
                newTextWS.AddLemma(SWordSequenceLemma(Postfix));
            factFields.AddValue(TextFieldName, newTextWS);
        } else {
            const CTextWS* pTextWS = dynamic_cast<const CTextWS*>(h.GetSourceWordSequence());
            if (pTextWS) {
                CTextWS newTextWS = *pTextWS;
                if (!Postfix.empty())
                    newTextWS.AddLemma(SWordSequenceLemma(Postfix));
                factFields.AddValue(TextFieldName, newTextWS);
            } else {
                if (h.GetSourceWordSequence() == 0 || h.GetSourceWordSequence()->Size() == 1) {
                    CTextWS newTextWS;
                    newTextWS.SetPair(pSent->m_Words.GetWord(WH).GetSourcePair());
                    newTextWS.SetMainWord(WH);
                    newTextWS.AddLemma(SWordSequenceLemma(h.GetLemma()));
                    if (!Postfix.empty())
                        newTextWS.AddLemma(SWordSequenceLemma(Postfix));
                    factFields.AddValue(TextFieldName, newTextWS);
                }
            }
        }
    }
}

// функция добавляет факт цитирования во множество фактов,
// на вход приходит  ситуация цитирования (VerbCommunic), из которой берется субъект цитирования,
// и QuoteStr - содержимое цитаты
void  CQuotesFinder::AddSimpleQuoteFact(const SValenciesValues& VerbCommunic, CSentenceRusProcessor* pSent,
                                        const Wtroka& QuoteStr, SLeadInfo LeadInfo)
{

    DECLARE_STATIC_RUS_WORD(kOB, "ОБ");
    DECLARE_STATIC_RUS_WORD(kSUB, "СУБ");
    DECLARE_STATIC_RUS_WORD(kPO_SLOVAM, "ПО_СЛОВАМ");

    CFactFields factFields("Quotes");
    factFields.m_LeadInfo = LeadInfo;
    Wtroka ObjStr;
    {
        SWordHomonymNum Obj = VerbCommunic.GetValue(kOB);
        if (Obj.IsValid()) {
            const CHomonym& h = pSent->m_Words[Obj];
            ObjStr = h.GetLemma();
        }

        // фунция берет из ситуации VerbCommunic валентность СУБ и пытается построить по ней строку,
        // если это не выходит, выходим
        SWordHomonymNum Subj = VerbCommunic.GetValue(kSUB);
        if (!Subj.IsValid())
            return;
        else
            CreateTextField("Fio",pSent,Subj, factFields, Wtroka());
    }
    {
        SWordHomonymNum Predic = VerbCommunic.m_NodeWH;
        if (!Predic.IsValid())
            Predic = VerbCommunic.GetValue(kPO_SLOVAM);
        if (Predic.IsValid())
            CreateTextField("QuotePredicate", pSent, Predic, factFields, ObjStr);
    }

    if (!CreateQuoteValue(QuoteStr, factFields))
        return;

    CFactsWS* factWS = new CFactsWS();
    factWS->SetPair(0,0);
    factWS->PutArticle(m_MainArticle);
    factWS->AddFact(factFields);
    pSent->GetWordSequences().push_back(factWS);
};

bool CQuotesFinder::AddQuoteFact(const SValenciesValues& VerbCommunic, CSentenceRusProcessor* pSent,
                                 const Wtroka& QuoteStr, const yvector<SFactAddress>& FioInQuotes, SLeadInfo LeadInfo)
{
    Wtroka qstr = StripString(QuoteStr);
    if (qstr.size() < 3)
        return false;

    if (!(LeadInfo.m_iLastSent == -1 && LeadInfo.m_iFirstSent == -1) &&
        !(LeadInfo.m_iFirstSent >= 0 && LeadInfo.m_iLastSent >= 0))
        ythrow yexception() << "CQuotesFinder::AddQuoteFact : bad lead info";

    if (m_bCreateDBFact) {
        DECLARE_STATIC_RUS_WORD(kSUB, "СУБ");
        if (!AddQuoteDBFact(VerbCommunic.GetValue(kSUB), pSent, qstr, LeadInfo, true))
            return false;

        for (size_t i=0; i < FioInQuotes.size(); i++)
            if (!AddQuoteDBFact(FioInQuotes[i], GetSentPrc(FioInQuotes[i].m_iSentNum), qstr, LeadInfo, false))
                return false;
    } else
        AddSimpleQuoteFact(VerbCommunic, pSent, qstr, LeadInfo);
    return true;
}

bool LessBySize(const CClause* C1, const CClause* C2)
{
    return C1->Size() < C2->Size();
};

void CQuotesFinder::AddFios(int SentNo, const yset<int>& QuoteWords, yvector<SFactAddress>& FioInQuotes)
{
    CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);
    yvector<SWordHomonymNum>* fioWHs = pSentPrc->GetMultiWordCreator().GetFoundWords(GlobalDictsHolder->BuiltinKWTypes().Fio, KW_DICT);
    if (fioWHs)
        for (size_t i=0; i < fioWHs->size(); i++) {
            const CHomonym& pHom =  pSentPrc->m_Words[(*fioWHs)[i] ];
            if (QuoteWords.find(pHom.GetSourceWordSequence()->FirstWord()) != QuoteWords.end()) {
                SFactAddress A;
                A.m_HomNum = (*fioWHs)[i].m_HomNum;
                A.m_WordNum = (*fioWHs)[i].m_WordNum;
                A.m_bOriginalWord = (*fioWHs)[i].m_bOriginalWord;
                A.m_iSentNum = SentNo;
                FioInQuotes.push_back(A);
            }
        }

};

void CQuotesFinder::AddFiosFromPeriod(int SentNo, const CWordsPair& Pair, yvector<SFactAddress>& FioInQuotes)
{
    CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);
    yvector<SWordHomonymNum>* fioWHs = pSentPrc->GetMultiWordCreator().GetFoundWords(GlobalDictsHolder->BuiltinKWTypes().Fio, KW_DICT);
    if (fioWHs)
        for (size_t i=0; i < fioWHs->size(); i++) {
            const CHomonym& pHom =  pSentPrc->m_Words[(*fioWHs)[i] ];
            if (Pair.Includes(*pHom.GetSourceWordSequence())) {
                SFactAddress A;
                A.m_HomNum = (*fioWHs)[i].m_HomNum;
                A.m_WordNum = (*fioWHs)[i].m_WordNum;
                A.m_bOriginalWord = (*fioWHs)[i].m_bOriginalWord;
                A.m_iSentNum = SentNo;
                FioInQuotes.push_back(A);
            }
        }
}

// обычно функция возвращает clause->ToString(),
// если в клаузе есть открывающая кавычка, а нет закрывающей, тогда  функция добавляет к результату
// все слова до закрывающей кавычки
Wtroka CQuotesFinder::FindRightQuoteIfHas(const CWordsPair& PeriodToPrint, int SentNo,
                                          const CWordsPair& GroupToExclude, yvector<SFactAddress>& FioInQuotes)
{
    FioInQuotes.clear();
    CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);
    yset<int> QuoteWords;
    Wtroka ClauseStr;
    static const Wtroka COMMA = Wtroka::FromAscii(",");
    for (int i = PeriodToPrint.FirstWord(); i <=  PeriodToPrint.LastWord(); i++) {
        const CWord& w = pSentPrc->m_Words.GetOriginalWord(i);
        if (!GroupToExclude.Contains(i) &&
            !(GroupToExclude.Contains(i - 1) && w.GetText() == COMMA) &&
            !(GroupToExclude.Contains(i + 1) && w.GetText() == COMMA)) {
            ClauseStr += w.GetOriginalText() + ' ';
            QuoteWords.insert(i);
        }
    }

    size_t index = ClauseStr.find('"');
    if (index != Wtroka::npos && ClauseStr.rfind('"') == index) {
        yset<int> AddQuoteWords;
        Wtroka Add;
        // если только одна кавычка, то пойдем искать вторую кавычку
        int i;
        for (i = PeriodToPrint.LastWord()+1; i < (int)pSentPrc->getWordsCount(); i++) {
            const CWord& w = pSentPrc->m_Words.GetOriginalWord(i);
            if (!GroupToExclude.Contains(i)) {
                Add += w.GetOriginalText() + ' ';
                AddQuoteWords.insert(i);
            }
            if (w.HasCloseQuote())
                break;
        }
        if (i != (int)pSentPrc->getWordsCount()) {
            ClauseStr += ' ';
            ClauseStr += Add;
            QuoteWords.insert(AddQuoteWords.begin(), AddQuoteWords.end());
        }

    }

    AddFios(SentNo, QuoteWords, FioInQuotes);

    return ClauseStr;
};

//выдает абсолютный адрес найденной ситуации цитирования
CWordsPair CQuotesFinder::GetWordIndexOfQuoteSituation(const SValenciesValues& VerbCommunic, int SentNo) const
{
    CWordsPair PairToSearch;
    PairToSearch.SetPair(-1,-1);
    const CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);
    if (VerbCommunic.m_NodeWH.IsValid()) {
        // если валидна вершина,берем координаты веришины
        PairToSearch = pSentPrc->m_Words.GetWord(VerbCommunic.m_NodeWH).GetSourcePair();
    } else {
        // иначе  берем координаты первой валентности
        if (VerbCommunic.m_values.empty() || !VerbCommunic.m_values[0].m_Value.IsValid())
            return CWordsPair();
        PairToSearch = pSentPrc->m_Words.GetWord(VerbCommunic.m_values[0].m_Value).GetSourcePair();
    };
    return PairToSearch;
};

// проверяет, содержит ли ситуация VerbCommunic косвенную речь в качестве актанта-клаузы. Примеры:
// "Петров сказал, что мы уходим в Яндекс."
// По словам Сергея Крапина , я уехал
// Она призналась , что "придумали проект наружной рекламы , но притормозили его , не захотев оказаться" .
bool CQuotesFinder::TryIndirectSpeech(const SValenciesValues& VerbCommunic, int SentNo)
{
    CWordsPair PairToSearch = GetWordIndexOfQuoteSituation(VerbCommunic, SentNo);
    if (PairToSearch.FirstWord() == -1) return false;

    const_clause_it min_it = GetSentPrc(SentNo)->GetClauseStructureOrBuilt().GetMinimalClauseThatContains(PairToSearch);
    CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);
    const CClauseVariant& ClauseVariant = pSentPrc->GetClauseStructureOrBuilt();
    if (min_it == ClauseVariant.GetEndClause())
        return false;

    // пропускаем непервые клаузы, которые заканчиваются на тире, поскольку, это может быть только прямая речь.
    if ((**min_it).FirstWord() != 0 && pSentPrc->getWordRus((**min_it).LastWord())->GetText() == Wtroka::FromAscii("-"))
        return false;

    // вычисляем союз по названию статьи, например, из "_глагол_со_чтобы" получаем "чтобы"
    Wtroka strValencySubConj;
    {
        DECLARE_STATIC_RUS_WORD(kChto, "что");
        DECLARE_STATIC_RUS_WORD(kChtoby, "чтобы");
        Wtroka t = VerbCommunic.m_pArt->get_title();
        if (t.length() > 4 && t[t.length() - 4] == '_' && t.has_suffix(kChto))
            strValencySubConj = kChto;
        else if (t.length() > 6 && t[t.length() - 6] == '_' && t.has_suffix(kChtoby))
            strValencySubConj = kChtoby;
    }
    // если справа  стоит клауза со "что"
    if (!strValencySubConj.empty()) {
        yvector<const CClause*> SubClauses;
        ClauseVariant.GetIncludedClauses(min_it, SubClauses);
        std::sort(SubClauses.begin(), SubClauses.end(), LessBySize);
        {
            // берем первую справа
            const_clause_it next_it;
            ClauseVariant.GetNeighbourClauseR(min_it, next_it);
            if (next_it != ClauseVariant.GetEndClause())
                SubClauses.push_back(*next_it);
        }
        // берем вложенную клаузу справа (это лучший вариант)
        {
            const CClause* pSubClause = ClauseVariant.GetIncludedClauseR(min_it);
            if (pSubClause) SubClauses.push_back(pSubClause);
        }

        for (int i=SubClauses.size()-1; i >=0; i--) {
            const CClause* pSubClause = SubClauses[i];
            if (pSubClause->FromRight(PairToSearch))
                for (int ConjNo=0; ConjNo < pSubClause->GetConjsCount(); ConjNo++) {
                    const CHomonym& Conj = pSubClause->GetConj(ConjNo);
                    const article_t* piConjArt = GlobalDictsHolder->GetAuxArticle(Conj, CONJ_DICT);
                    if (piConjArt->get_title().substr(0, strValencySubConj.length()) == strValencySubConj) {
                        yvector<SFactAddress> FioInQuotes;
                        Wtroka QuoteStr = FindRightQuoteIfHas(*pSubClause, SentNo, CWordsPair(), FioInQuotes);
                        if (AddQuoteFact(VerbCommunic, pSentPrc, QuoteStr, FioInQuotes, SLeadInfo()))
                            return true;
                    }
                };
        }
    }

    // если клауза является вводной
    DECLARE_STATIC_RUS_WORD(kPo_slovam_Ivanova, "по_словам_иванова");
    DECLARE_STATIC_RUS_WORD(kKak, "как");
    if  ((VerbCommunic.m_pArt->get_title() == kPo_slovam_Ivanova &&  (*min_it)->HasType(Parenth)) ||
    // или использована с союзом "как", например, "как рассказал Петров, мы уже ушли"
         (pSentPrc->getWordRus((*min_it)->FirstWord())->FindLemma(kKak) && ClauseVariant.GetIncludedClauseL(min_it) == 0)) {
        // если вводная калуза стоит на первом месте и в предложении нет других клауз, тогда берем
        // все предложение начиная от конца вводной клаузы до
        if ((**min_it).FirstWord() == 0) {
            yvector<SFactAddress> FioInQuotes;
            Wtroka QuoteStr = FindRightQuoteIfHas(CWordsPair((**min_it).LastWord(), pSentPrc->getWordsCount() - 1),
                                                  SentNo, **min_it, FioInQuotes);
            if (AddQuoteFact(VerbCommunic, pSentPrc, QuoteStr, FioInQuotes, SLeadInfo()))
                return true;

        };
        // берем главную клаузу
        const CClause* pMainClause = ClauseVariant.GetMainClause(min_it);
        if (!pMainClause) {
            const_clause_it next_it;
            ClauseVariant.GetNeighbourClauseR(min_it, next_it);
            if (next_it != ClauseVariant.GetEndClause())
                pMainClause = *next_it;

        }
        if (pMainClause) {
            // печатаем главную клаузу без вложенной
            yvector<SFactAddress> FioInQuotes;
            Wtroka QuoteStr = FindRightQuoteIfHas(*pMainClause, SentNo, **min_it, FioInQuotes);
            if (AddQuoteFact(VerbCommunic, pSentPrc, QuoteStr,FioInQuotes, SLeadInfo()))
                return true;
        }
    }

    return false;
}

// находит открывающую кавычку в предыдущих трех предложениях, если таковой нет возвращает -1
int CQuotesFinder::FindBeginQuote(int SentNo) const
{
    for (int i=SentNo; i>=0 && (i>=SentNo-3); i--) {
        const CSentence* pSent = GetSentPrc(i);
        if (pSent->getWordRus(0)->HasOpenQuote())
            return i;
    };
    return  -1;
};

// ищет закрывающую кавычку в текущем или последующем предложениях,
// добавляет все слова до закрывающей кавычки к ResultToAdd
// игнорируем внутренние кавычки
bool CQuotesFinder::FindCloseQuoteInNextSentences(int StartSentNo, int StartWordForFirstSentence, Wtroka& ResultToAdd,
                                                  yvector<SFactAddress>& FioInQuotes, SLeadInfo& LeadInfo)
{
    Wtroka Add;
    yvector<SFactAddress> AddFioInQuotes;
    int Depth = 1;
    for (int SentNo=StartSentNo; (SentNo-StartSentNo<=2)&& (SentNo< (int)m_vecSentence.size()); SentNo++) {
        CSentenceRusProcessor* pSent = GetSentPrc(SentNo);
        yset<int> QuoteWords;
        int k=0;
        if (SentNo == StartSentNo)
            k = StartWordForFirstSentence;
        for (; k < (int)pSent->getWordsCount(); k++) {
            const CWord& w = *pSent->getWordRus(k);
            Add += w.GetOriginalText() + ' ';
            QuoteWords.insert(k);
            bool bHasCloseQuote = false;
            if (w.HasOpenQuote()) {
                if (ispunct(w.GetText()[0])) {
                    Depth--;                    // одиночную кавычку  знака препинания считаем закр. кавычкой  (кто-то по ошибке поставил пробел)
                    bHasCloseQuote = true;;
                } else
                    Depth++;
            }
            if (w.HasCloseQuote()) {
                Depth--;
                if ((Depth == 1) // Закрывающая кавычка в конце предложения закрывает все открытые кавычки.
                        &&  ((k+1 == (int)pSent->getWordsCount())
                                ||      ((k+2 == (int)pSent->getWordsCount())
                                            &&  pSent->getWordRus(k+1)->IsPunct()
                                        )
                            )
                    ) {
                    Depth  = 0;
                }
                bHasCloseQuote = true;
            }
            if (w.GetText() == Wtroka::FromAscii("\"")) {
                if (k+1 == (int)pSent->getWordsCount() || pSent->getWordRus(k+1)->IsPunct()) {
                    bHasCloseQuote = true;
                    Depth--;
                }
            }
            if (bHasCloseQuote && Depth == 0) {
                if (!Add.empty() && Add[0] == '-') {
                    const wchar16* beg = Add.begin() + 1;
                    StripRangeBegin(beg, Add.end());
                    Add.assign(beg, Add.end() - beg);
                }
                if (ResultToAdd.back() == ',' && Add.size() > 0 && NStr::IsLangAlpha(Add[0], TMorph::GetMainLanguage()) && ::IsUpper(Add[0])) {
                    ResultToAdd.erase(ResultToAdd.size() - 1);
                    ResultToAdd += CharToWide(". ");
                }

                ResultToAdd += Add;
                AddFios(SentNo, QuoteWords, AddFioInQuotes);
                FioInQuotes.insert(FioInQuotes.end(), AddFioInQuotes.begin(), AddFioInQuotes.end());
                LeadInfo.m_iLastSent = SentNo;
                return true;
            }
        }
        AddFios(SentNo, QuoteWords, AddFioInQuotes);
    }
    return false;
}

// находит открывающую кавычку в текущем предложении;
// учитывается, что закавыченный фрагмент может содержать другие кавычки.
bool CQuotesFinder::FindOpenQuoteInThisSentence(int SentNo, int DirectSpeechEndPosition, Wtroka& QuoteStr,
                                                yvector<SFactAddress>& FioInQuotes)
{
    CSentenceRusProcessor* pSent = GetSentPrc(SentNo);
    int Depth = 0;
    // если нет закрывающей кавычки, значит DirectSpeechEndPosition, указывает на место, где прямая речь
    // только прерывается, например:
    // "Насколько мне известно , - говорит заместитель генерального директора завода Сергей Никульцев , - водку под названием "Русская" сейчас не производит ни один крупный завод" .
    if (!pSent->getWordRus(DirectSpeechEndPosition)->HasCloseQuote())
        Depth--;
    else {
        // если есть закрывающая и открывающая кавычки, значит, скорее всего,
        // DirectSpeechEndPosition, указывает на закавыченное слово. Мы не будем считать  в таком случае, что
        // открывающая кавычка слова - есть открывающая кавычка прямой речи, например:
        // "Мост IKEA заслонит вид, открывающийся с Ленинградского шоссе на памятник "Ежи" , - говорит Игорь Гончаренко.
        if (DirectSpeechEndPosition > 0 && pSent->getWordRus(DirectSpeechEndPosition)->HasOpenQuote())
            Depth++;
    }
    for (int k = DirectSpeechEndPosition; k >=0; k--) {
        const CWord& w = *pSent->getWordRus(k);
        if (w.HasCloseQuote())
            Depth--;
        bool bHasOpenQuote = w.HasOpenQuote() || w.GetText() == Wtroka::FromAscii(":\"");
        if (bHasOpenQuote)
            Depth++;
        if ((Depth == 0) && bHasOpenQuote) {
            CWordsPair p(k, DirectSpeechEndPosition);
            QuoteStr += pSent->ToString(p);
            AddFiosFromPeriod(SentNo,p, FioInQuotes);

            return true;
        }
    }
    return false;

}

// проверяет, содержит ли ситуация VerbCommunic прямую речь в качестве актанта-клаузы, примеры:
// "Я счастлив", - сказал Петя.
// "Я счастлив. Я очень счастлив", - сказал Петя.
// "Я счаcтлив, - сказал Петя. - Я очень счастлив!"
bool CQuotesFinder::TryDirectSpeechWithDashes(const SValenciesValues& VerbCommunic, int SentNo)
{
    if (!VerbCommunic.m_NodeWH.IsValid())
        return false;

    CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);

    const CWordsPair PairToSearch = GetWordIndexOfQuoteSituation(VerbCommunic, SentNo);
    if (PairToSearch.FirstWord() == -1) return false;

    int EndDirectSpeech = PairToSearch.FirstWord();

    {
        bool bFound = false;
        // ищем тире  на расстоянии 5 слов от глагола
        for (int i=EndDirectSpeech-1;  i>0 && EndDirectSpeech-i<5; i--) {
            if (pSentPrc->getWordRus(i)->IsDash()) {
                EndDirectSpeech = i;
                bFound = true;
                break;
            }
        }

        if (!bFound)
            return false;
    }

    EndDirectSpeech--;
    if (EndDirectSpeech < 0 ||!pSentPrc->getWordRus(EndDirectSpeech)->IsComma())
        return false;

    EndDirectSpeech--;
    bool bHasCloseQuote = true;
    if (EndDirectSpeech < 0 ||!pSentPrc->getWordRus(EndDirectSpeech)->HasCloseQuote()) {
        bHasCloseQuote = false;
        EndDirectSpeech ++;
    }

    yvector<SFactAddress> FioInQuotes;
    Wtroka QuoteStr;
    SLeadInfo LeadInfo;
    LeadInfo.m_iFirstSent = SentNo;
    LeadInfo.m_iLastSent = SentNo;

    if (FindOpenQuoteInThisSentence(SentNo, EndDirectSpeech, QuoteStr, FioInQuotes)) {
    } else {
        // ищем открывающую кавычку в другом предложении
        int StartSentNo = FindBeginQuote(SentNo);
        if (StartSentNo == -1)
            return false;
        LeadInfo.m_iFirstSent = StartSentNo;
        for (int i=StartSentNo; i+1<=SentNo; i++) {
            CSentenceRusProcessor* pSnt = GetSentPrc(i);
            QuoteStr += pSnt->ToString();
            QuoteStr += ' ';
            AddFiosFromPeriod(i, CWordsPair(0, (int)pSnt->getWordsCount() - 1),FioInQuotes);
        }
        LeadInfo.m_iLastSent = SentNo;
        CWordsPair p(0, EndDirectSpeech);
        QuoteStr += pSentPrc->ToString(p);
        AddFiosFromPeriod(SentNo, p, FioInQuotes);
    }

    if (!bHasCloseQuote) {
        // берем клаузу
        const_clause_it min_it = GetSentPrc(SentNo)->GetClauseStructureOrBuilt().GetMinimalClauseThatContains(PairToSearch);
        const CClauseVariant& ClauseVariant = pSentPrc->GetClauseStructureOrBuilt();
        if (min_it == ClauseVariant.GetEndClause())
            return false;

        // проверяем, что клауза с  ситуацией в предложении последняя
        if ((**min_it).LastWord()+1 == (int)pSentPrc->getWordsCount()) {
            // ищет закрывающую кавычку в следующем предложении
            if (!FindCloseQuoteInNextSentences(SentNo+1, 0, QuoteStr, FioInQuotes, LeadInfo))
                return false;
        } else if (!FindCloseQuoteInNextSentences(SentNo, (**min_it).LastWord(), QuoteStr, FioInQuotes, LeadInfo))
            return false;
    }

    return AddQuoteFact(VerbCommunic, pSentPrc, QuoteStr, FioInQuotes, LeadInfo);
}

// он сказал : "У меня было право - я исполнял обязанности премьера" .
// он сказал : " У меня было право - я исполнял обязанности премьера" .
bool CQuotesFinder::TryDirectSpeechWithColon(const SValenciesValues& VerbCommunic, int SentNo)
{
    if (!VerbCommunic.m_NodeWH.IsValid())
        return false;

    CSentenceRusProcessor* pSentPrc = GetSentPrc(SentNo);

    const CWordsPair PairToSearch = GetWordIndexOfQuoteSituation(VerbCommunic, SentNo);
    if (PairToSearch.FirstWord() == -1) return false;

    const_clause_it min_it =  GetSentPrc(SentNo)->GetClauseStructureOrBuilt().GetMinimalClauseThatContains(PairToSearch);

    const CClauseVariant& ClauseVariant = pSentPrc->GetClauseStructureOrBuilt();
    if (min_it == ClauseVariant.GetEndClause())
        return false;

    const CClause* pEnclosedClause = ClauseVariant.GetIncludedClauseR(min_it);
    CWordsPair QuoteValuePair;
    if (pEnclosedClause && pEnclosedClause->FirstWord() != 0) {
        const Wtroka& WordStr = pSentPrc->getWordRus(pEnclosedClause->FirstWord() - 1)->GetText();
        if (WordStr != COLON &&
            (WordStr != Wtroka::FromAscii("\"") || pEnclosedClause->FirstWord() < 2 ||
             pSentPrc->getWordRus(pEnclosedClause->FirstWord() - 2)->GetText() != COLON))
            return false;

        QuoteValuePair = *pEnclosedClause;
    } else {
        int LastWord = (**min_it).LastWord();
        const Wtroka& WordStr = pSentPrc->getWordRus(LastWord)->GetText();
        if (WordStr != COLON &&
            (WordStr != Wtroka::FromAscii("\"") || LastWord < 1 ||
             pSentPrc->getWordRus(LastWord - 1)->GetText() != COLON))
            return false;

        if (LastWord + 1 == (int)pSentPrc->getWordsCount())
            return false;
        QuoteValuePair = CWordsPair(LastWord+1, (int)pSentPrc->getWordsCount() - 1);
    }
    yvector<SFactAddress> FioInQuotes;
    AddFiosFromPeriod(SentNo, QuoteValuePair, FioInQuotes);
    return AddQuoteFact(VerbCommunic, pSentPrc, pSentPrc->ToString(QuoteValuePair), FioInQuotes, SLeadInfo());

}

bool CQuotesFinder::FindQuotesFacts(int SentNo)
{
    //try
    {
        const yvector<SValenciesValues>* pValValues =
            GetSentPrc(SentNo)->GetFoundVals(SArtPointer(GlobalDictsHolder->BuiltinKWTypes().CommunicVerb));
        if (pValValues != NULL) {
            yset<int> Roots;
            for (size_t i=0; i < pValValues->size(); i++) {
                const SValenciesValues& VerbCommunic = (*pValValues)[i];
                // каждую вершину обрабатываем только один раз
                if (Roots.find(VerbCommunic.m_NodeWH.m_WordNum) != Roots.end()) continue;

                if (TryIndirectSpeech(VerbCommunic, SentNo)) {
                    //int UU =0;        //unused!;
                } else if (TryDirectSpeechWithDashes(VerbCommunic, SentNo)) {
                        break;
                    } else if (TryDirectSpeechWithColon(VerbCommunic, SentNo)) {
                            break;
                        }

                Roots.insert(VerbCommunic.m_NodeWH.m_WordNum);

            }
        }
    }
    /*catch (...)
    {
        Stroka strError;
        strError.Format("exception in CQuotesFinder::FindQuotesFacts; sentence N %i", SentNo);
        ythrow yexception() << (const char*)strError;

    }*/
    return true;
};

void CFactsCollection::GroupByQuotes()
{
    //yvector<const SetOfFactsIt> Quotes;
    //for ( SetOfFactsIt it = m_UniqueFacts.begin(); it != m_UniqueFacts.end(); it++ )
    //{
    //  CFactFields& rFact = m_pText->GetFact(*it);
    //  if (rFact.GetFactName() != "QuoteDB") continue;
    //  CTextWS* pTextWs = rFact.GetTextValue(CFactFields::QuoteValue);
    //  if (pTextWs == 0)
    //      throw yexception("CFactsCollection::GroupByQutes(): bad quote fact structure(1)");
    //  CQuoteWS* pQuoteWs = dynamic_cast<CQuoteWS*>(pTextWs);
    //  if (pQuoteWs == 0)
    //      throw yexception("CFactsCollection::GroupByQutes(): bad quote fact structure(2)");
    //  Quotes.push_back(it);
    //}

    //for (size_t i=0; i < Quotes.size(); i++)
    //{
    //  const CFactFields& rFact1 = m_pText->GetFact(*Quotes[i]);
    //  CQuoteWS* pQuoteWs1 = (CQuoteWS*)rFact1.GetTextValue(CFactFields::QuoteValue);
    //  for (size_t j=0; j < Quotes.size(); j++)
    //  {
    //      CFactFields& rFact2 = m_pText->GetFact(*Quotes[j]);
    //      CQuoteWS* pQuoteWs2 = (CQuoteWS*)rFact2.GetTextValue(CFactFields::QuoteValue);
    //      if (pQuoteWs2->m_QuoteId == pQuoteWs1->m_QuoteId)
    //      {
    //          SFactAddressAndFieldName s1;
    //          s1.m_FactAddr = *Quotes[i];
    //          s1.m_strFieldName = "QuoteValue";

    //          SFactAddressAndFieldName s2;
    //          s2.m_FactAddr = *Quotes[j];
    //          s2.m_strFieldName = "QuoteValue";

    //          SEqualFactsPair Pair(SameEntry, EqualByFio, s1, s2);
    //          m_EqualFactPairs.insert(Pair);
    //      }
    //  };

    //}
}

