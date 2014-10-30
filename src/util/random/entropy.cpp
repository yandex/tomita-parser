#include "randcpp.h"
#include "entropy.h"
#include "mersenne.h"

#include <util/stream/ios.h>
#include <util/stream/zlib.h>
#include <util/system/info.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/system/execpath.h>
#include <util/system/datetime.h>
#include <util/system/hostname.h>
#include <util/generic/buffer.h>
#include <util/generic/singleton.h>
#include <util/digest/murmur.h>
#include <util/datetime/cputimer.h>
#include <util/ysaveload.h>

namespace {
    static inline void Permute(char* buf, size_t len) {
        TRand r;

        r.srandom(*buf + len);

        for (size_t i = 0; i < len; ++i) {
            DoSwap(buf[i], buf[r.random() % len]);
        }
    }

    class THostEntropy: public TBuffer {
        public:
            inline THostEntropy() {
                {
                    TBufferOutput buf(*this);
                    TZLibCompress out(&buf);

                    Save(&out, GetCycleCount());
                    Save(&out, MicroSeconds());
                    Save(&out, TThread::CurrentThreadId());
                    Save(&out, NSystemInfo::CachedNumberOfCpus());
                    Save(&out, HostName());
                    try {
                        Save(&out, GetExecPath());
                    } catch (...) {
                        //workaround - sometimes fails on FreeBSD
                    }
                    Save(&out, (size_t)Data());

                    double la[3];

                    NSystemInfo::LoadAverage(la, ARRAY_SIZE(la));

                    out.Write(la, sizeof(la));
                }

                {
                    TMemoryOutput out(Data(), Size());

                    //replace zlib header with hash
                    Save(&out, MurmurHash<ui64>(Data(), Size()));
                }

                Permute(Data(), Size());
            }

            static inline const TBuffer& Instance() {
                return *Singleton<THostEntropy>();
            }
    };

    //not thread-safe
    class TMersenneInput: public TInputStream {
            typedef ui64 TKey;
            typedef TMersenne<TKey> TRnd;
        public:
            inline TMersenneInput(const TBuffer& rnd = THostEntropy::Instance())
                : Rnd_((const TKey*)rnd.Data(), rnd.Size() / sizeof(TKey))
            {
            }

            virtual ~TMersenneInput() throw () {
            }

            virtual size_t DoRead(void* buf, size_t len) {
                size_t toRead = len;

                while (toRead) {
                    const TKey next = Rnd_.GenRand();
                    const size_t toCopy = Min(toRead, sizeof(next));

                    memcpy(buf, &next, toCopy);

                    buf = (char*)buf + toCopy;
                    toRead -= toCopy;
                }

                return len;
            }

        private:
            TRnd Rnd_;
    };

    class TEntropyPoolStream: public TInputStream {
        public:
            inline TEntropyPoolStream()
                : Bi_(&Mi_, 8192)
            {
            }

            virtual size_t DoRead(void* buf, size_t len) {
                TGuard<TMutex> guard(Mutex_);

                return Bi_.Read(buf, len);
            }

        private:
            TMutex Mutex_;
            TMersenneInput Mi_;
            TBufferedInput Bi_;
    };
}

TInputStream* EntropyPool() {
    return Singleton<TEntropyPoolStream>();
}

const TBuffer& HostEntropy() {
    return THostEntropy::Instance();
}
