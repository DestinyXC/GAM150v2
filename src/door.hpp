#ifndef DOOR_HPP
#define DOOR_HPP

#include "AEEngine.h"

#define MAXDOORS 2

typedef struct {
    float x, y;
    float baseWidth, baseHeight;
    int spritePos;
    float offsetX;
    float alpha;
    float animTimer;
    int active;
    AEGfxVertexList* mesh;
} Door;

extern Door doors[MAXDOORS];
extern int doorCount;

// Initialize doors at safezone position (call from GameInit)
void DoorInit(float safezoneymax);

// Update doors based on player movement (call from GameUpdate after UpdatePhysics)
void DoorUpdate(float dt, float playerY, float movey, float safezoneymax);

// Render doors (call from GameDraw after RenderRocks)
void DoorDraw(AEGfxTexture* tilesetTexture);

// Cleanup (call from GameFree)
void DoorFree(void);

#endif
