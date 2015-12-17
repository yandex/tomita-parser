#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/split_iterator.h>
#include <kernel/gazetteer/common/serialize.h>
#include <FactExtract/Parser/common/utilit.h>

#include "grammaritem.h"


bool SPostfixStore::operator ==(const SPostfixStore& rPostfixStore) const
{
    return m_hom == rPostfixStore.m_hom
        && m_nohomonyms == rPostfixStore.m_nohomonyms
        && m_hreg == rPostfixStore.m_hreg
        && m_hreg1 == rPostfixStore.m_hreg1
        && m_hreg2 == rPostfixStore.m_hreg2
        && m_dict == rPostfixStore.m_dict
        && m_lat == rPostfixStore.m_lat
        && m_org_morph_dict == rPostfixStore.m_org_morph_dict
        && m_geo_morph_dict == rPostfixStore.m_geo_morph_dict
        && m_name_morph_dict == rPostfixStore.m_name_morph_dict
        && m_lreg == rPostfixStore.m_lreg
        && m_fw == rPostfixStore.m_fw
        && m_is_multiword == rPostfixStore.m_is_multiword
        && m_exclude_from_normalization == rPostfixStore.m_exclude_from_normalization
        && m_is_none_kwtype == rPostfixStore.m_is_none_kwtype
        && m_dict_kwtype == rPostfixStore.m_dict_kwtype
        && m_dict_not_kwtype == rPostfixStore.m_dict_not_kwtype
        && m_WordRegExp == rPostfixStore.m_WordRegExp;
};

bool SPostfixStore::operator <(const SPostfixStore& rPostfixStore) const
{
    if (m_hom != rPostfixStore.m_hom)
        return m_hom < rPostfixStore.m_hom;

    if (m_nohomonyms != rPostfixStore.m_nohomonyms)
        return m_nohomonyms < rPostfixStore.m_nohomonyms;

    if (m_hreg != rPostfixStore.m_hreg)
        return m_hreg < rPostfixStore.m_hreg;

    if (m_hreg1 != rPostfixStore.m_hreg1)
        return m_hreg1 < rPostfixStore.m_hreg1;

    if (m_hreg2 != rPostfixStore.m_hreg2)
        return m_hreg2 < rPostfixStore.m_hreg2;

    if (m_dict != rPostfixStore.m_dict)
        return m_dict < rPostfixStore.m_dict;

    if (m_lat != rPostfixStore.m_lat)
        return m_lat < rPostfixStore.m_lat;

    if (m_org_morph_dict != rPostfixStore.m_org_morph_dict)
        return m_org_morph_dict < rPostfixStore.m_org_morph_dict;

    if (m_geo_morph_dict != rPostfixStore.m_geo_morph_dict)
        return m_geo_morph_dict < rPostfixStore.m_geo_morph_dict;

    if (m_name_morph_dict != rPostfixStore.m_name_morph_dict)
        return m_name_morph_dict < rPostfixStore.m_name_morph_dict;

    if (m_lreg != rPostfixStore.m_lreg)
        return m_lreg < rPostfixStore.m_lreg;

    if (m_fw != rPostfixStore.m_fw)
        return m_fw < rPostfixStore.m_fw;

    if (m_is_multiword != rPostfixStore.m_is_multiword)
        return m_is_multiword < rPostfixStore.m_is_multiword;

    if (m_exclude_from_normalization != rPostfixStore.m_exclude_from_normalization)
        return m_exclude_from_normalization < rPostfixStore.m_exclude_from_normalization;

    if (m_is_none_kwtype != rPostfixStore.m_is_none_kwtype)
        return m_is_none_kwtype < rPostfixStore.m_is_none_kwtype;

    if (!(m_dict_kwtype == rPostfixStore.m_dict_kwtype))
        return m_dict_kwtype < rPostfixStore.m_dict_kwtype;

    if (!(m_dict_not_kwtype == rPostfixStore.m_dict_not_kwtype))
        return m_dict_not_kwtype < rPostfixStore.m_dict_not_kwtype;

    return m_WordRegExp < rPostfixStore.m_WordRegExp;

};

void SPostfixStore::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_hom);
    ::Save(buffer, m_nohomonyms);
    ::Save(buffer, m_hreg);
    ::Save(buffer, m_hreg1);
    ::Save(buffer, m_hreg2);
    ::Save(buffer, m_dict);
    ::Save(buffer, m_lat);
    ::Save(buffer, m_org_morph_dict);
    ::Save(buffer, m_geo_morph_dict);
    ::Save(buffer, m_name_morph_dict);
    ::Save(buffer, m_name_morph_dict);  // ?
    ::Save(buffer, m_fw);
    ::Save(buffer, m_is_multiword);
    ::Save(buffer, m_exclude_from_normalization);

    ::Save(buffer, m_is_none_kwtype);

    ::Save(buffer, m_dict_kwtype);
    ::Save(buffer, m_dict_not_kwtype);
    ::Save(buffer, m_dict_label);
    ::Save(buffer, m_dict_not_label);
    ::Save(buffer, m_WordRegExp);
}

void SPostfixStore::Load(TInputStream* buffer)
{
    ::Load(buffer, m_hom);
    ::Load(buffer, m_nohomonyms);
    ::Load(buffer, m_hreg);
    ::Load(buffer, m_hreg1);
    ::Load(buffer, m_hreg2);
    ::Load(buffer, m_dict);
    ::Load(buffer, m_lat);
    ::Load(buffer, m_org_morph_dict);
    ::Load(buffer, m_geo_morph_dict);
    ::Load(buffer, m_name_morph_dict);
    ::Load(buffer, m_name_morph_dict);  // ?
    ::Load(buffer, m_fw);
    ::Load(buffer, m_is_multiword);
    ::Load(buffer, m_exclude_from_normalization);

    ::Load(buffer, m_is_none_kwtype);

    ::Load(buffer, m_dict_kwtype);
    ::Load(buffer, m_dict_not_kwtype);
    ::Load(buffer, m_dict_label);
    ::Load(buffer, m_dict_not_label);
    ::Load(buffer, m_WordRegExp);
}

/*--------------------------------class CGrammarItem-----------------------------------------------*/
CGrammarItem::CGrammarItem()
{
    m_bGrammarRoot = true;
    m_bCanHaveManyHomonyms = true;
    m_bSynMain = false;
    m_ItemGrammems.Reset();
    m_NegativeItemGrammems.Reset();
    m_Type = siUnknown;
    m_Source = "<empty>";
};

bool IsAttributeName(const Stroka& S)
{
    if (S == "hom") return true;
    if (S == "type") return true;
    if (S == "SemClass") return true;
    if (S == "nb") return true;
    if (S == "case") return true;
    if (S == "name") return true;
    if (S == "grm") return true;
    if (S == "syn") return true;
    if (S == "register") return true;
    return false;
};

bool CGrammarItem::operator <(const CGrammarItem& _X1) const
{
    if (m_ItemStrId != _X1.m_ItemStrId)
        return m_ItemStrId < _X1.m_ItemStrId;

    if (m_Type != _X1.m_Type)
        return m_Type < _X1.m_Type;

    if (m_ItemGrammems != _X1.m_ItemGrammems)
        return m_ItemGrammems < _X1.m_ItemGrammems;

    if (m_NegativeItemGrammems != _X1.m_NegativeItemGrammems)
        return m_NegativeItemGrammems < _X1.m_NegativeItemGrammems;

    if (m_bCanHaveManyHomonyms != _X1.m_bCanHaveManyHomonyms)
        return m_bCanHaveManyHomonyms < _X1.m_bCanHaveManyHomonyms;

    if (m_vReplaceInterp != _X1.m_vReplaceInterp)
        return m_vReplaceInterp < _X1.m_vReplaceInterp;

    if (!(m_PostfixStore == _X1.m_PostfixStore))
        return m_PostfixStore < _X1.m_PostfixStore;

    return m_Attributes < _X1.m_Attributes;
};

bool CGrammarItem::operator ==(const CGrammarItem& _X1) const
{
    bool b =     m_ItemStrId == _X1.m_ItemStrId
            &&  m_Type == _X1.m_Type
            &&  m_Attributes == _X1.m_Attributes
            &&  m_ItemGrammems == _X1.m_ItemGrammems
            &&  m_NegativeItemGrammems == _X1.m_NegativeItemGrammems
            &&  m_bCanHaveManyHomonyms == _X1.m_bCanHaveManyHomonyms
            &&  m_vReplaceInterp == _X1.m_vReplaceInterp
            &&  m_PostfixStore == _X1.m_PostfixStore;
    return b;
};

inline Stroka TypeToString(SimpleItemEnum t)
{
    switch (t) {
    case siRomanNumber:
        return "ROMAN";
    case siAnyWord:
        return "ANYWORD";
    case siNumber:
        return "NUMBER";
    case siMorphologicalyUnknown:
        return "LEX-UNK";
    case siMorphPattern:
        return "MORPH";
    default:
        break;
    }
    return  "";
}

Stroka CGrammarItem::GetDumpString() const
{
    assert(m_Type != siUnknown);

    if (m_Type == siString)
        return Substitute("'$0'",m_ItemStrId);

    Stroka Attributes;
    for (ymap<Stroka, Stroka>::const_iterator it = m_Attributes.begin(); it != m_Attributes.end(); it++)
        Attributes += Substitute("$0=$1 ", it->first,it->second);

    if (!m_GrmAttribute.empty())
        Attributes += Substitute("grm=\"$0\" ", m_GrmAttribute);

    if (!m_bCanHaveManyHomonyms)
        Attributes += "hom=\"no\" ";

    if (m_bSynMain)
        Attributes += "syn=\"root\" ";

    Attributes = StripString(Attributes);

    Stroka Meta;
    if (m_Type == siMeta)
        Meta = m_ItemStrId;
    else
        Meta = TypeToString(m_Type);

    return Substitute("[$0 $1]", Meta, Attributes);
};

ePostfix CGrammarItem::HasPostfix(ePrefix e_Val) const
{
    switch (e_Val) {
        case HOM:
            return m_PostfixStore.m_hom;
        case HREG:
            return m_PostfixStore.m_hreg;
        case HREG1:
            return m_PostfixStore.m_hreg1;
        case HREG2:
            return m_PostfixStore.m_hreg2;
        case DICT:
            return m_PostfixStore.m_dict;
        case LAT:
            return m_PostfixStore.m_lat;
        case ORGAN:
            return m_PostfixStore.m_org_morph_dict;
        case GEO:
            return m_PostfixStore.m_geo_morph_dict;
        case NAME_DIC:
            return m_PostfixStore.m_name_morph_dict;
        case LREG:
            return m_PostfixStore.m_lreg;
        case FIRST_LET_IN_SENT:
            return m_PostfixStore.m_fw;
        case IS_MULTIWORD:
            return m_PostfixStore.m_is_multiword;
        case NO_HOMS:
            return m_PostfixStore.m_nohomonyms;
        case EXCLUDE_FROM_NORM:
            return m_PostfixStore.m_exclude_from_normalization;
        default:
            break;
    }
    return PostfixCount;
}

bool CGrammarItem::HasPostfix(ymap<Stroka,Stroka>::const_iterator& it, ePrefix e_ValPref, ePostfix e_ValPostf) const
{
    return (it->first == AlphabetPrefix[e_ValPref] && it->second == AttributeValues[e_ValPostf]);
};

bool CGrammarItem::HasPostfixKWType(ePrefix e_ValPref, Stroka& sArt) const
{
    sArt.clear();
    if (0 == m_Attributes.size()) return false;
    ymap<Stroka,Stroka>::const_iterator it = m_Attributes.find(AlphabetPrefix[e_ValPref]);
    if (it == m_Attributes.end()) return false;
    sArt = it->second;
    return true;
};

const SArtPointer& CGrammarItem::GetPostfixKWType(ePrefix e_ValPref) const
{
    switch (e_ValPref) {
        case DICT_KWTYPE:
            return m_PostfixStore.m_dict_kwtype;
        case DICT_NOT_KWTYPE:
            return m_PostfixStore.m_dict_not_kwtype;
        case DICT_LABEL:
            return m_PostfixStore.m_dict_label;
        case DICT_NOT_LABEL:
            return m_PostfixStore.m_dict_not_label;
        default:
            break;
    }
    ythrow yexception() << "Argument is wrong in CGrammarItem::GetPostfixKWType(ePrefix e_ValPref).";
}

const CWordFormRegExp* CGrammarItem::GetRegExpPostfix(ePrefix e_ValPref) const
{
    switch (e_ValPref) {
    case NOT_REG_EXP:
        if (!m_PostfixStore.m_WordRegExp.IsEmpty() && m_PostfixStore.m_WordRegExp.IsNegative)
            return &m_PostfixStore.m_WordRegExp;
        else
            return NULL;
    case REG_EXP:
        if (!m_PostfixStore.m_WordRegExp.IsEmpty() && !m_PostfixStore.m_WordRegExp.IsNegative)
            return &m_PostfixStore.m_WordRegExp;
        else
            return NULL;
    default:
        break;
    }
    ythrow yexception() << "Argument is wrong in CGrammarItem::GetPostfixKWType(ePrefix e_ValPref).";
    //return NULL;
}

Stroka CGrammarItem::HasPostfix(Stroka s_Val) const
{
    ymap<Stroka,Stroka>::const_iterator it = m_Attributes.find(s_Val);
    if (it == m_Attributes.end())  return "";
    return it->second;
};

void CGrammarItem::LoadReplacementInterp()
{
    ymap<Stroka,Stroka>::iterator it = m_Attributes.find(AlphabetPrefix[REPLACE_INTERP]);
    if (it == m_Attributes.end()) return;
    TDelimitersSplit split(it->second.c_str(), it->second.length(), "\\");
    TDelimitersSplit::TIterator jt = split.Iterator();
    while (!jt.Eof()) {
        try {
            m_vReplaceInterp.push_back(::FromString<i64>(jt.NextStroka()));
        } catch (...) {
            printf ("Warning: can't convert replacement grammar interpretation for the item %s.\n", m_ItemStrId.c_str());
        }
    }

    m_Attributes.erase(it);
}

void CGrammarItem::LoadPostfixStore()
{
    for (int i = 0; i < PrefixCount; i++)
        LoadPostfixStore((ePrefix)i);
}

void CGrammarItem::LoadPostfixStore(ePrefix Pref)
{
    ymap<Stroka,Stroka>::iterator it = m_Attributes.find(AlphabetPrefix[Pref]);
    if (it == m_Attributes.end()) return;

    switch (Pref) {
        case HOM: {
                    m_PostfixStore.m_hom = Str2Postfix(it->second);
                    break;
                  }
        case NO_HOMS: {
                    m_PostfixStore.m_nohomonyms = Str2Postfix(it->second);
                    break;
                  }
        case EXCLUDE_FROM_NORM: {
                    m_PostfixStore.m_exclude_from_normalization = Str2Postfix(it->second);
                    break;
                  }
        case HREG: {
                    m_PostfixStore.m_hreg = Str2Postfix(it->second);
                    break;
                  }
        case HREG1: {
                    m_PostfixStore.m_hreg1 = Str2Postfix(it->second);
                    break;
                  }
        case HREG2: {
                    m_PostfixStore.m_hreg2 = Str2Postfix(it->second);
                    break;
                  }
        case FIRST_LET_IN_SENT: {
                    m_PostfixStore.m_fw = Str2Postfix(it->second);
                    break;
                  }
        case IS_MULTIWORD: {
                    m_PostfixStore.m_is_multiword = Str2Postfix(it->second);
                    break;
                  }
        case DICT: {
                    m_PostfixStore.m_dict = Str2Postfix(it->second);
                    break;
                  }
        case LAT: {
                    m_PostfixStore.m_lat = Str2Postfix(it->second);
                    break;
                  }
        case ORGAN: {
                    m_PostfixStore.m_org_morph_dict = Str2Postfix(it->second);
                    break;
                  }
        case GEO: {
                    m_PostfixStore.m_geo_morph_dict = Str2Postfix(it->second);
                    break;
                  }
        case NAME_DIC: {
                    m_PostfixStore.m_name_morph_dict = Str2Postfix(it->second);
                    break;
                  }
        case LREG: {
                    m_PostfixStore.m_lreg = Str2Postfix(it->second);
                    break;
                  }
        case DICT_KWTYPE: {
                    // special case <kwtype=none>
                    if (IsNoneKWType(it->second))
                        m_PostfixStore.m_is_none_kwtype = true;
                    else
                        m_PostfixStore.m_dict_kwtype.PutType(GetKWTypePool().FindMessageTypeByName(it->second), NStr::DecodeTomita(it->second));
                    break;
                  }
        case DICT_LABEL: {
                    m_PostfixStore.m_dict_label.PutType(GetKWTypePool().FindMessageTypeByName(it->second), NStr::DecodeTomita(it->second));
                    break;
                  }
        case DICT_NOT_KWTYPE: {
                    m_PostfixStore.m_dict_not_kwtype.PutType(GetKWTypePool().FindMessageTypeByName(it->second), NStr::DecodeTomita(it->second));
                    break;
                  }
        case DICT_NOT_LABEL: {
                    m_PostfixStore.m_dict_not_label.PutType(GetKWTypePool().FindMessageTypeByName(it->second), NStr::DecodeTomita(it->second));
                    break;
                  }
        case NOT_REG_EXP: {
                    m_PostfixStore.m_WordRegExp.IsNegative = true;
                    m_PostfixStore.m_WordRegExp.SetRegexUtf8(it->second);
                    break;
                  }
        case REG_EXP: {
                    m_PostfixStore.m_WordRegExp.IsNegative = false;
                    m_PostfixStore.m_WordRegExp.SetRegexUtf8(it->second);
                    break;
                  }

        default: return;
    }

    m_Attributes.erase(it);
}

void CGrammarItem::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_Type);
    ::Save(buffer, m_ItemStrId);
    ::Save(buffer, m_Lemma);
    ::Save(buffer, m_Attributes);
    ::Save(buffer, m_Source);
    ::Save(buffer, m_ItemGrammems);
    ::Save(buffer, m_NegativeItemGrammems);
    ::Save(buffer, m_GrmAttribute);
    ::Save(buffer, m_bGrammarRoot);
    ::Save(buffer, m_bSynMain);
    ::Save(buffer, m_bCanHaveManyHomonyms);
    ::Save(buffer, m_vReplaceInterp);
    ::Save(buffer, m_PostfixStore);

}

void CGrammarItem::Load(TInputStream* buffer)
{
    ::Load(buffer, m_Type);
    ::Load(buffer, m_ItemStrId);
    ::Load(buffer, m_Lemma);
    ::Load(buffer, m_Attributes);
    ::Load(buffer, m_Source);
    ::Load(buffer, m_ItemGrammems);
    ::Load(buffer, m_NegativeItemGrammems);
    ::Load(buffer, m_GrmAttribute);
    ::Load(buffer, m_bGrammarRoot);
    ::Load(buffer, m_bSynMain);
    ::Load(buffer, m_bCanHaveManyHomonyms);
    ::Load(buffer, m_vReplaceInterp);
    ::Load(buffer, m_PostfixStore);

    LoadReplacementInterp();
    LoadPostfixStore();
}
/*
void CGrammarItem::SaveVector(TOutputStream* buffer, const yvector<CGrammarItem>& items)    // static
{
    ::SaveProtectedSize(buffer, (ui32)items.size());
    for (size_t i = 0; i < items.size(); ++i)
        items[i].Save(buffer);
}

void CGrammarItem::LoadVector(TInputStream* buffer, yvector<CGrammarItem>& items)  // static
{
    ui32 size = ::LoadProtectedSize(buffer);
    items.resize(size);
    for (size_t i = 0; i < size; ++i)
        items[i].Load(buffer);
}
*/
