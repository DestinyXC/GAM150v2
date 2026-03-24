#pragma once
#ifndef LOSESCREEN_HPP
#define LOSESCREEN_HPP

#include "AEEngine.h"

// ---------------------------------------------------------------------------
// losescreen.hpp  -  Per-enemy lose screen for Core Break
// ---------------------------------------------------------------------------
//
// HOW IT WORKS
// ------------
// When the enemy system kills the player, call LoseScreen_Trigger(killer_type)
// before Enemy_Update gets a chance to reset player_is_dead.
// The lose screen blocks normal gameplay for LOSESCREEN_DISPLAY_TIME seconds,
// showing a full-screen image specific to the killer, then auto-resumes OR
// dismisses immediately when the player presses SPACE / ENTER / left-click.
//
// Lifecycle (mirrors the pause menu pattern already in the project):
//
//   Game_Init  ->  LoseScreen_Load(font_id)
//   Game_Kill  ->  LoseScreen_Unload()
//
//   Game_Update  (every frame):
//       // Check BEFORE Enemy_Update so we catch the death flag first.
//       if (lose_screen_is_active)
//       {
//           if (LoseScreen_Update() == LOSE_RESULT_CONTINUE)
//               lose_screen_is_active = 0;  // let normal flow resume
//           return;  // skip the rest of Game_Update while screen is up
//       }
//       // Detect kill (player_hp hits 0 but flag not yet set by Enemy_Update)
//       if (player_hp <= 0.0f && !player_is_dead)
//           LoseScreen_Trigger(lose_screen_killer_pending);
//       Enemy_Update(dt, player_x, player_y, &player_x, &player_y);
//       ...
//
//   Game_Draw  (every frame, LAST so it renders on top):
//       if (lose_screen_is_active)
//           LoseScreen_Draw();
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// CONFIGURATION  -  tweak to taste
// ---------------------------------------------------------------------------

// Seconds the screen stays up before auto-dismissing.
// Set to 0.0f to require a key press every time.
#define LOSESCREEN_DISPLAY_TIME     4.0f

// Full-screen overlay dimensions (should match your window size).
#define LOSESCREEN_WIDTH            1600.0f
#define LOSESCREEN_HEIGHT           900.0f

// Opacity of the dark background drawn behind the lose image (0-1).
#define LOSESCREEN_OVERLAY_ALPHA    0.80f

// ---------------------------------------------------------------------------
// RESULT CODES
// ---------------------------------------------------------------------------
#define LOSE_RESULT_NONE        0   // Still showing  - skip gameplay this frame
#define LOSE_RESULT_CONTINUE    1   // Dismissed       - allow respawn to proceed

// ---------------------------------------------------------------------------
// KILLER TYPE
// Mirrors EnemyType in enemy.hpp but kept independent so include order
// does not matter.
// ---------------------------------------------------------------------------
typedef enum
{
    LOSE_KILLER_FERAL = 0,   // The Feral
    LOSE_KILLER_INSANE = 1,   // The Insane
    LOSE_KILLER_MOLE = 2,    // The Mole
    LOSE_KILLER_OXYGEN = 3
} LoseKillerType;

// ---------------------------------------------------------------------------
// GLOBALS  (defined in losescreen.cpp)
// ---------------------------------------------------------------------------

// 1 while the overlay is visible, 0 otherwise.
// Read this in Game_Update / Game_Draw to skip normal logic.
extern int           lose_screen_is_active;

// Which enemy triggered the current (or last) lose screen.
extern LoseKillerType lose_screen_killer;

// ---------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ---------------------------------------------------------------------------

// Allocate meshes and load the three lose-screen textures.
// Call once during Game_Init.  font_id is the same one used everywhere else.
void LoseScreen_Load(s8 font_id);

// Free all resources.  Call once during Game_Kill.
void LoseScreen_Unload();

// Activate the lose screen for a specific killer enemy.
// Call as soon as player_hp <= 0 is detected (before Enemy_Update resets it).
void LoseScreen_Trigger(LoseKillerType killer);

// Per-frame update while the screen is active.
// Returns LOSE_RESULT_CONTINUE once dismissed; LOSE_RESULT_NONE otherwise.
int  LoseScreen_Update();

// Per-frame draw.  Call LAST in Game_Draw so it sits on top of everything.
void LoseScreen_Draw();

#endif // LOSESCREEN_HPP