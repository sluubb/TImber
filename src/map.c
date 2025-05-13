#include "map.h"

void init_portal(struct Wall *wall, const struct Sector *target_sector, uint24_t target_wall_i) {
    wall->is_portal = 1;
    wall->target_sector = target_sector;
    wall->target_wall_i = target_wall_i;
}

void create_wall(struct Wall *wall, vec2i_t edge_a, vec2i_t edge_b) {
    vec2i_t diff = vec2i(edge_b.x - edge_a.x, edge_b.y - edge_a.y);
    int24_t length_sq = diff.x * diff.x + diff.y * diff.y;

    wall->edge_a = edge_a;
    wall->edge_b = edge_b;
    wall->length_sq = length_sq;
    wall->color = 4 + 2 * (diff.x * abs(diff.x) / length_sq); // TODO: actual color implementation
    wall->is_portal = 0;
}

void create_sector(struct Sector *sector, uint8_t num_edges, const vec2i_t *edges) {
    sector->num_walls = num_edges;
    if (num_edges == 0) return;

    uint24_t last = num_edges - 1;

    for (uint24_t i = 0; i < last; i++) {
        create_wall(&sector->walls[i], edges[i], edges[i + 1]);
    }

    create_wall(&sector->walls[last], edges[last], edges[0]);
}