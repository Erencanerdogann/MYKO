// =========================================================================
// S115 — League (Lig) Round-Robin Tournament
// Yazan: CHIP | Tarih: 2026-05-28
// =========================================================================
// Bracket'tan FARK: ELEME degil, PUAN-BAZLI (herkes herkesle round-robin).
//   3-8 klan, her klan digerleriyle 1 mac
//   Mac: kazanan +3, beraberlik +1, kaybeden 0 puan
//   Final: en cok puan = sampiyon (esitlik: averaj kill GoalsFor-GoalsAgainst)
//
// Mac motoru: mevcut Tournament (_TOURNAMENT_DATA + leagueMatchID)
// LeagueAutoStartTimer: PENDING maclari sirayla baslatir (her saniye)
// Puan guncelleme: SP_LEAGUE_MATCH_FINISH (DB tarafinda)
//
// MATRIX migration 118: _MK_LEAGUE + _MK_LEAGUE_REG + _MK_LEAGUE_MATCHES + 8 SP
// PG temiz: yeni opcode YOK, mevcut tournament uzerine.
// =========================================================================

#include "stdafx.h"

// =====================================================================
// RAM Cache
// =====================================================================
struct _LEAGUE_MATCH_INFO {
	int32_t matchID;
	int32_t leagueID;
	uint8   round;
	uint8   matchOrder;
	uint16  redClanID;
	uint16  blueClanID;
	std::string redName;
	std::string blueName;
	uint8   zoneID;
	std::string status;       // PENDING/ACTIVE/FINISHED
	bool    finished;
	uint16  winnerClanID;
	time_t  startTime;
};

struct _LEAGUE_INFO {
	int32_t leagueID;
	std::string name;
	uint8   maxClans;
	uint8   currentRound;
	uint8   totalRounds;
	std::string status;       // REGISTRATION/ACTIVE/FINISHED/CANCELLED
	uint16  winnerClanID;
	std::string winnerClanName;
	std::vector<_LEAGUE_MATCH_INFO> matches;
};

static std::vector<_LEAGUE_INFO> g_leagues;
static std::recursive_mutex g_leagueLock;

static const uint16 LEAGUE_MATCH_DURATION_MIN = 7;  // her lig maci 7 dakika

// =====================================================================
// HELPER
// =====================================================================
static _LEAGUE_INFO* FindLeague(int32_t leagueID)
{
	for (auto& l : g_leagues) {
		if (l.leagueID == leagueID) return &l;
	}
	return nullptr;
}

static _LEAGUE_MATCH_INFO* FindLeagueMatch(int32_t matchID)
{
	for (auto& l : g_leagues) {
		for (auto& m : l.matches) {
			if (m.matchID == matchID) return &m;
		}
	}
	return nullptr;
}

static void RefreshLeagueMatches(_LEAGUE_INFO* l)
{
	if (l == nullptr) return;
	std::vector<CDBAgent::_LEAGUE_MATCH_ROW> rows;
	if (!g_DBAgent.LeagueLoadMatches(l->leagueID, rows)) {
		printf("[LEAGUE] RefreshMatches DB hata leagueID=%d\n", l->leagueID);
		return;
	}
	l->matches.clear();
	for (auto& r : rows) {
		_LEAGUE_MATCH_INFO m;
		m.matchID      = r.matchID;
		m.leagueID     = l->leagueID;
		m.round        = r.roundNumber;
		m.matchOrder   = r.matchOrder;
		m.redClanID    = r.redClanID;
		m.blueClanID   = r.blueClanID;
		m.redName      = r.redName;
		m.blueName     = r.blueName;
		m.zoneID       = r.zoneID;
		m.status       = r.status;
		m.winnerClanID = r.winnerClanID;
		m.finished     = (r.status == "FINISHED");
		m.startTime    = 0;
		l->matches.push_back(m);
	}
	printf("[LEAGUE] RefreshMatches leagueID=%d %zu mac yuklendi\n", l->leagueID, l->matches.size());
}

// =====================================================================
// LOAD — server init
// =====================================================================
void LoadLeaguesFromDB()
{
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);
	g_leagues.clear();

	std::vector<CDBAgent::_LEAGUE_INFO_ROW> rows;
	if (!g_DBAgent.LeagueLoadActive(rows)) {
		printf("[LEAGUE] LoadLeaguesFromDB: SELECT hata (tablo yok, MATRIX 118 bekliyor)\n");
		return;
	}
	for (auto& r : rows) {
		_LEAGUE_INFO l;
		l.leagueID = r.leagueID;
		l.name = r.name;
		l.maxClans = r.maxClans;
		l.currentRound = r.currentRound;
		l.totalRounds = r.totalRounds;
		l.status = r.status;
		l.winnerClanID = r.winnerClanID;
		l.winnerClanName = r.winnerClanName;
		g_leagues.push_back(l);
	}
	for (auto& l : g_leagues) RefreshLeagueMatches(&l);
	printf("[LEAGUE] LoadLeaguesFromDB: %zu lig yuklendi\n", g_leagues.size());
}

// =====================================================================
// PUBLIC API — GM komutlari
// =====================================================================
int32_t CreateLeague(const std::string& name, uint8 maxClans, const std::string& createdByGM)
{
	if (maxClans < 3 || maxClans > 8) {
		printf("[LEAGUE] CreateLeague: maxClans 3-8 olmali (got %u)\n", maxClans);
		return 0;
	}
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);

	int32_t leagueID = g_DBAgent.LeagueCreate(name, maxClans, createdByGM);
	if (leagueID == 0) {
		printf("[LEAGUE] CreateLeague DB hata\n");
		return 0;
	}
	_LEAGUE_INFO l;
	l.leagueID = leagueID;
	l.name = name;
	l.maxClans = maxClans;
	l.currentRound = 0;
	l.totalRounds = 0;
	l.status = "REGISTRATION";
	l.winnerClanID = 0;
	g_leagues.push_back(l);

	printf("[LEAGUE] Created: ID=%d Name='%s' MaxClans=%u GM=%s\n",
		leagueID, name.c_str(), maxClans, createdByGM.c_str());
	return leagueID;
}

bool RegisterClanToLeague(int32_t leagueID, uint16 clanID,
                          const std::string& clanName, const std::string& leaderName)
{
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);
	_LEAGUE_INFO* l = FindLeague(leagueID);
	if (l == nullptr) {
		printf("[LEAGUE] Register: leagueID=%d RAM'de yok\n", leagueID);
		return false;
	}
	if (l->status != "REGISTRATION") {
		printf("[LEAGUE] Register: leagueID=%d REGISTRATION disi (%s)\n", leagueID, l->status.c_str());
		return false;
	}
	std::string result;
	bool ok = g_DBAgent.LeagueRegister(leagueID, clanID, clanName, leaderName, result);
	if (!ok) {
		printf("[LEAGUE] Register failed: %s (%s)\n", clanName.c_str(), result.c_str());
		return false;
	}
	printf("[LEAGUE] Registered: leagueID=%d Clan=%s\n", leagueID, clanName.c_str());
	return true;
}

bool StartLeague(int32_t leagueID)
{
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);
	_LEAGUE_INFO* l = FindLeague(leagueID);
	if (l == nullptr) {
		printf("[LEAGUE] Start: leagueID=%d RAM'de yok\n", leagueID);
		return false;
	}
	if (l->status != "REGISTRATION") {
		printf("[LEAGUE] Start: leagueID=%d zaten basladi (%s)\n", leagueID, l->status.c_str());
		return false;
	}

	// DB fikstur olustur (round-robin)
	if (!g_DBAgent.LeagueGenerateFixtures(leagueID)) {
		printf("[LEAGUE] Start: GenerateFixtures DB hata leagueID=%d\n", leagueID);
		return false;
	}

	l->status = "ACTIVE";
	l->currentRound = 1;
	RefreshLeagueMatches(l);

	printf("[LEAGUE] Started: leagueID=%d Round 1, %zu mac RAM (LeagueAutoStartTimer baslatacak)\n",
		leagueID, l->matches.size());
	return true;
}

bool CancelLeague(int32_t leagueID)
{
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);
	_LEAGUE_INFO* l = FindLeague(leagueID);
	if (l == nullptr) return false;

	if (!g_DBAgent.LeagueCancel(leagueID)) {
		printf("[LEAGUE] Cancel DB hata leagueID=%d (RAM iptal)\n", leagueID);
	}
	l->status = "CANCELLED";

	// Aktif maclari kapat
	for (auto& m : l->matches) {
		if (m.status == "ACTIVE") {
			m.status = "FINISHED";
			_TOURNAMENT_DATA* tInfo = g_pMain->m_ClanVsDataList.GetData(m.zoneID);
			if (tInfo != nullptr) {
				g_pMain->KickOutZoneUsers(m.zoneID, ZONE_MORADON, (uint8)Nation::ALL);
				g_pMain->m_ClanVsDataList.DeleteData(m.zoneID);
			}
		}
	}
	printf("[LEAGUE] Cancelled: leagueID=%d\n", leagueID);
	return true;
}

void GetLeagueStatus(int32_t leagueID)
{
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);
	_LEAGUE_INFO* l = FindLeague(leagueID);
	if (l == nullptr) {
		printf("[LEAGUE] Status: leagueID=%d not found\n", leagueID);
		return;
	}
	printf("====== LEAGUE %d STATUS ======\n", leagueID);
	printf("  Name: %s | Status: %s | MaxClans: %u | Round: %u/%u\n",
		l->name.c_str(), l->status.c_str(), l->maxClans, l->currentRound, l->totalRounds);
	printf("  Winner: %u (%s)\n", l->winnerClanID, l->winnerClanName.c_str());
	printf("  Matches: %zu\n", l->matches.size());
	for (auto& m : l->matches) {
		printf("    R%u M%u: %s vs %s @ Zone%u [%s] Winner=%u\n",
			m.round, m.matchOrder, m.redName.c_str(), m.blueName.c_str(),
			m.zoneID, m.status.c_str(), m.winnerClanID);
	}
	printf("==============================\n");
}

// =====================================================================
// HELPER — lig maci icin _TOURNAMENT_DATA olustur (Bracket benzeri)
// =====================================================================
static bool StartLeagueMatchTournament(_LEAGUE_MATCH_INFO& m, const std::string& leagueName)
{
	CKnights* pRed  = g_pMain->GetClanPtr(m.redClanID);
	CKnights* pBlue = g_pMain->GetClanPtr(m.blueClanID);
	if (pRed == nullptr || pBlue == nullptr) {
		printf("[LEAGUE] StartMatch: klan yok (matchID=%d red=%u blue=%u)\n",
			m.matchID, m.redClanID, m.blueClanID);
		return false;
	}

	_TOURNAMENT_DATA* pData = new _TOURNAMENT_DATA();
	pData->aTournamentZoneID         = m.zoneID;
	pData->aTournamentClanNum[0]     = pRed->GetID();
	pData->aTournamentClanNum[1]     = pBlue->GetID();
	pData->aTournamentTimer          = (uint32)LEAGUE_MATCH_DURATION_MIN * 60;
	pData->aTournamentisAttackable   = true;
	pData->aTournamentisStarted      = true;
	pData->aTournamentisFinished     = false;
	pData->leagueMatchID             = m.matchID;  // KRITIK — OnLeagueMatchFinish tetigi

	std::string startedBy = "league_auto";
	pData->dbTournamentID = g_DBAgent.TournamentLogStart(
		m.zoneID, pRed->GetID(), pBlue->GetID(),
		pRed->GetName(), pBlue->GetName(), LEAGUE_MATCH_DURATION_MIN, startedBy);

	if (!g_pMain->m_ClanVsDataList.PutData(m.zoneID, pData)) {
		delete pData;
		printf("[LEAGUE] StartMatch: PutData fail (Zone=%u)\n", m.zoneID);
		return false;
	}

	extern void OpenTournamentBets(uint8 zoneID);
	OpenTournamentBets(m.zoneID);

	const char* zoneName =
		(m.zoneID == 77) ? "Ardream"   : (m.zoneID == 78) ? "Ronark"    :
		(m.zoneID == 96) ? "PartyVs-1" : (m.zoneID == 97) ? "PartyVs-2" :
		(m.zoneID == 98) ? "PartyVs-3" : (m.zoneID == 99) ? "PartyVs-4" : "?";

	char buf[300] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[LIG / LEAGUE %s] Round %u: %s vs %s @ %s (%u dk/min)",
		leagueName.c_str(), m.round, pRed->GetName().c_str(), pBlue->GetName().c_str(),
		zoneName, LEAGUE_MATCH_DURATION_MIN);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	m.status = "ACTIVE";
	m.startTime = UNIXTIME;

	printf("[LEAGUE] AUTO-START: matchID=%d R%u Zone%u %s vs %s\n",
		m.matchID, m.round, m.zoneID, pRed->GetName().c_str(), pBlue->GetName().c_str());
	return true;
}

// =====================================================================
// HOOK — lig maci biti (TournamentSystem.cpp::HandleTournamentEnd'ten)
// =====================================================================
void OnLeagueMatchFinish(int32_t matchID, uint16 winnerClanID, uint16 redScore, uint16 blueScore)
{
	if (matchID <= 0) return;
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);

	// DB puan guncelle (SP_LEAGUE_MATCH_FINISH puan tablosunu da gunceller)
	if (!g_DBAgent.LeagueMatchFinish(matchID, redScore, blueScore, winnerClanID)) {
		printf("[LEAGUE] MatchFinish DB hata matchID=%d\n", matchID);
	}

	_LEAGUE_MATCH_INFO* m = FindLeagueMatch(matchID);
	if (m == nullptr) {
		printf("[LEAGUE] MatchFinish: matchID=%d RAM'de yok\n", matchID);
		return;
	}
	m->status = "FINISHED";
	m->finished = true;
	m->winnerClanID = winnerClanID;

	int32_t cachedLeagueID = m->leagueID;
	m = nullptr;  // RefreshMatches sonrasi invalid

	_LEAGUE_INFO* l = FindLeague(cachedLeagueID);
	if (l == nullptr) return;

	// Tum maclar bitti mi → lig sonu + sampiyon
	// (League round-robin: maclar bagimsiz, round sirasi onemli degil — sadece
	//  TUM maclar FINISHED olunca lig biter)
	bool allFinished = true;
	for (auto& mm : l->matches) {
		if (mm.status != "FINISHED") { allFinished = false; break; }
	}

	if (allFinished && l->status == "ACTIVE") {
		l->status = "FINISHED";
		// Sampiyon: SP_LEAGUE_MATCH_FINISH son macta DB'de WinnerClanID/Name set etti
		// (en cok puan, esitlik averaj). Duyuru icin DB'den cek — basit SELECT.
		// RAM winnerClanName guncel olmayabilir, duyuruda lig adi + standings yonlendir.
		char buf[280] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[LIG BITTI / LEAGUE ENDED] %s tamamlandi! Puan tablosu: +leaguestandings %d | Final standings: +leaguestandings %d",
			l->name.c_str(), l->leagueID, l->leagueID);
		std::string msg = buf;
		g_pMain->SendNotice(msg.c_str());

		printf("[LEAGUE] FINISHED: leagueID=%d (sampiyon DB STANDINGS'te, en cok puan)\n", l->leagueID);
	}
}

// =====================================================================
// TIMER — her saniye GameEventMainTimer'dan (PENDING mac baslatma)
// =====================================================================
void LeagueAutoStartTimer()
{
	std::lock_guard<std::recursive_mutex> lock(g_leagueLock);

	// Klan-yok beraberlik maclari loop disinda islenir (iterator guard + allFinished tetigi)
	std::vector<int32_t> byeMatchIDs;

	for (size_t li = 0; li < g_leagues.size(); li++) {
		_LEAGUE_INFO& l = g_leagues[li];
		if (l.status != "ACTIVE") continue;

		// League round-robin: maclar BAGIMSIZ (eleme degil). Round sirasi zorunlu degil —
		// zone musaitse herhangi PENDING mac baslar. Bu restart-safe (currentRound DB
		// persist sorununu onler) + paralel zone kullanimi (6 zone) verimli.
		for (size_t mi = 0; mi < l.matches.size(); mi++) {
			_LEAGUE_MATCH_INFO& m = l.matches[mi];
			if (m.status != "PENDING") continue;

			// Zone bos mu? (ayni zone'da baska tournament/lig maci varsa bekle)
			if (g_pMain->m_ClanVsDataList.GetData(m.zoneID) != nullptr) continue;

			// Klan var mi?
			CKnights* pRed  = g_pMain->GetClanPtr(m.redClanID);
			CKnights* pBlue = g_pMain->GetClanPtr(m.blueClanID);
			if (pRed == nullptr || pBlue == nullptr) {
				// Klan silinmis — bu maci beraberlik say (0-0), loop sonrasi isle (iterator guard)
				printf("[LEAGUE] Klan yok, mac beraberlik: matchID=%d\n", m.matchID);
				byeMatchIDs.push_back(m.matchID);
				continue;
			}

			StartLeagueMatchTournament(m, l.name);
		}
	}

	// Klan-yok maclar: OnLeagueMatchFinish ile isle (DB puan + RAM FINISHED + allFinished
	// tetigi — son mac bye ise lig FINISHED olur, asili kalmaz). SP Status=FINISHED guard
	// cift islem onler. OnLeagueMatchFinish kendi g_leagueLock'unu alir (recursive ok).
	for (int32_t mid : byeMatchIDs) {
		extern void OnLeagueMatchFinish(int32_t matchID, uint16 winnerClanID, uint16 redScore, uint16 blueScore);
		OnLeagueMatchFinish(mid, 0, 0, 0);
	}
}

// =====================================================================
// CONSOLE KOMUTLARI
// =====================================================================
COMMAND_HANDLER(CGameServerDlg::HandleLeagueCreateCommand)
{
	if (vargs.size() < 2) {
		printf("Usage: /leaguecreate \"Ad\" MaxClans (3-8)\n");
		return true;
	}
	std::string name = vargs.front(); vargs.pop_front();
	uint8 maxClans = (uint8)SafeAtoi(vargs.front(), 3, 8);
	int32_t lid = CreateLeague(name, maxClans, "console");
	if (lid > 0) printf("[LEAGUE] Created: LeagueID=%d\n", lid);
	else printf("[LEAGUE] Create hata\n");
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleLeagueStartCommand)
{
	if (vargs.empty()) { printf("Usage: /leaguestart LeagueID\n"); return true; }
	int32_t lid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);
	if (StartLeague(lid)) printf("[LEAGUE] Started: LeagueID=%d\n", lid);
	else printf("[LEAGUE] Start hata: LeagueID=%d\n", lid);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleLeagueStatusCommand)
{
	if (vargs.empty()) { printf("Usage: /leaguestatus LeagueID\n"); return true; }
	int32_t lid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);
	GetLeagueStatus(lid);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleLeagueCancelCommand)
{
	if (vargs.empty()) { printf("Usage: /leaguecancel LeagueID\n"); return true; }
	int32_t lid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);
	if (CancelLeague(lid)) printf("[LEAGUE] Cancelled: LeagueID=%d\n", lid);
	else printf("[LEAGUE] Cancel hata: LeagueID=%d\n", lid);
	return true;
}

// +leaguereg LeagueID — klan lideri kayit
COMMAND_HANDLER(CUser::HandleLeagueRegCommand)
{
	if (!isInClan()) {
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin. | Must be in a clan.");
		return true;
	}
	if (!isClanLeader()) {
		g_pMain->SendHelpDescription(this, "Sadece klan lideri kayit yapabilir. | Only clan leader can register.");
		return true;
	}
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "+leaguereg LeagueID. Ornek: +leaguereg 1");
		return true;
	}
	int32_t lid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);
	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr) {
		g_pMain->SendHelpDescription(this, "Klan bulunamadi. | Clan not found.");
		return true;
	}

	bool ok = RegisterClanToLeague(lid, pClan->GetID(), pClan->GetName(), GetName());
	char buf[200] = {0};
	if (ok) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[LIG / LEAGUE] %s ligine kayit oldun (Lig %d)! | Registered to league.",
			pClan->GetName().c_str(), lid);
	} else {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[LIG / LEAGUE] Kayit hata (lig yok/kayit kapali/dolu/zaten kayitli). | Register failed.");
	}
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);
	return true;
}

// +leaguestandings LeagueID — oyuncu puan tablosunu chat'te gor (TR/EN)
COMMAND_HANDLER(CUser::HandleLeagueStandingsCommand)
{
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "+leaguestandings LeagueID. Ornek: +leaguestandings 1");
		return true;
	}
	int32_t lid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	std::vector<CDBAgent::_LEAGUE_STANDING_ROW> rows;
	if (!g_DBAgent.LeagueStandings(lid, rows)) {
		g_pMain->SendHelpDescription(this, "Lig bulunamadi veya DB hata. | League not found or DB error.");
		return true;
	}
	if (rows.empty()) {
		g_pMain->SendHelpDescription(this, "Bu ligde kayit yok. | No clans in this league.");
		return true;
	}

	// Header
	char buf[200] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"=== LIG %d PUAN TABLOSU / STANDINGS (O-G-B-M Avr Puan) ===", lid);
	std::string header = buf;
	{ Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &header); Send(&p); }

	// Satirlar (sirali, SP zaten Points DESC)
	int rank = 1;
	for (auto& r : rows) {
		const char* medal = (rank == 1) ? "[1]" : (rank == 2) ? "[2]" : (rank == 3) ? "[3]" : "   ";
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"%s %d. %s: %u-%u-%u-%u Avr%+d PUAN %d",
			medal, rank, r.clanName.c_str(),
			r.played, r.win, r.draw, r.loss, r.goalDiff, r.points);
		std::string line = buf;
		Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &line); Send(&p);
		rank++;
	}
	return true;
}
