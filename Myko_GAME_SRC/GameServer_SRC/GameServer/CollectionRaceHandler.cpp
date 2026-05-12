#include "stdafx.h"
#include "DBAgent.h"

#pragma region CUser::HandleCollectionRaceStart
COMMAND_HANDLER(CUser::HandleCollectionRaceStart)
{
	if (!isGM())
		return false;

	if (vargs.empty() || vargs.size() < 1)
	{
		g_pMain->SendHelpDescription(this, "+cropen sEventIndex");
		return false;
	}

	uint16 sIndexID = (uint16)SafeAtoi(vargs.front(), 0, 65535);
	vargs.pop_front();

	_COLLECTION_RACE_EVENT_LIST * pCollect = g_pMain->m_CollectionRaceListArray.GetData(sIndexID);
	if (pCollect == nullptr)
	{
		g_pMain->SendHelpDescription(this, "CollectionRace sEventIndex is nullptr");
		return false;
	}

	g_pMain->CollectionRaceStart(pCollect, sIndexID, this);

	return true;
}
#pragma endregion

COMMAND_HANDLER(CUser::HandleCollectionRaceClose)
{
	if (!isGM()) return false;

	if (!g_pMain->pCollectionRaceEvent.isCRActive) {
		g_pMain->SendHelpDescription(this, "CR event is already closed.");
		return false;
	} g_pMain->CollectionRaceEnd();
	return true;
}

void CGameServerDlg::ReqCollectionRaceStart(Packet &pkt) {

	pCollectionRaceEvent.openrequest = false;

	uint16 index = pkt.read<uint16>();

	_COLLECTION_RACE_EVENT_LIST* pList = g_pMain->m_CollectionRaceListArray.GetData(index);
	if (pList == nullptr)
		return;

	bool RaceEventZone = true;
	for (int i = 0; i < 3; i++)
	{
		if (pList->RewardItemID[i]) {
			_ITEM_TABLE pTable = g_pMain->GetItemPtr(pList->RewardItemID[i]);
			if (pTable.isnull())
				return;
		}
		if (pList->RewardItemCount) {
			if (pList->RewardItemCount[i] > ZONEITEM_MAX && (pList->RewardItemID[i] == ITEM_GOLD || pList->RewardItemID[i] == ITEM_EXP))
				return;
			else if (pList->RewardItemCount[i] > 9999 && pList->RewardItemID[i] != ITEM_GOLD && pList->RewardItemID[i] != ITEM_EXP)
				return;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		pCollectionRaceEvent.ProtoID[i] = pList->ProtoID[i];
		pCollectionRaceEvent.KillCount[i] = pList->KillCount[i];
		pCollectionRaceEvent.RewardItemID[i] = 0;
		pCollectionRaceEvent.RewardItemCount[i] = 0;
		pCollectionRaceEvent.RewardItemTime[i] = 0;
	}

	g_DBAgent.LoadCollectionReward(pList->m_EventID);
	pCollectionRaceEvent.MinLevel = pList->MinLevel;
	pCollectionRaceEvent.MaxLevel = pList->MaxLevel;
	pCollectionRaceEvent.ZoneID = pList->ZoneID;
	pCollectionRaceEvent.EventTime = (uint32)UNIXTIME + (pList->EventTime * 60);
	pCollectionRaceEvent.UserLimit = pList->UserLimit;
	pCollectionRaceEvent.EventName = pList->EventName;
	pCollectionRaceEvent.TotalCount = 0;
	pCollectionRaceEvent.isCRActive = true;
	pCollectionRaceEvent.rankbug = myrand(g_pMain->pRankBug.CrMinComp > 0 ? g_pMain->pRankBug.CrMinComp : 1, g_pMain->pRankBug.CrMaxComp > 0 ? g_pMain->pRankBug.CrMaxComp : 1);
	pCollectionRaceEvent.isRepeat = pList->RepeatStatus;
	pCollectionRaceEvent.m_userlit.DeleteAllData();

	std::string descp1 = "", descp2 = "Quest : ", descp3 = "Reward : ", descp4 = "", title = "Collection_Race_Info";
	descp1.append(string_format("Mission : %s", pCollectionRaceEvent.EventName.c_str()));
	for (int i = 0; i < 3; i++) {
		if (pCollectionRaceEvent.ProtoID[i]) {
			auto* pnpc = m_arMonTable.GetData(pCollectionRaceEvent.ProtoID[i]);
			if (pnpc) {
				if (i == 2)descp2.append(string_format("%dx : %s", pCollectionRaceEvent.KillCount[i], pnpc->m_strName.c_str()));
				else descp2.append(string_format("%dx : %s -", pCollectionRaceEvent.KillCount[i], pnpc->m_strName.c_str()));
			}
		}
	}
	for (int i = 0; i < 3; i++) {
		if (pCollectionRaceEvent.RewardItemID[i]) {
			auto pitem = GetItemPtr(pCollectionRaceEvent.RewardItemID[i]);
			if (!pitem.isnull()) {
				if (i == 2)
					descp3.append(string_format("%dx : %s", pCollectionRaceEvent.RewardItemCount[i], pitem.m_sName.c_str()));
				else
					descp3.append(string_format("%dx : %s -", pCollectionRaceEvent.RewardItemCount[i], pitem.m_sName.c_str()));
			}
		}
	}

	std::string zonename = "-";
	auto* pzone = g_pMain->m_ZoneArray.GetData(pCollectionRaceEvent.ZoneID);
	if (pzone) zonename = pzone->m_nZoneName;
	descp4.append(string_format("Zone: %s - level: (%d-%d)", zonename.c_str(), pCollectionRaceEvent.MinLevel, pCollectionRaceEvent.MaxLevel));

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		pUser->CollectionRace.CheckFinish = false;

		for (int j = 0; j < 3; j++)
			pUser->CollectionRace.KillCount[j] = 0;

		if (pUser->GetLevel() < pCollectionRaceEvent.MinLevel || pUser->GetLevel() > pCollectionRaceEvent.MaxLevel)
			continue;

		/*if (pCollectionRaceEvent.ZoneID != 0 && pUser->GetZoneID() != pCollectionRaceEvent.ZoneID)
			RaceEventZone = false;*/

			//continue;
		if (RaceEventZone) //16.05.2021 CrEvent zonesinde bulunmayanlara g�stermeyecek fakat listeye kay�t edecek di�er t�rl� hi� ekrana gelmiyor sonradan evente katilsan bile
		{
			Packet result(XSafe);
			result << uint8(XSafeOpCodes::CR) << uint8(0x00);

			for (int i = 0; i < 3; i++)
				result << pCollectionRaceEvent.ProtoID[i] << pCollectionRaceEvent.KillCount[i];

			for (int i = 0; i < 3; i++)
				result << pCollectionRaceEvent.RewardItemID[i] << pCollectionRaceEvent.RewardItemCount[i] << pCollectionRaceEvent.RewardItemRate[i];

			result << (uint32)(pList->EventTime * 60) << uint16(g_pMain->pCollectionRaceEvent.TotalCount * g_pMain->pCollectionRaceEvent.rankbug) << pCollectionRaceEvent.UserLimit << pUser->GetNation();
			result << pCollectionRaceEvent.EventName << pCollectionRaceEvent.ZoneID;
			pUser->Send(&result);
		}

		_CR_USER_LIST* ptest = pCollectionRaceEvent.m_userlit.GetData(pUser->GetName());
		if (ptest != nullptr) {
			ptest->KillCount[0] = 0;
			ptest->KillCount[1] = 0;
			ptest->KillCount[2] = 0;
			ptest->isFinish = false;
		}
		else {
			_CR_USER_LIST* pNewUser = new _CR_USER_LIST();
			pNewUser->KillCount[0] = 0;
			pNewUser->KillCount[1] = 0;
			pNewUser->KillCount[2] = 0;
			pNewUser->isFinish = false;
			if (!pCollectionRaceEvent.m_userlit.PutData(pUser->GetName(), pNewUser)) {
				delete pNewUser;
				continue;
			}
		}

		{
			Packet newpm1, newpm2, newpm3, newpm4;
			ChatPacket::Construct(&newpm1, (uint8)ChatType::PRIVATE_CHAT, descp1.c_str(), title.c_str(), pUser->GetNation());
			ChatPacket::Construct(&newpm2, (uint8)ChatType::PRIVATE_CHAT, descp2.c_str(), title.c_str(), pUser->GetNation());
			ChatPacket::Construct(&newpm3, (uint8)ChatType::PRIVATE_CHAT, descp3.c_str(), title.c_str(), pUser->GetNation());
			ChatPacket::Construct(&newpm4, (uint8)ChatType::PRIVATE_CHAT, descp4.c_str(), title.c_str(), pUser->GetNation());
			pUser->Send(&newpm1);
			pUser->Send(&newpm2);
			pUser->Send(&newpm3);
			pUser->Send(&newpm4);
		}
	}

	std::string buffer = "Collection Race Event started.";
	SendChat<ChatType::PUBLIC_CHAT>(buffer.c_str(), (uint8)Nation::ALL, true);
}

void CGameServerDlg::CollectionRaceStart(_COLLECTION_RACE_EVENT_LIST * pList, uint16 sIndex, CUser *pUser)
{
	if (pList == nullptr)
		return;

	if (pCollectionRaceEvent.isCRActive)
		CollectionRaceEnd();

	if (pCollectionRaceEvent.openrequest) {
		if(pUser)
			SendHelpDescription(pUser, "Daha Once Collection Race acilma istegi yolland�. L�tfen bekleyiniz");

		return;
	}

	if (pUser)
		SendHelpDescription(pUser, "Collection Race acilma istegi yolland�. L�tfen bekleyiniz");

	pCollectionRaceEvent.openrequest = true;
	Packet newpkt(WIZ_DB_SAVE, uint8(ProcDbServerType::CollectionRaceStart));
	newpkt << sIndex;
	g_pMain->AddDatabaseRequest(newpkt);
}

void CUser::CollectionRaceFirstLoad()
{
	if (!g_pMain->pCollectionRaceEvent.isCRActive)
		return;

	_CR_USER_LIST *pList = new _CR_USER_LIST();
	pList->KillCount[0] = 0;
	pList->KillCount[1] = 0;
	pList->KillCount[2] = 0;
	pList->isFinish = false;
	pList->sStage = 0;
	if (!g_pMain->pCollectionRaceEvent.m_userlit.PutData(GetName(), pList))
		delete pList;

	_CR_USER_LIST * ptest = g_pMain->pCollectionRaceEvent.m_userlit.GetData(GetName());
	if (ptest == nullptr || ptest->isFinish)
		return;

	if (GetLevel() < g_pMain->pCollectionRaceEvent.MinLevel || GetLevel() > g_pMain->pCollectionRaceEvent.MaxLevel)
		return;


	/*CollectionRace.CheckFinish = false;

	for (int j = 0; j < 3; j++)
		CollectionRace.KillCount[j] = 0;

	if (GetLevel() < g_pMain->pCollectionRaceEvent.MinLevel || GetLevel() > g_pMain->pCollectionRaceEvent.MaxLevel)
		return;
	*/

	uint32 RemainingTime = 0;
	if (g_pMain->pCollectionRaceEvent.EventTime > (uint32)UNIXTIME)
		RemainingTime = g_pMain->pCollectionRaceEvent.EventTime - (uint32)UNIXTIME;

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::CR) << uint8(0x01);

	for (int i = 0; i < 3; i++)
	{
		CollectionRace.KillCount[i] = ptest->KillCount[i];
		uint16 KillCount = g_pMain->pCollectionRaceEvent.KillCount[i];
		if (ptest->sStage > 0)
			KillCount = (uint16)(KillCount * 5 / 2);

		result << g_pMain->pCollectionRaceEvent.ProtoID[i] << KillCount << ptest->KillCount[i];
	}
	for (int i = 0; i < 3; i++)
		result << g_pMain->pCollectionRaceEvent.RewardItemID[i] << g_pMain->pCollectionRaceEvent.RewardItemCount[i] << g_pMain->pCollectionRaceEvent.RewardItemRate[i];

	result << RemainingTime << uint16(g_pMain->pCollectionRaceEvent.TotalCount*g_pMain->pCollectionRaceEvent.rankbug) << g_pMain->pCollectionRaceEvent.UserLimit << GetNation();
	result << g_pMain->pCollectionRaceEvent.EventName << g_pMain->pCollectionRaceEvent.ZoneID;
	Send(&result);
}

void CUser::CollectionGetActiveTime()
{
	if (!g_pMain->pCollectionRaceEvent.isCRActive)
		return;

	_CR_USER_LIST * ptest = g_pMain->pCollectionRaceEvent.m_userlit.GetData(GetName());
	if (ptest == nullptr || ptest->isFinish)
		return;

	if (GetLevel() < g_pMain->pCollectionRaceEvent.MinLevel || GetLevel() > g_pMain->pCollectionRaceEvent.MaxLevel)
		return;

	/*CollectionRace.CheckFinish = false;

	for (int j = 0; j < 3; j++)
		CollectionRace.KillCount[j] = 0;

	if (GetLevel() < g_pMain->pCollectionRaceEvent.MinLevel || GetLevel() > g_pMain->pCollectionRaceEvent.MaxLevel)
		return;
	*/

	uint32 RemainingTime = 0;
	if (g_pMain->pCollectionRaceEvent.EventTime > (uint32)UNIXTIME)
		RemainingTime = g_pMain->pCollectionRaceEvent.EventTime - (uint32)UNIXTIME;

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::CR) << uint8(0x01);

	for (int i = 0; i < 3; i++)
	{
		uint16 KillCount = g_pMain->pCollectionRaceEvent.KillCount[i];
		if (ptest->sStage > 0)
			KillCount = (uint16)(KillCount * 5 / 2);

		result << g_pMain->pCollectionRaceEvent.ProtoID[i] << KillCount << ptest->KillCount[i];
	}
	for (int i = 0; i < 3; i++)
		result << g_pMain->pCollectionRaceEvent.RewardItemID[i] << g_pMain->pCollectionRaceEvent.RewardItemCount[i] << g_pMain->pCollectionRaceEvent.RewardItemRate[i];

	result << RemainingTime << uint16(g_pMain->pCollectionRaceEvent.TotalCount* g_pMain->pCollectionRaceEvent.rankbug) << g_pMain->pCollectionRaceEvent.UserLimit << GetNation();
	result << g_pMain->pCollectionRaceEvent.EventName << g_pMain->pCollectionRaceEvent.ZoneID;
	Send(&result);
}

void CUser::CollectionRaceHide()
{
	if (!g_pMain->pCollectionRaceEvent.isCRActive)
		return;

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::CR) << uint8(0x05);
	Send(&result);
}

void CGameServerDlg::CollectionRaceTimer()
{
	if (pCollectionRaceEvent.isCRActive)
	{
		if (pCollectionRaceEvent.EventTime > 0)
		{
			uint32 RemainingTime = 0;
			if (pCollectionRaceEvent.EventTime > (uint32)UNIXTIME)
				RemainingTime = pCollectionRaceEvent.EventTime - (uint32)UNIXTIME;
		/*	if (RemainingTime == 900)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 15), 1, 240, 1);
			else if (RemainingTime == 600)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 10), 1, 240, 1);
			else if (RemainingTime == 300)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 5), 1, 240, 1);
			else if (RemainingTime == 180)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 3), 1, 240, 1);
			else if (RemainingTime == 120)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 2), 1, 240, 1);
			else if (RemainingTime == 60)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 1), 1, 240, 1);*/

			if (!RemainingTime)
				CollectionRaceEnd();
		}
		else
			CollectionRaceEnd();
	}
}

void CGameServerDlg::CollectionRaceCounter()
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		if (pUser->GetLevel() < pCollectionRaceEvent.MinLevel || pUser->GetLevel() > pCollectionRaceEvent.MaxLevel)
			continue;

		/*if (pCollectionRaceEvent.ZoneID != 0 && pUser->GetZoneID() != pCollectionRaceEvent.ZoneID)
			continue;*/

		Packet result(XSafe);
		result << uint8(XSafeOpCodes::CR) << uint8(0x03) << uint16(pCollectionRaceEvent.TotalCount * pCollectionRaceEvent.rankbug) << pCollectionRaceEvent.UserLimit;
		pUser->Send(&result);
	}
}

void CGameServerDlg::CollectionRaceSendDead(Unit * pKiller, uint16 ProtoID)
{
	if (pKiller == nullptr || !pKiller->isPlayer())
		return;

	auto * pKillerUser = TO_USER(pKiller);

	if (pKillerUser->GetLevel() < g_pMain->pCollectionRaceEvent.MinLevel || pKillerUser->GetLevel() > g_pMain->pCollectionRaceEvent.MaxLevel)
		return;

	/*if (g_pMain->pCollectionRaceEvent.ZoneID != 0 && pKillerUser->GetZoneID() != g_pMain->pCollectionRaceEvent.ZoneID)
		return;*/

	if (pKillerUser->CollectionRace.CheckFinish)
		return;

	_CR_USER_LIST * ptest = pCollectionRaceEvent.m_userlit.GetData(pKiller->GetName());
	if (!ptest)
		return;

	bool CheckFinish = true;
	for (int i = 0; i < 3; i++)
	{
		uint16 KillCount = g_pMain->pCollectionRaceEvent.KillCount[i];
		if (ptest->sStage > 0)
			KillCount = (uint16)(KillCount * 5 / 2);

		if (g_pMain->pCollectionRaceEvent.ProtoID[i] == ProtoID)
		{
			if (pKillerUser->CollectionRace.KillCount[i] < KillCount && KillCount) {
				pKillerUser->CollectionRace.KillCount[i]++;
				ptest->KillCount[i] = pKillerUser->CollectionRace.KillCount[i];
				
			}
		}

		if (pKillerUser->CollectionRace.KillCount[i] != KillCount && KillCount)
			CheckFinish = false;
	}

	if (CheckFinish)
	{
		bool isSave = false;
		if (g_pMain->pCollectionRaceEvent.isRepeat == 0)
			isSave = true;
		else if (g_pMain->pCollectionRaceEvent.isRepeat == 1)
		{
			ptest->sStage++;
			if (ptest->sStage > 1)
				isSave = true;
		}

		if (isSave)
		{
			pKillerUser->CollectionRace.CheckFinish = CheckFinish;
			ptest->isFinish = CheckFinish;
		}
	}
	if (CheckFinish)
	{
		bool kapat = true;
		if (g_pMain->pCollectionRaceEvent.isRepeat == 2)
			kapat = false;
		if (g_pMain->pCollectionRaceEvent.isRepeat == 1 && ptest->sStage < 2)
			kapat = false;

			if (kapat)
			{
				Packet result(XSafe);
				result << uint8(XSafeOpCodes::CR) << uint8(0x04);
				pKillerUser->Send(&result);

			}
		
			if (!kapat)
			{
				for (int i = 0; i < 3;i++)
					ptest->KillCount[i] = 0;

				pKillerUser->CollectionRaceFirstLoad();
			}

		if (kapat)
			ptest->isFinish = CheckFinish;

		g_pMain->pCollectionRaceEvent.TotalCount++;

		if (g_pMain->pCollectionRaceEvent.TotalCount <= g_pMain->pCollectionRaceEvent.UserLimit) 
		{
			pKillerUser->CollectionRaceFinish();
			CollectionRaceCounter();
		}

		if (g_pMain->pCollectionRaceEvent.TotalCount >= g_pMain->pCollectionRaceEvent.UserLimit)
			CollectionRaceEnd();
	}

	Packet Dead(XSafe);
	Dead << uint8(XSafeOpCodes::CR) << uint8(0x02) << ProtoID << pKillerUser->CollectionRace.KillCount[0] << pKillerUser->CollectionRace.KillCount[1] << pKillerUser->CollectionRace.KillCount[2];
	TO_USER(pKiller)->Send(&Dead);
}
void CUser::GiveRandomItem(uint32& nItemID, uint32& nCount, uint8 bySession)
{
	std::map <uint16, _RANDOM_ITEM*> nRandom;
	int say = 0;

		foreach(itr, g_pMain->m_RandomItemArray)
		{

			_RANDOM_ITEM* pRandom = *itr;
			if (pRandom->SessionID != bySession)
				continue;
			nRandom.insert(std::make_pair(say, pRandom));
			say++;
		}
		int thisrand = static_cast<int>(nRandom.size());
		if (thisrand == 0)
			return;
		thisrand -= 1;
		int rand = myrand(0, thisrand);

		auto it = nRandom.find(rand);
		if (it != nRandom.end())
		{
			_RANDOM_ITEM* pRandom = it->second;

			nItemID = pRandom->ItemID;
			nCount = pRandom->ItemCount;
			
		}
		//printf("bySession Item ID %d %d \n", nItemID, bySession);


}
void CUser::CollectionRaceFinish()
{
	_CR_USER_LIST * ptest = g_pMain->pCollectionRaceEvent.m_userlit.GetData(GetName());
	if (ptest != nullptr) {
		ptest->isFinish = CollectionRace.CheckFinish;
	}
	uint32 tmp_RewardItemID[3]{};
	uint32 tmp_RewardItemCount[3]{};
	for (int i = 0; i < 3; i++)
	{
		CollectionRace.KillCount[i] = 0;
		tmp_RewardItemID[i] = 0;
		tmp_RewardItemCount[i] = 0;
	}

	bool isCind = pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID());

	bool lettercheck = false;
	for (int i = 0; i < 3; i++)
	{
		if (g_pMain->pCollectionRaceEvent.RewardItemID[i] > 0)
		{
			if (isCind)
			{
				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_GOLD)
				{
					pCindWar.gainnoah += g_pMain->pCollectionRaceEvent.RewardItemCount[i];
					continue;
				}

				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_EXP)
				{
					pCindWar.gainexp += g_pMain->pCollectionRaceEvent.RewardItemCount[i];
					continue;
				}
			}
			else
			{
				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_GOLD)
				{
					GoldGain(g_pMain->pCollectionRaceEvent.RewardItemCount[i]);
					continue;
				}

				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_EXP)
				{
					ExpChange("collection race", g_pMain->pCollectionRaceEvent.RewardItemCount[i], true);
					continue;
				}
			}

			if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_COUNT)
			{
				SendLoyaltyChange("collection race", g_pMain->pCollectionRaceEvent.RewardItemCount[i]);
				continue;
			}
			
			uint32 xItemID = g_pMain->pCollectionRaceEvent.RewardItemID[i];
			uint32 xItemTime = g_pMain->pCollectionRaceEvent.RewardItemTime[i];
			uint32 xCount = g_pMain->pCollectionRaceEvent.RewardItemCount[i];
			uint32 xRand = g_pMain->pCollectionRaceEvent.RewardItemRate[i];

			if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == 900004000)
				GiveRandomItem(xItemID, xCount, g_pMain->pCollectionRaceEvent.RewardSession[i]);

			tmp_RewardItemID[i] = xItemID;
			tmp_RewardItemCount[i] = xCount;

			_ITEM_TABLE pTable = g_pMain->GetItemPtr(xItemID);
			if (!pTable.isnull()) {

				int nRate = 0;
				if (xRand > 100) xRand = 100;
				if (xRand) nRate = myrand(0, 10000);

				if (nRate && static_cast<int>(xRand * 100) < nRate) {
					tmp_RewardItemID[i] = 0;
					tmp_RewardItemCount[i] = 0;
					continue;
				}

				int8 pos = FindSlotForItem(xItemID, xCount);
				if ((pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID())) || pos < 0)
				{
					Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CollectionRaceReward));
					newpkt << xItemID << xCount;
					g_pMain->AddDatabaseRequest(newpkt, this);
				}
				else
					GiveItem("Collection Race", xItemID, xCount, true, xItemTime);
			}
		}
	}
	
	Packet result(WIZ_QUEST);
	result << uint8(0x0A);
	for (int i = 0; i < 3; i++)
		result << tmp_RewardItemID[i] << tmp_RewardItemCount[i];
		
	result << uint32_t(0x00) << uint32_t(0x00) << uint32_t(0x00) << uint32_t(0x00);
	Send(&result);
}

void CGameServerDlg::CollectionRaceEnd()
{
	if (pCollectionRaceEvent.isCRActive)
	{
		std::string buffer = "Collection Race Event has end.";
		SendChat<ChatType::PUBLIC_CHAT>(buffer.c_str(), (uint8)Nation::ALL, true);
	}

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::CR) << uint8(0x04);
	
	//Send_All(&result);

	CollectionRaceSend(&result);
	CollectionRaceDataReset();
}

void CGameServerDlg::CollectionRaceDataReset()
{
	pCollectionRaceEvent.m_userlit.DeleteAllData();
	pCollectionRaceEvent.isCRActive = false;
	pCollectionRaceEvent.EventTime = 0;
	memset(&pCollectionRaceEvent.RewardItemID, 0, sizeof(pCollectionRaceEvent.RewardItemID));
	memset(&pCollectionRaceEvent.RewardItemCount, 0, sizeof(pCollectionRaceEvent.RewardItemCount));
	memset(&pCollectionRaceEvent.ProtoID, 0, sizeof(pCollectionRaceEvent.ProtoID));
	memset(&pCollectionRaceEvent.KillCount, 0, sizeof(pCollectionRaceEvent.KillCount));
	pCollectionRaceEvent.UserLimit = pCollectionRaceEvent.TotalCount = 0;
	pCollectionRaceEvent.ZoneID = pCollectionRaceEvent.MinLevel = pCollectionRaceEvent.MaxLevel = 0;
	pCollectionRaceEvent.EventName = "";
}

void CGameServerDlg::CollectionRaceSend(Packet * pkt)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		if (pUser->GetLevel() < pCollectionRaceEvent.MinLevel || pUser->GetLevel() > pCollectionRaceEvent.MaxLevel)
			continue;

		pUser->CollectionRace.CheckFinish = false;

		for (int j = 0; j < 3; j++)
			pUser->CollectionRace.KillCount[j] = 0;

		pUser->Send(pkt);
	}
}

void CGameServerDlg::LogosYolla(std::string LogosName, std::string LogosMessage, uint8 R, uint8 G, uint8 B)
{
	Packet Logos(WIZ_LOGOSSHOUT);
	std::string Birlestir = LogosName + " : " + LogosMessage;
	Logos.SByte();
	Logos << uint8(2) << uint8(1) << R << G << B << uint8(0) << Birlestir;
	g_pMain->Send_All(&Logos);
}