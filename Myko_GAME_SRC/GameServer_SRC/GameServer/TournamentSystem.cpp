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

// S115 Plan A B9 — Helper: Kazanan klan uyelerine odul dagit (gold + np)
// itemID = 0 ise item dagitilmaz. winnerClanID = 0 ise berabere -> kucuk odul iki klana
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

		std::string msg = isWinner
			? "[CLAN WAR] Tebrikler! Klanin kazandi. Odul aldin!"
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
}

#pragma region CGameServerDlg::ClanTournamentTimer()
void CGameServerDlg::ClanTournamentTimer()
{
	_TOURNAMENT_DATA* TournamentAndream = g_pMain->m_ClanVsDataList.GetData(77);
	if (TournamentAndream != nullptr)
	{
		if (TournamentAndream->aTournamentisStarted == true)
		{
			if (TournamentAndream->aTournamentisFinished == false)
			{
				if (TournamentAndream->aTournamentTimer == 0)
				{
					// S115 Plan A — sade chat + odul dagit (helper)
					HandleTournamentEnd(TournamentAndream);
					TournamentAndream->aTournamentOutTimer = UNIXTIME + 60;  // 60sn geri donus
					TournamentAndream->aTournamentisStarted = false;
				}
			}

			if (TournamentAndream->aTournamentOutTimer != 0 && TournamentAndream->aTournamentOutTimer <= UNIXTIME)
				TournamentAndream->aTournamentisFinished = true;

			if (TournamentAndream->aTournamentOutTimer <= UNIXTIME
				&& TournamentAndream->aTournamentisFinished == true)
			{
				
				CKnights *pRedClan = g_pMain->GetClanPtr(TournamentAndream->aTournamentClanNum[0]);/*Red Clan*/
				CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentAndream->aTournamentClanNum[1]);/*Blue Clan*/
				
				/*DateTime time;
				WriteTournamentLogs(string_format("[Tournament Finish Time - %d:%d:%d] Red Clan Name = %s : %s Blue Clan Name (Red Board %d : %d Blue Board)\n",
					time.GetHour(), time.GetMinute(), time.GetSecond(), pRedClan == nullptr ? "null" : pRedClan->GetName().c_str(), pBlueClan == nullptr ? "null" : pBlueClan->GetName().c_str(), TournamentAndream->aTournamentScoreBoard[0], TournamentAndream->aTournamentScoreBoard[1]));*/

				KickOutZoneUsers(ZONE_CLAN_WAR_ARDREAM, ZONE_MORADON);
				m_ClanVsDataList.DeleteData(77);
			}
		}

		if (TournamentAndream->aTournamentTimer > 0)
			TournamentAndream->aTournamentTimer--;

		// S115 Plan A — Her 5sn'de bir scoreboard+timer yayinla (client UI senkron)
		if (TournamentAndream->aTournamentisStarted && (TournamentAndream->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(TournamentAndream);
	}

	_TOURNAMENT_DATA* TournamentRonarkLand = g_pMain->m_ClanVsDataList.GetData(78);
	if (TournamentRonarkLand != nullptr)
	{
		if (TournamentRonarkLand->aTournamentisStarted == true)
		{
			if (TournamentRonarkLand->aTournamentisFinished == false)
			{
				if (TournamentRonarkLand->aTournamentTimer == 0)
				{
					HandleTournamentEnd(TournamentRonarkLand);
					TournamentRonarkLand->aTournamentOutTimer = UNIXTIME + 60;
					TournamentRonarkLand->aTournamentisStarted = false;
				}
			}

			if (TournamentRonarkLand->aTournamentOutTimer != 0 && TournamentRonarkLand->aTournamentOutTimer <= UNIXTIME)
				TournamentRonarkLand->aTournamentisFinished = true;

			if (TournamentRonarkLand->aTournamentOutTimer <= UNIXTIME
				&& TournamentRonarkLand->aTournamentisFinished == true)
			{
				
				CKnights *pRedClan = g_pMain->GetClanPtr(TournamentRonarkLand->aTournamentClanNum[0]);/*Red Clan*/
				CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentRonarkLand->aTournamentClanNum[1]);/*Blue Clan*/
				
				/*DateTime time;
				WriteTournamentLogs(string_format("[Tournament Finish Time - %d:%d:%d] Red Clan Name = %s : %s Blue Clan Name (Red Board %d : %d Blue Board)\n",
					time.GetHour(), time.GetMinute(), time.GetSecond(), pRedClan == nullptr ? "null" : pRedClan->GetName().c_str(), pBlueClan == nullptr ? "null" : pBlueClan->GetName().c_str(), TournamentRonarkLand->aTournamentScoreBoard[0], TournamentRonarkLand->aTournamentScoreBoard[1]));*/

				KickOutZoneUsers(ZONE_CLAN_WAR_RONARK, ZONE_MORADON);
				m_ClanVsDataList.DeleteData(78);
			}
		}

		if (TournamentRonarkLand->aTournamentTimer > 0)
			TournamentRonarkLand->aTournamentTimer--;

		if (TournamentRonarkLand->aTournamentisStarted && (TournamentRonarkLand->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(TournamentRonarkLand);
	}

	_TOURNAMENT_DATA* TournamentParty1 = g_pMain->m_ClanVsDataList.GetData(96);
	if (TournamentParty1 != nullptr)
	{
		if (TournamentParty1->aTournamentisStarted == true)
		{
			if (TournamentParty1->aTournamentisFinished == false)
			{
				if (TournamentParty1->aTournamentTimer == 0)
				{
					HandleTournamentEnd(TournamentParty1);
					TournamentParty1->aTournamentOutTimer = UNIXTIME + 60;
					TournamentParty1->aTournamentisStarted = false;
				}
			}

			if (TournamentParty1->aTournamentOutTimer != 0 && TournamentParty1->aTournamentOutTimer <= UNIXTIME)
				TournamentParty1->aTournamentisFinished = true;

			if (TournamentParty1->aTournamentOutTimer <= UNIXTIME
				&& TournamentParty1->aTournamentisFinished == true)
			{
				CKnights *pRedClan = g_pMain->GetClanPtr(TournamentParty1->aTournamentClanNum[0]);/*Red Clan*/
				CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentParty1->aTournamentClanNum[1]);/*Blue Clan*/
				
				/*DateTime time;
				WriteTournamentLogs(string_format("[Tournament Finish Time - %d:%d:%d] Red Clan Name = %s : %s Blue Clan Name (Red Board %d : %d Blue Board)\n",
					time.GetHour(), time.GetMinute(), time.GetSecond(), pRedClan == nullptr ? "null" : pRedClan->GetName().c_str(), pBlueClan == nullptr ? "null" : pBlueClan->GetName().c_str(), TournamentParty1->aTournamentScoreBoard[0], TournamentParty1->aTournamentScoreBoard[1]));*/

				KickOutZoneUsers(ZONE_PARTY_VS_1, ZONE_MORADON);
				m_ClanVsDataList.DeleteData(96);
			}
		}

		if (TournamentParty1->aTournamentTimer > 0)
			TournamentParty1->aTournamentTimer--;

		if (TournamentParty1->aTournamentisStarted && (TournamentParty1->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(TournamentParty1);
	}

	_TOURNAMENT_DATA* TournamentParty2 = g_pMain->m_ClanVsDataList.GetData(97);
	if (TournamentParty2 != nullptr)
	{
		if (TournamentParty2->aTournamentisStarted == true)
		{
			if (TournamentParty2->aTournamentisFinished == false)
			{
				if (TournamentParty2->aTournamentTimer == 0)
				{
					HandleTournamentEnd(TournamentParty2);
					TournamentParty2->aTournamentOutTimer = UNIXTIME + 60;
					TournamentParty2->aTournamentisStarted = false;
				}
			}

			if (TournamentParty2->aTournamentOutTimer != 0 && TournamentParty2->aTournamentOutTimer <= UNIXTIME)
				TournamentParty2->aTournamentisFinished = true;

			if (TournamentParty2->aTournamentOutTimer <= UNIXTIME
				&& TournamentParty2->aTournamentisFinished == true)
			{
				CKnights *pRedClan = g_pMain->GetClanPtr(TournamentParty2->aTournamentClanNum[0]);/*Red Clan*/
				CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentParty2->aTournamentClanNum[1]);/*Blue Clan*/
				
				/*DateTime time;
				WriteTournamentLogs(string_format("[Tournament Finish Time - %d:%d:%d] Red Clan Name = %s : %s Blue Clan Name (Red Board %d : %d Blue Board)\n",
					time.GetHour(), time.GetMinute(), time.GetSecond(), pRedClan == nullptr ? "null" : pRedClan->GetName().c_str(), pBlueClan == nullptr ? "null" : pBlueClan->GetName().c_str(), TournamentParty2->aTournamentScoreBoard[0], TournamentParty2->aTournamentScoreBoard[1]));*/

				KickOutZoneUsers(ZONE_PARTY_VS_2, ZONE_MORADON);
				m_ClanVsDataList.DeleteData(97);
			}
		}

		if (TournamentParty2->aTournamentTimer > 0)
			TournamentParty2->aTournamentTimer--;

		if (TournamentParty2->aTournamentisStarted && (TournamentParty2->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(TournamentParty2);
	}

	_TOURNAMENT_DATA* TournamentParty3 = g_pMain->m_ClanVsDataList.GetData(98);
	if (TournamentParty3 != nullptr)
	{
		if (TournamentParty3->aTournamentisStarted == true)
		{
			if (TournamentParty3->aTournamentisFinished == false)
			{
				if (TournamentParty3->aTournamentTimer == 0)
				{
					HandleTournamentEnd(TournamentParty3);
					TournamentParty3->aTournamentOutTimer = UNIXTIME + 60;
					TournamentParty3->aTournamentisStarted = false;
				}
			}

			if (TournamentParty3->aTournamentOutTimer != 0 && TournamentParty3->aTournamentOutTimer <= UNIXTIME)
				TournamentParty3->aTournamentisFinished = true;

			if (TournamentParty3->aTournamentOutTimer <= UNIXTIME
				&& TournamentParty3->aTournamentisFinished == true)
			{
				
				CKnights *pRedClan = g_pMain->GetClanPtr(TournamentParty3->aTournamentClanNum[0]);/*Red Clan*/
				CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentParty3->aTournamentClanNum[1]);/*Blue Clan*/
				
				/*DateTime time;
				WriteTournamentLogs(string_format("[Tournament Finish Time - %d:%d:%d] Red Clan Name = %s : %s Blue Clan Name (Red Board %d : %d Blue Board)\n",
					time.GetHour(), time.GetMinute(), time.GetSecond(), pRedClan == nullptr ? "null" : pRedClan->GetName().c_str(), pBlueClan == nullptr ? "null" : pBlueClan->GetName().c_str(), TournamentParty3->aTournamentScoreBoard[0], TournamentParty3->aTournamentScoreBoard[1]));*/

				KickOutZoneUsers(ZONE_PARTY_VS_3, ZONE_MORADON);
				m_ClanVsDataList.DeleteData(98);
			}
		}

		if (TournamentParty3->aTournamentTimer > 0)
			TournamentParty3->aTournamentTimer--;

		if (TournamentParty3->aTournamentisStarted && (TournamentParty3->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(TournamentParty3);
	}

	_TOURNAMENT_DATA* TournamentParty4 = g_pMain->m_ClanVsDataList.GetData(99);
	if (TournamentParty4 != nullptr)
	{
		if (TournamentParty4->aTournamentisStarted == true)
		{
			if (TournamentParty4->aTournamentisFinished == false)
			{
				if (TournamentParty4->aTournamentTimer == 0)
				{
					HandleTournamentEnd(TournamentParty4);
					TournamentParty4->aTournamentOutTimer = UNIXTIME + 60;
					TournamentParty4->aTournamentisStarted = false;
				}
			}

			if (TournamentParty4->aTournamentOutTimer != 0 && TournamentParty4->aTournamentOutTimer <= UNIXTIME)
				TournamentParty4->aTournamentisFinished = true;

			if (TournamentParty4->aTournamentOutTimer <= UNIXTIME
				&& TournamentParty4->aTournamentisFinished == true)
			{
				CKnights *pRedClan = g_pMain->GetClanPtr(TournamentParty4->aTournamentClanNum[0]);/*Red Clan*/
				CKnights *pBlueClan = g_pMain->GetClanPtr(TournamentParty4->aTournamentClanNum[1]);/*Blue Clan*/
				
				/*DateTime time;
				WriteTournamentLogs(string_format("[Tournament Finish Time - %d:%d:%d] Red Clan Name = %s : %s Blue Clan Name (Red Board %d : %d Blue Board)\n",
					time.GetHour(), time.GetMinute(), time.GetSecond(), pRedClan == nullptr ? "null" : pRedClan->GetName().c_str(), pBlueClan == nullptr ? "null" : pBlueClan->GetName().c_str(), TournamentParty4->aTournamentScoreBoard[0], TournamentParty4->aTournamentScoreBoard[1]));*/

				KickOutZoneUsers(ZONE_PARTY_VS_4, ZONE_MORADON);
				m_ClanVsDataList.DeleteData(99);
			}
		}

		if (TournamentParty4->aTournamentTimer > 0)
			TournamentParty4->aTournamentTimer--;

		if (TournamentParty4->aTournamentisStarted && (TournamentParty4->aTournamentTimer % 5) == 0)
			SendTournamentScorePacket(TournamentParty4);
	}
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

	// Score + Timer paketi yayinla (helper kullan)
	SendTournamentScorePacket(TournamentInfo);
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

	CKnights *pUserClan = g_pMain->GetClanPtr(puser->GetClanID());
	_TOURNAMENT_DATA* TournamentClanInfo = g_pMain->m_ClanVsDataList.GetData(puser->GetZoneID());
	if (pUserClan == nullptr
		|| TournamentClanInfo == nullptr
		|| (pUserClan->GetID() != TournamentClanInfo->aTournamentClanNum[0]
			&& pUserClan->GetID() != TournamentClanInfo->aTournamentClanNum[1]))
	{
		puser->NativeZoneReturn();
		puser->UserDataSaveToAgent();
		puser->Disconnect();
		return;
	}

	// S115 Plan A — Monument kill catch-up bonus (refactor)
	// Eski kod yanlis kontrol yapiyordu (klan ID karsilastirma -> kucuk ID hep bonus alirdi)
	// Dogru mantik: Anit kiran klan'in score'u EZILIYORSA (geride ise) +bonus al
	if (TournamentClanInfo == nullptr) return;
	if (!TournamentClanInfo->aTournamentisStarted) return;
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