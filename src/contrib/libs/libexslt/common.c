/* The following code is the modified part of the libexslt
 * available at http://xmlsoft.org/XSLT/EXSLT/
 * under the terms of the MIT License
 * http://opensource.org/licenses/mit-license.html
 */

#define IN_LIBEXSLT
#include "libexslt.h"

#if defined(WIN32) && !defined (__CYGWIN__) && (!__MINGW32__)
#include <contrib/libs/libxslt/win32config.h>
//#else
//#include "config.h"
#endif

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <contrib/libs/libxslt/xsltconfig.h>
#include <contrib/libs/libxslt/xsltutils.h>
#include <contrib/libs/libxslt/xsltInternals.h>
#include <contrib/libs/libxslt/extensions.h>
#include <contrib/libs/libxslt/transform.h>
#include <contrib/libs/libxslt/extra.h>
#include <contrib/libs/libxslt/preproc.h>

#include "exslt.h"

static void
exsltNodeSetFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    if (xmlXPathStackIsNodeSet (ctxt)) {
	xsltFunctionNodeSet (ctxt, nargs);
	return;
    } else {
	xmlDocPtr fragment;
	xsltTransformContextPtr tctxt = xsltXPathGetTransformContext(ctxt);
	xmlNodePtr txt;
	xmlChar *strval;
	xmlXPathObjectPtr obj;
	/*
	* SPEC EXSLT:
	* "You can also use this function to turn a string into a text
	* node, which is helpful if you want to pass a string to a
	* function that only accepts a node-set."
	*/
	fragment = xsltCreateRVT(tctxt);
	if (fragment == NULL) {
	    xsltTransformError(tctxt, NULL, tctxt->inst,
		"exsltNodeSetFunction: Failed to create a tree fragment.\n");
	    tctxt->state = XSLT_STATE_STOPPED; 
	    return;
	}
	xsltRegisterLocalRVT(tctxt, fragment);

	strval = xmlXPathPopString (ctxt);
	
	txt = xmlNewDocText (fragment, strval);
	xmlAddChild((xmlNodePtr) fragment, txt);
	obj = xmlXPathNewNodeSet(txt);	
	if (obj == NULL) {
	    xsltTransformError(tctxt, NULL, tctxt->inst,
		"exsltNodeSetFunction: Failed to create a node set object.\n");
	    tctxt->state = XSLT_STATE_STOPPED;
	} else {
	    /*
	     * Mark it as a function result in order to avoid garbage
	     * collecting of tree fragments
	     */
	    xsltExtensionInstructionResultRegister(tctxt, obj);
	}
	if (strval != NULL)
	    xmlFree (strval);
	
	valuePush (ctxt, obj);
    }
}

static void
exsltObjectTypeFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr obj, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    obj = valuePop(ctxt);

    switch (obj->type) {
    case XPATH_STRING:
	ret = xmlXPathNewCString("string");
	break;
    case XPATH_NUMBER:
	ret = xmlXPathNewCString("number");
	break;
    case XPATH_BOOLEAN:
	ret = xmlXPathNewCString("boolean");
	break;
    case XPATH_NODESET:
	ret = xmlXPathNewCString("node-set");
	break;
    case XPATH_XSLT_TREE:
	ret = xmlXPathNewCString("RTF");
	break;
    case XPATH_USERS:
	ret = xmlXPathNewCString("external");
	break;
    default:
	xsltGenericError(xsltGenericErrorContext,
		"object-type() invalid arg\n");
	ctxt->error = XPATH_INVALID_TYPE;
	xmlXPathFreeObject(obj);
	return;
    }
    xmlXPathFreeObject(obj);
    valuePush(ctxt, ret);
}


/**
 * exsltCommonRegister:
 *
 * Registers the EXSLT - Common module
 */

void
exsltCommonRegister (void) {
    xsltRegisterExtModuleFunction((const xmlChar *) "node-set",
				  EXSLT_COMMON_NAMESPACE,
				  exsltNodeSetFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "object-type",
				  EXSLT_COMMON_NAMESPACE,
				  exsltObjectTypeFunction);
    xsltRegisterExtModuleElement((const xmlChar *) "document",
				 EXSLT_COMMON_NAMESPACE,
				 (xsltPreComputeFunction) xsltDocumentComp,
				 (xsltTransformFunction) xsltDocumentElem);
}
