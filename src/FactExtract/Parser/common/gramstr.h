#pragma once

#include "toma_constants.h"
#include "serializers.h"

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/charset/wide.h>

  typedef enum
  {
    OnTheRight,
    OnTheLeft,
    OnTheBeginning,
    AfterVerb,
    AfterPrep,
    AnyPosition,
    PositionsCount
  } EPosition;

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(EPosition)

  typedef enum
  {
    Fragment,
    WordActant,
    ActantTypeCount
  } EActantType;

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(EActantType)

  typedef enum
  {
    ComplPredic,
    SubjDat,
    Compl,
    Subj,
    SubjGen,
    Attr,
    AnyRel,
    SynRelationCount
  } ESynRelation;

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(ESynRelation)

  //Р_РчР_Р+С:Р_Р_РёР_Р_С_С'С_ Р_Р°Р>РчР_С'Р_Р_С_С'Рё
  typedef enum
  {
    Necessary,
    Possibly,
    ModalityCount
  } EModality;

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(EModality)

  enum EActantCondinionType
  {
    FollowACType = 0,
    AgrACType,
    ActantCondinionTypeCount
  };

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(EActantCondinionType)

  enum EFactEntryEqualityType {SameEntry, FromOneDoc, FromOneSent, EFactEntryEqualityTypeCount};

  const char FactEntryEqualityTypeStr[EFactEntryEqualityTypeCount][iMaxWordLen] =
  {
    "SameEntry",
    "SameDocument",
    "SameSent"
  };

  //С'РёРїС< С"С_Р°Р_Р_РчР_С'Р_Р_ РїР_ С_С'РчРїРчР_Рё РїР_Р>Р_Р_С'С<
  typedef enum
  {
    HasSubject,
    NeedSubject,
    Impersonal,
    Ellipsis,
    VerbAdverbEllipsis,
    ParticipleEllipsis,
    WithInf,
    WithoutInf,
    Empty,
    SubConjEmpty,
    SubConjEmptyNeedEnd,
    WithConj,
    WithoutConj,
    SingleParticiple,
    FullParticiple,
    HasNounNominativ,
    DashClause,
    NeedNode,
    WantsVal,
    WantsNothing,
    AnyClauseType,
    UnknownClause,
    Parenth,
    Brackets,
    ClauseTypesCount
  } EClauseType;

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(EClauseType)

enum EArticleTextAlg
{
    CompInTextAlg = 0,
    GeoInTextAlg,
    ArtAlgCount
};

enum EPartOfSpeech      // from old morph - will be dropped soon
{
      Noun  = 0,                //gSubstantive
      Adj = 1,                  //gAdjective
      Verb = 2,                 //gVerb
      PronounNoun = 3,          //gSubstPronoun
      PronounAdj = 4,           //gAdjPronoun
      PronounPredk = 5,
      CardinalNumeral  = 6,     //gNumeral
      OrdinalNumeral = 7,       //gAdjNumeral
      Adverb = 8,               //gAdverb
      Predic  = 9,              //gPraedic
      Prep = 10,                //gPreposition
      POSL = 11,
      Conj = 12,                //gConjunction
      Interj = 13,              //gInterjunction
      Parenthesis  = 14,        //gParenth
      _DUMMY = 15,
      Phrase = 16,
      Particle = 17,            //gParticle
      UnknownPOS = 18,
      Noun_g  = 19,             //gSubstantive + gGeo
      Adj_g = 20,               //gAdjective + gGeo
      Noun_n  = 21,             //gSubstantive + gFirstName
      AdjShort = 22,            //gAdjective + gShort
      Participle = 23,          //gParticiple
      VerbAdverb = 24,          //gGerund
      PartShort = 25,           ////gParticiple + gShort
      Infinitive = 26,          //gInfinitive
      Noun_o  = 27,
      Adj_o = 28,
      Comparative = 29,         //gComparative
      PossesiveAdj = 30,        //gAdjective + gPossessive
      PersonalPronoun = 31,
      SubConj = 32,             //gConjunction + gSubConj
      SimConj = 33,             //gConjunction + gSimConj
      AdvConj = 34,
      PronounConj = 35,         //gConjunction + gPronounConj
      Number = 36,
      Imperativ = 37,           //gImperative
      Corr = 38,                //gComposite?
      SimConjAnd = 39,
      AuxVerb = 40,
      POSCount = 41
};

const char rPartOfSpeeches[POSCount][iMaxWordLen] =
{   "С",  // 0
    "П", // РїР_Р>Р_Р_Рч РїС_РёР>Р°Р_Р°С'РчР>С_Р_Р_Рч
    "Г",
    "МС",
    "МС-П",
    "МС-ПРЕДК",
    "ЧИСЛ",
    "ЧИСЛ-П",
    "Н",
    "ПРЕДК",
    "ПРЕДЛ",
    "ПОСЛ",
    "СОЮЗ",
    "МЕЖД",
    "ВВОДН",
    "_dummy",
    "ФРАЗ",
    "ЧАСТ", // = 17
    "ПУСТ-ГРАММЕМА",  // С'Р°Р_, Р_Р_Рч Р_Р_С_С"Р_Р>Р_Р_РёС_ С_С'Р°Р_РёС' ??
     "С_g",   // С_С_С%РчС_С'Р_РёС'РчР>С_Р_Р_Рч РёР· Р_РчР_Р_С_Р°С"РёС+РчС_РєР_Р_Р_ С_Р>Р_Р_Р°С_С_
    "П_g",    // РїС_РёР>Р°Р_Р°С'РчР>С_Р_Р_Рч РёР· Р_РчР_Р_С_Р°С"РёС+РчС_РєР_Р_Р_ С_Р>Р_Р_Р°С_С_
    "С_n",   // С_С_С%РчС_С'Р_РёС'РчР>С_Р_Р_Рч РёР· С_Р>Р_Р_Р°С_С_ РёР_РчР_ 21
    "КР_ПРИЛ",   // РєС_Р°С'РєР_Рч РїС_РёР>Р°Р_Р°С'РчР>С_Р_Р_Рч
    "ПРИЧАСТИЕ", //23
    "ДЕЕПРИЧАСТИЕ", //24
    "КР_ПРИЧАСТИЕ", // 25
    "ИНФИНИТИВ",  //26
    "С_o",
    "П_o", //28
    "СРАВН", //29
    "ПРИТЯЖ_ПРИЛ",
    "ЛИЧН_МЕСТ",
    "ПОДЧ_СОЮЗ",
    "СОЧ_СОЮЗ",
    "НАР_СОЮЗ",
    "МЕСТ_СОЮЗ",
    "ЦИФРЫ",
    "ИМПЕРАТИФ",
    "КОРРЕЛЯТ",
    "СОЧ_СОЮЗ_И",
    "ВСПОМ_ГЛ"
};

  const char  ClauseTypeNames[ClauseTypesCount][iMaxWordLen] =
  {
    "есть_подл",
    "хочу_подл",
    "безличн",
    "эллипсис",
    "деепр_эллипсис",
    "прич_эллипсис",
    "есть_инф",
    "нет_инф",
    "пустыха",
    "подч_союз_пустыха",
    "подч_союз_пустыха_неполный",
    "есть_союз",
    "нет_союза",
    "одиночное_прч",
    "прч_с_зав_словами",
    "сущ_им",
    "связка_тире",
    "хочу_вершину",
    "хочу",
    "не_хочу",
    "любой",
    "неизвестно",
    "вводн",
    "скобки"
  };

  const char  BoolStrVals[2][iMaxWordLen] =
  {
    "нет",
    "да"
  };

// for local russian constant words
#define DECLARE_STATIC_RUS_WORD(VarName, WordText) static const Wtroka VarName = UTF8ToWide(WordText)


template <class TDerived>
class TStaticNames
{
private:
    yvector<Wtroka> Names;

    typedef ymap<Wtroka, size_t, ::TLess<TWtringBuf> > TNameIndex;
    TNameIndex NameIndex;

    DECLARE_NOCOPY(TStaticNames);

protected:
    TStaticNames()
    {
    }

    Wtroka Add(const Wtroka& name) {
        TPair<TNameIndex::iterator, bool> ins = NameIndex.insert(MakePair(name, 0));
        if (ins.second) {
            ins.first->second = Names.size();
            Names.push_back(name);
        }
        return name;
    }

    Wtroka Add(const Stroka& name) {
        return Add(UTF8ToWide(name));
    }

    void AddWords(const Stroka& words) {
        TStringBuf names(words);
        names = StripString(names);
        while (!names.empty()) {
            Add(::ToString(names.NextTok(' ')));
            names = StripString(names);
        }
    }

public:
    static const TDerived* Static() {
        return Singleton<TDerived>();
    }

    static size_t Size() {
        return Static()->Names.size();
    }

    static const yvector<Wtroka>& GetList() {
        return Static()->Names;
    }

    static const Wtroka& Get(size_t i) {
        return Static()->Names[i];
    }

    static bool Has(const TWtringBuf& name) {
        return Static()->NameIndex.find(name) != Static()->NameIndex.end();
    }

    static const size_t Invalid = static_cast<size_t>(-1);

    static size_t IndexOf(const TWtringBuf& name) {
        TNameIndex::const_iterator it = Static()->NameIndex.find(name);
        if (it != Static()->NameIndex.end())
            return it->second;
        else
            return Invalid;
    }
};

#define DECLARE_STATIC_NAMES(Type, Names) \
struct Type: public TStaticNames<Type> { \
    Type() { AddWords(Names); } \
}


DECLARE_STATIC_NAMES(TRusMonths, "январь февраль март апрель май июнь июль август сентябрь октябрь ноябрь декабрь");

  const char  PositionsStr[PositionsCount][iMaxWordLen] =
  {
    "справа",
    "слева",
    "начало",
    "после_глагола",
    "после_предлога",
    "любое"
  };

  const char  ActantTypesStr[ActantTypeCount][iMaxWordLen] =
  {
    "фрагмент",
    "слово"
  };

  const char  SynRelationsStr[SynRelationCount][iMaxWordLen] =
  {
    "доп_сказ",
    "подл_дат",
    "доп",
    "подл_им",
    "подл_рд",
    "опр",
    "любое"
  };

  const char  ModalityStr[ModalityCount][iMaxWordLen] =
  {
    "необходимо",
    "возможно"
  };

  EGrammems Str2Grammem(const char* str);
  Wtroka ClauseType2Str(EClauseType ClauseType);
  Wtroka Modality2Str(EModality Modality);
