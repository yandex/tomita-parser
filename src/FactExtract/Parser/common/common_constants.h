#pragma once

#include <util/charset/wide.h>
#include "serializers.h"
#include <library/lemmer/dictlib/yx_gram.h>

const int iMaxWordLen = 50;

//agreement function types
typedef enum {
    SubjVerb,
    SubjAdjShort,
    GendreNumberCase,
    GendreNumber,
    NumberCaseAgr,
    CaseAgr,
    NumberAgr,
    AnyFunc,
    CheckLettForAbbrevs,
    CheckQuoted,
    CheckLQuoted,
    CheckRQuoted,
    FeminCaseAgr,
    FioAgr,
    GendreCase,
    IzafetAgr,
    AfterNumAgr,
    GeoAgr,
    CoordFuncCount
} ECoordFunc;

//declare as simple type for auto-serialization
//DECLARE_PODTYPE(ECoordFunc)


template <>
inline void Save(TOutputStream* out, const ECoordFunc& t) {
    ::Save<ui32>(out, static_cast<ui8>(t));
}

template <>
inline void Load(TInputStream* in, ECoordFunc& t) {
    ui32 raw;
    ::Load<ui32>(in, raw);
    // in the binary was compiled with some newer agreement type, treat it as unknown
    if (raw > CoordFuncCount)
        t = CoordFuncCount;
    else
        t = static_cast<ECoordFunc>(raw);
}


const char  CoordFuncsStr[CoordFuncCount][iMaxWordLen] =
  {
    "подл_глаг",
    "подл_кр_пр",
    "род_число_падеж",
    "род_число",
    "род_падеж",
    "падеж",
    "число",
    "любая"
  };

enum EGrammems {
         Musculinum = 0,        //gMasculine
         Feminum    = 1,        //gFeminine
         Neutrum    = 2,        //gNeuter
         MascFem    = 3,        //gMasFem
         Name       = 4,        //gFirstName
         Patronymic = 5,        //gPatr
         SurName    = 6,        //gSurname
         AllGenders   = (1<<Musculinum) | (1<<Feminum) | (1<<Neutrum),          //grammarhelper:: AllGenders - gMasFem

         Plural     = 7,        //gPlural
         Singular   = 8,        //gSingular
         AllNumbers = (1<<Singular) | (1<<Plural),                              //grammarhelper::AllNumbers

         Nominativ  = 9,        //gNominative
         Genitiv    = 10,       //gGenitive
         Dativ      = 11,       //gDative
         Accusativ  = 12,       //gAccusative
         Instrumentalis = 13,   //gInstrumental
         Vocativ    = 14,       //gAblative
         AllCases   = (1<<Nominativ) | (1<<Genitiv) | (1<<Dativ) | (1<<Accusativ) | (1<<Instrumentalis) | (1<<Vocativ), //grammarhelper:: AllCases-AllSecondCases

         ShortForm = 15,        //gShort

         Animative = 16,        //gAnimated
         NonAnimative = 17,     //gInanimated
         AllAnimative   = (1<<Animative) | (1<<NonAnimative),               //???

         Compar = 18,           //gComparative

         Perfective = 19,       //gPerfect
         NonPerfective = 20,    //gImperfect

         NonTransitive = 21,    //gIntransitive
         Transitive = 22,       //gTransitive

         ActiveVoice = 23,      //gActive
         PassiveVoice = 24,     //gPassive

         PresenceTime = 25,     //gPresent
         FutureTime = 26,       //gFuture
         PastTime = 27,         //gPast
         AllTimes   = (1<<PresenceTime) | (1<<FutureTime) | (1<<PastTime),  //???

         FirstPerson = 28,      //gPerson1
         SecondPerson = 29,     //gPerson2
         ThirdPerson = 30,      //gPerson3
         AllPersons = (1 << FirstPerson) | (1 << SecondPerson) | (1 << ThirdPerson),    //???

         Imperative = 31,       //gImperative

         Indeclinable = 32,     //???
         Initialism = 33,       //gAbbreviation

         GrammemsCount = 34,

         AllGrammems  = 0xFFFFFFFF
    };

const char Grammems[GrammemsCount][iMaxWordLen] = { "мр","жр","ср","мр-жр","имя","отч","фам",
            "мн","ед",
          "им","рд","дт","вн","тв","пр",
          "кр",
          "од","но",
          "сравн",
          "св","нс",
          "нп","пе",
          "дст","стр",
          "нст","буд","прш",
          "1л","2л","3л",
          "пвл",
          "0", "аббр"};

//mapping EGrammems -> TGrammar
const TGrammar EGrammem2TGrammar[GrammemsCount] = {
    gMasculine, gFeminine, gNeuter, gMasFem, gFirstName, gPatr, gSurname,
    gPlural, gSingular,
    gNominative, gGenitive, gDative, gAccusative, gInstrumental, gAblative,
    gShort,
    gAnimated, gInanimated,
    gComparative,
    gPerfect, gImperfect,
    gIntransitive, gTransitive,
    gActive, gPassive,
    gPresent, gFuture, gPast,
    gPerson1, gPerson2, gPerson3,
    gImperative,
    gInvalid /* not defined in ya-morph! */,
    gAbbreviation
};

  bool IsInList(const char* str_list, int count, const char* str);
  int IsInList_i(const char* str_list, int count, const char* str);

extern const Wtroka ACTANT_LABEL_SYMBOL; // 'Х';

const wchar16 YO_UPPER = 0x0401;   // Ё
const wchar16 YO_LOWER = 0x0451;   // ё
const wchar16 E_UPPER = 0x0415;    // Е
const wchar16 E_LOWER = 0x0435;    // е

const char RUS_LOWER_A = 0xE0; // строчная "а" в Windows 1251
const char RUS_LOWER_YA = 0xFF; // строчная "я" в Windows 1251

extern const Wtroka RUS_WORD_I;         // "и";
