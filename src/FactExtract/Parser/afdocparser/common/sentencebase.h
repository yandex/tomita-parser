#pragma once

#include <util/generic/stroka.h>

#include "wordbase.h"
#include <FactExtract/Parser/afdocparser/rus/namecluster.h>
#include "graminfo.h"
#include "datetime.h"

struct SFioInDifferentFields
{
    SFioInDifferentFields()
        : m_Fio()
    {
        Reset();
    }

    void BuildFio();

    void Reset()
    {
        m_pMainWord = NULL;
        m_pNameWord = NULL;
        m_pPatronymicWord = NULL;
        m_bAddAsvariant = false;
        m_Fio.Reset();
    }

    CWordBase* m_pMainWord;
    CWordBase* m_pNameWord;
    CWordBase* m_pPatronymicWord;
    bool   m_bAddAsvariant; //если не нашли имени или отчества, то добавим их как варианты к фамилии, чтобы хоть по /с3 можно было их найти
    SFullFIO m_Fio;

};

//////////////////////////////////////////////////////////////////////////////
// abstract base class

class CNumber;
class CSentenceBase
{
public:
    CSentenceBase();
    virtual ~CSentenceBase(void);

// Data
public:
    size_t m_pos;
    size_t m_len;

    bool m_bPBreak;
    bool m_bDateField;

    ui16 m_vn;  // vector norm
    bool m_2sum;    // move to summary

// Code
public:
    //virtual CWordBase* createWord(const CPrimGroup &group, const Wtroka& txt) = 0;
    virtual size_t getWordsCount() const = 0;
    virtual CWordBase* getWord(size_t iNum) const = 0;
    virtual void FreeData() = 0;
    virtual void AddWord(CWordBase*) = 0;
    virtual Wtroka BuildDates() = 0;
    virtual void ToString(Wtroka&) const {}
    virtual Wtroka ToString() const { return Wtroka();}

    virtual void BuildNumbers() {};
    virtual void AddNumber(CNumber&) {};

    void SetErrorStream(TOutputStream* pErrStream) { m_pErrorStream = pErrStream; }
    void PrintError(const char* msg) { if (m_pErrorStream) (*m_pErrorStream) << msg << Endl; }

    void PrintError(const Stroka& msg, const yexception* error);

protected:
    TOutputStream* m_pErrorStream;
};

