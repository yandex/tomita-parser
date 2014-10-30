#pragma once

#include <util/generic/stroka.h>
#include <FactExtract/Parser/common/toma_constants.h>
#include <FactExtract/Parser/common/fact-types.h>

// Interface for checking referred articles types or titles
class IArticleNames {
public:
    virtual bool HasArticleOrType(const Wtroka& name) const = 0;
    virtual TKeyWordType GetKWType(const Stroka& name) const = 0;
    virtual bool HasArticleField(const Wtroka& name, const Stroka& field) const = 0;
};

// Interface for checking referred fact-types
class IFactTypeNames {
public:
    virtual bool HasFactType(const Stroka& fact) const = 0;
    virtual bool HasFactField(const Stroka& fact, const Stroka& field) const = 0;
};

class IFactTypeDictionary: public IFactTypeNames {
public:
    virtual const fact_type_t* GetFactType(const Stroka& fact) const = 0;

    // implements IFactTypeNames -------------------------------------
    virtual bool HasFactType(const Stroka& fact) const {
        return GetFactType(fact) != NULL;
    }

    virtual bool HasFactField(const Stroka& fact, const Stroka& field) const {
        const fact_type_t* factType = GetFactType(fact);
        return factType != NULL && factType->get_field(field) != NULL;
    }
};
