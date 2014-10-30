#include "platform.h"

#ifdef _win_
    #include "winint.h"
    #include <process.h>
#else
    #include <pthread.h>
    #include <sched.h>
#endif

void SchedYield() throw () {
#if defined(_unix_)
    sched_yield();
#else
    Sleep(0);
#endif
}

void ThreadYield() throw () {
#if defined(_freebsd_)
    pthread_yield();
#else
    SchedYield();
#endif
}
