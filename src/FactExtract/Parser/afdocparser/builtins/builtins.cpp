#include "builtins.h"

#include <util/generic/singleton.h>

#include "articles_base.pb.h"
#include "facttypes_base.pb.h"
#include "kwtypes_base.pb.h"

class TTomitaBuiltinProtos: public NGzt::NBuiltin::TFileCollection {
public:
    TTomitaBuiltinProtos() {

        AddFileSafe<TAuxDicArticle>(
            "FactExtract/Parser/afdocparser/builtins/articles_base.proto",
            "articles_base.proto"
        );

        AddFileSafe<NFactType::TFact>(
            "FactExtract/Parser/afdocparser/builtins/facttypes_base.proto",
            "facttypes_base.proto"
        );

        AddFileSafe<fio>(
            "FactExtract/Parser/afdocparser/builtins/kwtypes_base.proto",
            "kwtypes_base.proto"
        );
    }
};

const NGzt::NBuiltin::TFileCollection& TomitaBuiltinProtos() {
    return Default<TTomitaBuiltinProtos>();
}
