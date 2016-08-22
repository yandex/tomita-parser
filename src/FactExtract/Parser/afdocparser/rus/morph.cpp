#include <util/generic/ylimits.h>
#include <util/charset/wide.h>
#include <util/generic/list.h>

#include "morph.h"

#include <library/lemmer/dictlib/grammarhelper.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/afdocparser/rus/homonym.h>


bool TMorph::UseThreadLocalMainLanguage = false;

// Careful, default LANG_RUS is required for correct aux_dic_kw compilation.
// Do not set to LANG_UNK.
docLanguage TMorph::MainLanguage = LANG_RUS;
POD_THREAD(docLanguage) TMorph::ThreadLocalMainLanguage(LANG_RUS);

const NLemmer::TAlphabetWordNormalizer* TMorph::MainLangAlpha = NULL;
POD_THREAD(const NLemmer::TAlphabetWordNormalizer*) TMorph::ThreadLocalMainLangAlpha(
    (const NLemmer::TAlphabetWordNormalizer*)0);

void TMorph::InitGlobalData(CGramInfo* pGramInfo, const Stroka& dicPath, const Stroka& binPath)
{
    YASSERT(pGramInfo != NULL);
    pGramInfo->Init(dicPath, binPath);

    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    Singleton<TMorph>()->GramInfo = pGramInfo;
}

void TMorph::InitGlobalData(CGramInfo* pGramInfo, docLanguage language, const Stroka& langDataPath,
                            const Stroka& binPath, bool needAuxKwDict, ECharset langDataEncoding,
                             bool useThreadLocalMainLanguage)
{
    YASSERT(pGramInfo != NULL);

    pGramInfo->s_bNeedAuxKwDict = needAuxKwDict;

    SetMainLanguage(language);
    UseThreadLocalMainLanguage = useThreadLocalMainLanguage;

    // init gram info and name dictionaries
    InitGlobalData(pGramInfo, langDataPath, binPath);
    pGramInfo->InitNames(PathHelper::Join(langDataPath, g_strNameDic), langDataEncoding);

    Singleton<CLangData>()->AddWordsWithPoint(pGramInfo->LoadRusExtension());
    Singleton<CLangData>()->AddAbbrevs(pGramInfo->LoadRusAbbrev());

    // init surname predictor
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);
    Singleton<TMorph>()->LoadSurnamePrediction(pGramInfo);
}

void TMorph::SetMainLanguage(docLanguage lang) {
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);
    MainLanguage = lang;
    MainLangAlpha = NLemmer::GetAlphaRulesAnyway(lang);
}

void TMorph::SetThreadLocalMainLanguage(docLanguage lang) {
    ThreadLocalMainLanguage = lang;
    ThreadLocalMainLangAlpha = NLemmer::GetAlphaRulesAnyway(lang);
}


TMorph::TMorph()
    : GramInfo(NULL)
{
    InitNumberFlex2Grammems();
}


size_t TMorph::ReplaceYo(Wtroka& str)
{
    size_t count = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == YO_LOWER) {
            str.begin()[i] = E_LOWER;
            ++count;
        } else if (str[i] == YO_UPPER) {
            str.begin()[i] = E_UPPER;
            ++count;
        }
    }
    return count;
}

TGramBitSet& TMorph::NormalizeGrammemsInplace(TGramBitSet& g)
{
    //activation mask - to minimize mean number of checks
    static const TGramBitSet mask = TGramBitSet(gNotpast, gPartitive, gLocative, gVocative, gMasFem);
    if (!g.HasAny(mask))
        return g;

    //tenses
    if (g.Has(gNotpast)) {
        //g.Reset(gNotpast);
        if (g.Has(gImperfect))
            g.Set(gPresent);
        if (g.Has(gPerfect))
            g.Set(gFuture);
    } else {
        //cases
        if (g.Has(gPartitive))
            g.Set(gGenitive);
        if (g.Has(gLocative))
            g.Set(gAblative);
        if (g.Has(gVocative))
            g.Set(gNominative);

        if (g.Has(gMasFem)) {
            g.Set(gMasculine);
            g.Set(gFeminine);
        }
    }
    return g;
}

TGramBitSet& TMorph::DenormalizeGrammemsInplace(TGramBitSet& g)
{
    //activation mask - to minimize mean number of checks
    static const TGramBitSet mask = TGramBitSet(gNotpast, gPartitive, gLocative, gVocative, gMasFem);
    if (!g.HasAny(mask))
        return g;

    //tenses
    if (g.Has(gNotpast)) {
        g.Reset(gPresent);
        g.Reset(gFuture);
    } else {
        //cases
        if (g.Has(gPartitive))
            g.Reset(gGenitive);
        if (g.Has(gLocative))
            g.Reset(gAblative);
        if (g.Has(gVocative))
            g.Reset(gNominative);

        if (g.Has(gMasFem)) {
            g.Reset(gMasculine);
            g.Reset(gFeminine);
        }
    }
    return g;
}

TGramBitSet TMorph::GuessNormalGenderNumberCase(const TGramBitSet& form_grammems, const TGramBitSet& desired)
{
    TGramBitSet cases = desired & NSpike::AllCases;
    if (cases.count() != 1)
        cases = TGramBitSet(gNominative);                       //Nominative is default

    TGramBitSet numbers = desired & NSpike::AllNumbers;
    if (numbers.none())
        numbers = form_grammems & NSpike::AllNumbers;
    if (numbers.count() != 1)
        numbers = TGramBitSet(gSingular);                       //Singular is default

    TGramBitSet genders = desired & NSpike::AllGenders;
    if (genders.none() || form_grammems.Has(gSubstantive))      //for nouns use their gender, not desired
        genders = form_grammems & NSpike::AllGenders;
    size_t gcount = genders.count();
    if (gcount != 1) {
        if (genders.Has(gMasculine) || (gcount == 0 && !numbers.Has(gPlural)))
            genders = TGramBitSet(gMasculine);                  //Masculine is default, except plural
        else if (genders.Has(gFeminine))
            genders = TGramBitSet(gFeminine);
        else if (genders.Has(gNeuter))
            genders = TGramBitSet(gNeuter);

    }

    return cases | numbers | genders;
}

void TMorph::GenerateWordForms(const TYandexLemma& lemma, TWordformArray& res, const TGramBitSet& required_grammems)
{
    if (lemma.GetLanguage() != LANG_UNK) {
        Stroka chars;
        GrammemsToString(DenormalizeGrammems(required_grammems), chars);
        NLemmer::Generate(lemma, res, ~chars);
    }
}

void TMorph::PredictLemmasWithGrammems(TWtringBuf strWord, TWLemmaArray& res, const TGramBitSet& grammems) //static
{
    Stroka gramStr;
    GrammemsToString(grammems, gramStr);

    NLemmer::GetLanguageById(GetMainLanguage())->Recognize(~strWord, +strWord, res, TLanguage::TRecognizeOpt(NLemmer::AccBastard, ~gramStr));
}

void TMorph::FindAllLemmas(TWtringBuf strWord, TWLemmaArray& res)
{
    NLemmer::AnalyzeWord(~strWord, +strWord, res, GetMainLanguage());
}

bool TMorph::CorrectLemmaGrammems(const TGrammarBunch& form_grammems, TGramBitSet& res_grammems)
{
    //workaround for verbs: take as lemma an infinitive with same perfect/imperfect flag as in a given form
    //for example [translit]: "umer" -> "umeret'", but not "umirat'"

    // + for participles: take as lemma a participle with Nominative+Singular+Masculine (not Infinitive)
    // but only for full participles, not short

    static const TGramBitSet mask(gPerfect, gImperfect);
    for (TGrammarBunch::const_iterator it = form_grammems.begin(); it != form_grammems.end(); ++it) {
        if (!it->Has(gVerb))        // just gVerb enough - do not check for Participle or Gerund (because we want to correct lemma, not wordform)
            return false;
        res_grammems |= *it & mask;
    }

    if (res_grammems.Has(gPerfect) != res_grammems.Has(gImperfect)) {
        res_grammems.Set(gInfinitive);
        return true;
    }

    return false;
}

THomonymPtr TMorph::YandexLemmaToHomonym(const TYandexLemma& lemma, bool bIndexed)
{
    THomonymBasePtr pH = CreateHomonym();
    pH->ResetYLemma(lemma);

    NSpike::TGrammarBunch grammems;
    ToNormalizedGrammarBunch(lemma, grammems);

    TGramBitSet corrected_lemma_grammems;
    //correct lemma if necessary (participle -> participle, verb perfect -> infinitive perfect)
    if (CorrectLemmaGrammems(grammems, corrected_lemma_grammems)) {
        Wtroka corrected_lemma = GetFormWithGrammems(lemma, corrected_lemma_grammems);
        if (!corrected_lemma.empty()) {
            pH->Init(corrected_lemma, grammems, bIndexed);
            return dynamic_cast<CHomonym*>(pH.Get());
        }
    }

    pH->Init(Wtroka(lemma.GetText(), lemma.GetTextLength()), grammems, bIndexed);
    return dynamic_cast<CHomonym*>(pH.Get());
}

bool TMorph::GetGrammems(const Wtroka& word, yvector<TGrammarBunch>& res, bool bUsePrediction /*= false*/)
{
    TWLemmaArray lemmas;
    FindAllLemmas(word, lemmas);
    for (size_t i = 0; i < lemmas.size(); ++i) {
        bool predicted = IsBastard(lemmas[i]);
        if (!predicted || bUsePrediction) {
            TGrammarBunch grammems;
            ToNormalizedGrammarBunch(lemmas[i], grammems);
            if (!grammems.empty())
                res.push_back(grammems);
        }
    }
    return res.size() > 0;
}

TGramBitSet TMorph::GetCommonGrammems(const TYandexLemma& lemma)
{
    TGramBitSet stem_grammems;
    NSpike::ToGramBitset(lemma.GetStemGram(), stem_grammems);

    TGramBitSet common_flex_grammems;      //collect grammems, common for all forms of @lemma
    if (lemma.FlexGramNum() > 0) {
        common_flex_grammems = ~TGramBitSet();  // set all to 1
        for (size_t j = 0; j < lemma.FlexGramNum(); ++j) {
            TGramBitSet bits;
            NSpike::ToGramBitset(lemma.GetFlexGram()[j], bits);
            common_flex_grammems &= bits;
        }
    }
    return NormalizeGrammems(stem_grammems | common_flex_grammems);
}

const TGramBitSet& TMorph::AllPOS() {
    static const TGramBitSet pos_mask =
        TGramBitSet(gSubstantive, gAdjective, gVerb, gSubstPronoun) |
        TGramBitSet(gAdjPronoun, gNumeral, gAdjNumeral, gAdverb, gAdvPronoun) |
        TGramBitSet(gPraedic, gPreposition, gConjunction, gInterjunction) |
        TGramBitSet(gParenth, gParticle, gParticiple, gGerund) |
        TGramBitSet(gInfinitive, gShort, gComparative, gPossessive) |
        TGramBitSet(gSubConj, gSimConj, gPronounConj) |
        TGramBitSet(gCorrelateConj, gImperative, gAuxVerb);
    return pos_mask;
}

size_t TMorph::GetGrammemCost(TGrammar g)
{
    static const TGramBitSet null_cost(TGramBitSet(gNominative, gSingular, gMasculine) & AllPOS());
    return null_cost.Has(g) ? 0 : 1;
}

size_t TMorph::GetGrammemsCost(const TGramBitSet& grammems)
{
    size_t res = 0;
    for (TGrammar g = grammems.FindFirst(); g < TGramBitSet::End; g = grammems.FindNext(g))
        res += GetGrammemCost(g);
    return res;
}

TGramBitSet TMorph::GetMostNormalFormGrammems(const TYandexLemma& lemma)
{
    TGramBitSet stem_grammems;
    NSpike::ToGramBitset(lemma.GetStemGram(), stem_grammems);

    TGramBitSet best_flex;
    size_t best_cost = ::Max<size_t>();
    for (size_t j = 0; j < lemma.FlexGramNum(); ++j) {
        TGramBitSet bits;
        NSpike::ToGramBitset(lemma.GetFlexGram()[j], bits);
        size_t cur_cost = GetGrammemsCost(bits);
        if (cur_cost < best_cost || (cur_cost == best_cost && bits.count() < best_flex.count())) {
            best_flex = bits;
            best_cost = cur_cost;
        }
    }
    return NormalizeGrammems(stem_grammems | best_flex);
}

TGramBitSet TMorph::GetLemmaGrammems(const Wtroka& lemma, const TGramBitSet& desired_grammems)
{
    TWLemmaArray lemmas;
    FindAllLemmas(lemma, lemmas);

    TGramBitSet res;

    yvector<TGramBitSet> best_grammems;       // for each lemma save its most "normal" grammems
    bool check_desired_grammems = desired_grammems.any();
    for (size_t i = 0; i < lemmas.size(); ++i)
        //use only regular lemmas (from dictionary) and make sure we take grammems of lemma we have just put in
        if (!IsBastard(lemmas[i]) && TMorph::IsLemmaForm(lemmas[i])) {
            res = GetMostNormalFormGrammems(lemmas[i]);
            if (check_desired_grammems)
                best_grammems.push_back(res);
            else
                return res;
        }

    if (best_grammems.empty())
        return res;

    //now pick grammems having maximal intersection with desired_grammems
    size_t best_i = 0;
    size_t best_count = (best_grammems[0] & desired_grammems).count();
    for (size_t i = 1; i < best_grammems.size(); ++i) {
        size_t cur_count = (best_grammems[i] & desired_grammems).count();
        if (cur_count > best_count) {
            best_count = cur_count;
            best_i = i;
        }
    }
    return best_grammems[best_i];
}

bool TMorph::GetHomonyms(const Wtroka& word, THomonymBaseVector& dict_homs, bool bUsePrediction, THomonymBaseVector& predicted_homs)
{
    dict_homs.clear();
    predicted_homs.clear();
    TWLemmaArray lemmas;
    FindAllLemmas(word, lemmas);

    for (size_t i = 0; i < lemmas.size(); ++i) {
         bool predicted = IsBastard(lemmas[i]);
         THomonymBasePtr pH = YandexLemmaToHomonym(lemmas[i], !predicted).Get();
         if (predicted)
             predicted_homs.push_back(pH);
         else
             dict_homs.push_back(pH);
    }
  
    if (Singleton<TMorph>()->GramInfo->s_BastardMode == EBastardMode::Always ||
        (Singleton<TMorph>()->GramInfo->s_BastardMode == EBastardMode::OutOfDict && dict_homs.size() == 0)) {
        dict_homs.insert(dict_homs.end(), predicted_homs.begin(), predicted_homs.end());
        predicted_homs.clear();
    }

    if (dict_homs.empty() && bUsePrediction) {
        THomonymBasePtr pH = CreateHomonym();
        pH->InitText(word, false);
        dict_homs.push_back(pH);
    }
    return dict_homs.size() > 0;
}

bool TMorph::GetHomonyms(const Wtroka& word, THomonymBaseVector& dict_homs, bool bUsePrediction)
{
    THomonymBaseVector predicted;
    return GetHomonyms(word, dict_homs, bUsePrediction, predicted);
}

bool TMorph::GetDictHomonyms(const Wtroka& word, THomonymVector& out)
{
    THomonymBaseVector dict, predicted;
    GetHomonyms(word, dict, false, predicted);
    out.clear();
    for (THomonymBaseVector::iterator h = dict.begin(); h != dict.end(); ++h)
        if (dynamic_cast<CHomonym*>(h->Get()) != NULL)
            out.push_back(static_cast<CHomonym*>(h->Release()));

    return out.size() > 0;
}

bool TMorph::GetHomonymsWithPrefixs(const Wtroka& word, THomonymBaseVector& out)
{
    return GetHomonyms(word, out, false);
}

bool TMorph::PredictHomonyms(TWtringBuf word, THomonymBaseVector& out_homs)
{
    out_homs.clear();
    TWLemmaArray lemmas;
    FindAllLemmas(word, lemmas);
    for (size_t i = 0; i < lemmas.size(); ++i)
        out_homs.push_back(YandexLemmaToHomonym(lemmas[i], !IsBastard(lemmas[i])).Get());
    return out_homs.size() > 0;
}

bool TMorph::PredictFormGrammemsByKnownLemma(const Wtroka& word, const Wtroka& potential_lemma, TGrammarBunch& res)
{
    TWLemmaArray lemmas;
    FindAllLemmas(word, lemmas);
    if (lemmas.size() == 0)
        return false;

    yvector<TGrammarBunch> fitting_forms;
    for (size_t i = 0; i < lemmas.size(); ++i)
        if (IsLemmaText(lemmas[i], potential_lemma)) {
            TGrammarBunch tmp;
            ToNormalizedGrammarBunch(lemmas[i], tmp);
            if (!tmp.empty())
                fitting_forms.push_back(tmp);
        }

    if (fitting_forms.empty())
        return false;

    //now take only most common grammems to avoid wrong ambiguity resolution
    TGramBitSet common_grammems = TGramBitSet::UniteGrammems(fitting_forms[0]);
    for (size_t i = 1; i < fitting_forms.size(); ++i)
        common_grammems &= TGramBitSet::UniteGrammems(fitting_forms[i]);

    if (common_grammems.none())
        return false;

    //take first lemma forms, having non-empty intersection with common_grammems
    for (size_t i = 0; i < fitting_forms.size(); ++i) {
        for (TGrammarBunch::const_iterator it = fitting_forms[i].begin(); it != fitting_forms[i].end(); ++it) {
            TGramBitSet g = *it & common_grammems;
            if (g.any())
                res.insert(g);
        }
        if (!res.empty())
            return true;
    }
    return false;
}

bool TMorph::HasDictLemma(const Wtroka& word)
{
    TWLemmaArray lemmas;
    FindAllLemmas(word, lemmas);

    for (size_t i = 0; i < lemmas.size(); ++i)
        if (!IsBastard(lemmas[i]) && IsLemmaText(lemmas[i], word))
            return true;

    return false;
}

static inline bool SameLemmaText(const TYandexLemma& lemma1, const TYandexLemma& lemma2) {
    return TWtringBuf(lemma1.GetText(), lemma1.GetTextLength()) ==
           TWtringBuf(lemma2.GetText(), lemma2.GetTextLength());
}

static inline bool SameLemmaStemGram(const TYandexLemma& lemma1, const TYandexLemma& lemma2) {
    return strcmp(lemma1.GetStemGram(), lemma2.GetStemGram()) == 0;
}

static inline bool SameLemma(const TYandexLemma& lemma1, const TYandexLemma& lemma2) {
    return lemma1.GetLanguage() ==  lemma2.GetLanguage() &&
        SameLemmaText(lemma1, lemma2) &&
        SameLemmaStemGram(lemma1, lemma2);
}

static inline bool IsFormOfLemma(const TWtringBuf& word, const TYandexLemma& lemma, TWordformArray& forms_placeholder) {
    forms_placeholder.clear();
    TMorph::GenerateWordForms(lemma, forms_placeholder);
    for (size_t j = 0; j < forms_placeholder.size(); ++j)
        if (TWtringBuf(forms_placeholder[j].GetText()) == word)
            return true;
    return false;
}

bool TMorph::Similar(TWtringBuf word1, TWtringBuf word2, const TGramBitSet& required_grammems)     //static
{
    //do not treat similar shord words of len<3 or having different first pair of chars
    if (word1.size() < 3 || word2.size() < 3 || word1.SubStr(0, 2) != word2.SubStr(0,2))
        return false;
    if (word1 == word2)
        return true;

    TWLemmaArray lemmas1, lemmas2;
    PredictLemmasWithGrammems(word1, lemmas1, required_grammems);
    PredictLemmasWithGrammems(word2, lemmas2, required_grammems);

    if (lemmas1.empty() || lemmas2.empty())
        return false;

    // first do easy check, it should work most of times
    for (size_t i1 = 0; i1 < lemmas1.size(); ++i1)
        for (size_t i2 = 0; i2 < lemmas2.size(); ++i2)
            if (SameLemma(lemmas1[i1], lemmas2[i2]))
                return true;

    // Otherwise examine predicted paradigms more accurately:
    // try to find one of the words in paradigm of the second one. (e.g. 'Blandell' <-> 'Blandella')
    // More detailed for this example:
    // 'Blandell' predicted as indeclinable 'blandell',
    // 'Blandella' predicted as 'blandella', having 'blandell' as a form in the paradigm.
    // Thus, there are no common lemma, and the only relation is that
    // one of the words is a form in other's lemma paradigm.

    // Do this check only form non-dictionary (predicted) lemmas,
    // as it is quite slow to run each time.

    // use normalized words for comparison with forms
    word1 = TWtringBuf(lemmas1[0].GetForma(), lemmas1[0].GetFormLength());
    word2 = TWtringBuf(lemmas2[0].GetForma(), lemmas2[0].GetFormLength());

    TWordformArray forms;
    for (size_t i = 0; i < lemmas1.size(); ++i)
        if (lemmas1[i].IsBastard() && IsFormOfLemma(word2, lemmas1[i], forms))
            return true;
    for (size_t i = 0; i < lemmas2.size(); ++i)
        if (lemmas2[i].IsBastard() && IsFormOfLemma(word1, lemmas2[i], forms))
            return true;

    return false;
}

bool TMorph::Similar(TWtringBuf w1, TWtringBuf w2)
{
    return Similar(w1, w2, TGramBitSet());
}

bool TMorph::HasStemGrammem(const TYandexLemma& word, TGrammar stemgram)
{
    TGramBitSet grammems;
    NSpike::ToGramBitset(word.GetStemGram(), grammems);
    return NormalizeGrammems(grammems).Has(stemgram);
}

bool TMorph::SimilarSurnSing(TWtringBuf w1, TWtringBuf w2)
{
    static const TGramBitSet sing(gSingular);
    return Similar(w1, w2, sing);    //don't require gSurname for now
}

bool TMorph::IdentifSimilar(TWtringBuf w1, TWtringBuf w2)
{
    static const TGramBitSet noun(gSubstantive);
    return Similar(w1, w2, noun);
}

int TMorph::MaxLenFlex(TWtringBuf word)
{
    int res = 0;
    if (word.size() >= 3) {
        TWLemmaArray lemmas;
        FindAllLemmas(word, lemmas);
        for (size_t i = 0; i < lemmas.size(); ++i)
            if (res < (int)lemmas[i].GetFlexLen())
                res = lemmas[i].GetFlexLen();
    }
    return res;
}

bool TMorph::FindWord(TWtringBuf word)
{
    TWLemmaArray lemmas;
    FindAllLemmas(word, lemmas);
    for (size_t i = 0; i < lemmas.size(); ++i)
        if (!IsBastard(lemmas[i]))
            return true;
    return false;
}

Wtroka TMorph::GetFormWithLongestPrefix(const yvector<Wtroka>& forms, const Wtroka& prefix)
{
    size_t forms_count = forms.size();
    YASSERT(forms_count > 0);
    if (prefix.empty() || forms_count == 1)
        return forms[0];

    size_t best_form = 0;
    size_t best_prefix = GetLongestPrefixLength(forms[0], prefix);
    // if there is a tie in best prefix len, select a form with minimal suffix
    YASSERT(forms[0].size() >= best_prefix);
    size_t best_suffix = forms[0].size() - best_prefix;
    for (size_t i = 1; i < forms_count; ++i) {
        size_t cur_prefix = GetLongestPrefixLength(forms[i], prefix);
        YASSERT(forms[i].size() >= cur_prefix);
        size_t cur_suffix = forms[i].size() - cur_prefix;
        if (cur_prefix > best_prefix || (cur_prefix == best_prefix && cur_suffix < best_suffix)) {
            best_prefix = cur_prefix;
            best_suffix = cur_suffix;
            best_form = i;
        }
    }
    return forms[best_form];
}

Wtroka TMorph::GetFormWithGrammems(const TYandexLemma& lemma, const TGramBitSet& grammems)
{
    TWordformArray forms;
    GenerateWordForms(lemma, forms, grammems);
    //select form with grammems which are as close as possible to requested @grammems
    //for now just take a form with minimal number of grammems + having the longest prefix with @lemma
    yvector<Wtroka> res;
    FilterFormsWithMinGrammems(forms, TMorph::NoFilter, res);
    return (res.empty()) ? Wtroka() : res[0];
}

bool TMorph::GetAllFormsOfLemma(const Wtroka& lemma, yvector< TPair<Wtroka, TGrammarBunch> >& res,
                                bool bUsePrediction /*=false*/)  //static
{
    TWLemmaArray lemmas;
    FindAllLemmas(lemma, lemmas);
    TWordformArray forms;
    for (size_t i = 0; i < lemmas.size(); ++i)
        //take all lemmas[i] where analyzed form is same as lemma
        if ((bUsePrediction || !IsBastard(lemmas[i])) && TMorph::IsLemmaForm(lemmas[i])) {
            forms.clear();
            GenerateWordForms(lemmas[i], forms);
            for (size_t i = 0; i < forms.size(); ++i) {
                res.push_back();
                res.back().first = forms[i].GetText();
                ToNormalizedGrammarBunch(forms[i], res.back().second);
            }
        }
    return forms.size() > 0;
}

size_t TMorph::SelectBestForms(const TWordformArray& forms, const TGramBitSet& desired_grammems, TWordformArray& best_forms)
{
    size_t best_count = 0;
    yvector<size_t> best_form_count(forms.size(), 0);
    for (size_t i = 0; i < forms.size(); ++i) {
        TGrammarBunch fgram;
        ToNormalizedGrammarBunch(forms[i], fgram);
        for (TGrammarBunch::const_iterator fg = fgram.begin(); fg != fgram.end(); ++fg) {
            size_t cur_count = (*fg & desired_grammems).count();
            if (cur_count > best_form_count[i]) {
                best_form_count[i] = cur_count;
                if (cur_count > best_count)
                    best_count = cur_count;
            }
        }
    }
    // remove forms with less than @best_count intersection
    best_forms.reserve(best_forms.size() + forms.size());
    for (size_t i = 0; i < forms.size(); ++i)
        if (best_form_count[i] >= best_count)
            best_forms.push_back(forms[i]);

    return best_count;
}

Wtroka TMorph::GetClosestForm(const Wtroka& lemma, const TGramBitSet& desired_grammems)
{
    return GetClosestForm(lemma, lemma, NoFilter, desired_grammems, false);
}

void TMorph::PrintGrammems(const TGramBitSet& grammems, Stroka& out)
{
    out += grammems.ToString(", ");
}

void PrintGrammarBunch(const TGrammarBunch& grammems, Stroka& out)
{
    if (!grammems.empty()) {
        TGrammarBunch::const_iterator it = grammems.begin();
        TMorph::PrintGrammems(*it, out);
        for (++it; it != grammems.end(); ++it) {
            out += " | ";
            TMorph::PrintGrammems(*it, out);
        }
    }
}

THomonymBasePtr TMorph::CreateHomonym()
{
    return new CHomonym(GetMainLanguage());
}

void TMorph::InitNumberFlex2Grammems(const Wtroka& numeral, size_t flexmap_index)
{
    TWLemmaArray lemmas;
    FindAllLemmas(numeral, lemmas);
    for (size_t i = 0; i < lemmas.size(); ++i) {
        TGramBitSet grammems;
        NSpike::ToGramBitset(lemmas[i].GetStemGram(), grammems);
        if (grammems.Has(gAdjNumeral)) {
            TWordformArray forms;
            GenerateWordForms(lemmas[i], forms);
            for (size_t j = 0; j < forms.size(); ++j) {
                const TYandexWordform& form = forms[j];
                TGrammarBunch formgrammems, filteredforms;
                ToNormalizedGrammarBunch(form, formgrammems);
                for (TGrammarBunch::const_iterator fgit = formgrammems.begin(); fgit != formgrammems.end(); ++fgit)
                    //filter excessive unnecessary forms (currently, only obsolete)
                    if (!fgit->Has(gObsolete))
                        filteredforms.insert(*fgit);

                if (!filteredforms.empty()) {
                    const Wtroka& s = form.GetText();
                    for (size_t i = 1; i <= 4 && i < s.length(); ++i) {
                        Wtroka flex = s.substr(s.length() - i);
                        m_NumberFlex2Grammems[flexmap_index][flex].insert(filteredforms.begin(), filteredforms.end());
                        //do same for index 0 (contains all numerals mixed, as in old behaviour)
                        m_NumberFlex2Grammems[0][flex].insert(filteredforms.begin(), filteredforms.end());
                    }

                    Wtroka norm_flex = s.substr(s.length() - Min<int>(2, s.length()));     //last 2 chars as normal flex, e.g. 2-oy, 1-iy
                    if (!norm_flex.empty() && norm_flex[0] == 0x44c) // 'ь'
                        norm_flex = norm_flex.substr(1);
                    if (!norm_flex.empty())
                        for (TGrammarBunch::const_iterator fgit = filteredforms.begin(); fgit != filteredforms.end(); ++fgit) {
                            m_NumberGrammems2Flex[flexmap_index][*fgit] = norm_flex;
                            m_NumberGrammems2Flex[0][*fgit] = norm_flex;        //for unknown numbers
                        }
                }
            }
            break;
        }
    }
}

void TMorph::InitNumberFlex2Grammems()
{
    //distinct cases of last 3-chars flex
    if (GetMainLanguage() == LANG_RUS) {
        InitNumberFlex2Grammems(UTF8ToWide("первый"), 1);
        InitNumberFlex2Grammems(UTF8ToWide("второй"), 2);
        InitNumberFlex2Grammems(UTF8ToWide("третий"), 3);

        InitNumberFlex2Grammems(UTF8ToWide("четвертый"), 1);
        InitNumberFlex2Grammems(UTF8ToWide("шестой"), 2);
        InitNumberFlex2Grammems(UTF8ToWide("седьмой"), 2);

        InitNumberFlex2Grammems(UTF8ToWide("сороковой"), 2);

        InitNumberFlex2Grammems(UTF8ToWide("тысячный"), 1);
    }
}

size_t TMorph::GetFlexMapIndex(const TWtringBuf& numeral) const
{
    size_t L = numeral.size();
    if (L <= 0)
        return 0;

    // 10-iy, 11-iy, 12, ... , 19-iy
    if (L >= 2 && numeral[L-2] == '1' && ::IsCommonDigit(numeral[L-1]))
        return 1;

    switch (numeral[L-1]) {
        case '0':
            if (L >= 2 && ::IsCommonDigit(numeral[L-2]) && numeral[L-2] != '4')
                return 1;   // 10-iy, 20-iy, 100-iy, 1000-iy, ...
            else
                return 2;   // 0-oy, 40-oy
        case '1':
        case '4':
        case '5':
        case '9':
            return 1;

        case '2':
        case '6':
        case '7':
        case '8':
            return 2;

        case '3':
            return 3;

        default:
            return 0;
    }
}

const TGrammarBunch* TMorph::GetNumeralGrammemsByFlex(const TWtringBuf& numeral, const TWtringBuf& s)
{
    const TMorph* morph = Singleton<TMorph>();

    size_t map_index = morph->GetFlexMapIndex(numeral);
    //currently unknown numerals are forbidden
    if (map_index == 0)
        return NULL;

    TWtringBuf flex = s.SubStr(s.size() - Min<int>(4, s.size()));
    TNumberFlex2GrammemsMap::const_iterator it = morph->m_NumberFlex2Grammems[map_index].find(flex);
    if (it != morph->m_NumberFlex2Grammems[map_index].end())
        return &(it->second);
    else
        return NULL;
}

const Wtroka* TMorph::GetNumeralFlexByGrammems(const TWtringBuf& numeral, const TGramBitSet& grm)
{
    const TMorph* morph = Singleton<TMorph>();

    size_t map_index = morph->GetFlexMapIndex(numeral);
    //currently unknown numerals are forbidden
    if (map_index == 0)
        return NULL;

    static const TGramBitSet mask(NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers | TGramBitSet(gAnimated, gInanimated));
    TGramBitSet req_grammems = grm & mask;
    ymap<TGramBitSet, Wtroka>::const_iterator it = morph->m_NumberGrammems2Flex[map_index].begin();

    const Wtroka* res = NULL;
    for (; it != morph->m_NumberGrammems2Flex[map_index].end(); it++) {
        const TGramBitSet& key = it->first;
        if (key.HasAll(req_grammems)) {
            const Wtroka* cur = &(it->second);
            if (res == NULL)
                res = cur;
            else if (*res != *cur)      // ambiguity
                return NULL;
        }
    }
    return res;
}

void TMorph::LoadSurnamePrediction(CGramInfo* pGramInfo)
{
    // private, no locking required
    if (m_SurnamePredictor.Get() == NULL) {
        THolder<TSurnamePredictor> tmp(pGramInfo->LoadSurnamePrediction());
        m_SurnamePredictor.Reset(tmp.Release());
    }
}

bool TMorph::PredictSurname(const Wtroka& word, yvector<TSurnamePredictor::TPredictedSurname>& predictedSurnames)
{
    TMorph* morph = Singleton<TMorph>();
    if (morph->m_SurnamePredictor.Get() != NULL)
        return morph->m_SurnamePredictor->Predict(word, predictedSurnames);
    else
        return false;
}
