#pragma once

#include <contrib/libs/libxml/include/libxml/parser.h>
#include <contrib/libs/libxml/include/libxml/xpath.h>

#include <contrib/libs/libxml/include/libxml/xpathInternals.h>
#include <contrib/libs/libxml/include/libxml/tree.h>
#include <contrib/libs/libxml/include/libxml/parser.h>
#include <contrib/libs/libxml/include/libxml/xmlsave.h>


#include <util/generic/stroka.h>
#include <util/charset/recyr.hh>
#include <util/string/cast.h>


namespace NXml {

class TXmlDocFree {
public:
    static void Destroy(xmlDoc* t) {
        xmlFreeDoc(t);
    }
};

class TXmlNodeFree {
public:
    static void Destroy(xmlNode* t) {
        xmlFreeNode(t);
    }
};

class TXmlFree {
public:
    template <typename T>
    static void Destroy(T* t) {
        xmlFree(t);
    }
};

typedef THolder<xmlChar, TXmlFree> TXmlStringHolder;

// Simple wrapper on xmlNodePtr without ownership
class TNodePtrBase: public TPointerBase<TNodePtrBase, xmlNode> {
public:
    TNodePtrBase(xmlNodePtr node = NULL)
        : Node(node)
    {
    }

    xmlNodePtr Get() const {
        return Node;
    }


    // returned string is utf-8
    Stroka GetNodeContent() const {
        TXmlStringHolder s(xmlNodeGetContent(Node));
        return Stroka((const char*)s.Get());
    }

    // returned string is utf-8
    Stroka GetAttr(const char* strAttr) const {
        TXmlStringHolder s(xmlGetProp(Node, (const xmlChar*)strAttr));
        return Stroka((const char*)s.Get());
    }

    Wtroka GetWAttr(const char* strAttr) const {
        TXmlStringHolder s(xmlGetProp(Node, (const xmlChar*)strAttr));
        return UTF8ToWide((const char*)s.Get());
    }

    template <typename T>
    T GetNumericAttr(const char* strAttr) const {
        //STATIC_CHECK(TTypeTraits<T>::IsArithmetic, "Non-arithmetic type");
        TXmlStringHolder s(xmlGetProp(Node, (const xmlChar*)strAttr));
        return ::FromString<T>((const char*)s.Get());
    }

    bool HasAttr(const char* strAttr) const {
        return NULL != xmlHasProp(Node, (const xmlChar*)strAttr);
    }

    bool HasName(const char* name) const {
        return xmlStrcmp(Node->name, (const xmlChar*)name) == 0;
    }


    TNodePtrBase AddChild(xmlNodePtr node) {
       return xmlAddChild(Node, node);
    }

    TNodePtrBase AddNewChild(const char* strName) {
       return AddChild(xmlNewNode(NULL, (const xmlChar*)strName));
    }

    TNodePtrBase AddNewChild(const Stroka& strName) {
       return AddNewChild(strName.c_str());
    }

    TNodePtrBase AddNewTextNodeChild(const TWtringBuf& content) {
        return xmlAddChild(Node, NewTextNode(content));
    }

    TNodePtrBase AddNewTextNodeChild(const char* strName, const TWtringBuf& content);

    void ClearChildren() {
        xmlNodePtr children = Get()->children;
        xmlReplaceNode(children, NULL);
        xmlFreeNodeList(children);
    }



    TNodePtrBase& AddAttr(const char* attrName, const TWtringBuf& attrVal);

    // @attrVal should be UTF8 (e.g. ASCII)
    TNodePtrBase& AddAttr(const char* attrName, const Stroka& attrVal) {
        return AddRawAttr(attrName, attrVal.c_str());
    }

    TNodePtrBase& AddAttr(const char* attrName, int val) {
        char buffer[256];
        size_t len = ::ToString(val, buffer, 255);
        buffer[len] = 0;
        return AddRawAttr(attrName, buffer);
    }

    void RemoveAttr(const char* strAttrName) {
        xmlAttrPtr n = xmlHasProp(Node, (const xmlChar*)strAttrName);
        if (n != NULL)
            xmlRemoveProp(n);
    }


    // unlink the node from current context and free it
    void Unlink() {
        xmlReplaceNode(Get(), NULL);
        xmlFreeNode(Get());
        Node = NULL;
    }

    void SaveToStream(TOutputStream* output, ECharset encoding) const;

protected:
    static xmlNodePtr NewTextNode(const TWtringBuf& content);

private:
    TNodePtrBase& AddRawAttr(const char* attrName, const char* attrVal) {
        // @attrVal should be null-terminated!
        xmlNewProp(Node, (const xmlChar*)attrName, (const xmlChar*)attrVal);
        return *this;
    }

private:
    xmlNodePtr Node;
};


// Smart pointer with ownership (TAutoPtr)
class TNodePtr: public TAutoPtr<xmlNode, TXmlNodeFree>, public TNodePtrBase {
    typedef TAutoPtr<xmlNode, TXmlNodeFree> TAutoPtrBase;
public:
    TNodePtr()
        : TAutoPtrBase()
        , TNodePtrBase()
    {
    }

    TNodePtr(xmlNodePtr node)
        : TAutoPtrBase(node)
        , TNodePtrBase(node)
    {
    }

    explicit TNodePtr(const char* name)
        : TAutoPtrBase(xmlNewNode(NULL, (const xmlChar*)name))
        , TNodePtrBase(TAutoPtrBase::Get())
    {
    }

    explicit TNodePtr(const Stroka& name)
        : TAutoPtrBase(xmlNewNode(NULL, (const xmlChar*)name.c_str()))
        , TNodePtrBase(TAutoPtrBase::Get())
    {
    }

    static TNodePtr NewTextNode(const TWtringBuf& content) {
        return TNodePtr(TNodePtrBase::NewTextNode(content));
    }

    using TAutoPtrBase::Get;

    TNodePtrBase AddChild(TNodePtr node) {
        return TNodePtrBase::AddChild(node.Release());
    }

    TNodePtrBase AddChildFront(TNodePtr node);
};

class TDocPtr: public TAutoPtr<xmlDoc, TXmlDocFree> {
    typedef TAutoPtr<xmlDoc, TXmlDocFree> TBase;
public:
    TDocPtr()
        : TBase()
    {
    }

    TDocPtr(xmlDocPtr doc)
        : TBase(doc)
    {
    }

    explicit TDocPtr(const char* version) {
        Reset(version);
    }

    static TDocPtr ParseFromFile(const Stroka& file) {
        return xmlParseFile(file.c_str());
    }

    void SaveToFile(const Stroka& file, ECharset encoding) const {
        xmlSaveFileEnc(file.c_str(), Get(), NameByCharset(encoding));
    }



    void Reset(const char* version) {
        TBase::Reset(xmlNewDoc((const xmlChar*)version));
    }

    TNodePtrBase AddNewChild(const char* name, const char* content = "") {
        xmlNodePtr ret = xmlNewDocNode(Get(), NULL, (const xmlChar*)name, (const xmlChar*)content);
        xmlDocSetRootElement(Get(), ret);
        return ret;
    }

    TNodePtrBase AddNewChildPI(const char* name, const char* content) {
        xmlNodePtr ret = xmlNewDocPI(Get(), (const xmlChar *)name, (const xmlChar*)content);
        xmlDocSetRootElement(Get(), ret);
        return ret;
    }

    void SetEncoding(ECharset encoding) {
        Get()->encoding = xmlStrdup((const xmlChar*)NameByCharset(encoding));
    }
};


}   //namespace NXml


typedef NXml::TNodePtr TXmlNodePtr;
typedef NXml::TNodePtrBase TXmlNodePtrBase;

typedef NXml::TDocPtr TXmlDocPtr;
//typedef xmlDocPtr TXmlDocPtrBase;



