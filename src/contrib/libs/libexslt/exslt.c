/* The following code is the modified part of the libexslt
 * available at http://xmlsoft.org/XSLT/EXSLT/
 * under the terms of the MIT License
 * http://opensource.org/licenses/mit-license.html
 */

#define IN_LIBEXSLT
#include "libexslt.h"

#include <libxml/xmlversion.h>

//#include "config.h"

#include <contrib/libs/libxslt/xsltconfig.h>
#include <contrib/libs/libxslt/extensions.h>

#include "exsltconfig.h"
#include "exslt.h"

const char *exsltLibraryVersion = LIBEXSLT_VERSION_STRING 
				LIBEXSLT_VERSION_EXTRA;
const int exsltLibexsltVersion = LIBEXSLT_VERSION;
const int exsltLibxsltVersion = LIBXSLT_VERSION;
const int exsltLibxmlVersion = LIBXML_VERSION;

/**
 * exsltRegisterAll:
 *
 * Registers all available EXSLT extensions
 */
void
exsltRegisterAll (void) {
    exsltCommonRegister();
#ifdef EXSLT_CRYPTO_ENABLED
    exsltCryptoRegister();
#endif
    exsltMathRegister();
    exsltSetsRegister();
    exsltFuncRegister();
    exsltStrRegister();
    exsltDateRegister();
    exsltSaxonRegister();
    exsltDynRegister();
}

