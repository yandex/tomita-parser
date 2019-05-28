/*
 * unicode_support.h -- declaration of the EnableUnicodeSequences feature.
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


#ifndef PIRE_EXTRA_SUPPORT_UNICODE_H
#define PIRE_EXTRA_SUPPORT_UNICODE_H

#include <memory>

namespace Pire {
class Feature;
namespace Features {

   /**
    * A feature which tells Pire to convert \x{...} and \x... sequences
    * to accordingly UTF-32 symbols
    * e.g. \x00 == '\0', \x41 == A
    */
    Feature* EnableUnicodeSequences();
}
}

#endif
