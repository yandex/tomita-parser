#include "afdocbase.h"

static const wchar16 HTML_TAG_OPEN = '<';
static const wchar16 HTML_TAG_CLOSE = '>';

//////////////////////////////////////////////////////////////////////////////
// tags skipped entirely with all contents
// * == new i.e. not based on old version [CPlainText.cpp]

static const char *tags_NoContent[] =
{
//  old - Теги разметки
//  "COMMENT",  // ???
    "STYLE",
//  old - Редкие
    "APPLET",
    "SCRIPT",
    "SELECT",   // in FORM
    "TEXTAREA", // in FORM
//  old - Дополнительные
    "BANNER",
    "FRAMESET",
    "OBJECT",
// new
//  "FORM",     //*
};

bool CDocBase::isTagNoContent(const Stroka &tag) const
{
    stroka tag_ci = tag;
    for (size_t i=0; i<sizeof(tags_NoContent)/sizeof(tags_NoContent[0]); i++) {
        if (tag_ci == tags_NoContent[i])
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// tags producing ParaBreak

static const char *tags_ParaBreak[] =
{
    "P",
    "/P",
    "H1",
    "H2",
    "H3",
    "H4",
    "H5",
    "H6",
    "TABLE",
    "/TABLE",   //*
    "HR",       //*
    "DL",       //*
};

bool CDocBase::isTagParaBreak(const Stroka &tag) const
{
    stroka tag_ci = tag;
    for (size_t i=0; i<sizeof(tags_ParaBreak)/sizeof(tags_ParaBreak[0]); i++)
        if (tag_ci == tags_ParaBreak[i])
            return true;

    return false;
}

//////////////////////////////////////////////////////////////////////////////
// tags producing SentBreak

static const char *tags_SentBreak[] =
{
    "/H1",  //*
    "/H2",  //*
    "/H3",  //*
    "/H4",  //*
    "/H5",  //*
    "/H6",  //*

    "TR",   //*
    "TH",   //*
    "TD",
    "LI",
    "DD",   //*
    "DT",   //*
};

bool CDocBase::isTagSentBreak(const Stroka &tag) const
{
    stroka tag_ci = tag;
    for (size_t i=0; i<sizeof(tags_SentBreak)/sizeof(tags_SentBreak[0]); i++) {
        if (tag_ci == tags_SentBreak[i])
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// proceedHtmlHead

int CDocBase::proceedHtmlHead()
{
    int beg, end;
    if (findHtmlTag(0, "TITLE", beg, end)) {
        int beg2, end2;
        if (findHtmlTag(end, "/TITLE", beg2, end2))
            proceedHtmlTitle(end, beg2);
        else
            PrintError("</TITLE> not found");
    }

    if (!findHtmlTag(0, "BODY", beg, end)) {
        PrintError("<BODY> not found");
        return 0;
    }

    for (int i = 0; i < end; ++i)
        *(m_Source.begin() + i) = CTEXT_REM_SYMBOL;

    m_iTagsSize += end;
    return end;
}

//////////////////////////////////////////////////////////////////////////////
// proceedHtmlTitle

void CDocBase::proceedHtmlTitle(int beg, int end)
{
    m_pText->setHtmlSource(true);
    m_pText->Proceed(~m_Source, beg, end - beg);
    m_pText->setHtmlSource(false);
}

//////////////////////////////////////////////////////////////////////////////
// findHtmlTag

bool CDocBase::findHtmlTag(int from, int &beg, int &end, Stroka &tag)
{
    beg = m_Source.find(HTML_TAG_OPEN, from);
    if ((size_t)beg == Wtroka::npos)
        return false;
    int len = m_Source.size();
    int pt1 = beg + 1;
    while (pt1 < len && isSpace(m_Source[pt1]))
        pt1++;
    int pt2 = pt1;
    while (pt2 < len && m_Source[pt2] != HTML_TAG_CLOSE && !isSpace(m_Source[pt2]))
        pt2++;

    tag = WideToChar(m_Source.substr(pt1,pt2-pt1));
    end = m_Source.find(HTML_TAG_CLOSE, pt2);
    if ((size_t)end == Wtroka::npos)
        end = len;
    else
        end++;
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// findHtmlTag

bool CDocBase::findHtmlTag(int from,const Stroka &tag,int &beg,int &end)
{
    int len = m_Source.size();
    for (beg = from; beg < len;) {
        beg = m_Source.find(HTML_TAG_OPEN, beg);
        if ((size_t)beg == Wtroka::npos)
            return false;
        int pt1 = beg+1;
        while (pt1 < len && isSpace(m_Source[pt1]))
            pt1++;
        int pt2 = pt1;
        while (pt2 < len && m_Source[pt2] != HTML_TAG_CLOSE && !isSpace(m_Source[pt2]))
            pt2++;
        Stroka tmp = WideToChar(m_Source.substr(pt1, pt2 - pt1));
        if (stroka(tmp) != stroka(tag)) {
            beg++;
            continue;
        }
        end = m_Source.find(HTML_TAG_CLOSE,pt2);
        if ((size_t)end == Wtroka::npos)
            end = len;
        else
            end++;
        return true;
    }
    return false;
}

void CDocBase::removeHtmlTag(int beg, int end, int brk)
{
    wchar16* src = m_Source.begin();
    for (int i = beg; i < end; ++i)
        src[i] = CTEXT_REM_SYMBOL;

    m_iTagsSize += end - beg;

    if (end - beg < 3)
        return;

    if (brk == ParaBreak) {
        src[beg] = CTEXT_CTR_SYMBOL;
        src[beg + 1] = 'P';
        src[beg + 2] = CTEXT_CTR_SYMBOL;
    } else if (brk == SentBreak) {
        src[beg] = CTEXT_CTR_SYMBOL;
        src[beg + 1] = 'S';
        src[beg + 2] = CTEXT_CTR_SYMBOL;
    } else if (brk == WordBreak) {
        src[beg] = CTEXT_CTR_SYMBOL;
        src[beg + 1] = 'W';
        src[beg + 2] = CTEXT_CTR_SYMBOL;
    }
}

bool CDocBase::refineHtmlStr(int beg, int end)
{
    bool ret = false;
    for (int i = beg; i < end; ++i) {
        if (m_Source[i] == '\n')
            m_Source.begin()[i] = ' ';
        if (!ret && !::IsSpace(m_Source[i]))
            ret = true;
    }
    return ret;
}

static inline bool IsBr(const Stroka& tag)
{
    stroka tag_ci = tag;
    return tag_ci == "BR" || tag_ci == "BR/";
}

void CDocBase::proceedHtml()
{
    int pos = proceedHtmlHead();

    int beg,end;
    Stroka tag;
    bool bBR2 = false;
    while (findHtmlTag(pos,beg,end,tag)) {
        bool bSym = refineHtmlStr(pos,beg);

        if (!IsBr(tag))
            bBR2 = false;

        if (isTagNoContent(tag)) {
            int beg2,end2;
            Stroka tagend = "/";
            tagend += tag;
            if (!findHtmlTag(pos, tagend, beg2, end2)) {
                PrintError("end of <no content> tag not found");
                end2 = m_Source.size();
            }
            removeHtmlTag(beg,end2,NoBreak);
            pos = end2;
            continue;
        }

        if (isTagParaBreak(tag))
            removeHtmlTag(beg,end,ParaBreak);

        else if (isTagSentBreak(tag))
            removeHtmlTag(beg,end,SentBreak);

        else if (IsBr(tag)) {
            if (bBR2 && !bSym) {
                removeHtmlTag(beg,end,ParaBreak);
                bBR2 = false;
            } else {
                removeHtmlTag(beg,end,WordBreak);
                bBR2 = true;
            }
        } else
            removeHtmlTag(beg,end,NoBreak);

        pos = end;
    }

    m_pText->setHtmlSource(true);
    proceedText();
    m_pText->setHtmlSource(false);
}

