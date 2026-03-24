#include "AEEngine.h"
#include "pausemenu.hpp"
#include <stdio.h>

// ---------------------------------------------------------------------------
// PUBLIC STATE
int pause_is_active = 0;

// ---------------------------------------------------------------------------
// PRIVATE CONSTANTS

// Screen dimensions – match main.cpp
#define PM_SCREEN_W   1600.0f
#define PM_SCREEN_H    900.0f

// Panel (the dark centred box)
#define PM_PANEL_W     500.0f
#define PM_PANEL_H     380.0f

// Buttons  (pill-shaped, same style as the main menu)
#define PM_BTN_W       360.0f
#define PM_BTN_H        70.0f
#define PM_BTN_HALF_W  (PM_BTN_W * 0.5f) 
#define PM_BTN_HALF_H  (PM_BTN_H * 0.5f) 
#define PM_BTN_SEGMENTS 12          // semicircle smoothness

// Vertical positions in AE normalised coords (0 = centre, ±1 = edge)
// These are passed to AEGfxPrint; button hit-boxes use pixel coords.
//----------------------------------------------------------------------------
//____________________________________________________________________________
#define PM_RESUME_Y_NORM    0.0f
#define PM_QUIT_Y_NORM     -0.20f 

// Pixel Y for hit-testing (AE origin is screen centre, Y grows upward)
#define PM_RESUME_Y_PX    ( 0.03f * (PM_SCREEN_H * 0.5f) ) // height of the button hover over resume text
#define PM_QUIT_Y_PX      ( -0.17f * (PM_SCREEN_H * 0.5f) ) //height of the button hover over quit text

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// PRIVATE DATA

static s8  pm_font = -1;

// Meshes
static AEGfxVertexList* pm_overlay_mesh = NULL;   // full-screen dim
static AEGfxVertexList* pm_panel_mesh = NULL;   // centred box
static AEGfxVertexList* pm_btn_mesh = NULL;   // normal button
static AEGfxVertexList* pm_btn_hov_mesh = NULL;   // hovered button

// Hover flags
static int pm_resume_hovered = 0;
static int pm_quit_hovered = 0;

// ---------------------------------------------------------------------------
// INTERNAL HELPERS

// Build a solid-colour rectangle mesh centred at origin.
static AEGfxVertexList* PM_CreateRect(float w, float h, unsigned int colour)
{
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    AEGfxMeshStart();
    AEGfxTriAdd(-hw, -hh, colour, 0.0f, 1.0f,
        hw, -hh, colour, 1.0f, 1.0f,
        -hw, hh, colour, 0.0f, 0.0f);
    AEGfxTriAdd(hw, -hh, colour, 1.0f, 1.0f,
        hw, hh, colour, 1.0f, 0.0f,
        -hw, hh, colour, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

// Build a pill-shaped (stadium) button mesh to match the main menu style.
static AEGfxVertexList* PM_CreatePill(unsigned int colour)
{
    float hw = PM_BTN_HALF_W;
    float hh = PM_BTN_HALF_H;

    AEGfxMeshStart();

    // Centre rectangle body
    AEGfxTriAdd(-(hw - hh), -hh, colour, 0.0f, 1.0f,
        (hw - hh), -hh, colour, 1.0f, 1.0f,
        -(hw - hh), hh, colour, 0.0f, 0.0f);
    AEGfxTriAdd((hw - hh), -hh, colour, 1.0f, 1.0f,
        (hw - hh), hh, colour, 1.0f, 0.0f,
        -(hw - hh), hh, colour, 0.0f, 0.0f);

    // Left semicircle
    for (int i = 0; i < PM_BTN_SEGMENTS; ++i)
    {
        float a1 = 0.5f * 3.14159f + (float)i * 3.14159f / PM_BTN_SEGMENTS;
        float a2 = 0.5f * 3.14159f + (float)(i + 1) * 3.14159f / PM_BTN_SEGMENTS;
        AEGfxTriAdd(-(hw - hh), 0.0f, colour, 0.5f, 0.5f,
            -(hw - hh) + hh * cosf(a1), hh * sinf(a1), colour, 0.0f, 0.5f,
            -(hw - hh) + hh * cosf(a2), hh * sinf(a2), colour, 0.0f, 0.5f);
    }

    // Right semicircle
    for (int i = 0; i < PM_BTN_SEGMENTS; ++i)
    {
        float a1 = -0.5f * 3.14159f + (float)i * 3.14159f / PM_BTN_SEGMENTS;
        float a2 = -0.5f * 3.14159f + (float)(i + 1) * 3.14159f / PM_BTN_SEGMENTS;
        AEGfxTriAdd((hw - hh), 0.0f, colour, 0.5f, 0.5f,
            (hw - hh) + hh * cosf(a1), hh * sinf(a1), colour, 1.0f, 0.5f,
            (hw - hh) + hh * cosf(a2), hh * sinf(a2), colour, 1.0f, 0.5f);
    }

    return AEGfxMeshEnd();
}

// Draw a mesh at a given world position (camera locked to 0,0).
static void PM_DrawMesh(AEGfxVertexList* mesh, float x, float y,
    float scaleX, float scaleY, float alpha)
{
    AEMtx33 s, r, t, transform;
    AEMtx33Scale(&s, scaleX, scaleY);
    AEMtx33Rot(&r, 0.0f);
    AEMtx33Trans(&t, x, y);
    AEMtx33Concat(&transform, &r, &s);
    AEMtx33Concat(&transform, &t, &transform);

    AEGfxSetTransparency(alpha);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// Simple AABB point-in-box using AE pixel coords (origin = screen centre, Y up).
static int PM_PointInBox(float px, float py,
    float boxX, float boxY,
    float halfW, float halfH)
{
    return (px >= boxX - halfW && px <= boxX + halfW &&
        py >= boxY - halfH && py <= boxY + halfH);
}

// ---------------------------------------------------------------------------
// PUBLIC FUNCTIONS

void PauseMenu_Load(s8 font_id)
{
    pm_font = font_id;

    pause_is_active = 0;

    // Full-screen dim overlay: near-black, semi-transparent
    pm_overlay_mesh = PM_CreateRect(PM_SCREEN_W, PM_SCREEN_H, 0xFF000000);

    // Centred dark panel
    pm_panel_mesh = PM_CreateRect(PM_PANEL_W, PM_PANEL_H, 0xFF1A1A1A);

    // Buttons – transparent fill like the main menu (text carries the colour)
    pm_btn_mesh = PM_CreatePill(0x00AAAAAA);   // transparent normal
    pm_btn_hov_mesh = PM_CreatePill(0x44FFFFFF);   // slight white tint on hover
}

void PauseMenu_Unload()
{
    if (pm_overlay_mesh) { AEGfxMeshFree(pm_overlay_mesh);  pm_overlay_mesh = NULL; }
    if (pm_panel_mesh) { AEGfxMeshFree(pm_panel_mesh);    pm_panel_mesh = NULL; }
    if (pm_btn_mesh) { AEGfxMeshFree(pm_btn_mesh);      pm_btn_mesh = NULL; }
    if (pm_btn_hov_mesh) { AEGfxMeshFree(pm_btn_hov_mesh);  pm_btn_hov_mesh = NULL; }
    pm_font = -1;
}

int PauseMenu_Update()
{
    // Get mouse position in AE pixel coords (origin = screen centre, Y up)
    s32 mx_raw, my_raw;
    AEInputGetCursorPosition(&mx_raw, &my_raw);
    float mx = (float)mx_raw - PM_SCREEN_W * 0.5f;
    float my = -(float)my_raw + PM_SCREEN_H * 0.5f;

    pm_resume_hovered = PM_PointInBox(mx, my, 0.0f, PM_RESUME_Y_PX,
        PM_BTN_HALF_W, PM_BTN_HALF_H);
    pm_quit_hovered = PM_PointInBox(mx, my, 0.0f, PM_QUIT_Y_PX,
        PM_BTN_HALF_W, PM_BTN_HALF_H);

    if (AEInputCheckTriggered(AEVK_Q))
        return PAUSE_RESULT_RESUME;

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (pm_resume_hovered) return PAUSE_RESULT_RESUME;
        if (pm_quit_hovered)   return PAUSE_RESULT_QUIT;
    }

    return PAUSE_RESULT_NONE;
}

void PauseMenu_Draw()
{
    // Lock the camera to screen-space for the overlay
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // --- Full-screen dim ---
    PM_DrawMesh(pm_overlay_mesh, 0.0f, 0.0f, 1.0f, 1.0f, 0.55f);

    // --- Dark panel ---
    PM_DrawMesh(pm_panel_mesh, 0.0f, 0.0f, 1.0f, 1.0f, 0.85f);

    // --- Resume button ---
    {
        AEGfxVertexList* mesh = pm_resume_hovered ? pm_btn_hov_mesh : pm_btn_mesh;
        PM_DrawMesh(mesh, 0.0f, PM_RESUME_Y_PX, 1.0f, 1.0f, 1.0f);
    }

    // --- Quit button ---
    {
        AEGfxVertexList* mesh = pm_quit_hovered ? pm_btn_hov_mesh : pm_btn_mesh;
        PM_DrawMesh(mesh, 0.0f, PM_QUIT_Y_PX, 1.0f, 1.0f, 1.0f);
    }

    // --- Text (drawn last so it sits on top of everything) ---
    if (pm_font >= 0)
    {
        char buf[64];

        // Title
        sprintf_s(buf, "PAUSED");
        AEGfxPrint(pm_font, buf, -0.12f, 0.22f, 2.2f, 1.0f, 1.0f, 1.0f, 1.0f);

        // Resume
        sprintf_s(buf, "RESUME");
        if (pm_resume_hovered)
            AEGfxPrint(pm_font, buf, -0.1f, PM_RESUME_Y_NORM, 1.7f, 0.0f, 1.0f, 0.4f, 1.0f); // green
        else
            AEGfxPrint(pm_font, buf, -0.1f, PM_RESUME_Y_NORM, 1.7f, 1.0f, 1.0f, 1.0f, 1.0f); // white

        // Quit to Menu
        sprintf_s(buf, "QUIT TO MENU");
        if (pm_quit_hovered)
            AEGfxPrint(pm_font, buf, -0.17f, PM_QUIT_Y_NORM, 1.5f, 1.0f, 0.3f, 0.3f, 1.0f); // red
        else
            AEGfxPrint(pm_font, buf, -0.17f, PM_QUIT_Y_NORM, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f); // white

        // Hint
        sprintf_s(buf, "Q to resume");
        AEGfxPrint(pm_font, buf, -0.10f, -0.57f, 0.9f, 0.6f, 0.6f, 0.6f, 1.0f);
    }
}