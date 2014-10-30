#include "abc.h"
#include "alphaux.h"

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/system/maxlen.h>

using NLemmer::NDetail::TTransdRet;
using NLemmer::NDetail::TTr;

static size_t GetCharLength(const TChar* word, size_t length, size_t i) {
    size_t ret = NDetail::SymbolSize(word + i, word + length);
    while (i + ret < length && IsCombining(word[i + ret]))
        ++ret;
    return ret;
}

namespace NLemmer {
/////////////////////////////////////////////
///////////////////////////////// TConvertRet
    TConvertRet::TConvertRet(size_t len, bool valid)
    : Valid(valid)
    , Length(len)
    {
    }

    static void SetIfChanged(TConvertRet& rt, EConvert mode, bool isChanged) {
        if (isChanged)
            rt.IsChanged.Set(mode);
    }
    namespace {
/////////////////////////////////////////////
///////////////////////////////// TStepRes
        struct TStepRes: TConvertRet {
            TChar* StepResBuf;
            size_t StepResMaxLength;

            size_t StepResLength;
            size_t StepProcessed;

            TStepRes(size_t length)
            : TConvertRet(length)
            , StepResBuf(0)
            , StepResMaxLength(0)
            , StepResLength(0)
            , StepProcessed(0)
            {
            }

            bool Changed(const TChar* src) const  {
                if (StepResLength != StepProcessed)
                    return true;
                if (StepResLength == 1)
                    return *StepResBuf != *src;
                return memcmp(StepResBuf, src, StepResLength * sizeof(TChar)) != 0;
            }

            bool Set(TChar c) {
                if (!StepResMaxLength)
                    return false;
                YASSERT(StepResBuf);
                StepResLength = 1;
                StepProcessed = 1;
                StepResBuf[0] = c;
                return true;
            }

            bool Set(const TChar* s) {
                YASSERT(!StepResMaxLength || StepResBuf);
                StepProcessed = 1;
                StepResLength = 0;
                if (!s)
                    return true;
                while (*s) {
                    if (StepResLength == StepResMaxLength)
                        return false;
                    StepResBuf[StepResLength++] = *(s++);
                }
                return true;
            }

            bool Set(const wchar32* s) {
                YASSERT(!StepResMaxLength || StepResBuf);
                StepProcessed = 1;
                StepResLength = 0;
                if (!s)
                    return true;
                while (*s) {
                    size_t sz = CharSize(*s);
                    if (StepResLength + sz >= StepResMaxLength)
                        return false;
                    TChar* r = StepResBuf + StepResLength;
                    sz = WriteSymbol(*s, r);
                    StepResLength += sz;
                    ++s;
                }
                return true;
            }

            bool Set(const TChar* s, size_t len) {
                YASSERT(!StepResMaxLength || StepResBuf);
                StepProcessed = 1;
                StepResLength = 0;
                while (len--) {
                    if (StepResLength == StepResMaxLength)
                        return false;
                    StepResBuf[StepResLength++] = *(s++);
                }
                return true;
            }
        private:
            static size_t CharSize(wchar32 c) {
                if (c <= 0xFFFF)
                    return 1;
                return 2;
            }
        };

/////////////////////////////////////////////
///////////////////////////////// TBuffSelector
        class TBuffSelector {
        private:
            static const size_t staticBufferSize = 1024;
            typedef NLemmerAux::TBuffer<staticBufferSize> TBuf;
        private:
            TBuf Buffers[2];
            size_t CurrentBuf;
        public:
            TBuffSelector()
                : CurrentBuf(0)
            {}

            void Switch() {
                CurrentBuf = !CurrentBuf;
            }

            TBuf& CurrentBuffer() {
                return Buffers[CurrentBuf];
            }

            const TBuf& CurrentBuffer() const {
                return Buffers[CurrentBuf];
            }

            TBuf& OtherBuffer() {
                return Buffers[!CurrentBuf];
            }

            const TBuf& OtherBuffer() const {
                return Buffers[!CurrentBuf];
            }
        };

/////////////////////////////////////////////
///////////////////////////////// TFromToSelector
        class TFromToSelector {
            TBuffSelector Buffers;
            const TChar* InitialSource;
            TChar* FinalDest;
            const size_t FinalDestSize;

            bool ReadBuffer;
            bool WriteBuffer;
        public:
            TFromToSelector(const TChar* word, TChar* converted, size_t bufLen)
                : InitialSource(word)
                , FinalDest(converted)
                , FinalDestSize(bufLen)
                , ReadBuffer(false)
                , WriteBuffer(true)
            {}

            void SwitchSource() {
                Buffers.Switch();
                ReadBuffer = true;
            }

            void SwitchToFinalDest() {
                WriteBuffer = false;
            }

            void TryToMulltiplyBuffer(size_t mul) {
                if (WriteBuffer)
                    Buffers.CurrentBuffer()(FinalDestSize * mul);
            }

            const TChar* GetSource() const {
                if (ReadBuffer)
                    return Buffers.OtherBuffer().GetPtr();
                return InitialSource;
            }

            TChar* GetDest() {
                if (WriteBuffer)
                    return Buffers.CurrentBuffer()(0);
                return FinalDest;
            }

            size_t GetDestSize() const {
                if (WriteBuffer)
                    return Buffers.CurrentBuffer().GetSize();
                return FinalDestSize;
            }

            bool IsFinal() const {
                return !WriteBuffer;
            }
            void ResetFinal() {
                WriteBuffer = true;
            }
        };
    }

/////////////////////////////////////////////
///////////////////////////////// TAlphabetWordNormalizer::THelper
    class TAlphabetWordNormalizer::THelper {
    public:
        struct TTrCaller {
            const TTr* Tr;
            const EConvert Mode;

            TTrCaller(const TTr* tr, const EConvert mode)
                : Tr(tr)
                , Mode(mode)
            {}

            bool Skip() const {
                return !Tr;
            }
            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const {// returns "false" if did nothing
                if (!Tr)
                    return false;

                ret.StepProcessed = fromLength;
                TTransdRet r = (*Tr)(from, fromLength, ret.StepResBuf, ret.StepResMaxLength);
                ret.Length = r.Length;
                if (!r.Valid)
                    ret.Valid = false;
                SetIfChanged(ret, Mode, r.Changed);
                return true;
            }

            bool operator() (const TChar* word, size_t, size_t i, TStepRes& ret) const { // returns "true" if valid
                if (!Tr)
                    return ret.Set(word[i]);

                ret.StepProcessed = 1;
                TTransdRet r = (*Tr)(word + i, 1, ret.StepResBuf, ret.StepResMaxLength);
                ret.StepResLength = r.Length;
                return r.Valid;
            }
        };

        struct TDerenyxer: public TTrCaller {
            static const size_t BufMultiplier = 2;

            TDerenyxer(const TAlphabetWordNormalizer& a)
                : TTrCaller(a.Derenyxer, CnvDerenyx)
            {}
        };

        struct TConverter: public TTrCaller {
            static const size_t BufMultiplier = 2;

            TConverter(const TAlphabetWordNormalizer& a)
                : TTrCaller(a.Converter, CnvConvert)
            {}
        };

        struct TCaseCaller {
            const TTr* PreCaser;
            void (*Caser)(const TChar*, size_t, TChar*);

            TCaseCaller(const TTr* pre, void (*caser)(const TChar*, size_t, TChar*))
            : PreCaser(pre)
            , Caser(caser)
            {
                YASSERT(Caser);
            }

            bool Proceed(const TChar* from, size_t fromLength, TStepRes& ret) const {
                if (fromLength > ret.StepResMaxLength) {
                    ret.Valid = false;
                    fromLength = ret.StepResMaxLength;
                }
                ret.StepProcessed = fromLength;
                bool val = true;
                const TChar* ptr = from;
                if (PreCaser) {
                    TTransdRet r = (*PreCaser)(from, fromLength, ret.StepResBuf, ret.StepResMaxLength);
                    ptr = ret.StepResBuf;
                    fromLength = r.Length;
                    if (!r.Valid)
                        val = false;
                }
                (*Caser)(ptr, fromLength, ret.StepResBuf);
                ret.StepResLength = fromLength;
                return val;
            }
        };

        struct TLowerCaser {
            static const size_t BufMultiplier = 2;
            static const EConvert Mode = CnvLowCase;

            TCaseCaller Impl;

            TLowerCaser(const TAlphabetWordNormalizer& a)
                : Impl(a.PreLowerCaser, ::ToLower)
            {}

            bool Skip() const {
                return false;
            }

            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const { // returns "false" if did nothing
                if (!Proceed(from, fromLength, ret))
                    ret.Valid = false;
                ret.Length = ret.StepResLength;
                SetIfChanged(ret, Mode, true);
                return true;
            }

            bool operator() (const TChar* word, size_t length, size_t i, TStepRes& ret) const { // returns "true" if valid
                size_t len = GetCharLength(word, length, i);
                return Proceed(word + i, len, ret);
            }

            bool Proceed(const TChar* from, size_t fromLength, TStepRes& ret) const {
                bool val = Impl.Proceed(from, fromLength, ret);
                if (!ret.StepResLength)
                    return val;
                SetIfChanged(ret, Mode, combineI(ret.StepResBuf, ret.StepResLength));
                return val;
            }
        private:
            static bool combineI(TChar* s, size_t& len) {
                bool changed = false;
                for (size_t i = len - 1; i > 0; --i) {
                    if ((s[i - 1] == 'i' || s[i - 1] == 0x131) &&  s[i] > 0x200) {
                        size_t dti = findDot(s + (i - 1), len - (i - 1));
                        if (!dti)
                            continue;
                        --len;
                        s[i - 1] = 'i';
                        for (size_t j = dti + i - 1; j < len; ++j)
                            s[j] = s[j + 1];
                        changed = true;
                    }
                }
                return changed;
            }
            static size_t findDot(const TChar* s, size_t len) {
                for (size_t i = 1; i < len; ++i) {
                    if (s[i] == 0x307)
                        return i;
                    if (!IsCombining(s[i]))
                        break;
                }
                return 0;
            }
        };

        struct TUpperCaser: public TCaseCaller {
            static const size_t BufMultiplier = 2;
            static const EConvert Mode = CnvUpCase;

            TUpperCaser(const TAlphabetWordNormalizer& a)
                : TCaseCaller(a.PreUpperCaser, ::ToUpper)
            {}

            bool Skip() const {
                return false;
            }

            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const { // returns "false" if did nothing
                if (!Proceed(from, fromLength, ret))
                    ret.Valid = false;
                ret.Length = ret.StepResLength;
                SetIfChanged(ret, Mode, true);
                return true;
            }

            bool operator() (const TChar* word, size_t length, size_t i, TStepRes& ret) const { // returns "true" if valid
                size_t len = GetCharLength(word, length, i);
                return Proceed(word + i, len, ret);
            }
        };

        struct TTitleCaser {
            static const size_t BufMultiplier = 2;
            static const EConvert Mode = CnvTitleCase;

            TCaseCaller TitleCaser;
            TLowerCaser LowerCaser;
            TTitleCaser(const TAlphabetWordNormalizer& a)
                : TitleCaser(a.PreTitleCaser, ::ToTitle)
                , LowerCaser(a)
            {}

            bool Skip() const {
                return false;
            }

            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const { // returns "false" if did nothing
                size_t len = GetCharLength(from, fromLength, 0);

                TitleCaser.Proceed(from, len, ret);
                size_t lenTo = ret.StepResLength;
                ret.StepResBuf += lenTo;
                ret.StepResMaxLength -= lenTo;
                LowerCaser.Proceed(from + len, fromLength - len, ret);

                ret.StepResLength += lenTo;
                ret.StepResBuf -= lenTo;
                ret.StepResMaxLength += lenTo;
                ret.Length = ret.StepResLength;
                SetIfChanged(ret, Mode, true);
                return true;
            }

            bool operator() (const TChar* word, size_t length, size_t i, TStepRes& ret) const { // returns "true" if valid
                size_t len = GetCharLength(word, length, i);
                if (i == 0)
                    return TitleCaser.Proceed(word, len, ret);
                else
                    return LowerCaser.Proceed(word + i, len, ret);
            }
        };

        struct TGlobalComposer {
            TTransdRet (*Processor)(const TChar* str, size_t len, TChar* buf, size_t bufLen);
            const EConvert Mode;

            TGlobalComposer(TTransdRet (*processor)(const TChar* str, size_t len, TChar* buf, size_t bufLen), const EConvert mode)
                : Processor(processor)
                , Mode(mode)
            {
                YASSERT(Processor);
            }

            bool Skip() const {
                return false;
            }

            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const { // returns "false" if did nothing
                TTransdRet r = (*Processor)(from, fromLength, ret.StepResBuf, ret.StepResMaxLength);
                ret.StepResLength = r.Length;
                ret.StepProcessed = fromLength;
                ret.Length = ret.StepResLength;
                if (!r.Valid)
                    ret.Valid = false;
                SetIfChanged(ret, Mode, r.Changed);
                return true;
            }

            bool operator() (const TChar* word, size_t length, size_t i, TStepRes& ret) const { // returns "true" if valid
                ret.StepProcessed = GetCharLength(word, length, i);
                TTransdRet r = (*Processor)(word + i, ret.StepProcessed, ret.StepResBuf, ret.StepResMaxLength);
                ret.StepResLength = r.Length;
                return r.Valid;
            }
        };
        struct TDecomposer: public TGlobalComposer {
            static const size_t BufMultiplier = 4;

            TDecomposer(const TAlphabetWordNormalizer&)
             : TGlobalComposer(NLemmerAux::Decompose, CnvDecompose)
             {};
        };

        struct TSoftDecomposer: public TGlobalComposer {
            static const size_t BufMultiplier = 4;

            TSoftDecomposer(const TAlphabetWordNormalizer&)
             : TGlobalComposer(NLemmerAux::SoftDecompose, CnvSoftDecompose)
             {};
        };

        struct TComposer: public TGlobalComposer {
            static const size_t BufMultiplier = 2;

            const TAlphabet* Alpha;
            TComposer(const TAlphabetWordNormalizer& a)
            : TGlobalComposer(NLemmerAux::Compose, CnvCompose)
            , Alpha(a.Alphabet)
            {}

            TComposer(const TAlphabetWordNormalizer& a, TTransdRet (*processor)(const TChar* str, size_t len, TChar* buf, size_t bufLen))
            : TGlobalComposer(processor, CnvSoftCompose)
            , Alpha(a.Alphabet)
            {}

            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const { // returns "false" if did nothing
                if (!Alpha)
                    return TGlobalComposer::operator()(from, fromLength, ret);
                TTransdRet r = NLemmer::NDetail::ComposeWord(*Alpha, from, fromLength, ret.StepResBuf, ret.StepResMaxLength);
                ret.StepProcessed = fromLength;
                ret.StepResLength = r.Length;
                if (!r.Valid)
                    ret.Valid = false;
                SetIfChanged(ret, Mode, r.Changed);
                ret.Length = ret.StepResLength;
                return true;
            }

            bool operator() (const TChar* word, size_t length, size_t i, TStepRes& ret) const { // returns "true" if valid
                if (!Alpha)
                    return TGlobalComposer::operator()(word, length, i, ret);
                TTransdRet r = NLemmer::NDetail::ComposeAlpha(*Alpha, word + i, length - i, ret.StepResBuf, ret.StepResMaxLength);
                ret.StepResLength = r.Length;
                ret.StepProcessed = r.Processed;
                return r.Valid;
            }

        };
        struct TSoftComposer: public TComposer {
            static const size_t BufMultiplier = 2;

            const TAlphabet* Alpha;
            TSoftComposer(const TAlphabetWordNormalizer& a)
            : TComposer(a, NLemmerAux::SoftCompose)
            {}
        };
        struct TAdvConverter {
            static const EConvert Mode = CnvAdvancedConvert;
            static const size_t BufMultiplier = 2;

            const TAlphabetWordNormalizer& Alpha;

            TAdvConverter(const TAlphabetWordNormalizer& a)
                : Alpha(a)
            {}

            bool Skip() const {
                return false;
            }

            bool operator() (const TChar* from, size_t fromLength, TStepRes& ret) const { // returns "true" if changed
                return Alpha.AdvancedConvert(from, fromLength, ret.StepResBuf, ret.StepResMaxLength, ret);
            }
        };

        template<class TTrans>
        static void Caller(TFromToSelector& fromTo, TStepRes& ret, TConvertMode& mode, const TTrans& tr) {
            if (!ret.Valid || !mode.Test(tr.Mode))
                return;
            mode.Reset(tr.Mode);
            if (tr.Skip())
                return;

            if (!mode.any())
                fromTo.SwitchToFinalDest();
            fromTo.TryToMulltiplyBuffer(tr.BufMultiplier);
            ret.StepResBuf = fromTo.GetDest();
            ret.StepResMaxLength = fromTo.GetDestSize();
            if (tr(fromTo.GetSource(), ret.Length, ret)) {
                if (!fromTo.IsFinal())
                    fromTo.SwitchSource();
            } else if (fromTo.IsFinal()) {
                fromTo.ResetFinal();
            }
        }

        template<class TTrans>
        static NLemmer::TConvertRet Caller(const TChar* from, size_t fromLength, TChar* resBuffer, size_t bufferSize, const TTrans& tr) {
            TStepRes ret(fromLength);
            ret.StepResBuf = resBuffer;
            ret.StepResMaxLength = bufferSize;
            bool skipped = tr.Skip() || !tr(from, fromLength, ret);
            SetIfChanged(ret, tr.Mode, !skipped);

            if (skipped) {
                ret.Length = Min(fromLength, bufferSize);
                ret.Valid = ret.Length == fromLength;
                memcpy(resBuffer, from, ret.Length * sizeof(TChar));
            }
            if (ret.Length < bufferSize)
                resBuffer[ret.Length] = 0;
            return ret;
        }

        template<class TTrans>
        static Wtroka CallerInt(const Wtroka& s, size_t i, TStepRes& ret, const TTrans& tr) {
            Wtroka resBuffer;
            resBuffer.reserve(+s);
            resBuffer.append(~s, i);
            resBuffer.append(ret.StepResBuf, ret.StepResLength);
            i += ret.StepProcessed;
            while (i < +s) {
                if (!tr(~s, +s, i, ret))
                    ret.Valid = false;
                resBuffer.append(ret.StepResBuf, ret.StepResLength);
                i += ret.StepProcessed;
            }
            return resBuffer;
        }

        template<class TTrans>
        static void Caller(Wtroka& s, TStepRes& ret, const TTrans& tr) {
            if (tr.Skip())
                return;
            size_t i = 0;
            while (i < +s) {
                if (!tr(~s, +s, i, ret))
                    ret.Valid = false;

                if (ret.Changed(~s + i)) {
                    s = CallerInt(s, i, ret, tr);
                    ret.IsChanged.Set(tr.Mode);
                    break;
                }
                i += ret.StepProcessed;
            }

            ret.Length = s.length();
        }

        template<class TTrans>
        static void Caller(Wtroka& s, TStepRes& ret, TConvertMode& mode, const TTrans& tr) {
            if (!mode.Test(tr.Mode) || !ret.Valid)
                return;
            mode.Reset(tr.Mode);
            Caller(s, ret, tr);
        }

        template<class TTrans>
        static NLemmer::TConvertRet Caller(Wtroka& s, const TTrans& tr) {
            static const size_t bufferSize = 64;
            TChar buffer[bufferSize];

            TStepRes ret(+s);
            ret.StepResBuf = buffer;
            ret.StepResMaxLength = bufferSize;

            Caller(s, ret, tr);
            return ret;
        }

        static TChar CaseAdapter(TChar c, const TTr* preCaser, wchar32 (*caser)(wchar32)) {
            if (preCaser)
                (*preCaser)(&c, 1);
            return static_cast<TChar>((*caser)(c));
        }
    };

/////////////////////////////////////////////
///////////////////////////////// TAlphabetWordNormalizer
    TAlphabetWordNormalizer::TAlphabetWordNormalizer(
            const TAlphabet* alphabet,
            const TTr* preConverter,
            const TTr* derenyxer,
            const TTr* converter,
            const TTr* preLowerCaser,
            const TTr* preUpperCaser,
            const TTr* preTitleCaser,
            const TDiacriticsMap* diacriticsMap)
        : Alphabet (alphabet)
        , PreConverter(preConverter)
        , Derenyxer (derenyxer)
        , Converter (converter)
        , PreLowerCaser(preLowerCaser)
        , PreUpperCaser(preUpperCaser)
        , PreTitleCaser(preTitleCaser)
        , DiacriticsMap(diacriticsMap)
    {}

    TChar TAlphabetWordNormalizer::ToLower(TChar a) const {
        return THelper::CaseAdapter(a, PreLowerCaser, ::ToLower);
    }

    TChar TAlphabetWordNormalizer::ToUpper(TChar a) const {
        return THelper::CaseAdapter(a, PreUpperCaser, ::ToUpper);
    }

    TChar TAlphabetWordNormalizer::ToTitle(TChar a) const {
        return THelper::CaseAdapter(a, PreTitleCaser, ::ToTitle);
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::ToLower(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TLowerCaser(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::ToLower(Wtroka& s) const {
        return THelper::Caller(s, THelper::TLowerCaser(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::ToUpper(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TUpperCaser(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::ToUpper(Wtroka& s) const {
        return THelper::Caller(s, THelper::TUpperCaser(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::ToTitle(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TTitleCaser(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::ToTitle(Wtroka& s) const {
        return THelper::Caller(s, THelper::TTitleCaser(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::Decompose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TDecomposer(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::Decompose(Wtroka& s) const {
        return THelper::Caller(s, THelper::TDecomposer(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::Compose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TComposer(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::Compose(Wtroka& s) const {
        return THelper::Caller(s, THelper::TComposer(*this));
    }

    //versions with softer normalization (use Normalization::NFD, Normalization::NFC)
    NLemmer::TConvertRet TAlphabetWordNormalizer::SoftDecompose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TSoftDecomposer(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::SoftDecompose(Wtroka& s) const {
        return THelper::Caller(s, THelper::TSoftDecomposer(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::SoftCompose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TSoftComposer(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::SoftCompose(Wtroka& s) const {
        return THelper::Caller(s, THelper::TSoftComposer(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::Derenyx(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TDerenyxer(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::Derenyx(Wtroka& s) const {
        return THelper::Caller(s, THelper::TDerenyxer(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::Convert(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TConverter(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::Convert(Wtroka& s) const {
        return THelper::Caller(s, THelper::TConverter(*this));
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::AdvancedConvert(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
        return THelper::Caller(word, length, converted, bufLen, THelper::TAdvConverter(*this));
    }
    NLemmer::TConvertRet TAlphabetWordNormalizer::AdvancedConvert(Wtroka& s) const {
        TConvertRet ret(+s);
        AdvancedConvert(s, ret);
        return ret;
    }

    static const size_t COMPOSE_BUFFER_SIZE = MAXWORD_BUF * 2;

    NLemmer::TConvertRet TAlphabetWordNormalizer::PreConvert(
            const TChar* word,
            size_t length,
            TChar* resultBuffer,
            size_t bufferLength) const
    {
        TConvertRet result(length);

        TChar composeBuffer[COMPOSE_BUFFER_SIZE];
        TTransdRet composeResult;
        if (Alphabet) {
            composeResult = NLemmer::NDetail::ComposeWord(
                *Alphabet, word, length, composeBuffer, COMPOSE_BUFFER_SIZE);
        } else {
            composeResult = NLemmerAux::Compose(
                word, length, composeBuffer, COMPOSE_BUFFER_SIZE);
        }
        size_t composeLength = composeResult.Length;
        result.Valid = composeResult.Valid;
        result.Length = composeLength;

        if (PreConverter != NULL) {
            YASSERT(composeLength <= bufferLength);
            (*PreConverter)(composeBuffer,
                            composeLength,
                            resultBuffer,
                            bufferLength);
        } else {
            memcpy(resultBuffer, composeBuffer, composeLength * sizeof(TChar));
        }

        return result;
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::Normalize(const TChar* word, size_t length, TChar* converted, size_t bufLen, TConvertMode mode) const {
        TFromToSelector fromTo(word, converted, bufLen);
        TStepRes ret(length);

        const bool check = mode.Test(CnvCheckCharacters);
        mode.Reset(CnvCheckCharacters);
        THelper::Caller(fromTo, ret, mode, THelper::TDecomposer(*this));
        THelper::Caller(fromTo, ret, mode, THelper::TComposer(*this));
        THelper::Caller(fromTo, ret, mode, THelper::TLowerCaser(*this));
        THelper::Caller(fromTo, ret, mode, THelper::TDerenyxer(*this));
        THelper::Caller(fromTo, ret, mode, THelper::TConverter(*this));
        THelper::Caller(fromTo, ret, mode, THelper::TAdvConverter(*this));

        if (fromTo.GetDest() != converted) {
            if (ret.Length > bufLen)
                ret.Length = bufLen;
            memcpy(converted, fromTo.GetSource(), ret.Length * sizeof(TChar));
        }
        if (ret.Length < bufLen)
            converted[ret.Length] = 0;

        if (ret.Valid && check)
            ret.Valid = IsNormalized(converted, ret.Length);

        return ret;
    }

    NLemmer::TConvertRet TAlphabetWordNormalizer::Normalize(Wtroka& s, TConvertMode mode) const {
        static const size_t bufferSize = 64;
        TChar buffer[bufferSize];

        TStepRes ret(+s);
        ret.StepResBuf = buffer;
        ret.StepResMaxLength = bufferSize;

        THelper::Caller(s, ret, mode, THelper::TDecomposer(*this));
        THelper::Caller(s, ret, mode, THelper::TComposer(*this));
        THelper::Caller(s, ret, mode, THelper::TLowerCaser(*this));
        THelper::Caller(s, ret, mode, THelper::TDerenyxer(*this));
        THelper::Caller(s, ret, mode, THelper::TConverter(*this));

        if (ret.Valid && mode.Test(CnvAdvancedConvert))
            AdvancedConvert(s, ret);
        if (ret.Valid && mode.Test(CnvCheckCharacters))
            ret.Valid = IsNormalized(s);

        return ret;
    }

    bool TAlphabetWordNormalizer::AdvancedConvert(const TChar*, size_t, TChar*, size_t, TConvertRet&) const {
        return false;
    }

    bool TAlphabetWordNormalizer::AdvancedConvert(Wtroka&, TConvertRet&) const {
        return false;
    }

    bool TAlphabetWordNormalizer::IsNormalized(const TChar* word, size_t length) const {
        if (!Alphabet)
            return true;
        static const TAlphabet::TCharClass checkClass = TAlphabet::CharAlphaNormal | TAlphabet::CharSign;

        for (size_t i = 0; i < length; ++i) {
            if (!(Alphabet->CharClass(word[i]) & checkClass))
                return false;
        }
        return true;
    }
}
