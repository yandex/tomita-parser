#pragma once

#include <util/generic/stroka.h>
#include <util/generic/pair.h>
#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/map.h>
#include "tomasyntax.pb.h"

#include <library/token/enumbitset.h>

struct TTypedOption {
    enum EType {
        OPT_TERMINAL,
        OPT_REDUCE,
        OPT_BINARY,
        OPT_INTERP,
        OPT_SPECIAL,

        OPT_END
    };

    typedef TEnumBitSet<EType, OPT_TERMINAL, OPT_END> TMask;

    Stroka Text;
    EType Type;

    TTypedOption(Stroka text, EType type)
        : Text(text)
        , Type(type)
    {
    }
};

class TOptionPrinter {
public:

    static Stroka PrintKeywords(const NTomita::TKeyWordsProto& value, const char* name);
    static Stroka PrintGrammems(const NTomita::TGramProto& value, const char* name);
    static Stroka PrintRegex(const NTomita::TRegexProto& value, const char* name);
    static Stroka PrintAgr(const NTomita::TAgrProto& agr);
    static Stroka PrintAgrType(NTomita::EAgrType type);
    static Stroka PrintInterp(const NTomita::TInterpProto& value);
    //static Stroka PrintGztWeightField(const Stroka& gztWeightField);

    static Stroka PrintOption(NTomita::TPostfixProto::EOption value, const char* name);
    static Stroka PrintOption(const Stroka& value, const char* name);

    static Stroka PrintOption(bool value, const char* name) {
        return value ? name : Stroka();
    }
    static Stroka PrintOption(const NTomita::TKeyWordsProto& value, const char* name) {
        return PrintKeywords(value, name);
    }
    static Stroka PrintOption(const NTomita::TGramProto& value, const char* name) {
        return PrintGrammems(value, name);
    }
    static Stroka PrintOption(const NTomita::TRegexProto& value, const char* name) {
        return PrintRegex(value, name);
    }
    static Stroka PrintOption(const NTomita::TAgrProto& value, const char* /*name*/) {
        return PrintAgr(value);
    }
    static Stroka PrintOption(const NTomita::TInterpProto& value, const char* /*name*/) {
        return PrintInterp(value);
    }

    static void CollectTypedOptions(const NTomita::TRuleItemProto& item, yvector<TTypedOption>& res);

    static Stroka AppendOptionToName(const Stroka& name, const Stroka& option);
    static Stroka MergeOptionsToName(const NTomita::TRuleItemProto& item);

    static Stroka PrintPostfix(const NTomita::TRuleItemProto& item, bool withoutReduceInfo);
};

// Essentially same as yset<Stroka>, representing set of "good" (downgradeable to terminal) options
class TTerminalPostfixSet {
public:

    static bool Extract(const NTomita::TRuleItemProto& item, yset<Stroka>* resPostfix);
    static void Clear(NTomita::TPostfixProto* postfix);
    static void Merge(NTomita::TRuleProto* dstRule, const NTomita::TPostfixProto& postfix);

    inline TTerminalPostfixSet() {
    }

    explicit inline TTerminalPostfixSet(const NTomita::TRuleItemProto& item) {
        yset<Stroka> tmp;
        Extract(item, &tmp);
        if (!tmp.empty()) {
            Options.Reset(new yset<Stroka>);
            Options->swap(tmp);
        }
    }

    inline bool Empty() const {
        return Options.Get() == NULL;
    }

    inline bool operator == (const TTerminalPostfixSet& other) const {
        if (Empty())
            return other.Empty();
        else
            return !other.Empty() && *Options == *other.Options;
    }

    inline bool operator < (const TTerminalPostfixSet& other) const {
        if (other.Empty())
            return false;
        else
            return Empty() ? true : *Options < *other.Options;
    }

    inline void operator |= (const TTerminalPostfixSet& other) {
        if (!other.Empty()) {
            if (Empty())
                Options = other.Options;
            else
                Options->insert(other.Options->begin(), other.Options->end());
        }
    }

    inline Stroka ToString() const {
        Stroka res;
        if (!Empty())
            for (yset<Stroka>::const_iterator it = Options->begin(); it != Options->end(); ++it)
                res += "," + *it;
        return res;
    }
private:
    TSharedPtr< yset<Stroka> > Options;
};

class TPostfixIndex
{
public:
    typedef TPair<size_t, size_t> TPos;     // position of non-terminal: rule index + right part item index
    typedef TPair<Stroka, TTerminalPostfixSet> TTermName;     // composite term name: original term name itself + postfix options
    typedef ymap<TTermName, yvector<TPos> > TPostfixMap; // non-terminal name -> non-terminal positions
    typedef ymap<Stroka, yvector<size_t> > TRuleMap;     // non-terminal name -> rule positions

    explicit TPostfixIndex(const NTomita::TFileProto& file) {
        for (size_t i = 0; i < file.RuleSize(); ++i)
            AddToIndex(i, file.GetRule(i));
    }

    void AddToIndex(size_t ruleIndex, const NTomita::TRuleProto& rule);

    inline bool Empty() const {
        return PostfixMap.empty();
    }

    bool Pop(Stroka& newRuleName, yvector<TPos>& termPositions, yvector<size_t>& rulePositions);

    static inline NTomita::TTermProto* GetTerm(NTomita::TFileProto& file, const TPos& pos) {
        return file.MutableRule(pos.first)->MutableItem(pos.second)->MutableTerm();
    }

private:
    void NormalizeName(TTermName& name) const {
        ymap<Stroka, TTermName>::const_iterator it = UsedNames.find(name.first);
        if (it != UsedNames.end()) {
            name.first = it->second.first;
            name.second |= it->second.second;
        }
    }

    TRuleMap RuleMap;
    TPostfixMap PostfixMap;
    ymap<Stroka, TTermName> UsedNames;
};
