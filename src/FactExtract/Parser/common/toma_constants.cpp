#include "toma_constants.h"

#include <util/system/guard.h>
#include <contrib/libs/protobuf/descriptor.pb.h>
#include <library/lemmer/dictlib/terminals.h>

EFactAlgorithm Str2FactAlgorithm(const char* str)
{
    int i = IsInList_i((const char*)FactAlg,eFactAlgCount, str);
    return ((i == -1) ? eFactAlgCount : (EFactAlgorithm)i);
}

const char* Str2FactAlgorithm(EFactAlgorithm alg)
{
    YASSERT(alg < eFactAlgCount);
    return FactAlg[alg];
}

eTerminals TDefaultTerminalDictionary::FindTerminal(const Stroka& name) const {
    // Note that an implementation of TerminalByName is not linked into FactExtract/Parser/common library.
    // A proper cpp (either terminal.cpp or terminal_fdo.cpp from library/lemmer/dictlib) should be included
    // in SRCS() in CMakeLists.txt for corresponding high-level executable or library.
    const NSpike::TTerminal* terminal = NSpike::TerminalByName(name);
    if (terminal == NULL || terminal->Code < 0)
        return TermCount;
    else
        return static_cast<eTerminals>(terminal->Code);
}

bool TDefaultTerminalDictionary::IsTerminal(const Stroka& name) const {
    const NSpike::TTerminal* terminal = NSpike::TerminalByName(name);
    return terminal != NULL && terminal->Code >= 0;

}

TFactexTerminalDictionary::TFactexTerminalDictionary() {
    for (size_t i = 0; i < TermCount; ++i)
        Index[TerminalValues[i]] = static_cast<eTerminals>(i);
}

eTerminals TFactexTerminalDictionary::FindTerminal(const Stroka& name) const {
    TIndex::const_iterator it = Index.find(name);
    return it != Index.end() ? it->second : TermCount;
}

bool TFactexTerminalDictionary::IsTerminal(const Stroka& name) const {
    return Index.find(name) != Index.end();
}

bool SkipKnownPrefix(TStringBuf& keytext)
{
    return SkipTomitaPrefix(keytext) ||
           SkipAlgPrefix(keytext);
}

bool SkipPrefix(TStringBuf& keytext, const TStringBuf& prefix)
{
    if (keytext.has_prefix(prefix)) {
        keytext.Skip(prefix.size());
        return true;
    } else
        return false;
}

bool SkipTomitaPrefix(TStringBuf& keytext)
{
    return SkipPrefix(keytext, GetTomitaPrefix());
}

bool SkipAlgPrefix(TStringBuf& keytext)
{
    return SkipPrefix(keytext, GetAlgPrefix());
}

TFixedKWTypeNames::TFixedKWTypeNames()
{
    Add(Number, "number");
    Add(Date, "date");
    Add(DateChain, "date_chain");

    Add(CompanyAbbrev, "abbrev");
    Add(CompanyInQuotes, "company_in_quotes");
    Add(CompanyChain, "company_chain");
    Add(StockCompany, "stock_company");

    Add(Fio, "fio");
    Add(FioWithoutSurname, "fio_without_surname");
    Add(FioChain, "fio_chain");
    Add(FdoChain, "fdo_chain");
    Add(TlVariant, "tl_variant");

    Add(Parenthesis, "parenthesis");
    Add(CommunicVerb, "verb_communic");

    Add(Geo, "geo");
    //Add(GeoContinent, "geo_continent");
    //Add(GeoCountry, "geo_country");
    Add(GeoAdm, "geo_adm");
    //Add(GeoDistrict, "geo_district");
    Add(GeoCity, "geo_city");
    Add(GeoHauptstadt, "geo_hauptstadt");
    Add(GeoSmallCity, "geo_small_city");
    Add(GeoVill, "geo_vill");

    //Add(GeoAdj, "geo_adj");
    //Add(National, "national");

    Add(AntiStatus, "antistatus_word");
    Add(LightStatus, "light_status_word");
}

void TFixedKWTypeNames::Add(Stroka& field, const char* name)
{
    field = name;
    List.push_back(field);
}

const TKWTypePool::TDescriptor* TKWTypePool::FindMessageTypeByName(const Stroka& name) const
{
    yhash<Stroka, const TDescriptor*>::const_iterator descr = mRegisteredTypes.find(name);
    if (descr != mRegisteredTypes.end())
        return descr->second;
    else
        return NULL;
}

const TKWTypePool::TDescriptor* TKWTypePool::RequireMessageType(const Stroka& name) const
{
    const TDescriptor* res = FindMessageTypeByName(name);
    if (res == NULL)
        ythrow yexception() << "KW-type \"" << name << "\" is not registered.";
    return res;
}

void TKWTypePool::SaveKWType(TOutputStream* buffer, const google::protobuf::Descriptor* type) const
{
    yhash<const ::google::protobuf::Descriptor*, ui32>::const_iterator res = mSaveIndex.find(type);
    if (res != mSaveIndex.end())
        ::Save(buffer, res->second);
    else
        ythrow yexception() << "Unknown kw-type " << type->full_name() << " is serialized, possibly a logic error in program.";

}

const google::protobuf::Descriptor* TKWTypePool::LoadKWType(TInputStream* buffer) const
{
    ui32 index;
    ::Load(buffer, index);
    if (index >= mLoadIndex.size())
        ythrow yexception() << "Could not load kw-type: wrong index (" << index << ").";
    return mLoadIndex[index];
}

void TKWTypePool::Save(TOutputStream* buffer) const
{
    yvector<Stroka> names(mSaveIndex.size());
    for (yhash<const TDescriptor*, ui32>::const_iterator it = mSaveIndex.begin(); it != mSaveIndex.end(); ++it) {
        const TDescriptor* descriptor = it->first;
        names[it->second] = (descriptor != NULL) ? descriptor->full_name() : "";
    }
    ::Save(buffer, names);
}

void TKWTypePool::Load(TInputStream* buffer)
{
    yvector<Stroka> names;
    ::Load(buffer, names);

    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    // rebuild mLoadIndex
    mLoadIndex.clear();
    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i].empty()) {
            static const TDescriptor* null_descriptor = NULL;
            mLoadIndex.push_back(null_descriptor);
            continue;
        }

        // all loaded names should be among registred descriptors
        yhash<Stroka, const TDescriptor*>::const_iterator descr = mRegisteredTypes.find(names[i]);
        if (descr != mRegisteredTypes.end())
            mLoadIndex.push_back(descr->second);
        else
            ythrow yexception() << "Unknown kw-type " << names[i] << " is found. "
                                << "You should re-compile your grammar files.";
    }
}

const google::protobuf::Descriptor* TFakeProtoPool::FindMessageTypeByName(const Stroka& name)
{
    // first, try search in generated pool
    const google::protobuf::Descriptor* res = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(name);
    if (res == NULL) {
        TGuard<TMutex> lock(Mutex);
        res = Pool.FindMessageTypeByName(name);
        if (res == NULL) {
            // prepare fake proto-file with single descriptor and add it to @Pool
            google::protobuf::FileDescriptorProto file_proto;
            file_proto.set_name(name);      // set proto-file name same as message-type being added
            google::protobuf::DescriptorProto* descr_proto = file_proto.add_message_type();
            descr_proto->set_name(name);

            const google::protobuf::FileDescriptor* fake_file = Pool.BuildFile(file_proto);
            YASSERT(fake_file != NULL && fake_file->message_type_count() == 1);
            res = fake_file->message_type(0);
        }
    }
    return res;
}

//------------------------class SArtPointer ---------------------------------------------

bool SArtPointer::operator<(const SArtPointer& p) const
{
    if (m_KWType != p.m_KWType)
        return m_KWType < p.m_KWType;
    return m_strArt < p.m_strArt;
}

bool SArtPointer::operator==(const SArtPointer& _X) const
{
    if (m_KWType != _X.m_KWType)
        return false;
    if (m_KWType != NULL)
        return true;
    return m_strArt == _X.m_strArt;
}

TKeyWordType SArtPointer::GetKWType() const
{
    return m_KWType;
}

bool SArtPointer::HasKWType() const
{
    return m_KWType != NULL;
}
bool SArtPointer::HasStrType() const
{
    return !m_strArt.empty();
}

const Wtroka& SArtPointer::GetStrType() const
{
    return m_strArt;
}

void SArtPointer::PutKWType(TKeyWordType kw)
{
    if (kw == NULL)
        return;
    m_KWType = kw;
    m_strArt.clear();
}

void SArtPointer::PutStrType(const Wtroka& s)
{
    if (s.empty())
        return;
    m_strArt = s;
    m_KWType = NULL;
}

void SArtPointer::PutType(TKeyWordType kw, const Wtroka& s)
{
    if (kw != NULL)
        PutKWType(kw);
    else
        PutStrType(s);
}

bool SArtPointer::IsValid() const
{
    return m_KWType != NULL || !m_strArt.empty();
}

void SArtPointer::Clear()
{
    m_KWType = NULL;
    m_strArt.clear();
}

void SArtPointer::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_strArt);
    ::Save(buffer, m_KWType);

}

void SArtPointer::Load(TInputStream* buffer)
{
    ::Load(buffer, m_strArt);
    ::Load(buffer, m_KWType);
}
