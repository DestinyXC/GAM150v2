#ifndef PLAYERANIMATION_HPP
#define PLAYERANIMATION_HPP

#include "AEEngine.h"

// Player animation configuration
#define PLAYER_IDLE_FRAMES 2
#define PLAYER_MOVING_FRAMES 5
#define PLAYER_MINING_FRAMES 4
#define PLAYER_DIGGING_FRAMES 6
#define PLAYER_FRAME_WIDTH 220.0f
#define PLAYER_FRAME_HEIGHT 340.0f

// Animation states
typedef enum {
    ANIM_STATE_IDLE = 0,
    ANIM_STATE_MOVING = 1,
    ANIM_STATE_MINING = 2,
	ANIM_STATE_DIGGING = 3
} AnimationState;

typedef struct {
    // Idle animation
    AEGfxTexture* idle_spritesheet;
    int idle_frame_count;

    // Moving animation
    AEGfxTexture* moving_spritesheet;
    int moving_frame_count;

    // Mining animation
	AEGfxTexture* mining_spritesheet;
	int mining_frame_count;

    // Digging/Shovel animation
	AEGfxTexture* digging_spritesheet;
	int digging_frame_count;

    // Current state
    AnimationState current_state;
    AnimationState previous_state;
    int current_frame;
    float frame_timer;
    float frame_speed;
    float idle_frame_speed;
    int is_looping;
} PlayerAnimator;

// Initialize the player animator
void PlayerAnim_Init(PlayerAnimator* pa, const char* idle_path, const char* moving_path, const char* mining_path, const char* digging_path, float frame_speed, float idle_frame_speed, int loop);

// Update animation based on player movement
void PlayerAnim_Update(PlayerAnimator* pa, float dt, int is_moving, int is_mining, int is_digging);

// Get UV coordinates for current frame
void PlayerAnim_GetUVs(PlayerAnimator* pa, float* u1, float* v1, float* u2, float* v2);

// Get current frame index
int PlayerAnim_GetCurrentFrame(PlayerAnimator* pa);

// Reset animation to frame 0
void PlayerAnim_Reset(PlayerAnimator* pa);

// Unload all spritesheets
void PlayerAnim_Free(PlayerAnimator* pa);

#endif // PLAYERANIMATION_HPP