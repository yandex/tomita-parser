/*
 * capture.h -- definition of CapturingScanner
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


#ifndef PIRE_EXTRA_CAPTURE_H
#define PIRE_EXTRA_CAPTURE_H


#include <contrib/libs/pire/pire/scanners/loaded.h>
#include <contrib/libs/pire/pire/fsm.h>
#include <contrib/libs/pire/pire/re_lexer.h>

namespace Pire {

/**
* A capturing scanner.
* Requires source FSM to be deterministic, matches input string
* against a single regexp (taking O(strlen(str)) time) and
* captures a substring between a single pair of parentheses.
*
* Requires regexp pattern to satisfy certain conditions
* (I still do not know exactly what they are :) )
*/
class CapturingScanner: public LoadedScanner {
public:
	enum {
		NoAction = 0,
		BeginCapture = 1,
		EndCapture   = 2,
		
		FinalFlag = 1
	};

	class State {
	public:
		bool Captured() const { return (m_begin != npos) && (m_end != npos); }
		size_t Begin() const { return m_begin; }
		size_t End() const { return m_end; }
	private:
		static const size_t npos = static_cast<size_t>(-1);
		size_t m_state;
		size_t m_begin;
		size_t m_end;
		size_t m_counter;
		friend class CapturingScanner;

#ifdef PIRE_DEBUG
		friend yostream& operator << (yostream& s, const State& state)
		{
			s << state.m_state;
			if (state.m_begin != State::npos || state.m_end != npos) {
				s << " [";
				if (state.m_begin != State::npos)
					s << 'b';
				if (state.m_end != State::npos)
					s << 'e';
				s << "]";
			}
			return s;
		}
#endif
	};

	void Initialize(State& state) const
	{
		state.m_state = m.initial;
		state.m_begin = state.m_end = State::npos;
		state.m_counter = 0;
	}

	void TakeAction(State& s, Action a) const
	{
		if ((a & BeginCapture) && !s.Captured())
			s.m_begin = s.m_counter - 1;
		else if ((a & EndCapture) && !s.Captured())
			s.m_end = s.m_counter - 1;
	}

	Action Next(State& s, Char c) const
	{
		return NextTranslated(s, m_letters[c]);
	}

	Action Next(const State& current, State& n, Char c) const
	{
		n = current;
		return Next(n, c);
	}

	bool CanStop(const State& s) const
	{
		return Final(s);
	}

	bool Final(const State& s) const { return m_tags[(reinterpret_cast<Transition*>(s.m_state) - m_jumps) / m.lettersCount] & FinalFlag; }

	bool Dead(const State&) const { return false; }

	CapturingScanner() {}
	CapturingScanner(const CapturingScanner& s): LoadedScanner(s) {}
	explicit CapturingScanner(Fsm& fsm)
	{
		fsm.Canonize();
		Init(fsm.Size(), fsm.Letters(), fsm.Initial());
		BuildScanner(fsm, *this);
	}
	
	void Swap(CapturingScanner& s) { LoadedScanner::Swap(s); }
	CapturingScanner& operator = (const CapturingScanner& s) { CapturingScanner(s).Swap(*this); return *this; }

#ifdef PIRE_DEBUG
	size_t StateIndex(const State& s) const { return StateIdx(s.m_state); }
#endif

private:

	Action NextTranslated(State& s, unsigned char c) const
	{
		Transition x = reinterpret_cast<const Transition*>(s.m_state)[c];
		s.m_state += SignExtend(x.shift);
		++s.m_counter;

		return x.action;
	}
			
	friend void BuildScanner<CapturingScanner>(const Fsm&, CapturingScanner&);
};
namespace Features {
	Feature* Capture(size_t pos);
}

}


#endif
