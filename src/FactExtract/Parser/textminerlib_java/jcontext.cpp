#include "jcontext.h"

void JContext::ConfigureThreadLocals()
{
    Singleton<CDictsHolder>(&m_dictsHolder);
    Singleton<CLangData>(&m_langData);
    Singleton<TMorph>(&m_morph);
    Singleton<TFactexTerminalDictionary>(&m_factexTerminalDictionary);
    Singleton<TDefaultTerminalDictionary>(&m_defaultTerminalDictionary);

    Singleton<TTomitaAgreementNames>(&m_tomitaAgreementNames);
    Singleton<TFixedKWTypeNames>(&m_fixedKWTypeNames);
    Singleton<TKWTypePool>(&m_kwTypePool);
    Singleton<TFakeProtoPool>(&m_fakeProtoPool);
    Singleton<NInfl::TNumFlexLemmer>(&m_numFlexLemmer);
}
