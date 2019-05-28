/*
 * unicode_support.cpp -- implementation for the EnableUnicodeSequences feature.
 *
 * Copyright (c) 2018 YANDEX LLC
 * Author: Andrey Logvin <andry@logvin.net>
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


#include "unicode_support.h"

#include <contrib/libs/pire/pire/re_lexer.h>
//#include <util/stream/output.h>

namespace Pire {
	
namespace {
    bool IsHexDigit(wchar32 ch) {
        return ch < 256 && std::isxdigit(ch) != 0;
    }

    static const long hextable[] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    class EnableUnicodeSequencesImpl : public Feature {
    public:
        bool Accepts(wchar32 c) const {
            return c == (Control | 'x');
        }

        Term Lex() {
            ystring hexStr;
            GetChar();
            wchar32 ch = PeekChar();
            if (ch == '{') {
                GetChar();
                hexStr = ReadHexDigit([](wchar32 ch, size_t numAdded) -> bool {return ch == End || (numAdded != 0 && ch == '}');});
                ch = GetChar();
                if (ch != '}') {
                    Error("Pire::EnableUnicodeSequencesImpl::Lex(): \"\\x{...\" sequence should be closed by \"}\"");
                }
            } else {
                hexStr = ReadHexDigit([](wchar32, size_t numAdded) -> bool {return numAdded == 2;});
                if (hexStr.size() != 2) {
                    Error("Pire::EnableUnicodeSequencesImpl::Lex(): \"\\x...\" sequence should contain two symbols");
                }
            }
            return Term::Character(HexToDec(hexStr));
        }

    private:
        const wchar32 MAX_UNICODE = 0x10FFFF;

//        ystring ReadHexDigit(std::function<bool(wchar32, size_t)> shouldStop) {
        template<typename Lambda>
        ystring ReadHexDigit(Lambda shouldStop) {
            ystring result;
            wchar32 ch = GetChar();
            while (!shouldStop(ch, result.size())) {
                if (!IsHexDigit(ch)) {
                    Error("Pire::EnableUnicodeSequencesImpl::Lex(): \"\\x...\" sequence contains non-valid hex number");
                }
                result.push_back(ch);
                ch = GetChar();
            }
            UngetChar(ch);
            return result;
        }

        wchar32 HexToDec(const ystring& hexStr) {
            wchar32 converted;
            try {
                // The above code is replacement for this
                // converted = std::stoul(hexStr, 0, 16);

                // StdOutStream() << hexStr << Endl;
                converted = 0;
                for (auto it = hexStr.begin(); it != hexStr.end(); ++it) {
                    converted = (converted << 4) | hextable[*it];
                }

                if (converted > MAX_UNICODE) {
                    throw std::out_of_range("stoul");
                }

            } catch (std::out_of_range&) {
                converted = MAX_UNICODE + 1;
            }
            if (converted > MAX_UNICODE) {
                Error("Pire::EnableUnicodeSequencesImpl::Lex(): hex number in \"\\x...\" sequence is too large");
            }
            return converted;
        }
    };
}
	
namespace Features {
    Feature* EnableUnicodeSequences() { return new EnableUnicodeSequencesImpl; }
};
}
