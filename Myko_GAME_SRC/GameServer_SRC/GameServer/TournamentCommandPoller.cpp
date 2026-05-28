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
extern int32_t CreateBracket(const std::string& name, uint8 maxClans, const std::string& createdByGM);
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

	// Server-wide duyuru
	char ann[200] = {0};
	_snprintf_s(ann, sizeof(ann), _TRUNCATE,
		"[GM Tournament] %s vs %s @ Zone %u basladi (%u dk)!",
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

		// Result trunc (NVARCHAR 500 limit)
		if (r.result.length() > 490) r.result = r.result.substr(0, 490) + "...";

		g_DBAgent.TournamentCommandUpdateResult(cmd.commandID, r.status, r.result);

		printf("[TCMD] #%d %s by %s -> %s: %s\n",
			cmd.commandID, cmd.commandType.c_str(), cmd.requestedBy.c_str(),
			r.status.c_str(), r.result.c_str());
	}
}
