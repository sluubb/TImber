#include <ti/real.h>
#include <keypadc.h>
#include <graphx.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <debug.h>

#define SCREEN_WIDTH (GFX_LCD_WIDTH)
#define HALF_SCREEN_WIDTH (SCREEN_WIDTH / 2)
#define SCREEN_HEIGHT (GFX_LCD_HEIGHT)
#define HALF_SCREEN_HEIGHT (SCREEN_HEIGHT / 2)

#define TARGET_FRAMERATE (10)

#define MOVE_SPEED (300 / TARGET_FRAMERATE)
#define LOOK_SPEED (80 / TARGET_FRAMERATE)
#define COLLISION_RADIUS (100)
#define COLLISION_RADIUS_SQ (COLLISION_RADIUS * COLLISION_RADIUS)
#define CLIP_DISTANCE (50)

#define MAX_WALLS (6)
#define MAX_SECTORS (6)

// map inputs for computer with keyboard instead of for calculator
#define WASD_MAPPING

// input mappings
#ifdef WASD_MAPPING
#define KEY_LOOK_LEFT (kb_KeyLeft)
#define KEY_LOOK_RIGHT (kb_KeyRight)
#define KEY_MOVE_FORWARD (kb_KeySub)
#define KEY_MOVE_BACKWARD (kb_KeyLn)
#define KEY_STRAFE_LEFT (kb_KeyMath)
#define KEY_STRAFE_RIGHT (kb_KeyRecip)
#define KEY_ACTION (kb_KeyDel)
#else
#define KEY_LOOK_LEFT (kb_KeyLeft)
#define KEY_LOOK_RIGHT (kb_KeyRight)
#define KEY_MOVE_FORWARD (kb_Key8)
#define KEY_MOVE_BACKWARD (kb_Key5)
#define KEY_STRAFE_LEFT (kb_Key4)
#define KEY_STRAFE_RIGHT (kb_Key6)
#define KEY_ACTION (kb_KeyDel)
#endif

#define WALL_HEIGHT (100)

char * uint24_to_str(char * result, uint24_t value);

typedef struct Vector2i {
    int24_t x, y;
} vec2i_t;

vec2i_t vec2i(int24_t x, int24_t y) {
    vec2i_t v;
    v.x = x; v.y = y;
    return v;
}

typedef struct Vector2f {
    float x, y;
} vec2f_t;

vec2f_t vec2f(float x, float y) {
    vec2f_t v;
    v.x = x; v.y = y;
    return v;
}

typedef struct Wall {
    vec2i_t corner_a, corner_b;
    int24_t length_sq;
    uint24_t color;
    int24_t portal_target; // portal_target < 0 means it's not a portal
} wall_t;

struct Sector {
    uint24_t num_walls;
    wall_t walls[MAX_WALLS];
};
struct Sector Sectors[MAX_SECTORS];
uint24_t num_sectors;

struct Player {
    vec2i_t position;
    int24_t rotation;
    vec2f_t direction;
    uint24_t current_sector;
};
struct Player Player;

struct MathCache {
    float cos[360];
    float sin[360];
};
struct MathCache Math;

// pass t as a quotient for performance
// WARNING: high values may cause integer overflow
void lerpi(int24_t * value, int24_t target, int24_t t_num, int24_t t_den) {
    *value += (target - (*value)) * t_num / t_den;
}

void create_wall(struct Sector * sector, uint24_t i, vec2i_t corner_a, vec2i_t corner_b, int24_t portal_target) {
    vec2i_t diff = vec2i(corner_b.x - corner_a.x, corner_b.y - corner_a.y);
    int24_t length_sq = diff.x * diff.x + diff.y * diff.y;

    if (portal_target >= MAX_SECTORS) portal_target = -1;

    sector->walls[i].corner_a = corner_a;
    sector->walls[i].corner_b = corner_b;
    sector->walls[i].length_sq = length_sq;
    sector->walls[i].color = 4 + 2 * (diff.x * abs(diff.x) / length_sq); // TODO: actual color implementation
    sector->walls[i].portal_target = portal_target;
}

void create_sector(struct Sector * sector, uint8_t num_walls, vec2i_t * corners, int24_t * portal_target) {
    sector->num_walls = num_walls;
    if (num_walls == 0) return;

    uint24_t last = num_walls - 1;

    for (uint24_t i = 0; i < last; i++) {
        create_wall(sector, i, corners[i], corners[i + 1], portal_target[i]);
    }

    create_wall(sector, last, corners[last], corners[0], portal_target[last]);
}

void init() {
    // cache cos and sin values
    for (uint24_t a = 0; a < 360; a++) {
        Math.cos[a] = cosf(a / 180.0f * M_PI);
        Math.sin[a] = sinf(a / 180.0f * M_PI);
    }

    // init player
    Player.position = vec2i(0, 0); Player.rotation = 0; Player.current_sector = 0;

    // init sectors
    num_sectors = 2;

    vec2i_t corners0[] = {
        {-400,  400},
        { 0  ,  400},
        { 400,  400},
        { 400, -400},
        {-400, -400}
    };
    int24_t portal_targets0[] = {-1, 1, -1, -1, -1};
    create_sector(&Sectors[0], 5, corners0, portal_targets0);

    vec2i_t corners1[] = {
        { 0  ,  400},
        { 0  ,  600},
        { 400,  600},
        { 400,  400}
    };
    int24_t portal_targets1[] = {-1, -1, -1, 0};
    create_sector(&Sectors[1], 4, corners1, portal_targets1);
}

// draws a paralellogram mirrored on the screen y axis
void draw_wall_2d(int24_t x0, int24_t x1, uint24_t y0, uint24_t y1) {
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

bool project_wall(vec2i_t * screen_a, vec2i_t * screen_b, struct Wall wall, int24_t min_x, int24_t max_x) {
    // translate coords
    vec2i_t corner_a_translated = vec2i(wall.corner_a.x - Player.position.x, wall.corner_a.y - Player.position.y);
    vec2i_t corner_b_translated = vec2i(wall.corner_b.x - Player.position.x, wall.corner_b.y - Player.position.y);

    // rotate coords
    vec2i_t corner_a_local = vec2i(corner_a_translated.x * Player.direction.y - corner_a_translated.y * Player.direction.x, corner_a_translated.y * Player.direction.y + corner_a_translated.x * Player.direction.x);
    vec2i_t corner_b_local = vec2i(corner_b_translated.x * Player.direction.y - corner_b_translated.y * Player.direction.x, corner_b_translated.y * Player.direction.y + corner_b_translated.x * Player.direction.x);

    // don't draw if fully behind the player
    if (corner_a_local.y < 1 && corner_b_local.y < 1) return true;

    // weird shit happens with y coords close to or under zero so we avoid that by clipping
    if (corner_a_local.y < CLIP_DISTANCE) {
        lerpi(&corner_a_local.x, corner_b_local.x, CLIP_DISTANCE - corner_a_local.y, corner_b_local.y - corner_a_local.y);
        corner_a_local.y = CLIP_DISTANCE;
    } else if (corner_b_local.y < CLIP_DISTANCE) {
        lerpi(&corner_b_local.x, corner_a_local.x, CLIP_DISTANCE - corner_b_local.y, corner_a_local.y - corner_b_local.y);
        corner_b_local.y = CLIP_DISTANCE;
    }

    // screen x coords
    screen_a->x = corner_a_local.x * 200 / corner_a_local.y + HALF_SCREEN_WIDTH;
    screen_b->x = corner_b_local.x * 200 / corner_b_local.y + HALF_SCREEN_WIDTH;

    // don't draw if viewed from the wrong side
    if (screen_a->x > screen_b->x) return true;

    // don't draw if the wall is completely outside the set bounds
    if (screen_b->x < min_x || screen_a->x > max_x) {
        return true;
    }

    // height on screen
    screen_a->y = WALL_HEIGHT * 200 / corner_a_local.y;
    screen_b->y = WALL_HEIGHT * 200 / corner_b_local.y;

    // clip screen x coords so that they are within minx and maxx
    if (screen_a->x < min_x) {
        lerpi(&screen_a->y, screen_b->y, min_x - screen_a->x, screen_b->x - screen_a->x);
        screen_a->x = min_x;
    }
    if (screen_b->x > max_x) {
        lerpi(&screen_b->y, screen_a->y, max_x - screen_b->x, screen_a->x - screen_b->x);
        screen_b->x = max_x;
    }

    return false;
}

void draw_sector(uint24_t i, int24_t min_x, int24_t max_x) {
    uint24_t last = Sectors[i].num_walls - 1;

    vec2i_t screen_a, screen_a;

    for (uint24_t j = 0; j < last; j++) {
        if (project_wall(&screen_a, &screen_a, Sectors[i].walls[j], min_x, max_x)) continue;

        if (Sectors[i].walls[j].portal_target < 0) {
            gfx_SetColor(Sectors[i].walls[j].color);
            draw_wall_2d(screen_a.x, screen_a.x, screen_a.y, screen_a.y);
        } else {
            draw_sector(Sectors[i].walls[j].portal_target, screen_a.x, screen_a.x);
        }
    }
    
    if (project_wall(&screen_a, &screen_a, Sectors[i].walls[last], min_x, max_x)) return;

    if (Sectors[i].walls[last].portal_target < 0) {
        gfx_SetColor(Sectors[i].walls[last].color);
        draw_wall_2d(screen_a.x, screen_a.x, screen_a.y, screen_a.y);
    } else {
        draw_sector(Sectors[i].walls[last].portal_target, screen_a.x, screen_a.x);
    }
}

void move() {
    if (kb_IsDown(KEY_LOOK_LEFT)) { Player.rotation -= LOOK_SPEED; if (Player.rotation < 0) { Player.rotation += 360; } }
    if (kb_IsDown(KEY_LOOK_RIGHT)) { Player.rotation += LOOK_SPEED; if (Player.rotation > 359) { Player.rotation -= 360; } }

    uint24_t i = (uint24_t)(Player.rotation);
    Player.direction.x = Math.sin[i];
    Player.direction.y = Math.cos[i];
    vec2i_t diff = vec2i(Player.direction.x * MOVE_SPEED, Player.direction.y * MOVE_SPEED);

    if (kb_IsDown(KEY_MOVE_FORWARD)) { Player.position.x += diff.x; Player.position.y += diff.y; }
    if (kb_IsDown(KEY_MOVE_BACKWARD)) { Player.position.x -= diff.x; Player.position.y -= diff.y; }
    if (kb_IsDown(KEY_STRAFE_LEFT)) { Player.position.x -= diff.y; Player.position.y += diff.x; }
    if (kb_IsDown(KEY_STRAFE_RIGHT)) { Player.position.x += diff.y; Player.position.y -= diff.x; }
}

void collide_wall(vec2i_t corner_a, vec2i_t corner_b, int24_t portal_target) {
    if (portal_target >= 0) {
        if ((corner_a.y - Player.position.y) * (corner_b.x - Player.position.x) + -(corner_a.x - Player.position.x) * (corner_b.y - Player.position.y) < 0) {
            Player.current_sector = portal_target;
        }

        return;
    }

    vec2i_t ab = vec2i(corner_b.x - corner_a.x, corner_b.y - corner_a.y);
    vec2i_t ap = vec2i(Player.position.x - corner_a.x, Player.position.y - corner_a.y);
    int24_t t_num = ab.x*ap.x + ab.y*ap.y, t_den = ab.x*ab.x + ab.y*ab.y;
    if (t_num > t_den) t_num = t_den; if (t_num < 0) t_num = 0;
    vec2i_t q = vec2i(corner_a.x + ab.x * t_num / t_den, corner_a.y + ab.y * t_num / t_den); // closest point on wall

    vec2i_t qp = vec2i(q.x - Player.position.x, q.y - Player.position.y);
    int24_t dist_sq = qp.x*qp.x + qp.y*qp.y;
    if (dist_sq < COLLISION_RADIUS_SQ) {
        int24_t dist = sqrtf(dist_sq);
        int24_t error = COLLISION_RADIUS - dist;
        Player.position.x -= qp.x * error / dist;
        Player.position.y -= qp.y * error / dist;
    }
}

void collide() {
    struct Sector s = Sectors[Player.current_sector];
    if (s.num_walls == 0) return;

    uint24_t last = s.num_walls - 1;

    for (uint24_t i = 0; i < last; i++) {
        collide_wall(s.walls[i].corner_a, s.walls[i+1].corner_a, s.walls[i].portal_target);
    }
    collide_wall(s.walls[last].corner_a, s.walls[0].corner_a, s.walls[last].portal_target);
}

int main(void)
{
    init();

    gfx_Begin();
    gfx_SetDrawBuffer();

    gfx_SetTextBGColor(0);
    gfx_SetTextFGColor(22);
    gfx_SetTextScale(1, 1);

    uint24_t fps = 60;
    clock_t frame_start;

    do {
        frame_start = clock();

        gfx_FillScreen(0);
        draw_sector(Player.current_sector, 0, SCREEN_WIDTH-1);

        // draw framerate text
        char framerate_str[8];
        uint24_to_str(framerate_str, (uint24_t)(fps + 0.5));
        gfx_PrintStringXY(
            framerate_str,
            GFX_LCD_WIDTH - gfx_GetStringWidth(framerate_str) - 10, 10
            );

        gfx_SwapDraw();

        kb_Scan();
        move();
        collide();

        do {
            fps = CLOCKS_PER_SEC / (clock() - frame_start);
        } while (fps > TARGET_FRAMERATE);
    } while (!kb_IsDown(kb_KeyClear));

    gfx_End();

    return 0;
}

char * uint24_to_str(char * result, uint24_t value) {
    char * ptr = result, * ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= 10;
        *ptr++ = "0123456789" [tmp_value - value * 10];
    } while ( value );

    *ptr-- = '\0';
  
    // reverse the string
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }

    return result;
}