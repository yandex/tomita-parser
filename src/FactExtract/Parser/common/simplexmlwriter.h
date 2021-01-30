#pragma once


#include "libxmlutil.h"
#include <util/system/mutex.h>
#include <util/stream/file.h>
#include <util/generic/stroka.h>

class CSimpleXMLWriter
{
public:
    CSimpleXMLWriter(Stroka path, Stroka strRoot, ECharset encoding);
    CSimpleXMLWriter(Stroka path, Stroka strRoot, ECharset encoding, Stroka strFirstTag, bool bAppend = false);
    CSimpleXMLWriter(TOutputStream* outStream, Stroka strRoot, ECharset encoding, Stroka strFirstTag, bool bAppend = false);

    ~CSimpleXMLWriter();

    inline xmlDocPtr GetXMLDoc() { return m_piDoc.Get(); };
    inline TXmlNodePtrBase GetXMLRoot() { return m_piRoot; };

    void Start();
    void Finish();

    CSimpleXMLWriter& operator<<(const Stroka& s);
    CSimpleXMLWriter& operator<<(const char* s);

protected:
    void SaveSubTree();
    void SaveSubTreeToStream(TOutputStream& stream);

    Stroka EncodeText(const Wtroka& text) const {
        return WideToChar(text, Encoding);
    }

    const char* EncodingName() {
        return NameByCharset(Encoding);
    }
private:
    void Open(Stroka path, Stroka strRoot, Stroka strFirstTag, bool bAppend);

protected:
    TOutputStream* m_ActiveOutput;
    ECharset Encoding;

private:
    TXmlDocPtr m_piDoc;
    TXmlNodePtrBase m_piRoot;

    THolder<TOFStream> m_outFile;

    Stroka m_strRoot;
    TMutex m_SaveStringCS;
    bool m_bAppend;
    bool m_finished;
};
