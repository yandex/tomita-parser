/* The following code is the modified part of the zlib library
 * available at http://www.zlib.net
 * under the terms of the ZLIB License
 * http://www.zlib.net/zlib_license.html
 */

/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

void ZLIB_INTERNAL inflate_fast OF((z_streamp strm, unsigned start));
