#include "wordbase.h"

#include <util/charset/unidata.h>


const wchar16 SINGLE_QUOTE_CHAR = '\'',
              DOUBLE_QUOTE_CHAR = '\"';

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CWordBase::CWordBase(docLanguage lang,const CPrimGroup &prim,const Wtroka &strWord)
    : m_lang(lang)
{
    m_bHasAltWordPart = false;
    m_typ = prim.m_gtyp;
    m_num = prim.m_prim.size();

    if (strWord.size() >= MAXWORD_LEN)
        SetText(strWord.substr(0, MAXWORD_LEN - 2));
    else
        SetText(strWord);

    m_pos = prim.m_pos;
    m_len = prim.m_len;

    m_bUp = false;
    m_bHasUnusefulPostfix = false;
    m_bHasOpenQuote = false;
    m_bHasCloseQuote = false;

    m_bSingleOpenQuote = false;
    m_bSingleCloseQuote = false;
    m_bIsPunct = false;
    InitPunc();
}

CWordBase::CWordBase(docLanguage lang, const Wtroka &strWord)
    : m_lang(lang)
{
    m_bHasAltWordPart = false;
    m_typ = UnknownPrim;
    m_num = 0;

    SetText(strWord);
    m_pos = 0;
    m_len = 0;

    m_bUp = false;
    m_bHasUnusefulPostfix = false;
    m_bHasOpenQuote = false;
    m_bHasCloseQuote = false;

    m_bSingleOpenQuote = false;
    m_bSingleCloseQuote = false;
    m_bIsPunct = false;
    InitPunc();

}

//////////////////////////////////////////////////////////////////////////////
// AddVariant

void CWordBase::AddVariant(THomonymBasePtr pHom)
{
    m_variant.push_back(pHom);
}

void CWordBase::SetHasAltWordPart(bool val)
{
    m_bHasAltWordPart = val;
}

bool  CWordBase::GetHasAltWordPart() const
{
    return m_bHasAltWordPart;
}

bool CWordBase::GetIsUp() const
{
    return m_bUp;
}

void CWordBase::PutIsUp(bool val)
{
    m_bUp = val;
}

void CWordBase::InitPunc()
{
    if (m_typ == Punct && strchr("!,-.:;?\'\"", m_txt[0]))
        m_bIsPunct = true;
}

static inline bool IsChar(const Wtroka& s, wchar16 ch)
{
    return s.size() == 1 && s[0] == ch;
}

Wtroka CWordBase::GetOriginalText() const
{
    //затычка для дурацких символов, порождаемых для <BR> после конца предложения
    if (m_typ == Punct && IsChar(m_txt, 'W'))
        return Wtroka();

    size_t quoteCount = (m_bHasOpenQuote ? 1 : 0) + (m_bHasCloseQuote ? 1 : 0);

    // optimize most frequent case
    if (quoteCount == 0)
        return m_txt;

    Wtroka res;
    res.reserve(m_txt.size() + quoteCount);
    if (m_bHasOpenQuote)
        res += m_bSingleOpenQuote ? SINGLE_QUOTE_CHAR : DOUBLE_QUOTE_CHAR;
    res += m_txt;
    if (m_bHasCloseQuote)
        res += m_bSingleCloseQuote ? SINGLE_QUOTE_CHAR : DOUBLE_QUOTE_CHAR;
    return res;
}

bool CWordBase::IsQuote() const
{
    return IsDoubleQuote() || IsSingleQuote();
}

bool CWordBase::IsDoubleQuote() const
{
    return m_bIsPunct && CWordBase::IsDoubleQuote(m_txt);
}

bool CWordBase::IsSingleQuote() const
{
    return m_bIsPunct && CWordBase::IsSingleQuote(m_txt);
}

bool CWordBase::IsQuote(const Wtroka& str)
{
    return IsSingleQuote(str) || IsDoubleQuote(str);
}

bool CWordBase::IsSingleQuote(const Wtroka& str)
{
    return str.size() == 1 && str[0] == SINGLE_QUOTE_CHAR;
}

bool CWordBase::IsDash(const Wtroka& str)
{
    return str.size() == 1 && ::IsDash(str[0]);

}

bool CWordBase::IsDoubleQuote(const Wtroka& str)
{
    return str.size() == 1 && ::IsQuotation(str[0]) && str[0] != SINGLE_QUOTE_CHAR;
    /*return IsChar(str, DOUBLE_QUOTE_CHAR) ||
           IsChar(str, 127) ||                  // wtf?
           IsChar(str, 0x00BB) ||               // »
           IsChar(str, 0x00AB) ||               // «
           IsChar(str, 0x201C) ||               // “
           IsChar(str, 0x201D);                 // ”
    */
}

bool CWordBase::IsPunct() const
{
    return m_bIsPunct;
}

bool CWordBase::IsExclamationMark() const
{
    return m_bIsPunct && IsChar(m_txt, '!');
}

bool CWordBase::IsInterrogationMark() const
{
    return m_bIsPunct && IsChar(m_txt, '?');
}

bool CWordBase::IsComma() const
{
    return m_bIsPunct && IsChar(m_txt, ',');
}

bool CWordBase::IsPoint() const
{
    return m_bIsPunct && IsChar(m_txt, '.');
}

bool CWordBase::HasOpenQuote(bool& bSingle) const
{
    bSingle = m_bSingleOpenQuote;
    return m_bHasOpenQuote;
}

bool CWordBase::HasOpenQuote() const
{
    bool bSingle;
    return HasOpenQuote(bSingle);
}

bool CWordBase::HasCloseQuote(bool& bSingle) const
{
    bSingle = m_bSingleCloseQuote;
    return m_bHasCloseQuote;
}

bool CWordBase::HasCloseQuote() const
{
    bool bSingle;
    return HasCloseQuote(bSingle);
}

void CWordBase::PutIsOpenQuote(bool bSingle)
{
    m_bHasOpenQuote = true;
    m_bSingleOpenQuote = bSingle;
}

void CWordBase::PutIsCloseQuote(bool bSingle)
{
    m_bHasCloseQuote = true;
    m_bSingleCloseQuote = bSingle;
}

bool CWordBase::IsOpenBracket() const
{
    return m_typ == Punct && IsChar(m_txt, '(');
}

bool CWordBase::IsColon() const
{
    return m_typ == Punct && IsChar(m_txt, ':');
}

bool CWordBase::IsDash() const
{
    return (m_typ == Punct) && IsDash(m_txt);
}

bool CWordBase::IsCloseBracket() const
{
    return m_typ == Punct && IsChar(m_txt, ')');
}

//////////////////////////////////////////////////////////////////////////////
