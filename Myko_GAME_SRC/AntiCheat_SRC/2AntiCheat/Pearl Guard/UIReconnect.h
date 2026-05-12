#pragma once
class CUIReconnect;
#include "stdafx.h"
#include "Pearl Engine.h"
#include "N3BASE/N3UIBase.h"
#include "N3BASE/N3UIString.h"
#include "N3BASE/N3UIButton.h"

class CUIReconnect : public CN3UIBase
{
public:
	CN3UIButton* m_btnReconnect;
	CN3UIButton* m_btnExit;
	CN3UIString* m_txtTitle;
	CN3UIString* m_txtMsg;

	ULONGLONG m_reconnectTime;	// otomatik reconnect zamani (GetTickCount64)
	int m_countdown;			// kalan saniye (geri sayim)
	ULONGLONG m_lastTickTime;	// son tick zamani (saniye guncelleme)
	bool m_bActive;				// reconnect UI aktif mi

public:
	CUIReconnect();
	~CUIReconnect();
	bool Load(HANDLE hFile);
	bool ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg);
	void Open();
	void Close();
	void Tick();
	void UpdateCountdown();
	bool DoReconnect();
};
