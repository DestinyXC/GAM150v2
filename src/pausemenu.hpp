#ifndef PAUSEMENU_H
#define PAUSEMENU_H
#include "AEEngine.h"

// ---------------------------------------------------------------------------
// Pause Menu
// Uses Alpha Engine (AEEngine) to match the rest of the project.
// ---------------------------------------------------------------------------

// ---- State -----------------------------------------------------------------
// 1 while the pause overlay is visible, 0 otherwise.
// Read this in mainmenu.cpp / Game_Update to skip normal updates.
extern int pause_is_active;

// ---- Result codes returned by PauseMenu_Update ----------------------------
#define PAUSE_RESULT_NONE     0   // Still paused, do nothing
#define PAUSE_RESULT_RESUME   1   // Player chose Resume  ? unpause
#define PAUSE_RESULT_QUIT     2   // Player chose Quit    ? go back to main menu

// ---- Lifecycle -------------------------------------------------------------
// Call once when entering gameplay (after Game_Init).
void PauseMenu_Load(s8 font_id);

// Call once when leaving gameplay (before / inside Game_Kill).
void PauseMenu_Unload();

// ---- Per-frame -------------------------------------------------------------
// Call every frame while paused.  Returns one of the PAUSE_RESULT_* codes.
int  PauseMenu_Update();

// Call every frame while paused, AFTER Game_Draw so the overlay is on top.
void PauseMenu_Draw();

#endif // PAUSEMENU_H