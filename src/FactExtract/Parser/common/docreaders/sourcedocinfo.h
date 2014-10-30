#pragma once

#include <util/generic/stroka.h>

const char g_strOpenTagList[] = "<documents>";
const char g_strCloseTagList[] = "</documents>";
const char g_strDocumentTag[] = "document";
const char g_strFileTag[] = "file";
const char g_strAfDocDescrTag[] = "afdoc";
const char g_strKeywordsTagOld[] = "keywords";
const char g_strKeywordsTag[] = "kw";
const char g_strWordTag[] = "w";
const char g_strAttrTag[] = "attributes";
const char g_strDocDescrTag[] = "d";
const char g_strUrlTag[] = "u";
const char g_strTimeTag[] = "t";

const char g_strFilePattern[] = "file";
const char g_strAttributesPattern[] = "attributes";

/*
    информация о том, откуда (в каком-то определенном смысле) брать документ,
    и еще кое-какая информация
*/
struct SSourceDocInfo
{
    Stroka m_strFilePath;
    Stroka m_strURL;
    Stroka m_strTime;
};
