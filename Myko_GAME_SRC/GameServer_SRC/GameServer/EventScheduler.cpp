// =========================================================================
// S115 — EVENT SCHEDULER (zamanlanmis turnuva: KAYIT -> BAHIS -> MAC)
// Yazan: CHIP | Tarih: 2026-05-28
// =========================================================================
// PATRON: "GM baslattigi gibi bahis aciliyor, kimsenin haberi yok, 120sn dusuk.
//          On-kayit calismasi olmali, ustu uste bildirim, anons."
//
// AKIS (her asama GM ayarli sure, asagidaki varsayilanlar):
//   REGISTRATION (600sn/10dk) -> anons: acildi / yari / son1dk
//   BETTING      (600sn/10dk) -> _TOURNAMENT_DATA olusur (aBettingPhase=true, savas YOK),
//                                bahis ACIK. anons: acildi / yari / son1dk
//   COUNTDOWN    (30sn)       -> geri sayim anonsu (10/5/3/2/1)
//   RUNNING      (1800sn/30dk)-> aTournamentisStarted=true (savas baslar), bahis KAPANIR
//   DONE
//
// Anlik /partyvs BOZULMAZ — bu AYRI sistem (/eventcreate ile).
// Bahis MAC ONCESI toplandigi icin manipulasyon yok (kimse "kim onde" gormeden bahis koyar).
// =========================================================================

#include "stdafx.h"

// =====================================================================
// Event yapisi
// =====================================================================
enum class EventPhase : uint8 {
	REGISTRATION = 0,
	BETTING      = 1,
	COUNTDOWN    = 2,
	RUNNING      = 3,
	DONE         = 4
};

enum class EventKind : uint8 {
	PARTY_VS = 0,   // 2 party (lider karakter adlari ile)
	CLAN_VS  = 1,   // 2 klan
	// BRACKET/LEAGUE ileride ayni scheduler'a baglanabilir (su an party/clan duello)
};

struct _SCHEDULED_EVENT {
	int32_t   eventID;
	EventKind kind;
	uint8     zoneID;
	EventPhase phase;
	time_t    phaseEndTime;      // bu asama ne zaman biter (UNIXTIME)

	// Sureler (GM ayarli, saniye)
	uint32    regSec;
	uint32    betSec;
	uint32    countdownSec;
	uint32    matchSec;          // dakika*60

	// Katilimcilar — PARTY: party ID + lider adi | CLAN: clan ID + adi
	uint16    redID;             // party ID veya clan ID
	uint16    blueID;
	std::string redName;         // duyuru icin (lider adi + " Party" veya klan adi)
	std::string blueName;

	std::string createdBy;       // GM

	// Anons takibi (her esikte 1 kez): bit flag — 1=acildi, 2=yari, 4=son1dk
	uint8     announceFlags;
};

static std::vector<_SCHEDULED_EVENT> g_events;
static std::recursive_mutex g_eventLock;
static int32_t g_nextEventID = 1;

// GM ayarli varsayilan sureler (saniye)
static uint32 g_defRegSec       = 600;   // 10 dk kayit
static uint32 g_defBetSec       = 600;   // 10 dk bahis
static uint32 g_defCountdownSec = 30;    // 30 sn geri sayim
static uint32 g_defMatchSec     = 1800;  // 30 dk mac

void SetEventDefaults(uint32 reg, uint32 bet, uint32 countdown, uint32 match)
{
	if (reg > 0)       g_defRegSec       = reg;
	if (bet > 0)       g_defBetSec       = bet;
	if (countdown > 0) g_defCountdownSec = countdown;
	if (match > 0)     g_defMatchSec     = match;
}

void GetEventDefaults(uint32& reg, uint32& bet, uint32& countdown, uint32& match)
{
	reg = g_defRegSec; bet = g_defBetSec; countdown = g_defCountdownSec; match = g_defMatchSec;
}

// =====================================================================
// Zone bir event tarafindan rezerve mi? (REGISTRATION asamasinda mac verisi
// henuz yok ama zone o event'e ait — anlik /partyvs/bracket/lig CARPMASIN).
// Diger dosyalar (TournamentSystem/Bracket/League) bunu cagirir.
// =====================================================================
bool IsZoneReservedByEvent(uint8 zoneID)
{
	std::lock_guard<std::recursive_mutex> lock(g_eventLock);
	for (auto& e : g_events) {
		if (e.zoneID == zoneID && e.phase != EventPhase::DONE) return true;
	}
	return false;
}

// =====================================================================
// HELPER — zone adi
// =====================================================================
static const char* EventZoneName(uint8 z)
{
	return (z == 77) ? "Ardream"   : (z == 78) ? "Ronark"    :
	       (z == 96) ? "PartyVs-1" : (z == 97) ? "PartyVs-2" :
	       (z == 98) ? "PartyVs-3" : (z == 99) ? "PartyVs-4" : "?";
}

// Server-wide anons (TR/EN tek satir)
static void EventNotice(const char* fmt, ...)
{
	char buf[400] = {0};
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
	va_end(ap);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());
}

// =====================================================================
// Event olustur (PARTY VS) — GM iki party liderinin KARAKTER adini verir
// Donus: eventID (>0 basarili)
// =====================================================================
int32_t CreatePartyVsEvent(uint8 zoneID, uint16 redPartyID, uint16 bluePartyID,
                           const std::string& redName, const std::string& blueName,
                           uint32 regSec, uint32 betSec, uint32 countdownSec, uint32 matchSec,
                           const std::string& createdBy)
{
	std::lock_guard<std::recursive_mutex> lock(g_eventLock);

	// Bu zone'da zaten event/mac var mi?
	for (auto& e : g_events) {
		if (e.zoneID == zoneID && e.phase != EventPhase::DONE) {
			printf("[EVENT] Zone %u zaten event'te (eventID=%d)\n", zoneID, e.eventID);
			return 0;
		}
	}
	if (g_pMain->m_ClanVsDataList.GetData(zoneID) != nullptr) {
		printf("[EVENT] Zone %u dolu (aktif mac var)\n", zoneID);
		return 0;
	}

	_SCHEDULED_EVENT e;
	e.eventID      = g_nextEventID++;
	e.kind         = EventKind::PARTY_VS;
	e.zoneID       = zoneID;
	e.phase        = EventPhase::REGISTRATION;
	e.regSec       = (regSec > 0) ? regSec : g_defRegSec;
	e.betSec       = (betSec > 0) ? betSec : g_defBetSec;
	e.countdownSec = (countdownSec > 0) ? countdownSec : g_defCountdownSec;
	e.matchSec     = (matchSec > 0) ? matchSec : g_defMatchSec;
	e.redID        = redPartyID;
	e.blueID       = bluePartyID;
	e.redName      = redName;
	e.blueName     = blueName;
	e.createdBy    = createdBy;
	e.announceFlags = 0;
	e.phaseEndTime = UNIXTIME + e.regSec;

	g_events.push_back(e);

	// KAYIT ACILDI anonsu (ilk)
	EventNotice(">> ETKINLIK YAKLASIYOR! %s vs %s (%s). Hazirlanin, %u dakika sonra bahisler aciliyor! (betting in %u min)",
		redName.c_str(), blueName.c_str(), EventZoneName(zoneID), e.regSec / 60, e.regSec / 60);

	LOG(LogCategory::LOG_GENERAL,
		"[EVENT CREATE] id=%d zone=%u %s vs %s reg=%u bet=%u cd=%u match=%u gm=%s",
		e.eventID, zoneID, redName.c_str(), blueName.c_str(),
		e.regSec, e.betSec, e.countdownSec, e.matchSec, createdBy.c_str());

	printf("[EVENT] CREATE id=%d zone=%u %s vs %s (reg=%u bet=%u cd=%u match=%u)\n",
		e.eventID, zoneID, redName.c_str(), blueName.c_str(),
		e.regSec, e.betSec, e.countdownSec, e.matchSec);
	return e.eventID;
}

// Event iptal (GM)
bool CancelEvent(int32_t eventID)
{
	std::lock_guard<std::recursive_mutex> lock(g_eventLock);
	for (auto it = g_events.begin(); it != g_events.end(); ++it) {
		if (it->eventID == eventID) {
			// BETTING/RUNNING asamasinda mac verisi olustuysa temizle + bahis iade
			if (it->phase == EventPhase::BETTING || it->phase == EventPhase::COUNTDOWN) {
				extern void CancelTournamentBets(uint8 zoneID);  // bahis iade
				CancelTournamentBets(it->zoneID);
				g_pMain->m_ClanVsDataList.DeleteData(it->zoneID);
			}
			EventNotice(">> ETKINLIK IPTAL! %s vs %s yapilmayacak. Bahisler geri verildi. (cancelled, refunded)",
				it->redName.c_str(), it->blueName.c_str());
			g_events.erase(it);
			return true;
		}
	}
	return false;
}

// =====================================================================
// BETTING asamasina gecis — mac verisi olustur (savas YOK, bahis ACIK)
// =====================================================================
static void EnterBettingPhase(_SCHEDULED_EVENT& e)
{
	// Zone bos mu teyit (event sirasinda baska mac girmis olabilir)
	if (g_pMain->m_ClanVsDataList.GetData(e.zoneID) != nullptr) {
		printf("[EVENT] BETTING: Zone %u dolu, event iptal id=%d\n", e.zoneID, e.eventID);
		e.phase = EventPhase::DONE;
		return;
	}

	_TOURNAMENT_DATA* pData = new _TOURNAMENT_DATA();
	pData->aTournamentZoneID      = e.zoneID;
	pData->aTournamentisStarted   = false;   // SAVAS YOK
	pData->aBettingPhase          = true;    // BAHIS ACIK
	pData->aTournamentisFinished  = false;
	pData->aTournamentTimer       = e.matchSec;  // mac suresi (RUNNING'de kullanilir)
	pData->aTournamentisAttackable = false;      // bahis asamasinda saldiri yok
	// ORPHAN GUARD deadline: betSec + countdownSec + 120sn buffer. EventScheduler normalde
	// bu sureden once RUNNING'e gecirir (bettingPhase=false yapar). Gecmezse (restart/kayip)
	// tick bu deadline'da betting data'yi temizler -> zone sonsuz kilit kalmaz.
	pData->aTournamentOutTimer    = UNIXTIME + (time_t)(e.betSec + e.countdownSec + 120);

	if (e.kind == EventKind::PARTY_VS) {
		pData->participantType        = 1;
		pData->aTournamentPartyNum[0] = e.redID;
		pData->aTournamentPartyNum[1] = e.blueID;
		pData->aTournamentClanNum[0]  = e.redID;   // bahis pool icin
		pData->aTournamentClanNum[1]  = e.blueID;
	} else {
		pData->participantType        = 0;
		pData->aTournamentClanNum[0]  = e.redID;
		pData->aTournamentClanNum[1]  = e.blueID;
	}

	if (!g_pMain->m_ClanVsDataList.PutData(e.zoneID, pData)) {
		delete pData;
		printf("[EVENT] BETTING: PutData fail zone=%u\n", e.zoneID);
		e.phase = EventPhase::DONE;
		return;
	}

	// Bahis penceresi ac — betSec boyunca acik
	extern void OpenTournamentBetsForDuration(uint8 zoneID, time_t durationSec);
	OpenTournamentBetsForDuration(e.zoneID, (time_t)e.betSec);

	EventNotice(">> BAHISLER ACILDI! %s vs %s (%s). %u dakika sure! Bahis yapmak icin yaz: +bet red MIKTAR ya da +bet blue MIKTAR. (betting %u min)",
		e.redName.c_str(), e.blueName.c_str(), EventZoneName(e.zoneID), e.betSec / 60, e.betSec / 60);
}

// =====================================================================
// RUNNING asamasina gecis — savas baslar, bahis kapanir
// =====================================================================
static void EnterRunningPhase(_SCHEDULED_EVENT& e)
{
	_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(e.zoneID);
	if (info == nullptr) {
		printf("[EVENT] RUNNING: mac verisi yok zone=%u, event iptal id=%d\n", e.zoneID, e.eventID);
		e.phase = EventPhase::DONE;
		return;
	}

	// Bahis KAPAT (pencereyi simdiye cek -> CheckBetWindowClose "BAHIS KAPANDI" duyurusu yapar)
	extern void ForceCloseBetWindow(uint8 zoneID);
	ForceCloseBetWindow(e.zoneID);

	// SAVAS BASLAT
	info->aBettingPhase        = false;
	info->aTournamentisStarted = true;
	info->aTournamentisAttackable = true;
	info->aTournamentTimer     = e.matchSec;
	info->aTournamentOutTimer  = 0;   // betting orphan deadline'i temizle (artik savas, Faz 2 yonetir)
	info->aTournamentScoreBoard[0] = 0;
	info->aTournamentScoreBoard[1] = 0;

	// Katilimcilari zone'a cagir + roster doldur
	if (e.kind == EventKind::PARTY_VS) {
		extern int SummonPartyToZone(uint8 zoneID, uint16 redPartyID, uint16 bluePartyID);
		SummonPartyToZone(e.zoneID, e.redID, e.blueID);
		// roster (DC reconnect)
		_PARTY_GROUP* pRP = g_pMain->GetPartyPtr(e.redID);
		_PARTY_GROUP* pBP = g_pMain->GetPartyPtr(e.blueID);
		for (int i = 0; i < MAX_PARTY_USERS; i++) {
			if (pRP) { CUser* u = g_pMain->GetUserPtr(pRP->uid[i]); if (u) info->rosterRed.insert(u->GetName()); }
			if (pBP) { CUser* u = g_pMain->GetUserPtr(pBP->uid[i]); if (u) info->rosterBlue.insert(u->GetName()); }
		}
	} else {
		extern void SummonClanMembersToZone(uint8 zoneID, uint16 redClanID, uint16 blueClanID);
		SummonClanMembersToZone(e.zoneID, e.redID, e.blueID);
	}

	EventNotice(">> MAC BASLADI! %s vs %s (%s). Savas %u dakika surecek! Savasin! (MATCH STARTED)",
		e.redName.c_str(), e.blueName.c_str(), EventZoneName(e.zoneID), e.matchSec / 60);

	LOG(LogCategory::LOG_GENERAL,
		"[EVENT RUNNING] id=%d zone=%u %s vs %s match=%u sn",
		e.eventID, e.zoneID, e.redName.c_str(), e.blueName.c_str(), e.matchSec);
}

// =====================================================================
// Asamali anons (her asamada: acildi/yari/son1dk esiklerinde 1 kez)
// =====================================================================
static void PhaseAnnounce(_SCHEDULED_EVENT& e, const char* phaseName, time_t remaining)
{
	// yari (bit 2)
	uint32 totalSec = (e.phase == EventPhase::REGISTRATION) ? e.regSec :
	                  (e.phase == EventPhase::BETTING) ? e.betSec : 0;
	if (totalSec == 0) return;

	if ((e.announceFlags & 2) == 0 && remaining <= (time_t)(totalSec / 2)) {
		e.announceFlags |= 2;
		EventNotice(">> %s vs %s - %s asamasi: %lld dakika kaldi! (%lld min left)",
			e.redName.c_str(), e.blueName.c_str(), phaseName,
			(long long)(remaining / 60 + 1), (long long)(remaining / 60 + 1));
	}
	// son 1 dk (bit 4)
	if ((e.announceFlags & 4) == 0 && remaining <= 60) {
		e.announceFlags |= 4;
		EventNotice(">> %s vs %s - %s asamasi: SON 1 DAKIKA! Acele edin! (LAST 1 MINUTE)",
			e.redName.c_str(), e.blueName.c_str(), phaseName);
	}
}

// =====================================================================
// TIMER — her saniye GameEventMainTimer'dan cagrilir
// =====================================================================
void EventSchedulerTimer()
{
	std::lock_guard<std::recursive_mutex> lock(g_eventLock);
	time_t now = UNIXTIME;

	for (size_t i = 0; i < g_events.size(); i++) {
		_SCHEDULED_EVENT& e = g_events[i];
		if (e.phase == EventPhase::DONE) continue;

		time_t remaining = (e.phaseEndTime > now) ? (e.phaseEndTime - now) : 0;

		switch (e.phase) {
		case EventPhase::REGISTRATION:
			PhaseAnnounce(e, "KAYIT/REG", remaining);
			if (now >= e.phaseEndTime) {
				e.phase = EventPhase::BETTING;
				e.phaseEndTime = now + e.betSec;
				e.announceFlags = 0;  // yeni asama, anons reset
				EnterBettingPhase(e);
			}
			break;

		case EventPhase::BETTING:
			PhaseAnnounce(e, "BAHIS/BET", remaining);
			if (now >= e.phaseEndTime) {
				e.phase = EventPhase::COUNTDOWN;
				e.phaseEndTime = now + e.countdownSec;
				e.announceFlags = 0;
			}
			break;

		case EventPhase::COUNTDOWN: {
			// Geri sayim anonsu (10/5/3/2/1)
			if (remaining == 10 || remaining == 5 || remaining == 3 || remaining == 2 || remaining == 1) {
				EventNotice(">> %s vs %s - Mac %lld saniye sonra basliyor! Hazir olun! (starts in %lld)",
					e.redName.c_str(), e.blueName.c_str(), (long long)remaining, (long long)remaining);
			}
			if (now >= e.phaseEndTime) {
				e.phase = EventPhase::RUNNING;
				EnterRunningPhase(e);
				// RUNNING asamasi suresi mac motoru (TickOneTournamentZone) tarafindan
				// yurutulur (aTournamentTimer). Event burada DONE'a gecer, mac kendi biter.
				e.phase = EventPhase::DONE;
			}
			break;
		}

		default:
			break;
		}
	}

	// DONE event'leri temizle (vector compact)
	for (auto it = g_events.begin(); it != g_events.end(); ) {
		if (it->phase == EventPhase::DONE) it = g_events.erase(it);
		else ++it;
	}
}

// Aktif event'leri listele (GM status)
void ListEvents()
{
	std::lock_guard<std::recursive_mutex> lock(g_eventLock);
	printf("====== AKTIF EVENT'LER (%zu) ======\n", g_events.size());
	for (auto& e : g_events) {
		const char* ph = (e.phase == EventPhase::REGISTRATION) ? "KAYIT" :
		                 (e.phase == EventPhase::BETTING) ? "BAHIS" :
		                 (e.phase == EventPhase::COUNTDOWN) ? "GERISAYIM" :
		                 (e.phase == EventPhase::RUNNING) ? "MAC" : "BITTI";
		time_t rem = (e.phaseEndTime > UNIXTIME) ? (e.phaseEndTime - UNIXTIME) : 0;
		printf("  id=%d zone=%u %s vs %s | asama=%s kalan=%llds\n",
			e.eventID, e.zoneID, e.redName.c_str(), e.blueName.c_str(), ph, (long long)rem);
	}
	printf("===================================\n");
}

// =====================================================================
// CONSOLE KOMUTLARI
// =====================================================================
#include "GameServerDlg.h"

// /eventcreate <RedLider> <BlueLider> <Zone> [regDk] [betDk] [macDk]
// Zamanlanmis party vs event (KAYIT->BAHIS->MAC asamali). Sureler dakika, opsiyonel (varsayilan kullanir).
COMMAND_HANDLER(CGameServerDlg::HandleEventCreateCommand)
{
	if (vargs.size() < 3) {
		printf("Usage: /eventcreate <RedLider> <BlueLider> <Zone(96-99/77/78)> [regDk] [betDk] [macDk]\n");
		printf("  Ornek: /eventcreate Ahmet Mehmet 96 10 10 30  (10dk kayit, 10dk bahis, 30dk mac)\n");
		printf("  Sure girilmezse varsayilan: 10/10/30 dk\n");
		return true;
	}
	std::string redLeader  = vargs.front(); vargs.pop_front();
	std::string blueLeader = vargs.front(); vargs.pop_front();
	uint8 zoneID = (uint8)SafeAtoi(vargs.front(), 1, 255); vargs.pop_front();

	uint32 regSec = 0, betSec = 0, matchSec = 0;
	if (!vargs.empty()) { regSec   = (uint32)SafeAtoi(vargs.front(), 1, 120) * 60; vargs.pop_front(); }
	if (!vargs.empty()) { betSec   = (uint32)SafeAtoi(vargs.front(), 1, 120) * 60; vargs.pop_front(); }
	if (!vargs.empty()) { matchSec = (uint32)SafeAtoi(vargs.front(), 1, 120) * 60; }

	CUser* pRL = GetUserPtr(redLeader, NameType::TYPE_CHARACTER);
	CUser* pBL = GetUserPtr(blueLeader, NameType::TYPE_CHARACTER);
	if (pRL == nullptr || !pRL->isInGame()) { printf("[EVENT] Red lider offline/yok: %s\n", redLeader.c_str()); return true; }
	if (pBL == nullptr || !pBL->isInGame()) { printf("[EVENT] Blue lider offline/yok: %s\n", blueLeader.c_str()); return true; }
	if (!pRL->isInParty()) { printf("[EVENT] %s party'de degil\n", redLeader.c_str()); return true; }
	if (!pBL->isInParty()) { printf("[EVENT] %s party'de degil\n", blueLeader.c_str()); return true; }

	uint16 redPartyID  = (uint16)pRL->GetPartyID();
	uint16 bluePartyID = (uint16)pBL->GetPartyID();
	if (redPartyID == bluePartyID) { printf("[EVENT] Ikisi de ayni party'de\n"); return true; }

	std::string redName  = pRL->GetName() + " Party";
	std::string blueName = pBL->GetName() + " Party";

	extern int32_t CreatePartyVsEvent(uint8, uint16, uint16, const std::string&, const std::string&,
	                                  uint32, uint32, uint32, uint32, const std::string&);
	int32_t eid = CreatePartyVsEvent(zoneID, redPartyID, bluePartyID, redName, blueName,
	                                 regSec, betSec, 0, matchSec, "console");
	if (eid > 0) printf("[EVENT] Olusturuldu: eventID=%d (KAYIT->BAHIS->MAC otomatik)\n", eid);
	else printf("[EVENT] Olusturulamadi (zone dolu/event var)\n");
	return true;
}

// /eventconfig <regDk> <betDk> <geriSayimSn> <macDk> — varsayilan sureleri ayarla
COMMAND_HANDLER(CGameServerDlg::HandleEventConfigCommand)
{
	if (vargs.size() < 4) {
		uint32 r, b, c, m;
		extern void GetEventDefaults(uint32&, uint32&, uint32&, uint32&);
		GetEventDefaults(r, b, c, m);
		printf("Mevcut varsayilan: kayit=%udk bahis=%udk geriSayim=%usn mac=%udk\n", r/60, b/60, c, m/60);
		printf("Usage: /eventconfig <regDk> <betDk> <geriSayimSn> <macDk>\n");
		return true;
	}
	uint32 reg = (uint32)SafeAtoi(vargs.front(), 1, 120) * 60; vargs.pop_front();
	uint32 bet = (uint32)SafeAtoi(vargs.front(), 1, 120) * 60; vargs.pop_front();
	uint32 cd  = (uint32)SafeAtoi(vargs.front(), 5, 300);      vargs.pop_front();
	uint32 mat = (uint32)SafeAtoi(vargs.front(), 1, 120) * 60;
	extern void SetEventDefaults(uint32, uint32, uint32, uint32);
	SetEventDefaults(reg, bet, cd, mat);
	printf("[EVENT] Varsayilan ayarlandi: kayit=%udk bahis=%udk geriSayim=%usn mac=%udk\n",
		reg/60, bet/60, cd, mat/60);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleEventListCommand)
{
	extern void ListEvents();
	ListEvents();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleEventCancelCommand)
{
	if (vargs.empty()) { printf("Usage: /eventcancel <eventID>\n"); return true; }
	int32_t eid = SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	extern bool CancelEvent(int32_t);
	if (CancelEvent(eid)) printf("[EVENT] Iptal edildi: eventID=%d\n", eid);
	else printf("[EVENT] Bulunamadi: eventID=%d\n", eid);
	return true;
}

// =====================================================================
// PARTY BRACKET / LIG komutlari (DB persist'li — restart-safe)
// =====================================================================
extern int32_t CreateBracket(const std::string&, uint8, const std::string&, uint8);
extern int32_t CreateLeague(const std::string&, uint8, const std::string&, uint8);
extern bool RegisterPartyToBracket(int32_t bracketID, uint16 partyID, const std::string& leaderName, uint8 memberCount);
extern bool RegisterPartyToLeague(int32_t leagueID, uint16 partyID, const std::string& leaderName);
extern bool StartBracket(int32_t bracketID);
extern bool StartLeague(int32_t leagueID);

// /partybracketcreate "Ad" MaxParti(4/8/16) — party eleme turnuvasi olustur
COMMAND_HANDLER(CGameServerDlg::HandlePartyBracketCreateCommand)
{
	if (vargs.size() < 2) { printf("Usage: /partybracketcreate \"Ad\" MaxParti(4/8/16)\n"); return true; }
	std::string name = vargs.front(); vargs.pop_front();
	uint8 maxP = (uint8)SafeAtoi(vargs.front(), 4, 32);
	int32_t bid = CreateBracket(name, maxP, "console", 1);  // participantType=1 PARTY
	if (bid > 0) printf("[PARTY BRACKET] Olusturuldu: ID=%d (party liderleri +partybracketreg %d ile kayit)\n", bid, bid);
	else printf("[PARTY BRACKET] Olusturulamadi (maxParti 4/8/16/32 olmali)\n");
	return true;
}

// /partyleaguecreate "Ad" MaxParti(3-8) — party lig olustur
COMMAND_HANDLER(CGameServerDlg::HandlePartyLeagueCreateCommand)
{
	if (vargs.size() < 2) { printf("Usage: /partyleaguecreate \"Ad\" MaxParti(3-8)\n"); return true; }
	std::string name = vargs.front(); vargs.pop_front();
	uint8 maxP = (uint8)SafeAtoi(vargs.front(), 3, 8);
	int32_t lid = CreateLeague(name, maxP, "console", 1);  // participantType=1 PARTY
	if (lid > 0) printf("[PARTY LEAGUE] Olusturuldu: ID=%d (party liderleri +partyleaguereg %d ile kayit)\n", lid, lid);
	else printf("[PARTY LEAGUE] Olusturulamadi\n");
	return true;
}

// /partybracketstart <ID> | /partyleaguestart <ID> — fikstur olustur, maclar otomatik baslar
COMMAND_HANDLER(CGameServerDlg::HandlePartyBracketStartCommand)
{
	if (vargs.empty()) { printf("Usage: /partybracketstart <ID>\n"); return true; }
	int32_t bid = SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	if (StartBracket(bid)) printf("[PARTY BRACKET] Basladi: ID=%d (maclar AutoStartTimer ile)\n", bid);
	else printf("[PARTY BRACKET] Baslatilamadi ID=%d\n", bid);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandlePartyLeagueStartCommand)
{
	if (vargs.empty()) { printf("Usage: /partyleaguestart <ID>\n"); return true; }
	int32_t lid = SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	if (StartLeague(lid)) printf("[PARTY LEAGUE] Basladi: ID=%d\n", lid);
	else printf("[PARTY LEAGUE] Baslatilamadi ID=%d\n", lid);
	return true;
}

// +partybracketreg <ID> — party lideri party'sini bracket'a kaydeder (oyuncu komutu)
COMMAND_HANDLER(CUser::HandlePartyBracketRegCommand)
{
	if (vargs.empty()) { g_pMain->SendHelpDescription(this, "Kullanim: +partybracketreg <ID>"); return true; }
	if (!isInParty()) { g_pMain->SendHelpDescription(this, "Party'de degilsin. | Not in a party."); return true; }
	if (!isPartyLeader()) { g_pMain->SendHelpDescription(this, "Sadece party lideri kayit yapar. | Only party leader."); return true; }
	int32_t bid = SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	uint16 partyID = (uint16)GetPartyID();
	uint8 memberCount = GetPartyMemberAmount();
	if (RegisterPartyToBracket(bid, partyID, GetName(), memberCount))
		g_pMain->SendHelpDescription(this, "Party bracket'a kaydedildi! | Registered to party bracket.");
	else
		g_pMain->SendHelpDescription(this, "Kayit basarisiz (bracket yok/dolu/REGISTRATION disi). | Registration failed.");
	return true;
}

// +partyleaguereg <ID> — party lideri party'sini lige kaydeder
COMMAND_HANDLER(CUser::HandlePartyLeagueRegCommand)
{
	if (vargs.empty()) { g_pMain->SendHelpDescription(this, "Kullanim: +partyleaguereg <ID>"); return true; }
	if (!isInParty()) { g_pMain->SendHelpDescription(this, "Party'de degilsin. | Not in a party."); return true; }
	if (!isPartyLeader()) { g_pMain->SendHelpDescription(this, "Sadece party lideri kayit yapar. | Only party leader."); return true; }
	int32_t lid = SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	uint16 partyID = (uint16)GetPartyID();
	if (RegisterPartyToLeague(lid, partyID, GetName()))
		g_pMain->SendHelpDescription(this, "Party lige kaydedildi! | Registered to party league.");
	else
		g_pMain->SendHelpDescription(this, "Kayit basarisiz (lig yok/dolu/REGISTRATION disi). | Registration failed.");
	return true;
}
