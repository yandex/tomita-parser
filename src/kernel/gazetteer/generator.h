#pragma once

#include "builtin.h"
#include "gazetteer.h"

#include <kernel/gazetteer/base.pb.h>

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/descriptor.h>

#include <util/charset/doccodes.h>
#include <util/generic/stroka.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>

class TGztOptions;

namespace NGzt {

// from syntax.proto
class TGztFileDescriptorProto;

class TGztGenerator {
public:
    // When @parentGzt is supplied, required proto's definitions (import "*.proto";)
    // will be taken from @parentGzt's descriptor pool instead of searching them on disk.
    TGztGenerator(const TGazetteer* parentGzt = NULL, const Stroka& encoding = "utf8");

    ~TGztGenerator();

    void AddImport(const Stroka& fileName);
    void AddOptions(const TGztOptions& options);

    // If you have a ready protobuf-generated class TProto it is possible to use
    // its builtin descriptor definition from binary (generated_pool), without doing explicit import.
    template <typename TProto>
    void UseBuiltinDescriptor() {
        BuiltinProto.AddFileSafe<TProto>();
        AddImport(TProto::descriptor()->file()->name());
    }

    void UseAllBuiltinDescriptors() {
        BuiltinProto.Maximize();
    }

    // direct protobuf article, don't add necessary imports into Collection
    void AddArticle(const NProtoBuf::Message& art, const Stroka& encodedTitle = Stroka());

    // Compile all collected so far articles into new ready gazetteer.
    TAutoPtr<TGazetteer> MakeGazetteer() const;




    // key generation helpers for common usage cases

    static TSearchKey* AddKey(NProtoBuf::RepeatedPtrField<TSearchKey>& keys, const Stroka& encodedText,
                              TMorphMatchRule::EType morph = TMorphMatchRule::ALL_FORMS, docLanguage lang = LANG_UNK);

    template <typename TArticleSubType>
    static TSearchKey* AddKey(TArticleSubType& art, const Stroka& encodedKey) {
        // default: key is lemma
        return AddKey(*art.mutable_key(), encodedKey, TMorphMatchRule::ALL_FORMS);
    }

    template <typename TArticleSubType>
    static TSearchKey* AddExactFormKey(TArticleSubType& art, const Stroka& encodedKey) {
        return AddKey(*art.mutable_key(), encodedKey, TMorphMatchRule::EXACT_FORM);
    }

    template <typename TArticleSubType>
    static TSearchKey* AddAllLemmaFormsKey(TArticleSubType& art, const Stroka& encodedKey, docLanguage lang) {
        // lemmer lang must be specified for ALL_LEMMA_FORMS, otherwise russian is used.
        return AddKey(*art.mutable_key(), encodedKey, TMorphMatchRule::ALL_LEMMA_FORMS, lang);
    }

    Stroka DebugString();

private:
    const TGazetteer* ParentGzt;
    NBuiltin::TFileCollection BuiltinProto;

    THolder<TGztFileDescriptorProto> GeneratedFile;
};



}   // namespace NGzt
