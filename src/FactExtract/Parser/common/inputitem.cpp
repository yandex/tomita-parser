#include "inputitem.h"

void CInputItem::AddTerminalSymbol(TerminalSymbolType s) const
{
  m_AutomatSymbolInterpetationUnion.insert(s);
}

void CInputItem::EraseTerminalSymbol(TerminalSymbolType s) const
{
    m_AutomatSymbolInterpetationUnion.erase(s);
}

void CInputItem::DeleteTerminalSymbols()
{
  m_AutomatSymbolInterpetationUnion.clear();
}

