// includes

#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "GameStates.h"
#include "pausemenu.hpp"
#include "shop.hpp"
#include <string>
#include <iostream>
#include "storyscreen.hpp" 



// HELPER FUNCTIONS
s8 trycreatefont(const char* FontName, int sizeF) {
	// Create font (ID 1) with size 40
	s8 fontId = AEGfxCreateFont(FontName, sizeF);
	// Check if font creation failed
	if (fontId < 0)
	{
		// Try alternative fonts using stack-allocated buffers
		char altPath1[512];
		sprintf_s(altPath1, sizeof(altPath1), "../%s", FontName);
		fontId = AEGfxCreateFont(altPath1, sizeF);

		if (fontId < 0)
		{
			char altPath2[512];
			sprintf_s(altPath2, sizeof(altPath2), "../../%s", FontName);
			fontId = AEGfxCreateFont(altPath2, sizeF);
		}
	}
	return fontId;
}

// create a new helper file to create the image
AEGfxTexture* tryLoadTexture(const char* imagePath) {
	AEGfxTexture* texture = AEGfxTextureLoad(imagePath);

	// Check if texture loading failed
	if (texture == nullptr)
	{
		std::cout << "Failed to load texture at: " << imagePath << std::endl;
		
		// Try alternative paths using stack-allocated buffers
		char altPath1[512];
		sprintf_s(altPath1, sizeof(altPath1), "../%s", imagePath);
		texture = AEGfxTextureLoad(altPath1);

		if (texture == nullptr)
		{
			std::cout << "Failed to load texture at: " << altPath1 << std::endl;
			
			char altPath2[512];
			sprintf_s(altPath2, sizeof(altPath2), "../../%s", imagePath);
			texture = AEGfxTextureLoad(altPath2);
			if (texture == nullptr)
			{
				std::cout << "Failed to load texture at: " << altPath2 << std::endl;
			}
			else
			{
				std::cout << "Successfully loaded texture at: " << altPath2 << std::endl;
			}
		}
		else
		{
			std::cout << "Successfully loaded texture at: " << altPath1 << std::endl;
		}
	}
	else
	{
		std::cout << "Successfully loaded texture at: " << imagePath << std::endl;
	}
	return texture;
}

// Simple point-in-box check for button clicks
int CheckPointInBox(float px, float py, float boxX, float boxY, float width, float height)
{
	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;

	if (px >= boxX - halfWidth && px <= boxX + halfWidth &&
		py >= boxY - halfHeight && py <= boxY + halfHeight)
	{
		return 1;
	}
	return 0;
}

// Game states
enum GameState
{
	GS_MAINMENU,
	GS_STORY,
	GS_GAMEPLAY,
	GS_CREDITS,
	GS_QUIT
};

// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	int gGameRunning = 1;
	GameState currentState = GS_MAINMENU;

	// Menu button variables
	const float buttonWidth = 660.0f;//500 original changed
	const float buttonHeight = 135.0f;//50 original chanaged
	const float buttonSpacing = 172.0f;
	const float buttonX = -30.0f;// new x to move buttons left


	float playButtonY = 20.0f;
	float exitButtonY = playButtonY - buttonSpacing;
	float creditsButtonY = exitButtonY - buttonSpacing;

	int playButtonHovered = 0;
	int exitButtonHovered = 0;
	int creditsButtonHovered = 0;

	// Rock animation variables
	float rockVibrateTime = 0.0f;
	const float rockVibrateSpeed = 20.0f;  // Speed of vibration
	const float rockVibrateAmount = 3.0f;  // How much to vibrate (pixels)

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Core Break");

	s8 fontId = trycreatefont("../Assets/Fonts/BitcountPropSingle_Cursive-Bold.ttf", 40);
	StoryScreen_Load(fontId);
	AEGfxTexture* backgroundTexture = tryLoadTexture("../Assets/MainMenu/MainmenuImg.png");

	if (backgroundTexture == nullptr)
	{
		std::cout << "ERROR: Background texture is NULL! Image will not display." << std::endl;
	}
	else
	{
		std::cout << "Background texture loaded successfully!" << std::endl;
	}


	// ============== MENU MESHES ==============

	// Create menu button mesh with curved left/right sides (stadium/pill shape)
	AEGfxMeshStart();

	const float buttonHalfWidth = 320.0f;
	const float buttonHalfHeight = 60.0f;
	const int sideSegments = 16;  // Number of segments for semicircles (more = smoother)

	//u32 buttonColor = 0xFFBBA888;
	u32 buttonColor = 0x00BBA888;  // 0x00 = fully transparent
	// Center rectangle (main body)
	AEGfxTriAdd(
		-buttonHalfWidth + buttonHalfHeight, -buttonHalfHeight, buttonColor, 0.0f, 1.0f,
		buttonHalfWidth - buttonHalfHeight, -buttonHalfHeight, buttonColor, 1.0f, 1.0f,
		-buttonHalfWidth + buttonHalfHeight, buttonHalfHeight, buttonColor, 0.0f, 0.0f);
	AEGfxTriAdd(
		buttonHalfWidth - buttonHalfHeight, -buttonHalfHeight, buttonColor, 1.0f, 1.0f,
		buttonHalfWidth - buttonHalfHeight, buttonHalfHeight, buttonColor, 1.0f, 0.0f,
		-buttonHalfWidth + buttonHalfHeight, buttonHalfHeight, buttonColor, 0.0f, 0.0f);

	// Left semicircle (curved left side)
	for (int i = 0; i < sideSegments; ++i)
	{
		float angle1 = 0.5f * 3.14159f + i * 3.14159f / sideSegments;
		float angle2 = 0.5f * 3.14159f + (i + 1) * 3.14159f / sideSegments;

		AEGfxTriAdd(
			-buttonHalfWidth + buttonHalfHeight, 0.0f, buttonColor, 0.5f, 0.5f,
			-buttonHalfWidth + buttonHalfHeight + buttonHalfHeight * cosf(angle1),
			buttonHalfHeight * sinf(angle1), buttonColor, 0.0f, 0.5f,
			-buttonHalfWidth + buttonHalfHeight + buttonHalfHeight * cosf(angle2),
			buttonHalfHeight * sinf(angle2), buttonColor, 0.0f, 0.5f);
	}

	// Right semicircle (curved right side)
	for (int i = 0; i < sideSegments; ++i)
	{
		float angle1 = -0.5f * 3.14159f + i * 3.14159f / sideSegments;
		float angle2 = -0.5f * 3.14159f + (i + 1) * 3.14159f / sideSegments;

		AEGfxTriAdd(
			buttonHalfWidth - buttonHalfHeight, 0.0f, buttonColor, 0.5f, 0.5f,
			buttonHalfWidth - buttonHalfHeight + buttonHalfHeight * cosf(angle1),
			buttonHalfHeight * sinf(angle1), buttonColor, 1.0f, 0.5f,
			buttonHalfWidth - buttonHalfHeight + buttonHalfHeight * cosf(angle2),
			buttonHalfHeight * sinf(angle2), buttonColor, 1.0f, 0.5f);
	}

	AEGfxVertexList* buttonMesh = AEGfxMeshEnd();


	// Create button hover mesh (lighter shade with curved sides)
	AEGfxMeshStart();

	//u32 hoverColor = 0xFFD4BCA4;
	u32 hoverColor = 0x00D4BCA4;  // 0x00 = fully transparent
	// Center rectangle
	AEGfxTriAdd(
		-buttonHalfWidth + buttonHalfHeight, -buttonHalfHeight, hoverColor, 0.0f, 1.0f,
		buttonHalfWidth - buttonHalfHeight, -buttonHalfHeight, hoverColor, 1.0f, 1.0f,
		-buttonHalfWidth + buttonHalfHeight, buttonHalfHeight, hoverColor, 0.0f, 0.0f);
	AEGfxTriAdd(
		buttonHalfWidth - buttonHalfHeight, -buttonHalfHeight, hoverColor, 1.0f, 1.0f,
		buttonHalfWidth - buttonHalfHeight, buttonHalfHeight, hoverColor, 1.0f, 0.0f,
		-buttonHalfWidth + buttonHalfHeight, buttonHalfHeight, hoverColor, 0.0f, 0.0f);

	// Left semicircle
	for (int i = 0; i < sideSegments; ++i)
	{
		float angle1 = 0.5f * 3.14159f + i * 3.14159f / sideSegments;
		float angle2 = 0.5f * 3.14159f + (i + 1) * 3.14159f / sideSegments;

		AEGfxTriAdd(
			-buttonHalfWidth + buttonHalfHeight, 0.0f, hoverColor, 0.5f, 0.5f,
			-buttonHalfWidth + buttonHalfHeight + buttonHalfHeight * cosf(angle1),
			buttonHalfHeight * sinf(angle1), hoverColor, 0.0f, 0.5f,
			-buttonHalfWidth + buttonHalfHeight + buttonHalfHeight * cosf(angle2),
			buttonHalfHeight * sinf(angle2), hoverColor, 0.0f, 0.5f);
	}

	// Right semicircle
	for (int i = 0; i < sideSegments; ++i)
	{
		float angle1 = -0.5f * 3.14159f + i * 3.14159f / sideSegments;
		float angle2 = -0.5f * 3.14159f + (i + 1) * 3.14159f / sideSegments;

		AEGfxTriAdd(
			buttonHalfWidth - buttonHalfHeight, 0.0f, hoverColor, 0.5f, 0.5f,
			buttonHalfWidth - buttonHalfHeight + buttonHalfHeight * cosf(angle1),
			buttonHalfHeight * sinf(angle1), hoverColor, 1.0f, 0.5f,
			buttonHalfWidth - buttonHalfHeight + buttonHalfHeight * cosf(angle2),
			buttonHalfHeight * sinf(angle2), hoverColor, 1.0f, 0.5f);
	}

	AEGfxVertexList* buttonHoverMesh = AEGfxMeshEnd();

	// Create rock decoration (irregular shape) - UNCOMMENTED
	AEGfxMeshStart();
	u32 rockColor = 0xFF9B7B5B;  // Brown rock color
	AEGfxTriAdd(
		-25.0f, -20.0f, rockColor, 0.0f, 1.0f,
		30.0f, -15.0f, rockColor, 1.0f, 1.0f,
		-10.0f, 25.0f, rockColor, 0.0f, 0.0f);
	AEGfxTriAdd(
		30.0f, -15.0f, rockColor, 1.0f, 1.0f,
		20.0f, 20.0f, rockColor, 1.0f, 0.0f,
		-10.0f, 25.0f, rockColor, 0.0f, 0.0f);
	AEGfxVertexList* rockMesh = AEGfxMeshEnd();

	// Create rock 2 (different shape)
	AEGfxMeshStart();
	u32 rock2Color = 0xFF8A6A4A;  // Slightly darker brown
	AEGfxTriAdd(
		-30.0f, -18.0f, rock2Color, 0.0f, 1.0f,
		25.0f, -20.0f, rock2Color, 1.0f, 1.0f,
		-15.0f, 28.0f, rock2Color, 0.0f, 0.0f);
	AEGfxTriAdd(
		25.0f, -20.0f, rock2Color, 1.0f, 1.0f,
		15.0f, 22.0f, rock2Color, 1.0f, 0.0f,
		-15.0f, 28.0f, rock2Color, 0.0f, 0.0f);
	AEGfxVertexList* rock2Mesh = AEGfxMeshEnd();

	// Create rock 3 (smaller, rounder)
	AEGfxMeshStart();
	u32 rock3Color = 0xFFAA8A6A;  // Lighter brown
	AEGfxTriAdd(
		-20.0f, -15.0f, rock3Color, 0.0f, 1.0f,
		22.0f, -12.0f, rock3Color, 1.0f, 1.0f,
		-8.0f, 20.0f, rock3Color, 0.0f, 0.0f);
	AEGfxTriAdd(
		22.0f, -12.0f, rock3Color, 1.0f, 1.0f,
		18.0f, 18.0f, rock3Color, 1.0f, 0.0f,
		-8.0f, 20.0f, rock3Color, 0.0f, 0.0f);
	AEGfxVertexList* rock3Mesh = AEGfxMeshEnd();

	// Create background mesh (full screen quad)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxVertexList* backgroundMesh = AEGfxMeshEnd();

	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		// Get delta time for animation
		float dt = (float)AEFrameRateControllerGetFrameTime();

		// Update rock vibration animation
		rockVibrateTime += dt * rockVibrateSpeed;

		// Get mouse position
		s32 mouseX, mouseY;
		AEInputGetCursorPosition(&mouseX, &mouseY);

		// Convert screen coordinates to world coordinates
		float worldMouseX = (float)mouseX - 800.0f;  // Center X (1600/2)
		float worldMouseY = 450.0f - (float)mouseY;  // Center Y (900/2) and flip Y

		// ============== MAIN MENU STATE ==============
		if (currentState == GS_MAINMENU)
		{
			// Check button hover states
			playButtonHovered = CheckPointInBox(worldMouseX, worldMouseY, buttonX, playButtonY, buttonWidth, buttonHeight);
			exitButtonHovered = CheckPointInBox(worldMouseX, worldMouseY, buttonX, exitButtonY, buttonWidth, buttonHeight);
			creditsButtonHovered = CheckPointInBox(worldMouseX, worldMouseY, buttonX, creditsButtonY, buttonWidth, buttonHeight);

			// Check button clicks
			if (AEInputCheckTriggered(AEVK_LBUTTON))
			{
				if (playButtonHovered)
				{
					StoryScreen_Reset();            
					currentState = GS_STORY;
				}
				else if (exitButtonHovered)
				{
					currentState = GS_QUIT;
					gGameRunning = 0;
				}
				else if (creditsButtonHovered)
				{
					currentState = GS_CREDITS;
				}
			}

			// RENDERING MAIN MENU
			AEGfxSetBackgroundColor(0.73f, 0.66f, 0.59f); // Tan/beige background

			// Draw background image (draw FIRST, before other elements)
			if (backgroundTexture != nullptr)
			{
				AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
				AEGfxTextureSet(backgroundTexture, 0.0f, 0.0f);
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
				AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
				AEGfxSetTransparency(1.0f);
				AEGfxSetBlendMode(AE_GFX_BM_BLEND);

				// Use identity transform and just scale the mesh
				AEMtx33 bgTransform;
				AEMtx33Scale(&bgTransform, 1600.0f, 900.0f);

				AEGfxSetTransform(bgTransform.m);
				AEGfxMeshDraw(backgroundMesh, AE_GFX_MDM_TRIANGLES);
			}
			else
			{
				// Debug: Draw a colored rectangle if texture fails to load
				AEGfxSetRenderMode(AE_GFX_RM_COLOR);
				AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); // Red to indicate error
				AEMtx33 testTransform;
				AEMtx33Scale(&testTransform, 1600.0f, 900.0f);
				AEGfxSetTransform(testTransform.m);
				AEGfxMeshDraw(backgroundMesh, AE_GFX_MDM_TRIANGLES);
			}

			// Set render mode back for other elements
			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
			AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetTransparency(1.0f);

			// Helper function to draw a button
			auto drawButton = [&](float xPos, float yPos, int hovered, AEGfxVertexList* mesh) {

				// Draw button
				AEMtx33 btnTransform = { 0 };
				AEMtx33 btnScale = { 0 };
				AEMtx33 btnRotate = { 0 };
				AEMtx33 btnTranslate = { 0 };
				AEMtx33Scale(&btnScale, 1.0f, 1.0f);
				AEMtx33Rot(&btnRotate, 0);
				AEMtx33Trans(&btnTranslate, xPos, yPos);  // Use xPos here
				AEMtx33Concat(&btnTransform, &btnRotate, &btnScale);
				AEMtx33Concat(&btnTransform, &btnTranslate, &btnTransform);
				AEGfxSetTransform(btnTransform.m);
				AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				};

			// Draw PLAY button
			drawButton(buttonX, playButtonY, playButtonHovered, playButtonHovered ? buttonHoverMesh : buttonMesh);

			// Draw EXIT button
			drawButton(buttonX, exitButtonY, exitButtonHovered, exitButtonHovered ? buttonHoverMesh : buttonMesh);

			// Draw CREDITS button
			drawButton(buttonX, creditsButtonY, creditsButtonHovered, creditsButtonHovered ? buttonHoverMesh : buttonMesh);

			// Draw vibrating rock 1 - Bottom right
			float rock1VibrateX = sinf(rockVibrateTime) * rockVibrateAmount;
			float rock1VibrateY = cosf(rockVibrateTime * 1.3f) * rockVibrateAmount * 0.5f;

			AEMtx33 rock1Transform = { 0 };
			AEMtx33 rock1Scale = { 0 };
			AEMtx33 rock1Rotate = { 0 };
			AEMtx33 rock1Translate = { 0 };
			AEMtx33Scale(&rock1Scale, 2.5f, 2.5f);
			AEMtx33Rot(&rock1Rotate, sinf(rockVibrateTime * 0.5f) * 0.1f);
			AEMtx33Trans(&rock1Translate, 630.0f + rock1VibrateX, -380.0f + rock1VibrateY);
			AEMtx33Concat(&rock1Transform, &rock1Rotate, &rock1Scale);
			AEMtx33Concat(&rock1Transform, &rock1Translate, &rock1Transform);
			AEGfxSetTransform(rock1Transform.m);
			AEGfxMeshDraw(rockMesh, AE_GFX_MDM_TRIANGLES);

			// Draw vibrating rock 2 - Most Bottom left 
			float rock2VibrateX = sinf(rockVibrateTime * 0.8f) * rockVibrateAmount * 1.2f;
			float rock2VibrateY = cosf(rockVibrateTime * 1.1f) * rockVibrateAmount * 0.6f;

			AEMtx33 rock2Transform = { 0 };
			AEMtx33 rock2Scale = { 0 };
			AEMtx33 rock2Rotate = { 0 };
			AEMtx33 rock2Translate = { 0 };
			AEMtx33Scale(&rock2Scale, 2.0f, 2.0f);
			AEMtx33Rot(&rock2Rotate, sinf(rockVibrateTime * 0.6f) * 0.12f);
			AEMtx33Trans(&rock2Translate, -480.0f + rock2VibrateX, -360.0f + rock2VibrateY);
			AEMtx33Concat(&rock2Transform, &rock2Rotate, &rock2Scale);
			AEMtx33Concat(&rock2Transform, &rock2Translate, &rock2Transform);
			AEGfxSetTransform(rock2Transform.m);
			AEGfxMeshDraw(rock2Mesh, AE_GFX_MDM_TRIANGLES);

			// Draw vibrating rock 4 - Bottom Middle left
			float rock4VibrateX = sinf(rockVibrateTime * 1.5f) * rockVibrateAmount * 1.1f;
			float rock4VibrateY = cosf(rockVibrateTime * 1.4f) * rockVibrateAmount * 0.4f;

			AEMtx33 rock4Transform = { 0 };
			AEMtx33 rock4Scale = { 0 };
			AEMtx33 rock4Rotate = { 0 };
			AEMtx33 rock4Translate = { 0 };
			AEMtx33Scale(&rock4Scale, 2.2f, 2.2f);
			AEMtx33Rot(&rock4Rotate, sinf(rockVibrateTime * 0.4f) * 0.09f);
			AEMtx33Trans(&rock4Translate, -450.0f + rock4VibrateX, -370.0f + rock4VibrateY);
			AEMtx33Concat(&rock4Transform, &rock4Rotate, &rock4Scale);
			AEMtx33Concat(&rock4Transform, &rock4Translate, &rock4Transform);
			AEGfxSetTransform(rock4Transform.m);
			AEGfxMeshDraw(rockMesh, AE_GFX_MDM_TRIANGLES);


			// Draw vibrating rock 3 - Bottom Left last
			float rock3VibrateX = sinf(rockVibrateTime * 1.2f) * rockVibrateAmount * 0.8f;
			float rock3VibrateY = cosf(rockVibrateTime * 0.9f) * rockVibrateAmount * 0.7f;

			AEMtx33 rock3Transform = { 0 };
			AEMtx33 rock3Scale = { 0 };
			AEMtx33 rock3Rotate = { 0 };
			AEMtx33 rock3Translate = { 0 };
			AEMtx33Scale(&rock3Scale, 1.8f, 1.8f);
			AEMtx33Rot(&rock3Rotate, sinf(rockVibrateTime * 0.7f) * 0.08f);
			AEMtx33Trans(&rock3Translate, -415.0f + rock3VibrateX, -390.0f + rock3VibrateY);
			AEMtx33Concat(&rock3Transform, &rock3Rotate, &rock3Scale);
			AEMtx33Concat(&rock3Transform, &rock3Translate, &rock3Transform);
			AEGfxSetTransform(rock3Transform.m);
			AEGfxMeshDraw(rock3Mesh, AE_GFX_MDM_TRIANGLES);

			// Draw vibrating rock 5 - Bottom center-right
			float rock5VibrateX = sinf(rockVibrateTime * 0.9f) * rockVibrateAmount * 0.9f;
			float rock5VibrateY = cosf(rockVibrateTime * 1.6f) * rockVibrateAmount * 0.55f;

			AEMtx33 rock5Transform = { 0 };
			AEMtx33 rock5Scale = { 0 };
			AEMtx33 rock5Rotate = { 0 };
			AEMtx33 rock5Translate = { 0 };
			AEMtx33Scale(&rock5Scale, 1.6f, 1.6f);
			AEMtx33Rot(&rock5Rotate, sinf(rockVibrateTime * 0.8f) * 0.11f);
			AEMtx33Trans(&rock5Translate, 570.0f + rock5VibrateX, -385.0f + rock5VibrateY);
			AEMtx33Concat(&rock5Transform, &rock5Rotate, &rock5Scale);
			AEMtx33Concat(&rock5Transform, &rock5Translate, &rock5Transform);
			AEGfxSetTransform(rock5Transform.m);
			AEGfxMeshDraw(rock2Mesh, AE_GFX_MDM_TRIANGLES);

			// Draw vibrating rock 9 - Bottom right
			float rock9VibrateX = sinf(rockVibrateTime * 1.2f) * rockVibrateAmount * 0.8f;
			float rock9VibrateY = cosf(rockVibrateTime * 0.9f) * rockVibrateAmount * 0.7f;

			AEMtx33 rock9Transform = { 0 };
			AEMtx33 rock9Scale = { 0 };
			AEMtx33 rock9Rotate = { 0 };
			AEMtx33 rock9Translate = { 0 };
			AEMtx33Scale(&rock9Scale, 1.8f, 1.8f);
			AEMtx33Rot(&rock9Rotate, sinf(rockVibrateTime * 0.7f) * 0.08f);
			AEMtx33Trans(&rock9Translate, 590.0f + rock9VibrateX, -390.0f + rock9VibrateY);
			AEMtx33Concat(&rock9Transform, &rock9Rotate, &rock9Scale);
			AEMtx33Concat(&rock9Transform, &rock9Translate, &rock9Transform);
			AEGfxSetTransform(rock9Transform.m);
			AEGfxMeshDraw(rock3Mesh, AE_GFX_MDM_TRIANGLES);
			// Draw vibrating rock 6 - Bottom Middle right
			float rock6VibrateX = sinf(rockVibrateTime * 1.5f) * rockVibrateAmount * 1.1f;
			float rock6VibrateY = cosf(rockVibrateTime * 1.4f) * rockVibrateAmount * 0.4f;

			AEMtx33 rock6Transform = { 0 };
			AEMtx33 rock6Scale = { 0 };
			AEMtx33 rock6Rotate = { 0 };
			AEMtx33 rock6Translate = { 0 };
			AEMtx33Scale(&rock6Scale, 2.2f, 2.2f);
			AEMtx33Rot(&rock6Rotate, sinf(rockVibrateTime * 0.4f) * 0.09f);
			AEMtx33Trans(&rock6Translate, 350.0f + rock6VibrateX, -370.0f + rock6VibrateY);
			AEMtx33Concat(&rock6Transform, &rock6Rotate, &rock6Scale);
			AEMtx33Concat(&rock6Transform, &rock6Translate, &rock6Transform);
			AEGfxSetTransform(rock6Transform.m);
			AEGfxMeshDraw(rockMesh, AE_GFX_MDM_TRIANGLES);

			// Draw vibrating rock 7 - Bottom center-right
			float rock7VibrateX = sinf(rockVibrateTime * 0.9f) * rockVibrateAmount * 0.9f;
			float rock7VibrateY = cosf(rockVibrateTime * 1.6f) * rockVibrateAmount * 0.55f;

			AEMtx33 rock7Transform = { 0 };
			AEMtx33 rock7Scale = { 0 };
			AEMtx33 rock7Rotate = { 0 };
			AEMtx33 rock7Translate = { 0 };
			AEMtx33Scale(&rock7Scale, 1.6f, 1.6f);
			AEMtx33Rot(&rock7Rotate, sinf(rockVibrateTime * 0.8f) * 0.11f);
			AEMtx33Trans(&rock7Translate, 410.0f + rock7VibrateX, -385.0f + rock7VibrateY);
			AEMtx33Concat(&rock7Transform, &rock7Rotate, &rock7Scale);
			AEMtx33Concat(&rock7Transform, &rock7Translate, &rock7Transform);
			AEGfxSetTransform(rock7Transform.m);
			AEGfxMeshDraw(rock2Mesh, AE_GFX_MDM_TRIANGLES);

			// Draw vibrating rock 8 - Bottom center-right
			float rock8VibrateX = sinf(rockVibrateTime * 1.2f) * rockVibrateAmount * 0.8f;
			float rock8VibrateY = cosf(rockVibrateTime * 0.9f) * rockVibrateAmount * 0.7f;

			AEMtx33 rock8Transform = { 0 };
			AEMtx33 rock8Scale = { 0 };
			AEMtx33 rock8Rotate = { 0 };
			AEMtx33 rock8Translate = { 0 };
			AEMtx33Scale(&rock8Scale, 1.8f, 1.8f);
			AEMtx33Rot(&rock8Rotate, sinf(rockVibrateTime * 0.7f) * 0.08f);
			AEMtx33Trans(&rock8Translate, 380.0f + rock8VibrateX, -390.0f + rock8VibrateY);
			AEMtx33Concat(&rock8Transform, &rock8Rotate, &rock8Scale);
			AEMtx33Concat(&rock8Transform, &rock8Translate, &rock8Transform);
			AEGfxSetTransform(rock8Transform.m);
			AEGfxMeshDraw(rock3Mesh, AE_GFX_MDM_TRIANGLES);

			// Print text
			char strBuffer[1024];
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);

			// Title "CORE BREAK"
			sprintf_s(strBuffer, "CORE BREAK");
			AEGfxPrint(fontId, strBuffer, -0.45f, 0.45f, 2.8f, 0.0f, 0.0f, 0.0f, 1.0f);

			// PLAY button text - scale + color animation
			sprintf_s(strBuffer, "PLAY");
			if (playButtonHovered)
			{
				AEGfxPrint(fontId, strBuffer, -0.16f, 0.00f, 1.9f, 1.0f, 0.84f, 0.0f, 1.0f); // Bigger + yellow
			}
			else
			{
				AEGfxPrint(fontId, strBuffer, -0.15f, 0.00f, 1.6f, 0.0f, 0.0f, 0.0f, 1.0f); // Normal
			}

			// EXIT button text - scale + color animation
			sprintf_s(strBuffer, "EXIT");
			if (exitButtonHovered)
			{
				AEGfxPrint(fontId, strBuffer, -0.14f, -0.4f, 1.9f, 1.0f, 0.0f, 0.0f, 1.0f); // Bigger + red
			}
			else
			{
				AEGfxPrint(fontId, strBuffer, -0.15f, -0.4f, 1.6f, 0.0f, 0.0f, 0.0f, 1.0f); // Normal
			}

			// CREDITS button text - scale + color animation
			sprintf_s(strBuffer, "CREDITS");
			if (creditsButtonHovered)
			{
				AEGfxPrint(fontId, strBuffer, -0.25f, -0.77f, 1.9f, 0.0f, 0.6f, 1.0f, 1.0f); // Bigger + cyan
			}
			else
			{
				AEGfxPrint(fontId, strBuffer, -0.22f, -0.77f, 1.6f, 0.0f, 0.0f, 0.0f, 1.0f); // Normal
			}
		}
		// ============== STORY STATE ==============
		else if (currentState == GS_STORY)
		{
			int result = StoryScreen_Update();
			StoryScreen_Draw();

			if (result == STORY_RESULT_DONE)
			{
				Game_Init();                    // load all gameplay resources
				currentState = GS_GAMEPLAY;
			}
		}
		// ============== GAMEPLAY STATE ==============
		else if (currentState == GS_GAMEPLAY)
		{
			// ESC → free gameplay resources, only go back to menu when shop not active
			if (!shop_is_active && AEInputCheckTriggered(AEVK_ESCAPE))
			{
				Game_Kill();                        // free all gameplay textures/meshes
				currentState = GS_MAINMENU;
			}
			else if (pause_is_active)
			{
				int result = PauseMenu_Update();
				Game_Draw();
				PauseMenu_Draw();
				if (result == PAUSE_RESULT_RESUME)
					pause_is_active = 0;
				else if (result == PAUSE_RESULT_QUIT)
				{
					pause_is_active = 0;
					Game_Kill();
					currentState = GS_MAINMENU;
				}
			}
			else
			{
				if (AEInputCheckTriggered(AEVK_Q))
					pause_is_active = 1;
				else
				{

					Game_Update();                      // physics, mining, camera, oxygen, shop …
					Game_Draw();                        // render the whole mining world
				}
			}
		}
		// ============== CREDITS STATE ==============
		else if (currentState == GS_CREDITS)
		{
			// Return to menu
			if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON))
			{
				currentState = GS_MAINMENU;
			}

			AEGfxSetBackgroundColor(0.73f, 0.66f, 0.59f);

			char strBuffer[1024];
			sprintf_s(strBuffer, "CREDITS");
			AEGfxPrint(fontId, strBuffer, -0.18f, 0.7f, 2.5f, 0.0f, 0.0f, 0.0f, 1.0f);

			sprintf_s(strBuffer, "Game by: Your Name");
			AEGfxPrint(fontId, strBuffer, -0.25f, 0.3f, 1.5f, 0.0f, 0.0f, 0.0f, 1.0f);

			sprintf_s(strBuffer, "Press ESC or Click to return");
			AEGfxPrint(fontId, strBuffer, -0.35f, -0.5f, 1.2f, 0.0f, 0.0f, 0.0f, 1.0f);
		}

		// Basic way to trigger exiting the application
		if (0 == AESysDoesWindowExist())
			gGameRunning = 0;

		// Informing the system about the loop's end
		AESysFrameEnd();
	}

	// Free the system
	StoryScreen_Unload();
	AEGfxDestroyFont(fontId);
	AEGfxMeshFree(buttonMesh);
	AEGfxMeshFree(buttonHoverMesh);
	AEGfxMeshFree(rockMesh);
	AEGfxMeshFree(rock2Mesh);
	AEGfxMeshFree(rock3Mesh);
	AEGfxMeshFree(backgroundMesh);
	if (backgroundTexture != nullptr)
	{
		AEGfxTextureUnload(backgroundTexture);
	}
	AESysExit();

	return 0;
}