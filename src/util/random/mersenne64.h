#pragma once

#include <util/system/defaults.h>

#define NN 312
#define MM 156
#define MATRIX_A ULL(0xB5026F5AA96619E9)
#define UM ULL(0xFFFFFFFF80000000)
#define LM ULL(0x7FFFFFFF)

namespace NPrivate {
    inline static double ToRes53(ui64 v) throw () {
        return double(v * (1.0 / 18446744073709551616.0L));
    }

    class TMersenne64 {
        public:
            inline TMersenne64(ui64 s = ULL(19650218))
                : mti(NN + 1)
            {
                InitGenRand(s);
            }

            inline TMersenne64(const ui64 keys[], size_t len) throw ()
                : mti(NN + 1)
            {
                InitByArray(keys, len);
            }

            inline ui64 GenRand() throw () {
                int i;
                ui64 x;
                ui64 mag01[2] = {
                    ULL(0),
                    MATRIX_A
                };

                if (mti >= NN) {
                    if (mti == NN + 1) {
                        InitGenRand(ULL(5489));
                    }

                    for (i = 0; i < NN - MM; ++i) {
                        x = (mt[i] & UM) | (mt[i + 1] & LM);
                        mt[i] = mt[i + MM] ^ (x >> 1) ^ mag01[(int)(x & ULL(1))];
                    }

                    for (; i < NN - 1; ++i) {
                        x = (mt[i] & UM) | (mt[i + 1] & LM);
                        mt[i] = mt[i + (MM - NN) ] ^ (x >> 1) ^ mag01[(int)(x & ULL(1))];
                    }

                    x = (mt[NN - 1] & UM) | (mt[0] & LM);
                    mt[NN - 1] = mt[MM - 1] ^ (x >> 1) ^ mag01[(int)(x & ULL(1))];

                    mti = 0;
                }

                x = mt[mti++];

                x ^= (x >> 29) & ULL(0x5555555555555555);
                x ^= (x << 17) & ULL(0x71D67FFFEDA60000);
                x ^= (x << 37) & ULL(0xFFF7EEE000000000);
                x ^= (x >> 43);

                return x;
            }

            inline i64 GenRandSigned() throw () {
                return (i64)(GenRand() >> 1);
            }

            inline double GenRandReal1() throw () {
                return (GenRand() >> 11) * (1.0 / 9007199254740991.0);
            }

            inline double GenRandReal2() throw () {
                return (GenRand() >> 11) * (1.0 / 9007199254740992.0);
            }

            inline double GenRandReal3() throw () {
                return ((GenRand() >> 12) + 0.5) * (1.0 / 4503599627370496.0);
            }

            inline double GenRandReal4() throw () {
                return ToRes53(GenRand());
            }

        private:
            inline void InitGenRand(ui64 seed) throw () {
                mt[0] = seed;

                for (mti = 1; mti < NN; ++mti) {
                    mt[mti] = (ULL(6364136223846793005) * (mt[mti - 1] ^ (mt[mti - 1] >> 62)) + mti);
                }
            }

            inline void InitByArray(const ui64 init_key[], size_t key_length) throw () {
                ui64 i = 1;
                ui64 j = 0;
                ui64 k;

                InitGenRand(ULL(19650218));

                k = NN > key_length ? NN : key_length;

                for (; k; --k) {
                    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * ULL(3935559000370003845))) + init_key[j] + j;

                    ++i;
                    ++j;

                    if (i >= NN) {
                        mt[0] = mt[NN - 1];
                        i = 1;
                    }

                    if (j >= key_length) {
                        j = 0;
                    }
                }

                for (k = NN - 1; k; --k) {
                    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * ULL(2862933555777941757))) - i;

                    ++i;

                    if (i >= NN) {
                        mt[0] = mt[NN - 1];
                        i = 1;
                    }
                }

                mt[0] = ULL(1) << 63;
            }

        private:
            ui64 mt[NN];
            int mti;
    };
}

#undef NN
#undef MM
#undef MATRIX_A
#undef UM
#undef LM
