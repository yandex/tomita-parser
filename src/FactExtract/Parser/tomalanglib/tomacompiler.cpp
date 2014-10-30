#include <util/generic/map.h>
#include <contrib/libs/protobuf/stubs/substitute.h>

#include <kernel/gazetteer/common/protohelpers.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/afglrparserlib/simplegrammar.h>
#include <FactExtract/Parser/afglrparserlib/sourcegrammar.h>

#include "tomacompiler.h"
#include "tomaparser.h"
#include "postfix.h"


// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false

using ::google::protobuf::strings::Substitute;

TTomitaCompiler::TTomitaCompiler(const IArticleNames* articles,
                                 const IFactTypeDictionary* facttypes,
                                 const ITerminalDictionary* terminals,
                                 TOutputStream& log)
    : HadErrors(false)
    , ErrorCollector(new TErrorCollector)
    , Log(log)
    , Articles(articles)
    , FactTypes(facttypes)
    , Terminals(terminals)
{
}

TTomitaCompiler::~TTomitaCompiler() {
    // for sake of TErrorCollector
}

void TTomitaCompiler::AddError(const Stroka& file, const Stroka& error)
{
    if (ErrorCollector.Get() != NULL)
        ErrorCollector->AddError(file, -1, -1, HadErrors ? error : "\n" + error);
    HadErrors = true;
}

void TTomitaCompiler::AddError(const NTomita::TFileLocation& location, const Stroka& error)
{
    if (ErrorCollector.Get() != NULL)
        ErrorCollector->AddError(location.GetFile(), location.GetLine(), location.GetColumn(), HadErrors ? error : "\n" + error);
    HadErrors = true;
}

void TTomitaCompiler::AddRuleError(const NTomita::TRuleProto& rule, const Stroka& error)
{
    AddError(rule.GetLocation(), Substitute("Rule \"$0\": ", rule.GetName()) + error);
}

bool TTomitaCompiler::Compile(TInputStream* input, CWorkGrammar* target, const Stroka& root)
{
    HadErrors = false;

    TTomitaParser parser;
    parser.RecordErrorsTo(ErrorCollector.Get());
    parser.CheckArticlesIn(Articles);
    parser.CheckFactTypesIn(FactTypes);
    parser.CheckTerminalsIn(Terminals);

    NTomita::TFileProto parsedFile;
    DO(parser.Parse(input, parsedFile));

    if (!root.empty())
        parsedFile.SetGrammarRoot(root);

    DO(BuildFile(parsedFile));

    target->m_Language = morphRussian;
    DO(BuildGrammar(parsedFile, target));

    // no dependencies for streamed input
    // CollectDependencies("", parsedFile, target);

    return !HadErrors;
}

bool TTomitaCompiler::Compile(const Stroka& file, CWorkGrammar* target)
{
    HadErrors = false;

    TTomitaParser parser;
    parser.RecordErrorsTo(ErrorCollector.Get());
    parser.CheckArticlesIn(Articles);
    parser.CheckFactTypesIn(FactTypes);
    parser.CheckTerminalsIn(Terminals);

    NTomita::TFileProto parsedFile;
    DO(parser.Parse(file, parsedFile));
    bool res = BuildFile(parsedFile);

    if (parsedFile.GetDebug())
        Clog << "\n\n========== " << parsedFile.GetName() << " ==========\n" << PrintFile(parsedFile) << "\n" << Endl;

    if (!res)
        return false;

    target->m_Language = morphRussian;
    DO(BuildGrammar(parsedFile, target));

    // collect all dependency files
    CollectDependencies(PathHelper::DirName(file), parsedFile, target);

    Log << "\t(" << target->m_UniqueGrammarItems.size() << " unique symbols, " << target->m_EncodedRules.size() << " rules)\t";
    Log.Flush();

    return !HadErrors;
}

bool TTomitaCompiler::Compile(const Stroka& srcFile, const Stroka& targetFile, bool force)
{
    if (!force && IsGoodBinary(srcFile, targetFile))
        return true;

    CWorkGrammar targetGrammar;
    if (!Compile(srcFile, &targetGrammar))
        return false;

    PathHelper::MakePath(PathHelper::DirName(targetFile));
    targetGrammar.SaveToFile(targetFile);
    return true;
}

bool TTomitaCompiler::BuildFile(NTomita::TFileProto& file)
{
    if (file.RuleSize() == 0)
        return true;

    DO(CheckRules(file));

    // several transformations
    DO(RemoveUnrefRules(file));

    DO(FixTerminalInterps(file));
    DO(KleeneTransform(file));
    DO(DowngradePostfixes(file));

    // just to be sure
    DO(CheckRules(file));

    return true;
}

bool TTomitaCompiler::CheckRules(NTomita::TFileProto& file)
{
    bool error = false;
    for (size_t i = 0; i < file.RuleSize(); ++i) {
        NTomita::TRuleProto& rule = *file.MutableRule(i);
        if (!CheckMainWords(&rule) ||
            !CheckEpsilonErrors(rule) ||
            !CheckBinaryAgreements(rule) ||
            !CheckGztFields(&rule))
                error = true;
    }
    return !error && CheckUniqRules(file) && CheckUndefinedTerms(file);
}

static inline bool IsOptionalItem(const NTomita::TRuleItemProto& item) {
    return item.GetOption() == NTomita::TRuleItemProto::PARENTH ||
           item.GetOption() == NTomita::TRuleItemProto::STAR;
}

bool TTomitaCompiler::CheckMainWords(NTomita::TRuleProto* rule)
{
    bool hasMainWord = false;
    for (size_t j = 0; j < rule->ItemSize(); ++j) {
        const NTomita::TTermProto& term = rule->GetItem(j).GetTerm();
        if (term.HasPostfix() && term.GetPostfix().GetRoot()) {
            if (IsOptionalItem(rule->GetItem(j))) {
                AddRuleError(*rule, "Main word is specified on optional right part item.");
                return false;
            } else if (hasMainWord) {
                AddRuleError(*rule, "Two main words explicitly specified.");
                return false;
            } else
                hasMainWord = true;
        }
    }
    if (!hasMainWord) {
        // mark the first term as main word
        NTomita::TTermProto* firstTerm = rule->MutableItem(0)->MutableTerm();
        firstTerm->MutablePostfix()->SetRoot(true);
    }
    return true;
}

bool TTomitaCompiler::CheckEpsilonErrors(const NTomita::TRuleProto& rule)
{
    size_t nonEmptyCount = 0;
    size_t firstNonEmpty = 0;
    for (size_t i = 0; i < rule.ItemSize(); ++i)
        if (!IsOptionalItem(rule.GetItem(i))) {
            if (nonEmptyCount == 0)
                firstNonEmpty = i;
            nonEmptyCount += 1;
        }
    if (nonEmptyCount == 0) {
        AddRuleError(rule, "Right part could be empty (check '()' or '*' operators).");
        return false;
    } else if (nonEmptyCount == 1) {
        const NTomita::TTermProto& term = rule.GetItem(firstNonEmpty).GetTerm();
        if (term.GetType() == NTomita::TTermProto::NON_TERMINAL && rule.GetName() == term.GetName()) {
            AddRuleError(rule, "Right part could be equal to left part (check '()' or '*' operators).");
            return false;
        }
    }
    return true;
}

bool TTomitaCompiler::CheckBinaryAgreements(const NTomita::TRuleProto& rule)
{
    typedef const NTomita::TAgrProto* TAgrPtr;
    ymap<ui32, yvector<TAgrPtr> > agrs;      // agreement id -> first agr, second agr...

    // group agreemtns by id
    for (size_t i = 0; i < rule.ItemSize(); ++i) {
        const NTomita::TPostfixProto& postfix = rule.GetItem(i).GetTerm().GetPostfix();
        for (size_t j = 0; j < postfix.AgrSize(); ++j)
            agrs[postfix.GetAgr(j).GetId()].push_back(&postfix.GetAgr(j));
    }

    bool error = false;
    for (ymap<ui32, yvector<TAgrPtr> >::const_iterator it = agrs.begin(); it != agrs.end(); ++it) {
        ui32 agrId = it->first;
        const yvector<TAgrPtr>& a = it->second;
        if (a.size() == 1) {
            AddRuleError(rule, Substitute("Found unpaired binary agreement postfixes with id = $1 "
                                          "(must have 2 or more items with same id).", a.size(), agrId));
            error = true;
        } else for (size_t i = 1; i < a.size(); ++i)
            if (a[0]->GetType() != a[i]->GetType() || a[0]->GetNegative() != a[i]->GetNegative()) {
                AddRuleError(rule, Substitute("Inconsistent binary agreement with id = $0. "
                                              "All items with equal id must be exactly the same.", agrId));
                error = true;
                break;
            }
    }
    return !error;
}

bool TTomitaCompiler::CheckGztFields(NTomita::TRuleProto* rule)
{
    for (size_t j = 0; j < rule->ItemSize(); ++j) {
        NTomita::TPostfixProto* postfix = rule->MutableItem(j)->MutableTerm()->MutablePostfix();
        if (postfix->HasGztWeight() && !postfix->GetGztWeight().HasArticle()) {
            if (!postfix->HasKWType() || postfix->GetKWType().KeyWordSize() != 1) {
                AddRuleError(*rule, "\"gztweight\" option requires explicit single \"kwtype\""
                                    " option without negation within the same postfix.");
                return false;
            }

            if (postfix->GetKWType().GetNegative()) {
                AddRuleError(*rule, "Cannot use \"gztweight\" with negated kwtype");
                return false;
            }

            const Stroka& kwtype = postfix->GetKWType().GetKeyWord(0);

            if (IsNoneKWType(kwtype)) {
                AddRuleError(*rule, "Cannot use \"gztweight\" with kwtype=none");
                return false;
            }

            if (Articles == NULL || !Articles->HasArticleField(UTF8ToWide(kwtype), postfix->GetGztWeight().GetField())) {
                AddRuleError(*rule, Substitute("Cannot find field \"$0\" for kwtype \"$1\".",
                                               postfix->GetGztWeight().GetField(), kwtype));
                return false;
            }

            // otherwise copy kwtype into Article
            postfix->MutableGztWeight()->SetArticle(kwtype);
        }
    }
    return true;
}

bool TTomitaCompiler::CheckUndefinedTerms(NTomita::TFileProto& file)
{
    yset<Stroka> definedTerms;
    for (size_t i = 0; i < file.RuleSize(); ++i)
        definedTerms.insert(file.GetRule(i).GetName());

    bool found = false;
    for (size_t i = 0; i < file.RuleSize(); ++i)
        for (size_t j = 0; j < file.GetRule(i).ItemSize(); ++j) {
            const NTomita::TTermProto& term = file.GetRule(i).GetItem(j).GetTerm();
            if (term.GetType() == NTomita::TTermProto::NON_TERMINAL &&
                definedTerms.find(term.GetName()) == definedTerms.end()) {
                    AddRuleError(file.GetRule(i), Substitute("Undefined non-terminal \"$0\" in right part.", term.GetName()));
                    found = true;
            }
        }

    return !found;
}

template <typename TProto>
static bool HasSameSerialization(const TProto& a1, const TProto& a2, Stroka& tmpbuf1, Stroka& tmpbuf2)
{
    a1.SerializeToString(&tmpbuf1);
    a2.SerializeToString(&tmpbuf2);
    return tmpbuf1 == tmpbuf2;
}

static bool IsTwinRules(const NTomita::TRuleProto& r1, const NTomita::TRuleProto& r2) {

    // compare rules content (except locations)
    if (r1.GetName() != r2.GetName() || r1.ItemSize() != r2.ItemSize())
        return false;
    Stroka buf1, buf2;
    for (size_t i = 0; i < r1.ItemSize(); ++i)
        if (!HasSameSerialization(r1.GetItem(i), r2.GetItem(i), buf1, buf2))
            return false;

    if (!HasSameSerialization(r1.GetInfo(), r2.GetInfo(), buf1, buf2))
        return false;

    return true;
}

static void RemoveRules(NTomita::TFileProto& file, yset<size_t>& indexes)
{
    indexes.insert(file.RuleSize());        // stop point
    yset<size_t>::const_iterator it = indexes.begin();
    size_t free = *it, cur = *it + 1;
    for (++it; it != indexes.end(); ++it) {
        for (; cur < *it; ++cur, ++free)
            file.MutableRule()->SwapElements(free, cur);
        cur = *it + 1;
    }
    // clear last  unref.size()-1  rules
    for (size_t i = 1; i < indexes.size(); ++i)
        file.MutableRule()->RemoveLast();
}

bool TTomitaCompiler::CheckUniqRules(NTomita::TFileProto& file)
{
    typedef ymap<Stroka, const NTomita::TRuleProto*> TDump;
    TDump fileDump;
    bool uniq = true;

    // Indices of rules having complete twins.
    // They will be just removed, without producing error message.
    yset<size_t> twins;

    for (size_t i = 0; i < file.RuleSize(); ++i) {
        const NTomita::TRuleProto& rule = file.GetRule(i);
        Stroka ruleDump = PrintRule(rule, 1);       // detailLevel = 1: with reduce postfix, but without interpretations
        TPair<TDump::iterator, bool> ins = fileDump.insert(MakePair(ruleDump, &rule));
        if (!ins.second) {
            const NTomita::TRuleProto* eqRule = ins.first->second;
            if (IsTwinRules(rule, *eqRule))
                twins.insert(i);
            else {
                Stroka position = Substitute("$0:$1", eqRule->GetLocation().GetLine() + 1, eqRule->GetLocation().GetColumn() + 1);
                if (eqRule->GetLocation().GetFile() != rule.GetLocation().GetFile())
                    position = Substitute("$0:$1", eqRule->GetLocation().GetFile(), position);
                AddRuleError(rule, Substitute("Same rule already defined with different interpretation "
                    "or output information (see rule at $0):\n$1\n$2", position, PrintRule(rule, 2), PrintRule(*eqRule, 2)));
                uniq = false;
            }
        }
    }

    if (!twins.empty())
        RemoveRules(file, twins);

    return uniq;
}

namespace {

struct TRuleIndex {
    typedef ui32 TNameId;
    yvector<Stroka> Names;              // id -> name
    ymap<Stroka, TNameId> NameIndex;    // name -> id
    yvector<TNameId> Left;              // rule index -> id
    yvector< yvector<TNameId> > Right;  // rule index -> term index -> term id
    TNameId ExplicitRoot;

    explicit TRuleIndex(NTomita::TFileProto& file);
    TNameId AddName(const Stroka& name);

    bool HasExplicitRoot() const {
        return ExplicitRoot != static_cast<TNameId>(-1);
    }

    bool FindUnrefRules(TNameId root, yset<size_t>& rules) const;

    void FindImplicitRoots(yvector<TNameId>& roots) const;
};

TRuleIndex::TRuleIndex(NTomita::TFileProto& file)
    : ExplicitRoot(-1)
{
    size_t ruleCount = file.RuleSize();
    Left.reserve(ruleCount);
    Right.reserve(ruleCount);
    for (size_t i = 0; i < ruleCount; ++i) {
        const NTomita::TRuleProto& rule = file.GetRule(i);
        TNameId id = AddName(rule.GetName());
        if (rule.GetName() == file.GetGrammarRoot())
            ExplicitRoot = id;

        Left.push_back(id);
        Right.push_back();
        for (size_t j = 0; j < rule.ItemSize(); ++j) {
            const Stroka& itemName = rule.GetItem(j).GetTerm().GetName();
            Right.back().push_back(AddName(itemName));
        }
    }
}

TRuleIndex::TNameId TRuleIndex::AddName(const Stroka& name)
{
    TNameId id = Names.size();
    TPair<ymap<Stroka, TNameId>::iterator, bool> res = NameIndex.insert(MakePair(name, id));
    if (res.second) {
        Names.push_back(name);
        return id;
    } else
        return res.first->second;
}

void TRuleIndex::FindImplicitRoots(yvector<TNameId>& roots) const
{
    roots.clear();
    yvector<bool> referred(Names.size(), false);   // true if name i is referred from some other name j (j <> i)
    for (size_t i = 0; i < Right.size(); ++i)
        for (size_t j = 0; j < Right[i].size(); ++j) {
            TNameId termId = Right[i][j];
            if (termId != Left[i] && !referred[termId])
                referred[termId] = true;
        }

    for (size_t i = 0; i < referred.size(); ++i)
        if (!referred[i])
            roots.push_back(i);
}

bool TRuleIndex::FindUnrefRules(TNameId root, yset<size_t>& unrefRules) const
{
    size_t N = Names.size();
    yvector< yvector<size_t> > index(N);
    for (size_t i = 0; i < Left.size(); ++i)
        index[Left[i]].push_back(i);

    yvector<bool> traversed(N, false);
    yvector<size_t> next;
    next.reserve(N);

    // traverse rules tree starting from specified @root
    next.push_back(root);
    traversed[root] = true;

    for (size_t i = 0; i < next.size(); ++i) {
        size_t id = next[i];
        for (size_t j = 0; j < index[id].size(); ++j) {
            const yvector<TNameId>& children = Right[index[id][j]];
            for (size_t k = 0; k < children.size(); ++k)
                if (!traversed[children[k]]) {
                    next.push_back(children[k]);
                    traversed[children[k]] = true;
                }
        }
    }

    for (size_t i = 0; i < traversed.size(); ++i)
        if (!traversed[i])
            unrefRules.insert(index[i].begin(), index[i].end());

    return !unrefRules.empty();
}

} // anonymous namespace

bool TTomitaCompiler::RemoveUnrefRules(NTomita::TFileProto& file)
{
    TRuleIndex rules(file);

    TRuleIndex::TNameId gRoot;
    if (rules.HasExplicitRoot())
        gRoot = rules.ExplicitRoot;
    else {
        if (!file.GetGrammarRoot().empty()) {
            AddError(file.GetName(), Substitute("Undefined rule \"$0\" specified as grammar root.", file.GetGrammarRoot()));
            return false;
        }
        yvector<TRuleIndex::TNameId> roots;
        rules.FindImplicitRoots(roots);
        if (roots.size() != 1) {
            AddError(file.GetName(), Substitute("Cannot deduce grammar root from rule graph (found $0 implicit roots). "
                                        "You need to specify the root explicitly.", roots.size()));
            return false;
        }
        // otherwise set this implicit root as explicit (to avoid deleting it accidently next time)
        gRoot = roots[0];
        file.SetGrammarRoot(rules.Names[gRoot]);
    }

    // find rules with unreferences left-side item and remove them.
    yset<size_t> unref;
    if (rules.FindUnrefRules(gRoot, unref))
        RemoveRules(file, unref);
    return true;
}

static Stroka PrintKleeneOperator(const NTomita::TRuleItemProto& item)
{
    Stroka res;
    switch (item.GetOption()) {
        case NTomita::TRuleItemProto::SINGLE:
            break;
        case NTomita::TRuleItemProto::PARENTH:
            res += '?'; break;
        case NTomita::TRuleItemProto::STAR:
            res += '*'; break;
        case NTomita::TRuleItemProto::PLUS:
            res += '+'; break;
        default:
            res += "unk";
    }
    if (item.GetCoord())
        res += '!';
    if (item.GetAgr() != NTomita::Any)
        res += "[" + TOptionPrinter::PrintAgrType(item.GetAgr()) + "]";
    return res;
}

Stroka TTomitaCompiler::PrintRule(const NTomita::TRuleProto& rule, size_t detailLevel) {
    Stroka res = rule.GetName() + " ->";
    for (size_t j = 0; j < rule.ItemSize(); ++j) {
        const NTomita::TRuleItemProto& item = rule.GetItem(j);
        res += " ";
        if (item.GetTerm().GetType() == NTomita::TTermProto::LEMMA) {
            res += '"';
            res += item.GetTerm().GetName();
            res += '"';
        } else
            res += item.GetTerm().GetName();
        res += TOptionPrinter::PrintPostfix(item, detailLevel == 0);
        if (detailLevel > 0) {
            res += PrintKleeneOperator(item);

            if (detailLevel > 1) {
                if (item.InterpSize() > 0) {
                    res += " as(";
                    res += TOptionPrinter::PrintInterp(item.GetInterp(0));
                    for (size_t k = 1; k < item.InterpSize(); ++k) {
                        res += ';';
                        res += TOptionPrinter::PrintInterp(item.GetInterp(k));
                    }
                    res += ')';
                }
            }
        }
    }
    res += ";";
    return res;
}

Stroka TTomitaCompiler::PrintFile(const NTomita::TFileProto& file) {
    Stroka res;
    for (size_t i = 0; i < file.RuleSize(); ++i) {
        res += PrintRule(file.GetRule(i), 2);   // detailLeve = 2: max details - all postfix, interpretations
        res += '\n';
    }
    return res;
}

// Move some postfixes from non-terminals down to terminals, for example:

// XXX -> B C<rt>
// A -> XXX<~dict>

// is transformed to:

// {XXX,~dict} -> B C<rt,~dict>
// A -> {XXX,~dict}

bool TTomitaCompiler::DowngradePostfixes(NTomita::TFileProto& file)
{
    TPostfixIndex index(file);

    Stroka postfixName;
    yvector<TPostfixIndex::TPos> termPositions;
    yvector<size_t> rulePositions;

    // source postfix, for copying its "good" (downgradeable) options into new artificial rules main-terms (terms with rt option)
    NTomita::TPostfixProto srcPostfix;

    while (!index.Empty()) {
        Stroka newRuleName;
        bool isNew = index.Pop(newRuleName, termPositions, rulePositions);
        YASSERT(termPositions.size());
        NTomita::TTermProto* srcTerm = index.GetTerm(file, termPositions[0]);

        // making a copy of source postfix, as it will soon be cleared off "good" options
        srcPostfix.CopyFrom(srcTerm->GetPostfix());

        // remove downgraded postfix from all corresponding right part non-terminals
        for (size_t i = 0; i < termPositions.size(); ++i) {
            NTomita::TTermProto* mterm = index.GetTerm(file, termPositions[i]);
            mterm->SetName(newRuleName);
            TTerminalPostfixSet::Clear(mterm->MutablePostfix());
        }

        // clone all srcTerm rules with postfix appended to its name
        // but only if there was not already rule with such name (otherwise it will lead to infinite recursion)
        if (isNew) {
            for (size_t i = 0; i < rulePositions.size(); ++i) {
                NTomita::TRuleProto* newRule = file.AddRule();
                newRule->CopyFrom(file.GetRule(rulePositions[i]));
                newRule->SetName(newRuleName);
                TTerminalPostfixSet::Merge(newRule, srcPostfix);

                // update index for newly added rule
                index.AddToIndex(file.RuleSize() - 1, *newRule);
            }
        }
    }
    // Remove unreferenced rules once again
    return RemoveUnrefRules(file);
}

static inline NTomita::TRuleProto* AddRule(NTomita::TFileProto* file, const Stroka& name)
{
    NTomita::TRuleProto* newRule = file->AddRule();
    newRule->SetName(name);
    return newRule;
}

static NTomita::TRuleItemProto* AddRuleItem(NTomita::TRuleProto* rule, const Stroka& name, NTomita::TTermProto::EType type, bool main = false)
{
    NTomita::TRuleItemProto* newItem = rule->AddItem();
    newItem->MutableTerm()->SetName(name);
    newItem->MutableTerm()->SetType(type);
    if (main)
        newItem->MutableTerm()->MutablePostfix()->SetRoot(true);
    return newItem;
}

static NTomita::TRuleProto* MakeRuleFromItem(NTomita::TFileProto& file, NTomita::TRuleItemProto* srcItem, const Stroka& ruleName)
{
    // make separate rule for specified right-part @srcItem, e.g. from
    // A -> B C<x> D
    // we can separate C<x> as new rule Cx:

    // Cx -> C<x>
    // A -> B Cx D

    NTomita::TRuleProto* newRule = AddRule(&file, ruleName);
    NTomita::TRuleItemProto* newItem = newRule->AddItem();

    // move all terminal info from @srcItem to @newItem, except binary agr
    newItem->MutableTerm()->Swap(srcItem->MutableTerm());

    //newItem->MutableInterp()->Swap(srcItem->MutableInterp());
    //newItem->MutableReplace()->swap(*srcItem->MutableReplace());

    srcItem->MutableTerm()->SetName(ruleName);
    srcItem->MutableTerm()->SetType(NTomita::TTermProto::NON_TERMINAL);
    // restore agreements and root
    srcItem->MutableTerm()->MutablePostfix()->SetRoot(newItem->GetTerm().GetPostfix().GetRoot());
    srcItem->MutableTerm()->MutablePostfix()->MutableAgr()->Swap(newItem->MutableTerm()->MutablePostfix()->MutableAgr());

    newItem->MutableTerm()->MutablePostfix()->SetRoot(true);
    return newRule;
}

static void AddTermAgreement(NTomita::TRuleItemProto* item, ui32 id, NTomita::EAgrType agrType, bool negative)
{
    NTomita::TAgrProto* agr = item->MutableTerm()->MutablePostfix()->AddAgr();
    agr->SetId(id);
    agr->SetType(agrType);
    agr->SetNegative(negative);
}

static inline void SetAgreement(NTomita::TRuleItemProto* item1, NTomita::TRuleItemProto* item2,
                                ui32 id, NTomita::EAgrType agrType, bool negative)
{
    if (agrType != NTomita::Any) {
        AddTermAgreement(item1, id, agrType, negative);
        AddTermAgreement(item2, id, agrType, negative);
    }
}

static void TransformAgreements(const NTomita::TRuleItemProto& srcTerm, NTomita::TRuleItemProto* item1, NTomita::TRuleItemProto* item2)
{
    YASSERT(item1->GetTerm().GetPostfix().AgrSize() == 0);
    YASSERT(item2->GetTerm().GetPostfix().AgrSize() == 0);

    size_t agrCount = srcTerm.GetTerm().GetPostfix().AgrSize();
    for (size_t i = 0; i < agrCount; ++i) {
        const NTomita::TAgrProto& agr = srcTerm.GetTerm().GetPostfix().GetAgr(i);
        SetAgreement(item1, item2, i, agr.GetType(), agr.GetNegative());
    }
    SetAgreement(item1, item2, agrCount, srcTerm.GetAgr(), false);
}

static void RemoveAgreements(NTomita::TPostfixProto* postfix, const yset<ui32>& argIds) {
    for (size_t i = 0; i < postfix->AgrSize();) {
        if (argIds.find(postfix->GetAgr(i).GetId()) != argIds.end()) {
            postfix->MutableAgr()->SwapElements(i, postfix->AgrSize() - 1);
            postfix->MutableAgr()->RemoveLast();
        } else
            ++i;
    }
}

static void AddCoordRules(NTomita::TFileProto& file, const Stroka& ruleName, const NTomita::TRuleItemProto& srcTerm)
{
    // "A![agr]" in rule right part is transformed into 3 new rules:
    // {A!} -> {A!}<agr[0]> Comma A<agr[0]>;
    // {A!} -> {A!}<agr[0]> SimConjAnd A<agr[0]>;
    // {A!} -> A;

    // All all binary agreements from A are transferred to corresponding items

    const Stroka& termName = srcTerm.GetTerm().GetName();
    NTomita::TTermProto::EType termType = srcTerm.GetTerm().GetType();

    // {A!} -> {A!}<agr[0]> Comma A<agr[0]>;
    NTomita::TRuleProto* newRule = AddRule(&file, ruleName);
    AddRuleItem(newRule, ruleName, NTomita::TTermProto::NON_TERMINAL, true);        // rt
    AddRuleItem(newRule, TerminalValues[T_COMMA], NTomita::TTermProto::TERMINAL);
    AddRuleItem(newRule, termName, termType);
    TransformAgreements(srcTerm, newRule->MutableItem(0), newRule->MutableItem(2));

    // {A!} -> {A!}<agr[0]> SimConjAnd A<agr[0]>;
    newRule = AddRule(&file, ruleName);
    AddRuleItem(newRule, ruleName, NTomita::TTermProto::NON_TERMINAL, true);        // rt
    AddRuleItem(newRule, TerminalValues[T_CONJAND], NTomita::TTermProto::TERMINAL);
    AddRuleItem(newRule, termName, termType);
    TransformAgreements(srcTerm, newRule->MutableItem(0), newRule->MutableItem(2));

    // {A!} -> A;
    newRule = AddRule(&file, ruleName);
    AddRuleItem(newRule, termName, termType, true);     // rt
}

static void AddPlusRules(NTomita::TFileProto& file, const Stroka& ruleName, const NTomita::TRuleItemProto& srcTerm)
{
    // "A*[agr]" or "A+[agr]" in rule right part is transformed into 2 new rules:
    // {A+} -> {A+}<agr[0]> A<agr[0]>;
    // {A+} -> A;

    const Stroka& termName = srcTerm.GetTerm().GetName();
    NTomita::TTermProto::EType termType = srcTerm.GetTerm().GetType();

    // {Ax+} -> {Ax+}<agr[0]> Ax<agr[0]>;
    NTomita::TRuleProto* newRule = AddRule(&file, ruleName);
    AddRuleItem(newRule, ruleName, NTomita::TTermProto::NON_TERMINAL, true);            // rt
    AddRuleItem(newRule, termName, termType);
    TransformAgreements(srcTerm, newRule->MutableItem(0), newRule->MutableItem(1));
//    CopyInterpPlus(srcTerm, newRule->MutableItem(1));

    // {Ax+} -> Ax;
    newRule = AddRule(&file, ruleName);
    AddRuleItem(newRule, termName, termType, true);     // rt
//    newRule->MutableItem(0)->MutableInterp()->Swap(srcTerm.MutableInterp());        // move interp from original item to this new one (it will be the beginning of plus sequence).
}

bool TTomitaCompiler::KleeneTransform(NTomita::TFileProto& file)
{
    yset<Stroka> knownRules;
    ymap<Stroka, Stroka> pseudo;        // mapping from full merged postfixed names to short pseudonyms

    for (size_t i = 0; i < file.RuleSize(); ++i)
        knownRules.insert(file.GetRule(i).GetName());

    for (size_t i = 0; i < file.RuleSize(); ++i) {
        NTomita::TRuleProto* rule = file.MutableRule(i);
        for (size_t j = 0; j < rule->ItemSize(); ++j) {
            NTomita::TRuleItemProto* item = rule->MutableItem(j);
            Stroka newRuleName;

            if (item->GetCoord() ||
                item->GetOption() == NTomita::TRuleItemProto::STAR ||
                item->GetOption() == NTomita::TRuleItemProto::PLUS) {

                newRuleName = TOptionPrinter::MergeOptionsToName(*item);
                // tmpName could be an original terminal name
                if (item->GetTerm().GetName() != newRuleName) {
                    ymap<Stroka, Stroka>::const_iterator it = pseudo.find(newRuleName);
                    if (it != pseudo.end())
                        newRuleName = it->second;
                    else {
                        // for item 'XXX' make pseudonym 'XXX@@@..'
                        Stroka tmpName = TOptionPrinter::AppendOptionToName(item->GetTerm().GetName(), "@");
                        while (!knownRules.insert(tmpName).second)
                            tmpName = TOptionPrinter::AppendOptionToName(tmpName, "@");
                        pseudo[newRuleName] = tmpName;
                        newRuleName.swap(tmpName);
                        MakeRuleFromItem(file, item, newRuleName);
                    }
                }

                if (item->GetCoord()) {
                    newRuleName = TOptionPrinter::AppendOptionToName(newRuleName, "!");
                    if (knownRules.insert(newRuleName).second)
                        AddCoordRules(file, newRuleName, *item);
                } else {
                    newRuleName = TOptionPrinter::AppendOptionToName(newRuleName, "+");
                    if (knownRules.insert(newRuleName).second)
                        AddPlusRules(file, newRuleName, *item);
                }
            }

            // rename source item and clean it up (but leave PARENTH or STAR option to process it later)
            if (!newRuleName.empty()) {
                item->MutableTerm()->SetName(newRuleName);
                item->MutableTerm()->SetType(NTomita::TTermProto::NON_TERMINAL);
            }
            if (!IsOptionalItem(*item))
                item->SetOption(NTomita::TRuleItemProto::SINGLE);
            item->SetAgr(NTomita::Any);
            item->SetCoord(false);
        }
    }

    // run over rules again and fix optional terms this time
    // (ones that can be skipped, i.e. with () or * operator)
    for (size_t i = 0; i < file.RuleSize(); ++i) {
        NTomita::TRuleProto* rule = file.MutableRule(i);
        for (size_t j = 0; j < rule->ItemSize(); ++j) {
            NTomita::TRuleItemProto* item = rule->MutableItem(j);
            if (IsOptionalItem(*item)) {
                // transform rule "A -> B (C) D" into 2 rules:
                // A -> B C D
                // A -> B D

                // just clear kleene options from (C) to get "A -> B C D":
                item->SetOption(NTomita::TRuleItemProto::SINGLE);

                // copy rule and delete C to get "A -> B D"
                NTomita::TRuleProto* newRule = file.AddRule();
                newRule->CopyFrom(*rule);
                for (size_t k = j + 1; k < newRule->ItemSize(); ++k)
                    newRule->MutableItem()->SwapElements(k-1, k);
                newRule->MutableItem()->RemoveLast();

                // remove unpaired agreements
                yset<ui32> agrIds;
                for (size_t a = 0; a < item->GetTerm().GetPostfix().AgrSize(); ++a)
                    agrIds.insert(item->GetTerm().GetPostfix().GetAgr(a).GetId());
                if (!agrIds.empty())
                    for (size_t k = 0; k < newRule->ItemSize(); ++k)
                        RemoveAgreements(newRule->MutableItem(k)->MutableTerm()->MutablePostfix(),  agrIds);
            }
        }
    }

    // we should do this after modifying existing rules,
    // as we could now have duplicates
    return CheckUniqRules(file);
}

bool TTomitaCompiler::FixTerminalInterps(NTomita::TFileProto& file)
{
    // We cannot have interpretations with terminal/direct lemmas, so make a simple transformation:

    // "A -> T interp (X)" where T is a terminal (direct lemma) is transformed to
    // "A -> {T%TERM} interp (X)"
    // "{T%TERM} -> T"

    yset<Stroka> pseudo;        // already created fake rules

    for (size_t i = 0; i < file.RuleSize(); ++i) {
        NTomita::TRuleProto* rule = file.MutableRule(i);
        for (size_t j = 0; j < rule->ItemSize(); ++j) {
            NTomita::TRuleItemProto* item = rule->MutableItem(j);

            if (item->GetTerm().GetType() != NTomita::TTermProto::NON_TERMINAL &&
                item->GetInterp().size() > 0) {

                Stroka suffix = item->GetTerm().GetType() == NTomita::TTermProto::TERMINAL ? "%TERM" : "%LEMMA";
                Stroka newRuleName = TOptionPrinter::AppendOptionToName(item->GetTerm().GetName(), suffix);

                if (pseudo.insert(newRuleName).second) {
                    NTomita::TRuleProto* newRule = AddRule(&file, newRuleName);
                    AddRuleItem(newRule, item->GetTerm().GetName(), item->GetTerm().GetType(), true);
                }

                item->MutableTerm()->SetName(newRuleName);
                item->MutableTerm()->SetType(NTomita::TTermProto::NON_TERMINAL);
            }
        }
    }

    // we should do this after modifying existing rules,
    // as we could now have duplicates
    return CheckUniqRules(file);
}

// Rules grouped by similarity (equal by symbols without reduce info)
class TRuleGroups {
    typedef const NTomita::TRuleProto* TRulePtr;
public:
    // Given @file should not be changed during TRuleGroups live
    explicit TRuleGroups(const NTomita::TFileProto& file);
    size_t Size() const {
        return Groups.size();
    }

    TRulePtr GetFirstRule(size_t index) const {
        return Groups[index].first;
    }

    const yvector<TRulePtr>& GetSimilarRules(size_t index) const {
        return Groups[index].second;
    }

    bool HasSimilar(size_t index) const {
        return !Groups[index].second.empty();
    }

private:
    typedef TPair<TRulePtr, yvector<TRulePtr> > TGroup;
    yvector<TGroup> Groups;
};

TRuleGroups::TRuleGroups(const NTomita::TFileProto& file)
{
    ymap<Stroka, size_t> index;
    for (size_t i = 0; i < file.RuleSize(); ++i) {
        TRulePtr rule = &file.GetRule(i);
        // we use postfix printer to determine similar rules -
        // just print them without reduce postfixes (minimal detail level)
        Stroka dump = TTomitaCompiler::PrintRule(*rule, 0);
        TPair<ymap<Stroka, size_t>::iterator, bool> ins = index.insert(MakePair(dump, Groups.size()));
        if (ins.second) {
            Groups.push_back();
            Groups.back().first = rule;
        } else
            Groups[ins.first->second].second.push_back(rule);
    }
}

#define PUSH_BOOL_OPTION(Option, PrefixIndex, ValueIndex) \
    if (postfix.Get##Option()) \
        dst->m_Attributes[AlphabetPrefix[PrefixIndex]] = AttributeValues[ValueIndex]

#define PUSH_FUZZY_OPTION(Option, PrefixIndex, ValueIndex) \
    switch (postfix.Get##Option()) { \
        case NTomita::TPostfixProto::ALLOW: dst->m_Attributes[AlphabetPrefix[PrefixIndex]] = AttributeValues[ValueIndex]; break; \
        case NTomita::TPostfixProto::DENY:  dst->m_Attributes[AlphabetPrefix[PrefixIndex]] = AttributeValues[NOt]; break; \
        default: break; \
    }

static Stroka JoinKeywords(const ::google::protobuf::RepeatedPtrField<Stroka>& keywords) {
    if (keywords.size() == 0)
        return Stroka();
    Stroka res = keywords.Get(0);
    for (int i = 1; i < keywords.size(); ++i) {
        res += '+';
        res += keywords.Get(i);
    }
    return res;
}

// CClonedRuleLoader::FDOLang_LoadRightItem version for NTomita::TRuleItemProto
static void ConvertToGrammarItem(const NTomita::TTermProto& term, CGrammarItem* dst, bool filter, const Stroka& replacement = Stroka())
{
    const NTomita::TPostfixProto& postfix = term.GetPostfix();

    dst->m_ItemStrId = term.GetName();
    if (term.GetType() == NTomita::TTermProto::LEMMA)
        dst->m_Lemma = NStr::DecodeTomita(term.GetName());
    //dst->m_Source = r_term.s_parent_item;
    dst->m_bSynMain = postfix.GetRoot();
    dst->m_Type = (term.GetType() == NTomita::TTermProto::NON_TERMINAL) ? siMeta : siString;
    if (term.GetName() == "?")
        dst->m_Type = siAnyWord;

    // here we take only grammems for terminals and lemmas, otherwise they go through unary agreements
    if (postfix.GramSize() > 0 && term.GetType() != NTomita::TTermProto::NON_TERMINAL)
        for (size_t i = 0; i < postfix.GramSize(); ++i) {
            TGramBitSet g = TGramBitSet::FromBytes(postfix.GetGram(i).GetGrammems());
            if (postfix.GetGram(i).GetNegative())
                dst->m_NegativeItemGrammems = g;
            else
                dst->m_ItemGrammems = g;
        }

    if (!replacement.empty())
        dst->m_Attributes[AlphabetPrefix[REPLACE_INTERP]] = replacement;

    PUSH_BOOL_OPTION(NoPrep,  HOM,     NO_PREP);
    PUSH_BOOL_OPTION(NoHom,   NO_HOMS, NO_HOMONYMS);
    PUSH_BOOL_OPTION(Bastard, DICT,    NOt);
    PUSH_BOOL_OPTION(LReg,    LREG,    ALL_LET);
    PUSH_BOOL_OPTION(HReg,    HREG,    ALL_LET);
    PUSH_BOOL_OPTION(HReg1,   HREG1,   FIRST_LET);

    PUSH_FUZZY_OPTION(HReg2,  HREG2,    SECOND_LET);
    PUSH_FUZZY_OPTION(Latin,  LAT,      LATIN);
    PUSH_FUZZY_OPTION(IsName, NAME_DIC, DICT_NAME);
    PUSH_FUZZY_OPTION(IsGeo,  GEO,      DICT_GEO);

    PUSH_FUZZY_OPTION(MultiWord, IS_MULTIWORD, YEs);
    PUSH_BOOL_OPTION(ExcludeFromNorm, EXCLUDE_FROM_NORM, YEs);

    // if we have several kwtypes or labels (a contradiction), just join them to get new name
    // which we can hope does not exist and will never be found therefore.
    if (postfix.GetKWType().KeyWordSize() > 0)
        dst->m_Attributes[AlphabetPrefix[postfix.GetKWType().GetNegative() ? DICT_NOT_KWTYPE : DICT_KWTYPE]] =
            JoinKeywords(postfix.GetKWType().GetKeyWord());
    if (postfix.GetLabel().KeyWordSize() > 0)
        dst->m_Attributes[AlphabetPrefix[postfix.GetLabel().GetNegative() ? DICT_NOT_LABEL : DICT_LABEL]] =
            JoinKeywords(postfix.GetLabel().GetKeyWord());

    //regexps in filter support
    if (filter && !postfix.GetRegex().GetMask().empty())
        dst->m_Attributes[AlphabetPrefix[postfix.GetRegex().GetNegative() ? NOT_REG_EXP : REG_EXP]] = postfix.GetRegex().GetMask();
}

#undef PUSH_FUZZY_OPTION
#undef PUSH_BOOL_OPTION

// CClonedRuleLoader::FDOLang_ReadFromRules version for NTomita::TFileProto
static void ConvertToParseRules(const TRuleGroups& srcRuleGroups, CParseGrammar* dstRules)
{
    // take only the first rule in each group
    for (size_t g = 0; g < srcRuleGroups.Size(); ++g) {
        const NTomita::TRuleProto& src = *srcRuleGroups.GetFirstRule(g);

        dstRules->m_Rules.push_back();
        CParseRule* dst = &dstRules->m_Rules.back();

        dst->m_LeftPart.m_ItemStrId = src.GetName();
        dst->m_LeftPart.m_Source = src.GetName();     // wtf?
        dst->m_LeftPart.m_Type = siMeta;              // wtf?

        dst->m_RightParts.push_back();
        CParseLeftPart& rightPart = dst->m_RightParts.back();
        for (size_t j = 0; j < src.ItemSize(); ++j) {
            rightPart.push_back();
            ConvertToGrammarItem(src.GetItem(j).GetTerm(), &rightPart.back(), false, src.GetItem(j).GetReplace());     // not a filter
        }
        rightPart.m_SourceLineNo = src.GetLocation().GetLine() + 1;
        rightPart.m_SourceLine = src.GetLocation().GetFile();
    }
}

static void ConvertToParseFilter(const NTomita::TFileProto& file, CWorkGrammar* dst)
{
    yvector<CGrammarItem>* uniqFilters = &dst->m_FilterStore.m_UniqueFilterItems;
    for (size_t i = 0; i < file.FilterSize(); ++i) {
        const NTomita::TFilterProto& src = file.GetFilter(i);
        yvector<CFilterPair> dstFilters;

        for (size_t j = 0; j < src.ItemSize(); ++j) {
            CGrammarItem dstItem;
            const NTomita::TFilterItemProto& item = src.GetItem(j);
            ConvertToGrammarItem(item.GetTerm(), &dstItem, true);       // a filter
            yvector<CGrammarItem>::const_iterator it_item = std::find(uniqFilters->begin(), uniqFilters->end(), dstItem);
            if (it_item != uniqFilters->end())
                dstFilters.push_back(CFilterPair(it_item - uniqFilters->begin(), item.GetDistance()));
            else {
                uniqFilters->push_back(dstItem);
                dstFilters.push_back(CFilterPair(uniqFilters->ysize() - 1, item.GetDistance()));
            }
        }
        yvector< yvector<CFilterPair> >& filterSet = dst->m_FilterStore.m_FilterSet;
        if (dstFilters.size() > 0 && std::find(filterSet.begin(), filterSet.end(), dstFilters) == filterSet.end()) {
            filterSet.push_back();
            filterSet.back().swap(dstFilters);
        }
    }
}

static void ConvertReduceAgrInfo(const NTomita::TRuleProto& rule, SRuleExternalInformationAndAgreements* info)
{
    yvector<SAgr> tmpArgs;
    ymap<ui32, size_t> index;      // agreement id -> index in tmpArgs

    // group agreements by id and convert groups to SAgr
    for (size_t i = 0; i < rule.ItemSize(); ++i) {
        const NTomita::TPostfixProto& postfix = rule.GetItem(i).GetTerm().GetPostfix();
        for (size_t j = 0; j < postfix.AgrSize(); ++j) {
            const NTomita::TAgrProto& tagr = postfix.GetAgr(j);
            TPair<ymap<ui32, size_t>::iterator, bool> ins = index.insert(MakePair(tagr.GetId(), tmpArgs.size()));
            if (ins.second) {
                // new id, create new rule agr:
                tmpArgs.push_back();
                SAgr& agr = tmpArgs.back();
                agr.m_bNegativeAgreement = tagr.GetNegative();
                switch (tagr.GetType()) {
                    case NTomita::Any:              agr.e_AgrProcedure = AnyFunc; break;
                    case NTomita::GenderNumberCase: agr.e_AgrProcedure = GendreNumberCase; break;
                    case NTomita::NumberCase:       agr.e_AgrProcedure = NumberCaseAgr; break;
                    case NTomita::Case:             agr.e_AgrProcedure = CaseAgr; break;
                    case NTomita::GenderNumber:     agr.e_AgrProcedure = GendreNumber; break;
                    case NTomita::GenderCase:       agr.e_AgrProcedure = GendreCase; break;
                    case NTomita::SubjectPredicate: agr.e_AgrProcedure = SubjVerb; break;
                    case NTomita::Fio:              agr.e_AgrProcedure = FioAgr; break;
                    case NTomita::FemCase:          agr.e_AgrProcedure = FeminCaseAgr; break;
                    case NTomita::Izafet:           agr.e_AgrProcedure = IzafetAgr; break;
                    case NTomita::AfterNumber:      agr.e_AgrProcedure = AfterNumAgr; break;
                    case NTomita::Geo:              agr.e_AgrProcedure = GeoAgr; break;
                    default: ythrow yexception() << "Unknown tomita agreement.";
                }
            }
            tmpArgs[ins.first->second].m_AgreeItems.push_back(i);
        }
    }

    // copy paired tmpAgr to info->m_RulesAgrs
    // since we have checked binary agreements last time, we could have lost some of item pair
    // during kleene operators removing.
    for (size_t i = 0; i < tmpArgs.size(); ++i) {
        if (tmpArgs[i].m_AgreeItems.size() == 2) {
            info->m_RulesAgrs.push_back();
            SAgr& agr = info->m_RulesAgrs.back();
            agr.m_bNegativeAgreement = tmpArgs[i].m_bNegativeAgreement;
            agr.e_AgrProcedure = tmpArgs[i].e_AgrProcedure;
            agr.m_AgreeItems.swap(tmpArgs[i].m_AgreeItems);
        }
    }

    // add quotation options as unary agreements
    for (size_t i = 0; i < rule.ItemSize(); ++i) {
        const NTomita::TPostfixProto& postfix = rule.GetItem(i).GetTerm().GetPostfix();
        if (postfix.GetLeftQuoted() != NTomita::TPostfixProto::UNDEF) {
            info->m_RulesAgrs.push_back();
            SAgr& agr = info->m_RulesAgrs.back();
            agr.m_bNegativeAgreement = (postfix.GetLeftQuoted() == NTomita::TPostfixProto::DENY);
            agr.m_AgreeItems.push_back(i);
            if (postfix.GetRightQuoted() == postfix.GetLeftQuoted()) {
                agr.e_AgrProcedure = CheckQuoted;
                continue;
            } else
                agr.e_AgrProcedure = CheckLQuoted;
        }
        if (postfix.GetRightQuoted() != NTomita::TPostfixProto::UNDEF) {
            info->m_RulesAgrs.push_back();
            SAgr& agr = info->m_RulesAgrs.back();
            agr.m_bNegativeAgreement = (postfix.GetRightQuoted() == NTomita::TPostfixProto::DENY);
            agr.m_AgreeItems.push_back(i);
            agr.e_AgrProcedure = CheckRQuoted;
        }
    }
}

static void ConvertToFactInterp(const NTomita::TInterpProto& interp, fact_field_reference_t* res,
                                const IFactTypeDictionary* factTypes)
{
    res->m_bConcatenation = interp.GetPlus();
    res->m_strFactTypeName = interp.GetFact();
    res->m_strFieldName = interp.GetField();

    YASSERT(factTypes != NULL);
    const fact_type_t* type = factTypes->GetFactType(interp.GetFact());
    YASSERT(type != NULL);
    const fact_field_descr_t* descr = type->get_field(interp.GetField());
    YASSERT(descr != NULL);
    res->m_Field_type = descr->m_Field_type;

    if (!interp.GetFromField().empty()) {
        res->m_strSourceFieldName = interp.GetFromField();
        if (!interp.GetFromFact().empty())
            res->m_strSourceFactName = interp.GetFromFact();
    }

    if (!interp.GetAssignedValue().empty()) {
        res->m_bHasValue = true;
        if (interp.GetAssignedValue() == "true")
            res->m_bBoolValue = true;
        else if (interp.GetAssignedValue() == "false")
            res->m_bBoolValue = false;
        else
            res->m_StringValue = NStr::DecodeTomita(interp.GetAssignedValue());
    }

    res->m_eFactAlgorithm.Reset();
    for (size_t i = 0; i < interp.OptionSize(); ++i)
        res->m_eFactAlgorithm.SafeSet(::Str2FactAlgorithm(interp.GetOption(i).c_str()));

    if (!interp.GetNormGrammems().empty()) {
        res->m_iNormGrammems = TGramBitSet::FromBytes(interp.GetNormGrammems());
    }
}

static inline SArtPointer MakeArtPointer(const Stroka name, const IArticleNames* dict)
{
    TKeyWordType kwtype = dict->GetKWType(name);
    if (kwtype == NULL)
        return SArtPointer(NStr::DecodeTomita(name));
    else
        return SArtPointer(kwtype);
}

static void ConvertKWSet(const NTomita::TKeyWordsProto& src, SKWSet* dst,
                         const IArticleNames* dict)
{
    YASSERT(dict != NULL);
    dst->m_bNegative = src.GetNegative();
    dst->m_bApplyToTheFirstWord = src.GetFirstWord();
    for (size_t i = 0; i < src.KeyWordSize(); ++i)
        dst->m_KWset.insert(MakeArtPointer(src.GetKeyWord(i), dict));
}

static void ConvertRegex(const NTomita::TRegexProto& src, CWordFormRegExp* dst)
{
    dst->SetRegexUtf8(src.GetMask());
    dst->IsNegative = src.GetNegative();
    switch (src.GetWord()) {
        case NTomita::TRegexProto::FIRST: dst->Place = CWordFormRegExp::ApplyToFirstWord; break;
        case NTomita::TRegexProto::MAIN:  dst->Place = CWordFormRegExp::ApplyToMainWord; break;
        case NTomita::TRegexProto::LAST:  dst->Place = CWordFormRegExp::ApplyToLastWord; break;
        default: ythrow yexception() << "Unknown NTomita::TRegexProto::EType value.";
    }
}

static void ConvertTermReduceInfo(const NTomita::TRuleItemProto& item, size_t index, SRuleExternalInformationAndAgreements* info,
                                  const IArticleNames* kwtypes, const IFactTypeDictionary* factTypes)
{
    const NTomita::TPostfixProto& p = item.GetTerm().GetPostfix();
    // LoadRuleAgreement(r_term, iItem, rRule);  - agr is collected separately, see CollectReduceAgrInfo

    // LoadRuleUnaryAgreement(r_term, iItem, rRule);
    if (item.GetTerm().GetType() == NTomita::TTermProto::NON_TERMINAL)
        for (size_t i = 0; i < p.GramSize(); ++i)
            info->m_RulesAgrs.push_back(SAgr(index, TGramBitSet::FromBytes(p.GetGram(i).GetGrammems()), p.GetGram(i).GetNegative()));

    // LoadRuleKWSets(r_term, iItem);
    if (p.GetKWSet().KeyWordSize() > 0)
        ConvertKWSet(p.GetKWSet(), &info->m_KWSets[index], kwtypes);

    // LoadRuleFactFieldInterpretation(r_term, iItem);
    for (size_t i = 0; i < item.InterpSize(); ++i) {
        yvector<fact_field_reference_t>& interps = info->m_FactsInterpretation[index];
        interps.push_back();
        ConvertToFactInterp(item.GetInterp(i), &interps.back(), factTypes);
    }

    // LoadRuleWFSets(r_term, iItem);
    if (!p.GetRegex().GetMask().empty())
        ConvertRegex(p.GetRegex(), &info->m_WFRegExps[index]);

    //LoadRuleGrammemUnion(r_term, iItem);
    for (size_t i = 0; i < p.GramUnionSize(); ++i) {
        SGrammemUnion& gu = info->m_RuleGrammemUnion[index];
        TGramBitSet gram = TGramBitSet::FromBytes(p.GetGramUnion(i).GetGrammems());
        bool negative = p.GetGramUnion(i).GetNegative();
        gu.m_GramUnion.push_back(negative ? TGramBitSet() : gram);
        gu.m_NegGramUnion.push_back(negative ? gram : TGramBitSet());
        gu.m_CheckFormGrammem.push_back(!negative && !p.GetGramUnion(i).GetMixForms());     // mix forms when negative anyway (original behaviour)
    }

    //LoadRuleIsFirstWordConstraint(r_term, iItem);
    if (p.GetFirstWord() != NTomita::TPostfixProto::UNDEF)
        info->m_FirstWordConstraints[index] = (p.GetFirstWord() == NTomita::TPostfixProto::ALLOW);

    if (p.HasGztWeight()) {
        SArtPointer art = MakeArtPointer(p.GetGztWeight().GetArticle(), kwtypes);
        info->m_GztWeight[index] = TGztArticleField(art, p.GetGztWeight().GetField());
    }
}

static void ConvertRuleOutInfo(const NTomita::TRuleInfoProto& src, SExtraRuleInfo* dst)
{
    dst->clear();
    if (!src.GetOutgram().empty())
        dst->m_OutGrammems = TGramBitSet::FromBytes(src.GetOutgram());
    dst->m_OutWeight = src.GetWeight();
    dst->m_OutCount = src.GetCount();
    dst->m_bTrim = src.GetTrim();
    if (src.GetLeftDominant())
        dst->m_DomReces.SetDominant();
    else if (src.GetRightRecessive())
        dst->m_DomReces.SetRecessive();
    dst->m_bNotHRegFact = src.GetNotHRegFact();
}

static void ConvertRuleReduceInfo(const NTomita::TRuleProto& rule, CRuleAgreement& info,
                                  const IArticleNames* kwtypes, const IFactTypeDictionary* factTypes)
{
    info.push_back();
    SRuleExternalInformationAndAgreements& item = info.back();
    ConvertReduceAgrInfo(rule, &item);
    for (size_t j = 0; j < rule.ItemSize(); ++j)
        ConvertTermReduceInfo(rule.GetItem(j), j, &item, kwtypes, factTypes);
    ConvertRuleOutInfo(rule.GetInfo(), &item.m_ExtraRuleInfo);
}

static void ConvertReduceInfo(const TRuleGroups& groups, yvector<CRuleAgreement>& info,
                              const IArticleNames* kwtypes, const IFactTypeDictionary* factTypes)
{
    for (size_t i = 0; i < groups.Size(); ++i) {
        info.push_back();
        ConvertRuleReduceInfo(*groups.GetFirstRule(i), info.back(), kwtypes, factTypes);
        const yvector<const NTomita::TRuleProto*>& similar = groups.GetSimilarRules(i);
        for (size_t j = 0; j < similar.size(); ++j)
            ConvertRuleReduceInfo(*similar[j], info.back(), kwtypes, factTypes);
    }
}

bool TTomitaCompiler::BuildGrammar(const NTomita::TFileProto& file, CWorkGrammar* target)
{
    CParseGrammar loader;
    loader.m_Language = target->m_Language;

    // group rule to avoid duplicate on grammar encoding
    TRuleGroups groups(file);

    yvector<CRuleAgreement> agreements;
    ConvertReduceInfo(groups, agreements, Articles, FactTypes);

    ConvertToParseRules(groups, &loader);
    DO(loader.AssignSynMainItemIfNotDefined());
    DO(loader.CheckCoherence());
    loader.EncodeGrammar(*target);
    ConvertToParseFilter(file, target);
    if (target->GetCountOfRoots() == 0 && file.RuleSize() > 0) {
        AddError(file.GetName(), "No roots found in grammar.");
        return false;
    }

    if (!target->IsValid())
        return false;

    if (file.RuleSize() > 0) {
        target->Build_FIRST_Set_k(1);
        target->BuildAutomat(target->m_RootPrefixes);
        DO(target->AugmentGrammar(agreements));
        target->Build_FIRST_Set();
        target->Build_FOLLOW_Set();
        target->TransferOutCountFromSymbolToItsDependencies(agreements);
        DO(target->m_GLRTable.BuildGLRTable(file.GetName()));
    }

    target->m_ExternalGrammarMacros.m_b10FiosLimit = file.GetFioLimit() > 0;
    target->m_ExternalGrammarMacros.m_bNoInterpretation = file.GetNoInterp();
    for (size_t i = 0; i < file.ExtraKeywordSize(); ++i)
        target->m_ExternalGrammarMacros.m_KWsNotInGrammar.insert(MakeArtPointer(file.GetExtraKeyword(i), Articles));

    target->m_GLRTable.SwapRuleAgreements(agreements);

    return true;
}

static inline void AddDependency(const TStringBuf& basedir, const Stroka& file, CWorkGrammar* target)
{
    if (!file.empty())
        target->SourceFiles[file] = NUtil::CalculateFileChecksum(PathHelper::Join(basedir, file));
}

void TTomitaCompiler::CollectDependencies(const TStringBuf& baseDir, const NTomita::TFileProto& file, CWorkGrammar* target)
{
    target->SourceFiles.clear();
    for (size_t i = 0; i < file.IncludeSize(); ++i)
        AddDependency(baseDir, file.GetInclude(i), target);
    AddDependency(baseDir, file.GetName(), target);
}

class TTomitaDependencies: public TBinaryGuard {
public:
    TTomitaDependencies(const Stroka& srcFile)
        : BaseDir(PathHelper::DirName(srcFile))
    {
    }

    virtual bool LoadDependencies(const Stroka& binFile, yvector<TBinaryGuard::TDependency>& res) const {
        CWorkGrammar tmpGrammar;
        {
            TIFStream f(binFile);
            if (!tmpGrammar.LoadHeader(&f))
                return false;
        }

        for (ymap<Stroka, Stroka>::const_iterator it = tmpGrammar.SourceFiles.begin(); it != tmpGrammar.SourceFiles.end(); ++it)
            res.push_back(TBinaryGuard::TDependency(it->first, it->second, false));
        return true;
    }

    virtual Stroka GetDiskFileName(const TBinaryGuard::TDependency& dep) const {
        return PathHelper::Join(BaseDir, dep.Name);
    }
private:
    Stroka BaseDir;
};

bool TTomitaCompiler::IsGoodBinary(const Stroka& srcFile, const Stroka& binFile) {
    return TTomitaDependencies(srcFile).IsGoodBinary(srcFile, binFile);
}

bool TTomitaCompiler::CompilationRequired(const Stroka& srcFile, const Stroka& binFile, TBinaryGuard::ECompileMode mode) {
    return TTomitaDependencies(srcFile).RebuildRequired(srcFile, binFile, mode);
}

#undef DO
