#include "sequence.h"
#include "homonym.h"

#include <FactExtract/Parser/afdocparser/rus/morph.h>
#include <FactExtract/Parser/afdocparser/rus/numberprocessor.h>

#include <FactExtract/Parser/afdocparser/rusie/primitivegroup.h>
#include <FactExtract/Parser/afdocparser/rusie/dictsholder.h>

#include <FactExtract/Parser/inflectorlib/gramfeatures.h>
#include <FactExtract/Parser/inflectorlib/complexword.h>
#include <dict/dictutil/dictutil.h>

using NInfl::DefaultFeatures;
using NInfl::TFeature;
using NInfl::TComplexWord;
using NInfl::TSimpleAutoColloc;
using NInfl::TAgreementMeasure;

CWordsPair TTomitaWords::TomitaCoords(const CWordsPair& group) const
{
    int first = -1, last = -1;
    for (size_t i = 0; i < TomitaWords.size(); ++i) {
        const CWordsPair& p = Words.GetWord(TomitaWords[i]).GetSourcePair();
        if (p.FirstWord() == group.FirstWord())
            first = i;

        if (p.LastWord() == group.LastWord()) {
            last = i;
            break;
        }
    }

    return (first != -1 && last != -1) ? CWordsPair(first, last) : CWordsPair();
};

const CFactSynGroup* TTomitaWords::GetChildGroup(const CFactSynGroup& parent, const CWordSequence& seq) const {
    CWordsPair p = TomitaCoords(seq);
    if (!p.IsValid())
        return NULL;

    const CFactSynGroup* group = parent.GetChildByWordPair(p);
    if (!group && p.LastWord() + 1 == parent.LastWord() && (*this)[parent.LastWord()].IsEndOfStreamWord()) {
        p.SetPair(p.FirstWord(), p.LastWord() + 1);
        group = parent.GetChildByWordPair(p);
    }
    return group;
}

inline bool GroupHasWord(const CGroup* group, int wordIndex) {
    return wordIndex >= group->FirstWord() && wordIndex <= group->LastWord();
}

SWordHomonymNum TTomitaWords::GetFirstHomonymNum(const CSynGroup* group, size_t wordIndex) const
{
    yvector<int> tmp;
    return GetFirstHomonymNum(group, wordIndex, tmp);
}

SWordHomonymNum TTomitaWords::GetFirstHomonymNum(const CSynGroup* group, size_t wordIndex, yvector<int>& alternativeHomIDs) const {
    YASSERT(GroupHasWord(group, wordIndex));
    ui32 homIDs = group->GetChildHomonym(wordIndex - group->FirstWord());
    const CWord& w = (*this)[wordIndex];
    int homIndex = w.HasFirstHomonymByHomIDs_i(homIDs);
    YASSERT(homIndex >= 0);
    for (CWord::SHomIt itHom = w.IterHomonyms(); itHom.Ok(); ++itHom)
        if(itHom.TestMask(homIDs) && (int)itHom.GetID() != homIndex)
            alternativeHomIDs.push_back(itHom.GetID());
    SWordHomonymNum wh = TomitaWords[wordIndex];
    wh.m_HomNum = homIndex;
    return wh;
}

void TSequenceInflector::ResetMainWord(const CFactSynGroup* group) {
    MainWordIndex = group->GetMainPrimitiveGroup()->FirstWord();
}

void TSequenceInflector::Init() {

    ResetMainWord(Group);

    Items.resize(TomitaWords.Size());
    int openQuote = -1;
    for (int w = Group->FirstWord(); w <= Group->LastWord(); ++w) {
        // initialize pointers only for @Group words
        Items[w].Word = &TomitaWords[w];

        if (Items[w].Word->HasOpenQuote())
            openQuote = w;

        // use quotes only if they are completely inside of inflected sequence
        if (Items[w].Word->HasCloseQuote() && openQuote != -1) {
            Items[w].UseQuotes = true;
            Items[openQuote].UseQuotes = true;
        }
    }

    ExtractAgreements(*Group);

    // first, select best agreed homonym for main word
    Items[MainWordIndex].Homonym = SelectBestHomonym(MainWordIndex, Items[MainWordIndex].AlternativeHomonyms);

    // then re-select homonyms with best agreement for other words
    for (int w = Group->FirstWord(); w <= Group->LastWord(); ++w)
        Items[w].Homonym = SelectBestHomonym(w, Items[w].AlternativeHomonyms);

    // finally, if some of homonyms are still not selected, use the first one
    for (int w = Group->FirstWord(); w <= Group->LastWord(); ++w) {
        if (Items[w].Homonym == NULL)
            Items[w].Homonym = &TomitaWords.GetFirstHomonym(Group, w, Items[w].AlternativeHomonyms);
        YASSERT(Items[w].Homonym != NULL);
    }
}

void TSequenceInflector::ExtractAgreements(const CFactSynGroup& group) {

    for (size_t a = 0; a < group.m_CheckedAgrs.size(); ++a) {
        const CCheckedAgreement& agr = group.m_CheckedAgrs[a];

        // if the words to be argeed are out of the specified sequence of words
        // then ignore such agreement
        if (!WordSequence.Includes(TomitaWords[agr.m_WordNo1].GetSourcePair()) ||
            !WordSequence.Includes(TomitaWords[agr.m_WordNo2].GetSourcePair()))
            continue;

        if (agr.IsUnary())
            Items[agr.m_WordNo1].Enforce |= agr.m_AgreementType.m_Grammems;
        else
            ExtractBinaryAgreement(agr);
    }
}

void TSequenceInflector::ExtractBinaryAgreement(const CCheckedAgreement& agr) {
    TGramBitSet agree;
    const NInfl::TFeatureHolder& ft = DefaultFeatures();
    switch (agr.m_AgreementType.e_AgrProcedure) {
        case SubjVerb:
        case SubjAdjShort:      agree = ft.BitSet(TFeature::Gender, TFeature::Number, TFeature::Person); break;

        case GendreNumberCase:
        case AfterNumAgr:       agree = ft.BitSet(TFeature::Gender, TFeature::Number, TFeature::Case, TFeature::Anim); break;

        case GendreNumber:      agree = ft.BitSet(TFeature::Gender, TFeature::Number); break;
        case NumberCaseAgr:     agree = ft.BitSet(TFeature::Number, TFeature::Case, TFeature::Anim); break;

        case CaseAgr:
        case FeminCaseAgr:      agree = ft.BitSet(TFeature::Case, TFeature::Anim); break;

        case NumberAgr:         agree = ft.BitSet(TFeature::Number); break;
        case GendreCase:        agree = ft.BitSet(TFeature::Gender, TFeature::Case, TFeature::Anim); break;

        case IzafetAgr:
            // TODO: use word order?
            Items[agr.m_WordNo1].Keep |= ft.BitSet(TFeature::Poss, TFeature::Predic);
            Items[agr.m_WordNo2].Keep |= ft.BitSet(TFeature::Poss, TFeature::Predic);
            break;

        case CoordFuncCount:    YASSERT(false);
        default:                /* do nothing */ ;
    }
    if (agree.any())
        AddBinaryAgreement(agr.m_WordNo1, agr.m_WordNo2, agree);
}

void TSequenceInflector::AddBinaryAgreement(size_t w1, size_t w2, const TGramBitSet& agree) {
    for (size_t i = 0; i < BinAgrs.size(); ++i)
        if ((BinAgrs[i].Word1 == w1 && BinAgrs[i].Word2 == w2) ||
            (BinAgrs[i].Word2 == w1 && BinAgrs[i].Word1 == w2)) {
                BinAgrs[i].Grammems |= agree;
                return;
        }
    TBinAgr bin = {w1, w2, agree};
    BinAgrs.push_back(bin);
}

void TSequenceInflector::CollectExcludedWords(const CGroup& group)
{
    if (Grammar == NULL)
        return;

    const CGrammarItem& grammarItem = Grammar->m_UniqueGrammarItems[group.GetActiveSymbol()];
    if (grammarItem.m_PostfixStore.m_exclude_from_normalization == YEs) {
        for (int i = group.FirstWord(); i <= group.LastWord(); ++i)
            if (i >= 0)
                ExcludeWord(i);
        return;
    }

    if (!group.IsPrimitive()) {
        const CFactSynGroup* pSynGroup = CheckedCast<const CFactSynGroup*>(&group);
        for (size_t i = 0; i < pSynGroup->ChildrenSize(); ++i)
            CollectExcludedWords(*pSynGroup->GetChild(i));
    }
}

void TSequenceInflector::ExcludeOdinIz() {

    if (TMorph::GetMainLanguage() != LANG_RUS)
        return;

    DECLARE_STATIC_RUS_WORD(kOdin, "один");
    DECLARE_STATIC_RUS_WORD(kIz, "из");

    const CHomonym* hom = Items[MainWordIndex].Homonym;
    if (hom->GetLemma() != kOdin || MainWordIndex + 1 >= TomitaWords.Size())
        return;

    const CWord& pIzWord = TomitaWords[MainWordIndex + 1];
    if (pIzWord.GetLowerText() != kIz)
        return;

    size_t start = 2;
    if (MainWordIndex + start != (size_t)Group->LastWord()) {
        const CWord& pPossibleNumeral = TomitaWords[MainWordIndex + start];
        if (pPossibleNumeral.m_typ == Digit || pPossibleNumeral.HasPOS(gNumeral))
            start++;
    }

    const CFactSynGroup* innerGroup = Group->GetMaxChildWithThisFirstWord(MainWordIndex + start);
    if (innerGroup == NULL)
        return;

    for (size_t i = 0; i < start; ++i)
        ExcludeWord(MainWordIndex + i);

    ResetMainWord(innerGroup);
}

void TSequenceInflector::DelegateExcludedDescriptors() {
    // when descriptor of some named entity is excluded, transfer its grammar role to described entity.
    // e.g.: [glave repspubliki Bashkiria] -> [glava Bashkortostana]
    for (int i = Group->FirstWord(); i < Group->LastWord(); ++i) {
        if (!Items[i].Excluded)
            continue;

        const CFactSynGroup* innerGroup = Group->GetMaxChildWithThisFirstWord(i + 1);
        if (innerGroup == NULL)
            continue;

        size_t inner = innerGroup->GetMainPrimitiveGroup()->FirstWord();
        if (!Items[inner].Excluded && Items[inner].Enforce.Has(gNominative)) {
            TGramBitSet mut = DefaultFeatures().Mutable(Items[inner].Enforce);
            mut = DefaultFeatures().ExtendAllFeatures(mut);
            Items[inner].Enforce &= ~mut;
            AddBinaryAgreement(i, inner, mut);

            if ((size_t)i == MainWordIndex)
                ResetMainWord(innerGroup);
        }
    }
}

static TAgreementMeasure FindBestAgreement(const TGramBitSet& form1, const CHomonym& hom2) {
    if (!hom2.Grammems.HasForms())
        return TAgreementMeasure(form1, hom2.Grammems.All());

    TAgreementMeasure best;
    for (THomonymGrammems::TFormIter form2 = hom2.Grammems.IterForms(); form2.Ok(); ++form2)
        best.ResetIfBetter(form1, *form2);
    return best;
}

static TAgreementMeasure FindBestAgreement(const TGramBitSet& form1, const CWord& word2, ui32 mask) {
    TAgreementMeasure best;
    for (CWord::SHomIt hom2 = word2.IterHomonyms(); hom2.Ok(); ++hom2)
        if (hom2.TestMask(mask))
            best.ResetIfBetter(FindBestAgreement(form1, *hom2));
    return best;
}

class TSequenceInflector::TImpl {
public:
    static TAgreementMeasure FindBestAgreedForm(const TSequenceInflector& infl, size_t wordIndex, const TGramBitSet& form1);
};

TAgreementMeasure TSequenceInflector::TImpl::FindBestAgreedForm(const TSequenceInflector& infl,
                                                                size_t wordIndex, const TGramBitSet& form) {

    TAgreementMeasure best;
    for (size_t i = 0; i < infl.BinAgrs.size(); ++i) {
        if (infl.BinAgrs[i].Word1 != wordIndex && infl.BinAgrs[i].Word2 != wordIndex)
            continue;

        size_t w2 = infl.BinAgrs[i].Word1 + infl.BinAgrs[i].Word2 - wordIndex;
        if (infl.Items[w2].Homonym == NULL) {
            ui32 ids2 = infl.Group->GetChildHomonyms()[w2 - infl.Group->FirstWord()];
            best.ResetIfBetter(FindBestAgreement(form, *infl.Items[w2].Word, ids2));
        } else
            best.ResetIfBetter(FindBestAgreement(form, *infl.Items[w2].Homonym));
    }

    return best;
}

TGramBitSet TSequenceInflector::SelectBestHomoform(size_t wordIndex, const CHomonym& hom, const TGramBitSet& desired) const {

    if (!hom.Grammems.HasForms())
        return hom.Grammems.All();

    bool found = false;
    TGramBitSet best;
    TAgreementMeasure bestAgr;

    for (THomonymGrammems::TFormIter it = hom.Grammems.IterForms(); it.Ok(); ++it) {
        TAgreementMeasure curFormBest = TImpl::FindBestAgreedForm(*this, wordIndex, *it);
        if (bestAgr.ResetIfBetter(curFormBest)) {
            best = *it;
            found = true;
        }
    }
    return found ? best : THomonymInflector::SelectBestHomoform(hom, desired);
}

const CHomonym* TSequenceInflector::SelectBestHomonym(size_t wordIndex, yvector<const CHomonym*>& alternativeHomonyms) const {

    ui32 ids = Group->GetChildHomonyms()[wordIndex - Group->FirstWord()];
    const CHomonym* best = NULL;
    TAgreementMeasure bestAgr;

    for (CWord::SHomIt homIt = Items[wordIndex].Word->IterHomonyms(); homIt.Ok(); ++homIt) {
        if (!homIt.TestMask(ids))
            continue;

        TAgreementMeasure curHomBestAgr;
        if (!homIt->Grammems.HasForms())
            curHomBestAgr = TImpl::FindBestAgreedForm(*this, wordIndex, homIt->Grammems.All());
        else for (THomonymGrammems::TFormIter it = homIt->Grammems.IterForms(); it.Ok(); ++it)
            curHomBestAgr.ResetIfBetter(TImpl::FindBestAgreedForm(*this, wordIndex, *it));

        if (bestAgr.ResetIfBetter(curHomBestAgr))
            best = &(*homIt);
    }

    if (best)
    {
        for (CWord::SHomIt homIt = Items[wordIndex].Word->IterHomonyms(); homIt.Ok(); ++homIt) {
            if (!homIt.TestMask(ids))
                continue;
            if (best == &(*homIt))
                continue;

            TAgreementMeasure curHomBestAgr;
            if (!homIt->Grammems.HasForms())
                curHomBestAgr = TImpl::FindBestAgreedForm(*this, wordIndex, homIt->Grammems.All());
            else
                for (THomonymGrammems::TFormIter it = homIt->Grammems.IterForms(); it.Ok(); ++it)
                    curHomBestAgr.ResetIfBetter(TImpl::FindBestAgreedForm(*this, wordIndex, *it));

            if (bestAgr.ArgeedExplicitly == curHomBestAgr.ArgeedExplicitly && bestAgr.Disagreed >= curHomBestAgr.Disagreed)
                alternativeHomonyms.push_back(&(*homIt));
        }
    }

    if (alternativeHomonyms.size() > 0)
        return NULL;

    return best;
}

static inline bool IsNumber(const CHomonym& hom) {
    return hom.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Number, KW_DICT);
}

static bool HasGztLemma(const CHomonym& h) {
    if (h.HasGztArticle() && h.GetGztArticle().GetLemmaInfo() != NULL)
        return true;

    const article_t* piArt = GlobalDictsHolder->GetAuxArticle(h, KW_DICT);
    return piArt != NULL && piArt->has_lemma();
}


bool TSequenceInflector::ShouldUseGztLemma(size_t wordIndex) const {
    const CHomonym& h = *Items[wordIndex].Homonym;
    bool hasGztLemma = false;
    if (h.HasGztArticle()) {
        const NGzt::TMessage* lemma_info = h.GetGztArticle().GetLemmaInfo();
        if (lemma_info != NULL) {
            if (h.GetGztArticle().GetLemmaAlways(*lemma_info))
                return true;
            hasGztLemma = true;
        }
    } else {
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(h, KW_DICT);
        if (piArt != NULL && piArt->has_lemma()) {
            if (piArt->get_lemma_info().m_bAlwaysReplace)
                return true;
            hasGztLemma = true;
        }
    }

    if (hasGztLemma && !RestrictGztLemma && Grammar != NULL)
        return WasCheckedByKWType(wordIndex);

    return false;
}

inline bool TSequenceInflector::WasCheckedByKWType(size_t wordIndex) const {
    return WasCheckedByKWType(wordIndex, Group) ||
           WasCheckedByKWSet(wordIndex, Group);
}

bool TSequenceInflector::WasCheckedByKWType(size_t wordIndex, const CGroup* group) const {

    YASSERT(Grammar != NULL);
    CWordsPair wp(wordIndex);
    const CGrammarItem& grammarItem = Grammar->m_UniqueGrammarItems[group->GetActiveSymbol()];
    if (*(group) == wp) {
        //const CGrammarItem& grammarItem = Grammar->m_UniqueGrammarItems[group->GetActiveSymbol()];
        const SArtPointer& kwtype = grammarItem.GetPostfixKWType(DICT_KWTYPE);
        // TODO: use exact article title matching
        //if (kwtype.IsValid() && (GlobalDictsHolder->MatchArticlePtr(*Items[wordIndex].Homonym, KW_DICT, kwtype, true)))
        if (GlobalDictsHolder->HasArticle(*Items[wordIndex].Homonym, kwtype, KW_DICT))
            return true;
    }

    if (!group->IsPrimitive()) {
        const CFactSynGroup* fgroup = CheckedCast<const CFactSynGroup*>(group);
        for (size_t i = 0; i < fgroup->ChildrenSize(); ++i) {
            const CGroup* child = fgroup->GetChild(i);
            if (child->Includes(wp)) {
                if (WasCheckedByKWType(wordIndex, child))
                    return true;
                else
                    break; // no other child of @fgroup can include @wp
            }
        }
    }
    return false;
}

bool TSequenceInflector::WasCheckedByKWSet(size_t wordIndex, const CGroup* group) const {

    const CFactSynGroup* fgroup = dynamic_cast<const CFactSynGroup*>(group);
    if (fgroup == NULL)
        return false;

    if (fgroup->CheckedConstraints() != NULL) {
        typedef ymap<int, SKWSet> TChildKWSets;
        const TChildKWSets& kwsets = fgroup->CheckedConstraints()->m_KWSets;
        for (TChildKWSets::const_iterator it = kwsets.begin(); it != kwsets.end(); ++it) {
            if (it->first < 0 || it->first >= (int)fgroup->ChildrenSize())
                continue;

            if (WasCheckedByKWSet(wordIndex, fgroup->GetChild(it->first), it->second))
                return true;
        }
    }

    // check same for constraints of children
    CWordsPair wp(wordIndex);
    for (size_t i = 0; i < fgroup->ChildrenSize(); ++i) {
        const CGroup* child = fgroup->GetChild(i);
        if (child->Includes(wp)) {
            if (WasCheckedByKWSet(wordIndex, child))
                return true;
            else
                break;
        }
    }

    return false;
}

bool TSequenceInflector::WasCheckedByKWSet(size_t wordIndex, const CGroup* child, const SKWSet& kwset) const {
    if (kwset.m_bNegative)
        return false;

    CWordsPair wp(wordIndex);
    if (child->Includes(wp)) {
        const CGroup* subchild = (kwset.m_bApplyToTheFirstWord) ? child->GetFirstGroup() : child->GetMainGroup();
        if (*subchild->GetMainPrimitiveGroup() == wp)
            return true;
    }
    return false;
}

static const SArticleLemmaInfo& GetLemmaInfo(const CHomonym& h, Wtroka& lemma_text, SArticleLemmaInfo& lemma_info_scratch) {
    if (h.HasGztArticle() && h.GetGztArticle().FillSArticleLemmaInfo(lemma_info_scratch, lemma_text))
        return lemma_info_scratch;
    else {
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(h, KW_DICT);
        if (piArt == NULL) {
            lemma_info_scratch.Reset();
            lemma_text.clear();
            return lemma_info_scratch;
        }
        lemma_text = piArt->get_lemma();
        return piArt->get_lemma_info();
    }
}

static void CopyQuotes(Wtroka& text, const CWord& w) {
    const wchar16 DQUOTE = L'\"';
    if (!text.empty()) {
        if (w.HasOpenQuote() && !::IsQuotation(text[0]))
            text.prepend(DQUOTE);
        if (w.HasCloseQuote() && !::IsQuotation(text.back()))
            text.append(DQUOTE);
    }
}

const CHomonym& TSequenceInflector::GetHomonymWithBestGztLemmaByLevenshteinDist(size_t wordIndex) const
{
    const CHomonym* bestHom = Items[wordIndex].Homonym;
    if (Items[wordIndex].AlternativeHomonyms.empty())
        return *bestHom;
    if (!bestHom->HasGztArticle())
        return *bestHom;
    const CWord& word = *Items[wordIndex].Word;
    const Wtroka& originalText = word.GetLowerText();
    SArticleLemmaInfo tmp;
    Wtroka lemmaText;
    GetLemmaInfo(*bestHom, lemmaText, tmp);
    int minDist = strdist(originalText, lemmaText);
    for (int i = 0; i < Items[wordIndex].AlternativeHomonyms.ysize(); i++)
    {
        const CHomonym* hom = Items[wordIndex].AlternativeHomonyms[i];
        GetLemmaInfo(*hom, lemmaText, tmp);
        int dist = strdist(originalText, lemmaText);
        if(dist < minDist) {
            bestHom = hom;
            minDist = dist;
        }
    }
    return *bestHom;
}

const SArticleLemmaInfo& TSequenceInflector::GetBestGztLemmaByLevenshteinDist(const CWord& w, const CHomonym& hom, Wtroka& lemmaText, SArticleLemmaInfo& lemmInfo) const
{
    GetLemmaInfo(hom, lemmaText, lemmInfo);
    if(hom.GetExtraGztArticles().empty())
        return lemmInfo;

    const Wtroka& originalText = w.GetLowerText();
    const yset<TGztArticle>& arts = hom.GetExtraGztArticles();
    yset<TGztArticle>::const_iterator it = arts.begin();
    const TGztArticle* bestArt = &hom.GetGztArticle();
    int minDist = strdist(originalText, lemmaText);
    for(; it != arts.end(); it++)
    {
        const TGztArticle& art = *it;
        if(art.FillSArticleLemmaInfo(lemmInfo, lemmaText))
        {
            int dist = strdist(originalText, lemmaText);
            if(dist < minDist)
            {
                minDist = dist;
                bestArt = &(*it);
            }
        }
    }
    if (!bestArt->FillSArticleLemmaInfo(lemmInfo, lemmaText)) {
        lemmInfo.Reset();
        lemmaText.clear();
    }

    return lemmInfo;
}

bool TSequenceInflector::InflectGztLemma(size_t wordIndex, const TGramBitSet& grammems,
                                         Wtroka& restext, TGramBitSet& resgram) const {

    const CWord& word = *Items[wordIndex].Word;
    const CHomonym& hom = GetHomonymWithBestGztLemmaByLevenshteinDist(wordIndex);


    SArticleLemmaInfo tmp;
    Wtroka lemmaText;
    const SArticleLemmaInfo& lemma_info = GetBestGztLemmaByLevenshteinDist(word, hom, lemmaText, tmp);
    if (lemma_info.IsEmpty())
        return false;

    TMorph::ToLower(lemmaText);
    if (lemmaText == hom.Lemma)
    {
        bool bRes = THomonymInflector(word, &hom, Words).Inflect(grammems, restext, resgram);     // shorter way
        TComplexWord::MimicCapitalization(word.DecapitalizedText(), restext);
        return bRes;
    }

    // source word grammems
    TGramBitSet curgram = SelectBestHomoform(wordIndex, hom, grammems);
    TGramBitSet fixed = DefaultFeatures().Mutable(DefaultFeatures().ReplaceFeatures(curgram, grammems));

    bool found = false;
    if (lemma_info.m_bIndeclinable || DefaultFeatures().IsNormalOnly(fixed)) {
        // a bit of optimization: do not launch full inflecting procedure if we are already normalized
        restext = lemmaText;
        resgram = fixed;
        found = true;

    } else if (lemma_info.m_Lemmas.size() == 1) {
        TComplexWord infl(hom.Lang, lemma_info.m_Lemmas[0], lemma_info.m_Lemmas[0]);
        found = infl.Inflect(fixed, restext, &resgram);

    } else {
        size_t main = lemma_info.m_iMainWord;
        TSimpleAutoColloc colloc;
        for (size_t i = 0; i < lemma_info.m_Lemmas.size(); ++i) {
            if (i == main)
                // main word lemma must be written in normal form (in gazetteer)
                colloc.AddWord(TComplexWord(hom.Lang, lemma_info.m_Lemmas[i], lemma_info.m_Lemmas[i]), true);
            else
                // otherwise do not fix lemma, it will be selected later on collocation re-agreement
                colloc.AddWord(TComplexWord(hom.Lang, lemma_info.m_Lemmas[i]), false);
        }
        colloc.ReAgree();
        yvector<Wtroka> modLemma;
        if (colloc.Inflect(fixed, modLemma, &resgram)) {
            YASSERT(!modLemma.empty());
            restext = modLemma[0];
            for (size_t i = 1; i < modLemma.size(); ++i)
                restext.append(' ').append(modLemma[i]);
            found = true;
        }
    }

    // hot fix (FACTEX-2258)
    if (!found) {
        restext = lemmaText;
        found = true;
    }

    if (found) {
        // as gazetteer lemmas could have different word number, use more complex capitalization scheme
        TComplexWord::MimicCapitalization(word.DecapitalizedText(), restext);
        return true;
    } else
        return false;
}

bool TSequenceInflector::NormalizeAsNumber(const CHomonym& hom, Wtroka& restext) const {
    YASSERT(IsNumber(hom));
    const CNumber* pNumber = CheckedCast<const CNumber*>(hom.GetSourceWordSequence());
    if (!pNumber->m_Numbers.empty()) {
        UTF8ToWide(ToString(static_cast<int>(pNumber->m_Numbers[0])), restext);
        return true;
    } else
        return false;
}

static inline Wtroka ExtractSequenceText(CWordSequence* ws) {
    if (dynamic_cast<CFioWordSequence*>(ws) != NULL)
        return ws->CWordSequence::GetCapitalizedLemma();
    else
        return ws->GetCapitalizedLemma();
}

bool TSequenceInflector::InflectArtificialLemma(size_t wordIndex, const TGramBitSet& grammems,
                                                Wtroka& restext, TGramBitSet& resgram) const {

    yvector<Wtroka> tmpLemmas;
    CWordSequence* ws = Items[wordIndex].Homonym->GetSourceWordSequence();

    YASSERT(ws != NULL);
    YASSERT(Items[wordIndex].Homonym->HasArtificialLemma());

    if (grammems.any() && DefaultFeatures().IsNormalOnly(grammems)) {

        bool usenorm = dynamic_cast<CFioWordSequence*>(ws) == NULL;
                    //   (CWordsPair&)(*Group) == (CWordsPair&)(*ws);

        if (ws->IsNormalizedLemma() && usenorm) {
            restext = ExtractSequenceText(ws);
            resgram = grammems;
            return true;
        } else if (!InflectSimpleSequence(wordIndex, *ws, grammems, tmpLemmas, resgram))
            return false;

        // lazy normalization is performed here
        ws->ResetLemmas(tmpLemmas, true);
        restext = ExtractSequenceText(ws);
        return true;

    } else if (InflectSimpleSequence(wordIndex, *ws, grammems, tmpLemmas, resgram)) {
        CWordSequence tmp;
        tmp.ResetLemmas(tmpLemmas, true);
        restext = ExtractSequenceText(&tmp);
        return true;
    }

    return false;
}


// Modify main word and words before the main word if they look agreed with it.
bool TSequenceInflector::InflectSimpleSequence(size_t wordIndex, const CWordSequence& seq, const TGramBitSet& grammems,
                                               yvector<Wtroka>& restext, TGramBitSet& resgram) const
{
    if (THomonymInflector::FindInForms(seq.GetGrammems(), grammems, resgram)) {
        for (int i = seq.FirstWord(); i <= seq.LastWord(); ++i)
            restext.push_back(Words[i].GetText());
        return true;
    }

    SWordHomonymNum main = seq.GetMainWord();
    const CWord& mainWord = Words.GetWord(main);
    const CHomonym& mainHom = Words[main];
    TGramBitSet mainGram = DefaultFeatures().Mutable(SelectBestHomoform(wordIndex, mainHom, grammems));

    TSimpleAutoColloc colloc;
    for (int i = seq.FirstWord(); i < mainWord.GetSourcePair().FirstWord(); ++i)
        colloc.AddWord(THomonymInflector(Words[i], NULL, Words).MakeComplexWord(mainGram, true));
    colloc.AddMainWord(THomonymInflector(mainWord, &mainHom, Words).MakeComplexWord(grammems, false));

    // append following adjectives
    // (for cases like "������ ���������������" or "2-�� ������������"
    // where the first is the main by default)
    int last = mainWord.GetSourcePair().LastWord() + 1;
    for (; last <= seq.LastWord() && Words[last].HasMorphAdj(); ++last)
        colloc.AddWord(THomonymInflector(Words[last], NULL, Words).MakeComplexWord(mainGram, true));

    colloc.ReAgree();


    restext.clear();
    if (!colloc.Inflect(grammems, restext, &resgram))
        return false;

    // append the rest words unchanged (if any)
    for (int i = last; i <= seq.LastWord(); ++i)
        restext.push_back(Words[i].GetText());

    return true;
}

bool TSequenceInflector::InflectWord(size_t wordIndex, const TGramBitSet& grammems, bool isnew,
                                     Wtroka& restext, TGramBitSet& resgram) const  {

    const CWord& word = *Items[wordIndex].Word;
    const CHomonym& hom = *Items[wordIndex].Homonym;

    bool useGztLemma = ShouldUseGztLemma(wordIndex);
    if (useGztLemma) {
        if (InflectGztLemma(wordIndex, grammems, restext, resgram))
            return true;

        if (IsNumber(hom) && NormalizeAsNumber(hom, restext)) {
            resgram = grammems;
            return true;
        }
    }

    if (isnew && hom.HasArtificialLemma()) {
        bool isFio = hom.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Fio, KW_DICT);
        if ((isFio && WasCheckedByKWType(wordIndex)) ||
            (!isFio && word.IsMultiWord()))
                if (InflectArtificialLemma(wordIndex, grammems, restext, resgram))
                    return true;
    }

    if (!isnew || word.IsMultiWord()) {
        restext = Words.ToString(word);
        resgram = grammems;
        return true;
    }

    return THomonymInflector(word, &hom, Words).Inflect(grammems, restext, resgram);
}

bool TSequenceInflector::InflectWord(size_t wordIndex, const TGramBitSet& grammems)  {

    YASSERT(GroupHasWord(Group, wordIndex));
    TItem& item = Items[wordIndex];

    TGramBitSet extra = (item.Homonym->Grammems.All() & item.Keep) | item.Enforce;
    TGramBitSet requested = DefaultFeatures().ReplaceFeatures(extra, grammems);
    bool isnew = grammems.any();
    if (InflectWord(wordIndex, requested, isnew, item.ResultText, item.ResultGrammems)) {
        if (item.UseQuotes)
            CopyQuotes(item.ResultText, *item.Word);
        return true;
    } else
        return false;
}


bool TSequenceInflector::InflectItems(const TGramBitSet& grammems) {

    // normalized text will be collected here:
    YASSERT(Items.size() == TomitaWords.Size());
    for (size_t w = 0; w < TomitaWords.Size(); ++w) {
        Items[w].ResultText.clear();
        Items[w].ResultGrammems.Reset();
        Items[w].AgreedNoun = false;
    }

    //get words excluded from normalization with "cut" label
    CollectExcludedWords(*Group);

    ExcludeOdinIz();
    DelegateExcludedDescriptors();

    // just for debugging
    const CWord& mainword = *Items[MainWordIndex].Word; (void)mainword;
    const CHomonym& mainhom = *Items[MainWordIndex].Homonym; (void)mainhom;

    // MAIN WORD
    if (!InflectWord(MainWordIndex, grammems))
        return false;

    // DEPENDENT WORDS
    bool checkAgain = true;
    while (checkAgain) {
        // first enforce binary agreements
        while (checkAgain) {
            checkAgain = false;
            for (size_t a = 0; a < BinAgrs.size(); ++a) {
                const TBinAgr& agr = BinAgrs[a];
                if (Items[agr.Word1].ResultText.empty() == Items[agr.Word2].ResultText.empty())
                    continue;
                size_t srcWord = agr.Word1;
                size_t dstWord = agr.Word2;
                if (Items[srcWord].ResultText.empty())
                    ::DoSwap(srcWord, dstWord);

                if (InflectWord(dstWord, Items[srcWord].ResultGrammems & agr.Grammems)) {
                    Items[dstWord].AgreedNoun = true;
                    checkAgain = true;
                }
            }
        }

        // enforce still unchecked unary agrs
        for (int i = Group->FirstWord(); i <= Group->LastWord(); ++i)
            if (Items[i].ResultText.empty() && Items[i].Enforce.any())
                if (InflectWord(i, TGramBitSet()))   // will be enforced inside
                    checkAgain = true;
    }

    return true;
}

static inline const CHomonym& SelectAdjectiveHom(const CWord& word) {
    int iH = word.HasMorphAdj_i();
    if (iH == -1) {
        YASSERT(word.GetHomonymsCount() > 0);
        return *word.IterHomonyms();
    } else
        return word.GetRusHomonym(iH);
}

static inline const CHomonym& SelectNounHom(const CWord& word) {
    int iH = word.HasMorphNoun_i();
    if (iH == -1) {
        YASSERT(word.GetHomonymsCount() > 0);
        return *word.IterHomonyms();
    } else
        return word.GetRusHomonym(iH);
}

//������ ������ � ������������ ";" ������� (�����, ����� ��� ����������
//���� ����� �������� ��� ����� �� ���� - ��� ��������)]
static bool AddCommaSeparatedWordInfo(const CHomonym& h, bool isMain, CTextWS& ws) {
    Wtroka lemma_text;
    SArticleLemmaInfo tmp_info;
    const SArticleLemmaInfo& lemma_info = GetLemmaInfo(h, lemma_text, tmp_info);

    if (lemma_info.IsEmpty())
        return false;

    //���� �����.����� = ��
    if (lemma_info.m_bIndeclinable) {
        for (size_t i = 0; i < lemma_info.m_Lemmas.size(); ++i) {
            Wtroka t = lemma_info.m_Lemmas[i];
            ws.AddWordInfo(lemma_info.m_Lemmas[i], isMain && (int)i == lemma_info.m_iMainWord);
        }
        return true;
    }

    for (size_t i = 0; i < lemma_info.m_Lemmas.size(); ++i) {
        if (isMain && (int)i == lemma_info.m_iMainWord) {
            ws.AddWordInfo(lemma_info.m_Lemmas[i], true);
            continue;
        }

        CWord w(lemma_info.m_Lemmas[i], false);
        w.CreateHomonyms();
        bool foundLemma = false;
        for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it)
            if (it->GetLemma() == lemma_info.m_Lemmas[i]) {
                foundLemma = true;
                break;
            }

        if (foundLemma)
            ws.AddWordInfo(lemma_info.m_Lemmas[i]);
        else {
        // otherwise use simple heuristic that a word before the main word is probably an adjective
        // after the main word - a noun.
        // TODO: check if this heuristic work in non-russian languges
            if ((int)i < lemma_info.m_iMainWord)
                ws.AddWordInfo(SelectAdjectiveHom(w).GetLemma());
            else
                ws.AddWordInfo(SelectNounHom(w).GetLemma());
        }
    }

    return true;
}

Wtroka TSequenceInflector::GetOriginalText(size_t wordIndex) const {
    YASSERT(GroupHasWord(Group, wordIndex));

    const TItem& item = Items[wordIndex];
    // still enforce unary grammar agreements!
    TGramBitSet enforced = (item.Homonym->Grammems.All() & item.Keep) | item.Enforce;

    Wtroka restext;
    TGramBitSet resgram;
    if (InflectWord(wordIndex, enforced, false, restext, resgram)) {
        if (item.UseQuotes)
            CopyQuotes(restext, *Items[wordIndex].Word);
        return restext;
    }

    const CWordsPair& p = TomitaWords[wordIndex].GetSourcePair();
    YASSERT(p.Size() > 0);
    const CWord& first = Words[p.FirstWord()];
    restext = first.GetOriginalText();      // TODO: use CWord::GetText()
    if (first.IsFirstInSentence() && !first.HasAtLeastTwoUpper())
        TMorph::ToLower(restext); // Remove title-case for the first word of a sentence (not having other upper-cased letters)
    for (int i = p.FirstWord() + 1; i <= p.LastWord(); ++i)
        restext.append(' ').append(Words[i].GetOriginalText());

    return restext;
}

void TSequenceInflector::Inflect(const TGramBitSet& grammems) {
    if (Group == NULL || WordSequence.HasLemmas())
        return;

    InflectItems(grammems);

    // apply Items state to WordSequence
    WordSequence.ClearWordsInfos();
    WordSequence.ClearLemmas();

    int lastWord = Group->LastWord();
    if (TomitaWords[lastWord].IsEndOfStreamWord())
        --lastWord;

    //adding special labels
    if (!Items[MainWordIndex].Homonym->IsMorphNoun())
        WordSequence.SetAdj();

    yset<Wtroka> tmpwords;

    for (int i = Group->FirstWord(); i <= lastWord; ++i) {
        const CWord& w = *Items[i].Word;
        const CHomonym& h = *Items[i].Homonym;
        const CWordsPair& span = w.GetSourcePair();

        if (Items[i].Excluded)
            continue;

        Wtroka s = Items[i].ResultText;
        if (s.empty())
            s = GetOriginalText(i);
        WordSequence.AddLemma(SWordSequenceLemma(s));

        bool isMain = (i == (int)MainWordIndex || Items[i].AgreedNoun);
        // if there is dictionary lemma then we should append distinct words contained in Lemma property
        if (HasGztLemma(h) && AddCommaSeparatedWordInfo(h, isMain, WordSequence))
            continue;

        if (Items[i].ResultText.empty() && span.Size() == 1) {
            // retrieve lemma for words which has not been made agree
            WordSequence.AddWordInfo(h.GetLemma(), isMain);
            continue;
        }

        CWordSequence* pSourceWS = h.GetSourceWordSequence();
        if (pSourceWS == NULL) {
             WordSequence.AddWordInfo(s, isMain);
             continue;
        }

        CFioWordSequence* pFioWs = dynamic_cast<CFioWordSequence*>(pSourceWS);
        if (pFioWs != NULL) {
            WordSequence.AddWordInfoIfNonEmpty(pFioWs->GetFio().m_strSurname, isMain);
            WordSequence.AddWordInfoIfNonEmpty(pFioWs->GetFio().m_strName, isMain);
            WordSequence.AddWordInfoIfNonEmpty(pFioWs->GetFio().m_strPatronomyc);
        } else {
            CTextWS* pSourceTextWS = dynamic_cast<CTextWS*>(pSourceWS);
            if (pSourceTextWS)
                WordSequence.AddWordInfo(*pSourceTextWS, isMain);
            else
                for (int k = pSourceWS->FirstWord(); k <= pSourceWS->LastWord(); ++k) {
                    tmpwords.clear();
                    const CWord& ww = Words[k];
                    for (CWord::SHomIt h_it = ww.IterHomonyms(); h_it.Ok(); ++h_it)
                        tmpwords.insert(h_it->GetLemma());
                    for (yset<Wtroka>::const_iterator it = tmpwords.begin(); it != tmpwords.end(); ++it)
                        WordSequence.AddWordInfo(*it, isMain);
                }
        }
    }
}

inline TGramBitSet TSequenceInflector::SelectNormGrammems() const {
    // use infinitive as normal form for verbs
    if (Group != NULL && !WordSequence.HasLemmas()) {
        const CHomonym* hom = Items[MainWordIndex].Homonym;
        if (hom && hom->IsPersonalVerb())
            return TGramBitSet(gInfinitive);
    }
    // default form in other cases is nominative+singular
    return DefaultFeatures().BitSet(TFeature::Number, TFeature::Case)
         & DefaultFeatures().NormalMutableSet;
}

void TSequenceInflector::Normalize() {
    Inflect(SelectNormGrammems());
}

void TSequenceInflector::Normalize(const TGramBitSet& grammems) {
    TGramBitSet request = SelectNormGrammems();
    request = DefaultFeatures().ReplaceFeatures(request, grammems);
    Inflect(request);
}

void TSequenceInflector::ReplaceLemmaAlways() {
    bool prev = RestrictGztLemma;
    RestrictGztLemma = true;
    CollectExcludedWords(*Group);
    for (int i = Group->FirstWord(); i <= Group->LastWord(); ++i) {
        if (Items[i].Excluded)
            continue;
        WordSequence.AddLemma(SWordSequenceLemma(GetOriginalText(i)));
        WordSequence.AddWordInfo(Items[i].Homonym->GetLemma());
    }
    RestrictGztLemma = prev;
}
