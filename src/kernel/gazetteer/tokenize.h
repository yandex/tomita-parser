#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

#include <util/string/vector.h>

class TSearchKey;

namespace NGzt
{

static const wchar16 SPACE = ' ';
static const wchar16 EXACT_FORM_MARKER = '!';
static const wchar16 ANY_WORD_WILDCARD = '*';

const char ARTICLE_MARKER = '$';

static inline bool IsMarkerChar(const wchar16 ch) {
    return ch == EXACT_FORM_MARKER || ch == ARTICLE_MARKER;
}

static inline bool HasExactFormMarker(const TWtringBuf& word) {
    return word.length() >= 2 && word[0] == EXACT_FORM_MARKER;
}


static inline bool HasArticleMarker(const TWtringBuf& word) {
    return word.length() >= 2 && word[0] == ARTICLE_MARKER;
}


static inline bool IsAnyWord(const TWtringBuf& word) {
    return word.size() == 1 && word[0] == ANY_WORD_WILDCARD;
}

class TRetokenizer {
public:
    enum ETokCharClass { ALPHA, DIGIT, PUNCT, WHITESPACE };

    TRetokenizer(const TSearchKey& key);
    ~TRetokenizer();

    const TSearchKey& GetKey() {
         if (!!FixedKey) {
             return *FixedKey;
         } else {
             return *Key;
        }
    }


    bool Do(Wtroka& key_text); //checks and applies the key tokenize options

private:

    bool IsTokenMarker() {
        return IsMarkerChar(KeyText[TextPos]) && (TextPos == 0 || KeyText[TextPos - 1] == SPACE)
            && TextPos + 1 < KeyText.size() && IsAlnum(KeyText[TextPos + 1]);
    }

    bool NextWordBegins(bool wasOriginalSpace);

    template <bool doCheckCharClass>
    bool RetokenizeKey(); // constructs WordsMap; do CheckAlnum, CheckNextCharClass

    template <bool doCheckCharClass>
    bool CheckCurrentSymbol();

    bool CheckAlnum();

    bool CheckNextCharClass();


    ETokCharClass GetTokCharClass();

    bool HasWords() const;

    template<class T>
    bool HasWords(const T& trg) const;


    void FixWords();

    size_t CorrectWordIndex(size_t index);

    template<class T>
    void FixWords(T* trg);


private:
    const TSearchKey* Key;
    THolder<TSearchKey> FixedKey;
    Wtroka KeyText;

    yvector<size_t> WordsMap; // wordsMap[i] = j means that after retokenization i-th word of the text becomes j-th word.
                              // Example: for "Alpha-Centauri explosion" wordsMap[0] = 0, wordsMap[1] = 2, because "Centauri" becomes second word with index 1.
    size_t TextPos;
    size_t CurWord;
    size_t WordsMapShift;

    ETokCharClass CurCharClass;
    ETokCharClass PrevCharClass;

};

} // namespace NGzt
