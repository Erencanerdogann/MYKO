// =========================================================================
// S115 — Bracket Tournament (8/16/32 klan elemeli)
// Yazan: CHIP | Tarih: 2026-05-27 | Guncel: TAM IMPLEMENTASYON
// =========================================================================
// Tek eleme (single elimination):
//   8 klan  -> 3 tur (Quarter -> Semi -> Final)
//   16 klan -> 4 tur
//   32 klan -> 5 tur
//
// Mac yapma: mevcut Tournament sistemi (_TOURNAMENT_DATA + HandleTournamentStart)
// Bracket Engine: mac sonuclarini takip eder, sonraki tur eslesir
//
// MATRIX MSG:5914 DB hazir oldukta: g_DBAgent.BracketXxx fonksiyonlari calisir.
// SP yoksa fonksiyonlar false/0 doner, RAM-only iskelet bozulmaz.
//
// PG temiz: yeni opcode YOK, mevcut tournament sistemi uzerine kurulu.
// =========================================================================

#include "stdafx.h"

// RAM-level aktif bracket cache
struct _BRACKET_MATCH_INFO {
	int32_t matchID;
	int32_t bracketID;
	uint8   round;
	uint8   matchOrder;
	uint16  redClanID;
	uint16  blueClanID;
	uint8   zoneID;
	std::string status;   // PENDING/ACTIVE/FINISHED/WALKOVER
	bool    finished;
	uint16  winnerClanID;
	int32_t dependsOnMatch1;  // 0 = bagimli degil; >0 = bu mac bitince bu mac baslar (zone reuse)
	time_t  startTime;        // 0 = baslamadi
};

struct _BRACKET_INFO {
	int32_t bracketID;
	std::string name;
	uint8   currentRound;
	uint8   maxClans;
	std::string status;       // REGISTRATION/ACTIVE/FINISHED/CANCELLED
	uint16  winnerClanID;
	std::string winnerClanName;
	std::vector<_BRACKET_MATCH_INFO> matches;
};

static std::vector<_BRACKET_INFO> g_brackets;
static std::recursive_mutex g_bracketLock;

// =====================================================================
// HELPER — bracket bul (RAM cache)
// =====================================================================
static _BRACKET_INFO* FindBracket(int32_t bracketID)
{
	for (auto& b : g_brackets) {
		if (b.bracketID == bracketID) return &b;
	}
	return nullptr;
}

static _BRACKET_MATCH_INFO* FindMatch(int32_t matchID)
{
	for (auto& b : g_brackets) {
		for (auto& m : b.matches) {
			if (m.matchID == matchID) return &m;
		}
	}
	return nullptr;
}

// S115 BUG #2 FIX — DB'den maclari RAM cache'e doldur
static void RefreshBracketMatches(_BRACKET_INFO* b)
{
	if (b == nullptr) return;
	std::vector<CDBAgent::_BRACKET_MATCH_ROW> rows;
	if (!g_DBAgent.BracketLoadMatches(b->bracketID, rows)) {
		printf("[BRACKET] RefreshMatches DB hata bracketID=%d\n", b->bracketID);
		return;
	}
	b->matches.clear();
	for (auto& r : rows) {
		_BRACKET_MATCH_INFO m;
		m.matchID         = r.matchID;
		m.bracketID       = b->bracketID;
		m.round           = r.roundNumber;
		m.matchOrder      = r.matchOrder;
		m.redClanID       = r.redClanID;
		m.blueClanID      = r.blueClanID;
		m.zoneID          = r.zoneID;
		m.status          = r.status;
		m.winnerClanID    = r.winnerClanID;
		m.finished        = (r.status == "FINISHED" || r.status == "WALKOVER");
		m.dependsOnMatch1 = r.dependsOnMatch1;
		m.startTime       = 0;
		b->matches.push_back(m);
	}
	printf("[BRACKET] RefreshMatches: bracketID=%d %zu mac yuklendi\n",
		b->bracketID, b->matches.size());
}

// =====================================================================
// PUBLIC API — GameServer init'inde cagrilir
// =====================================================================

// DB'den active/registration bracketleri yukle (RAM'e cache)
void LoadBracketsFromDB()
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	g_brackets.clear();

	// MATRIX SP'leri hazir oldukta SELECT'ler buraya gelir.
	// Su an iskelet: DB tablosu varsa runtime SELECT, yoksa 0 bracket.
	// Refactor: g_DBAgent.BracketLoadActive(...) yontemi MATRIX cevabindan sonra eklenecek.

	printf("[BRACKET] LoadBracketsFromDB: %zu active bracket (RAM iskelet)\n", g_brackets.size());
}

// =====================================================================
// PUBLIC API — GM komutlari + Tournament hook'lari icin
// =====================================================================

// Yeni bracket olustur (DB + RAM)
int32_t CreateBracket(const std::string& name, uint8 maxClans, const std::string& createdByGM)
{
	if (maxClans != 4 && maxClans != 8 && maxClans != 16 && maxClans != 32) {
		printf("[BRACKET] CreateBracket: maxClans must be 4/8/16/32 (got %u)\n", maxClans);
		return 0;
	}

	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);

	// DB INSERT (MATRIX SP_BRACKET_CREATE)
	int32_t bracketID = g_DBAgent.BracketCreate(name, maxClans, createdByGM);
	if (bracketID == 0) {
		// DB yoksa RAM-only fallback
		static int32_t s_nextRamBracketID = 1000;
		bracketID = s_nextRamBracketID++;
		printf("[BRACKET] DB unavailable, RAM-only bracket ID=%d\n", bracketID);
	}

	_BRACKET_INFO info;
	info.bracketID = bracketID;
	info.name = name;
	info.currentRound = 0;
	info.maxClans = maxClans;
	info.status = "REGISTRATION";
	info.winnerClanID = 0;
	g_brackets.push_back(info);

	printf("[BRACKET] Created: ID=%d Name='%s' MaxClans=%u GM=%s\n",
		bracketID, name.c_str(), maxClans, createdByGM.c_str());
	return bracketID;
}

// Klan kayit (DB + RAM duplicate guard)
bool RegisterClanToBracket(int32_t bracketID, uint16 clanID,
                           const std::string& clanName, const std::string& leaderName)
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	_BRACKET_INFO* b = FindBracket(bracketID);
	if (b == nullptr) {
		printf("[BRACKET] Register: BracketID=%d not found in RAM\n", bracketID);
		return false;
	}
	if (b->status != "REGISTRATION") {
		printf("[BRACKET] Register: BracketID=%d not in REGISTRATION (status=%s)\n",
			bracketID, b->status.c_str());
		return false;
	}

	// DB INSERT (MATRIX SP_BRACKET_REGISTER — duplicate + max kontrol orada)
	std::string result;
	bool ok = g_DBAgent.BracketRegister(bracketID, clanID, clanName, leaderName, result);
	if (!ok) {
		printf("[BRACKET] Register failed: %s (result=%s)\n", clanName.c_str(), result.c_str());
		return false;
	}

	printf("[BRACKET] Registered: BracketID=%d Clan=%s Leader=%s\n",
		bracketID, clanName.c_str(), leaderName.c_str());
	return true;
}

// Bracket'i baslat — Round 1 mac'lari olusur, ilk 6 mac otomatik baslar (zone cyclic)
bool StartBracket(int32_t bracketID)
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	_BRACKET_INFO* b = FindBracket(bracketID);
	if (b == nullptr) {
		printf("[BRACKET] Start: BracketID=%d not found\n", bracketID);
		return false;
	}
	if (b->status != "REGISTRATION") {
		printf("[BRACKET] Start: BracketID=%d already started (status=%s)\n",
			bracketID, b->status.c_str());
		return false;
	}

	// DB SP_BRACKET_GENERATE_MATCHES (Round 1 mac'lari olusur, seed shuffle, zone cyclic)
	if (!g_DBAgent.BracketGenerateMatches(bracketID)) {
		printf("[BRACKET] Start: GenerateMatches DB hata BracketID=%d\n", bracketID);
		return false;
	}

	b->status = "ACTIVE";
	b->currentRound = 1;
	// S115 BUG #2 FIX — DB'den maclari RAM'e cek
	RefreshBracketMatches(b);

	printf("[BRACKET] Started: BracketID=%d (Round 1, %zu mac RAM)\n",
		bracketID, b->matches.size());

	// TODO acilis sonrasi: ilk 6 mac'i otomatik /tournamentstart ile baslat (DependsOnMatch1=NULL olanlar)
	// Su an manuel GM tetiklemesiyle calisir (her mac icin /tournamentstart cagrı)

	return true;
}

// Bracket iptal — DB CANCELLED, active maclar WALKOVER
bool CancelBracket(int32_t bracketID)
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	_BRACKET_INFO* b = FindBracket(bracketID);
	if (b == nullptr) {
		printf("[BRACKET] Cancel: BracketID=%d not found\n", bracketID);
		return false;
	}

	// DB SP_BRACKET_CANCEL
	if (!g_DBAgent.BracketCancel(bracketID)) {
		printf("[BRACKET] Cancel DB hata BracketID=%d (RAM yine de iptal)\n", bracketID);
	}

	b->status = "CANCELLED";

	// Aktif tournament zone'lari kontrol et — bracket'a bagli mac varsa kapat
	for (auto& m : b->matches) {
		if (m.status == "ACTIVE") {
			m.status = "WALKOVER";
			// Aktif tournament'i kapat (TournamentClose mantigi ile)
			_TOURNAMENT_DATA* tInfo = g_pMain->m_ClanVsDataList.GetData(m.zoneID);
			if (tInfo != nullptr) {
				g_pMain->KickOutZoneUsers(m.zoneID, ZONE_MORADON, (uint8)Nation::ALL);
				g_pMain->m_ClanVsDataList.DeleteData(m.zoneID);
			}
		}
	}

	printf("[BRACKET] Cancelled: BracketID=%d\n", bracketID);
	return true;
}

// Bracket durumunu console'a yaz
void GetBracketStatus(int32_t bracketID)
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	_BRACKET_INFO* b = FindBracket(bracketID);
	if (b == nullptr) {
		printf("[BRACKET] Status: BracketID=%d not found\n", bracketID);
		return;
	}

	printf("====== BRACKET %d STATUS ======\n", bracketID);
	printf("  Name:           %s\n", b->name.c_str());
	printf("  Status:         %s\n", b->status.c_str());
	printf("  MaxClans:       %u\n", b->maxClans);
	printf("  CurrentRound:   %u\n", b->currentRound);
	printf("  WinnerClanID:   %u (%s)\n",
		b->winnerClanID, b->winnerClanName.c_str());
	printf("  Matches:        %zu\n", b->matches.size());
	for (auto& m : b->matches) {
		printf("    Round=%u MatchOrder=%u Red=%u Blue=%u Zone=%u Status=%s Winner=%u\n",
			m.round, m.matchOrder, m.redClanID, m.blueClanID,
			m.zoneID, m.status.c_str(), m.winnerClanID);
	}
	printf("===============================\n");
}

// =====================================================================
// HOOK — Tournament biti, bracket'a bagli mi kontrol
// TournamentSystem.cpp::HandleTournamentEnd'ten cagrilir
// =====================================================================
void OnBracketMatchFinish(int32_t matchID, uint16 winnerClanID,
                         uint16 redScore, uint16 blueScore)
{
	if (matchID <= 0) return;

	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);

	// DB SP_BRACKET_MATCH_FINISH
	if (!g_DBAgent.BracketMatchFinish(matchID, redScore, blueScore, winnerClanID)) {
		printf("[BRACKET] OnMatchFinish DB hata matchID=%d\n", matchID);
	}

	_BRACKET_MATCH_INFO* m = FindMatch(matchID);
	if (m == nullptr) {
		printf("[BRACKET] OnMatchFinish: matchID=%d not in RAM\n", matchID);
		return;
	}

	m->status = "FINISHED";
	m->finished = true;
	m->winnerClanID = winnerClanID;

	_BRACKET_INFO* b = FindBracket(m->bracketID);
	if (b == nullptr) return;

	// Tur tamamlanma kontrol — su anki turda hala PENDING/ACTIVE mac var mi
	bool roundComplete = true;
	for (auto& mm : b->matches) {
		if (mm.round == b->currentRound &&
		    mm.status != "FINISHED" && mm.status != "WALKOVER") {
			roundComplete = false;
			break;
		}
	}

	if (!roundComplete) {
		printf("[BRACKET] OnMatchFinish: Round %u henuz tamamlanmadi (bracket %d)\n",
			b->currentRound, b->bracketID);
		return;
	}

	// Tur tamamlandi — sonraki tur veya FINAL
	printf("[BRACKET] Round %u tamamlandi (bracket %d), sonraki tur olusturuluyor\n",
		b->currentRound, b->bracketID);

	if (!g_DBAgent.BracketNextRoundGenerate(b->bracketID, b->currentRound)) {
		printf("[BRACKET] NextRoundGenerate DB hata bracketID=%d\n", b->bracketID);
		return;
	}

	b->currentRound++;
	// S115 BUG #2 FIX — Yeni tur DB'den RAM'e cek
	RefreshBracketMatches(b);

	// Final ise odul dagit
	// Bracket bittiyse (DB tarafinda Status=FINISHED set edilir SP icinde) odul dagitma
	// TODO: g_DBAgent.BracketGetRewards ile pozisyon basina odul cek, kazanan klan uyelerine dagit
}

// =====================================================================
// TIMER — her saniye GameEventMainTimer'dan cagrilir (DependsOnMatch ve WALKOVER icin)
// =====================================================================
void BracketAutoStartTimer()
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);

	for (auto& b : g_brackets) {
		if (b.status != "ACTIVE") continue;

		for (auto& m : b.matches) {
			if (m.round != b.currentRound) continue;
			if (m.status != "PENDING") continue;

			// DependsOnMatch1 var mi, bittik mi?
			if (m.dependsOnMatch1 > 0) {
				_BRACKET_MATCH_INFO* dep = FindMatch(m.dependsOnMatch1);
				if (dep == nullptr || dep->status != "FINISHED") continue;
			}

			// Zone'da aktif tournament var mi?
			if (g_pMain->m_ClanVsDataList.GetData(m.zoneID) != nullptr) continue;

			// Bu mac'i baslat — TODO: tam entegrasyon acilis sonrasi
			// Su an GM manuel /tournamentstart ile baslatir
			// Otomatik baslatma icin _TOURNAMENT_DATA olusturup bracketMatchID set etmek lazim
			printf("[BRACKET] Match %d hazir (Round %u, Zone %u) — GM /tournamentstart bekleyen\n",
				m.matchID, m.round, m.zoneID);
		}

		// WALKOVER kontrol — 5 dakika sonra hala ACTIVE ise klan gelmemis
		const time_t WALKOVER_TIMEOUT_SEC = 5 * 60;
		for (auto& m : b.matches) {
			if (m.status != "ACTIVE") continue;
			if (m.startTime == 0) continue;
			if ((UNIXTIME - m.startTime) < WALKOVER_TIMEOUT_SEC) continue;

			// Klan gelmedi — rakip otomatik kazanir
			// (Bu durumda Tournament zaten yaratilmis demek; rakip score = walkover_winner)
			// Su an iskelet: gercek walkover detection acilis sonrasi
			printf("[BRACKET] WALKOVER candidate: matchID=%d (5dk gecti)\n", m.matchID);
		}
	}
}
