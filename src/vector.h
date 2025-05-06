#ifndef VECTOR_H
#define VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/real.h>

struct Vector2i {
    int24_t x, y;
};

struct Vector2f {
    float x, y;
};

typedef struct Vector2i vec2i_t;
typedef struct Vector2f vec2f_t;

vec2i_t vec2i(int24_t x, int24_t y);
vec2f_t vec2f(float x, float y);

#ifdef __cplusplus
}
#endif

#endif