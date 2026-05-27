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

// Forward declarations
void DistributeBracketRewards(int32_t bracketID, uint16 clanID, const std::string& position);

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

	// NOT: ilk 6 mac (Round 1, DependsOnMatch1=0 olanlar) BracketAutoStartTimer
	// tarafindan otomatik baslatilir (her saniye GameEventMainTimer'dan).
	// Round 2/3/Final hooks OnBracketMatchFinish -> RefreshBracketMatches -> tekrar
	// AutoStartTimer pickup ile zincir devam eder.
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

	// BUG #13 FIX — lokal kopya, RefreshBracketMatches sonrasi m invalid olacak
	int32_t cachedBracketID = m->bracketID;
	m = nullptr;  // pointer artik kullanma sinyali

	_BRACKET_INFO* b = FindBracket(cachedBracketID);
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
	// BUG #13 NOT: `m` pointer'i bu satirdan sonra INVALID — `m` kullanma!
	RefreshBracketMatches(b);
	// m = nullptr;  // güvenlik (compile error vermesin diye komment)

	// S115 BUG #3 FIX — Final tamamlandi mi kontrol et + odul dagit
	// Final tamamlanma kriteri: yeni tur olusmayinca veya tum maclar bitince
	bool allFinished = true;
	for (auto& mm : b->matches) {
		if (mm.status != "FINISHED" && mm.status != "WALKOVER") {
			allFinished = false;
			break;
		}
	}

	if (allFinished) {
		// Son tur galibi = bracket sampiyonu
		uint16 championClanID = winnerClanID;
		b->winnerClanID = championClanID;
		b->status = "FINISHED";

		CKnights* pChampion = g_pMain->GetClanPtr(championClanID);
		if (pChampion) {
			b->winnerClanName = pChampion->GetName();
		}

		printf("[BRACKET] FINISHED: BracketID=%d Champion=%u (%s)\n",
			b->bracketID, championClanID, b->winnerClanName.c_str());

		// BUG #7 FIX: Final mac (en yuksek round) → kaybeden klan
		// `m` parametre matchID ile FindMatch'ten geldi, bu son biten mac (Final olabilir)
		// Guvenli: en yuksek round'daki maca bak
		uint8 maxRound = 0;
		_BRACKET_MATCH_INFO* finalMatch = nullptr;
		for (auto& mm : b->matches) {
			if (mm.round > maxRound) {
				maxRound = mm.round;
				finalMatch = &mm;
			}
		}

		DistributeBracketRewards(b->bracketID, championClanID, "CHAMPION");

		if (finalMatch != nullptr) {
			uint16 finalLoserClanID = (finalMatch->redClanID == championClanID)
				? finalMatch->blueClanID
				: finalMatch->redClanID;
			if (finalLoserClanID > 0 && finalLoserClanID != championClanID) {
				DistributeBracketRewards(b->bracketID, finalLoserClanID, "RUNNER_UP");
			}
		}
	}
}

// =====================================================================
// HELPER — Bracket reward dagitim (S115 BUG #3 FIX)
// =====================================================================
void DistributeBracketRewards(int32_t bracketID, uint16 clanID, const std::string& position)
{
	int32_t gold = 0, np = 0, itemID = 0, premiumHours = 0;
	int16_t itemCount = 0;

	if (!g_DBAgent.BracketGetRewards(bracketID, position, gold, np, itemID, itemCount, premiumHours)) {
		printf("[BRACKET] Reward yok: bracket=%d position=%s\n", bracketID, position.c_str());
		return;
	}

	printf("[BRACKET] Reward dagit: bracket=%d clan=%u pos=%s gold=%d np=%d item=%d x%d premium=%dh\n",
		bracketID, clanID, position.c_str(), gold, np, itemID, itemCount, premiumHours);

	int distributedCount = 0;
	for (uint16 i = 0; i < MAX_USER; i++) {
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;
		if (pUser->GetClanID() != clanID) continue;

		if (gold > 0) pUser->GoldGain((uint32)gold);
		if (np > 0) pUser->SendLoyaltyChange("bracket", np, false, false, false);
		if (itemID > 0 && itemCount > 0) {
			pUser->GiveItem("bracket_reward", (uint32)itemID, (uint16)itemCount, true, 0);
		}

		std::string msg;
		if (position == "CHAMPION") {
			msg = "[BRACKET] TEBRIKLER! Klanin bracket turnuvasinda SAMPIYON oldu!";
		} else if (position == "RUNNER_UP") {
			msg = "[BRACKET] Final maclari! Klanin 2. oldu. Tebrikler!";
		} else {
			msg = "[BRACKET] Iyi savas! Katilim odulun aldin.";
		}
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		pUser->Send(&pkt);

		distributedCount++;
	}

	// Klan premium (CHAMPION icin standart 168h, MATRIX DB'den)
	if (position == "CHAMPION" && premiumHours > 0) {
		CKnights* pClan = g_pMain->GetClanPtr(clanID);
		if (pClan != nullptr) {
			pClan->sPremiumTime = (uint32)UNIXTIME + (uint32)(premiumHours * 3600);
			printf("[BRACKET] Klan premium +%d saat verildi: clanID=%u\n",
				premiumHours, clanID);
		}
	}

	printf("[BRACKET] Reward dagitildi %d oyuncuya (clan=%u)\n", distributedCount, clanID);
}

// =====================================================================
// HELPER — Bracket mac'i icin _TOURNAMENT_DATA olustur + zone'a yayinla
// Manuel /tournamentstart mantiginin bracket'a uyarlanmis versiyonu (ChatHandler:1369).
// Geri donus: true=tournament basladi, false=hata (klan yok / PutData fail)
// =====================================================================
static const uint16 BRACKET_MATCH_DURATION_MIN = 10;  // her bracket maci 10 dakika

static bool StartBracketMatchTournament(_BRACKET_MATCH_INFO& m, const std::string& bracketName)
{
	// Klanlari bul
	CKnights* pRedClan  = g_pMain->GetClanPtr(m.redClanID);
	CKnights* pBlueClan = g_pMain->GetClanPtr(m.blueClanID);

	if (pRedClan == nullptr || pBlueClan == nullptr) {
		printf("[BRACKET] StartMatch: klan bulunamadi (matchID=%d red=%u blue=%u)\n",
			m.matchID, m.redClanID, m.blueClanID);
		return false;
	}

	// _TOURNAMENT_DATA olustur — HandleTournamentStart ile birebir ayni (sade)
	_TOURNAMENT_DATA* pData = new _TOURNAMENT_DATA();
	pData->aTournamentZoneID         = m.zoneID;
	pData->aTournamentClanNum[0]     = pRedClan->GetID();
	pData->aTournamentClanNum[1]     = pBlueClan->GetID();
	pData->aTournamentScoreBoard[0]  = 0;
	pData->aTournamentScoreBoard[1]  = 0;
	pData->aTournamentTimer          = (uint32)BRACKET_MATCH_DURATION_MIN * 60;
	pData->aTournamentMonumentKilled = 0;
	pData->aTournamentOutTimer       = 0;
	pData->aTournamentisAttackable   = true;
	pData->aTournamentisStarted      = true;
	pData->aTournamentisFinished     = false;

	// KRITIK — bracketMatchID set, OnBracketMatchFinish bu sayede tetiklenir
	pData->bracketMatchID = m.matchID;

	// DB log
	std::string startedBy = "bracket_auto";
	pData->dbTournamentID = g_DBAgent.TournamentLogStart(
		m.zoneID,
		pRedClan->GetID(),  pBlueClan->GetID(),
		pRedClan->GetName(), pBlueClan->GetName(),
		BRACKET_MATCH_DURATION_MIN, startedBy);

	// Thread-safe insert
	if (!g_pMain->m_ClanVsDataList.PutData(m.zoneID, pData)) {
		delete pData;
		printf("[BRACKET] StartMatch: PutData fail (Zone=%u matchID=%d)\n",
			m.zoneID, m.matchID);
		return false;
	}

	// Bahis penceresi (forward declaration TournamentSystem.cpp'de OpenTournamentBets)
	extern void OpenTournamentBets(uint8 zoneID);
	OpenTournamentBets(m.zoneID);

	// Server-wide duyuru
	const char* zoneName =
		(m.zoneID == 77) ? "Ardream"   :
		(m.zoneID == 78) ? "Ronark"    :
		(m.zoneID == 96) ? "PartyVs-1" :
		(m.zoneID == 97) ? "PartyVs-2" :
		(m.zoneID == 98) ? "PartyVs-3" :
		(m.zoneID == 99) ? "PartyVs-4" : "?";

	char buf[256] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[BRACKET %s] Round %u: %s vs %s @ %s (%u dk) — sampiyonluk yolunda!",
		bracketName.c_str(), m.round,
		pRedClan->GetName().c_str(), pBlueClan->GetName().c_str(),
		zoneName, BRACKET_MATCH_DURATION_MIN);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	// Mac state guncelle
	m.status    = "ACTIVE";
	m.startTime = UNIXTIME;

	printf("[BRACKET] AUTO-START: matchID=%d Round=%u Zone=%u Red=%s(%u) Blue=%s(%u) dbTID=%d\n",
		m.matchID, m.round, m.zoneID,
		pRedClan->GetName().c_str(), pRedClan->GetID(),
		pBlueClan->GetName().c_str(), pBlueClan->GetID(),
		pData->dbTournamentID);

	return true;
}

// =====================================================================
// TIMER — her saniye GameEventMainTimer'dan cagrilir (DependsOnMatch ve WALKOVER icin)
//
// ITERATOR INVALIDATION DIKKAT: OnBracketMatchFinish() icinde
// RefreshBracketMatches() cagrilir, bu b.matches.clear() yapar.
// Bu yuzden BYE ve WALKOVER aksiyonlarini TOPLAYIP loop sonrasi isliyoruz.
// =====================================================================
struct _BRACKET_FINISH_ACTION {
	int32_t matchID;
	uint16  winnerClanID;
	uint16  redScore;
	uint16  blueScore;
	uint8   zoneID;       // > 0 ise tournament_data cleanup yap (no-show case)
	bool    isNoShow;     // true = zone kick + DeleteData gerek
};

void BracketAutoStartTimer()
{
	std::vector<_BRACKET_FINISH_ACTION> pendingFinishes;

	{
		std::lock_guard<std::recursive_mutex> lock(g_bracketLock);

		// Indeks loop — vector resize edilebilir ihtimaline karsi guvenli
		for (size_t bi = 0; bi < g_brackets.size(); bi++) {
			_BRACKET_INFO& b = g_brackets[bi];
			if (b.status != "ACTIVE") continue;

			// PASS 1 — Mac baslatma (PENDING -> ACTIVE) ve BYE detection
			for (size_t mi = 0; mi < b.matches.size(); mi++) {
				_BRACKET_MATCH_INFO& m = b.matches[mi];
				if (m.round != b.currentRound) continue;
				if (m.status != "PENDING") continue;

				// DependsOnMatch1 var mi, bittik mi?
				if (m.dependsOnMatch1 > 0) {
					_BRACKET_MATCH_INFO* dep = FindMatch(m.dependsOnMatch1);
					if (dep == nullptr || dep->status != "FINISHED") continue;
				}

				// Tek tarafli kayit (BYE) — rakipsiz mac, finish-action kuyruga ekle
				if (m.redClanID == 0 || m.blueClanID == 0) {
					uint16 walkoverWinner = (m.redClanID > 0) ? m.redClanID : m.blueClanID;
					if (walkoverWinner == 0) {
						// Iki klan da bos — bracket bozuk, atla (log)
						printf("[BRACKET] BYE: matchID=%d iki klan da bos, atlandi\n", m.matchID);
						m.status = "WALKOVER";
						m.finished = true;
						continue;
					}
					printf("[BRACKET] BYE: matchID=%d winner=%u (rakip yok)\n",
						m.matchID, walkoverWinner);

					// RAM status simdi WALKOVER — DB SP fail olsa bile sonraki tick'te
					// "PENDING" kontrolu atlar, infinite loop yok. RefreshBracketMatches
					// DB'den okudugunda SP basarili olduysa FINISHED gelir, fail olduysa
					// PENDING gelir — bu durumda sonraki tick yeniden tetikler (geride DB
					// duzelmesi beklenir). Pratikte: SP'ler MATRIX migration 108'de hazir,
					// fail beklenmez.
					m.status = "WALKOVER";
					m.finished = true;
					m.winnerClanID = walkoverWinner;

					_BRACKET_FINISH_ACTION a;
					a.matchID = m.matchID;
					a.winnerClanID = walkoverWinner;
					a.redScore = 1;
					a.blueScore = 0;
					a.zoneID = 0;       // BYE — tournament data yok
					a.isNoShow = false;
					pendingFinishes.push_back(a);
					continue;
				}

				// Zone'da aktif tournament var mi?
				if (g_pMain->m_ClanVsDataList.GetData(m.zoneID) != nullptr) continue;

				// Klan silinmis mi kontrol (silinmisse o klani kaybeden yap, log spam yok)
				CKnights* pRedCheck  = g_pMain->GetClanPtr(m.redClanID);
				CKnights* pBlueCheck = g_pMain->GetClanPtr(m.blueClanID);
				if (pRedCheck == nullptr || pBlueCheck == nullptr) {
					uint16 survivor = 0;
					if (pRedCheck != nullptr) survivor = m.redClanID;
					else if (pBlueCheck != nullptr) survivor = m.blueClanID;

					printf("[BRACKET] Klan silinmis: matchID=%d red=%u(%s) blue=%u(%s) → WALKOVER winner=%u\n",
						m.matchID, m.redClanID, pRedCheck ? "OK" : "YOK",
						m.blueClanID, pBlueCheck ? "OK" : "YOK", survivor);

					m.status = "WALKOVER";
					m.finished = true;
					m.winnerClanID = survivor;

					if (survivor > 0) {
						_BRACKET_FINISH_ACTION a;
						a.matchID = m.matchID;
						a.winnerClanID = survivor;
						a.redScore = 1;
						a.blueScore = 0;
						a.zoneID = 0;
						a.isNoShow = false;
						pendingFinishes.push_back(a);
					}
					continue;
				}

				// Bu mac'i otomatik baslat
				StartBracketMatchTournament(m, b.name);
			}

			// PASS 2 — 5dk sonra hala 0-0 ise no-show WALKOVER (ACTIVE maclar icin)
			const time_t WALKOVER_TIMEOUT_SEC = 5 * 60;
			for (size_t mi = 0; mi < b.matches.size(); mi++) {
				_BRACKET_MATCH_INFO& m = b.matches[mi];
				if (m.status != "ACTIVE") continue;
				if (m.startTime == 0) continue;
				if ((UNIXTIME - m.startTime) < WALKOVER_TIMEOUT_SEC) continue;

				_TOURNAMENT_DATA* tInfo = g_pMain->m_ClanVsDataList.GetData(m.zoneID);
				if (tInfo == nullptr) continue;  // zaten cleanup olmus

				uint16 redScore  = tInfo->aTournamentScoreBoard[0];
				uint16 blueScore = tInfo->aTournamentScoreBoard[1];
				if (redScore != 0 || blueScore != 0) continue;  // savas var, normal timer bitirecek

				// No-show: kimse gelmedi 5dk gecti → red default kazanan
				printf("[BRACKET] WALKOVER (no-show 5dk): matchID=%d red=%u default winner\n",
					m.matchID, m.redClanID);

				// RAM status simdi WALKOVER — tekrar tetiklenmesin (defansif, OnBracketMatchFinish
				// zaten FINISHED set edecek; ama RefreshBracketMatches arasinda iki tur garantisi)
				m.status = "WALKOVER";
				m.finished = true;
				m.winnerClanID = m.redClanID;

				_BRACKET_FINISH_ACTION a;
				a.matchID = m.matchID;
				a.winnerClanID = m.redClanID;
				a.redScore = 1;
				a.blueScore = 0;
				a.zoneID = m.zoneID;   // cleanup gerekiyor
				a.isNoShow = true;
				pendingFinishes.push_back(a);
			}
		}
	}

	// PASS 3 — Toplanmis finish action'lari isle (g_bracketLock disinda da olabilir,
	// OnBracketMatchFinish kendi lock'unu alir; ama no-show cleanup g_pMain ile etkilesir)
	for (auto& a : pendingFinishes) {
		if (a.isNoShow && a.zoneID > 0) {
			g_pMain->KickOutZoneUsers(a.zoneID, ZONE_MORADON, (uint8)Nation::ALL);
			g_pMain->m_ClanVsDataList.DeleteData(a.zoneID);
		}
		OnBracketMatchFinish(a.matchID, a.winnerClanID, a.redScore, a.blueScore);
	}
}

// =====================================================================
// PUBLIC HELPER — Bracket8v8.cpp ve ZoneChangeWarpHandler.cpp icin
// =====================================================================

// matchID -> bracketID lookup (TOURNAMENT_DATA.bracketMatchID'den BracketID cikarma)
// Donus: 0 = bulunamadi
int32_t BracketGetBracketIDByMatchID(int32_t matchID)
{
	if (matchID <= 0) return 0;
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	for (auto& b : g_brackets) {
		for (auto& m : b.matches) {
			if (m.matchID == matchID) return b.bracketID;
		}
	}
	return 0;
}

// matchID -> _BRACKET_MATCH_INFO partial info (zone giris kontrolu icin)
// Donus: true=bulundu, false=yok
bool BracketGetMatchInfo(int32_t matchID, int32_t& bracketIDOut,
                        uint16& redClanIDOut, uint16& blueClanIDOut)
{
	if (matchID <= 0) return false;
	std::lock_guard<std::recursive_mutex> lock(g_bracketLock);
	for (auto& b : g_brackets) {
		for (auto& m : b.matches) {
			if (m.matchID == matchID) {
				bracketIDOut  = b.bracketID;
				redClanIDOut  = m.redClanID;
				blueClanIDOut = m.blueClanID;
				return true;
			}
		}
	}
	return false;
}
