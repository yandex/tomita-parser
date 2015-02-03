#include "last_getopt.h"

#include <util/stream/format.h>
#include <util/string/escape.h>
#include <util/generic/ylimits.h>
#include <util/generic/utility.h>
#include <util/generic/algorithm.h>

#if defined(YMAKE)
    #include <library/svnversion/svnversion.h>
#endif

#include <ctype.h>

namespace NLastGetopt {

static const Stroka DefaultHelp = "No help, sorry";
static const TStringBuf ExcludedShortNameChars = STRINGBUF("= -");
static const TStringBuf ExcludedLongNameChars = STRINGBUF("= ");

static const TStringBuf SPad = STRINGBUF("  ");

// Like Stroka::Quote(), but does not quote digits-only string
static Stroka QuoteForHelp(const Stroka& str) {
    if (str.empty())
        return str.Quote();
    for (size_t i = 0; i < str.size(); ++i) {
        if (!isdigit(str[i]))
            return str.Quote();
    }
    return str;
}

namespace NPrivate {
    Stroka OptToString(char c) {
        TStringStream ss;
        ss << "-" << c;
        return ss;
    }

    Stroka OptToString(const Stroka& longOption) {
        TStringStream ss;
        ss << "--" << longOption;
        return ss;
    }

    Stroka OptToString(const TOpt* opt) {
        return opt->ToShortString();
    }

}

bool TOpt::NameIs(const Stroka& name) const {
    for (yvector<Stroka>::const_iterator it = LongNames_.begin(); it != LongNames_.end(); ++it) {
        const Stroka& next = *it;
        if (next == name)
            return true;
    }
    return false;
}

bool TOpt::CharIs(char c) const {
    for (yvector<char>::const_iterator it = Chars_.begin(); it != Chars_.end(); ++it) {
        char next = *it;
        if (next == c)
            return true;
    }
    return false;
}

char TOpt::GetChar() const {
    if (Chars_.empty())
        ythrow TConfException() << "no char for option " << this->ToShortString();
    return Chars_.at(0);
}

char TOpt::GetCharOr0() const {
    if (Chars_.empty())
        return 0;
    return GetChar();
}

Stroka TOpt::GetName() const {
    if (LongNames_.empty())
        ythrow TConfException() << "no name for option " << this->ToShortString();
    return LongNames_.at(0);
}

bool TOpt::IsAllowedShortName(unsigned char c) {
    return isprint(c) && TStringBuf::npos == ExcludedShortNameChars.find(c);
}

TOpt& TOpt::AddShortName(unsigned char c) {
    if (!IsAllowedShortName(c))
        ythrow TUsageException() << "option char '" << c << "' is not allowed";
    Chars_.push_back(c);
    return *this;
}

bool TOpt::IsAllowedLongName(const Stroka& name, unsigned char* out) {
    for (size_t i = 0; i != name.size(); ++i) {
        const unsigned char c = name[i];
        if (!isprint(c) || TStringBuf::npos != ExcludedLongNameChars.find(c)) {
            if (NULL != out)
                *out = c;
            return false;
        }
    }
    return true;
}

TOpt& TOpt::AddLongName(const Stroka& name) {
    unsigned char c = 0;
    if (!IsAllowedLongName(name, &c))
        ythrow TUsageException() << "option char '" << c
            << "' in long '" << name << "' is not allowed";
    LongNames_.push_back(name);
    return *this;
}


Stroka TOpt::ToShortString() const {
    if (!LongNames_.empty())
        return NPrivate::OptToString(LongNames_.front());
    if (!Chars_.empty())
        return NPrivate::OptToString(Chars_.front());
    return "?";
}

TOpts::TOpts(const TStringBuf& optstring)
    : ArgPermutation_(DEFAULT_ARG_PERMUTATION)
    , AllowSingleDashForLong_(false)
    , AllowPlusForLong_(false)
    , AllowUnknownCharOptions_(false)
    , AllowUnknownLongOptions_(false)
    , FreeArgsMin_(0)
    , FreeArgsMax_(Max<ui32>())
{
    if (!optstring.empty()) {
        AddCharOptions(optstring);
    }
    AddVersionOption(0);
}

void TOpts::AddCharOptions(const TStringBuf& optstring) {
    size_t p = 0;
    if (optstring[p] == '+') {
        ArgPermutation_ = REQUIRE_ORDER;
        ++p;
    } else if (optstring[p] == '-') {
        ArgPermutation_ = RETURN_IN_ORDER;
        ++p;
    }

    while (p < optstring.size()) {
        char c = optstring[p];
        p++;
        EHasArg ha = NO_ARGUMENT;
        if (p < optstring.size() && optstring[p] == ':') {
            ha = REQUIRED_ARGUMENT;
            p++;
        }
        if (p < optstring.size() && optstring[p] == ':') {
            ha = OPTIONAL_ARGUMENT;
            p++;
        }
        AddCharOption(c, ha);
    }
}

const TOpt* TOpts::FindLongOption(const TStringBuf& name) const {
    for (TOptsVector::const_iterator it = Opts_.begin(); it != Opts_.end(); ++it) {
        const TOpt* opt = it->Get();
        if (IsIn(opt->GetLongNames(), name))
            return opt;
    }
    return 0;
}

TOpt* TOpts::FindLongOption(const TStringBuf& name) {
    for (TOptsVector::iterator it = Opts_.begin(); it != Opts_.end(); ++it) {
        TOpt* opt = it->Get();
        if (IsIn(opt->GetLongNames(), name))
            return opt;
    }
    return 0;
}

const TOpt* TOpts::FindCharOption(char c) const {
    for (TOptsVector::const_iterator it = Opts_.begin(); it != Opts_.end(); ++it) {
        const TOpt* opt = it->Get();
        if (IsIn(opt->GetShortNames(), c))
            return opt;
    }
    return 0;
}

TOpt* TOpts::FindCharOption(char c) {
    for (TOptsVector::iterator it = Opts_.begin(); it != Opts_.end(); ++it) {
        TOpt* opt = it->Get();
        if (IsIn(opt->GetShortNames(), c))
            return opt;
    }
    return 0;
}

const TOpt& TOpts::GetCharOption(char c) const {
    const TOpt* option = FindCharOption(c);
    if (!option)
        ythrow TException() << "unknown char option '" << c << "'";
    return *option;
}

TOpt& TOpts::GetCharOption(char c) {
    TOpt* option = FindCharOption(c);
    if (!option)
        ythrow TException() << "unknown char option '" << c << "'";
    return *option;
}

const TOpt& TOpts::GetLongOption(const TStringBuf& name) const {
    const TOpt* option = FindLongOption(name);
    if (!option)
        ythrow TException() << "unknown option " << name;
    return *option;
}

TOpt& TOpts::GetLongOption(const TStringBuf& name) {
    TOpt* option = FindLongOption(name);
    if (!option)
        ythrow TException() << "unknown option " << name;
    return *option;
}

bool TOpts::HasAnyShortOption() const {
    for (TOptsVector::const_iterator it = Opts_.begin(); it != Opts_.end(); ++it) {
        const TOpt* opt = it->Get();
        if (!opt->GetShortNames().empty())
            return true;
    }
    return false;
}

bool TOpts::HasAnyLongOption() const {
    for (TOptsVector::const_iterator it = Opts_.begin(); it != Opts_.end(); ++it) {
        TOpt* opt = it->Get();
        if (!opt->GetLongNames().empty())
            return true;
    }
    return false;
}

void TOpts::Validate() const {
    for (TOptsVector::const_iterator i = Opts_.begin(); i != Opts_.end(); ++i) {
        TOpt* opt = i->Get();
        const TOpt::TShortNames& shortNames = opt->GetShortNames();
        for (TOpt::TShortNames::const_iterator it = shortNames.begin(); it != shortNames.end(); ++it) {
            char c = *it;
            for (TOptsVector::const_iterator j = i + 1; j != Opts_.end(); ++j) {
                TOpt* nextOpt = j->Get();
                if (nextOpt->CharIs(c))
                    ythrow TConfException() << "option "
                        << NPrivate::OptToString(c)
                        << " is defined more than once";
            }
        }
        const TOpt::TLongNames& longNames = opt->GetLongNames();
        for (TOpt::TLongNames::const_iterator it = longNames.begin(); it != longNames.end(); ++it) {
            const Stroka& longName = *it;
            for (TOptsVector::const_iterator j = i + 1; j != Opts_.end(); ++j) {
                TOpt* nextOpt = j->Get();
                if (nextOpt->NameIs(longName))
                    ythrow TConfException() << "option "
                        << NPrivate::OptToString(longName)
                        << " is defined more than once";
            }
        }
    }
    if (FreeArgsMax_ < FreeArgsMin_) {
        ythrow TConfException() << "FreeArgsMax must be >= FreeArgsMin";
    }
    if (FreeArgSpecs_.size() > FreeArgsMax_) {
        ythrow TConfException() << "Described args count is greater than FreeArgsMax. Either increase FreeArgsMax or remove unreachable descriptions";
    }
}

TOpt& TOpts::AddOption(const TOpt& option) {
    if (option.GetShortNames().empty() && option.GetLongNames().empty())
        ythrow TConfException() << "bad option: no chars, no long names";
    Opts_.push_back(new TOpt(option));
    return *Opts_.back();
}

size_t TOpts::IndexOf(const TOpt* opt) const {
    TOptsVector::const_iterator it = NStl::find(Opts_.begin(), Opts_.end(), opt);
    if (it == Opts_.end())
        throw TException() << "unknown option";
    return it - Opts_.begin();
}

const Stroka& TOpts::GetFreeArgTitle(size_t pos) const {
    if (pos < FreeArgSpecs_.size()) {
        const Stroka& title = FreeArgSpecs_.at(pos).Title;
        if (!title.Empty())
            return title;
    }
    return DefaultArgTitle_;
}

const Stroka& TOpts::GetFreeArgHelp(size_t pos) const {
    if (pos < FreeArgSpecs_.size()) {
        const Stroka& help = FreeArgSpecs_.at(pos).Help;
        if (!help.Empty())
            return help;
    }
    return DefaultHelp;
}

void TOpts::SetFreeArgTitle(size_t pos, const Stroka& title, const Stroka& help) {
    if (FreeArgSpecs_.size() <= pos)
        FreeArgSpecs_.resize(pos + 1);
    FreeArgSpecs_[pos] = TFreeArgSpec(title, help);
}

void TOpts::SetFreeArgDefaultTitle(const Stroka& title, const Stroka& help) {
    DefaultArgTitle_ = title;
    DefaultArgHelp_ = help;
    CustomDefaultArg_ = true;
}

static Stroka FormatOption(const TOpt* option) {
    TStringStream result;
    const TOpt::TShortNames& shorts = option->GetShortNames();
    const TOpt::TLongNames& longs = option->GetLongNames();

    const size_t nopts = shorts.size() + longs.size();
    const bool multiple = 1 < nopts;
    if (multiple)
        result << '{';
    for (size_t i = 0; i < nopts; ++i) {
        if (multiple && 0 != i)
            result << '|';

        if (i < shorts.size()) // short
            result << '-' << shorts[i];
        else
            result << "--" << longs[i - shorts.size()];
    }
    if (multiple)
        result << '}';

    static const Stroka metavarDef("VAL");
    const Stroka& title = option->ArgTitle_;
    const Stroka& metavar = title.Empty() ? metavarDef : title;

    if (option->HasArg_ == OPTIONAL_ARGUMENT) {
        result << " [" << metavar;
        if (option->HasOptionalValue())
            result << ':' << option->GetOptionalValue();
        result << ']';
    } else if (option->HasArg_ == REQUIRED_ARGUMENT)
        result << ' ' << metavar;
    else
        YASSERT(option->HasArg_ == NO_ARGUMENT);

    return result.Str();
}

void TOpts::PrintCmdLine(const TStringBuf& program, TOutputStream& os) const {
    os << "Usage: " << program << " [OPTIONS]";

    for (ui32 i = 0; i < FreeArgsMin_; ++i)
        os << ' ' << GetFreeArgTitle(i);

    if (FreeArgsMax_ > FreeArgsMin_) {
        if (FreeArgsMax_ == Max<ui32>()) {
            // print all described args
            for (ui32 i = FreeArgsMin_; i < FreeArgSpecs_.size(); ++i)
                os << " [" << GetFreeArgTitle(i) << "]";
            os << " [" << DefaultArgTitle_ << "]...";
        } else {
            for (ui32 i = FreeArgsMin_; i < FreeArgsMax_; ++i)
                os << " [" << GetFreeArgTitle(i) << "]";
        }
    }

    os << Endl;
}

void TOpts::PrintUsage(const TStringBuf& program, TOutputStream& os) const {
    if (!Title.empty())
        os << Title << "\n\n";

    PrintCmdLine(program, os);

    yvector<Stroka> leftColumn(Opts_.size());
    size_t leftWidth = 0;
    size_t requiredOptionsCount = 0;

    for (size_t i = 0; i < Opts_.size(); i++) {
        const TOpt* opt = Opts_[i].Get();
        if (opt->IsHidden())
            continue;
        leftColumn[i] = FormatOption(opt);
        leftWidth = Max(leftWidth, leftColumn[i].size());
        if (opt->IsRequired())
            requiredOptionsCount++;
    }

    const size_t kMaxLeftWidth = 25;
    leftWidth = Min(leftWidth, kMaxLeftWidth);
    const Stroka maxPadding(kMaxLeftWidth, ' ');
    const Stroka leftPadding(leftWidth, ' ');

    for (size_t sectionId = 0; sectionId <= 1; sectionId++) {
        bool requiredOptionsSection = (sectionId == 0);

        if (requiredOptionsSection) {
            if (requiredOptionsCount == 0)
                continue;
            os << Endl << "Required parameters:" << Endl;
        } else {
            if (requiredOptionsCount == Opts_.size())
                continue;
            if (requiredOptionsCount == 0)
                os << Endl << "Options:" << Endl;
            else
                os << Endl << "Optional parameters:" << Endl;  // optional options would be a tautology
        }

        for (size_t i = 0; i < Opts_.size(); i++) {
            const TOpt* opt = Opts_[i].Get();

            if (opt->IsHidden())
                continue;
            if (opt->IsRequired() != requiredOptionsSection)
                continue;

            if (leftColumn[i].size() > leftWidth && !opt->Help_.empty())
                os <<
                    SPad << leftColumn[i] << Endl <<
                    SPad << maxPadding << ' ';
            else
                os << SPad << RightPad(leftColumn[i], leftWidth, ' ') << ' ';

            bool multiLineHelp = false;
            if (!opt->Help_.empty()) {
                yvector<TStringBuf> helpLines;
                Split(opt->Help_, "\n", helpLines);
                multiLineHelp = (helpLines.size() > 1);
                os << helpLines[0];
                for (size_t j = 1; j < helpLines.size(); ++j) {
                    if (helpLines[j].empty())
                        continue;
                    os << Endl << SPad << leftPadding << ' ' << helpLines[j];
                }
            }

            if (opt->HasDefaultValue()) {
                Stroka quotedDef = QuoteForHelp(opt->GetDefaultValue());
                if (multiLineHelp)
                    os << Endl << SPad << leftPadding << " Default: " << quotedDef;
                else if (opt->Help_.empty())
                    os << "Default: " << quotedDef;
                else
                    os << " (default: " << quotedDef << ")";
            }

            os << Endl;
        }
    }
    PrintFreeArgsDesc(os);
}

void TOpts::PrintFreeArgsDesc(TOutputStream& os) const {
    if (0 == FreeArgsMax_)
        return;

    size_t leftFreeWidth = 0;
    for (size_t i = 0; i < FreeArgSpecs_.size(); ++i) {
        leftFreeWidth = Max(leftFreeWidth, GetFreeArgTitle(i).size());
    }

    if (CustomDefaultArg_) {
        leftFreeWidth = Max(leftFreeWidth, DefaultArgTitle_.size());
    }

    leftFreeWidth = Min(leftFreeWidth, size_t(30));
    os << Endl << "Free args:";

    os << " min: " << FreeArgsMin_ << ",";
    os << " max: ";
    if (FreeArgsMax_ != Max<ui32>()) {
        os << FreeArgsMax_;
    } else {
        os << "unlimited";
    }

    os << " (listed described args only)" << Endl;
    for (size_t i = 0; i < FreeArgSpecs_.size(); ++i) {
        const Stroka& help = GetFreeArgHelp(i);
        os << "  " << RightPad(GetFreeArgTitle(i), leftFreeWidth, ' ');

        if (!help.Empty())
            os << "  " << help;

        os << Endl;
    }

    if (CustomDefaultArg_) {
        os << "  " << RightPad(DefaultArgTitle_, leftFreeWidth, ' ');

        if (!DefaultArgHelp_.Empty())
            os << "  " << (!DefaultArgHelp_ ? DefaultHelp : DefaultArgHelp_);

        os << Endl;
    }
}

void TOpt::FireHandlers(const TOptsParser* parser) const {
    for (TOptHandlers::const_iterator i = Handlers_.begin(); i != Handlers_.end(); ++i) {
        (*i)->HandleOpt(parser);
    }
}

void PrintUsageAndExit(const TOptsParser* parser) {
    parser->PrintUsage();
    exit(0);
}

void PrintVersionAndExit(const TOptsParser*) {
    int retCode = 0;
#if defined(PROGRAM_VERSION)
    Cout << PROGRAM_VERSION << Endl;
#elif defined(SVN_REVISION)
    Cout << "revision: " << SVN_REVISION << " from " << SVN_ARCROOT << " at " << SVN_TIME << Endl;
#elif defined(GIT_TAG)
    Cout << "revision: " << GIT_TAG << Endl;
#else
    Cerr << "program version: not implemented" << Endl;
    retCode = 1;
#endif
    exit(retCode);
}

void TOptsParser::Init(const TOpts* opts, int argc, const char* argv[]) {
    opts->Validate();

    Opts_ = opts;

    if (argc < 1)
        ythrow TUsageException() << "argv must have at least one argument";

    Argc_ = argc;
    Argv_ = argv;

    ProgramName_ = argv[0];

    Pos_ = 1;
    Sop_ = 0;
    CurrentOpt_ = 0;
    CurrentValue_ = 0;
    GotMinusMinus_ = false;
    Stopped_ = false;
    OptsSeen_.clear();
    OptsDefault_.clear();
}

void TOptsParser::Init(const TOpts* opts, int argc, char* argv[]) {
    Init(opts, argc, const_cast<const char**>(argv));
}

void TOptsParser::Swap(TOptsParser& that) {
    DoSwap(Opts_, that.Opts_);
    DoSwap(Argc_, that.Argc_);
    DoSwap(Argv_, that.Argv_);
    DoSwap(TempCurrentOpt_, that.TempCurrentOpt_);
    DoSwap(ProgramName_, that.ProgramName_);
    DoSwap(Pos_, that.Pos_);
    DoSwap(Sop_, that.Sop_);
    DoSwap(Stopped_, that.Stopped_);
    DoSwap(CurrentOpt_, that.CurrentOpt_);
    DoSwap(CurrentValue_, that.CurrentValue_);
    DoSwap(GotMinusMinus_, that.GotMinusMinus_);
    DoSwap(OptsSeen_, that.OptsSeen_);
}

bool TOptsParser::Commit(const TOpt* currentOpt, const TStringBuf& currentValue, size_t pos, size_t sop) {
    Pos_ = pos;
    Sop_ = sop;
    CurrentOpt_ = currentOpt;
    CurrentValue_ = currentValue;
    if (NULL != currentOpt)
        OptsSeen_.insert(currentOpt);
    return true;
}

bool TOptsParser::CommitEndOfOptions(size_t pos) {
    Pos_ = pos;
    Sop_ = 0;
    YASSERT(!CurOpt());
    YASSERT(!CurVal());

    YASSERT(!Stopped_);

    if (Opts_->FreeArgsMin_ == Opts_->FreeArgsMax_ && Argc_ - Pos_ != Opts_->FreeArgsMin_)
        ythrow TUsageException() << "required exactly " << Opts_->FreeArgsMin_ << " free args";
    else if (Argc_ - Pos_ < Opts_->FreeArgsMin_)
        ythrow TUsageException() << "required at least " << Opts_->FreeArgsMin_ << " free args";
    else if (Argc_ - Pos_ > Opts_->FreeArgsMax_)
        ythrow TUsageException() << "required at most " << Opts_->FreeArgsMax_ << " free args";

    return false;
}

bool TOptsParser::ParseUnknownShortOptWithinArg(size_t pos, size_t sop) {
    YASSERT(pos < Argc_);
    const TStringBuf arg(Argv_[pos]);
    YASSERT(sop > 0);
    YASSERT(sop < arg.length());
    YASSERT(EIO_NONE != IsOpt(arg));

    if (!Opts_->AllowUnknownCharOptions_)
        ythrow TUsageException() << "unknown option '" << EscapeC(arg[sop])
            << "' in '" << arg << "'";

    TempCurrentOpt_.Reset(new TOpt);
    TempCurrentOpt_->AddShortName(arg[sop]);

    sop += 1;

    // mimic behavior of Opt: unknown option has arg only if char is last within arg
    if (sop < arg.length()) {
        return Commit(TempCurrentOpt_.Get(), 0, pos, sop);
    }

    pos += 1;
    sop = 0;
    if (pos == Argc_ || EIO_NONE != IsOpt(Argv_[pos])) {
        return Commit(TempCurrentOpt_.Get(), 0, pos, 0);
    }

    return Commit(TempCurrentOpt_.Get(), Argv_[pos], pos + 1, 0);
}

bool TOptsParser::ParseShortOptWithinArg(size_t pos, size_t sop) {
    YASSERT(pos < Argc_);
    const TStringBuf arg(Argv_[pos]);
    YASSERT(sop > 0);
    YASSERT(sop < arg.length());
    YASSERT(EIO_NONE != IsOpt(arg));

    size_t p = sop;
    char c = arg[p];
    const TOpt* opt = Opts_->FindCharOption(c);
    if (!opt)
        return ParseUnknownShortOptWithinArg(pos, sop);
    p += 1;
    if (p == arg.length()) {
        return ParseOptParam(opt, pos + 1);
    }
    if (opt->HasArg_ == NO_ARGUMENT) {
        return Commit(opt, 0, pos, p);
    }
    return Commit(opt, arg + p, pos + 1, 0);
}

bool TOptsParser::ParseShortOptArg(size_t pos) {
    YASSERT(pos < Argc_);
    const TStringBuf arg(Argv_[pos]);
    YASSERT(EIO_NONE != IsOpt(arg));
    YASSERT(!arg.has_prefix("--"));
    return ParseShortOptWithinArg(pos, 1);
}

bool TOptsParser::ParseOptArg(size_t pos) {
    YASSERT(pos < Argc_);
    TStringBuf arg(Argv_[pos]);
    const EIsOpt eio = IsOpt(arg);
    YASSERT(EIO_NONE != eio);
    if (EIO_DDASH == eio || EIO_PLUS == eio || (Opts_->AllowSingleDashForLong_ || !Opts_->HasAnyShortOption())) {
        // long option
        bool singleCharPrefix = EIO_DDASH != eio;
        arg.Skip(singleCharPrefix ? 1 : 2);
        TStringBuf optionName = arg.NextTok('=');
        const TOpt* option = Opts_->FindLongOption(optionName);
        if (!option) {
            if (singleCharPrefix && !arg.IsInited()) {
                return ParseShortOptArg(pos);
            } else {
                ythrow TUsageException() << "unknown option '" << optionName
                    << "' in '" << Argv_[pos] << "'";
            }
        }
        if (arg.IsInited()) {
            if (option->HasArg_ == NO_ARGUMENT)
                ythrow TUsageException() << "option " << optionName << " must have no arg";
            return Commit(option, arg, pos + 1, 0);
        }
        ++pos;
        return ParseOptParam(option, pos);
    } else {
        return ParseShortOptArg(pos);
    }
}

bool TOptsParser::ParseOptParam(const TOpt* opt, size_t pos) {
    YASSERT(opt);
    if (opt->HasArg_ == NO_ARGUMENT) {
        return Commit(opt, 0, pos, 0);
    }
    if (pos == Argc_) {
        if (opt->HasArg_ == REQUIRED_ARGUMENT)
            ythrow TUsageException() << "option " << opt->ToShortString() << " must have arg";
        return Commit(opt, 0, pos, 0);
    }
    const TStringBuf arg(Argv_[pos]);
    if (!arg.has_prefix("-") || opt->HasArg_ == REQUIRED_ARGUMENT) {
        return Commit(opt, arg, pos + 1, 0);
    }
    return Commit(opt, 0, pos, 0);
}

TOptsParser::EIsOpt TOptsParser::IsOpt(const TStringBuf& arg) const {
    EIsOpt eio = EIO_NONE;
    if (1 < arg.length()) {
        switch (arg[0]) {
        default:
            break;
        case '-':
            if ('-' != arg[1])
                eio = EIO_SDASH;
            else if (2 < arg.length())
                eio = EIO_DDASH;
            break;
        case '+':
            if (Opts_->AllowPlusForLong_)
                eio = EIO_PLUS;
            break;
        }
    }
    return eio;
}

static void memrotate(void* ptr, size_t size, size_t shift) {
    TTempBuf buf(shift);
    memcpy(buf.Data(), (char*) ptr + size - shift, shift);
    memmove((char*) ptr + shift, ptr, size - shift);
    memcpy(ptr, buf.Data(), shift);
}

bool TOptsParser::ParseWithPermutation() {
    YASSERT(Sop_ == 0);
    YASSERT(Opts_->ArgPermutation_ == PERMUTE);

    const size_t p0 = Pos_;

    size_t pc = Pos_;

    for (; pc < Argc_ && EIO_NONE == IsOpt(Argv_[pc]); ++pc) {
        // count non-args
    }

    if (pc == Argc_) {
        return CommitEndOfOptions(Pos_);
    }

    Pos_ = pc;

    bool r = ParseOptArg(Pos_);
    YASSERT(r);
    while (Pos_ == pc) {
        YASSERT(Sop_ > 0);
        r = ParseShortOptWithinArg(Pos_, Sop_);
        YASSERT(r);
    }

    size_t p2 = Pos_;

    YASSERT(p2 - pc >= 1);
    YASSERT(p2 - pc <= 2);

    memrotate(Argv_ + p0, (p2 - p0) * sizeof(void*), (p2 - pc) * sizeof(void*));

    bool r2 = ParseOptArg(p0);
    YASSERT(r2);
    return r2;
}

bool TOptsParser::DoNext() {
    YASSERT(Pos_ <= Argc_);

    if (Pos_ == Argc_)
        return CommitEndOfOptions(Pos_);

    if (GotMinusMinus_ && Opts_->ArgPermutation_ == RETURN_IN_ORDER) {
        YASSERT(Sop_ == 0);
        return Commit(0, Argv_[Pos_], Pos_ + 1, 0);
    }

    if (Sop_ > 0)
        return ParseShortOptWithinArg(Pos_, Sop_);

    size_t pos = Pos_;
    const TStringBuf arg(Argv_[pos]);
    if (EIO_NONE != IsOpt(arg)) {
        return ParseOptArg(pos);
    } else if (arg == "--") {
        if (Opts_->ArgPermutation_ == RETURN_IN_ORDER) {
            pos += 1;
            if (pos == Argc_)
                return CommitEndOfOptions(pos);
            GotMinusMinus_ = true;
            return Commit(0, Argv_[pos], pos + 1, 0);
        } else {
            return CommitEndOfOptions(pos + 1);
        }
    } else if (Opts_->ArgPermutation_ == RETURN_IN_ORDER) {
        return Commit(0, arg, pos + 1, 0);
    } else if (Opts_->ArgPermutation_ == REQUIRE_ORDER) {
        return CommitEndOfOptions(Pos_);
    } else {
        return ParseWithPermutation();
    }
}

bool TOptsParser::Next() {
    bool r = false;

    if (OptsDefault_.empty()) {
        CurrentOpt_ = 0;
        TempCurrentOpt_.Destroy();

        CurrentValue_ = 0;

        if (Stopped_)
            return false;

        TOptsParser copy = *this;

        r = copy.DoNext();

        Swap(copy);

        if (!r) {
            Stopped_ = true;
            // we are done; check for missing options
            Finish();
        }
    }

    if (!r && !OptsDefault_.empty()) {
        CurrentOpt_ = OptsDefault_.front();
        CurrentValue_ = CurrentOpt_->GetDefaultValue();
        OptsDefault_.pop_front();
        r = true;
    }

    if (r) {
        if (CurOpt())
            CurOpt()->FireHandlers(this);
    }

    return r;
}

void TOptsParser::Finish() {
    const TOpts::TOptsVector& optvec = Opts_->Opts_;
    if (optvec.size() == OptsSeen_.size())
        return;

    yvector<Stroka> missingLong;
    yvector<char> missingShort;

    TOpts::TOptsVector::const_iterator it;
    for (it = optvec.begin(); it != optvec.end(); ++it) {
        const TOpt* opt = (*it).Get();
        if (NULL == opt)
            continue;
        if (OptsSeen_.has(opt))
            continue;

        if (opt->IsRequired()) {
            const TOpt::TLongNames& optnames = opt->GetLongNames();
            if (!optnames.empty())
                missingLong.push_back(optnames[0]);
            else {
                const char ch = opt->GetCharOr0();
                if (0 != ch)
                    missingShort.push_back(ch);
            }
            continue;
        }

        if (opt->HasDefaultValue())
            OptsDefault_.push_back(opt);
    }

    // also indicates subsequent options, if any, haven't been seen actually
    OptsSeen_.clear();

    const size_t nmissing = missingLong.size() + missingShort.size();
    if (0 == nmissing)
        return;

    TUsageException usage;
    usage << "The following option";
    if (1 == nmissing)
        usage << " is";
    else
        usage << "s are";
    usage << " required:";
    for (size_t i = 0; i != missingLong.size(); ++i)
        usage << " --" << missingLong[i];
    for (size_t i = 0; i != missingShort.size(); ++i)
        usage << " -" << missingShort[i];
    throw usage; // don't need lineinfo, just the message
}

const TOptParseResult* TOptsParseResult::FindParseResult(const TdVec& vec, const TOpt* opt) {
    for (TdVec::const_iterator it = vec.begin(); it != vec.end(); ++it) {
        const TOptParseResult& r = *it;
        if (r.OptPtr() == opt)
            return &r;
    }
    return 0;
}

const TOptParseResult* TOptsParseResult::FindOptParseResult(const TOpt* opt, bool includeDefault) const {
    const TOptParseResult* r = FindParseResult(Opts_, opt);
    if (NULL == r && includeDefault)
        r = FindParseResult(OptsDef_, opt);
    return r;
}

const TOptParseResult* TOptsParseResult::FindLongOptParseResult(const Stroka& name, bool includeDefault) const {
    return FindOptParseResult(&Parser_->Opts_->GetLongOption(name), includeDefault);
}

const TOptParseResult* TOptsParseResult::FindCharOptParseResult(char c, bool includeDefault) const {
    return FindOptParseResult(&Parser_->Opts_->GetCharOption(c), includeDefault);
}

bool TOptsParseResult::Has(const TOpt* opt, bool includeDefault) const {
    YASSERT(opt);
    return FindOptParseResult(opt, includeDefault) != 0;
}

bool TOptsParseResult::Has(const Stroka& name, bool includeDefault) const {
    return FindLongOptParseResult(name, includeDefault) != 0;
}

bool TOptsParseResult::Has(char c, bool includeDefault) const {
    return FindCharOptParseResult(c, includeDefault) != 0;
}

const char* TOptsParseResult::Get(const TOpt* opt, bool includeDefault) const {
    YASSERT(opt);
    const TOptParseResult* r = FindOptParseResult(opt, includeDefault);
    if (!r || r->Empty()) {
        try {
            ythrow TUsageException() << "option " << opt->ToShortString() << " is unspecified";
        } catch (...) {
            HandleError();
            // unreachable
            throw;
        }
    } else {
        return r->Back();
    }
}

const char* TOptsParseResult::GetOrElse(const TOpt* opt, const char* defaultValue) const {
    YASSERT(opt);
    const TOptParseResult* r = FindOptParseResult(opt);
    if (!r || r->Empty()) {
        return defaultValue;
    } else {
        return r->Back();
    }
}

const char* TOptsParseResult::Get(const Stroka& name, bool includeDefault) const {
    return Get(&Parser_->Opts_->GetLongOption(name), includeDefault);
}

const char* TOptsParseResult::Get(char c, bool includeDefault) const {
    return Get(&Parser_->Opts_->GetCharOption(c), includeDefault);
}

const char* TOptsParseResult::GetOrElse(const Stroka& name, const char* defaultValue) const {
    if (!Has(name))
        return defaultValue;
    return Get(name);
}

const char* TOptsParseResult::GetOrElse(char c, const char* defaultValue) const {
    if (!Has(c))
        return defaultValue;
    return Get(c);
}

TOptParseResult& TOptsParseResult::OptParseResult() {
    const TOpt* opt = Parser_->CurOpt();
    YASSERT(opt);
    TdVec& opts = Parser_->IsExplicit() ? Opts_ : OptsDef_;
    if (Parser_->IsExplicit()) // default options won't appear twice
        for (TdVec::iterator it = opts.begin(); it != opts.end(); ++it)
            if (it->OptPtr() == opt)
                return *it;
    opts.push_back(TOptParseResult(opt));
    return opts.back();
}

Stroka TOptsParseResult::GetProgramName() const {
    return Parser_->ProgramName_;
}

size_t TOptsParseResult::GetFreeArgsPos() const {
    return Parser_->Pos_;
}

yvector<Stroka> TOptsParseResult::GetFreeArgs() const {
    yvector<Stroka> v;
    for (size_t i = GetFreeArgsPos(); i < Parser_->Argc_; ++i) {
        v.push_back(Parser_->Argv_[i]);
    }
    return v;
}

size_t TOptsParseResult::GetFreeArgCount() const {
    return Parser_->Argc_ - GetFreeArgsPos();
}

void TOptsParseResult::Init(const TOpts* options, int argc, const char** argv) {
    try {
        Parser_.Reset(new TOptsParser(options, argc, argv));
        while (Parser_->Next()) {
            TOptParseResult& r = OptParseResult();
            r.AddValue(Parser_->CurValOrOpt().data());
        }
    } catch (...) {
        HandleError();
    }
}

void TOptsParseResult::HandleError() const {
    Cerr << CurrentExceptionMessage() << Endl;
    if (Parser_.Get()) // parser initializing can fail (and we get here, see Init)
        Parser_->Opts_->PrintUsage(Parser_->ProgramName_);
    exit(1);
}

void TOptsParseResultException::HandleError() const {
    throw;
}

//
// TEasySetup
//
TEasySetup::TEasySetup(const TStringBuf& optstring)
    : TOpts(optstring)
{
    AddHelpOption();
}

TOpt& TEasySetup::AdjustParam(const char* longName, const char* help, const char* argName, bool required) {
    YASSERT(longName);
    TOpt& o = AddLongOption(longName);
    if (help) {
        o.Help(help);
    }
    if (argName) {
        o.RequiredArgument(argName);
    } else {
        o.HasArg(NO_ARGUMENT);
    }
    if (required) {
        o.Required();
    }
    return o;
}

TEasySetup& TEasySetup::operator()(char shortName, const char* longName, const char* help, bool required) {
    AdjustParam(longName, help, NULL, required).AddShortName(shortName);
    return *this;
}

TEasySetup& TEasySetup::operator()(char shortName, const char* longName, const char* argName, const char* help, bool required) {
    AdjustParam(longName, help, argName, required).AddShortName(shortName);
    return *this;
}

TEasySetup& TEasySetup::operator()(const char* longName, const char* help, bool required) {
    AdjustParam(longName, help, NULL, required);
    return *this;
}

TEasySetup& TEasySetup::operator()(const char* longName, const char* argName, const char* help, bool required) {
    AdjustParam(longName, help, argName, required);
    return *this;
}

}
