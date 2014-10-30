#pragma once

#include <util/draft/relaxed_escaper.h>
#include <util/generic/ptr.h>
#include <util/generic/refcount.h>


namespace NProtobufJson {

class IStringTransform : public TRefCounted<IStringTransform> {
public:
    virtual ~IStringTransform() {}

    /// Some transforms have special meaning.
    /// For example, escape transforms cause generic JSON escaping to be turned off.
    enum Type {
        EscapeTransform = 0x1,
    };

    virtual int GetType() const = 0;

    virtual void Transform(Stroka& str) const = 0;
};

typedef TIntrusivePtr<IStringTransform> TStringTransformPtr;

template <bool quote, bool tounicode>
class TEscapeJTransform : public IStringTransform {
public:
    virtual int GetType() const {
        return EscapeTransform;
    }

    virtual void Transform(Stroka& str) const {
        Stroka newStr;
        NEscJ::EscapeJ<quote, tounicode>(str, newStr);
        str = newStr;
    }
};

class TCEscapeTransform : public IStringTransform {
public:
    virtual int GetType() const {
        return EscapeTransform;
    }

    virtual void Transform(Stroka& str) const;
};

class TSafeUtf8CEscapeTransform : public IStringTransform {
public:
    virtual int GetType() const {
        return EscapeTransform;
    }

    virtual void Transform(Stroka& str) const;
};

} // namespace NProtobufJson
