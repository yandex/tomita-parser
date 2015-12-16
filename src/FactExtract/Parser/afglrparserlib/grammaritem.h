#pragma once

#include <util/system/defaults.h>
#include <util/system/compat.h>
#include <util/generic/vector.h>

#include <FactExtract/Parser/common/serializers.h>
#include "agreement.h"

#include <library/lemmer/dictlib/grammarhelper.h>


enum SimpleItemEnum {siUnknown, siString, siMeta, siRomanNumber, siFileList, /*siThesEntry,*/ siMorphologicalyUnknown, siMorphPattern, siNumber, siAnyWord};

//declare SimpleItemEnum as simple type for auto-serialization
DECLARE_PODTYPE(SimpleItemEnum)

struct SPostfixStore
{
    SPostfixStore()
        : m_hom(PostfixCount), m_nohomonyms(PostfixCount)
        , m_hreg(PostfixCount), m_hreg1(PostfixCount), m_hreg2(PostfixCount)
        , m_dict(PostfixCount)
        , m_lat(PostfixCount)
        , m_org_morph_dict(PostfixCount)
        , m_geo_morph_dict(PostfixCount)
        , m_name_morph_dict(PostfixCount)
        , m_lreg(PostfixCount), m_fw(PostfixCount), m_is_multiword(PostfixCount)
        , m_exclude_from_normalization(PostfixCount)
        , m_is_none_kwtype(false)
    {
    }

    ePostfix m_hom, m_nohomonyms;
    ePostfix m_hreg, m_hreg1, m_hreg2;
    ePostfix m_dict;
    ePostfix m_lat;
    ePostfix m_org_morph_dict;// словарь организаций  из морфологии
    ePostfix m_geo_morph_dict;// словарь географии  из морфологии
    ePostfix m_name_morph_dict;// словарь имен из морфологии
    ePostfix m_lreg, m_fw, m_is_multiword;
    ePostfix m_exclude_from_normalization; //при нормализации не брать поддерево с этой пометой

    bool m_is_none_kwtype;      // special case: <kwtype=none>

    SArtPointer m_dict_kwtype;
    SArtPointer m_dict_not_kwtype; //КС тип с отрицанием
    SArtPointer m_dict_label;
    SArtPointer m_dict_not_label; //label тип с отрицанием

    CWordFormRegExp m_WordRegExp;

    bool operator ==(const SPostfixStore& rPostfixStore) const;
    bool operator <(const SPostfixStore& rPostfixStore) const;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

};

struct CGrammarItem
{
    SimpleItemEnum        m_Type;
    Stroka                m_ItemStrId;
    Wtroka                m_Lemma;

    ymap<Stroka, Stroka> m_Attributes;
    Stroka                m_Source;

    TGramBitSet            m_ItemGrammems;
    TGramBitSet            m_NegativeItemGrammems;
    Stroka            m_GrmAttribute;

    //  m_bGrammarRoot  is valid only in Work grammars
    bool                m_bGrammarRoot;

    //    m_bSynMain is true if this item should be the main in syntax structure
    bool                m_bSynMain;

    // attribute hom="yes"/"no" should have a special processing,  and it should not
    //  be a distinguishing feature of a grammar symbol,  that's why it should be excluded from
    // attributes and mirrored to m_bCanHaveManyHomonyms
    bool                m_bCanHaveManyHomonyms;

    //nim 20.09.04 : for the replacement grammar interpretation a:b
    yvector<i64> m_vReplaceInterp;

    //for speeding-up all the postfixes are translated from map m_Attributes to the SPostfixStore struct
    SPostfixStore    m_PostfixStore;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    //static void SaveVector(TOutputStream* buffer, const yvector<CGrammarItem>& items);
    //static void LoadVector(TInputStream* buffer, yvector<CGrammarItem>& items);

    CGrammarItem();

    Stroka    GetDumpString() const;

    bool operator <(const CGrammarItem& _X1) const;
    bool operator ==(const CGrammarItem& _X1) const;

    ePostfix HasPostfix(ePrefix e_Val) const;
    bool HasPostfix(ymap<Stroka,Stroka>::const_iterator& it, ePrefix e_ValPref, ePostfix e_ValPostf) const;

    bool IsNonePostfixKWType() const
    {
        return m_PostfixStore.m_is_none_kwtype;
    }

    const SArtPointer& GetPostfixKWType(ePrefix e_ValPref) const;
    const CWordFormRegExp* GetRegExpPostfix(ePrefix e_ValPref) const;
    bool HasPostfixKWType(ePrefix e_ValPref, Stroka& sArt) const;
    Stroka HasPostfix(Stroka s_Val) const;
    //eTerminals GetTerminalID(const Stroka& s_Term) const;
    void LoadReplacementInterp();
    void LoadPostfixStore();

protected:
    void LoadPostfixStore(ePrefix Pref);
};
