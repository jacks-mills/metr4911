#ifndef POLYS_H_
#define POLYS_H_

#include <stdint.h>
#include "error.h"


error_t polys_from_string(
        uint32_t *polysCount, uint32_t polysLength, uint64_t polys[polysLength],
        uint32_t stringLength, const char string[stringLength]);

#endif /* POLYS_H_ */
