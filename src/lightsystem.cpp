#include "AEEngine.h"
#include "lightsystem.hpp"
#include <stdio.h>
#include <math.h>

// ---------------------------------------------------------------------------
// lightsystem.cpp  -  Darkness overlay + additive glow torch for Core Break
// ---------------------------------------------------------------------------
//
// HOW IT WORKS
// ------------
//   Pass 1 - LightSystem_DrawDarkness()
//     Draws a semi-transparent black rectangle over the underground area.
//     The safe-zone band is skipped so the surface stays fully lit.
//
//   Pass 2 - LightSystem_DrawGlow()
//     Draws the glow texture centred on the player using AE_GFX_BM_ADD
//     (additive blending). Additive blending adds the glow's brightness
//     on top of the darkness, burning a bright circle through it.
//     Wall torches are drawn the same way in a dimmer orange colour.
//
//     IMPORTANT: Call this BEFORE RenderPlayer() so the player sprite
//     is drawn on top of the glow, not underneath it.
//
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// PRIVATE - resource storage
// ---------------------------------------------------------------------------

static AEGfxVertexList* g_darknessMesh = NULL;  // unit rect, scaled at draw time
static AEGfxTexture* g_glowTexture = NULL;  // radial glow, generated at init
static AEGfxVertexList* g_glowMesh = NULL;  // unit rect for glow texture

// ---------------------------------------------------------------------------
// PRIVATE - helpers
// ---------------------------------------------------------------------------

// Build a unit (1x1) solid colour rectangle centred at (0,0).
// Scaled via transform matrix at draw time.
static AEGfxVertexList* MakeUnitRect(unsigned int col)
{
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, col, 0.0f, 1.0f,
        0.5f, -0.5f, col, 1.0f, 1.0f,
        -0.5f, 0.5f, col, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, col, 1.0f, 1.0f,
        0.5f, 0.5f, col, 1.0f, 0.0f,
        -0.5f, 0.5f, col, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

// Generate a soft radial glow texture in memory.
// Centre = bright white, edge = fully transparent black.
static AEGfxTexture* MakeGlowTexture(void)
{
    static unsigned char pixels[GLOW_TEX_SIZE * GLOW_TEX_SIZE * 4];
    float half = GLOW_TEX_SIZE / 2.0f;

    for (int y = 0; y < GLOW_TEX_SIZE; y++)
    {
        for (int x = 0; x < GLOW_TEX_SIZE; x++)
        {
            float dx = (x - half) / half;
            float dy = (y - half) / half;
            float dist = sqrtf(dx * dx + dy * dy);

            // Linear falloff clamped to 0..1, then squared for a softer edge
            float a = 1.0f - dist;
            if (a < 0.0f) a = 0.0f;
            a = a * a;  // squared = softer gradient

            int idx = (y * GLOW_TEX_SIZE + x) * 4;
            pixels[idx + 0] = 255;                          // R
            pixels[idx + 1] = 255;                          // G
            pixels[idx + 2] = 255;                          // B
            pixels[idx + 3] = (unsigned char)(a * 255.0f); // A
        }
    }

    return AEGfxTextureLoadFromMemory(pixels, GLOW_TEX_SIZE, GLOW_TEX_SIZE);
}

// Draw a rect at (cx, cy) scaled to (w, h) using the current render state.
static void DrawScaledRect(AEGfxVertexList* mesh,
    float cx, float cy,
    float w, float h)
{
    AEMtx33 scale, trans, xf;
    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, cx, cy);
    AEMtx33Concat(&xf, &trans, &scale);
    AEGfxSetTransform(xf.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// ---------------------------------------------------------------------------
// LightSystem_Init
// ---------------------------------------------------------------------------
void LightSystem_Init(void)
{
    // Unit darkness rectangle (scaled to map size at draw time)
    g_darknessMesh = MakeUnitRect(0xFF000000);
    if (!g_darknessMesh)
        printf("LightSystem_Init: failed to create darkness mesh!\n");
    else
        printf("LightSystem_Init: darkness mesh created.\n");

    // Procedural glow texture
    g_glowTexture = MakeGlowTexture();
    if (!g_glowTexture)
        printf("LightSystem_Init: failed to create glow texture!\n");
    else
        printf("LightSystem_Init: glow texture created (%dx%d).\n",
            GLOW_TEX_SIZE, GLOW_TEX_SIZE);

    // Unit mesh for drawing the glow (scaled at draw time)
    g_glowMesh = MakeUnitRect(0xFFFFFFFF);
    if (!g_glowMesh)
        printf("LightSystem_Init: failed to create glow mesh!\n");
    else
        printf("LightSystem_Init: glow mesh created.\n");
}

// ---------------------------------------------------------------------------
// LightSystem_DrawDarkness
// ---------------------------------------------------------------------------
// Covers the underground area with a semi-transparent black rectangle.
// The safe-zone band (above LIGHT_SAFEZONE_Y_MAX) is left fully lit.
// Two rectangles are used so the safe zone gap is preserved.
// ---------------------------------------------------------------------------
void LightSystem_DrawDarkness(float camera_x, float camera_y,
    float player_x, float player_y)
{
    if (!g_darknessMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(DARKNESS_ALPHA);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float map_world_w = (float)LIGHT_MAP_WIDTH * LIGHT_TILE_SIZE;
    float map_world_h = (float)LIGHT_MAP_HEIGHT * LIGHT_TILE_SIZE;
    float map_top = map_world_h * 0.5f;
    float map_bottom = -map_world_h * 0.5f;
    float darkness_w = map_world_w + 500.0f;   // a little wider to cover edges

    // ------------------------------------------------------------------
    // Upper rect: map top --> safe-zone top (LIGHT_SAFEZONE_Y_MAX)
    // ------------------------------------------------------------------
    float upper_h = map_top - LIGHT_SAFEZONE_Y_MAX;
    float upper_cy = LIGHT_SAFEZONE_Y_MAX + upper_h * 0.5f;
    if (upper_h > 0.0f)
        DrawScaledRect(g_darknessMesh, 0.0f, upper_cy, darkness_w, upper_h);

    // ------------------------------------------------------------------
    // Lower rect: safe-zone bottom (-3200) --> map bottom
    // ------------------------------------------------------------------
    float lower_top = -3200.0f;
    float lower_h = lower_top - map_bottom;
    float lower_cy = map_bottom + lower_h * 0.5f;
    if (lower_h > 0.0f)
        DrawScaledRect(g_darknessMesh, 0.0f, lower_cy, darkness_w, lower_h);
}

// ---------------------------------------------------------------------------
// LightSystem_DrawGlow
// ---------------------------------------------------------------------------
// Uses ADDITIVE blending to burn a bright circle through the darkness.
// Call this AFTER LightSystem_DrawDarkness and BEFORE RenderPlayer so the
// player sprite appears on top of the glow, not underneath it.
// ---------------------------------------------------------------------------
void LightSystem_DrawGlow(float camera_x, float camera_y,
    float player_x, float player_y,
    float glow_radius,
    float* torch_xs, float* torch_ys,
    float* torch_radii, int torch_count)
{
    if (!g_glowTexture || !g_glowMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);          // additive: burns through darkness
    AEGfxTextureSet(g_glowTexture, 0, 0);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // ------------------------------------------------------------------
    // Player glow  (warm white/yellow)
    // Only draw underground (below safe zone top)
    // ------------------------------------------------------------------
    AEGfxSetColorToMultiply(GLOW_COLOR_R, GLOW_COLOR_G, GLOW_COLOR_B, 1.0f);
    float diameter = glow_radius * 2.0f;
    DrawScaledRect(g_glowMesh, player_x, player_y, diameter, diameter);

    // ------------------------------------------------------------------
    // Wall torch glows  (orange, dimmer)
    // ------------------------------------------------------------------
    if (torch_xs && torch_ys && torch_radii && torch_count > 0)
    {
        AEGfxSetColorToMultiply(TORCH_COLOR_R, TORCH_COLOR_G, TORCH_COLOR_B, 1.0f);
        AEGfxSetTransparency(TORCH_BRIGHTNESS);

        for (int i = 0; i < torch_count; i++)
        {
            float td = torch_radii[i] * 2.0f;
            DrawScaledRect(g_glowMesh, torch_xs[i], torch_ys[i], td, td);
        }
    }
}

// ---------------------------------------------------------------------------
// LightSystem_Kill
// ---------------------------------------------------------------------------
void LightSystem_Kill(void)
{
    if (g_darknessMesh)
    {
        AEGfxMeshFree(g_darknessMesh);
        g_darknessMesh = NULL;
    }
    if (g_glowMesh)
    {
        AEGfxMeshFree(g_glowMesh);
        g_glowMesh = NULL;
    }
    if (g_glowTexture)
    {
        AEGfxTextureUnload(g_glowTexture);
        g_glowTexture = NULL;
    }

    printf("LightSystem_Kill: all resources freed.\n");
}