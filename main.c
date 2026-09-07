#include "stdio.h"
#include "raylib.h"
// #include <stdlib.h> // not on the heap, also already included by "not_vector.h"
#include <math.h> // cosine and sine
#include <stdint.h> // uint64_t
#include "not_vector.h"



// balance
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 800
#define MAX_BULLETS 3000
#define MAX_ENEMIES 75
#define PLAYER_SPEED_FAST 40.0f
#define PLAYER_SPEED_SLOW 10.0f

// might not use, using 64-bit bitmask
#define MAX_PATTERNS 64
#define MAX_PATTERN_TIMERS 256
#define MAX_BULLET_TIMERS 12000

typedef struct GameData GameData;

typedef struct {
	float* items;
	size_t count;
	size_t capacity;
} FloatArr;

typedef struct {
	void (**items)(GameData*, int); // in the parenthesis, cause outside means it's void* is part of return type of function signature
	size_t count;
	size_t capacity;
} CallbackArr;

// uses seconds
typedef struct {
	double startTime;
	double lifeTime;
} Timer;

// typedef enum {
	// PATTERN_TIMER,
	// BULLET_TIMER,
// } TimerType;

typedef struct {
	short PatternId;
	
	float baseAngle;
	int angleIncrement;
	int bulletRows;
	float bulletSpeed;
	// float patternTimerCooldown[3];
	FloatArr patternTimerCooldown;
	// void (*patternCallback[3])(GameData*, int);
	CallbackArr patternCallback;
	// float bulletTimerCooldown[3];
	FloatArr bulletTimerCooldown;
	// void (*bulletCallback[3])(GameData*, int);
	CallbackArr bulletCallback;
	
	// Timer timeSincePatternStart;
} Pattern;


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
	uint64_t bulletPatternBitfield[MAX_BULLETS]; // acts as 2d array of booleans, might only need one pattern ngl, seems 1:N
	// Timer bulletTimer[MAX_BULLETS];
	
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
	
	Pattern patterns[MAX_PATTERNS];
	// uint64_t patternTimerBitfield[MAX_PATTERNS]; // acts as 2d array of booleans
	// void (*patternCallback[MAX_PATTERNS])(GameData*, int);
	
	int inactivePatternCount;
	int inactivePatternId[MAX_PATTERNS]; // acts as stack
	
	
	
	
	// PATTERN TIMER: specifically for patterns
	int patternTimerId[MAX_PATTERN_TIMERS];
	
	int activePatternTimerCount;
	int activePatternTimerId[MAX_PATTERN_TIMERS];
	
	int patternTimerPatternRef[MAX_PATTERN_TIMERS];
	Timer patternTimers[MAX_PATTERN_TIMERS];
	void (*patternTimerCallback[MAX_PATTERN_TIMERS])(GameData*, int);
	
	// // probably won't need to differentiate pending vs active, but maybe will. just comment
	// int pendingPatternTimerCount;
	// int pendingPatternTimerId[MAX_PATTERN_TIMERS];
	
	int inactivePatternTimerCount;
	int inactivePatternTimerId[MAX_PATTERN_TIMERS]; // acts as stack
	
	
	
	
	
	// BULLET TIMER: specifically for bullets
	int bulletTimerId[MAX_BULLET_TIMERS];
	
	int activeBulletTimerCount;
	int activeBulletTimerId[MAX_BULLET_TIMERS];
	
	int bulletTimerBulletRef[MAX_BULLET_TIMERS];
	Timer bulletTimers[MAX_BULLET_TIMERS];
	void (*bulletTimerCallback[MAX_BULLET_TIMERS])(GameData*, int);
	
	// // probably won't need to differentiate pending vs active, but maybe will. maybe for things like custom state transitions...? might have to do void pointers...
	// int pendingBulletTimerCount;
	// int pendingBulletTimerId[MAX_BULLET_TIMERS];
	
	int inactiveBulletTimerCount;
	int inactiveBulletTimerId[MAX_BULLET_TIMERS]; // acts as stack
	
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

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Add and delete within tables



void addBulletTimer(GameData* gameData, int bulletId, void (*bulletCallback)(GameData*, int), float cooldown)
{
	// remove from inactive stack
	gameData->inactiveBulletTimerCount -= 1;
	int ind = gameData->inactiveBulletTimerId[gameData->inactiveBulletTimerCount];
	
	// add to active bulletTimers
	gameData->activeBulletTimerId[gameData->activeBulletTimerCount] = ind;
	gameData->activeBulletTimerCount += 1;
	
	// add the fields
	gameData->bulletTimerBulletRef[ind] = bulletId;
	gameData->bulletTimerCallback[ind] = bulletCallback;
	
	Timer timer;
	StartTimer(&timer, cooldown);
	gameData->bulletTimers[ind] = timer;
	return;
}

void removeBulletTimer(GameData* gameData, int ind)
{
	int count = 0;
	for(int i = 0; i < gameData->activeBulletTimerCount; ++i)
	{
		if (gameData->activeBulletTimerId[i] != ind)
		{
			gameData->activeBulletTimerId[count] = gameData->activeBulletTimerId[i];
			count += 1;
		}
	}
	gameData->activeBulletTimerCount = count;
	
	gameData->inactiveBulletTimerId[gameData->inactiveBulletTimerCount] = ind;
	gameData->inactiveBulletTimerCount += 1;
	return;
}

void addPatternTimer(GameData* gameData, int patternId, void (*patternCallback)(GameData*, int), float cooldown)
{
	// remove for inactive stack
	gameData->inactivePatternTimerCount -= 1;
	int ind = gameData->inactivePatternTimerId[gameData->inactivePatternTimerCount];
	
	// add to activePattern
	gameData->activePatternTimerId[gameData->activePatternTimerCount] = ind;
	gameData->activePatternTimerCount += 1;
	
	// add to fields
	gameData->patternTimerPatternRef[ind] = patternId;
	gameData->patternTimerCallback[ind] = patternCallback;
	
	Timer timer;
	StartTimer(&timer, cooldown);
	gameData->patternTimers[ind] = timer;
	
	
	printf("Note(kt): TEST addPatterTimer\n");
	return;
}

void removePatternTimer(GameData* gameData, int ind)
{
	int count = 0;
	for(int i = 0; i < gameData->activePatternTimerCount; ++i)
	{
		if (gameData->activePatternTimerId[i] != ind)
		{
			gameData->activePatternTimerId[count] = gameData->activePatternTimerId[i];
			count += 1;
		}
	}
	gameData->activePatternTimerCount = count;
	
	gameData->inactivePatternTimerId[gameData->inactivePatternTimerCount] = ind;
	gameData->inactivePatternTimerCount += 1;
	printf("Note(kt): TEST removePatternTimer\n");
	return;
}

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
	
	// register the timers given by the pattern
	// find the pattern
	for (int i = 0; i < gameData->activePatternCount; ++i)
	{
		int p_ind = gameData->activePatternId[i];
		
		// pattern match -> get the bulletCallbacks within the pattern
		if ((bulletPatternBitfield >> p_ind) & 1)
		{
			Pattern pattern = gameData->patterns[p_ind];
			for (int j = 0; j < pattern.bulletCallback.count; ++j) // pattern.bulletCallback array size
			{
				if (gameData->bulletTimerCallback[j] != NULL) addBulletTimer(gameData, ind, pattern.bulletCallback.items[j], pattern.bulletTimerCooldown.items[j]);
			}
		}
	}
}

void removeBullet(GameData* gameData, int ind)
{
	// remove the timer first
	int j = 0;
	while (j < gameData->activeBulletTimerCount)
	{
		int timerId = gameData->activeBulletTimerId[j];
		
		if (gameData->bulletTimerBulletRef[timerId] == ind)
		{
			removeBulletTimer(gameData, timerId);
		}
		else
		{
			++j;
		}
	}
	
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

void addPattern(GameData* gameData, Pattern pattern)
{
	// remove from inactive stack
	gameData->inactivePatternCount -= 1;
	int ind = gameData->inactivePatternId[gameData->inactivePatternCount];
	
	// insert into activePattern
	gameData->activePatternId[gameData->activePatternCount] = ind;
	gameData->activePatternCount += 1;
	
	// add the Fields
	gameData->patterns[ind] = pattern;
	
	// create the timers
	for (int i = 0; i < pattern.patternTimerCooldown.count; ++i)
	{
		addPatternTimer(gameData, ind, pattern.patternCallback.items[i], pattern.patternTimerCooldown.items[i]);
	}
	printf("Note(kt): TEST addPattern\n");
	return;
}

void removePattern(GameData* gameData, int ind)
{
	// remove associated timers
	int j = 0;
	while(j < gameData->activePatternTimerCount)
	{
		int timerId = gameData->activePatternTimerId[j];
		if(gameData->patternTimerPatternRef[timerId] == j)
		{
			removePatternTimer(gameData, timerId);
		}
		else
		{
			j++;
		}
	}
	
	int count = 0;
	for (int i = 0; i < gameData->activePatternCount; ++i)
	{
		if (gameData->activePatternId[i] != ind)
		{
			gameData->activePatternId[count] = gameData->activePatternId[i];
			count++;
		}
	}
	gameData->activePatternCount = count;
	
	gameData->inactivePatternId[gameData->inactiveBulletCount] = ind;
	gameData->inactiveBulletCount += 1;
	printf("Note(kt): UNIMPLEMENTED removePattern\n");
	return;
}

void addEnemy(GameData* gameData, Vector2 position, uint64_t enemyPatternBitfield)
{
	// remove from inactiveEnemyId stack
	gameData->inactiveEnemyCount -= 1;
	int ind = gameData->inactiveEnemyId[gameData->inactiveEnemyCount];
	
	// add to activeEnemyId
	gameData->activeEnemyId[gameData->activeBulletCount] = ind;
	gameData->activeEnemyCount += 1;
	
	// add the fields
	gameData->enemyPos[ind] = position;
	gameData->enemyPatternBitfield[ind] = enemyPatternBitfield;
}

void removeEnemy(GameData* gameData, int ind)
{
	int count = 0;
	for (int i = 0; i < gameData->activeEnemyCount; ++i)
	{
		if (gameData->activeEnemyId[i] != count)
		{
			gameData->activeEnemyId[count] = gameData->activeEnemyId[i];
			count += 1;
		}
	}
	
	gameData->activeEnemyCount = count;
	
	gameData->inactiveEnemyId[gameData->inactiveEnemyCount] = ind;
	gameData->inactiveEnemyCount += 1;
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> SPECIFIC BEHAVIORS


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
	
	
	for (int i = 0; i < MAX_BULLET_TIMERS; ++i)
	{
		gameData->bulletTimerId[i] = i;
		
		gameData->inactiveBulletTimerId[i] = MAX_BULLET_TIMERS - i - 1;
	}
	gameData->activeBulletTimerCount = 0;
	gameData->inactiveBulletTimerCount = MAX_BULLET_TIMERS;
	
	
	for (int i = 0; i < MAX_PATTERNS; ++i)
	{
		gameData->patternId[i] = i;
		
		gameData->inactivePatternId[i] = MAX_PATTERNS - i - 1;
	}
	gameData->activePatternCount = 0;
	gameData->inactivePatternCount = MAX_PATTERNS;
	
	
	for (int i = 0; i < MAX_PATTERN_TIMERS; ++i)
	{
		gameData->patternTimerId[i] = i;
		
		gameData->inactivePatternTimerId[i] = MAX_PATTERN_TIMERS - i - 1;
	}
	gameData->activePatternTimerCount = 0;
	gameData->inactivePatternTimerCount = MAX_PATTERN_TIMERS;
	
	
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

// pattern 1 (unused)
void pattern_1_callback(GameData* gameData, int bulletId)
{
	float elapsed = GetElapsed(gameData->bulletTimers[bulletId]);
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

// unused
void pattern_1(GameData* gameData, int bulletId)
{
	pattern_1_callback(gameData, bulletId);
}

void timer_1test_callback(GameData* gameData, int patternTimerId)
{
	Vector2 enemyPosition = {
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*1/4,
	};
	Vector2 enemyPosition2 = {
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*1/2,
	};
	
	float resetLifetime = gameData->patternTimers[patternTimerId].lifeTime;
	Timer timer;
	StartTimer(&timer, resetLifetime);
	gameData->patternTimers[patternTimerId] = timer;
	
	//spawnCooldownTimer = gameData.patterns[0].PatternCooldown[0];
	
	float degreesPerRow = 360.0f / gameData->patterns[0].bulletRows;
	for (int row = 0; row < gameData->patterns[0].bulletRows; ++row)
	{
		if (gameData->activeBulletCount < MAX_BULLETS)
		{
			Vector2 bulletSpawnPos = enemyPosition;
			Vector2 bulletSpawnPos2 = enemyPosition2;
			Color bulletColor = (row%2 == 0) ? RED : BLUE;
			
			float bulletInitDirection = gameData->patterns[0].baseAngle + (row * degreesPerRow);
	
			// if (TimerDone(gameData.timers[1]))
			// {
			// 	float resetLifetime2 = gameData.timers[1].lifeTime;
			// 	Timer timer2;
			// 	StartTimer(&timer2, resetLifetime2);
			// 	gameData.timers[1] = timer2;
			// 	
			// 	speed_mod = !speed_mod;
			// }
			
			Vector2 bulletInitVel = {
				.x = (gameData->patterns[0].bulletSpeed * cosf(bulletInitDirection * DEG2RAD)),
				.y = (gameData->patterns[0].bulletSpeed * sinf(bulletInitDirection * DEG2RAD)),
			};
			
			// printf("bullet Added\n");
			addBullet(gameData, bulletSpawnPos, RED, bulletInitVel, 0);
			addBullet(gameData, bulletSpawnPos2, BLUE, bulletInitVel, 1);
			
		}
	}
	
	gameData->patterns[0].baseAngle += gameData->patterns[0].angleIncrement;
}

void timer_1testb_callback(GameData* gameData, int patternTimerId)
{
	float resetLifetime = gameData->patternTimers[patternTimerId].lifeTime;
	Timer timer;
	StartTimer(&timer, resetLifetime);
	gameData->patternTimers[patternTimerId] = timer;
	
	if (gameData->patterns[0].bulletSpeed == 3.0f)
	{
		gameData->patterns[0].bulletSpeed = 1.5f;
	}
	else
	{
		gameData->patterns[0].bulletSpeed = 3.0f;
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
	gameData.patternTimerCallback[0] = pattern_1_callback;
	
	
	InitGameData(&gameData);
	
	// temp for testing
	// enemy (abstract circle)
	Vector2 enemyPosition = {
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*1/4,
	};
	Vector2 enemyPosition2 = {
		.x = (float)SCREEN_WIDTH/2,
		.y = (float)SCREEN_HEIGHT*1/2,
	};
	// float enemyRotation = 0.0f;
	
	// Init Pattern
	Pattern pattern_test = {0};
	
	pattern_test.baseAngle = 0;
	pattern_test.angleIncrement = 11;
	pattern_test.bulletRows = 6;
	pattern_test.bulletSpeed = 3.0f;
	da_append(pattern_test.patternTimerCooldown, 0.067f);
	da_append(pattern_test.patternCallback, timer_1test_callback);
	
	da_append(pattern_test.patternTimerCooldown, 0.16f);
	da_append(pattern_test.patternCallback, timer_1testb_callback);
	
	// pattern_test.bulletTimerCooldown[2] = 3.0f;
	// pattern_test.bulletCallback[2] = timer_1test_callback;
	
	// Add pattern
	addPattern(&gameData, pattern_test);
	// gameData.patternId[0] = 0;
	// gameData.patterns[0] = pattern_test;
	
	
	// Add patternTimer
	// addPatternTimer(&gameData, 0, gameData.patterns[0].patternCallback.items[0], gameData.patterns[0].patternTimerCooldown.items[0]);
	// addPatternTimer(&gameData, 1, gameData.patterns[0].patternCallback.items[1], gameData.patterns[0].patternTimerCooldown.items[1]);
	
	// gameData.patternTimerId[0] = 0;
	// gameData.activePatternTimerId[gameData.activePatternTimerCount] = 0;
	// Timer timer;
	// StartTimer(&timer, gameData.patterns[0].patternTimerCooldown.items[0]);
	// gameData.patternTimers[gameData.activePatternTimerCount] = timer;
	// gameData.patternTimerCallback[gameData.activePatternTimerCount] = gameData.patterns[0].patternCallback.items[0];
	// gameData.activePatternTimerCount++;
	
	// gameData.patternTimerId[1] = 1;
	// gameData.activePatternTimerId[gameData.activePatternTimerCount] = 1;
	// Timer timer2;
	// StartTimer(&timer2, gameData.patterns[0].patternTimerCooldown.items[1]);
	// gameData.patternTimers[gameData.activePatternTimerCount] = timer2;
	// gameData.patternTimerCallback[gameData.activePatternTimerCount] = gameData.patterns[0].patternCallback.items[1];
	// gameData.activePatternTimerCount++;
	
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
		
		// run all the patternTimer Callbacks
		for (int i = 0; i < gameData.activePatternTimerCount; ++i)
		{
			// printf("test %d\n", i);
		
			int ind = gameData.activePatternTimerId[i];
			if (TimerDone(gameData.patternTimers[ind]))
			{
				// printf("Something here (pattern)...? %d\n", i);
				gameData.patternTimerCallback[ind](&gameData, ind);
				// printf("Probably not...?\n");
			}
		}
		
		// run all the bulletTimer Callbacks
		for (int i = 0; i < gameData.activeBulletTimerCount; ++i)
		{
			int ind = gameData.activeBulletTimerId[i];
			if (TimerDone(gameData.bulletTimers[ind]))
			{
				// printf("Something here (bullet)...? %d\n", i);
				gameData.bulletTimerCallback[ind](&gameData, ind);
				// printf("Probably not...?\n");
			}
		}
		// printf("We outta there...\n");
		
		
		// test += 1.0f;
		
		// if ((int)test % 60 == 0)
			// addBullet(&gameData, enemyPosition, RED, (Vector2){.x = 0, .y = 1,}, 0x0);
		
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