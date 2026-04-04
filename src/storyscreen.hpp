#ifndef STORYSCREEN_HPP
#define STORYSCREEN_HPP

#include "AEEngine.h"

// ---------------------------------------------------------------------------
// storyscreen.hpp  -  4-panel story cutscene for Core Break
// ---------------------------------------------------------------------------
//
// HOW IT WORKS
// ------------
// Sits between the main menu and gameplay as a GS_STORY game state.
// Four full-screen panel images are shown one at a time.
// The player clicks (LMB / SPACE / ENTER) to advance through them.
// After the final panel the caller should call Game_Init() and switch
// to GS_GAMEPLAY.
//
// ASSET PATHS (place your PNGs in ../Assets/Story/):
//   story_panel1.png
//   story_panel2.png
//   story_panel3.png
//   story_panel4.png
//
// LIFECYCLE
// ---------
//   Before entering GS_STORY:
//       StoryScreen_Load(fontId);
//       StoryScreen_Reset();
//
//   Each frame while in GS_STORY:
//       int result = StoryScreen_Update();
//       StoryScreen_Draw();
//       if (result == STORY_RESULT_DONE)  { ... switch to GS_GAMEPLAY }
//
//   When leaving gameplay (Game_Kill):
//       StoryScreen_Unload();
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CONFIGURATION
// ---------------------------------------------------------------------------

#define STORY_PANEL_COUNT       4
#define STORY_SCREEN_WIDTH      1600.0f
#define STORY_SCREEN_HEIGHT     900.0f

// How long each panel must be visible before a click is accepted (anti-skip)
#define STORY_MIN_PANEL_TIME    0.3f

// ---------------------------------------------------------------------------
// RESULT CODES
// ---------------------------------------------------------------------------
#define STORY_RESULT_NONE       0   // Still showing panels
#define STORY_RESULT_DONE       1   // All panels shown - enter game

// ---------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ---------------------------------------------------------------------------

// Load textures and build meshes. Call once before entering GS_STORY.
// Pass the same font ID used everywhere else in the project.
void StoryScreen_Load(s8 font_id);

// Free all resources. Call when the story screen will no longer be needed.
void StoryScreen_Unload();

// Reset to panel 0. Call each time the player starts a new run.
void StoryScreen_Reset();

// Per-frame update. Returns STORY_RESULT_DONE when all panels are dismissed.
int  StoryScreen_Update();

// Per-frame draw. Call every frame while in GS_STORY.
void StoryScreen_Draw();

#endif // STORYSCREEN_HPP
