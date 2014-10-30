#include "dictutil.h"
#include <util/charset/codepage.h>

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

TScriptMask ClassifyScript(const wchar16* str, size_t len) {
    TScriptMask scriptMask = 0;
    const wchar16* end = str + len;
    for (const wchar16* p = str; p != end; ++p)
        scriptMask |= ScriptMaskForScript(ScriptByGlyph(*p));
    scriptMask &= ~0x0001; // remove bit SCRIPT_UNKNOWN
    return scriptMask;
}

TScriptMask ClassifyScript(const WStroka& str) {
    return ClassifyScript(str.c_str(), str.length());
}

TScriptMask ClassifyScript(const Stroka& str) {
    TScriptMask scriptMask = 0;
    const char* p = str.c_str();
    const char* end = p + str.length();
    while (p != end) {
        wchar16 ch = (wchar16)csYandex.unicode[(ui8)*p++];
        scriptMask |= ScriptMaskForScript(ScriptByGlyph(ch));
    }
    scriptMask &= ~0x0001; // remove bit SCRIPT_UNKNOWN
    return scriptMask;
}
