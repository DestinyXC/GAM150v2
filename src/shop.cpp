#include "shop.hpp"
#include <stdio.h>
#include "AEEngine.h"

// ---------------------------------------------------------------------------
// SHOP SYSTEM GLOBALS
// ---------------------------------------------------------------------------

// Shop state
int shop_is_active = 0;
int current_shop_page = 0;           // 0 = page 1, 1 = page 2

// Shop popup dimensions
float shop_popup_width = 1600.0f;
float shop_popup_height = 900.0f;

// Shop upgrade boxes (4 boxes)
float upgrade_box_width = 220.0f;
float upgrade_box_height = 50.0f;
float upgrade_box_spacing = 130.0f;

// Arrow button dimensions
float arrow_button_width = 80.0f;    
float arrow_button_height = 80.0f;   

// Hover states for each box
int hover_box1 = 0;  // Pick/Shovel upgrade
int hover_box2 = 0;  // Oxygen upgrade
int hover_box3 = 0;  // Sanity upgrade
int hover_box4 = 0;  // Torch/Light upgrade
int hover_left_arrow = 0;            
int hover_right_arrow = 0;           

// Upgrade levels (shared with main.cpp)
int oxygen_upgrade_level = 0;
int pick_upgrade_level = 0;
int sanity_upgrade_level = 0;
int torch_upgrade_level = 0;
int shovel_repair_level = 0;   

// BOARD OVERLAY DIMENSIONS
static const float BOARD_W = 330.0f;
static const float BOARD_H = 116.0f;
static const float BOARD_OY = -182.0f;
static const float BOARD_X_OFFSETS[5] = { 25.0f, 10.0f, -4.0f, -12.0f, 17.0f};

// Textures
AEGfxTexture* shopBackgroundTexture = nullptr;
AEGfxTexture* shopBackgroundTexture2 = nullptr;

// Meshes
AEGfxVertexList* shopBackgroundImageMesh = nullptr;
AEGfxVertexList* upgradeBoxMesh = nullptr;
AEGfxVertexList* upgradeBoxBorderMesh = nullptr;
AEGfxVertexList* arrowButtonMesh = nullptr;  

// Font
s8 shop_font_id = -1;
s8 shop_description_font_id = -1;  

// ---------------------------------------------------------------------------
// MESH CREATION FUNCTIONS
// ---------------------------------------------------------------------------

AEGfxVertexList* CreateShopBackgroundImageMesh(float width, float height)
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

AEGfxVertexList* CreateUpgradeBoxMesh(float width, float height)
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

AEGfxVertexList* CreateUpgradeBoxBorderMesh(float width, float height, float thickness)
{
    AEGfxMeshStart();

    float hw = width / 2.0f;
    float hh = height / 2.0f;
    float t = thickness;

    // Top border
    AEGfxTriAdd(-hw, hh - t, 0xFFFFFFFF, 0.0f, 0.0f, hw, hh - t, 0xFFFFFFFF, 1.0f, 0.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(hw, hh - t, 0xFFFFFFFF, 1.0f, 0.0f, hw, hh, 0xFFFFFFFF, 1.0f, 1.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);

    // Bottom border
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, 0.0f, 0.0f, hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, -hw, -hh + t, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, hw, -hh + t, 0xFFFFFFFF, 1.0f, 1.0f, -hw, -hh + t, 0xFFFFFFFF, 0.0f, 1.0f);

    // Left border
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, 0.0f, 0.0f, -hw + t, -hh, 0xFFFFFFFF, 1.0f, 0.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(-hw + t, -hh, 0xFFFFFFFF, 1.0f, 0.0f, -hw + t, hh, 0xFFFFFFFF, 1.0f, 1.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);

    // Right border
    AEGfxTriAdd(hw - t, -hh, 0xFFFFFFFF, 0.0f, 0.0f, hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, hw - t, hh, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, hw, hh, 0xFFFFFFFF, 1.0f, 1.0f, hw - t, hh, 0xFFFFFFFF, 0.0f, 1.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateArrowButtonMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    // Right arrow (pointing right)
    AEGfxTriAdd(
        -half_width, -half_height, 0xFF806050, 0.0f, 0.0f,
        half_width, 0.0f, 0xFF806050, 1.0f, 0.5f,
        -half_width, half_height, 0xFF806050, 0.0f, 1.0f);

    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// COST FUNCTIONS
// ---------------------------------------------------------------------------

int GetPickUpgradeCost()
{
    switch (pick_upgrade_level) // max 4
    {
    case 0: return 7;
    case 1: return 15;
    case 2: return 25;
    case 3: return 34;
    default: return -1;
    }
}

int GetOxygenUpgradeCost()
{
    switch (oxygen_upgrade_level) // max 5
    {
    case 0: return 3;
    case 1: return 7;
    case 2: return 15;
    case 3: return 30;
    case 4: return 54;
    default: return -1;
    }
}

int GetSanityUpgradeCost()
{
    switch (sanity_upgrade_level) // max 3
    {
    case 0: return 10;
    case 1: return 20;
    case 2: return 35;
    default: return -1;
    }
}

int GetTorchUpgradeCost()
{
    switch (torch_upgrade_level) // max 3
    {
    case 0: return 12;
    case 1: return 24;
    case 2: return 37;
    default: return -1;
    }
}

int GetShovelRepairCost()
{
    switch (shovel_repair_level) // max 6
    {
    case 0: return 0;  // Unlock ability to dig dirt
    case 1: return 1;  // Unlock ability to attack with shovel
    case 2: return 7; 
	case 3: return 15;
	case 4: return 25;
	case 5: return 34;
    default: return -1;
    }
}

// ---------------------------------------------------------------------------
// UPGRADE FUNCTIONS (WITH ROCK CURRENCY CHECK)
// ---------------------------------------------------------------------------

void PickUpgrade()
{
    int cost = GetPickUpgradeCost();

    if (rocks_mined >= cost && pick_upgrade_level < 5)
    {
        rocks_mined -= cost;
        pick_upgrade_level++;

        // Update mining speed
        switch (pick_upgrade_level)
        {
        case 1: rock_mining_duration = 3.0f; break;
        case 2: rock_mining_duration = 2.0f; break;
        case 3: rock_mining_duration = 1.0f; break;
        case 4: rock_mining_duration = 0.5f; break;
        }

        printf("Pick upgraded to level %d! (Cost: %d rocks)\n", pick_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Pick already at max level!\n");
    }
}

void OxygenUpgrade()
{
    int cost = GetOxygenUpgradeCost();

    if (rocks_mined >= cost && oxygen_upgrade_level < 5)
    {
        rocks_mined -= cost;
        oxygen_upgrade_level++;

        switch (oxygen_upgrade_level)
        {
        case 1: oxygen_max = 30.0f;  oxygen_percentage = 30.0f; break;
		case 2: oxygen_max = 50.0f;  oxygen_percentage = 50.0f; break;
		case 3: oxygen_max = 75.0f;  oxygen_percentage = 75.0f; break;
		case 4: oxygen_max = 100.0f; oxygen_percentage = 100.0f; break;
		case 5: oxygen_max = 120.0f; oxygen_percentage = 120.0f; break;
        }
        printf("Oxygen upgraded to level %d! (Cost: %d rocks)\n", oxygen_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Oxygen already at max level!\n");
    }
}

void SanityUpgrade()
{
    int cost = GetSanityUpgradeCost();

    if (rocks_mined >= cost && sanity_upgrade_level < 3)
    {
        rocks_mined -= cost;
        sanity_upgrade_level++;
        printf("Sanity upgraded to level %d! (Cost: %d rocks)\n", sanity_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Sanity already at max level!\n");
    }
}

void TorchUpgrade()
{
    int cost = GetTorchUpgradeCost();

    if (rocks_mined >= cost && torch_upgrade_level < 3)
    {
        rocks_mined -= cost;
        torch_upgrade_level++;

        switch (torch_upgrade_level)
        {
        case 1: player_light_radius = 300.0f; break;
		case 2: player_light_radius = 400.0f; break;
		case 3: player_light_radius = 500.0f; break;
        }

        printf("Torch upgraded to level %d! (Cost: %d rocks)\n", torch_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Torch already at max level!\n");
    }
}

void ShovelRepairUpgrade()
{
    int cost = GetShovelRepairCost();

    int current_max = shovel2_collected ? 6 : 1;

    if (shovel_repair_level >= current_max)
    {
        printf("Shovel repair already at max level!\n");
        return;
    }
    if (rocks_mined >= cost)
    {
        rocks_mined -= cost;
        shovel_repair_level++;

        // Update digging speed
        switch (shovel_repair_level)
        {
        case 3: shovel_dig_duration = 3.0f; break;
        case 4: shovel_dig_duration = 2.0f; break;
        case 5: shovel_dig_duration = 1.0f; break;
        case 6: shovel_dig_duration = 0.5f; break;
        }

        printf("Shovel repair upgraded to level %d! (Cost: %d rocks)\n", shovel_repair_level, cost);
    }
    else
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
	}
}

// ---------------------------------------------------------------------------
// ROUNDED RECTANGLE OVERLAY HELPER
// Draws a pill-shaped semi-transparent overlay at (cx, cy).
// Used for hover/affordability tinting over the wooden board icons.
// ---------------------------------------------------------------------------
static void DrawBoardOverlay(float cx, float cy, float w, float h, unsigned int col, float alpha)
{
    const int SEGS = 12;
    float r = h / 2.0f;
    float inner_w = w - h;
    if (inner_w < 0.0f) inner_w = 0.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(alpha);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(NULL, 0, 0);

    // Center rectangle
    {
        AEGfxMeshStart();
        float hw = inner_w / 2.0f;
        AEGfxTriAdd(-hw, -r, col, 0, 0, hw, -r, col, 1, 0, -hw, r, col, 0, 1);
        AEGfxTriAdd(hw, -r, col, 1, 0, hw, r, col, 1, 1, -hw, r, col, 0, 1);
        AEGfxVertexList* m = AEGfxMeshEnd();
        AEMtx33 t; AEMtx33Trans(&t, cx, cy);
        AEGfxSetTransform(t.m);
        AEGfxMeshDraw(m, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(m);
    }

    // Left cap
    {
        AEGfxMeshStart();
        for (int i = 0; i < SEGS; i++)
        {
            float a0 = 1.5708f + (float)i / (float)SEGS * 3.14159f;
            float a1 = 1.5708f + (float)(i + 1) / (float)SEGS * 3.14159f;
            AEGfxTriAdd(0.0f, 0.0f, col, 0.5f, 0.5f,
                cosf(a0) * r, sinf(a0) * r, col, 0, 0,
                cosf(a1) * r, sinf(a1) * r, col, 0, 0);
        }
        AEGfxVertexList* m = AEGfxMeshEnd();
        AEMtx33 t; AEMtx33Trans(&t, cx - inner_w / 2.0f, cy);
        AEGfxSetTransform(t.m);
        AEGfxMeshDraw(m, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(m);
    }

    // Right cap
    {
        AEGfxMeshStart();
        for (int i = 0; i < SEGS; i++)
        {
            float a0 = -1.5708f + (float)i / (float)SEGS * 3.14159f;
            float a1 = -1.5708f + (float)(i + 1) / (float)SEGS * 3.14159f;
            AEGfxTriAdd(0.0f, 0.0f, col, 0.5f, 0.5f,
                cosf(a0) * r, sinf(a0) * r, col, 0, 0,
                cosf(a1) * r, sinf(a1) * r, col, 0, 0);
        }
        AEGfxVertexList* m = AEGfxMeshEnd();
        AEMtx33 t; AEMtx33Trans(&t, cx + inner_w / 2.0f, cy);
        AEGfxSetTransform(t.m);
        AEGfxMeshDraw(m, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(m);
    }

    AEGfxSetTransparency(1.0f);  // restore
}

// Helper function to check if mouse is inside a box
int IsMouseInBox(float mouse_x, float mouse_y, float box_x, float box_y, float box_width, float box_height)
{
    float half_width = box_width / 2.0f;
    float half_height = box_height / 2.0f;

    return (mouse_x >= box_x - half_width && mouse_x <= box_x + half_width &&
        mouse_y >= box_y - half_height && mouse_y <= box_y + half_height);
}

// Helper function to get upgrade description
const char* GetUpgradeDescription(int page, int box_index)
{
    if (page == 0) 
    {
        switch (box_index)
        {
        case 0: return "Increases mining speed";
        case 1: return "Extends oxygen capacity";
        case 2: return "Reduces sanity loss";
        case 3: return "Expands light radius";
        default: return "";
        }
    }
    else 
    {
        switch (box_index)
        {
        case 0: return "Repair shovel to dig dirt";
        default: return "";
        }
    }
}

// ---------------------------------------------------------------------------
// SHOP STATE FUNCTIONS
// ---------------------------------------------------------------------------

void Shop_Load()
{
    shopBackgroundTexture = AEGfxTextureLoad("Assets/shopui.png");
    shopBackgroundTexture2 = AEGfxTextureLoad("Assets/shopui2.png");
    shopBackgroundImageMesh = CreateShopBackgroundImageMesh(shop_popup_width, shop_popup_height);
    upgradeBoxMesh = CreateUpgradeBoxMesh(upgrade_box_width, upgrade_box_height);
    upgradeBoxBorderMesh = CreateUpgradeBoxBorderMesh(upgrade_box_width, upgrade_box_height, 3.0f);
    arrowButtonMesh = CreateArrowButtonMesh(arrow_button_width, arrow_button_height);
    shop_font_id = AEGfxCreateFont("Assets/fonts/liberation-mono.ttf", 24);
    shop_description_font_id = AEGfxCreateFont("Assets/fonts/liberation-mono.ttf", 40);
    printf("Shop state loaded!\n");
}

void Shop_Initialize()
{
    shop_is_active = 1;
    current_shop_page = 0;
    printf("Shop state initialized!\n");
}

void Shop_Update()
{
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);

    float mouse_x = (float)mouse_screen_x - 800.0f;
    float mouse_y = 450.0f - (float)mouse_screen_y;

    float box_y = -150.0f;
    float box1_x, box2_x, box3_x, box4_x, box5_x;

    if (current_shop_page == 0)
    {
        // 4 boxes for page 1
        float total_width = (upgrade_box_width * 4) + (upgrade_box_spacing * 3);
        float start_x = -total_width / 2.0f + upgrade_box_width / 2.0f;

        box1_x = start_x;
        box2_x = start_x + (upgrade_box_width + upgrade_box_spacing);
        box3_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 2;
        box4_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 3;
        box5_x = start_x;

        hover_box1 = IsMouseInBox(mouse_x, mouse_y, box1_x + BOARD_X_OFFSETS[0], BOARD_OY, BOARD_W, BOARD_H);
        hover_box2 = IsMouseInBox(mouse_x, mouse_y, box2_x + BOARD_X_OFFSETS[1], BOARD_OY, BOARD_W, BOARD_H);
        hover_box3 = IsMouseInBox(mouse_x, mouse_y, box3_x + BOARD_X_OFFSETS[2], BOARD_OY, BOARD_W, BOARD_H);
        hover_box4 = IsMouseInBox(mouse_x, mouse_y, box4_x + BOARD_X_OFFSETS[3], BOARD_OY, BOARD_W, BOARD_H);
    }
    else
    {
		// 1 centered box for page 2
        box5_x = 0.0f;

        hover_box1 = IsMouseInBox(mouse_x, mouse_y, box5_x + BOARD_X_OFFSETS[4], BOARD_OY, BOARD_W, BOARD_H);
        hover_box2 = 0;
        hover_box3 = 0;
        hover_box4 = 0;
    }

    // Arrow buttons
    float arrow_y = -10.0f;
    float left_arrow_x = -695.0f;
    float right_arrow_x = 710.0f;
    
    if (shovel_collected)
    {
        // Hovering arrow buttons
        hover_left_arrow = IsMouseInBox(mouse_x, mouse_y, left_arrow_x, arrow_y, arrow_button_width, arrow_button_height);
        hover_right_arrow = IsMouseInBox(mouse_x, mouse_y, right_arrow_x, arrow_y, arrow_button_width, arrow_button_height);

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (hover_left_arrow && current_shop_page > 0)
            {
                current_shop_page--;
                printf("Switched to page %d\n", current_shop_page + 1);
            }
            else if (hover_right_arrow && current_shop_page < 1)
            {
                current_shop_page++;
                printf("Switched to page %d\n", current_shop_page + 1);
            }
        }
    }

    // Upgrade clicks (always available regardless of shovel)
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (current_shop_page == 0)
        {
            if (hover_box1) PickUpgrade();
            else if (hover_box2) OxygenUpgrade();
            else if (hover_box3) SanityUpgrade();
            else if (hover_box4) TorchUpgrade();
        }
        else if (current_shop_page == 1)
        {
            if (hover_box1) ShovelRepairUpgrade();
        }
    }
}

void Shop_Draw()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Draw background based on current page
	AEGfxTexture* current_texture = (current_shop_page == 0) ? shopBackgroundTexture : shopBackgroundTexture2;

    if (shopBackgroundTexture && shopBackgroundImageMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(current_texture, 0, 0);

        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, 1.0f, 1.0f);
        AEMtx33Rot(&rotate, 0.0f);
        AEMtx33Trans(&translate, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(shopBackgroundImageMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Calculate positions based on page
    float box_y = -150.0f;
    float box1_x, box2_x, box3_x, box4_x, box5_x;

    if (current_shop_page == 0)
    {
        // Page 1: 4 boxes layout
        float total_width = (upgrade_box_width * 4) + (upgrade_box_spacing * 3);
        float start_x = -total_width / 2.0f + upgrade_box_width / 2.0f;

        box1_x = start_x;
        box2_x = start_x + (upgrade_box_width + upgrade_box_spacing);
        box3_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 2;
        box4_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 3;
        box5_x = 0.0f;
    }
    else
    {
        // Page 2: 1 centered box
        box1_x = 0.0f;  // Centered
        box2_x = 0.0f;
        box3_x = 0.0f;
        box4_x = 0.0f;
        box5_x = 0.0f;
    }

    // Get costs and levels based on current page
    int cost1{}, cost2{}, cost3{}, cost4{};
    int level1{}, level2{}, level3{}, level4{};
    int max_level1{}, max_level2{}, max_level3{}, max_level4{};

    if (current_shop_page == 0)
    {
        cost1 = GetPickUpgradeCost();
        cost2 = GetOxygenUpgradeCost();
        cost3 = GetSanityUpgradeCost();
        cost4 = GetTorchUpgradeCost();
        level1 = pick_upgrade_level;
        level2 = oxygen_upgrade_level;
        level3 = sanity_upgrade_level;
        level4 = torch_upgrade_level;
        max_level1 = 4;
        max_level2 = 5;
        max_level3 = 3;
        max_level4 = 3;
    }
    else  // Page 2
    {
        cost1 = GetShovelRepairCost();
        level1 = shovel_repair_level;
        max_level1 = shovel2_collected ? 6 : 1;
    }

    // ---------------------------------------------------------------------------
    // BOARD OVERLAY - covers the full wooden board area per upgrade slot
    // ---------------------------------------------------------------------------
    float board_xs[5] = { box1_x + BOARD_X_OFFSETS[0], box2_x + BOARD_X_OFFSETS[1], box3_x + BOARD_X_OFFSETS[2], box4_x + BOARD_X_OFFSETS[3], box5_x + BOARD_X_OFFSETS[4]};
    int   costs[4]    = { cost1, cost2, cost3, cost4 };
    int   levels[4]   = { level1, level2, level3, level4 };
    int   max_lvls[4] = { max_level1, max_level2, max_level3, max_level4 };
    int   num_boxes   = (current_shop_page == 0) ? 4 : 1;
	int   board_start = (current_shop_page == 0) ? 0 : 4;  // box5_x is used for page 2

    for (int i = 0; i < num_boxes; i++)
    {
        float bx    = board_xs[board_start + i];
        int   cost  = costs[i];
        int   level = levels[i];
        int   maxl  = max_lvls[i];
        int   maxed = (level >= maxl);
        int   can_afford = (rocks_mined >= cost);

        if (maxed)
        {
            // Maxed
            DrawBoardOverlay(bx, BOARD_OY, BOARD_W, BOARD_H, 0xFFFF0000, 0.18f);
        }
        else if (!can_afford)
        {
            // Can't afford
            DrawBoardOverlay(bx, BOARD_OY, BOARD_W, BOARD_H, 0xFFFFDD88, 0.40f);
        }
        else if (can_afford)
        {
            // Affordable, not hovered
            DrawBoardOverlay(bx, BOARD_OY, BOARD_W, BOARD_H, 0xFF00FF00, 0.25f);
        }
    }

    // Draw arrow buttons
    float arrow_y = -10.0f;
    float left_arrow_x = -695.0f;
    float right_arrow_x = 710.0f;

    // Left arrow (only if not on first page)
    if (shovel_collected)
    {
        if (current_shop_page > 0)
        {
            if (hover_left_arrow)
                AEGfxSetColorToMultiply(0.78f, 0.71f, 0.62f, 1.0f);  // Brown on hover
            else
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);  // White

            AEMtx33 arrow_scale, arrow_rotate, arrow_translate, arrow_transform;
            AEMtx33Scale(&arrow_scale, 1.0f, 1.0f);
            AEMtx33Rot(&arrow_rotate, 3.14159f);  // Rotate 180 degrees (pointing left)
            AEMtx33Trans(&arrow_translate, left_arrow_x, arrow_y);
            AEMtx33Concat(&arrow_transform, &arrow_rotate, &arrow_scale);
            AEMtx33Concat(&arrow_transform, &arrow_translate, &arrow_transform);
            AEGfxSetTransform(arrow_transform.m);
            AEGfxMeshDraw(arrowButtonMesh, AE_GFX_MDM_TRIANGLES);
        }

        // Right arrow (only if not on last page)
        if (current_shop_page < 1)  // Max page index is 1 (page 2)
        {
            if (hover_right_arrow)
                AEGfxSetColorToMultiply(0.78f, 0.71f, 0.62f, 1.0f);  // Brown on hover
            else
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);  // White

            AEMtx33 arrow_scale, arrow_rotate, arrow_translate, arrow_transform;
            AEMtx33Scale(&arrow_scale, 1.0f, 1.0f);
            AEMtx33Rot(&arrow_rotate, 0.0f);  // No rotation (pointing right)
            AEMtx33Trans(&arrow_translate, right_arrow_x, arrow_y);
            AEMtx33Concat(&arrow_transform, &arrow_rotate, &arrow_scale);
            AEMtx33Concat(&arrow_transform, &arrow_translate, &arrow_transform);
            AEGfxSetTransform(arrow_transform.m);
            AEGfxMeshDraw(arrowButtonMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Display text
    if (shop_font_id >= 0)
    {
        // Display current rock count at the bottom
        char rocks_text[64];
        sprintf_s(rocks_text, 64, "Rocks: %d", rocks_mined);
        AEGfxPrint(shop_font_id, rocks_text, -0.2f, -0.85f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        // Display page indicator
        char page_text[32];
        sprintf_s(page_text, 32, "Page %d/2", current_shop_page + 1);
        AEGfxPrint(shop_font_id, page_text, -0.05f, 0.68f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        if (shovel_collected)
        {
            // Display page indicator
            char page_text[32];
            sprintf_s(page_text, 32, "Page %d/2", current_shop_page + 1);
            AEGfxPrint(shop_font_id, page_text, -0.05f, 0.68f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        char level_text[64];

        if (current_shop_page == 0)
        {
            // Display all 4 boxes for page 1
            // PICKAXE
            if (cost1 < 0)
                AEGfxPrint(shop_font_id, (char*)"MAX LEVEL", -0.71f, -0.35f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            else
            {
                sprintf_s(level_text, 64, "Lv %d", level1);
                AEGfxPrint(shop_font_id, level_text, -0.67f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
                sprintf_s(level_text, 64, "%d", cost1);
                AEGfxPrint(shop_font_id, level_text, -0.50f, -0.44f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }

            // OXYGEN 
            if (cost2 < 0)
                AEGfxPrint(shop_font_id, (char*)"MAX LEVEL", -0.29f, -0.35f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
			else
            {
                sprintf_s(level_text, 64, "Lv %d", level2);
                AEGfxPrint(shop_font_id, level_text, -0.24f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
                sprintf_s(level_text, 64, "%d", cost2);
                AEGfxPrint(shop_font_id, level_text, -0.08f, -0.44f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }

            // SANITY
			if (cost3 < 0)
				AEGfxPrint(shop_font_id, (char*)"MAX LEVEL", 0.13f, -0.35f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            else
            {
                sprintf_s(level_text, 64, "Lv %d", level3);
                AEGfxPrint(shop_font_id, level_text, 0.18f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
                sprintf_s(level_text, 64, "%d", cost3);
                AEGfxPrint(shop_font_id, level_text, 0.34f, -0.44f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }

            // TORCH
			if (cost4 < 0)
                AEGfxPrint(shop_font_id, (char*)"MAX LEVEL", 0.56f, -0.35f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
			else
            {
                sprintf_s(level_text, 64, "Lv %d", level4);
                AEGfxPrint(shop_font_id, level_text, 0.61f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
                sprintf_s(level_text, 64, "%d", cost4);
                AEGfxPrint(shop_font_id, level_text, 0.77f, -0.44f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
        else
        {
            // Display shovel upgrade only for page 2
			int current_max = shovel2_collected ? 6 : 1;

            if (shovel_repair_level >= current_max)
            {
                AEGfxPrint(shop_font_id, (char*)"MAX LEVEL", -0.06f, -0.35f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            }
            else
            {
                sprintf_s(level_text, 64, "Lv %d", level1);
                AEGfxPrint(shop_font_id, level_text, -0.02f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
                sprintf_s(level_text, 64, "%d", cost1);
                AEGfxPrint(shop_font_id, level_text, 0.155f, -0.44f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
    }

     //Draw descriptions below each box
    if (shop_description_font_id >= 0)
    {
        if (current_shop_page == 0)
        {
            AEGfxPrint(shop_description_font_id, (char*)"Increases", -0.70f, -0.42f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
            AEGfxPrint(shop_description_font_id, (char*)"mining speed", -0.72f, -0.48f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);

            AEGfxPrint(shop_description_font_id, (char*)"Extends", -0.26f, -0.42f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
            AEGfxPrint(shop_description_font_id, (char*)"oxygen capacity", -0.32f, -0.48f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);

            AEGfxPrint(shop_description_font_id, (char*)"Reduces", 0.16f, -0.42f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
            AEGfxPrint(shop_description_font_id, (char*)"sanity loss", 0.13f, -0.48f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);

            AEGfxPrint(shop_description_font_id, (char*)"Expands", 0.59f, -0.42f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
            AEGfxPrint(shop_description_font_id, (char*)"light radius", 0.55f, -0.48f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
        }
        else
        {
            AEGfxPrint(shop_description_font_id, (char*)"Repair shovel", -0.08f, -0.42f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
            AEGfxPrint(shop_description_font_id, (char*)"to dig dirt", -0.07f, -0.48f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f);
        }
    }
}

void Shop_Free()
{
    shop_is_active = 0;
    printf("Shop state freed!\n");
}

void Shop_Unload()
{
    // Free Meshes
    if (shopBackgroundImageMesh) {
        AEGfxMeshFree(shopBackgroundImageMesh);
        shopBackgroundImageMesh = nullptr;
    }
    if (upgradeBoxMesh) {
        AEGfxMeshFree(upgradeBoxMesh);
        upgradeBoxMesh = nullptr;
    }
    if (upgradeBoxBorderMesh) {
        AEGfxMeshFree(upgradeBoxBorderMesh);
        upgradeBoxBorderMesh = nullptr;
    }
    if (arrowButtonMesh) {
        AEGfxMeshFree(arrowButtonMesh);
        arrowButtonMesh = nullptr;
    }
    if (shopBackgroundTexture) {
        AEGfxTextureUnload(shopBackgroundTexture);
        shopBackgroundTexture = nullptr;
    }

	// Unload textures and fonts
    if (shopBackgroundTexture2) {
        AEGfxTextureUnload (shopBackgroundTexture2);
        shopBackgroundTexture2 = nullptr;
	}
    if (shop_font_id >= 0) {
        AEGfxDestroyFont(shop_font_id);
        shop_font_id = -1;
    }
    if (shop_description_font_id >= 0) { 
        AEGfxDestroyFont(shop_description_font_id);
        shop_description_font_id = -1;
    }
    printf("Shop state unloaded!\n");
}
