#ifndef DRAW_H
#define DRAW_H

#define SCREEN_WIDTH (GFX_LCD_WIDTH)
#define HALF_SCREEN_WIDTH (SCREEN_WIDTH / 2)
#define SCREEN_HEIGHT (GFX_LCD_HEIGHT)
#define HALF_SCREEN_HEIGHT (SCREEN_HEIGHT / 2)

#define PERSPECTIVE_SCALE (200)

#define WALL_HEIGHT (100)

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include <debug.h>

#include "vector.h"
#include "map.h"
#include "utils.h"

struct WallDrawCall {
    vec2i_t edge_a_screen, edge_b_screen;
};

void draw_sector(const struct Sector *sector, vec2i_t camera_orig, vec2f_t camera_dir, int24_t min_x_screen, int24_t max_x_screen);
void draw_wall(const struct Wall *wall, vec2i_t camera_orig, vec2f_t camera_dir, int24_t min_x_screen, int24_t max_x_screen);
vec2i_t relative_coord(vec2i_t orig, vec2f_t dir, vec2i_t p_abs);
bool clip_wall(vec2i_t *edge_a_rel, vec2i_t *edge_b_rel, int24_t min_x_screen, int24_t max_x_screen);
void project(vec2i_t *p_screen, vec2i_t p_rel);
void draw_wall_2d(const struct WallDrawCall *draw_call);

#ifdef __cplusplus
}
#endif

#endif