// =========================================================================
// S115 — Bracket Tournament (8/16/32 klan elemeli)
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// Tek eleme (single elimination):
//   8 klan  -> 3 tur (Quarter -> Semi -> Final)
//   16 klan -> 4 tur
//   32 klan -> 5 tur
//
// Mac yapma: mevcut Tournament sistemi (_TOURNAMENT_DATA + HandleTournamentStart)
// Bracket Engine: mac sonuclarini takip eder, sonraki tur eslesir
//
// PG temiz: yeni opcode YOK, mevcut tournament sistemi uzerine kurulu.
// =========================================================================

#include "stdafx.h"

// RAM-level aktif bracket cache
struct _BRACKET_MATCH_INFO {
	int32_t matchID;         // DB MatchID
	int32_t bracketID;
	uint8   round;
	uint8   matchOrder;
	uint16  redClanID;
	uint16  blueClanID;
	uint8   zoneID;
	bool    finished;
	uint16  winnerClanID;
};

struct _BRACKET_INFO {
	int32_t bracketID;
	std::string name;
	uint8   currentRound;
	uint8   maxClans;
	bool    finished;
	uint16  winnerClanID;
	std::vector<_BRACKET_MATCH_INFO> matches;
};

static std::vector<_BRACKET_INFO> g_brackets;
static std::recursive_mutex g_bracketLock;

// MATRIX SP'den brackets yukle (acilis sonrasi DB hazir oldukca)
void LoadBracketsFromDB()
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	g_brackets.clear();

	// TODO: MATRIX MSG:5909 bracket DB tablo + SP'ler hazir olduktan sonra
	// SELECT * FROM _MK_BRACKET_TOURNAMENT WHERE Status IN ('REGISTRATION', 'ACTIVE')
	// SELECT * FROM _MK_BRACKET_MATCHES WHERE BracketID IN (...)
	// Su an iskelet — MATRIX cevap geldikten sonra entegrasyon yapilacak

	printf("[BRACKET] Loaded %zu brackets from DB\n", g_brackets.size());
}

// Yeni bracket olustur
int32_t CreateBracket(const std::string& name, uint8 maxClans, const std::string& createdByGM)
{
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);

	// MATRIX SP_BRACKET_CREATE cagri (DB hazir olunca uncomment)
	// int32_t bracketID = g_DBAgent.BracketCreate(name, maxClans, createdByGM);

	// Su an RAM-only iskelet
	static int32_t s_nextBracketID = 1000; // RAM-only counter
	int32_t bracketID = s_nextBracketID++;

	_BRACKET_INFO info;
	info.bracketID = bracketID;
	info.name = name;
	info.currentRound = 0;
	info.maxClans = maxClans;
	info.finished = false;
	info.winnerClanID = 0;
	g_brackets.push_back(info);

	printf("[BRACKET] Created bracket: ID=%d Name=%s MaxClans=%d\n",
		bracketID, name.c_str(), maxClans);
	return bracketID;
}

// Klan kayit
bool RegisterClanToBracket(int32_t bracketID, uint16 clanID,
                           const std::string& clanName, const std::string& leaderName)
{
	// MATRIX SP_BRACKET_REGISTER cagri (DB hazir olunca)
	// Su an iskelet
	printf("[BRACKET] Register: BracketID=%d Clan=%s Leader=%s\n",
		bracketID, clanName.c_str(), leaderName.c_str());
	return true;
}

// Bracket'i baslat (kayit kapanır, Round 1 mac'lari olusur)
bool StartBracket(int32_t bracketID)
{
	// MATRIX SP_BRACKET_GENERATE_MATCHES cagri
	// Mac'lari RAM'e yukle, Round 1'i baslat
	// Her mac _TOURNAMENT_DATA olusturup HandleTournamentStart mantigi ile baslar
	printf("[BRACKET] Start bracket: ID=%d (TODO: SP_BRACKET_GENERATE_MATCHES)\n", bracketID);
	return true;
}

// Mac biti — kazanani bracket'e isaretle, sonraki tur kontrol
void OnBracketMatchFinish(int32_t matchID, uint16 winnerClanID,
                         uint16 redScore, uint16 blueScore)
{
	// MATRIX SP_BRACKET_MATCH_FINISH cagri
	// Tur tamamlandiysa SP_BRACKET_NEXT_ROUND_GENERATE cagri
	// Final ise bracket'i FINISHED yap + sampiyon odul
	printf("[BRACKET] Match finished: MatchID=%d Winner=%u Score=%u-%u\n",
		matchID, winnerClanID, redScore, blueScore);
}
