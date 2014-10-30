#include "postfix.h"
#include "tomaparser.h"
#include <library/lemmer/dictlib/grammarhelper.h>


Stroka TOptionPrinter::PrintOption(NTomita::TPostfixProto::EOption value, const char* name)
{
    switch (value) {
        case NTomita::TPostfixProto::ALLOW:
            return name;
        case NTomita::TPostfixProto::DENY:
            return Stroka("~") + name;
        default:
            return Stroka();
    }
}

Stroka TOptionPrinter::PrintKeywords(const NTomita::TKeyWordsProto& value, const char* name)
{
    if (value.KeyWordSize() == 0)
        return Stroka();

    Stroka str = name;
    if (value.GetFirstWord())
        str += 'f';
    str += '=';
    if (value.GetNegative())
        str += '~';
    if (value.KeyWordSize() == 1)
        str += value.GetKeyWord(0);
    else {
        str += '[';
        str += value.GetKeyWord(0);
        for (size_t i = 1; i < value.KeyWordSize(); ++i)
            str += "," + value.GetKeyWord(i);
        str += ']';
    }
    return str;
}

Stroka TOptionPrinter::PrintGrammems(const NTomita::TGramProto& value, const char* name)
{
    if (value.GetGrammems().empty())
        return Stroka();

    Stroka str = name;
    str += '=';
    if (value.GetNegative())
        str += '~';
    if (value.GetMixForms())
        str += '&';
    //str += '"';
    str += TGramBitSet::FromBytes(value.GetGrammems()).ToString(",");
    //str += '"';
    return str;
}

Stroka TOptionPrinter::PrintRegex(const NTomita::TRegexProto& value, const char* name)
{
    if (value.GetMask().empty())
        return Stroka();

    Stroka str = name;
    switch (value.GetWord()) {
        case NTomita::TRegexProto::MAIN:
            str += 'm'; break;
        case NTomita::TRegexProto::FIRST:
            str += 'f'; break;
        case NTomita::TRegexProto::LAST:
            str += 'l'; break;
        default:
            break;
    }
    str += '=';
    if (value.GetNegative())
        str += '~';
    str += '"';
    str += value.GetMask();
    str += '"';
    return str;
}

Stroka TOptionPrinter::PrintAgrType(NTomita::EAgrType type) {
    return TTomitaAgreementNames::GetName(type);
}

Stroka TOptionPrinter::PrintAgr(const NTomita::TAgrProto& agr)
{
    Stroka str;
    if (agr.GetNegative())
        str += '~';
    str += PrintAgrType(agr.GetType());
    str += '[';
    str += ::ToString(agr.GetId());
    str += ']';
    return str;
}

Stroka TOptionPrinter::PrintInterp(const NTomita::TInterpProto& value)
{
    Stroka str;
    if (value.GetPlus())
        str += '+';
    str += value.GetFact();
    str += '.';
    str += value.GetField();
    if (!value.GetFromField().empty()) {
        str += "<-";
        if (!value.GetFromFact().empty()) {
            str += value.GetFromFact();
            str += '.';
        }
        str += value.GetFromField();
    }
    if (!value.GetAssignedValue().empty()) {
        str += '=';
        str += value.GetAssignedValue();
    }
    for (size_t i = 0; i < value.OptionSize(); ++i) {
        str += "::";
        str += value.GetOption(i);
    }
    if (!value.GetNormGrammems().empty()) {
        str += "::norm=";
        str += TGramBitSet::FromBytes(value.GetNormGrammems()).ToString(",");
    }
    return str;
}

Stroka TOptionPrinter::PrintOption(const Stroka& value, const char* name)
{
    Stroka str;
    if (!value.empty()) {
        str += name;
        str += '=';
        str += value;
    }
    return str;
}

#define PUSH_POSTFIX_OPTION(VALUE, TYPE, NAME) \
    { Stroka tmp = PrintOption(VALUE, NAME); \
    if (!tmp.empty()) res.push_back(TTypedOption(tmp, TTypedOption::OPT_##TYPE)); }

void TOptionPrinter::CollectTypedOptions(const NTomita::TRuleItemProto& item, yvector<TTypedOption>& res)
{
    const NTomita::TPostfixProto& p = item.GetTerm().GetPostfix();

    PUSH_POSTFIX_OPTION(p.GetRoot(), SPECIAL, "rt");

    PUSH_POSTFIX_OPTION(p.GetBastard(), TERMINAL, "~dict");
    PUSH_POSTFIX_OPTION(p.GetNoPrep(),  TERMINAL, "~prep");
    PUSH_POSTFIX_OPTION(p.GetNoHom(),   TERMINAL, "~hom");

    PUSH_POSTFIX_OPTION(p.GetLReg(),  TERMINAL, "lreg");
    PUSH_POSTFIX_OPTION(p.GetHReg(),  TERMINAL, "hreg");
    PUSH_POSTFIX_OPTION(p.GetHReg1(), TERMINAL, "hreg1");
    PUSH_POSTFIX_OPTION(p.GetHReg2(), TERMINAL, "hreg2");

    PUSH_POSTFIX_OPTION(p.GetLatin(),  TERMINAL, "lat");
    PUSH_POSTFIX_OPTION(p.GetIsName(), TERMINAL, "name");
    PUSH_POSTFIX_OPTION(p.GetIsGeo(),  TERMINAL, "geo");

    PUSH_POSTFIX_OPTION(p.GetMultiWord(),       TERMINAL, "mw");
    PUSH_POSTFIX_OPTION(p.GetExcludeFromNorm(), TERMINAL, "cut");

    PUSH_POSTFIX_OPTION(p.GetFirstWord(), REDUCE, "fw");

    YASSERT(p.GramSize() <= 2);
    for (size_t i = 0; i < p.GramSize(); ++i)
        if (item.GetTerm().GetType() == NTomita::TTermProto::NON_TERMINAL) {
            PUSH_POSTFIX_OPTION(p.GetGram(i), REDUCE,   "gram");
        } else {
            PUSH_POSTFIX_OPTION(p.GetGram(i), TERMINAL, "gram");
        }

    for (size_t i = 0; i < p.GramUnionSize(); ++i)
        PUSH_POSTFIX_OPTION(p.GetGramUnion(i), REDUCE, "GU");
    PUSH_POSTFIX_OPTION(p.GetKWSet(), REDUCE, "kwset");

    PUSH_POSTFIX_OPTION(p.GetKWType(), TERMINAL, "kw");
    PUSH_POSTFIX_OPTION(p.GetLabel(),  TERMINAL, "label");

    PUSH_POSTFIX_OPTION(p.GetRegex(), REDUCE, "wf");
    if (p.GetLeftQuoted() == p.GetRightQuoted()) {
        PUSH_POSTFIX_OPTION(p.GetLeftQuoted(),  REDUCE, "quot");
    } else {
        PUSH_POSTFIX_OPTION(p.GetLeftQuoted(),  REDUCE, "lquot");
        PUSH_POSTFIX_OPTION(p.GetRightQuoted(), REDUCE, "rquot");
    }

    for (size_t i = 0; i < p.AgrSize(); ++i)
        PUSH_POSTFIX_OPTION(p.GetAgr(i), BINARY, "");         // name is not printed for agreements
/*  do not treat as postfix for now, it is printed explicitly in TTomitaCompiler::PrintRule()
    for (size_t i = 0; i < item.InterpSize(); ++i)
        PUSH_POSTFIX_OPTION(item.GetInterp(i), INTERP, "");   // name is not printed for interpretations
*/

    if (p.HasGztWeight())
        PUSH_POSTFIX_OPTION(p.GetGztWeight().GetField(), REDUCE, "gztweight");
}

#undef PUSH_POSTFIX_OPTION

Stroka TOptionPrinter::AppendOptionToName(const Stroka& name, const Stroka& option)
{
    if (option.empty())
        return name;
    else if (name.has_prefix("{") && name.has_suffix("}"))
        return name.substr(0, name.size() - 1) + option + "}";
    else
        return "{" + name + option + "}";
}

Stroka TOptionPrinter::MergeOptionsToName(const NTomita::TRuleItemProto& item)
{
    yvector<TTypedOption> all;
    CollectTypedOptions(item, all);
    yset<Stroka> good;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i].Type == TTypedOption::OPT_TERMINAL ||
            all[i].Type == TTypedOption::OPT_REDUCE)
        good.insert(all[i].Text);

    Stroka optstr;
    for (yset<Stroka>::const_iterator it = good.begin(); it != good.end(); ++it) {
        optstr += ',';
        optstr += *it;
    }
    return AppendOptionToName(item.GetTerm().GetName(), optstr);
}

static inline bool IsGoodOption(const TTypedOption& option, bool withoutReduceInfo) {
    switch (option.Type) {
        case TTypedOption::OPT_TERMINAL:
            return true;
        case TTypedOption::OPT_BINARY:
        case TTypedOption::OPT_REDUCE:
        case TTypedOption::OPT_SPECIAL:
            return !withoutReduceInfo;
        default:
            return false;
    }
}

Stroka TOptionPrinter::PrintPostfix(const NTomita::TRuleItemProto& item, bool withoutReduceInfo)
{
    yvector<TTypedOption> res;
    CollectTypedOptions(item, res);
    size_t i = 0;
    for (; i < res.size(); ++i)
        if (IsGoodOption(res[i], withoutReduceInfo))
            break;

    if (i >= res.size())
        return Stroka();

    Stroka strPostfix = "<";
    strPostfix += res[i].Text;
    for (++i; i < res.size(); ++i)
        if (IsGoodOption(res[i], withoutReduceInfo)) {
            strPostfix += ',';
            strPostfix += res[i].Text;
        }
    strPostfix += '>';
    return strPostfix;
}

static void MergeKeywords(const NTomita::TKeyWordsProto& src, NTomita::TKeyWordsProto* dst)
{
    if (src.KeyWordSize() > 0) {
        if (dst->KeyWordSize() == 0) {
            dst->CopyFrom(src);
            return;
        } else {

            if (src.SerializeAsString() == dst->SerializeAsString())
                return; // same keywords

            // we have a contradiction here.
            // set kwtype which does not exist
            // and will never be found therefore

            yset<Stroka> kwtypes;
            for (size_t i = 0; i < src.KeyWordSize(); ++i)
                kwtypes.insert(src.GetKeyWord(i));
            for (size_t i = 0; i < dst->KeyWordSize(); ++i)
                kwtypes.insert(dst->GetKeyWord(i));

            dst->ClearKeyWord();
            for (yset<Stroka>::const_iterator it = kwtypes.begin(); it != kwtypes.end(); ++it)
                dst->AddKeyWord(*it);
        }
    }
}

// Protobuf bool and EOption cannot be assigned via mutable pointer,
// so we will use following macros for better readability:
#define MERGE_BOOL_OPTION(option)  if (src.Get##option()) dst->Set##option(true)
#define MERGE_EOPTION(option) if (src.Get##option() != NTomita::TPostfixProto::UNDEF) dst->Set##option(src.Get##option())

static void MergeSimplePostfix(const NTomita::TPostfixProto& src, NTomita::TPostfixProto* dst)
{
    MERGE_BOOL_OPTION(Bastard);
    MERGE_BOOL_OPTION(NoPrep);
    MERGE_BOOL_OPTION(NoHom);
    MERGE_BOOL_OPTION(LReg);
    MERGE_BOOL_OPTION(HReg);
    //MERGE_BOOL_OPTION(HReg1); -- moved to first-word postfixes
    MERGE_EOPTION(HReg2);
    MERGE_EOPTION(Latin);
    MERGE_EOPTION(IsName);
    MERGE_EOPTION(IsGeo);
    MERGE_EOPTION(MultiWord);
}

static void MergeTerminalFirstWordPostfix(const NTomita::TPostfixProto& src, NTomita::TPostfixProto* dst)
{
    MERGE_BOOL_OPTION(HReg1);
}

static void MergeTerminalAllWordsPostfix(const NTomita::TPostfixProto& src, NTomita::TPostfixProto* dst)
{
    MERGE_BOOL_OPTION(ExcludeFromNorm);
}

static void MergeTerminalMainWordPostfix(const NTomita::TPostfixProto& src, NTomita::TPostfixProto* dst)
{
    MergeSimplePostfix(src, dst);
    MergeKeywords(src.GetKWType(), dst->MutableKWType());
    MergeKeywords(src.GetLabel(), dst->MutableLabel());
}

#undef MERGE_FUZZY_OPTION
#undef MERGE_BOOL_OPTION

void TTerminalPostfixSet::Merge(NTomita::TRuleProto* dstRule, const NTomita::TPostfixProto& postfix)
{
    for (size_t j = 0; j < dstRule->ItemSize(); ++j) {
        NTomita::TTermProto* mterm = dstRule->MutableItem(j)->MutableTerm();
        // somes posfixes are merged into first word, some (most) into main word.
        if (j == 0)
            MergeTerminalFirstWordPostfix(postfix, mterm->MutablePostfix());
        if (mterm->GetPostfix().GetRoot())
            MergeTerminalMainWordPostfix(postfix, mterm->MutablePostfix());
        MergeTerminalAllWordsPostfix(postfix, mterm->MutablePostfix());
    }
}

bool TTerminalPostfixSet::Extract(const NTomita::TRuleItemProto& item, yset<Stroka>* resPostfix)
{
    yvector<TTypedOption> options;
    TOptionPrinter::CollectTypedOptions(item, options);

    for (size_t i = 0; i < options.size(); ++i)
        if (options[i].Type == TTypedOption::OPT_TERMINAL)
            resPostfix->insert(options[i].Text);

    return !resPostfix->empty();
}

void TTerminalPostfixSet::Clear(NTomita::TPostfixProto* postfix)
{
    postfix->SetBastard(false);
    postfix->SetNoPrep(false);
    postfix->SetNoHom(false);

    postfix->SetLReg(false);
    postfix->SetHReg(false);
    postfix->SetHReg1(false);
    postfix->SetHReg2(NTomita::TPostfixProto::UNDEF);

    postfix->SetLatin(NTomita::TPostfixProto::UNDEF);
    postfix->SetIsName(NTomita::TPostfixProto::UNDEF);
    postfix->SetIsGeo(NTomita::TPostfixProto::UNDEF);
    postfix->SetMultiWord(NTomita::TPostfixProto::UNDEF);
    postfix->SetExcludeFromNorm(false);

    postfix->MutableKWType()->Clear();
    postfix->MutableLabel()->Clear();
}

void TPostfixIndex::AddToIndex(size_t ruleIndex, const NTomita::TRuleProto& rule)
{
    const Stroka& ruleName = rule.GetName();
    RuleMap[ruleName].push_back(ruleIndex);
    UsedNames.insert(MakePair(ruleName, TTermName(ruleName, TTerminalPostfixSet())));

    for (size_t j = 0; j < rule.ItemSize(); ++j) {
        const NTomita::TTermProto& t = rule.GetItem(j).GetTerm();
        if (t.GetType() == NTomita::TTermProto::NON_TERMINAL) {
            TTerminalPostfixSet options(rule.GetItem(j));
            if (!options.Empty())
                PostfixMap[TTermName(t.GetName(), options)].push_back(MakePair(ruleIndex, j));
        }
    }
}

bool TPostfixIndex::Pop(Stroka& newName, yvector<TPos>& termPositions, yvector<size_t>& rulePositions)
{
    TPostfixMap::iterator it = PostfixMap.begin();
    YASSERT(it != PostfixMap.end());
    TTermName termName = it->first;      // not normalized
    rulePositions = RuleMap.find(termName.first)->second;

    NormalizeName(termName);
    newName = TOptionPrinter::AppendOptionToName(termName.first, termName.second.ToString());
    termPositions.swap(it->second);

    PostfixMap.erase(it);

    TPair<ymap<Stroka, TTermName>::iterator, bool> res = UsedNames.insert(MakePair(newName, termName));
    return res.second;  // if @newName is really new rule, never met before
}

