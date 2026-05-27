#include "stdafx.h"

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

// S115 TUR 6 — Champion's Treasure Chest: kazanan klan uyelerine 1 adet random item
// Sabit liste (acilis sonrasi DB _MK_TOURNAMENT_REWARDS'tan okunur)
// Item ID'leri standart KO 1098 source — PATRON gercek itemIDsini sonra ayarlar
struct _TOURNAMENT_CHEST_ITEM {
	uint32 itemID;
	uint16 chance;   // 1-100 yuzde
	const char* name;
};

// PATRON: bu liste config/DB'ye cevirilebilir
static const _TOURNAMENT_CHEST_ITEM s_chestItems[] = {
	{ 379014000, 80, "Premium Upgrade Scroll" },  // ornek itemID — DB'de dogrula
	{ 379015000, 15, "Premium Bag" },
	{ 900000000, 5,  "Bonus Noah Bag" },          // ITEM_GOLD - 10M noah
};
static const int s_chestItemCount = sizeof(s_chestItems) / sizeof(s_chestItems[0]);

// Random item secimi (weighted)
static uint32 PickRandomChestItem()
{
	uint16 roll = rand() % 100;  // 0-99
	uint16 acc = 0;
	for (int i = 0; i < s_chestItemCount; i++)
	{
		acc += s_chestItems[i].chance;
		if (roll < acc) return s_chestItems[i].itemID;
	}
	return s_chestItems[0].itemID;  // fallback
}

// S115 Plan A B9 — Helper: Kazanan klan uyelerine odul dagit (gold + np + chest)
// winnerClanID = 0 ise berabere -> kucuk odul iki klana
static void DistributeTournamentRewards(uint16 winnerClanID, uint16 loserClanID,
                                        uint8 zoneID, uint16 winnerScore, uint16 loserScore)
{
	// Acilis Plan A — sabit odul (acilis sonrasi DB tablo `_MK_TOURNAMENT_REWARDS` ile dinamik olur)
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

		// S115 TUR 6 — Sadece kazanana Champion's Treasure Chest (1 random item)
		if (isWinner && winnerClanID > 0)
		{
			uint32 chestItemID = PickRandomChestItem();
			if (chestItemID > 0)
			{
				pUser->GiveItem("tournament_chest", chestItemID, 1, true, 0);
			}
		}

		std::string msg = isWinner
			? "[CLAN WAR] Tebrikler! Klanin kazandi. Champion's Treasure Chest aldin!"
			: "[CLAN WAR] Iyi savas! Katilim odulun aldin.";
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		pUser->Send(&pkt);
	}
}

// S115 Plan A — Helper: Tournament bittiginde duyuru + odul dagit
// Tek yerden tum sonuc mantığı (6 zone'a kopyali kod kaldirildi)
static void HandleTournamentEnd(_TOURNAMENT_DATA* info)
{
	if (info == nullptr) return;

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

	char buf[256] = { 0 };
	if (redScore > blueScore)
	{
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[CLAN WAR] %s Zone biti! Kazanan: %s (%u-%u)",
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
			"[CLAN WAR] %s Zone biti! Kazanan: %s (%u-%u)",
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
			"[CLAN WAR] %s Zone biti! Berabere (%u-%u)",
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

	uint16 killerClan = pUser->GetClanID();
	uint16 redClan    = TournamentInfo->aTournamentClanNum[0];
	uint16 blueClan   = TournamentInfo->aTournamentClanNum[1];

	// Katilimci klan disi (klansiz, GM, baska klan) ise score sayma, gec
	if (killerClan != redClan && killerClan != blueClan)
		return;

	// Kill cooldown: spam onlemi (5 saniye iki kill arasinda)
	if (UNIXTIME - pUser->m_nLastTournamentKillTime < 5)
		return;
	pUser->m_nLastTournamentKillTime = (uint32)UNIXTIME;

	if (killerClan == redClan)
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
		CKnights* pRedClan  = GetClanPtr(redClan);
		CKnights* pBlueClan = GetClanPtr(blueClan);
		const char* zoneName =
			(TournamentInfo->aTournamentZoneID == 77) ? "Ardream"   :
			(TournamentInfo->aTournamentZoneID == 78) ? "Ronark"    :
			(TournamentInfo->aTournamentZoneID == 96) ? "PartyVs-1" :
			(TournamentInfo->aTournamentZoneID == 97) ? "PartyVs-2" :
			(TournamentInfo->aTournamentZoneID == 98) ? "PartyVs-3" :
			(TournamentInfo->aTournamentZoneID == 99) ? "PartyVs-4" : "?";

		char buf[200] = { 0 };
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[CLAN WAR %s] %s %u - %u %s",
			zoneName,
			pRedClan  ? pRedClan->GetName().c_str()  : "Red",
			TournamentInfo->aTournamentScoreBoard[0],
			TournamentInfo->aTournamentScoreBoard[1],
			pBlueClan ? pBlueClan->GetName().c_str() : "Blue");

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