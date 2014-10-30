#pragma once

#include "json_reader.h"

namespace NJson {

struct TJsonPrettifier {
    bool Unquote;
    ui8  Padding;
    bool SingleQuotes;
    bool Compactify;

    TJsonPrettifier()
        : Unquote(false)
        , Padding(4)
        , SingleQuotes(false)
        , Compactify(false)
    {}

    static TJsonPrettifier Prettifier(bool unquote, ui8 padding, bool singlequotes) {
        TJsonPrettifier p;
        p.Unquote = unquote;
        p.Padding = padding;
        p.SingleQuotes = singlequotes;
        return p;
    }

    static TJsonPrettifier Compactifier(bool unquote, bool singlequote = false) {
        TJsonPrettifier p;
        p.Unquote = unquote;
        p.Padding = 0;
        p.Compactify = true;
        p.SingleQuotes = singlequote;
        return p;
    }

    bool Prettify(TStringBuf in, TOutputStream& out);
    Stroka Prettify(TStringBuf in);
};

inline Stroka PrettifyJson(TStringBuf in, bool unquote = false, ui8 padding = 4, bool sq = false) {
    return TJsonPrettifier::Prettifier(unquote, padding, sq).Prettify(in);
}

inline bool PrettifyJson(TStringBuf in, TOutputStream& out, bool unquote = false, ui8 padding = 4, bool sq = false) {
    return TJsonPrettifier::Prettifier(unquote, padding, sq).Prettify(in, out);
}

inline bool CompactifyJson(TStringBuf in, TOutputStream& out, bool unquote = false, bool sq = false) {
    return TJsonPrettifier::Compactifier(unquote, sq).Prettify(in, out);
}

inline Stroka CompactifyJson(TStringBuf in, bool unquote = false, bool sq = false) {
    return TJsonPrettifier::Compactifier(unquote, sq).Prettify(in);
}

}
