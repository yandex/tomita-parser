#include "timehelper.h"


 CTimeSpan::CTimeSpan() throw() :
    m_timeSpan(0)
{
}

 CTimeSpan::CTimeSpan(time_t time) throw() :
    m_timeSpan(time)
{
}

 CTimeSpan::CTimeSpan(long lDays, int nHours, int nMins, int nSecs) throw()
{
     m_timeSpan = nSecs + 60* (nMins + 60* (nHours + i64(24) * lDays));
}

 i64 CTimeSpan::GetDays() const throw()
{
    return(m_timeSpan/(24*3600));
}

 i64 CTimeSpan::GetTotalHours() const throw()
{
    return(m_timeSpan/3600);
}

 long CTimeSpan::GetHours() const throw()
{
    return(long(GetTotalHours()-(GetDays()*24)));
}

 i64 CTimeSpan::GetTotalMinutes() const throw()
{
    return(m_timeSpan/60);
}

 long CTimeSpan::GetMinutes() const throw()
{
    return(long(GetTotalMinutes()-(GetTotalHours()*60)));
}

 i64 CTimeSpan::GetTotalSeconds() const throw()
{
    return(m_timeSpan);
}

 long CTimeSpan::GetSeconds() const throw()
{
    return(long(GetTotalSeconds()-(GetTotalMinutes()*60)));
}

 time_t CTimeSpan::GetTimeSpan() const throw()
{
    return(m_timeSpan);
}

 CTimeSpan CTimeSpan::operator+(CTimeSpan span) const throw()
{
    return(CTimeSpan(m_timeSpan+span.m_timeSpan));
}

 CTimeSpan CTimeSpan::operator-(CTimeSpan span) const throw()
{
    return(CTimeSpan(m_timeSpan-span.m_timeSpan));
}

 CTimeSpan& CTimeSpan::operator+=(CTimeSpan span) throw()
{
    m_timeSpan += span.m_timeSpan;
    return(*this);
}

 CTimeSpan& CTimeSpan::operator-=(CTimeSpan span) throw()
{
    m_timeSpan -= span.m_timeSpan;
    return(*this);
}

 bool CTimeSpan::operator==(CTimeSpan span) const throw()
{
    return(m_timeSpan == span.m_timeSpan);
}

 bool CTimeSpan::operator!=(CTimeSpan span) const throw()
{
    return(m_timeSpan != span.m_timeSpan);
}

 bool CTimeSpan::operator<(CTimeSpan span) const throw()
{
    return(m_timeSpan < span.m_timeSpan);
}

 bool CTimeSpan::operator>(CTimeSpan span) const throw()
{
    return(m_timeSpan > span.m_timeSpan);
}

 bool CTimeSpan::operator<=(CTimeSpan span) const throw()
{
    return(m_timeSpan <= span.m_timeSpan);
}

 bool CTimeSpan::operator>=(CTimeSpan span) const throw()
{
    return(m_timeSpan >= span.m_timeSpan);
}

/////////////////////////////////////////////////////////////////////////////
// CTime
/////////////////////////////////////////////////////////////////////////////

CTime   CTime::GetCurrentTime() throw()
{

    return(CTime(::time(0)));
}

 CTime::CTime() throw() :
    m_time(0)
{
}

 CTime::CTime(time_t time)  throw():
    m_time(time)
{
}

 CTime::CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
    int nDST)
{

    if (!((nYear >= 1900) &&
        (nMonth >= 1 && nMonth <= 12)&&
        (nDay >= 1 && nDay <= 31)&&
        (nHour >= 0 && nHour <= 23)&&
        (nMin >= 0 && nMin <= 59)&&
        (nSec >= 0 && nSec <= 59)))
        ythrow yexception() << "Bad date in \"CTime::CTime\"";

    struct tm atm;

    atm.tm_sec = nSec;
    atm.tm_min = nMin;
    atm.tm_hour = nHour;
    atm.tm_mday = nDay;
    atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
    atm.tm_year = nYear - 1900;     // tm_year is 1900 based
    atm.tm_isdst = nDST;

    m_time = mktime(&atm);
    if (m_time == -1) {
        ythrow yexception() << "Bad date from mktime in \"CTime::CTime\"";
    }
}

#ifdef _win_
CTime::CTime(const FILETIME& fileTime, int /*nDST*/)
{
    timeval tv;
    FileTimeToTimeval(&fileTime, &tv);
    m_time = tv.tv_sec;
}
#endif

 CTime& CTime::operator=(const CTime& time) throw()
 {
    m_time = time.m_time;
    return(*this);
 }

 CTime& CTime::operator=(time_t time) throw()
{
    m_time = time;
    return(*this);
}

 bool CTime::operator==(CTime time) const throw()
{
    return(m_time == time.m_time);
}

 bool CTime::operator!=(CTime time) const throw()
{
    return(m_time != time.m_time);
}

 bool CTime::operator<(CTime time) const throw()
{
    return(m_time < time.m_time);
}

 bool CTime::operator>(CTime time) const throw()
{
    return(m_time > time.m_time);
}

 bool CTime::operator<=(CTime time) const throw()
{
    return(m_time <= time.m_time);
}

 bool CTime::operator>=(CTime time) const throw()
{
    return(m_time >= time.m_time);
}

 time_t CTime::GetTime() const throw()
{
    return(m_time);
}

 int CTime::GetYear() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? (ptm->tm_year) + 1900 : 0;
}

 int CTime::GetMonth() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? ptm->tm_mon + 1 : 0;
}

 int CTime::GetDay() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? ptm->tm_mday : 0;
}

 int CTime::GetHour() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? ptm->tm_hour : -1;
}

 int CTime::GetMinute() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? ptm->tm_min : -1;
}

 int CTime::GetSecond() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? ptm->tm_sec : -1;
}

 int CTime::GetDayOfWeek() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = localtime_r(&m_time,&ttm);
    return ptm ? ptm->tm_wday + 1 : 0;
}

CTime& CTime::operator+=(CTimeSpan span)
{
    m_time += span.GetTimeSpan();

    return(*this);
}

CTime& CTime::operator-=(CTimeSpan span)
{
    m_time -= span.GetTimeSpan();

    return(*this);
}

CTimeSpan CTime::operator-(CTime time)
{
    return(CTimeSpan(m_time-time.m_time));
}

CTime CTime::operator-(CTimeSpan span)
{
    return(CTime(m_time-span.GetTimeSpan()));
}

CTime CTime::operator+(CTimeSpan span)
{
    return(CTime(m_time+span.GetTimeSpan()));
}
