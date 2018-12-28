#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/singleton.h>

#include <util/system/tls.h>

#include <library/lemmer/alpha/abc.h>

#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/common/langdata.h>
#include <FactExtract/Parser/afdocparser/rus/gleiche.h>
#include <FactExtract/Parser/lemmerlib/lemmerlib.h>

#include <FactExtract/Parser/surname_predictor/surname_predictor.h>


class CHomonymBase;
typedef TIntrusivePtr<CHomonymBase> THomonymBasePtr;
typedef yvector<THomonymBasePtr> THomonymBaseVector;

class CHomonym;
typedef TIntrusivePtr<CHomonym> THomonymPtr;
typedef yvector<THomonymPtr> THomonymVector;

class TMorph
{
public:
    TMorph();
private:
    // can only be created as singleton
    DECLARE_SINGLETON_FRIEND(TMorph)

public:
    static void SetMainLanguage(docLanguage lang);
    // TMorph should be refactored so MainLanguage is not static
    // but right now each thread in program can set up the language it wants
    static void SetThreadLocalMainLanguage(docLanguage lang);

    static inline docLanguage GetMainLanguage() {
        if (UseThreadLocalMainLanguage) {
            return ThreadLocalMainLanguage;
        } else {
            return MainLanguage;
        }
    }

    // minimal initialiazation (without lang data: name dictionaries, surname prediction, etc.)
    // but still it is completely valid initialization with gazetteer, aux-dic and morphology
    static void InitGlobalData(CGramInfo* pGramInfo, const Stroka& dicPath, const Stroka& binPath);

    // full initialization (with lang data)
    // if useThreadLocalMainLanguage=true then each thread should call SetThreadLocalMainLanguage
    // before using anything that wants TMorph
    static void InitGlobalData(CGramInfo* pGramInfo, docLanguage language, const Stroka& langDataPath,
                               const Stroka& binPath, bool needAuxKwDict, ECharset langDataEncoding,
                               bool useThreadLocalMainLanguage=false);

    static inline const CGramInfo* GlobalGramInfo() {
        return Singleton<TMorph>()->GramInfo;
    }

    static inline CGramInfo* GlobalMutableGramInfo() {
        return Singleton<TMorph>()->GramInfo;
    }

    static size_t ReplaceYo(Wtroka& str);


    // ----------------------------------------------------------
    // Language dependant capitalization, currently only differs for Turkish

    static bool ToLower(Wtroka& str) {
        return GetMainLanguage() != LANG_TUR ? str.to_lower() : GetMainLangAlpha()->ToLower(str).IsChanged.any();
    }

    static bool ToUpper(Wtroka& str) {
        return GetMainLanguage() != LANG_TUR ? str.to_upper() : GetMainLangAlpha()->ToUpper(str).IsChanged.any();
    }

    static bool ToTitle(Wtroka& str) {
        return GetMainLanguage() != LANG_TUR ? str.to_title() : GetMainLangAlpha()->ToTitle(str).IsChanged.any();
    }

    // ----------------------------------------------------------

    static TGramBitSet& NormalizeGrammemsInplace(TGramBitSet& g);
    static TGramBitSet NormalizeGrammems(const TGramBitSet& g) {
        TGramBitSet res = g;
        return NormalizeGrammemsInplace(res);
    }

    //reverse to normalization - for sending back to liblemmer (for searching forms, for example)
    static TGramBitSet& DenormalizeGrammemsInplace(TGramBitSet& g);
    static TGramBitSet DenormalizeGrammems(const TGramBitSet& g) {
        TGramBitSet res = g;
        return DenormalizeGrammemsInplace(res);
    }

    static TGramBitSet GuessNormalGenderNumberCase(const TGramBitSet& form_grammems, const TGramBitSet& desired);

    template <typename TLemmaOrWordform>
    static void ToNormalizedGrammarBunch(const TLemmaOrWordform& word, TGrammarBunch& res);

    static bool CorrectLemmaGrammems(const TGrammarBunch& form_grammems, TGramBitSet& res_grammems);

    //working with TYandexLemma
    static void GenerateWordForms(const TYandexLemma& lemma, TWordformArray& res, const TGramBitSet& required_grammems = TGramBitSet());
    static Wtroka GetFormWithGrammems(const TYandexLemma& lemma, const TGramBitSet& grammems);
    static THomonymPtr YandexLemmaToHomonym(const TYandexLemma& lemma, bool bIndexed);
    //fill TWLemmaArray with all possible lemmas of word (including bastards)
    static void FindAllLemmas(TWtringBuf strWord, TWLemmaArray& res);
    static void PredictLemmasWithGrammems(TWtringBuf strWord, TWLemmaArray& res, const TGramBitSet& grammems);

    //fills res with all forms of given @lemma (which should be a word in initial form!)
    static bool GetAllFormsOfLemma(const Wtroka& lemma, yvector<TPair<Wtroka, TGrammarBunch> >& res, bool bUsePrediction=false);

    //filters forms of @lemma with filter @f and returns filtered forms with minimal number of grammems
    //GrammemsFilter could be a (bool)(*filter)(const TGramBitSet&) or a functinoid with operator()(const TGramBitSet&)
    template <typename GrammemsFilter>
    static Wtroka GetClosestForm(const Wtroka& lemma, const Wtroka& word,
                                  GrammemsFilter filter, const TGramBitSet& desired_grammems = TGramBitSet(),
                                  bool bUsePrediction = false);

    //return form of @lemma, having most of @desired_grammems and in case of tie, the one with most normal and minimal set of grammems
    static Wtroka GetClosestForm(const Wtroka& lemma, const TGramBitSet& desired_grammems);

    // Remove all forms from @forms except those having grammems closest to @desired_grammems
    // Return maximal number of intersection with @desired_grammems in @best_forms.
    static size_t SelectBestForms(const TWordformArray& forms, const TGramBitSet& desired_grammems, TWordformArray& best_forms);

    static bool GetGrammems(const Wtroka& word, yvector<TGrammarBunch>& res, bool bUsePrediction = false);
    static TGramBitSet GetCommonGrammems(const TYandexLemma& lemma);
    //grammems of stem for lemma (!) of @word
    static TGramBitSet GetLemmaGrammems(const Wtroka& lemma, const TGramBitSet& desired_grammems = TGramBitSet());

    static const TGramBitSet& AllPOS();

    static bool GetDictHomonyms(const Wtroka& word, THomonymVector &out);
    static bool HasDictLemma(const Wtroka& word);

    static bool GetHomonyms(const Wtroka& word, THomonymBaseVector& dict_homs, bool bUsePrediction, THomonymBaseVector& predicted_homs);
    static bool GetHomonyms(const Wtroka& word, THomonymBaseVector& dict_homs, bool bUsePrediction);

    static bool PredictHomonyms(TWtringBuf word, THomonymBaseVector& out);
    static bool PredictFormGrammemsByKnownLemma(const Wtroka& word, const Wtroka& potential_lemma, TGrammarBunch& res);
    static bool GetHomonymsWithPrefixs(const Wtroka& word, THomonymBaseVector& out);
    static bool Similar(TWtringBuf w1, TWtringBuf w2);
    static bool Similar(TWtringBuf w1, TWtringBuf w2, const TGramBitSet& required_grammems);

    static bool IdentifSimilar(TWtringBuf w1, TWtringBuf w2);
    static bool SimilarSurnSing(TWtringBuf w1, TWtringBuf w2);

    static int MaxLenFlex(TWtringBuf word);
    static bool FindWord(TWtringBuf word);

    template <class TGrammarsCollection>
    static bool HasFormWithGrammems(const TGramBitSet& grammems, const TGrammarsCollection& flexgrammars, const TGramBitSet& stemgrammar = TGramBitSet(TGrammar(0)));

    //static helpers - partially moved from NSpike or TGramBitSet (as language dependent)

    //some special part-of-speach checkers
    static bool IsPersonalVerb(const TGramBitSet& g) {
        //check if @grammems describes a verb in personal form (not participle, gerund, infinitive, imperative)
        static const TGramBitSet not_verb(gParticiple, gGerund, gInfinitive, gImperative, gAuxVerb);
        return g.Has(gVerb) && !g.HasAny(not_verb);
    }

    static bool IsPresentTense(const TGramBitSet& g) {
        // conventional interpretation
        static const TGramBitSet mask(gNotpast, gImperfect);
        return g.Has(gPresent) || g.HasAll(mask);
    }

    static bool IsFutureTense(const TGramBitSet& g) {
        // conventional interpretation
        static const TGramBitSet mask(gNotpast, gPerfect);
        static const TGramBitSet aspect(gPerfect, gImperfect);
        return g.Has(gFuture) || g.HasAll(mask) || (g.Has(gNotpast) && !g.HasAny(aspect));//for "budet" without aspect
    }

    static bool IsPastTense(const TGramBitSet& g) {
        static const TGramBitSet past(gPast, gPast2);
        return g.HasAny(past);
    }

    static bool IsPresentOrFutureTense(const TGramBitSet& g) {
        static const TGramBitSet mask(gFuture, gPresent, gNotpast);
        return g.HasAny(mask);
    }

    static bool IsFullAdjective(const TGramBitSet& g) {
        return g.Has(gAdjective) && !g.Has(gShort);
    }

    static bool IsFullParticiple(const TGramBitSet& g) {
        return g.Has(gParticiple) && !g.Has(gShort);
    }

    static bool IsShortAdjective(const TGramBitSet& g) {
        static const TGramBitSet mask(gAdjective, gShort);
        return g.HasAll(mask);
    }

    static bool IsShortParticiple(const TGramBitSet& g) {
        static const TGramBitSet mask(gParticiple, gShort);
        return g.HasAll(mask);
    }

    static bool IsMorphNoun(const TGramBitSet& g) {
        static const TGramBitSet mask(gSubstantive, gSubstPronoun, gPronounConj);
        return g.HasAny(mask);
    }

    static bool IsMorphAdjective(const TGramBitSet& g) {
        static const TGramBitSet mask(gAdjPronoun, gAdjective, gParticiple, gAdjNumeral);
        return g.HasAny(mask) && !g.Has(gShort);
    }

    static void PrintGrammems(const TGramBitSet& grammems, Stroka& out);
    static void PrintGrammarBunch(const TGrammarBunch& grammems, Stroka& out);

protected:
    //helper methods
    static bool IsBastard(const TYandexLemma& lemma)
    {
        //consider prefixoids as non-bastards but good dictionary word
        return lemma.IsBastard() && (lemma.GetQuality() & TYandexLemma::QPrefixoid) == 0;
    }

    static bool IsLemmaText(const TYandexLemma& lemma, const TWtringBuf& text) {
        return TWtringBuf(lemma.GetText(), lemma.GetTextLength()) == text;
    }

    static bool IsInfinitive(const TYandexLemma& lemma)
    {
        if (lemma.FlexGramNum() == 1)   //look at single flex only (ugly!)
        {
            const char* gram = lemma.GetFlexGram()[0];
            if (gram != NULL)
                for (int i = 0; gram[i]; ++i)
                    if (NTGrammarProcessing::ch2tg(gram[i]) == gInfinitive)
                       return true;
        }
        return false;
    }

    static bool IsLemmaForm(const TYandexLemma& lemma)
    {
        //True if wordform is the same as lemma of this wordform, i.e. given @lemma is a word in its initial form
        //2009-08-02: special condition for verbs - treat infinitives as lemma form (to cope with mixed perfect-imperfect paradigm)
        return IsLemmaText(lemma, TWtringBuf(lemma.GetForma(), lemma.GetFormLength()))
            || IsInfinitive(lemma);
    }

    //static bool HasSpecialGrammems(const TYandexLemma& word, TGrammar stemgram, TGrammar flexgram);
    static bool HasStemGrammem(const TYandexLemma& word, TGrammar stemgram);

    static void GrammemsToString(const TGramBitSet& grammems, Stroka& res) { grammems.ToBytes(res); }

    static size_t GetLongestPrefixLength(const Wtroka& s1, const Wtroka& s2)
    {
        size_t i = 0, len = Min<size_t>(s1.size(), s2.size());
        for (; i < len && s1[i] == s2[i]; ++i) { /* do nothing */ }
        return i;
    }

    template <typename GrammemsFilter>
    static void FilterFormsWithMinGrammems(const TWordformArray& forms, GrammemsFilter filter, yvector<Wtroka>& res);

    static Wtroka GetFormWithLongestPrefix(const yvector<Wtroka>& res, const Wtroka& prefix);

    static size_t GetGrammemCost(TGrammar g);
    static size_t GetGrammemsCost(const TGramBitSet& grammems);
    static TGramBitSet GetMostNormalFormGrammems(const TYandexLemma& lemma);

    static bool NoFilter(const TGramBitSet&) { return true; }

public:
    static bool PredictSurname(const Wtroka& word, yvector<TSurnamePredictor::TPredictedSurname>& predictedSurnames);

    static THomonymBasePtr CreateHomonym();

    static const TGrammarBunch* GetNumeralGrammemsByFlex(const TWtringBuf& numeral, const TWtringBuf& s);
    static const Wtroka* GetNumeralFlexByGrammems(const TWtringBuf& numeral, const TGramBitSet& grm);

private:
    void LoadSurnamePrediction(CGramInfo* pGramInfo);
    void InitNumberFlex2Grammems(const Wtroka& numeral, size_t flexmap_index);   //for specific numeral
    void InitNumberFlex2Grammems();

    size_t GetFlexMapIndex(const TWtringBuf& numeral) const;

private: // DATA //

    CGramInfo* GramInfo;   // just pointer, without ownership

    //последние буквы порядковых числительных (не больше 4) и соответствующие грамматики
    //для предсказания, напр, "47-е место"
    typedef ymap<Wtroka, TGrammarBunch, TLess<TWtringBuf> > TNumberFlex2GrammemsMap;
    TNumberFlex2GrammemsMap m_NumberFlex2Grammems[4];     //grammems of numerals flexes: 1,2,3 (index 0 for unknown nummber, corresponds to old behaviour)
    ymap<TGramBitSet, Wtroka> m_NumberGrammems2Flex[4];       //norm flex of numerals: 1,2,3 (index 0 for unknown numbers)

    THolder<TSurnamePredictor> m_SurnamePredictor;

    static bool UseThreadLocalMainLanguage;

    static docLanguage MainLanguage;    // default is LANG_RUS
    POD_STATIC_THREAD(docLanguage) ThreadLocalMainLanguage;

    // current MainLanguage alphabet normalizer, for example, a special one is used for lower-casing for turkish
    static const NLemmer::TAlphabetWordNormalizer* MainLangAlpha;
    POD_STATIC_THREAD(const NLemmer::TAlphabetWordNormalizer*) ThreadLocalMainLangAlpha;

    static inline const NLemmer::TAlphabetWordNormalizer* GetMainLangAlpha() {
        if (UseThreadLocalMainLanguage) {
            return ThreadLocalMainLangAlpha;
        } else {
            return MainLangAlpha;
        }
    }
};

inline void InitGlobalData(CGramInfo* pGramInfo, docLanguage language, const Stroka& langDataPath,
                           const Stroka& binPath, bool needAuxKwDict, ECharset langDataEncoding,
                           bool useThreadLocalMainLanguage=false) {
    TMorph::InitGlobalData(pGramInfo, language, langDataPath, binPath, needAuxKwDict, langDataEncoding, useThreadLocalMainLanguage);
}

inline void InitGlobalData(CGramInfo* pGramInfo, const Stroka& dicsPath, const Stroka& binPath = "") {
    TMorph::InitGlobalData(pGramInfo, dicsPath, binPath);
}

inline bool GlobalDataInitialized() {
    return TMorph::GlobalGramInfo() != NULL;
}

inline const CGramInfo* GetGlobalGramInfo() {
    YASSERT(GlobalDataInitialized());
    return TMorph::GlobalGramInfo();
}

#define GlobalGramInfo (GetGlobalGramInfo())




// inline definitions of TMorph

template <typename TLemmaOrWordform>
void TMorph::ToNormalizedGrammarBunch(const TLemmaOrWordform& word, TGrammarBunch& res)
{
    for (size_t i = 0; i < word.FlexGramNum(); ++i) {
        TGramBitSet bits;
        NSpike::ToGramBitset(word.GetStemGram(), word.GetFlexGram()[i], bits);
        if (bits.any())
            res.insert(NormalizeGrammemsInplace(bits));
    }
}

template <class TGrammarsCollection>
inline bool TMorph::HasFormWithGrammems(const TGramBitSet& grammems, const TGrammarsCollection& flexgrammars, const TGramBitSet& stemgrammar /*= 0*/)
{
    for (typename TGrammarsCollection::const_iterator it = flexgrammars.begin(); it != flexgrammars.end(); ++it)
        if ((*it | stemgrammar).HasAll(grammems))
            return true;
    //last chance - if flexgrammars were empty
    return stemgrammar.HasAll(grammems);
}

template <typename GrammemsFilter>
void TMorph::FilterFormsWithMinGrammems(const TWordformArray& forms, GrammemsFilter filter, yvector<Wtroka>& res)
{
    // Finds a forms conforming to @filter and having minimal number of grammems
    // (to exclude excessive grammems like gObsolete if not requested explicitly)
    static const size_t INF_GRAMMEMS = TGramBitSet::BitsetSize + 1;
    size_t min_grammems = INF_GRAMMEMS;
    yvector<size_t> best_forms(forms.size());    //indices of best found forms
    size_t best_forms_count = 0;

    for (size_t i = 0; i < forms.size(); ++i) {
        //for each form find minimal number of grammems it could have
        TGrammarBunch grammems;
        ToNormalizedGrammarBunch(forms[i], grammems);
        size_t cur_grammems = INF_GRAMMEMS;
        // TODO: make normilized grammems iterator (instead of filling TGrammarBunch)
        for (TGrammarBunch::const_iterator it = grammems.begin(); it != grammems.end(); ++it) {
            if (!filter(*it))
                continue;
            size_t tmp = it->count();
            if (tmp < cur_grammems)
                cur_grammems = tmp;
        }

        if (cur_grammems == INF_GRAMMEMS || cur_grammems > min_grammems)
            continue;
        if (cur_grammems < min_grammems) {
            min_grammems = cur_grammems;
            best_forms_count = 0;
        }
        best_forms[best_forms_count++] = i;
    }
    for (size_t i = 0; i < best_forms_count; ++i)
        res.push_back(forms[best_forms[i]].GetText());
}

template <typename GrammemsFilter>
Wtroka TMorph::GetClosestForm(const Wtroka& lemma, const Wtroka& word,
                               GrammemsFilter filter, const TGramBitSet& desired_grammems,
                               bool bUsePrediction /*=false*/)
{
    TWLemmaArray lemmas;
    FindAllLemmas(lemma, lemmas);

    TWordformArray all_forms;
    for (size_t i = 0; i < lemmas.size(); ++i)
        // take only lemmas where analyzed form is same as lemma
        if (IsBastard(lemmas[i]) == bUsePrediction && TMorph::IsLemmaForm(lemmas[i]))
            //for each lemma generate all its wordforms (and collect them in @all_forms)
            GenerateWordForms(lemmas[i], all_forms);

    // when nothing remained, make filtering less strict for bastards
    if (all_forms.empty() && bUsePrediction)
        for (size_t i = 0; i < lemmas.size(); ++i)
            if (IsBastard(lemmas[i]))
                GenerateWordForms(lemmas[i], all_forms);

    TWordformArray* best_forms = &all_forms;
    TWordformArray desired_forms;
    if (desired_grammems.any()) {
        // select forms most close to @desired_grammems
        SelectBestForms(all_forms, desired_grammems, desired_forms);
        best_forms = &desired_forms;
    }

    // pick forms matching to @filter and having minimal number of grammems
    yvector<Wtroka> res;
    FilterFormsWithMinGrammems(*best_forms, filter, res);

    if (res.empty())
        return Wtroka();
    else {
        //return form having the longest common prefix with @word
        Wtroka prefix = word;
        TMorph::ToLower(prefix);
        return GetFormWithLongestPrefix(res, prefix);
    }
}
