// =========================================================================
// S115 FAZ 16 — 1v1 Bracket Turnuva (Solo)
// Yazan: CHIP | Tarih: 2026-05-27 (entegrasyon: BUGUN)
// =========================================================================
// Clan Bracket'a benzer, ama klan yerine solo oyuncu.
// 16/32/64 kisi destekler.
//
// MATRIX SP'leri (migration 112_1v1_bracket): SP_1V1_CREATE/REGISTER/...
//
// Komutlar:
//   /1v1create "Ad" MaxPlayer  (console)
//   /1v1start BID
//   /1v1cancel BID
//   +1v1reg BID                (oyun ici)
//
// Otomatik akis:
//   1) /1v1start BID -> GenerateMatches DB + RAM cache fill
//   2) OneVsOneAutoStartTimer her saniye: PENDING + dependsOn bittik mac ->
//      2 oyuncu zone'a teleport (Tournament zone reuse, BracketAutoStartTimer
//      benzeri ama daha basit, klan yok user var)
//   3) Olum tetigi: UserHealtMagicSpSystem.cpp'de PVP kill -> zone 96-99 ise
//      OnUserDeath1v1Check cagri -> kaybeden belirlenir -> match finish ->
//      OnOneVsOneMatchFinish -> sonraki tur veya FINAL
//
// PG temiz: WIZ_CHAT + standart PVP, yeni opcode YOK.
// =========================================================================

#include "stdafx.h"

// =====================================================================
// RAM Cache (DB SP_1V1_LOAD_MATCHES yerine direkt SELECT, restart-safe)
// =====================================================================
struct _1V1_MATCH_INFO {
	int32_t matchID;
	int32_t bid;
	uint8   round;
	uint8   matchOrder;
	int32_t redUserID;
	int32_t blueUserID;
	std::string redName;
	std::string blueName;
	uint8   zoneID;
	std::string status;       // PENDING/ACTIVE/FINISHED/WALKOVER
	bool    finished;
	int32_t winnerUserID;
	int32_t dependsOnMatch1;
	time_t  startTime;
};

struct _1V1_INFO {
	int32_t bid;
	std::string name;
	uint8   maxPlayers;
	uint8   currentRound;
	std::string status;       // OPEN/STARTED/FINISHED/CANCELLED
	int32_t winnerUserID;
	std::string winnerName;
	std::vector<_1V1_MATCH_INFO> matches;
};

static std::vector<_1V1_INFO> g_1v1Brackets;
static std::recursive_mutex g_1v1Lock;

static const uint16 ONE_V_ONE_MATCH_DURATION_MIN = 5;  // her 1v1 maci 5 dakika

// GetUserPtr(socketID) icin DB user ID'sinden CUser bulma helper
static CUser* FindUserByDBID(int32_t userID)
{
	if (userID <= 0) return nullptr;
	for (uint16 i = 0; i < MAX_USER; i++) {
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;
		if (pUser->GetID() == userID) return pUser;
	}
	return nullptr;
}

// =====================================================================
// HELPER
// =====================================================================
static _1V1_INFO* Find1v1Bracket(int32_t bid)
{
	for (auto& b : g_1v1Brackets) {
		if (b.bid == bid) return &b;
	}
	return nullptr;
}

static _1V1_MATCH_INFO* Find1v1Match(int32_t matchID)
{
	for (auto& b : g_1v1Brackets) {
		for (auto& m : b.matches) {
			if (m.matchID == matchID) return &m;
		}
	}
	return nullptr;
}

static void Refresh1v1Matches(_1V1_INFO* b)
{
	if (b == nullptr) return;
	std::vector<CDBAgent::_1V1_MATCH_ROW> rows;
	if (!g_DBAgent.OneVsOneLoadMatches(b->bid, rows)) {
		printf("[1V1] RefreshMatches DB hata bid=%d\n", b->bid);
		return;
	}
	b->matches.clear();
	for (auto& r : rows) {
		_1V1_MATCH_INFO m;
		m.matchID         = r.matchID;
		m.bid             = b->bid;
		m.round           = r.roundNumber;
		m.matchOrder      = r.matchOrder;
		m.redUserID       = r.redUserID;
		m.blueUserID      = r.blueUserID;
		m.redName         = r.redName;
		m.blueName        = r.blueName;
		m.zoneID          = r.zoneID;
		m.status          = r.status;
		m.winnerUserID    = r.winnerUserID;
		m.finished        = (r.status == "FINISHED" || r.status == "WALKOVER");
		m.dependsOnMatch1 = r.dependsOnMatch1;
		m.startTime       = 0;
		b->matches.push_back(m);
	}
	printf("[1V1] RefreshMatches bid=%d %zu mac yuklendi\n", b->bid, b->matches.size());
}

// =====================================================================
// LOAD — server init'de cagrilir (mevcut acik 1v1 bracket'lari RAM'e cek)
// =====================================================================
void LoadOneVsOneBracketsFromDB()
{
	std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);
	g_1v1Brackets.clear();

	std::vector<CDBAgent::_1V1_INFO_ROW> rows;
	if (!g_DBAgent.OneVsOneLoadBrackets(rows)) {
		printf("[1V1] LoadBracketsFromDB: SELECT hata (tablo yok olabilir, MATRIX 112 apply bekliyor)\n");
		return;
	}

	for (auto& r : rows) {
		_1V1_INFO b;
		b.bid = r.bid;
		b.name = r.name;
		b.maxPlayers = r.maxPlayers;
		b.currentRound = r.currentRound;
		b.status = r.status;
		b.winnerUserID = r.winnerUserID;
		b.winnerName = r.winnerName;
		g_1v1Brackets.push_back(b);
	}

	// Her aktif bracket icin matches yukle
	for (auto& b : g_1v1Brackets) {
		Refresh1v1Matches(&b);
	}

	printf("[1V1] LoadBracketsFromDB: %zu bracket yuklendi\n", g_1v1Brackets.size());
}

// =====================================================================
// HELPER — 1v1 maci icin 2 oyuncuyu zone'a teleport + duyuru
// =====================================================================
static const float ONE_V_ONE_RED_X  = 135.0f;
static const float ONE_V_ONE_RED_Z  = 115.0f;
static const float ONE_V_ONE_BLUE_X = 120.0f;
static const float ONE_V_ONE_BLUE_Z = 115.0f;

static bool Start1v1MatchTeleport(_1V1_MATCH_INFO& m, const std::string& bracketName)
{
	// Kullanicilar online mi?
	CUser* pRed  = FindUserByDBID(m.redUserID);
	CUser* pBlue = FindUserByDBID(m.blueUserID);

	if (pRed == nullptr || !pRed->isInGame() ||
	    pBlue == nullptr || !pBlue->isInGame()) {
		printf("[1V1] StartMatch: kullanici offline (matchID=%d red=%d(%s) blue=%d(%s))\n",
			m.matchID, m.redUserID, pRed ? "ON" : "OFF",
			m.blueUserID, pBlue ? "ON" : "OFF");
		return false;
	}

	// Olu mu / trade mi?
	if (pRed->isDead() || pBlue->isDead()) return false;
	if (pRed->isTrading() || pBlue->isTrading()) return false;
	if (pRed->isMerchanting() || pBlue->isMerchanting()) return false;

	// Zone'da aktif tournament var mi? (cakisma)
	if (g_pMain->m_ClanVsDataList.GetData(m.zoneID) != nullptr) {
		// 1v1 zone reuse — eger clan tournament zone'undaysa, atla
		return false;
	}

	// Iki kullaniciyi zone'a teleport (check=false ile CanChangeZone bypass)
	pRed->ZoneChange(m.zoneID,  ONE_V_ONE_RED_X,  ONE_V_ONE_RED_Z,  -1, false);
	pBlue->ZoneChange(m.zoneID, ONE_V_ONE_BLUE_X, ONE_V_ONE_BLUE_Z, -1, false);

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
		"[1V1 %s] Round %u: %s vs %s @ %s (%u dk) — Solo dovus basliyor!",
		bracketName.c_str(), m.round,
		m.redName.c_str(), m.blueName.c_str(),
		zoneName, ONE_V_ONE_MATCH_DURATION_MIN);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	// Mac state
	m.status    = "ACTIVE";
	m.startTime = UNIXTIME;

	printf("[1V1] AUTO-START: matchID=%d Round=%u Zone=%u Red=%s(%d) Blue=%s(%d)\n",
		m.matchID, m.round, m.zoneID,
		m.redName.c_str(), m.redUserID,
		m.blueName.c_str(), m.blueUserID);

	return true;
}

// =====================================================================
// MATCH FINISH HOOK — death detection ile cagrilir (asagida) veya WALKOVER
// =====================================================================
static void OnOneVsOneMatchFinish(int32_t matchID, int32_t winnerUserID)
{
	if (matchID <= 0) return;

	std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);

	// DB
	if (!g_DBAgent.OneVsOneMatchFinish(matchID, winnerUserID)) {
		printf("[1V1] MatchFinish DB hata matchID=%d\n", matchID);
	}

	_1V1_MATCH_INFO* m = Find1v1Match(matchID);
	if (m == nullptr) {
		printf("[1V1] MatchFinish: matchID=%d RAM'de yok\n", matchID);
		return;
	}

	m->status = "FINISHED";
	m->finished = true;
	m->winnerUserID = winnerUserID;

	int32_t cachedBID = m->bid;
	m = nullptr;  // RefreshMatches'ten sonra invalid

	_1V1_INFO* b = Find1v1Bracket(cachedBID);
	if (b == nullptr) return;

	// Tur tamamlandi mi?
	bool roundComplete = true;
	for (auto& mm : b->matches) {
		if (mm.round == b->currentRound &&
		    mm.status != "FINISHED" && mm.status != "WALKOVER") {
			roundComplete = false;
			break;
		}
	}

	if (!roundComplete) {
		printf("[1V1] Round %u henuz tamamlanmadi (bid=%d)\n", b->currentRound, b->bid);
		return;
	}

	printf("[1V1] Round %u tamamlandi (bid=%d), sonraki tur olusturuluyor\n",
		b->currentRound, b->bid);

	if (!g_DBAgent.OneVsOneNextRoundGenerate(b->bid, b->currentRound)) {
		printf("[1V1] NextRoundGenerate DB hata bid=%d\n", b->bid);
		return;
	}

	b->currentRound++;
	Refresh1v1Matches(b);

	// FINAL bitti mi (yeni tur olusmadi veya tum maclar bittik)?
	bool allFinished = true;
	for (auto& mm : b->matches) {
		if (mm.status != "FINISHED" && mm.status != "WALKOVER") {
			allFinished = false;
			break;
		}
	}

	if (allFinished) {
		b->status = "FINISHED";
		b->winnerUserID = winnerUserID;

		CUser* pChampion = FindUserByDBID(winnerUserID);
		if (pChampion && pChampion->isInGame()) {
			b->winnerName = pChampion->GetName();
		}

		printf("[1V1] BRACKET FINISHED: bid=%d Champion=%d (%s)\n",
			b->bid, winnerUserID, b->winnerName.c_str());

		// Sampiyon odul: 10M Noah + 2000 NP
		if (pChampion && pChampion->isInGame()) {
			pChampion->GoldGain(10000000);
			pChampion->SendLoyaltyChange("1v1_champion", 2000, false, false, false);

			char buf[256] = { 0 };
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[1V1 %s] TEBRIKLER %s! Bracket sampiyonu oldun. 10M Noah + 2000 NP odul!",
				b->name.c_str(), b->winnerName.c_str());
			std::string msg = buf;
			g_pMain->SendNotice(msg.c_str());
		}
	}
}

// =====================================================================
// AUTO-START TIMER — her saniye GameEventMainTimer'dan cagrilir
// =====================================================================
struct _1V1_FINISH_ACTION {
	int32_t matchID;
	int32_t winnerUserID;
};

void OneVsOneAutoStartTimer()
{
	std::vector<_1V1_FINISH_ACTION> pendingFinishes;

	{
		std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);

		for (size_t bi = 0; bi < g_1v1Brackets.size(); bi++) {
			_1V1_INFO& b = g_1v1Brackets[bi];
			if (b.status != "STARTED") continue;

			// PASS 1: PENDING -> ACTIVE
			for (size_t mi = 0; mi < b.matches.size(); mi++) {
				_1V1_MATCH_INFO& m = b.matches[mi];
				if (m.round != b.currentRound) continue;
				if (m.status != "PENDING") continue;

				// DependsOn check
				if (m.dependsOnMatch1 > 0) {
					_1V1_MATCH_INFO* dep = Find1v1Match(m.dependsOnMatch1);
					if (dep == nullptr || dep->status != "FINISHED") continue;
				}

				// BYE — rakipsiz mac (sadece red veya sadece blue)
				if (m.redUserID == 0 || m.blueUserID == 0) {
					int32_t walkoverWinner = (m.redUserID > 0) ? m.redUserID : m.blueUserID;
					printf("[1V1] BYE: matchID=%d winner=%d\n", m.matchID, walkoverWinner);
					m.status = "WALKOVER";
					m.finished = true;
					m.winnerUserID = walkoverWinner;
					if (walkoverWinner > 0) {
						_1V1_FINISH_ACTION a;
						a.matchID = m.matchID;
						a.winnerUserID = walkoverWinner;
						pendingFinishes.push_back(a);
					}
					continue;
				}

				Start1v1MatchTeleport(m, b.name);
			}

			// PASS 2: 5dk gectiyse hala ACTIVE -> WALKOVER (kullanici disconnect olmus)
			const time_t TIMEOUT_SEC = ONE_V_ONE_MATCH_DURATION_MIN * 60;
			for (size_t mi = 0; mi < b.matches.size(); mi++) {
				_1V1_MATCH_INFO& m = b.matches[mi];
				if (m.status != "ACTIVE") continue;
				if (m.startTime == 0) continue;
				if ((UNIXTIME - m.startTime) < TIMEOUT_SEC) continue;

				// Zaman doldu — iki kullanicidan online olan kazanir (basit: HP yuksek olan)
				CUser* pRed  = FindUserByDBID(m.redUserID);
				CUser* pBlue = FindUserByDBID(m.blueUserID);

				int32_t winnerUID = 0;
				if (pRed && pRed->isInGame() && (!pBlue || !pBlue->isInGame())) {
					winnerUID = m.redUserID;
				} else if (pBlue && pBlue->isInGame() && (!pRed || !pRed->isInGame())) {
					winnerUID = m.blueUserID;
				} else if (pRed && pBlue && pRed->isInGame() && pBlue->isInGame()) {
					// Iki kullanici da online — HP yuksek olan kazanir
					winnerUID = (pRed->m_sHp >= pBlue->m_sHp) ? m.redUserID : m.blueUserID;
				} else {
					// Iki kullanici da offline — red default kazanir
					winnerUID = m.redUserID;
				}

				printf("[1V1] TIMEOUT (5dk): matchID=%d winner=%d (HP/online)\n",
					m.matchID, winnerUID);

				m.status = "WALKOVER";
				m.finished = true;
				m.winnerUserID = winnerUID;

				// Kullanicilar zone'dan cikar
				if (pRed && pRed->isInGame())  pRed->ZoneChange(ZONE_MORADON, 905.0f, 870.0f);
				if (pBlue && pBlue->isInGame()) pBlue->ZoneChange(ZONE_MORADON, 905.0f, 870.0f);

				_1V1_FINISH_ACTION a;
				a.matchID = m.matchID;
				a.winnerUserID = winnerUID;
				pendingFinishes.push_back(a);
			}
		}
	}

	// PASS 3: pendingFinishes isle (g_1v1Lock disinda, OnOneVsOneMatchFinish kendi lock'unu alir)
	for (auto& a : pendingFinishes) {
		OnOneVsOneMatchFinish(a.matchID, a.winnerUserID);
	}
}

// =====================================================================
// DEATH HOOK — UserHealtMagicSpSystem.cpp'den cagrilir
// PVP kill zone 77/78/96-99'da olursa, 1v1 maci var mi check
// =====================================================================
void OnUserDeath1v1Check(int32_t killedUserID, uint8 zoneID)
{
	std::vector<_1V1_FINISH_ACTION> pendingFinishes;

	{
		std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);

		for (auto& b : g_1v1Brackets) {
			if (b.status != "STARTED") continue;
			for (auto& m : b.matches) {
				if (m.status != "ACTIVE") continue;
				if (m.zoneID != zoneID) continue;
				if (m.redUserID != killedUserID && m.blueUserID != killedUserID) continue;

				// Kaybeden = oldurulen, kazanan = digeri
				int32_t winnerUID = (m.redUserID == killedUserID) ? m.blueUserID : m.redUserID;

				printf("[1V1] DEATH HOOK: matchID=%d killed=%d winner=%d zone=%u\n",
					m.matchID, killedUserID, winnerUID, zoneID);

				m.status = "FINISHED";
				m.finished = true;
				m.winnerUserID = winnerUID;

				// Iki kullaniciyi Moradon'a (kazanan + kaybeden)
				CUser* pRed  = FindUserByDBID(m.redUserID);
				CUser* pBlue = FindUserByDBID(m.blueUserID);
				if (pRed && pRed->isInGame())  pRed->ZoneChange(ZONE_MORADON, 905.0f, 870.0f);
				if (pBlue && pBlue->isInGame()) pBlue->ZoneChange(ZONE_MORADON, 905.0f, 870.0f);

				_1V1_FINISH_ACTION a;
				a.matchID = m.matchID;
				a.winnerUserID = winnerUID;
				pendingFinishes.push_back(a);
				break;  // 1 maci tetikleyince yeter
			}
		}
	}

	for (auto& a : pendingFinishes) {
		OnOneVsOneMatchFinish(a.matchID, a.winnerUserID);
	}
}

// =====================================================================
// GM Komutlari (console)
// =====================================================================
COMMAND_HANDLER(CGameServerDlg::HandleOneVsOneCreateCommand)
{
	if (vargs.size() < 2) {
		printf("Usage: /1v1create \"Ad\" MaxPlayers (16/32/64)\n");
		return true;
	}
	std::string name = vargs.front(); vargs.pop_front();
	uint8 maxPlayers = (uint8)SafeAtoi(vargs.front(), 16, 64);

	if (maxPlayers != 16 && maxPlayers != 32 && maxPlayers != 64) {
		printf("[1V1] MaxPlayers must be 16/32/64\n");
		return true;
	}

	int32_t bid = g_DBAgent.OneVsOneCreate(name, maxPlayers, "console");
	if (bid <= 0) {
		printf("[1V1] Create DB hata (MATRIX 112 apply edildi mi?)\n");
		return true;
	}

	// RAM cache'e ekle
	{
		std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);
		_1V1_INFO b;
		b.bid = bid;
		b.name = name;
		b.maxPlayers = maxPlayers;
		b.currentRound = 0;
		b.status = "OPEN";
		b.winnerUserID = 0;
		g_1v1Brackets.push_back(b);
	}

	printf("[1V1] Created: BID=%d Name='%s' MaxPlayers=%u\n",
		bid, name.c_str(), maxPlayers);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleOneVsOneStartCommand)
{
	if (vargs.empty()) {
		printf("Usage: /1v1start BID\n");
		return true;
	}
	int32_t bid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	if (!g_DBAgent.OneVsOneGenerateMatches(bid)) {
		printf("[1V1] Start hata: BID=%d (MATRIX 112 apply, oyuncu sayisi yeterli mi?)\n", bid);
		return true;
	}

	std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);
	_1V1_INFO* b = Find1v1Bracket(bid);
	if (b == nullptr) {
		// Belki create RAM'de yok — DB'den reload
		LoadOneVsOneBracketsFromDB();
		b = Find1v1Bracket(bid);
	}
	if (b == nullptr) {
		printf("[1V1] Start: BID=%d RAM'de yok (LoadBrackets sonrasi)\n", bid);
		return true;
	}

	b->status = "STARTED";
	b->currentRound = 1;
	Refresh1v1Matches(b);

	printf("[1V1] Started: BID=%d Round 1, %zu mac RAM (otomatik AutoStartTimer baslatacak)\n",
		bid, b->matches.size());
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleOneVsOneCancelCommand)
{
	if (vargs.empty()) {
		printf("Usage: /1v1cancel BID\n");
		return true;
	}
	int32_t bid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	if (g_DBAgent.OneVsOneCancel(bid)) {
		std::lock_guard<std::recursive_mutex> lock(g_1v1Lock);
		_1V1_INFO* b = Find1v1Bracket(bid);
		if (b) {
			b->status = "CANCELLED";

			// Aktif maclari WALKOVER yap + zone'daki kullanicilari Moradon'a
			for (auto& m : b->matches) {
				if (m.status != "ACTIVE") continue;

				// Iki kullaniciyi Moradon'a
				CUser* pRed  = FindUserByDBID(m.redUserID);
				CUser* pBlue = FindUserByDBID(m.blueUserID);
				if (pRed && pRed->isInGame())   pRed->ZoneChange(ZONE_MORADON, 905.0f, 870.0f);
				if (pBlue && pBlue->isInGame()) pBlue->ZoneChange(ZONE_MORADON, 905.0f, 870.0f);

				m.status = "WALKOVER";
				m.finished = true;
				printf("[1V1] Cancel: matchID=%d WALKOVER + user kick out\n", m.matchID);
			}
		}
		printf("[1V1] Cancelled: BID=%d\n", bid);
	} else {
		printf("[1V1] Cancel hata: BID=%d\n", bid);
	}
	return true;
}

// +1v1reg BID
COMMAND_HANDLER(CUser::HandleOneVsOneRegCommand)
{
	if (isDead()) {
		g_pMain->SendHelpDescription(this, "Olu iken 1v1 bracket kayit yapamazsin.");
		return true;
	}
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "+1v1reg BID. Ornek: +1v1reg 1");
		return true;
	}
	int32_t bid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	std::string result;
	bool ok = g_DBAgent.OneVsOneRegister(bid, GetID(), GetName(),
		(uint8)GetClass(), GetLevel(), result);

	char buf[200] = { 0 };
	if (ok) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[1V1] Bracket %d kaydin onaylandi. Bekleyenler arasindasin!", bid);
	} else {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[1V1] Hata: %s", result.c_str());
	}
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);
	return true;
}
