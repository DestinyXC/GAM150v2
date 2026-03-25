#include "enemy.hpp"
#include "AEEngine.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "losescreen.hpp"


// Extern map helpers from main.cpp

extern int  tileMap[][17];
extern void GetTileWorldPosition(int row, int col, float* out_x, float* out_y);
extern int sanity_upgrade_level;

#define _MAP_WIDTH   17
#define _MAP_HEIGHT  100
#define _TILE_SIZE   64.0f
#define _TILE_STONE  1
#define _TILE_DIRT   0
#define _TILE_LAVA   3

// Safe-zone respawn (matches main.cpp safe zone)
#define RESPAWN_X    0.0f
#define RESPAWN_Y   (-2750.0f)

// Surface row in the tilemap
#define SURFACE_ROW  9

// GLOBALS

Enemy enemies[MAX_ENEMIES];
int   enemy_count = 0;
float player_hp = PLAYER_MAX_HP;
int   player_is_dead = 0;
EnemyType last_killer_type = ENEMY_TYPE_FERAL;

// PRIVATE - Meshes & Textures

static AEGfxVertexList* g_dashMesh = NULL;   // 1x1 unit square for circle dashes
static AEGfxVertexList* g_bodyMesh = NULL;   // fallback coloured rect
static AEGfxVertexList* g_hpBgMesh = NULL;
static AEGfxVertexList* g_hpFgMesh = NULL;

// Feral textures
static AEGfxTexture* g_feralTex = NULL;
static AEGfxVertexList* g_feralTexMesh = NULL;

// Insane textures
static AEGfxTexture* g_insaneTex = NULL;
static AEGfxVertexList* g_insaneTexMesh = NULL;

// Mole textures
static AEGfxTexture* g_moleTex = NULL;
static AEGfxVertexList* g_moleTexMesh = NULL;

// PRIVATE - Mesh helpers

static AEGfxVertexList* MakeSolidRect(float w, float h, unsigned int col)
{
    float hw = w * 0.5f, hh = h * 0.5f;
    AEGfxMeshStart();
    AEGfxTriAdd(-hw, -hh, col, 0, 1, hw, -hh, col, 1, 1, -hw, hh, col, 0, 0);
    AEGfxTriAdd(hw, -hh, col, 1, 1, hw, hh, col, 1, 0, -hw, hh, col, 0, 0);
    return AEGfxMeshEnd();
}

static AEGfxVertexList* MakeTexRect(float w, float h)
{
    float hw = w * 0.5f, hh = h * 0.5f;
    AEGfxMeshStart();
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, 0, 1, hw, -hh, 0xFFFFFFFF, 1, 1, -hw, hh, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, 1, 1, hw, hh, 0xFFFFFFFF, 1, 0, -hw, hh, 0xFFFFFFFF, 0, 0);
    return AEGfxMeshEnd();
}

// Draw a rect in world space
static void DrawRect(AEGfxVertexList* mesh, float x, float y, float sx, float sy)
{
    AEMtx33 s, t, m;
    AEMtx33Scale(&s, sx, sy);
    AEMtx33Trans(&t, x, y);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// PRIVATE - Draw dashed translucent red detection circle

static void DrawRangeCircle(float cx, float cy, float radius)
{
    if (!g_dashMesh) return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.4f);                             // translucent
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 0.4f);       // soft red

    const float DASH_WIDTH = 3.0f;

    for (int i = 0; i < CIRCLE_SEGMENTS; ++i)
    {
        if (i % 2 == 0) continue;   // alternating dashes

        float a0 = (float)i / (float)CIRCLE_SEGMENTS * 6.28318f;
        float a1 = (float)(i + 1) / (float)CIRCLE_SEGMENTS * 6.28318f;
        float am = (a0 + a1) * 0.5f;

        float px = cx + cosf(am) * radius;
        float py = cy + sinf(am) * radius;
        float arc = radius * (a1 - a0);

        AEMtx33 sc, ro, tr, xf;
        AEMtx33Scale(&sc, arc * 0.9f, DASH_WIDTH);
        AEMtx33Rot(&ro, am + 1.5708f);
        AEMtx33Trans(&tr, px, py);
        AEMtx33Concat(&xf, &ro, &sc);
        AEMtx33Concat(&xf, &tr, &xf);
        AEGfxSetTransform(xf.m);
        AEGfxMeshDraw(g_dashMesh, AE_GFX_MDM_TRIANGLES);
    }
}


//  Spawn one enemy

static void SpawnEnemy(int row, int col, EnemyType type)
{
    if (enemy_count >= MAX_ENEMIES) return;

    float ex, ey;
    GetTileWorldPosition(row, col, &ex, &ey);
    ey += _TILE_SIZE;   // sit on top of tile

    Enemy* e = &enemies[enemy_count++];
    e->x = ex;
    e->y = ey;
    e->patrol_x = ex;
    e->patrol_range = _TILE_SIZE * 3.0f;
    e->direction = 1;
    e->active = 1;
    e->is_chasing = 0;
    e->type = type;

    if (type == ENEMY_TYPE_FERAL)
    {
        e->speed = ENEMY_SPEED_FERAL;
        e->damage = DAMAGE_FERAL;
        e->detect_radius = DETECT_RADIUS_FERAL;
    }
    else if (type == ENEMY_TYPE_INSANE)
    {
        e->speed = ENEMY_SPEED_INSANE;
        e->damage = DAMAGE_INSANE;
        e->detect_radius = DETECT_RADIUS_INSANE;
    }
    else  // MOLE
    {
        e->speed = ENEMY_SPEED_MOLE;
        e->damage = DAMAGE_MOLE;
        e->detect_radius = DETECT_RADIUS_MOLE;
    }
}


// ENEMY_INIT

void Enemy_Init()
{
    enemy_count = 0;
    player_hp = PLAYER_MAX_HP;
    player_is_dead = 0;

    //  Create meshes 
    g_dashMesh = MakeSolidRect(1.0f, 1.0f, 0xFFFF0000);
    g_bodyMesh = MakeSolidRect(ENEMY_WIDTH, ENEMY_HEIGHT, 0xFFAA2222);
    g_hpBgMesh = MakeSolidRect(1.0f, 1.0f, 0xFF222222);
    g_hpFgMesh = MakeSolidRect(1.0f, 1.0f, 0xFFDD1111);

    //  Load Feral texture 
    g_feralTex = AEGfxTextureLoad("../Assets/enemy_feral.png");
    if (g_feralTex)
    {
        g_feralTexMesh = MakeTexRect(ENEMY_WIDTH, ENEMY_HEIGHT);
        printf("Enemy_Init: Feral texture loaded.\n");
    }
    else
    {
        printf("Enemy_Init: enemy_feral.png not found - using fallback colour.\n");
    }

    //  Load Insane texture 
    g_insaneTex = AEGfxTextureLoad("../Assets/enemy_insane.png");
    if (g_insaneTex)
    {
        g_insaneTexMesh = MakeTexRect(ENEMY_WIDTH, ENEMY_HEIGHT);
        printf("Enemy_Init: Insane texture loaded.\n");
    }
    else
    {
        printf("Enemy_Init: enemy_insane.png not found - using fallback colour.\n");
    }

    //  Load Mole texture 
    g_moleTex = AEGfxTextureLoad("../Assets/enemy_mole_test.png");
    if (g_moleTex)
    {
        g_moleTexMesh = MakeTexRect(ENEMY_WIDTH, ENEMY_HEIGHT);
        printf("Enemy_Init: Mole texture loaded.\n");
    }
    else
    {
        printf("Enemy_Init: enemy_mole.png not found - using fallback colour.\n");
    }

    // Spawn Feral enemies (shallow zone)
    // Rows: SURFACE_ROW + FERAL_SPAWN_DEPTH  to  SURFACE_ROW + INSANE_SPAWN_DEPTH
    for (int row = SURFACE_ROW + FERAL_SPAWN_DEPTH;
        row < SURFACE_ROW + INSANE_SPAWN_DEPTH && row < _MAP_HEIGHT - 1;
        row += 6)
    {
        for (int col = 2; col < _MAP_WIDTH - 2; col += 4)
        {
            if (enemy_count >= MAX_ENEMIES) break;
            int tile = tileMap[row][col];
            if (tile != _TILE_STONE && tile != _TILE_DIRT && tile != _TILE_LAVA) continue;
            float chance = (float)rand() / (float)RAND_MAX;
            if (chance > ENEMY_SPAWN_CHANCE) continue;
            SpawnEnemy(row, col, ENEMY_TYPE_FERAL);
        }
        if (enemy_count >= MAX_ENEMIES) break;
    }

    //  Spawn Insane enemies (mid zone) 
    // Rows: SURFACE_ROW + INSANE_SPAWN_DEPTH  to  SURFACE_ROW + MOLE_SPAWN_DEPTH
    for (int row = SURFACE_ROW + INSANE_SPAWN_DEPTH;
        row < SURFACE_ROW + MOLE_SPAWN_DEPTH && row < _MAP_HEIGHT - 1;
        row += 6)
    {
        for (int col = 2; col < _MAP_WIDTH - 2; col += 4)
        {
            if (enemy_count >= MAX_ENEMIES) break;
            int tile = tileMap[row][col];
            if (tile != _TILE_STONE && tile != _TILE_DIRT && tile != _TILE_LAVA) continue;
            float chance = (float)rand() / (float)RAND_MAX;
            if (chance > ENEMY_SPAWN_CHANCE) continue;
            SpawnEnemy(row, col, ENEMY_TYPE_INSANE);
        }
        if (enemy_count >= MAX_ENEMIES) break;
    }

    //  Spawn Mole enemies (deepest zone) 
    // Rows: SURFACE_ROW + MOLE_SPAWN_DEPTH  to  end of map
    for (int row = SURFACE_ROW + MOLE_SPAWN_DEPTH;
        row < _MAP_HEIGHT - 1;
        row += 6)
    {
        for (int col = 2; col < _MAP_WIDTH - 2; col += 4)
        {
            if (enemy_count >= MAX_ENEMIES) break;
            int tile = tileMap[row][col];
            if (tile != _TILE_STONE && tile != _TILE_DIRT && tile != _TILE_LAVA) continue;
            float chance = (float)rand() / (float)RAND_MAX;
            if (chance > ENEMY_SPAWN_CHANCE) continue;
            SpawnEnemy(row, col, ENEMY_TYPE_MOLE);
        }
        if (enemy_count >= MAX_ENEMIES) break;
    }

    printf("Enemy_Init: %d enemies spawned total.\n", enemy_count);
}

// ENEMY_UPDATE

void Enemy_Update(float dt,
    float px, float py,
    float* out_px, float* out_py)
{
    // Handle respawn frame
    if (player_is_dead)
    {
        // Only do the actual position reset when the lose screen has finished
        if (!lose_screen_is_active)
        {
            player_is_dead = 0;
            player_hp = PLAYER_MAX_HP;
            *out_px = RESPAWN_X;
            *out_py = RESPAWN_Y;
        }
    }

    //  Restore HP when player is in the safe zone 
    float safezone_x_min = -500.0f;
    float safezone_x_max = 500.0f;
    float safezone_y_min = -3200.0f;
    float safezone_y_max = -2530.0f;

    int in_safezone = (px >= safezone_x_min && px <= safezone_x_max &&
        py >= safezone_y_min && py <= safezone_y_max);

    if (in_safezone && player_hp < PLAYER_MAX_HP)
    {
        player_hp += 20.0f * dt;   // restore 20 HP per second
        if (player_hp > PLAYER_MAX_HP)
            player_hp = PLAYER_MAX_HP;
    }

    //  Update each enemy 
    float total_damage = 0.0f;

    // Wall bounds to stop enemies entering walls
    float wall_left = -(_MAP_WIDTH * _TILE_SIZE / 2.0f) + _TILE_SIZE * 2.0f;
    float wall_right = (_MAP_WIDTH * _TILE_SIZE / 2.0f) - _TILE_SIZE * 2.0f;
    float wall_bottom = -(_MAP_HEIGHT * _TILE_SIZE / 2.0f) + _TILE_SIZE;

    for (int i = 0; i < enemy_count; i++)
    {
        Enemy* e = &enemies[i];
        if (!e->active) continue;

        float dx = px - e->x;
        float dy = py - e->y;
        float dist = sqrtf(dx * dx + dy * dy);

        e->is_chasing = (dist <= e->detect_radius) ? 1 : 0;

        if (e->is_chasing)
        {
            // Chase player with wall bounds check
            if (dist > 1.0f)
            {
                float next_x = e->x + (dx / dist) * e->speed;
                float next_y = e->y + (dy / dist) * e->speed;

                if (next_x > wall_left && next_x < wall_right)
                    e->x = next_x;
                if (next_y > wall_bottom)
                    e->y = next_y;
            }

            // Deal damage when touching
            float touch_dist = (ENEMY_WIDTH + 75.0f) * 0.5f;
            if (dist <= touch_dist)
            {
                total_damage += e->damage * dt;
            }
        }
        else
        {
            // Patrol left/right with wall bounds check
            float next_patrol_x = e->x + e->speed * 0.4f * (float)e->direction;

            if (next_patrol_x > wall_left && next_patrol_x < wall_right)
                e->x = next_patrol_x;
            else
                e->direction *= -1;  // bounce off wall boundary

            // Reverse at patrol range boundary
            if (e->x > e->patrol_x + e->patrol_range)
            {
                e->x = e->patrol_x + e->patrol_range;
                e->direction = -1;
            }
            else if (e->x < e->patrol_x - e->patrol_range)
            {
                e->x = e->patrol_x - e->patrol_range;
                e->direction = 1;
            }
        }
    }

    // Apply accumulated damage
    if (total_damage > 0.0f)
    {
        float damage_multiplier = 1.0f;
        switch (sanity_upgrade_level)
        {
        case 1: damage_multiplier = 0.75f; break;  // 25% less damage
        case 2: damage_multiplier = 0.50f; break;  // 50% less damage
        case 3: damage_multiplier = 0.25f; break;  // 75% less damage
        }

        player_hp -= total_damage * damage_multiplier;
        if (player_hp < 0.0f) player_hp = 0.0f;
    }

    // Death check
    if (player_hp <= 0.0f && !player_is_dead)
    {
        // This loop MUST be before player_is_dead = 1
        float closest = 999999.0f;
        last_killer_type = ENEMY_TYPE_FERAL;
        for (int k = 0; k < enemy_count; k++)
        {
            if (!enemies[k].active || !enemies[k].is_chasing) continue;
            float kdx = px - enemies[k].x;
            float kdy = py - enemies[k].y;
            float kdist = sqrtf(kdx * kdx + kdy * kdy);
            if (kdist < closest)
            {
                closest = kdist;
                last_killer_type = enemies[k].type;  // set BEFORE player_is_dead
            }
        }

        player_is_dead = 1;   // set AFTER the loop
        player_hp = PLAYER_MAX_HP;
        *out_px = RESPAWN_X;
        *out_py = RESPAWN_Y;
    }
}


// ENEMY_DRAW

void Enemy_Draw(float camera_x, float camera_y)
{
    AEGfxSetCamPosition(camera_x, camera_y);

    for (int i = 0; i < enemy_count; i++)
    {
        Enemy* e = &enemies[i];
        if (!e->active) continue;

        // 1. Dashed translucent red detection circle
        DrawRangeCircle(e->x, e->y, e->detect_radius);

        // 2. Pick correct texture and mesh for enemy type
        AEGfxTexture* tex = (e->type == ENEMY_TYPE_FERAL) ? g_feralTex :
            (e->type == ENEMY_TYPE_INSANE) ? g_insaneTex :
            g_moleTex;

        AEGfxVertexList* tmsh = (e->type == ENEMY_TYPE_FERAL) ? g_feralTexMesh :
            (e->type == ENEMY_TYPE_INSANE) ? g_insaneTexMesh :
            g_moleTexMesh;

        if (tex && tmsh)
        {
            // Draw with texture
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxSetColorToAdd(0, 0, 0, 0);
            AEGfxTextureSet(tex, 0, 0);
            DrawRect(tmsh, e->x, e->y, 1.0f, 1.0f);
        }
        else
        {
            // Fallback coloured rectangle
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToAdd(0, 0, 0, 0);

            if (e->type == ENEMY_TYPE_FERAL)
                AEGfxSetColorToMultiply(0.9f, 0.4f, 0.1f, 1.0f);   // orange
            else if (e->type == ENEMY_TYPE_INSANE)
                AEGfxSetColorToMultiply(0.7f, 0.05f, 0.05f, 1.0f); // deep red
            else
                AEGfxSetColorToMultiply(0.4f, 0.25f, 0.1f, 1.0f);  // dark brown (Mole)

            DrawRect(g_bodyMesh, e->x, e->y, 1.0f, 1.0f);
        }

        // 3. Yellow alert dot above enemy when chasing
        if (e->is_chasing)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToAdd(0, 0, 0, 0);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);  // yellow
            DrawRect(g_hpFgMesh,
                e->x,
                e->y + ENEMY_HEIGHT * 0.5f + 12.0f,
                8.0f, 8.0f);
        }
    }
}

void Enemy_Take_Damage(int enemy_index, float damage)
{
    if (enemy_index < 0 || enemy_index >= enemy_count)
        return;

    Enemy* e = &enemies[enemy_index];
    if (!e->active)
        return;

    // Enemies die in one hit for now
    // You can add enemy HP system later if needed
    e->active = 0;
    printf("Enemy %d destroyed by player attack!\n", enemy_index);
}

// ENEMY_KILL

void Enemy_Kill()
{
    if (g_dashMesh) { AEGfxMeshFree(g_dashMesh);       g_dashMesh = NULL; }
    if (g_bodyMesh) { AEGfxMeshFree(g_bodyMesh);       g_bodyMesh = NULL; }
    if (g_hpBgMesh) { AEGfxMeshFree(g_hpBgMesh);       g_hpBgMesh = NULL; }
    if (g_hpFgMesh) { AEGfxMeshFree(g_hpFgMesh);       g_hpFgMesh = NULL; }
    if (g_feralTexMesh) { AEGfxMeshFree(g_feralTexMesh);   g_feralTexMesh = NULL; }
    if (g_insaneTexMesh) { AEGfxMeshFree(g_insaneTexMesh);  g_insaneTexMesh = NULL; }
    if (g_moleTexMesh) { AEGfxMeshFree(g_moleTexMesh);    g_moleTexMesh = NULL; }
    if (g_feralTex) { AEGfxTextureUnload(g_feralTex);  g_feralTex = NULL; }
    if (g_insaneTex) { AEGfxTextureUnload(g_insaneTex); g_insaneTex = NULL; }
    if (g_moleTex) { AEGfxTextureUnload(g_moleTex);   g_moleTex = NULL; }

    printf("Enemy_Kill: all resources freed.\n");
}