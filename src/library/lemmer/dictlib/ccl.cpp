/*
 * (C)++ iseg
 *
 *
 */

#include <stdarg.h>

#include "yx_gram.h"
#include <util/string/vector.h>
#include <util/charset/wide.h>

inline static TGramClass GramClass(unsigned char c) { return GetClass(c) & bGram; }

/*
проверка 2 списков характеристик на несовместимость
ПРЕДУСЛОВИЕ: они должны быть отсортированы
*/
int incompatible(char *gram1, char *gram2) {
  int cl1, cl2;
  while ((cl1 = GramClass(*gram1)) != 0 && (cl2 = GramClass(*gram2)) != 0) {
    /*
    если совпадают классы но не совпадают значения характеристик
    */
    if (cl1 == cl2 && *gram1 != *gram2)
      return 1;

    /*
    продвигать указатель строчки с меньшим классом
    */
    if ((*gram1) >= (*gram2))
      gram2++;
    if ((*gram1) <= (*gram2))
      gram1++;
  }
  return 0;
}

bool grammar_byk_from_rus(const char* grammar, char* outbuf, int bufsize) {
    if (bufsize < 1)
        return false;
    bool br = true;
    int l_byk_gram = 0;
    VectorStrok grams;
    SplitStroku(&grams, grammar, ",");
    for (VectorStrok::iterator i = grams.begin(); i != grams.end(); i++) {
        int c = TGrammarIndex::GetCode(*i);
        if (c > 0) {
            if (l_byk_gram >= bufsize - 1) {
                br = false;
                break;
            }
            outbuf[l_byk_gram++] = char(c);
        }
    }
    l_byk_gram = uniqsort(outbuf, l_byk_gram);
    outbuf[l_byk_gram] = 0;
    return br;
}

// 14-09-96 uniq
int uniqsort(char *str, int rc) {
  int i;
  int j;

  if (rc <= 1)
    return rc;

  // insertion sort
  for (i = 1; i < rc; i++) {
    char ch = str[i];
    for (j = i; j > 0 && str[j-1] > ch; j--)
      str[j] = str[j-1];
    str[j] = ch;
  }

  // unique
  for (i = j = 0; i < rc; i++, j++) {
    if (j < i)
      str[j] = str[i];
    while ((i + 1 < rc) && str[i] == str[i + 1])
      i++;
  }

  // return number of unique elements
  return j;
}

extern const POS_SET Productive_PartsOfSpeech =
    POS_BIT(gAdjective)|
    POS_BIT(gAdjNumeral)|
    POS_BIT(gSubstantive)|
    POS_BIT(gVerb)|
    POS_BIT(gAdverb)
;

extern const POS_SET Preffixable_PartsOfSpeech =
    POS_BIT(gAdjective)|
    POS_BIT(gSubstantive)
;

extern const POS_SET ShortProductive_PartsOfSpeech =
    POS_BIT(gSubstantive)
;

Stroka sprint_grammar(const char* grammar, TGramClass hidden_classes, bool russ) {
    using NTGrammarProcessing::ch2tg;
    Stroka printed;
    if (!grammar)
        return printed;

    for (unsigned i = 0; grammar[i]; i++) {
        if (GramClass(grammar[i]) & hidden_classes)
            continue;
        const char *g = russ ? TGrammarIndex::GetName(ch2tg(grammar[i])) :
            TGrammarIndex::GetLatinName(ch2tg(grammar[i]));
        if (*g != 0) {
            if (!printed.empty())
                printed += ',';
            printed += g;
        }
    }
    return printed;
}

int common_grammar(Stroka gram[], int size, char *outgram) {
    int lengram = 0;
    char i_bool[gMax - gBefore];
    char common_bool[gMax - gBefore];
    const char *p;
    int i,j;

    // взведем все грамматические характеристики в битовой карте результата
    memset(common_bool + 1, '\x01', gMax - gBefore - 1);

    for (i = 0; i < size; i++) {

        // в битовой карте данной словоформы оставим взведенными только
        // ее грам_характеристики
        memset(i_bool + 1, '\x00', gMax - gBefore - 1);
        for (p = gram[i].c_str(); *p; p++) {
            assert((unsigned char)*p - gBefore < gMax - gBefore && (unsigned char)*p - gBefore >= 0);
            i_bool[(unsigned char)*p - gBefore] = 1;
        }

        // погасим отсутствующие характеристики в битовой карте результата
        for (j = 1; j < (gMax-gBefore); j++)
            common_bool[j] &= i_bool[j];
    }

    // заполним массив результата
    for (i = 1; i < (gMax-gBefore); i++)
        if (common_bool[i]) {
            *outgram++ = (char)(gBefore + i);
            lengram++;
        }
        *outgram++ = '\x00';
        return lengram;
}

// Singleton index of morphological info
const TGrammarIndex TGrammarIndex::TheIndex;

void TGrammarIndex::RegisterGrammeme(TGrammar code, const char* name, const char* latin_name, TGramClass grClass) {

    const size_t n = static_cast<size_t> (code);
    if (YandexNameByCodeIndex.size() <= n) {
        YandexNameByCodeIndex.resize(n + 1);
        Utf8NameByCodeIndex.resize(n + 1);
        LatinNameByCodeIndex.resize(n + 1);
        WNameByCodeIndex.resize(n + 1);
        WLatinNameByCodeIndex.resize(n + 1);
        ClassByCodeIndex.resize(n + 1);
    }

    WNameByCodeIndex[n] = UTF8ToWide(name);
    WLatinNameByCodeIndex[n] = UTF8ToWide(latin_name);

    Utf8NameByCodeIndex[n] = name;
    YandexNameByCodeIndex[n] = WideToChar(WNameByCodeIndex[n], CODES_YANDEX);
    LatinNameByCodeIndex[n] = latin_name;
    ClassByCodeIndex[n] = grClass;

    CodeByNameIndex[Utf8NameByCodeIndex[n]] = code;
    CodeByNameIndex[YandexNameByCodeIndex[n]] = code;
    CodeByNameIndex[LatinNameByCodeIndex[n]] = code;

    WCodeByNameIndex[WNameByCodeIndex[n]] = code;
    WCodeByNameIndex[WLatinNameByCodeIndex[n]] = code;

    TClassToCodes::iterator it = CodesByClassIndex.find(grClass);
    if (it == CodesByClassIndex.end())
        it = CodesByClassIndex.insert(TClassToCodes::value_type(grClass, yvector<TGrammar>())).first;
    (*it).second.push_back(code);
}

TGrammarIndex::TGrammarIndex()
    : YandexNameByCodeIndex(256)
    , Utf8NameByCodeIndex(256)
    , LatinNameByCodeIndex(256)
    , WNameByCodeIndex(256)
    , WLatinNameByCodeIndex(256)
    , ClassByCodeIndex(256)
{
// Части речи
    RegisterGrammeme(gAdjective, "A", "A", bNom);
    RegisterGrammeme(gAdverb, "ADV", "ADV", bNom);
    RegisterGrammeme(gComposite, "COM", "COM", bNom);
    RegisterGrammeme(gConjunction, "CONJ", "CONJ", bNom);
    RegisterGrammeme(gInterjunction, "INTJ", "INTJ", bNom);
    RegisterGrammeme(gNumeral, "NUM", "NUM", bNom);
    RegisterGrammeme(gParticle, "PART", "PART", bNom);
    RegisterGrammeme(gPreposition, "PR", "PR", bNom);
    RegisterGrammeme(gSubstantive, "S", "S", bNom);
    RegisterGrammeme(gVerb, "V", "V", bNom);
    RegisterGrammeme(gAdjNumeral, "ANUM", "ANUM", bNom);
    RegisterGrammeme(gAdjPronoun, "APRO", "APRO", bNom);
    RegisterGrammeme(gAdvPronoun, "ADVPRO", "ADVPRO", bNom);
    RegisterGrammeme(gSubstPronoun, "SPRO", "SPRO", bNom);
    RegisterGrammeme(gArticle, "ART", "ART", bNom);
    RegisterGrammeme(gPostposition, "POSTP", "POSTP", bNom );
    RegisterGrammeme(gPartOfIdiom, "идиом", "idiom", bNom); // части идиом (прежде всего иностр. слов)
// Особые пометы
    RegisterGrammeme(gReserved, "reserved", "reserved", bUniq);
    RegisterGrammeme(gIrregularStem, "нерег", "irreg", bUniq); // чередования в основе или супплетивизм
    RegisterGrammeme(gContracted, "стяж", "contr", bUniq); // стяжённые формы (фр. qu', ит. quell')
    RegisterGrammeme(gAbbreviation, "сокр", "abbr", bUniq); // сокращения
    RegisterGrammeme(gObscene, "обсц", "obsc", bUniq);
    RegisterGrammeme(gRare, "редк", "rare", bUniq);
    RegisterGrammeme(gAwkward, "затр", "awkw", bUniq);
    RegisterGrammeme(gObsolete, "устар", "obsol", bUniq);
    RegisterGrammeme(gSubstAdjective, "адъект", "adj", bUniq);
    RegisterGrammeme(gPraedic, "прдк", "praed", bPrdc);
    RegisterGrammeme(gInformal, "разг", "inform", bUniq);
    RegisterGrammeme(gDistort, "искаж", "dist", bUniq);
    RegisterGrammeme(gParenth, "вводн", "parenth", bPrnth);
    // Классы имен собственных
    RegisterGrammeme(gFirstName, "имя", "persn", bNS);
    RegisterGrammeme(gSurname, "фам", "famn", bNS);
    RegisterGrammeme(gPatr, "отч", "patrn", bNS);
    RegisterGrammeme(gGeo, "гео", "geo", bNS);
    RegisterGrammeme(gProper, "собств", "proper", bNS);
// Время
    RegisterGrammeme(gPresent, "наст", "praes", bTemp);  // Tempus
    RegisterGrammeme(gNotpast, "непрош", "inpraes", bTemp);
    RegisterGrammeme(gPast, "прош", "praet", bTemp);
    RegisterGrammeme(gFuture, "буд", "fut", bTemp);         // будущее (фр., ит.)
    RegisterGrammeme(gPast2, "прош2", "aor", bTemp);        // passe simple, passato remoto
// Падеж
    RegisterGrammeme(gNominative, "им", "nom", bCas);    // Casus
    RegisterGrammeme(gGenitive, "род", "gen", bCas);
    RegisterGrammeme(gDative, "дат", "dat", bCas);
    RegisterGrammeme(gAccusative, "вин", "acc", bCas);
    RegisterGrammeme(gInstrumental, "твор", "ins", bCas);
    RegisterGrammeme(gAblative, "пр", "abl", bCas);
    RegisterGrammeme(gVocative, "зват", "voc", bCas);
    RegisterGrammeme(gPartitive, "парт", "part", bCas);
    RegisterGrammeme(gLocative, "местн", "loc", bCas);
    RegisterGrammeme(gElative, "исх", "elat", bCas);
    RegisterGrammeme(gCount, "счетн", "count", bCas);
// Число
    RegisterGrammeme(gSingular, "ед", "sg", bNum);    // Numerus
    RegisterGrammeme(gPlural, "мн", "pl", bNum);
    RegisterGrammeme(gDual, "дв", "dual", bNum);
    RegisterGrammeme(gAssocPlural, "ассоц-мн", "apl", bNum);
// Форма глагола
    RegisterGrammeme(gGerund, "деепр", "ger", bMod); // Modus
    RegisterGrammeme(gInfinitive, "инф", "inf", bUniq);
    RegisterGrammeme(gParticiple, "прич", "partcp", bUniq);
    RegisterGrammeme(gIndicative, "изъяв", "indic", bMod);
    RegisterGrammeme(gImperative, "пов", "imper", bMod);
    RegisterGrammeme(gConditional, "усл", "cond", bMod);    // условное наклонение
    RegisterGrammeme(gSubjunctive, "сослаг", "subj", bMod); // сослагательное наклонение
    RegisterGrammeme(gObligatory, "долж", "oblig", bMod);
    RegisterGrammeme(gOptative, "опт", "opt", bMod);
    RegisterGrammeme(gConverb, "конверб", "converb", bUniq);
// Степень
    RegisterGrammeme(gShort, "кр", "brev", bRepr);    // Gradus
    RegisterGrammeme(gFull, "полн", "plen", bRepr);    // Gradus

    RegisterGrammeme(gSuperlative, "прев", "supr", bGrad);
    RegisterGrammeme(gComparative, "срав", "comp", bGrad);

    RegisterGrammeme(gPossessive, "притяж", "poss", bPoss);
// Лицо
    RegisterGrammeme(gPerson1, "1-л", "1p", bPers);   // Personae
    RegisterGrammeme(gPerson2, "2-л", "2p", bPers);
    RegisterGrammeme(gPerson3, "3-л", "3p", bPers);
// Род
    RegisterGrammeme(gFeminine, "жен", "f", bGend);   // Gender (genus)
    RegisterGrammeme(gMasculine, "муж", "m", bGend);
    RegisterGrammeme(gNeuter, "сред", "n", bGend);
    RegisterGrammeme(gMasFem, "мж", "mf", bGend);
// Аспект
    RegisterGrammeme(gPerfect, "сов", "pf", bPerf);   // Aspect
    RegisterGrammeme(gImperfect, "несов", "ipf", bPerf);
// Залог
    RegisterGrammeme(gPassive, "страд", "pass", bVox); // Voice (Genus)
    RegisterGrammeme(gActive, "действ", "act", bVox); // Voice (Genus)
    RegisterGrammeme(gReflexive, "возвр", "reflex", bVox);
    RegisterGrammeme(gImpersonal, "безл", "impers", bVox);
    RegisterGrammeme(gMedial, "мед", "med", bVox);
// Одушевленность
    RegisterGrammeme(gAnimated, "од", "anim", bAnim);    // Animated
    RegisterGrammeme(gInanimated, "неод", "inan", bAnim);
    RegisterGrammeme(gHuman, "человек", "hum", bAnim);
// Переходность
    RegisterGrammeme(gTransitive, "пе", "tran", bTran);
    RegisterGrammeme(gIntransitive, "нп", "intr", bTran);
// Определенность
    RegisterGrammeme(gDefinite, "опред", "det", bDet);      // определенный артикль
    RegisterGrammeme(gIndefinite, "неопр", "indet", bDet);  // неопределенный артикль
// Possessiveness
    RegisterGrammeme(gPoss1p, "притяж-1л", "poss-1p", bPoss);
    RegisterGrammeme(gPoss1pSg, "притяж-1л-ед", "poss-1p-sg", bPoss);
    RegisterGrammeme(gPoss1pPl, "притяж-1л-мн", "poss-1p-pl", bPoss);
    RegisterGrammeme(gPoss2p, "притяж-2л", "poss-2p", bPoss);
    RegisterGrammeme(gPoss2pSg, "притяж-2л-ед", "poss-2p-sg", bPoss);
    RegisterGrammeme(gPoss2pPl, "притяж-2л-мн", "poss-2p-pl", bPoss);
    RegisterGrammeme(gPoss3p, "притяж-3л", "poss-3p", bPoss);
    RegisterGrammeme(gPoss3pSg, "притяж-3л-ед", "poss-3p-sg", bPoss);
    RegisterGrammeme(gPoss3pPl, "притяж-3л-мн", "poss-3p-pl", bPoss);
// Predicativeness
    RegisterGrammeme(gPredic1pSg, "сказ-1л-ед", "predic-1p-sg", bPredic);
    RegisterGrammeme(gPredic1pPl, "сказ-1л-мн", "predic-1p-pl", bPredic);
    RegisterGrammeme(gPredic2pSg, "сказ-2л-ед", "predic-2p-sg", bPredic);
    RegisterGrammeme(gPredic2pPl, "сказ-2л-мн", "predic-2p-pl", bPredic);
    RegisterGrammeme(gPredic3pSg, "сказ-3л-ед", "predic-3p-sg", bPredic);
    RegisterGrammeme(gPredic3pPl, "сказ-3л-мн", "predic-3p-pl", bPredic);
    RegisterGrammeme(gPredicPoss, "сказ-притяж", "predic-poss", bPredic);
    RegisterGrammeme(gPredic, "сказ", "predic", bPredic);
//  Part-of-speech conversions within the paradigm
    RegisterGrammeme(gDerivedAdjective, "deriv-A", "deriv-A", bDerivNom);
//
    RegisterGrammeme(gEvidential, "заглаз", "evid", bUniq);
    RegisterGrammeme(gNegated, "отриц", "neg", bUniq);
    RegisterGrammeme(gPotential, "потенц", "poten", bUniq);
// Armenian converb classes
    RegisterGrammeme(gSimultaneous, "одновр", "simul",     bConvClass);
    RegisterGrammeme(gConnegative, "соотриц", "conneg",  bConvClass);
    RegisterGrammeme(gResultative, "результ", "result",  bConvClass);
    RegisterGrammeme(gSubjective, "субъект", "subject", bConvClass);

    RegisterGrammeme(gGoodForm, "хор", "good", bUniq);

    RegisterGrammeme(gIntention, "намер", "intent", bUniq);
    RegisterGrammeme(gHonorific, "уваж", "hon", bUniq);
    RegisterGrammeme(gEquative, "экв", "equat", bUniq);
}
