/*
 * fsm.cpp -- the implementation of the FSM class.
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


#include <algorithm>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <iterator>
#include <numeric>
#include <queue>
#include <utility>

#include <iostream>
#include <stdio.h>
#include <contrib/libs/pire/pire/stub/lexical_cast.h>

#include "fsm.h"
#include "vbitset.h"
#include "partition.h"
#include "determine.h"
#include "platform.h"

namespace Pire {
	
ystring CharDump(Char c)
{
	char buf[8];
	if (c >= 32 && c < 127)
		return ystring(1, static_cast<char>(c));
	else if (c == '\n')
		return ystring("\\n");
	else if (c == '\t')
		return ystring("\\t");
	else if (c == '\r')
		return ystring("\\r");
	else if (c < 256) {
		snprintf(buf, sizeof(buf)-1, "\\%03o", static_cast<int>(c));
		return ystring(buf);
	} else if (c == Epsilon)
		return ystring("<Epsilon>");
	else if (c == BeginMark)
		return ystring("<Begin>");
	else if (c == EndMark)
		return ystring("<End>");
	else
		return ystring("<?" "?" "?>");
}

void Fsm::DumpState(yostream& s, size_t state) const
{
	s << "    ";

	// Fill in a 'row': Q -> exp(V) (for current state)
	yvector< ybitset<MaxChar> > row(Size());
	for (TransitionRow::const_iterator rit = m_transitions[state].begin(), rie = m_transitions[state].end(); rit != rie; ++rit)
		for (StatesSet::const_iterator sit = rit->second.begin(), sie = rit->second.end(); sit != sie; ++sit) {
			if (*sit >= Size()) {
				std::cerr << "WTF?! Transition from " << state << " on letter " << rit->first << " leads to non-existing state " << *sit << "\n";
				YASSERT(false);
			}
			if (Letters().Contains(rit->first)) {
				const yvector<Char>& letters = Letters().Klass(Letters().Representative(rit->first));
				for (yvector<Char>::const_iterator lit = letters.begin(), lie = letters.end(); lit != lie; ++lit)
					row[*sit].set(*lit);
			} else
				row[*sit].set(rit->first);
		}

	// Display each destination state
	for (yvector< ybitset<MaxChar> >::iterator rit = row.begin(), rie = row.end(); rit != rie; ++rit) {
		unsigned begin = 0, end = 0;

		bool empty = true;
		if (rit->test(Epsilon)) {
			s << "<Epsilon> ";
			empty = false;
		}
		if (rit->test(BeginMark)) {
			s << "<Begin> ";
			empty = false;
		}
		if (rit->test(EndMark)) {
			s << "<End> ";
			empty = false;
		}
		while (begin < 256) {
			for (begin = end; begin < 256 && !rit->test(begin); ++begin)
				;
			for (end = begin; end < 256 && rit->test(end); ++end)
				;
			if (begin + 1 == end) {
				s << CharDump(begin);
				empty = false;
			} else if (begin != end) {
				s << CharDump(begin) << ".." << (CharDump(end-1));
				empty = false;
			}
			if (!empty)
				s << " ";
		}
		if (!empty) {
			s << " => " << std::distance(row.begin(), rit);

			// Display outputs
			Outputs::const_iterator oit = outputs.find(state);
			if (oit != outputs.end()) {
				Outputs::value_type::second_type::const_iterator oit2 = oit->second.find(std::distance(row.begin(), rit));
				if (oit2 == oit->second.end())
					;
				else {
					yvector<int> payload;
					for (unsigned i = 0; i < sizeof(oit2->second) * 8; ++i)
						if (oit2->second & (1ul << i))
							payload.push_back(i);
					if (!payload.empty())
						s << "[" << Join(payload.begin(), payload.end(), ", ") << "]";
				}
			}
			s << "\n    ";
		}
	}
	if (Initial() == state)
		s << "[initial] ";
	if (IsFinal(state))
		s << "[final] ";
	Tags::const_iterator ti = tags.find(state);
	if (ti != tags.end())
		s << "[tags: " << ti->second << "] ";
	s << "\n";
}

void Fsm::DumpTo(yostream& s) const
{
	for (size_t state = 0; state < Size(); ++state) {
		s << "*** State " << state << "\n";
		DumpState(s, state);
	}
}

yostream& operator << (yostream& s, const Fsm& fsm) { fsm.DumpTo(s); return s; }


namespace {
	template<class Vector> void resizeVector(Vector& v, size_t s) { v.resize(s); }
}

Fsm::Fsm():
	m_transitions(1),
	initial(0),
	letters(m_transitions),
	m_sparsed(false),
	determined(false),
	isAlternative(false)
{
	m_final.insert(0);
}

Fsm Fsm::MakeFalse()
{
	Fsm f;
	f.SetFinal(0, false);
	return f;
}

Char Fsm::Translate(Char c) const
{
	if (!m_sparsed || c == Epsilon)
		return c;
	else
		return Letters().Representative(c);
}

bool Fsm::Connected(size_t from, size_t to, Char c) const
{
	TransitionRow::const_iterator it = m_transitions[from].find(Translate(c));
	return (it != m_transitions[from].end() && it->second.find(to) != it->second.end());
}

bool Fsm::Connected(size_t from, size_t to) const
{
	for (TransitionRow::const_iterator i = m_transitions[from].begin(), ie = m_transitions[from].end(); i != ie; ++i)
		if (i->second.find(to) != i->second.end())
			return true;
	return false;
}

const Fsm::StatesSet& Fsm::Destinations(size_t from, Char c) const
{
	TransitionRow::const_iterator i = m_transitions[from].find(Translate(c));
	return (i != m_transitions[from].end()) ? i->second : DefaultValue<StatesSet>();
}

yset<Char> Fsm::OutgoingLetters(size_t state) const
{
	yset<Char> ret;
	for (TransitionRow::const_iterator i = m_transitions[state].begin(), ie = m_transitions[state].end(); i != ie; ++i)
		ret.insert(i->first);
	return ret;
}

size_t Fsm::Resize(size_t newSize)
{
	size_t ret = Size();
	m_transitions.resize(newSize);
	return ret;
}

void Fsm::Swap(Fsm& fsm)
{
	DoSwap(m_transitions, fsm.m_transitions);
	DoSwap(initial, fsm.initial);
	DoSwap(m_final, fsm.m_final);
	DoSwap(letters, fsm.letters);
	DoSwap(determined, fsm.determined);
	DoSwap(outputs, fsm.outputs);
	DoSwap(tags, fsm.tags);
	DoSwap(isAlternative, fsm.isAlternative);
}

void Fsm::SetFinal(size_t state, bool final)
{
	if (final)
		m_final.insert(state);
	else
		m_final.erase(state);
}

Fsm& Fsm::AppendDot()
{
	Resize(Size() + 1);
	for (size_t letter = 0; letter != (1 << (sizeof(char)*8)); ++letter)
		ConnectFinal(Size() - 1, letter);
	ClearFinal();
	SetFinal(Size() - 1, true);
	determined = false;
    return *this;
}

Fsm& Fsm::Append(char c)
{
	Resize(Size() + 1);
	ConnectFinal(Size() - 1, static_cast<unsigned char>(c));
	ClearFinal();
	SetFinal(Size() - 1, true);
	determined = false;
    return *this;
}
    
Fsm& Fsm::Append(const ystring& str)
{
    for (ystring::const_iterator i = str.begin(), ie = str.end(); i != ie; ++i)
        Append(*i);
    return *this;
}

Fsm& Fsm::AppendSpecial(Char c)
{
	Resize(Size() + 1);
	ConnectFinal(Size() - 1, c);
	ClearFinal();
	SetFinal(Size() - 1, true);
	determined = false;
    return *this;
}

Fsm& Fsm::AppendStrings(const yvector<ystring>& strings)
{
	for (yvector<ystring>::const_iterator i = strings.begin(), ie = strings.end(); i != ie; ++i)
		if (i->empty())
			throw Error("None of strings passed to appendStrings() can be empty");

	Resize(Size() + 1);
	size_t end = Size() - 1;

	// A local transitions table: (oldstate, char) -> newstate.
	// Valid for all letters in given strings except final ones,
	// which are always connected to the end state.

	// NB: since each FSM contains at least one state,
	// state #0 cannot appear in LTRs. Thus we can use this
	// criteria to test whether a transition has been created or not.
	typedef ypair<size_t, char> Transition;
	ymap<char, size_t> startLtr;
	ymap<Transition, size_t> ltr;

	// A presense of a transition in this set indicates that
	// a that transition already points somewhere (either to end
	// or somewhere else). Another attempt to create such transition
	// will clear `determined flag.
	yset<Transition> usedTransitions;
	yset<char> usedFirsts;

	for (yvector<ystring>::const_iterator sit = strings.begin(), sie = strings.end(); sit != sie; ++sit) {
		const ystring& str = *sit;

		if (str.size() > 1) {

			// First letter: all previously final states are connected to the new state
			size_t& firstJump = startLtr[str[0]];
			if (!firstJump) {
				firstJump = Resize(Size() + 1);
				ConnectFinal(firstJump, static_cast<unsigned char>(str[0]));
				determined = determined && (usedFirsts.find(str[0]) != usedFirsts.end());
			}

			// All other letters except last one
			size_t state = firstJump;
			for (ystring::const_iterator cit = str.begin() + 1, cie = str.end() - 1; cit != cie; ++cit) {
				size_t& newState = ltr[ymake_pair(state, *cit)];
				if (!newState) {
					newState = Resize(Size() + 1);
					Connect(state, newState, static_cast<unsigned char>(*cit));
					determined = determined && (usedTransitions.find(ymake_pair(state, *cit)) != usedTransitions.end());
				}
				state = newState;
			}

			// The last letter: connect the current state to end
			unsigned char last = static_cast<unsigned char>(*(str.end() - 1));
			Connect(state, end, last);
			determined = determined && (usedTransitions.find(ymake_pair(state, last)) != usedTransitions.end());

		} else {
			// The single letter: connect all the previously final states to end
			ConnectFinal(end, static_cast<unsigned char>(str[0]));
			determined = determined && (usedFirsts.find(str[0]) != usedFirsts.end());
		}
	}

	ClearFinal();
	SetFinal(end, true);
    return *this;
}

void Fsm::Import(const Fsm& rhs)
{
//     PIRE_IFDEBUG(LOG_DEBUG("fsm") << "Importing");
//     PIRE_IFDEBUG(LOG_DEBUG("fsm") << "=== Left-hand side ===\n" << *this);
//     PIRE_IFDEBUG(LOG_DEBUG("fsm") << "=== Right-hand side ===\n" << rhs);

	size_t oldsize = Resize(Size() + rhs.Size());

	for (TransitionTable::iterator outer = m_transitions.begin(), outerEnd = m_transitions.end(); outer != outerEnd; ++outer) {
		for (LettersTbl::ConstIterator lit = letters.Begin(), lie = letters.End(); lit != lie; ++lit) {
			TransitionRow::const_iterator targets = outer->find(lit->first);
			if (targets == outer->end())
				continue;
			for (yvector<Char>::const_iterator cit = lit->second.second.begin(), cie = lit->second.second.end(); cit != cie; ++cit)
				if (*cit != lit->first)
					outer->insert(ymake_pair(*cit, targets->second));
		}
	}

	TransitionTable::iterator dest = m_transitions.begin() + oldsize;
	for (TransitionTable::const_iterator outer = rhs.m_transitions.begin(), outerEnd = rhs.m_transitions.end(); outer != outerEnd; ++outer, ++dest) {
		for (TransitionRow::const_iterator inner = outer->begin(), innerEnd = outer->end(); inner != innerEnd; ++inner) {
			yset<size_t> targets;
			std::transform(inner->second.begin(), inner->second.end(), std::inserter(targets, targets.begin()),
				std::bind2nd(std::plus<size_t>(), oldsize));
			dest->insert(ymake_pair(inner->first, targets));
		}

		for (LettersTbl::ConstIterator lit = rhs.letters.Begin(), lie = rhs.letters.End(); lit != lie; ++lit) {
			TransitionRow::const_iterator targets = dest->find(lit->first);
			if (targets == dest->end())
				continue;
			for (yvector<Char>::const_iterator cit = lit->second.second.begin(), cie = lit->second.second.end(); cit != cie; ++cit)
				if (*cit != lit->first)
					dest->insert(ymake_pair(*cit, targets->second));
		}
	}

	// Import outputs
	for (Outputs::const_iterator oit = rhs.outputs.begin(), oie = rhs.outputs.end(); oit != oie; ++oit) {
		Outputs::value_type::second_type& dest = outputs[oit->first + oldsize];
		for (Outputs::value_type::second_type::const_iterator rit = oit->second.begin(), rie = oit->second.end(); rit != rie; ++rit)
			dest.insert(ymake_pair(rit->first + oldsize, rit->second));
	}

	// Import tags
	for (Tags::const_iterator it = rhs.tags.begin(), ie = rhs.tags.end(); it != ie; ++it)
		tags.insert(ymake_pair(it->first + oldsize, it->second));

	letters = LettersTbl(LettersEquality(m_transitions));
}

void Fsm::Connect(size_t from, size_t to, Char c /* = Epsilon */)
{
	m_transitions[from][c].insert(to);
	ClearHints();
}

void Fsm::ConnectFinal(size_t to, Char c /* = Epsilon */)
{
	for (FinalTable::iterator it = m_final.begin(), ie = m_final.end(); it != ie; ++it)
		Connect(*it, to, c);
	ClearHints();
}

void Fsm::Disconnect(size_t from, size_t to, Char c)
{
	TransitionRow::iterator i = m_transitions[from].find(c);
	if (i != m_transitions[from].end())
		i->second.erase(to);
	ClearHints();
}

void Fsm::Disconnect(size_t from, size_t to)
{
	for (TransitionRow::iterator i = m_transitions[from].begin(), ie = m_transitions[from].end(); i != ie; ++i)
		i->second.erase(to);
	ClearHints();
}

unsigned long Fsm::Output(size_t from, size_t to) const
{
	Outputs::const_iterator i = outputs.find(from);
	if (i == outputs.end())
		return 0;
	Outputs::mapped_type::const_iterator j = i->second.find(to);
	if (j == i->second.end())
		return 0;
	else
		return j->second;
}

Fsm& Fsm::operator += (const Fsm& rhs)
{
	size_t lhsSize = Size();
	Import(rhs);

	const TransitionRow& row = m_transitions[lhsSize + rhs.initial];

	for (TransitionRow::const_iterator outer = row.begin(), outerEnd = row.end(); outer != outerEnd; ++outer)
		for (StatesSet::const_iterator inner = outer->second.begin(), innerEnd = outer->second.end(); inner != innerEnd; ++inner)
			ConnectFinal(*inner, outer->first);

	Outputs::const_iterator out = rhs.outputs.find(rhs.initial);
	if (out != rhs.outputs.end())
		for (Outputs::value_type::second_type::const_iterator it = out->second.begin(), ie = out->second.end(); it != ie; ++it) {
			for (FinalTable::iterator fit = m_final.begin(), fie = m_final.end(); fit != fie; ++fit)
				outputs[*fit].insert(ymake_pair(it->first + lhsSize, it->second));
		}

	ClearFinal();
	for (FinalTable::const_iterator it = rhs.m_final.begin(), ie = rhs.m_final.end(); it != ie; ++it)
		SetFinal(*it + lhsSize, true);
	determined = false;

	ClearHints();
	PIRE_IFDEBUG(Cdbg << "=== After addition ===" << Endl << *this << Endl);

	return *this;
}

Fsm& Fsm::operator |= (const Fsm& rhs)
{
	size_t lhsSize = Size();

	Import(rhs);
	for (FinalTable::const_iterator it = rhs.m_final.begin(), ie = rhs.m_final.end(); it != ie; ++it)
		m_final.insert(*it + lhsSize);	
	
	if (!isAlternative && !rhs.isAlternative) {
		Resize(Size() + 1);
		Connect(Size() - 1, initial);
		Connect(Size() - 1, lhsSize + rhs.initial);
		initial = Size() - 1;
	} else if (isAlternative && !rhs.isAlternative) {
		Connect(initial, lhsSize + rhs.initial, Epsilon);
	} else if (!isAlternative && rhs.isAlternative) {
		Connect(lhsSize + rhs.initial, initial, Epsilon);
		initial = rhs.initial + lhsSize;
	} else if (isAlternative && rhs.isAlternative) {
		const StatesSet& tos = rhs.Destinations(rhs.initial, Epsilon);
		for (StatesSet::const_iterator to = tos.begin(), toe = tos.end(); to != toe; ++to) {
			Connect(initial, *to + lhsSize, Epsilon);
			Disconnect(rhs.initial + lhsSize, *to + lhsSize, Epsilon);
		}
	}

	determined = false;
	isAlternative = true;
	return *this;
}

Fsm& Fsm::operator &= (const Fsm& rhs)
{
	Fsm rhs2(rhs);
	Complement();
	rhs2.Complement();
	*this |= rhs2;
	Complement();
	return *this;
}

Fsm& Fsm::Iterate()
{
	PIRE_IFDEBUG(Cdbg << "Iterating:" << Endl << *this << Endl);
	Resize(Size() + 2);

	Connect(Size() - 2, Size() - 1);
	Connect(Size() - 2, initial);
	ConnectFinal(initial);
	ConnectFinal(Size() - 1);

	ClearFinal();
	SetFinal(Size() - 1, true);
	initial = Size() - 2;

	determined = false;

	PIRE_IFDEBUG(Cdbg << "Iterated:" << Endl << *this << Endl);
	return *this;
}

Fsm& Fsm::Complement()
{
	if (!Determine())
		throw Error("Regexp pattern too complicated");
	Minimize();
	Resize(Size() + 1);
	for (size_t i = 0; i < Size(); ++i)
		if (!IsFinal(i))
			Connect(i, Size() - 1);
	ClearFinal();
	SetFinal(Size() - 1, true);
	determined = false;

	return *this;
}

Fsm Fsm::operator *(size_t count)
{
	Fsm ret;
	while (count--)
		ret += *this;
	return ret;
}

void Fsm::MakePrefix()
{
	RemoveDeadEnds();
	for (size_t i = 0; i < Size(); ++i)
		if (!m_transitions[i].empty())
			m_final.insert(i);
	ClearHints();
}

void Fsm::MakeSuffix()
{
	for (size_t i = 0; i < Size(); ++i)
		if (i != initial)
			Connect(initial, i);
	ClearHints();
}
	
Fsm& Fsm::Reverse()
{	
	Fsm out;
	out.Resize(Size() + 1);
	out.letters = Letters();
	
	// Invert transitions
	for (size_t from = 0; from < Size(); ++from)
		for (TransitionRow::iterator i = m_transitions[from].begin(), ie = m_transitions[from].end(); i != ie; ++i)
			for (StatesSet::iterator j = i->second.begin(), je = i->second.end(); j != je; ++j)
				out.Connect(*j, from, i->first);
		
	// Invert initial and final states
	out.SetFinal(initial, true);
	for (FinalTable::iterator i = m_final.begin(), ie = m_final.end(); i != ie; ++i)
		out.Connect(Size(), *i, Epsilon);
	out.SetInitial(Size());
	
	// Invert outputs
	for (Outputs::iterator i = outputs.begin(), ie = outputs.end(); i != ie; ++i)
		for (Outputs::mapped_type::iterator j = i->second.begin(), je = i->second.end(); j != je; ++j)
			out.SetOutput(j->first, i->first, j->second);
	
	// Preserve tags (although thier semantics are usually heavily broken at this point)
	out.tags = tags;
	
	// Apply
	Swap(out);
	return *this;
}

yset<size_t> Fsm::DeadStates() const
{
	// Build an FSM with inverted transitions
	Fsm inverted;
	inverted.Resize(Size());
	for (TransitionTable::const_iterator j = m_transitions.begin(), je = m_transitions.end(); j != je; ++j) {
		for (TransitionRow::const_iterator k = j->begin(), ke = j->end(); k != ke; ++k) {
			for (StatesSet::const_iterator toSt = k->second.begin(), toSte = k->second.end(); toSt != toSte; ++toSt) {
				// We only care if the states are connected or not regerdless through what letter
				inverted.Connect(*toSt, j - m_transitions.begin(), 0);
			}
		}
	}

	yvector<bool> unchecked(Size(), true);
	yvector<bool> useless(Size(), true);
	ydeque<size_t> queue;

	// Put all final states into queue, marking them useful
	for (size_t i = 0; i < Size(); ++i)
		if (IsFinal(i)) {
			useless[i] = false;
			queue.push_back(i);
		}

	// Do the breadth-first search, marking all states
	// from which already marked states are reachable
	while (!queue.empty()) {
		size_t to = queue.front();
		queue.pop_front();

		// All the states that are connected to this state in inverted transition matrix are useful
		const StatesSet& connections = (inverted.m_transitions[to])[0];
		for (StatesSet::const_iterator fr = connections.begin(), fre = connections.end(); fr != fre; ++fr) {
			// Enqueue the state for further traversal if it hasnt been already checked
			if (unchecked[*fr] && useless[*fr]) {
				useless[*fr] = false;
				queue.push_back(*fr);
			}
		}

		// Now we consider this state checked
		unchecked[to] = false;
	}

	yset<size_t> res;

	for (size_t i = 0; i < Size(); ++i) {
		if (useless[i]) {
			res.insert(i);
		}
	}

	return res;
}

void Fsm::RemoveDeadEnds()
{
	PIRE_IFDEBUG(Cdbg << "Removing dead ends on:" << Endl << *this << Endl);

	yset<size_t> dead = DeadStates();
	// Erase all useless states
	for (yset<size_t>::iterator i = dead.begin(), ie = dead.end(); i != ie; ++i) {
		PIRE_IFDEBUG(Cdbg << "Removing useless state " << *i << Endl);
		m_transitions[*i].clear();
		for (TransitionTable::iterator j = m_transitions.begin(), je = m_transitions.end(); j != je; ++j)
			for (TransitionRow::iterator k = j->begin(), ke = j->end(); k != ke; ++k)
				k->second.erase(*i);
	}
	ClearHints();

	PIRE_IFDEBUG(Cdbg << "Result:" << Endl << *this << Endl);
}

// This method is one step of Epsilon-connection removal algorithm.
// It merges transitions, tags, and outputs of 'to' state into 'from' state
void Fsm::MergeEpsilonConnection(size_t from, size_t to)
{
	unsigned long frEpsOutput = 0;
	bool fsEpsOutputExists = false;

	// Is there an output for 'from'->'to' transition?
	if (outputs.find(from) != outputs.end() && outputs[from].find(to) != outputs[from].end()) {
		frEpsOutput = outputs[from][to];
		fsEpsOutputExists = true;
	}

	// Merge transitions from 'to' state into transitions from 'from' state
	for (TransitionRow::const_iterator it = m_transitions[to].begin(), ie = m_transitions[to].end(); it != ie; ++it) {
		yset<size_t> connStates;
		std::copy(it->second.begin(), it->second.end(),
			std::inserter(m_transitions[from][it->first], m_transitions[from][it->first].end()));

		// If there is an output of the 'from'->'to' connection it has to be set to all
		// new connections that were merged from 'to' state
		if (fsEpsOutputExists) {
			// Compute the set of states that are reachable from 'to' state
			std::copy(it->second.begin(), it->second.end(), std::inserter(connStates, connStates.end()));

			// For each of these states add an output equal to the Epsilon-connection output
			for (yset<size_t>::const_iterator newConnSt = connStates.begin(), ncse = connStates.end(); newConnSt != ncse; ++newConnSt) {
				outputs[from][*newConnSt] |= frEpsOutput;
			}
		}
	}

	// Mark 'from' state final if 'to' state is final
	if (IsFinal(to))
		SetFinal(from, true);

	// Combine tags
	Tags::iterator ti = tags.find(to);
	if (ti != tags.end())
		tags[from] |= ti->second;

	// Merge all 'to' into 'from' outputs:
	//      outputs[from][i] |= (outputs[from][to] | outputs[to][i])
	Outputs::iterator toOit = outputs.find(to);
	if (toOit != outputs.end()) {
		for (Outputs::value_type::second_type::const_iterator oit = toOit->second.begin(), oite = toOit->second.end(); oit != oite; ++oit) {
			outputs[from][oit->first] |= (frEpsOutput | oit->second);
		}
	}
}

// Assuming the epsilon transitions is possible from 'from' to 'thru',
// finds all states which are Epsilon-reachable from 'thru' and connects
// them directly to 'from' with Epsilon transition having proper output.
// Updates inverse map of epsilon transitions as well.
void Fsm::ShortCutEpsilon(size_t from, size_t thru, yvector< yset<size_t> >& inveps)
{
	PIRE_IFDEBUG(Cdbg << "In Fsm::ShortCutEpsilon(" << from << ", " << thru << ")\n");
	const StatesSet& to = Destinations(thru, Epsilon);
	Outputs::iterator outIt = outputs.find(from);
	unsigned long fromThruOut = Output(from, thru);
	for (StatesSet::const_iterator toi = to.begin(), toe = to.end(); toi != toe; ++toi) {
		PIRE_IFDEBUG(Cdbg << "Epsilon connecting " << from << " --> " << thru << " --> " << *toi << "\n");
		Connect(from, *toi, Epsilon);
		inveps[*toi].insert(from);
		if (outIt != outputs.end())
			outIt->second[*toi] |= (fromThruOut | Output(thru, *toi));
	}	
}

// Removes all Epsilon-connections by iterating though states and merging each Epsilon-connection
// effects from 'to' state into 'from' state
void Fsm::RemoveEpsilons()
{
	Unsparse();
	
	// Build inverse map of epsilon transitions
	yvector< yset<size_t> > inveps(Size()); // We have to use yset<> here since we want it sorted
	for (size_t from = 0; from != Size(); ++from) {
		const StatesSet& tos = Destinations(from, Epsilon);
		for (StatesSet::const_iterator to = tos.begin(), toe = tos.end(); to != toe; ++to)
			inveps[*to].insert(from);
	}
	
	// Make a transitive closure of all epsilon transitions (Floyd-Warshall algorithm)
	// (if there exists an epsilon-path between two states, epsilon-connect them directly)
	for (size_t thru = 0; thru != Size(); ++thru)
		for (StatesSet::iterator from = inveps[thru].begin(); from != inveps[thru].end(); ++from)
			// inveps[thru] may alter during loop body, hence we cannot cache ivneps[thru].end()
			if (*from != thru)
				ShortCutEpsilon(*from, thru, inveps);
	
	PIRE_IFDEBUG(Cdbg << "=== After epsilons shortcut\n" << *this << Endl);
	
	// Iterate through all epsilon-connected state pairs, merging states together
	for (size_t from = 0; from != Size(); ++from) {
		const StatesSet& to = Destinations(from, Epsilon);
		for (StatesSet::const_iterator toi = to.begin(), toe = to.end(); toi != toe; ++toi)
			if (*toi != from)
				MergeEpsilonConnection(from, *toi); // it's a NOP if to == from, so don't waste time
	}
	
	PIRE_IFDEBUG(Cdbg << "=== After epsilons merged\n" << *this << Endl);
	
	// Drop all epsilon transitions
	for (TransitionTable::iterator i = m_transitions.begin(), ie = m_transitions.end(); i != ie; ++i)
		i->erase(Epsilon);
	
	Sparse();
	ClearHints();
}

bool Fsm::LettersEquality::operator()(Char a, Char b) const
{
	for (Fsm::TransitionTable::const_iterator outer = m_tbl->begin(), outerEnd = m_tbl->end(); outer != outerEnd; ++outer) {
		Fsm::TransitionRow::const_iterator ia = outer->find(a);
		Fsm::TransitionRow::const_iterator ib = outer->find(b);
		if (ia == outer->end() && ib == outer->end())
			continue;
		else if (ia == outer->end() || ib == outer->end() || ia->second != ib->second) {
			return false;
		}
	}
	return true;
}

void Fsm::Sparse()
{
	letters = LettersTbl(LettersEquality(m_transitions));
	for (unsigned letter = 0; letter < MaxChar; ++letter)
		if (letter != Epsilon)
			letters.Append(letter);

	m_sparsed = true;
	PIRE_IFDEBUG(Cdbg << "Letter classes = " << letters << Endl);
}

void Fsm::Unsparse()
{
	for (LettersTbl::ConstIterator lit = letters.Begin(), lie = letters.End(); lit != lie; ++lit)
		for (TransitionTable::iterator i = m_transitions.begin(), ie = m_transitions.end(); i != ie; ++i)
			for (yvector<Char>::const_iterator j = lit->second.second.begin(), je = lit->second.second.end(); j != je; ++j)
				(*i)[*j] = (*i)[lit->first];
	m_sparsed = false;
}

// Returns a set of 'terminal states', which are those of the final states,
// from which a transition to themselves on any letter is possible.
yset<size_t> Fsm::TerminalStates() const
{
	yset<size_t> terminals;
	for (FinalTable::const_iterator fit = m_final.begin(), fie = m_final.end(); fit != fie; ++fit) {
		bool ok = true;
		for (LettersTbl::ConstIterator lit = letters.Begin(), lie = letters.End(); lit != lie; ++lit) {
			TransitionRow::const_iterator dests = m_transitions[*fit].find(lit->first);
			ok = ok && (dests != m_transitions[*fit].end() && dests->second.find(*fit) != dests->second.end());
		}
		if (ok)
			terminals.insert(*fit);
	}
	return terminals;
}

namespace Impl {
class FsmDetermineTask {
public:
	typedef yvector<size_t> State;
	typedef Fsm::LettersTbl LettersTbl;
	typedef ymap<State, size_t> InvStates;
	
	FsmDetermineTask(const Fsm& fsm)
		: mFsm(fsm)
		, mTerminals(fsm.TerminalStates())
	{
		PIRE_IFDEBUG(Cdbg << "Terminal states: [" << Join(mTerminals.begin(), mTerminals.end(), ", ") << "]" << Endl);
	}
	const LettersTbl& Letters() const { return mFsm.letters; }
	
	State Initial() const { return State(1, mFsm.initial); }
	bool IsRequired(const State& state) const
	{
		for (State::const_iterator i = state.begin(), ie = state.end(); i != ie; ++i)
			if (mTerminals.find(*i) != mTerminals.end())
				return false;
		return true;
	}
	
	State Next(const State& state, Char letter) const
	{
		State next;
		next.reserve(20);
		for (State::const_iterator from = state.begin(), fromEnd = state.end(); from != fromEnd; ++from) {
			const Fsm::StatesSet& part = mFsm.Destinations(*from, letter);
			std::copy(part.begin(), part.end(), std::back_inserter(next));
		}

		std::sort(next.begin(), next.end());
		next.erase(std::unique(next.begin(), next.end()), next.end());
		PIRE_IFDEBUG(Cdbg << "Returning transition [" << Join(state.begin(), state.end(), ", ") << "] --" << letter
		                  << "--> [" << Join(next.begin(), next.end(), ", ") << "]" << Endl);
		return next;
	}
	
	void AcceptStates(const yvector<State>& states)
	{
		mNewFsm.Resize(states.size());
		mNewFsm.initial = 0;
		mNewFsm.determined = true;
		mNewFsm.letters = Letters();
		mNewFsm.m_final.clear();
		for (size_t ns = 0; ns < states.size(); ++ns) {
			PIRE_IFDEBUG(Cdbg << "State " << ns << " = [" << Join(states[ns].begin(), states[ns].end(), ", ") << "]" << Endl);
			for (State::const_iterator j = states[ns].begin(), je = states[ns].end(); j != je; ++j) {
				
				// If it was a terminal state, connect it to itself
				if (mTerminals.find(*j) != mTerminals.end()) {
					for (LettersTbl::ConstIterator letterIt = Letters().Begin(), letterEnd = Letters().End(); letterIt != letterEnd; ++letterIt)
						mNewFsm.Connect(ns, ns, letterIt->first);
					mNewTerminals.insert(ns);
					PIRE_IFDEBUG(Cdbg << "State " << ns << " becomes terminal because of old state " << *j << Endl);
				}
			}
			for (State::const_iterator j = states[ns].begin(), je = states[ns].end(); j != je; ++j) {
				// If any state containing in our one is marked final, mark the new state final as well
				if (mFsm.IsFinal(*j)) {
					PIRE_IFDEBUG(Cdbg << "State " << ns << " becomes final because of old state " << *j << Endl);
					mNewFsm.SetFinal(ns, true);
					if (mFsm.tags.empty())
						// Weve got no tags and already know that the state is final,
						// hence weve done with this state and got nothing more to do.
						break;
				}
				
				// Bitwise OR all tags in states
				Fsm::Tags::const_iterator ti = mFsm.tags.find(*j);
				if (ti != mFsm.tags.end()) {
					PIRE_IFDEBUG(Cdbg << "State " << ns << " carries tag " << ti->second << " because of old state " << *j << Endl);
					mNewFsm.tags[ns] |= ti->second;
				}
			}
		}
		// For each old state, prepare a list of new state it is contained in
		typedef ymap< size_t, yvector<size_t> > Old2New;
		Old2New old2new;
		for (size_t ns = 0; ns < states.size(); ++ns)
			for (State::const_iterator j = states[ns].begin(), je = states[ns].end(); j != je; ++j)
				old2new[*j].push_back(ns);
		
		// Copy all outputs
		for (Fsm::Outputs::const_iterator i = mFsm.outputs.begin(), ie = mFsm.outputs.end(); i != ie; ++i) {
			for (Fsm::Outputs::mapped_type::const_iterator j = i->second.begin(), je = i->second.end(); j != je; ++j) {
				Old2New::iterator from = old2new.find(i->first);
				Old2New::iterator to   = old2new.find(j->first);
				if (from != old2new.end() && to != old2new.end()) {
					for (yvector<size_t>::iterator k = from->second.begin(), ke = from->second.end(); k != ke; ++k)
						for (yvector<size_t>::iterator l = to->second.begin(), le = to->second.end(); l != le; ++l)
							mNewFsm.outputs[*k][*l] |= j->second;
				}
			}
		}
		PIRE_IFDEBUG(Cdbg << "New terminals = [" << Join(mNewTerminals.begin(), mNewTerminals.end(), ",") << "]" << Endl);
	}
	
	void Connect(size_t from, size_t to, Char letter)
	{
		PIRE_IFDEBUG(Cdbg << "Connecting " << from << " --" << letter << "--> " << to << Endl);
		YASSERT(mNewTerminals.find(from) == mNewTerminals.end());
		mNewFsm.Connect(from, to, letter);
	}
	typedef bool Result;
	Result Success() { return true; }
	Result Failure() { return false; }
	
	Fsm& Output() { return mNewFsm; }
private:
	const Fsm& mFsm;
	Fsm mNewFsm;
	yset<size_t> mTerminals;
	yset<size_t> mNewTerminals;
};
}

bool Fsm::Determine(size_t maxsize /* = 0 */)
{
	static const unsigned MaxSize = 200000;
	if (determined)
		return true;

	PIRE_IFDEBUG(Cdbg << "=== Initial ===" << Endl << *this << Endl);
	
	RemoveEpsilons();
	PIRE_IFDEBUG(Cdbg << "=== After all epsilons removed" << Endl << *this << Endl);
	
	Impl::FsmDetermineTask task(*this);
	if (Pire::Impl::Determine(task, maxsize ? maxsize : MaxSize)) {
		task.Output().Swap(*this);
		PIRE_IFDEBUG(Cdbg << "=== Determined ===" << Endl << *this << Endl);
		return true;
	} else
		return false;
}


// Faster transition table representation for determined FSM
typedef yvector<size_t> DeterminedTransitions;

// Mapping of states into partitions in minimization algorithm.
typedef yvector<size_t> StateClassMap;

struct MinimizeEquality: public ybinary_function<size_t, size_t, bool> {
public:

	MinimizeEquality(const DeterminedTransitions& tbl, const yvector<Char>& letters, const StateClassMap* clMap):
		m_tbl(&tbl), m_letters(&letters), m_prev(clMap), m_final(0) {}

	MinimizeEquality(const DeterminedTransitions& tbl, const yvector<Char>& letters, const Fsm::FinalTable& final):
		m_tbl(&tbl), m_letters(&letters), m_prev(0), m_final(&final) {}

	inline bool operator()(size_t a, size_t b) const
	{
		if (m_final && (m_final->find(a) != m_final->end()) != (m_final->find(b) != m_final->end()))
			return false;
		if (m_prev) {
			if ((*m_prev)[a] != (*m_prev)[b])
				return false;
			for (yvector<Char>::const_iterator it = m_letters->begin(), ie = m_letters->end(); it != ie; ++it)
				if ((*m_prev)[Next(a, *it)] != (*m_prev)[Next(b, *it)])
					return false;
		}
		return true;
	};

private:
	const DeterminedTransitions* m_tbl;
	const yvector<Char>* m_letters;
	const StateClassMap* m_prev;
	const Fsm::FinalTable* m_final;

	size_t Next(size_t state, Char letter) const
	{
		return (*m_tbl)[state * MaxChar + letter];
	}
};

// Updates the mapping of states to partitions.
// Returns true if the mapping has changed.
// TODO: Think how to update only changed states
bool UpdateStateClassMap(StateClassMap& clMap, const Partition<size_t, MinimizeEquality>& stPartition)
{
	YASSERT(clMap.size() != 0);
	bool changed = false;
	for (size_t st = 0; st < clMap.size(); st++) {
		size_t cl = stPartition.Representative(st);
		if (clMap[st] != cl) {
			clMap[st] = cl;
			changed = true;
		}
	}
	return changed;
}

void Fsm::Minimize()
{
	// Minimization algorithm is only applicable to a determined FSM.
	YASSERT(determined);

	PIRE_IFDEBUG(Cdbg << "=== Minimizing ===" << Endl << *this << Endl);

	DeterminedTransitions detTran(Size() * MaxChar);
	for (TransitionTable::const_iterator j = m_transitions.begin(), je = m_transitions.end(); j != je; ++j) {
		for (TransitionRow::const_iterator k = j->begin(), ke = j->end(); k != ke; ++k) {
			YASSERT(k->second.size() == 1);
			detTran[(j - m_transitions.begin()) * MaxChar + k->first] = *(k->second.begin());
		}
	}

	// Precompute letter classes
	yvector<Char> distinctLetters;
	distinctLetters.reserve(letters.Size());
	for (LettersTbl::ConstIterator lit = letters.Begin(); lit != letters.End(); ++lit) {
		distinctLetters.push_back(lit->first);
	}

	typedef Partition<size_t, MinimizeEquality> StateClasses;

	StateClasses last(MinimizeEquality(detTran, distinctLetters, m_final));

	PIRE_IFDEBUG(Cdbg << "Initial finals: { " << Join(m_final.begin(), m_final.end(), ", ") << " }" << Endl);
	// Make an initial states partition
	for (size_t state = 0; state < Size(); ++state)
		last.Append(state);

	StateClassMap stateClassMap(Size());

	PIRE_IFDEBUG(unsigned cnt = 0);
	// Iteratively split states into equality classes
	while (UpdateStateClassMap(stateClassMap, last)) {
		PIRE_IFDEBUG(Cdbg << "Stage " << cnt++ << ": state classes = " << last << Endl);
		last.Split(MinimizeEquality(detTran, distinctLetters, &stateClassMap));
	}

	// Resize FSM
	TransitionTable oldTransitions;
	FinalTable oldFinal;
	Outputs oldOutputs;
	Tags oldTags;
	m_transitions.swap(oldTransitions);
	m_final.swap(oldFinal);
	outputs.swap(oldOutputs);
	tags.swap(oldTags);
	Resize(last.Size());
	PIRE_IFDEBUG(Cdbg << "[min] Resizing FSM to " << last.Size() << " states" << Endl);

	// Union equality classes into new states
	size_t fromIdx = 0;
	for (TransitionTable::iterator from = oldTransitions.begin(), fromEnd = oldTransitions.end(); from != fromEnd; ++from, ++fromIdx) {
		size_t dest = last.Index(fromIdx);
		PIRE_IFDEBUG(Cdbg << "[min] State " << fromIdx << " becomes state " << dest << Endl);
		for (TransitionRow::iterator letter = from->begin(), letterEnd = from->end(); letter != letterEnd; ++letter) {
			YASSERT(letter->second.size() == 1 || !"FSM::minimize(): FSM not deterministic");
			PIRE_IFDEBUG(Cdbg << "[min] connecting " << dest << " --" << CharDump(letter->first) << "--> "
				<< last.Index(*letter->second.begin()) << Endl);
			Connect(dest, last.Index(*letter->second.begin()), letter->first);
		}
		if (oldFinal.find(fromIdx) != oldFinal.end()) {
			SetFinal(dest, true);
			PIRE_IFDEBUG(Cdbg << "[min] New state " << dest << " becomes final because of old state " << fromIdx << Endl);
		}

		// Append tags
		Tags::iterator ti = oldTags.find(fromIdx);
		if (ti != oldTags.end()) {
			tags[dest] |= ti->second;
			PIRE_IFDEBUG(Cdbg << "[min] New state " << dest << " carries tag " << ti->second << " because of old state " << fromIdx << Endl);
		}
	}
	initial = last.Representative(initial);

	// Restore outputs
	for (Outputs::iterator oit = oldOutputs.begin(), oie = oldOutputs.end(); oit != oie; ++oit)
		for (Outputs::value_type::second_type::iterator oit2 = oit->second.begin(), oie2 = oit->second.end(); oit2 != oie2; ++oit2)
			outputs[last.Index(oit->first)].insert(ymake_pair(last.Index(oit2->first), oit2->second));

	ClearHints();
	PIRE_IFDEBUG(Cdbg << "=== Minimized (" << Size() << " states) ===" << Endl << *this << Endl);
}

Fsm& Fsm::Canonize(size_t maxSize /* = 0 */)
{
	if (!IsDetermined()) {
		if (!Determine(maxSize)) 
			throw Error("regexp pattern too complicated");
	}
	Minimize();
	return *this;
}

void Fsm::PrependAnything()
{
	size_t newstate = Size();
	Resize(Size() + 1);
	for (size_t letter = 0; letter < MaxChar; ++letter)
		Connect(newstate, newstate, letter);
	
	Connect(newstate, initial);
	initial = newstate;

	determined = false;
}

void Fsm::AppendAnything()
{
	size_t newstate = Size();
	Resize(Size() + 1);
	for (size_t letter = 0; letter < MaxChar; ++letter)
		Connect(newstate, newstate, letter);
	
	ConnectFinal(newstate);
	ClearFinal();
	SetFinal(newstate, 1);

	determined = false;
}

Fsm& Fsm::Surround()
{
	PrependAnything();
	AppendAnything();
	return *this;
}

void Fsm::Divert(size_t from, size_t to, size_t dest)
{
	if (to == dest)
		return;

	// Assign the output
	Outputs::iterator oi = outputs.find(from);
	if (oi != outputs.end()) {
		Outputs::mapped_type::iterator oi2 = oi->second.find(to);
		if (oi2 != oi->second.end()) {
			unsigned long output = oi2->second;
			oi->second.erase(oi2);
			oi->second.insert(ymake_pair(dest, output));
		}
	}

	// Assign the transition
	for (TransitionRow::iterator i = m_transitions[from].begin(), ie = m_transitions[from].end(); i != ie; ++i) {
		TransitionRow::mapped_type::iterator di = i->second.find(to);
		if (di != i->second.end()) {
			i->second.erase(di);
			i->second.insert(dest);
		}
	}

	ClearHints();
}


}
