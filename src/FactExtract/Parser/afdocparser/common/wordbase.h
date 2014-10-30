#pragma once


#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "primitive.h"
#include "homonymbase.h"


class CWordBase
{
public:
    inline virtual ~CWordBase() {
    }

    static bool IsQuote(const Wtroka& str);
    static bool IsSingleQuote(const Wtroka& str);
    static bool IsDoubleQuote(const Wtroka& str);
    static bool IsDash(const Wtroka& str);

protected:
    CWordBase(docLanguage lang, const CPrimGroup &prim, const Wtroka &strWord);
    CWordBase(docLanguage lang, const Wtroka &strWord);

public:
    virtual size_t GetHomonymsCount() const = 0;
    virtual THomonymBasePtr GetHomonym(size_t iHom) const = 0;
    virtual void AddHomonym(THomonymBasePtr pHom, bool bAllowIgnore) = 0;
    virtual void AddPredictedHomonym(THomonymBasePtr pHom, bool bAllowIgnore) = 0;
    virtual void AddFio(yset<Wtroka>& /*strFios*/, bool /*bIndexed*/) {};

    // quotes added if the word was quoted in original text
    Wtroka GetOriginalText() const;

    inline const Wtroka& GetText() const
    {
        return m_txt;
    }

    inline const Wtroka& GetLowerText() const
    {
        return IsLowerTextSameAsOriginal() ? m_txt : m_lowercase_txt;
    }

    inline void SetText(const Wtroka& text)
    {
        m_txt = text;
        m_lowercase_txt = text;
        if (!TMorph::ToLower(m_lowercase_txt))
            m_lowercase_txt.clear();
    }

    bool    IsOpenBracket() const;
    bool    IsColon() const;
    bool    IsDash() const;
    bool    IsCloseBracket() const;
    bool    IsPoint() const;
    bool    IsComma() const;
    bool    IsExclamationMark() const;
    bool    IsInterrogationMark() const;
    bool    IsSingleQuote() const;
    bool    IsDoubleQuote() const;
    bool    IsQuote() const;

    virtual bool HasNameHom() const { return false; }
    virtual bool HasGeoHom() const  { return false; }

public:
    const docLanguage m_lang;
    size_t m_pos;
    size_t m_len;
    bool   m_bHasAltWordPart;

protected:
    Wtroka m_txt;
    Wtroka m_lowercase_txt;    // to preserve memory this field is empty when m_txt contains only lower-case letters

    inline bool IsLowerTextSameAsOriginal() const
    {
        return m_lowercase_txt.empty();
    }

public:
    ETypeOfPrim m_typ;
    size_t m_num;

    THomonymBaseVector m_variant;
    yvector<THomonymBaseVector> m_variant_for_search; //разложение откусанных частей из, например,
                                           //дефисных слов. например в "стекло-генерал-садовод"
                                           //здесь будет <<стекло><генерал>><<стечь><генерал>>, а в m_homonym
                                            //будет только садовод, в m_variant_for_search хранятся указатели из
                                            //m_variant, т.е. m_variant_for_search  удалять не надо
    bool m_bUp;
    bool m_bHasUnusefulPostfix;

protected:
    bool m_bHasOpenQuote;
    bool m_bHasCloseQuote;
    bool m_bSingleCloseQuote;
    bool m_bSingleOpenQuote;

public:

    void SetHasAltWordPart(bool val);
    bool GetHasAltWordPart() const;
    bool GetIsUp() const;
    void PutIsUp(bool val);
    void AddPunct(char c);
    bool HasOpenQuote(bool& bSingle) const;
    bool HasOpenQuote() const;
    bool HasCloseQuote(bool& bSingle) const;
    bool HasCloseQuote() const;
    void PutIsOpenQuote(bool bSingle);
    void PutIsCloseQuote(bool bSingle);
    void    InitPunc();
    bool    IsPunct() const;
    void    PutIsPunct(bool bPunct) const { m_bIsPunct = bPunct; };

public:
    virtual void AddVariant(THomonymBasePtr pHom);
    mutable bool m_bIsPunct;
};
