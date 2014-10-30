#pragma once

#include <util/system/yassert.h>

/*
 * thread-safe random number generator.
 *
 * specialized for:
 *  all unsigned types (return value in range [0, MAX_VALUE_FOR_TYPE])
 *  bool
 *  long double (return value in range [0, 1))
 *  double (return value in range [0, 1))
 *  float (return value in range [0, 1))
 */
template <class T>
T RandomNumber();

/*
 * returns value in range [0, max)
 */
template <class T>
T RandomNumber(T max);

/*
 * return value in range [0, max) from any generator
 */
template <class T, class TRandGen>
static T GenUniform(T max, TRandGen &gen) {
    VERIFY(max > 0, "Invalid random number range [0, 0)");
    T randmax = gen.RandMax() - gen.RandMax() % max;
    T rand;
    while ((rand = gen.GenRand()) >= randmax) {
        /* no-op */
    }
    return rand % max;
}
