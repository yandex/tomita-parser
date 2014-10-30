/*
 * re_lexer.cpp -- implementation of Lexer class
 *
 * Copyright (c) 2007-2010, Dmitry Prokoptsev <dprokoptsev@gmail.com>,
 *                          Alexander Gololobov <agololobov@gmail.com>
 *
 * This file is part of Pire, the Perl Incompatible
 * Regular Expressions library.
 *
 * Pire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Pire is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 * You should have received a copy of the GNU Lesser Public License
 * along with Pire.  If not, see <http://www.gnu.org/licenses>.
 */


#include <ctype.h>
#include <stdexcept>

#include <contrib/libs/pire/pire/stub/stl.h>
#include <contrib/libs/pire/pire/stub/utf8.h>
#include <contrib/libs/pire/pire/stub/singleton.h>

#include "re_lexer.h"
#include "re_parser.h"
#include "fsm.h"

namespace Pire {

namespace Impl {
    int yre_parse(Pire::Lexer& lexer);
}

Term Term::Character(wchar32 c) { Term::CharacterRange cr; cr.first.insert(Term::String(1, c)); cr.second = false; return Term(TokenTypes::Letters, cr); }
Term Term::Repetition(int lower, int upper) { return Term(TokenTypes::Count, RepetitionCount(lower, upper)); }
Term Term::Dot() { return Term(TokenTypes::Dot, DotTag()); }
Term Term::BeginMark() { return Term(TokenTypes::BeginMark, BeginTag()); }
Term Term::EndMark() { return Term(TokenTypes::EndMark, EndTag()); }

Lexer::~Lexer()
{
    for (yvector<Feature*>::iterator i = m_features.begin(), ie = m_features.end(); i != ie; ++i)
        delete *i;
}

wchar32 Lexer::GetChar()
{
    if (m_input.empty())
        return End;
    else if (m_input.front() == '\\') {
        m_input.pop_front();
        if (m_input.empty())
            Error("Regexp must not end with a backslash");
        wchar32 ch = m_input.front();
        m_input.pop_front();
        return Control | ch;
    } else {
        wchar32 ch = m_input.front();
        m_input.pop_front();
        return ch;
    }
}

wchar32 Lexer::PeekChar()
{
    if (m_input.empty())
        return End;
    else
        return m_input.front();
}

void Lexer::UngetChar(wchar32 c)
{
    if (c != End)
        m_input.push_front(c);
}

namespace {
    class CompareFeaturesByPriority: public ybinary_function<Feature*, Feature*, bool> {
    public:
        bool operator()(Feature* a, Feature* b) const
        {
            return a->Priority() < b->Priority();
        }
    };
}

Lexer& Lexer::AddFeature(Feature* feature)
{
    feature->m_lexer = this;
    m_features.insert(LowerBound(m_features.begin(), m_features.end(), feature, CompareFeaturesByPriority()), feature);
    return *this;
}

Term Lexer::DoLex()
{
    static const char* controls = "|().*+?^$\\";
    for (;;) {
        UngetChar(GetChar());
        wchar32 ch = PeekChar();
        if (ch == End)
            return Term(TokenTypes::End);
        for (yvector<Feature*>::iterator i = m_features.begin(), ie = m_features.end(); i != ie; ++i) {
            if ((*i)->Accepts(ch)) {
                Term ret = (*i)->Lex();
                if (ret.Type())
                    return ret;
            }
        }
        ch = GetChar();

        if (ch == '|')
            return Term(TokenTypes::Or);
        else if (ch == '(') {
            return Term(TokenTypes::Open);
        } else if (ch == ')')
            return Term(TokenTypes::Close);
        else if (ch == '.')
            return Term::Dot();
        else if (ch == '*')
            return Term::Repetition(0, Inf);
        else if (ch == '+')
            return Term::Repetition(1, Inf);
        else if (ch == '?')
            return Term::Repetition(0, 1);
        else if (ch == '^')
            return Term::BeginMark();
        else if (ch == '$')
            return Term::EndMark();
        else if ((ch & ControlMask) == Control && strchr(controls, ch & ~ControlMask))
            return Term::Character(ch & ~ControlMask);
        else
            return Term::Character(ch);
    }
}

Term Lexer::Lex()
{
    Term t = DoLex();

    for (yvector<Feature*>::reverse_iterator i = m_features.rbegin(), ie = m_features.rend(); i != ie; ++i)
        (*i)->Alter(t);

    if (t.Value().IsA<Term::CharacterRange>()) {
        const Term::CharacterRange& chars = t.Value().As<Term::CharacterRange>();
        //std::cerr << "lex: type " << t.type() << "; chars = { " << join(chars.first.begin(), chars.first.end(), ", ") << " }" << std::endl;
        for (Term::Strings::const_iterator i = chars.first.begin(), ie = chars.first.end(); i != ie; ++i)
            for (Term::String::const_iterator j = i->begin(), je = i->end(); j != je; ++j)
                if ((*j & ControlMask) == Control)
                    Error("Control character in tokens sequence");
    }
    
    int type = t.Type();
    if (type == TokenTypes::Letters)
        type = YRE_LETTERS;
    else if (type == TokenTypes::Count)
        type = YRE_COUNT;
    else if (type == TokenTypes::Dot)
        type = YRE_DOT;
    else if (type == TokenTypes::Open)
        type = '(';
    else if (type == TokenTypes::Close)
        type = ')';
    else if (type == TokenTypes::Or)
        type = '|';
    else if (type == TokenTypes::And)
        type = YRE_AND;
    else if (type == TokenTypes::Not)
        type = YRE_NOT;
    else if (type == TokenTypes::BeginMark)
        type = '^';
    else if (type == TokenTypes::EndMark)
        type = '$';
    else if (type == TokenTypes::End)
        type = 0;
    return Term(type, t.Value());
}
    
void Lexer::Parenthesized(Fsm& fsm)
{
    for (yvector<Feature*>::reverse_iterator i = m_features.rbegin(), ie = m_features.rend(); i != ie; ++i)
        (*i)->Parenthesized(fsm);
}

wchar32 Feature::CorrectChar(wchar32 c, const char* controls)
{
    bool ctrl = (strchr(controls, c & 0xFF) != 0);
    if ((c & ControlMask) == Control && ctrl)
        return c & ~ControlMask;
    if (c <= 0xFF && ctrl)
        return c | Control;
    return c;
}

namespace {
    class CharacterRangeReader: public Feature {
    public:
        bool Accepts(wchar32 c) const { return c == '[' || c == (Control | '[') || c == (Control | ']'); }

        Term Lex()
        {
            static const char* controls = "^[]-\\";
            static const char* controls2 = "*+{}()$?.&~";
            wchar32 ch = CorrectChar(GetChar(), controls);
            if (ch == '[' || ch == ']')
                return Term::Character(ch);

            Term::CharacterRange cs;
            ch = CorrectChar(GetChar(), controls);
            if (ch == (Control | '^')) {
                cs.second = true;
                ch = CorrectChar(GetChar(), controls);
            }

            for (; ch != End && ch != (Control | ']'); ch = CorrectChar(GetChar(), controls)) {
                if ((ch & ControlMask) != Control && CorrectChar(PeekChar(), controls) == (Control | '-')) {
                    GetChar();
                    wchar32 end = CorrectChar(GetChar(), controls);
                    if ((end & ControlMask) == Control)
                        Error("Wrong character range");
                    for (; ch <= end; ++ch)
                        cs.first.insert(Term::String(1, ch));
                } else if (ch == (Control | '-'))
                    cs.first.insert(Term::String(1, '-'));
                else if ((ch & ControlMask) == Control && (strchr(controls2, ch & ~ControlMask) || strchr(controls, ch & ~ControlMask)))
                    cs.first.insert(Term::String(1, ch & ~ControlMask));
                else if ((ch & ControlMask) != Control || !strchr(controls, ch & ~ControlMask))
                    cs.first.insert(Term::String(1, ch));
                else
                    Error("Wrong character in range");
            }
            if (ch == End)
                Error("Unexpected end of pattern");

            return Term(TokenTypes::Letters, cs);
        }
    };

    class RepetitionCountReader: public Feature {
    public:
        bool Accepts(wchar32 c) const { return c == '{' || c == (Control | '{') || c == (Control | '}'); }

        Term Lex()
        {
            wchar32 ch = GetChar();
            if (ch == (Control | '{') || ch == (Control | '}'))
                return Term::Character(ch & ~ControlMask);
            ch = GetChar();
            int lower = 0, upper = 0;

            if (!is_digit(ch))
                Error("Wrong repetition count");

            for (; is_digit(ch); ch = GetChar())
                lower = lower * 10 + (ch - '0');
            if (ch == '}')
                return Term::Repetition(lower, lower);
            else if (ch != ',')
                Error("Wrong repetition count");

            ch = GetChar();
            if (ch == '}')
                return Term::Repetition(lower, Inf);
            else if (!is_digit(ch))
                Error("Wrong repetition count");
            for (; is_digit(ch); ch = GetChar())
                upper = upper * 10 + (ch - '0');

            if (ch != '}')
                Error("Wrong repetition count");
            return Term::Repetition(lower, upper);
        }
    };

    class CaseInsensitiveImpl: public Feature {
    public:
        void Alter(Term& t)
        {
            if (t.Value().IsA<Term::CharacterRange>()) {
                typedef Term::CharacterRange::first_type CharSet;
                const CharSet& old = t.Value().As<Term::CharacterRange>().first;
                CharSet altered;
                for (CharSet::const_iterator i = old.begin(), ie = old.end(); i != ie; ++i) {
                    if (i->size() == 1) {
                        altered.insert(Term::String(1, to_upper((*i)[0])));
                        altered.insert(Term::String(1, to_lower((*i)[0])));
                    } else
                        altered.insert(*i);
                }
                t = Term(t.Type(), Term::CharacterRange(altered, t.Value().As<Term::CharacterRange>().second));
            }
        }
    };
    class AndNotSupportImpl: public Feature {
    public:
        bool Accepts(wchar32 c) const
        {
            return c == '&' || c == '~' || c == (Control | '&') || c == (Control | '~');
        }
        
        Term Lex()
        {
            wchar32 ch = GetChar();
            if (ch == (Control | '&') || ch == (Control | '~'))
                return Term::Character(ch & ~ControlMask);
            else if (ch == '&')
                return Term(TokenTypes::And);
            else if (ch == '~')
                return Term(TokenTypes::Not);
            else {
                Error("Pire::AndNotSupport::Lex(): strange input character");
                return Term(0); // Make compiler happy
            }
        }
    };
}

namespace Features {
    Feature* CaseInsensitive() { return new CaseInsensitiveImpl; }
    Feature* CharClasses();
    Feature* AndNotSupport() { return new AndNotSupportImpl; }
};

void Lexer::InstallDefaultFeatures()
{
    AddFeature(new CharacterRangeReader);
    AddFeature(new RepetitionCountReader);
    AddFeature(Features::CharClasses());
}

Fsm Lexer::Parse()
{
    if (!Impl::yre_parse(*this))
        return m_retval.As<Fsm>();
    else {
        Error("Syntax error in regexp");
        return Fsm(); // Make compiler happy
    }
}

}
