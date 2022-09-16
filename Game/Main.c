#pragma warning(push, 3)
// Header files

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <psapi.h>
#include <emmintrin.h>
#include <stdint.h>
#include "Main.h"

#pragma comment (lib, "Winmm.lib")

BOOL gGameIsRunning;
HWND gGameWindow;
GAMEBITMAP gBackBuffer;
GAMEPERFDATA gPerformanceData;
PLAYER gPlayer;
PLAYER gEnemyTank;
__m128i data;
BOOL gWindowHasFocus;
int gTimeToChangeDirection;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int iCmdShow)
{
	MSG	msg;
	HMODULE NtDllModuleHandle = NULL;
	int64_t FrameStart = 0;
	int64_t FrameEnd = 0;
	int64_t ElapseMicroSeconds = 0;
	int64_t ElapsedMicroseondsPerFrameAccumulatorRaw = 0;
	int64_t ElapsedMicrosecondsPerFrameAccumulatorCooked = 0;
	FILETIME ProcessCreationTime = { 0 };
	FILETIME ProcessExitTime = { 0 };
	int64_t CurrentUserCPUTime = 0;
	int64_t CurrentKernelCPUTime = 0;
	int64_t PreviousUserCPUTime = 0;
	int64_t PreviousKernelCPUTime = 0;
	HANDLE ProcessHandle = GetCurrentProcess();

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpszCmdLine);
	UNREFERENCED_PARAMETER(iCmdShow);

	if ((NtDllModuleHandle = GetModuleHandle(TEXT("ntdll.dll"))) == NULL) {
		MessageBox(NULL, TEXT("Couldn't load ntdll.dll!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution")) == NULL) {
		MessageBox(NULL, TEXT("Couldn't find NtQueryTimerResolution function in ntdll.dll!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	NtQueryTimerResolution(&gPerformanceData.MinimumTimerResoltuion, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);

	GetSystemInfo(&gPerformanceData.SystemInfo);
	GetSystemTimeAsFileTime((FILETIME*)&gPerformanceData.PreviousSystemTime);

	if (timeBeginPeriod(1) == TIMERR_NOCANDO) {
		MessageBox(NULL, "Failed to set global timer resolution!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	if (SetPriorityClass(ProcessHandle, HIGH_PRIORITY_CLASS) == 0) {
		MessageBox(NULL, "Failed to set process priority!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) == 0) {
		MessageBox(NULL, "Failed to set thread priority!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	if (CreateGameWindow() != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Failed to create game window."), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}
	
	// Sense of timing
	QueryPerformanceFrequency((LARGE_INTEGER*)&gPerformanceData.PerfFrequency);
	
	// Game drawings
	gBackBuffer.BitMapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitMapInfo.bmiHeader);
	gBackBuffer.BitMapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;
	gBackBuffer.BitMapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;
	gBackBuffer.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
	gBackBuffer.BitMapInfo.bmiHeader.biCompression = BI_RGB;
	gBackBuffer.BitMapInfo.bmiHeader.biPlanes = 1;

	gBackBuffer.Memory = VirtualAlloc(NULL,
		GAME_DRAWING_AREA_MEMORY,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);

	if(gBackBuffer.Memory == NULL) {
		MessageBox(NULL,
			TEXT("Failed to allocate memory for drawing suface!"),
			TEXT("Error"),
			MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY);

	if (InitializeTank() != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Failed to initialize the tank!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}

	if (InitializeEnemyTanks() != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Failed to initialize enemy tank!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return (0);
	}
	
	// GetSystemInfo(&gPerformanceData.SystemInfo);
	srand((unsigned int)time(0));
	gTimeToChangeDirection = rand() % (GAME_RES_WIDTH - GAME_RES_HEIGHT);
	gGameIsRunning = TRUE;

	while (gGameIsRunning == TRUE) {
		QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);

		if (gTimeToChangeDirection < 2) {
			gTimeToChangeDirection = rand() % GAME_RES_HEIGHT;
			gEnemyTank.Direction = rand() % 4;
		}
		
		while (PeekMessage(&msg, gGameWindow, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
		// Process player inputs
		ProcessPlayerInput();
		RenderFrameGraphics();
		EnemyTankMovements();

		QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

		ElapseMicroSeconds = FrameEnd - FrameStart;
		ElapseMicroSeconds *= 1000000;
		ElapseMicroSeconds /= gPerformanceData.PerfFrequency;
		gPerformanceData.TotalFramesRendered++;
		ElapsedMicroseondsPerFrameAccumulatorRaw += ElapseMicroSeconds;

		while (ElapseMicroSeconds < TARGET_MICROSECONDS_PER_FRAME) {
			ElapseMicroSeconds = FrameEnd - FrameStart;
			ElapseMicroSeconds *= 1000000;
			ElapseMicroSeconds /= gPerformanceData.PerfFrequency;
			QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);
			if (ElapseMicroSeconds < (TARGET_MICROSECONDS_PER_FRAME * 0.75f))
				Sleep(1);
		}
		ElapsedMicrosecondsPerFrameAccumulatorCooked += ElapseMicroSeconds;

		if ((gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES) == 0) {
			GetSystemTimeAsFileTime((FILETIME*)&gPerformanceData.CurrentSystemTime);
			GetProcessTimes(ProcessHandle,
				&ProcessCreationTime,
				&ProcessExitTime,
				(FILETIME*)&CurrentKernelCPUTime,
				(FILETIME*)&CurrentUserCPUTime);

			gPerformanceData.CPUPercent = (CurrentKernelCPUTime - PreviousKernelCPUTime) + \
				(CurrentUserCPUTime - PreviousUserCPUTime);

			gPerformanceData.CPUPercent /= (gPerformanceData.CurrentSystemTime - gPerformanceData.PreviousSystemTime);
			gPerformanceData.CPUPercent /= gPerformanceData.SystemInfo.dwNumberOfProcessors;
			gPerformanceData.CPUPercent *= 100;

			GetProcessHandleCount(ProcessHandle, &gPerformanceData.HandleCount);
			K32GetProcessMemoryInfo(ProcessHandle, (PROCESS_MEMORY_COUNTERS*)&gPerformanceData.MemInfo, sizeof(gPerformanceData.MemInfo));
			gPerformanceData.CookedFramesPerSecondAverage = 1.0f / ((ElapsedMicrosecondsPerFrameAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);
			gPerformanceData.RawFramesPerSecondsAverage = 1.0f / ((ElapsedMicroseondsPerFrameAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

			ElapsedMicroseondsPerFrameAccumulatorRaw = 0;
			ElapsedMicrosecondsPerFrameAccumulatorCooked = 0;

			PreviousKernelCPUTime = CurrentKernelCPUTime;
			PreviousUserCPUTime = CurrentUserCPUTime;
			gPerformanceData.PreviousSystemTime = gPerformanceData.CurrentSystemTime;
		}
		gTimeToChangeDirection--;
	}

	return ((int)msg.wParam);
}

LRESULT CALLBACK WndProc(_In_ HWND hwnd, _In_ UINT iMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_DESTROY:
		gGameIsRunning = FALSE;
		PostQuitMessage(0);
		break;
	case WM_ACTIVATE:
		if (wParam == 0) {
			gWindowHasFocus = FALSE;
		}
		else {
			gWindowHasFocus = TRUE;
			ShowCursor(FALSE);
		}
		break;
	}

	return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}

DWORD CreateGameWindow(void)
{
	WNDCLASS wndclass;
	DWORD Result = ERROR_SUCCESS;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = GetModuleHandle(NULL);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);

	// Creating custom color to highlight once the drawing is done
	// there are not any gaps.

	wndclass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
	wndclass.lpszClassName = szGameName;
	wndclass.lpszMenuName = NULL;

	if (!RegisterClass(&wndclass)) {
		Result = GetLastError();
		MessageBox(NULL, TEXT("Program requires Windows NT"), szGameName, MB_ICONERROR);
		goto Exit;
	}

	gGameWindow = CreateWindow(szGameName,
		szGameName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		640,
		480,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL);

	if (gGameWindow == NULL) {
		Result = GetLastError();
		goto Exit;
	}

	gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0) {
		Result = GetLastError();
		goto Exit;
	}

	gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;
	gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

	/* It's a good practice to take out WS_OVERLAPPEDWINDOW by following way
	*  programmer may not know the state of the flag.
	*/

	if (SetWindowLongPtr(gGameWindow, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0) {
		Result = GetLastError();
		goto Exit;
	}

	if (SetWindowPos(gGameWindow,
		HWND_TOP,
		gPerformanceData.MonitorInfo.rcMonitor.left,
		gPerformanceData.MonitorInfo.rcMonitor.top,
		gPerformanceData.MonitorWidth,
		gPerformanceData.MonitorHeight,
		SWP_FRAMECHANGED) == 0) {
		Result = GetLastError();
		goto Exit;
	}

Exit:
	return (Result);
}

void ProcessPlayerInput()
{
	BOOL MbAnswer;
	if (gWindowHasFocus == FALSE)
		return (0);
	int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
	int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F1);
	int16_t LeftKeyIsDown = GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('A');
	int16_t RightKeyIsDown = GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('D');
	int16_t UpKeyIsDown = GetAsyncKeyState(VK_UP) | GetAsyncKeyState('W');
	int16_t DownKeyIsDown = GetAsyncKeyState(VK_DOWN) | GetAsyncKeyState('S');
	int16_t SpaceKeyIsDown = GetAsyncKeyState(VK_SPACE);

	static int16_t DebugKeyWasDown;
	static int16_t LeftKeyWasDown;
	static int16_t RightKeyWasDown;

	if (EscapeKeyIsDown) {
		MbAnswer = MessageBox(NULL,
			TEXT("Are you sure to close the game"),
			TEXT("Battle City"),
			MB_ICONQUESTION | MB_OKCANCEL);

		if (MbAnswer == 1)
			SendMessage(gGameWindow, WM_DESTROY, 0, 0);
	}

	if (DebugKeyIsDown && !DebugKeyWasDown) {
		gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
	}

	if (LeftKeyIsDown){// && (UpKeyIsDown || !DownKeyIsDown)) {
		if (gPlayer.ScreenPosX > 0) {
			gPlayer.ScreenPosX--;
			gPlayer.Direction = DIRECTION_LEFT;
		}
	}
	else if (RightKeyIsDown){// && (!UpKeyIsDown || !DownKeyIsDown)) {
		if (gPlayer.ScreenPosX < GAME_RES_WIDTH - 16) {
			gPlayer.ScreenPosX++;
			gPlayer.Direction = DIRECTION_RIGHT;
		}
	}
	else if (UpKeyIsDown){// && (!LeftKeyIsDown || !RightKeyIsDown)) {
		if (gPlayer.ScreenPosY > 0) {
			gPlayer.ScreenPosY--;
			gPlayer.Direction = DIRECTION_UP;
		}
	}
	else if (DownKeyIsDown){// && (!LeftKeyIsDown || !RightKeyIsDown)) {
		if (gPlayer.ScreenPosY < GAME_RES_HEIGHT - 16) {
			gPlayer.ScreenPosY++;
			gPlayer.Direction = DIRECTION_DOWN;
		}
	}
	if (SpaceKeyIsDown) {

	}
	
	DebugKeyWasDown = DebugKeyIsDown;
	//LeftKeyWasDown = LeftKeyIsDown;
	//RightKeyWasDown = RightKeyIsDown;

}

DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_  GAMEBITMAP* GameBitmap)
{
	DWORD Error = ERROR_SUCCESS;
	HANDLE FileHandle = INVALID_HANDLE_VALUE;
	WORD BitMapHeader = 0;
	DWORD PixelDataOffset = 0;
	DWORD NumberOfBytesToRead = 2;



	if ((FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		Error = GetLastError();
		goto Exit;
	}

	if (ReadFile(FileHandle, &BitMapHeader, 2, &NumberOfBytesToRead, NULL) == 0) {
		Error = GetLastError();
		goto Exit;
	}

	if (BitMapHeader != 0x4d42) { // BM - Bitmap file
		Error = ERROR_FILE_INVALID;
		goto Exit;
	}

	if (SetFilePointer(FileHandle, 0xA, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		Error = GetLastError();
		goto Exit;
	}

	if (ReadFile(FileHandle, &PixelDataOffset, sizeof(DWORD), &NumberOfBytesToRead, NULL) == 0) {
		Error = GetLastError();
		goto Exit;
	}

	if (SetFilePointer(FileHandle, 0xE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		Error = GetLastError();
		goto Exit;
	}

	if (ReadFile(FileHandle, &GameBitmap->BitMapInfo.bmiHeader, sizeof(BITMAPINFOHEADER), &NumberOfBytesToRead, NULL) == 0) {
		Error = GetLastError();
		goto Exit;
	}

	if ((GameBitmap->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GameBitmap->BitMapInfo.bmiHeader.biSizeImage)) == NULL) {
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto Exit;
	}

	if (SetFilePointer(FileHandle, PixelDataOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		Error = GetLastError();
		goto Exit;
	}

	if (ReadFile(FileHandle, GameBitmap->Memory, GameBitmap->BitMapInfo.bmiHeader.biSizeImage, &NumberOfBytesToRead, NULL) == 0) {
		Error = GetLastError();
		goto Exit;
	}

	//memcpy_s(GameBitmap->Memory, GameBitmap->BitMapInfo.bmiHeader.biSizeImage, )

Exit:
	if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE))
		CloseHandle(FileHandle);
	return (Error);
}

DWORD InitializeTank(void)
{
	DWORD Error = ERROR_SUCCESS;

	gPlayer.ScreenPosX = 32;
	gPlayer.ScreenPosY = 32;
	gPlayer.CurrentPower = TANK_WITHOUT_POWER;
	gPlayer.Direction = DIRECTION_UP;
	
	if ((Error = Load32BppBitmapFromFile("tank_up.bmp", &gPlayer.Sprite[TANK_WITHOUT_POWER][DIRECTION_UP])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}
	
	if ((Error = Load32BppBitmapFromFile("tank_down.bmp", &gPlayer.Sprite[TANK_WITHOUT_POWER][DIRECTION_DOWN])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}

	if ((Error = Load32BppBitmapFromFile("tank_right.bmp", &gPlayer.Sprite[TANK_WITHOUT_POWER][DIRECTION_RIGHT])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}

	if ((Error = Load32BppBitmapFromFile("tank_left.bmp", &gPlayer.Sprite[TANK_WITHOUT_POWER][DIRECTION_LEFT])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}

Exit:
	return (Error);
}

DWORD InitializeEnemyTanks(void)
{
	DWORD Error = ERROR_SUCCESS;

	gEnemyTank.ScreenPosX = 0;
	gEnemyTank.ScreenPosY = 0;
	gPlayer.CurrentPower = TANK_WITHOUT_POWER;
	gPlayer.Direction = DIRECTION_DOWN;

	if ((Error = Load32BppBitmapFromFile("tank_down.bmp", &gEnemyTank.Sprite[TANK_WITHOUT_POWER][DIRECTION_DOWN])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}
	if ((Error = Load32BppBitmapFromFile("tank_up.bmp", &gEnemyTank.Sprite[TANK_WITHOUT_POWER][DIRECTION_UP])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}
	if ((Error = Load32BppBitmapFromFile("tank_left.bmp", &gEnemyTank.Sprite[TANK_WITHOUT_POWER][DIRECTION_LEFT])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}
	if ((Error = Load32BppBitmapFromFile("tank_right.bmp", &gEnemyTank.Sprite[TANK_WITHOUT_POWER][DIRECTION_RIGHT])) != ERROR_SUCCESS) {
		MessageBox(NULL, TEXT("Load32BppBitmapFromFile failed."), TEXT("Error"), MB_ICONERROR);
		goto Exit;
	}
Exit:
	return (Error);
}

void RenderFrameGraphics(void)
{
	// memset(gBackBuffer.Memory, 0xFF, (GAME_RES_WIDTH * 4)*10);
	__m128i QuadPixel = { 0x7f, 0x00, 0x00, 0xff,
		0x7f, 0x00, 0x00, 0xff,
		0x7f, 0x00, 0x00, 0xff,
		0x7f, 0x00, 0x00, 0xff
	};
	char DebugTextBuffer[64] = { 0 };

#ifdef SIMD
	ClearScreen(&QuadPixel);
#else
	PIXEL32 Pixel = { 0x7f, 0x00, 0x00, 0xff };
	ClearScreen(&Pixel);
#endif

	Blit32BppBitmapToBuffer(&gPlayer.Sprite[gPlayer.CurrentPower][gPlayer.Direction], gPlayer.ScreenPosX, gPlayer.ScreenPosY);

	Blit32BppBitmapToBuffer(&gEnemyTank.Sprite[gEnemyTank.CurrentPower][gEnemyTank.Direction], gEnemyTank.ScreenPosX, gEnemyTank.ScreenPosY);

	HDC DeviceContext = GetDC(gGameWindow);

	StretchDIBits(DeviceContext,
		0,
		0,
		gPerformanceData.MonitorWidth,
		gPerformanceData.MonitorHeight,
		0,
		0,
		GAME_RES_WIDTH,
		GAME_RES_HEIGHT,
		gBackBuffer.Memory,
		&gBackBuffer.BitMapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);

	if (gPerformanceData.DisplayDebugInfo == TRUE) {
		SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Raw:  %.01f", gPerformanceData.RawFramesPerSecondsAverage);
		TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Cooked:  %.01f", gPerformanceData.CookedFramesPerSecondAverage);
		TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Minimum Timer Resolution:  %.02f", gPerformanceData.MinimumTimerResoltuion / 10000.0f);
		TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Maximum Timer Resolution:  %.02f", gPerformanceData.MaximumTimerResolution / 10000.0f);
		TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Current Timer Resolution:  %.02f", gPerformanceData.CurrentTimerResolution / 10000.0f);
		TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Handles:  %lu", gPerformanceData.HandleCount);
		TextOutA(DeviceContext, 0, 65, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Memory:  %d KB", (int)gPerformanceData.MemInfo.PrivateUsage / 1024);
		TextOutA(DeviceContext, 0, 78, DebugTextBuffer, (int)strlen(DebugTextBuffer));
		sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "CPU:  %.01f %%", gPerformanceData.CPUPercent);
		TextOutA(DeviceContext, 0, 91, DebugTextBuffer, (int)strlen(DebugTextBuffer));
	}

	ReleaseDC(gGameWindow, DeviceContext);
}

void EnemyTankMovements(void)
{
	if (gEnemyTank.ScreenPosX == 0 ||
		gEnemyTank.ScreenPosY == 0 ||
		gEnemyTank.ScreenPosX == GAME_RES_WIDTH ||
		gEnemyTank.ScreenPosY == GAME_RES_HEIGHT) {
		gTimeToChangeDirection = 0;
	}
	if (gEnemyTank.Direction == DIRECTION_LEFT) {
		if (gEnemyTank.ScreenPosX > 0) {
			gEnemyTank.ScreenPosX--;
		}
	}
	if (gEnemyTank.Direction == DIRECTION_RIGHT) {
		if (gEnemyTank.ScreenPosX < GAME_RES_WIDTH - 16) {
			gEnemyTank.ScreenPosX++;
		}
	}
	if (gEnemyTank.Direction == DIRECTION_UP) {
		if (gEnemyTank.ScreenPosY > 0) {
			gEnemyTank.ScreenPosY--;
		}
	}
	if (gEnemyTank.Direction == DIRECTION_DOWN) {
		if (gEnemyTank.ScreenPosY < GAME_RES_HEIGHT - 16) {
			gEnemyTank.ScreenPosY++;
		}
	}
		
	Blit32BppBitmapToBuffer(&gEnemyTank.Sprite[gEnemyTank.CurrentPower][gEnemyTank.Direction], gEnemyTank.ScreenPosX, gEnemyTank.ScreenPosY);
}

#ifdef SIMD
__forceinline void ClearScreen(_In_ __m128i *Color)
{
	for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x += 4)
		_mm_store_si128((PIXEL32*)gBackBuffer.Memory + x, *Color);
}
#else
__forceinline void ClearScreen(_In_ PIXEL32* Pixel)
{
	for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x++)
		memcpy((PIXEL32*)gBackBuffer.Memory + x, Pixel, sizeof(PIXEL32));
}
#endif

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitMap, _In_ uint16_t x, _In_ uint16_t y)
{
	int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - (GAME_RES_WIDTH * y) + x;

//	int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - \
		(GAME_RES_WIDTH * ScreenY) + ScreenX;
	int32_t StartingBitmapPixel = ((GameBitMap->BitMapInfo.bmiHeader.biWidth * GameBitMap->BitMapInfo.bmiHeader.biHeight) - \
		GameBitMap->BitMapInfo.bmiHeader.biWidth);

	int32_t MemoryOffset = 0;
	int32_t BitmapOffset = 0;
	PIXEL32 BitmapPixel = { 0 };
	PIXEL32 BackgroundPixel = { 0 };

	for (int16_t yPixel = 0; yPixel < GameBitMap->BitMapInfo.bmiHeader.biHeight; ++yPixel) {
		for (int16_t xPixel = 0; xPixel < GameBitMap->BitMapInfo.bmiHeader.biWidth; ++xPixel) {
			MemoryOffset = StartingScreenPixel + xPixel - (GAME_RES_WIDTH * yPixel);
			BitmapOffset = StartingBitmapPixel + xPixel - (GameBitMap->BitMapInfo.bmiHeader.biWidth * yPixel);

			memcpy_s(&BitmapPixel, sizeof(PIXEL32), (PIXEL32*)GameBitMap->Memory + BitmapOffset, sizeof(PIXEL32));
			if (BitmapPixel.Alpha == 255) {
				memcpy_s((PIXEL32*)gBackBuffer.Memory + MemoryOffset, sizeof(PIXEL32), &BitmapPixel, sizeof(PIXEL32));
			}
		}
	}
}
