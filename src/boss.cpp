//boss.cpp
#include "boss.hpp"
#include "AEEngine.h"
#include "player.hpp"
#include "enemy.hpp"  
#include <math.h>
#include <stdlib.h>


#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1600
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 900
#endif


int boss_defeated = 0;  // Flag for victory screen
extern int depth;
extern int rocks_mined;

// ---------------------------------------------------------------------------
// EXTERN GLOBALS FROM main.cpp
extern float player_x, player_y, player_width, player_height;
extern float camera_x, camera_y;
extern int   tileMap[MAP_HEIGHT][MAP_WIDTH];

int  CheckCollisionRectangle(float x1, float y1, float w1, float h1,
    float x2, float y2, float w2, float h2);
int  CheckTileCollision(float x, float y);
void GetTileAtPosition(float wx, float wy, int* out_row, int* out_col);
int  IsTileValid(int row, int col);
AEGfxVertexList* CreateTextureMesh(float width, float height);
AEGfxVertexList* CreateRectangleMesh(float width, float height, unsigned int color);

// ---------------------------------------------------------------------------
// FORWARD DECLARATIONS
static void BossFireRockBurstFrom(Boss* b, float ox, float oy);
static void BossFireFireballBurstFrom(Boss* b, float ox, float oy);
static void StopGravityPull();

// ---------------------------------------------------------------------------
// SANITY SYSTEM
float sanity_percentage = 100.0f;
float sanity_max = 100.0f;

// ---------------------------------------------------------------------------
// GRAVITY PULL FORCE (read by main.cpp UpdatePhysics)
float g_boss_pull_force_x = 0.0f;
float g_boss_pull_force_y = 0.0f;

// ---------------------------------------------------------------------------
// GRAVITY PULL CONSTANTS
#define PULL_RADIUS        350.0f
#define PULL_STRENGTH      180.0f
#define PULL_DAMAGE_RATE    6.0f
#define PULL_DURATION       2.5f

static float pull_damage_timer = 0.0f;
static float pull_ring_alpha = 0.0f;

// ---------------------------------------------------------------------------
// BLINK DASH STATE
#define BLINK_ATTEMPTS        30
#define BLINK_MIN_DIST       200.0f
#define BLINK_MAX_DIST       500.0f
#define BLINK_POSTFIRE_DELAY   0.25f

static float blink_postfire_timer = 0.0f;
static int   blink_rocks_fired = 0;
static int   blink_flash_active = 0;
static float blink_flash_timer = 0.0f;

// ---------------------------------------------------------------------------
// BOSS GLOBALS
Boss g_boss;
AEGfxTexture* g_bossTexture = NULL;
AEGfxTexture* g_bRockTexture = NULL;
AEGfxTexture* g_bfireballTexture = NULL;

static AEGfxVertexList* g_bossMesh = NULL;
static AEGfxVertexList* g_bRockMesh = NULL;
static AEGfxVertexList* g_bfireballMesh = NULL;

// ---------------------------------------------------------------------------
// BOSS HEALTH BAR MESHES
static AEGfxVertexList* g_healthBarBgMesh = NULL;
static AEGfxVertexList* g_healthBarFgMesh = NULL;

// =========================================================================
// VICTORY SCREEN DEBUG LOGGING
static float victory_screen_timer = 0.0f;
static int victory_screen_log_printed = 0;

void LogVictoryState(void)
{
    static int frame_counter = 0;
    frame_counter++;

    // Log every 30 frames (approximately once per half second at 60fps)
    if (frame_counter % 30 == 0)
    {
        printf("[VICTORY LOG - Frame %d]\n", frame_counter);
        printf("  boss_defeated: %d\n", boss_defeated);
        printf("  g_boss.active: %d\n", g_boss.active);
        printf("  g_boss.health: %.1f / %.1f\n", g_boss.health, g_boss.max_health);
        printf("  Victory screen timer: %.2f seconds\n", victory_screen_timer);
        printf("\n");
    }
}

// =========================================================================

// ---------------------------------------------------------------------------
// GRAVITY PULL HELPERS

static void UpdateGravityPull(Boss* b, float dt)
{
    float dx = b->x - player_x;
    float dy = b->y - player_y;
    float dist = sqrtf(dx * dx + dy * dy);

    // Fade ring in
    pull_ring_alpha += dt * 3.0f;
    if (pull_ring_alpha > 1.0f) pull_ring_alpha = 1.0f;

    // Only pull if player is inside the ring
    if (dist <= PULL_RADIUS && dist > 1.0f)
    {
        g_boss_pull_force_x = (dx / dist) * PULL_STRENGTH;
        g_boss_pull_force_y = (dy / dist) * PULL_STRENGTH;

        pull_damage_timer -= dt;
        if (pull_damage_timer <= 0.0f)
        {
            sanity_percentage -= PULL_DAMAGE_RATE * dt;
            if (sanity_percentage < 0.0f) sanity_percentage = 0.0f;
            pull_damage_timer = 0.05f;
        }
    }
    else
    {
        // Outside radius — no pull, no damage
        g_boss_pull_force_x = 0.0f;
        g_boss_pull_force_y = 0.0f;
    }
}

static void StopGravityPull()
{
    g_boss_pull_force_x = 0.0f;
    g_boss_pull_force_y = 0.0f;
    pull_damage_timer = 0.0f;
    pull_ring_alpha = 0.0f;
}

// ---------------------------------------------------------------------------
// BLINK DASH HELPERS

static int BossBlinkToRandomSpot(Boss* b)
{
    for (int attempt = 0; attempt < BLINK_ATTEMPTS; attempt++)
    {
        float angle = ((float)(rand() % 360)) * (3.14159f / 180.0f);
        float dist = BLINK_MIN_DIST +
            ((float)(rand() % (int)(BLINK_MAX_DIST - BLINK_MIN_DIST)));

        float target_x = player_x + cosf(angle) * dist;
        float target_y = player_y + sinf(angle) * dist;

        float half_w = (MAP_WIDTH * TILE_SIZE) / 2.0f;
        float half_h = (MAP_HEIGHT * TILE_SIZE) / 2.0f;
        if (target_x < -half_w + b->width || target_x > half_w - b->width)  continue;
        if (target_y < -half_h + b->height || target_y > half_h - b->height) continue;

        if (CheckTileCollision(target_x, target_y)) continue;

        b->x = target_x;
        b->y = target_y;
        return 1;
    }
    return 0;
}

static void UpdateBlinkDash(Boss* b, float dt)
{
    // 0.0 - 0.3s : windup
    // 0.3s       : teleport
    // 0.3 + 0.25s: fire rock burst
    // 1.5s       : end

    if (b->state_timer >= 0.3f && b->state_timer - dt < 0.3f)
    {
        BossBlinkToRandomSpot(b);
        blink_rocks_fired = 0;
        blink_postfire_timer = 0.0f;
        blink_flash_active = 1;
        blink_flash_timer = 0.15f;
    }

    if (b->state_timer >= 0.3f && !blink_rocks_fired)
    {
        blink_postfire_timer += dt;
        if (blink_postfire_timer >= BLINK_POSTFIRE_DELAY)
        {
            BossFireRockBurstFrom(b, b->x, b->y);
            blink_rocks_fired = 1;
        }
    }

    if (blink_flash_active)
    {
        blink_flash_timer -= dt;
        if (blink_flash_timer <= 0.0f)
            blink_flash_active = 0;
    }
}

// ---------------------------------------------------------------------------
// ROCK BURST

static void BossFireRockBurstFrom(Boss* b, float ox, float oy)
{
    float base_speed = 600.0f;
    int   slot = 0;
    for (int i = 0; i < 6; i++)
    {
        while (slot < MAX_BOSS_ROCKS && b->proj_rocks[slot].active) slot++;
        if (slot >= MAX_BOSS_ROCKS) break;

        float angle = (3.14159f * 2.0f / 6.0f) * i;
        b->proj_rocks[slot].x = ox;
        b->proj_rocks[slot].y = oy;
        b->proj_rocks[slot].spawn_x = ox;
        b->proj_rocks[slot].spawn_y = oy;
        b->proj_rocks[slot].vx = cosf(angle) * base_speed;
        b->proj_rocks[slot].vy = sinf(angle) * base_speed;
        b->proj_rocks[slot].lifetime = 3.0f;
        b->proj_rocks[slot].active = 1;
        slot++;
    }
}

// ---------------------------------------------------------------------------
// FIREBALL BURST (fires fireballs in all 8 directions, twice)

static void BossFireFireballBurstFrom(Boss* b, float ox, float oy)
{
    float base_speed = 250.0f;
    int   slot = 0;

    // Fire 8 fireballs in all directions
    for (int i = 0; i < 8; i++)
    {
        while (slot < MAX_BOSS_FIREBALLS && b->proj_fireballs[slot].active) slot++;
        if (slot >= MAX_BOSS_FIREBALLS) break;

        float angle = (3.14159f * 2.0f / 8.0f) * i;
        b->proj_fireballs[slot].x = ox;
        b->proj_fireballs[slot].y = oy;
        b->proj_fireballs[slot].vx = cosf(angle) * base_speed;
        b->proj_fireballs[slot].vy = sinf(angle) * base_speed;
        b->proj_fireballs[slot].lifetime = 4.0f;
        b->proj_fireballs[slot].active = 1;
        slot++;
    }
}

static void UpdateFireballBurst(Boss* b, float dt)
{
    // Fireball attack sequence:
    // 0.0 - 0.5s: windup
    // 0.5s: fire first burst
    // 0.5 - 1.0s: wind up for second burst
    // 1.0s: fire second burst
    // 1.5s: end attack

    if (b->state_timer >= 0.5f && b->state_timer - dt < 0.5f)
    {
        BossFireFireballBurstFrom(b, b->x, b->y);
    }

    if (b->state_timer >= 1.0f && b->state_timer - dt < 1.0f)
    {
        BossFireFireballBurstFrom(b, b->x, b->y);
    }
}

// =========================================================================== 
// FINAL MOVE FIREBALL BURST (fireballs home to player, then explode into 5x projectiles)

static void BossFireFinalMoveFireballBurstFrom(Boss* b, float ox, float oy)
{
    float base_speed = 250.0f;
    int   slot = 0;

    // Fire 8 homing fireballs that will explode on proximity
    for (int i = 0; i < 24; i++)
    {
        while (slot < MAX_BOSS_FIREBALLS && b->proj_fireballs[slot].active) slot++;
        if (slot >= MAX_BOSS_FIREBALLS) break;

        float angle = (3.14159f * 2.0f / 24.0f) * i;

        b->proj_fireballs[slot].x = ox;
        b->proj_fireballs[slot].y = oy;
        b->proj_fireballs[slot].vx = cosf(angle) * base_speed;
        b->proj_fireballs[slot].vy = sinf(angle) * base_speed;
        b->proj_fireballs[slot].lifetime = 6.0f;
        b->proj_fireballs[slot].active = 1;
        slot++;
    }
}

static void UpdateFinalMove(Boss* b, float dt)
{
    // Final move attack sequence with MORE bursts:
    // 0.0 - 0.4s: windup
    // 0.4s: fire first burst
    // 0.8s: fire second burst
    // 1.2s: fire third burst
    // 1.6s: fire fourth burst (NEW)
    // 2.0s: fire fifth burst (NEW)
    // 2.5s: end attack

    if (b->state_timer >= 0.4f && b->state_timer - dt < 0.4f)
    {
        BossFireFinalMoveFireballBurstFrom(b, b->x, b->y);
    }

    if (b->state_timer >= 0.8f && b->state_timer - dt < 0.8f)
    {
        BossFireFinalMoveFireballBurstFrom(b, b->x, b->y);
    }

    if (b->state_timer >= 1.2f && b->state_timer - dt < 1.2f)  // ← NEW
    {
        BossFireFinalMoveFireballBurstFrom(b, b->x, b->y);
    }

    if (b->state_timer >= 1.6f && b->state_timer - dt < 1.6f)
    {
        BossFireFinalMoveFireballBurstFrom(b, b->x, b->y);
    }

    if (b->state_timer >= 2.0f && b->state_timer - dt < 2.0f)  // ← NEW
    {
        BossFireFinalMoveFireballBurstFrom(b, b->x, b->y);
    }
}
// ---------------------------------------------------------------------------
void InitBoss()
{
    //float spawn_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;

    g_boss.x = 0.0f;
    g_boss.y = 0.0f;
    g_boss.width = 100.0f;
    g_boss.height = 100.0f;
    g_boss.active = 1;
    g_boss.state = BOSS_IDLE;
    g_boss.state_timer = 0.0f;
    g_boss.attack_cooldown = 2.0f;
    g_boss.wander_dir_x = 1.0f;
    g_boss.wander_dir_y = 0.0f;
    g_boss.wander_timer = 0.0f;
    g_boss.charge_speed = 0.0f;
    g_boss.contact_damage_cooldown = 0.0f;
    g_boss.max_health = 100.0f;
    g_boss.health = g_boss.max_health;

    for (int i = 0; i < MAX_BOSS_ROCKS; i++)
        g_boss.proj_rocks[i].active = 0;

    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
        g_boss.proj_fireballs[i].active = 0;

    StopGravityPull();
    blink_rocks_fired = 0;
    blink_flash_active = 0;
    blink_flash_timer = 0.0f;
    g_boss.final_move_used = 0;

    // Create health bar meshes with proper dimensions
    g_healthBarBgMesh = CreateRectangleMesh(150.0f, 12.0f, 0xFF222222);
    g_healthBarFgMesh = CreateRectangleMesh(150.0f, 12.0f, 0xFF00FF00);

    // Ensure the mesh is created AFTER checking texture is loaded
    g_bossMesh = CreateTextureMesh(g_boss.width, g_boss.height);
    g_bRockMesh = CreateTextureMesh(20.0f, 20.0f);
    g_bfireballMesh = CreateTextureMesh(30.0f, 30.0f);
}

// ---------------------------------------------------------------------------
static void BossPickNextAttack(Boss* b)
{
    int roll = rand() % 5;  // Increased from 4 to 5 for new attack
    switch (roll)
    {
    case 0: b->state = BOSS_CHARGE;         break;
    case 1: b->state = BOSS_ROCK_BURST;     break;
    case 2: b->state = BOSS_GRAVITY_PULL;   break;
    case 3: b->state = BOSS_BLINK_DASH;     break;
        //case 4: b->state = BOSS_FIREBALL_BURST; break;
    }
    b->state_timer = 0.0f;
}

// ---------------------------------------------------------------------------
void UpdateBoss(float dt)
{
    if (!g_boss.active) return;
    Boss* b = &g_boss;

    // ========== PLAYER ATTACK DAMAGE CHECK ==========
    // Check if player hit the boss
    if (player_attack_state.attack_hit_boss)
    {
        b->health -= PLAYER_ATTACK_DAMAGE;
        // Debug output
        printf(">>> BOSS HIT! Damage: %.1f | Boss HP: %.1f / %.1f (%.1f%%)\n",
            PLAYER_ATTACK_DAMAGE,
            b->health,
            b->max_health,
            (b->health / b->max_health) * 100.0f);

        // Clamp health to 0 minimum
        if (b->health < 0.0f)
        {
            b->health = 0.0f;
        }

        // Check if boss is dead
        if (b->health <= 0.0f)
        {
            b->active = 0;
            boss_defeated = 1;  // Set victory flag
            victory_screen_timer = 0.0f;  // Reset victory timer
            victory_screen_log_printed = 0;  // Reset log flag
            printf("========== BOSS DEFEATED! ==========\n");
        }
        player_attack_state.attack_hit_boss = 0;
    }

    // Check if player hit an enemy
    if (player_attack_state.attack_hit_enemy >= 0)
    {
        Enemy_Take_Damage(player_attack_state.attack_hit_enemy, PLAYER_ATTACK_DAMAGE);
        printf("Enemy %d hit! Damage: %.1f\n",
            player_attack_state.attack_hit_enemy,
            PLAYER_ATTACK_DAMAGE);
        player_attack_state.attack_hit_enemy = -1;
    }
    // ================================================

    b->state_timer += dt;
    b->attack_cooldown -= dt;
    b->contact_damage_cooldown -= dt;
    if (b->contact_damage_cooldown < 0.0f)
        b->contact_damage_cooldown = 0.0f;

    // Passive contact damage — always on
    if (CheckCollisionRectangle(b->x, b->y, b->width, b->height,
        player_x, player_y, player_width, player_height))
    {
        if (b->contact_damage_cooldown <= 0.0f)
        {
            sanity_percentage -= 10.0f;
            if (sanity_percentage < 0.0f) sanity_percentage = 0.0f;
            b->contact_damage_cooldown = 0.5f;
        }
    }

    // Projectile rocks — always update
    for (int i = 0; i < MAX_BOSS_ROCKS; i++)
    {
        BossRock* r = &b->proj_rocks[i];
        if (!r->active) continue;

        r->lifetime -= dt;
        if (r->lifetime <= 0.0f) { r->active = 0; continue; }

        r->x += r->vx * dt;
        r->y += r->vy * dt;

        int rock_row, rock_col;
        GetTileAtPosition(r->x, r->y, &rock_row, &rock_col);
        if (IsTileValid(rock_row, rock_col) &&
            tileMap[rock_row][rock_col] == 2)
        {
            r->active = 0;
            continue;
        }

        float speed = sqrtf(r->vx * r->vx + r->vy * r->vy);
        float damage = 5.0f + (speed / 300.0f) * 25.0f;

        if (CheckCollisionRectangle(r->x, r->y, 20.0f, 20.0f,
            player_x, player_y, player_width, player_height))
        {
            sanity_percentage -= damage;
            if (sanity_percentage < 0.0f) sanity_percentage = 0.0f;
            r->active = 0;
        }
    }

    //// Projectile fireballs — always update
    //for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
    //{
    //    BossFireball* fb = &b->proj_fireballs[i];
    //    if (!fb->active) continue;

    //    fb->lifetime -= dt;
    //    if (fb->lifetime <= 0.0f) { fb->active = 0; continue; }

    //    // ========== HOMING BEHAVIOR ==========
    //    float dx = player_x - fb->x;
    //    float dy = player_y - fb->y;
    //    float dist = sqrtf(dx * dx + dy * dy);

    //    if (dist > 1.0f)
    //    {
    //        // Normalize direction to player
    //        float dir_x = dx / dist;
    //        float dir_y = dy / dist;

    //        // Current speed
    //        float current_speed = sqrtf(fb->vx * fb->vx + fb->vy * fb->vy);
    //        
    //        // Homing strength (higher = stronger tracking, 0.15f is a good balance)
    //        float homing_strength = 0.8f;
    //        
    //        // Blend current velocity with direction to player
    //        fb->vx = fb->vx * (1.0f - homing_strength) + dir_x * current_speed * homing_strength;
    //        fb->vy = fb->vy * (1.0f - homing_strength) + dir_y * current_speed * homing_strength;
    //    }

    //    // Update position
    //    fb->x += fb->vx * dt;
    //    fb->y += fb->vy * dt;

    //    // ========== COLLISION WITH TILES ==========
    //    int fireball_row, fireball_col;
    //    GetTileAtPosition(fb->x, fb->y, &fireball_row, &fireball_col);
    //    if (IsTileValid(fireball_row, fireball_col) &&
    //        tileMap[fireball_row][fireball_col] == 2)
    //    {
    //        fb->active = 0;
    //        continue;
    //    }

    //    // ========== COLLISION WITH PLAYER ==========
    //    float fireball_damage = 12.0f;

    //    if (CheckCollisionRectangle(fb->x, fb->y, 25.0f, 25.0f,
    //        player_x, player_y, player_width, player_height))
    //    {
    //        sanity_percentage -= fireball_damage;
    //        if (sanity_percentage < 0.0f) sanity_percentage = 0.0f;
    //        fb->active = 0;
    //    }
    //}

        // Projectile fireballs — always update
    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
    {
        BossFireball* fb = &b->proj_fireballs[i];
        if (!fb->active) continue;

        fb->lifetime -= dt;
        if (fb->lifetime <= 0.0f) { fb->active = 0; continue; }

        // ========== HOMING BEHAVIOR (only for non-final move) ==========
        float dx = player_x - fb->x;
        float dy = player_y - fb->y;
        float dist = sqrtf(dx * dx + dy * dy);

        // Only home if NOT in final move OR if scattered fireball (they don't home)
        if (b->state != BOSS_FINAL_MOVE && dist > 1.0f)
        {
            // Normalize direction to player
            float dir_x = dx / dist;
            float dir_y = dy / dist;

            // Current speed
            float current_speed = sqrtf(fb->vx * fb->vx + fb->vy * fb->vy);

            // Homing strength
            float homing_strength = 0.8f;

            // Blend current velocity with direction to player
            fb->vx = fb->vx * (1.0f - homing_strength) + dir_x * current_speed * homing_strength;
            fb->vy = fb->vy * (1.0f - homing_strength) + dir_y * current_speed * homing_strength;
        }

        // Update position
        fb->x += fb->vx * dt;
        fb->y += fb->vy * dt;

        // ========== COLLISION WITH TILES ==========
        int fireball_row, fireball_col;
        GetTileAtPosition(fb->x, fb->y, &fireball_row, &fireball_col);
        if (IsTileValid(fireball_row, fireball_col) &&
            tileMap[fireball_row][fireball_col] == 2)
        {
            // ========== FINAL MOVE: EXPLODE INTO 5 SCATTER FIREBALLS ==========
            if (b->state == BOSS_FINAL_MOVE)
            {
                // Spawn 5 scatter fireballs in all directions from explosion point
                float explosion_x = fb->x;
                float explosion_y = fb->y;

                for (int scatter = 0; scatter < 8; scatter++)
                {
                    int slot = 0;
                    while (slot < MAX_BOSS_FIREBALLS && b->proj_fireballs[slot].active) slot++;
                    if (slot >= MAX_BOSS_FIREBALLS) break;

                    float scatter_angle = (3.14159f * 2.0f / 8.0f) * scatter;
                    float scatter_speed = 250.0f;

                    b->proj_fireballs[slot].x = explosion_x;
                    b->proj_fireballs[slot].y = explosion_y;
                    b->proj_fireballs[slot].vx = cosf(scatter_angle) * scatter_speed;
                    b->proj_fireballs[slot].vy = sinf(scatter_angle) * scatter_speed;
                    b->proj_fireballs[slot].lifetime = 3.0f;
                    b->proj_fireballs[slot].active = 1;
                }
            }
            // ================================================

            fb->active = 0;
            continue;
        }

        // ========== COLLISION WITH PLAYER ==========
        float fireball_damage = 12.0f;

        if (CheckCollisionRectangle(fb->x, fb->y, 25.0f, 25.0f,
            player_x, player_y, player_width, player_height))
        {
            // ========== FINAL MOVE: EXPLODE INTO 5 SCATTER FIREBALLS ==========
            if (b->state == BOSS_FINAL_MOVE)
            {
                // Spawn 5 scatter fireballs in all directions from explosion point
                float explosion_x = fb->x;
                float explosion_y = fb->y;

                for (int scatter = 0; scatter < 5; scatter++)
                {
                    int slot = 0;
                    while (slot < MAX_BOSS_FIREBALLS && b->proj_fireballs[slot].active) slot++;
                    if (slot >= MAX_BOSS_FIREBALLS) break;

                    float scatter_angle = (3.14159f * 2.0f / 5.0f) * scatter;
                    float scatter_speed = 250.0f;

                    b->proj_fireballs[slot].x = explosion_x;
                    b->proj_fireballs[slot].y = explosion_y;
                    b->proj_fireballs[slot].vx = cosf(scatter_angle) * scatter_speed;
                    b->proj_fireballs[slot].vy = sinf(scatter_angle) * scatter_speed;
                    b->proj_fireballs[slot].lifetime = 3.0f;
                    b->proj_fireballs[slot].active = 1;
                }

                fb->active = 0;  // Destroy original fireball
                continue;
            }
            else
            {
                // Normal fireball damage (non-final move)
                sanity_percentage -= fireball_damage;
                if (sanity_percentage < 0.0f) sanity_percentage = 0.0f;
                fb->active = 0;
            }
        }
    }

    // ========== FINAL MOVE CHECK ==========
    if (!b->final_move_used && b->health <= (b->max_health * 0.25f) && b->health > 0.0f)
    {
        // Clear old projectiles before final move
        for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
            b->proj_fireballs[i].active = 0;
        for (int i = 0; i < MAX_BOSS_ROCKS; i++)
            b->proj_rocks[i].active = 0;

        b->state = BOSS_FINAL_MOVE;
        b->state_timer = 0.0f;
        b->final_move_used = 1;
        printf("========== BOSS FINAL MOVE ACTIVATED! ==========\n");
    }
    // ================================================
    // 
    // -----------------------------------------------------------------------
    switch (b->state)
    {
    case BOSS_IDLE:
    {
        StopGravityPull();

        b->wander_timer -= dt;
        if (b->wander_timer <= 0.0f)
        {
            float angle = ((float)(rand() % 360)) * (3.14159f / 180.0f);
            b->wander_dir_x = cosf(angle);
            b->wander_dir_y = sinf(angle);
            b->wander_timer = 0.5f + ((float)(rand() % 100)) / 100.0f;
        }

        float new_x = b->x + b->wander_dir_x * 80.0f * dt;
        if (!CheckTileCollision(new_x, b->y)) b->x = new_x;
        else b->wander_timer = 0.0f;

        float new_y = b->y + b->wander_dir_y * 80.0f * dt;
        if (!CheckTileCollision(b->x, new_y)) b->y = new_y;
        else b->wander_timer = 0.0f;

        if (b->attack_cooldown <= 0.0f)
        {
            BossPickNextAttack(b);
            b->attack_cooldown = 3.0f + ((float)(rand() % 200)) / 100.0f;
        }
        break;
    }

    case BOSS_CHARGE:
    {
        StopGravityPull();

        if (b->state_timer <= dt)
        {
            float angle = ((float)(rand() % 360)) * (3.14159f / 180.0f);
            b->charge_dir_x = cosf(angle);
            b->charge_dir_y = sinf(angle);
            b->charge_speed = 500.0f;
        }

        float new_x = b->x + b->charge_dir_x * b->charge_speed * dt;
        if (!CheckTileCollision(new_x, b->y)) b->x = new_x;
        else { b->charge_dir_x = -b->charge_dir_x; b->state_timer = 0.6f; }

        float new_y = b->y + b->charge_dir_y * b->charge_speed * dt;
        if (!CheckTileCollision(b->x, new_y)) b->y = new_y;
        else { b->charge_dir_y = -b->charge_dir_y; b->state_timer = 0.6f; }

        if (CheckCollisionRectangle(b->x, b->y, b->width, b->height,
            player_x, player_y, player_width, player_height))
        {
            if (b->contact_damage_cooldown <= 0.0f)
            {
                sanity_percentage -= 10.0f;
                if (sanity_percentage < 0.0f) sanity_percentage = 0.0f;
                b->contact_damage_cooldown = 0.8f;
            }
        }

        if (b->state_timer >= 0.6f)
        {
            b->state = BOSS_IDLE;
            b->state_timer = 0.0f;
        }
        break;
    }

    case BOSS_ROCK_BURST:
    {
        StopGravityPull();

        if (b->state_timer <= dt)
            BossFireRockBurstFrom(b, b->x, b->y);

        if (b->state_timer >= 2.0f)
        {
            b->state = BOSS_IDLE;
            b->state_timer = 0.0f;
        }
        break;
    }

    case BOSS_GRAVITY_PULL:
    {
        UpdateGravityPull(b, dt);

        if (b->state_timer >= PULL_DURATION)
        {
            StopGravityPull();
            b->state = BOSS_IDLE;
            b->state_timer = 0.0f;
        }
        break;
    }

    case BOSS_BLINK_DASH:
    {
        StopGravityPull();
        UpdateBlinkDash(b, dt);

        if (b->state_timer >= 2.5f) // can change 
        {
            b->state = BOSS_IDLE;
            b->state_timer = 0.0f;
        }
        break;
    }

    case BOSS_FIREBALL_BURST:
    {
        StopGravityPull();
        UpdateFireballBurst(b, dt);

        if (b->state_timer >= 1.5f)
        {
            b->state = BOSS_IDLE;
            b->state_timer = 0.0f;
        }
        break;
    }
    case BOSS_FINAL_MOVE:
    {
        StopGravityPull();
        UpdateFinalMove(b, dt);

        if (b->state_timer >= 2.5f)
        {
            b->state = BOSS_IDLE;
            b->state_timer = 0.0f;
        }
        break;
    }
    }
}

extern s8 g_font_id;
// ---------------------------------------------------------------------------
// BOSS HEALTH BAR RENDER HELPER
static void RenderBossHealthBar(Boss* b)
{
    if (!b->active) return;
    if (!g_healthBarBgMesh || !g_healthBarFgMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    const float BAR_W = 150.0f;
    const float BAR_H = 12.0f;
    const float OFFSET = 65.0f;
    const float BORDER = 2.0f;

    float bx = b->x;
    float by = b->y + OFFSET;

    // ========== WHITE BORDER ==========
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxVertexList* borderMesh = CreateRectangleMesh(BAR_W + BORDER * 2.0f, BAR_H + BORDER * 2.0f, 0xFFFFFFFF);
    AEMtx33 transform;
    AEMtx33Trans(&transform, bx, by);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(borderMesh);

    // ========== DARK BACKGROUND ==========
    AEGfxSetColorToMultiply(0.13f, 0.13f, 0.13f, 1.0f);
    AEMtx33Trans(&transform, bx, by);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_healthBarBgMesh, AE_GFX_MDM_TRIANGLES);

    // ========== COLOURED HEALTH FILL ==========
    float frac = (b->max_health > 0.0f) ? (b->health / b->max_health) : 0.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    float fill_w = BAR_W * frac;

    if (fill_w > 0.1f)
    {
        if (frac > 0.6f)
            AEGfxSetColorToMultiply(0.2f, 0.85f, 0.2f, 1.0f);  // Green
        else if (frac > 0.3f)
            AEGfxSetColorToMultiply(0.9f, 0.80f, 0.1f, 1.0f);  // Yellow
        else
            AEGfxSetColorToMultiply(0.9f, 0.15f, 0.15f, 1.0f); // Red

        AEGfxVertexList* fillMesh = CreateRectangleMesh(fill_w, BAR_H, 0xFFFFFFFF);
        float fill_cx = bx - BAR_W / 2.0f + fill_w / 2.0f;
        AEMtx33Trans(&transform, fill_cx, by);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(fillMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(fillMesh);
    }

    // ========== HP TEXT DISPLAY ==========
    // Only display text if font is valid
    if (g_font_id >= 0)
    {
        // Switch to screen space for text rendering
        AEGfxSetCamPosition(0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // Create HP text "XXX/XXX"
        char hp_text[32];
        sprintf_s(hp_text, 32, "%.0f/%.0f", b->health, b->max_health);

        // Convert world position to screen space for text
        // Text will appear above the health bar
        // Convert boss world pos to screen pos
        float screen_x = (bx - camera_x) / (SCREEN_WIDTH / 2.0f);
        float screen_y = (by - camera_y) / (SCREEN_HEIGHT / 2.0f);

        // Clamp to valid screen range
        if (screen_x > -1.0f && screen_x < 1.0f && screen_y > -1.0f && screen_y < 1.0f)
        {
            // Color the text based on health
            if (frac > 0.6f)
                AEGfxPrint(g_font_id, hp_text, screen_x - 0.04f, screen_y - 0.01f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
            else if (frac > 0.2f)
                AEGfxPrint(g_font_id, hp_text, screen_x - 0.04f, screen_y - 0.01f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
            else
                AEGfxPrint(g_font_id, hp_text, screen_x - 0.04f, screen_y - 0.01f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Switch camera back to world for next render
        AEGfxSetCamPosition(camera_x, camera_y);
    }
}
// ---------------------------------------------------------------------------
void RenderBoss()
{
    if (!g_boss.active || !g_bossMesh) return;
    Boss* b = &g_boss;

    AEGfxSetCamPosition(camera_x, camera_y);

    // ---- Gravity pull ring ----
    if (b->state == BOSS_GRAVITY_PULL)
    {
        float pulse = 0.4f + 0.3f * sinf(b->state_timer * 6.0f);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);
        AEGfxSetTransparency(pull_ring_alpha * (0.25f + 0.15f * pulse));
        AEGfxSetColorToMultiply(0.5f, 0.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 ring;
        AEMtx33Scale(&ring, PULL_RADIUS * 2.0f, PULL_RADIUS * 2.0f);
        AEMtx33Trans(&ring, b->x, b->y);
        AEGfxSetTransform(ring.m);
        AEGfxMeshDraw(g_bossMesh, AE_GFX_MDM_TRIANGLES);

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(pull_ring_alpha * 0.85f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        float inner_size = (PULL_RADIUS - 12.0f) * 2.0f;
        AEMtx33Scale(&ring, inner_size, inner_size);
        AEMtx33Trans(&ring, b->x, b->y);
        AEGfxSetTransform(ring.m);
        AEGfxMeshDraw(g_bossMesh, AE_GFX_MDM_TRIANGLES);

        AEGfxSetBlendMode(AE_GFX_BM_ADD);
        AEGfxSetColorToMultiply(0.8f, 0.2f, 1.0f, 1.0f);

        for (int i = 0; i < 8; i++)
        {
            float pAngle = (3.14159f * 2.0f / 8) * i + b->state_timer * 2.5f;
            float px = b->x + cosf(pAngle) * PULL_RADIUS;
            float py = b->y + sinf(pAngle) * PULL_RADIUS;

            AEGfxSetTransparency(pull_ring_alpha * (0.5f + 0.3f * pulse));

            AEMtx33 pm;
            AEMtx33Scale(&pm, 14.0f, 14.0f);
            AEMtx33Trans(&pm, px, py);
            AEGfxSetTransform(pm.m);
            AEGfxMeshDraw(g_bRockMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // ---- Blink dash arrival flash ----
    if (blink_flash_active)
    {
        float flash_alpha = blink_flash_timer / 0.15f;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);
        AEGfxSetTransparency(flash_alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

        AEMtx33 ft;
        AEMtx33Scale(&ft, b->width * 2.5f, b->height * 2.5f);
        AEMtx33Trans(&ft, b->x, b->y);
        AEGfxSetTransform(ft.m);
        AEGfxMeshDraw(g_bossMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ---- Boss body ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(g_bossTexture, 0, 0);

    AEMtx33 t;
    AEMtx33Trans(&t, b->x, b->y);
    AEGfxSetTransform(t.m);
    AEGfxMeshDraw(g_bossMesh, AE_GFX_MDM_TRIANGLES);

    // ---- Projectile rocks ----
    AEGfxTextureSet(g_bRockTexture, 0, 0);
    for (int i = 0; i < MAX_BOSS_ROCKS; i++)
    {
        if (!b->proj_rocks[i].active) continue;
        AEMtx33Trans(&t, b->proj_rocks[i].x, b->proj_rocks[i].y);
        AEGfxSetTransform(t.m);
        AEGfxMeshDraw(g_bRockMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ---- Projectile fireballs ----
    if (g_bfireballTexture && g_bfireballMesh)
    {
        AEGfxSetCamPosition(camera_x, camera_y);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(g_bfireballTexture, 0, 0);

        AEMtx33 fbTransform;
        //AEMtx33Trans(&fbTransform, b->x, b->y);
        //AEGfxSetTransform(fbTransform.m);
        //AEGfxMeshDraw(g_bfireballMesh, AE_GFX_MDM_TRIANGLES);
        for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
        {
            if (!b->proj_fireballs[i].active) continue;


            AEMtx33Trans(&fbTransform, b->proj_fireballs[i].x, b->proj_fireballs[i].y);
            AEGfxSetTransform(fbTransform.m);
            AEGfxMeshDraw(g_bfireballMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // ---- Boss health bar ----
    RenderBossHealthBar(b);
}
void RenderVictoryScreen()
{
    if (!boss_defeated) return;

    // Set to screen space (fixed UI)
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // ========== DARK OVERLAY ==========
    AEGfxSetTransparency(0.8f);  // 80% opaque overlay
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Draw full-screen overlay
    AEGfxVertexList* overlay = CreateRectangleMesh(3200.0f, 1800.0f, 0xFF000000);
    AEMtx33 overlayTransform;
    AEMtx33Trans(&overlayTransform, 0.0f, 0.0f);
    AEGfxSetTransform(overlayTransform.m);
    AEGfxMeshDraw(overlay, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(overlay);

    // ========== VICTORY TEXT ==========
    if (g_font_id >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // Main "VICTORY!" text - large and centered
        AEGfxPrint(g_font_id, (char*)"VICTORY!", -0.15f, 0.4f, 2.0f,
            1.0f, 1.0f, 0.0f, 1.0f);  // Yellow text

        // Boss defeated message
        AEGfxPrint(g_font_id, (char*)"The Boss Has Been Defeated!", -0.255f, 0.2f, 1.0f,
            0.8f, 0.8f, 0.8f, 1.0f);  // White text

        // Stats display
        char stats_text[256];
        sprintf_s(stats_text, 256, "Final Depth: %dm | Rocks Mined: %d",
            depth * 2, rocks_mined);

        AEGfxPrint(g_font_id, stats_text, -0.25f, 0.0f, 0.8f,
            0.9f, 0.9f, 0.9f, 1.0f);

        // Instructions
        AEGfxPrint(g_font_id, (char*)"Press ESC to Return to Menu", -0.23f, -0.3f, 0.9f,
            0.5f, 0.8f, 1.0f, 1.0f);  // Light blue text
    }
}
// =========================================================================

void FreeBoss()
{
    StopGravityPull();
    if (g_bossMesh) { AEGfxMeshFree(g_bossMesh);     g_bossMesh = NULL; }
    if (g_bRockMesh) { AEGfxMeshFree(g_bRockMesh);    g_bRockMesh = NULL; }
    if (g_bfireballMesh) { AEGfxMeshFree(g_bfireballMesh);    g_bfireballMesh = NULL; }
    if (g_healthBarBgMesh) { AEGfxMeshFree(g_healthBarBgMesh); g_healthBarBgMesh = NULL; }
    if (g_healthBarFgMesh) { AEGfxMeshFree(g_healthBarFgMesh); g_healthBarFgMesh = NULL; }
    if (g_bossTexture) { AEGfxTextureUnload(g_bossTexture);  g_bossTexture = NULL; }
    if (g_bRockTexture) { AEGfxTextureUnload(g_bRockTexture); g_bRockTexture = NULL; }
    if (g_bfireballTexture) { AEGfxTextureUnload(g_bfireballTexture); g_bfireballTexture = NULL; }
}
