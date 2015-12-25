#pragma once

// Author: smikler@


#include <util/generic/stroka.h>

/* Converts pcre regular expression to pire compatible format:
 *   - replaces "\\#" with "#"
 *   - replaces "\\=" with "="
 *   - replaces "\\:" with ":"
 *   - removes "?P<...>"
 *   - removes "?:"
 *   - removes "()" recursively
 *   - replaces "??" with "?"
 *   - replaces "*?" with "*"
 * NOTE:
 *   - Not fully tested!
 */
Stroka Pcre2Pire(const Stroka& src);
