#pragma once

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/stroka.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NLemmer {

typedef ui16 TDiaMask; // Маска диакритики: набор битов, определяющих специализацию обобщенного символа

class TDiacriticsMap {
private:
    struct TDiacriticsSpecialization {
        static const size_t MAX_SPECIALIZATIONS = 3;
        wchar16 Symbols[MAX_SPECIALIZATIONS];
        size_t Bits;

        void Save(TOutputStream* out) const {
            ::SaveArray(out, Symbols, MAX_SPECIALIZATIONS);
            ::Save(out, Bits);
        }
        void Load(TInputStream* in) {
            ::LoadArray(in, Symbols, MAX_SPECIALIZATIONS);
            ::Load(in, Bits);
        }
    };

    struct TDiacriticsGeneralization {
        wchar16 Symbol;
        size_t Shift;
        size_t Index;

        void Save(TOutputStream* out) const {
            ::Save(out, Symbol);
            ::Save(out, Shift);
            ::Save(out, Index);
        }
        void Load(TInputStream* in) {
            ::Load(in, Symbol);
            ::Load(in, Shift);
            ::Load(in, Index);
        }
    };

    typedef ymap<TChar, TDiacriticsSpecialization> TSpecializations;
    TSpecializations Specializations;

    typedef ymap<TChar, TDiacriticsGeneralization> TGeneralizations;
    TGeneralizations Generalizations;

public:
    inline TDiaMask Generalize(const wchar16 * data, size_t len, wchar16 * out = NULL) const {
        TDiaMask result = 0;
        for (size_t i = 0; i != len; ++i) {
            TGeneralizations::const_iterator it = Generalizations.find(data[i]);
            if (it != Generalizations.end()) {
                const TDiacriticsGeneralization& gen = it->second;
                result <<= gen.Shift;
                result |= gen.Index;
                if (out) {
                    out[i] = gen.Symbol;
                }
            } else if (out) {
                out[i] = data[i];
            }
        }
        return result;
    }

    inline void SpecializeBackward(const wchar16 * data, size_t len, wchar16 * out, TDiaMask mask) const {
        for (size_t i = 0; i != len; ++i) {
            TSpecializations::const_iterator it = Specializations.find(data[i]);
            if (it != Specializations.end()) {
                const TDiacriticsSpecialization& spec = it->second;
                size_t index = mask & ((1 << spec.Bits) - 1);
                YASSERT(index < TDiacriticsSpecialization::MAX_SPECIALIZATIONS);
                mask >>= spec.Bits;
                out[i] = spec.Symbols[index];
            } else {
                out[i] = data[i];
            }
        }
    }

    inline void SkipAndSpecialize(const wchar16 * data, int skip, int len, wchar16 * out, TDiaMask mask) const {
        if (len == 0) {
            return;
        }
        for (int i = len + skip - 1; i >= 0; --i) {
            TSpecializations::const_iterator it = Specializations.find(data[i]);
            if (it != Specializations.end()) {
                const TDiacriticsSpecialization& spec = it->second;
                size_t index = mask & ((1 << spec.Bits) - 1);
                YASSERT(index < TDiacriticsSpecialization::MAX_SPECIALIZATIONS);
                mask >>= spec.Bits;
                if (i < len) {
                    out[i] = spec.Symbols[index];
                }
            } else if (i < len) {
                out[i] = data[i];
            }
        }
    }

    inline size_t CountDistortions(const wchar16 * word, const wchar16 * gen, int len, TDiaMask mask) const {
        size_t result = 0;
        for (int i = len - 1; i >= 0; --i) {
            TSpecializations::const_iterator it = Specializations.find(gen[i]);
            if (it != Specializations.end()) {
                const TDiacriticsSpecialization& spec = it->second;
                size_t index = mask & ((1 << spec.Bits) - 1);
                YASSERT(index < TDiacriticsSpecialization::MAX_SPECIALIZATIONS);
                mask >>= spec.Bits;
                if (word[i] != spec.Symbols[index]) {
                    ++result;
                }
            }
        }
        return result;
    }

    void Clear() {
        Specializations.clear();
        Generalizations.clear();
    }

    void Add(const TWtringBuf& rule) {
        YASSERT(!rule.empty());
        YASSERT(rule.size() <= TDiacriticsSpecialization::MAX_SPECIALIZATIONS);
        TDiacriticsSpecialization& spec = Specializations[rule[0]];
        spec.Bits = GetValueBitCount(rule.size() - 1);
        for (size_t i = 0; i != rule.size(); ++i) {
            spec.Symbols[i] = rule[i];
        }
        for (size_t i = 0; i != rule.size(); ++i) {
            TDiacriticsGeneralization& gen = Generalizations[rule[i]];
            gen.Symbol = rule[0];
            gen.Shift = spec.Bits;
            gen.Index = i;
        }
    }

    void Add(const wchar16 * rule) {
        Add(TWtringBuf(rule));
    }

    void Save(TOutputStream* out) const {
        ::Save(out, Generalizations);
        ::Save(out, Specializations);
    }

    void Load(TInputStream* in) {
        ::Load(in, Generalizations);
        ::Load(in, Specializations);
    }
};
}

