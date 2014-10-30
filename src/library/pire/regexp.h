#pragma once

#include "pire.h"

#include <util/charset/doccodes.h>
#include <util/charset/recyr.hh>
#include <util/generic/chartraits.h>
#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NRegExp {
    struct TMatcher;

    struct TFsmBase {
        struct TOptions {
            inline TOptions()
                : CaseInsensitive(false)
                , Surround(false)
                , CaptureSet(false)
                , Charset(CODES_UNKNOWN)
            {
            }

            inline TOptions& SetCaseInsensitive(bool v) throw () {
                CaseInsensitive = v;
                return *this;
            }

            inline TOptions& SetSurround(bool v) throw () {
                Surround = v;
                return *this;
            }

            inline TOptions& SetCapture(size_t pos) throw () {
                CaptureSet = true;
                CapturePos = pos;
                return *this;
            }

            inline TOptions& SetCharset(ECharset charset) throw () {
                Charset = charset;
                return *this;
            }

            bool CaseInsensitive;
            bool Surround;
            bool CaptureSet;
            size_t CapturePos;
            ECharset Charset;
        };

        static inline NPire::TFsm Parse(const TFixedString<char>& regexp,
            const TOptions& opts)
        {
            NPire::TLexer lexer;
            if (opts.Charset == CODES_UNKNOWN) {
                lexer.Assign(regexp.Start, regexp.Start + regexp.Length);
            } else {
                yvector<wchar32> ucs4(regexp.Length);
                RecodeToUnicode(opts.Charset, regexp.Start, ~ucs4,
                    regexp.Length, regexp.Length);

                lexer.Assign(ucs4.begin(),
                    ucs4.begin() + TCharTraits<wchar32>::GetLength(~ucs4));
            }

            if (opts.CaseInsensitive) {
                lexer.AddFeature(NPire::NFeatures::CaseInsensitive());
            }

            if (opts.CaptureSet) {
                lexer.AddFeature(NPire::NFeatures::Capture(opts.CapturePos));
            }

            switch (opts.Charset) {
            case CODES_UNKNOWN:
                break;
            case CODES_UTF8:
                lexer.SetEncoding(NPire::NEncodings::Utf8());
                break;
            case CODES_KOI8:
                lexer.SetEncoding(NPire::NEncodings::Koi8r());
                break;
            default:
                lexer.SetEncoding(NPire::NEncodings::Get(opts.Charset));
                break;
            }

            NPire::TFsm ret = lexer.Parse();

            if (opts.Surround) {
                ret.Surround();
            }

            ret.Determine();

            return ret;
        }
    };

    template <class TScannerType>
    class TFsmParser: public TFsmBase {
    public:
        typedef TScannerType TScanner;

    public:
        inline explicit TFsmParser(const TFixedString<char>& regexp,
            const TOptions& opts = TOptions())
            : Scanner(Parse(regexp, opts).template Compile<TScanner>())
        {
        }

        inline const TScanner& GetScanner() const throw () {
            return Scanner;
        }

        static inline TFsmParser False() {
            return TFsmParser(NPire::TFsm::MakeFalse().Compile<TScanner>());
        }

    protected:
        inline explicit TFsmParser(const TScanner& compiled)
            : Scanner(compiled)
        {
            if (Scanner.Empty())
                ythrow yexception() << "Can't create fsm with empty scanner";
        }

    private:
        TScanner Scanner;
    };

    class TFsm: public TFsmParser<NPire::TNonrelocScanner> {
    public:
        inline explicit TFsm(const TFixedString<char>& regexp,
            const TOptions& opts = TOptions())
            : TFsmParser<TScanner>(regexp, opts)
        {
        }

        inline TFsm(const TFsmParser<TScanner>& fsm)
            : TFsmParser<TScanner>(fsm)
        {
        }

        static inline TFsm Glue(const TFsm&l, const TFsm& r) {
            return TFsm(TScanner::Glue(l.GetScanner(), r.GetScanner()));
        }

    private:
        inline explicit TFsm(const TScanner& compiled)
            : TFsmParser<TScanner>(compiled)
        {
        }
    };

    static inline TFsm operator | (const TFsm& l, const TFsm& r) {
        return TFsm::Glue(l, r);
    }

    struct TCapturingFsm: TFsmParser<NPire::TCapturingScanner> {
        inline explicit TCapturingFsm(const TFixedString<char>& regexp,
            TOptions opts = TOptions())
            : TFsmParser<TScanner>(regexp,
                opts.SetSurround(true).CaptureSet ? opts : opts.SetCapture(1))
        {
        }

        inline TCapturingFsm(const TFsmParser<TScanner>& fsm)
            : TFsmParser<TScanner>(fsm)
        {
        }
    };

    template <class TFsm>
    class TMatcherBase {
    public:
        typedef typename TFsm::TScanner::State TState;

    public:
        inline explicit TMatcherBase(const TFsm& fsm)
            : Fsm(fsm)
        {
            Fsm.GetScanner().Initialize(State);
        }

        inline bool Final() const throw () {
            return GetScanner().Final(GetState());
        }

    protected:
        inline void Run(const char* data, size_t len, bool wrap = false) throw () {
            if (wrap) {
                NPire::Step(GetScanner(), State, NPire::BeginMark);
            }
            NPire::Run(GetScanner(), State, data, data + len);
            if (wrap) {
                NPire::Step(GetScanner(), State, NPire::EndMark);
            }
        }

        inline const typename TFsm::TScanner& GetScanner() const throw () {
            return Fsm.GetScanner();
        }

        inline const TState& GetState() const throw () {
            return State;
        }

    private:
        const TFsm& Fsm;
        TState State;
    };

    struct TMatcher: TMatcherBase<TFsm> {
        inline explicit TMatcher(const TFsm& fsm)
            : TMatcherBase<TFsm>(fsm)
        {
        }

        inline TMatcher& Match(const char* data, size_t len) throw () {
            Run(data, len);
            return *this;
        }

        inline TMatcher& Match(const TFixedString<char>& s) throw () {
            return Match(s.Start, s.Length);
        }

        inline const char* Find(const char* b, const char* e) throw () {
            return NPire::ShortestPrefix(GetScanner(), b, e);
        }

        typedef TPair<const size_t*, const size_t*> TMatchedRegexps;

        inline TMatchedRegexps MatchedRegexps() const throw () {
            return GetScanner().AcceptedRegexps(GetState());
        }
    };

    class TSearcher: public TMatcherBase<TCapturingFsm> {
    public:
        inline explicit TSearcher(const TCapturingFsm& fsm)
            : TMatcherBase<TCapturingFsm>(fsm)
        {
        }

        inline bool Captured() const throw () {
            return GetState().Captured();
        }

        inline TSearcher& Search(const char* data, size_t len) throw () {
            Data = TStringBuf(data, len);
            Run(data, len, true);
            return *this;
        }

        inline TSearcher& Search(const TFixedString<char>& s) throw () {
            return Search(s.Start, s.Length);
        }

        inline TStringBuf GetCaptured() const throw() {
            return TStringBuf(~Data + GetState().Begin() - 1,
                ~Data + GetState().End() - 1);
        }

    private:
        TStringBuf Data;
    };
}

