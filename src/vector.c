#include "vector.h"

vec2i_t vec2i(int24_t x, int24_t y) {
    vec2i_t v;
    v.x = x; v.y = y;
    return v;
}

vec2f_t vec2f(float x, float y) {
    vec2f_t v;
    v.x = x; v.y = y;
    return v;
}