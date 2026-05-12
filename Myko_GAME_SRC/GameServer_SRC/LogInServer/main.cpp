#include "stdafx.h"
#include "../shared/signal_handler.h"
#include "../GameServer/ConsoleColor.h"

LoginServer * g_pMain;
static Condition s_hEvent;
bool g_bRunning = true;

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	SetConsoleTitle("ByNoise Server Files (LogIn Server) v" STRINGIFY(__VERSION));

	// Override the console handler
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);

	HookSignals(&s_hEvent);

	g_pMain = new LoginServer();
	g_pMain->s_hEvent = &s_hEvent;

	// Startup server
	if (g_pMain->Startup())
	{
		con_green();
		printf("╔══════════════════════════════════════╗\n");
		printf("║          B Y N O I S E E             ║\n");
		printf("║    Knight Online Login Server v2369  ║\n");
		printf("╚══════════════════════════════════════╝\n");
		printf("\nLogin Server basariyla baslatildi!\n\n");
		con_white();

		// Wait until console's signaled as closing
		s_hEvent.Wait();
	}
	else
	{
		con_red(); printf("ERROR: Startup failed!\n"); con_white();
		Sleep(5000);
	}

	con_yellow(); printf("Server kapatiliyor, Lutfen bekleyin...\n"); con_white();

	g_bRunning = false; 
	delete g_pMain;
	UnhookSignals();

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
