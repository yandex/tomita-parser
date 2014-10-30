#pragma once

#include "catxml.h"

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/region.h>

class TBuffer;
class Stroka;
class TInputStream;
class TOutputStream;

//! represents set of name-value pairs that should be passed to transformation
//! names should correspond to top-level xsl <xsl:param name='Name'/> declarations
typedef ymap<Stroka, Stroka> TXslParams;

//! represents xpath function type
//! the real type is
//! typedef void (*TxmlXPathFunction) (xmlXPathParserContextPtr ctxt, int nargs);
//! but is hidden here to avoid libxml headers
//! you will need that headers in order to implement the function itself
typedef void* TxmlXPathFunction;

//! represents stylesheet ready to perform transformation
class TXslTransform {
private:
    class TImpl;
    THolder<TImpl> Impl;

public:
    //! read and compile xslt from string
    TXslTransform(const Stroka& style, const Stroka& base = "");
    //! read and compile xslt from stream
    TXslTransform(TInputStream& style, const Stroka& base = "");
    ~TXslTransform();

    //! register an xpath function
    void SetFunction(const Stroka& name, const Stroka& uri, TxmlXPathFunction fn);

    //! perform transformation in the most efficient way
    //! @param[in]  src   source xml data, src can be TBuffer
    //! @param[out] dest  result of transformation
    //! @param[in]  p     parameters to pass to xsl
    void Transform(const TDataRegion& src, TBuffer& dest, const TXslParams& p = TXslParams());
    //! perform transformation of multiple xml files concatenated by root element
    //! @param[in]  src   source xml streams to concat and root element name
    //! @param[out] dest  result of transformation
    //! @param[in]  p     parameters to pass to xsl
    void Transform(const TXmlConcatTask& src, TBuffer& dest, const TXslParams& p = TXslParams());
    //! perform transformation
    //! @param[in]  src   source xml data stream
    //! @param[out] dest  result of transformation stream
    //! @param[in]  p     parameters to pass to xsl
    void Transform(TInputStream* src, TOutputStream* dest, const TXslParams& p = TXslParams());
    //! perform transformation
    //! @param[in]  src   source xml data
    //! @param[in]  p     parameters to pass to xsl
    //! @return           result of transformation
    Stroka operator()(const Stroka& src, const TXslParams& p = TXslParams());
};
