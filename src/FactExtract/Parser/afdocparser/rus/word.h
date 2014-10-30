#pragma once

#include <util/generic/stroka.h>
#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/wordbase.h>
#include "homonym.h"
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/common/inputitem.h>

//////////////////////////////////////////////////////////////////////////////

class CWordSequence;

class CWord : public CWordBase,
              public CInputItem
{

public:

    class SHomIt {
        friend class CWord;
        public:
            SHomIt()
                : Current(), Begin(), End()
            {
            }

            bool Ok() const {
                return Current != End;
            }

            bool Ok32() const {
                return Ok() && GetID() < 32;
            }

            void operator++();

            const CHomonym* operator->() const {
                return GetPtr().Get();
            }

            const CHomonym& operator*() const {
                return *GetPtr();
            }

            size_t GetID() const {
                YASSERT(Ok());
                return Current - Begin;
            }

            bool TestMask(ui32 mask) const {
                return (static_cast<ui32>(1) << GetID()) & mask;
            }

            //bool operator==(CWord::hom_iter_const it) const { return it == m_CurIt; }

            bool operator==(const CWord::SHomIt& it) const {
                return it.Current == Current;
            }

            bool operator!=(const CWord::SHomIt& it) const {
                return it.Current != Current;
            }

        private:
            SHomIt(const THomonymVector& homonyms);

            const THomonymPtr& GetPtr() const {
                YASSERT(Ok());
                return *Current;
            }

            THomonymVector::const_iterator Current;
            THomonymVector::const_iterator Begin;
            THomonymVector::const_iterator End;

    };

    class THomMaskIterator: public SHomIt {
    public:
        // mask 0 means - all available homonyms
        inline THomMaskIterator(const CWord& w, ui32 mask = 0)
            : SHomIt(w.IterHomonyms())
            , Mask(mask ? mask : ~static_cast<ui32>(0))
        {
        }

        inline bool Ok() const {
            return SHomIt::Ok32();
        }

        inline void operator++() {
            SHomIt::operator++();
            for (; Ok(); SHomIt::operator++())
                if (TestMask(Mask))
                    return;
        }
    private:
        ui32 Mask;
    };

    friend class CWordVector;
    friend class CDictsHolder;
    friend class CSentence;
    friend class CFragmentationRunner;
    friend class CMultiWordCreator;
    friend class CTerminalsAssigner;
public:
    CWord(const CPrimGroup &prim, const Wtroka &strWord, bool ignoreUpperCase);
    CWord(const Wtroka& strWord, bool ignoreUpperCase);

    //сама применяет к m_txt морфологию и заполняет омонимы
    void CreateHomonyms(bool bUsePrediction = true);

    bool    HasHomonym(const Wtroka& str) const;
    int     GetHomonymIndex(const Wtroka& str) const;

    // TTStaticNames - any class, derived from TStaticNames
    template <typename TTStaticNames>
    bool HasHomonymFromList() const {
        for (SHomIt it = IterHomonyms(); it.Ok(); ++it)
            if (TTStaticNames::Has(it->GetLemma()))
                return true;
        return false;
    }

    bool    IsAndConj() const;
    bool    IsName(ENameType type) const;
    int     IsName_i(ENameType type, int iStartHom = 0) const;
    bool    IsDictionary() const;

    bool HasPOS(TGrammar POS) const;
    int HasPOSi(TGrammar POS) const;
    bool HasPOS(const TGramBitSet& POS) const;
    int HasPOSi(const TGramBitSet& POS) const;

    int HasPOSi(TGrammar POS, const Wtroka& strLemma) const;

    bool HasUnknownPOS() const;
    bool HasOnlyUnknownPOS() const;

    bool HasAnyOfPOS(const TGrammarBunch& POSes) const;
    bool HasAnyOfPOS(const TGramBitSet& POSes) const;

    bool HasOnlyPOS(TGrammar POS) const;
    bool HasOnlyPOS(const TGramBitSet& POS) const;

    bool HasOnlyPOSes(const TGramBitSet& POSes) const;

    bool    IsNameFoundInDic() const;
    virtual CWord* AsWordRus() {return this; };
    void    AddTerminalSymbolToAllHomonyms(TerminalSymbolType iS);
    void    DeleteAllTerminalSymbols();
    void    UniteHomonymsTerminalSymbols();
    bool    HasCommonHomonyms(const CWord& w) const;
    bool    IsRusComplexWordAsCompName() const;
    bool    IsLatComplexWordAsCompName() const;
    bool    IsComplexWordAsCompName() const;
    bool    IsComplexWordAsDate() const;//для дат типа 11.06.07 чтобы тоже считались за Word
    int HasFirstHomonymByTerminal_i(TerminalSymbolType s) const;
    bool HasFirstHomonymByTerminal(TerminalSymbolType s) const;
    bool HasDeletedHomonyms() const;

    int HasFirstHomonymByHomIDs_i(long homIDs) const;
    bool HasFirstHomonymByHomIDs(long homIDs) const;

    //искусственное ли это слово, созданное для обозначение EndOfStream
    bool IsEndOfStreamWord() const;
    bool IsFirstInSentence() const {
        return (GetSourcePair().FirstWord() == 0);      // true if @this is the first word of sentence
    }

    virtual bool HasNameHom() const;
    virtual bool HasGeoHom() const;

    int HasHomFromMorphDic(char dicType) const;
    int HasHomFromMorphDicWithPOS(char dicType, TGrammar pos) const;

    bool HasAtLeastTwoUpper() const;
    bool IsUpperLetterPointInitial() const;

    Wtroka DecapitalizedText() const {
        // Remove title-case for the first word of a sentence (not having other upper-cased letters)
        return (IsFirstInSentence() && !HasAtLeastTwoUpper()) ? GetLowerText() : GetText();
    }

    virtual void    AddFio(yset<Wtroka>& strFios, bool bIndexed);

public:

    virtual size_t GetHomonymsCount() const  {
        return m_Homonyms.size();
    }
    virtual THomonymBasePtr GetHomonym(size_t iHom) const;

    size_t GetRusHomonymsCount() const;
    const CHomonym& GetRusHomonym(size_t iH) const;
    CHomonym& GetRusHomonym(size_t iH);

    const THomonymVector& GetPredictedHomonyms() const
    {
        return m_PredictedHomonyms;
    }

    THomonymVector& GetPredictedHomonyms()
    {
        return m_PredictedHomonyms;
    }

    virtual void AddHomonym(THomonymBasePtr pHom, bool bAllowIgnore);   // previously was bCanDelete, meaning that homonym could be ignored if !CanAddHomonym()
    virtual void AddPredictedHomonym(THomonymBasePtr pHom, bool bAllowIgnore);
    int AddHomonym(THomonymPtr pHom, bool bAllowIgnore);
    int AddRusHomonym(THomonymPtr pHom);

    bool InitNameType();
    bool PredictHyphenSurname();

    void AddHyphenSurnameLemma(int iH, const THomonymGrammems& forms, const Wtroka& strLemma);
    bool TryToPredictPatronymic();
    THomonymPtr PredictAsPatronymic();
    bool PredictAsSurname();

    SHomIt IterHomonyms() const  {
        return SHomIt(m_Homonyms);
    }

    void DeleteHomonym(size_t i);
    void DeleteHomonymWithPOS(const TGramBitSet& POS);
    void DeleteAllHomonyms();

private:
    void DeleteHomonym(CHomonym& hom);
    void UndeleteHomonym(CHomonym& hom);

public:
    void SetSourcePair(int iW1, int iW2);
    bool    LessBySourcePair(CWord*& pW) const;

    int     HasMorphNoun_i() const;
    bool    HasMorphNoun() const;
    bool    HasMorphAdj() const;
    int     HasMorphAdj_i() const;
    int     HasConjHomonym_i() const;
    bool    HasConjHomonym() const;
    bool    HasOneConjHomonym() const;
    int     HasSubConjHomonym_i() const;
    bool    HasSubConjHomonym() const;
    int     HasSimConjHomonym_i() const;
    bool    HasSimConjHomonym() const;

    const   CWordsPair& GetSourcePair() const;

    bool    FindLemma(const Wtroka& strLemma) const;
    bool    IsOriginalWord() const;

    bool    HasMorphNounWithGrammems(const TGramBitSet& grammems) const;
    int     HasMorphNounWithGrammems_i(const TGramBitSet& grammems) const;
    bool    HasHomonymWithGrammems(const TGramBitSet& grammems) const;

    bool HasAuxArticle(EDicType dic) const;
    bool HasGztArticle() const;

    int PutAuxArticle(int iH, const SDictIndex& dicIndex, bool bCloneAnyWay /*=true*/);
    int PutGztArticle(int iH, const TGztArticle& gzt_article, bool bCloneAnyWay /*=true*/);

    //del
    bool    HasArticle(yset<TKeyWordType>&  kw_types) const;

    bool    HasArticle(const SArtPointer& artP) const;
    int     HasArticle_i(const SArtPointer& artP) const;
    bool    HasArticle(const SArtPointer& artP, ui32& homIDs) const;
    void    GetHomIDsByKWType(const SArtPointer& artP, ui32& homIDs) const;

    inline bool HasKWType(TKeyWordType type) const
    {
        return HasArticle(SArtPointer(type));
    }

    bool IsAnalyticVerbForm() const;
    int HasAnalyticVerbFormHomonym_i(TGrammar POS) const;
    bool HasAnalyticVerbFormHomonym(TGrammar POS) const;
    void ArchiveAutomatSymbolInterpetationUnions(yvector<TerminalSymbolType>& vecUnionsArchive);
    virtual bool IsMultiWord() const { return false; }
    bool IgnoreUpperCase() const { return m_bIgnoreUpperCase; }

protected:
    bool CanAddHomonym(const CHomonym& h) const;
    bool RightPartIsSurname(int& iH, THomonymGrammems& grammems, Wtroka& strLemma);

protected:
    THomonymVector m_Homonyms;
    THomonymVector m_PredictedHomonyms;  // here saved all bastard homonyms found by morphology
                                         // they could be transferred to @m_homonym if article with corresponding lemma is found

    size_t m_DeletedHomCount;
    CWordsPair  m_SourceWords;  //если обычное слово - m_iFirstWord = m_iLastWord = просто его номер в предложении
                                //если многословное слово - то координаты отрезка, из которого оно получено
    bool m_bIgnoreUpperCase;

};

class CMultiWord: public CWord
{
public:
    CMultiWord(const CPrimGroup &prim, const Wtroka &strWord, bool ignoreUpperCase)
        : CWord(prim, strWord, ignoreUpperCase)
    {
    }

    CMultiWord(const Wtroka& strWord, bool ignoreUpperCase)
        : CWord(strWord, ignoreUpperCase)
    {
    }

    virtual bool IsMultiWord() const {
        return true;
    }
};

