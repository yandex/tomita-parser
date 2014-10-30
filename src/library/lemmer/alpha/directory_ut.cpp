#include <library/unittest/registar.h>

#include "abc.h"

namespace NDirectoryTest {

static const size_t MAX_WORD_LENGTH = 6;

// Chinese/Japanese/Korean words
static const size_t HanWordsCount = 10;
static const TChar HanWords[][MAX_WORD_LENGTH] = {
    {0x5988, 0, 0, 0, 0, 0},
    {0x9ebb, 0, 0, 0, 0, 0},
    {0x9a6c, 0, 0, 0, 0, 0},
    {0x725b, 0, 0, 0, 0, 0},
    {0x4e3a, 0x4ec0, 0x4e48, 0, 0, 0},
    {0x6905, 0x5b50, 0, 0, 0, 0},
    {0x87f9, 0, 0, 0, 0, 0},
    {0x9762, 0x5305, 0, 0, 0, 0},
    {0x2ff0, 0x6c34, 0x2ff1, 0x53e3, 0x5ddb, 0},
    {0x2ff1, 0x4e95, 0x86d9, 0, 0, 0}
};

// Chinese phonetic alphabet words
static const size_t BopomofoWordsCount = 4;
static const TChar BopomofoWords[][MAX_WORD_LENGTH] = {
    {0x3105, 0x311a, 0, 0, 0, 0},
    {0x3114, 0x3128, 0, 0, 0, 0},
    {0x3109, 0x3127, 0x311d, 0, 0, 0},
    {0x3127, 0x3125, 0, 0, 0, 0}
};

// Japanese alphabet words
static const size_t HiraganaWordsCount = 4;
static const TChar HiraganaWords[][MAX_WORD_LENGTH] = {
    {0x3044, 0x3059, 0, 0, 0, 0},
    {0x304b, 0x304d, 0, 0, 0, 0},
    {0x306f, 0x3044, 0, 0, 0, 0},
    {0x3044, 0x3044, 0x3048, 0, 0, 0}
};

// Japanese phonetic alphabet words
static const size_t KatakanaWordsCount = 4;
static const TChar KatakanaWords[][MAX_WORD_LENGTH] = {
    {0x30c6, 0x30ec, 0x30d3, 0, 0, 0},
    {0x30a2, 0x30e1, 0x30ea, 0x30ab, 0, 0},
    {0x30c1, 0x30e3, 0x30fc, 0x30cf, 0x30f3, 0},
    {0x30b3, 0x30fc, 0x30d2, 0x30fc, 0, 0}
};

// Korean words
static const size_t HangulWordsCount = 4;
static const TChar HangulWords[][MAX_WORD_LENGTH] = {
    {0xd55c, 0xae00, 0, 0, 0, 0},
    {0xb099, 0, 0, 0, 0, 0},
    {0xb9cc, 0xbc29, 0, 0, 0, 0},
    {0xbd88, 0xace9, 0xae30, 0, 0, 0}
};

class TClassifyLanguageTest : public TTestBase {
    UNIT_TEST_SUITE(TClassifyLanguageTest);
        UNIT_TEST(TestChinese);
        UNIT_TEST(TestJapanese);
        UNIT_TEST(TestKorean);
    UNIT_TEST_SUITE_END();

private:
    void TestWords(const TChar words[][MAX_WORD_LENGTH],
                   size_t wordsCount,
                   docLanguage language);


    void TestChinese();
    void TestJapanese();
    void TestKorean();
};


void TClassifyLanguageTest::TestWords(const TChar words[][MAX_WORD_LENGTH],
                                      size_t wordsCount,
                                      docLanguage language)
{
    for (size_t wordIndex = 0; wordIndex < wordsCount; ++wordIndex) {
        size_t wordLength = TCharTraits<TChar>::GetLength(words[wordIndex]);
        TLangMask mask =
            NLemmer::ClassifyLanguageAlpha(words[wordIndex], wordLength, true);
        UNIT_ASSERT(mask.HasAny(TLangMask(language)));
    }
}

void TClassifyLanguageTest::TestChinese() {
    TestWords(HanWords, HanWordsCount, LANG_CHI);
    TestWords(BopomofoWords, BopomofoWordsCount, LANG_CHI);
}

void TClassifyLanguageTest::TestJapanese() {
    TestWords(HanWords, HanWordsCount, LANG_JPN);
    TestWords(HiraganaWords, HiraganaWordsCount, LANG_JPN);
    TestWords(KatakanaWords, KatakanaWordsCount, LANG_JPN);
}

void TClassifyLanguageTest::TestKorean() {
    TestWords(HanWords, HanWordsCount, LANG_KOR);
    TestWords(HangulWords, HangulWordsCount, LANG_KOR);
}

UNIT_TEST_SUITE_REGISTRATION(TClassifyLanguageTest);


} // NDirectoryTest
