#include <stdint.h>
#include "hash.h"

uint64_t hash(uint64_t k) {
    /* splitmix64 hash
     * source: https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64
     */
    k = (k ^ (k >> 30)) * 0xBF58476D1CE4E5B9ULL;
    k = (k ^ (k >> 27)) * 0x94D049BB133111EBULL;
    return k ^ (k >> 31);
}
