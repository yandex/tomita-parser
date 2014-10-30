// Primitive.cpp: implementation of the CPrimitive & CPrimGroup classes.
//////////////////////////////////////////////////////////////////////////////

#include "primitive.h"

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CPrimGroup::CPrimGroup(void)
{
    m_gtyp = UnknownPrim;
    m_pos = 0;
    m_len = 0;
    m_break = NoBreak;
}

CPrimGroup::CPrimGroup(const CPrimitive &prim)
{
    m_gtyp = prim.m_typ;
    m_pos  = prim.m_pos;
    m_len  = prim.m_len;
    m_prim.push_back(prim);

    m_break = NoBreak;
}

CPrimGroup::~CPrimGroup(void)
{
}

//////////////////////////////////////////////////////////////////////////////
// has...

bool CPrimGroup::hasFirstDigits(void)
{
    return (m_prim.size() > 0) && (m_prim[0].m_typ==Digit);
}

bool CPrimGroup::hasDigits(void)
{
    for (size_t i=0; i<m_prim.size(); i++) {
        if (m_prim[i].m_typ==Digit)
            return true;
    }
    return false;
}

bool CPrimGroup::hasWords(void)
{
    for (size_t i=0; i<m_prim.size(); i++) {
        if (m_prim[i].m_typ==Word)
            return true;
    }
    return false;
}

bool CPrimGroup::hasAltWords(void)
{
    for (size_t i=0; i<m_prim.size(); i++) {
        if (m_prim[i].m_typ==AltWord)
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// getETypeUid

Stroka CPrimGroup::getETypeUid(ETypeOfPrim type)
{
    switch (type) {
        case NewLine:    return "N";
        case Space:        return "Z";
        case Punct:        return "P";
        case Symbol:    return "S";
        case Digit:        return "D";
        case Word:        return "W";
        case AltWord:    return "A";

        case Initial:    return "I";
        case AltInitial:    return "aI";

        case Abbrev:    return "AB";
        case AbbEos:    return "AE";
        case AbbDig:    return "AD";
        case AbbAlt:    return "ABa";
        case AbbEosAlt:    return "AEa";
        case AbbDigAlt:    return "ADa";

        case Complex:    return "C";
        case Hyphen:    return "H";
        case DivWord:    return "Wd";
        case HypDiv:    return "HD";
        case DigHyp:    return "Dh";
        case ComplexDigit:    return "Cd";
        case ComplexDigitHyp:    return "CDh";
        case WordShy:    return "Shy";

        default:        return "u";
    }
}

//////////////////////////////////////////////////////////////////////////////
