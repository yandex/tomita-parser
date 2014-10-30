/*
 * count.cpp -- CountingScanner compiling routine
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


#include <contrib/libs/pire/pire/fsm.h>
#include <contrib/libs/pire/pire/determine.h>
#include <contrib/libs/pire/pire/glue.h>
#include <contrib/libs/pire/pire/stub/lexical_cast.h>

#include "count.h"

namespace Pire {

namespace {
	Pire::Fsm FsmForDot() { Pire::Fsm f; f.AppendDot(); return f; }
	Pire::Fsm FsmForChar(Pire::Char c) { Pire::Fsm f; f.AppendSpecial(c); return f; }
}

CountingScanner::CountingScanner(const Fsm& re, const Fsm& sep)
{
	Fsm res = re;
	res.Surround();
	Fsm sep_re = ((sep & ~res) /* | Fsm()*/) + re;
	sep_re.Determine();

	Fsm dup = sep_re;
	for (size_t i = 0; i < dup.Size(); ++i)
		dup.SetTag(i, Matched);
	size_t oldsize = sep_re.Size();
	sep_re.Import(dup);
	for (Fsm::FinalTable::const_iterator i = sep_re.Finals().begin(), ie = sep_re.Finals().end(); i != ie; ++i)
		if (*i < oldsize)
			sep_re.Connect(*i, oldsize + *i);

	sep_re |= (FsmForDot() | FsmForChar(Pire::BeginMark) | FsmForChar(Pire::EndMark));

	// Make a full Cartesian product of two sep_res
	sep_re.Determine();
	sep_re.Unsparse();
	yset<size_t> dead = sep_re.DeadStates();

	PIRE_IFDEBUG(Cdbg << "=== Original FSM ===" << Endl << sep_re << ">>> " << sep_re.Size() << " states, dead: [" << Join(dead.begin(), dead.end(), ", ") << "]" << Endl);

	Fsm sq;

	typedef ypair<size_t, size_t> NewState;
	yvector<NewState> states;
	ymap<NewState, size_t> invstates;

	states.push_back(NewState(sep_re.Initial(), sep_re.Initial()));
	invstates.insert(ymake_pair(states.back(), states.size() - 1));

	// TODO: this loop reminds me a general determination task...
	for (size_t curstate = 0; curstate < states.size(); ++curstate) {

		unsigned long tag = sep_re.Tag(states[curstate].first);
		if (tag)
			sq.SetTag(curstate, tag);
		sq.SetFinal(curstate, sep_re.IsFinal(states[curstate].first));

		PIRE_IFDEBUG(Cdbg << "State " << curstate << " = (" << states[curstate].first << ", " << states[curstate].second << ")" << Endl);
		for (Fsm::LettersTbl::ConstIterator lit = sep_re.Letters().Begin(), lie = sep_re.Letters().End(); lit != lie; ++lit) {

			Char letter = lit->first;

			const Fsm::StatesSet& mr = sep_re.Destinations(states[curstate].first, letter);
			const Fsm::StatesSet& br = sep_re.Destinations(states[curstate].second, letter);

			if (mr.size() != 1)
				YASSERT(!"Wrong transition size for main");
			if (br.size() != 1)
				YASSERT(!"Wrong transition size for backup");

			NewState ns(*mr.begin(), *br.begin());
			PIRE_IFDEBUG(NewState savedNs = ns);
			unsigned long outputs = 0;

			PIRE_IFDEBUG(ystring dbgout);
			if (dead.find(ns.first) != dead.end()) {
				PIRE_IFDEBUG(dbgout = ((sep_re.Tag(ns.first) & Matched) ? ", ++cur" : ", max <- cur"));
				outputs = DeadFlag | (sep_re.Tag(ns.first) & Matched);
				ns.first = ns.second;
			}
			if (sep_re.IsFinal(ns.first) || (sep_re.IsFinal(ns.second) && !(sep_re.Tag(ns.first) & Matched)))
				ns.second = sep_re.Initial();

			PIRE_IFDEBUG(if (ns != savedNs) Cdbg << "Diverted transition to (" << savedNs.first << ", " << savedNs.second << ") on " << (char) letter << " to (" << ns.first << ", " << ns.second << ")" << dbgout << Endl);

			ymap<NewState, size_t>::iterator nsi = invstates.find(ns);
			if (nsi == invstates.end()) {
				PIRE_IFDEBUG(Cdbg << "New state " << states.size() << " = (" << ns.first << ", " << ns.second << ")" << Endl);
				states.push_back(ns);
				nsi = invstates.insert(ymake_pair(states.back(), states.size() - 1)).first;
				sq.Resize(states.size());
			}

			for (yvector<Char>::const_iterator li = lit->second.second.begin(), le = lit->second.second.end(); li != le; ++li)
			sq.Connect(curstate, nsi->second, *li);
			if (outputs)
				sq.SetOutput(curstate, nsi->second, outputs);
		}
	}

	sq.Determine();

	PIRE_IFDEBUG(Cdbg << "=== FSM ===" << Endl << sq << Endl);
	Init(sq.Size(), sq.Letters(), sq.Initial(), 1);
	BuildScanner(sq, *this);
}


namespace Impl {

class CountingScannerGlueTask: public ScannerGlueCommon<CountingScanner> {
public:
	typedef ymap<State, size_t> InvStates;

	CountingScannerGlueTask(const CountingScanner& lhs, const CountingScanner& rhs)
		: ScannerGlueCommon<CountingScanner>(lhs, rhs, LettersEquality<CountingScanner>(lhs.m_letters, rhs.m_letters))
	{
	}

	void AcceptStates(const yvector<State>& states)
	{
		States = states;
		SetSc(new CountingScanner);
		Sc().Init(states.size(), Letters(), 0, Lhs().RegexpsCount() + Rhs().RegexpsCount());

		for (size_t i = 0; i < states.size(); ++i)
			Sc().SetTag(i, Lhs().m_tags[Lhs().StateIdx(states[i].first)] | (Rhs().m_tags[Rhs().StateIdx(states[i].second)] << 3));
	}

	void Connect(size_t from, size_t to, Char letter)
	{
		Sc().SetJump(from, letter, to,
			Action(Lhs(), States[from].first, letter) | (Action(Rhs(), States[from].second, letter) << Lhs().RegexpsCount()));
	}

private:
	yvector<State> States;
	CountingScanner::Action Action(const CountingScanner& sc, CountingScanner::InternalState state, Char letter) const
	{
		return sc.m_actions[sc.StateIdx(state) * sc.m.lettersCount + sc.m_letters[letter]];
	}
};

}

CountingScanner CountingScanner::Glue(const CountingScanner& lhs, const CountingScanner& rhs, size_t maxSize /* = 0 */)
{
	static const size_t DefMaxSize = 250000;
	Impl::CountingScannerGlueTask task(lhs, rhs);
	return Impl::Determine(task, maxSize ? maxSize : DefMaxSize);
}

}
