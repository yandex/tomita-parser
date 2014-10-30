/*
 * capture.cpp -- a helper for compiling CapturingScanner
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


#include <stdexcept>

#include "capture.h"

namespace Pire {
	
namespace {
	class CaptureImpl: public Feature {
	public:
		CaptureImpl(size_t pos)
			: State(0)
			, Pos(pos)
			, Level(0)
		{}
		
		bool Accepts(wchar32 c) const { return c == '('; }
		Term Lex()
		{
			if (GetChar() != '(')
				Error("How did we get here?!..");
			
			if (State == 0 && Pos > 1)
				--Pos;
			else if (State == 0 && Pos == 1) {
				State = 1;
				Level = 0;
			} else if (State == 1) {
				++Level;
			}
			return Term(TokenTypes::Open);
		}
		
		void Parenthesized(Fsm& fsm)
		{
			if (State == 1 && Level == 0) {
				SetCaptureMark(fsm);
				State = 2;
			} else if (State == 1 && Level > 0)
				--Level;
		}
	private:
		unsigned State;
		size_t Pos;
		size_t Level;
		
		void SetCaptureMark(Fsm& fsm)
		{
			fsm.Resize(fsm.Size() + 2);
			fsm.Connect(fsm.Size() - 2, fsm.Initial());
			fsm.ConnectFinal(fsm.Size() - 1);

			fsm.ClearOutputs();
			fsm.SetOutput(fsm.Size() - 2, fsm.Initial(), CapturingScanner::BeginCapture);
			for (size_t state = 0; state < fsm.Size() - 2; ++state)
				if (fsm.IsFinal(state))
					fsm.SetOutput(state, fsm.Size() - 1, CapturingScanner::EndCapture);

			fsm.SetInitial(fsm.Size() - 2);
			fsm.ClearFinal();
			fsm.SetFinal(fsm.Size() - 1, true);
			fsm.SetIsDetermined(false);
		}
		
		void FinishBuild() {}
	};
}
	
namespace Features {
	Feature* Capture(size_t pos) { return new CaptureImpl(pos); }
};
}
