// ---------------------------------------------------------------------------
// player.cpp - Player attack system implementation
// ---------------------------------------------------------------------------

#include "player.hpp"
#include "enemy.hpp"
#include "boss.hpp"
#include <math.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// EXTERNAL DECLARATIONS

// Boss (from boss.cpp)
extern Boss g_boss;

// Enemy system (from enemy.cpp)
extern Enemy enemies[];
extern int enemy_count;

// ---------------------------------------------------------------------------
// GLOBALS

PlayerAttackState player_attack_state;

// ---------------------------------------------------------------------------
// PRIVATE - Meshes for attack visualization

static AEGfxVertexList* g_attackArcMesh = NULL;
static AEGfxVertexList* g_attackSlashMesh = NULL;
static AEGfxVertexList* g_attackRangeCircleMesh = NULL; // NEW: Attack range circle mesh

// ---------------------------------------------------------------------------
// HELPER - Create attack arc mesh (cone shape)

static AEGfxVertexList* CreateAttackArcMesh()
{
    AEGfxMeshStart();

    const int segments = 16;
    const float arc_angle = 0.785f; // 45 degrees in radians

    // Center point
    float cx = 0.0f;
    float cy = 0.0f;

    for (int i = 0; i < segments; i++)
    {
        float angle1 = -arc_angle + (float)i / (float)segments * arc_angle * 2.0f;
        float angle2 = -arc_angle + (float)(i + 1) / (float)segments * arc_angle * 2.0f;

        float x1 = cosf(angle1) * PLAYER_ATTACK_RANGE;
        float y1 = sinf(angle1) * PLAYER_ATTACK_RANGE;
        float x2 = cosf(angle2) * PLAYER_ATTACK_RANGE;
        float y2 = sinf(angle2) * PLAYER_ATTACK_RANGE;

        AEGfxTriAdd(
            cx, cy, 0x88FFFF00, 0.0f, 0.0f,
            x1, y1, 0x00FFFF00, 0.0f, 0.0f,
            x2, y2, 0x00FFFF00, 0.0f, 0.0f
        );
    }

    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// HELPER - Create slash effect mesh (rectangle)

static AEGfxVertexList* CreateSlashMesh()
{
    float w = PLAYER_ATTACK_RANGE;
    float h = 10.0f;
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    AEGfxMeshStart();
    AEGfxTriAdd(
        -hw, -hh, 0xFFFFFFFF, 0.0f, 1.0f,
        hw, -hh, 0xFFFFFFFF, 1.0f, 1.0f,
        -hw, hh, 0xFFFFFFFF, 0.0f, 0.0f
    );
    AEGfxTriAdd(
        hw, -hh, 0xFFFFFFFF, 1.0f, 1.0f,
        hw, hh, 0xFFFFFFFF, 1.0f, 0.0f,
        -hw, hh, 0xFFFFFFFF, 0.0f, 0.0f
    );
    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// HELPER - Create attack range circle mesh

static AEGfxVertexList* CreateAttackRangeCircleMesh()
{
    AEGfxMeshStart();

    const int segments = 32;
    const float radius = PLAYER_ATTACK_RANGE;

    // Center point
    float cx = 0.0f;
    float cy = 0.0f;

    for (int i = 0; i < segments; i++)
    {
        float angle1 = (float)i / (float)segments * 6.28318f; // 2 * PI
        float angle2 = (float)(i + 1) / (float)segments * 6.28318f;

        float x1 = cosf(angle1) * radius;
        float y1 = sinf(angle1) * radius;
        float x2 = cosf(angle2) * radius;
        float y2 = sinf(angle2) * radius;

        AEGfxTriAdd(
            cx, cy, 0x88FFFFFF, 0.0f, 0.0f,
            x1, y1, 0x00FFFFFF, 0.0f, 0.0f,
            x2, y2, 0x00FFFFFF, 0.0f, 0.0f
        );
    }

    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// PUBLIC FUNCTIONS

void Player_Attack_Init()
{
    player_attack_state.is_attacking = 0;
    player_attack_state.attack_timer = 0.0f;
    player_attack_state.cooldown_timer = 0.0f;
    player_attack_state.attack_angle = 0.0f;
    player_attack_state.attack_hit_enemy = -1;
    player_attack_state.attack_hit_boss = 0;

    // Create meshes
    g_attackArcMesh = CreateAttackArcMesh();
    g_attackSlashMesh = CreateSlashMesh();
    g_attackRangeCircleMesh = CreateAttackRangeCircleMesh(); // NEW: Create attack range circle mesh

    printf("Player_Attack_Init: Attack system initialized.\n");
}

int Player_Can_Attack()
{
    return (player_attack_state.cooldown_timer <= 0.0f &&
        !player_attack_state.is_attacking);
}

int Player_Trigger_Attack(float player_x, float player_y,
    float mouse_world_x, float mouse_world_y)
{
    if (!Player_Can_Attack())
        return 0;

    // Calculate attack direction
    float dx = mouse_world_x - player_x;
    float dy = mouse_world_y - player_y;
    float angle = atan2f(dy, dx);

    // Start attack
    player_attack_state.is_attacking = 1;
    player_attack_state.attack_timer = PLAYER_ATTACK_DURATION;
    player_attack_state.cooldown_timer = PLAYER_ATTACK_COOLDOWN;
    player_attack_state.attack_angle = angle;
    player_attack_state.attack_hit_enemy = -1;
    player_attack_state.attack_hit_boss = 0;

    // Check for hits on enemies
    for (int i = 0; i < enemy_count; i++)
    {
        Enemy* e = &enemies[i];
        if (!e->active) continue;

        float edx = e->x - player_x;
        float edy = e->y - player_y;
        float dist = sqrtf(edx * edx + edy * edy);

        // Check if enemy is within attack range
        if (dist <= PLAYER_ATTACK_RANGE)
        {
            // Check if enemy is within attack arc (45 degree cone)
            float enemy_angle = atan2f(edy, edx);
            float angle_diff = fabsf(enemy_angle - angle);

            // Normalize angle difference to [-PI, PI]
            while (angle_diff > 3.14159f) angle_diff -= 6.28318f;
            while (angle_diff < -3.14159f) angle_diff += 6.28318f;

            if (fabsf(angle_diff) < 0.785f) // 45 degrees
            {
                // Hit! (Note: Enemy damage will be handled in Enemy_Take_Damage)
                player_attack_state.attack_hit_enemy = i;
                printf("Player hit enemy %d for %.1f damage!\n", i, PLAYER_ATTACK_DAMAGE);
                break;
            }
        }
    }

    // Check for hits on boss
    if (g_boss.active)
    {
        float bdx = g_boss.x - player_x;
        float bdy = g_boss.y - player_y;
        float boss_dist = sqrtf(bdx * bdx + bdy * bdy);

        if (boss_dist <= PLAYER_ATTACK_RANGE * 2.0f) // Boss has larger hitbox
        {
            float boss_angle = atan2f(bdy, bdx);
            float angle_diff = fabsf(boss_angle - angle);

            while (angle_diff > 3.14159f) angle_diff -= 6.28318f;
            while (angle_diff < -3.14159f) angle_diff += 6.28318f;

            if (fabsf(angle_diff) < 0.785f)
            {
                player_attack_state.attack_hit_boss = 1;
                printf("Player hit boss for %.1f damage!\n", PLAYER_ATTACK_DAMAGE);
            }
        }
    }

    return 1;
}

void Player_Attack_Update(float dt,
    float player_x, float player_y,
    float mouse_world_x, float mouse_world_y)
{
    // Update attack animation timer
    if (player_attack_state.is_attacking)
    {
        player_attack_state.attack_timer -= dt;
        if (player_attack_state.attack_timer <= 0.0f)
        {
            player_attack_state.is_attacking = 0;
            player_attack_state.attack_timer = 0.0f;
        }
    }

    // Update cooldown timer
    if (player_attack_state.cooldown_timer > 0.0f)
    {
        player_attack_state.cooldown_timer -= dt;
        if (player_attack_state.cooldown_timer < 0.0f)
            player_attack_state.cooldown_timer = 0.0f;
    }

    // Trigger attack on mouse click
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        Player_Trigger_Attack(player_x, player_y, mouse_world_x, mouse_world_y);
    }
}

void Player_Attack_Draw(float camera_x, float camera_y,
    float player_x, float player_y)
{
    if (!player_attack_state.is_attacking)
        return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Fade out effect based on attack timer
    float alpha = player_attack_state.attack_timer / PLAYER_ATTACK_DURATION;
    AEGfxSetTransparency(alpha * 0.6f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 0.3f, 1.0f); // Yellow slash
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Draw attack arc
    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, player_attack_state.attack_angle);
    AEMtx33Trans(&translate, player_x, player_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_attackArcMesh, AE_GFX_MDM_TRIANGLES);

    // Draw slash line for extra effect
    AEGfxSetTransparency(alpha * 0.8f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // White flash

    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, player_attack_state.attack_angle);
    AEMtx33Trans(&translate,
        player_x + cosf(player_attack_state.attack_angle) * PLAYER_ATTACK_RANGE * 0.5f,
        player_y + sinf(player_attack_state.attack_angle) * PLAYER_ATTACK_RANGE * 0.5f);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_attackSlashMesh, AE_GFX_MDM_TRIANGLES);
}

// NEW: Draw the attack range circle around player
void Player_Attack_DrawRange(float camera_x, float camera_y,
    float player_x, float player_y)
{
    if (!g_attackRangeCircleMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.3f);  // Semi-transparent
    AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f); // Yellow circle
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEMtx33 transform;
    AEMtx33Trans(&transform, player_x, player_y);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_attackRangeCircleMesh, AE_GFX_MDM_TRIANGLES);
}

void Player_Attack_Kill()
{
    if (g_attackArcMesh)
    {
        AEGfxMeshFree(g_attackArcMesh);
        g_attackArcMesh = NULL;
    }

    if (g_attackSlashMesh)
    {
        AEGfxMeshFree(g_attackSlashMesh);
        g_attackSlashMesh = NULL;
    }

    if (g_attackRangeCircleMesh) // NEW: Free attack range circle mesh
    {
        AEGfxMeshFree(g_attackRangeCircleMesh);
        g_attackRangeCircleMesh = NULL;
    }

    printf("Player_Attack_Kill: Attack system resources freed.\n");
}
