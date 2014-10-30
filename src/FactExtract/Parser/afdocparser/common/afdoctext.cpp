#include "afdocbase.h"


void CDocBase::proceedText()
{
    m_pText->ProcessAttributes();
    m_pText->Proceed(m_Source, 0, -1);
}

//////////////////////////////////////////////////////////////////////////////
