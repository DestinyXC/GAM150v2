// ---------------------------------------------------------------------------
// player.hpp - Player attack system for Core Break
// Handles player attacking enemies and boss
// ---------------------------------------------------------------------------

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "AEEngine.h"

// ---------------------------------------------------------------------------
// CONFIGURATION

#define PLAYER_ATTACK_RANGE     150.0f   // Attack range in pixels
#define PLAYER_ATTACK_DAMAGE    25.0f    // Damage per attack
#define PLAYER_ATTACK_COOLDOWN  0.5f     // Cooldown between attacks (seconds)
#define PLAYER_ATTACK_DURATION  0.3f     // Visual attack animation duration

// ---------------------------------------------------------------------------
// ATTACK STATE

typedef struct {
    int is_attacking;           // Currently performing attack animation
    float attack_timer;         // Timer for attack animation
    float cooldown_timer;       // Cooldown before next attack allowed
    float attack_angle;         // Direction of attack (radians)
    int attack_hit_enemy;       // Enemy index that was hit (-1 if none)
    int attack_hit_boss;        // Boss was hit (0 or 1)
} PlayerAttackState;

// ---------------------------------------------------------------------------
// GLOBAL STATE (accessed by main.cpp)

extern PlayerAttackState player_attack_state;

// ---------------------------------------------------------------------------
// FUNCTIONS

// Initialize player attack system
void Player_Attack_Init();

// Update attack system (call every frame)
// player_x, player_y: player world position
// mouse_x, mouse_y: mouse world position
void Player_Attack_Update(float dt,
    float player_x, float player_y,
    float mouse_world_x, float mouse_world_y);

// Draw attack range circle (call before player is drawn)
void Player_Attack_DrawRange(float camera_x, float camera_y,
    float player_x, float player_y);

// Draw attack visuals (call after player is drawn)
void Player_Attack_Draw(float camera_x, float camera_y,
    float player_x, float player_y);

// Free resources
void Player_Attack_Kill();

// Check if player can attack right now
int Player_Can_Attack();

// Trigger attack (returns 1 if attack started, 0 if on cooldown)
int Player_Trigger_Attack(float player_x, float player_y,
    float mouse_world_x, float mouse_world_y);

#endif // PLAYER_HPP