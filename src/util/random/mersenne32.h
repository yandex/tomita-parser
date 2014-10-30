#pragma once

#include <util/system/defaults.h>

#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL
#define UPPER_MASK 0x80000000UL
#define LOWER_MASK 0x7fffffffUL

namespace NPrivate {
    inline static double ToRes53Mix(ui32 x, ui32 y) throw () {
        return ToRes53((ui64)x | ((ui64)y << 32));
    }

    class TMersenne32 {
        public:
            inline TMersenne32(ui32 s  = 19650218UL) throw ()
                : mti(N + 1)
            {
                InitGenRand(s);
            }

            inline TMersenne32(const ui32 init_key[], size_t key_length) throw ()
                : mti(N + 1)
            {
                InitByArray(init_key, key_length);
            }

            inline ui32 GenRand() throw () {
                ui32 y;
                ui32 mag01[2] = { 0x0UL, MATRIX_A };

                if (mti >= N) {
                    int kk;

                    if (mti == N + 1) {
                        InitGenRand(5489UL);
                    }

                    for (kk = 0; kk < N - M; ++kk) {
                        y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
                        mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
                    }

                    for (; kk < N - 1; ++kk) {
                        y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
                        mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
                    }

                    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
                    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

                    mti = 0;
                }

                y = mt[mti++];

                y ^= (y >> 11);
                y ^= (y << 7) & 0x9d2c5680UL;
                y ^= (y << 15) & 0xefc60000UL;
                y ^= (y >> 18);

                return y;
            }

            inline i32 GenRandSigned() throw () {
                return (i32)(GenRand() >> 1);
            }

            inline double GenRandReal1() throw () {
                return GenRand() * (1.0 / 4294967295.0);
            }

            inline double GenRandReal2() throw () {
                return GenRand() * (1.0 / 4294967296.0);
            }

            inline double GenRandReal3() throw () {
                return ((double)GenRand() + 0.5) * (1.0 / 4294967296.0);
            }

            inline double GenRandReal4() throw () {
                const ui32 x = GenRand();
                const ui32 y = GenRand();

                return ToRes53Mix(x, y);
            }

        private:
            inline void InitGenRand(ui32 s) throw () {
                mt[0] = s;

                for (mti = 1; mti < N; ++mti) {
                    mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
                }
            }

            inline void InitByArray(const ui32 init_key[], size_t key_length) throw () {
                InitGenRand(19650218UL);

                ui32 i = 1;
                ui32 j = 0;
                ui32 k = ui32(N > key_length ? N : key_length);

                for (; k; k--) {
                    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525UL)) + init_key[j] + j;

                    ++i;
                    ++j;

                    if (i >= N) {
                        mt[0] = mt[N - 1];
                        i = 1;
                    }

                    if (j >= key_length) {
                        j = 0;
                    }
                }

                for (k = N - 1; k; k--) {
                    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941UL)) - i;

                    ++i;

                    if (i >= N) {
                        mt[0] = mt[N - 1];
                        i = 1;
                    }
                }

                mt[0] = 0x80000000UL;
            }

        private:
            ui32 mt[N];
            int mti;
    };
}

#undef N
#undef M
#undef MATRIX_A
#undef UPPER_MASK
#undef LOWER_MASK
