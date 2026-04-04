#include "storyscreen.hpp"
#include "AEEngine.h"
#include <stdio.h>
#include <math.h>


static s8   g_font_id       = -1;
static int  g_current_panel = 0;        // 0 .. STORY_PANEL_COUNT-1
static float g_panel_timer  = 0.0f;    // time spent on current panel
static int  g_done          = 0;

// Full-screen textured quad
static AEGfxVertexList* g_panelMesh = NULL;

// One texture per panel
static AEGfxTexture* g_panelTex[STORY_PANEL_COUNT] = { NULL, NULL, NULL, NULL };

// Assets
static const char* PANEL_PATHS[STORY_PANEL_COUNT] = {
    "../Assets/story_panel1.png",
    "../Assets/story_panel2.png",
    "../Assets/story_panel3.png",
    "../Assets/story_panel4.png"
};

// Prompt text shown on each panel
static const char* PANEL_PROMPTS[STORY_PANEL_COUNT] = {
    "Click or press SPACE to continue...",
    "Click or press SPACE to continue...",
    "Click or press SPACE to continue...",
    "Click or press SPACE to begin!"
};

// Attempt to load a texture from a primary path then two fallback paths.
static AEGfxTexture* TryLoad(const char* path)
{
    AEGfxTexture* tex = AEGfxTextureLoad(path);
    if (tex) { printf("StoryScreen: loaded %s\n", path); return tex; }

    char alt1[512];
    sprintf_s(alt1, sizeof(alt1), "../%s", path);
    tex = AEGfxTextureLoad(alt1);
    if (tex) { printf("StoryScreen: loaded %s\n", alt1); return tex; }

    char alt2[512];
    sprintf_s(alt2, sizeof(alt2), "../../%s", path);
    tex = AEGfxTextureLoad(alt2);
    if (tex) { printf("StoryScreen: loaded %s\n", alt2); return tex; }

    printf("StoryScreen: WARNING - could not load %s (fallback colour will be used)\n", path);
    return NULL;
}

// Build a unit (1x1) textured quad centred at origin.
static AEGfxVertexList* MakeQuad()
{
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
         0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(
         0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
         0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

// Draw the mesh scaled to (w, h) centred at screen position (cx, cy).
// Camera must already be at (0, 0) for screen-space drawing.
static void DrawFullScreen(float w, float h)
{
    AEMtx33 xf;
    AEMtx33Scale(&xf, w, h);
    AEGfxSetTransform(xf.m);
    AEGfxMeshDraw(g_panelMesh, AE_GFX_MDM_TRIANGLES);
}

// StoryScreen_Load
void StoryScreen_Load(s8 font_id)
{
    g_font_id = font_id;

    g_panelMesh = MakeQuad();
    if (!g_panelMesh)
        printf("StoryScreen_Load: ERROR - failed to create panel mesh!\n");

    for (int i = 0; i < STORY_PANEL_COUNT; i++)
        g_panelTex[i] = TryLoad(PANEL_PATHS[i]);

    g_current_panel = 0;
    g_panel_timer   = 0.0f;
    g_done          = 0;

    printf("StoryScreen_Load: complete (%d panels).\n", STORY_PANEL_COUNT);
}

// StoryScreen_Unload
void StoryScreen_Unload()
{
    if (g_panelMesh)
    {
        AEGfxMeshFree(g_panelMesh);
        g_panelMesh = NULL;
    }

    for (int i = 0; i < STORY_PANEL_COUNT; i++)
    {
        if (g_panelTex[i])
        {
            AEGfxTextureUnload(g_panelTex[i]);
            g_panelTex[i] = NULL;
        }
    }

    printf("StoryScreen_Unload: all resources freed.\n");
}

// StoryScreen_Reset
void StoryScreen_Reset()
{
    g_current_panel = 0;
    g_panel_timer   = 0.0f;
    g_done          = 0;
}

// StoryScreen_Update
int StoryScreen_Update()
{
    if (g_done)
        return STORY_RESULT_DONE;

    float dt = (float)AEFrameRateControllerGetFrameTime();
    g_panel_timer += dt;

    // Only accept input after the panel has been visible long enough to prevent accidental instant-skipping from the menu click.
    if (g_panel_timer < STORY_MIN_PANEL_TIME)
        return STORY_RESULT_NONE;

    int advance =
        AEInputCheckTriggered(AEVK_LBUTTON) ||
        AEInputCheckTriggered(AEVK_SPACE)   ||
        AEInputCheckTriggered(AEVK_RETURN);

    if (advance)
    {
        g_current_panel++;
        g_panel_timer = 0.0f;

        if (g_current_panel >= STORY_PANEL_COUNT)
        {
            g_done = 1;
            return STORY_RESULT_DONE;
        }
    }

    return STORY_RESULT_NONE;
}

// StoryScreen_Draw
void StoryScreen_Draw()
{
    if (g_done || !g_panelMesh) return;

    // Draw in screen space
    AEGfxSetCamPosition(0.0f, 0.0f);

    // 1. Panel image (full screen)
    AEGfxTexture* tex = g_panelTex[g_current_panel];

    if (tex)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(tex, 0.0f, 0.0f);
        DrawFullScreen(STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT);
    }
    else
    {
        // Fallback: dark background with panel number if texture missing
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.08f, 0.05f, 0.02f, 1.0f); // very dark brown
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        DrawFullScreen(STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT);
    }

    // 2. Panel counter  (top-right, e.g. "1 / 4")
    if (g_font_id >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        char counter[16];
        sprintf_s(counter, sizeof(counter), "%d / %d",
                  g_current_panel + 1, STORY_PANEL_COUNT);

        // Top-right corner (normalised coords: x ~0.7, y ~0.85)
        AEGfxPrint(g_font_id, counter,
                   0.72f, 0.85f,
                   1.0f,
                   1.0f, 1.0f, 1.0f, 0.7f);

        // 3. "Click to continue" prompt - fades in after STORY_MIN_PANEL_TIME
        float prompt_alpha = (g_panel_timer - STORY_MIN_PANEL_TIME) / 0.4f;
        if (prompt_alpha > 1.0f) prompt_alpha = 1.0f;
        if (prompt_alpha < 0.0f) prompt_alpha = 0.0f;

        // Gentle pulse so it draws the player's eye
        float pulse = 0.7f + 0.3f * sinf(g_panel_timer * 3.0f);

        AEGfxPrint(g_font_id,
                   (char*)PANEL_PROMPTS[g_current_panel],
                   -0.37f, -0.88f,
                   0.9f,
                   1.0f, 1.0f, 1.0f,
                   prompt_alpha * pulse);
    }
}
