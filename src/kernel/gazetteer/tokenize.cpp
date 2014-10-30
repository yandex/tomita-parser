#include "tokenize.h"
#include <kernel/gazetteer/base.pb.h>


namespace NGzt
{
    TRetokenizer::TRetokenizer(const TSearchKey& key)
        : Key(&key)
    {
    }

    TRetokenizer::~TRetokenizer() {
    }

    bool TRetokenizer::Do(Wtroka& key_text) {
        bool retokenized = false;
        KeyText = key_text;
        if (Key->tokenize().delim() == TTokenizeOptions::NON_ALNUM)
            retokenized = RetokenizeKey<false>();
        else if (Key->tokenize().delim() == TTokenizeOptions::CHAR_CLASS) {
            retokenized = RetokenizeKey<true>();
        }
        if (retokenized && HasWords()) {
            FixedKey.Reset(Key->New());
            FixedKey->CopyFrom(*Key);
            FixWords();
        }
        key_text = KeyText;
        return retokenized;
    }

    bool TRetokenizer::NextWordBegins(bool originalSpace) {
        if (originalSpace) {
            WordsMap.push_back(CurWord + WordsMapShift);
            CurWord++;
        } else {
            WordsMapShift++;
            return true;
        }
        return false;
    }

    template<bool doCheckCharClass>
    bool TRetokenizer::CheckCurrentSymbol() {
        bool res = false;
        if (CheckAlnum()) {
            res = true;
        }
        if (doCheckCharClass && CheckNextCharClass()) {
            res = true;
        }
        return res;
    }

    template<bool doCheckCharClass>
    bool TRetokenizer::RetokenizeKey() {
        bool retokenized = false;
        CurWord = 0;
        WordsMapShift = 0;
        WordsMap.clear();
        bool originalSpace = true;
        bool modified = false;
        for (TextPos = 0; TextPos < KeyText.size(); ++TextPos) {
            if (KeyText[TextPos] == SPACE) {
                originalSpace = true;
                PrevCharClass = GetTokCharClass();
            } else {
                if (CheckCurrentSymbol<doCheckCharClass>())
                    modified = true; // we are looking on the space now
                else if (modified || originalSpace) {
                    retokenized |= NextWordBegins(originalSpace);
                    modified = false;
                    originalSpace = false;
                }
            }
        }
        NextWordBegins(originalSpace);
        return retokenized;
    }

    bool TRetokenizer::CheckAlnum() {
        if (!IsAlnum(KeyText[TextPos]) && KeyText[TextPos] != SPACE) {
            if (IsTokenMarker()) // should preserve special markers at token beginning (!a, $a)
                return false;
            KeyText.begin()[TextPos] = SPACE;
            return true;
        }
        return false;
    }

    bool TRetokenizer::CheckNextCharClass() { // changed PrevCharClass, do it on each non-space symbol
        if (TextPos == 0) {
            PrevCharClass = GetTokCharClass();
            return false;
        }
        CurCharClass = GetTokCharClass();
        if (CurCharClass != PrevCharClass && CurCharClass != WHITESPACE && PrevCharClass != WHITESPACE) {
            KeyText.insert(TextPos, 1, SPACE);
            PrevCharClass = WHITESPACE;
            return true;
        } else
            PrevCharClass = CurCharClass;
        return false;
    }


    TRetokenizer::ETokCharClass TRetokenizer::GetTokCharClass() {
        wchar16 a = KeyText[TextPos];
        if (IsAlpha(a))
            return ALPHA;
        else if (IsWhitespace(a))
            return WHITESPACE;
        else if (IsDigit(a))
            return DIGIT;
        else if (IsTokenMarker())
            return WHITESPACE;
        else
            return PUNCT;
    }

    bool TRetokenizer::HasWords() const {
        if (HasWords(Key->morph()))
            return true;
        if (HasWords(Key->gram()))
            return true;
        if (HasWords(Key->case_()))
            return true;
        if (HasWords(Key->agr()))
            return true;
        return false;
    }

    template <class T>
    bool TRetokenizer::HasWords(const T& trg) const {
        for (int i = 0; i < trg.size(); ++i)
            if (trg.Get(i).wordSize() > 0)
                return true;
        return false;
    }


    void TRetokenizer::FixWords() {
        FixWords(FixedKey->mutable_morph());
        FixWords(FixedKey->mutable_gram());
        FixWords(FixedKey->mutable_case_());
        FixWords(FixedKey->mutable_agr());
    }

    size_t TRetokenizer::CorrectWordIndex(size_t index) {
        if (index == 0 || index > WordsMap.size())
            return index;
        else
            return WordsMap[index - 1] + 1;
    }

    template <class T>
    void TRetokenizer::FixWords(T* trg) {
        for (int i = 0; i < trg->size(); ++i) {
            for (size_t j = 0; j < trg->Get(i).wordSize(); ++j) {
                size_t index = trg->Get(i).Getword(j);
                size_t correctWordIndex = CorrectWordIndex(index);
                if (correctWordIndex != index) {
                    trg->Mutable(i)->Setword(j, correctWordIndex);
                }
            }
        }
    }

} // namespace NGzt
