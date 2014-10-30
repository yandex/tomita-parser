#pragma once

#include <util/generic/stroka.h>

/// @}

/// @addtogroup Strings_FormattedText
/// @{

/// Преобразует текст в XML-код.
/// @details Символы, запрещенные спецификацией XML 1.0, удаляются.
Stroka EncodeXML(const char* str, int qEncodeEntities = 1);

/// Преобразует текст в XML-код, в котором могут присутствовать только цифровые сущности.
/// @details Cимволы, запрещенные спецификацией XML 1.0, не удаляются.
Stroka EncodeXMLString(const char* str);

/// @}
