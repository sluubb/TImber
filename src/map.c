#include "map.h"

void create_wall(struct Wall *wall, vec2i_t corner_a, vec2i_t corner_b, const struct Sector *portal_target) {
    vec2i_t diff = vec2i(corner_b.x - corner_a.x, corner_b.y - corner_a.y);
    int24_t length_sq = diff.x * diff.x + diff.y * diff.y;

    wall->corner_a = corner_a;
    wall->corner_b = corner_b;
    wall->length_sq = length_sq;
    wall->color = 4 + 2 * (diff.x * abs(diff.x) / length_sq); // TODO: actual color implementation
    wall->portal_target = portal_target;
}

void create_sector(struct Sector *sector, uint8_t num_walls, const vec2i_t *corners, const struct Sector **portal_targets) {
    sector->num_walls = num_walls;
    if (num_walls == 0) return;

    uint24_t last = num_walls - 1;

    for (uint24_t i = 0; i < last; i++) {
        create_wall(&sector->walls[i], corners[i], corners[i + 1], portal_targets[i]);
    }

    create_wall(&sector->walls[last], corners[last], corners[0], portal_targets[last]);
}