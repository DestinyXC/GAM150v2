#include "door.hpp"
#include "AEEngine.h"

// Forward declare mesh helper from your main .cpp
AEGfxVertexList* CreateSpritesheetTileMesh(int spriteposition, float size);

extern float camera_x;
extern float camera_y;
extern AEGfxTexture* g_tilesetTexture;  // Use existing tileset texture

// Local constant
static const float TILESIZE = 64.0f;

Door doors[MAXDOORS];
int doorCount = 0;

void DoorInit(float safezoneymax) {
    DoorFree();
    doorCount = 0;

    float doorY = safezoneymax - (TILESIZE);

    // Use the existing tileset texture from main.cpp
    for (int i = 0; i < 2; i++) {
        // Move right by 0.5 tiles (add TILESIZE * 0.5f instead of subtract)
        doors[i].x = (i == 0) ? -TILESIZE / 2.0f - TILESIZE + (TILESIZE * 0.5f) : TILESIZE / 2.0f - TILESIZE + (TILESIZE * 0.5f);
        doors[i].y = doorY;
        doors[i].baseWidth = TILESIZE;
        doors[i].baseHeight = TILESIZE;
        doors[i].spritePos = 60;  // Sprite positions 42 and 43
        doors[i].mesh = CreateSpritesheetTileMesh(doors[i].spritePos, TILESIZE);
        doors[i].offsetX = 0.0f;
        doors[i].alpha = 1.0f;
        doors[i].animTimer = 0.0f;
        doors[i].active = (doors[i].mesh != NULL) ? 1 : 0;
        doorCount++;
    }
}
void DoorUpdate(float dt, float playerY, float movey, float safezoneymax) {
    if (doorCount <= 0 || dt <= 0.0f) return;

    // Door Y position - must match DoorInit doorY
    float doorY = safezoneymax - (TILESIZE);

    // Distance from player to door (how far vertically)
    float distanceToDoor = fabsf(playerY - doorY);

    // Door opens when player is within 1 tile (TILESIZE = 64) of door
    // Player can be above or below the door
    int approaching = (distanceToDoor <= TILESIZE);

    for (int i = 0; i < doorCount; i++) {
        Door* d = &doors[i];
        if (!d->active || !d->mesh) continue;

        float slideDir = (i == 0) ? -1.0f : 1.0f;  // Door 0 slides left, Door 1 slides right

        if (approaching) {
            // Open doors - slide outward and fade in
            d->offsetX += slideDir * 150.0f * dt;
            d->alpha -= 2.0f * dt;
        }
        else {
            // Close doors - slide back inward and fade out
            d->offsetX -= slideDir * 100.0f * dt;
            d->alpha += 2.0f * dt;
        }

        // Clamp offsetX so doors slide out smoothly
        if (d->offsetX * slideDir > TILESIZE)
            d->offsetX = TILESIZE * slideDir;
        if (d->offsetX * slideDir < 0.0f)
            d->offsetX = 0.0f;

        // Clamp alpha between 0 and 1
        if (d->alpha > 1.0f) d->alpha = 1.0f;
        if (d->alpha < 0.0f) d->alpha = 0.0f;

        d->animTimer += dt;
    }
}

void DoorDraw(AEGfxTexture* tilesetTexture) {
    if (!tilesetTexture || doorCount <= 0) return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxTextureSet(tilesetTexture, 0, 0);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < doorCount; i++) {
        Door* d = &doors[i];
        if (!d->active || d->alpha <= 0.01f || !d->mesh) continue;

        AEGfxSetTransparency(d->alpha);

        AEMtx33 scale, rot, trans, tf;
        AEMtx33Scale(&scale, 1.0f, 1.0f);
        AEMtx33Rot(&rot, 0.0f);
        AEMtx33Trans(&trans, d->x + d->offsetX, d->y);
        AEMtx33Concat(&tf, &rot, &scale);
        AEMtx33Concat(&tf, &trans, &tf);
        AEGfxSetTransform(tf.m);

        AEGfxMeshDraw(d->mesh, AE_GFX_MDM_TRIANGLES);
    }
}

void DoorFree(void) {
    for (int i = 0; i < MAXDOORS; i++) {
        if (doors[i].mesh != NULL) {
            AEGfxMeshFree(doors[i].mesh);
            doors[i].mesh = NULL;
        }
        doors[i].active = 0;
        doors[i].alpha = 0.0f;
        doors[i].offsetX = 0.0f;
    }
    doorCount = 0;
}