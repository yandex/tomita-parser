#include "json_prettifier.h"

#include <util/generic/deque.h>
#include <util/memory/pool.h>
#include <util/string/util.h>

#include <util/draft/relaxed_escaper.h>
#include <util/draft/accessors.h>

namespace NJson {

struct TRewritableOut {
    TOutputStream& Slave;

    char Last;
    bool Dirty;

    TRewritableOut(TOutputStream& sl)
        : Slave(sl)
        , Last()
        , Dirty()
    {}

    template <typename T>
    void Write(const T& t) {
        Flush();
        Slave << t;
    }

    void Hold(char c) {
        if (Dirty)
            Flush();
        Last = c;
        Dirty = true;
    }

    void Flush() {
        if (Dirty) {
            Slave << Last;
            Dirty = false;
        }
    }

    void Revert() {
        Dirty = false;
    }
};

struct TSpaces {
    char S[256];

    TSpaces() {
        memset(&S, ' ', sizeof(S));
    }

    TStringBuf Get(ui8 sz) const {
        return TStringBuf(S, sz);
    }
};

static bool CanUnescape(TStringBuf s) {
    static str_spn alpha("a-zA-Z_@$", true);
    static str_spn alnum("a-zA-Z_@$0-9", true);
    static TStringBuf true0("true");
    static TStringBuf false0("false");
    static TStringBuf true1("on"), true2("da"); // old scheme sucks
    static TStringBuf false1("off"), false2("net"); // old scheme sucks

    static TStringBuf null0("null");

    return !!s && alpha.chars_table[(ui8)s[0]] && alnum.cbrk(s.begin() + 1, s.end()) == s.end()
        && !NAccessors::In(s, null0, true0, false0, true1, false1, true2, false2);
}

class TPrettifier : public TJsonCallbacks {
    TRewritableOut Out;
    TStringBuf Spaces;
    TStringBuf Quote;
    TStringBuf Unsafe;
    TStringBuf Safe;

    ui32 Level;

    bool Unquote;
    bool Compactify;

public:

    TPrettifier(TOutputStream& out, const TJsonPrettifier& p)
        : Out(out)
        , Level()
        , Unquote(p.Unquote)
        , Compactify(p.Compactify)
    {
        static TSpaces spaces;
        Spaces = spaces.Get(p.Padding);
        if (p.SingleQuotes) {
            Quote = Unsafe = "'";
            Safe = "\"";
        } else {
            Quote = Unsafe = "\"";
            Safe = "'";
        }
    }

    void Pad(bool close = false) {
        if (Compactify) {
            Out.Flush();
            return;
        }

        if (Level || close)
            Out.Write("\n");
        for (ui32 i = 0; i < Level; ++i)
            Out.Write(Spaces);
    }

    void WriteSpace(char sp) {
        if (Compactify) {
            Out.Flush();
            return;
        }

        Out.Write(sp);
    }

    void OnVal() {
        if (Out.Dirty && ':' == Out.Last) {
            WriteSpace(' ');
        } else {
            Pad();
        }
    }

    void AfterVal() {
        Out.Hold(',');
    }

    template <typename T>
    bool WriteVal(const T& t) {
        OnVal();
        Out.Write(t);
        AfterVal();
        return true;
    }

    bool OnNull() {
        return WriteVal(STRINGBUF("null"));
    }

    bool OnBoolean(bool v) {
        return WriteVal(v ? STRINGBUF("true") : STRINGBUF("false"));
    }

    bool OnInteger(long long i) {
        return WriteVal(i);
    }

    bool OnUInteger(unsigned long long i) {
        return WriteVal(i);
    }

    bool OnDouble(double d) {
        return WriteVal(d);
    }

    void WriteString(TStringBuf s) {
        if (Unquote && CanUnescape(s)) {
            Out.Slave << s;
        } else {
            Out.Slave << Quote;
            NEscJ::EscapeJ<false, true>(s, Out.Slave, Safe, Unsafe);
            Out.Slave << Quote;
        }
    }

    bool OnString(const TStringBuf& s) {
        OnVal();
        WriteString(s);
        AfterVal();
        return true;
    }

    bool OnOpen(char c) {
        OnVal();
        Level++;
        Out.Hold(c);
        return true;
    }

    bool OnOpenMap() {
        return OnOpen('{');
    }

    bool OnOpenArray() {
        return OnOpen('[');
    }

    bool OnMapKey(const TStringBuf& k) {
        OnVal();
        WriteString(k);
        WriteSpace(' ');
        Out.Hold(':');
        return true;
    }

    bool OnClose(char c) {
        if (!Level)
            return false;

        Level--;

        if (Out.Dirty && c == Out.Last) {
            WriteSpace(' ');
        } else {
            Out.Revert();
            Pad(true);
        }

        return true;
    }

    bool OnCloseMap() {
        if (!OnClose('{'))
            return false;
        Out.Write("}");
        AfterVal();
        return true;
    }

    bool OnCloseArray() {
        if (!OnClose('['))
            return false;
        Out.Write("]");
        AfterVal();
        return true;
    }

    bool OnEnd() {
        return !Level;
    }
};

bool TJsonPrettifier::Prettify(TStringBuf in, TOutputStream& out) {
    TPrettifier p(out, *this);
    return ReadJsonFast(in, &p);
}

Stroka TJsonPrettifier::Prettify(TStringBuf in) {
    TStringStream s;
    if (Prettify(in, s))
        return s;
    return Stroka();
}

}
