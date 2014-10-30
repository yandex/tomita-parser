#pragma once

#include <FactExtract/Parser/common/fact-types.h>

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/hash.h>

namespace NGzt {
    class TProtoPool;
}

class TFactTypeHolder {
public:
    typedef fact_type_t TFactType;

    size_t Size() const {
        return FactTypes.size();
    }

    const TFactType& operator[](size_t i) const {
        return FactTypes[i];
    }

    const TFactType* Find(const TStringBuf& factTypeName) const {
        TIndex::const_iterator it = Index.find(factTypeName);
        return it != Index.end() ? &FactTypes[it->second] : NULL;
    }

    bool Has(const TStringBuf& factTypeName) const {
        return Find(factTypeName) != NULL;
    }

    bool HasFactField(const TStringBuf& factTypeName, const TStringBuf& fieldName) const {
        const TFactType* factType = Find(factTypeName);
        return factType != NULL && factType->get_field(fieldName) != NULL;
    }

    void Add(const TFactType& factType);

    void Add(const yvector<TFactType>& factTypes) {
        for (size_t i = 0; i < factTypes.size(); ++i)
            Add(factTypes[i]);
    }

    // convert all descriptors from @pool descriptors which are descendants of NFactType::TFact
    void AddFromGazetteer(const NGzt::TProtoPool& pool);

private:
    yvector<TFactType> FactTypes;

    typedef yhash<Stroka, size_t> TIndex;
    TIndex Index;
};
