#pragma once

#include <FactExtract/Parser/afdocparser/rusie/dictsholder.h>
#include <FactExtract/Parser/tomalanglib/tomaparser.h>
#include <FactExtract/Parser/inflectorlib/numeral.h>

/*
Данный класс хранит основные структуры томиты, которые
в оригинальной томите глобальные. Храним их здесь для того, чтобы
подсунуть через thread local в соответствующий singleton. Таким
образом, удается убрать race conditions, которые в противном
случае наблюдались бы при создании и использовании нескольких
инстансов томиты.
*/
class JContext
{
public:
    void ConfigureThreadLocals();

private:
    CDictsHolder m_dictsHolder;
    CLangData m_langData;
    TMorph m_morph;
    TFactexTerminalDictionary m_factexTerminalDictionary;
    TDefaultTerminalDictionary m_defaultTerminalDictionary;
    TTomitaAgreementNames m_tomitaAgreementNames;
    TFixedKWTypeNames m_fixedKWTypeNames;
    TKWTypePool m_kwTypePool;
    TFakeProtoPool m_fakeProtoPool;
    NInfl::TNumFlexLemmer m_numFlexLemmer;
};
