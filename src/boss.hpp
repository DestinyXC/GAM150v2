// boss.hpp
#ifndef BOSS_HPP
#define BOSS_HPP

#include "AEEngine.h"

// ---------------------------------------------------------------------------
#define MAP_WIDTH   17
#define MAP_HEIGHT  100
#define TILE_SIZE   64.0f

// ---------------------------------------------------------------------------
typedef enum {
    BOSS_IDLE = 0,
    BOSS_CHARGE,
    BOSS_ROCK_BURST,
    BOSS_GRAVITY_PULL,
    BOSS_BLINK_DASH,
    BOSS_FIREBALL_BURST,
    BOSS_FINAL_MOVE
} BossState;

// ---------------------------------------------------------------------------
#define MAX_BOSS_ROCKS 24
typedef struct {
    float x, y;
    float vx, vy;
    float spawn_x, spawn_y;
    float lifetime;
    int   active;
} BossRock;

// ---------------------------------------------------------------------------
#define MAX_BOSS_FIREBALLS 32
typedef struct {
    float x, y;
    float vx, vy;
    float lifetime;
    int   active;
} BossFireball;

// ---------------------------------------------------------------------------
typedef struct {
    float      x, y;
    float      width, height;
    int        active;
    BossState  state;
    float      state_timer;
    float      attack_cooldown;
    float      wander_dir_x;
    float      wander_dir_y;
    float      wander_timer;
    float      charge_dir_x;
    float      charge_dir_y;
    float      charge_speed;
    BossRock   proj_rocks[MAX_BOSS_ROCKS];
    BossFireball proj_fireballs[MAX_BOSS_FIREBALLS];
    float      contact_damage_cooldown;
    // boss HP
    float      health;
    float      max_health;
    // final move tracking
    int        final_move_used;
} Boss;

// ---------------------------------------------------------------------------
extern Boss g_boss;
extern float sanity_percentage;
extern float sanity_max;
extern int boss_defeated;
// Gravity pull force � applied to player velocity in main.cpp UpdatePhysics
// Set to (0,0) when not pulling
extern float g_boss_pull_force_x;
extern float g_boss_pull_force_y;

extern AEGfxTexture* g_bossTexture;
extern AEGfxTexture* g_bRockTexture;
extern AEGfxTexture* g_bfireballTexture;


// ---------------------------------------------------------------------------
void InitBoss();
void UpdateBoss(float dt);
void RenderBoss();
void FreeBoss();
void RenderVictoryScreen();

#endif // BOSS_HPP
