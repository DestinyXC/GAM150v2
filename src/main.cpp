#include <crtdbg.h>
#include "AEEngine.h"
#include <stdio.h>
#include <time.h>
#include "shop.hpp"
#include "enemy.hpp"
#include "pausemenu.hpp"
#include "boss.hpp"
#include "animation.hpp"
#include "door.hpp"
#include "lightsystem.hpp"
#include "losescreen.hpp"
#include "player.hpp"

// ===========================================================================
// FORWARD DECLARATIONS
// =========================================================================== 
void Game_Kill(void);

// ===========================================================================
// AUDIO
// =========================================================================== 
static AEAudio g_miningSound;
static AEAudioGroup g_sfxGroup;
static int g_is_playing_mining = 0;  // track if sound is playing

static AEAudio g_bgmSound;
static AEAudioGroup g_bgmGroup;

static AEAudio g_enemyZoneSound;
static AEAudioGroup g_enemyZoneGroup;
static int g_is_playing_enemyzone = 0;

// ===========================================================================
// CONFIGURATION
// ===========================================================================

#define MAP_WIDTH 17       // Fixed screen width in tiles (25 * 64 = 1600)
#define MAP_HEIGHT 100        // Deep vertical map
#define TILE_SIZE 64.0f

// Spritesheet settings (256x192, 16x16 tiles)
#define SPRITESHEET_WIDTH 256
#define SPRITESHEET_HEIGHT 192
#define SPRITE_SIZE 16
#define SPRITESHEET_COLUMNS 16    
#define SPRITESHEET_ROWS 12     

// Screen
#define SCREEN_WIDTH 1600.0f
#define SCREEN_HEIGHT 900.0f

// Lighting configurations
#define MAX_TORCHES 50
#define TORCH_GLOW_RADIUS 200.0f;
#define HEADLAMP_GLOW_RADIUS 180.0f;

// Mineable rocks configurations
#define MAX_ROCKS 500
#define ROCK_SPAWN_CHANCE 0.2f  

// Throwing rocks (attack)
#define MAX_THROWN_ROCKS 16
#define THROWN_ROCK_SPEED 600.0f
#define THROWN_ROCK_DAMAGE 20.0f
#define THROWN_ROCK_SIZE 40.0f

// Particle system 
#define MAX_PARTICLES 200
#define PARTICLES_PER_FRAME 5

// Pickup (visual collectible that moves to the player)
#define MAX_PICKUPS 64

// Rocks drop chance when mining
#define MULTI_ROCK_CHANCE_1 0.4f  // 40% chance to get 1 extra rock (total 2)
#define MULTI_ROCK_CHANCE_2 0.15f // 15% chance to get 2 extra rocks (total 3)

// ===========================================================================
// TILE TYPES
// ===========================================================================

enum
{
    TILE_EMPTY = 5,
    TILE_DIRT = 0,
    TILE_STONE = 1,
    TILE_WALL = 2,
    TILE_LAVA = 3,
    TILE_BOSS = 4
};

// Sprite positions: (row * 16) + col
#define DIRT_SPRITE_POSITION 191    
#define STONE_SPRITE_POSITION 41   
#define WALL_SPRITE_POSITION 16 
#define LAVA_SPRITE_POSITION 142
#define BOSS_SPRITE_POSITION 189

// ===========================================================================
// STRUCTS
// ===========================================================================

// TORCH STRUCTURE
typedef struct {
    float x;
    float y;
    int active;
    float glow_radius;
    float flicker_time;
    float flicker_offset;
} Torch;

// ROCK STRUCTURE
typedef struct {
    float x;
    float y;
    int active;
} Rock;

// SHOVEL STRUCTURE
typedef struct {
    float x;
    float y;
    int active;
    float float_time; // Floating animation
    float base_y; // Base Y position for floating
} Shovel;

// THROWN ROCK STRUCTURE
typedef struct {
    float x;
    float y;
    float vel_x;
    float vel_y;
    int active;
} ThrownRock;

// PICKUP STRUCTURE (for rocks flying to player when collected)
typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float speed;
    float scale;
    float target_scale; // desired final scale while homing
    float shrink_rate;
    int active;
} Pickup;

// PARTICLE STRUCTURE (for mining particles)
typedef struct {
    float x;
    float y;
    float vel_x;
    float vel_y;
    float lifetime;
    float max_lifetime;
    float scale;
    int active;
} Particle;

// ===========================================================================
// GLOBALS
// ===========================================================================

int tileMap[MAP_HEIGHT][MAP_WIDTH];

// ---- Camera ----
float camera_x = 0.0f;
float camera_y = 0.0f;
float camera_smoothness = 0.15f;

// ---- Player ----
float player_x = 0.0f;
float player_y = 0.0f;
float player_width = 90.0f;
float player_height = 90.0f;
float player_speed = 5.0f;

// ---- Mining ----
float mining_range = 120.0f;
int currently_mining_row = -1;
int currently_mining_col = -1;
int current_mining_rock = -1;
float rock_mining_timer = 0.0f; 
float rock_mining_duration = 4.0f; 

// ---- Statistics ----
int depth = 0;
int max_depth = 0;
int rocks_mined = 0;

// ---- Lighting ----
Torch torches[MAX_TORCHES];
int torch_count = 0;
float game_time = 0.0f;

// ---- Rocks ----
Rock rocks[MAX_ROCKS];
int rock_count = 0;

// ---- Rock Throwing (hold RMB to aim) ----
ThrownRock thrown_rocks[MAX_THROWN_ROCKS];
int is_aiming = 0;
float aim_dir_x = 1.0f;
float aim_dir_y = 0.0f;

// ---- Pickup rocks (for rocks flying to player when collected) ----
Pickup pickups[MAX_PICKUPS];
int pickup_count = 0;

// ---- Mining particles ----
Particle particles[MAX_PARTICLES];
int particle_count = 0;

// ---- Shovels ----
Shovel shovel, shovel2;
int shovel_collected = 0;
int shovel2_collected = 0;
float shovel_float_speed = 2.0f; // Speed of up/down floating motion
float shovel_float_amplitude = 10.0f; // How high up/down the shovel floats
float shovel_width = 80.0f;
float shovel_height = 80.0f;
float shovel2_width = 90.0f;
float shovel2_height = 85.0f;

// ---- Shovel digging system ----
int is_shovel_digging = 0; 
float shovel_dig_timer = 0.0f;
float shovel_dig_duration = 4.0f; 
int shovel_target_row = -1;
int shovel_target_col = -1;
int show_no_rocks_message = 0;
float no_rocks_message_timer = 0.0f;
float no_rocks_message_duration = 2.0f;

// ---- Shovel collection banners ----
int show_shovel_banner = 0;
int show_shovel2_banner = 0;
float banner_display_time = 0.0f;
float banner2_display_time = 0.0f;
float banner_duration = 3.0f; 
float banner_width = 400.0f;
float banner_height = 100.0f;

// ---- HUD icon dimensions ----
float oxygeniconwidth = 100.0f, oxygeniconheight = 100.0f;
float sanityiconwidth = 100.0f, sanityiconheight = 100.0f;
float bouldericonwidth = 100.0f, bouldericonheight = 100.0f;
float mapiconwidth = 500.0f, mapiconheight = 200.0f;

// ---- Oxygen system ----
float oxygen_percentage = 10.0f;
float oxygen_max = 10.0f;
float oxygen_drain_rate = 1.0f;  
float oxygen_refill_rate = 3.0f; 

// ---- Safe zone ----
float safezone_x_min = -500.0f;
float safezone_x_max = 500.0f;
float safezone_y_min = -3200.0f;
float safezone_y_max = -2530.0f;

// ---- Guide banners ----
int show_guide_banner = 0;
int first_exit_triggered = 0;
float guide_banner_width = 500.0f;
float guide_banner_height = 400.0f;

int show_first_shovel_banner = 0;
int first_shovel_banner_triggered = 0;

int show_second_shovel_banner = 0;
int second_shovel_banner_triggered = 0;
float second_shovel_banner_width = 700.0f;
float second_shovel_banner_height = 300.0f;

// ---- Fall-through system ----
float last_safe_y = 0.0f;
float last_move_dir_y = 0.0f;
int is_falling_through = 0;
float fall_through_timer = 0.0f;
float fall_through_duration = 0.6f;

// --- Danger tint ---
float danger_tint_intensity = 0.0f;
float danger_tint_fade_out = 1.0f;

//  ---- Oxygen death ----
int oxygen_killed_player = 0;

// ---- Light radius ----
float player_light_radius = TORCH_RADIUS_DEFAULT;

// ---- Player animator & direction ----
PlayerAnimator g_playerAnimator;
int player_facing_left = 1; // 1 = right, 0 = left

// --- Shop ---
int player_in_shop_zone = 0;
float shop_trigger_x = 400.0f;
float shop_trigger_y = -2700.0f;
float shop_trigger_width = 160.0f;
float shop_trigger_height = 150.0f;
float shop_solid_x = 400.0f;
float shop_solid_y = -2668.0f;
float shop_solid_width = 150.0f;
float shop_solid_height = 86.0f;

// --- Font ---
s8 g_font_id = -1;

// ===========================================================================
// FILE-SCOPE TEXTURES & MESHES
// ===========================================================================

// Textures (alphabetical order)
static AEGfxTexture* g_bouldericonTexture = NULL;
static AEGfxTexture* g_glowTexture = NULL;
static AEGfxTexture* g_guideBannerTexture = NULL;
static AEGfxTexture* g_mapiconTexture = NULL;
static AEGfxTexture* g_oxygeniconTexture = NULL;
static AEGfxTexture* g_playerTexture = NULL;
static AEGfxTexture* g_rockTexture = NULL;
static AEGfxTexture* g_sanityiconTexture = NULL;
static AEGfxTexture* g_shovelTexture = NULL;
static AEGfxTexture* g_shovel2Texture = NULL;
static AEGfxTexture* g_shovelBannerTexture = NULL;
static AEGfxTexture* g_shovel2BannerTexture = NULL;
static AEGfxTexture* g_shopTriggerTexture = NULL;
static AEGfxTexture* g_shovelGuideBannerTexture = NULL;
static AEGfxTexture* g_shovelGuideBannerTexture2 = NULL;
static AEGfxTexture* g_tilesetTexture = NULL;
static AEGfxTexture* g_thrownRockTexture = NULL;

// Meshes
static AEGfxVertexList* g_aimCursorMesh = NULL;
static AEGfxVertexList* g_bosstileMesh = NULL;
static AEGfxVertexList* g_bouldericonMesh = NULL;
static AEGfxVertexList* g_cursorMesh = NULL;
static AEGfxVertexList* g_dirtMesh = NULL;
static AEGfxVertexList* g_glowMesh = NULL;
static AEGfxVertexList* g_guideBannerMesh = NULL;
static AEGfxVertexList* g_lavaMesh = NULL;
static AEGfxVertexList* g_leftBlackoutMesh = NULL;
static AEGfxVertexList* g_mapiconMesh = NULL;
static AEGfxVertexList* g_oxygeniconMesh = NULL;
static AEGfxVertexList* g_particleMesh = NULL;
static AEGfxVertexList* g_playerMesh = NULL;
static AEGfxVertexList* g_rightBlackoutMesh = NULL;
static AEGfxVertexList* g_rockMesh = NULL;
static AEGfxVertexList* g_sanityiconMesh = NULL;
static AEGfxVertexList* g_shopTriggerMesh = NULL;
static AEGfxVertexList* g_shovel2BannerMesh = NULL;
static AEGfxVertexList* g_shovel2Mesh = NULL;
static AEGfxVertexList* g_shovelBannerMesh = NULL;
static AEGfxVertexList* g_shovelGuideBannerMesh = NULL;
static AEGfxVertexList* g_shovelGuideBannerMesh2 = NULL;
static AEGfxVertexList* g_shovelMesh = NULL;
static AEGfxVertexList* g_stoneMesh = NULL;
static AEGfxVertexList* g_thrownRockMesh = NULL;
static AEGfxVertexList* g_wallMesh = NULL;

static int g_texture_loaded = 0;

// ===========================================================================
// UV MAPPING
// ===========================================================================

void GetTileUV(int sprite_position, float* u_min, float* v_min, float* u_max, float* v_max)
{
    if (sprite_position < 0)
    {
        *u_min = *v_min = *u_max = *v_max = 0.0f;
        return;
    }

    int col = sprite_position % SPRITESHEET_COLUMNS;
    int row = sprite_position / SPRITESHEET_COLUMNS;

    float tile_width_uv = (float)SPRITE_SIZE / (float)SPRITESHEET_WIDTH;
    float tile_height_uv = (float)SPRITE_SIZE / (float)SPRITESHEET_HEIGHT;

    *u_min = col * tile_width_uv;
    *v_min = row * tile_height_uv;
    *u_max = *u_min + tile_width_uv;
    *v_max = *v_min + tile_height_uv;
}

// ===========================================================================
// MESH CREATION HELPERS
// ===========================================================================

AEGfxVertexList* CreateSpritesheetTileMesh(int sprite_position, float size)
{
    float u_min, v_min, u_max, v_max;
    GetTileUV(sprite_position, &u_min, &v_min, &u_max, &v_max);

    AEGfxMeshStart();

    float half = size / 2.0f;

    AEGfxTriAdd(
        -half, -half, 0xFFFFFFFF, u_min, v_max,
        half, -half, 0xFFFFFFFF, u_max, v_max,
        -half, half, 0xFFFFFFFF, u_min, v_min);

    AEGfxTriAdd(
        half, -half, 0xFFFFFFFF, u_max, v_max,
        half, half, 0xFFFFFFFF, u_max, v_min,
        -half, half, 0xFFFFFFFF, u_min, v_min);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateRectangleMesh(float width, float height, unsigned int color)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, color, 0.0f, 1.0f,
        half_width, -half_height, color, 1.0f, 1.0f,
        -half_width, half_height, color, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, color, 1.0f, 1.0f,
        half_width, half_height, color, 1.0f, 0.0f,
        -half_width, half_height, color, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

// Creates a mesh for rendering a texture with full UVs (player, enemy)
AEGfxVertexList* CreateTextureMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, 0xFFFFFFFF, 0.0f, 1.0f,
        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
        half_width, half_height, 0xFFFFFFFF, 1.0f, 0.0f,
        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateGlowMesh(float size)
{
    AEGfxMeshStart();

    float half = size / 2.0f;

    AEGfxTriAdd(
        -half, -half, 0xFFFFFFFF, 0.0f, 1.0f,
        half, -half, 0xFFFFFFFF, 1.0f, 1.0f,
        -half, half, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        half, -half, 0xFFFFFFFF, 1.0f, 1.0f,
        half, half, 0xFFFFFFFF, 1.0f, 0.0f,
        -half, half, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateSideBlackoutMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, 0xFF000000, 0.0f, 1.0f,
        half_width, -half_height, 0xFF000000, 1.0f, 1.0f,
        -half_width, half_height, 0xFF000000, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, 0xFF000000, 1.0f, 1.0f,
        half_width, half_height, 0xFF000000, 1.0f, 0.0f,
        -half_width, half_height, 0xFF000000, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

// ===========================================================================
// WORLD LOADING & GENERATION
// ===========================================================================

int LoadWorldFromFile(const char* filepath)
{
    FILE* file = NULL;
    if (fopen_s(&file, filepath, "r") != 0 || !file)
    {
        printf("LoadWorldFromFile: Failed to open '%s'. Falling back to procedural generation.\n", filepath);
        return 0;
    }

    // Read into temp buffer first, reverse row order later
    int temp[MAP_HEIGHT][MAP_WIDTH];

    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            int tile_value = 0;
            if (fscanf_s(file, "%d", &tile_value) != 1)
            {
                printf("LoadWorldFromFile: Unexpected end of file at row %d, col %d.\n", row, col);
                fclose(file);
                return 0;
            }
            temp[row][col] = tile_value;
        }
    }

    fclose(file);

    // Reverse row order: row 0 -> tileMap[MAP_HEIGHT - 1]
    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            tileMap[MAP_HEIGHT - 1 - row][col] = temp[row][col];
        }
    }
    printf("LoadWorldFromFile: Map loaded successfully from '%s'.\n", filepath);
    return 1;
}

void GenerateWorld()
{
    srand((unsigned int)time(NULL));

    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            if (col == MAP_WIDTH / 2 && row == 9) {
                tileMap[row][col] = TILE_DIRT;
            }
            else if (col == MAP_WIDTH / 2 - 1 && row == 9) {
                tileMap[row][col] = TILE_DIRT;
            }
            else if (col != MAP_WIDTH - 1 && col != 0 && row == 51) {
                tileMap[row][col] = TILE_EMPTY;
            }
            else if (col == MAP_WIDTH / 2 && row == 90) {
                tileMap[row][col] = TILE_DIRT;
            }
            else if (col == MAP_WIDTH / 2 - 1 && row == 90) {
                tileMap[row][col] = TILE_DIRT;
            }
            else if (col == 0) {
                tileMap[row][col] = TILE_WALL;
            }
            else if (col == MAP_WIDTH - 1) {
                tileMap[row][col] = TILE_WALL;
            }
            else if (row == 9) {
                tileMap[row][col] = TILE_WALL;
            }
            // Wall border (first row)
            else if (row < 1)
            {
                tileMap[row][col] = TILE_WALL;
            }
            else if (row == 90) {
                tileMap[row][col] = TILE_WALL;
            }
            // Safe zone tile
            else if (row < 10)
            {
                tileMap[row][col] = TILE_STONE;
            }
            // Dirt tile area
            else if (row <= 50)
            {
                tileMap[row][col] = TILE_DIRT;
            }
            else if (row > 90)
            {
                tileMap[row][col] = TILE_BOSS;
            }
            // Lava tile area
            else {
                tileMap[row][col] = TILE_LAVA;
            }
        }
    }
}

// ===========================================================================
// WORLD HELPERS
// ===========================================================================

void GetTileWorldPosition(int row, int col, float* out_x, float* out_y)
{
    *out_x = (col * TILE_SIZE) - (MAP_WIDTH * TILE_SIZE / 2.0f) + (TILE_SIZE / 2.0f);
    *out_y = (row * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) + (TILE_SIZE / 2.0f);
}

void GetTileAtPosition(float world_x, float world_y, int* out_row, int* out_col)
{
    float offset_x = world_x + (MAP_WIDTH * TILE_SIZE / 2.0f);
    float offset_y = world_y + (MAP_HEIGHT * TILE_SIZE / 2.0f);

    *out_col = (int)(offset_x / TILE_SIZE);
    *out_row = (int)(offset_y / TILE_SIZE);
}

int IsTileValid(int row, int col)
{
    return (row >= 0 && row < MAP_HEIGHT && col >= 0 && col < MAP_WIDTH);
}

int IsTileSolid(int tile_type)
{
    return (tile_type == TILE_WALL);
}

// ===========================================================================
// COLLISION
// ===========================================================================

int CheckCollisionRectangle(float x1, float y1, float width1, float height1,
    float x2, float y2, float width2, float height2)
{
    float half_width1 = width1 / 2.0f;
    float half_height1 = height1 / 2.0f;
    float half_width2 = width2 / 2.0f;
    float half_height2 = height2 / 2.0f;

    return (x1 + half_width1 > x2 - half_width2 &&
        x1 - half_width1 < x2 + half_width2 &&
        y1 + half_height1 > y2 - half_height2 &&
        y1 - half_height1 < y2 + half_height2);
}

int CheckTileCollision(float x, float y)
{
    // Get the tile the player is in
    int player_row, player_col;
    GetTileAtPosition(x, y, &player_row, &player_col);

    // Check tiles around the player (3x3 grid)
    for (int dr = -1; dr <= 1; dr++)
    {
        for (int dc = -1; dc <= 1; dc++)
        {
            int check_row = player_row + dr;
            int check_col = player_col + dc;

            if (!IsTileValid(check_row, check_col)) continue;

            int tile_type = tileMap[check_row][check_col];

            // Check collision with solid tiles (DIRT, STONE, WALL)
            if (IsTileSolid(tile_type))
            {
                float tile_x, tile_y;
                GetTileWorldPosition(check_row, check_col, &tile_x, &tile_y);

                if (CheckCollisionRectangle(x, y, player_width, player_height,
                    tile_x, tile_y, TILE_SIZE, TILE_SIZE))
                {
                    return 1;  // Collision detected
                }
            }
        }
    }
    // Check collision with active rocks
    for (int i = 0; i < rock_count; i++)
    {
        if (!rocks[i].active) continue;

        if (CheckCollisionRectangle(x, y, player_width, player_height,
            rocks[i].x, rocks[i].y, TILE_SIZE, TILE_SIZE))
        {
            return 1;  // Collision with rock detected
        }
    }

    // Check shop solid collision
    if (CheckCollisionRectangle(x, y, player_width, player_height,
        shop_solid_x, shop_solid_y, shop_solid_width, shop_solid_height))
    {
        return 1;
    }

    return 0;  // No collision
}


// ===========================================================================
// INITIALIZATION HELPERS
// ===========================================================================

void InitializeTorches()
{
    torch_count = 0;

    // Scan the map for wall tiles and place torches on them
    for (int row = 3; row < MAP_HEIGHT; row += 5)  // Every 5 rows
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            if (torch_count >= MAX_TORCHES) break;

            // Place torches on left and right walls
            if ((col == 0 || col == MAP_WIDTH - 1) && tileMap[row][col] == TILE_WALL)
            {
                float torch_x, torch_y;
                GetTileWorldPosition(row, col, &torch_x, &torch_y);

                torches[torch_count].x = torch_x;
                torches[torch_count].y = torch_y;
                torches[torch_count].active = 1;
                torches[torch_count].glow_radius = TORCH_GLOW_RADIUS;
                torches[torch_count].flicker_time = 0.0f;
                torches[torch_count].flicker_offset = (float)(rand() % 100) / 100.0f * 6.28f;
                torch_count++;
            }
        }
    }
}

void InitializeRocks()
{
    rock_count = 0;

    for (int row = 15; row < MAP_HEIGHT; row += 5) {
        for (int col = 1; col < MAP_WIDTH; col += 2)
        {
            if (rock_count >= MAX_ROCKS) break;
            // Place rocks in stone layers
            if (tileMap[row][col] == TILE_DIRT || tileMap[row][col] == TILE_LAVA)
            {
                float random_value = (float)rand() / (float)RAND_MAX;
                if (random_value < ROCK_SPAWN_CHANCE)
                {
                    float rock_x, rock_y;
                    GetTileWorldPosition(row, col, &rock_x, &rock_y);

                    rocks[rock_count].x = rock_x;
                    rocks[rock_count].y = rock_y;
                    rocks[rock_count].active = 1;
                    rock_count++;
                }
            }
        }
    }
}

void InitializeShovel(Shovel* s, float x, float y)
{
    s->x = 0.0f;
    s->y = 0.0f;
    s->active = 1;
    s->float_time = 0.0f;
    s->base_y = y;
}

void InitializeParticles()
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        particles[i].active = 0;
    }
    particle_count = 0;
}

// ===========================================================================
// UPDATE SYSTEMS
// ===========================================================================

// ---- Physics ----
void UpdatePhysics(float dt)
{
    // Fall-through detection
    int centre_row, centre_col;
    float detect_offset_y = (last_move_dir_y >= 0.0f) ? -30.0f : 30.0f;
    GetTileAtPosition(player_x, player_y + detect_offset_y, &centre_row, &centre_col);

    if (!is_falling_through && IsTileValid(centre_row, centre_col) && tileMap[centre_row][centre_col] == TILE_EMPTY)
    {
        // Player just stepped into an empty tile - start fall animation
        is_falling_through = 1;
        fall_through_timer = 0.0f;
    }

    if (is_falling_through)
    {
        fall_through_timer += dt;

        if (fall_through_timer >= fall_through_duration)
        {
            float snap_offset = (last_move_dir_y <= 0.0f) ? TILE_SIZE : -TILE_SIZE;
            player_y = last_safe_y + snap_offset;
            is_falling_through = 0;
            fall_through_timer = 0.0f;
        }

        
        return; // Block all movement and input during fall animation
    }

    // Still on solid ground - keep recording safe position
    last_safe_y = player_y;

	// 4-directional movement
    float move_x = 0.0f;
    float move_y = 0.0f;

    if (AEInputCheckCurr(AEVK_A)) move_x -= player_speed;
    if (AEInputCheckCurr(AEVK_D)) move_x += player_speed;
    if (AEInputCheckCurr(AEVK_W)) move_y += player_speed;
    if (AEInputCheckCurr(AEVK_S)) move_y -= player_speed;

    // Track last vertical movement direction for fall-through snap
    if (move_y != 0.0f)
    {
        last_move_dir_y = move_y;
    }

    // Normalize diagonal movement
    if (move_x != 0.0f && move_y != 0.0f)
    {
        move_x *= 0.707f;
        move_y *= 0.707f;
    }

    // Horizontal movement
    float new_x = player_x + move_x;
    float new_y = player_y;

    if (!CheckTileCollision(new_x, new_y))
    {
        player_x = new_x;
    }

    // Vertical movement
    new_x = player_x;
    new_y = player_y + move_y;

    if (!CheckTileCollision(new_x, new_y))
    {
        player_y = new_y;
    }

	// World bounds checking (keep player within map edges)
    float world_width = MAP_WIDTH * TILE_SIZE;
    float world_height = MAP_HEIGHT * TILE_SIZE;

    float min_x = -world_width / 2.0f + player_width / 2.0f;
    float max_x = world_width / 2.0f - player_width / 2.0f;
    float min_y = -world_height / 2.0f + player_height / 2.0f;
    float max_y = world_height / 2.0f - player_height / 2.0f;

    if (player_x < min_x) player_x = min_x;
    if (player_x > max_x) player_x = max_x;
    if (player_y < min_y) player_y = min_y;
    if (player_y > max_y) player_y = max_y;

    // Update depth tracking
    int player_row, player_col;
    GetTileAtPosition(player_x, player_y, &player_row, &player_col);
    depth = player_row - 9; 
    if (depth < 0) depth = 0;
    if (depth > max_depth) max_depth = depth;

    // Boss gravity pull
    move_x += g_boss_pull_force_x * dt;
    move_y += g_boss_pull_force_y * dt;
}

// ---- Camera ---- 
void UpdateCamera(float dt)
{
    camera_y += (player_y - camera_y) * camera_smoothness;
    camera_x = 0.0f;

    // Vertical bounds
    float world_height = MAP_HEIGHT * TILE_SIZE;
    float min_y = -world_height / 2.0f + SCREEN_HEIGHT / 2.0f;
    float max_y = world_height / 2.0f - SCREEN_HEIGHT / 2.0f;

    if (camera_y < min_y) camera_y = min_y;
    if (camera_y > max_y) camera_y = max_y;
}

// ---- Oxygen ----
void UpdateOxygenSystem(float dt)
{
    // Check if player is in safezone
    int in_safezone = (player_x >= safezone_x_min && player_x <= safezone_x_max &&
        player_y >= safezone_y_min && player_y <= safezone_y_max);

    if (in_safezone)
    {
        // Refill oxygen in safezone
        oxygen_percentage += oxygen_refill_rate * dt;
        if (oxygen_percentage > oxygen_max)
            oxygen_percentage = oxygen_max;
    }
    else
    {
        if (!first_exit_triggered)
        {
            first_exit_triggered = 1;
            show_guide_banner = 1;
        }
        else if (shovel_repair_level == 1 && !first_shovel_banner_triggered)
        {
            first_shovel_banner_triggered = 1;
            show_first_shovel_banner = 1;
        }
        else if (shovel_repair_level == 2 && !second_shovel_banner_triggered)
        {
            second_shovel_banner_triggered = 1;
            show_second_shovel_banner = 1;
        }

        // Drain oxygen outside safezone
        oxygen_percentage -= oxygen_drain_rate * dt;
        if (oxygen_percentage < 0.0f)
            oxygen_percentage = 0.0f;
    }

    // Player death when oxygen reaches 0
    if (oxygen_percentage <= 0.0f && !lose_screen_is_active && !oxygen_killed_player)
    {
            oxygen_killed_player = 1;
            LoseScreen_Trigger(LOSE_KILLER_OXYGEN);
    }
}

// ---- Shop ---- 
void UpdateShopSystem(float dt)
{
    // Check if player is inside shop trigger box
    player_in_shop_zone = CheckCollisionRectangle(
        player_x, player_y, player_width, player_height,
        shop_trigger_x, shop_trigger_y, shop_trigger_width, shop_trigger_height
    );

    if (shop_is_active)
    {
        // Shop is open - handle shop updates
        Shop_Update();

        // Close shop with ESC or ENTER (shop.cpp handles clicks)
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_RETURN))
        {
            Shop_Free();  // This sets shop_is_active = 0
        }
    }
    else if (player_in_shop_zone)
    {
        if (AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(AEVK_RETURN))
        {
            Shop_Initialize();
		}
    }
}

// ---- Mining particles ----
void CreateParticle(float spawn_x, float spawn_y, int is_break_particle)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!particles[i].active)
        {
            float offset_range = is_break_particle ? 40.0f : 20.0f; // Random offset from spawn position
            float velocity_mult = is_break_particle ? 150.0f : 100.0f; // Random velocity (stronger for break particles)
            float scale_mult = is_break_particle ? 0.3f : 0.2f; // Random scale (larger for break particles)

            particles[i].x = spawn_x + ((float)rand() / RAND_MAX - 0.5f) * offset_range;
            particles[i].y = spawn_y + ((float)rand() / RAND_MAX - 0.5f) * offset_range;
            particles[i].vel_x = ((float)rand() / RAND_MAX - 0.5f) * velocity_mult;
            particles[i].vel_y = ((float)rand() / RAND_MAX - 0.5f) * velocity_mult + 50.0f;
            particles[i].scale = 0.1f + ((float)rand() / RAND_MAX) * scale_mult;
            particles[i].max_lifetime = 0.5f + ((float)rand() / RAND_MAX) * 1.5f;
            particles[i].lifetime = particles[i].max_lifetime;
            particles[i].active = 1;
            particle_count++;
            break;
        }
    }
}

void SpawnMiningParticles(float rock_x, float rock_y, int num_particles, int is_break)
{
    for (int i = 0; i < num_particles; i++)
    {
        CreateParticle(rock_x, rock_y, is_break);
    }
}

void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!particles[i].active) continue;

        // Update lifetime
        particles[i].lifetime -= dt;

        if (particles[i].lifetime <= 0.0f)
        {
            particles[i].active = 0;
            particle_count--;
            continue;
        }

        // Apply gravity
        particles[i].vel_y -= 150.0f * dt;

        // Update position
        particles[i].x += particles[i].vel_x * dt;
        particles[i].y += particles[i].vel_y * dt;
    }
}

// ---- Pickup rocks ----
static void SpawnPickup(float x, float y)
{
    for (int i = 0; i < MAX_PICKUPS; ++i)
    {
        if (!pickups[i].active)
        {
            pickups[i].active = 1;
            pickups[i].x = x;
            pickups[i].y = y;
            pickups[i].vx = 0.0f;
            pickups[i].vy = 0.0f;
            pickups[i].speed = 700.0f; // pixels per second (tune as needed)
            pickups[i].scale = 0.52f;  // relative to TILE_SIZE when rendering
            pickups[i].target_scale = 0.06f; // final size near player
            pickups[i].shrink_rate = 6.0f; // how fast it shrinks (higher = faster)

            pickup_count++;
            break;
        }
    }
}

static void UpdatePickups(float dt)
{
    for (int i = 0; i < MAX_PICKUPS; ++i)
    {
        if (!pickups[i].active) continue;

        // Direction to player
        float dx = player_x - pickups[i].x;
        float dy = player_y - pickups[i].y;
        float distSq = dx * dx + dy * dy;

        // If very close, simply deactivate (DO NOT increment rocks_mined here;
        // currency was already awarded when the rock was mined)
        const float collectRadius = 28.0f; // tune
        if (distSq <= collectRadius * collectRadius)
        {
            pickups[i].active = 0;
            pickup_count--;
            continue;
        }

        // Normalize direction and set velocity towards player.
        float invDist = 1.0f / sqrtf(distSq + 1e-6f);

        // Smooth homing: set velocity to direction times speed (could lerp for nicer motion)
        pickups[i].vx = dx * invDist * pickups[i].speed;
        pickups[i].vy = dy* invDist * pickups[i].speed;

        // Move
        pickups[i].x += pickups[i].vx * dt;
        pickups[i].y += pickups[i].vy * dt;

        // Smoothly shrink the visual size toward target_scale
        // simple lerp: scale += (target - scale) * rate * dt
        pickups[i].scale += (pickups[i].target_scale - pickups[i].scale) * pickups[i].shrink_rate * dt;

        // Clamp to avoid overshoot
        if (pickups[i].scale < 0.01f) pickups[i].scale = 0.01f;

    }
}

// ---- Mining ----
void UpdateMining(float dt)
{
    // Get mouse position in world space
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);
    float mouse_world_x = (float)mouse_screen_x - SCREEN_WIDTH / 2 + camera_x;
    float mouse_world_y = SCREEN_HEIGHT / 2 - (float)mouse_screen_y + camera_y;

    int target_row, target_col;
    GetTileAtPosition(mouse_world_x, mouse_world_y, &target_row, &target_col);

    // ---- Rock mining ----
    int can_mine_rock = 0;
    int target_rock = -1;
    for (int i = 0; i < rock_count; i++)
    {
        if (!rocks[i].active) continue;

        float dist = sqrtf((rocks[i].x - player_x) * (rocks[i].x - player_x) +
            (rocks[i].y - player_y) * (rocks[i].y - player_y));

        if (dist <= mining_range && CheckCollisionRectangle(mouse_world_x, mouse_world_y, 1.0f, 1.0f,
            rocks[i].x, rocks[i].y, TILE_SIZE, TILE_SIZE))
        {
            can_mine_rock = 1;
            target_rock = i;
            break;
        }
    }

    // Mining logic
    if (AEInputCheckCurr(AEVK_LBUTTON))
    {
        if (can_mine_rock)
        {
            // Play mining sound if not already playing
            if (!g_is_playing_mining)
            {
                AEAudioPlay(g_miningSound, g_sfxGroup, 0.7f, 1.0f, -1);
                g_is_playing_mining = 1;
            }

            // Mining a rock
            if (current_mining_rock != target_rock)
            {
                // Started mining a new rock
                current_mining_rock = target_rock;
                rock_mining_timer = 0.0f;
            }
            else
            {
                // Continue mining the same rock
                rock_mining_timer += dt;

                // While mining (continuous small particles)
                SpawnMiningParticles(rocks[current_mining_rock].x,
                    rocks[current_mining_rock].y,
                    PARTICLES_PER_FRAME, 0); // 0 = normal mining particles

                // Check if mining is complete
                if (rock_mining_timer >= rock_mining_duration)
                {
                    // On rock break (large explosion of particles)
                    SpawnMiningParticles(rocks[current_mining_rock].x,
                        rocks[current_mining_rock].y,
                        20, 1); // 1 = break explosion particles

                    // Rock mined successfully
                    // Store position for visual pickup
                    float px = rocks[current_mining_rock].x;
                    float py = rocks[current_mining_rock].y;

                    rocks[current_mining_rock].active = 0;

                    // Multi-rock drop chance
                    float roll = (float)rand() / (float)RAND_MAX;
                    int bonus_rocks = 0;
                    if (roll < 0.02f)       bonus_rocks = 5; // 2%  -> total 6
                    else if (roll < 0.07f)  bonus_rocks = 4; // 5%  -> total 5
                    else if (roll < 0.15f)  bonus_rocks = 3; // 8%  -> total 4
                    else if (roll < 0.30f)  bonus_rocks = 2; // 15% -> total 3
                    else if (roll < 0.55f)  bonus_rocks = 1; // 25% -> total 2

                    rocks_mined += 1 + bonus_rocks;
                    SpawnPickup(px, py); // Spawn 1 pickup for the base rock

                    // Spawn extra pickups spread around the break point
                    static const float offsets[5][2] = {
                        { 25.0f, -25.0f},
                        {-25.0f,  25.0f},
                        { 30.0f,  30.0f},
                        {-30.0f,  30.0f},
                        {  0.0f, -30.0f}
                    };
                    for (int b = 0; b < bonus_rocks; b++)
                    {
                        SpawnPickup(px + offsets[b][0], py + offsets[b][1]);
                    }

                    printf(">>> Mined %d rock(s)! (roll=%.2f)\n", 1 + bonus_rocks, roll);

                    current_mining_rock = -1;
                    rock_mining_timer = 0.0f;
                }
            }
        }
        else
        {
            // Not mining anything
            current_mining_rock = -1;
            rock_mining_timer = 0.0f;

            // Stop mining sound
            if (g_is_playing_mining)
            {
                AEAudioStopGroup(g_sfxGroup);
                g_is_playing_mining = 0;
            }
        }
    }
    else
    {
        // Mouse button released - reset mining
        current_mining_rock = -1;
        rock_mining_timer = 0.0f;
    }

	// ---- Shovel digging/filling ----
    is_shovel_digging = 0;

    if (shovel_collected && shovel_repair_level >= 1 && !can_mine_rock)
    {
        // Check if the hovered tile is within mining range
        float tile_x, tile_y;
        GetTileWorldPosition(target_row, target_col, &tile_x, &tile_y);
        float dist = sqrtf((tile_x - player_x) * (tile_x - player_x) + (tile_y - player_y) * (tile_y - player_y));

        int tile_in_range = static_cast<bool>(IsTileValid(target_row, target_col) && dist <= mining_range);
        int hovered_tile = tile_in_range ? tileMap[target_row][target_col] : -1;
        int can_shovel = (hovered_tile == TILE_DIRT || hovered_tile == TILE_EMPTY);

        if (AEInputCheckCurr(AEVK_LBUTTON) && can_shovel)
        {
            // Re-check range on the tile every frame
            float locked_tile_x, locked_tile_y;
            GetTileWorldPosition(target_row, target_col, &locked_tile_x, &locked_tile_y);
            float locked_dist = sqrtf((locked_tile_x - player_x) * (locked_tile_x - player_x) +
                (locked_tile_y - player_y) * (locked_tile_y - player_y));

            if (locked_dist > mining_range)
            {
                // Player walked out of range -> cancel
                shovel_target_row = -1;
                shovel_target_col = -1;
                shovel_dig_timer = 0.0f;
            }
            else
            {
                // Player is still in range, continue digging/filling
                is_shovel_digging = 1;

                // Reset timer when switching to a different tile
                if (shovel_target_row != target_row || shovel_target_col != target_col)
                {
                    shovel_target_row = target_row;
                    shovel_target_col = target_col;
                    shovel_dig_timer = 0.0f;
                }
                else
                {
                    shovel_dig_timer += dt;

                    if (shovel_dig_timer >= shovel_dig_duration)
                    {
                        shovel_dig_timer = 0.0f;

                        if (hovered_tile == TILE_DIRT)
                        {
                            rocks_mined++; // Count dirt as mined rocks
                            tileMap[target_row][target_col] = TILE_EMPTY;
                            printf("Shovel: dug dirt at row %d, col %d\n", target_row, target_col);
                        }
                        else if (hovered_tile == TILE_EMPTY)
                        {
                            if (rocks_mined <= 0)
                            {
                                // Not enough rocks to fill    
                                show_no_rocks_message = 1;
                                no_rocks_message_timer = 0.0f;
                                shovel_target_row = -1;
                                shovel_target_col = -1;
                                shovel_dig_timer = 0.0f;
                                return;
                            }
                            rocks_mined--; // Minus a rock when filling
                            tileMap[target_row][target_col] = TILE_DIRT;
                            printf("Shovel: filled empty at row %d, col %d\n", target_row, target_col);
                        }

                        shovel_target_row = -1;
                        shovel_target_col = -1;
                    }
                }
            }
        }
        else if (!AEInputCheckCurr(AEVK_LBUTTON))
        {
            shovel_target_row = -1;
            shovel_target_col = -1;
            shovel_dig_timer = 0.0f;
		}
    }
    else
    {
        shovel_target_row = -1;
        shovel_target_col = -1;
        shovel_dig_timer = 0.0f;
    }
}

// ---- Rock throwing ----
void UpdateRockThrowing(float dt)
{
    // Only available when shovel2 is collected and upgraded
    if (!shovel2_collected || shovel_repair_level < 2 || shop_is_active) return;

    // Get mouse position in world space
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);
    float mouse_world_x = (float)mouse_screen_x - SCREEN_WIDTH / 2.0f + camera_x;
    float mouse_world_y = SCREEN_HEIGHT / 2.0f - (float)mouse_screen_y + camera_y;

    float dx = mouse_world_x - player_x;
    float dy = mouse_world_y - player_y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.0f)
    {
        aim_dir_x = dx / len;
        aim_dir_y = dy / len;
    }

    // Hold RMB to aim (requires at least one rock)
    is_aiming = (AEInputCheckCurr(AEVK_RBUTTON) && rocks_mined > 0) ? 1 : 0;

    // Release RMB fires the rock
    if (AEInputCheckReleased(AEVK_RBUTTON) && rocks_mined > 0 && shovel2_collected && shovel_repair_level >= 2)
    {
        // Find a free slot
        for (int i = 0; i < MAX_THROWN_ROCKS; i++)
        {
            if (!thrown_rocks[i].active)
            {
                float spawn_offset = (player_width * 0.65f);
                thrown_rocks[i].x = player_x + aim_dir_x * spawn_offset;
                thrown_rocks[i].y = player_y + aim_dir_y * spawn_offset;
                thrown_rocks[i].vel_x = aim_dir_x * THROWN_ROCK_SPEED;
                thrown_rocks[i].vel_y = aim_dir_y * THROWN_ROCK_SPEED;
                thrown_rocks[i].active = 1;
                rocks_mined--;
                printf("Rock thrown! Direction: (%.2f, %.2f) | Rocks left: %d\n",
                    aim_dir_x, aim_dir_y, rocks_mined);
                break;
            }
        }
    }
}

void UpdateThrownRocks(float dt)
{
    float half_size = THROWN_ROCK_SIZE / 2.0f;

    for (int i = 0; i < MAX_THROWN_ROCKS; i++)
    {
        if (!thrown_rocks[i].active) continue;

        thrown_rocks[i].x += thrown_rocks[i].vel_x * dt;
        thrown_rocks[i].y += thrown_rocks[i].vel_y * dt;

        // Wall collision check
        int rock_row, rock_col;
        GetTileAtPosition(thrown_rocks[i].x, thrown_rocks[i].y, &rock_row, &rock_col);
        if (IsTileValid(rock_row, rock_col))
        {
            int tile = tileMap[rock_row][rock_col];
            if (tile == TILE_WALL)
            {
                thrown_rocks[i].active = 0;
                continue;
            }
        }

        // World bounds check
        float world_half_h = (MAP_HEIGHT * TILE_SIZE) / 2.0f;
        float world_half_w = (MAP_WIDTH * TILE_SIZE) / 2.0f;
        if (thrown_rocks[i].x < -world_half_w || thrown_rocks[i].x > world_half_w ||
            thrown_rocks[i].y < -world_half_h || thrown_rocks[i].y > world_half_h)
        {
            thrown_rocks[i].active = 0;
            continue;
        }

        // Mineable rock collision check
        for (int r = 0; r < rock_count; r++)
        {
            if (!rocks[r].active) continue;

            if (CheckCollisionRectangle(
                thrown_rocks[i].x, thrown_rocks[i].y, THROWN_ROCK_SIZE, THROWN_ROCK_SIZE,
                rocks[r].x, rocks[r].y, TILE_SIZE, TILE_SIZE))
            {
                thrown_rocks[i].active = 0;
                printf("Thrown rock hit mineable rock %d!\n", r);
                break;
            }
        }

		// Enemy collision check
        for (int e = 0; e < enemy_count; e++)
        {
            if (!enemies[e].active) continue;

            if (CheckCollisionRectangle(
                thrown_rocks[i].x, thrown_rocks[i].y, THROWN_ROCK_SIZE, THROWN_ROCK_SIZE,
                enemies[e].x, enemies[e].y, ENEMY_WIDTH, ENEMY_HEIGHT))
            {
                enemies[e].active = 0; // Kill enemy on hit
                thrown_rocks[i].active = 0;
                printf("Rock hit enemy %d!\n", e);
                break;
            }
        }

        if (!thrown_rocks[i].active) continue;

		// Boss collision check
        if (g_boss.active)
        {
            if (CheckCollisionRectangle(
                thrown_rocks[i].x, thrown_rocks[i].y, THROWN_ROCK_SIZE, THROWN_ROCK_SIZE,
                g_boss.x, g_boss.y, g_boss.width, g_boss.height))
            {
                g_boss.health -= THROWN_ROCK_DAMAGE;
                if (g_boss.health < 0.0f) g_boss.health = 0.0f;
                if (g_boss.health <= 0.0f) g_boss.active = 0;
                thrown_rocks[i].active = 0;
                printf("Rock hit boss! Boss HP: %.1f\n", g_boss.health);
            }
        }
    }
}

// ---- Shovel pickup ----
void UpdateShovel(Shovel* s, int* collected, int* show_banner, float* banner_time, float dt)
{
    if (!s->active) return;

    // Update floating animation
    s->float_time += dt;
    s->y = s->base_y + sinf(s->float_time * shovel_float_speed) * shovel_float_amplitude;

    // Check collision with player
    if (CheckCollisionRectangle(player_x, player_y, player_width, player_height,
        s->x, s->y, shovel_width, shovel_height))
    {
        // Player collected the shovel
        s->active = 0;
        *collected = 1;

        // Trigger banner display
        *show_banner = 1;
        *banner_time = 0.0f;

        printf("Shovel collected!\n");
    }
}

// ---- Banners & messages ----
void UpdateShovelBanner(int* show_banner, float* banner_time, float dt)
{
    if (!*show_banner) return;

    *banner_time += dt;

    if (*banner_time >= banner_duration)
    {
        *show_banner = 0;
        *banner_time = 0.0f;
    }
}

void UpdateNoRocksMessage(float dt)
{
    if (!show_no_rocks_message) return;

    no_rocks_message_timer += dt;

    if (no_rocks_message_timer >= no_rocks_message_duration)
    {
        show_no_rocks_message = 0;
        no_rocks_message_timer = 0.0f;
    }
}

// ---- Danger tint ----
void UpdateDangerTint(float dt)
{
    float oxygen_frac = oxygen_percentage / oxygen_max;
    float sanity_frac = player_hp / 100.0f;

    // Use whichever stat is lower as the danger driver
    float lowest = (oxygen_frac < sanity_frac) ? oxygen_frac : sanity_frac;

    if (lowest < 0.30f)
    {
        // Map 0.25 -> 0.0 (no tint) to 0.0 -> 1.0 (full tint)
        float target_intensity = 1.0f - (lowest / 0.25f);

        // Smoothly ramp UP the intensity
        danger_tint_intensity += (target_intensity - danger_tint_intensity) * 2.0f * dt;
    }
    else
    {
        // Smoothly fade OUT when stats recover
        danger_tint_intensity -= danger_tint_intensity * 3.0f * dt;
    }

    // Clamp
    if (danger_tint_intensity < 0.0f) danger_tint_intensity = 0.0f;
    if (danger_tint_intensity > 1.0f) danger_tint_intensity = 1.0f;

    // Fade out toward lose screen
    if (lose_screen_is_active)
    {
        danger_tint_fade_out -= 3.0f * dt;  // fades in ~0.33 seconds
        if (danger_tint_fade_out < 0.0f) danger_tint_fade_out = 0.0f;
    }
    else
    {
        danger_tint_fade_out = 1.0f;  // reset if lose screen is not active
    }
}

// ---- Audio ---- 
void UpdateEnemyZoneAudio()
{
    // Check if ANY enemy is currently chasing the player
    int any_enemy_chasing = 0;
    for (int i = 0; i < enemy_count; i++)
    {
        if (enemies[i].active && enemies[i].is_chasing)
        {
            any_enemy_chasing = 1;
            break;
        }
    }

    if (any_enemy_chasing && !g_is_playing_enemyzone)
    {
        // Enemy detected player - switch to enemy music
        AEAudioStopGroup(g_bgmGroup);
        AEAudioPlay(g_enemyZoneSound, g_enemyZoneGroup, 0.6f, 1.0f, -1);
        g_is_playing_enemyzone = 1;
    }
    else if (!any_enemy_chasing && g_is_playing_enemyzone)
    {
        // No enemy chasing - stop enemy music, resume BGM
        AEAudioStopGroup(g_enemyZoneGroup);
        g_is_playing_enemyzone = 0;
        AEAudioPlay(g_bgmSound, g_bgmGroup, 0.5f, 1.0f, -1);
    }
}

// ===========================================================================
// RENDER HELPERS
// ===========================================================================

// Draws 8 corner-bracket lines around a tile-sized box centred at (cx, cy).
static void DrawCornerBrackets(AEGfxVertexList* bracketMesh, float cx, float cy)
{
    const float hw = TILE_SIZE / 2.0f;
    const float hh = TILE_SIZE / 2.0f;
    const float bracket_length = 15.0f;
    const float bracket_thickness = 3.0f;

    AEMtx33 scale, translate, transform;

    // Helper lambda-style macro to avoid repetition
#define DRAW_BRACKET(sx, sy, tx, ty)            \
        AEMtx33Scale(&scale, sx, sy);               \
        AEMtx33Trans(&translate, tx, ty);           \
        AEMtx33Concat(&transform, &translate, &scale); \
        AEGfxSetTransform(transform.m);             \
        AEGfxMeshDraw(bracketMesh, AE_GFX_MDM_TRIANGLES)

    // Top-left
    DRAW_BRACKET(bracket_length, bracket_thickness,
        cx - hw + bracket_length / 2.0f, cy + hh - bracket_thickness / 2.0f);
    DRAW_BRACKET(bracket_thickness, bracket_length,
        cx - hw + bracket_thickness / 2.0f, cy + hh - bracket_length / 2.0f);
    // Top-right
    DRAW_BRACKET(bracket_length, bracket_thickness,
        cx + hw - bracket_length / 2.0f, cy + hh - bracket_thickness / 2.0f);
    DRAW_BRACKET(bracket_thickness, bracket_length,
        cx + hw - bracket_thickness / 2.0f, cy + hh - bracket_length / 2.0f);
    // Bottom-left
    DRAW_BRACKET(bracket_length, bracket_thickness,
        cx - hw + bracket_length / 2.0f, cy - hh + bracket_thickness / 2.0f);
    DRAW_BRACKET(bracket_thickness, bracket_length,
        cx - hw + bracket_thickness / 2.0f, cy - hh + bracket_length / 2.0f);
    // Bottom-right
    DRAW_BRACKET(bracket_length, bracket_thickness,
        cx + hw - bracket_length / 2.0f, cy - hh + bracket_thickness / 2.0f);
    DRAW_BRACKET(bracket_thickness, bracket_length,
        cx + hw - bracket_thickness / 2.0f, cy - hh + bracket_length / 2.0f);

#undef DRAW_BRACKET

}

// Draws a three-part progress bar (border / background / fill) above (bar_x, bar_y).
static void DrawProgressBar(float bar_x, float bar_y, float progress)
{
    const float bar_width = 70.0f;
    const float bar_height = 10.0f;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(NULL, 0, 0);

    AEMtx33 transform;

    // White border
    AEGfxVertexList* borderMesh = CreateRectangleMesh(bar_width + 4.0f, bar_height + 4.0f, 0xFFFFFFFF);
    AEMtx33Trans(&transform, bar_x, bar_y);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(borderMesh);

    // Dark background
    AEGfxVertexList* bgMesh = CreateRectangleMesh(bar_width, bar_height, 0xFF222222);
    AEMtx33Trans(&transform, bar_x, bar_y);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(bgMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(bgMesh);

    // Green fill
    if (progress > 0.01f)
    {
        float filled_width = bar_width * progress;
        float prog_x = bar_x - (bar_width / 2.0f) + (filled_width / 2.0f);
        AEGfxVertexList* progMesh = CreateRectangleMesh(filled_width, bar_height, 0xFF00FF00);
        AEMtx33Trans(&transform, prog_x, bar_y);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(progMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(progMesh);
    }
}

// Renders a single HUD icon texture at a fixed screen position.
static void RenderHUDIcon(AEGfxTexture* texture, AEGfxVertexList* mesh, float screen_x, float screen_y)
{
    if (!texture || !mesh) return;
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33 transform;
    AEMtx33Trans(&transform, screen_x, screen_y);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// ===========================================================================
// RENDER SYSTEMS
// ===========================================================================

void RenderBackground(AEGfxTexture* tileset,
    AEGfxVertexList* dirtMesh, AEGfxVertexList* stoneMesh,
    AEGfxVertexList* wallMesh, AEGfxVertexList* lavaMesh,
    AEGfxVertexList* bosstileMesh)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(tileset, 0, 0);

    int start_row = (int)((camera_y - SCREEN_HEIGHT / 2 - TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    int end_row = (int)((camera_y + SCREEN_HEIGHT / 2 + TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    if (start_row < 0)           start_row = 0;
    if (end_row >= MAP_HEIGHT)   end_row = MAP_HEIGHT - 1;

    for (int row = start_row; row <= end_row; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            int tile_type = tileMap[row][col];
            if (tile_type == TILE_EMPTY) continue;

            float tile_x, tile_y;
            GetTileWorldPosition(row, col, &tile_x, &tile_y);

            AEMtx33 transform;
            AEMtx33Trans(&transform, tile_x, tile_y);
            AEGfxSetTransform(transform.m);

            switch (tile_type)
            {
            case TILE_DIRT:  AEGfxMeshDraw(dirtMesh, AE_GFX_MDM_TRIANGLES); break;
            case TILE_STONE: AEGfxMeshDraw(stoneMesh, AE_GFX_MDM_TRIANGLES); break;
            case TILE_WALL:  AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES); break;
            case TILE_LAVA:  AEGfxMeshDraw(lavaMesh, AE_GFX_MDM_TRIANGLES); break;
            case TILE_BOSS:  AEGfxMeshDraw(bosstileMesh, AE_GFX_MDM_TRIANGLES); break;
            }
        }
    }
}

void RenderRocks(AEGfxTexture* rockTexture, AEGfxVertexList* rockMesh)
{
    if (!rockTexture || !rockMesh) return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(rockTexture, 0, 0);

    int start_row = (int)((camera_y - SCREEN_HEIGHT / 2 - TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    int end_row = (int)((camera_y + SCREEN_HEIGHT / 2 + TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    if (start_row < 0)         start_row = 0;
    if (end_row >= MAP_HEIGHT) end_row = MAP_HEIGHT - 1;

    for (int i = 0; i < rock_count; i++)
    {
        if (!rocks[i].active) continue;
        int rock_row = (int)((rocks[i].y + (MAP_HEIGHT * TILE_SIZE / 2.0f)) / TILE_SIZE);
        if (rock_row < start_row || rock_row > end_row) continue;

        AEMtx33 transform;
        AEMtx33Trans(&transform, rocks[i].x, rocks[i].y);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(rockMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void RenderPlayer(AEGfxVertexList* playerMesh)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float fall_scale = 1.0f;
    float fall_alpha = 1.0f;
    if (is_falling_through)
    {
        float t = fall_through_timer / fall_through_duration;
        fall_scale = 1.0f - t;
        fall_alpha = 1.0f - t;
        if (fall_scale < 0.0f) fall_scale = 0.0f;
        if (fall_alpha < 0.0f) fall_alpha = 0.0f;
    }
    if (fall_alpha <= 0.0f) return;

    AEGfxSetTransparency(fall_alpha);

    // Pick animation spritesheet
    AEGfxTexture* playerTexture = NULL;
    switch (g_playerAnimator.current_state)
    {
    case ANIM_STATE_MINING:  playerTexture = g_playerAnimator.mining_spritesheet;  break;
    case ANIM_STATE_MOVING:  playerTexture = g_playerAnimator.moving_spritesheet;  break;
    case ANIM_STATE_DIGGING: playerTexture = g_playerAnimator.digging_spritesheet; break;
    default:                 playerTexture = g_playerAnimator.idle_spritesheet;    break;
    }
    if (playerTexture) AEGfxTextureSet(playerTexture, 0, 0);

    float u1, v1, u2, v2;
    PlayerAnim_GetUVs(&g_playerAnimator, &u1, &v1, &u2, &v2);

    if (player_facing_left == 1) { float tmp = u1; u1 = u2; u2 = tmp; }

    float hw = player_width / 2.0f;
    float hh = player_height / 2.0f;

    AEGfxMeshStart();
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, u1, v2,
        hw, -hh, 0xFFFFFFFF, u2, v2,
        -hw, hh, 0xFFFFFFFF, u1, v1);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, u2, v2,
        hw, hh, 0xFFFFFFFF, u2, v1,
        -hw, hh, 0xFFFFFFFF, u1, v1);
    AEGfxVertexList* frameMesh = AEGfxMeshEnd();

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, fall_scale, fall_scale);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, player_x, player_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(frameMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(frameMesh);
}

void RenderMiningCursor(AEGfxVertexList* cursorMesh)
{
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);
    float mouse_world_x = (float)mouse_screen_x - SCREEN_WIDTH / 2 + camera_x;
    float mouse_world_y = SCREEN_HEIGHT / 2 - (float)mouse_screen_y + camera_y;

    // Find hovered rock within mining range
    int hovered_rock = -1;
    for (int i = 0; i < rock_count; i++)
    {
        if (!rocks[i].active) continue;
        float dist = sqrtf((rocks[i].x - player_x) * (rocks[i].x - player_x) +
            (rocks[i].y - player_y) * (rocks[i].y - player_y));
        if (dist <= mining_range &&
            CheckCollisionRectangle(mouse_world_x, mouse_world_y, 1.0f, 1.0f,
                rocks[i].x, rocks[i].y, TILE_SIZE, TILE_SIZE))
        {
            hovered_rock = i;
            break;
        }
    }

    // Corner brackets on hovered rock (red)
    if (hovered_rock >= 0)
    {
        AEGfxSetCamPosition(camera_x, camera_y);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEGfxVertexList* bracketMesh = CreateRectangleMesh(1.0f, 1.0f, 0xFFFF0000);
        DrawCornerBrackets(bracketMesh, rocks[hovered_rock].x, rocks[hovered_rock].y);
        AEGfxMeshFree(bracketMesh);
    }

    // Corner brackets on hovered shovelable tile (brown = dirt, yellow = empty)
    if (shovel_collected && shovel_repair_level >= 1 && hovered_rock < 0)
    {
        int target_row, target_col;
        GetTileAtPosition(mouse_world_x, mouse_world_y, &target_row, &target_col);

        if (IsTileValid(target_row, target_col))
        {
            float tile_x, tile_y;
            GetTileWorldPosition(target_row, target_col, &tile_x, &tile_y);
            float dist = sqrtf((tile_x - player_x) * (tile_x - player_x) +
                (tile_y - player_y) * (tile_y - player_y));

            int tile_type = tileMap[target_row][target_col];
            int can_shovel = (tile_type == TILE_DIRT || tile_type == TILE_EMPTY);

            if (dist <= mining_range && can_shovel)
            {
                AEGfxSetCamPosition(camera_x, camera_y);
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

                if (tile_type == TILE_DIRT)
                    AEGfxSetColorToMultiply(0.6f, 0.3f, 0.0f, 1.0f);
                else
                    AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);

                AEGfxVertexList* shovBracketMesh = CreateRectangleMesh(1.0f, 1.0f, 0xFFFFFFFF);
                DrawCornerBrackets(shovBracketMesh, tile_x, tile_y);
                AEGfxMeshFree(shovBracketMesh);
            }
        }
    }
}

void RenderRockMiningProgress()
{
    if (current_mining_rock < 0 || current_mining_rock >= rock_count
        || !rocks[current_mining_rock].active)
        return;

    float progress = rock_mining_timer / rock_mining_duration;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    float bar_x = rocks[current_mining_rock].x;
    float bar_y = rocks[current_mining_rock].y + 40.0f;
    DrawProgressBar(bar_x, bar_y, progress);
}

void RenderShovelDigProgress()
{
    if (!is_shovel_digging || shovel_target_row < 0 || shovel_target_col < 0
        || !IsTileValid(shovel_target_row, shovel_target_col))
        return;

    float tile_x, tile_y;
    GetTileWorldPosition(shovel_target_row, shovel_target_col, &tile_x, &tile_y);

    float progress = shovel_dig_timer / shovel_dig_duration;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    DrawProgressBar(tile_x, tile_y + 40.0f, progress);
}

void RenderShovel(Shovel* s, AEGfxTexture* texture, AEGfxVertexList* mesh)
{
    if (!s->active || !texture || !mesh) return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33 transform;
    AEMtx33Trans(&transform, s->x, s->y);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void RenderShovelBanner(int show_banner, float banner_time,
    AEGfxTexture* texture, AEGfxVertexList* mesh)
{
    if (!show_banner || !texture || !mesh) return;

    float alpha = 1.0f;
    if (banner_time < 0.5f)                   alpha = banner_time / 0.5f;
    else if (banner_time > banner_duration - 0.5f) alpha = (banner_duration - banner_time) / 0.5f;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(alpha);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33 transform;
    AEMtx33Trans(&transform, 0.0f, 350.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void RenderShopTrigger(AEGfxVertexList* triggerMesh)
{
    AEGfxSetCamPosition(camera_x, camera_y);
    AEMtx33 transform;
    AEMtx33Trans(&transform, shop_trigger_x, shop_trigger_y);

    if (g_shopTriggerTexture)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(g_shopTriggerTexture, 0, 0);
    }
    else
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.7f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(triggerMesh, AE_GFX_MDM_TRIANGLES);
}

void RenderSideBlackout(AEGfxVertexList* leftMesh, AEGfxVertexList* rightMesh)
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float playable_width = MAP_WIDTH * TILE_SIZE;
    float blackout_width = (SCREEN_WIDTH - playable_width) / 2.0f;

    AEMtx33 transform;
    AEMtx33Trans(&transform, -SCREEN_WIDTH / 2.0f + blackout_width / 2.0f, 0.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(leftMesh, AE_GFX_MDM_TRIANGLES);

    AEMtx33Trans(&transform, SCREEN_WIDTH / 2.0f - blackout_width / 2.0f, 0.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(rightMesh, AE_GFX_MDM_TRIANGLES);
}

void RenderAimingCursor()
{
    if (!is_aiming || !g_aimCursorMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.85f);
    AEGfxSetColorToMultiply(1.0f, 0.4f, 0.1f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float angle = atan2f(aim_dir_y, aim_dir_x);
    float cursor_x = player_x + aim_dir_x * 80.0f;
    float cursor_y = player_y + aim_dir_y * 80.0f;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 28.0f, 28.0f);
    AEMtx33Rot(&rotate, angle);
    AEMtx33Trans(&translate, cursor_x, cursor_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_aimCursorMesh, AE_GFX_MDM_TRIANGLES);
}

void RenderThrownRocks()
{
    if (!g_thrownRockMesh || !g_thrownRockTexture) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(g_thrownRockTexture, 0, 0);

    for (int i = 0; i < MAX_THROWN_ROCKS; i++)
    {
        if (!thrown_rocks[i].active) continue;
        AEMtx33 transform;
        AEMtx33Trans(&transform, thrown_rocks[i].x, thrown_rocks[i].y);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_thrownRockMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void RenderParticles()
{
    if (!g_particleMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!particles[i].active) continue;

        float alpha = particles[i].lifetime / particles[i].max_lifetime;
        float life_ratio = alpha;
        AEGfxSetTransparency(alpha);

        if (life_ratio > 0.5f) AEGfxSetColorToMultiply(0.85f, 0.79f, 0.57f, 1.0f);
        else                   AEGfxSetColorToMultiply(0.6f, 0.4f, 0.2f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, particles[i].scale * 10.0f, particles[i].scale * 10.0f);
        AEMtx33Rot(&rotate, 0.0f);
        AEMtx33Trans(&translate, particles[i].x, particles[i].y);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_particleMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void RenderPickups()
{
    if (pickup_count <= 0) return;

    if (g_rockMesh && g_rockTexture)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(g_rockTexture, 0, 0);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < MAX_PICKUPS; ++i)
        {
            if (!pickups[i].active) continue;
            AEMtx33 scale, rotate, translate, transform;
            AEMtx33Scale(&scale, pickups[i].scale, pickups[i].scale);
            AEMtx33Rot(&rotate, 0.0f);
            AEMtx33Trans(&translate, pickups[i].x, pickups[i].y);
            AEMtx33Concat(&transform, &rotate, &scale);
            AEMtx33Concat(&transform, &translate, &transform);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_rockMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
    else
    {
        // Fallback: unit mesh coloured quad
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.9f, 0.85f, 0.6f, 1.0f);

        for (int i = 0; i < MAX_PICKUPS; ++i)
        {
            if (!pickups[i].active) continue;
            AEMtx33 s, tr, m;
            AEMtx33Scale(&s, TILE_SIZE * pickups[i].scale, TILE_SIZE * pickups[i].scale);
            AEMtx33Trans(&tr, pickups[i].x, pickups[i].y);
            AEMtx33Concat(&m, &tr, &s);
            AEGfxSetTransform(m.m);
            AEGfxMeshDraw(g_cursorMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
}

// ===========================================================================
// RENDER HUD
// ===========================================================================

void RenderDepthDisplay(s8 font_id)
{
    if (font_id < 0) return;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    char depth_text[32];
    sprintf_s(depth_text, 32, "DEPTH");

    char depth_num[32];
    sprintf_s(depth_num, 32, "%dm", depth * 2);

    AEGfxPrint(font_id, depth_text, -0.99f, 0.15f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);
    AEGfxPrint(font_id, depth_num, -0.99f, 0.00f, 1.0f, 1.0f, 1.0f, 0.3f, 1.0f);
}

void RenderOxygenUI(s8 g_font_id)
{
    if (g_font_id < 0)
    {
        printf("DEBUG: Font ID is invalid: %d\n", g_font_id);
        return;
    }

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    char text[64];
    sprintf_s(text, 64, "%.0f%%", oxygen_percentage);

    AEGfxPrint(g_font_id, text, 0.16f, -0.95f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);// adjustb for location of text
}

void RenderRockCountUI(s8 font_id)
{
    if (font_id < 0) return;

    // Set camera to 0,0 for fixed UI (doesn't move with game world)
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    char text[64];
    sprintf_s(text, 64, "x%d", rocks_mined);

    // Fixed screen position (normalized coordinates -1 to 1)
    // X: 0.35 = right side (near boulder icon)
    // Y: -0.95 = bottom of screen
    AEGfxPrint(font_id, text, 0.35f, -0.95f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

void RenderSanityUI(s8 font_id)
{
    if (font_id < 0) return;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(NULL, 0, 0);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float frac = player_hp / 100.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    const float BORDER = 2.0f;
    const float BAR_W = 120.0f;
    const float BAR_H = 22.0f;
    float bx = 453.0f;   //  right 
    float by = -415.0f;  //  lower


    AEMtx33 s, tr, m;

    // White border
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEMtx33Scale(&s, BAR_W + BORDER * 2.0f, BAR_H + BORDER * 2.0f);
    AEMtx33Trans(&tr, bx, by);
	AEMtx33Concat(&m, &tr, &s);  // Ensure scaling is applied before translation
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(g_cursorMesh, AE_GFX_MDM_TRIANGLES);

    // Dark background
    AEGfxSetColorToMultiply(0.13f, 0.13f, 0.13f, 1.0f);
    AEMtx33Scale(&s, BAR_W, BAR_H);
    AEMtx33Trans(&tr, bx, by);
    AEMtx33Concat(&m, &tr, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(g_cursorMesh, AE_GFX_MDM_TRIANGLES);

    // Coloured fill
    float fill_w = BAR_W * frac;
    if (fill_w > 1.0f)
    {
        if (frac > 0.6f) AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);  // green
        else if (frac > 0.3f) AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);  // yellow
        else                  AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);  // red

        float fill_cx = (bx - BAR_W * 0.5f) + fill_w * 0.5f;

        AEMtx33Scale(&s, fill_w, BAR_H);
        AEMtx33Trans(&tr, fill_cx, by);
        AEMtx33Concat(&m, &tr, &s);
        AEGfxSetTransform(m.m);
        AEGfxMeshDraw(g_cursorMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void RenderShopPrompt(s8 font_id)
{
    if (font_id < 0 || !player_in_shop_zone || shop_is_active) return;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Display prompt at top of screen
    AEGfxPrint(font_id, (char*)"Press ENTER or LEFT CLICK to open shop", -0.4f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void RenderNoRocksMessage(s8 font_id)
{
    if (!show_no_rocks_message || font_id < 0) return;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxPrint(font_id, (char*)"NOT ENOUGH ROCKS!!!", -0.25f, 0.5f, 1.0f, 1.0f, 0.2f, 0.2f, 1.0f);
}

void RenderDangerTint()
{
    if (danger_tint_intensity <= 0.0f || danger_tint_fade_out <= 0.0f) return;

    float pulse = 0.5f + 0.5f * sinf(game_time * 4.0f);
    float base_alpha = danger_tint_intensity * 0.40f;        // max 40% opacity
    float tint_alpha = (base_alpha + 0.05f * pulse) * danger_tint_fade_out;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(tint_alpha);
    AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(NULL, 0, 0);

    AEMtx33 t;
    AEMtx33Scale(&t, SCREEN_WIDTH, SCREEN_HEIGHT);
    AEGfxSetTransform(t.m);
    AEGfxMeshDraw(g_cursorMesh, AE_GFX_MDM_TRIANGLES);
}

// ===========================================================================
// RENDER GUIDE BANNERS
// ===========================================================================

void RenderGuideBanner(s8 font_id)
{
    if (!show_guide_banner || !g_guideBannerTexture || !g_guideBannerMesh) return;

    // Banner position (screen space, centred)
    const float BANNER_X = 0.0f;
    const float BANNER_Y = 40.0f;

    // X close button (top-right corner of banner)
    const float BTN_SIZE = 30.0f;
    const float BTN_X = BANNER_X + guide_banner_width / 2.0f - BTN_SIZE / 2.0f - 8.0f;
    const float BTN_Y = BANNER_Y + guide_banner_height / 2.0f - BTN_SIZE / 2.0f - 8.0f;

    AEGfxSetCamPosition(0.0f, 0.0f);

    // Draw banner image
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(g_guideBannerTexture, 0, 0);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, BANNER_X, BANNER_Y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_guideBannerMesh, AE_GFX_MDM_TRIANGLES);

    // Draw text to the left of the banner icon
    if (font_id >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // HOW TO SURVIVE BANNER
        AEGfxPrint(font_id, (char*)"How to survive", -0.25f, 0.40f, 1.3f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"1. Oxygen slowly depletes in mine!", -0.28f, 0.30f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"2. Come back to safe zone to refill!", -0.28f, 0.24f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"3. No oxygen = death!", -0.28f, 0.18f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);

        // HOW TO MINE BANNER
        AEGfxPrint(font_id, (char*)"How to mine", -0.25f, -0.05f, 1.3f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"1. Aim cursor at stones!", -0.28f, -0.15f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"2. Hold left click to break!", -0.28f, -0.21f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"3. Collect them stones!", -0.28f, -0.27f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Draw invisible X close button
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshStart();
    float hs = BTN_SIZE / 2.0f;
    AEGfxTriAdd(-hs, -hs, 0x00000000, 0, 0, hs, -hs, 0x00000000, 1, 0, -hs, hs, 0x00000000, 0, 1);
    AEGfxTriAdd(hs, -hs, 0x00000000, 1, 0, hs, hs, 0x00000000, 1, 1, -hs, hs, 0x00000000, 0, 1);
    AEGfxVertexList* btnMesh = AEGfxMeshEnd();

    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEMtx33Trans(&translate, BTN_X, BTN_Y);
    AEGfxSetTransform(translate.m);
    AEGfxMeshDraw(btnMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(btnMesh);

    // Draw "X" label
    if (font_id >= 0)
    {
        AEGfxSetTransparency(1.0f);
        AEGfxPrint(font_id, (char*)"X", 0.27f, 0.44f, 1.5f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Check click on X button to close
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        s32 mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - SCREEN_WIDTH / 2.0f;
        float wy = SCREEN_HEIGHT / 2.0f - (float)my;

        if (wx >= BTN_X - hs && wx <= BTN_X + hs &&
            wy >= BTN_Y - hs && wy <= BTN_Y + hs)
        {
            show_guide_banner = 0;
        }
    }
}

void RenderShovelGuideBanner(s8 font_id)
{
    if (!show_first_shovel_banner || !g_shovelGuideBannerTexture|| !g_guideBannerMesh) return;

    // Banner position (screen space, centred)
    const float BANNER_X = 0.0f;
    const float BANNER_Y = 40.0f;

    // X close button (top-right corner of banner)
    const float BTN_SIZE = 30.0f;
    const float BTN_X = BANNER_X + guide_banner_width / 2.0f - BTN_SIZE / 2.0f - 8.0f;
    const float BTN_Y = BANNER_Y + guide_banner_height / 2.0f - BTN_SIZE / 2.0f - 8.0f;

    AEGfxSetCamPosition(0.0f, 0.0f);

    // Draw banner image
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(g_shovelGuideBannerTexture, 0, 0);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, BANNER_X, BANNER_Y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_shovelGuideBannerMesh, AE_GFX_MDM_TRIANGLES);

    // Draw text to the left of the banner icon
    if (font_id >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

		// HOW TO DIG HOLES BANNER
        AEGfxPrint(font_id, (char*)"How to dig", -0.25f, 0.40f, 1.3f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"1. Aim at the ground!", -0.28f, 0.30f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"2. Hold left click to dig!", -0.28f, 0.24f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"3. Now it becomes an empty hole!", -0.28f, 0.18f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);

        // HOW TO FILL HOLES BANNER
        AEGfxPrint(font_id, (char*)"How to fill holes", -0.25f, -0.05f, 1.05f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"1. Aim cursor at holes!", -0.28f, -0.15f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"2. Hold left click to fill!", -0.28f, -0.21f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"3. Hole filled! but lose a rock!", -0.28f, -0.27f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Draw invisible X close button
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshStart();
    float hs = BTN_SIZE / 2.0f;
    AEGfxTriAdd(-hs, -hs, 0x00000000, 0, 0, hs, -hs, 0x00000000, 1, 0, -hs, hs, 0x00000000, 0, 1);
    AEGfxTriAdd(hs, -hs, 0x00000000, 1, 0, hs, hs, 0x00000000, 1, 1, -hs, hs, 0x00000000, 0, 1);
    AEGfxVertexList* btnMesh = AEGfxMeshEnd();

    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEMtx33Trans(&translate, BTN_X, BTN_Y);
    AEGfxSetTransform(translate.m);
    AEGfxMeshDraw(btnMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(btnMesh);

    // Draw "X" label
    if (font_id >= 0)
    {
        AEGfxSetTransparency(1.0f);
        AEGfxPrint(font_id, (char*)"X", 0.27f, 0.44f, 1.5f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Check click on X button to close
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        s32 mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - SCREEN_WIDTH / 2.0f;
        float wy = SCREEN_HEIGHT / 2.0f - (float)my;

        if (wx >= BTN_X - hs && wx <= BTN_X + hs &&
            wy >= BTN_Y - hs && wy <= BTN_Y + hs)
        {
            show_first_shovel_banner = 0;
        }
    }
}

void RenderShovelGuideBanner2(s8 font_id)
{
    if (!show_second_shovel_banner || !g_shovelGuideBannerTexture2|| !g_shovelGuideBannerMesh2) return;

    // Banner position (screen space, centred)
    const float BANNER_X = 0.0f;
    const float BANNER_Y = 40.0f;

    // X close button (top-right corner of banner)
    const float BTN_SIZE = 30.0f;
    const float BTN_X = BANNER_X + second_shovel_banner_width / 2.0f - BTN_SIZE / 2.0f - 8.0f;
    const float BTN_Y = BANNER_Y + second_shovel_banner_height / 2.0f - BTN_SIZE / 2.0f - 8.0f;

    AEGfxSetCamPosition(0.0f, 0.0f);

    // Draw banner image
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(g_shovelGuideBannerTexture2, 0, 0);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, BANNER_X, BANNER_Y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_shovelGuideBannerMesh2, AE_GFX_MDM_TRIANGLES);

    // Draw text to the left of the banner icon
    if (font_id >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // HOW TO SHOOT BANNER
        AEGfxPrint(font_id, (char*)"How to shoot", -0.35f, 0.25f, 1.3f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"1. Hold right click to aim!", -0.38f, 0.15f, 0.7f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"2. Release to shoot!", -0.38f, 0.09f, 0.7f, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(font_id, (char*)"3. Consumes rocks when shooting!", -0.38f, 0.03f, 0.7f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Draw invisible X close button
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshStart();
    float hs = BTN_SIZE / 2.0f;
    AEGfxTriAdd(-hs, -hs, 0x00000000, 0, 0, hs, -hs, 0x00000000, 1, 0, -hs, hs, 0x00000000, 0, 1);
    AEGfxTriAdd(hs, -hs, 0x00000000, 1, 0, hs, hs, 0x00000000, 1, 1, -hs, hs, 0x00000000, 0, 1);
    AEGfxVertexList* btnMesh = AEGfxMeshEnd();

    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEMtx33Trans(&translate, BTN_X, BTN_Y);
    AEGfxSetTransform(translate.m);
    AEGfxMeshDraw(btnMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxMeshFree(btnMesh);

    // Draw "X" label
    if (font_id >= 0)
    {
        AEGfxSetTransparency(1.0f);
        AEGfxPrint(font_id, (char*)"X", 0.40f, 0.32f, 1.5f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Check click on X button to close
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        s32 mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - SCREEN_WIDTH / 2.0f;
        float wy = SCREEN_HEIGHT / 2.0f - (float)my;

        if (wx >= BTN_X - hs && wx <= BTN_X + hs &&
            wy >= BTN_Y - hs && wy <= BTN_Y + hs)
        {
            show_second_shovel_banner = 0;
        }
    }
}

void RenderLighting(AEGfxTexture* glowTexture, AEGfxVertexList* glowMesh)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxTextureSet(glowTexture, 0, 0);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < torch_count; i++)
    {
        if (!torches[i].active) continue;
        float glow_scale = torches[i].glow_radius / 128.0f;
        AEGfxSetTransparency(0.5f);

        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, glow_scale, glow_scale);
        AEMtx33Rot(&rotate, 0.0f);
        AEMtx33Trans(&translate, torches[i].x, torches[i].y);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(glowMesh, AE_GFX_MDM_TRIANGLES);
    }
}

// ===========================================================================
// GAME STATE HELPERS
// ===========================================================================

// Called when returning to main menu OR starting a fresh run.
void ResetGameState()
{
    // --- Position & camera ---
    player_x = 0.0f;
    player_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;
    last_safe_y = player_y;
    camera_x = 0.0f;
    camera_y = 0.0f;

    // --- Stats ---
    depth = 0;
    max_depth = 0;
    rocks_mined = 1000;

    // --- Oxygen ---
    oxygen_percentage = 100.0f;
    oxygen_max = 100.0f;
    oxygen_killed_player = 0;

    // --- Player ---
    player_hp = PLAYER_MAX_HP;

    // --- Upgrades ---
    oxygen_upgrade_level = 0;
    pick_upgrade_level = 0;
    sanity_upgrade_level = 0;
    torch_upgrade_level = 0;
    shovel_repair_level = 0;

    // Restore upgrade-modified defaults
    rock_mining_duration = 4.0f;
    shovel_dig_duration = 4.0f;
    player_light_radius = TORCH_RADIUS_DEFAULT;

    // --- Shovels ---
    shovel_collected = 0;
    shovel2_collected = 0;
    InitializeShovel(&shovel, 0.0f, -2000.0f);
    InitializeShovel(&shovel2, 100.0f, -1900.0f);

    // --- Mining ---
    current_mining_rock = -1;
    rock_mining_timer = 0.0f;
    is_falling_through = 0;
    fall_through_timer = 0.0f;

    // --- Particles & pickups ---
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = 0;
    particle_count = 0;
    for (int i = 0; i < MAX_PICKUPS; i++) pickups[i].active = 0;
    pickup_count = 0;

    // --- Shop ---
    if (shop_is_active) Shop_Free();
    shop_is_active = 0;

    // --- Guide banners ---
    show_guide_banner = 0;
    first_exit_triggered = 0;
    show_first_shovel_banner = 0;
    first_shovel_banner_triggered = 0;
    show_second_shovel_banner = 0;
    second_shovel_banner_triggered = 0;

    // --- Boss ---
    boss_defeated = 0;
}

// Called on player death: full reset of everything
void RespawnPlayer()
{

    ResetGameState();

    // Clear oxygen-death flag so UpdateOxygenSystem doesn't re-trigger
    oxygen_killed_player = 0;

    // Cancel any in-progress mining/digging input state
    is_falling_through = 0;
    fall_through_timer = 0.0f;
    last_safe_y = player_y;

    // Clear transient visual effects
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = 0;
    particle_count = 0;
    for (int i = 0; i < MAX_PICKUPS; i++) pickups[i].active = 0;
    pickup_count = 0;

    // Close shop if open
    if (shop_is_active) Shop_Free();
}

// ===========================================================================
// GAME_INIT
// ===========================================================================

void Game_Init(void)
{
    // --- Font ---
    g_font_id = AEGfxCreateFont("Assets/fonts/liberation-mono.ttf", 24);
    if (g_font_id < 0) g_font_id = 0;

    // --- World ---
    if (!LoadWorldFromFile("Assets/map.txt"))
        GenerateWorld();

    // --- Boss textures ---
    g_bossTexture = AEGfxTextureLoad("Assets/boss.png");
    g_bRockTexture = AEGfxTextureLoad("Assets/bossrock.png");
    g_bfireballTexture = AEGfxTextureLoad("Assets/fireball.png");

    // --- Door ---
    DoorInit(safezone_y_max);

    // --- Textures ---
    g_tilesetTexture = AEGfxTextureLoad("Assets/tileset.png");
    g_oxygeniconTexture = AEGfxTextureLoad("Assets/o2icon.png");
    g_sanityiconTexture = AEGfxTextureLoad("Assets/sanityicon.png");
    g_bouldericonTexture = AEGfxTextureLoad("Assets/bouldericon.png");
    g_mapiconTexture = AEGfxTextureLoad("Assets/mapicon.png");
    g_playerTexture = AEGfxTextureLoad("Assets/player-animation-idle.png");
    g_glowTexture = AEGfxTextureLoad("Assets/glow_texture.png");
    g_rockTexture = AEGfxTextureLoad("Assets/mineable_rock.png");
    g_shovelTexture = AEGfxTextureLoad("Assets/shovel.png");
    g_shovel2Texture = AEGfxTextureLoad("Assets/shovel2.png");
    g_shovelBannerTexture = AEGfxTextureLoad("Assets/shovel-banner.png");
    g_shovel2BannerTexture = AEGfxTextureLoad("Assets/shovel2-banner.png");
    g_thrownRockTexture = AEGfxTextureLoad("Assets/thrown-rock.png");
    g_shopTriggerTexture = AEGfxTextureLoad("Assets/shop.png");
    g_guideBannerTexture = AEGfxTextureLoad("Assets/guide_banner1.png");
    g_shovelGuideBannerTexture = AEGfxTextureLoad("Assets/guide_banner2.png");
    g_shovelGuideBannerTexture2 = AEGfxTextureLoad("Assets/guide_banner3.png");

    // --- Tile meshes ---
    g_texture_loaded = 0;
    if (g_tilesetTexture)
    {
        g_dirtMesh = CreateSpritesheetTileMesh(DIRT_SPRITE_POSITION, TILE_SIZE);
        g_stoneMesh = CreateSpritesheetTileMesh(STONE_SPRITE_POSITION, TILE_SIZE);
        g_wallMesh = CreateSpritesheetTileMesh(WALL_SPRITE_POSITION, TILE_SIZE);
        g_lavaMesh = CreateSpritesheetTileMesh(LAVA_SPRITE_POSITION, TILE_SIZE);
        g_bosstileMesh = CreateSpritesheetTileMesh(BOSS_SPRITE_POSITION, TILE_SIZE);
        g_texture_loaded = 1;
    }
    else
    {
        g_dirtMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0xFF8B4513);
        g_stoneMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0xFF808080);
    }

    // --- Player mesh ---
    g_playerMesh = g_playerTexture ? CreateTextureMesh(player_width, player_height) : CreateRectangleMesh(player_width, player_height, 0xFF0000FF);

    // --- Other meshes ---
    g_cursorMesh = CreateRectangleMesh(1.0f, 1.0f, 0xFFFFFFFF);
    g_glowMesh = CreateGlowMesh(256.0f);
    g_rockMesh = CreateGlowMesh(TILE_SIZE);
    g_shovelMesh = CreateTextureMesh(shovel_width, shovel_height);
    g_shovel2Mesh = CreateTextureMesh(shovel2_width, shovel2_height);
    g_shovelBannerMesh = CreateTextureMesh(banner_width, banner_height);
    g_shovel2BannerMesh = CreateTextureMesh(banner_width, banner_height);
    g_oxygeniconMesh = CreateTextureMesh(oxygeniconwidth, oxygeniconheight);
    g_sanityiconMesh = CreateTextureMesh(sanityiconwidth, sanityiconheight);
    g_bouldericonMesh = CreateTextureMesh(bouldericonwidth, bouldericonheight);
    g_mapiconMesh = CreateTextureMesh(mapiconwidth, mapiconheight);
    g_guideBannerMesh = CreateTextureMesh(guide_banner_width, guide_banner_height);
    g_shovelGuideBannerMesh = CreateTextureMesh(guide_banner_width, guide_banner_height);
    g_shovelGuideBannerMesh2 = CreateTextureMesh(second_shovel_banner_width, second_shovel_banner_height);
    g_thrownRockMesh = CreateTextureMesh(THROWN_ROCK_SIZE, THROWN_ROCK_SIZE);
    g_particleMesh = CreateRectangleMesh(1.0f, 1.0f, 0xFFFFFFFF);

    // Aim cursor: unit right-pointing triangle
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFF6620, 0.0f, 0.0f,
            0.5f, 0.0f, 0xFFFF6620, 1.0f, 0.5f,
            -0.5f, 0.5f, 0xFFFF6620, 0.0f, 1.0f);
        g_aimCursorMesh = AEGfxMeshEnd();
    }

    // Side blackout bars
    float playable_width = MAP_WIDTH * TILE_SIZE;
    float blackout_width = (SCREEN_WIDTH - playable_width) / 2.0f;
    g_leftBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);
    g_rightBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);

    // Shop trigger mesh
    g_shopTriggerMesh = CreateTextureMesh(shop_trigger_width, shop_trigger_height);
    printf(g_shopTriggerTexture ? "DEBUG: Shop image texture loaded successfully!\n" : "DEBUG: Failed to load shop image texture!\n");

    // --- Reset flags ---
    boss_defeated = 0;
    first_exit_triggered = 0;
    show_guide_banner = 0;
    show_first_shovel_banner = 0;
    first_shovel_banner_triggered = 0;

    // Zero out thrown rocks
    for (int i = 0; i < MAX_THROWN_ROCKS; i++)
        thrown_rocks[i].active = 0;

    // --- World objects ---
    InitializeTorches();
    InitializeRocks();
    InitializeParticles();
    InitializeShovel(&shovel, 0.0f, -2000.0f);
    InitializeShovel(&shovel2, 100.0f, -1900.0f);

    // --- Subsystems ---
    Player_Attack_Init();
    Shop_Load();
    Enemy_Init();
    LightSystem_Init();
    PlayerAnim_Init(&g_playerAnimator,
        "Assets/player-idle-animation-spritesheet.png",
        "Assets/player-animation-spritesheet.png",
        "Assets/mining-animation-spritesheet.png",
        "Assets/player-shovel-spritesheet.png",
        0.1f, 0.5f, 1);
    PauseMenu_Load(g_font_id);
    LoseScreen_Load(g_font_id);
    InitBoss();

    // --- Player start position ---
    player_x = 0.0f;
    player_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;
    last_safe_y = player_y;

    // --- Audio ---
    g_miningSound = AEAudioLoadSound("Assets/mining_sound.mp3");
    g_sfxGroup = AEAudioCreateGroup();

    g_bgmSound = AEAudioLoadMusic("Assets/background_sound.mp3");
    g_bgmGroup = AEAudioCreateGroup();
    AEAudioPlay(g_bgmSound, g_bgmGroup, 0.5f, 1.0f, -1);

    g_enemyZoneSound = AEAudioLoadMusic("Assets/Enemy_zone_sound.mp3");
    g_enemyZoneGroup = AEAudioCreateGroup();

    // --- Initial game state ---
    ResetGameState();
}

// ===========================================================================
// GAME_UPDATE
// ===========================================================================

void Game_Update(void)
{
    float dt = AEFrameRateControllerGetFrameTime();

    // Victory screen: ESC exits to main menu (full reset)
    if (boss_defeated)
    {
        if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            ResetGameState();
            Game_Kill();
            Game_Init();
        }
        return;
    }

    // Lose screen blocks all gameplay
    if (lose_screen_is_active)
    {
        int result = LoseScreen_Update();
        if (result == LOSE_RESULT_CONTINUE)
        {
            RespawnPlayer();
        }
        return;
    }

    // Enemy-kill death detection
    if (player_is_dead)
    {
        LoseScreen_Trigger((LoseKillerType)(int)last_killer_type);
        player_is_dead = 0;
        return;
    }

    // --- Mouse world position ---
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);
    float mouse_world_x = (float)mouse_screen_x - SCREEN_WIDTH / 2.0f + camera_x;
    float mouse_world_y = SCREEN_HEIGHT / 2.0f - (float)mouse_screen_y + camera_y;

    // --- Player facing direction ---
    if (mouse_world_x > player_x) player_facing_left = 1;
    else if (mouse_world_x < player_x) player_facing_left = -1;

    // Guide banners freeze gameplay input
    if (show_guide_banner || show_first_shovel_banner || show_second_shovel_banner)
    {
        UpdateCamera(dt);
        return;
    }

    // --- Animation state ---
    int is_moving = AEInputCheckCurr(AEVK_W) || AEInputCheckCurr(AEVK_A)
        || AEInputCheckCurr(AEVK_S) || AEInputCheckCurr(AEVK_D);
    int is_mining = (current_mining_rock >= 0);
    PlayerAnim_Update(&g_playerAnimator, dt, is_moving, is_mining, is_shovel_digging);

    // --- Door ---
    float previous_player_y = player_y;
    float move_y = player_y - previous_player_y;
    DoorUpdate(dt, player_y, move_y, safezone_y_max);

    // --- Core updates ---
    UpdatePhysics(dt);
    UpdateMining(dt);
    UpdateCamera(dt);
    UpdateOxygenSystem(dt);
    UpdateShopSystem(dt);

    // --- Enemies & boss ---
    Enemy_Update(dt, player_x, player_y, &player_x, &player_y);
    UpdateEnemyZoneAudio();
    UpdateBoss(dt);

    // --- Projectiles ---
    UpdateRockThrowing(dt);
    UpdateThrownRocks(dt);

    // --- World objects ---
    UpdateParticles(dt);
    UpdatePickups(dt);
    UpdateShovel(&shovel, &shovel_collected, &show_shovel_banner, &banner_display_time, dt);
    UpdateShovel(&shovel2, &shovel2_collected, &show_shovel2_banner, &banner2_display_time, dt);

    // --- UI timers ---
    UpdateShovelBanner(&show_shovel_banner, &banner_display_time, dt);
    UpdateShovelBanner(&show_shovel2_banner, &banner2_display_time, dt);
    UpdateNoRocksMessage(dt);
    UpdateDangerTint(dt);

    // --- Player attack ---
    Player_Attack_Update(dt, player_x, player_y, mouse_world_x, mouse_world_y);
}

// ===========================================================================
// GAME_DRAW
// ===========================================================================

void Game_Draw(void)
{
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetCamPosition(camera_x, camera_y);

    // --- World ---
    if (g_texture_loaded)
        RenderBackground(g_tilesetTexture, g_dirtMesh, g_stoneMesh, g_wallMesh, g_lavaMesh, g_bosstileMesh);

    if (g_rockTexture) RenderRocks(g_rockTexture, g_rockMesh);

    RenderParticles();
    RenderPickups();

    // --- World objects ---
    if (g_shovelTexture)  RenderShovel(&shovel, g_shovelTexture, g_shovelMesh);
    if (g_shovel2Texture) RenderShovel(&shovel2, g_shovel2Texture, g_shovel2Mesh);
    RenderShopTrigger(g_shopTriggerMesh);
    if (g_tilesetTexture) DoorDraw(g_tilesetTexture);

    // --- Cursors & projectiles ---
    RenderMiningCursor(g_cursorMesh);
    RenderThrownRocks();
    RenderAimingCursor();

    // ------------------------------------------------------------------
    // 2. Darkness overlay drawn over the world
    // ------------------------------------------------------------------
    LightSystem_DrawDarkness(camera_x, camera_y, player_x, player_y);

    // ------------------------------------------------------------------
    // 3. Glow burns through the darkness (BEFORE player and enemies
    //    so they appear on top of the glow circle)
    // ------------------------------------------------------------------
    LightSystem_DrawGlow(camera_x, camera_y,
        player_x, player_y,
        player_light_radius,
        NULL, NULL, NULL, 0);

    // --- Characters ---
    RenderPlayer(g_playerMesh);
    Enemy_Draw(camera_x, camera_y);
    RenderBoss();

    // --- Progress bars ---
    RenderRockMiningProgress();
    RenderShovelDigProgress();

    // --- Screen-space overlays ---
    RenderSideBlackout(g_leftBlackoutMesh, g_rightBlackoutMesh);
    RenderDepthDisplay(g_font_id);

    if (!shop_is_active)
    {
        // HUD icons
        if (g_mapiconMesh)     RenderHUDIcon(g_mapiconTexture, g_mapiconMesh, 300.0f, -350.0f);
        if (g_oxygeniconMesh)  RenderHUDIcon(g_oxygeniconTexture, g_oxygeniconMesh, 150.0f, -350.0f);
        if (g_sanityiconMesh)  RenderHUDIcon(g_sanityiconTexture, g_sanityiconMesh, 450.0f, -350.0f);
        if (g_bouldericonMesh) RenderHUDIcon(g_bouldericonTexture, g_bouldericonMesh, 300.0f, -350.0f);

        // HUD text
        RenderOxygenUI(g_font_id);
        RenderRockCountUI(g_font_id);
        RenderSanityUI(g_font_id);

        // Prompts & messages
        RenderShopPrompt(g_font_id);
        RenderNoRocksMessage(g_font_id);

        // Guide banners
        RenderGuideBanner(g_font_id);
        RenderShovelGuideBanner(g_font_id);
        RenderShovelGuideBanner2(g_font_id);

        // Collection banners
        if (g_shovelBannerMesh)
            RenderShovelBanner(show_shovel_banner, banner_display_time, g_shovelBannerTexture, g_shovelBannerMesh);
        if (g_shovel2BannerMesh)
            RenderShovelBanner(show_shovel2_banner, banner2_display_time, g_shovel2BannerTexture, g_shovel2BannerMesh);
    }
    else
    {
        Shop_Draw();
    }

    // --- Lose / victory screens (always on top) ---
    if (lose_screen_is_active) LoseScreen_Draw();

    // Danger tint: fade out instantly when lose screen appears
    danger_tint_fade_out = lose_screen_is_active ? 0.0f : 1.0f;
    RenderDangerTint();

    RenderVictoryScreen();
}

// ===========================================================================
// GAME_KILL
// ===========================================================================

void Game_Kill(void)
{
    // --- Free meshes ---
    if (g_dirtMesh) { AEGfxMeshFree(g_dirtMesh);               g_dirtMesh = NULL; }
    if (g_stoneMesh) { AEGfxMeshFree(g_stoneMesh);              g_stoneMesh = NULL; }
    if (g_wallMesh) { AEGfxMeshFree(g_wallMesh);               g_wallMesh = NULL; }
    if (g_lavaMesh) { AEGfxMeshFree(g_lavaMesh);               g_lavaMesh = NULL; }
    if (g_bosstileMesh) { AEGfxMeshFree(g_bosstileMesh);           g_bosstileMesh = NULL; }
    if (g_glowMesh) { AEGfxMeshFree(g_glowMesh);               g_glowMesh = NULL; }
    if (g_rockMesh) { AEGfxMeshFree(g_rockMesh);               g_rockMesh = NULL; }
    if (g_shovelMesh) { AEGfxMeshFree(g_shovelMesh);             g_shovelMesh = NULL; }
    if (g_shovel2Mesh) { AEGfxMeshFree(g_shovel2Mesh);            g_shovel2Mesh = NULL; }
    if (g_shovelBannerMesh) { AEGfxMeshFree(g_shovelBannerMesh);       g_shovelBannerMesh = NULL; }
    if (g_shovel2BannerMesh) { AEGfxMeshFree(g_shovel2BannerMesh);      g_shovel2BannerMesh = NULL; }
    if (g_playerMesh) { AEGfxMeshFree(g_playerMesh);             g_playerMesh = NULL; }
    if (g_cursorMesh) { AEGfxMeshFree(g_cursorMesh);             g_cursorMesh = NULL; }
    if (g_oxygeniconMesh) { AEGfxMeshFree(g_oxygeniconMesh);         g_oxygeniconMesh = NULL; }
    if (g_sanityiconMesh) { AEGfxMeshFree(g_sanityiconMesh);         g_sanityiconMesh = NULL; }
    if (g_bouldericonMesh) { AEGfxMeshFree(g_bouldericonMesh);        g_bouldericonMesh = NULL; }
    if (g_mapiconMesh) { AEGfxMeshFree(g_mapiconMesh);            g_mapiconMesh = NULL; }
    if (g_leftBlackoutMesh) { AEGfxMeshFree(g_leftBlackoutMesh);       g_leftBlackoutMesh = NULL; }
    if (g_rightBlackoutMesh) { AEGfxMeshFree(g_rightBlackoutMesh);      g_rightBlackoutMesh = NULL; }
    if (g_shopTriggerMesh) { AEGfxMeshFree(g_shopTriggerMesh);        g_shopTriggerMesh = NULL; }
    if (g_thrownRockMesh) { AEGfxMeshFree(g_thrownRockMesh);         g_thrownRockMesh = NULL; }
    if (g_aimCursorMesh) { AEGfxMeshFree(g_aimCursorMesh);          g_aimCursorMesh = NULL; }
    if (g_guideBannerMesh) { AEGfxMeshFree(g_guideBannerMesh);        g_guideBannerMesh = NULL; }
    if (g_shovelGuideBannerMesh) { AEGfxMeshFree(g_shovelGuideBannerMesh);  g_shovelGuideBannerMesh = NULL; }
    if (g_shovelGuideBannerMesh2) { AEGfxMeshFree(g_shovelGuideBannerMesh2); g_shovelGuideBannerMesh2 = NULL; }
    if (g_particleMesh) { AEGfxMeshFree(g_particleMesh);           g_particleMesh = NULL; }

    // --- Unload textures ---
    if (g_tilesetTexture) { AEGfxTextureUnload(g_tilesetTexture);            g_tilesetTexture = NULL; }
    if (g_oxygeniconTexture) { AEGfxTextureUnload(g_oxygeniconTexture);         g_oxygeniconTexture = NULL; }
    if (g_sanityiconTexture) { AEGfxTextureUnload(g_sanityiconTexture);         g_sanityiconTexture = NULL; }
    if (g_bouldericonTexture) { AEGfxTextureUnload(g_bouldericonTexture);        g_bouldericonTexture = NULL; }
    if (g_mapiconTexture) { AEGfxTextureUnload(g_mapiconTexture);            g_mapiconTexture = NULL; }
    if (g_playerTexture) { AEGfxTextureUnload(g_playerTexture);             g_playerTexture = NULL; }
    if (g_glowTexture) { AEGfxTextureUnload(g_glowTexture);               g_glowTexture = NULL; }
    if (g_rockTexture) { AEGfxTextureUnload(g_rockTexture);               g_rockTexture = NULL; }
    if (g_shovelTexture) { AEGfxTextureUnload(g_shovelTexture);             g_shovelTexture = NULL; }
    if (g_shovel2Texture) { AEGfxTextureUnload(g_shovel2Texture);            g_shovel2Texture = NULL; }
    if (g_shovelBannerTexture) { AEGfxTextureUnload(g_shovelBannerTexture);       g_shovelBannerTexture = NULL; }
    if (g_shovel2BannerTexture) { AEGfxTextureUnload(g_shovel2BannerTexture);      g_shovel2BannerTexture = NULL; }
    if (g_bossTexture) { AEGfxTextureUnload(g_bossTexture);               g_bossTexture = NULL; }
    if (g_bRockTexture) { AEGfxTextureUnload(g_bRockTexture);              g_bRockTexture = NULL; }
    if (g_bfireballTexture) { AEGfxTextureUnload(g_bfireballTexture);          g_bfireballTexture = NULL; }
    if (g_thrownRockTexture) { AEGfxTextureUnload(g_thrownRockTexture);         g_thrownRockTexture = NULL; }
    if (g_shopTriggerTexture) { AEGfxTextureUnload(g_shopTriggerTexture);        g_shopTriggerTexture = NULL; }
    if (g_guideBannerTexture) { AEGfxTextureUnload(g_guideBannerTexture);        g_guideBannerTexture = NULL; }
    if (g_shovelGuideBannerTexture) { AEGfxTextureUnload(g_shovelGuideBannerTexture);  g_shovelGuideBannerTexture = NULL; }
    if (g_shovelGuideBannerTexture2) { AEGfxTextureUnload(g_shovelGuideBannerTexture2); g_shovelGuideBannerTexture2 = NULL; }

    // --- Font ---
    if (g_font_id >= 0) { AEGfxDestroyFont(g_font_id); g_font_id = -1; }

    // --- Subsystems (same order as Init) ---
    PlayerAnim_Free(&g_playerAnimator);
    Shop_Unload();
    PauseMenu_Unload();
    Enemy_Kill();
    LoseScreen_Unload();
    LightSystem_Kill();
    FreeBoss();
    Player_Attack_Kill();

    // --- Audio ---
    AEAudioStopGroup(g_sfxGroup);
    AEAudioStopGroup(g_bgmGroup);
    AEAudioStopGroup(g_enemyZoneGroup);
}