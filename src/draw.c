#include "draw.h"

void draw_sector(const struct Sector *sector, vec2i_t camera_orig, vec2f_t camera_dir, int24_t min_x_screen, int24_t max_x_screen) {
    for (uint24_t i = 0; i < sector->num_walls; i++) {
        draw_wall(&sector->walls[i], camera_orig, camera_dir, min_x_screen, max_x_screen);
    }
}

void draw_wall(const struct Wall *wall, vec2i_t camera_orig, vec2f_t camera_dir, int24_t min_x_screen, int24_t max_x_screen) {
    vec2i_t edge_a_rel = relative_coord(camera_orig, camera_dir, wall->corner_a),
            edge_b_rel = relative_coord(camera_orig, camera_dir, wall->corner_b);
    
    if (!clip_wall(&edge_a_rel, &edge_b_rel, min_x_screen, max_x_screen)) return;

    struct WallDrawCall draw_call;
    project(&draw_call.edge_a_screen, edge_a_rel);
    project(&draw_call.edge_b_screen, edge_b_rel);
    
    if (wall->is_portal == 0) {
        gfx_SetColor(wall->color);
        draw_wall_2d(&draw_call);
    } else {
        draw_sector(wall->target_sector, camera_orig, camera_dir, draw_call.edge_a_screen.x, draw_call.edge_b_screen.x);
    }

    gfx_SetColor(224);
    gfx_FillCircle(draw_call.edge_a_screen.x, HALF_SCREEN_HEIGHT + draw_call.edge_a_screen.y, 2);
    gfx_FillCircle(draw_call.edge_a_screen.x, HALF_SCREEN_HEIGHT - draw_call.edge_a_screen.y, 2);
    gfx_SetColor(7);
    gfx_FillCircle(draw_call.edge_b_screen.x, HALF_SCREEN_HEIGHT + draw_call.edge_b_screen.y, 2);
    gfx_FillCircle(draw_call.edge_b_screen.x, HALF_SCREEN_HEIGHT - draw_call.edge_b_screen.y, 2);
}

vec2i_t relative_coord(vec2i_t orig, vec2f_t dir, vec2i_t p_abs) {
    vec2i_t p_trans = vec2i(
        p_abs.x - orig.x,
        p_abs.y - orig.y);

    vec2i_t p_rel = vec2i(
        p_trans.x * dir.y - p_trans.y * dir.x,
        p_trans.y * dir.y + p_trans.x * dir.x);

    return p_rel;
}

bool intersect_points_line(vec2i_t *intersect, const vec2i_t p_a, const vec2i_t p_b, const int24_t k_num, const int24_t k_den) {
    int24_t num = p_b.x*p_a.y - p_a.x*p_b.y;
    int24_t den = k_den*(p_a.y - p_b.y) + k_num*(p_b.x - p_a.x);
    float q = (float)num / den;

    int24_t x_intersect = k_den * q;
    int24_t y_intersect = k_num * q;

    *intersect = vec2i(x_intersect, y_intersect);

    return y_intersect >= 0 && ((p_b.x >= x_intersect && x_intersect >= p_a.x) || (p_b.x <= x_intersect && x_intersect <= p_a.x));
}

bool clip_wall(vec2i_t *edge_a_rel, vec2i_t *edge_b_rel, int24_t min_x_screen, int24_t max_x_screen) {
    vec2i_t edge_a_rel_clip = *edge_a_rel;
    vec2i_t edge_b_rel_clip = *edge_b_rel;

    if (edge_a_rel_clip.y < 0 && edge_b_rel_clip.y < 0) {
        return false;
    }

    const int24_t min_x_perspective = min_x_screen - HALF_SCREEN_WIDTH,
                  max_x_perspective = max_x_screen - HALF_SCREEN_WIDTH;
    
    const bool a_over_max = PERSPECTIVE_SCALE*edge_a_rel_clip.x > max_x_perspective*edge_a_rel_clip.y,
               a_under_min = PERSPECTIVE_SCALE*edge_a_rel_clip.x < min_x_perspective*edge_a_rel_clip.y,
               b_over_max = PERSPECTIVE_SCALE*edge_b_rel_clip.x > max_x_perspective*edge_b_rel_clip.y,
               b_under_min = PERSPECTIVE_SCALE*edge_b_rel_clip.x < min_x_perspective*edge_b_rel_clip.y;

    if ((a_over_max && b_over_max) || (b_under_min && a_under_min)) {
        return false;
    }

    vec2i_t min_intersect, max_intersect;
    bool has_min_intersect = intersect_points_line(&min_intersect, edge_a_rel_clip, edge_b_rel_clip, PERSPECTIVE_SCALE, min_x_perspective);
    bool has_max_intersect = intersect_points_line(&max_intersect, edge_a_rel_clip, edge_b_rel_clip, PERSPECTIVE_SCALE, max_x_perspective);

    if (has_min_intersect && has_max_intersect) {
        if (edge_a_rel_clip.x*abs(edge_b_rel_clip.y) > edge_b_rel_clip.x*abs(edge_a_rel_clip.y)) {
            edge_a_rel_clip = max_intersect;
            edge_b_rel_clip = min_intersect;
        } else {
            edge_a_rel_clip = min_intersect;
            edge_b_rel_clip = max_intersect;
        }
    } else if (has_min_intersect) {
        if (a_over_max || a_under_min) {
            edge_a_rel_clip = min_intersect;
        } else {
            edge_b_rel_clip = min_intersect;
        }
    } else if (has_max_intersect) {
        if (a_over_max || a_under_min) {
            edge_a_rel_clip = max_intersect;
        } else {
            edge_b_rel_clip = max_intersect;
        }
    } else if (a_over_max || a_under_min || b_over_max || b_under_min) {
        return false;
    }

    if (edge_a_rel_clip.x*edge_b_rel_clip.y > edge_b_rel_clip.x*edge_a_rel_clip.y) {
        return false;
    }

    *edge_a_rel = edge_a_rel_clip;
    *edge_b_rel = edge_b_rel_clip;

    return true;
}

void project(vec2i_t *p_screen, vec2i_t p_rel) {
    p_screen->x = p_rel.x * PERSPECTIVE_SCALE / p_rel.y + HALF_SCREEN_WIDTH;
    p_screen->y = WALL_HEIGHT * PERSPECTIVE_SCALE / p_rel.y;
}

// draws a paralellogram mirrored on the screen y axis
void draw_wall_2d(const struct WallDrawCall *draw_call) {
    int24_t x0 = draw_call->edge_a_screen.x,
            y0 = draw_call->edge_a_screen.y,
            x1 = draw_call->edge_b_screen.x,
            y1 = draw_call->edge_b_screen.y;

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