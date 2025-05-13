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
    vec2i_t edge_a, edge_b;
    int24_t length_sq;
    uint24_t color;

    int24_t is_portal;
    const struct Sector *target_sector;
    uint24_t target_wall_i;
};

struct Sector {
    uint24_t num_walls;
    struct Wall walls[MAX_WALLS];
};

void create_wall(struct Wall *wall, vec2i_t edge_a, vec2i_t edge_b);
void init_portal(struct Wall *wall, const struct Sector *target_sector, uint24_t target_wall_i);
void create_sector(struct Sector *sector, uint8_t num_edges, const vec2i_t *edges);

#ifdef __cplusplus
}
#endif

#endif