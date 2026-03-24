#include "animation.hpp"
#include "AEEngine.h"  

void PlayerAnim_Init(PlayerAnimator* pa, const char* idle_path, const char* moving_path, const char* mining_path, const char* digging_path, float frame_speed, float idle_frame_speed, int loop)
{
    pa->idle_spritesheet = AEGfxTextureLoad(idle_path);
    pa->moving_spritesheet = AEGfxTextureLoad(moving_path);
	pa->mining_spritesheet = AEGfxTextureLoad(mining_path);
    pa->digging_spritesheet = AEGfxTextureLoad(digging_path);

    pa->idle_frame_count = PLAYER_IDLE_FRAMES;
    pa->moving_frame_count = PLAYER_MOVING_FRAMES;
	pa->mining_frame_count = PLAYER_MINING_FRAMES;
	pa->digging_frame_count = PLAYER_DIGGING_FRAMES;

    pa->current_state = ANIM_STATE_IDLE;
    pa->previous_state = ANIM_STATE_IDLE;
    pa->current_frame = 0;
    pa->frame_timer = 0.0f;
    pa->frame_speed = frame_speed;
	pa->idle_frame_speed = idle_frame_speed;
    pa->is_looping = loop;
}

void PlayerAnim_Update(PlayerAnimator* pa, float dt, int is_moving, int is_mining, int is_digging)
{
    // Determine which animation to play
    AnimationState new_state = ANIM_STATE_IDLE;

    if (is_mining) {
		new_state = ANIM_STATE_MINING;
    }
    else if (is_moving) {
        new_state = ANIM_STATE_MOVING;
	}
    else if (is_digging) {
        new_state = ANIM_STATE_DIGGING;
	}

    // If state changed, reset animation
    if (new_state != pa->current_state)
    {
        pa->current_state = new_state;
        pa->current_frame = 0;
        pa->frame_timer = 0.0f;
    }

    pa->previous_state = pa->current_state;

    // Get frame count for current state
	int frame_count = PLAYER_IDLE_FRAMES;

    if (pa->current_state == ANIM_STATE_MOVING)
    {
        frame_count = pa->moving_frame_count;
	}
    else if (pa->current_state == ANIM_STATE_MINING)
    {
		frame_count = pa->mining_frame_count;
    }
	else if (pa->current_state == ANIM_STATE_DIGGING)
    {
        frame_count = pa->digging_frame_count;
    }
    else {
		frame_count = pa->idle_frame_count;
    }
        
    if (frame_count <= 1) return;

	// Use different frame speed for idle animation
	float current_frame_speed = (pa->current_state == ANIM_STATE_IDLE) ? pa->idle_frame_speed : pa->frame_speed;

    // Update frame timer
    pa->frame_timer += dt;

    if (pa->frame_timer >= current_frame_speed)
    {
        pa->frame_timer = 0.0f;
        pa->current_frame++;

        if (pa->current_frame >= frame_count)
        {
            if (pa->is_looping)
            {
                pa->current_frame = 0;
            }
            else
            {
                pa->current_frame = frame_count - 1;
            }
        }
    }
}

void PlayerAnim_GetUVs(PlayerAnimator* pa, float* u1, float* v1, float* u2, float* v2)
{
	// Select texture based on current state
	AEGfxTexture* current_texture = pa->idle_spritesheet;
	int frame_count = pa->idle_frame_count;

    if (pa->current_state == ANIM_STATE_MOVING)
    {
        current_texture = pa->moving_spritesheet;
        frame_count = pa->moving_frame_count;
	}
    else if (pa->current_state == ANIM_STATE_MINING)
    {
        current_texture = pa->mining_spritesheet;
        frame_count = pa->mining_frame_count;
	}
    else if (pa->current_state == ANIM_STATE_DIGGING)
    {
		current_texture = pa->digging_spritesheet;
		frame_count = pa->digging_frame_count;
    }

    if (!current_texture) {
        *u1 = 0.0f;
        *v1 = 0.0f;
        *u2 = 1.0f;
        *v2 = 1.0f;
		return;
    }

    // Calculate total spritesheet width
    float spritesheet_width = PLAYER_FRAME_WIDTH * frame_count;
    float frame_width_uv = PLAYER_FRAME_WIDTH / spritesheet_width;

    *u1 = pa->current_frame * frame_width_uv;
    *v1 = 0.0f;
    *u2 = (pa->current_frame + 1) * frame_width_uv;
    *v2 = 1.0f;
}

int PlayerAnim_GetCurrentFrame(PlayerAnimator* pa)
{
    return pa->current_frame;
}

void PlayerAnim_Reset(PlayerAnimator* pa)
{
    pa->current_frame = 0;
    pa->frame_timer = 0.0f;
}

void PlayerAnim_Free(PlayerAnimator* pa)
{
    if (pa->idle_spritesheet)
    {
        AEGfxTextureUnload(pa->idle_spritesheet);
        pa->idle_spritesheet = NULL;
    }

    if (pa->moving_spritesheet)
    {
        AEGfxTextureUnload(pa->moving_spritesheet);
        pa->moving_spritesheet = NULL;
    }

    if (pa->mining_spritesheet)
    {
		AEGfxTextureUnload(pa->mining_spritesheet);
		pa->mining_spritesheet = NULL;
    }
    if (pa->digging_spritesheet)
    {
        AEGfxTextureUnload(pa->digging_spritesheet);
		pa->digging_spritesheet = NULL;
    }
}
