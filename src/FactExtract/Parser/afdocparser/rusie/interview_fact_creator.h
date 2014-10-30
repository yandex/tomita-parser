#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>


class CInterviewFactCreator
{
    yvector<CSentence*>&    m_vecSentence;

    const TArticleRef& m_MainArticle;

    bool IsAlreadyCreated();
    void HasFioInInterviewFact(SFullFIO& interviewFio, int iSent, int& iGoodCount, int& iBadCount, int& iMaybeCount);

public:
    CInterviewFactCreator(yvector<CSentence*>& vecSentence,
                          const TArticleRef& article);

    bool CreateFact(Wtroka strFio, int iLastTitleSent);

};
