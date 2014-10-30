#pragma once

#include <util/generic/strbuf.h>

bool IsSpace(const char* s, size_t len) throw ();

/// Определяет, является ли строка набором пробельных символов.
static inline bool IsSpace(const TStringBuf& s) throw () {
    return IsSpace(~s, +s);
}

/// Определяет, можно ли интерпретировать значение строки как арабское число.
// Returns "true" if given string is number ([0-9]+)
bool IsNumber(const TStringBuf& s) throw ();

/// Возвращает 'true' если аргумент - одна из строк "yes", "on", "1", "true", "da".
/// @details Регистр символов не учитывается.
bool istrue(const TStringBuf& value);

/// Возвращает 'false' если аргумент - одна из строк "no", "off", "0", "false", "net".
/// @details Регистр символов не учитывается.
bool isfalse(const TStringBuf& value);
