#pragma once

#include <util/generic/vector.h>

#include "group.h"
#include <FactExtract/Parser/afdocparser/rus/word.h>

class CPrimitiveGroup :
    public CGroup
{
public:
    CPrimitiveGroup(const CWord* pWord, int iW)
        : CGroup(iW)
        , m_pWord(pWord)
        , m_ActiveHomonyms(0)
    {
        ResetGrammems();
        m_AutomatSymbolInterpetationUnion = m_pWord->m_AutomatSymbolInterpetationUnion;
    }

    virtual const CGroup* GetMainGroup() const {
        return this;
    }

    virtual const CWord* GetMainWord() const {
        return m_pWord;
    }

    virtual CGroup* Clone();

    virtual const THomonymGrammems& GetGrammems() const {
        return Grammems;
    }

    virtual void SetGrammems(const THomonymGrammems&) {
        ythrow yexception() << "Cannot set grammems of primitive group.";
    }

    virtual int GetMainTerminalSymbol() const {
        return GetActiveSymbol();
    }

    virtual CPrimitiveGroup* GetMainPrimitiveGroup() {
        return this;
    }

    virtual const CPrimitiveGroup* GetMainPrimitiveGroup() const {
        return this;
    }

    virtual void SetActiveSymbol(int iS);

    bool ContainsHomonym(int iH) {
        return m_ActiveHomonyms & (1 << iH);
    }

    virtual void GetMainClauseWord(int& iClauseWord, int& iH) const;

    virtual ui32 GetMainHomIDs() const {
        return m_ActiveHomonyms;
    }

    virtual bool IsPrimitive() const {
        return true;
    }

    virtual void IntersectChildHomonyms(const TChildHomonyms& homs);

    virtual TChildHomonyms GetChildHomonyms() const {
        //YASSERT(m_ActiveHomonyms != 0);
        return TChildHomonyms(&m_ActiveHomonyms, &m_ActiveHomonyms + 1);
    }

    virtual Stroka DebugString() const;

private:
    void ResetGrammems();

private:
    const CWord* m_pWord;
    THomonymGrammems Grammems;

    //храним битиками омонимы, соотвтествующие этому терминалу (не больше 32)
    THomMask m_ActiveHomonyms;
};
