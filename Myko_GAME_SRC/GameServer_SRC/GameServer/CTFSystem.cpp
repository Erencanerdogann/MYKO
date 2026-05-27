// =========================================================================
// S115 FAZ 13 — Crystal CTF (Capture The Flag)
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// 2 klan zone'da savasir, ortada Crystal var.
// Tasiyan: 5sn buff (basit), oldurulurse yere dusurur (item drop).
// Karsi klanin base'ine teslim = 1 puan.
// 10 puan veya 30dk = mac biter.
//
// MATRIX SP'leri (migration 110): SP_CTF_START/FINISH/FLAG_PICKUP
//
// PG temiz: WIZ_CHAT + standart item paketleri, yeni opcode YOK.
// Simdilik **iskelet** — gercek item drop + zone integration acilis sonrasi
// (KODCU TBL item ekleyince) finalize.
// =========================================================================

#include "stdafx.h"

struct _CTF_MATCH_INFO {
	int32_t ctfID;
	uint8   zoneID;
	uint16  redClanID;
	uint16  blueClanID;
	std::string redName;
	std::string blueName;
	uint8   redScore;
	uint8   blueScore;
	time_t  startTime;
	uint16  durationMinutes;
	bool    finished;
};

static std::vector<_CTF_MATCH_INFO> g_ctfMatches;
static std::recursive_mutex g_ctfLock;

const uint8 CTF_MAX_SCORE = 10;
const uint16 CTF_DEFAULT_DURATION_MIN = 30;

static _CTF_MATCH_INFO* FindCTFByZone(uint8 zoneID)
{
	for (auto& m : g_ctfMatches) {
		if (m.zoneID == zoneID && !m.finished) return &m;
	}
	return nullptr;
}

// Public API — CTF baslat (GM komutu)
int32_t StartCTFMatch(uint8 zoneID, uint16 redClanID, const std::string& redName,
                     uint16 blueClanID, const std::string& blueName)
{
	std::lock_guard<std::recursive_mutex> lock(g_ctfLock);

	// Zaten aktif CTF varsa hata
	if (FindCTFByZone(zoneID) != nullptr) {
		printf("[CTF] Zone %u zaten aktif CTF var\n", zoneID);
		return 0;
	}

	int32_t ctfID = g_DBAgent.CTFStart(zoneID, redClanID, redName, blueClanID, blueName);
	if (ctfID == 0) {
		// RAM-only fallback
		static int32_t s_nextRamCTFID = 5000;
		ctfID = s_nextRamCTFID++;
	}

	_CTF_MATCH_INFO m;
	m.ctfID = ctfID;
	m.zoneID = zoneID;
	m.redClanID = redClanID;
	m.blueClanID = blueClanID;
	m.redName = redName;
	m.blueName = blueName;
	m.redScore = 0;
	m.blueScore = 0;
	m.startTime = UNIXTIME;
	m.durationMinutes = CTF_DEFAULT_DURATION_MIN;
	m.finished = false;
	g_ctfMatches.push_back(m);

	printf("[CTF] Started: ID=%d Zone=%u Red=%s Blue=%s\n",
		ctfID, zoneID, redName.c_str(), blueName.c_str());

	// Zone'a duyuru (basit)
	char buf[256] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[CRYSTAL CTF] %s vs %s basladi! Ortadaki Crystal'i karsi base'e teslim et. "
		"Hedef: %u puan veya %u dk.",
		redName.c_str(), blueName.c_str(), CTF_MAX_SCORE, CTF_DEFAULT_DURATION_MIN);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	return ctfID;
}

// Public API — CTF kapat (GM komutu veya timer)
void FinishCTFMatch(uint8 zoneID, uint16 winnerClanID)
{
	std::lock_guard<std::recursive_mutex> lock(g_ctfLock);
	_CTF_MATCH_INFO* m = FindCTFByZone(zoneID);
	if (m == nullptr) return;

	m->finished = true;
	g_DBAgent.CTFFinish(m->ctfID, winnerClanID, m->redScore, m->blueScore);

	const char* winnerName = "Berabere";
	if (winnerClanID == m->redClanID) winnerName = m->redName.c_str();
	else if (winnerClanID == m->blueClanID) winnerName = m->blueName.c_str();

	char buf[256] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[CRYSTAL CTF] Zone %u biti! Kazanan: %s (%u-%u)",
		zoneID, winnerName, m->redScore, m->blueScore);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	printf("[CTF] Finished: ID=%d Winner=%u\n", m->ctfID, winnerClanID);
}

// Public API — Crystal teslim edildi (gelecekte item handler'dan cagrilir)
void CTFCrystalDelivered(uint8 zoneID, uint16 deliveringClanID, int32_t userID, const std::string& userName)
{
	std::lock_guard<std::recursive_mutex> lock(g_ctfLock);
	_CTF_MATCH_INFO* m = FindCTFByZone(zoneID);
	if (m == nullptr) return;

	if (deliveringClanID == m->redClanID) m->redScore++;
	else if (deliveringClanID == m->blueClanID) m->blueScore++;
	else return;

	g_DBAgent.CTFFlagLog(m->ctfID, userID, userName, "DELIVER");

	char buf[200] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[CTF] %s Crystal'i teslim etti! Skor: %u-%u",
		userName.c_str(), m->redScore, m->blueScore);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	// 10 puan = bitti
	if (m->redScore >= CTF_MAX_SCORE) {
		FinishCTFMatch(zoneID, m->redClanID);
	} else if (m->blueScore >= CTF_MAX_SCORE) {
		FinishCTFMatch(zoneID, m->blueClanID);
	}
}

// Timer — her 60sn cagrilir, 30dk gecen maclari kapat
void CTFTimer()
{
	std::lock_guard<std::recursive_mutex> lock(g_ctfLock);
	for (auto& m : g_ctfMatches) {
		if (m.finished) continue;
		if ((UNIXTIME - m.startTime) >= (m.durationMinutes * 60)) {
			uint16 winnerClanID = 0;
			if (m.redScore > m.blueScore) winnerClanID = m.redClanID;
			else if (m.blueScore > m.redScore) winnerClanID = m.blueClanID;
			FinishCTFMatch(m.zoneID, winnerClanID);
		}
	}
}

// GM komutlari (console)
COMMAND_HANDLER(CGameServerDlg::HandleCTFStartCommand)
{
	if (vargs.size() < 3) {
		printf("Usage: /ctfstart RedClan BlueClan ZoneID\n");
		return true;
	}
	std::string redName = vargs.front(); vargs.pop_front();
	std::string blueName = vargs.front(); vargs.pop_front();
	uint8 zoneID = (uint8)SafeAtoi(vargs.front(), 1, 255);

	CKnights* pRed = nullptr;
	CKnights* pBlue = nullptr;
	g_pMain->m_KnightsArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_KnightsArray) {
		if (itr->second == nullptr) continue;
		if (itr->second->GetName() == redName) pRed = itr->second;
		else if (itr->second->GetName() == blueName) pBlue = itr->second;
	}
	g_pMain->m_KnightsArray.m_lock.unlock();
	if (pRed == nullptr || pBlue == nullptr) {
		printf("[CTF] Klan bulunamadi (Red=%s Blue=%s)\n", redName.c_str(), blueName.c_str());
		return true;
	}
	StartCTFMatch(zoneID, pRed->GetID(), redName, pBlue->GetID(), blueName);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleCTFCloseCommand)
{
	if (vargs.empty()) {
		printf("Usage: /ctfclose ZoneID\n");
		return true;
	}
	uint8 zoneID = (uint8)SafeAtoi(vargs.front(), 1, 255);
	FinishCTFMatch(zoneID, 0);
	return true;
}
