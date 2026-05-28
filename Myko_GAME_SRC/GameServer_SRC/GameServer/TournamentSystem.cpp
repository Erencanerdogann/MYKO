#include "stdafx.h"

// =====================================================================
// S115 — Ortak helper: Klan uyelerini tournament zone'una otomatik cagir
// (Bracket + League auto-start maclarinda kullanilir; manuel /tournamentstart
//  zaten kendi icinde yapiyor — bu helper kod tekrarini onler)
// Red sol-base, Blue sag-base ayri spawn (clan war layout).
// =====================================================================
void SummonClanMembersToZone(uint8 zoneID, uint16 redClanID, uint16 blueClanID)
{
	float redX = 0.0f, redZ = 0.0f, blueX = 0.0f, blueZ = 0.0f;
	switch (zoneID)
	{
		case 77: case 78:  // Clan War (buyuk map)
			redX = 400.0f;  redZ = 1600.0f;
			blueX = 1600.0f; blueZ = 400.0f;
			break;
		case 96: case 97: case 98: case 99:  // Party VS (kucuk arena)
			redX = 80.0f;   redZ = 130.0f;
			blueX = 175.0f; blueZ = 130.0f;
			break;
		default:
			redX = blueX = 1000.0f; redZ = blueZ = 1000.0f;
			break;
	}

	int warpedCount = 0;
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pTarget = g_pMain->GetUserPtr(i);
		if (pTarget == nullptr || !pTarget->isInGame()) continue;

		uint16 tClanID = pTarget->GetClanID();
		if (tClanID != redClanID && tClanID != blueClanID) continue;
		if (pTarget->GetZoneID() == zoneID) continue;  // zaten zone'da

		bool isRed = (tClanID == redClanID);
		pTarget->ZoneChange(zoneID, isRed ? redX : blueX, isRed ? redZ : blueZ);

		std::string privNotice = isRed
			? "[TURNUVA / MATCH] Base'e aktarildin, savasa hazirlan! | Warped to base, get ready!"
			: "[TURNUVA / MATCH] Base'e aktarildin, savasa hazirlan! | Warped to base, get ready!";
		Packet privPkt;
		ChatPacket::Construct(&privPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &privNotice);
		pTarget->Send(&privPkt);
		warpedCount++;
	}
	printf("[TOURNAMENT] SummonClanMembers Zone=%u Red=%u Blue=%u -> %d uye cagirildi\n",
		zoneID, redClanID, blueClanID, warpedCount);
}

// =====================================================================
// S115 — PARTY VS PARTY: party uyelerini zone'a cagir (clan'in party versiyonu)
// patron: "party leaderleri cekilince butun party all alana cekilmeli"
// pParty->uid[0..MAX_PARTY_USERS] -> tum party uyeleri warp.
// Red party sol-base, Blue party sag-base (clan war layout ile ayni).
// Donus: kac uye warp edildi (Red + Blue toplam)
// =====================================================================
int SummonPartyToZone(uint8 zoneID, uint16 redPartyID, uint16 bluePartyID)
{
	float redX = 0.0f, redZ = 0.0f, blueX = 0.0f, blueZ = 0.0f;
	switch (zoneID)
	{
		case 77: case 78:
			redX = 400.0f;  redZ = 1600.0f;
			blueX = 1600.0f; blueZ = 400.0f;
			break;
		case 96: case 97: case 98: case 99:
			redX = 80.0f;   redZ = 130.0f;
			blueX = 175.0f; blueZ = 130.0f;
			break;
		default:
			redX = blueX = 1000.0f; redZ = blueZ = 1000.0f;
			break;
	}

	int warpedCount = 0;
	// Iki party'yi sirayla isle (Red sonra Blue)
	for (int side = 0; side < 2; side++)
	{
		uint16 partyID = (side == 0) ? redPartyID : bluePartyID;
		if (partyID == 0xFFFF || partyID == 0) continue;

		_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(partyID);
		if (pParty == nullptr) continue;

		bool isRed = (side == 0);
		for (int i = 0; i < MAX_PARTY_USERS; i++)
		{
			CUser* pTarget = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pTarget == nullptr || !pTarget->isInGame()) continue;
			if (pTarget->GetZoneID() == zoneID) continue;  // zaten zone'da

			pTarget->ZoneChange(zoneID, isRed ? redX : blueX, isRed ? redZ : blueZ);

			std::string privNotice =
				"[PARTY VS] Base'e aktarildin, savasa hazirlan! | Warped to base, get ready!";
			Packet privPkt;
			ChatPacket::Construct(&privPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &privNotice);
			pTarget->Send(&privPkt);
			warpedCount++;
		}
	}
	printf("[PARTY VS] SummonParty Zone=%u Red=%u Blue=%u -> %d uye cagirildi\n",
		zoneID, redPartyID, bluePartyID, warpedCount);
	return warpedCount;
}

// =====================================================================
// S115 — PARTY VS PARTY duello baslatma (2 party tek mac)
// Clan tournament motorunun party versiyonu. participantType=1 ile isaretlenir.
// HandleTournamentEnd party tipini taniyip party odulu dagitir.
// redPartyLeaderID / bluePartyLeaderID = party liderlerinin user ID'si
// Donus: true = basladi
// =====================================================================
bool StartPartyVsMatch(uint8 zoneID, uint16 redPartyID, uint16 bluePartyID,
                       uint16 durationMin, const std::string& startedBy)
{
	_PARTY_GROUP* pRedParty  = g_pMain->GetPartyPtr(redPartyID);
	_PARTY_GROUP* pBlueParty = g_pMain->GetPartyPtr(bluePartyID);
	if (pRedParty == nullptr || pBlueParty == nullptr) {
		printf("[PARTY VS] Start: party yok (red=%u blue=%u)\n", redPartyID, bluePartyID);
		return false;
	}
	if (redPartyID == bluePartyID) {
		printf("[PARTY VS] Start: ayni party secilemez\n");
		return false;
	}

	// Zone bos mu? (clan tournament ile cakisma onle)
	if (g_pMain->m_ClanVsDataList.GetData(zoneID) != nullptr) {
		printf("[PARTY VS] Start: Zone %u dolu, baska mac var\n", zoneID);
		return false;
	}
	// Zone bir event'e rezerve mi (REGISTRATION asamasinda mac verisi yok ama zone o event'in)
	{
		extern bool IsZoneReservedByEvent(uint8 zoneID);
		if (IsZoneReservedByEvent(zoneID)) {
			printf("[PARTY VS] Start: Zone %u bir event'e rezerve, anlik mac baslamaz\n", zoneID);
			return false;
		}
	}

	// Lider isimleri (duyuru icin) — uid[0] genelde lider, ama isPartyLeader teyit
	std::string redName = "RedParty", blueName = "BlueParty";
	{
		CUser* pRL = g_pMain->GetUserPtr(pRedParty->uid[0]);
		if (pRL != nullptr) redName = pRL->GetName() + " Party";
		CUser* pBL = g_pMain->GetUserPtr(pBlueParty->uid[0]);
		if (pBL != nullptr) blueName = pBL->GetName() + " Party";
	}

	_TOURNAMENT_DATA* pData = new _TOURNAMENT_DATA();
	pData->aTournamentZoneID         = zoneID;
	pData->participantType           = 1;  // PARTY
	pData->aTournamentPartyNum[0]    = redPartyID;
	pData->aTournamentPartyNum[1]    = bluePartyID;
	// Bahis pool hesabi aTournamentClanNum okur (betClanID ile eslesir). Party ID'yi
	// buraya da yaz -> bahis kodu party ID'yi "katilimci ID" olarak kullanir, ResolveTournamentBets
	// party ID ile cozer, eslesme tutar. (+bet komutu FAZ 5'te party-aware olacak.)
	pData->aTournamentClanNum[0]     = redPartyID;
	pData->aTournamentClanNum[1]     = bluePartyID;
	pData->aTournamentTimer          = (uint32)durationMin * 60;
	pData->aTournamentisAttackable   = true;
	pData->aTournamentisStarted      = true;
	pData->aTournamentisFinished     = false;

	// DC RECONNECT — roster: katilimci karakter adlari (DC olan geri girince isimle warp)
	for (int i = 0; i < MAX_PARTY_USERS; i++) {
		CUser* pR = g_pMain->GetUserPtr(pRedParty->uid[i]);
		if (pR != nullptr) pData->rosterRed.insert(pR->GetName());
		CUser* pB = g_pMain->GetUserPtr(pBlueParty->uid[i]);
		if (pB != nullptr) pData->rosterBlue.insert(pB->GetName());
	}

	if (!g_pMain->m_ClanVsDataList.PutData(zoneID, pData)) {
		delete pData;
		printf("[PARTY VS] Start: PutData fail (Zone=%u)\n", zoneID);
		return false;
	}

	// Bahis penceresi ac (turnuva harici party vs'de dahi disardan bahis)
	extern void OpenTournamentBets(uint8 zoneID);
	OpenTournamentBets(zoneID);

	// Party uyelerini otomatik zone'a cagir (patron: "butun party all alana cekilmeli")
	SummonPartyToZone(zoneID, redPartyID, bluePartyID);

	char buf[300] = {0};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[PARTY VS / PARTY MATCH] %s vs %s @ Zone %u (%u dk/min)! Bahis acik / Betting open!",
		redName.c_str(), blueName.c_str(), zoneID, durationMin);
	std::string msg = buf;
	g_pMain->SendNotice(msg.c_str());

	printf("[PARTY VS] START Zone=%u Red=%u Blue=%u dk=%u by=%s\n",
		zoneID, redPartyID, bluePartyID, durationMin, startedBy.c_str());
	return true;
}

// S115 Plan A — Helper: Tournament zone'una scoreboard + timer paketi yayinla
// Her 5 saniyede bir cagrilir (spam onlemi). Score paketinin disinda timer'in da
// client UI'da AKMASI icin gerekli (zone giriste tek seferlik gonderim yetmez).
static void SendTournamentScorePacket(_TOURNAMENT_DATA* info)
{
	if (info == nullptr || !info->aTournamentisStarted)
		return;

	Packet pkt(WIZ_BATTLE_EVENT);
	pkt << uint8(0x12)
	    << uint8(2)                                  // 2=Board1
	    << info->aTournamentScoreBoard[0]            // Red score
	    << info->aTournamentScoreBoard[1]            // Blue score
	    << info->aTournamentTimer                    // Timer (saniye)
	    << info->aTournamentMonumentKilled;          // Monument advantage
	g_pMain->Send_Zone(&pkt, info->aTournamentZoneID);

	Packet pktTimer(WIZ_BIFROST);
	pktTimer << uint8(5) << uint16(info->aTournamentTimer);
	g_pMain->Send_Zone(&pktTimer, info->aTournamentZoneID);
}

// S115 — Champion's Treasure Chest: kazanan klan uyelerine DB-driven item
// _MK_TOURNAMENT_REWARDS tablosundan zone+position bazli okunur.
// Eger DB bos donerse hicbir item verilmez (yalnizca gold + np)

// S115 Plan A B9 — Helper: Kazanan klan uyelerine odul dagit (gold + np + chest)
// winnerClanID = 0 ise berabere -> kucuk odul iki klana
static void DistributeTournamentRewards(uint16 winnerClanID, uint16 loserClanID,
                                        uint8 zoneID, uint16 winnerScore, uint16 loserScore)
{
	// Base odul sabitleri (DB tablo `_MK_TOURNAMENT_REWARDS` ile chest item dinamik gelir, asagida)
	const uint32 WINNER_GOLD = 5000000;   // 5M Noah
	const uint32 WINNER_NP   = 1000;       // 1000 NP
	const uint32 LOSER_GOLD  = 1000000;   // 1M Noah (katilim)
	const uint32 LOSER_NP    = 200;        // 200 NP

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		uint16 cid = pUser->GetClanID();
		if (cid == 0) continue;
		if (cid != winnerClanID && cid != loserClanID) continue;

		bool isWinner = (cid == winnerClanID);
		uint32 gold = isWinner ? WINNER_GOLD : LOSER_GOLD;
		uint32 np   = isWinner ? WINNER_NP   : LOSER_NP;

		pUser->GoldGain(gold);
		pUser->SendLoyaltyChange("tournament", (int32)np, false, false, false);

		// S115 — Champion's Treasure Chest (DB-driven, kazanan + kaybeden + berabere ayri)
		// Position'a gore item listesi cek + random sec + ver
		std::string position;
		if (winnerClanID == 0) position = "DRAW";
		else if (isWinner)     position = "WINNER";
		else                   position = "LOSER";

		std::vector<std::pair<uint32, uint16>> rewards;
		if (g_DBAgent.LoadTournamentRewards(zoneID, position, rewards) && !rewards.empty())
		{
			// Random secim (ileride chance/weight eklenebilir; su an her item ayni sansa)
			size_t idx = rand() % rewards.size();
			uint32 itemID = rewards[idx].first;
			uint16 itemCount = rewards[idx].second;
			if (itemID > 0 && itemCount > 0)
				pUser->GiveItem("tournament_chest", itemID, itemCount, true, 0);
		}

		std::string msg = isWinner
			? "[CLAN WAR] Tebrikler! Klanin kazandi. Champion's Treasure Chest aldin!"
			: "[CLAN WAR] Iyi savas! Katilim odulun aldin.";
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		pUser->Send(&pkt);
	}
}

// =====================================================================
// S115 — GM MANUEL ODUL (patron: "kazanana GM belirledigi odulu manuel verir")
// Otomatik DistributeTournamentRewards'tan FARK: GM O AN miktari/tipi secer.
// rewardType: 0=Noah(default) 1=Item 2=NP
// targetType: 0=Klan tum uyeleri 1=Klan lideri 2=Tek oyuncu(isim)
// targetClanID: targetType 0/1 icin | targetName: targetType 2 icin
// amount: Noah miktari / NP miktari | itemID+itemCount: Item icin
// Aninda teslim — online hedeflere direkt yansir, offline LOG'lanir.
// Donus: kac kisiye verildi (0 = hedef bulunamadi/offline)
// =====================================================================
int GMGiveTournamentReward(uint8 rewardType, uint8 targetType,
                           uint16 targetClanID, const std::string& targetName,
                           uint32 amount, uint32 itemID, uint16 itemCount,
                           const std::string& gmName)
{
	int givenCount = 0;

	// Esnek odul verme — tek user'a uygular (icerde tip secimi)
	auto giveOne = [&](CUser* pU) -> bool {
		if (pU == nullptr || !pU->isInGame()) return false;
		switch (rewardType) {
			case 1:  // Item
				if (itemID > 0 && itemCount > 0)
					pU->GiveItem("gm_tournament_reward", itemID, itemCount, true, 0);
				break;
			case 2:  // NP
				pU->SendLoyaltyChange("gm_tournament_reward", (int32)amount, false, false, false);
				break;
			default: // 0 = Noah (default)
				pU->GoldGain(amount);
				break;
		}
		// Bildirim
		char nbuf[200] = {0};
		if (rewardType == 1)
			_snprintf_s(nbuf, sizeof(nbuf), _TRUNCATE,
				"[GM ODUL / GM REWARD] Item #%u x%u verildi! | Item granted.", itemID, itemCount);
		else if (rewardType == 2)
			_snprintf_s(nbuf, sizeof(nbuf), _TRUNCATE,
				"[GM ODUL / GM REWARD] +%u NP verildi! | National Points granted.", amount);
		else
			_snprintf_s(nbuf, sizeof(nbuf), _TRUNCATE,
				"[GM ODUL / GM REWARD] +%u Noah verildi! | Gold granted.", amount);
		std::string m = nbuf;
		Packet p;
		ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m);
		pU->Send(&p);
		return true;
	};

	if (targetType == 2) {
		// Tek oyuncu (isim)
		CUser* pU = g_pMain->GetUserPtr(targetName, NameType::TYPE_CHARACTER);
		if (giveOne(pU)) givenCount++;
	} else {
		// Klan (tum uyeler veya sadece lider)
		CKnights* pClan = g_pMain->GetClanPtr(targetClanID);
		std::string leaderName = (pClan != nullptr) ? pClan->m_strChief : "";
		for (uint16 i = 0; i < MAX_USER; i++) {
			CUser* pU = g_pMain->GetUserPtr(i);
			if (pU == nullptr || !pU->isInGame()) continue;
			if (pU->GetClanID() != targetClanID) continue;
			if (targetType == 1 && pU->GetName() != leaderName) continue;  // sadece lider
			if (giveOne(pU)) givenCount++;
		}
	}

	// KALICI LOG (GM kim, ne verdi, kime, kac kisi — denetim kaniti)
	const char* rtName = (rewardType == 1) ? "ITEM" : (rewardType == 2) ? "NP" : "NOAH";
	const char* ttName = (targetType == 1) ? "KLAN_LIDER" : (targetType == 2) ? "OYUNCU" : "KLAN_TUMU";
	LOG(LogCategory::LOG_GM,
		"[GM REWARD] gm=%s tip=%s hedef=%s clan=%u oyuncu=%s amount=%u item=%u x%u -> %d kisiye verildi",
		gmName.c_str(), rtName, ttName, targetClanID,
		targetName.empty() ? "-" : targetName.c_str(),
		amount, itemID, itemCount, givenCount);

	return givenCount;
}

// =====================================================================
// S115 — PARTY ODUL: kazanan party LIDERINE toplu odul (patron karari)
// Lider bolustur. Default 5M Noah + 1000 NP (clan ile ayni base).
// Berabere: iki party liderine katilim (1M Noah).
// =====================================================================
static void DistributePartyRewards(uint16 winnerPartyID, uint16 loserPartyID, bool isDraw)
{
	const uint32 WINNER_GOLD = 5000000, WINNER_NP = 1000;
	const uint32 DRAW_GOLD   = 1000000, DRAW_NP   = 200;

	auto giveToLeader = [&](uint16 partyID, uint32 gold, uint32 np, const char* label) {
		_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(partyID);
		if (pParty == nullptr) return;
		// Lider = uid[0] (party olusturan), isPartyLeader ile teyit
		CUser* pLeader = nullptr;
		for (int i = 0; i < MAX_PARTY_USERS; i++) {
			CUser* pU = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pU != nullptr && pU->isInGame() && pU->isPartyLeader()) { pLeader = pU; break; }
		}
		if (pLeader == nullptr) {  // lider offline -> uid[0] fallback
			pLeader = g_pMain->GetUserPtr(pParty->uid[0]);
			if (pLeader == nullptr || !pLeader->isInGame()) return;
		}
		pLeader->GoldGain(gold);
		pLeader->SendLoyaltyChange("party_tournament", (int32)np, false, false, false);
		char nbuf[220] = {0};
		_snprintf_s(nbuf, sizeof(nbuf), _TRUNCATE,
			"[PARTY VS] %s! Toplu odul lidere: +%u Noah +%u NP (party ile bolustur). | Reward to leader, share with party.",
			label, gold, np);
		std::string m = nbuf;
		Packet p;
		ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m);
		pLeader->Send(&p);
		// KALICI LOG (party odul denetim kaniti)
		LOG(LogCategory::LOG_GENERAL,
			"[PARTY REWARD] party=%u lider=%s sonuc=%s gold=%u np=%u",
			partyID, pLeader->GetName().c_str(), label, gold, np);
	};

	if (isDraw) {
		giveToLeader(winnerPartyID, DRAW_GOLD, DRAW_NP, "Berabere/Draw");
		giveToLeader(loserPartyID,  DRAW_GOLD, DRAW_NP, "Berabere/Draw");
	} else {
		giveToLeader(winnerPartyID, WINNER_GOLD, WINNER_NP, "KAZANDINIZ/WON");
	}
}

// S115 Plan A — Helper: Tournament bittiginde duyuru + odul dagit
// Tek yerden tum sonuc mantığı (6 zone'a kopyali kod kaldirildi)
static void HandleTournamentEnd(_TOURNAMENT_DATA* info)
{
	if (info == nullptr) return;

	// S115 — PARTY VS PARTY: ayri akis (clan kodu calismasin, party ID'yi clan sanmasin)
	if (info->participantType == 1)
	{
		uint16 redScoreP  = info->aTournamentScoreBoard[0];
		uint16 blueScoreP = info->aTournamentScoreBoard[1];
		uint16 redPartyID  = info->aTournamentPartyNum[0];
		uint16 bluePartyID = info->aTournamentPartyNum[1];

		// Bahis resolve — kazanan party ID'si (bahis betClanID alanini party ID olarak kullanir)
		uint16 winnerPartyID = 0;
		bool isDraw = (redScoreP == blueScoreP);
		if (!isDraw) winnerPartyID = (redScoreP > blueScoreP) ? redPartyID : bluePartyID;

		// Odul (party liderine toplu) — bahisten AYRI havuz (patron: karismaz)
		if (isDraw)
			DistributePartyRewards(redPartyID, bluePartyID, true);
		else
			DistributePartyRewards(winnerPartyID,
				(winnerPartyID == redPartyID) ? bluePartyID : redPartyID, false);

		// Bahis havuzu cozumle (turnuva harici dahi disardan bahis acilabilir)
		{
			extern void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID);
			ResolveTournamentBets(info->aTournamentZoneID, winnerPartyID);
		}

		char pbuf[260] = {0};
		if (isDraw)
			_snprintf_s(pbuf, sizeof(pbuf), _TRUNCATE,
				"[PARTY VS BITTI / ENDED] Zone %u — Berabere/Draw (%u-%u)",
				info->aTournamentZoneID, redScoreP, blueScoreP);
		else
			_snprintf_s(pbuf, sizeof(pbuf), _TRUNCATE,
				"[PARTY VS BITTI / ENDED] Zone %u — Kazanan party/Winner: %s (%u-%u)",
				info->aTournamentZoneID,
				(winnerPartyID == redPartyID) ? "RED" : "BLUE",
				(redScoreP > blueScoreP) ? redScoreP : blueScoreP,
				(redScoreP > blueScoreP) ? blueScoreP : redScoreP);
		std::string pmsg = pbuf;
		Packet ppkt;
		ChatPacket::Construct(&ppkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &pmsg);
		g_pMain->Send_All(&ppkt);

		LOG(LogCategory::LOG_GENERAL,
			"[PARTY VS FINISH] zone=%u redParty=%u blueParty=%u skor=%u-%u winner=%u %s",
			info->aTournamentZoneID, redPartyID, bluePartyID, redScoreP, blueScoreP,
			winnerPartyID, isDraw ? "BERABERE" : "");
		return;  // clan kodu calismasin
	}

	CKnights *pRedClan  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
	CKnights *pBlueClan = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);

	uint16 redScore  = info->aTournamentScoreBoard[0];
	uint16 blueScore = info->aTournamentScoreBoard[1];

	const char* zoneName =
		(info->aTournamentZoneID == 77) ? "Ardream"   :
		(info->aTournamentZoneID == 78) ? "Ronark"    :
		(info->aTournamentZoneID == 96) ? "PartyVs-1" :
		(info->aTournamentZoneID == 97) ? "PartyVs-2" :
		(info->aTournamentZoneID == 98) ? "PartyVs-3" :
		(info->aTournamentZoneID == 99) ? "PartyVs-4" : "?";

	char buf[300] = { 0 };
	if (redScore > blueScore)
	{
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[TURNUVA BITTI / MATCH ENDED] %s — Kazanan/Winner: %s (%u-%u)",
			zoneName,
			pRedClan ? pRedClan->GetName().c_str() : "?",
			redScore, blueScore);

		if (pRedClan != nullptr && pBlueClan != nullptr)
			DistributeTournamentRewards(pRedClan->GetID(), pBlueClan->GetID(),
			                            info->aTournamentZoneID, redScore, blueScore);
	}
	else if (blueScore > redScore)
	{
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[TURNUVA BITTI / MATCH ENDED] %s — Kazanan/Winner: %s (%u-%u)",
			zoneName,
			pBlueClan ? pBlueClan->GetName().c_str() : "?",
			blueScore, redScore);

		if (pRedClan != nullptr && pBlueClan != nullptr)
			DistributeTournamentRewards(pBlueClan->GetID(), pRedClan->GetID(),
			                            info->aTournamentZoneID, blueScore, redScore);
	}
	else
	{
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[TURNUVA BITTI / MATCH ENDED] %s — Berabere/Draw (%u-%u)",
			zoneName, redScore, blueScore);
		// Berabere odul: iki klan'a katilim odulu (winnerID=0 -> ikisi de loser sayilir)
		// DistributeTournamentRewards (winnerClanID=0, loserClanID=Red, ...) -> Red uyeleri loser olarak alir
		// Sonra Blue icin ayri cagri (loserClanID=Blue)
		if (pRedClan != nullptr)
			DistributeTournamentRewards(0, pRedClan->GetID(),
			                            info->aTournamentZoneID, 0, 0);
		if (pBlueClan != nullptr)
			DistributeTournamentRewards(0, pBlueClan->GetID(),
			                            info->aTournamentZoneID, 0, 0);
	}

	std::string notice = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &notice);
	g_pMain->Send_All(&pkt);

	// S115 TUR 8 — Spectator Bet havuzunu cozumle
	uint16 winnerCID = 0;
	if (redScore > blueScore && pRedClan)  winnerCID = pRedClan->GetID();
	else if (blueScore > redScore && pBlueClan) winnerCID = pBlueClan->GetID();
	// Berabere durumunda winnerCID = 0 -> iade
	{
		extern void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID);
		ResolveTournamentBets(info->aTournamentZoneID, winnerCID);
	}

	// S115 TUR 9 — DB log: SP_CLAN_TOURNAMENT_FINISH cagri (MATRIX MSG:5897)
	if (info->dbTournamentID > 0)
	{
		bool ok = g_DBAgent.TournamentLogFinish(
			info->dbTournamentID,
			redScore, blueScore,
			info->aTournamentMonumentKilled,
			winnerCID);
		if (ok)
			printf("[TOURNAMENT_DB] Logged FINISH ID=%d Score=%u-%u Winner=%u\n",
				info->dbTournamentID, redScore, blueScore, winnerCID);
		else
			printf("[TOURNAMENT_DB] FINISH log failed (DB hata)\n");
	}
	else
	{
		printf("[TOURNAMENT_LOG] Zone=%d Red=%s Blue=%s Score=%u-%u Winner=%u (DB log YOK, dbID=0)\n",
			info->aTournamentZoneID,
			pRedClan  ? pRedClan->GetName().c_str()  : "?",
			pBlueClan ? pBlueClan->GetName().c_str() : "?",
			redScore, blueScore, winnerCID);
	}

	// S115 — Bracket Tournament hook: eger bu tournament bir bracket macı ise
	// BracketEngine'i bilgilendir, sonraki tur otomatik baslar
	if (info->bracketMatchID > 0)
	{
		extern void OnBracketMatchFinish(int32_t matchID, uint16 winnerClanID,
		                                 uint16 redScore, uint16 blueScore);
		OnBracketMatchFinish(info->bracketMatchID, winnerCID, redScore, blueScore);
	}

	// S115 — League (lig) hook: bu tournament bir lig macı ise puan guncelle
	if (info->leagueMatchID > 0)
	{
		extern void OnLeagueMatchFinish(int32_t matchID, uint16 winnerClanID,
		                                uint16 redScore, uint16 blueScore);
		OnLeagueMatchFinish(info->leagueMatchID, winnerCID, redScore, blueScore);
	}

	// S115 TUR 7 — Klan Premium 24sa kazanana
	// Kazanan klan'in sPremiumTime = UNIXTIME + 86400 (24sa) set edilir
	// Tum klan uyeleri exp/drop bonus alir (mevcut premium sistem)
	{
		CKnights* pWinnerClan = nullptr;
		if (redScore > blueScore)       pWinnerClan = pRedClan;
		else if (blueScore > redScore)  pWinnerClan = pBlueClan;
		// Berabere durumunda premium YOK

		if (pWinnerClan != nullptr)
		{
			pWinnerClan->sPremiumTime = (uint32)UNIXTIME + 86400; // 24 saat

			// Klan uyelerine premium paketi yolla (online olanlar)
			for (uint16 i = 0; i < MAX_USER; i++)
			{
				CUser* pUser = g_pMain->GetUserPtr(i);
				if (pUser == nullptr || !pUser->isInGame()) continue;
				if (pUser->GetClanID() != pWinnerClan->GetID()) continue;
				pUser->SendClanPremium(pWinnerClan, false);
			}

			// Duyuru (sade chat)
			char premBuf[200] = { 0 };
			_snprintf_s(premBuf, sizeof(premBuf), _TRUNCATE,
				"[CLAN WAR %s] %s klanin 24 saat klan premium aktif!",
				zoneName, pWinnerClan->GetName().c_str());
			std::string premMsg = premBuf;
			Packet premPkt;
			ChatPacket::Construct(&premPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &premMsg);
			g_pMain->Send_All(&premPkt);
		}
	}

	// S115 TUR 5 — MVP hesapla + ekstra odul
	if (!info->killCountByUser.empty())
	{
		uint16 mvpUserID = 0;
		uint16 mvpKills  = 0;
		for (auto& it : info->killCountByUser)
		{
			if (it.second > mvpKills)
			{
				mvpKills = it.second;
				mvpUserID = it.first;
			}
		}

		if (mvpUserID > 0 && mvpKills > 0)
		{
			CUser* pMVP = g_pMain->GetUserPtr(mvpUserID);
			if (pMVP != nullptr && pMVP->isInGame())
			{
				// MVP odul: +2M gold + 500 NP
				pMVP->GoldGain(2000000);
				pMVP->SendLoyaltyChange("tournament_mvp", 500, false, false, false);

				// Tum sunucuya MVP duyurusu
				char mvpBuf[200] = { 0 };
				_snprintf_s(mvpBuf, sizeof(mvpBuf), _TRUNCATE,
					"[CLAN WAR %s] MVP: %s (%u kill) — +2M Noah +500 NP",
					zoneName, pMVP->GetName().c_str(), mvpKills);
				std::string mvpMsg = mvpBuf;
				Packet mvpPkt;
				ChatPacket::Construct(&mvpPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &mvpMsg);
				g_pMain->Send_All(&mvpPkt);
			}
		}
	}

	// First Blood duyuru (sonuc ile birlikte hatirlat)
	if (info->firstBloodUserID > 0 && !info->firstBloodUserName.empty())
	{
		char fbBuf[160] = { 0 };
		_snprintf_s(fbBuf, sizeof(fbBuf), _TRUNCATE,
			"[CLAN WAR %s] First Blood: %s",
			zoneName, info->firstBloodUserName.c_str());
		std::string fbMsg = fbBuf;
		Packet fbPkt;
		ChatPacket::Construct(&fbPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &fbMsg);
		g_pMain->Send_All(&fbPkt);
	}
}

// S115 Plan A — Helper: Tek zone tournament timer adımı
// Mantık:
//   Faz 1 (isStarted=true, Timer>0): geri sayım, 5sn'de bir paket yayınla
//   Faz 2 (Timer==0, isStarted=true): bitir + ödül + isStarted=false + OutTimer=now+60
//   Faz 3 (isStarted=false, OutTimer<=UNIXTIME): kick out + delete (RAM temizle)
static void TickOneTournamentZone(CGameServerDlg* pMain, uint8 zoneID)
{
	_TOURNAMENT_DATA* info = pMain->m_ClanVsDataList.GetData(zoneID);
	if (info == nullptr) return;

	// S115 ORPHAN GUARD (event BETTING data): EventScheduler RAM'den kaybolursa (restart/iptal)
	// betting verisi (started=false+finished=false+bettingPhase) hicbir faza girmez -> zone SONSUZ KILIT.
	// aTournamentOutTimer betting deadline olarak kullanilir (EnterBettingPhase set eder).
	// Deadline gectiyse + hala betting'de (EventScheduler RUNNING'e gecirmemis) -> guvenlik temizligi.
	if (info->aBettingPhase && !info->aTournamentisStarted
		&& info->aTournamentOutTimer != 0 && info->aTournamentOutTimer <= UNIXTIME)
	{
		extern void CancelTournamentBets(uint8 zoneID);
		CancelTournamentBets(zoneID);  // bekleyen bahisleri iade
		pMain->m_ClanVsDataList.DeleteData(zoneID);
		printf("[EVENT] ORPHAN betting data temizlendi (zone=%u, EventScheduler kayip)\n", zoneID);
		return;
	}

	// Faz 2: Aktif tournament suresi bitti
	if (info->aTournamentisStarted && info->aTournamentTimer == 0)
	{
		HandleTournamentEnd(info);
		info->aTournamentOutTimer  = UNIXTIME + 60;  // 60sn geri donus suresi
		info->aTournamentisStarted = false;
		info->aTournamentisFinished = true;
		return; // sonraki saniyede cleanup blokuna gelir
	}

	// Faz 3: Bitmis tournament, kick + cleanup zamani gelmis
	if (!info->aTournamentisStarted && info->aTournamentisFinished
		&& info->aTournamentOutTimer != 0 && info->aTournamentOutTimer <= UNIXTIME)
	{
		pMain->KickOutZoneUsers(zoneID, ZONE_MORADON, (uint8)Nation::ALL);
		pMain->m_ClanVsDataList.DeleteData(zoneID);
		return;
	}

	// Faz 1: Aktif, geri sayim + 5sn paket
	if (info->aTournamentisStarted)
	{
		if (info->aTournamentTimer > 0)
			info->aTournamentTimer--;

		// Her 5sn'de bir scoreboard + timer yayinla (client UI senkron)
		if ((info->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(info);

		// S115 TUR 3 — Geri sayim duyurusu (60/30/10/5/1)
		// PG kontrolu: sade chat paketi (WIZ_CHAT), yeni opcode YOK ✅
		const uint32 t = info->aTournamentTimer;
		if (t == 60 || t == 30 || t == 10 || t == 5 || t == 1)
		{
			const char* zoneName =
				(info->aTournamentZoneID == 77) ? "Ardream"   :
				(info->aTournamentZoneID == 78) ? "Ronark"    :
				(info->aTournamentZoneID == 96) ? "PartyVs-1" :
				(info->aTournamentZoneID == 97) ? "PartyVs-2" :
				(info->aTournamentZoneID == 98) ? "PartyVs-3" :
				(info->aTournamentZoneID == 99) ? "PartyVs-4" : "?";

			char buf[160] = { 0 };
			if (t == 60)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[CLAN WAR %s] 1 dakika kaldi! Son saldiri!", zoneName);
			else if (t == 30)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[CLAN WAR %s] 30 saniye!", zoneName);
			else if (t == 10)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[CLAN WAR %s] 10... savas bitiyor!", zoneName);
			else if (t == 5)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[CLAN WAR %s] 5...", zoneName);
			else // t == 1
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[CLAN WAR %s] 1!", zoneName);

			std::string notice = buf;
			Packet pkt;
			ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &notice);
			// Sadece tournament zone'una yayinla (zone disi spam yapma)
			pMain->Send_Zone(&pkt, info->aTournamentZoneID);
		}
	}
}

#pragma region CGameServerDlg::ClanTournamentTimer()
void CGameServerDlg::ClanTournamentTimer()
{
	// S115 Plan A — Refactor: 6 zone tek helper ile, faz bazli temiz akis
	TickOneTournamentZone(this, ZONE_CLAN_WAR_ARDREAM);  // 77
	TickOneTournamentZone(this, ZONE_CLAN_WAR_RONARK);   // 78
	TickOneTournamentZone(this, ZONE_PARTY_VS_1);        // 96
	TickOneTournamentZone(this, ZONE_PARTY_VS_2);        // 97
	TickOneTournamentZone(this, ZONE_PARTY_VS_3);        // 98
	TickOneTournamentZone(this, ZONE_PARTY_VS_4);        // 99
}
#pragma endregion

#pragma region CGameServerDlg::UpdateClanTournamentScoreBoard(CUser* pUser)
void CGameServerDlg::UpdateClanTournamentScoreBoard(CUser* pUser)
{
	if (pUser == nullptr)
		return;

	bool TournamentTrueZone = (pUser->GetZoneID() == 77
		|| pUser->GetZoneID() == 78
		|| pUser->GetZoneID() == 96
		|| pUser->GetZoneID() == 97
		|| pUser->GetZoneID() == 98
		|| pUser->GetZoneID() == 99);

	if (!TournamentTrueZone)
		return;

	// S115 Plan A — Validation gevsetildi (eski kod GM oldurdugunde Dis ediyordu)
	// Tournament katilimci klan disi (GM dahil) ise sadece SKIP, disconnect YOK
	_TOURNAMENT_DATA* TournamentInfo = g_pMain->m_ClanVsDataList.GetData(pUser->GetZoneID());
	if (TournamentInfo == nullptr || !TournamentInfo->aTournamentisStarted)
		return;
	if (TournamentInfo->aTournamentTimer == 0)
		return;

	// S115 — PARTY VS: party tipinde clan yerine PARTY ID ile takim belirle
	bool isRedSide = false, isParticipant = false;
	if (TournamentInfo->participantType == 1)
	{
		uint16 killerParty = (uint16)pUser->GetPartyID();
		uint16 redParty    = TournamentInfo->aTournamentPartyNum[0];
		uint16 blueParty   = TournamentInfo->aTournamentPartyNum[1];
		if (killerParty == redParty)       { isRedSide = true;  isParticipant = true; }
		else if (killerParty == blueParty) { isRedSide = false; isParticipant = true; }
		else {
			// DC sonrasi party'den dusmus olabilir (party ID kaybolur) — ROSTER (isim) ile teyit
			std::string nm = pUser->GetName();
			if (TournamentInfo->rosterRed.find(nm) != TournamentInfo->rosterRed.end())
				{ isRedSide = true;  isParticipant = true; }
			else if (TournamentInfo->rosterBlue.find(nm) != TournamentInfo->rosterBlue.end())
				{ isRedSide = false; isParticipant = true; }
		}
	}
	else
	{
		uint16 killerClan = pUser->GetClanID();
		uint16 redClan    = TournamentInfo->aTournamentClanNum[0];
		uint16 blueClan   = TournamentInfo->aTournamentClanNum[1];
		if (killerClan == redClan)       { isRedSide = true;  isParticipant = true; }
		else if (killerClan == blueClan) { isRedSide = false; isParticipant = true; }
	}

	// Katilimci disi (klansiz/partysiz, GM, baska takim) ise score sayma, gec
	if (!isParticipant)
		return;

	// Kill cooldown: spam onlemi (5 saniye iki kill arasinda)
	if (UNIXTIME - pUser->m_nLastTournamentKillTime < 5)
		return;
	pUser->m_nLastTournamentKillTime = (uint32)UNIXTIME;

	if (isRedSide)
		TournamentInfo->aTournamentScoreBoard[0]++;
	else
		TournamentInfo->aTournamentScoreBoard[1]++;

	// S115 TUR 5 — MVP tracking (her oyuncunun kill sayisi)
	uint16 killerUserID = pUser->GetID();
	TournamentInfo->killCountByUser[killerUserID]++;

	// S115 TUR 5 — First Blood (ilk kan)
	if (TournamentInfo->firstBloodUserID == 0)
	{
		TournamentInfo->firstBloodUserID = killerUserID;
		TournamentInfo->firstBloodUserName = pUser->GetName();

		// Tournament zone'una duyuru
		const char* zoneName =
			(TournamentInfo->aTournamentZoneID == 77) ? "Ardream"   :
			(TournamentInfo->aTournamentZoneID == 78) ? "Ronark"    :
			(TournamentInfo->aTournamentZoneID == 96) ? "PartyVs-1" :
			(TournamentInfo->aTournamentZoneID == 97) ? "PartyVs-2" :
			(TournamentInfo->aTournamentZoneID == 98) ? "PartyVs-3" :
			(TournamentInfo->aTournamentZoneID == 99) ? "PartyVs-4" : "?";
		char fbBuf[160] = { 0 };
		_snprintf_s(fbBuf, sizeof(fbBuf), _TRUNCATE,
			"[CLAN WAR %s] FIRST BLOOD: %s!", zoneName, pUser->GetName().c_str());
		std::string fbMsg = fbBuf;
		Packet fbPkt;
		ChatPacket::Construct(&fbPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &fbMsg);
		Send_Zone(&fbPkt, TournamentInfo->aTournamentZoneID);

		// First Blood odulu (+100k gold)
		pUser->GoldGain(100000);
	}

	// Score + Timer paketi yayinla (helper kullan)
	SendTournamentScorePacket(TournamentInfo);

	// S115 TUR 4 — Live score broadcast (tum sunucuya)
	// Izleyici oyuncular Moradon'dan/diger zone'lardan tournament score'unu gorur
	// PG kontrolu: WIZ_CHAT (sade chat), yeni opcode YOK ✅
	{
		const char* zoneName =
			(TournamentInfo->aTournamentZoneID == 77) ? "Ardream"   :
			(TournamentInfo->aTournamentZoneID == 78) ? "Ronark"    :
			(TournamentInfo->aTournamentZoneID == 96) ? "PartyVs-1" :
			(TournamentInfo->aTournamentZoneID == 97) ? "PartyVs-2" :
			(TournamentInfo->aTournamentZoneID == 98) ? "PartyVs-3" :
			(TournamentInfo->aTournamentZoneID == 99) ? "PartyVs-4" : "?";

		// Takim isimleri: party tipinde RED/BLUE, clan tipinde klan adi
		std::string redLabel = "Red", blueLabel = "Blue";
		const char* eventLabel = "CLAN WAR";
		if (TournamentInfo->participantType == 1) {
			redLabel = "RED Party"; blueLabel = "BLUE Party"; eventLabel = "PARTY VS";
		} else {
			CKnights* pRedClan  = GetClanPtr(TournamentInfo->aTournamentClanNum[0]);
			CKnights* pBlueClan = GetClanPtr(TournamentInfo->aTournamentClanNum[1]);
			if (pRedClan)  redLabel  = pRedClan->GetName();
			if (pBlueClan) blueLabel = pBlueClan->GetName();
		}

		char buf[200] = { 0 };
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[%s %s] %s %u - %u %s",
			eventLabel, zoneName,
			redLabel.c_str(),
			TournamentInfo->aTournamentScoreBoard[0],
			TournamentInfo->aTournamentScoreBoard[1],
			blueLabel.c_str());

		std::string notice = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &notice);
		Send_All(&pkt);
	}
}
#pragma endregion

#pragma region void TournamentMonumentKillProcess(CUser* puser)
void CNpc::TournamentMonumentKillProcess(CUser* puser)
{
	if (puser == nullptr)
		return;

	bool TournamentTrueZone = (puser->GetZoneID() == 77
		|| puser->GetZoneID() == 78
		|| puser->GetZoneID() == 96
		|| puser->GetZoneID() == 97
		|| puser->GetZoneID() == 98
		|| puser->GetZoneID() == 99);

	if (!TournamentTrueZone)
		return;

	// S115 Plan A — Validation gevsetildi (eski kod GM monument kirinca Dis ediyordu)
	_TOURNAMENT_DATA* TournamentClanInfo = g_pMain->m_ClanVsDataList.GetData(puser->GetZoneID());
	if (TournamentClanInfo == nullptr || !TournamentClanInfo->aTournamentisStarted) return;
	if (TournamentClanInfo->aTournamentTimer == 0) return;

	uint16 killerClan = puser->GetClanID();
	uint16 redClan    = TournamentClanInfo->aTournamentClanNum[0];
	uint16 blueClan   = TournamentClanInfo->aTournamentClanNum[1];

	if (killerClan != redClan && killerClan != blueClan)
		return; // Tournament katilimci klan disi - bu fonksiyon zaten validation gecmis olmali

	uint16 redScore  = TournamentClanInfo->aTournamentScoreBoard[0];
	uint16 blueScore = TournamentClanInfo->aTournamentScoreBoard[1];

	// Anit kiran klan, kendi score'u dusukse half-diff/2 bonus alır (max 50)
	uint16 myScore     = (killerClan == redClan) ? redScore  : blueScore;
	uint16 enemyScore  = (killerClan == redClan) ? blueScore : redScore;

	if (myScore >= enemyScore)
		return; // Onde isen catch-up bonus YOK

	uint16 diff      = enemyScore - myScore;
	uint16 addScore  = diff / 2;
	if (addScore > 50) addScore = 50;
	if (addScore == 0) return;

	if (killerClan == redClan)
		TournamentClanInfo->aTournamentScoreBoard[0] += addScore;
	else
		TournamentClanInfo->aTournamentScoreBoard[1] += addScore;

	TournamentClanInfo->aTournamentMonumentKilled++;

	// Score + Timer paketi yayinla
	SendTournamentScorePacket(TournamentClanInfo);
}
#pragma endregion

#pragma region void CUser::TournamentSendTimer()
void CUser::TournamentSendTimer()
{
	bool TournamentTrueZone = (GetZoneID() == 77
		|| GetZoneID() == 78
		|| GetZoneID() == 96
		|| GetZoneID() == 97
		|| GetZoneID() == 98
		|| GetZoneID() == 99);

	if (!TournamentTrueZone)
		return;

	_TOURNAMENT_DATA* TournamentClanInfo = g_pMain->m_ClanVsDataList.GetData(GetZoneID());
	if (TournamentClanInfo != nullptr)
	{
		if (TournamentClanInfo->aTournamentisStarted == true)
		{
			CKnights *pRedClan = g_pMain->GetClanPtr(TournamentClanInfo->aTournamentClanNum[0]);/*Red Clan*/
			CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentClanInfo->aTournamentClanNum[1]);/*Blue Clan*/
			if (pRedClan != nullptr && pBlueClan != nullptr)
			{
				Packet TournamentClanPacket;
				TournamentClanPacket.Initialize(WIZ_BIFROST);
				TournamentClanPacket << uint8(5) << uint16(TournamentClanInfo->aTournamentTimer);
				Send(&TournamentClanPacket);

				TournamentClanPacket.clear();
				TournamentClanPacket.Initialize(WIZ_BATTLE_EVENT);
				TournamentClanPacket << uint8(0x12)
					<< uint8(2)/*2:Board1,3:Board2*/
					<< TournamentClanInfo->aTournamentScoreBoard[0]/*Red Clan Score Board */
					<< TournamentClanInfo->aTournamentScoreBoard[1]/*Blue Clan Score Board */
					<< TournamentClanInfo->aTournamentTimer/*Timer*/
					<< TournamentClanInfo->aTournamentMonumentKilled;/*Monument Killed Advantage*/
				Send(&TournamentClanPacket);
			}
		}
	}
}
#pragma endregion

// =====================================================================
// S115 — GM MANUEL ODUL KONSOL KOMUTU (/tournamentreward)
// patron: "kazanana GM belirledigi odulu manuel girebilecegi sistem"
// Kullanim:
//   /tournamentreward noah klan <KlanAdi> <miktar>
//   /tournamentreward noah lider <KlanAdi> <miktar>
//   /tournamentreward noah oyuncu <Karakter> <miktar>
//   /tournamentreward np   klan <KlanAdi> <miktar>
//   /tournamentreward item oyuncu <Karakter> <itemID> <adet>
// Tip: noah(default)/np/item | Hedef: klan/lider/oyuncu
// Aninda teslim — online hedeflere yansir.
// =====================================================================

// Local helper — klan adindan ID bul (poller'daki static'e erisilemiyor)
static uint16 RewardFindClanID(const std::string& name)
{
	uint16 found = 0;
	g_pMain->m_KnightsArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_KnightsArray) {
		CKnights* pClan = itr->second;
		if (pClan != nullptr && pClan->m_strName == name) {
			found = pClan->GetID();
			break;
		}
	}
	g_pMain->m_KnightsArray.m_lock.unlock();
	return found;
}

extern int GMGiveTournamentReward(uint8 rewardType, uint8 targetType,
                                  uint16 targetClanID, const std::string& targetName,
                                  uint32 amount, uint32 itemID, uint16 itemCount,
                                  const std::string& gmName);

COMMAND_HANDLER(CGameServerDlg::HandleTournamentRewardCommand)
{
	if (vargs.size() < 4) {
		printf("Usage: /tournamentreward <noah|np|item> <klan|lider|oyuncu> <Ad> <miktar> [adet(item)]\n");
		printf("  Ornek: /tournamentreward noah klan RedClan 5000000\n");
		printf("  Ornek: /tournamentreward item oyuncu Ahmet 379010 1\n");
		return true;
	}

	std::string sType   = vargs.front(); vargs.pop_front();
	std::string sTarget = vargs.front(); vargs.pop_front();
	std::string sName   = vargs.front(); vargs.pop_front();

	uint8 rewardType = 0;  // 0=noah 1=item 2=np
	if (sType == "item") rewardType = 1;
	else if (sType == "np") rewardType = 2;

	uint8 targetType = 0;  // 0=klan 1=lider 2=oyuncu
	if (sTarget == "lider") targetType = 1;
	else if (sTarget == "oyuncu") targetType = 2;

	uint16 clanID = 0;
	std::string charName;
	if (targetType == 2) {
		charName = sName;
	} else {
		clanID = RewardFindClanID(sName);
		if (clanID == 0) { printf("[GM REWARD] Klan bulunamadi: %s\n", sName.c_str()); return true; }
	}

	uint32 amount = 0, itemID = 0;
	uint16 itemCount = 1;
	if (rewardType == 1) {
		// item: <Ad> itemID adet
		itemID = (uint32)SafeAtoi(vargs.front(), 0, 0x7FFFFFFF); vargs.pop_front();
		if (!vargs.empty()) itemCount = (uint16)SafeAtoi(vargs.front(), 1, 9999);
		if (itemID == 0) { printf("[GM REWARD] Gecersiz itemID\n"); return true; }
	} else {
		// noah/np: miktar
		amount = (uint32)SafeAtoi(vargs.front(), 1, COIN_MAX);
		if (amount == 0) { printf("[GM REWARD] Gecersiz miktar\n"); return true; }
	}

	int given = GMGiveTournamentReward(rewardType, targetType, clanID, charName,
	                                   amount, itemID, itemCount, "console");
	printf("[GM REWARD] %d kisiye verildi (tip=%s hedef=%s)\n",
		given, sType.c_str(), sTarget.c_str());
	return true;
}

// =====================================================================
// S115 — PARTY VS PARTY duello konsol komutu (/partyvs)
// patron: "party vs party yok, ozel karmasik party vsler de yapilabilmeli"
// Kullanim: /partyvs <RedLiderKarakter> <BlueLiderKarakter> <Zone> <Dakika>
//   GM iki party liderinin KARAKTER adini verir -> party ID'leri bulunur.
//   Lider olmasa bile party'deyse o party kullanilir (uyari ile).
// =====================================================================
extern bool StartPartyVsMatch(uint8 zoneID, uint16 redPartyID, uint16 bluePartyID,
                              uint16 durationMin, const std::string& startedBy);

COMMAND_HANDLER(CGameServerDlg::HandlePartyVsCommand)
{
	if (vargs.size() < 4) {
		printf("Usage: /partyvs <RedLider> <BlueLider> <Zone(96-99/77/78)> <Dakika(1-60)>\n");
		printf("  Ornek: /partyvs Ahmet Mehmet 96 10\n");
		return true;
	}

	std::string redLeader  = vargs.front(); vargs.pop_front();
	std::string blueLeader = vargs.front(); vargs.pop_front();
	uint8  zoneID   = (uint8)SafeAtoi(vargs.front(), 1, 255); vargs.pop_front();
	uint16 duration = (uint16)SafeAtoi(vargs.front(), 1, 60);

	CUser* pRL = GetUserPtr(redLeader, NameType::TYPE_CHARACTER);
	CUser* pBL = GetUserPtr(blueLeader, NameType::TYPE_CHARACTER);
	if (pRL == nullptr || !pRL->isInGame()) { printf("[PARTY VS] Red lider offline/yok: %s\n", redLeader.c_str()); return true; }
	if (pBL == nullptr || !pBL->isInGame()) { printf("[PARTY VS] Blue lider offline/yok: %s\n", blueLeader.c_str()); return true; }
	if (!pRL->isInParty()) { printf("[PARTY VS] %s party'de degil\n", redLeader.c_str()); return true; }
	if (!pBL->isInParty()) { printf("[PARTY VS] %s party'de degil\n", blueLeader.c_str()); return true; }

	uint16 redPartyID  = (uint16)pRL->GetPartyID();
	uint16 bluePartyID = (uint16)pBL->GetPartyID();
	if (redPartyID == bluePartyID) { printf("[PARTY VS] Ikisi de ayni party'de\n"); return true; }

	if (StartPartyVsMatch(zoneID, redPartyID, bluePartyID, duration, "console"))
		printf("[PARTY VS] Basladi: %s(party %u) vs %s(party %u) @ Zone %u %u dk\n",
			redLeader.c_str(), redPartyID, blueLeader.c_str(), bluePartyID, zoneID, duration);
	else
		printf("[PARTY VS] Baslatilamadi (zone dolu veya party yok)\n");
	return true;
}