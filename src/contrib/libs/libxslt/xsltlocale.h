/* The following code is the modified part of the libxslt
 * available at http://xmlsoft.org/libxslt
 * under the terms of the MIT License
 * http://opensource.org/licenses/mit-license.html
 */

/*
 * Summary: Locale handling
 * Description: Interfaces for locale handling. Needed for language dependent
 *              sorting.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Nick Wellnhofer
 */

#ifndef __XML_XSLTLOCALE_H__
#define __XML_XSLTLOCALE_H__

#include <libxml/xmlstring.h>

#ifdef XSLT_LOCALE_XLOCALE

#include <locale.h>
#include <xlocale.h>

#ifdef __GLIBC__
/*locale_t is defined only if _GNU_SOURCE is defined*/
typedef __locale_t xsltLocale;
#else
typedef locale_t xsltLocale;
#endif
typedef xmlChar xsltLocaleChar;

#elif defined(XSLT_LOCALE_WINAPI)

#include <windowsa.h>
#include <winnls.h>

typedef LCID xsltLocale;
typedef wchar_t xsltLocaleChar;

#else

#ifndef XSLT_LOCALE_NONE
#define XSLT_LOCALE_NONE
#endif

typedef void *xsltLocale;
typedef xmlChar xsltLocaleChar;

#endif

xsltLocale xsltNewLocale(const xmlChar *langName);
void xsltFreeLocale(xsltLocale locale);
xsltLocaleChar *xsltStrxfrm(xsltLocale locale, const xmlChar *string);
int xsltLocaleStrcmp(xsltLocale locale, const xsltLocaleChar *str1, const xsltLocaleChar *str2);

#endif /* __XML_XSLTLOCALE_H__ */
