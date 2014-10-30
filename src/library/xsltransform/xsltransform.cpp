#include <contrib/libs/libxslt/xslt.h>
#include <contrib/libs/libxslt/transform.h>
#include <contrib/libs/libxslt/xsltutils.h>
#include <contrib/libs/libxslt/extensions.h>

#include <libxml/xinclude.h>

#include <contrib/libs/libexslt/exslt.h>

#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/stream/buffer.h>
#include <util/stream/helpers.h>
#include <util/string/util.h>

#include "xsltransform.h"

template<class T, void (*DestroyFun)(T*)>
struct TFunctionDestroy {
    static inline void Destroy(T* t) throw () {
        if (t)
            DestroyFun(t);
    }
};

#define DEF_HOLDER(type, free) typedef THolder<type, TFunctionDestroy<type, free> > T ## type ## Holder
#define DEF_PTR(type, free) typedef TAutoPtr<type, TFunctionDestroy<type, free> > T ## type ## AutoPtr

// typedef THolder<xmlDoc, TFunctionDestroy<xmlDoc, xmlFreeDoc> > TxmlDocHolder;
DEF_HOLDER(xmlDoc, xmlFreeDoc);
DEF_HOLDER(xmlNode, xmlFreeNode);
DEF_HOLDER(xsltStylesheet, xsltFreeStylesheet);
DEF_PTR(xmlDoc, xmlFreeDoc);
DEF_PTR(xsltStylesheet, xsltFreeStylesheet);
// DEF_HOLDER(xmlOutputBuffer, xmlOutputBufferClose);
#undef DEF_HOLDER
#undef DEF_PTR

static class TXsltIniter {
    public:
        TXsltIniter() {
            xmlInitMemory();
            exsltRegisterAll();
        }

        ~TXsltIniter() throw () {
            xsltCleanupGlobals();
        }
} INITER;

struct TParamsArray {
    yvector<const char*> Array;

    TParamsArray(const TXslParams& p) {
        if (p.size()) {
            for (TXslParams::const_iterator i = p.begin(); i != p.end(); ++i) {
                Array.push_back(~i->first);
                Array.push_back(~i->second);
            }
            Array.push_back((const char*)0);
        }
    }
};

class TXslTransform::TImpl {
private:
    TxsltStylesheetHolder Stylesheet;
    Stroka ErrorMessage;
private:
    static void XMLCDECL XmlErrorSilent(void*, const char*, ...) throw() {
    }
    static void XMLCDECL XmlErrorSave(void* ctx, const char* msg, ...) throw() {
        va_list args;
        va_start(args, msg);
        Stroka line;
        vsprintf(line, msg, args);
        TImpl* self = (TImpl*)ctx;
        self->ErrorMessage += line;
    }
    void InitError() {
        ErrorMessage.clear();
        xmlSetGenericErrorFunc(this, XmlErrorSave); // claimed to be thread-safe
    }

    void SaveResult(xmlDocPtr doc, xsltStylesheetPtr stylesheet, TBuffer& to) {
        xmlOutputBufferPtr buf = xmlAllocOutputBuffer(NULL); // NULL means UTF8
        xsltSaveResultTo(buf, doc, stylesheet);
        if (buf->conv != NULL) {
            to.Assign((const char*)buf->conv->content, buf->conv->use);
        } else {
            to.Assign((const char*)buf->buffer->content, buf->buffer->use);
        }
        xmlOutputBufferClose(buf);
    }

    void Transform(const TxmlDocHolder& srcDoc, TBuffer& dest, const TXslParams& params) {
        //xmlXIncludeProcessFlags(srcDoc.Get(), XSLT_PARSE_OPTIONS);
        if (!srcDoc)
            ythrow yexception() << "source xml is not valid: " << ErrorMessage;
        TxmlDocHolder res(xsltApplyStylesheet(Stylesheet.Get(), srcDoc.Get(), ~TParamsArray(params).Array)); // no params
        ENSURE(!!res, "Failed to apply xslt!");
        SaveResult(res.Get(), Stylesheet.Get(), dest);
    }
public:
    TImpl(const Stroka& style, const Stroka& base = "") {
        InitError();
        TxmlDocHolder sheetDoc(xmlParseMemory(~style, +style));
        if (!!base) {
            xmlNodeSetBase(sheetDoc->children, (xmlChar*)base.c_str());
        }
        if (!sheetDoc)
            ythrow yexception() << "cannot parse xml of xslt: " << ErrorMessage;
        Stylesheet.Reset(xsltParseStylesheetDoc(sheetDoc.Get()));
        if (!Stylesheet)
            ythrow yexception() << "cannot parse xslt: " << ErrorMessage;
        // sheetDoc - ownership transferred to Stylesheet
        sheetDoc.Release();
    }
    ~TImpl() {
        xmlSetGenericErrorFunc(0, 0); // restore default (that is cerr echo)
    }


    void Transform(const TDataRegion& src, TBuffer& dest, const TXslParams& params) {
        InitError();
        TxmlDocHolder srcDoc(xmlParseMemory(~src, +src));
        Transform(srcDoc, dest, params);
    }

    xmlNodePtr ConcatTaskToNode(const TXmlConcatTask& task) {
        const xmlChar* nodeName = reinterpret_cast<const xmlChar*>(task.Element.empty() ? "node" : ~task.Element);
        TxmlNodeHolder taskNode(xmlNewNode(NULL, nodeName));
        for (const TSharedPtr<TInputStream>& stream : task.Streams) {
            TBufferOutput out;
            TransferData(stream.Get(), &out);
            TxmlDocHolder document(xmlParseMemory(out.Buffer().Data(), out.Buffer().Size()));
            xmlNodePtr docRoot = xmlDocGetRootElement(document.Get());
            // nobody owns docRoot
            xmlUnlinkNode(docRoot);
            // taskNode owns docRoot
            xmlAddChild(taskNode.Get(), docRoot);
        }
        for (const TXmlConcatTask& childTask : task.Children) {
            xmlNodePtr child = ConcatTaskToNode(childTask);
            xmlAddChild(taskNode.Get(), child);
        }
        return taskNode.Release();
    }

    void Transform(const TXmlConcatTask& src, TBuffer& dest, const TXslParams& params) {
        InitError();
        TxmlDocHolder srcDoc(xmlNewDoc(NULL));
        xmlNodePtr rootElem = ConcatTaskToNode(src);
        // now srcDoc owns rootElem, shouldn't free it
        xmlDocSetRootElement(srcDoc.Get(), rootElem);
        Transform(srcDoc, dest, params);
    }

    void SetFunction(const Stroka& name, const Stroka& uri, TxmlXPathFunction fn) {
        InitError();

        if (xsltRegisterExtModuleFunction(BAD_CAST ~name, BAD_CAST ~uri, (xmlXPathFunction)fn) < 0) {
            ythrow yexception() << "cannot register xsl function " << uri << ':' << name << ": " << ErrorMessage;
        }
    }
};

TXslTransform::TXslTransform(const Stroka& style, const Stroka& base)
    : Impl(new TImpl(style, base))
{
}

TXslTransform::TXslTransform(TInputStream& str, const Stroka& base)
    : Impl(new TImpl(str.ReadAll(), base))
{
}

TXslTransform::~TXslTransform() {
}

void TXslTransform::SetFunction(const Stroka& name, const Stroka& uri, TxmlXPathFunction fn) {
    Impl->SetFunction(name, uri, fn);
}

void TXslTransform::Transform(const TDataRegion& src, TBuffer& dest, const TXslParams& p) {
    Impl->Transform(src, dest, p);
}

void TXslTransform::Transform(const TXmlConcatTask& src, TBuffer& dest, const TXslParams& p) {
    Impl->Transform(src, dest, p);
}

void TXslTransform::Transform(TInputStream* src, TOutputStream* dest, const TXslParams& p) {
    TBufferOutput srcbuf;
    TransferData(src, &srcbuf);
    TBuffer result;
    Transform(TDataRegion(~srcbuf.Buffer(), +srcbuf.Buffer()), result, p);
    dest->Write(~result, +result);
}

Stroka TXslTransform::operator()(const Stroka& src, const TXslParams& p) {
    TBuffer result;
    Transform(TDataRegion(~src, +src), result, p);
    return Stroka(~result, +result);
}
