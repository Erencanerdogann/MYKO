#include "stdafx.h"
#include "KingSystem.h"
#include "DBAgent.h"
#include <iostream>
#include "MagicInstance.h"

uint32 CGameServerDlg::Timer_UpdateGameTime(void * lpParam)
{
	while (g_bRunning)
	{
		g_pMain->UpdateGameTime();

		if (!g_pMain->pCollectionRaceEvent.isCRActive && UNIXTIME > g_pMain->autocrchecktime) {
			g_pMain->m_CollectionRaceListArray.m_lock.lock();
			auto copylist = g_pMain->m_CollectionRaceListArray.m_UserTypeMap;
			g_pMain->m_CollectionRaceListArray.m_lock.unlock();

			foreach(itr, copylist) {
				auto* p = itr->second;
				if (!p) continue;

				if (!p->autostart || p->autohour < 0 || p->autominute < 0)
					continue;

				if (p->autohour == g_pMain->m_sHour && p->autominute == g_pMain->m_sMin && g_pMain->m_sSec == 0) {
					g_pMain->autocrchecktime = UNIXTIME + 5;
					g_pMain->CollectionRaceStart(p, itr->first, nullptr);
				}
			}
		}

		sleep(1 * SECOND);
	}
	return 0;
}

void CGameServerDlg::UpdateGameTime()
{
	DateTime now(&g_localTime);
	g_pMain->LicenseSystem(); // 18.10.2020 Belirlenen Tarihe Lisanslama
	Packet result;

	if (m_sSec != now.GetSecond())
	{
		// Check timed King events.
		m_KingSystemArray.m_lock.lock();
		foreach_stlmap_nolock(itr, m_KingSystemArray)
		{
			if (itr->second == nullptr)
				continue;

			itr->second->CheckKingTimer();
		}
		m_KingSystemArray.m_lock.unlock();

		RLOCK(m_BotArray);
		foreach_stlmap_nolock(itr, m_BotArray)
		{
			CBot * pUser = itr->second;

			if (pUser == nullptr)
				continue;

			if (!pUser->isInGame())
				continue;
			
			if (pUser->restyping > 12 && pUser->restyping < 15)
				pUser->FindMonsterAttackSlot();
		
			if ((pUser->m_bResHpType == USER_MINING
				|| pUser->m_bResHpType == USER_FLASHING)
				&& pUser->m_for_bottime + 15 < uint32(UNIXTIME))
			{
				Packet result(WIZ_MINING, uint8(MiningAttempt));
				uint16 resultCode = MiningResultSuccess, Random = myrand(0, 10000);
				uint16 sEffect = 0;

				if (Random > 4000
					|| pUser->m_bResHpType == USER_SITDOWN) // EXP
					sEffect = 13082; // "XP" effect
				else
					sEffect = 13081; // "Item" effect

				result << resultCode << pUser->GetID() << sEffect;
				pUser->SendToRegion(&result);
				pUser->m_for_bottime = uint32(UNIXTIME);
			}
		}
		RULOCK(m_BotArray);
	}

	if (m_sMin != now.GetMinute())
	{
		m_ReloadKnightAndUserRanksMinute++;
		if (m_ReloadKnightAndUserRanksMinute == RELOAD_KNIGHTS_AND_USER_RATING) {
			m_ReloadKnightAndUserRanksMinute = 0;
			ReloadKnightAndUserRanks(false);
		}

		// Player Ranking Rewards
		std::list<std::string> vargs = StrSplit(m_sPlayerRankingsRewardZones, ",");
		uint8 nZones = (uint8)vargs.size();
		if (nZones > 0)
		{
			uint8 nZoneID = 0;
			for (int i = 0; i < nZones; i++)
			{
				nZoneID = (uint8)SafeAtoi(vargs.front(), 0, 255);
				SetPlayerRankingRewards(nZoneID);
				vargs.pop_front();
			}
		}
	}

	// Every hour
	if (m_sHour != now.GetHour())
	{
		ResetPlayerRankings();
		UpdateWeather();
		//SetGameTime();

		if (m_bSantaOrAngel)
			SendFlyingSantaOrAngel();

		result.clear();
		result.Initialize(WIZ_DB_SAVE);
		result << uint8(ProcDbServerType::UpdateKnights);
		g_pMain->AddDatabaseRequest(result);
	}

	// Every day
	if (m_sDate != now.GetDay())
	{
		EventTimerSet();
		g_pMain->UpdateFlagAndCape();
		//printf("G3 Ustu Klanlarin Pelerinleri Dusuruldu!\n");


		result.clear();
		result.Initialize(WIZ_DB_SAVE);
		result << uint8(ProcDbServerType::UpdateSiegeWarfareDb);
		g_pMain->AddDatabaseRequest(result);
		
		m_KingSystemArray.m_lock.lock();
		foreach_stlmap_nolock(itr, m_KingSystemArray)
		{
			if (itr->second == nullptr)
				continue;

			result.clear();
			result.Initialize(WIZ_DB_SAVE);
			result << uint8(ProcDbServerType::UpdateKingSystemDb);
			result << itr->second->m_byNation << itr->second->m_nNationalTreasury << itr->second->m_nTerritoryTax;
			g_pMain->AddDatabaseRequest(result);
		}
		m_KingSystemArray.m_lock.unlock();
	}

	// Every month
	if (m_sMonth != now.GetMonth())
	{
		// Reset monthly NP.
		result.clear();
		result.Initialize(WIZ_DB_SAVE);
		result << uint8(ProcDbServerType::ResetLoyalty);
		g_pMain->AddDatabaseRequest(result);
	}

	if (!m_sSec)
	{
		foreach_stlmap(itr, g_pMain->m_AutomaticCommandArray)
		{
			auto* p = itr->second;
			if (!p || p->command.empty()) continue;

			if (p->hour != m_sHour || p->minute != m_sMin)
				continue;

			if (p->iDay != 7 && p->iDay != g_localTime.tm_wday)
				continue;

			ProcessServerCommand(p->command);
		}
	}
	
	// Update the server time
	m_sYear = now.GetYear();
	m_sMonth = now.GetMonth();
	m_sDate = now.GetDay();
	m_sHour = now.GetHour();
	m_sMin = now.GetMinute();
	m_sSec = now.GetSecond();

}

void CUser::BotUsingSkill(uint16 sTargetID, uint32 nSkillID) {
	MagicInstance instance;
	instance.bIsRunProc = true;
	instance.sCasterID = sTargetID;
	instance.sTargetID = GetSocketID();
	instance.nSkillID = nSkillID;
	instance.sSkillCasterZoneID = GetZoneID();
	instance.Run();
}

uint32 CGameServerDlg::Timer_t_1(void* lpParam) {
#if(SKILLTEST)

	return 0;

	while (g_bRunning) {
		for (uint16 i = 0; i < MAX_USER; i++) {
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr || !pUser->isInGame()) continue;

			CNpcThread* zoneitrThread = g_pMain->m_arNpcThread.GetData(pUser->GetZoneID());
			if (!zoneitrThread)
				continue;

			zoneitrThread->m_arNpcArray.m_lock.lock();
			auto copymap = zoneitrThread->m_arNpcArray.m_UserTypeMap;
			zoneitrThread->m_arNpcArray.m_lock.unlock();

			foreach(ax, copymap) {
				if (ax->second && ax->second->GetProtoID() == 13013)
					pUser->ClientEvent(ax->second->GetID());
			}
}

		sleep(1000);
	}
#endif
	return (uint32)0;
}

uint32 CGameServerDlg::Timer_t_2(void* lpParam) {
#if(SKILLTEST)


	while (g_bRunning) {
		/*g_pMain->m_arNpcThread.m_lock.lock();
		foreach_stlmap_nolock(iyt, g_pMain->m_arNpcThread) {
			if (!iyt->second) continue;
			iyt->second->m_arNpcArray.m_lock.lock();
			foreach_stlmap_nolock(a, iyt->second->m_arNpcArray) {
				if (a->second) {
					a->second->IsNoPathFind(10.0f);
					a->second->Type4Duration();
				}
			}
			iyt->second->m_arNpcArray.m_lock.unlock();
		}
		g_pMain->m_arNpcThread.m_lock.unlock();*/


		for (uint16 i = 0; i < MAX_USER; i++) {
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr || !pUser->isInGame())
				continue;

			/*pUser->SendPremiumInfo();

			pUser->InitType4();
			pUser->Type4Duration();
			pUser->Update();
			pUser->m_bInvisibilityType = (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE;
			pUser->Type9Duration(true);
			pUser->RemoveStealth();*/

			if (pUser->isWarrior()) {
				if (pUser->isNoviceWarrior()) {
					pUser->BotUsingSkill(pUser->GetSocketID(), 105002); //sprint

				}
				else if (pUser->isMasteredWarrior()) {
					if (pUser->GetNation() == 1) {
						pUser->BotUsingSkill(pUser->GetSocketID(), 106002); //sprint
					}
					else {
						pUser->BotUsingSkill(pUser->GetSocketID(), 206002); //sprint
					}
				}
			}

			if (UNIXTIME > pUser->testskillusetime) {
				pUser->BotUsingSkill(pUser->GetSocketID(), 500354);//814678000
				pUser->BotUsingSkill(pUser->GetSocketID(), 500512);//800220000
				pUser->BotUsingSkill(pUser->GetSocketID(), 500508);//800130000
				pUser->BotUsingSkill(pUser->GetSocketID(), 490160);//800127000

				if (!pUser->CheckExistItem(814678000)) pUser->GiveItem("tst", 814678000);
				if (!pUser->CheckExistItem(800220000)) pUser->GiveItem("tst", 800220000);
				if (!pUser->CheckExistItem(800130000)) pUser->GiveItem("tst", 800130000);
				if (!pUser->CheckExistItem(800127000)) pUser->GiveItem("tst", 800127000);
			}

			if (UNIXTIME > pUser->testskillusetime2) {
				std::vector<uint8> willDel;
				pUser->m_buffLock.lock();
				foreach(itr, pUser->m_buffMap) willDel.push_back(itr->first);
				pUser->m_buffLock.unlock();
				foreach(itr, willDel)
					CMagicProcess::RemoveType4Buff((*itr), pUser, true, pUser->isLockableScroll((*itr)));
				pUser->testskillusetime2 = UNIXTIME + myrand(120, 240);
			}
		}
		sleep(1 * SECOND);
	}
#endif
	return (uint32)0;
}

uint32 CGameServerDlg::Timer_t_3(void* lpParam) {
#if(SKILLTEST)
	return 0;
	while (g_bRunning) {
		for (uint16 i = 0; i < MAX_USER; i++) {
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr || !pUser->isInGame())
				continue;

			CNpcThread* zoneitrThread = g_pMain->m_arNpcThread.GetData(pUser->GetZoneID());
			if (!zoneitrThread)
				continue;

			zoneitrThread->m_arNpcArray.m_lock.lock();
			auto copymap = zoneitrThread->m_arNpcArray.m_UserTypeMap;
			zoneitrThread->m_arNpcArray.m_lock.unlock();

			foreach(ax, copymap) {
				if (ax->second && ax->second->GetProtoID() == 13013)
					pUser->ClientEvent(ax->second->GetID());
			}

		}
		sleep(myrand(1250, 2000));
	}
#endif
	return (uint32)0;
}

#pragma region CGameServerDlg::Timer_UpdateSessions(void * lpParam)
uint32 CGameServerDlg::Timer_UpdateSessions(void * lpParam)
{
	// CURRENTUSER heartbeat (askida char B yontem): her 30sn online char'larin last_heartbeat'ini
	// damgala. isInOnline (DB) son 2dk heartbeat'i olani online sayar -> crash/DC'de heartbeat durur
	// -> 2dk sonra DB otomatik hayalet sayip login acar (manuel/restart bagimsiz, kalici cozum).
	static ULONGLONG s_nextHeartbeat = 0;

	while (g_bRunning)
	{
		bool bHeartbeatTick = (UNIXTIME2 >= s_nextHeartbeat);
		if (bHeartbeatTick)
			s_nextHeartbeat = UNIXTIME2 + 30; // 30sn periyot (TTL 2dk'ya 4 tick tolerans)

		g_pMain->m_socketMgr.GetLock().lock();
		SessionMap sessMap = g_pMain->m_socketMgr.GetActiveSessionMap();
		g_pMain->m_socketMgr.GetLock().unlock();
		foreach(itr, sessMap) {
			CUser* pUser = TO_USER(itr->second);
			if (pUser == nullptr)
				continue;

			if (!pUser->m_strAccountID.empty() && !pUser->isInGame()) {
				ULONGLONG timeout = KOSOCKET_LOADING_TIMEOUT, nDifference = (UNIXTIME2 - pUser->GetLastResponseTime());
				if (nDifference >= timeout) {
					pUser->goDisconnect("time out", __FUNCTION__);
					continue;
				}
			}

			if (pUser->isInGame()) {
				pUser->Update();
				if (UNIXTIME2 > g_pMain->DelayedTime) {
					g_pMain->DelayedTime = UNIXTIME2 + (1 * SECOND);
					pUser->CheckDelayedTime();
				}

				// 30sn heartbeat damgasi (online char gercekten yasiyor)
				if (bHeartbeatTick && !pUser->m_strAccountID.empty())
					g_DBAgent.UpdateCurrentUserHeartbeat(pUser->m_strAccountID);
			}
		}

		// DC Grace Period: idle session'lardaki OffCharacter karakterlerin timer'ini calistir
		// DC olunca socket kapanir ve karakter m_activeSessions'tan m_idleSessions'a tasinir.
		// Update() sadece active session'lari taradigi icin, idle'daki DC grace period timer'i
		// hic tick etmez ve karakter sonsuza kadar askida kalir. Bu blok onu cozer.
		{
			g_pMain->m_socketMgr.GetLock().lock();
			SessionMap idleMap = g_pMain->m_socketMgr.GetIdleSessionMap();
			g_pMain->m_socketMgr.GetLock().unlock();
			foreach(itr, idleMap) {
				CUser* pUser = TO_USER(itr->second);
				if (pUser == nullptr)
					continue;

				// Sadece DC grace period'daki karakterleri isle
				if (!pUser->IsOffCharacter() || pUser->GetOffMerchantTime() == 0)
					continue;

				// isInGame kontrolu — state hala INGAME olmali
				if (!pUser->isInGame())
					continue;

				pUser->Update();
			}
		}

		/* RECONNECT PARK - Grace period timer blogu kaldirildi
		static uint32 s_nextGraceCheck = 0;
		if (UNIXTIME > s_nextGraceCheck)
		{
			...
		} */

		sleep(1 * SECOND);
	}
	return uint32(0);
}
#pragma endregion

#pragma region CGameServerDlg::Timer_UpdateConcurrent(void * lpParam)
uint32 CGameServerDlg::Timer_UpdateConcurrent(void * lpParam)
{
	while (true)
	{
		g_pMain->ReqUpdateConcurrent();
		sleep(120 * SECOND);
	}
	return 0;
}
#pragma endregion 

#pragma region CGameServerDlg::Timer_TimedNotice(void * lpParam)
uint32 CGameServerDlg::Timer_TimedNotice(void* lpParam)
{
	while (true) {
		foreach_stlmap_nolock(itr, g_pMain->m_TimedNoticeArray) {
			_TIMED_NOTICE* ptimed = itr->second;
			if (ptimed == nullptr || ptimed->usingtime > UNIXTIME) continue;
			if (ptimed->time < 1) ptimed->time = 1;
			ptimed->usingtime = (uint32)UNIXTIME + ptimed->time * MINUTE;
			Packet result;
			std::string notice = ptimed->notice;
			g_pMain->GetServerResource(IDP_ANNOUNCEMENT, &notice, notice.c_str());
			ChatPacket::Construct(&result, (uint8)ptimed->noticetype, &notice);
			if (ptimed->zoneid != 0)g_pMain->Send_Zone(&result, (uint8)ptimed->zoneid, nullptr, (uint8)Nation::ALL);
			else g_pMain->Send_All(&result, nullptr, (uint8)Nation::ALL);
		} sleep(60 * SECOND);


	} return uint32(0);
}
#pragma endregion 

#pragma region CGameServerDlg::ReqUpdateConcurrent()
void CGameServerDlg::ReqUpdateConcurrent()
{
	uint32 sCount = 0;
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (!pUser || !pUser->isInGame())
			continue;

		sCount++;
	}
	sCount += (uint32)g_pMain->m_BotcharacterNameMap.size();

	Packet result(WIZ_ZONE_CONCURRENT);
	result << uint32(m_nServerNo)
		<< sCount;
	AddDatabaseRequest(result);
}
#pragma endregion