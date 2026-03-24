#pragma once
#pragma once
#ifndef LIGHTSYSTEM_H
#define LIGHTSYSTEM_H
#include "AEEngine.h"

// ---------------------------------------------------------------------------
// lightsystem.hpp  -  Darkness overlay + glow torch for Core Break
// ---------------------------------------------------------------------------
//
// HOW IT WORKS
// ------------
// Uses a two-pass additive glow approach:
//
//   Pass 1 - Darkness rectangle
//     A semi-transparent black rectangle covers the underground area.
//     The safe-zone rows at the top are left uncovered.
//
//   Pass 2 - Glow texture (additive blending)
//     A radial glow texture is drawn centred on the player using
//     AE_GFX_BM_ADD blending. Additive blending ADDS the glow's
//     brightness directly to whatever is underneath, burning through
//     the darkness naturally without needing a transparent hole.
//
// DRAW ORDER in Game_Draw:
//   1. RenderBackground / tiles
//   2. RenderRocks
//   3. RenderMiningCursor
//   4. LightSystem_DrawDarkness()   <-- darkness over world
//   5. LightSystem_DrawGlow()       <-- glow burns through darkness
//   6. RenderPlayer()               <-- player drawn ON TOP of glow
//   7. Enemy_Draw()
//   8. UI elements
//
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CONFIGURATION
// ---------------------------------------------------------------------------

// How opaque the darkness is (0.0 = invisible, 1.0 = fully black)
#define DARKNESS_ALPHA          0.85f

// Default glow radius in world units
#define TORCH_RADIUS_DEFAULT    200.0f

// Glow texture size (generated at runtime, must be power of 2)
#define GLOW_TEX_SIZE           128

// Warm yellow-white colour for the player glow (R, G, B as 0.0-1.0)
#define GLOW_COLOR_R            1.0f
#define GLOW_COLOR_G            1.0f
#define GLOW_COLOR_B            0.8f

// Wall torch glow colour (orange)
#define TORCH_COLOR_R           1.0f
#define TORCH_COLOR_G           0.6f
#define TORCH_COLOR_B           0.2f
#define TORCH_BRIGHTNESS        0.6f    // 0.0-1.0, how bright wall torches are

// The safe-zone Y boundary: rows ABOVE this Y are NOT darkened
#define LIGHT_SAFEZONE_Y_MAX   -2530.0f

// Map dimensions - must match main.cpp
#define LIGHT_MAP_WIDTH         17
#define LIGHT_MAP_HEIGHT        100
#define LIGHT_TILE_SIZE         64.0f

// ---------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ---------------------------------------------------------------------------

// Allocate glow texture and meshes. Call once during Game_Init.
void LightSystem_Init(void);

// Draw the darkness overlay over the underground area.
// Call AFTER drawing tiles/rocks but BEFORE drawing glow and player.
void LightSystem_DrawDarkness(float camera_x, float camera_y,
    float player_x, float player_y);

// Draw the player glow that burns through the darkness.
// Call AFTER LightSystem_DrawDarkness but BEFORE RenderPlayer.
// Pass torch positions/count for wall torches (can pass NULL/0 to skip).
void LightSystem_DrawGlow(float camera_x, float camera_y,
    float player_x, float player_y,
    float glow_radius,
    float* torch_xs, float* torch_ys,
    float* torch_radii, int torch_count);

// Free all resources. Call during Game_Kill.
void LightSystem_Kill(void);

#endif // LIGHTSYSTEM_H