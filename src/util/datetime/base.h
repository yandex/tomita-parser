#pragma once

#include "systime.h"

#include <util/system/platform.h>
#include <util/system/datetime.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/ylimits.h>
#include <util/generic/utility.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>

#include <ctime>
#include <cstdio>

#include <time.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244) // conversion from 'time_t' to 'long', possible loss of data
#endif // _MSC_VER

// Microseconds since epoch
class TInstant;

// Duration is microseconds. Could be used to store timeouts, for example.
class TDuration;

/// Current time
static inline TInstant Now() throw ();

/// Use Now() method to obtain current time instead of *Seconds() unless you understand what are you doing.

class TDateTimeParseException: public yexception {
};

const int DATE_BUF_LEN = 4 + 2 + 2 + 1; // [YYYYMMDD*]

inline void sprint_date(char* buf, const struct tm& theTm) {
    sprintf(buf, "%04d%02d%02d", theTm.tm_year + 1900, theTm.tm_mon + 1, theTm.tm_mday);
}

inline long seconds(const struct tm& theTm) {
    return 60 * (60 * theTm.tm_hour + theTm.tm_min) + theTm.tm_sec;
}

inline void sprint_date(char* buf, time_t when, long* sec = 0) {
    struct tm theTm;
    localtime_r(&when, &theTm);
    sprint_date(buf, theTm);
    if (sec)
        *sec = seconds(theTm);
}

inline void sprint_gm_date(char* buf, time_t when, long* sec = 0) {
    struct tm theTm;
    ::Zero(theTm);
    GmTimeR(&when, &theTm);
    sprint_date(buf, theTm);
    if (sec)
        *sec = seconds(theTm);
}

inline bool sscan_date(const char* date, struct tm& theTm) {
    int year, mon, mday;
    if (sscanf(date, "%4d%2d%2d", &year, &mon, &mday) != 3)
        return false;
    theTm.tm_year = year - 1900;
    theTm.tm_mon = mon - 1;
    theTm.tm_mday = mday;
    return true;
}

const int DATE_8601_LEN = 21; // strlen("YYYY-MM-DDThh:mm:ssZ") = 20 + '\0'

inline void sprint_date8601(char* buf, time_t when) {
    struct tm theTm;
    struct tm* ret;
    ret = GmTimeR(&when, &theTm);
    if (ret != NULL) {
        sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ", theTm.tm_year+1900, theTm.tm_mon+1, theTm.tm_mday, theTm.tm_hour, theTm.tm_min, theTm.tm_sec);
    } else {
        *buf = '\0';
    }
}

bool ParseISO8601DateTime(const char* date, time_t& utcTime);
bool ParseISO8601DateTime(const char* date, size_t dateLen, time_t& utcTime);
bool ParseRFC822DateTime(const char* date, time_t& utcTime);
bool ParseRFC822DateTime(const char* date, size_t dateLen, time_t& utcTime);
bool ParseHTTPDateTime(const char* date, time_t& utcTime);
bool ParseHTTPDateTime(const char* date, size_t dateLen, time_t& utcTime);
bool ParseX509ValidityDateTime(const char* date, time_t& utcTime);
bool ParseX509ValidityDateTime(const char* date, size_t dateLen, time_t& utcTime);

inline long TVdiff(timeval r1, timeval r2) {
    return (1000000 * (r2.tv_sec - r1.tv_sec) + (r2.tv_usec - r1.tv_usec));
}

Stroka Strftime(const char* format, const struct tm* tm);

template <class S>
class TTimeBase {
public:
    typedef ui64 TValue;
    inline TTimeBase() throw ()
        : Value_(0)
    {
    }

    inline TTimeBase(const TValue& value) throw ()
        : Value_(value)
    {
    }

    inline TTimeBase(const struct timeval& tv) throw ()
        : Value_(tv.tv_sec * (ui64) 1000000 + tv.tv_usec)
    {
    }

    inline TValue GetValue() const throw () {
        return Value_;
    }

    inline double SecondsFloat() const throw () {
        return Value_ / 1000000.0;
    }

    inline TValue MicroSeconds() const throw () {
        return Value_;
    }

    inline TValue MilliSeconds() const throw () {
        return MicroSeconds() / 1000;
    }

    inline TValue Seconds() const throw () {
        return MilliSeconds() / 1000;
    }

    inline TValue Minutes() const throw () {
        return Seconds() / 60;
    }

    inline TValue Hours() const throw () {
        return Minutes() / 60;
    }

    inline TValue NanoSeconds() const throw () {
        if (MicroSeconds() >= (Max<TValue>() / (TValue)1000)) {
            return Max<TValue>();
        }

        return MicroSeconds() * (TValue)1000;
    }

    inline ui32 MicroSecondsOfSecond() const throw () {
        return MicroSeconds() % (TValue)1000000;
    }

    inline ui32 MilliSecondsOfSecond() const throw () {
        return MicroSecondsOfSecond() / (TValue)1000;
    }

    inline ui32 NanoSecondsOfSecond() const throw () {
        return MicroSecondsOfSecond() * (TValue)1000;
    }
protected:
    TValue Value_;
};

namespace NDateTimeHelpers {
    template <typename T>
    struct TPrecisionHelper {
        typedef ui64 THighPrecision;
    };

    template <>
    struct TPrecisionHelper<float> {
        typedef double THighPrecision;
    };

    template <>
    struct TPrecisionHelper<double> {
        typedef double THighPrecision;
    };
};

class TDuration: public TTimeBase<TDuration> {
    typedef TTimeBase<TDuration> TBase;
public:
    inline TDuration() throw () {
    }

    //better use static constructors
    inline explicit TDuration(TValue value) throw ()
        : TBase(value)
    {
    }

    inline TDuration(const struct timeval& tv) throw ()
        : TBase(tv)
    {
    }

    static inline TDuration MicroSeconds(ui64 us) throw () {
        return TDuration(us);
    }

    template <typename T>
    static inline TDuration MilliSeconds(T ms) throw () {
        return MicroSeconds((ui64)(typename NDateTimeHelpers::TPrecisionHelper<T>::THighPrecision(ms) * 1000));
    }

    using TBase::Hours;
    using TBase::Minutes;
    using TBase::Seconds;
    using TBase::MilliSeconds;
    using TBase::MicroSeconds;

    /// DeadLineFromTimeOut
    inline TInstant ToDeadLine() const;
    inline TInstant ToDeadLine(TInstant now) const;

    static inline TDuration Max() throw () {
        return TDuration(::Max<TValue>());
    }

    static inline TDuration Zero() throw () {
        return TDuration();
    }

    template <typename T>
    static inline TDuration Seconds(T s) throw () {
        return MilliSeconds(typename NDateTimeHelpers::TPrecisionHelper<T>::THighPrecision(s) * 1000);
    }

    static inline TDuration Minutes(ui64 m) throw () {
        return Seconds(m * 60);
    }

    static inline TDuration Hours(ui64 h) throw () {
        return Minutes(h * 60);
    }

    static inline TDuration Days(ui64 d) throw () {
        return Hours(d * 24);
    }

    /// parses strings like 10s, 15ms, 15.05s, 20us, or just 25 (us). See datetime_ut.cpp for details
    static TDuration Parse(const TStringBuf& input);

    static bool TryParse(const TStringBuf& input, TDuration& result);

    // note global Out method is defined for TDuration, so it could be written to TOutputStream as text

    template <class T>
    inline TDuration& operator+= (const T& t) throw () {
        return (*this = (*this + t));
    }

    template <class T>
    inline TDuration& operator-= (const T& t) throw () {
        return (*this = (*this - t));
    }

    template <class T>
    inline TDuration& operator*= (const T& t) throw () {
        return (*this = (*this * t));
    }

    template <class T>
    inline TDuration& operator/= (const T& t) throw () {
        return (*this = (*this / t));
    }

    Stroka ToString() const;
};

DECLARE_PODTYPE(TDuration);

/// TInstant and TDuration are guaranteed to have same precision
class TInstant: public TTimeBase<TInstant> {
    typedef TTimeBase<TInstant> TBase;
public:
    inline TInstant() {
    }

    //better use static constructors
    inline explicit TInstant(TValue value)
        : TBase(value)
    {
    }

    inline TInstant(const struct timeval& tv)
        : TBase(tv)
    {
    }

    static inline TInstant Now() {
        return TInstant::MicroSeconds(::MicroSeconds());
    }

    using TBase::Hours;
    using TBase::Minutes;
    using TBase::Seconds;
    using TBase::MilliSeconds;
    using TBase::MicroSeconds;

    static inline TInstant Max() throw () {
        return TInstant(::Max<TValue>());
    }

    static inline TInstant Zero() throw () {
        return TInstant();
    }

    /// us since epoch
    static inline TInstant MicroSeconds(ui64 us) throw () {
        return TInstant(us);
    }

    /// ms since epoch
    static inline TInstant MilliSeconds(ui64 ms) throw () {
        return MicroSeconds(ms * 1000);
    }

    /// seconds since epoch
    static inline TInstant Seconds(ui64 s) throw () {
        return MilliSeconds(s * 1000);
    }

    /// minutes since epoch
    static inline TInstant Minutes(ui64 m) throw () {
        return Seconds(m * 60);
    }

    inline time_t TimeT() const throw () {
        return (time_t)Seconds();
    }

    inline struct timeval TimeVal() const throw () {
        struct timeval tv;
        ::Zero(tv);
        tv.tv_sec = TimeT();
        tv.tv_usec = MicroSecondsOfSecond();
        return tv;
    }

    inline struct tm* LocalTime(struct tm* tm) const throw () {
        time_t clock = Seconds();
        return localtime_r(&clock, tm);
    }

    inline struct tm* GmTime(struct tm* tm) const throw () {
        time_t clock = Seconds();
        return GmTimeR(&clock, tm);
    }

    // note global Out method is defined to TInstant, so it can be written as text to TOutputStream
    Stroka ToString() const;
    Stroka ToStringUpToSeconds() const;

    static TInstant ParseIso8601(const TStringBuf&);
    static TInstant ParseRfc822(const TStringBuf&);
    static TInstant ParseHttp(const TStringBuf&);
    static TInstant ParseX509Validity(const TStringBuf&);

    template <class T>
    inline TInstant& operator+= (const T& t) throw () {
        return (*this = (*this + t));
    }

    template <class T>
    inline TInstant& operator-= (const T& t) throw () {
        return (*this = (*this - t));
    }
};

DECLARE_PODTYPE(TInstant)

template <class S>
static inline bool operator< (const TTimeBase<S>& l, const TTimeBase<S>& r) throw () {
    return l.GetValue() < r.GetValue();
}

template <class S>
static inline bool operator<= (const TTimeBase<S>& l, const TTimeBase<S>& r) throw () {
    return l.GetValue() <= r.GetValue();
}

template <class S>
static inline bool operator== (const TTimeBase<S>& l, const TTimeBase<S>& r) throw () {
    return l.GetValue() == r.GetValue();
}

template <class S>
static inline bool operator!= (const TTimeBase<S>& l, const TTimeBase<S>& r) throw () {
    return l.GetValue() != r.GetValue();
}

template <class S>
static inline bool operator> (const TTimeBase<S>& l, const TTimeBase<S>& r) throw () {
    return l.GetValue() > r.GetValue();
}

template <class S>
static inline bool operator>= (const TTimeBase<S>& l, const TTimeBase<S>& r) throw () {
    return l.GetValue() >= r.GetValue();
}

namespace NDateTimeHelpers {

    template <typename T>
    static inline T SumWithSaturation(T a, T b) {
        STATIC_ASSERT(!TNumericLimits<T>::is_signed)
        if (Max<T>() - a < b) {
            return Max<T>();
        }
        return a + b;
    }

    template <typename T>
    static inline T DiffWithSaturation(T a, T b) {
        STATIC_ASSERT(!TNumericLimits<T>::is_signed)
        if (a < b) {
            return 0;
        }
        return a - b;
    }
}

inline TDuration operator- (const TInstant& l, const TInstant& r) throw () {
    return TDuration(::NDateTimeHelpers::DiffWithSaturation(l.GetValue(), r.GetValue()));
}

inline TInstant operator+ (const TInstant& i, const TDuration& d) throw () {
    return TInstant(::NDateTimeHelpers::SumWithSaturation(i.GetValue(), d.GetValue()));
}

inline TInstant operator- (const TInstant& i, const TDuration& d) throw () {
    return TInstant(::NDateTimeHelpers::DiffWithSaturation(i.GetValue(), d.GetValue()));
}

inline TDuration operator- (const TDuration& l, const TDuration& r) throw () {
    return TDuration(::NDateTimeHelpers::DiffWithSaturation(l.GetValue(), r.GetValue()));
}

inline TDuration operator+ (const TDuration& l, const TDuration& r) throw () {
    return TDuration(::NDateTimeHelpers::SumWithSaturation(l.GetValue(), r.GetValue()));
}

template <class T>
inline TDuration operator* (const TDuration& d, const T& t) throw () {
    //TODO - check for overflow
    return TDuration(d.GetValue() * t);
}

template <class T>
inline TDuration operator/ (const TDuration& d, const T& t) throw () {
    return TDuration(d.GetValue() / t);
}

TInstant TDuration::ToDeadLine() const {
    return ToDeadLine(TInstant::Now());
}

TInstant TDuration::ToDeadLine(TInstant now) const {
    return now + *this;
}

static inline TInstant::TValue Local(TInstant instant) throw () {
    const time_t timeT = instant.TimeT();
    struct tm tm;

    localtime_r(&timeT, &tm); // in fact, this function never fails

    return (TInstant::Seconds(TimeGM(&tm)) + TDuration::MilliSeconds(instant.MilliSeconds() % instant.Seconds()) + TDuration::MicroSeconds(instant.MicroSeconds() % instant.MilliSeconds())).GetValue();
}

void Sleep(TDuration duration);

static inline void SleepUntil(TInstant instant) {
    TInstant now = TInstant::Now();
    if (instant <= now)
        return;
    TDuration duration = instant - now;
    Sleep(duration);
}

static inline TInstant Now() throw () {
    return TInstant::Now();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
