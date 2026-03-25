// ---------------------------------------------------------------------------
// losescreen.cpp  -  Per-enemy lose screen for Core Break
// ---------------------------------------------------------------------------
//
// Three separate full-screen images are shown depending on which enemy
// killed the player:
//
//   LOSE_KILLER_FERAL   ->  ../Assets/lose_feral.png
//   LOSE_KILLER_INSANE  ->  ../Assets/lose_insane.png
//   LOSE_KILLER_MOLE    ->  ../Assets/lose_mole.png
//
// Each image is drawn at full 1600x900 in screen space (camera reset to 0,0).
// A semi-transparent black rectangle is drawn underneath to dim the world.
// A short "Press SPACE / ENTER to continue" hint is drawn at the bottom.
//
// The screen auto-dismisses after LOSESCREEN_DISPLAY_TIME seconds OR
// immediately when SPACE, ENTER, or left mouse button is pressed.
// ---------------------------------------------------------------------------

#include "losescreen.hpp"
#include "AEEngine.h"
#include <stdio.h>
#include <math.h>   // fabsf

// ---------------------------------------------------------------------------
// GLOBALS  (declared extern in losescreen.hpp)
// ---------------------------------------------------------------------------

int            lose_screen_is_active = 0;
LoseKillerType lose_screen_killer = LOSE_KILLER_FERAL;

// ---------------------------------------------------------------------------
// PRIVATE - state
// ---------------------------------------------------------------------------

static s8   g_font_id = -1;
static float g_timer = 0.0f;   // counts UP; screen hides when >= LOSESCREEN_DISPLAY_TIME
static int   g_dismissed = 0;      // set to 1 the frame the player dismisses

// ---------------------------------------------------------------------------
// PRIVATE - resources
// ---------------------------------------------------------------------------

// Full-screen quad mesh (unit square, scaled at draw time)
static AEGfxVertexList* g_overlayMesh = NULL;   // solid black rect
static AEGfxVertexList* g_imageMesh = NULL;   // textured rect for the lose image

// One texture per killer type
static AEGfxTexture* g_feralTex = NULL;
static AEGfxTexture* g_insaneTex = NULL;
static AEGfxTexture* g_moleTex = NULL;
static AEGfxTexture* g_oxygenTex = NULL;

// ---------------------------------------------------------------------------
// PRIVATE - mesh builders (identical pattern to the rest of the project)
// ---------------------------------------------------------------------------

static AEGfxVertexList* MakeSolidRect(float w, float h, unsigned int col)
{
    float hw = w * 0.5f, hh = h * 0.5f;
    AEGfxMeshStart();
    AEGfxTriAdd(-hw, -hh, col, 0.0f, 1.0f,
        hw, -hh, col, 1.0f, 1.0f,
        -hw, hh, col, 0.0f, 0.0f);
    AEGfxTriAdd(hw, -hh, col, 1.0f, 1.0f,
        hw, hh, col, 1.0f, 0.0f,
        -hw, hh, col, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

static AEGfxVertexList* MakeTexRect(float w, float h)
{
    float hw = w * 0.5f, hh = h * 0.5f;
    AEGfxMeshStart();
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, 0.0f, 1.0f,
        hw, -hh, 0xFFFFFFFF, 1.0f, 1.0f,
        -hw, hh, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, 1.0f, 1.0f,
        hw, hh, 0xFFFFFFFF, 1.0f, 0.0f,
        -hw, hh, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// PRIVATE - draw a screen-space rectangle (camera must be at 0,0 first)
// ---------------------------------------------------------------------------
static void DrawScreenRect(AEGfxVertexList* mesh,
    float cx, float cy,
    float w, float h)
{
    AEMtx33 s, t, xf;
    AEMtx33Scale(&s, w, h);
    AEMtx33Trans(&t, cx, cy);
    AEMtx33Concat(&xf, &t, &s);
    AEGfxSetTransform(xf.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// ---------------------------------------------------------------------------
// LoseScreen_Load
// ---------------------------------------------------------------------------
void LoseScreen_Load(s8 font_id)
{
    g_font_id = font_id;

    // Build meshes
    // Overlay: unit square (1x1) - scaled to fill screen at draw time
    g_overlayMesh = MakeSolidRect(1.0f, 1.0f, 0xFF000000);

    // Image quad: exactly screen-sized so no scaling needed
    g_imageMesh = MakeTexRect(LOSESCREEN_WIDTH, LOSESCREEN_HEIGHT);

    if (!g_overlayMesh || !g_imageMesh)
        printf("LoseScreen_Load: mesh creation failed!\n");

    // Load per-enemy textures
    // Expected filenames  (place your art in the Assets folder with these names):
    //   lose_feral.png   lose_insane.png   lose_mole.png
    g_feralTex = AEGfxTextureLoad("../Assets/lose_feral.png");
    g_insaneTex = AEGfxTextureLoad("../Assets/lose_insane.png");
    g_moleTex = AEGfxTextureLoad("../Assets/lose_mole.png");
    g_oxygenTex = AEGfxTextureLoad("../Assets/oxygendeath.png");

    if (!g_feralTex)  printf("LoseScreen_Load: lose_feral.png not found  - will show overlay only.\n");
    if (!g_insaneTex) printf("LoseScreen_Load: lose_insane.png not found - will show overlay only.\n");
    if (!g_moleTex)   printf("LoseScreen_Load: lose_mole.png not found   - will show overlay only.\n");
    if (!g_oxygenTex) printf("LoseScreen_Load: oxygendeath.png not found - will show overlay only.\n");

    lose_screen_is_active = 0;
    lose_screen_killer = LOSE_KILLER_FERAL;
    g_timer = 0.0f;
    g_dismissed = 0;

    printf("LoseScreen_Load: complete.\n");
}

// ---------------------------------------------------------------------------
// LoseScreen_Unload
// ---------------------------------------------------------------------------
void LoseScreen_Unload()
{
    if (g_overlayMesh) { AEGfxMeshFree(g_overlayMesh);  g_overlayMesh = NULL; }
    if (g_imageMesh) { AEGfxMeshFree(g_imageMesh);    g_imageMesh = NULL; }

    if (g_feralTex) { AEGfxTextureUnload(g_feralTex);  g_feralTex = NULL; }
    if (g_insaneTex) { AEGfxTextureUnload(g_insaneTex); g_insaneTex = NULL; }
    if (g_moleTex) { AEGfxTextureUnload(g_moleTex);   g_moleTex = NULL; }
    if (g_oxygenTex) { AEGfxTextureUnload(g_oxygenTex); g_oxygenTex = NULL; }

    printf("LoseScreen_Unload: all resources freed.\n");
}

// ---------------------------------------------------------------------------
// LoseScreen_Trigger
// ---------------------------------------------------------------------------
void LoseScreen_Trigger(LoseKillerType killer)
{
    if (lose_screen_is_active) return;  // already showing, don't re-trigger

    lose_screen_killer = killer;
    lose_screen_is_active = 1;
    g_timer = 0.0f;
    g_dismissed = 0;

    const char* names[] = { "Feral", "Insane", "Mole", "Oxygen" };
    printf("LoseScreen_Trigger: killed by %s.\n", names[(int)killer]);
}

// ---------------------------------------------------------------------------
// LoseScreen_Update
// ---------------------------------------------------------------------------
int LoseScreen_Update()
{
    if (!lose_screen_is_active) return LOSE_RESULT_NONE;

    float dt = (float)AEFrameRateControllerGetFrameTime();
    g_timer += dt;

    // Check for dismiss input
    int space_pressed = AEInputCheckTriggered(AEVK_SPACE);
    int enter_pressed = AEInputCheckTriggered(AEVK_RETURN);
    int lmb_pressed = AEInputCheckTriggered(VK_LBUTTON);

    // Auto-dismiss when timer expires (if LOSESCREEN_DISPLAY_TIME > 0)
    int timer_expired = (LOSESCREEN_DISPLAY_TIME > 0.0f &&
        g_timer >= LOSESCREEN_DISPLAY_TIME);

    if (space_pressed || enter_pressed || lmb_pressed || timer_expired)
    {
        lose_screen_is_active = 0;
        g_dismissed = 1;
        g_timer = 0.0f;
        return LOSE_RESULT_CONTINUE;
    }

    return LOSE_RESULT_NONE;
}

// ---------------------------------------------------------------------------
// LoseScreen_Draw
// ---------------------------------------------------------------------------
void LoseScreen_Draw()
{
    if (!lose_screen_is_active) return;

    // Always draw in screen space - reset camera to origin
    AEGfxSetCamPosition(0.0f, 0.0f);

    // -----------------------------------------------------------------------
    // 1. Semi-transparent black overlay (dims the world behind the image)
    // -----------------------------------------------------------------------
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(LOSESCREEN_OVERLAY_ALPHA);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    DrawScreenRect(g_overlayMesh, 0.0f, 0.0f,
        LOSESCREEN_WIDTH, LOSESCREEN_HEIGHT);

    // -----------------------------------------------------------------------
    // 2. Killer-specific lose image (full screen)
    // -----------------------------------------------------------------------
    AEGfxTexture* tex = NULL;
    switch (lose_screen_killer)
    {
    case LOSE_KILLER_FERAL:  tex = g_feralTex;  break;
    case LOSE_KILLER_INSANE: tex = g_insaneTex; break;
    case LOSE_KILLER_MOLE:   tex = g_moleTex;   break;
    case LOSE_KILLER_OXYGEN:  tex = g_oxygenTex;  break;
    default:                 tex = g_feralTex;  break;
    }

    if (tex && g_imageMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(tex, 0.0f, 0.0f);

        DrawScreenRect(g_imageMesh, 0.0f, 0.0f,
            1.0f, 1.0f);   // mesh is already full-screen sized
    }
    else
    {
        // Fallback: no texture - show a coloured tinted overlay per killer
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.5f);

        switch (lose_screen_killer)
        {
        case LOSE_KILLER_FERAL:
            AEGfxSetColorToMultiply(0.9f, 0.3f, 0.0f, 1.0f); // orange tint
            break;
        case LOSE_KILLER_INSANE:
            AEGfxSetColorToMultiply(0.7f, 0.0f, 0.0f, 1.0f); // deep red tint
            break;
        case LOSE_KILLER_MOLE:
            AEGfxSetColorToMultiply(0.3f, 0.15f, 0.0f, 1.0f); // brown tint
            break;
        case LOSE_KILLER_OXYGEN:
            AEGfxSetColorToMultiply(0.0f, 0.4f, 0.8f, 1.0f); // blue tint
            break;
        default:
            AEGfxSetColorToMultiply(0.5f, 0.0f, 0.0f, 1.0f);
            break;
        }

        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        DrawScreenRect(g_overlayMesh, 0.0f, 0.0f,
            LOSESCREEN_WIDTH, LOSESCREEN_HEIGHT);
    }

    // -----------------------------------------------------------------------
    // 3. Text: enemy name + "YOU DIED" + prompt
    // -----------------------------------------------------------------------
    if (g_font_id >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // "YOU DIED" - large, centred near the top third
        AEGfxPrint(g_font_id,
            (char*)"YOU DIED",
            -0.14f, 0.25f,          // x, y in normalised screen coords 
            2.0f,                   // scale
            1.0f, 0.15f, 0.15f, 1.0f);  // RGBA (blood red)

        // Enemy name subtitle
        const char* killer_names[] = {
            "Killed by: The Feral",
            "Killed by: The Insane",
            "Killed by: The Mole",
            "Killed by: Oxygen Deprivation"
        };
        AEGfxPrint(g_font_id,
            (char*)killer_names[(int)lose_screen_killer],
            -0.30f, 0.0f, //ori is -.2,0
            1.2f,
            1.0f, 0.8f, 0.8f, 1.0f);

        // Timer / prompt line at the bottom
        if (LOSESCREEN_DISPLAY_TIME > 0.0f)
        {
            // Show a countdown
            float remaining = LOSESCREEN_DISPLAY_TIME - g_timer;
            if (remaining < 0.0f) remaining = 0.0f;

            // Use a static char buffer - no dynamic allocation
            char prompt[64];
            // Print integer seconds remaining to avoid printf float issues
            int secs = (int)remaining + 1;
            if (secs < 1) secs = 1;

            // Build the string manually to keep it simple
            char* p = prompt;
            const char* prefix = "Respawning in ";
            while (*prefix) *p++ = *prefix++;
            *p++ = (char)('0' + (secs % 10));
            *p++ = 's';
            *p++ = ' ';
            *p++ = '(';
            const char* suffix = "SPACE / ENTER to skip)";
            while (*suffix) *p++ = *suffix++;
            *p = '\0';

            AEGfxPrint(g_font_id,
                prompt,
                -0.30f, -0.55f, // original is -0.25, -0.35
                0.85f,
                0.9f, 0.9f, 0.9f, 1.0f);
        }
        else
        {
            AEGfxPrint(g_font_id,
                (char*)"Press SPACE / ENTER to respawn",
                -0.38f, -0.35f,
                0.85f,
                0.9f, 0.9f, 0.9f, 1.0f);
        }
    }
}