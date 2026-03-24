#ifndef SHOP_HPP
#define SHOP_HPP

// Shop state
extern int shop_is_active;

// Upgrade levels
extern int oxygen_upgrade_level;
extern int pick_upgrade_level;
extern int sanity_upgrade_level;
extern int torch_upgrade_level;
extern int shovel_repair_level;

// Rock currency
extern int rocks_mined;

// Shovel collections state
extern int shovel_collected;
extern int shovel2_collected;

// Mining duration
extern float rock_mining_duration;

// Oxygen system
extern float oxygen_max;
extern float oxygen_percentage;

// Light radius
extern float player_light_radius;

// Shovel digging duration
extern float shovel_dig_duration;

// Shop functions
void Shop_Load();
void Shop_Initialize();
void Shop_Update();
void Shop_Draw();
void Shop_Free();
void Shop_Unload();

// Upgrade functions
void PickUpgrade();
void OxygenUpgrade();
void SanityUpgrade();
void TorchUpgrade();
void ShovelRepairUpgrade();

// Cost functions
int GetPickUpgradeCost();
int GetOxygenUpgradeCost();
int GetSanityUpgradeCost();
int GetTorchUpgradeCost();
int GetShovelRepairCost();

#endif // SHOP_HPP
