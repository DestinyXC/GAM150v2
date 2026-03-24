#ifndef ENEMY_HPP
#define ENEMY_HPP



// CONFIGURATION

#define MAX_ENEMIES          20

// Depth (tile rows below surface row 9) where each type starts spawning
#define FERAL_SPAWN_DEPTH    20      // rows below surface (~1280 px)
#define INSANE_SPAWN_DEPTH   45      // rows below surface (~2880 px)
#define MOLE_SPAWN_DEPTH     65     // rows below surface (~3840 px)

#define ENEMY_SPAWN_CHANCE   0.35f

#define ENEMY_WIDTH          90.0f
#define ENEMY_HEIGHT         90.0f

#define ENEMY_SPEED_FERAL    3.0f
#define ENEMY_SPEED_INSANE   4.0f    
#define ENEMY_SPEED_MOLE     4.0f    

// Detection radius (the red dashed circle)
#define DETECT_RADIUS_FERAL  120.0f
#define DETECT_RADIUS_INSANE 150.0f
#define DETECT_RADIUS_MOLE   130.0f  

// Damage per second while touching player
#define DAMAGE_FERAL         20.0f
#define DAMAGE_INSANE        25.0f
#define DAMAGE_MOLE          30.0f   // Highest damage 

#define PLAYER_MAX_HP        100.0f
#define CIRCLE_SEGMENTS      64

// ENEMY TYPE ENUM

typedef enum
{
    ENEMY_TYPE_FERAL = 0,   // The Feral  - teeth, no weapon
    ENEMY_TYPE_INSANE = 1,   // The Insane - bloody hammer
    ENEMY_TYPE_MOLE = 2    // The Mole   - crawling enemy
} EnemyType;

// ENEMY STRUCTURE

typedef struct
{
    float     x, y;
    float     patrol_x;
    float     patrol_range;
    int       direction;
    int       active;
    int       is_chasing;
    EnemyType type;
    float     speed;
    float     damage;         // HP/sec on contact
    float     detect_radius;
} Enemy;


// GLOBALS  (defined in enemy.cpp)

extern Enemy enemies[MAX_ENEMIES];
extern int   enemy_count;
extern float player_hp;
extern int   player_is_dead;
extern EnemyType last_killer_type;

// PUBLIC FUNCTIONS

void Enemy_Init();
void Enemy_Update(float dt,
    float px, float py,
    float* out_px, float* out_py);
void Enemy_Draw(float camera_x, float camera_y);
//void Enemy_DrawPlayerHP(float camera_x, float camera_y,
//    float px, float py, float hp);
void Enemy_Kill();
void Enemy_Take_Damage(int enemy_index, float damage);

#endif // ENEMY_HPP