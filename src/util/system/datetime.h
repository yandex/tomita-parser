#pragma once

#include <util/system/defaults.h>
#include <util/system/platform.h>

#if defined(_win_) && !defined(__GCCXML__)
#   include <intrin.h>
#   pragma intrinsic(__rdtsc)
#endif // _win_

#if defined (_arm_)
#   include <mach/mach_time.h>
#endif

/// util/system/datetime.h contains only system time providers
/// for handy datetime utilities include util/datetime/base.h

/// Current time in milliseconds since epoch
ui64 millisec();
/// Current time in microseconds since epoch
ui64 MicroSeconds();
/// Current time in seconds since epoch
ui32 Seconds();
///Current thread time in microseconds
ui64 ThreadCPUUserTime();
ui64 ThreadCPUSystemTime();
ui64 ThreadCPUTime();

void NanoSleep(ui64 ns);

// GetCycleCount guarantees to return synchronous values on different cores
// and provide constant rate only on modern Intel and AMD processors
FORCED_INLINE ui64 GetCycleCount() {
#if defined(_MSC_VER) && !defined(__GCCXML__)
    // Generates the rdtsc instruction, which returns the processor time stamp.
    // The processor time stamp records the number of clock cycles since the last reset.
    return __rdtsc();
#else

    #if defined(_x86_64_)
        unsigned hi, lo;
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
        return ((unsigned long long)lo)|(((unsigned long long)hi)<<32);
    #elif defined(_i386_)
        ui64 x;
        __asm__ volatile ("rdtsc\n\t" : "=A" (x));
        return x;
    #elif defined (_arm_)
        return mach_absolute_time();
    #else
        #error "unsupported arch"
    #endif // _x86_64_

#endif
}
