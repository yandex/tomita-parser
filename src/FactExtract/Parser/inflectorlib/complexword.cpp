#include "complexword.h"
#include "gramfeatures.h"
#include "ylemma.h"
#include "numeral.h"
#include "better.h"


namespace NInfl {


static inline bool IsBastard(const TYandexLemma& lemma) {
    //consider prefixoids as non-bastards but good dictionary word
    return lemma.IsBastard() && (lemma.GetQuality() & TYandexLemma::QPrefixoid) == 0;
}

static inline size_t GrammemDiff(const TGramBitSet& g1, const TGramBitSet& g2) {
    return (g1 ^ g2).count();
}

static inline size_t SpecialGrammemDiff(const TGramBitSet& g1, const TGramBitSet& g2) {
    return g2.any() ? GrammemDiff(g1, g2) : 0;
}

static void ResetFlexion(TYandexLemma& lemma, size_t flex) {
    YASSERT(flex < lemma.FlexGramNum());
    TYemmaInflector::ResetFlexGram(lemma, lemma.GetFlexGram()[flex]);
}


struct TYemmaSelector {
    TWtringBuf LemmaText;
    TGramBitSet RequestedGrammems;

    const TYandexLemma* Best;
    size_t BestFlex;

    TAgreementMeasure BestAgreed;
    TAgreementMeasure BestRequested;

    bool FoundNonBastard;
    bool TryAgree_;

    TGramBitSet AgreeWithGrammems;

    // lemmaText could be empty, in this case it is not checked
    TYemmaSelector(const TWtringBuf& lemmaText = TWtringBuf(), const TGramBitSet& requested = TGramBitSet())
        : LemmaText(lemmaText)
        , RequestedGrammems(requested)
        , Best(NULL)
        , BestFlex(0)
        , FoundNonBastard(false)
        , TryAgree_(false)
    {
    }

    void TryAgreeWith(const TGramBitSet& grammems) {
        TryAgree_ = true;
        AgreeWithGrammems = grammems;
    }

    bool CanBeAccepted(const TYandexLemma& lemma) {
        if (FoundNonBastard && IsBastard(lemma))
            return false;

        // compare requested lemma text (if non-empty)
        if (!LemmaText.empty() && LemmaText != TWtringBuf(lemma.GetText(), lemma.GetTextLength()))
            return false;

        return true;
    }

    static TAgreementMeasure RequestedAgrMeasure(const TGramBitSet& g1, const TGramBitSet& g2) {
        TAgreementMeasure ret(g1, g2);
        ret.AgreedImplicitly = 0;   // do not count implicit agreements
        return ret;
    }

    TAgreementMeasure RequestedAgrMeasure(const TGramBitSet& g) {
        return RequestedAgrMeasure(RequestedGrammems, g);
    }

    void SelectIfBetter(const TYandexLemma& lemma, size_t flexIndex) {
        YASSERT(CanBeAccepted(lemma));
        TGramBitSet g = TComplexWord::ExtractFormGrammems(lemma, flexIndex);

        // do not allow controversy of requested features
        if (!DefaultFeatures().NonControversial(RequestedGrammems, g))
            return;

        NCmp::EResult state = (Best == NULL) ? NCmp::BETTER : NCmp::EQUAL;

        TAgreementMeasure curRequested = RequestedAgrMeasure(g);
        if (state == NCmp::EQUAL) {
            state = curRequested.Compare(BestRequested);
            if (state == NCmp::WORSE)
                return;
        }

        const TGramBitSet curMutable = DefaultFeatures().Mutable(g);
        TAgreementMeasure curAgreed;
        if (TryAgree_) {
            curAgreed = TAgreementMeasure(AgreeWithGrammems, curMutable);
            if (state == NCmp::EQUAL)
                state = curAgreed.Compare(BestAgreed);
        }

        if (state == NCmp::BETTER) {
            Best = &lemma;
            BestFlex = flexIndex;
            if (!IsBastard(lemma))
                FoundNonBastard = true;

            BestAgreed = curAgreed;
            BestRequested = curRequested;
        }
    }

    bool IsBetter(const TYemmaSelector& a) const {
        if (Best == NULL)
            return false;
        if (a.Best == NULL)
            return true;

        if (FoundNonBastard != a.FoundNonBastard)
            return a.FoundNonBastard;

        NCmp::EResult agrState = BestRequested.Compare(a.BestRequested);
        if (agrState != NCmp::EQUAL)
            return agrState == NCmp::BETTER;

        return BestAgreed.Compare(a.BestAgreed) == NCmp::BETTER;
    }

    bool SelectFrom(const TYandexLemma& candidate, TYandexLemma& result) {

        if (CanBeAccepted(candidate))
            for (size_t flex = 0; flex < candidate.FlexGramNum(); ++flex)
                SelectIfBetter(candidate, flex);

        if (Best != NULL) {
            result = *Best;
            ResetFlexion(result, BestFlex);
            return true;
        } else
            return false;
    }

    bool SelectFrom(const yvector<TYandexLemma>& candidates, TYandexLemma& result) {
        for (size_t i = 0; i < candidates.size(); ++i) {
            const TYandexLemma& yemma = candidates[i];
            if (CanBeAccepted(yemma))
                for (size_t flex = 0; flex < yemma.FlexGramNum(); ++flex)
                    SelectIfBetter(yemma, flex);
        }

        if (Best != NULL) {
            result = *Best;
            ResetFlexion(result, BestFlex);
            return true;
        } else
            return false;
    }
};


TComplexWord::TComplexWord(const TLangMask& langMask, const Wtroka& wordText, const Wtroka& lemmaText,
                           const TGramBitSet& knownGrammems, const TYandexLemma* knownLemma, bool useKnownGrammemsInLemmer)
    : LangMask(langMask)
    , WordTextBuffer(wordText), LemmaTextBuffer(lemmaText)
    , WordText(WordTextBuffer), LemmaText(LemmaTextBuffer)
    , KnownGrammems(knownGrammems)
    , UseKnownGrammemsInLemmer(useKnownGrammemsInLemmer)
    , HasYemma(false)
    , Analyzed(false)
{
    ResetYemma(knownLemma);
}

TComplexWord::TComplexWord(const TLangMask& langMask, const TWtringBuf& wordText, const TWtringBuf& lemmaText,
                           const TGramBitSet& knownGrammems, const TYandexLemma* knownLemma, bool useKnownGrammemsInLemmer)
    : LangMask(langMask)
    , WordText(wordText), LemmaText(lemmaText)
    , KnownGrammems(knownGrammems)
    , UseKnownGrammemsInLemmer(useKnownGrammemsInLemmer)
    , HasYemma(false)
    , Analyzed(false)
{
    ResetYemma(knownLemma);
}

TComplexWord::TComplexWord(const Wtroka& wordText, const yvector<TYandexLemma>& yemmas)
    : WordText(wordText)
    , UseKnownGrammemsInLemmer(false)
    , CandidateYemmas(yemmas)
    , HasYemma(false)
    , Analyzed(true)
{
    HasYemma = TYemmaSelector().SelectFrom(CandidateYemmas, Yemma);
    if (HasYemma) {
        LangMask.Set(Yemma.GetLanguage());
    }
}

TComplexWord::TComplexWord(const TWtringBuf& wordText, const yvector<TYandexLemma>& yemmas)
    : WordText(wordText)
    , UseKnownGrammemsInLemmer(false)
    , CandidateYemmas(yemmas)
    , HasYemma(false)
    , Analyzed(true)
{
    HasYemma = TYemmaSelector().SelectFrom(CandidateYemmas, Yemma);
    if (HasYemma) {
        LangMask.Set(Yemma.GetLanguage());
    }
}

bool TComplexWord::IsEqualText(const TWtringBuf& a, const TWtringBuf& b) {
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
        if (::ToLower(a[i]) != ::ToLower(b[i]))
            return false;

    return true;
}

void TComplexWord::ResetYemma(const TYandexLemma* knownLemma) {
    if (knownLemma != NULL && IsEqualText(TWtringBuf(knownLemma->GetForma(), knownLemma->GetFormLength()), WordText)) {
        TYemmaSelector sel(LemmaText, KnownGrammems);
        if (sel.SelectFrom(*knownLemma, Yemma)) {
            HasYemma = true;
            return;
        }
    }

    Analyze();
    TYemmaSelector sel(LemmaText, KnownGrammems);
    HasYemma = sel.SelectFrom(CandidateYemmas, Yemma);
}

void TComplexWord::Analyze() {
    if (!Analyzed && !WordText.empty()) {
        Stroka knownGrammemsStr;
        if (UseKnownGrammemsInLemmer)
            KnownGrammems.ToBytes(knownGrammemsStr);
        NLemmer::TAnalyzeWordOpt opt(~knownGrammemsStr);
        NLemmer::AnalyzeWord(~WordText, +WordText, CandidateYemmas, LangMask, NULL, opt);
        Analyzed = true;
    }
}


static inline bool IsWordDelim(const wchar16 chr) {
    switch (chr) {
        case L' ':
        case L'-':
        case L'_':
        case L'.':
        case L'/':
            return true;
        default:
            return false;
    }
}

static inline int StrIdx(int i, const TWtringBuf& sz, bool reverse) {
    return reverse ? (int)sz.size() - i - 1 : i;
}

static TWtringBuf SplitSubword(TWtringBuf& str) {
    // skip heading delimiters
    size_t start = 0;
    for (; start < str.size(); ++start)
        if (!IsWordDelim(str[start]))
            break;

    size_t delim = start + 1;
    for (; delim < str.size(); ++delim)
        if (IsWordDelim(str[delim]))
            break;

    TWtringBuf res = str.SubStr(start, delim - start);
    str.Skip(delim + 1);
    return res;
}

static TWtringBuf RSplitSubword(TWtringBuf& str) {
    // skip tailing delimiters
    size_t stop = str.size();
    for (; stop > 0; --stop)
        if (!IsWordDelim(str[stop - 1]))
            break;

    size_t delim = stop > 0 ? stop - 1 : 0;
    for (; delim > 0; --delim)
        if (IsWordDelim(str[delim - 1]))
            break;

    TWtringBuf res = str.SubStr(delim, stop - delim);
    str.Trunc(delim > 0 ? delim - 1 : 0);
    return res;
}

static inline ui64 FullUpperCaseMask() {
    return ~static_cast<ui64>(0);
}

ui64 GetCapitalizationMask(const TWtringBuf& str, bool& isNumber) {
    ui64 res = 0;
    size_t len = Min<size_t>(str.size(), 64);
    isNumber = true;
    bool hasLower = false;
    for (size_t i = 0; i < len; ++i)
    {
        if (!::IsDigit(str[i]))
            isNumber = false;
        if (::IsUpper(str[i]))
            res |= static_cast<ui64>(1) << i;
        else if (!hasLower && ::IsLower(str[i]))
            hasLower = true;
    }

    if (!hasLower)
        return FullUpperCaseMask();      // meaning all are upper-cased or not letters
    else
        return res;
}

// do not modify @dst if possible
static inline void MakeUpper(Wtroka& dst, size_t index) {
    wchar32 cur = dst[index];
    wchar32 up = ::ToUpper(cur);
    if (cur != up)
        dst.begin()[index] = static_cast<wchar16>(up);
}

static inline bool WordCharEqual(wchar16 a, wchar16 b) {
    return ::ToLower(a) == ::ToLower(b) || (IsWordDelim(a) && IsWordDelim(b));
}

static size_t CommonPrefixSize(const TWtringBuf& a, const TWtringBuf& b) {
    // case insensitive common-prefix size
    size_t minlen = Min<size_t>(+a, +b);
    for (size_t i = 0; i < minlen; ++i)
        if (!WordCharEqual(a[i], b[i]))
            return i;

    return minlen;
}

static size_t CommonSuffixSize(const TWtringBuf& a, const TWtringBuf& b) {
    // case insensitive common-suffix size
    size_t minlen = Min<size_t>(+a, +b);
    for (size_t i = 0; i < minlen; ++i)
        if (!WordCharEqual(a[a.size() - i - 1], b[b.size() - i - 1]))
            return i;

    return minlen;
}

static void ApplyCapitalization(const TWtringBuf& src, Wtroka& dst, size_t beg, size_t end) {
    YASSERT(end <= dst.size());

    if (beg >= end || src.empty())
        return;

    bool isDigit = false;
    ui64 mask = GetCapitalizationMask(src, isDigit);
    bool uppercase = (mask == FullUpperCaseMask());

    // always copy capitalization for the first letter
    if (mask & 1 && !isDigit)
        MakeUpper(dst, beg);

    // next, apply capitalization until the prefix is equal
    for (size_t i = beg; mask != 0 && i < end; ++i, mask >>= 1) {
        if (i - beg >= src.size()) {
            if (uppercase)
                for (; i < end; ++i)
                    MakeUpper(dst, i);
            break;
        } else if (::ToLower(src[i - beg]) != ::ToLower(dst[i]))
            break;

        if (uppercase || mask & 1)
            MakeUpper(dst, i);
    }
}

static void CopyCapitalizationImpl(TWtringBuf src, Wtroka& dst) {
    size_t start = 0;
    while (!src.empty() && start < dst.size()) {
        // @dst could be re-located, so refresh @dstbuf on each iteration
        TWtringBuf dstbuf = TWtringBuf(dst).SubStr(start);

        TWtringBuf wsrc = SplitSubword(src);
        TWtringBuf wdst = SplitSubword(dstbuf);

        size_t pos = wdst.data() - dst.data();
        ApplyCapitalization(wsrc, dst, pos, pos + wdst.size());
        start = dst.size() - dstbuf.size();
    }
}

static void RCopyCapitalizationImpl(TWtringBuf src, Wtroka& dst) {
    size_t stop = dst.size();
    while (!src.empty() && stop > 0) {
        // @dst could be re-located, so refresh @dstbuf on each iteration
        TWtringBuf dstbuf = TWtringBuf(dst).SubStr(0, stop);

        TWtringBuf wsrc = RSplitSubword(src);
        TWtringBuf wdst = RSplitSubword(dstbuf);

        size_t pos = wdst.data() - dst.data();
        ApplyCapitalization(wsrc, dst, pos, pos + wdst.size());
        stop = dstbuf.size();
    }
}

void TComplexWord::CopyCapitalization(TWtringBuf src, Wtroka& dst) {
    size_t pref = CommonPrefixSize(src, dst);
    if (pref >= dst.size() * 2 || pref >= CommonSuffixSize(src, dst))
        CopyCapitalizationImpl(src, dst);
    else
        RCopyCapitalizationImpl(src, dst);
}

static void MakeCamelCase(Wtroka& dst) {
    size_t start = 0;
    while (start < dst.size()) {
        // @dst could be re-located, so refresh @dstbuf on each iteration
        TWtringBuf dstbuf = TWtringBuf(dst).SubStr(start);
        TWtringBuf word = SplitSubword(dstbuf);
        MakeUpper(dst, word.data() - dst.data());
        start = dst.size() - dstbuf.size();
    }
}

static inline void MakeTitleCase(Wtroka& dst) {
    if (dst.size() > 0)
        MakeUpper(dst, 0);
}

static bool IsAbbreviation(TWtringBuf abbr, TWtringBuf full) {
    size_t wc = 0;
    while (!full.empty()) {
        if (wc >= abbr.size())
            return false;
        TWtringBuf word = SplitSubword(full);
        if (::ToUpper(word[0]) != ::ToUpper(abbr[wc]))
            return false;
        ++wc;
        // TODO: recognize mixed-case abbreviations (e.g. "PhD")
        // and abbreviations with flexion (e.g. "PhDs")

    }
    return wc == abbr.size();
}

void TComplexWord::MimicCapitalization(TWtringBuf src, Wtroka& dst) {
    // Possible patterns:
    // 1. @src is a camel-cased words, e.g. "Moscow State University"
    // 2. @src is an abbreviation, fully upper-cased, e.g. "MSU".
    // 3. @src is an abbreviation, but @dst is not a full variant, but just a single-word synonym, e.g. "RF" -> "russia".
    // ... to be continued

    TWtringBuf srcbuf = src;
    size_t srcWordCount = 0;
    bool allTitleCase = true;
    bool allUpperCase = true;
    bool allLowerCase = true;
    while (!srcbuf.empty()) {
        bool isDigit = false;
        ++srcWordCount;
        ui64 mask = GetCapitalizationMask(SplitSubword(srcbuf), isDigit);
        if (mask != FullUpperCaseMask())
            allUpperCase = false;
        if (mask != 0)
            allLowerCase = false;
        if (mask != 1 && !isDigit)
            allTitleCase = false;
    }

    size_t dstWordCount = 0;
    TWtringBuf dstbuf = dst;
    while (!dstbuf.empty()) {
        ++dstWordCount;
        SplitSubword(dstbuf);
    }

    size_t commonPrefixLen = CommonPrefixSize(src, dst);
    if (allLowerCase)
        dst.to_lower();
    else if (srcWordCount == dstWordCount && (commonPrefixLen > 0)) {
        // special case: RF -> Rossia (not ROSSIA)
        if (srcWordCount == 1 && allUpperCase && commonPrefixLen < src.size())
            MakeCamelCase(dst);
        else
            CopyCapitalization(src, dst);
    }
    else if (dstWordCount == 1 && allTitleCase && IsAbbreviation(dst, src))
        dst.to_upper();
    else if (srcWordCount == 1 && allUpperCase)
        MakeCamelCase(dst);
    else if (allTitleCase && srcWordCount > 1)
        MakeCamelCase(dst);
    else if (allTitleCase && srcWordCount == 1)
        MakeTitleCase(dst);                             // Гендиректор -> Генеральный директор (not генеральный Директор)
    else
        CopyCapitalization(src, dst);
}

bool TComplexWord::InflectYemma(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram) const {
    NInfl::TYemmaInflector infl(Yemma, 0);
    if (!infl.InflectSupported(grammems))
        return false;

    infl.ConstructText(res);
    CopyCapitalization(WordText, res);
    if (resgram != NULL)
        *resgram = infl.Grammems();
    return true;
}

bool TComplexWord::InflectInt(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram) const {

    res.clear();
    if (WordText.empty()) {
        if (resgram != NULL)
            *resgram = grammems;
        return true;
    }

    // first, try avoiding bastard inflections
    if (HasYemma && !IsBastard(Yemma) && InflectYemma(grammems, res, resgram))
        return true;

    // try recognizing abbreviated numerals, e.g. "17-iy"
    TNumeralAbbr numeral(LangMask, WordText);
    if (numeral.IsRecognized()) {
        bool found = numeral.Inflect(grammems, res, resgram);
        CopyCapitalization(WordText, res);
        return found;
    }

    // try splitting to more simple pieces
    for (TWtringBuf::const_iterator it = WordText.end(); it > WordText.begin(); ) {
        --it;
        if (IsWordDelim(*it) && SplitAndModify(it - WordText.begin(), grammems, res, resgram))
            return true;
    }

    // if nothing, try bastard inflections
    return HasYemma && IsBastard(Yemma) &&
           InflectYemma(grammems, res, resgram);
}


bool TComplexWord::Inflect(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram) const {
    bool ret = InflectInt(grammems, res, resgram);
/*
    Cout << "[" << WideToChar(~WordText, +WordText) << "] -> ";
    if (ret)
        Cout << WideToChar(~res, +res) << Endl;
    else
        Cout << "*" << Endl;
*/
    return ret;
}

bool TComplexWord::Normalize(Wtroka& res, TGramBitSet* resgrammems) const {
    return Inflect(DefaultFeatures().NormalMutableSet, res, resgrammems);
}

static inline void SimpleJoin(const TWtringBuf& head, const wchar16 delim, const TWtringBuf& tail, Wtroka& res) {
    res.reserve(head.size() + 1 + tail.size());
    res.AppendNoAlias(~head, +head);
    res.AppendNoAlias(&delim, 1);
    res.AppendNoAlias(~tail, +tail);
}

bool TComplexWord::SplitAndModify(size_t wordDelim, const TGramBitSet& grammems,
                                  Wtroka& res, TGramBitSet* resgrammems) const {

    TWtringBuf lemmaHead, lemmaTail;
    size_t lemmaDelim = LemmaText.rfind(WordText[wordDelim]);
    if (lemmaDelim != TWtringBuf::npos) {
        lemmaHead = LemmaText.Head(lemmaDelim);
        lemmaTail = LemmaText.Tail(lemmaDelim + 1);
    }

    // recursively analyze sub-parts
    TComplexWord head(LangMask, WordText.Head(wordDelim), lemmaHead, KnownGrammems);
    TComplexWord tail(LangMask, WordText.Tail(wordDelim + 1), lemmaTail, KnownGrammems);

    Wtroka modHead, modTail;

    // tail is considered to be more important
    if (tail.Inflect(grammems, modTail, resgrammems)) {

        // do not modify head's features which are controversial between head and tail
        TGramBitSet agreed = grammems;
        if (tail.HasYemma && head.HasYemma)
            agreed &= ~DefaultFeatures().ExtendControversial(tail.Grammems(), head.Grammems());

        TWtringBuf useHead = head.WordText;
        if (head.Inflect(agreed, modHead))
            useHead = modHead;

        SimpleJoin(useHead, WordText[wordDelim], modTail, res);
        return true;
    } else if ((!tail.HasYemma || IsBastard(tail.Yemma)) && head.HasYemma && head.Inflect(grammems, modHead, resgrammems)) {
        SimpleJoin(modHead, WordText[wordDelim], tail.WordText, res);
        return true;
    }
    return false;
}

bool TComplexWord::SelectBestAgreedYemma(const TGramBitSet& agree, TYemmaSelector& res) {
    TGramBitSet mutAgr = DefaultFeatures().Mutable(agree);
    TGramBitSet ambiguous = DefaultFeatures().ExtendControversial(mutAgr, KnownGrammems);

    TYemmaSelector sel(LemmaText, KnownGrammems & ~ambiguous);  // prefer grammems from @agree rather than @KnownGrammems
    sel.TryAgreeWith(mutAgr);

    Analyze();
    if (sel.SelectFrom(CandidateYemmas, Yemma)) {
        res = sel;
        return true;
    } else
        return false;
}

Stroka TComplexWord::DebugString() const {
    TStringStream out;
    out << "lang=" << NLanguageMasks::ToString(LangMask);
    out << " word=" << WordText << " lemma=" << LemmaText;
    out << " gram=" << KnownGrammems.ToString(",");
    out << " yemma=";

    if (HasYemma)
        out << TWtringBuf(Yemma.GetText(), Yemma.GetTextLength());
    else
        out << "?";

    return out.Str();
}

Stroka TSimpleAutoColloc::DebugString() const {
    TStringStream out;
    for (size_t w = 0; w < Words.size(); ++w) {
        out << Words[w].DebugString();
        if (w == Main)
            out << " *";
        out << '\n';
    }
    return out.Str();
}

void TSimpleAutoColloc::AgreeWithMain(const TGramBitSet& gram, TYemmaSelector* best) {
    for (size_t w = 0; w < Words.size(); ++w)
        if (w != Main) {
            TYemmaSelector sel;
            if (Words[w].SelectBestAgreedYemma(gram, sel) && best != NULL && sel.IsBetter(*best))
                *best = sel;
        }
}

void TSimpleAutoColloc::GuessMainWord() {
    for (size_t w = 0; w < Words.size(); ++w) {
        if (Words[w].Grammems().Has(gSubstantive)) {
            Main = w;
            break;
        }
    }
}

void TSimpleAutoColloc::ReAgree() {

    TComplexWord& mainWord = Words[Main];

    TGramBitSet mainGram = mainWord.HasYemma ? mainWord.SelectedYemmaGrammems() : TGramBitSet();
    TAgreementMeasure knownGramAgr = TYemmaSelector::RequestedAgrMeasure(mainGram, mainWord.KnownGrammems);

    // start with current state
    TYemmaSelector bestSel;
    AgreeWithMain(mainWord.Grammems(), &bestSel);

    bool foundBetter = false;
    size_t bestMain = 0;
    size_t bestMainFlex = 0;

    mainWord.Analyze();
    for (size_t i = 0; i < mainWord.CandidateYemmas.size(); ++i)
        for (size_t f = 0; f < mainWord.CandidateYemmas[i].FlexGramNum(); ++f) {
            TGramBitSet possibleMainGrammems = TComplexWord::ExtractFormGrammems(mainWord.CandidateYemmas[i], f);

            // do not allow switching main word back to form having worse agreement with known (requested) grammems
            TAgreementMeasure possibgleKnownGramAgr = TYemmaSelector::RequestedAgrMeasure(possibleMainGrammems, mainWord.KnownGrammems);
            if (possibgleKnownGramAgr.Compare(knownGramAgr) == NCmp::WORSE)
                continue;

            TYemmaSelector curSelBest;
            AgreeWithMain(possibleMainGrammems, &curSelBest);
            if (curSelBest.IsBetter(bestSel)) {
                foundBetter = true;
                bestMain = i;
                bestMainFlex = f;
                bestSel = curSelBest;
            }
        }

    // reset main word yemma
    if (foundBetter) {
        YASSERT(bestMain < mainWord.CandidateYemmas.size());
        mainWord.Yemma = mainWord.CandidateYemmas[bestMain];
        ResetFlexion(mainWord.Yemma, bestMainFlex);
        mainWord.HasYemma = true;
    }

    AgreeWithMain(mainWord.Grammems());

}

bool TSimpleAutoColloc::Inflect(const TGramBitSet& grammems, yvector<Wtroka>& res, TGramBitSet* resgram) const {
    if (Words.empty()) {
        if (resgram != NULL)
            *resgram = grammems;
        return true;
    }

    Wtroka modText;
    TGramBitSet modGrammems;
    if (!Words[Main].Inflect(grammems, modText, &modGrammems))
        return false;

    if (resgram != NULL)
        *resgram = modGrammems;

    // from here return true, even if some of dependent words cannot be modified (they are left unmodified)
    res.clear();
    res.resize(Words.size());
    res[Main] = modText;

    // combine mutual features from main word and requested @grammems
    TGramBitSet combined = DefaultFeatures().ReplaceFeatures(grammems, DefaultFeatures().Mutable(modGrammems));
    TGramBitSet origGrammems = Words[Main].Grammems();

    for (size_t i = 0; i < Words.size(); ++i)
        if (i != Main) {
            // do not modify head's features which are controversial between main word and dependent
            TGramBitSet agreed = combined & ~DefaultFeatures().ExtendControversial(origGrammems, Words[i].Grammems());
            if (!Words[i].Inflect(agreed, res[i]))
                res[i] = ::ToWtring(Words[i].WordText);
        }

    return true;
}

bool TSimpleAutoColloc::Normalize(yvector<Wtroka>& res) const {
    return Inflect(DefaultFeatures().NormalMutableSet, res);
}


}   // NInfl



