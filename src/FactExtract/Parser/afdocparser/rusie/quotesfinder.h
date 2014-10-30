#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>


class CQuotesFinder
{
    yvector<CSentence*>& m_vecSentence;
    const TArticleRef& m_MainArticle;
    bool m_bCreateDBFact;

    void AddFios(int SentNo, const yset<int>& QuoteWords, yvector<SFactAddress>& FioInQuotes);
    void AddFiosFromPeriod(int SentNo, const CWordsPair& Pair, yvector<SFactAddress>& FioInQuotes);

    void AddSimpleQuoteFact(const SValenciesValues& VerbCommunic, CSentenceRusProcessor* pSent, const Wtroka& QuoteStr, SLeadInfo LeadInfo);
    bool AddQuoteDBFact(SWordHomonymNum Subj, CSentenceRusProcessor* pSent, const Wtroka& QuoteStr, SLeadInfo LeadInfo, bool bSubject);
    bool CreateQuoteValue(const Wtroka& QuoteStr, CFactFields& factFields) const;
    bool CreateQuoteSubjectAsFio(SWordHomonymNum Subj,CSentenceRusProcessor* pSent, CFactFields& factFields);
    bool AddQuoteFact(const SValenciesValues& VerbCommunic, CSentenceRusProcessor* pSent, const Wtroka& QuoteStr,
                      const yvector<SFactAddress>& FioInQuotes, SLeadInfo LeadInfo);

    bool TryIndirectSpeech(const SValenciesValues& VerbCommunic, int SentNo);
    bool TryDirectSpeechWithDashes(const SValenciesValues& VerbCommunic, int SentNo);
    bool TryDirectSpeechWithColon(const SValenciesValues& VerbCommunic, int SentNo);
    int  FindBeginQuote(int SentNo) const;
    Wtroka FindRightQuoteIfHas(const CWordsPair& PeriodToPrint, int SentNo, const CWordsPair& GroupToExclude, yvector<SFactAddress>& FioInQuotes);

    bool FindCloseQuoteInNextSentences(int SentNo, int StartWordForFirstSentence, Wtroka& ResultToAdd,
                                       yvector<SFactAddress>& FioInQuotes, SLeadInfo& LeadInfo);
    CWordsPair GetWordIndexOfQuoteSituation(const SValenciesValues& VerbCommunic, int SentNo) const;
    void CreateTextField(const Stroka& TextFieldName, CSentence* pSent, const SWordHomonymNum& WH, CFactFields& factFields,
                         const Wtroka& Postfix);

    bool FindOpenQuoteInThisSentence(int SentNo, int DirectSpeechEndPosition, Wtroka& QuoteStr, yvector<SFactAddress>& FioInQuotes);
    CSentenceRusProcessor* GetSentPrc (int SentNo);
    const CSentenceRusProcessor* GetSentPrc (int SentNo) const;

public:
    CQuotesFinder(yvector<CSentence*>& vecSentence, const TArticleRef& article);

    bool FindQuotesFacts(int SentNo);
};
