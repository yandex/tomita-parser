#pragma once

class TBuffer;
class TInputStream;

/*
 * fast entropy pool, based on good prng,
 * initialized with some bits from system entropy pool
 */
TInputStream* EntropyPool();

/*
 * initial host entropy data
 */
const TBuffer& HostEntropy();
