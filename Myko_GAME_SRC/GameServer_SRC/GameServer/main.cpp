#include "stdafx.h"
#include "../shared/Condition.h"
#include "ConsoleInputThread.h"
#include "../shared/signal_handler.h"
#include "../shared/CrashHandler.h"//x
#include "ConsoleColor.h"

CGameServerDlg * g_pMain;
static Condition s_hEvent;

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);

bool g_bRunning = true;

int main()
{
	// Mutex: ayni anda sadece 1 GameServer calismali
	HANDLE hMutex = CreateMutexA(NULL, TRUE, "MYKO_GameServer_Mutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		con_red(); printf("[HATA] GameServer zaten calisiyor! 2. instance acilamaz.\n"); con_white();
		if (hMutex) CloseHandle(hMutex);
		return 1;
	}

	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	SetConsoleTitle("Knight Online Game Systems - v" STRINGIFY(__VERSION));

	// 20.10.2020 Gameserver aciis suresi hesaplama start
	DateTime time;
	clock_t x = clock();
	// 20.10.2020 Gameserver aciis suresi hesaplama end
	CCrashHandler XxX;//x
	XxX.SetProcessExceptionHandlers();//x
	XxX.SetThreadExceptionHandlers();//x

	// Override the console handler
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);

	HookSignals(&s_hEvent);

	// Start up the time updater thread
	StartTimeThread();

	ExplosionHandle::SetupExceptionHandler(); // BugTrap 27.09.2020

	// Start up the console input thread
	StartConsoleInputThread();

	g_pMain = new CGameServerDlg();
	g_pMain->s_hEvent = &s_hEvent;

	// Start up server
	if (g_pMain->Startup())
	{
		// Reset Battle Zone Variables.
		g_pMain->ResetBattleZone(BATTLEZONE_NONE);

		con_green();
		printf("╔══════════════════════════════════════╗\n");
		printf("║          B Y N O I S E E             ║\n");
		printf("║    Knight Online Game Server v2369   ║\n");
		printf("╚══════════════════════════════════════╝\n");
		printf("The game server has been successfully started.\n");
		printf("** > GameServer Started in (%.2lf Seconds) on %04d-%02d-%02d at %02d:%02d\n", (clock() - x) / (double)CLOCKS_PER_SEC, time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute());
		printf("The game has been started. Enjoyable Games.\n");
		con_white();
		// Wait until console's signaled as closing
		s_hEvent.Wait();  
	}
	else
	{
		con_red(); printf("[HATA] Startup basarisiz! Log dosyalarini kontrol edin.\n"); con_white();
		sleep(5000);
	}

	con_yellow(); printf("Server is shutting down, please wait...\n"); con_white();

	// This seems redundant, but it's not. 
	// We still have the destructor for the dialog instance, which allows time for threads to properly cleanup.
	g_bRunning = false; 

	delete g_pMain;

	CleanupTimeThread();
	CleanupConsoleInputThread();
	UnhookSignals();

	// Mutex serbest birak
	if (hMutex) {
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
	}

	return 0;
}

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	s_hEvent.BeginSynchronized();
	s_hEvent.Signal();
	s_hEvent.EndSynchronized();
	sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}
