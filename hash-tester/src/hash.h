#ifndef HASH_H_
#define HASH_H_

#include <stdint.h>

struct HashMetaData {
    const char *name;
    uint64_t (*hash)(uint64_t);
};

uint64_t fmix64(uint64_t k);
uint64_t splitmix64_mix(uint64_t k);

#endif /* HASH_H_ */
