#include "sentence.h"
#include "dateprocessor.h"
#include "namefinder.h"

#include <FactExtract/Parser/common/utilit.h>
#include <util/generic/cast.h>


CSentence::~CSentence(void)
{
    FreeData();
}

void CSentence::FreeData()
{
    m_Words.freeData();
}

//////////////////////////////////////////////////////////////////////////////
// pure virtual overwrites

void CSentence::AddWord(CWordBase* pWord)
{
    CWord* pWordRus = CheckedCast<CWord*>(pWord);
    m_words.push_back(pWordRus);
    m_Words.AddOriginalWord(pWordRus);
}

//CWord* CSentence::createWord(const CPrimGroup &group, const Wtroka& txt)
//{
//    return new CWord(group, txt);
//}

CWord* CSentence::getWordRus(size_t iNum) const
{
    if (iNum >= m_words.size())
        ythrow yexception() << "Bad iNum in CSentence::getWord()";
    return m_words[iNum].Get();
}

CWordBase* CSentence::getWord(size_t iNum) const
{
    return getWordRus(iNum);
}

bool CSentence::AddMarkUp(const SLinkRefMarkUp& linkRefMarkUp)
{
    size_t wc = getWordsCount();
    if (wc == 0)
        return false;
    CWord* pWLast = getWordRus(wc - 1);
    CWord* pWFirst = getWordRus(0);
    if (linkRefMarkUp.m_iBeg > (int)pWLast->m_pos)
        return false;
    if (linkRefMarkUp.m_iEnd < (int)pWFirst->m_pos + (int)pWFirst->m_len)
        return true;

    SSentMarkUp sentMarkUp;
    for (size_t i = 0; i < wc; i++) {
        CWord* pW = getWordRus(i);
        if ((int)pW->m_pos >= linkRefMarkUp.m_iBeg && sentMarkUp.FirstWord() == -1)
            sentMarkUp.SetPair(i,i);

        if ((int)pW->m_pos >= linkRefMarkUp.m_iEnd && sentMarkUp.FirstWord() < (int)i &&
            sentMarkUp.LastWord() == sentMarkUp.FirstWord()) {
            sentMarkUp.SetLastWord(i-1);
            break;
        }
    }

    bool bRes = true;
    if ((int)pWLast->m_pos < linkRefMarkUp.m_iEnd) {
        sentMarkUp.SetLastWord(wc - 1);
        bRes = false;
    }

    sentMarkUp.m_strInfo = linkRefMarkUp.m_strRef;
    sentMarkUp.m_Type = linkRefMarkUp.m_Type;
    m_SentMarkUps.insert(sentMarkUp);
    return bRes;
}

void CSentence::AddNumber(CNumber& number)
{
    m_Numbers.push_back(number);
}

Stroka CSentence::ToHTMLColorString(const yvector<CWordsPair>& wp, const Stroka& sColor, ECharset encoding) const
{
    Stroka str;
    for (size_t i = 0; i + 1 <= m_words.size(); ++i) {
        CWord* pW = m_words[i].Get();
        for (size_t j = 0; j < wp.size(); ++j)
            if (wp[j].FirstWord() == (int)i)
                str += Substitute("<font color=\"$0\">", sColor);
        if (i > 0 && (!pW->IsPunct() || pW->IsOpenBracket() || pW->IsCloseBracket()))
            str += " ";
        str += WideToChar(pW->GetOriginalText(), encoding);
        for (size_t j = 0; j < wp.size(); ++j)
            if (wp[j].LastWord() == (int)i)
                str += "</font>";
    }
    return StripString(str);
}

Wtroka CSentence::ToString(const CWordsPair& wp) const
{
    return m_Words.ToString(wp);
}

Wtroka CSentence::ToString() const
{
    return m_Words.SentToString();
}

void CSentence::ToString(Wtroka& str) const
{
    m_Words.SentToString(str);
}

bool CSentence::CheckAllAreNames()
{
    for (size_t i = 0; i < m_words.size(); i++)
        if (!m_words[i]->IsNameFoundInDic())
            return false;

    return true;
}

void CSentence::TryToPredictPatronymic(int iW, bool bForSearching)
{
    if (iW == 0)
        return;
    if (!m_words[iW-1]->IsName(FirstName))
        return;
    if (!bForSearching) {
        if (!m_words[iW]->m_bUp)
            return;
        if (((iW == 1) || !m_words[iW-2]->m_bUp) &&
                ((iW == (int)m_words.size()-1) || !m_words[iW+1]->m_bUp)
        )
        return;
    } else {
        if (m_words.size() == 2)
            return;
    }
    m_words[iW]->TryToPredictPatronymic();
}

void CSentence::InitNameTypes(bool bForSearching)
{
    for (size_t i = 0; i < m_words.size(); i++) {
        if (m_words[i]->m_bUp) {
            if (!m_words[i]->InitNameType())
                TryToPredictPatronymic(i, bForSearching);
        }
    }
}

void CSentence::FindNames(yvector<CFioWordSequence>& foundNames, yvector<int>& potentialSurnames)
{
    try {
        InitNameTypes();
        CNameFinder nameFinder(m_Words);
        nameFinder.Run(foundNames, potentialSurnames);
    } catch (yexception& e) {
        PrintError("Error in CSentence::FindNames()", &e);
        throw;
    } catch (...) {
        PrintError("Error in CSentence::FindNames()", NULL);
        throw;
    }
}

void CSentence::AddFIO(yset<Wtroka>& fioStrings, const CFIOOccurenceInText& wordsPairInText, bool bIndexed)
{
    SWordHomonymNum num;
    if (wordsPairInText.m_NameMembers[Surname].IsValid())
        num = wordsPairInText.m_NameMembers[Surname];
    else if (wordsPairInText.m_NameMembers[FirstName].IsValid())
        num = wordsPairInText.m_NameMembers[FirstName];
    else if (wordsPairInText.m_NameMembers[Patronomyc].IsValid())
        num = wordsPairInText.m_NameMembers[Patronomyc];
    else if (wordsPairInText.m_NameMembers[InitialName].IsValid())
        num = wordsPairInText.m_NameMembers[InitialName];

    if (num.IsValid())
        m_Words.GetWord(num).AddFio(fioStrings, bIndexed);
}

const CWord& CSentence::GetWord(const SWordHomonymNum& wh) const
{
    return m_Words.GetWord(wh);
}

const CHomonym& CSentence::GetHomonym(const SWordHomonymNum& wh) const
{
    return m_Words[wh];
}

void CSentence::TakeFioWS(TIntrusivePtr<CFioWordSequence> fio)
{
    int iAdded = -1;
    for (int i = m_wordSequences.size() - 1; i >= 0; --i) {
        if (m_wordSequences[i]->GetWSType() != FioWS)
            continue;
        if (*m_wordSequences[i] == *fio) {
            CFioWordSequence* pFoundFioWS = CheckedCast<CFioWordSequence*>(m_wordSequences[i].Get());

            if (pFoundFioWS->GetFio().m_Genders != fio->GetFio().m_Genders) {
                if (pFoundFioWS->m_iSimilarOccurencesCount > fio->m_iSimilarOccurencesCount) {
                    if (iAdded != -1)
                        m_wordSequences[iAdded] = NULL;
                    else
                        iAdded = 0;     // do not add
                    break;

                } else if (pFoundFioWS->m_iSimilarOccurencesCount < fio->m_iSimilarOccurencesCount) {
                    if (iAdded == -1) {
                        m_wordSequences[i] = fio.Get();
                        iAdded = i;
                    } else
                        m_wordSequences[i] = NULL;
                }
            }
        }
    }

    if (iAdded == -1)
        m_wordSequences.push_back(fio.Get());

    // remove NULLs
    for (int i = m_wordSequences.size() - 1; i >= 0; --i)
        if (m_wordSequences[i].Get() == NULL)
            m_wordSequences.erase(m_wordSequences.begin() + i);
}

void CSentence::AddFIOWS(const CFIOOccurence& fioOccurence, const SFullFIO& fio, int iSimilarOccurencesCount)
{
    TIntrusivePtr<CFioWordSequence> fioWS(new CFioWordSequence(fio));
    *(CFIOOccurence*)(fioWS.Get()) = fioOccurence;
    fioWS->PutWSType(FioWS);
    if (fio.m_Genders.any()) {
        THomonymGrammems gram = fioWS->GetGrammems();
        gram.Replace(NSpike::AllGenders, fio.m_Genders);
        fioWS->SetGrammems(gram);
    }

    fioWS->m_iSimilarOccurencesCount = iSimilarOccurencesCount;

    bool isManualFio = true;

    SWordHomonymNum wh = fioOccurence.m_NameMembers[Surname];
    if (wh.IsValid())
        if (m_Words.GetWord(wh).IsMultiWord())
            isManualFio = false;

    if (!fio.m_bFoundSurname &&
        fioOccurence.m_NameMembers[Surname].IsValid() &&
        !(fioOccurence.m_NameMembers[FirstName].IsValid() ||
            fioOccurence.m_NameMembers[InitialName].IsValid()) &&
            isManualFio) {
        CNameFinder nameFinder(m_Words);
        //если не смогли среди предсказанных фамилий
        //найти совпадающую с фамилией из fio, то вываливаемся
        if (!nameFinder.PredictSingleSurname(*fioWS, fio))
            return;
    }

    fioWS->ClearLemmas();
    if (!fio.m_strSurname.empty()) {
        Wtroka capLemma;
        if (fioOccurence.m_NameMembers[Surname].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[Surname]);
            capLemma = GetCapitalizedLemma(w, -1, fio.m_strSurname);
        } else {
            capLemma = fio.m_strSurname;
            NStr::ToFirstUpper(capLemma);
        }
        fioWS->AddLemma(SWordSequenceLemma(fio.m_strSurname, capLemma));
    }
    if (!fio.m_strName.empty()) {
        Wtroka capLemma;
        if (fioOccurence.m_NameMembers[FirstName].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[FirstName]);
            capLemma = GetCapitalizedLemma(w, -1, fio.m_strName);
        } else if (fioOccurence.m_NameMembers[InitialName].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[InitialName]);
            capLemma = GetCapitalizedLemma(w, -1, fio.m_strName);
        } else {
            capLemma = fio.m_strName;
            TMorph::ToTitle(capLemma);

        }
        fioWS->AddLemma(SWordSequenceLemma(fio.m_strName, capLemma));
    }
    if (!fio.m_strPatronomyc.empty()) {
        Wtroka capLemma;
        if (fioOccurence.m_NameMembers[Patronomyc].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[Patronomyc]);
            capLemma = GetCapitalizedLemma(w, -1, fio.m_strPatronomyc);
        } else if (fioOccurence.m_NameMembers[InitialPatronomyc].IsValid()) {
            const CWord& w = m_Words.GetWord(fioOccurence.m_NameMembers[InitialPatronomyc]);
            capLemma = GetCapitalizedLemma(w, -1, fio.m_strPatronomyc);
        } else {
            capLemma = fio.m_strPatronomyc;
            TMorph::ToTitle(capLemma);
        }

        fioWS->AddLemma(SWordSequenceLemma(fio.m_strPatronomyc, capLemma));
    }
    TakeFioWS(fioWS);
}

//переопределена в CSentenceRusProcessor
Wtroka CSentence::GetCapitalizedLemma(const CWord& , int, Wtroka strLemma)
{
    return strLemma;
}

void CSentence::BuildNumbers()
{
    try {
        CNumberProcessor numberProcessor(m_words, m_Numbers, m_wordSequences);
        numberProcessor.Run();
    } catch (yexception& e) {
         PrintError("Error in CSentence::BuildNumbers()", &e);
         throw;
    } catch (...) {
         PrintError("Error in CSentence::BuildNumbers()", NULL);
         throw;
    }
}

bool CSentence::GetWSLemmaString(Wtroka& sLemmas, const CWordSequence& ws, bool bLem) const
{
    sLemmas = ws.GetCapitalizedLemma();

    if (bLem)
        return !sLemmas.empty();

    if (sLemmas.empty())
        for (int j = ws.FirstWord(); j <= ws.LastWord(); j++) {
            if (!sLemmas.empty())
                sLemmas += ' ';
            sLemmas += getWord(j)->GetOriginalText();
        }

    static const Wtroka trim_chars = CharToWide(" \"\'");
    TWtringBuf res = sLemmas;
    while (!res.empty() && trim_chars.find(res[0]) != TWtringBuf::npos)
        res.Skip(1);
    while (!res.empty() && trim_chars.find(res.back()) != TWtringBuf::npos)
        res.Chop(1);

    if (sLemmas.size() != res.size())
        sLemmas = ::ToWtring(res);

    return true;
}

Wtroka CSentence::BuildDates()
{
    try {
        CDateProcessor dateProcessor(m_Words, m_wordSequences);
        dateProcessor.Run();
        return Wtroka();
    } catch (yexception& e) {
        PrintError("Error in CSentence::BuildDates()", &e);
        throw;
    } catch (...) {
        PrintError("Error in CSentence::BuildDates()", NULL);
        throw;
    }
}

