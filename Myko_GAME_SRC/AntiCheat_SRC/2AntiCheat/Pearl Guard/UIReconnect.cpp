/* ===== RECONNECT PARK (Session 25) — Tüm UIReconnect kodu devre dışı =====
 * Tekrar aktif etmek için: C:\temp\MYKO\yedekler\RECONNECT_BACKUP\UIReconnect.cpp
 * ========================================================================= */
#include "stdafx.h"
#include "UIReconnect.h"

#if 0 // RECONNECT PARK — Tüm UIReconnect implementasyonu devre dışı

CUIReconnect::CUIReconnect()
{
	m_btnReconnect = NULL;
	m_btnExit = NULL;
	m_txtTitle = NULL;
	m_txtMsg = NULL;
	m_reconnectTime = 0;
	m_countdown = 0;
	m_lastTickTime = 0;
	m_bActive = false;
}

CUIReconnect::~CUIReconnect()
{
}

bool CUIReconnect::Load(HANDLE hFile)
{
	if (CN3UIBase::Load(hFile) == false) return false;

	m_btnReconnect = (CN3UIButton*)GetChildByID(xorstr("btn_reconnect"));
	m_btnExit = (CN3UIButton*)GetChildByID(xorstr("btn_exit"));
	m_txtTitle = (CN3UIString*)GetChildByID(xorstr("text_title"));
	m_txtMsg = (CN3UIString*)GetChildByID(xorstr("text_msg"));

	SetPos(Engine->m_UiMgr->GetScreenCenter(this).x, Engine->m_UiMgr->GetScreenCenter(this).y);

	return true;
}

bool CUIReconnect::ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg)
{
	if (!IsVisible() || !pSender || dwMsg != UIMSG_BUTTON_CLICK)
		return false;

	if (pSender == m_btnReconnect)
	{
		CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
			((CUIReconnect*)param)->DoReconnect();
			return 0;
		}, this, 0, NULL);
		return true;
	}

	if (pSender == m_btnExit)
	{
		Close();
		TerminateProcess(GetCurrentProcess(), 0);
		return true;
	}

	return false;
}

void CUIReconnect::Open()
{
	if (m_bActive) return;
	m_bActive = true;
	m_countdown = 60;
	m_reconnectTime = GetTickCount64() + 60000;
	m_lastTickTime = GetTickCount64();
	if (m_txtTitle)
		m_txtTitle->SetString(xorstr("Reconnecting to the server"));
	UpdateCountdown();
	SetPos(Engine->m_UiMgr->GetScreenCenter(this).x, Engine->m_UiMgr->GetScreenCenter(this).y);
	SetVisible(true);
}

void CUIReconnect::Close()
{
	m_bActive = false;
	m_countdown = 0;
	m_reconnectTime = 0;
	SetVisible(false);
}

void CUIReconnect::Tick()
{
	if (!m_bActive || !IsVisible()) return;
	ULONGLONG now = GetTickCount64();
	if (now - m_lastTickTime >= 1000)
	{
		m_lastTickTime = now;
		if (m_countdown > 0) m_countdown--;
		UpdateCountdown();
	}
	if (now >= m_reconnectTime && m_reconnectTime > 0)
	{
		m_reconnectTime = 0;
		CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
			CUIReconnect* pUI = (CUIReconnect*)param;
			if (!pUI->DoReconnect())
			{
				pUI->m_countdown = 60;
				pUI->m_reconnectTime = GetTickCount64() + 60000;
				pUI->m_lastTickTime = GetTickCount64();
				pUI->UpdateCountdown();
			}
			return 0;
		}, this, 0, NULL);
	}
}

void CUIReconnect::UpdateCountdown()
{
	if (m_txtMsg)
	{
		std::string msg = string_format(xorstr("Please Wait 00:%02d to automatic connect to server."), m_countdown);
		m_txtMsg->SetString(msg);
	}
}

#define KO_ADDR_NEED_REPORT   0x00F3696A
#define KO_ADDR_IS_RESTARTING 0x00F36E48
#define KO_ADDR_SOCKET_PTR    0x00F368D8
#define KO_ADDR_PROC_CHARSEL  0x00F36908
#define KO_ADDR_PROC_ACTIVE_SET 0x004C6360
typedef void(__thiscall* tSocketDisconnect)(void* pThis);
typedef int(__thiscall* tSocketConnect)(void* pThis, HWND hWnd, const std::string& szIP, DWORD dwPort);
#define KO_ADDR_HWND_BASE 0x00DE22D8
static volatile bool g_bReconnecting = false;
typedef void(__cdecl* tProcActiveSet)(void* pProc);

extern void DCLog(const char* msg);

bool CUIReconnect::DoReconnect()
{
	if (g_bReconnecting) return false;
	g_bReconnecting = true;
	DCLog("DoReconnect: yeni process ile reconnect");
	Close();
	extern HANDLE myMutex;
	if (myMutex) { ReleaseMutex(myMutex); CloseHandle(myMutex); myMutex = NULL; }
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::string cmdLine = std::string(exePath) + " " + std::to_string(GetCurrentProcessId());
	STARTUPINFOA si = {}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	if (CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{ CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
	else { g_bReconnecting = false; return false; }
	*(bool*)KO_ADDR_NEED_REPORT = true;
	TerminateProcess(GetCurrentProcess(), 0);
	return true;
}

#endif // RECONNECT PARK
