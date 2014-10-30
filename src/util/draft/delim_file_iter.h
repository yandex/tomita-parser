#pragma once

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/stream/ios.h>

#include <util/draft/delim_stroka_iter.h>

class TDelimFileIter : public NStl::iterator< NStl::forward_iterator_tag, Stroka >
{
private:
    TInputStream*            InputStream;

    Stroka                   CurrLine;
    TDelimStrokaIter         ItLine;
    TDelimStrokaIter         EndLine;

    static const TStringBuf DEF_DELIM;
    TStringBuf              Delim;

    bool                     Finished;

public:
    TDelimFileIter(TInputStream* inputStream, TStringBuf delim = "\n")
      : InputStream(inputStream)
      , Delim(delim)
      , Finished(false)
    {
        NewLine();
    }

    TDelimFileIter() // fake end
      : Delim(DEF_DELIM)
      , Finished(true) {}

    TDelimFileIter& operator++()
    {
        assert(!CurrLine.empty());

        ++ItLine;
        if (ItLine == EndLine)
            NewLine();

        return *this;
    }

    Stroka operator*() const
    {
        return ToString(*ItLine);
    }

    bool IsFinished() const
    {
        return Finished;
    }

    // Little bit of fake (to be compatible with iterator conventions)
#ifndef NDEBUG
    bool operator!=(const TDelimFileIter& rhs) const
#else
    bool operator!=(const TDelimFileIter& /*rhs*/) const
#endif
    {
        assert(rhs.Finished);
        return !Finished;
    }

#ifndef NDEBUG
    bool operator==(const TDelimFileIter& rhs) const
#else
    bool operator==(const TDelimFileIter& /*rhs*/) const
#endif
    {
        assert(rhs.Finished);
        return Finished;
    }

    void NewLine()
    {
        Finished = !(InputStream->ReadLine(CurrLine));
        if (!Finished) {
            ItLine  = begin_delim(CurrLine, Delim);
            EndLine = end_delim(CurrLine, Delim);
        }
    }
};
