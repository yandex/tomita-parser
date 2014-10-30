#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

/// Преобразует текст в HTML-код. Служебные символы HTML («<», «>», ...) заменяются на сущности.
Stroka EncodeHtmlPcdata(const TStringBuf& str, bool qAmp = true);

// reverse of EncodeHtmlPcdata()
Stroka DecodeHtmlPcdata(const Stroka& sz);
