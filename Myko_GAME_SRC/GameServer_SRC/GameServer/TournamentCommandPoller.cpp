// =========================================================================
// S115 — Tournament Command Poller (GM Web Manuel Kontrol)
// Yazan: CHIP | Tarih: 2026-05-28
// =========================================================================
// WEB GM Panel _MK_TOURNAMENT_COMMAND tablosuna komut INSERT eder
// (SP_TOURNAMENT_CMD_INSERT). GameServer her 2sn'de bir PENDING komutlari
// pull eder (SP_TOURNAMENT_CMD_NEXT_PENDING), execute eder, sonucu yazar
// (SP_TOURNAMENT_CMD_UPDATE_RESULT).
//
// Komut tipleri (11 adet, Params pipe-separated):
//   TOURNAMENT_START    -> Red|Blue|Zone|Duration -> g_pMain->HandleTournamentStart
//   TOURNAMENT_CLOSE    -> Zone -> g_pMain->HandleTournamentClose
//   BRACKET_CREATE      -> Name|MaxClans -> CreateBracket
//   BRACKET_REG         -> BracketID|ClanName -> g_DBAgent.BracketRegister
//   BRACKET_START       -> BracketID -> StartBracket
//   BRACKET_CANCEL      -> BracketID -> CancelBracket
//   CTF_START           -> Red|Blue|Zone -> StartCTFMatch
//   CTF_CLOSE           -> Zone -> FinishCTFMatch
//   ONE_V_ONE_CREATE    -> Name|MaxPlayers -> g_DBAgent.OneVsOneCreate + RAM ekle
//   ONE_V_ONE_START     -> BID -> OneVsOneGenerateMatches + status=STARTED
//   ONE_V_ONE_CANCEL    -> BID -> g_DBAgent.OneVsOneCancel
//
// PG temiz: server-side, opcode YOK.
// =========================================================================

#include "stdafx.h"

// Forward declarations (BracketTournament.cpp + CTFSystem.cpp + others)
extern int32_t CreateBracket(const std::string& name, uint8 maxClans, const std::string& createdByGM, uint8 participantType = 0);
extern bool RegisterClanToBracket(int32_t bracketID, uint16 clanID,
                                   const std::string& clanName, const std::string& leaderName);
extern bool StartBracket(int32_t bracketID);
extern bool CancelBracket(int32_t bracketID);
extern int32_t StartCTFMatch(uint8 zoneID, uint16 redClanID, const std::string& redName,
                              uint16 blueClanID, const std::string& blueName);
extern void FinishCTFMatch(uint8 zoneID, uint16 winnerClanID);
extern void LoadOneVsOneBracketsFromDB();
extern bool AddOneVsOneBracketToRAM(int32_t bid, const std::string& name,
                                     uint8 maxPlayers, const std::string& createdBy);
extern bool StartOneVsOneBracketRAM(int32_t bid);

// Bahis config + kontrol (TournamentBet.cpp)
extern void OpenTournamentBets(uint8 zoneID);
extern void CancelTournamentBets(uint8 zoneID);
extern void SetBetLimits(uint32 minAmt, uint32 maxAmt);
extern void SetBetWindow(time_t sec);
extern void SetBetCommission(uint8 pct);
extern void GetBetConfig(uint32& minAmt, uint32& maxAmt, time_t& win, uint8& comm);
extern void ForceCloseBetWindow(uint8 zoneID);  // manuel kapatma
extern void ForceOpenBetWindow(uint8 zoneID);   // manuel acma

// Lig (League) kontrol (LeagueTournament.cpp)
extern int32_t CreateLeague(const std::string& name, uint8 maxClans, const std::string& createdByGM, uint8 participantType = 0);
extern bool RegisterClanToLeague(int32_t leagueID, uint16 clanID,
                                 const std::string& clanName, const std::string& leaderName);
extern bool StartLeague(int32_t leagueID);
extern bool CancelLeague(int32_t leagueID);

// =====================================================================
// HELPER — pipe-separated string split
// =====================================================================
static std::vector<std::string> SplitPipe(const std::string& s)
{
	std::vector<std::string> parts;
	size_t start = 0;
	while (start <= s.length()) {
		size_t pipe = s.find('|', start);
		if (pipe == std::string::npos) {
			parts.push_back(s.substr(start));
			break;
		}
		parts.push_back(s.substr(start, pipe - start));
		start = pipe + 1;
	}
	return parts;
}

// Klan adi -> CKnights* (BracketTournament.cpp:166 mantigi)
static CKnights* FindClanByName(const std::string& name)
{
	CKnights* found = nullptr;
	g_pMain->m_KnightsArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_KnightsArray) {
		if (itr->second == nullptr) continue;
		if (itr->second->GetName() == name) {
			found = itr->second;
			break;
		}
	}
	g_pMain->m_KnightsArray.m_lock.unlock();
	return found;
}

// =====================================================================
// COMMAND EXECUTORS — her CommandType icin
// =====================================================================

// Donus: {status, result} pair (PASS: status=EXECUTED, FAIL: status=FAILED)
struct _CMD_RESULT {
	std::string status;
	std::string result;
};

static _CMD_RESULT ExecTournamentStart(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 4) {
		r.status = "FAILED"; r.result = "Params eksik (Red|Blue|Zone|Duration bekleniyor)";
		return r;
	}

	std::string redName = p[0];
	std::string blueName = p[1];
	uint8 zoneID = (uint8)SafeAtoi(p[2], 0, 255);
	uint16 duration = (uint16)SafeAtoi(p[3], 1, 60);

	// Validation
	bool validZone = (zoneID == 77 || zoneID == 78 || zoneID == 96 ||
	                  zoneID == 97 || zoneID == 98 || zoneID == 99);
	if (!validZone) {
		r.status = "FAILED"; r.result = "Zone invalid (77/78/96-99 olmali)";
		return r;
	}
	if (duration < 1 || duration > 60) {
		r.status = "FAILED"; r.result = "Duration 1-60 dk olmali";
		return r;
	}
	if (g_pMain->m_ClanVsDataList.GetData(zoneID) != nullptr) {
		r.status = "FAILED"; r.result = "Zone zaten aktif tournament icinde";
		return r;
	}

	CKnights* pRed = FindClanByName(redName);
	CKnights* pBlue = FindClanByName(blueName);
	if (pRed == nullptr) { r.status = "FAILED"; r.result = "Red klan bulunamadi: " + redName; return r; }
	if (pBlue == nullptr) { r.status = "FAILED"; r.result = "Blue klan bulunamadi: " + blueName; return r; }

	// HandleTournamentStart mantigi (ChatHandler.cpp:1369) — _TOURNAMENT_DATA olustur
	_TOURNAMENT_DATA* pData = new _TOURNAMENT_DATA();
	pData->aTournamentZoneID         = zoneID;
	pData->aTournamentClanNum[0]     = pRed->GetID();
	pData->aTournamentClanNum[1]     = pBlue->GetID();
	pData->aTournamentTimer          = (uint32)duration * 60;
	pData->aTournamentisAttackable   = true;
	pData->aTournamentisStarted      = true;
	pData->aTournamentisFinished     = false;
	pData->bracketMatchID            = 0;  // standalone (bracket'a bagli degil)

	std::string startedBy = "web_gm";
	pData->dbTournamentID = g_DBAgent.TournamentLogStart(
		zoneID, pRed->GetID(), pBlue->GetID(),
		pRed->GetName(), pBlue->GetName(), duration, startedBy);

	if (!g_pMain->m_ClanVsDataList.PutData(zoneID, pData)) {
		delete pData;
		r.status = "FAILED"; r.result = "PutData fail (zone meşgul?)";
		return r;
	}

	extern void OpenTournamentBets(uint8 zoneID);
	OpenTournamentBets(zoneID);

	char buf[256] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"Tournament basladi: %s vs %s @ Zone %u (%u dk, dbTID=%d)",
		pRed->GetName().c_str(), pBlue->GetName().c_str(),
		zoneID, duration, pData->dbTournamentID);
	r.status = "EXECUTED"; r.result = buf;

	// Server-wide duyuru (oyuncu gorur — TR/EN)
	char ann[260] = {0};
	_snprintf_s(ann, sizeof(ann), _TRUNCATE,
		"[TURNUVA BASLADI / MATCH STARTED] %s vs %s @ Zone %u (%u dk/min)!",
		pRed->GetName().c_str(), pBlue->GetName().c_str(), zoneID, duration);
	std::string msg = ann;
	g_pMain->SendNotice(msg.c_str());

	return r;
}

static _CMD_RESULT ExecTournamentClose(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Zone parametre eksik"; return r; }
	uint8 zoneID = (uint8)SafeAtoi(p[0], 0, 255);

	_TOURNAMENT_DATA* tInfo = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (tInfo == nullptr) {
		r.status = "FAILED"; r.result = "Zone'da aktif tournament yok";
		return r;
	}

	// Bet iade + DB log finish + cleanup (ChatHandler::HandleTournamentClose mantigi)
	extern void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID);
	ResolveTournamentBets(zoneID, 0);  // winner=0 -> iade

	if (tInfo->dbTournamentID > 0) {
		// Manuel kapanis — skor 0-0, monument 0, winner 0 (iptal/iade)
		g_DBAgent.TournamentLogFinish(tInfo->dbTournamentID,
			/*redScore*/0, /*blueScore*/0,
			/*monumentKilled*/0, /*winnerClanID*/0);
	}

	g_pMain->KickOutZoneUsers(zoneID, ZONE_MORADON, (uint8)Nation::ALL);
	g_pMain->m_ClanVsDataList.DeleteData(zoneID);

	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Tournament Zone %u kapatildi (manuel)", zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBracketCreate(const std::string& params, const std::string& gmNick)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: Name|MaxClans"; return r; }
	std::string name = p[0];
	uint8 maxClans = (uint8)SafeAtoi(p[1], 4, 64);

	int32_t bid = CreateBracket(name, maxClans, gmNick);
	if (bid <= 0) {
		r.status = "FAILED"; r.result = "CreateBracket DB hata";
		return r;
	}
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bracket olusturuldu: BID=%d Name=%s MaxClans=%u",
		bid, name.c_str(), maxClans);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBracketReg(const std::string& params, const std::string& gmNick)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: BracketID|ClanName"; return r; }
	int32_t bracketID = SafeAtoi(p[0], 0, 0x7FFFFFFF);
	std::string clanName = p[1];

	CKnights* pClan = FindClanByName(clanName);
	if (pClan == nullptr) { r.status = "FAILED"; r.result = "Klan yok: " + clanName; return r; }

	// RegisterClanToBracket helper'i mevcut: RAM bracket kontrol + status REGISTRATION check + DB INSERT
	bool ok = RegisterClanToBracket(bracketID, pClan->GetID(), clanName, gmNick);
	if (!ok) {
		r.status = "FAILED";
		r.result = "BracketRegister fail (bracket yok/REGISTRATION disi/DB hata)";
		return r;
	}

	char buf[200] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bracket %d'e %s klani kayit oldu (manuel GM)",
		bracketID, clanName.c_str());
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBracketStart(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: BracketID"; return r; }
	int32_t bid = SafeAtoi(p[0], 0, 0x7FFFFFFF);

	if (!StartBracket(bid)) { r.status = "FAILED"; r.result = "StartBracket hata"; return r; }
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bracket %d basladi (AutoStartTimer maclari otomatik tetikleyecek)", bid);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBracketCancel(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: BracketID"; return r; }
	int32_t bid = SafeAtoi(p[0], 0, 0x7FFFFFFF);

	if (!CancelBracket(bid)) { r.status = "FAILED"; r.result = "CancelBracket hata"; return r; }
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bracket %d iptal edildi", bid);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecCtfStart(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 3) { r.status = "FAILED"; r.result = "Params: Red|Blue|Zone"; return r; }
	std::string redName = p[0];
	std::string blueName = p[1];
	uint8 zoneID = (uint8)SafeAtoi(p[2], 0, 255);

	CKnights* pRed = FindClanByName(redName);
	CKnights* pBlue = FindClanByName(blueName);
	if (pRed == nullptr || pBlue == nullptr) {
		r.status = "FAILED"; r.result = "Klan yok (Red veya Blue)";
		return r;
	}

	int32_t ctfID = StartCTFMatch(zoneID, pRed->GetID(), redName, pBlue->GetID(), blueName);
	if (ctfID <= 0) { r.status = "FAILED"; r.result = "StartCTFMatch hata"; return r; }
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "CTF basladi: CTFID=%d Zone=%u", ctfID, zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecCtfClose(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: Zone"; return r; }
	uint8 zoneID = (uint8)SafeAtoi(p[0], 0, 255);

	FinishCTFMatch(zoneID, 0);  // winner=0 -> beraber
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "CTF Zone %u kapatildi", zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecOneVsOneCreate(const std::string& params, const std::string& gmNick)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: Name|MaxPlayers"; return r; }
	std::string name = p[0];
	uint8 maxPlayers = (uint8)SafeAtoi(p[1], 16, 64);

	if (maxPlayers != 16 && maxPlayers != 32 && maxPlayers != 64) {
		r.status = "FAILED"; r.result = "MaxPlayers 16/32/64 olmali";
		return r;
	}

	int32_t bid = g_DBAgent.OneVsOneCreate(name, maxPlayers, gmNick);
	if (bid <= 0) { r.status = "FAILED"; r.result = "OneVsOneCreate DB hata"; return r; }

	// Sadece bu BID'i RAM'e ekle (LoadOneVsOneBracketsFromDB komple reload yerine —
	// mevcut ACTIVE maclar bozulmaz, startTime sifirlanma sorununu onler)
	AddOneVsOneBracketToRAM(bid, name, maxPlayers, gmNick);

	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "1v1 Bracket olusturuldu: BID=%d Name=%s Max=%u",
		bid, name.c_str(), maxPlayers);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecOneVsOneStart(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: BID"; return r; }
	int32_t bid = SafeAtoi(p[0], 0, 0x7FFFFFFF);

	if (!g_DBAgent.OneVsOneGenerateMatches(bid)) {
		r.status = "FAILED"; r.result = "1v1 GenerateMatches hata (oyuncu sayisi yetersiz?)";
		return r;
	}

	// Sadece bu BID'i STARTED + matches refresh (komple reload yerine)
	if (!StartOneVsOneBracketRAM(bid)) {
		r.status = "FAILED"; r.result = "1v1 RAM'de bracket yok (Create + Start arasinda restart oldu mu?)";
		return r;
	}

	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "1v1 Bracket %d basladi (AutoStartTimer maclari teleport eder)", bid);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecOneVsOneCancel(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: BID"; return r; }
	int32_t bid = SafeAtoi(p[0], 0, 0x7FFFFFFF);

	if (!g_DBAgent.OneVsOneCancel(bid)) {
		r.status = "FAILED"; r.result = "OneVsOneCancel hata";
		return r;
	}
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "1v1 Bracket %d iptal edildi", bid);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// =====================================================================
// BAHIS GM WEB KONTROL — open/close/cancel + config (S115 ekonomi)
// =====================================================================

static _CMD_RESULT ExecBetOpen(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: Zone"; return r; }
	uint8 zoneID = (uint8)SafeAtoi(p[0], 1, 255);

	_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (info == nullptr || !info->aTournamentisStarted) {
		r.status = "FAILED"; r.result = "Zone'da aktif tournament yok (once tournament basla)";
		return r;
	}
	ForceOpenBetWindow(zoneID);
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bahis penceresi Zone %u acildi", zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBetClose(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: Zone"; return r; }
	uint8 zoneID = (uint8)SafeAtoi(p[0], 1, 255);
	ForceCloseBetWindow(zoneID);
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bahis penceresi Zone %u kapatildi", zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBetCancel(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: Zone"; return r; }
	uint8 zoneID = (uint8)SafeAtoi(p[0], 1, 255);
	CancelTournamentBets(zoneID);
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bahisler Zone %u iptal + iade edildi", zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBetSetCommission(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: Pct (0-30)"; return r; }
	uint8 pct = (uint8)SafeAtoi(p[0], 0, 30);
	SetBetCommission(pct);
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bahis komisyonu %%%u ayarlandi (sink/para eritme)", pct);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBetSetLimits(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: Min|Max"; return r; }
	uint32 mn = (uint32)SafeAtoi(p[0], 1000, 0x7FFFFFFF);
	uint32 mx = (uint32)SafeAtoi(p[1], mn, 0x7FFFFFFF);
	SetBetLimits(mn, mx);
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bahis limit min=%u max=%u ayarlandi", mn, mx);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecBetSetWindow(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: Sec (30-600)"; return r; }
	time_t sec = (time_t)SafeAtoi(p[0], 30, 600);
	SetBetWindow(sec);
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Bahis penceresi %llds ayarlandi", (long long)sec);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// =====================================================================
// LIG GM WEB KONTROL (S115)
// =====================================================================
static _CMD_RESULT ExecLeagueCreate(const std::string& params, const std::string& gmNick)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: Name|MaxClans"; return r; }
	std::string name = p[0];
	uint8 maxClans = (uint8)SafeAtoi(p[1], 3, 8);

	int32_t lid = CreateLeague(name, maxClans, gmNick);
	if (lid <= 0) { r.status = "FAILED"; r.result = "CreateLeague DB hata (maxClans 3-8?)"; return r; }
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Lig olusturuldu: LeagueID=%d Name=%s MaxClans=%u",
		lid, name.c_str(), maxClans);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecLeagueReg(const std::string& params, const std::string& gmNick)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: LeagueID|ClanName"; return r; }
	int32_t lid = SafeAtoi(p[0], 0, 0x7FFFFFFF);
	std::string clanName = p[1];

	CKnights* pClan = FindClanByName(clanName);
	if (pClan == nullptr) { r.status = "FAILED"; r.result = "Klan yok: " + clanName; return r; }

	bool ok = RegisterClanToLeague(lid, pClan->GetID(), clanName, gmNick);
	if (!ok) { r.status = "FAILED"; r.result = "LeagueRegister fail (lig yok/kayit kapali/dolu/zaten)"; return r; }
	char buf[160] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Lig %d'e %s klani kayit oldu", lid, clanName.c_str());
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecLeagueStart(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: LeagueID"; return r; }
	int32_t lid = SafeAtoi(p[0], 0, 0x7FFFFFFF);
	if (!StartLeague(lid)) { r.status = "FAILED"; r.result = "StartLeague hata (kayit yetersiz?)"; return r; }
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Lig %d basladi (fikstur olustu, maclar otomatik baslar)", lid);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

static _CMD_RESULT ExecLeagueCancel(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty()) { r.status = "FAILED"; r.result = "Params: LeagueID"; return r; }
	int32_t lid = SafeAtoi(p[0], 0, 0x7FFFFFFF);
	if (!CancelLeague(lid)) { r.status = "FAILED"; r.result = "CancelLeague hata"; return r; }
	char buf[128] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Lig %d iptal edildi", lid);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// =====================================================================
// DUEL — GM iki oyuncuyu zone'a teleport (basit teke tek, eleme degil)
// Params: PlayerA|PlayerB|Zone (Zone default 48 Arena)
// =====================================================================
static _CMD_RESULT ExecDuelStart(const std::string& params)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 2) { r.status = "FAILED"; r.result = "Params: PlayerA|PlayerB|[Zone]"; return r; }
	std::string nameA = p[0];
	std::string nameB = p[1];
	uint8 zoneID = (p.size() >= 3) ? (uint8)SafeAtoi(p[2], 1, 255) : 48;  // default Arena 48

	CUser* pA = g_pMain->GetUserPtr(nameA, NameType::TYPE_CHARACTER);
	CUser* pB = g_pMain->GetUserPtr(nameB, NameType::TYPE_CHARACTER);
	if (pA == nullptr || !pA->isInGame()) { r.status = "FAILED"; r.result = "Oyuncu A offline/yok: " + nameA; return r; }
	if (pB == nullptr || !pB->isInGame()) { r.status = "FAILED"; r.result = "Oyuncu B offline/yok: " + nameB; return r; }
	if (pA == pB) { r.status = "FAILED"; r.result = "Ayni oyuncu secilemez"; return r; }
	if (pA->isDead() || pB->isDead()) { r.status = "FAILED"; r.result = "Olu oyuncu duel yapamaz"; return r; }
	if (pA->isTrading() || pB->isTrading()) { r.status = "FAILED"; r.result = "Ticaret durumunda duel yok"; return r; }

	// Arena pattern teleport (1v1 ile ayni — karsi karsiya)
	pA->ZoneChange(zoneID, 135.0f, 115.0f, -1, false);
	pB->ZoneChange(zoneID, 120.0f, 115.0f, -1, false);

	// Server-wide duyuru (TR/EN)
	char ann[260] = {0};
	_snprintf_s(ann, sizeof(ann), _TRUNCATE,
		"[DUELLO / DUEL] %s vs %s @ Zone %u! Teke tek dovus / 1v1 fight!",
		pA->GetName().c_str(), pB->GetName().c_str(), zoneID);
	std::string msg = ann;
	g_pMain->SendNotice(msg.c_str());

	char buf[160] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Duel basladi: %s vs %s @ Zone %u",
		nameA.c_str(), nameB.c_str(), zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// S115 — GM web PARTY VS PARTY duello baslat
// Params: RedLiderKarakter|BlueLiderKarakter|Zone|Dakika
//   GM iki party liderinin KARAKTER adini verir -> party ID'leri bulunur -> StartPartyVsMatch
static _CMD_RESULT ExecPartyVs(const std::string& params, const std::string& requestedBy)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 4) {
		r.status = "FAILED"; r.result = "Params: RedLider|BlueLider|Zone(96-99/77/78)|Dakika(1-60)";
		return r;
	}
	std::string redLeader  = p[0];
	std::string blueLeader = p[1];
	uint8  zoneID   = (uint8)SafeAtoi(p[2], 1, 255);
	uint16 duration = (uint16)SafeAtoi(p[3], 1, 60);

	CUser* pRL = g_pMain->GetUserPtr(redLeader, NameType::TYPE_CHARACTER);
	CUser* pBL = g_pMain->GetUserPtr(blueLeader, NameType::TYPE_CHARACTER);
	if (pRL == nullptr || !pRL->isInGame()) { r.status = "FAILED"; r.result = "Red lider offline/yok: " + redLeader; return r; }
	if (pBL == nullptr || !pBL->isInGame()) { r.status = "FAILED"; r.result = "Blue lider offline/yok: " + blueLeader; return r; }
	if (!pRL->isInParty()) { r.status = "FAILED"; r.result = redLeader + " party'de degil"; return r; }
	if (!pBL->isInParty()) { r.status = "FAILED"; r.result = blueLeader + " party'de degil"; return r; }

	uint16 redPartyID  = (uint16)pRL->GetPartyID();
	uint16 bluePartyID = (uint16)pBL->GetPartyID();
	if (redPartyID == bluePartyID) { r.status = "FAILED"; r.result = "Ikisi de ayni party'de"; return r; }

	extern bool StartPartyVsMatch(uint8 zoneID, uint16 redPartyID, uint16 bluePartyID,
	                              uint16 durationMin, const std::string& startedBy);
	if (!StartPartyVsMatch(zoneID, redPartyID, bluePartyID, duration, requestedBy)) {
		r.status = "FAILED"; r.result = "Baslatilamadi (zone dolu veya party yok)";
		return r;
	}
	char buf[200] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"Party VS basladi: %s vs %s @ Zone %u %u dk", redLeader.c_str(), blueLeader.c_str(), zoneID, duration);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// S115 — GM web manuel odul (kazanana elle odul ver)
// Params: rewardType|targetType|targetName|amount|[itemID]|[itemCount]
//   rewardType: noah|np|item    targetType: klan|lider|oyuncu
//   noah/np -> amount kullanilir   item -> itemID + itemCount kullanilir
//   Ornek: noah|klan|RedClan|5000000   |   item|oyuncu|Ahmet|0|379010|1
static _CMD_RESULT ExecTournamentReward(const std::string& params, const std::string& requestedBy)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 4) {
		r.status = "FAILED";
		r.result = "Params: rewardType(noah/np/item)|targetType(klan/lider/oyuncu)|Ad|amount|[itemID]|[itemCount]";
		return r;
	}

	std::string sType   = p[0];
	std::string sTarget = p[1];
	std::string sName   = p[2];

	uint8 rewardType = 0;
	if (sType == "item") rewardType = 1;
	else if (sType == "np") rewardType = 2;

	uint8 targetType = 0;
	if (sTarget == "lider") targetType = 1;
	else if (sTarget == "oyuncu") targetType = 2;

	uint16 clanID = 0;
	std::string charName;
	if (targetType == 2) {
		charName = sName;
	} else {
		CKnights* pClan = FindClanByName(sName);
		if (pClan == nullptr) { r.status = "FAILED"; r.result = "Klan bulunamadi: " + sName; return r; }
		clanID = pClan->GetID();
	}

	uint32 amount = (uint32)SafeAtoi(p[3], 0, COIN_MAX);
	uint32 itemID = 0;
	uint16 itemCount = 1;
	if (rewardType == 1) {
		if (p.size() < 6) { r.status = "FAILED"; r.result = "Item icin: ...|itemID|itemCount"; return r; }
		itemID = (uint32)SafeAtoi(p[4], 0, 0x7FFFFFFF);
		itemCount = (uint16)SafeAtoi(p[5], 1, 9999);
		if (itemID == 0) { r.status = "FAILED"; r.result = "Gecersiz itemID"; return r; }
	} else {
		if (amount == 0) { r.status = "FAILED"; r.result = "Gecersiz miktar"; return r; }
	}

	extern int GMGiveTournamentReward(uint8 rewardType, uint8 targetType,
	                                  uint16 targetClanID, const std::string& targetName,
	                                  uint32 amount, uint32 itemID, uint16 itemCount,
	                                  const std::string& gmName);
	int given = GMGiveTournamentReward(rewardType, targetType, clanID, charName,
	                                   amount, itemID, itemCount, requestedBy);

	if (given == 0) {
		r.status = "FAILED";
		r.result = "Hedef online degil / bulunamadi (0 kisiye verildi)";
		return r;
	}
	char buf[200] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"Odul verildi: tip=%s hedef=%s -> %d kisi", sType.c_str(), sTarget.c_str(), given);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// S115 — GM web ZAMANLANMIS EVENT (KAYIT->BAHIS->MAC asamali)
// Params: RedLider|BlueLider|Zone|regDk|betDk|macDk
static _CMD_RESULT ExecEventCreate(const std::string& params, const std::string& requestedBy)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.size() < 3) { r.status = "FAILED"; r.result = "Params: RedLider|BlueLider|Zone|[regDk]|[betDk]|[macDk]"; return r; }
	std::string redLeader = p[0], blueLeader = p[1];
	uint8 zoneID = (uint8)SafeAtoi(p[2], 1, 255);
	uint32 regSec   = (p.size() >= 4) ? (uint32)SafeAtoi(p[3], 1, 120) * 60 : 0;
	uint32 betSec   = (p.size() >= 5) ? (uint32)SafeAtoi(p[4], 1, 120) * 60 : 0;
	uint32 matchSec = (p.size() >= 6) ? (uint32)SafeAtoi(p[5], 1, 120) * 60 : 0;

	CUser* pRL = g_pMain->GetUserPtr(redLeader, NameType::TYPE_CHARACTER);
	CUser* pBL = g_pMain->GetUserPtr(blueLeader, NameType::TYPE_CHARACTER);
	if (pRL == nullptr || !pRL->isInGame()) { r.status = "FAILED"; r.result = "Red lider offline/yok: " + redLeader; return r; }
	if (pBL == nullptr || !pBL->isInGame()) { r.status = "FAILED"; r.result = "Blue lider offline/yok: " + blueLeader; return r; }
	if (!pRL->isInParty()) { r.status = "FAILED"; r.result = redLeader + " party'de degil"; return r; }
	if (!pBL->isInParty()) { r.status = "FAILED"; r.result = blueLeader + " party'de degil"; return r; }
	uint16 redPartyID = (uint16)pRL->GetPartyID(), bluePartyID = (uint16)pBL->GetPartyID();
	if (redPartyID == bluePartyID) { r.status = "FAILED"; r.result = "Ikisi de ayni party'de"; return r; }

	std::string redName = pRL->GetName() + " Party", blueName = pBL->GetName() + " Party";
	extern int32_t CreatePartyVsEvent(uint8, uint16, uint16, const std::string&, const std::string&,
	                                  uint32, uint32, uint32, uint32, const std::string&);
	int32_t eid = CreatePartyVsEvent(zoneID, redPartyID, bluePartyID, redName, blueName,
	                                 regSec, betSec, 0, matchSec, requestedBy);
	if (eid <= 0) { r.status = "FAILED"; r.result = "Event olusturulamadi (zone dolu/event var)"; return r; }
	char buf[160] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Event basladi: id=%d %s vs %s @ Zone %u (KAYIT->BAHIS->MAC)",
		eid, redLeader.c_str(), blueLeader.c_str(), zoneID);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// S115 — GM web HIZLI DUYURU (server-wide notice, tekrar destegi)
// Params: mesaj|tekrar|aralikSn   (tekrar/aralik opsiyonel; aralik su an anlik, tekrar 1-5)
static _CMD_RESULT ExecNotice(const std::string& params, const std::string& requestedBy)
{
	_CMD_RESULT r;
	auto p = SplitPipe(params);
	if (p.empty() || p[0].empty()) { r.status = "FAILED"; r.result = "Params: mesaj|[tekrar]|[aralikSn]"; return r; }
	std::string msg = p[0];
	int tekrar = (p.size() >= 2) ? (int)SafeAtoi(p[1], 1, 5) : 1;
	// Aralik: poller 2sn'de bir kostugu icin gercek gecikme veremeyiz; tekrar anlik gonderilir.
	// (Gercek aralikli tekrar gerekiyorsa ileride scheduler'a tasinir.)
	for (int i = 0; i < tekrar; i++)
		g_pMain->SendNotice(msg.c_str());
	LOG(LogCategory::LOG_GM, "[GM NOTICE] gm=%s tekrar=%d msg=%s", requestedBy.c_str(), tekrar, msg.c_str());
	char buf[120] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Duyuru gonderildi (%d kez)", tekrar);
	r.status = "EXECUTED"; r.result = buf;
	return r;
}

// =====================================================================
// POLLER — her 2 saniyede bir GameEventMainTimer'dan cagrilir
// =====================================================================
static time_t g_lastCmdPoll = 0;
static const time_t CMD_POLL_INTERVAL = 2;  // 2 saniye

void TournamentCommandPollerTimer()
{
	if (UNIXTIME - g_lastCmdPoll < CMD_POLL_INTERVAL) return;
	g_lastCmdPoll = UNIXTIME;

	std::vector<CDBAgent::_TOURNAMENT_CMD_ROW> commands;
	if (!g_DBAgent.TournamentCommandNextPending(commands)) {
		// DB hata — sessiz gec (115 apply olmamis olabilir)
		return;
	}
	if (commands.empty()) return;

	for (auto& cmd : commands) {
		_CMD_RESULT r;
		r.status = "FAILED"; r.result = "Bilinmeyen CommandType";

		if (cmd.commandType == "TOURNAMENT_START")        r = ExecTournamentStart(cmd.params);
		else if (cmd.commandType == "TOURNAMENT_CLOSE")   r = ExecTournamentClose(cmd.params);
		else if (cmd.commandType == "BRACKET_CREATE")     r = ExecBracketCreate(cmd.params, cmd.requestedBy);
		else if (cmd.commandType == "BRACKET_REG")        r = ExecBracketReg(cmd.params, cmd.requestedBy);
		else if (cmd.commandType == "BRACKET_START")      r = ExecBracketStart(cmd.params);
		else if (cmd.commandType == "BRACKET_CANCEL")     r = ExecBracketCancel(cmd.params);
		else if (cmd.commandType == "CTF_START")          r = ExecCtfStart(cmd.params);
		else if (cmd.commandType == "CTF_CLOSE")          r = ExecCtfClose(cmd.params);
		else if (cmd.commandType == "ONE_V_ONE_CREATE")   r = ExecOneVsOneCreate(cmd.params, cmd.requestedBy);
		else if (cmd.commandType == "ONE_V_ONE_START")    r = ExecOneVsOneStart(cmd.params);
		else if (cmd.commandType == "ONE_V_ONE_CANCEL")   r = ExecOneVsOneCancel(cmd.params);
		// S115 ekonomi — GM web bahis kontrol
		else if (cmd.commandType == "BET_OPEN")           r = ExecBetOpen(cmd.params);
		else if (cmd.commandType == "BET_CLOSE")          r = ExecBetClose(cmd.params);
		else if (cmd.commandType == "BET_CANCEL")         r = ExecBetCancel(cmd.params);
		else if (cmd.commandType == "BET_SET_COMMISSION") r = ExecBetSetCommission(cmd.params);
		else if (cmd.commandType == "BET_SET_LIMITS")     r = ExecBetSetLimits(cmd.params);
		else if (cmd.commandType == "BET_SET_WINDOW")     r = ExecBetSetWindow(cmd.params);
		// S115 lig — GM web lig kontrol
		else if (cmd.commandType == "LEAGUE_CREATE")      r = ExecLeagueCreate(cmd.params, cmd.requestedBy);
		else if (cmd.commandType == "LEAGUE_REG")         r = ExecLeagueReg(cmd.params, cmd.requestedBy);
		else if (cmd.commandType == "LEAGUE_START")       r = ExecLeagueStart(cmd.params);
		else if (cmd.commandType == "LEAGUE_CANCEL")      r = ExecLeagueCancel(cmd.params);
		// S115 — GM web duel (teke tek)
		else if (cmd.commandType == "DUEL_START")         r = ExecDuelStart(cmd.params);
		// S115 — GM web manuel odul (kazanana elle odul)
		else if (cmd.commandType == "TOURNAMENT_REWARD")  r = ExecTournamentReward(cmd.params, cmd.requestedBy);
		// S115 — GM web party vs party duello
		else if (cmd.commandType == "PARTY_VS")           r = ExecPartyVs(cmd.params, cmd.requestedBy);
		// S115 — GM web zamanlanmis event (KAYIT->BAHIS->MAC) + hizli duyuru
		else if (cmd.commandType == "EVENT_CREATE")       r = ExecEventCreate(cmd.params, cmd.requestedBy);
		else if (cmd.commandType == "NOTICE")             r = ExecNotice(cmd.params, cmd.requestedBy);

		// Result trunc (NVARCHAR 500 limit)
		if (r.result.length() > 490) r.result = r.result.substr(0, 490) + "...";

		g_DBAgent.TournamentCommandUpdateResult(cmd.commandID, r.status, r.result);

		printf("[TCMD] #%d %s by %s -> %s: %s\n",
			cmd.commandID, cmd.commandType.c_str(), cmd.requestedBy.c_str(),
			r.status.c_str(), r.result.c_str());
	}
}
