#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/fact-types.h>
#include <FactExtract/Parser/afdocparser/common/wordsequence.h>

class CTextWS: public CWordSequenceWithAntecedent,
               public CFieldAlgorithmInfo
{
    yvector<Wtroka>    m_MainWords;
    bool m_bGen;
    bool m_bAdj;

    void UpdateFlags(const CTextWS& Other);

public:

    CTextWS();
    CTextWS(CWordSequenceWithAntecedent& ws);
    void AddPostfix(const CTextWS& Postfix);
    void AddPrefix(const CTextWS& Postfix);
    void SetGen();
    void SetAdj();
    void ClearWordsInfos();
    Wtroka GetMainWordsDump() const;
    void DeletePronouns();
    void Clear();
    bool HasPossPronouns() const;

    void AddWordInfo(const Wtroka& s, bool isMain = false);
    inline void AddWordInfoIfNonEmpty(const Wtroka& s, bool isMain = false) {
        if (!s.empty())
            AddWordInfo(s, isMain);
    }

    void AddWordInfo(const CTextWS& ws, bool bMainWords);
};
