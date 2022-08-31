#pragma once

#define GAME_NAME "Battle_City"

// The 16x16 drawing tiles that's why width and height should be multiple of 16.
#define GAME_RES_WIDTH	384
#define GAME_RES_HEIGHT	240 
#define GAME_BPP		32
#define GAME_DRAWING_AREA_MEMORY (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP/8))
#define CALCULATE_AVG_FPS_EVERY_X_FRAMES 100
#define TARGET_MICROSECONDS_PER_FRAME   16667//14980 //16667

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
	LONG MinimumTimerResoltuion;
	LONG MaximumTimerResolution;
	LONG CurrentTimerResolution;
}GAMEPERFDATA;

LRESULT CALLBACK WndProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);

void ProcessPlayerInput(void);
void RenderFrameGraphics(void);
DWORD CreateGameWindow(void);

