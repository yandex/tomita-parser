#include "word.h"
#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/afdocparser/common/wordsequence.h>
#include <FactExtract/Parser/surname_predictor/surname_predictor.h>
#include <FactExtract/Parser/afdocparser/rus/morph.h>


CWord::SHomIt::SHomIt(const THomonymVector& homonyms)
    : Current(homonyms.begin())
    , Begin(homonyms.begin())
    , End(homonyms.end())
{
    if (Ok() && GetPtr()->IsDeleted())
        operator++();
}

void CWord::SHomIt::operator++()
{
    YASSERT(Ok());
    ++Current;
    while (Ok() && GetPtr()->IsDeleted())
        ++Current;
}

CWord::CWord(const CPrimGroup &prim,const Wtroka& strWord, bool ignoreUpperCase)
    : CWordBase(TMorph::GetMainLanguage(), prim, strWord)
    , m_Homonyms()
    , m_DeletedHomCount(0)
    , m_SourceWords()
    , m_bIgnoreUpperCase(ignoreUpperCase)
{
    /*if (TMorph::ReplaceYo(m_txt) > 0 && !IsLowerTextSameAsOriginal())
        TMorph::ReplaceYo(m_lowercase_txt);*/
}

CWord::CWord(const Wtroka& strWord, bool ignoreUpperCase)
    : CWordBase(TMorph::GetMainLanguage(), strWord)
    , m_Homonyms()
    , m_DeletedHomCount(0)
    , m_SourceWords()
    , m_bIgnoreUpperCase(ignoreUpperCase)
{

}

bool CWord::HasHomonym(const Wtroka& str) const
{
    return GetHomonymIndex(str) != -1;
}

int CWord::GetHomonymIndex(const Wtroka& str) const
{
    for (size_t i = 0; i < m_Homonyms.size(); ++i)
        if (m_Homonyms[i]->GetLemma() == str)
            return i;
    return -1;
}

void CWord::AddTerminalSymbolToAllHomonyms(TerminalSymbolType iS)
{
    for (size_t i = 0; i < m_Homonyms.size(); ++i)
        m_Homonyms[i]->AddTerminalSymbol(iS);
}

void CWord::DeleteAllTerminalSymbols()
{
    m_AutomatSymbolInterpetationUnion.clear();
    for (size_t i = 0; i < m_Homonyms.size(); ++i)
        m_Homonyms[i]->m_AutomatSymbolInterpetationUnion.clear();
}

void CWord::SetSourcePair(int iW1, int iW2)
{
    m_SourceWords.SetPair(iW1, iW2);
}

void CWord::AddFio(yset<Wtroka>& fioStrings, bool bIndexed)
{
    yset<Wtroka>::iterator it = fioStrings.begin();
    for (; it != fioStrings.end(); it++) {
        CHomonym* pNewHom = new CHomonym(TMorph::GetMainLanguage(), *it);
        if (*it == g_strFIONonTerminal)
            pNewHom->SetIsDictionary(true);
        else
            pNewHom->SetIsDictionary(bIndexed);
        m_variant.push_back(pNewHom);
    }
}

bool CWord::IsEndOfStreamWord() const
{
    return m_typ == UnknownPrim && NStr::IsEqual(m_txt, "EOS");
}

void CWord::UniteHomonymsTerminalSymbols()
{
    m_AutomatSymbolInterpetationUnion.clear();
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it) {
        set_union(it->m_AutomatSymbolInterpetationUnion.begin(),
                  it->m_AutomatSymbolInterpetationUnion.end(),
                  m_AutomatSymbolInterpetationUnion.begin(),
                  m_AutomatSymbolInterpetationUnion.end(),
                  inserter(m_AutomatSymbolInterpetationUnion, m_AutomatSymbolInterpetationUnion.begin()));
    }
}

bool CWord::HasArticle(yset<TKeyWordType>& kw_types) const
{
    return GlobalGramInfo->HasArticle(*this, kw_types);
}

int CWord::HasArticle_i(const SArtPointer& artP) const
{
    return GlobalGramInfo->HasArticle_i(*this, artP);
}

bool CWord::HasArticle(const SArtPointer& artP) const
{
    return HasArticle_i(artP) != -1;
}

void CWord::GetHomIDsByKWType(const SArtPointer& artP, ui32& homIDs) const
{
    HasArticle(artP, homIDs);
}

bool CWord::HasArticle(const SArtPointer& artP, ui32& homIDs) const
{
    bool bFound = false;
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (GlobalGramInfo->HasArticle(*it, artP, KW_DICT)) {
            homIDs |= 1 << it.GetID();
            bFound = true;
        }
    return bFound;
}

bool CWord::HasAuxArticle(EDicType dic) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasAuxArticle(dic))
            return true;
    return false;
}

bool CWord::HasGztArticle() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasGztArticle())
            return true;
    return false;
}

int CWord::IsName_i(ENameType type, int iStartHom) const
{
    for (size_t i = iStartHom; i < m_Homonyms.size(); ++i)
        if (!m_Homonyms[i]->IsDeleted() && m_Homonyms[i]->HasNameType(type))
            return i;
    return -1;
}

int CWord::HasPOSi(TGrammar POS) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasPOS(POS))
            return it.GetID();
    return -1;
}

int CWord::HasPOSi(const TGramBitSet& POS) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasPOS(POS))
            return it.GetID();
    return -1;
}

bool CWord::HasPOS(TGrammar POS) const
{
    return HasPOSi(POS) != -1;
}

bool CWord::HasPOS(const TGramBitSet& POS) const
{
    return HasPOSi(POS) != -1;
}

bool CWord::HasAnyOfPOS(const TGrammarBunch& POSes) const      // renamed from ambiguous "HasPOSes"
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasAnyOfPOS(POSes))
            return true;
    return false;
}

bool CWord::HasAnyOfPOS(const TGramBitSet& POSes) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasAnyOfPOS(POSes))
            return true;
    return false;
}

bool CWord::HasUnknownPOS() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasUnknownPOS())
            return true;
    return false;
}

int CWord::HasMorphNoun_i() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsMorphNoun())
            return it.GetID();
    return -1;
}

bool CWord::HasMorphNoun() const
{
    return HasMorphNoun_i() != -1;
}

bool CWord::HasMorphNounWithGrammems(const TGramBitSet& grammems) const
{
    return HasMorphNounWithGrammems_i(grammems) != -1;
}

int CWord::HasMorphNounWithGrammems_i(const TGramBitSet& grammems) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsMorphNoun() && it->Grammems.HasAll(grammems))
            return it.GetID();
    return -1;
}

bool CWord::HasHomonymWithGrammems(const TGramBitSet& grammems) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->Grammems.HasAll(grammems))
            return true;
    return false;
}

bool CWord::HasMorphAdj() const
{
    return HasMorphAdj_i() != -1;
}

int CWord::HasMorphAdj_i() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsMorphAdj())
            return it.GetID();
    return -1;
}

int CWord::HasPOSi(TGrammar POS, const Wtroka& strLemma) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->HasPOS(POS) && strLemma == it->Lemma)
            return it.GetID();
    return -1;
}

int CWord::HasConjHomonym_i() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsConj())
            return it.GetID();
    return -1;
}

bool CWord::HasConjHomonym() const
{
    return HasConjHomonym_i() != -1;
}

bool CWord::HasOneConjHomonym() const
{
    bool found = false;
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsConj()) {
            if (found)
                return false;
            else
                found = true;
        }
    return found;
}

int CWord::HasSimConjHomonym_i() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsSimConj())
            return it.GetID();
    return -1;
}

bool CWord::HasSimConjHomonym() const
{
    return HasSimConjHomonym_i() != -1;
}
/*
bool    CWord::HasSimConjNotWithCommaObligatory() const
{
    return GlobalGramInfo->HasSimConjNotWithCommaObligatory(*this);
}
*/
bool CWord::HasSubConjHomonym() const
{
    return (HasSubConjHomonym_i() != -1);
}

int CWord::HasSubConjHomonym_i() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsSubConj())
            return it.GetID();
    return -1;
}

bool CWord::HasOnlyPOSes(const TGramBitSet& POSes) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (!it->HasAnyOfPOS(POSes))
            return false;
    return true;
}

bool CWord::HasOnlyPOS(const TGramBitSet& POS) const
{
    return GetRusHomonymsCount() == 1 && HasPOS(POS);
}

bool CWord::HasOnlyPOS(TGrammar POS) const
{
    return GetRusHomonymsCount() == 1 && HasPOS(POS);
}

bool CWord::HasOnlyUnknownPOS() const
{
    return GetRusHomonymsCount() == 1 && HasUnknownPOS();
}

void CWord::DeleteHomonymWithPOS(const TGramBitSet& POS)
{
    if (m_Homonyms.size() == 1)
        return;
    yvector<THomonymPtr> del_homs;
    for (size_t i = 0; i < m_Homonyms.size(); ++i)
        if (!m_Homonyms[i]->IsDeleted() && m_Homonyms[i]->HasPOS(POS)) {
            DeleteHomonym(*m_Homonyms[i]);
            del_homs.push_back(m_Homonyms[i]);
        }

    //if all homonyms are removed accidentally - roll back the operation
    if (GetRusHomonymsCount() == 0)
        for (size_t i = 0; i < del_homs.size(); ++i)
            UndeleteHomonym(*del_homs[i]);
}

void CWord::DeleteHomonym(CHomonym& hom)
{
    if (!hom.IsDeleted()) {
        ++m_DeletedHomCount;
        hom.Delete();
    }
}

void CWord::UndeleteHomonym(CHomonym& hom)
{
    if (hom.IsDeleted()) {
        if (m_DeletedHomCount > 0)
            --m_DeletedHomCount;
        hom.Undelete();
    }
}

void CWord::DeleteHomonym(size_t i)
{
    if (GetRusHomonymsCount() > 1)
        DeleteHomonym(*m_Homonyms[i]);
}

void CWord::DeleteAllHomonyms()
{
    for (size_t i = 0; i < m_Homonyms.size(); ++i)
        DeleteHomonym(*m_Homonyms[i]);
}

bool CWord::HasDeletedHomonyms() const
{
    return m_DeletedHomCount > 0;
}

const CWordsPair& CWord::GetSourcePair() const
{
    return m_SourceWords;
}

THomonymPtr CWord::PredictAsPatronymic()
{
    THomonymBaseVector out;
    if (TMorph::PredictHomonyms(m_txt, out))
        for (size_t i = 0; i < out.size(); ++i) {
            THomonymPtr pH = dynamic_cast<CHomonym*>(out[i].Get());
            if (pH.Get() != NULL && pH->HasGrammem(gPatr))
                return pH;
        }
    return NULL;
}

//if we already tried to predict then return what we predicted
bool CWord::PredictAsSurname()
{
    int iH = HasMorphNounWithGrammems_i(TGramBitSet(gSurname));
    if (iH != -1)
        return !GetRusHomonym(iH).IsDictionary();

    yvector<TSurnamePredictor::TPredictedSurname> out;
    if (!TMorph::PredictSurname(GetLowerText(), out))
        return false;

    THomonymPtr pFirstHomonym = m_Homonyms[IterHomonyms().GetID()];

    bool bFound = IsDictionary();

    for (size_t i = 0; i < out.size(); i++) {
        TGrammarBunch forms;
        ToGrammarBunch(out[i].StemGrammar, out[i].FlexGrammars, forms);
        Wtroka lemma = out[i].Lemma;
        if (i == 0 && !bFound)
            pFirstHomonym->Init(lemma, forms, pFirstHomonym->IsDictionary());
        else {
            THomonymPtr pClonedHomonym = pFirstHomonym->Clone();
            pClonedHomonym->Init(lemma, forms, false);
            AddHomonym(pClonedHomonym, false);
        }
    }
    return true;
}

void CWord::CreateHomonyms(bool bUsePrediction /*= true*/)
{
    THomonymBaseVector out;
    TMorph::GetHomonyms(GetLowerText(), out, bUsePrediction);
    for (size_t i = 0; i < out.size(); i++)
        AddHomonym(out[i], false);
    if (out.size() == 0)
        AddHomonym(THomonymPtr(new CHomonym(TMorph::GetMainLanguage(), GetLowerText())), false);
}

bool CWord::TryToPredictPatronymic()
{
    THomonymPtr pHom = PredictAsPatronymic();
    if (pHom.Get() != NULL) {
        pHom->SetNameType(Patronomyc);
        AddHomonym(pHom, false);
        return true;
    }
    return false;
}

bool CWord::IsDictionary() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsDictionary())
            return true;
    return false;
}

CHomonym& CWord::GetRusHomonym(size_t iH)
{
    return *m_Homonyms[iH];
}

const CHomonym& CWord::GetRusHomonym(size_t iH) const
{
    return *m_Homonyms[iH];
}

size_t CWord::GetRusHomonymsCount() const
{
    return m_Homonyms.size() - m_DeletedHomCount;
}

bool CWord::FindLemma(const Wtroka& strLemma) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (strLemma == it->GetLemma())
            return true;
    return false;
}

bool CWord::HasNameHom() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsName())
            return true;
    return false;
}
bool CWord::HasGeoHom() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->IsGeo())
            return true;
    return false;
}

bool CWord::HasAtLeastTwoUpper() const
{
    if (m_bUp)
        for (size_t i = 1; i < m_txt.size(); i++)
            //should return false for "Gorno-Altaysk" (uppercased letter after hypen is not taken into account)
            if (::IsUpper(m_txt[i]) && !::IsDash(m_txt[i - 1]))
                return true;
    return false;
}

bool CWord::IsUpperLetterPointInitial() const
{
    return m_txt.size() == 2 && m_bUp && m_txt[1] == '.';
}

bool CWord::IsOriginalWord() const
{
    return !IsMultiWord();
}

bool CWord::IsNameFoundInDic() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->m_NameType.any())
            return true;
    return false;
}

bool CWord::IsName(ENameType type) const
{
    return IsName_i(type) != -1;
}

bool CWord::IsAndConj() const
{
    return HasHomonym(RUS_WORD_I);
}

THomonymBasePtr CWord::GetHomonym(size_t iHom) const
{
    return m_Homonyms[iHom].Get();
}

bool CWord::RightPartIsSurname(int& iH, THomonymGrammems& grammems, Wtroka& strLemma)
{
    iH = HasMorphNounWithGrammems_i(TGramBitSet(gSurname));
    if (iH != -1) {
        CHomonym& h = GetRusHomonym(iH);
        grammems = h.Grammems;
        strLemma = h.GetLemma();
        size_t ii = strLemma.find('-');
        YASSERT(ii != Wtroka::npos);
        strLemma = strLemma.substr(ii + 1);
        return true;
    }

    //if this word is in morphology - do not try to predict
    if (IsDictionary())
        return false;

    size_t ii = m_txt.find('-');
    if (ii == Wtroka::npos)
        return false;
    Wtroka strRightPart = m_txt.substr(ii + 1);
    TMorph::ToLower(strRightPart);

    yvector<TSurnamePredictor::TPredictedSurname> out;
    if (!TMorph::PredictSurname(strRightPart, out))
        return false;

    TGrammarBunch newForms;
    NSpike::ToGrammarBunch(out[0].StemGrammar, out[0].FlexGrammars, newForms);
    grammems.Reset(newForms);
    strLemma = out[0].Lemma;
    return true;
}

void CWord::AddHyphenSurnameLemma(int iH, const THomonymGrammems& forms, const Wtroka& strLemma)
{
    if (iH == -1) {
        if (m_Homonyms.size() == 1) {
            m_Homonyms[0]->SetLemma(strLemma);
            m_Homonyms[0]->SetGrammems(forms);
            m_Homonyms[0]->SetNameType(Surname);
            return;
        } else {
            CHomonym* pH = new CHomonym(TMorph::GetMainLanguage(), strLemma);
            pH->SetGrammems(forms);
            pH->SetNameType(Surname);
            AddRusHomonym(pH);
            return;
        }
    } else {
        CHomonym& h = GetRusHomonym(iH);
        h.SetLemma(strLemma);
        h.SetNameType(Surname);
    }
}

//Try to predict double-word surname if the last part is found in dictionary.
//Then check if the first part is found in dictionary as surname.
//If it is not then try predicting it.
bool CWord::PredictHyphenSurname()
{
    if (!((m_typ == Hyphen || m_typ == HypDiv || m_typ == DivWord) && m_variant.size() > 0))
        return false;

    size_t ii = m_txt.find('-');
    if (ii == Wtroka::npos)
        return false;
    //only one hyphen is allowed
    if (ii != m_txt.rfind('-'))
        return false;
    Wtroka strRightPart = m_txt.substr(ii + 1);
    if (strRightPart.size() < 1 || !::IsUpper(strRightPart[0]))
        return false;

    int iH = -1;
    THomonymGrammems rightPartGrammems;
    Wtroka strRightPartLemma;
    if (!RightPartIsSurname(iH, rightPartGrammems, strRightPartLemma))
        return false;

    ii = m_txt.find('-');       //unnecessary call?
    if (ii == Wtroka::npos)
        return false;
    Wtroka strFirstPart = m_txt.substr(0, ii);
    TMorph::ToLower(strFirstPart);

    //look in morphology
    THomonymVector res;
    TMorph::GetDictHomonyms(strFirstPart, res);
    bool found = false;
    for (size_t i = 0; i < res.size(); ++i) {
        if (!found && res[i]->HasGrammem(gSurname) &&
            NGleiche::Gleiche(res[i]->Grammems, rightPartGrammems, NGleiche::GenderNumberCaseCheck)) {
            found = true;
            Wtroka joined_lemma = res[i]->GetLemma() + '-' + strRightPartLemma;
            AddHyphenSurnameLemma(iH, rightPartGrammems, joined_lemma);
        }
    }

    if (found)
        return true;

    //if the word was in morphology then do not do any further predictions
    if (res.size() > 0)
        return false;

    yvector<TSurnamePredictor::TPredictedSurname> out;
    TMorph::PredictSurname(strFirstPart, out);
    if (out.size() > 0 && NGleiche::Gleiche(out[0].FlexGrammars, rightPartGrammems.Forms(), NGleiche::GenderNumberCaseCheck)) {
        Wtroka joined_lemma = out[0].Lemma + '-' + strRightPartLemma;
        AddHyphenSurnameLemma(iH, rightPartGrammems, joined_lemma);
        return true;
    }
    return false;
}

bool CWord::InitNameType()
{
    if (!m_bUp)
        return false;

    bool bRes = false;
    if ((m_typ == Hyphen || m_typ == HypDiv || m_typ == DivWord) && m_variant.size() > 0 && PredictHyphenSurname())
        return true;

    for (size_t i = 0; i < m_Homonyms.size(); ++i) {
        if (m_Homonyms[i]->IsDeleted())
            continue;

        if (m_Homonyms[i]->InitNameType())
            bRes = true;

        if (m_typ == Initial) {
            m_Homonyms[i]->SetNameType(InitialName);
            m_Homonyms[i]->SetNameType(InitialPatronomyc);
            bRes = true;
        }
    }
    return bRes;
}

int CWord::AddRusHomonym(THomonymPtr pHom)
{
    size_t newHomID = m_Homonyms.size();
    m_Homonyms.push_back(pHom);
    return newHomID;
}

int CWord::AddHomonym(THomonymPtr pHom, bool bAllowIgnore)
{
    if (bAllowIgnore && !CanAddHomonym(*pHom))
        return -1;
    else
        return AddRusHomonym(pHom);
}

void CWord::AddHomonym(THomonymBasePtr pHom, bool bAllowIgnore)
{
    YASSERT(dynamic_cast<CHomonym*>(pHom.Get()) != NULL);
    AddHomonym(THomonymPtr(static_cast<CHomonym*>(pHom.Get())), bAllowIgnore);
}

void CWord::AddPredictedHomonym(THomonymBasePtr pHom, bool bAllowIgnore)
{
    YASSERT(dynamic_cast<CHomonym*>(pHom.Get()) != NULL);
    THomonymPtr pHomRus = static_cast<CHomonym*>(pHom.Get());

    if (bAllowIgnore && !CanAddHomonym(*pHomRus))
        return;

    m_PredictedHomonyms.push_back(pHomRus);
}

bool CWord::CanAddHomonym(const CHomonym& h) const
{
    //make sure that abbreviations (e.g. PO, OAO, etc.) are uppercased
    if (h.HasGrammem(gAbbreviation) && !m_bIgnoreUpperCase) {
        if (!m_bUp)
            return false;
        for (size_t i = 0; i < m_txt.size(); ++i)
            if (!::IsUpper(m_txt[i]))
                return false;
    }
    if ((h.IsGeo() || h.IsName()) && h.IsMorphNoun() && !m_bUp  && !m_bIgnoreUpperCase)
        return false;

    return true;
}

int CWord::PutAuxArticle(int iH, const SDictIndex& dicIndex, bool bCloneAnyWay)
{
    CHomonym& h = GetRusHomonym(iH);
    bool has_gzt = dicIndex.m_DicType == KW_DICT && h.HasGztArticle();
    if (has_gzt || h.m_DicIndexes[dicIndex.m_DicType] != -1) {
        TKeyWordType kwType = GlobalGramInfo->GetAuxKWType(dicIndex);
        if (!bCloneAnyWay && !has_gzt && GlobalGramInfo->HasArticle(h, SArtPointer(kwType), KW_DICT)) {
            h.m_KWtype2Articles[kwType].push_back(dicIndex);
            return iH;
        } else {
            THomonymPtr pNewHom = h.Clone();
            pNewHom->PutAuxArticle(dicIndex);
            return AddRusHomonym(pNewHom);
        }
    } else {
        h.PutAuxArticle(dicIndex);
        return iH;
    }
}

int CWord::PutGztArticle(int iH, const TGztArticle& gzt_article, bool bClone)
{
    CHomonym& h = GetRusHomonym(iH);
    bool has_aux_kw = h.HasAuxArticle(KW_DICT);
    if (!has_aux_kw && !h.HasGztArticle()) {
        h.PutGztArticle(gzt_article);
        return iH;
    } else if (bClone || has_aux_kw || h.GetGztArticle().GetType() != gzt_article.GetType()) {
        THomonymPtr pNewHom = h.Clone();
        pNewHom->PutGztArticle(gzt_article);
        return AddRusHomonym(pNewHom);
    } else {
        YASSERT(h.HasGztArticle() && h.GetGztArticle().GetType() == gzt_article.GetType());
        h.m_ExtraGztArticles.insert(gzt_article);
        return iH;
    }
}

bool CWord::HasCommonHomonyms(const CWord& w) const
{
    if (!IsDictionary() || !w.IsDictionary()) {
        SHomIt it1 = IterHomonyms();
        SHomIt it2 = w.IterHomonyms();
        if (it1.Ok() && it2.Ok() && TMorph::IdentifSimilar(it1->Lemma, it2->Lemma))
            return true;
    }

    for (SHomIt it1 = IterHomonyms(); it1.Ok(); ++it1)
        for (SHomIt it2 = w.IterHomonyms(); it2.Ok(); ++it2)
            if (it1->Lemma == it2->Lemma)
                return true;

    return false;
}

bool CWord::LessBySourcePair(CWord*& pW) const
{
    return GetSourcePair() < pW->GetSourcePair();
}

bool CWord::IsLatComplexWordAsCompName() const
{
    if ((m_typ != Complex && m_typ != Hyphen && m_typ != AltInitial) || !m_bUp)
        return false;

    static const Wtroka allowed = CharToWide("-1234567890qwertyuiopasdfghjklzxcvbnm.\\/");

    const Wtroka& str = GetLowerText();
    if (str.find_first_not_of(allowed) != Wtroka::npos)
        return false;
    //allow only one slash
    if (str.find('\\') != str.rfind('\\') || str.find('/') != str.rfind('/'))
        return false;
    return true;
}

bool CWord::IsRusComplexWordAsCompName() const
{
    if (m_typ != Complex || !m_bUp)
        return false;

    DECLARE_STATIC_RUS_WORD(allowed, "-1234567890qwertyuiopasdfghjklzxcvbnmйцукенгшщзхъфывапролджэячсмитьбю.\\/");

    const Wtroka& str = GetLowerText();
    //retain cyrillic letters for stupid cases like yandex.py
    if (str.find_first_not_of(allowed) != Wtroka::npos)
        return false;
    //allow only one slash
    if (str.find('\\') != str.rfind('\\') || str.find('/') != str.rfind('/'))
        return false;
    return true;
}

bool CWord::IsComplexWordAsDate() const
//for treating dates like 11.06.07 as Words
{
    if (m_typ != Complex)
        return false;

    static const Wtroka allowed = CharToWide("-.\\/11234567890г");

    const Wtroka& str = GetText();
    return str.find_first_not_of(allowed) == Wtroka::npos;
}

bool CWord::IsComplexWordAsCompName() const
{
    return IsRusComplexWordAsCompName() || IsLatComplexWordAsCompName();    //repeats same actions twice, could be optimized
}

bool CWord::HasAnalyticVerbFormHomonym(TGrammar POS) const
{
    return HasAnalyticVerbFormHomonym_i(POS) != -1;
}

int CWord::HasAnalyticVerbFormHomonym_i(TGrammar POS) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (NULL != it->GetAnalyticMainVerbHom() && it->HasPOS(POS))
            return it.GetID();
    return -1;
}

bool CWord::IsAnalyticVerbForm() const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (NULL != it->GetAnalyticMainVerbHom())
            return true;
    return false;
}

int CWord::HasFirstHomonymByTerminal_i(TerminalSymbolType s) const
{
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it->ContainTerminal(s))
            return it.GetID();
    return -1;
}

int CWord::HasFirstHomonymByHomIDs_i(long homIDs) const
{
    int Result = -1;
    for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
        if (it.TestMask(homIDs)) {
            if (it->HasGrammem(gMasculine)) //return Musculine if found
                return it.GetID();
            else if (Result == -1)
                    Result = it.GetID();
        }

    // if there is no Musculine then return first found result
    return Result;
}

bool CWord::HasFirstHomonymByHomIDs(long homIDs) const
{
    return HasFirstHomonymByHomIDs_i(homIDs) != -1;
}

bool CWord::HasFirstHomonymByTerminal(TerminalSymbolType s) const
{
    return HasFirstHomonymByTerminal_i(s) != -1;
}

void CWord::ArchiveAutomatSymbolInterpetationUnions(yvector<TerminalSymbolType>& vecUnionsArchive)
{
    if (m_AutomatSymbolInterpetationUnion.size() == 0)
        return;
    vecUnionsArchive.push_back(m_AutomatSymbolInterpetationUnion.size());
    vecUnionsArchive.insert(vecUnionsArchive.end(), m_AutomatSymbolInterpetationUnion.begin(), m_AutomatSymbolInterpetationUnion.end());

    for (SHomIt it = IterHomonyms(); it.Ok(); ++it) {
        vecUnionsArchive.push_back(it->m_AutomatSymbolInterpetationUnion.size());
        vecUnionsArchive.insert(vecUnionsArchive.end(), it->m_AutomatSymbolInterpetationUnion.begin(), it->m_AutomatSymbolInterpetationUnion.end());
    }
}

