#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>

#include <util/charset/unidata.h>

template <class TString>
class abstract_string_tokenizer_t
{
public:
    typedef typename TString::char_type TChar;
    typedef TStringBufImpl<TChar> TStrBuf;

    abstract_string_tokenizer_t(const TString& pszString, const char* pszDelimeters = NULL)
        : m_String(pszString)
        , m_Current(m_String)
    {
        if (pszDelimeters == NULL) {
            static const TChar DefaultDelimeters[] = {',', '\t', ' ', '\n', 0};
            m_strDelimeters = DefaultDelimeters;
        } else
            for (const char* p = pszDelimeters; *p != 0; ++p) {
                YASSERT(static_cast<ui8>(*p) <= 0x7F);
                m_strDelimeters.append(static_cast<ui8>(*p));
            }
    }

    template <typename TInt>
    bool NextAsNumber(TInt& nItem, size_t& count)
    {
        count = 0;
        SkipDelimeters();

        if (Empty())
            return false;

        TStrBuf strItem(~m_Current, ~m_Current);
        if (!Empty() && ObservedChar() == '-') {
            strItem = TStrBuf(~strItem, +strItem + 1);
            MoveNext();
            ++count;
        }
        for (; !Empty() && ObservedIsDigit(); MoveNext(), ++count)
            strItem = TStrBuf(~strItem, +strItem + 1);

        if (count == 0)
            return false;

        if (!Empty() && !ObservedIsDelimeter())
            return false;

        try {
            nItem = FromString<TInt>(strItem);
            return true;
        } catch (...) {
            return false;
        }
    }

    template <typename TInt>
    bool NextAsNumber(TInt& nItem)
    {
        size_t count;
        return NextAsNumber(nItem, count);
    }

    bool NextAsString(TStrBuf& strItem)
    {
        SkipDelimeters();
        if (Empty())
            return false;

        strItem = TStrBuf(~m_Current, ~m_Current);
        while (!Empty()) {
            strItem = TStrBuf(~strItem, +strItem + 1);
            MoveNext();
            if (ObservedIsDelimeter())
                break;
        }
        return true;
    }

    bool NextAsString(TString& str)
    {
        TStrBuf res;
        if (NextAsString(res)) {
            str.assign(~res, +res);
            return true;
        } else
            return false;
    }

    template <class TVector>
    bool ToVector(TVector& res) const
    {
        abstract_string_tokenizer_t myCopy(*this);
        res.resize(myCopy.ComputeCount());
        for (size_t i = 0; i < res.size(); ++i)
            if (!myCopy.item(res[i]))
                return false;
        return true;
    }

    const TStrBuf& RemainedString() const {
        return m_Current;
    }

    TChar ObservedChar() const
    {
        YASSERT(!m_Current.empty());
        return m_Current[0];
    }

    bool IsDelimeter(TChar ch) const
    {
        return m_strDelimeters.find(ch) != TString::npos;
    }

    bool ObservedIsDelimeter()
    {
        return Empty() ? false : IsDelimeter(ObservedChar());
    }

    void SkipDelimeters()
    {
        while (ObservedIsDelimeter())
            MoveNext();
    }

    void SkipNonDelimeters()
    {
        while (!Empty())
            if (!ObservedIsDelimeter())
                MoveNext();
            else
                break;
    }

    static bool IsDigit(TChar ch)
    {
        return ::IsCommonDigit(ch);
    }

    bool ObservedIsDigit()
    {
        if (Empty())
            return false;
        else
            return this->IsDigit(ObservedChar());
    }

    bool MoveNext()
    {
        m_Current.Skip(1);
        return Empty();
    }

    bool Empty() const
    {
        return m_Current.empty();
    }

    void Reset()
    {
        m_Current = TStrBuf(m_String);
    }

    size_t ComputeCount()
    {
        SkipDelimeters();
        size_t nCount = 0;
        for (; !Empty(); ++nCount) {
            SkipNonDelimeters();
            SkipDelimeters();
        }
        Reset();
        return nCount;
    }

protected:
    TString m_String;
    TStrBuf m_Current;
    TString m_strDelimeters;
};

typedef abstract_string_tokenizer_t<Stroka> TStrokaTokenizer;
typedef abstract_string_tokenizer_t<Wtroka> TWtrokaTokenizer;

/**
  define TOKENIZER_USES_VECTOR_BOOL
  if you want to use vector<bool> in tokenizer.
  It must work faster with it, but it does not.
*/

/**
  The Reenterable replacement of strtok().


  Usage:

  char *text = "word, word2;word3"
  StringTokenizer token(text, " ,;");
  const char *word;
  while((word = token())){
    // handle token here
  }


*/

template <typename TString>
class StringTokenizer
{
    typedef typename TString::char_type TChar;
    typedef TStringBufImpl<TChar> TStrBuf;

    TStrBuf Text;
    TChar SingleDelimChar;
    TStrBuf Delims;

public:
    StringTokenizer(const TStrBuf& text, const TStrBuf& delims)
        : Text(text)
        , SingleDelimChar(0)
        , Delims(delims)
    {
    }

    StringTokenizer(const TStrBuf text, TChar delim)
        : Text(text)
        , SingleDelimChar(delim)
        , Delims(&SingleDelimChar, 1)
    {
    }

    bool NextToken(TStrBuf& res) {
        // skip delims
        while (!Text.empty() && Delims.find_first_of(Text[0]) != TStrBuf::npos)
            Text.Skip(1);

        if (Text.empty())
            return false;

        const TChar* res_start = ~Text;
        while (!Text.empty() && Delims.find_first_of(Text[0]) == TStrBuf::npos)
            Text.Skip(1);

        res = TStrBuf(res_start, ~Text);
        return true;
    }

    bool SkipNext() {
        TStrBuf tok;
        return NextToken(tok);
    }

    TStrBuf Remaining() const {
        return Text;
    }

    bool HasNext() const {
        return !Text.empty();
    }

};
