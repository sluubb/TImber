#ifndef MAP_H
#define MAP_H

#define MAX_WALLS (6)
#define MAX_SECTORS (6)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "vector.h"

struct Wall {
    vec2i_t corner_a, corner_b;
    int24_t length_sq;
    uint24_t color;
    const struct Sector *portal_target; // portal_target < 0 means it's not a portal
};

struct Sector {
    uint24_t num_walls;
    struct Wall walls[MAX_WALLS];
};

void create_wall(struct Wall *wall, vec2i_t corner_a, vec2i_t corner_b, const struct Sector *portal_target);
void create_sector(struct Sector *sector, uint8_t num_walls, const vec2i_t *corners, const struct Sector **portal_targets);

#ifdef __cplusplus
}
#endif

#endif