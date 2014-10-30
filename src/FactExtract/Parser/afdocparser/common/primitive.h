#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>


enum ETypeOfPrim
{
// Primitive
    UnknownPrim, // u-group
    NewLine,    // N
    Space,        // Z group
    Punct,        // P
    Symbol,        // S group
    Digit,        // D group
    Word,        // T&t group
    AltWord,    // A&a group

// PrimGroup
    Initial,    // <П.> or <Ив.>
    AltInitial,    // <I.> or <Jn.>
    Abbrev,        // abbreviature "прим." "т.д." not EOS
    AbbEos,        // abbreviature "прим." "т.д." EOS
    AbbDig,        // abbreviature type 2
    AbbAlt,
    AbbEosAlt,
    AbbDigAlt,

    Complex,    // Mixed: R123-X#17бис start&stop is T|A|D,
                //  inside may be single P('.'|'-') or single Z

    Hyphen,        // Word-with-hyphen(s) <->
    DivWord,    // Word-with-division  <-\n>
    HypDiv,        // Both: красно-сине-зеле-\nный
    DigHyp,        // DigitsOnly with-hyphen(s) <->
    ComplexDigit, //всякая херь, типа "15 300 000", "$50 000",  "50%", "1,4"
    ComplexDigitHyp, //всякая херь, типа "0,5-0,6"
    WordShy,    // изве&shy;стно
};

//////////////////////////////////////////////////////////////////////////////
// ETypeOfBreak

enum ETypeOfBreak
{
    NoBreak,
    SentBreak,
    ParaBreak,
    WordBreak,
};

//////////////////////////////////////////////////////////////////////////////
// CPrimitive

class CPrimitive
{
public:
    CPrimitive(void)
        : m_typ(UnknownPrim), m_pos(0), m_len(0) {}
    CPrimitive(ETypeOfPrim type,size_t pos,size_t len)
        : m_typ(type), m_pos(pos), m_len(len) {}
    virtual ~CPrimitive(void) {}

// Data
public:
    ETypeOfPrim m_typ;
    size_t  m_pos;
    size_t  m_len;

// Code
public:
    bool isWA() {
        return m_typ == Word || m_typ == AltWord;
    }
    bool isDWA() {
        return m_typ == Digit || m_typ == Word || m_typ == AltWord;
    }
    bool isNZ() {
        return m_typ == NewLine || m_typ == Space;
    }
};

//////////////////////////////////////////////////////////////////////////////
// CPrimGroup

class CPrimGroup
{
public:
    CPrimGroup(void);
    CPrimGroup(const CPrimitive &prim);
    virtual ~CPrimGroup(void);

// Data
public:
    ETypeOfPrim m_gtyp;
    size_t  m_pos;
    size_t  m_len;
    yvector<CPrimitive> m_prim;
    ETypeOfBreak m_break;

    Wtroka m_str;

// Code
public:
    bool isWA()    {
        return m_gtyp == Word || m_gtyp == AltWord;
    }
    bool isDWA() {
        return m_gtyp == Digit || m_gtyp == Word || m_gtyp == AltWord;
    }

    bool isNewLine() {
        return m_gtyp == NewLine;
    }
    bool isSpace() {
        return m_gtyp == Space;
    }
    bool isNZ() {
        return m_gtyp == NewLine || m_gtyp == Space;
    }
    bool isPS() {
        return m_gtyp == Punct || m_gtyp == Symbol;
    }

    bool hasFirstD() {
        return m_prim[0].m_typ == Digit;
    }
    bool hasFirstW() {
        return m_prim[0].m_typ == Word;
    }
    bool hasFirstA() {
        return m_prim[0].m_typ == AltWord;
    }
    bool hasFirstWA() {
        return hasFirstW() || hasFirstA();
    }
    bool hasFirstDWA() {
        return hasFirstD() || hasFirstW() || hasFirstA();
    }

    bool isForMorph() {
        return isWA() || m_gtyp == Hyphen || m_gtyp == DivWord || m_gtyp == HypDiv || m_gtyp == WordShy;
    }

    bool hasDigits();
    bool hasFirstDigits();
    bool hasWords();
    bool hasAltWords();

    bool isEOS() {
        return m_gtyp == AbbEos || m_gtyp == AbbEosAlt || m_gtyp == AbbDig || m_gtyp == AbbDigAlt;
    }

    static Stroka getETypeUid(ETypeOfPrim type); // temporary - for debug
};

