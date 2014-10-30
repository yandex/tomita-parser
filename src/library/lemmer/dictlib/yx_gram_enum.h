#pragma once

#include <util/system/defaults.h>

enum TGrammar {
    gInvalid = (unsigned char)0,
    gBefore = (unsigned char)126,
    /*--------------------*/
    gPostposition,  // POSTP
    gFirst  = gPostposition,
    gAdjective,     // A      // Nomenus
    gAdverb,        // ADV
    gComposite,     // COM(P)
    gConjunction,   // CONJ
    gInterjunction, // INTJ
    gNumeral,       // NUM
    gParticle,      // PCL
    gPreposition,   // PRE(P)
    gSubstantive,   // S
    gVerb,          // V
    gAdjNumeral,    // ANUM
    gAdjPronoun,    // APRO
    gAdvPronoun,    // ADVPRO
    gSubstPronoun,  // SPRO
    gArticle,       // артикли
    gPartOfIdiom,   // части идиом (прежде всего иностр. слов)
    gLastPartOfSpeech = gPartOfIdiom,
    gReserved,      // зарезервировано    // особые пометы
    gAbbreviation,  // сокращения
    gIrregularStem, // чередование в корне (или супплетивизм)
    gInformal,      // разговорная форма
    gDistort,       // искаженная форма
    gContracted,    // стяжённая форма (фр. q' и т.п.)
    gObscene,       // обсц
    gRare,          // редк
    gAwkward,       // затр
    gObsolete,      // устар
    gSubstAdjective,// адъект
    gFirstName,     // имя
    gSurname,       // фам
    gPatr,          // отч
    gGeo,           // гео
    gProper,        // собств
    gPresent,       // наст  // Tempus
    gNotpast,       // непрош
    gPast,          // прош
    gFuture,        // буд. время (фр., ит.)
    gPast2,         // фр. passe simple, ит. passato remoto
    gNominative,    // им    // Casus
    gGenitive,      // род
    gDative,        // дат
    gAccusative,    // вин
    gInstrumental,  // твор
    gAblative,      // пр
    gPartitive,     // парт(вин2)
    gLocative,      // местн(пр2)
    gVocative,      // звательный
    gSingular,      // ед    // Numerus
    gPlural,        // мн
    gGerund,        // деепр // Modus
    gInfinitive,    // инф
    gParticiple,    // прич
    gIndicative,    // изъяв
    gImperative,    // пов
    gConditional,   // усл. наклонение (фр., ит.)
    gSubjunctive,   // сослаг. накл. (фр., ит.)
    gShort,         // кр    // Gradus
    gFull,          // полн
    gSuperlative,   // прев
    gComparative,   // срав
    gPossessive,    // притяж
    gPerson1,       // 1-л   // Personae
    gPerson2,       // 2-л
    gPerson3,       // 3-л
    gFeminine,      // жен   // Gender (genus)
    gMasculine,     // муж
    gNeuter,        // сред
    gMasFem,        // мж
    gPerfect,       // сов   // Perfectum-imperfectum (Accept)
    gImperfect,     // несов
    gPassive,       // страд // Voice (Genus)
    gActive,        // действ
    gReflexive,     // возвратные
    gImpersonal,    // безличные
    gAnimated,      // од    // Animated
    gInanimated,    // неод
    gPraedic,       // прдк
    gParenth,       // вводн
    gTransitive,    // пе     //transitivity
    gIntransitive,  // нп
    gDefinite,      // опред. члены   //definiteness
    gIndefinite,    // неопред. члены

    gSimConj,       // сочинит. (для союзов)
    gSubConj,       // подчинит. (для союзов)
    gPronounConj,   // местоимение-союз ("я знаю, _что_ вы сделали прошлым летом")
    gCorrelateConj, // вторая зависимая часть парных союзов - "если ... _то_ ... ", "как ... _так_ и ..."

    gAuxVerb,       //вспомогательный глагол в аналитической форме ("_будем_ думать")
    /*--------------------*/
    /* Additional grammemes for Turkish and Armenian */
    gDual,
    gPoss1p,
    gPoss1pSg,
    gPoss1pPl,
    gPoss2p,
    gPoss2pSg,
    gPoss2pPl,
    gPoss3p,
    gPoss3pSg,
    gPoss3pPl,
    gPredic1pSg,
    gPredic1pPl,
    gPredic2pSg,
    gPredic2pPl,
    gPredic3pSg,
    gPredic3pPl,
    gElative,
    gDerivedAdjective,
    gEvidential,
    gNegated,
    gPotential,
    gMedial,
    gHuman,
    gAssocPlural,
    gOptative,
    gConverb,
    gSimultaneous,
    gConnegative,
    gResultative,
    gSubjective,
    gCount, // счётная форма
    gGoodForm,
    gPredicPoss,
    gIntention,
    gHonorific,
    gEquative,
    gPredic,
    gObligatory,
    gMax,
    gMax2 = gMax
};

enum TGramClassEnum {
    bVow    = 0x01000000,
    bText   = 0x02000000,
    bBar    = 0x04000000,
    bDigit  = 0x08000000,
    bKeywrd = 0x10000000,

    bUniq   = 0x00000000,
    bNom    = 0x00000001,
    bTemp   = 0x00000002,
    bCas    = 0x00000004,
    bNum    = 0x00000008,
    bMod    = 0x00000010,
    bRepr   = 0x00000020,
    bGrad   = 0x00000040,
    bPoss   = 0x00000080,
    bPers   = 0x00000100,
    bGend   = 0x00000200,
    bPerf   = 0x00000400,
    bVox    = 0x00000800,
    bAnim   = 0x00001000,
    bPrdc   = 0x00002000,
    bPrnth  = 0x00004000,
    bTran   = 0x00008000,
    bNS     = 0x00010000,
    bDet    = 0x00020000,
    bDerivNom  = 0x00040000,
    bConvClass = 0x00080000,
    //        0x00100000,
    //        0x00200000,
    //        0x00400000,
    //        0x00800000,
    //        0x01000000,
    //        0x02000000,
    //        0x04000000,
    //        0x08000000,
    bInfer  = 0x10000000,
    bNeg    = 0x20000000,
    bPot    = 0x40000000,
    bPredic = 0x80000000,
    bGram   = 0xFFFFFFFF,
};

typedef ui32 TGramClass;

const char BAD_BASTARD_GRAMMAR[] = {
    (char) (unsigned char) gReserved,
    (char) (unsigned char) gIrregularStem,
    (char) (unsigned char) gRare,
    (char) (unsigned char) gAwkward,
    (char) (unsigned char) gAbbreviation,
    (char) 0};
