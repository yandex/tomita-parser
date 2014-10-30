#pragma once

#include <util/generic/set.h>

typedef int TerminalSymbolType;

class CInputItem
{
public:
    virtual ~CInputItem() {
    }

  void AddTerminalSymbol(TerminalSymbolType s) const;
  void EraseTerminalSymbol(TerminalSymbolType s) const;
  void DeleteTerminalSymbols();



  struct TWeight {
        static const ui64 Precision = 100000000;
        ui64 Value;     // weight [0,1] multiplied by Precision

        TWeight()
            : Value (1 * Precision)
        {
        }

        explicit TWeight(double w)
            : Value (static_cast<ui64>(w * Precision))
        {
        }

        // special case
        static TWeight Zero() {
            return TWeight((ui64)0);
        }

        // combinations
        TWeight(TWeight w1, TWeight w2)
            : Value((w1.Value + w2.Value) / 2)
        {
        }

        TWeight(TWeight w1, TWeight w2, TWeight w3)
            : Value((w1.Value + w2.Value + w3.Value) / 3)
        {
        }


        TWeight(ui64 numerator, ui64 denominator)
            : Value (numerator * Precision / denominator)
        {
        }

        TWeight& operator *= (TWeight w) {
            Value = (Value * w.Value) / Precision;
            return *this;
        }

        double AsDouble() const {
            return double(Value) / Precision;
        }

        bool operator < (TWeight w) const {
            return Value < w.Value;
        }

        bool operator == (TWeight w) const {
            return Value == w.Value;
        }

        bool operator != (TWeight w) const {
            return Value != w.Value;
        }
    };

    virtual TWeight GetWeight() const {
        return TWeight();
    }

  //void GetItemWeight(double& w) const;

    virtual size_t GetCoverage() const {
        return 0;
    }

    virtual size_t GetFactsCount() const {
        return 0;
    }

  mutable yset<TerminalSymbolType> m_AutomatSymbolInterpetationUnion;

  virtual void AddWeightOfChild(const CInputItem*) {
      YASSERT(false);
  }
};
