#include "PearlGui.h"
#include "Timer.h"
#include "resource.h"
#include <future>
#include "CSpell.h"
#include "FunctionGuard.h"
#include "../resource.h"
#include "DiagLog.h"
using namespace std;
extern bool ischeatactive;
HRESULT WINAPI HookCreateDevice();

#define HOOK(func,addy)	o##func = (t##func)DetourFunction((PBYTE)addy,(PBYTE)hk##func)
#define D3D_RELEASE(D3D_PTR) if( D3D_PTR ){ D3D_PTR->Release(); D3D_PTR = NULL; }
#define ES	0
#define DIP	1
#define RES 2
#define FOX 3
HRESULT WINAPI hkReset(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9);
typedef HRESULT(WINAPI* tReset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(WINAPI* mySendMessage) (HWND, UINT, WPARAM, LPARAM);

typedef HRESULT(WINAPI* CreateDevice_t)(IDirect3D9* Direct3D_Object, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface);
HRESULT WINAPI D3DCreateDevice_hook(IDirect3D9 * Direct3D_Object, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters,
	IDirect3DDevice9 * *ppReturnedDeviceInterface);

CreateDevice_t D3DCreateDevice_orig;

typedef HRESULT(WINAPI* tSetScissorRect)(LPDIRECT3DDEVICE9 pDevice, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9* ppSB);
tSetScissorRect  oSetScissorRect;

HRESULT WINAPI SetScissorRect(LPDIRECT3DDEVICE9 pDevice, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9* ppSB)
{
	HRESULT ret = oSetScissorRect(pDevice, Type, ppSB);
	//printf("THETHYKE | EXCEPTION 4\n");
	ischeatactive = true;
	DiagMark("SetScissorRect (UI/chat clip cizim)"); // resize donmasi suphelisi: son olay bu mu?
	return ret;
}
PDWORD IDirect3D9_vtable = NULL;
#define CREATEDEVICE_VTI 16

DWORD VTable[3] = { 0 };
DWORD D3DEndScene;
DWORD D3DReset;
tReset oReset;
tEndScene oPresent;
tEndScene oEndScene;
WNDPROC oWndProc;
static int myfpslimit = 60;
static bool isfpslimit = false;
static bool sendHooked = false;
static bool reset = false;
static bool itsMe = false;

LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;
LPDIRECT3DINDEXBUFFER9  g_pIB = NULL;

HWND window = 0;
NOTIFYICONDATA nid;
bool hiddenWindow = false;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	

	switch (uMsg)
	{
	case WM_APP + 1:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			if (Engine)
				if (!IsWindowVisible(window)) {
					ShowWindow(window, SW_SHOW);
					hiddenWindow = false;
				}
			break;
		}
		break;
	}
	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
	//return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HRESULT WINAPI hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* Present)
{
	if (Engine->m_SettingsMgr == NULL) 
		Engine->m_SettingsMgr = new CSettingsManager();

	Present->Windowed = Engine->m_SettingsMgr->m_iRealFullScreen == 1 ? false : true;
	Present->PresentationInterval = Engine->m_SettingsMgr->m_iVsync == 1 ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

	if (Engine->m_SettingsMgr->m_iVsync == 1)
		Present->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	// DiagLog D3D: device Reset = alt-tab/resize/fullscreen-gecis/device-lost. Acilista
	// kapanma + alt-tab donma burada gorunur (Reset basarisiz olursa siyah ekran/crash).
	DiagMark("D3D Reset");
	DIAG("D3D", "Reset cagrildi: %ux%u Windowed=%d Vsync=%d (alt-tab/resize/device-lost)",
		Present->BackBufferWidth, Present->BackBufferHeight,
		(int)Present->Windowed, Engine->m_SettingsMgr->m_iVsync);

	reset = true;
	HRESULT hrReset = oReset(pDevice, Present);
	if (FAILED(hrReset))
		DIAG("D3D", "Reset BASARISIZ hr=0x%08X (siyah ekran/kapanma riski!)", hrReset);
	return hrReset;
}

extern CSpell* GetSkillBase(int iSkillID);
DWORD adresdd = 0x33C;

ULONGLONG thtime = GetTickCount64();
HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	// A/B TEST (patron S136, 2026-06-27): TUM diagnostic blok GECICI KAPALI (#if 0).
	// Sebep: bu blok HER FRAME GetForegroundWindow + TestCooperativeLevel + GetAsyncKeyState x9 cagiriyor,
	// DiagLog kapali olsa bile (g_DiagEnabled gate'i DISINDA) -> render thread'inde her-frame syscall =
	// "alt-tab donma + mouse yavas" adayi. Kapaliyken DUZELIRSE -> kok kesin bu -> dogru fix (g_DiagEnabled gate).
	// KURAL 0-B: kod silinmedi, #if 0 ile pasif. Render mantigi (217+) ETKILENMEZ.
#if 0
	// DiagLog FREEZE dedektoru: iki frame arasi gecen sure. Donma = render durur =
	// EndScene gec gelir. >300ms ise donma kaydet (chat resize donmasi + genel takilma).
	{
		static DWORD s_lastFrameTick = 0;
		DWORD now = GetTickCount();
		if (s_lastFrameTick != 0) {
			DWORD delta = now - s_lastFrameTick;
			if (delta > 500) { // FAZ1 #1.2 (S134): 300->500ms. 300 zayif-PC mikro-takilmayi cok yakaliyordu (3262 spam); 500=gercek donma kalir, gurultu gider
				// Donmadan ONCE en son ne oldu? SADECE donmaya YAKIN (son 2sn) olayi goster.
				// Bayat etiket (8dk onceki foreground) yaniltiyor -> 2sn disindaki "yakin olay yok".
				DWORD evAge = (g_DiagLastEventTick != 0) ? (now - g_DiagLastEventTick) : 999999;
				const char* yakin = (evAge <= 2000) ? g_DiagLastEvent : "(yakin tetik yok)";

				// Donma aninda DURUM fotosu: bellek + handle + son thread aktivitesi.
				// "Arka planda ne kasiyor" sorusu icin: MEM artiyorsa sizinti, thread takiliysa o.
				extern char  g_DiagThreadActivity[64]; // son calisan code.guard thread + zamani
				extern DWORD g_DiagThreadActivityTick;
				DWORD thrAge = (g_DiagThreadActivityTick != 0) ? (now - g_DiagThreadActivityTick) : 999999;

				PROCESS_MEMORY_COUNTERS pmc; pmc.cb = sizeof(pmc);
				DWORD memMB = 0;
				if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
					memMB = (DWORD)(pmc.WorkingSetSize / (1024 * 1024));

				// FAZ2 #2.2 (S134): "son cg-thread" YANILTICIYDI -> okuyan SuspendCheck'i SUCLU saniyordu.
				// Gercekte DiagThreadTick sadece "son tick atan thread"i yazar; SuspendCheck 1sn'de bir
				// tick attigi icin freeze'lerin %85'inde dogal olarak "en son" gorunur = istatistik artefakti,
				// NEDENSELLIK DEGIL. Etikete "(suclu degil)" notu + thrAge cok eskiyse "ilgisiz" diye isaretle.
				const char* thrNot = (thrAge > 1500) ? "(ilgisiz/bayat)" : "(sadece son tick, suclu degil)";
				DIAG("FREEZE", "Oyun %ums dondu | oncesi: %s (%ums) | son tick cg-thread: %s %s (%ums once) | RAM=%uMB",
					delta, yakin, (evAge <= 2000 ? evAge : 0),
					g_DiagThreadActivity, thrNot, thrAge, memMB);
			}
		}
		s_lastFrameTick = now;

		// FPS: her saniye bir kez ortalama FPS + dususte uyari. Trend gormek icin.
		{
			static DWORD s_fpsWindowStart = 0;
			static DWORD s_frameCount = 0;
			s_frameCount++;
			if (s_fpsWindowStart == 0) s_fpsWindowStart = now;
			DWORD elapsed = now - s_fpsWindowStart;
			if (elapsed >= 1000) {
				DWORD fps = (s_frameCount * 1000) / elapsed;
				// FAZ1 #1.1 (S134): eskiden her saniye fps<20 yazardi -> 3.5sa oturumda 9579 satir
				// = 1.3MB dosya sismesi. Artik SADECE fps<15 (gercek kasma) VE son FPS-log'dan
				// >=10sn gectiyse yaz. Trend yine gorunur (10sn'de 1 ornek), gurultu ~10x duser.
				static DWORD s_lastFpsLog = 0;
				if (fps < 15 && (now - s_lastFpsLog >= 10000)) {
					DIAG("FPS", "DUSUK fps=%u (son olay: %s)", fps, g_DiagLastEvent);
					s_lastFpsLog = now;
				}
				s_frameCount = 0;
				s_fpsWindowStart = now;
			}
		}

		// DiagLog INPUT (klavye sizmasi #242): arka plandayken hareket TUSU basili mi?
		// GetAsyncKeyState SADECE OKUMA (detour YOK -> SIGSEGV riski yok, S116 ders). Arka planda
		// olmasina ragmen WASD/ok/space basili gorunuyorsa -> klavye sizmasi (arka karakter hareket eder).
		{
			extern HWND gameWindow;
			bool arkaPlan = (gameWindow != NULL && GetForegroundWindow() != gameWindow);
			if (arkaPlan) {
				// hareket tuslari: W A S D + ok tuslari + space (oyun-ici hareket)
				static const int hareketTuslari[] = { 'W','A','S','D', VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT, VK_SPACE };
				bool basili = false; int tus = 0;
				for (int k = 0; k < (int)(sizeof(hareketTuslari)/sizeof(int)); k++) {
					if (GetAsyncKeyState(hareketTuslari[k]) & 0x8000) { basili = true; tus = hareketTuslari[k]; break; }
				}
				if (basili) {
					static DWORD s_lastKbLog = 0;
					if (now - s_lastKbLog > 2000) { // 2sn'de max 1 (sismesin)
						DIAG("INPUT", "KLAVYE SIZMA RISKI! arka planda ama hareket tusu basili (vk=0x%02X)", tus);
						s_lastKbLog = now;
					}
				}
			}
		}

		// D3D DEVICE LOST: cihaz kaybedildiyse render durur -> siyah ekran/donma. Reset'in
		// SEBEBI burada gorunur (alt-tab/fullscreen-cakisma/surucu reset). Durum DEGISIMINDE yaz.
		if (pDevice) {
			static HRESULT s_lastCoop = D3D_OK;
			HRESULT coop = pDevice->TestCooperativeLevel();
			if (coop != s_lastCoop) {
				const char* durum = (coop == D3DERR_DEVICELOST) ? "DEVICE LOST (kayip)" :
					(coop == D3DERR_DEVICENOTRESET) ? "DEVICE NOT RESET (reset bekliyor)" :
					(coop == D3D_OK) ? "OK (geri geldi)" : "bilinmeyen";
				DIAG("D3D", "TestCooperativeLevel degisti: %s (hr=0x%08X, son olay: %s)",
					durum, coop, g_DiagLastEvent);
				s_lastCoop = coop;
			}
		}
	}
#endif // A/B TEST: diagnostic blok kapali (yukaridaki #if 0)

	/*if (GetAsyncKeyState(VK_RETURN) & 1 && Engine->Adress > 0)
	{
		POINT pt;
		pt.x = 50;
		pt.y = 50;
		Engine->m_UiMgr->SendMouseProc(UI_MOUSE_LBCLICK, pt, pt);
	}*/

	if (GetAsyncKeyState(VK_F1) & 1)
	{
		//vector<int>offsets;
		//offsets.push_back(0x518); //0x518
		//offsets.push_back(0);

		//DWORD m_dVTableAddr = Engine->rdword(KO_DLG, offsets);
		//Engine->SetVisible(m_dVTableAddr, true);
		//Engine->UIScreenCenter(m_dVTableAddr);
		//Engine->AddListString()
	}

	if(!Engine->render)
		return oEndScene(pDevice);

	if (!Engine->power)
		return (BYTE)0x90;
	
	
	if (GetAsyncKeyState(VK_F12) & 1)
	{
		if (GetForegroundWindow() == window) {
			ShowWindow(window, false);
			hiddenWindow = true;
		}
		//Engine->m_UiMgr->uiTagChange->OpenTagChange();
	}

	if (hiddenWindow) Sleep(1000 / 60); //f12 basinca cpu d���rme

	if (!sendHooked) 
	{
		sendHooked = true;
		Engine->m_UiMgr = new CUIManager();
		Engine->m_UiMgr->Init(pDevice);
		Engine->InitSendHook();
		Engine->InitRecvHook();
		Engine->InitSetString();

		// Skill ID araliklari: 101xxx-115xxx (Karus), 201xxx-215xxx (ElMorad), 490xxx-510xxx (Item/System)
		uint32 skillRanges[][2] = {
			{101001, 115999}, {201001, 215999}, {301001, 315999},
			{401001, 415999}, {490001, 510999}, {600001, 610159}
		};
		for (auto& range : skillRanges) {
			for (uint32 i = range[0]; i <= range[1]; i++) {
				CSpell* spell = GetSkillBase(i);
				if (spell) {
					Engine->skillmap.insert({ spell->dwID, *spell });
					SpellCRC crc(crc32((uint8*)(DWORD)spell, 0xA8, -1), crc32((uint8*)((DWORD)spell + 0xB4), 0x2C, -1));
					Engine->skillcrc.insert({ spell->dwID + 2031, crc });
					Engine->skillmapBackup.insert({ spell->dwID, *spell });
				}
			}
		}
	}

	if (reset)
	{
		reset = false;
	
		if (Engine->uiTaskbarMain) Engine->uiTaskbarMain->UpdatePosition();
	}


	if (!Engine->drawMode || Engine->Loading == true)
		return oEndScene(pDevice);

	if (Engine->uiLogin == NULL)
	{
		Engine->uiLogin = new CUILogin();
		if (Engine->uiLogin->m_bGroupLogin == NULL)
			Engine->uiLogin = NULL;
	}

	//if (Engine->uiHpMenuPlug != NULL
	//	&& !Engine->IsVisible(Engine->uiHpMenuPlug->m_dVTableAddr)
	//		&& Engine->IsVisible(Engine->uiHPBarPlug->m_dVTableAddr))
	//		Engine->uiHpMenuPlug->OpenTopLeft();

	//if (Engine->uiHpMenuPlug != NULL
	//	&& Engine->IsVisible(Engine->uiHpMenuPlug->m_dVTableAddr)
	//	&& !Engine->IsVisible(Engine->uiHPBarPlug->m_dVTableAddr))
	//	Engine->SetVisible(Engine->uiHpMenuPlug->m_dVTableAddr, false);

	/*if (Engine->uiAnvil != NULL)
		Engine->uiAnvil->Tick();*/

	if (Engine->uiSeedHelperPlug != NULL)
		Engine->uiSeedHelperPlug->Tick();

	//if (Engine->uiPieceChangePlug != NULL)
	//	Engine->uiPieceChangePlug->Tick();

	if (Engine->uiState != NULL)
		Engine->uiState->Tick();

	if (Engine->uiTradePrice != NULL)
		Engine->uiTradePrice->Tick();

	if (Engine->uiSkillPage != NULL)
		Engine->uiSkillPage->Tick();

	if (Engine->uiScoreBoard) Engine->uiScoreBoard->Tick();

	if (Engine->m_UiMgr != NULL) {
		Engine->m_UiMgr->Tick();
		Engine->m_UiMgr->Render();
	}

	if (GetAsyncKeyState(VK_F1) & 1)
	{

		//Engine->m_UiMgr->uiTopLeft->FPS();
	}
	if (Engine->uiLottery != NULL)
		Engine->uiLottery->Tick();

	if (Engine->uiCindirella != NULL)
		Engine->uiCindirella->Tick();

	if (Engine->uiHPBarPlug != NULL)
		Engine->uiHPBarPlug->Tick();

	if (Engine->uiWheel != NULL)
		Engine->uiWheel->Tick();

	if (Engine->IsCRActive && Engine->uiCollection != NULL)
		Engine->uiCollection->Tick();

	if (GetTickCount64() > thtime) {
		if (Engine->MainThread) ResumeThread(Engine->MainThread);
		if (Engine->AliveThread) ResumeThread(Engine->AliveThread);
		if (Engine->ScanThread) ResumeThread(Engine->ScanThread);
		if (Engine->TitleThread) ResumeThread(Engine->TitleThread);
		thtime = GetTickCount64() + 1000;
	}

	//if (Engine->m_UiMgr->uixDailyQuest != NULL)
	//	Engine->m_UiMgr->uixDailyQuest->Tick();
	
	if (Engine->uiEventShowList != NULL)
		Engine->uiEventShowList->Tick();

	if (Engine->uiQuestPage != NULL)
		Engine->uiQuestPage->Tick();

	if (Engine->uiGenieMain != NULL)
		Engine->uiGenieMain->Tick();

	return oEndScene(pDevice);
}

LRESULT CALLBACK MsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void DX_Init(DWORD * table)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, xorstr("DX"), NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow(xorstr("DX"), NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), NULL, wc.hInstance, NULL);
	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	LPDIRECT3DDEVICE9 pd3dDevice;
	itsMe = true;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &d3dpp, &pd3dDevice);
	itsMe = false;
	DWORD * pVTable = (DWORD*)pd3dDevice;
	pVTable = (DWORD*)pVTable[0];

	table[FOX] = pVTable[17];
	table[ES]  = pVTable[42];
	table[DIP] = pVTable[82];
	table[RES] = pVTable[16];

	DestroyWindow(hWnd);
}
DWORD FindDevice(DWORD Len)
{
	DWORD dwObjBase = 0;

	dwObjBase = (DWORD)LoadLibraryA("d3d9.dll");
	while (dwObjBase++ < dwObjBase + Len)
		if ((*(WORD*)(dwObjBase + 0x00)) == 0x06C7 && (*(WORD*)(dwObjBase + 0x06)) == 0x8689 && (*(WORD*)(dwObjBase + 0x0C)) == 0x8689) {
			dwObjBase += 2; break;
		}

	return(dwObjBase);
}

DWORD GetDeviceAddress(int VTableIndex)
{
	PDWORD VTable;
	*(DWORD*)&VTable = *(DWORD*)FindDevice(0x128000);
	return VTable[VTableIndex];
}
DWORD WINAPI InitGUI()
{

	while (GetModuleHandle(xorstr("d3d9.dll")) == NULL) 
	{
		Sleep(250);
	}

	//HookCreateDevice(); //Fps sorunu burdan kaynaklan�yor kapatt�k sorun ��karsa a��caz 24.01.2021

	DX_Init(VTable);

	HOOK(EndScene, VTable[ES]);
	HOOK(Reset, VTable[RES]);

	char buff[50];
	ZeroMemory(buff, 50);
	sprintf_s(buff, "%s Client[%d]", AcsFolderName, GetCurrentProcessId()); //old version client name change	
	oSetScissorRect = (tSetScissorRect)DetourFunction((PBYTE)GetDeviceAddress(59), (PBYTE)SetScissorRect);

	window = NULL;
	for (int _retries = 0; window == NULL && _retries < 100; _retries++) {
		Sleep(100);
		window = FindWindowA(NULL, buff); //old version client name change
	}

	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
	std::memset(&nid, 0, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = window;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_APP + 1;
	nid.hIcon = LoadIcon(GetModuleHandle("CodeGuard"), MAKEINTRESOURCE(IDI_ICON1));
	lstrcpy(nid.szTip, buff);
	Shell_NotifyIcon(NIM_ADD, &nid);
	Shell_NotifyIcon(NIM_SETVERSION, &nid);

	return 0;
}


void UIMain() 
{
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitGUI, NULL, NULL, NULL);
}


BYTE ByteExt(DWORD ulBase)
{
	if (!IsBadReadPtr((VOID*)ulBase, sizeof(BYTE)))
	{
		return(*(BYTE*)(ulBase));
	}
	return 0;
}

const DWORD KO_MULTI_CAP = 0xC7C25C;// 0x00bb4c64;
const DWORD KO_MUTEX = 0xA84C91;// 0x00009DFE01;
DWORD WINAPI MultiPatch()
{
	char buff[50];
	memset(buff, 0, sizeof(buff)); // PG4 (2AntiCheat'ten): baslatilmamis byte kopyalanmasin
	sprintf_s(buff, "%s Client[%d]", AcsFolderName, GetCurrentProcessId());

	memcpy((LPVOID)KO_MULTI_CAP, buff, sizeof(buff)); // PG4: sabit 50 yerine sizeof(buff)

	return 1;
}

void PatchMulti()
{
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MultiPatch, NULL, NULL, NULL);
}

HRESULT WINAPI HookCreateDevice()
{
	IDirect3D9 * device = Direct3DCreate9(D3D_SDK_VERSION);
	if (!device)
		return D3DERR_INVALIDCALL;

	IDirect3D9_vtable = (DWORD*)*(DWORD*)device;
	device->Release();
	DWORD protectFlag;
	if (VirtualProtect(&IDirect3D9_vtable[CREATEDEVICE_VTI], sizeof(DWORD), PAGE_READWRITE, &protectFlag))
	{
		*(DWORD*)&D3DCreateDevice_orig = IDirect3D9_vtable[CREATEDEVICE_VTI];
		*(DWORD*)&IDirect3D9_vtable[CREATEDEVICE_VTI] = (DWORD)D3DCreateDevice_hook;
		if (!VirtualProtect(&IDirect3D9_vtable[CREATEDEVICE_VTI], sizeof(DWORD), protectFlag, &protectFlag))
		{
			return D3DERR_INVALIDCALL;
		}
	}
	else
		return D3DERR_INVALIDCALL;

	return D3D_OK;
}

HRESULT WINAPI D3DCreateDevice_hook(IDirect3D9* Direct3D_Object, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface)
{
	if (!itsMe) 
	{
		if (Engine->m_SettingsMgr == NULL) 
			Engine->m_SettingsMgr = new CSettingsManager();

		pPresentationParameters->Windowed = Engine->m_SettingsMgr->m_iRealFullScreen == 1 ? false : true;
		pPresentationParameters->PresentationInterval = Engine->m_SettingsMgr->m_iVsync == 1 ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
		if (Engine->m_SettingsMgr->m_iVsync == 1)
			pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	}

	return D3DCreateDevice_orig(Direct3D_Object, Adapter, DeviceType, hFocusWindow, BehaviorFlags | D3DCREATE_MULTITHREADED, pPresentationParameters, ppReturnedDeviceInterface);
}
