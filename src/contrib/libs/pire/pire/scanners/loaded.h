/*
 * loaded.h -- a definition of the LoadedScanner
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


#ifndef PIRE_SCANNERS_LOADED_H
#define PIRE_SCANNERS_LOADED_H

#include <string.h>

#include <contrib/libs/pire/pire/fsm.h>
#include <contrib/libs/pire/pire/partition.h>

#include "common.h"

#ifdef PIRE_DEBUG
#include <iostream>
#endif

namespace Pire {

/**
* A loaded scanner -- the deterministic scanner having actions
*                     associated with states and transitions
*
* Not a complete scanner itself (hence abstract), this class provides
* infrastructure for regexp-based algorithms (e.g. counts or captures),
* supporting major part of scanner construction, (de)serialization,
* mmap()-ing, etc.
*
* It is a good idea to override copy ctor, operator= and swap()
* in subclasses to avoid mixing different scanner types in these methods.
* Also please note that subclasses should not have any data members of thier own.
*/
class LoadedScanner {
public:
	typedef ui8         Letter;
	typedef ui32        Action;
	typedef ui8         Tag;

	typedef size_t InternalState;

	union Transition {
		size_t raw;			// alignment hint for compiler
		struct {
			ui32 shift;
			Action action;
		};
	};

	// Override in subclass, if neccessary
	enum { 
		FinalFlag = 0,
		DeadFlag  = 0
	};

protected:	
	LoadedScanner() { Alias(Null()); }
	
	LoadedScanner(const LoadedScanner& s): m(s.m)
	{
		if (s.m_buffer) {
			m_buffer = new char [BufSize()];
			memcpy(m_buffer, s.m_buffer, BufSize());
			Markup(m_buffer);
			m.initial = (InternalState)m_jumps + (s.m.initial - (InternalState)s.m_jumps);
		} else {
			Alias(s);
		}
	}

	void Swap(LoadedScanner& s)
	{
		DoSwap(m_buffer, s.m_buffer);
		DoSwap(m.statesCount, s.m.statesCount);
		DoSwap(m.lettersCount, s.m.lettersCount);
		DoSwap(m.regexpsCount, s.m.regexpsCount);
		DoSwap(m.initial, s.m.initial);
		DoSwap(m_letters, s.m_letters);
		DoSwap(m_jumps, s.m_jumps);
		DoSwap(m_actions, s.m_actions);
		DoSwap(m_tags, s.m_tags);
	}

	LoadedScanner& operator = (const LoadedScanner& s) { LoadedScanner(s).Swap(*this); return *this; }

public:
	size_t Size() const { return m.statesCount; }

	bool Empty() const { return m_jumps == Null().m_jumps; }

	size_t RegexpsCount() const { return Empty() ? 0 : m.regexpsCount; }

	const void* Mmap(const void* ptr, size_t size)
	{
		Impl::CheckAlign(ptr);
		LoadedScanner s;
		const size_t* p = reinterpret_cast<const size_t*>(ptr);
		Impl::ValidateHeader(p, size, 4, sizeof(s.m));

		Locals* locals;
		Impl::MapPtr(locals, 1, p, size);
		memcpy(&s.m, locals, sizeof(s.m));
		
		Impl::MapPtr(s.m_letters, MaxChar, p, size);
		Impl::MapPtr(s.m_jumps, s.m.statesCount * s.m.lettersCount, p, size);
		Impl::MapPtr(s.m_actions, s.m.statesCount * s.m.lettersCount, p, size);
		Impl::MapPtr(s.m_tags, s.m.statesCount, p, size);
		
		s.m.initial += reinterpret_cast<size_t>(s.m_jumps);
		Swap(s);

		return (const void*) p;
	}

	void Save(yostream*) const;
	void Load(yistream*);

		template<class Eq>
	void Init(size_t states, const Partition<Char, Eq>& letters, size_t startState, size_t regexpsCount = 1)
	{
		m.statesCount = states;
		m.lettersCount = letters.Size();
		m.regexpsCount = regexpsCount;
		m_buffer = new char[BufSize()];
		memset(m_buffer, 0, BufSize());
		Markup(m_buffer);

		m.initial = reinterpret_cast<size_t>(m_jumps + startState * m.lettersCount);

		// Build letter translation table
		Fill(m_letters, m_letters + sizeof(m_letters)/sizeof(*m_letters), 0);
		for (typename Partition<Char, Eq>::ConstIterator it = letters.Begin(), ie = letters.End(); it != ie; ++it)
			for (yvector<Char>::const_iterator it2 = it->second.second.begin(), ie2 = it->second.second.end(); it2 != ie2; ++it2)
				m_letters[*it2] = it->second.first;
	}


	void SetJump(size_t oldState, Char c, size_t newState, Action action)
	{
		YASSERT(m_buffer);
		YASSERT(oldState < m.statesCount);
		YASSERT(newState < m.statesCount);

		size_t shift = (newState - oldState) * m.lettersCount * sizeof(*m_jumps);
		Transition tr;
		tr.shift = (ui32)shift;
		tr.action = action;
		m_jumps[oldState * m.lettersCount + m_letters[c]] = tr;
		m_actions[oldState * m.lettersCount + m_letters[c]] = action;
	}

	Action RemapAction(Action action) { return action; }

	void SetInitial(size_t state) { YASSERT(m_buffer); m.initial = reinterpret_cast<size_t>(m_jumps + state * m.lettersCount); }
	void SetTag(size_t state, Tag tag) { YASSERT(m_buffer); m_tags[state] = tag; }
	void FinishBuild() {}

	size_t StateIdx(InternalState s) const
	{
		return (reinterpret_cast<Transition*>(s) - m_jumps) / m.lettersCount;
	}

	i64 SignExtend(i32 i) const { return i; }

protected:

	static const size_t MAX_RE_COUNT      = 16;

	static const Action IncrementMask     = 0x0f;
	static const Action ResetMask         = 0x0f << MAX_RE_COUNT;

	// TODO: maybe, put fields in private section and provide data accessors

	struct Locals {
		ui32 statesCount;
		ui32 lettersCount;
		ui32 regexpsCount;
		size_t initial;
	} m;

	char* m_buffer;

	Letter* m_letters;
	Transition* m_jumps;
	Action* m_actions;
	Tag* m_tags;

	virtual ~LoadedScanner();

	size_t BufSize() const
	{
		return
			MaxChar * sizeof(*m_letters)
			+ m.lettersCount * m.statesCount * sizeof(*m_jumps)
			+ m.statesCount * sizeof(*m_tags)
			+ m.lettersCount * m.statesCount * sizeof(*m_actions)
			;
	}

private:
	explicit LoadedScanner(Fsm& fsm)
	{
		fsm.Canonize();
		Init(fsm.Size(), fsm.Letters(), fsm.Initial());
		BuildScanner(fsm, *this);
	}

	// Only used to force Null() call during static initialization, when Null()::n can be
	// initialized safely by compilers that don't support thread safe static local vars
	// initialization
	static const LoadedScanner* m_null;

	inline static const LoadedScanner& Null()
	{
		static const LoadedScanner n = Fsm::MakeFalse().Compile<LoadedScanner>();
		return n;
	}
	
	void Markup(void* buf)
	{
		m_letters = reinterpret_cast<Letter*>(buf);
		m_jumps   = reinterpret_cast<Transition*>(m_letters + MaxChar);
		m_actions = reinterpret_cast<Action*>(m_jumps + m.statesCount * m.lettersCount);
		m_tags    = reinterpret_cast<Tag*>(m_actions + m.statesCount * m.lettersCount);
	}
	
	void Alias(const LoadedScanner& s)
	{
		memcpy(&m, &s.m, sizeof(m));
		m_buffer = 0;
		m_letters = s.m_letters;
		m_jumps = s.m_jumps;
		m_tags = s.m_tags;
		m_actions = s.m_actions;
	}

	template<class Eq>
	LoadedScanner(size_t states, const Partition<Char, Eq>& letters, size_t startState, size_t regexpsCount = 1)
	{
		Init(states, letters, startState, regexpsCount);
	}

	friend class Fsm;	
};
	
inline LoadedScanner::~LoadedScanner()
{
	delete [] m_buffer;
}

}


#endif
