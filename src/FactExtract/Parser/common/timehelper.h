
#ifndef timehelper_h
#define timehelper_h

#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4159 4127)
#endif


//#include <winsock2.h>

#include <util/generic/stroka.h>
#include <util/system/defaults.h>
#include <util/datetime/base.h>
#include <util/generic/yexception.h>

#include <time.h>


class CTimeSpan
{
public:
    CTimeSpan() throw();
    CTimeSpan(time_t time) throw();
    CTimeSpan(long lDays, int nHours, int nMins, int nSecs) throw();

    i64 GetDays() const throw();
    i64 GetTotalHours() const throw();
    long GetHours() const throw();
    i64 GetTotalMinutes() const throw();
    long GetMinutes() const throw();
    i64 GetTotalSeconds() const throw();
    long GetSeconds() const throw();

    time_t GetTimeSpan() const throw();

    CTimeSpan operator+(CTimeSpan span) const throw();
    CTimeSpan operator-(CTimeSpan span) const throw();
    CTimeSpan& operator+=(CTimeSpan span) throw();
    CTimeSpan& operator-=(CTimeSpan span) throw();
    bool operator==(CTimeSpan span) const throw();
    bool operator!=(CTimeSpan span) const throw();
    bool operator<(CTimeSpan span) const throw();
    bool operator>(CTimeSpan span) const throw();
    bool operator<=(CTimeSpan span) const throw();
    bool operator>=(CTimeSpan span) const throw();

//public:
//    Stroka Format( LPCTSTR pszFormat ) const;

//#ifdef _AFX
//    CArchive& Serialize64(CArchive& ar);
//#endif

private:
    time_t  m_timeSpan;
};

class CTime
{
public:
    static CTime   GetCurrentTime() throw();
    //static bool     IsValidFILETIME(const FILETIME& ft) throw();

    CTime() throw();
    CTime(time_t time) throw();
    CTime(const CTime& _time): m_time(_time.m_time) {};

    CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
        int nDST = -1);
    CTime(unsigned short wDosDate, unsigned short wDosTime, int nDST = -1);
    //CTime( const SYSTEMTIME& st, int nDST = -1 );
#ifdef _win_
    CTime(const FILETIME & ft, int nDST = -1);
#endif
    CTime& operator=(time_t time) throw();
    CTime& operator=(const CTime& time) throw();

    //CTime& operator+=( CTimeSpan span ) throw();
    //CTime& operator-=( CTimeSpan span ) throw();

    //CTimeSpan operator-( CTime time ) const throw();
    //CTime operator-( CTimeSpan span ) const throw();
    //CTime operator+( CTimeSpan span ) const throw();

    bool operator==(CTime time) const throw();
    bool operator!=(CTime time) const throw();
    bool operator<(CTime time) const throw();
    bool operator>(CTime time) const throw();
    bool operator<=(CTime time) const throw();
    bool operator>=(CTime time) const throw();

    time_t GetTime() const throw();

    int GetYear() const throw();
    int GetMonth() const throw();
    int GetDay() const throw();
    int GetHour() const throw();
    int GetMinute() const throw();
    int GetSecond() const throw();
    int GetDayOfWeek() const throw();

    // formatting using "C" strftime
    Stroka Format(const char* pszFormat) const;
    Stroka FormatGmt(const char* pszFormat) const;
    //Stroka Format( ui32 nFormatID ) const;
    //Stroka FormatGmt( ui32 nFormatID ) const;

    CTime& operator+=(CTimeSpan span);
    CTime& operator-=(CTimeSpan span);
    CTimeSpan operator-(CTime time);
    CTime operator-(CTimeSpan span);
    CTime operator+(CTimeSpan span);

private:
    time_t m_time;
};

// Used only if these strings could not be found in resources.
//extern __declspec(selectany) const char * const szInvalidDateTime = _T("Invalid DateTime");
//extern __declspec(selectany) const char * const szInvalidDateTimeSpan = _T("Invalid DateTimeSpan");

const int maxTimeBufferSize = 128;
const long maxDaysInSpan  =    3615897L;

inline Stroka CTime::Format(const char* pFormat) const
{
    if (pFormat == NULL)
        return pFormat;

    char szBuffer[maxTimeBufferSize];
    struct tm tmTemp;
    localtime_r(&m_time, &tmTemp);
    strftime(szBuffer, maxTimeBufferSize, pFormat, &tmTemp);
    return szBuffer;
}

inline Stroka CTime::FormatGmt(const char* pFormat) const
{
    if (pFormat == NULL)
        return pFormat;

    char szBuffer[maxTimeBufferSize];
    struct tm tmTemp;
    gmtime_r(&m_time, &tmTemp);
    strftime(szBuffer, maxTimeBufferSize, pFormat, &tmTemp);
    return szBuffer;
}

//#pragma pack(pop)

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  // timehelper_h
