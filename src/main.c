#include <ti/real.h>
#include <keypadc.h>
#include <graphx.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <debug.h>

#include "vector.h"
#include "utils.h"
#include "map.h"
#include "draw.h"

#define TARGET_FRAMERATE (20)

#define MOVE_SPEED (300 / TARGET_FRAMERATE)
#define LOOK_SPEED (80 / TARGET_FRAMERATE)
#define COLLISION_RADIUS (30)
#define COLLISION_RADIUS_SQ (COLLISION_RADIUS * COLLISION_RADIUS)

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

struct Player {
    vec2i_t position;
    int24_t rotation;
    vec2f_t direction;
    const struct Sector *current_sector;
}; struct Player Player;

struct Sector Sectors[MAX_SECTORS];
uint24_t num_sectors;

struct MathCache {
    float cos[360];
    float sin[360];
}; struct MathCache Math;

void init() {
    // cache cos and sin values
    for (uint24_t a = 0; a < 360; a++) {
        Math.cos[a] = cosf(a / 180.0f * M_PI);
        Math.sin[a] = sinf(a / 180.0f * M_PI);
    }

    // init sectors
    num_sectors = 2;

    const vec2i_t corners0[] = {
        {-400,  400},
        { 0  ,  400},
        { 400,  400},
        { 400, -400},
        {-400, -400}
    };
    create_sector(&Sectors[0], 5, corners0);

    const vec2i_t corners1[] = {
        { 0  ,  400},
        { 0  ,  600},
        { 400,  600},
        { 400,  400}
    };
    create_sector(&Sectors[1], 4, corners1);

    init_portal(&Sectors[0].walls[1], &Sectors[1], 3);

    dbg_sprintf(dbgout, "%d", Sectors[0].walls[1].target_wall_i);

    // init player
    Player.position = vec2i(0, 0); Player.rotation = 0; Player.current_sector = &Sectors[0];
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

void collide_wall(const struct Wall *wall) {
    if (wall->is_portal == 0) {
        vec2i_t ab = vec2i(wall->corner_b.x - wall->corner_a.x, wall->corner_b.y - wall->corner_a.y);
        vec2i_t ap = vec2i(Player.position.x - wall->corner_a.x, Player.position.y - wall->corner_a.y);
        float t = (float)(ab.x*ap.x + ab.y*ap.y) / (ab.x*ab.x + ab.y*ab.y);
        if (t > 1.) t = 1.; if (t < 0.) t = 0.;
        vec2i_t q = vec2i(wall->corner_a.x + ab.x * t, wall->corner_a.y + ab.y * t); // closest point on wall

        vec2i_t pq = vec2i(q.x - Player.position.x, q.y - Player.position.y);
        int24_t dist_sq = pq.x*pq.x + pq.y*pq.y;
        if (dist_sq < COLLISION_RADIUS_SQ) {
            int24_t dist = sqrtf(dist_sq);
            int24_t error = COLLISION_RADIUS - dist;
            Player.position.x -= pq.x * error / dist;
            Player.position.y -= pq.y * error / dist;
        }

        return;
    }

    if ((wall->corner_a.y - Player.position.y) * (wall->corner_b.x - Player.position.x) + -(wall->corner_a.x - Player.position.x) * (wall->corner_b.y - Player.position.y) < 0) {
        Player.current_sector = wall->target_sector;
    }
}

void collide() {
    for (uint24_t i = 0; i < Player.current_sector->num_walls; i++) {
        collide_wall(&Player.current_sector->walls[i]);
    }
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

        draw_sector(Player.current_sector, Player.position, Player.direction, 0, SCREEN_WIDTH-1);

        // crosshair
        gfx_SetColor(255);
        gfx_FillCircle(HALF_SCREEN_WIDTH, HALF_SCREEN_HEIGHT, 1);

        // framerate text
        char framerate_str[8];
        uint24_to_str(framerate_str, fps);
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