#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/real.h>

char *uint24_to_str(char *result, uint24_t value);

// pass t as a quotient for performance
// WARNING: high values may cause integer overflow
void lerpi(int24_t *value, int24_t target, int24_t t_num, int24_t t_den);

#ifdef __cplusplus
}
#endif

#endif