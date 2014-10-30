#pragma once

#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/common/inputitem.h>

#include <library/lemmer/dictlib/grammarhelper.h>
using NSpike::TGrammarBunch;

class CWord;
class CHomonym;
class THomonymGrammems;
class CPrimitiveGroup;


class CGroup: public CWordsPair, public CInputItem {
public:

    inline CGroup()
        : ActiveGramSymbol(-1)
    {
    }

    inline CGroup(int i)
        : CWordsPair(i)
        , ActiveGramSymbol(-1)
    {
    }

    inline CGroup(const CWordsPair& wp)
        : CWordsPair(wp)
        , ActiveGramSymbol(-1)
    {
    }

    virtual ~CGroup(void) {
    }

    virtual void SetMainGroup(CGroup*) {
    }

    virtual CWordsPair* CloneAsWordsPair() {
        return Clone();
    }

    virtual CGroup* Clone() = 0;
    virtual const CGroup* GetMainGroup() const = 0;
    virtual const CWord* GetMainWord() const = 0;
    //выдает омоним главного слова, используя GetMainTerminalSymbol
    //таких омонимов может быть несколько - выдем первый попавшийся
    virtual const CHomonym* GetFirstMainHomonym() const;

    virtual const CGroup* GetFirstGroup() const {
        return GetMainGroup();      // default is same as the main sub-group
    }

    virtual void SetActiveSymbol(int iS) {
        ActiveGramSymbol = iS;
    }

    inline int GetActiveSymbol() const {
        return ActiveGramSymbol;
    }

    virtual const THomonymGrammems& GetGrammems() const = 0;
    virtual void SetGrammems(const THomonymGrammems&) = 0;
    virtual bool HasEqualGrammems(const CGroup* other) const;

    virtual bool IsPrimitive() const = 0;
    virtual int GetMainTerminalSymbol() const = 0;

    virtual CPrimitiveGroup* GetMainPrimitiveGroup() {
        return NULL;
    }
    virtual const CPrimitiveGroup* GetMainPrimitiveGroup() const {
        return NULL;
    }

    typedef ui32 THomMask;
    struct TChildHomonyms {
        const THomMask* Start;
        const THomMask* Stop;

        TChildHomonyms(const THomMask* start, const THomMask* stop)
            : Start(start)
            , Stop(stop)
        {
            YASSERT(start <= stop);
        }

        explicit TChildHomonyms(const THomMask* singleHom)
            : Start(singleHom)
            , Stop(singleHom + 1)
        {
        }

        explicit TChildHomonyms(const yvector<THomMask>& homs)
            : Start(homs.begin())
            , Stop(homs.end())
        {
        }

        inline size_t Size() const {
            return Stop - Start;
        }

        inline THomMask operator[] (size_t i) const {
            YASSERT(Start + i < Stop);
            return Start[i];
        }

        TChildHomonyms Split(size_t i) {
            i = Min(i, Size());
            TChildHomonyms ret(Start, Start + i);
            Start += i;
            return ret;
        }
    };


    virtual TChildHomonyms GetChildHomonyms() const = 0;
    inline THomMask GetChildHomonym(size_t i) const {
        return GetChildHomonyms()[i];
    }

    virtual void IntersectChildHomonyms(const TChildHomonyms& homs) = 0;
    inline void IntersectChildHomonyms(const CGroup& group) {
        IntersectChildHomonyms(group.GetChildHomonyms());
    }


    virtual ui32 GetMainHomIDs() const = 0;
    virtual void GetMainClauseWord(int&, int&) const = 0;

//    virtual int GetChildHomonymsCount() const = 0;


    inline TWeight GetUserWeight() const {
        return UserWeight;
    }

    inline void AddUserWeight(TWeight w) {
        UserWeight *= w;
    }

    inline void AddUserWeight(const CInputItem* item) {
        const CGroup* group = dynamic_cast<const CGroup*>(item);
        if (group != NULL)
            AddUserWeight(group->GetUserWeight());
    }


    // debugging info
    void SetRuleName(const Stroka& name) {
        RuleName = name;
    }

    const Stroka& GetRuleName() const {
        return RuleName;
    }

    virtual Stroka DebugString() const {
        return Stroka();
    }

private:
    int ActiveGramSymbol;
    TWeight UserWeight;      // default is 1.0, weights are multiplicated when combined

    Stroka RuleName;        // for debugging, initialized explicitly
};
