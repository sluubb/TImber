#include "draw.h"

void draw_sector(const struct Sector *sector, vec2i_t camera_origin, vec2f_t camera_direction, int24_t min_x_screen, int24_t max_x_screen) {
    for (uint24_t i = 0; i < sector->num_walls; i++) {
        draw_wall(&sector->walls[i], camera_origin, camera_direction, min_x_screen, max_x_screen);
    }
}

void draw_wall(const struct Wall *wall, vec2i_t camera_origin, vec2f_t camera_direction, int24_t min_x_screen, int24_t max_x_screen) {
    struct WallDrawCall draw_call;

    vec2i_t corner_a_relative = relative_coord(camera_origin, camera_direction, wall->corner_a),
            corner_b_relative = relative_coord(camera_origin, camera_direction, wall->corner_b);
    
    if (!clip_wall(&corner_a_relative, &corner_b_relative, min_x_screen, max_x_screen)) return;

    project(&draw_call.p0, corner_a_relative);
    project(&draw_call.p1, corner_b_relative);
    
    if (wall->is_portal == 0) {
        gfx_SetColor(wall->color);
        draw_wall_2d(&draw_call);
    } else {
        draw_sector(wall->target_sector, camera_origin, camera_direction, draw_call.p0.x, draw_call.p1.x);
    }

    gfx_SetColor(224);
    gfx_FillCircle(draw_call.p0.x, HALF_SCREEN_HEIGHT + draw_call.p0.y, 2);
    gfx_FillCircle(draw_call.p0.x, HALF_SCREEN_HEIGHT - draw_call.p0.y, 2);
    gfx_SetColor(7);
    gfx_FillCircle(draw_call.p1.x, HALF_SCREEN_HEIGHT + draw_call.p1.y, 2);
    gfx_FillCircle(draw_call.p1.x, HALF_SCREEN_HEIGHT - draw_call.p1.y, 2);
}

vec2i_t relative_coord(vec2i_t origin, vec2f_t direction, vec2i_t absolute_coord) {
    vec2i_t translated_coord = vec2i(
        absolute_coord.x - origin.x,
        absolute_coord.y - origin.y);

    vec2i_t relative_coord = vec2i(
        translated_coord.x * direction.y - translated_coord.y * direction.x,
        translated_coord.y * direction.y + translated_coord.x * direction.x);

    return relative_coord;
}

bool intersect_points_line(vec2i_t *intersect, const vec2i_t point_a, const vec2i_t point_b, const int24_t k_num, const int24_t k_den) {
    int24_t num = point_b.x*point_a.y - point_a.x*point_b.y;
    int24_t den = k_den*(point_a.y - point_b.y) + k_num*(point_b.x - point_a.x);
    float q = (float)num / den;

    int24_t x_intersect = k_den * q;
    int24_t y_intersect = k_num * q;

    *intersect = vec2i(x_intersect, y_intersect);

    return y_intersect >= 0 && ((point_b.x >= x_intersect && x_intersect >= point_a.x) || (point_b.x <= x_intersect && x_intersect <= point_a.x));
}

bool clip_wall(vec2i_t *corner_a_relative, vec2i_t *corner_b_relative, int24_t min_x_screen, int24_t max_x_screen) {
    vec2i_t corner_a_relative_clipped = *corner_a_relative;
    vec2i_t corner_b_relative_clipped = *corner_b_relative;

    if (corner_a_relative_clipped.y < 0 && corner_b_relative_clipped.y < 0) {
        return false;
    }

    const int24_t min_x_perspective = min_x_screen - HALF_SCREEN_WIDTH,
                  max_x_perspective = max_x_screen - HALF_SCREEN_WIDTH;
    
    const bool a_over_max = PERSPECTIVE_SCALE*corner_a_relative_clipped.x > max_x_perspective*corner_a_relative_clipped.y,
               a_under_min = PERSPECTIVE_SCALE*corner_a_relative_clipped.x < min_x_perspective*corner_a_relative_clipped.y,
               b_over_max = PERSPECTIVE_SCALE*corner_b_relative_clipped.x > max_x_perspective*corner_b_relative_clipped.y,
               b_under_min = PERSPECTIVE_SCALE*corner_b_relative_clipped.x < min_x_perspective*corner_b_relative_clipped.y;

    if ((a_over_max && b_over_max) || (b_under_min && a_under_min)) {
        return false;
    }

    vec2i_t min_intersect, max_intersect;
    bool has_min_intersect = intersect_points_line(&min_intersect, corner_a_relative_clipped, corner_b_relative_clipped, PERSPECTIVE_SCALE, min_x_perspective);
    bool has_max_intersect = intersect_points_line(&max_intersect, corner_a_relative_clipped, corner_b_relative_clipped, PERSPECTIVE_SCALE, max_x_perspective);

    if (has_min_intersect && has_max_intersect) {
        if (corner_a_relative_clipped.x*abs(corner_b_relative_clipped.y) > corner_b_relative_clipped.x*abs(corner_a_relative_clipped.y)) {
            corner_a_relative_clipped = max_intersect;
            corner_b_relative_clipped = min_intersect;
        } else {
            corner_a_relative_clipped = min_intersect;
            corner_b_relative_clipped = max_intersect;
        }
    } else if (has_min_intersect) {
        if (a_over_max || a_under_min) {
            corner_a_relative_clipped = min_intersect;
        } else {
            corner_b_relative_clipped = min_intersect;
        }
    } else if (has_max_intersect) {
        if (a_over_max || a_under_min) {
            corner_a_relative_clipped = max_intersect;
        } else {
            corner_b_relative_clipped = max_intersect;
        }
    } else if (a_over_max || a_under_min || b_over_max || b_under_min) {
        return false;
    }

    if (corner_a_relative_clipped.x*corner_b_relative_clipped.y > corner_b_relative_clipped.x*corner_a_relative_clipped.y) {
        return false;
    }

    *corner_a_relative = corner_a_relative_clipped;
    *corner_b_relative = corner_b_relative_clipped;

    return true;
}

void project(vec2i_t *screen_coord, vec2i_t relative_coord) {
    screen_coord->x = relative_coord.x * PERSPECTIVE_SCALE / relative_coord.y + HALF_SCREEN_WIDTH;
    screen_coord->y = WALL_HEIGHT * PERSPECTIVE_SCALE / relative_coord.y;
}

// draws a paralellogram mirrored on the screen y axis
void draw_wall_2d(const struct WallDrawCall *draw_call) {
    int24_t x0 = draw_call->p0.x,
            y0 = draw_call->p0.y,
            x1 = draw_call->p1.x,
            y1 = draw_call->p1.y;

    // if all parts of the shape are higher than the screen height, just draw a rect
    if (y0 > HALF_SCREEN_HEIGHT && y1 > HALF_SCREEN_HEIGHT) {
        int24_t w = x1 - x0;
        if (w < 0) { x0 = x1; w = -w; }
        gfx_FillRectangle(x0, 0, w + 1, SCREEN_HEIGHT);
        return;
    }

    // h0 must be lower than h1 for the algorithm to work
    if (y0 > y1) {
        int24_t tmp;
        tmp = x0; x0 = x1; x1 = tmp;
        tmp = y0; y0 = y1; y1 = tmp;
    }

    // if it's infinitely thin, just draw a line
    if (x0 == x1) {
        gfx_VertLine(x1, HALF_SCREEN_HEIGHT - y1, 2 * y1);
        return;
    }
    
    int24_t dx = x1 - x0, dy = y1 - y0;
    int24_t dirx = 1;
    if (dx < 0) { dirx = -1; }
    dx *= dirx; // dx must be positive, dirx shows direction instead

    if (x1 < 0) { x1 = 0; }
    if (x1 > SCREEN_WIDTH - 1) { x1 = SCREEN_WIDTH - 1; }

    int24_t p = 2 * dy - dx;
    int24_t x = x0, y = y0;

    for (; x * dirx <= x1 * dirx; x += dirx) {
        while (p > 0 && y < HALF_SCREEN_HEIGHT) {
            y++;
            p -= 2 * dx;
        }
        p += 2 * dy;
        
        if (x < 0 || x > SCREEN_WIDTH - 1) continue;

        // if we are above/at the screen height, then the rest will be too and can be drawn as a rect
        if (y >= HALF_SCREEN_HEIGHT) {
            int24_t w = x1 - x;
            if (w < 0) { w = -w; x = x1; }
            gfx_FillRectangle_NoClip(x, 0, w + 1, SCREEN_HEIGHT);
            break;
        }

        gfx_VertLine_NoClip(x, HALF_SCREEN_HEIGHT - y, 2 * y);
    }
}