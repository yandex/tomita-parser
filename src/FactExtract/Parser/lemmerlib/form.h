#pragma once

#include "lemma.h"

class TSimpleLemma;

class TSimpleWordformKit {
public:
    TSimpleWordformKit(const TSimpleLemma& lemma, size_t flexIndex);
    TSimpleWordformKit(const TSimpleLemma& lemma);

    bool IsEmpty() const {
        return Lemma == NULL;
    }

    const char* GetStemGram() const {
        return (Lemma != NULL) ? Lemma->GetStemGram() : NULL;
    }

    const char* GetFlexGram() const;

    size_t CommonPrefixLength(const TSimpleWordformKit& kit) const {
        UNUSED(kit);
        return 0;
    }

    TWtringBuf ConstructText(TChar* buffer, size_t bufferSize) const;
    void ConstructText(Wtroka& text) const;
    void ConstructForm(TSimpleWordform& form) const;

protected:
    inline TSimpleWordformKit()
        : Lemma(NULL)
        , FormHandle(NULL)
        , FlexGrammar(NULL) {
    }

    void SetDefaultText(const TSimpleLemma& lemma, const TWtringBuf& text);
    void SetForm(MystemFormHandle* form, size_t flexPos = 0);

    friend class TSimpleWordformIterator;
    friend class TWordformCharIterator;

protected:
    const TSimpleLemma* Lemma;      // should outlive @this
    MystemFormHandle* FormHandle;
    Wtroka DefaultText;

    const char* FlexGrammar;
};


// base interface for wordform iterators
class ISimpleWordformIterator {
public:
    virtual inline ~ISimpleWordformIterator() {
    }

    virtual void operator++() = 0;
    virtual bool IsValid() const = 0;

    inline const TSimpleWordformKit& operator*() const {
        return GetValue();
    }

    inline const TSimpleWordformKit* operator->() const {
        return &GetValue();
    }

private:
    virtual const TSimpleWordformKit& GetValue() const = 0;
};

class TSimpleWordformIterator: public ISimpleWordformIterator {
public:
    explicit TSimpleWordformIterator(const TSimpleLemma& lemma);

    virtual ~TSimpleWordformIterator();

    // implements IWordformIterator

    virtual inline bool IsValid() const {
        return Current != NULL;
    }

    virtual void operator++();

private:
    virtual inline const TSimpleWordformKit& GetValue() const {
        YASSERT(Current != NULL);
        return *Current;
    }

    TSimpleWordformKit Lemma, Form;
    TSimpleWordformKit* Current;
    MystemFormsHandle* FormsHandle;
    size_t FormsCount;
    size_t FormsPos;
    size_t FlexPos;

    DECLARE_NOCOPY(TSimpleWordformIterator);
};

typedef TSimpleWordformKit TWordformKit;
typedef TSimpleWordformIterator TWordformKitIterator;

namespace NLemmer {
    class TGrammarFiltr {
    public:
        virtual void SetLemma(const TSimpleLemma& lemma) = 0;
        virtual bool IsProperStemGr() const = 0;
        virtual bool CheckFlexGr(const char* flexGram) const = 0;
        virtual TGrammarFiltr* Clone() const = 0;
        virtual ~TGrammarFiltr() {}
    };

    class TDummyGrammarFiltr: public TGrammarFiltr {
    public:
        TDummyGrammarFiltr()
            : IsProper(false)
        {}
        void SetLemma(const TSimpleLemma&) {
            IsProper = true;
        }
        bool IsProperStemGr() const {
            return IsProper;
        }
        bool CheckFlexGr(const char*) const {
            return IsProper;
        }
        TDummyGrammarFiltr* Clone() const {
            return new TDummyGrammarFiltr(*this);
        }
        ~TDummyGrammarFiltr() {};
    private:
        bool IsProper;
    };

    class TClueGrammarFiltr: public TGrammarFiltr {
    public:
        TClueGrammarFiltr(const Stroka neededGr[]);
        TClueGrammarFiltr(const char* neededGr);
        void SetLemma(const TSimpleLemma& lemma);
        bool IsProperStemGr() const {
            return !FlexGram.empty();
        }
        bool CheckFlexGr(const char* flexGram) const;
        TClueGrammarFiltr* Clone() const {
            return new TClueGrammarFiltr(*this);
        }
        ~TClueGrammarFiltr() {}
    private:
        yvector<Stroka> Grammar;
        yvector<Stroka> FlexGram;
    };


    class TFormGenerator {
    public:
        TFormGenerator(const TSimpleLemma& lemma, const TGrammarFiltr* filter = NULL)
            : Filter(filter ? filter->Clone() : new NLemmer::TDummyGrammarFiltr) {
            UNUSED(lemma);
        }

        // Iterator interface --------------------

        inline bool IsValid() const {
            return ItPtr.Get() != NULL && ItPtr->IsValid();
        }

        inline void operator++() {
            ItPtr->operator++();
            FilterNext();
        }

        inline const TSimpleWordformKit& operator*() const {
            return ItPtr->operator*();
        }

        inline const TSimpleWordformKit* operator->() const {
            return ItPtr->operator->();
        }

        // Generator interface --------------------

        bool GenerateNext(TSimpleWordform& form);

    protected:
        void FilterNext() {
            while (ItPtr->IsValid() && !Filter->CheckFlexGr((*ItPtr)->GetFlexGram()))
                ItPtr->operator++();
        }

        THolder<TGrammarFiltr> Filter;
        THolder<ISimpleWordformIterator> ItPtr;
    };

    class TDefaultFormGenerator: public TFormGenerator {
    public:

        TDefaultFormGenerator(const TSimpleLemma& lemma, const TGrammarFiltr* filter = NULL)
            : TFormGenerator(lemma, filter) {
            Filter->SetLemma(lemma);
            if (Filter->IsProperStemGr()) {
                ItPtr.Reset(new TSimpleWordformIterator(lemma));
                FilterNext();
            }
        }
    };

}
