#pragma warning(push, 3)
// Header files

#include <stdio.h>
#include <windows.h>
#include <stdint.h>
#include "Main.h"


BOOL gGameIsRunning;
HWND gGameWindow;
GAMEBITMAP gBackBuffer;
GAMEPERFDATA gPerformanceData;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int iCmdShow)
{
	MSG	msg;
	HMODULE NtDllModuleHandle;
	int64_t FrameStart = 0;
	int64_t FrameEnd = 0;
	int64_t ElapseMicroSecondsPerFrame;
	int64_t ElapsedMicroseondsPerFrameAccumulatorRaw = 0;
	int64_t ElapsedMicrosecondsPerFrameAccumulatorCooked = 0;

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

	if (!CreateGameWindow())
		return (0);
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

	gGameIsRunning = TRUE;
	while (gGameIsRunning == TRUE) {
		QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);

		while (PeekMessage(&msg, gGameWindow, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
		// Process player inputs
		ProcessPlayerInput();
		RenderFrameGraphics();

		QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

		ElapseMicroSecondsPerFrame = FrameEnd - FrameStart;
		ElapseMicroSecondsPerFrame *= 1000000;
		ElapseMicroSecondsPerFrame /= gPerformanceData.PerfFrequency;
		gPerformanceData.TotalFramesRendered++;
		ElapsedMicroseondsPerFrameAccumulatorRaw += ElapseMicroSecondsPerFrame;

		while (ElapseMicroSecondsPerFrame <= TARGET_MICROSECONDS_PER_FRAME) {
			ElapseMicroSecondsPerFrame = FrameEnd - FrameStart;
			ElapseMicroSecondsPerFrame *= 1000000;
			ElapseMicroSecondsPerFrame /= gPerformanceData.PerfFrequency;
			QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);
			if (ElapseMicroSecondsPerFrame <= ((int64_t)TARGET_MICROSECONDS_PER_FRAME - gPerformanceData.CurrentTimerResolution))
				Sleep(1);
		}
		ElapsedMicrosecondsPerFrameAccumulatorCooked += ElapseMicroSecondsPerFrame;

		if ((gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES) == 0) {

			gPerformanceData.CookedFramesPerSecondAverage = 1.0f / ((ElapsedMicrosecondsPerFrameAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);
			gPerformanceData.RawFramesPerSecondsAverage = 1.0f / ((ElapsedMicroseondsPerFrameAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

			ElapsedMicroseondsPerFrameAccumulatorRaw = 0;
			ElapsedMicrosecondsPerFrameAccumulatorCooked = 0;
		}
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
	}

	return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}

void ProcessPlayerInput()
{
	BOOL MbAnswer;
	int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
	int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F1);
	static int16_t DebugKeyWasDown;

	if (EscapeKeyIsDown) {
		MbAnswer = MessageBox(NULL,
			TEXT("Are you sure to close the game"),
			TEXT("Battle City"),
			MB_ICONQUESTION | MB_OKCANCEL);

		if(MbAnswer == 1)
			SendMessage(gGameWindow, WM_DESTROY, 0, 0);
	}

	if (DebugKeyIsDown && !DebugKeyWasDown) {
		gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
	}

	DebugKeyWasDown = DebugKeyIsDown;
}

DWORD CreateGameWindow(void)
{
	WNDCLASS wndclass;

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
		MessageBox(NULL, TEXT("Prgram requires Windows NT"), szGameName, MB_ICONERROR);
		return (0);
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

	gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0)
		return (-1);

	gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;
	gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

	/* It's a good practice to take out WS_OVERLAPPEDWINDOW by following way
	*  programmer may not know the state of the flag.
	*/

	SetWindowLongPtr(gGameWindow, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW);

	if (SetWindowPos(gGameWindow,
		HWND_TOP,
		gPerformanceData.MonitorInfo.rcMonitor.left,
		gPerformanceData.MonitorInfo.rcMonitor.top,
		gPerformanceData.MonitorWidth,
		gPerformanceData.MonitorHeight,
		SWP_FRAMECHANGED) == 0)
		return (-1);
}

void RenderFrameGraphics(void)
{
	// memset(gBackBuffer.Memory, 0xFF, (GAME_RES_WIDTH * 4)*10);
	PIXEL32 Pixel = { 0 };
	char DebugTextBuffer[64] = { 0 };
	
	Pixel.Blue = 0x7f;
	Pixel.Green = 0;
	Pixel.Red = 0;
	Pixel.Alpha = 0xff;

	for (int x=0; x < GAME_RES_WIDTH*GAME_RES_HEIGHT; ++x)
		memcpy((PIXEL32*)gBackBuffer.Memory + x, &Pixel, sizeof(PIXEL32));

	int32_t ScreenX = 25;
	int32_t ScreenY = 25;
	int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - \
		(GAME_RES_WIDTH * ScreenY) + ScreenX;

	for (int32_t y = 0; y < 16; ++y) {
		for (int32_t x = 0; x < 16; ++x)
			memset((PIXEL32*)gBackBuffer.Memory + (uintptr_t)StartingScreenPixel + x - ((uintptr_t)GAME_RES_WIDTH * y), 0xFF, sizeof(PIXEL32));
	}

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
	}


	ReleaseDC(gGameWindow, DeviceContext);
}
