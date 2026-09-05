#include "stdio.h"
#include "raylib.h"
// #include <stdlib.h> // not on the heap
#include <math.h> // cosine and sine
#include <stdint.h> // uint64_t



// balance
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 800
#define MAX_BULLETS 3000
#define MAX_ENEMIES 75
#define PLAYER_SPEED_FAST 40.0f
#define PLAYER_SPEED_SLOW 10.0f

// might not use, using 64-bit bitmask
#define MAX_PATTERNS 64

typedef struct GameData GameData;

// uses seconds
typedef struct {
	double startTime;
	double lifeTime;
} Timer;

struct GameData {
	// PLAYER
	Vector2 playerPos;
	float playerRad;
	
	// main timer
	Timer timeSinceStart;
	
	
	// BULLETS
	int bulletId[MAX_BULLETS];
	
	int activeBulletCount;
	int activeBulletId[MAX_BULLETS];
	Vector2 bulletPos[MAX_BULLETS];
	Vector2 bulletVel[MAX_BULLETS];
	// Vector2 bulletAcc[MAX_BULLETS];
	float bulletRad[MAX_BULLETS];
	uint64_t bulletPatternBitfield[MAX_BULLETS];
	Timer bulletTimer[MAX_BULLETS];
	
	// // not sure if this is the better way to do it rather than bulletFromSubPattern, given maybe it will have a decent amount less
	// int bulletWithCallbackCount;
	// int bulletWithCallbackId[MAX_BULLETS];
	
	// bullet Anim
	// int bulletTex; // so far, only a single texture
	Color bulletColor[MAX_BULLETS];
	
	int inactiveBulletCount;
	int inactiveBulletId[MAX_BULLETS]; // acts as stack
	
	
	// ENEMIES
	int enemyId[MAX_ENEMIES];
	
	
	int activeEnemyCount;
	int activeEnemyId[MAX_ENEMIES];
	Vector2 enemyPos[MAX_ENEMIES];
	uint64_t enemyPatternBitfield[MAX_ENEMIES];
	
	// enemy Anim
	// int enemyTex[MAX_ENEMIES];
	
	int inactiveEnemyCount;
	int inactiveEnemyId[MAX_ENEMIES]; // acts as stack
	
	
	
	// PATTERN
	int patternId[MAX_PATTERNS];
	
	int activePatternCount;
	int activePatternId[MAX_PATTERNS];
	void (*patternCallback[MAX_PATTERNS])(GameData*, int);
};

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> HELPER STRUCT FUNCTIONS <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/

void StartTimer(Timer* timer, double lifetime)
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool TimerDone(Timer timer)
{
	return GetTime() - timer.startTime >= timer.lifeTime;
}

float GetElapsed(Timer timer)
{
	return GetTime() - timer.startTime;
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> GAMEDATA INTERACTION (LOGIC) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/


void addBullet(GameData* gameData, Vector2 bulletSpawnPos, Color bulletColor, Vector2 bulletInitVel, uint64_t bulletPatternBitfield)
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
	gameData->bulletPatternBitfield[ind] = bulletPatternBitfield;
	
	Timer timer;
	StartTimer(&timer, 20000);
	gameData->bulletTimer[ind] = timer;
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
	
	gameData->playerPos = (Vector2){
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*2/3,
	};
	gameData->playerRad = 10.0f;
	
	Timer timer;
	StartTimer(&timer, 10000);
	gameData->timeSinceStart = timer;
	
	return;
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DRAWING OBJECTS (BOARD) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/

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

void drawPlayer(GameData gameData)
{
	DrawCircleV(gameData.playerPos, (float)gameData.playerRad, WHITE);
	DrawCircleLinesV(gameData.playerPos, (float)gameData.playerRad, BLACK);
}


/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MAIN LOOP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/



/*
TEST CALLBACK FUNCTIONS
*/

// pattern 1
void pattern_1(GameData* gameData, int bulletId)
{
	float elapsed = GetElapsed(gameData->bulletTimer[bulletId]);
	int seconds = (int)elapsed;
	
	// printf("%d: %f\n", bulletId, GetElapsed(gameData->bulletTimer[bulletId]));
	if ((seconds % 4 == 1) && (elapsed - seconds < 0.0166f))
	{
		Vector2 curVel = {
			.x = cosf(30 * DEG2RAD) * gameData->bulletVel[bulletId].x - sinf(30 * DEG2RAD) * gameData->bulletVel[bulletId].y,
			.y = sinf(30 * DEG2RAD) * gameData->bulletVel[bulletId].x + cosf(30 * DEG2RAD) * gameData->bulletVel[bulletId].y,
		};
		gameData->bulletVel[bulletId] = curVel;
	}
}


int main(void)
{
	const char* title = "Test SHMUP";
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title); // vertical shooting IG
	SetTargetFPS(60);
	static GameData gameData = {0};
	
	// test adding 1 test callback, only blue bullets should call this
	gameData.activePatternCount = 1;
	gameData.patternId[0] = 0;
	gameData.activePatternId[0] = 0;
	gameData.patternCallback[0] = pattern_1;
	
	
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
	float changeAngleTimer = 180;
	
	// enemy movement test
	float test = 1;
	
	
	while (!WindowShouldClose())
	{
		
		// handle input
//		handleInput(gameData);
		// i strangely like touhou's control where right and down are preferred when the opposite is pressed at same time (rather than do nothing)
		bool slowdown;
		if(IsKeyDown(KEY_LEFT_SHIFT)) slowdown = true;
		else slowdown = false;
		if(IsKeyDown(KEY_RIGHT)) gameData.playerPos.x += 3.5f - (1.5f * slowdown);
		else if(IsKeyDown(KEY_LEFT)) gameData.playerPos.x -= 3.5f - (1.5f * slowdown);
		if(IsKeyDown(KEY_DOWN)) gameData.playerPos.y += 3.5f - (1.5f * slowdown);
		else if(IsKeyDown(KEY_UP)) gameData.playerPos.y -= 3.5f - (1.5f * slowdown);
		
		// pattern
		spawnCooldownTimer--;
		switchCooldownTimer--;
		changeAngleTimer--;
		
//		if (GetElapsed(gameData->timeSinceStart) % start  )
//		{
//		}
		
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
					
					addBullet(&gameData, bulletSpawnPos, RED, bulletInitVel, 1);
					addBullet(&gameData, bulletSpawnPos2, BLUE, bulletInitVel, 1);
					
				}
			}
			
			baseAngle += angleIncrement;
		}
		
//		if (changeAngleTimer < 0)
//		{
//			changeAngleTimer = 10000;
//			for (int i = 0; i < gameData.activeBulletCount; i++)
//			{
//				int ind = gameData.activeBulletId[i];
//
//				// printf("%d\n", gameData.bulletFromSubPattern[ind]);
//				if (gameData.bulletFromSubPattern[ind] == 0) 
//				{
//					//printf("sus\n");
//					continue;
//				}
//				
//				Vector2 curVel = {
//					.x = cosf(30 * DEG2RAD) * gameData.bulletVel[ind].x - sinf(30 * DEG2RAD) * gameData.bulletVel[ind].y,
//					.y = sinf(30 * DEG2RAD) * gameData.bulletVel[ind].x + cosf(30 * DEG2RAD) * gameData.bulletVel[ind].y,
//				};
//				
//				gameData.bulletVel[ind] = curVel;
//
//			}
//		}
		
//		for (int i = 0; i < gameData.activeBulletCount; i++)
//		{
//			int ind = gameData.activeBulletId[i];
//			if (gameData.bulletFromSubPattern[ind] == 1)
//				pattern_1(&gameData, ind);
//		}

		for (int i = 0; i < gameData.activeBulletCount; ++i)
		{
			int ind = gameData.activeBulletId[i];
			uint64_t patternBitfield = gameData.bulletPatternBitfield[ind];
			
			// maybe optimization, idk
			if (!patternBitfield) continue;
			
			for (int j = 0; j < gameData.activePatternCount; j++)
			{
				int call_ind = gameData.activePatternId[j];
				
				if ((patternBitfield >> call_ind) & 1)
				{
					gameData.patternCallback[call_ind](&gameData, ind);
				}
			}
		}

		
		
		updateBulletPos(&gameData);
		
		// enemyPosition.x += -1 * cosf(test++ * DEG2RAD);
		// updateEnemyPos(&gameData);
		
		// >>>>>>>>>>>>>>>>>> DRAWING <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		BeginDrawing();
		ClearBackground(GetColor(0x202020FF));
		
		drawPlayer(gameData);
		drawBullet(gameData);
		
		EndDrawing();
	}
	
	CloseWindow();
	return(0);
}