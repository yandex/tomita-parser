#pragma once

#include <util/system/defaults.h>
#include <util/generic/utility.h>
#include <util/datetime/base.h>

/// portable getrusage

struct TRusage {
    // some fields may be zero if unsupported

    // RSS in bytes
    // returned value may be not accurate, see discussion
    // http://www.mail-archive.com/freebsd-stable@freebsd.org/msg77102.html
    ui64 Rss;
    TDuration Utime;
    TDuration Stime;

    TRusage()
        : Rss(0)
    {
    }

    void Fill();

    static TRusage Get() {
        TRusage r;
        r.Fill();
        return r;
    }
};
