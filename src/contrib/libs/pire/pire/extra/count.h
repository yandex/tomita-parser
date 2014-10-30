/*
 * count.h -- definition of the counting scanner
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


#ifndef PIRE_EXTRA_COUNT_H
#define PIRE_EXTRA_COUNT_H

#include <contrib/libs/pire/pire/scanners/loaded.h>
#include <contrib/libs/pire/pire/fsm.h>

namespace Pire {
class Fsm;
	
namespace Impl {
    template<class T>
    class ScannerGlueCommon;
	
    class CountingScannerGlueTask;
};

/**
 * A scanner which counts occurences of the
 * given regexp separated by another regexp
 * in input text.
 */
class CountingScanner: public LoadedScanner {
public:
	enum {
		IncrementAction = 1,
		ResetAction = 2,
	
		FinalFlag = 0,
		DeadFlag = 1,
		Matched = 2
	};

	static const size_t OPTIMAL_RE_COUNT = 4;

	class State {
	public:
		size_t Result(int i) const { return ymax(m_current[i], m_total[i]); }
	private:
		InternalState m_state;
		ui32 m_current[OPTIMAL_RE_COUNT];
		ui32 m_total[OPTIMAL_RE_COUNT];
		size_t m_updatedMask;
		friend class CountingScanner;

#ifdef PIRE_DEBUG
		friend yostream& operator << (yostream& s, const State& state)
		{
			s << state.m_state << " ( ";
			for (size_t i = 0; i < OPTIMAL_RE_COUNT; ++i)
				s << state.m_current[i] << '/' << state.m_total[i] << ' ';
			return s << ')';
		}
#endif
	};

	static CountingScanner Glue(const CountingScanner& a, const CountingScanner& b, size_t maxSize = 0);

	void Initialize(State& state) const
	{
		state.m_state = m.initial;
		memset(&state.m_current, 0, sizeof(state.m_current));
		memset(&state.m_total, 0, sizeof(state.m_total));
		state.m_updatedMask = 0;
	}

	FORCED_INLINE PIRE_HOT_FUNCTION
	void TakeAction(State& s, Action a) const
	{
		if (a & IncrementMask)
			PerformIncrement(s, a);
		if (a & ResetMask)
			PerformReset(s, a);
	}

	bool CanStop(const State&) const { return false; }

	Action Next(State& s, Char c) const
	{
		return NextTranslated(s, m_letters[c]);
	}

	Action Next(const State& current, State& n, Char c) const
	{
		n = current;
		return Next(n, c);
	}

	bool Final(const State& /*state*/) const { return false; }

	bool Dead(const State&) const { return false; }

	CountingScanner() {}
	CountingScanner(const CountingScanner& s): LoadedScanner(s) {}
	CountingScanner(const Fsm& re, const Fsm& sep);
	
	void Swap(CountingScanner& s) { LoadedScanner::Swap(s); }
	CountingScanner& operator = (const CountingScanner& s) { CountingScanner(s).Swap(*this); return *this; }

#ifdef PIRE_DEBUG
	size_t StateIndex(const State& s) const { return StateIdx(s.m_state); }
#endif

private:
	using LoadedScanner::Init;

	void PerformIncrement(State& s, Action mask) const
	{
		if (mask) {
			if (mask & 1) ++s.m_current[0];
			if (mask & 2) ++s.m_current[1];
			if (mask & 4) ++s.m_current[2];
			if (mask & 8) ++s.m_current[3];
			s.m_updatedMask |= ((size_t)mask) << MAX_RE_COUNT;
		}

	}

	void Reset(State &s, size_t i) const
	{
		if (s.m_current[i]) {
			s.m_total[i] = ymax(s.m_total[i], s.m_current[i]);
			s.m_current[i] = 0;
		}
	}

	void PerformReset(State& s, Action mask) const
	{
		mask &= s.m_updatedMask;
		if (mask) {
			if (mask & (1 << MAX_RE_COUNT)) Reset(s, 0);
			if (mask & (2 << MAX_RE_COUNT)) Reset(s, 1);
			if (mask & (4 << MAX_RE_COUNT)) Reset(s, 2);
			if (mask & (8 << MAX_RE_COUNT)) Reset(s, 3);
			s.m_updatedMask &= ~mask;
		}
	}

	Action NextTranslated(State& s, Char c) const
	{
		Transition x = reinterpret_cast<const Transition*>(s.m_state)[c];
		s.m_state += SignExtend(x.shift);

		return x.action;
	}

	void Next(InternalState& s, Char c) const
	{
		Transition x = reinterpret_cast<const Transition*>(s)[m_letters[c]];
		s += SignExtend(x.shift);
	}

	Action RemapAction(Action action)
	{
		if (action == (Matched | DeadFlag))
			return 1;
		else if (action == DeadFlag)
			return 1 << MAX_RE_COUNT;
		else
			return 0;
	}
	
	typedef LoadedScanner::InternalState InternalState;
	friend void BuildScanner<CountingScanner>(const Fsm&, CountingScanner&);
	friend class Impl::ScannerGlueCommon<CountingScanner>;
	friend class Impl::CountingScannerGlueTask;
};

}


#endif
