#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "propnameinquotesfinder.h"
#include "tomitachainsearcher.h"
#include "referencesearcher.h"

CPropNameInQuotesFinder::CPropNameInQuotesFinder(CWordVector& words, CTomitaChainSearcher& tomitaChainSearcher)
    : m_Words(words)
    , m_TomitaChainSearcher(tomitaChainSearcher)
{
}

void CPropNameInQuotesFinder::Run(const TArticleRef& article, yvector<CWordSequence*>& resWS)
{
    for (size_t iW = 0; iW < m_Words.OriginalWordsCount(); ++iW) {
        int iW2 = BuildCompanyNameInQuotes(iW);
        if (iW2 != -1) {
            if (iW == 0) {
                if (iW2 + 1 == (int)m_Words.OriginalWordsCount())
                    continue;
                if (iW2 + 2 == (int)m_Words.OriginalWordsCount()) {
                    CWord* pW = &m_Words[iW2 + 1];
                    if (pW->IsPoint())
                        continue;
                }
            }

            //запустим специальную грамматику для слов в кавычках,
            //которая вытаскивает собственно имя компании и CompanyDescr
            //например для "НК 'Юкос'"
            //если грамматика ничего не смогла построить, то создаем обычный CWordSequence как и раньше
            CWordSequence* pWS = ApplyGrammarToCompanyInQuotes(iW, iW2);
            if (pWS == NULL)
                pWS = new CWordSequence();

            pWS->SetPair(iW, iW2);
            pWS->PutWSType(CompanyWS);
            pWS->PutArticle(article);

            //главное слово устанавливаем на первую попавшуюся фигню - все равно никому не нужно
            SWordHomonymNum whMain(iW, m_Words.GetOriginalWord(iW).IterHomonyms().GetID());
            pWS->SetMainWord(whMain);
            resWS.push_back(pWS);
            iW = iW2;
        }
    }
}

int CPropNameInQuotesFinder::BuildCompanyNameInQuotes(size_t iW)
{
    size_t iWSave = iW;
    CWord* pW = &m_Words[iW];
    bool bFirstSingleQuote = false;
    if (!pW->HasOpenQuote(bFirstSingleQuote) && !pW->IsQuote())
        return -1;

    if (pW->IsQuote())
        bFirstSingleQuote = pW->IsSingleQuote();

    size_t iWsave = iW;
    int iWFound = -1;
    bool bHasVerb = false;
    for (; iW < m_Words.OriginalWordsCount(); iW++) {
        pW = &m_Words[iW];
        if (pW->IsPunct() && !pW->IsDash() && !pW->IsQuote()) {
            bool bBreak = true;
            if (pW->IsComma()) {
                //разрешаем только между двумя сущ.
                if (iW > 0 && iW + 1 < m_Words.OriginalWordsCount()) {
                    CWord* pW1 = &m_Words[iW - 1];
                    CWord* pW2 = &m_Words[iW + 1];

                    if ((pW1->HasPOS(gSubstantive) || pW1->HasUnknownPOS()) &&
                        (pW2->HasPOS(gSubstantive) || pW2->HasUnknownPOS()))
                            bBreak = false;

                }
            }
            //воскл. знак торлько в конце разрешаем ("Вперед,  Россия!")
            else if (pW->IsExclamationMark()) {
                if (iW > 0)
                    if (pW->HasCloseQuote())
                        bBreak = false;
                    else if (iW + 1 < m_Words.OriginalWordsCount())
                            if (m_Words[iW + 1].IsQuote())
                                bBreak = false;
            }
            if (bBreak)
                return -1;
        }
        if (pW->HasOnlyPOS(gVerb) || pW->HasOnlyPOS(gPraedic))
            bHasVerb = true;
        if (iW == iWsave)
            if (!pW->IsQuote() && !pW->m_bUp)
                return -1;
        if (iW == iWsave + 1 && m_Words[iW - 1].IsQuote() && !pW->m_bUp)
            return -1;

        bool bSingle;
        if (pW->HasCloseQuote(bSingle)) {
            iWFound = iW;
            if (bSingle == bFirstSingleQuote)
                break;
        }

        if (pW->IsSingleQuote() && (iW > iWSave)) {
            iWFound = iW;
            bool bFound = false;
            bool bBreak = false;
            size_t i;
            for (i = iW + 1; i < m_Words.OriginalWordsCount() && i < iW + 4; i++) {
                pW = &m_Words[i];
                bool bSingle;
                if (pW->HasOpenQuote()) {
                    bFound = false;
                    bBreak = true;
                    break;
                }
                if (pW->IsSingleQuote()) {
                    bFound = true;
                    iWFound = i;
                    break;
                }

                if (pW->HasCloseQuote(bSingle)) {
                    if (bSingle)
                        bFound = true;
                    iWFound = i;
                    break;
                }
            }
            if (bFound)
                iW = i;
            if (bBreak)
                break;
            else
                continue;
        }

        if (pW->IsQuote() && (iW > iWSave)) {
            iWFound = iW;
            bool bFound = false;
            size_t i;
            for (i = iW + 1; i < m_Words.OriginalWordsCount() && i < iW + 4; i++) {
                pW = &m_Words[i];

                if (pW->HasOpenQuote()) {
                    bFound = false;
                    break;
                }

                if (pW->HasCloseQuote()) {
                    bFound = true;
                    break;
                }
            }
            if (bFound) {
                iW = i;
                iWFound = iW;
            }
            break;
        }
    }

    if (iWFound != -1) {
        if (((iWFound - iWSave > 7) && bHasVerb) ||
            ((iWFound - iWSave > 15) && !bHasVerb))
            return -1;
        return iWFound;
    }
    return -1;
}

//применяем специальную грамматику "разбор_имени_в_кавычках",
//чтобы отделить company_descr от. собственно, имени компании
CWordSequence* CPropNameInQuotesFinder::ApplyGrammarToCompanyInQuotes(int iW1, int iW2)
{
    const CDictsHolder& dicts = *GlobalDictsHolder;

    // find grammar filename of this article in dicts-holder
    DECLARE_STATIC_RUS_WORD(article_name, "разбор_имени_в_кавычках");
    Stroka gram_file_name;
    NGzt::TArticlePtr art;
    if (dicts.Gazetteer()->ArticlePool().FindCustomArticleByName(article_name, art))
        for (NGzt::TCustomKeyIterator it(art, ::GetTomitaPrefix()); it.Ok(); ++it) {
            gram_file_name = *it;
            break;
        } else {
        const article_t* pArt = dicts.GetAuxArticle(article_name, KW_DICT);
        if (pArt != NULL)
            gram_file_name = pArt->get_gram_file_name();
    }

    if (gram_file_name.empty())
        ythrow yexception() << "Cannot resolve tomita grammar filename for article \"" << article_name << "\".";

    if (m_Words[iW1].IsQuote())
        iW1++;

    if (m_Words[iW2].IsQuote())
        iW2--;

    if (m_Words[iW2].IsQuote())
        iW2--;

    if (iW2 <= iW1)
        return NULL;

    CWordsPair wp(iW1, iW2);
    yvector<CWordSequence*> newWordSequences;
    m_TomitaChainSearcher.RunFactGrammar(gram_file_name, dicts.GetGrammarOrRegister(gram_file_name), newWordSequences, wp, false);

    CWordSequence* pRes = NULL;
    for (size_t i = 0; i < newWordSequences.size(); i++) {
        if (wp == *newWordSequences[i])
            pRes = newWordSequences[i];
        else
            delete newWordSequences[i];
    }

    return pRes;
}

