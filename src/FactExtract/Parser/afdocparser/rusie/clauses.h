#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "synchain.h"
#include "synchain.h"
#include "analyticformbuilder.h"
#include "dictsholder.h"


enum EDirection //направление при поиске чего-либо
{
    LeftToRight,
    RightToLeft
};

struct SValency
{
    SValency(): m_ActantType(WordActant), m_ClauseID(-1), m_ValNum(-1), m_Rel(AnyRel) {};

    bool IsValid() const
    {
        if (m_ActantType == WordActant)
            return m_Actant.IsValid();
        else
            return m_ClauseID != -1;
    }

    bool IsSubjVal() const
    {
        return ((m_Rel == SubjDat) || (m_Rel == Subj) || (m_Rel == SubjGen));
    }

    EActantType m_ActantType;
    SWordHomonymNum m_Actant;
    long    m_ClauseID; //если m_ActantType == Clause, то это ID клаузы, заполняющей валентность
    long    m_ValNum;   //номер валентности в статье (IValencyVariantsPtr = IValenciesPtr->GetValencyVariants(m_ValNum))
    ESynRelation    m_Rel;
};

struct SConjType
{
    SConjType(): m_Type(gInvalid), m_bSimConjAnd(false) {};

    static bool IsSimConjAnd(const CHomonymBase& hom) {
        return hom.IsSimConj() && (hom.Lemma == RUS_WORD_I);
    }

    static TGrammar GetConjType(const CHomonymBase& hom) {
        const TGrammar types[4] = {gSubConj, gSimConj, gPronounConj, gCorrelateConj};
        for (int i = 0; i < 4; ++i)
            if (hom.HasGrammem(types[i]))
                return types[i];
        return gInvalid;
    }

    bool IsSubConj();
    bool IsSimConj();
    bool IsSimConjAnd();
    bool IsCorr();

    TGrammar m_Type;
    bool m_bSimConjAnd;       // represents SimConjAnd grammems missing in Yandex morph

    bool IsInPair(CWordsPair& words_pair, const CWordVector& words);
    bool Equal(SConjType& conj)
    {
        return conj.m_Type == m_Type && conj.m_bSimConjAnd == m_bSimConjAnd && conj.m_WordNum == m_WordNum;
    }
    SWordHomonymNum m_WordNum;
    SValency    m_Val; // валентность для из поля СОЮЗ_УПР
    SValency    m_SupVal; // валентность для из поля ГЛАВ
};

struct SClauseType
{
    SClauseType(): m_NodePOS(), m_Type(UnknownClause), m_Modality(Necessary), m_bNegation(false) {};

    bool operator==(const SClauseType& ClauseType) const
    {
        return ClauseType.m_NodeNum == m_NodeNum;
    }

    bool HasPOS(TGrammar POS) const { return CHomonym::HasPOS(m_NodePOS, POS); }
    bool HasPOS(const TGramBitSet& POS) const { return CHomonym::HasPOS(m_NodePOS, POS); }

    //возвращает false, если не помещается в pair
    //а так - изменяет все слоты
    bool AdjustByPair(CWordsPair& words_pair, const CWordVector& words);
    bool HasSubjVal();
    int HasSubjVal_i() const;

    TGramBitSet m_NodePOS;
    EClauseType m_Type;
    SWordHomonymNum m_NodeNum; //этот слот должен обеспечивать уникальность типа (не может быть два типа с одинаковыми m_NodeNum)
    yvector<SValency> m_Vals; //валентности вершины - у SValency m_ValNum должен быть равен номеру в m_Vals
    SValency    m_SupVal; //валентность из поля ГЛАВ
    EModality  m_Modality;
    bool m_bNegation;   // клауза с отрицанием, т.е. перед вершиной стоит "не"
    SClauseType  Clone();
};

struct SBadComma
{
    SBadComma(): m_iNum(-1), m_iBadWeight(0) {};
    SBadComma(int num, int badWeight): m_iNum(num), m_iBadWeight(badWeight) {};

    int m_iNum;
    int m_iBadWeight;
};

struct SHom2Group
{
    SHom2Group(): m_iHomNum(0), m_pGroup(NULL) {};
    int m_iHomNum;
    CSynGroup* m_pGroup;

};

class CClause : public CWordsPair
{

friend class CClauseVariant;
friend class CClauseRules;
friend class CTopClauses;
friend class CClauseNode;
friend class CFragmentationRunner;

public:
    CClause(CWordVector& Words);

    virtual ~CClause() {};
    static CClause* Create(CWordVector& Words);

public:
    void Init(bool bBuildAnalyticForm = true);
    void InitClauseWords(); //заполняет m_ClauseWords
    void InitClauseWords(CClause& clause); //заполняет m_ClauseWord, копируя их из clause
    bool IsValid() const;

    //пытается найти многословные союзы и возвращает true
    //если нашелся многословный союз, у которого УД_ОМОНИМ = нет
    //то есть нужно породить новый вариант
    bool TryToFindMultiWordConjs();
    //ищет союзы в клаузе; возвращает true если нашла
    bool InitConjs();
    bool EqualByConj(CClause& clause);
    //ищет многословные слова и заменят из в m_ClauseWords
    void FindMultiWords();
    Wtroka ToString(bool bAllWords = true) const;
    virtual void RunSynAn() {};

    virtual void Copy(const CClause& clause);
    CClause* Clone();
    virtual CWordsPair* CloneAsWordsPair() { return Clone(); }
    //пытается выделить начало клаузы в отдельную, если вначале подряд стоят подч. союзы
    CClause* CloneBySubConj();
    void Print(TOutputStream& stream, ECharset encoding) const;
    void PrintNodeInfo(TOutputStream& stream, ECharset encoding) const;
    void PrintConjsInfo(TOutputStream& stream, ECharset encoding) const;

    //если iLastWord < m_iLastWord, то удаляет елементы из m_ClauseWords так, чтобы они влезали
    virtual void SetLastWord(int iLastWord);
    virtual void SetFirstWord(int iFirsWord);
    bool HasDash();
    bool HasColon();
    int  HasCorr_i();
    bool HasSimConj();
    int HasSimConjWithCorr_i();
    bool HasSimConjWithCorr();
    int HasSimConj_i();
    bool HasSubConj();
    int  HasSubConj_i();
    int GetConjsCount() const;
    bool HasConjs() const;
    const SConjType& GetConjType(int iConj) const;
    //смотрит на статью cоюза и изменяет типы
    void InitClauseTypesByConj();
    void  PutID(long ID);
    long GetID();
    void DeleteType(int iType);
    bool HasType(EClauseType Type) const;
    int HasType_i(EClauseType Type) const;
    int HasEmptyType();
    bool HasOneTypeNot(EClauseType Type);
    int HasOneTypeNot_i(EClauseType Type);

    bool HasOneTypeWithPOSNot(const TGramBitSet& POS);
    int HasOneTypeWithPOSNot_i(const TGramBitSet& POS);
    bool HasTypeWithSubj();

    bool HasTypeWithPOS(const TGramBitSet& POS);
    int HasTypeWithPOS_i(const TGramBitSet& POS);
    bool HasTypeWithPOS(TGrammar POS);
    int HasTypeWithPOS_i(TGrammar POS);

    bool HasPunctAtTheEnd(); //последнее из m_ClauseWords - запятатя
    bool HasPunctAtTheVeryEnd(); //слово m_LasWord - запятатя
    //добавляет ClauseWords из clause в m_ClauseWords в конец или в начало в зависимости от bAtTheEnd
    void AddClauseWords(CClause& clause, bool bAtTheEnd = true);
    //заполняет из vals нужные валентности для iType
    void AddValsToType(yvector<SValency> vals, int iType);
    void AddValToType(const SValency& val, int iType);
    //удалить все типы кроме iType
    void DelTypesExcept(int iType);
    void DelAllTypesAndAdd(SClauseType type);
    void PrintType(TOutputStream& stream, int iType, ECharset encoding);
    Wtroka PrintTypeForJava(int iType);
    void RefreshValencies();
    long CalculateBadWeight();
    long CalculateTypesBadWeight(); //возвращает минимальный плохой вес
    long GetTypeBadWeight(int iType);
    int FindSubjVal(int iType); //находит валентность по статье у этого типа
    void PrintClauseWord(Wtroka& strClauseWords);
    void PrintDisruptedWords(yvector< TPair<Wtroka, TPair<int, int> > >& words2pairs);
    bool IsDisrupted();
    int  GetFirstClauseWordWithType(); //выдает ClauseWord, который может быть вершиной
    bool IsSubClause();
    const CHomonym& GetConj(int iConj) const;
    int FindLemma(const Wtroka& strLemma, EDirection direction);
    bool  FindAntecedent(const Wtroka& strLemma);
    void  PutLastWordPunct();
    SWordHomonymNum FindNounNominative();
    bool HasNounWithOnlyNominative();
    bool HasWordsGroupSimConj();

//    virtual void PrintToXMLForJava(TXmlNodePtrBase /*piClausesEl*/, yvector<CWord*>& /*sentWords*/) {};

    int GetWordsCount() const;
    int GetTypesCount() const;
    const SClauseType& GetType(int i) const;
    SClauseType& GetType(int i);

    void AddBadWeight(int badWeight)
    {
        m_BadWeight += badWeight;
    }

protected:
    //ищет актанта для валентности piVal предикатива pred
    bool FindActant(SWordHomonymNum& pred, const valencie_t& piVal, SWordHomonymNum& actant, EDirection direction = LeftToRight);
    bool FindActantInWordSpan(int iW1, int iW2, SWordHomonymNum& pred, const valencie_t& piVal,
                              SWordHomonymNum& actant, EDirection direction);

    //ищет актанта для валентности piVal предикатива pred (но особым образом реагирует на ф-цию согл. SubjVerb и SubjAdjShort)
    bool FillValency(SWordHomonymNum& pred, const valencie_t& piVal, SWordHomonymNum& actant);

    //пытается заполнить валентности для Type
    bool FillValencies(SClauseType& Type, bool bRefresh = false);

    //проходит по всем валентностям и изменяет тип (WantsNothing или WantsVal) и модалтность (Possibly или Necessary)
    void CheckTypeByVals(SClauseType& Type);

    //изменяются границы клаузы, то изменяет и удаляет элементы из m_Types и m_Conjs
    void AdjustTypesAndConjs();

    //ищет все многословные зоюзы(причем и те, первое слово которых может залезать за левую границу)
    void FindAllMultiWordsConj(yvector<SWordHomonymNum>& multiWords);

    // ищет вершины
    void InitNodes();

    // заполняет валентности вершин
    void InitClauseTypes();

    //стирает старые союзы и типы и ищет все заново
    void  RefreshClauseInfo();

    //стирает старые типы и ищет их заново
    void  RefreshConjInfo();

    //заменят однословные слова (и только их) на данное многословное
    void SubstituteByMultiWord(SWordHomonymNum& WordHomonymNum);

    //смотрит может ли этот тип заполнять какую-нибудь валентность из piValVars (СОЮЗ_УПР)
    bool CompTypeWithConjVals(SClauseType& type, const or_valencie_list_t& piValVars);

    //накладывает валентность piVal на все омонимы word и возвращает первый подходящий
    int  CompValencyWithWord(const valencie_t& piVal, const CWord& word , EPosition pos);

    //накладывает валентность piVal на все омонимы word и подходящие запихивает в homs
    bool CompValencyWithWord(const valencie_t& piVal, const CWord& word , EPosition pos, yvector<int>& homs);

    //накладывает валентность piVal на hom
    bool CompValencyWithHomonym(const valencie_t& piVal, const CHomonym& hom, EPosition pos);

    bool ConjValsHasTypeWord(const or_valencie_list_t& piValVars, EClauseType& type);

    //правило о двух предикациях
    void DeleteHomPredic();

    //(для правила о двух предикациях) у всех слов удаляет омоним с ЧР глагол кроме "быть" и "стать"
    void DeleteVerbHomExceptBytStat(int iGoodNode);

    //(для правила о двух предикациях) у всех слов удаляет омоним с ЧР POS
    void DeleteHomWithPOS(const TGramBitSet& POS);

    //ищет слово у которого есть омоним только с такой частью речи
    bool FindFirmWord(const TGramBitSet& POS, int& iW);

    //определяет вершины и союзы
    void InitClauseInfo();

    //заполняет поле SClauseType::m_Type и, если возможно несколько вариантов заполнения,
    //порождает новые типы
    bool InitClauseType(SClauseType& Type, yvector<SClauseType>& new_types);

    //ищет подлежащее для Verb, AdjShort, PartShort
    static bool CanBeSubject(const CHomonymBase& hom, int& iBadWeight);
    bool FindSubjectOld(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc);
    bool FindSubject(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc);
    virtual void GetAllGroupsByMainWordWithGrammems(yvector<CGroup*>& /*groups*/, int /*iClauseWord*/, int /*iH*/, const TGramBitSet& /*grammems*/) {};

    virtual bool FindNounGroupAsSubject(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc);

    //ищет подлежащее для Predic
    bool FindSubjectForPredic(SWordHomonymNum& pred, SWordHomonymNum& subj, int iPredk);

    //возвращает true - если нашла слово с такой частью речи и оно неомонимично
    //  else - false
    bool TryToInitNode(const TGramBitSet& POS, yvector<size_t>& homs_count);

    void TryToAddEmptyType();

    //есть ли омоним сущ. или местоим. сущ. или личн. мест. или колич. числ.
    //к тому же присваевает плохой вес (есть омонимы, есть омонимичный им. п., и т.д.)
    //bool CanBeSubject(SWordHomonymNum& word, int& iBadWeight);

    void AddNode(SWordHomonymNum WordHomonymNum, const TGramBitSet& POS);

    //ищет  word в массиве m_ClauseWords
    int FindClauseWord(SWordHomonymNum& word);

    //если клауза является вводной конструкцией из словаря, то стираем все остальные типы
    bool InitParenthType();
    //если клауза заключена в скобки, то стираем все остальные типы
    bool InitBracketsType();
    bool CheckConjPosition(const CHomonym& hom, int iW, bool bPrevConjs);
    void  ClearConjs();
    void  ClearTypes();

    void MultiplyBadCommaWeight(CWordsPair& pair, int coef);
    void AddBadComma(int num, int w);
    void AddBadComma(int num, int w, CWordsPair& pair, int coef);
    long GetBadWeightForCommas();

    virtual bool CanBeNode(int iClauseWord, int iH);

    void LockHomonym(const CHomonymBase& hom) {
        LockedHomonyms.insert(&hom);
    }

    void UnlockHomonym(const CHomonymBase& hom) {
        LockedHomonyms.erase(&hom);
    }

    bool IsLockedHomonym(const CHomonymBase& hom) {
        return LockedHomonyms.find(&hom) != LockedHomonyms.end();
    }

    void BuildAnalyticWordForms();
    virtual bool HasSynInfo() { return false; }

protected:
    CWordVector& m_Words;
    yvector<SClauseType> m_Types;
    yvector<SConjType> m_Conjs;
    yvector<SWordHomonymNum> m_ClauseWords;

    yset<const CHomonymBase*> LockedHomonyms;

    long    m_ID;
    long    m_BadWeight;
    long    m_RuleBadWeight;

    yvector<SBadComma> m_BadCommas;

    bool    m_bCalculatedBadWeight;
};

class CClauseSyn: public CClause
{
friend class CClauseVariant;
friend class CClauseRules;
friend class CTopClauses;
friend class CClauseNode;
friend class CFragmentationRunner;

public:
    CClauseSyn(CWordVector& Words);
    virtual ~CClauseSyn();
    virtual void Copy(const CClause& _clause);
    virtual void RunSynAn();
    //проходится по синвариантам и ищет вариант, где это слово
    //либо является примитивной группой либо вершиной гл. группы
    virtual bool CanBeNode(int iClauseWord, int iH);
    //просто проходит по всем вариантам групп (и только групп, одиночные слова не берутся)
    //и ищет группу, которая имеет среди граммем Nominative и согласуется по граммемам
    //с предикатом
    virtual bool FindNounGroupAsSubject(SWordHomonymNum& pred, SWordHomonymNum& subj, ECoordFunc CoordFunc);
    virtual bool HasSynInfo() { return true; }
//    void PrintToXMLForJava(TXmlNodePtrBase piClauseEl, int iType, int iSynVar, yvector<CWord*>& sentWords);
//    Stroka PrintHomonym(const CHomonym& homRus, ECharset encoding);
//    virtual void PrintToXMLForJava(TXmlNodePtrBase piClauseEl, yvector<CWord*>& sentWords);

protected:
    virtual void GetAllGroupsByMainWordWithGrammems(yvector<CGroup*>& groups, int iClauseWord, int iH,
                                                    const TGramBitSet& grammems);

protected:
    CSynChains m_SynChains;
};
