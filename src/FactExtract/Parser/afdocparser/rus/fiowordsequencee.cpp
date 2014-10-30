#include "fiowordsequence.h"
#include <util/generic/ptr.h>
#include <FactExtract/Parser/afdocparser/rus/homonym.h>


EFIOType SFullFIO::GetType() const
{
    bool bF = !m_strSurname.empty();
    bool bI = !m_strName.empty();
    bool bO = !m_strPatronomyc.empty();

    if (bF && bI && bO) {
        if (m_bInitialName)
            return FIinOinName;
        else if (m_bInitialPatronomyc)
                return FIOinName;
        return FIOname;
    }
    if (bF && bI) {
        if (m_bInitialName)
            return FIinName;
        return FIname;
    }
    if (bF)
        return Fname;
    if (bI && bO)
        return IOname;
    if (bI)
        return Iname;

    throw yexception() << "Bad FIO in \"SFullFIO::GetType()\"";
}

SFullFIO::SFullFIO()
:   m_Genders(),
    m_iLocalSuranamePardigmID(-1),

    m_bFoundSurname(false),
    m_bInitialPatronomyc(false),
    m_bInitialName(false),
    m_bSurnameLemma(false),
    m_bFoundPatronymic(true),

    m_strSurname(),
    m_strName(),
    m_strPatronomyc()
{
}

Wtroka SFullFIO::ToString() const
{
    wchar16 UNDERLINE = '_';

    Wtroka strRes;
    if (!m_strSurname.empty()) {
        strRes += m_strSurname;
        strRes += UNDERLINE;
    }
    if (!m_strName.empty()) {
        strRes += m_strName;
        strRes += UNDERLINE;
    }
    if (!m_strPatronomyc.empty()) {
        strRes += m_strPatronomyc;
        strRes += UNDERLINE;
    }
    return strRes;
}

Wtroka SFullFIO::ToStringForChecker() const
{
    const wchar16 TAB = '\t';
    const wchar16 NEWLINE = '\n';
    const wchar16 OPENBRACE = '{';
    const wchar16 CLOSEBRACE = '}';

    Wtroka strRes;
    strRes += TAB;
    strRes += OPENBRACE;
    strRes += NEWLINE;
    if (!m_strSurname.empty()) {
        strRes += TAB;
        strRes += TAB;
        strRes += m_strSurname;
        strRes += NEWLINE;
    }
    if (!m_strName.empty()) {
        strRes += TAB;
        strRes += TAB;
        strRes += m_strName;
        strRes += NEWLINE;
    }
    if (!m_strPatronomyc.empty()) {
        strRes += TAB;
        strRes += TAB;
        strRes += m_strPatronomyc;
        strRes += NEWLINE;
    }
    strRes += TAB;
    strRes += CLOSEBRACE;
    return strRes;
}

void SFullFIO::Reset()
{
    m_bInitialPatronomyc = false;
    m_bInitialName = false;
    m_bFoundSurname = false;

    m_bSurnameLemma = false;
    m_bFoundPatronymic = true;
    m_strName.clear();
    m_strPatronomyc.clear();
    m_strSurname.clear();
    m_Genders.Reset();
    m_iLocalSuranamePardigmID = -1;
}

SFullFIO::SFullFIO(const SFullFIO& fio)
:   m_Genders(fio.m_Genders),
    m_iLocalSuranamePardigmID(fio.m_iLocalSuranamePardigmID),

    m_bFoundSurname(fio.m_bFoundSurname),
    m_bInitialPatronomyc(fio.m_bInitialPatronomyc),
    m_bInitialName(fio.m_bInitialName),
    m_bSurnameLemma(fio.m_bSurnameLemma),
    m_bFoundPatronymic(fio.m_bFoundPatronymic),

    m_strSurname(fio.m_strSurname),
    m_strName(fio.m_strName),
    m_strPatronomyc(fio.m_strPatronomyc)
{
}

bool SFullFIO::operator<(const SFullFIO& FIO) const
{
    EFIOType type = GetType();
    YASSERT(type == FIO.GetType());
    switch (type) {
        case FIOname:
        case FIinOinName:
        case FIOinName:
            return order_by_fio(FIO, false);
        case FIname:
        case FIinName:
            return order_by_fi(FIO, false);
        case Fname:
            return order_by_f(FIO, false);
        case IOname:
            return order_by_io(FIO, false);
        case Iname:
            return order_by_i(FIO, false);
        default:
            YASSERT(false);
    }
    return false;
}

bool SFullFIO::Gleiche(SFullFIO& fio)
{
    if (fio.m_Genders.any() && m_Genders.any())
        return (m_Genders & fio.m_Genders & NSpike::AllGenders).any();
    return true;
}

//предсказывает фамилию для недефисного слова,
//а для дефисного сначала пытается с помощью морфологии предсказать женскую лемму,
//а если не получается, то предсказывает
Wtroka SFullFIO::PredictFemaleSurname() const
{
    size_t ii = m_strSurname.find('-');
    if (ii == Wtroka::npos) {
        yvector<TSurnamePredictor::TPredictedSurname> out;
        if (!TMorph::PredictSurname(m_strSurname, out))
            return Wtroka();
        //пройдемся по предсказанным вариантам и выберем только женский
        //например, из "Гайдая", женской может получиться только лемма "Гайдая", но не "Гайдай"
        for (size_t i = 0; i < out.size(); i++)
            if (TMorph::HasFormWithGrammems(TGramBitSet(gFeminine), out[i].FlexGrammars, out[i].StemGrammar))
                return out[i].Lemma;
    } else {
        Wtroka strLeftPart = m_strSurname.substr(0, ii);
        Wtroka strRightPart = m_strSurname.substr(ii + 1);

        Wtroka* parts[2] = {&strLeftPart, &strRightPart};
        for (int i = 0; i < 2; ++i) {
            bool bFoundInMorph;
            Wtroka& part = *(parts[i]);
            Wtroka res = GetFemaleSurnameFromMorph(part, bFoundInMorph);
            if (res.empty() && !bFoundInMorph) {
                yvector<TSurnamePredictor::TPredictedSurname> out;
                if (!TMorph::PredictSurname(part, out))
                    return Wtroka();
                part = out[0].Lemma;
            } else
                part = res;
        }

        if (strRightPart.empty() || strLeftPart.empty())
            return Wtroka();
        return strLeftPart + '-' + strRightPart;
    }
    return Wtroka();
}

Wtroka SFullFIO::PredicSurnameAsAdj(TGrammar gender) const
{
    TWLemmaArray lemmas;
    TMorph::FindAllLemmas(m_strSurname, lemmas);
    for (size_t i = 0; i < lemmas.size(); i++) {
        THomonymPtr hom = TMorph::YandexLemmaToHomonym(lemmas[i], false);
        Wtroka res;
        if (hom->Grammems.HasAll(TGramBitSet(gSingular, gender)) && hom->Grammems.HasAny(NSpike::AllCases) &&
            (hom->IsFullAdjective() || hom->HasGrammem(gAdjNumeral)))
            res = TMorph::GetFormWithGrammems(lemmas[i], TGramBitSet(gSingular, gNominative, gender));

        if (!res.empty()) {
            TMorph::ToLower(res);
            return res;
        }
    }
    return Wtroka();
}

Wtroka SFullFIO::GetFemaleSurname() const
{
    if (!m_bFoundSurname) {
        Wtroka s = PredictFemaleSurname();
        if (s.empty())
            s = PredicSurnameAsAdj(gFeminine);
        return s;
    } else {
        bool bFoundInMorph;
        Wtroka res = GetFemaleSurnameFromMorph(m_strSurname, bFoundInMorph);
        if (res.empty() && !bFoundInMorph)
            return PredictFemaleSurname();
        else
            return res;
    }
}

//пытаемся найти в морфологии омоним с фамилией, а у него форму 'жр,им,ед'
Wtroka SFullFIO::GetFemaleSurnameFromMorph(Wtroka strSurname, bool& bFoundInMorph) const
{
    static const TGramBitSet required_grammems(gSingular, gNominative, gFeminine);
    bFoundInMorph = false;

    TWLemmaArray lemmas;
    TMorph::FindAllLemmas(strSurname, lemmas);
    for (size_t i = 0; i < lemmas.size(); i++)
        if (!lemmas[i].IsBastard()) {
            THomonymPtr hom = TMorph::YandexLemmaToHomonym(lemmas[i], true);
            if (hom->HasGrammem(gSurname)) {
                Wtroka res = TMorph::GetFormWithGrammems(lemmas[i], required_grammems);
                if (!res.empty()) {
                    TMorph::ToLower(res);
                    return res;
                }
            }
        }
    return Wtroka();
}

void SFullFIO::BuildFIOStringsForSearsching(yset<Wtroka>& fioStrings,
                                            bool bFullName, bool bFullPatronymic, bool bAnyPatronymic) const
{
    EFIOType type = GetType();

    const wchar16 UNDERLINE = '_';

    switch (type) {
        case Iname:
            break;
        case Fname:
        {
            fioStrings.insert(GetFioFormPrefix() + GetSurnameMark() + m_strSurname);
            fioStrings.insert(m_strSurname);
            break;
        }
        case FIOname:
        {
            fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                              m_strPatronomyc + UNDERLINE +
                              m_strSurname);

            if (!bFullPatronymic && !bAnyPatronymic)
                fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                                  GetPatronomycStub() + UNDERLINE +
                                  m_strSurname);

            if (!m_bInitialPatronomyc && !m_bInitialName) {
                if (!bFullPatronymic)
                    fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                                      Wtroka(m_strPatronomyc[0])  + GetInitialPatronomycStub() + UNDERLINE +
                                      m_strSurname);
                if (!bFullName)
                    fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                                      Wtroka(m_strPatronomyc[0]) + GetInitialPatronomycStub() + UNDERLINE +
                                      m_strSurname);
            }
            if (m_bInitialPatronomyc && !m_bInitialName && !bFullName)
                fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                                  m_strPatronomyc + UNDERLINE +
                                  m_strSurname);
            break;
        }
        case FIname:
        {
            fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                              m_strSurname);

            if (!m_bInitialName && !bFullName)
                fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                                  m_strSurname);
            break;
        }
        case IOname:
        {
            fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                              m_strPatronomyc);
            break;
        }

        default:
            break;
    }
    return;
}

void SFullFIO::BuildFIOStringRepresentations(yset<Wtroka>& fioStrings) const
{
    if (GetType() == Fname) {
        fioStrings.insert(GetFioFormPrefix() + GetNameStub() + m_strSurname);
        fioStrings.insert(GetFioFormPrefix() + GetSurnameMark() +  m_strSurname);
        fioStrings.insert(GetFioLemmaPrefix() +  m_strSurname);
        fioStrings.insert(g_strFIONonTerminal);

        return;
    }
    Wtroka fio;
    bool bHasName = !m_strName.empty();
    bool bHasPatronomyc = !m_strPatronomyc.empty();
    bool bHasSurname = !m_strSurname.empty();

    const wchar16 UNDERLINE = '_';

    if (bHasName && bHasPatronomyc && bHasSurname) {
        Wtroka fio_form_name = GetFioFormPrefix() + m_strName + UNDERLINE;
        Wtroka fio_lemma_name = GetFioLemmaPrefix() + m_strName + UNDERLINE;
        Wtroka patr = m_strPatronomyc + UNDERLINE;

        fioStrings.insert(fio_form_name + patr + m_strSurname);
        fioStrings.insert(fio_lemma_name + patr + m_strSurname);
        fioStrings.insert(fio_form_name + m_strSurname);

        if (!m_bInitialName && !m_bInitialPatronomyc) {
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + UNDERLINE +
                              Wtroka(m_strPatronomyc[0]) + UNDERLINE +
                              m_strSurname);
            fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                              Wtroka(m_strPatronomyc[0]) + UNDERLINE +
                              m_strSurname);
        }

        if (!m_bInitialName)
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + UNDERLINE +
                              m_strSurname);

        if (m_bInitialName) {
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                              Wtroka(m_strPatronomyc[0]) + UNDERLINE +
                              m_strSurname);
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                              m_strSurname);
        }

        if (m_bInitialPatronomyc) {
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + UNDERLINE +
                              m_strPatronomyc[0] + GetInitialPatronomycStub() + UNDERLINE +
                              m_strSurname);
            fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                              m_strPatronomyc[0] + GetInitialPatronomycStub() + UNDERLINE +
                              m_strSurname);
        }

        if (m_bInitialName && m_bInitialPatronomyc)
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                              m_strPatronomyc[0] + GetInitialPatronomycStub() + UNDERLINE +
                              m_strSurname);

        fioStrings.insert(GetFioFormPrefix() + GetSurnameMark() + m_strSurname);
        fioStrings.insert(g_strFIONonTerminal);
        return;
    }

    if (bHasName && bHasSurname) {
        fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                          m_strSurname);
        fioStrings.insert(GetFioLemmaPrefix() + m_strName + UNDERLINE +
                          m_strSurname);

        fioStrings.insert(GetFioFormPrefix() + m_strName + UNDERLINE +
                          GetPatronomycStub() + UNDERLINE +
                          m_strSurname);

        if (!m_bInitialName) {
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + UNDERLINE +
                              GetPatronomycStub() + UNDERLINE +
                              m_strSurname);
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + UNDERLINE +
                              m_strSurname);
        } else {
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                              m_strSurname);
            fioStrings.insert(GetFioFormPrefix() + Wtroka(m_strName[0]) + GetInitialNameStub() + UNDERLINE +
                              GetPatronomycStub() + UNDERLINE +
                              m_strSurname);
        }
        fioStrings.insert(GetFioFormPrefix() + GetSurnameMark() +  m_strSurname);
    }

    fioStrings.insert(g_strFIONonTerminal);
}

/*
Wtroka SFullFIO::BuildFIOString() const
{
    Wtroka ret;
    if (!m_strSurname.empty())
    {
        ret += m_strSurname;
        ret += "[f]";
    }
    if( !m_strName.empty() )
    {
        ret += UNDERLINE + m_strName;
        ret += "[i]";
    }
    if( !m_strPatronomyc.empty() )
    {
        ret += UNDERLINE + m_strPatronomyc;
        ret += "[o]";
    }

    return ret;
}
*/
bool SFullFIO::HasInitialName() const
{
    return m_bInitialName;
}

bool SFullFIO::HasInitialPatronomyc() const
{
    return m_bInitialPatronomyc;
}

bool SFullFIO::HasInitials() const
{
    return m_bInitialName || m_bInitialPatronomyc;
}

void SFullFIO::MakeLower()
{
    TMorph::ToLower(m_strSurname);
    TMorph::ToLower(m_strName);
    TMorph::ToLower(m_strPatronomyc);
}

void SFullFIO::MakeUpper()
{
    TMorph::ToUpper(m_strSurname);
    TMorph::ToUpper(m_strName);
    TMorph::ToUpper(m_strPatronomyc);

}

bool SFullFIO::IsEmpty() const
{
    return (m_strSurname.size() == 0) &&  (m_strName.size() == 0) && (m_strPatronomyc.size() == 0);
}

bool SFullFIO::equal_by_f(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    return !order_by_f(FIO2, bUseInitials) && !FIO2.order_by_f(*this, bUseInitials);
}

bool SFullFIO::equal_by_i(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    return !order_by_i(FIO2, bUseInitials) && !FIO2.order_by_i(*this, bUseInitials);
}

bool SFullFIO::equal_by_o(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    return !order_by_o(FIO2, bUseInitials) && !FIO2.order_by_o(*this, bUseInitials);
}

bool SFullFIO::order_by_f(const SFullFIO& FIO2, bool /*bUseInitials = true*/) const
{

    //если еще не присвоили m_iLocalSuranamePardigmID, то сравниваем просто лексикографически
    if ((m_iLocalSuranamePardigmID == -1) || (FIO2.m_iLocalSuranamePardigmID == -1))
        return m_strSurname < FIO2.m_strSurname;
    else
        return m_iLocalSuranamePardigmID < FIO2.m_iLocalSuranamePardigmID;
}

static inline bool OneIsPrefix(const Wtroka& s1, const Wtroka& s2)
{
    size_t len = Min<size_t>(+s1, +s2);
    return TWtringBuf(~s1, len) == TWtringBuf(~s2, len);
}

static inline bool InitialLess(const Wtroka& s1, const Wtroka& s2)
{
    if (s1.empty())
        return true;
    else if (s2.empty())
        return false;
    else
        return s1[0] < s2[0];
}

bool SFullFIO::order_by_i(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    if (bUseInitials && (m_bInitialName || FIO2.m_bInitialName) && OneIsPrefix(m_strName, FIO2.m_strName))
        return false;
    else
        return m_strName < FIO2.m_strName;
}

bool SFullFIO::order_by_iIn(const SFullFIO& FIO2) const
{
    return InitialLess(m_strName, FIO2.m_strName);
}

bool SFullFIO::default_order(const SFullFIO& cluster2) const
{
    return order_by_fio(cluster2, false);
}

bool SFullFIO::order_by_o(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    if (bUseInitials && (m_bInitialPatronomyc || FIO2.m_bInitialPatronomyc) && OneIsPrefix(m_strPatronomyc, FIO2.m_strPatronomyc))
        return false;
    else
        return m_strPatronomyc < FIO2.m_strPatronomyc;
}

bool SFullFIO::order_by_oIn(const SFullFIO& FIO2) const
{
    return InitialLess(m_strPatronomyc, FIO2.m_strPatronomyc);
}

bool SFullFIO::order_by_fio(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    bool bLess = order_by_f(FIO2);
    if (bLess || FIO2.order_by_f(*this))
        return bLess;
    return order_by_io(FIO2, bUseInitials);
}

bool SFullFIO::order_by_fiInoIn(const SFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    bool bLess = order_by_f(FIO2);
    if (bLess || FIO2.order_by_f(*this))
        return bLess;

    if (order_by_iIn(FIO2))
        return true;

    if (FIO2.order_by_iIn(*this))
        return false;

    if (order_by_oIn(FIO2))
        return true;

    return false;
}

bool SFullFIO::order_by_fioIn(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    bool bLess = order_by_f(FIO2);
    if (bLess || FIO2.order_by_f(*this))
        return bLess;

    if (order_by_i(FIO2, bUseInitials))
        return true;

    if (order_by_oIn(FIO2))
        return true;

    return false;
}

bool SFullFIO::order_by_io(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    if (m_strName != FIO2.m_strName) {
        if (bUseInitials && (m_bInitialName || FIO2.m_bInitialName) && OneIsPrefix(m_strName, FIO2.m_strName))
            return order_by_o(FIO2, bUseInitials);
        else
            return order_by_i(FIO2, bUseInitials);
    }
    return order_by_o(FIO2, bUseInitials);
}

bool SFullFIO::order_by_fi(const SFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    bool bLess = order_by_f(FIO2);
    if (bLess || FIO2.order_by_f(*this))
        return bLess;
    else
        return order_by_i(FIO2, bUseInitials);
}

bool SFullFIO::order_by_fiIn(const SFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    bool bLess = order_by_f(FIO2);
    if (bLess || FIO2.order_by_f(*this))
        return bLess;
    else
        return order_by_iIn(FIO2);
}

bool SFullFIO::operator==(const SFullFIO& FIO) const
{
    bool bEqualSurnames = !order_by_f(FIO) && !FIO.order_by_f(*this);
    return  bEqualSurnames &&
            m_strName == FIO.m_strName &&
            m_strPatronomyc == FIO.m_strPatronomyc &&
            m_bFoundPatronymic == FIO.m_bFoundPatronymic;
}

Wtroka CFioWordSequence::GetCapitalizedLemma() const
{
    return GetLemma();
}

Wtroka CFioWordSequence::GetLemma() const
{
    Wtroka strRes = m_Fio.m_strSurname;
    if (!strRes.empty() && !m_Fio.m_strName.empty())
        strRes += ' ';
    strRes += m_Fio.m_strName;
    if (!strRes.empty() && !m_Fio.m_strPatronomyc.empty())
        strRes += ' ';
    strRes += m_Fio.m_strPatronomyc;

    return StripString(strRes);
}

