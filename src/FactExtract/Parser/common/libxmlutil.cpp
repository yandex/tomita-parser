#include "libxmlutil.h"
#include <util/stream/output.h>

#include "utilit.h"

#include <kernel/gazetteer/common/recode.h>


namespace NXml {


static inline const xmlChar* WideToXmlChar(const TWtringBuf& str, NGzt::TStaticCharCodec<>& codec) {
    TStringBuf ret = codec.WideToUtf8(str);
    YASSERT(ret.IsInited() && *(ret.data() + ret.size()) == 0);     // TStaticCharCodec produces null-terminated strings
    return (const xmlChar*)ret.data();
}

xmlNodePtr TNodePtrBase::NewTextNode(const TWtringBuf& content) {
    NGzt::TStaticCharCodec<> codec;
    return xmlNewText(WideToXmlChar(content, codec));
}

TNodePtrBase TNodePtrBase::AddNewTextNodeChild(const char* strName, const TWtringBuf& content) {
    NGzt::TStaticCharCodec<> codec;
    return xmlNewTextChild(Node, NULL, (const xmlChar*)strName, WideToXmlChar(content, codec));
}

TNodePtrBase& TNodePtrBase::AddAttr(const char* attrName, const TWtringBuf& attrVal) {
    NGzt::TStaticCharCodec<> codec;
    return AddRawAttr(attrName, (const char*)WideToXmlChar(attrVal, codec));
}


static int WriteXmlBufToStream(void* context, const char * buffer, int len) {
    static_cast<TOutputStream*>(context)->Write(buffer, len);
    return len;
}

static int CloseStreamNoOp(void*) {
    return 0;
}


void TNodePtrBase::SaveToStream(TOutputStream* output, ECharset encoding) const {
    const char* encodingName = NameByCharset(encoding);
    xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(WriteXmlBufToStream, CloseStreamNoOp,
        output, xmlFindCharEncodingHandler(encodingName));
    xmlNodeDumpOutput(buf, Get()->doc, Get(), 0, 1, encodingName);
    xmlOutputBufferClose(buf);
}

TNodePtrBase TNodePtr::AddChildFront(TNodePtr node) {
    xmlNodePtr firstChild = Get()->children;
    if (firstChild == NULL)
        return AddChild(node);

    xmlNodePtr newFirstChild = xmlAddPrevSibling(firstChild, node.Get());
    if (newFirstChild == NULL)
        return firstChild;
    node.Release();     // already owned by @this.
    return newFirstChild;
}

}   // namespace NXml
