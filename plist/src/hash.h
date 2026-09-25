#ifndef HASH_H_
#define HASH_H_

#include <stdint.h>

uint64_t fmix64(uint64_t k);
uint64_t splitmix64_mix(uint64_t k);

#endif /* HASH_H_ */
