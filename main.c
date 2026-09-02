#include "stdio.h"
#include "raylib.h"
// #include <stdlib.h> // not on the heap
#include <math.h> // cosine and sine

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 800
#define MAX_BULLETS 3000
#define PLAYER_SPEED_FAST 40.0f
#define PLAYER_SPEED_SLOW 10.0f

typedef struct {
	// bullet game state
	Vector2 playerPos;
	
	int bulletId[MAX_BULLETS];
	
	int activeBulletCount;
	int activeBulletId[MAX_BULLETS];
	Vector2 bulletPos[MAX_BULLETS];
	Vector2 bulletVel[MAX_BULLETS];
	// Vector2 bulletAcc[MAX_BULLETS];
	float bulletRad[MAX_BULLETS];
	
	int inactiveBulletCount;
	int inactiveBulletId[MAX_BULLETS]; // acts as stack
	
	// bullet visuals
	// int bulletTex; // so far, only a single texture
	Color bulletColor[MAX_BULLETS];
	
} GameData;

void addBullet(GameData* gameData, Vector2 bulletSpawnPos, Color bulletColor, Vector2 bulletInitVel)
{
	// remove from inactive stack
	gameData->inactiveBulletCount -= 1;
	int ind = gameData->inactiveBulletId[gameData->inactiveBulletCount];
	
	// add to activeBullets
	gameData->activeBulletId[gameData->activeBulletCount] = ind;
	gameData->activeBulletCount += 1;

	// add the fields
	gameData->bulletPos[ind] = bulletSpawnPos;
	gameData->bulletVel[ind] = bulletInitVel;
	gameData->bulletRad[ind] = 5.0f;
	gameData->bulletColor[ind] = bulletColor;
}

void removeBullet(GameData* gameData, int ind)
{
	int count = 0;
	for (size_t i = 0; i < gameData->activeBulletCount; ++i)
	{
		if (gameData->activeBulletId[i] != ind)
		{
			gameData->activeBulletId[count] = gameData->activeBulletId[i];
			count += 1;
		}
	}
	gameData->activeBulletCount = count;
	
	
	gameData->inactiveBulletId[gameData->inactiveBulletCount] = ind;
	gameData->inactiveBulletCount += 1;
	return;
}

bool bulletOutOfBounds(GameData* gameData, int ind)
{
	return (gameData->bulletPos[ind].x < -gameData->bulletRad[ind]*2) ||
		(gameData->bulletPos[ind].x > SCREEN_WIDTH + gameData->bulletRad[ind]*2) ||
		(gameData->bulletPos[ind].y < -gameData->bulletRad[ind]*2) ||
		(gameData->bulletPos[ind].y > SCREEN_HEIGHT + gameData->bulletRad[ind]*2);
}

void updateBulletPos(GameData* gameData)
{
	int i = 0;
	while (i < gameData->activeBulletCount)
	{
		int ind = gameData->activeBulletId[i];
		
		gameData->bulletPos[ind].x += gameData->bulletVel[ind].x;
		gameData->bulletPos[ind].y += gameData->bulletVel[ind].y;
		
		if (bulletOutOfBounds(gameData, ind))
		{
			removeBullet(gameData, ind);
		}
		else
		{
			++i;
		}
	}
}

void InitGameData(GameData* gameData)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		gameData->bulletId[i] = i;
		
		gameData->inactiveBulletId[i] = MAX_BULLETS - i - 1;
	}
	gameData->activeBulletCount = 0;
	gameData->inactiveBulletCount = MAX_BULLETS;
	
	return;
}

void drawBullet(GameData gameData)
{
	for (int i = 0; i < gameData.activeBulletCount; ++i)
	{
		int ind = gameData.activeBulletId[i];
		
		DrawCircleV(gameData.bulletPos[ind], (float)gameData.bulletRad[ind], gameData.bulletColor[ind]);
		DrawCircleLinesV(gameData.bulletPos[ind], (float)gameData.bulletRad[ind], BLACK);
	}
	return;
}

int main(void)
{
	const char* title = "Test SHMUP";
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title); // vertical shooting IG
	SetTargetFPS(60);
	static GameData gameData = {0};
	
	InitGameData(&gameData);
	
	// temp for testing
	// enemy (abstract circle)
	Vector2 enemyPosition = {
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*1/3,
	};
	Vector2 enemyPosition2 = {
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*1/2,
	};
	// float enemyRotation = 0.0f;
	
	// test pattern
	float baseAngle = 0;
	int angleIncrement = 11;
	int bulletRows = 6;
	float spawnCooldown = 2;
	float spawnCooldownTimer = spawnCooldown;
	float bulletSpeed = 3.0f;
	bool speed_mod = false;
	float switchCooldown = 10;
	float switchCooldownTimer = switchCooldown;
	
	// enemy movement test
	float test = 1;
	
	
	while (!WindowShouldClose())
	{
		
		// pattern
		spawnCooldownTimer--;
		switchCooldownTimer--;
		
		if (spawnCooldownTimer < 0)
		{
			spawnCooldownTimer = spawnCooldown;
			
			float degreesPerRow = 360.0f / bulletRows;
			for (int row = 0; row < bulletRows; ++row)
			{
				if (gameData.activeBulletCount < MAX_BULLETS)
				{
					Vector2 bulletSpawnPos = enemyPosition;
					Vector2 bulletSpawnPos2 = enemyPosition2;
					Color bulletColor = (row%2 == 0) ? RED : BLUE;
					
					float bulletInitDirection = baseAngle + (row * degreesPerRow);
			
					if (switchCooldownTimer < 0)
					{
						switchCooldownTimer = switchCooldown;
						speed_mod = !speed_mod;
					}
					
					Vector2 bulletInitVel = {
						.x = (bulletSpeed - (1.5f * speed_mod)) * cosf(bulletInitDirection * DEG2RAD),
						.y = (bulletSpeed - (1.5f * speed_mod)) * sinf(bulletInitDirection * DEG2RAD),
					};
					
					addBullet(&gameData, bulletSpawnPos, bulletColor, bulletInitVel);
					addBullet(&gameData, bulletSpawnPos2, bulletColor, bulletInitVel);
				}
			}
			
			baseAngle += angleIncrement;
		}
		
		updateBulletPos(&gameData);
		
		enemyPosition.x += -1 * cosf(test++ * DEG2RAD);
		
		// >>>>>>>>>>>>>>>>>> DRAWING <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		BeginDrawing();
		ClearBackground(GetColor(0x181818FF));
		
		drawBullet(gameData);
		
		EndDrawing();
	}
	
	CloseWindow();
	return(0);
}