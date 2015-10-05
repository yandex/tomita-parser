#include "homonym.h"
#include <FactExtract/Parser/afdocparser/common/wordsequence.h>
#include <util/charset/recyr.hh>


CHomonym::CHomonym(docLanguage lang, const Wtroka& sLemma)
    : CHomonymBase(lang, sLemma)
    , m_NameType()
    , m_bDeleted(false)
    , m_pAnalyticMainVerbHom(NULL)
    , m_pSourceWordSequence(NULL)
{
    for (int i = 0; i < DICTYPE_COUNT; ++i)
        m_DicIndexes[i] = -1;
}

bool CHomonym::HasArticle(const SArtPointer& artP, EDicType dic) const
{
    return GlobalGramInfo->HasArticle(*this, artP, dic);
}

static bool GetSimilarSubjectForms(const CHomonym& h1, const CHomonym& h2, TGrammarBunch& res)
{
    for (CHomonym::TFormIter it1 = h1.Grammems.IterForms(); it1.Ok(); ++it1)
    for (CHomonym::TFormIter it2 = h2.Grammems.IterForms(); it2.Ok(); ++it2) {
        TGramBitSet common_grammems = NGleiche::GleicheGrammems(*it1, *it2, NGleiche::GenderNumberCaseCheck);
        if (common_grammems.none())
            continue;
        if (!common_grammems.HasAny(NSpike::AllGenders))
            //consider words like "devushka-operator" or "derevo-bereza" (not having common gender) to be agreed by both used genders
            common_grammems |= (*it1 | *it2) & NSpike::AllGenders;
        //insert into result corrected grammems of @this
        static const TGramBitSet no_gnc = ~(NSpike::AllCases | NSpike::AllNumbers | NSpike::AllGenders);
        res.insert((*it1 & no_gnc) | common_grammems);
    }
    return !res.empty();
}

void CHomonym::ResetGenderFromHomonym(const CHomonym& h)
{
    if (h.HasGrammem(gFeminine))
        Grammems.Replace(gMasculine, gFeminine);
    if (h.HasGrammem(gMasculine))
        Grammems.Replace(gFeminine, gMasculine);
}

void CHomonym::AddLemmaPrefix(const Wtroka& sPrefix)
{
    //append sPrefix to m_sLemma with checking if both have same POS and agreed. If they are - append normalized sPrefix.

    THomonymVector prefix_homonyms;
    TMorph::GetDictHomonyms(sPrefix, prefix_homonyms);

    if (prefix_homonyms.empty()) {
        Grammems.Delete(gFirstName);
        SetLemma(sPrefix + Lemma);
        return;
    }

    TGramBitSet lemma_grammems = TMorph::GetLemmaGrammems(Lemma, TGramBitSet::IntersectGrammems(Grammems.Forms()));

    const TGramBitSet pos = GetMinimalPOS();
    static const TGramBitSet accepted_pos(gSubstantive, gAdjective);
    if (!pos.HasAny(accepted_pos) || pos.Has(gShort)) {
        Grammems.Delete(gFirstName);
        SetLemma(sPrefix + Lemma);
        return;
    }

    //now select best homonym from prefix homonyms = having max number of flexes (forms) agreed with @this
    int best_homonym_index = -1;
    TGrammarBunch best_agreed_forms;
    size_t best_grammems_count = 0;
    int iFirstNameNominativeH = -1;
    for (size_t i = 0; i < prefix_homonyms.size(); ++i) {
        THomonymPtr prefix_homonym = prefix_homonyms[i];
        if (prefix_homonym->HasGrammem(gFirstName) &&  prefix_homonym->HasGrammem(gNominative))
            iFirstNameNominativeH = i;

        const TGramBitSet prefix_pos = prefix_homonym->GetMinimalPOS();
        TGrammarBunch agreed_forms;
        if (!prefix_pos.HasAny(accepted_pos) || prefix_pos.Has(gShort) ||
            !GetSimilarSubjectForms(*this, *prefix_homonym, agreed_forms) ||
            agreed_forms.size() < best_agreed_forms.size())
            continue;

        //total number of common grammems for two homonyms - used when current homonym has same number of agreed forms as best homonym
        size_t cur_grammems_count = (prefix_homonym->Grammems.All() & Grammems.All()).count();
        if (agreed_forms.size() > best_agreed_forms.size() || cur_grammems_count > best_grammems_count) {
            best_agreed_forms.swap(agreed_forms);
            best_homonym_index = i;
            best_grammems_count = cur_grammems_count;
        }
    }

    if (!best_agreed_forms.empty()) {
        Wtroka prefix_lemma;
        if (lemma_grammems.any())
            prefix_lemma = TMorph::GetClosestForm(prefix_homonyms[best_homonym_index]->Lemma, lemma_grammems);
        if (prefix_lemma.empty())
            SetLemma(prefix_homonyms[best_homonym_index]->Lemma + Lemma);
        else
            SetLemma(prefix_lemma + Lemma);
        SetGrammems(best_agreed_forms);
        if (!prefix_homonyms[best_homonym_index]->HasGrammem(gFirstName))
            Grammems.Delete(gFirstName);
        else
            ResetGenderFromHomonym(*prefix_homonyms[best_homonym_index]);
    } else {
        if (iFirstNameNominativeH == -1)
            Grammems.Delete(gFirstName);
        else
            ResetGenderFromHomonym(*prefix_homonyms[iFirstNameNominativeH]);
        SetLemma(sPrefix + Lemma);
    }
}

CWordSequence* CHomonym::GetSourceWordSequence() const
{
    return m_pSourceWordSequence;
}

void CHomonym::SetSourceWordSequence(CWordSequence* pSourceWordSequence)
{
    m_pSourceWordSequence = pSourceWordSequence;
}

void CHomonym::Delete()
{
    m_bDeleted= true;
}

void CHomonym::Undelete()
{
    m_bDeleted = false;
}

bool CHomonym::IsDeleted() const
{
    return m_bDeleted;
}

void CHomonym::PutAuxArticle(const SDictIndex& DictIndex)
{
    //YASSERT(DictIndex.m_DicType != GZT_DICT);
    m_DicIndexes[DictIndex.m_DicType] = DictIndex.m_iArt;

    // do not allow mixing AUX and GZT articles in same homonym
    if (DictIndex.m_DicType == KW_DICT)
        m_GztArticle = TGztArticle();
}

void CHomonym::PutGztArticle(const TGztArticle& gzt_article)
{
    m_GztArticle = gzt_article;

    // do not allow mixing AUX and GZT articles in same homonym
    m_DicIndexes[KW_DICT] = -1;

    // do not mix several kwtypes in single homonym
    if (!m_ExtraGztArticles.empty() && m_GztArticle.GetType() != m_ExtraGztArticles.begin()->GetType())
        m_ExtraGztArticles.clear();
}

int CHomonym::GetAuxArticleIndex(EDicType dicType) const
{
    return m_DicIndexes[dicType];
};

const TGztArticle& CHomonym::GetGztArticle() const
{
    return m_GztArticle;
};

bool CHomonym::HasGztArticle() const
{
    return !m_GztArticle.Empty();
}

bool CHomonym::HasAuxArticle(EDicType dicType) const
{
    switch (dicType) {
        case CONJ_DICT:
        case PREP_DICT:
        case TEMPLATE_DICT:
        case KW_DICT:
            return m_DicIndexes[dicType] != -1;
        //так как по смыслу оба словаря описывают вершину клаузы, то каждый раз вызывать эту ф-цию 2 раза для
        //словаря предикатов и шаблонов геморройно, тогда сделаю так, чтобы по запросу на PREDIC_DICT, если нет
        //статьи из него то смотрелся TEMPLATE_DICT
        case PREDIC_DICT:
            return (m_DicIndexes[PREDIC_DICT] != -1) || (m_DicIndexes[TEMPLATE_DICT] != -1);
        default:
            return false;
    }
}

bool CHomonym::ContainTerminal(TerminalSymbolType term) const
{
    return m_AutomatSymbolInterpetationUnion.find(term) != m_AutomatSymbolInterpetationUnion.end();
}

bool CHomonym::InitNameType()
{
    if ((Grammems.All() & NSpike::AllNumbers) == TGramBitSet(gPlural))
        return false;

    if (HasGrammem(gSurname)) {
        m_NameType.Set(Surname);
        return true;
    } else if (HasGrammem(gPatr)) {
        m_NameType.Set(Patronomyc);
        return true;
    } else if (HasGrammem(gFirstName) && !HasGrammem(gPossessive)) {   //do not treat "Ivanov" as First-name, Nominative
        m_NameType.Set(FirstName);
        return true;
    } else
        return false;
}

void CHomonym::CopyTo(CHomonymBase* pHom) const
{
    CHomonymBase::CopyTo(pHom);
    CHomonym* ph = dynamic_cast<CHomonym*>(pHom);
    if (ph == NULL)
        return;

    CHomonym& h = *ph;
    h.m_NameType = m_NameType;
    h.m_pSourceWordSequence = m_pSourceWordSequence;
    h.m_Labels = m_Labels;
    h.m_AutomatSymbolInterpetationUnion = m_AutomatSymbolInterpetationUnion;
    memcpy(h.m_DicIndexes, m_DicIndexes, sizeof(int)*DICTYPE_COUNT);

    h.m_KWtype2Articles = m_KWtype2Articles;
    h.m_GztArticle = m_GztArticle;
    h.m_ExtraGztArticles = m_ExtraGztArticles;
}

THomonymPtr CHomonym::Clone() const
{
    THomonymPtr pRes = new CHomonym(Lang);
    CopyTo(pRes.Get());
    return pRes;
}

Wtroka CHomonym::GetLemma() const
{
    if (m_pSourceWordSequence && (m_pSourceWordSequence->Size() > 1)) {
        Wtroka s = m_pSourceWordSequence->GetLemma();
        if (!s.empty())
            return s;
    }
    return CHomonymBase::GetLemma();
}

Wtroka CHomonym::GetShortLemma() const
{
    Wtroka s = GetLemma();
    if (s.size() > 30) {
        s = s.substr(0, 30);
        NStr::Append(s , "...");
    }
    return s;
}

void CHomonym::AddLabel(const Wtroka& strLabel)
{
    m_Labels.insert(strLabel);
}

bool CHomonym::HasLabel(const Wtroka& strLabel) const
{
    return m_Labels.find(strLabel) != m_Labels.end();
}

Stroka CHomonym::GetStrKWType(ECharset encoding) const
{
    return GlobalGramInfo->GetStrKWType(*this, encoding);
}

Stroka CHomonym::GetLabelsString(ECharset encoding) const
{
    Stroka s;
    if (m_Labels.size() == 0)
        return s;
    else
        s = " labels = (";
    yset<Wtroka>::const_iterator it = m_Labels.begin();
    for (; it != m_Labels.end(); ++it)
        s += WideToChar(*it, encoding);

    s += ")";
    return s;
}

void CHomonym::PrintGrammems(const TGramBitSet& grammems, TOutputStream& stream, ECharset encoding)
{
    Stroka grStr = grammems.ToString(", ");
    if (encoding != CODES_WIN)
        grStr = Recode(encoding, CODES_WIN, grStr);
    stream << grStr;
}

void CHomonym::PrintFormGrammems(TOutputStream& stream, ECharset encoding) const
{
    //extract all common grammems of forms ( ~ lemma's grammems)
    if (Grammems.HasForms()) {
        TFormIter it = Grammems.IterForms();
        PrintGrammems(*it,  stream, encoding);
        for (++it; it.Ok(); ++it) {
            stream << " | ";
            PrintGrammems(*it, stream, encoding);
        }
    } else
        PrintGrammems(Grammems.All(), stream, encoding);

}

void CHomonym::Print(TOutputStream& stream, const Stroka& strKwType, ECharset encoding) const
{
    Stroka s;
    if (strKwType.size())
        s = Substitute(" (<b>$0</b>) ", strKwType);

    stream << "  " << WideToChar(GetShortLemma(), encoding) << " ";
    PrintFormGrammems(stream, encoding);
    stream << s << GetLabelsString(encoding) << Endl;
}

void CHomonym::Print(TOutputStream& stream, ECharset encoding) const
{
    Stroka s;
    s = GetStrKWType(encoding);
    if (s.size())
        s = Substitute(" (<b>$0</b>) ", s);

    if (IsDeleted())
        s += "[deleted]";
    stream << WideToChar(GetShortLemma(), encoding) << " ";
    PrintGrammems(Grammems.All(), stream, encoding);
    stream << s << GetLabelsString(encoding);
}

bool CHomonym::HasTerminalSymbol(int iTerminal) const
{
    return m_AutomatSymbolInterpetationUnion.find((size_t)iTerminal) != m_AutomatSymbolInterpetationUnion.end();
}

bool CHomonym::HasArtificialLemma() const
{
    return m_pSourceWordSequence != NULL && m_pSourceWordSequence->HasLemmas();
}

