#pragma once

#define GAME_NAME "Battle_City"

// The 16x16 drawing tiles that's why width and height should be multiple of 16.
#define GAME_RES_WIDTH	384
#define GAME_RES_HEIGHT	240 
#define GAME_BPP		32
#define GAME_DRAWING_AREA_MEMORY (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP/8))
#define CALCULATE_AVG_FPS_EVERY_X_FRAMES 120
#define TARGET_MICROSECONDS_PER_FRAME  16667ULL //14980 //16667
#define TIMEOUT 500
#define SIMD
#define TANK_WITHOUT_POWER		0
#define TANK_WITH_ONE_STAR		1
#define TANK_WITH_TWO_STAR		2
#define TANK_WITH_CAP			3
#define TANK_WITH_THREE_STAR	4

#define DIRECTION_DOWN	0
#define DIRECTION_UP	1
#define DIRECTION_LEFT	2
#define DIRECTION_RIGHT	3



#pragma warning(disable: 4820)
#pragma warning(disable: 5045)

static TCHAR	szGameName[] = TEXT("Battle City");

typedef LONG(NTAPI* _NtQueryTimerResolution) (OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution);
_NtQueryTimerResolution NtQueryTimerResolution;

typedef struct GAMEBITMAP {
	BITMAPINFO BitMapInfo;   //44 bytes
	void* Memory;            // 8 bytes
} GAMEBITMAP;

typedef struct PIXEL32 {
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} PIXEL32;

typedef struct GAMEPERFDATA {
	uint64_t TotalFramesRendered;
	float RawFramesPerSecondsAverage;
	float CookedFramesPerSecondAverage;
	int64_t PerfFrequency;
	MONITORINFO MonitorInfo;
	int32_t MonitorWidth;
	int32_t MonitorHeight;
	BOOL DisplayDebugInfo;
	ULONG MinimumTimerResoltuion;
	ULONG MaximumTimerResolution;
	ULONG CurrentTimerResolution;
	DWORD HandleCount;
	PROCESS_MEMORY_COUNTERS_EX MemInfo;
	SYSTEM_INFO SystemInfo;
	// float CPUPercent;
	int64_t CurrentSystemTime;
	int64_t PreviousSystemTime;
	double CPUPercent;
}GAMEPERFDATA;

typedef struct PLAYER
{
	char Name[12];
	GAMEBITMAP Sprite[5][4];
	int32_t ScreenPosX;
	int32_t ScreenPosY;
	uint8_t MovementRemaining;
	uint8_t Direction;
	uint8_t CurrentPower;
	uint8_t SpriteIndex;
	int32_t HP;
	int32_t Strengh;
	int32_t MP;
} PLAYER;

LRESULT CALLBACK WndProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);


DWORD CreateGameWindow(void);
void ProcessPlayerInput(void);
DWORD Load32BppBitmapFromFile(_In_ char *FileName, _Inout_  GAMEBITMAP* GameBitmap);
DWORD InitializeTank(void);
DWORD InitializeEnemyTanks(void);
void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitMap, _In_ uint16_t x, _In_ uint16_t y);
void RenderFrameGraphics(void);
void EnemyTankMovements(void);

#ifdef SIMD
void ClearScreen(_In_ __m128i *Color);
#else
void ClearScreen(_In_ PIXEL32 *Color);
#endif
