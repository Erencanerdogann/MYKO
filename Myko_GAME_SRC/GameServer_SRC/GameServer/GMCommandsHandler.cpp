#include "stdafx.h"
#include "DBAgent.h"
#include "KingSystem.h"

using namespace std;

#pragma region CUser::OperatorCommand(Packet & pkt)
void CUser::OperatorCommand(Packet & pkt)
{
	if (!isGM())
		return;

	Packet result (WIZ_AUTHORITY_CHANGE);
	std::string strUserID;
	uint8 opcode;
	bool bIsOnline = false;
	DateTime time;
	std::string sNoticeMessage = "", sOperatorCommandType;
	pkt >> opcode >> strUserID;

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
		return;

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	CBot* pBot = nullptr;
	if (pUser == nullptr)
		bIsOnline = false;
	else
		bIsOnline = true;

	if (!bIsOnline)
	{
		pBot = g_pMain->GetBotPtr(strUserID, NameType::TYPE_CHARACTER);;
		if (pBot)
			bIsOnline = true;
	}

	switch (opcode)
	{
	case OPERATOR_ARREST:
		if (bIsOnline)
		{
			if (pBot)
				ZoneChange(pBot->GetZoneID(), pBot->m_curx, pBot->m_curz);
			else if (pUser)
				ZoneChange(pUser->GetZoneID(), pUser->m_curx, pUser->m_curz, 0, true, true);
			else
				return;
			sOperatorCommandType = "OPERATOR_ARREST";
		}
		break;
	case OPERATOR_SUMMON:
		if (bIsOnline)
		{
			if (pUser)
				pUser->ZoneChange(GetZoneID(), m_curx, m_curz, 0, true, true);
			else
				return;

			sOperatorCommandType = "OPERATOR_SUMMON";
		}
		break;
	case OPERATOR_CUTOFF:
		if (bIsOnline)
		{
			if (pUser)
				pUser->Disconnect();
			else if (pBot)
				pBot->UserInOut(INOUT_OUT);
			else
				return;

			sOperatorCommandType = "OPERATOR_CUTOFF";
		}
		break;
	}

	if(sNoticeMessage.length())g_pMain->SendNoticeWindAll(sNoticeMessage, 0xFFFFFF00);
}

#pragma endregion 

#pragma region CUser::HandleHelpCommand
COMMAND_HANDLER(CUser::HandleHelpCommand)
{
	if (!isGM())
		return false;

	foreach(itr, s_commandTable)
	{
		if (itr->second == nullptr)
			continue;

		auto i = itr->second;
		std::string s_Command = string_format("Command: +%s, Description: %s", i->Name, i->Help);
		g_pMain->SendHelpDescription(this, s_Command);
	}
	return true;
}
#pragma endregion

#pragma region COMMAND_HANDLER(CUser::HandleMuteCommand) 
COMMAND_HANDLER(CUser::HandleMuteCommand) 
{
	if (!isGM())
		return false;

	if (m_GameMastersMute != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}
		

	// Char name
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +mute CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE || !string_is_valid(strUserID)) {
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +mute CharacterName");
		return true;
	}

	uint32 period = 0;
	if (!vargs.empty()) {
		int raw_period = SafeAtoi(vargs.front(), 0, 1095);
		vargs.pop_front();
		if (raw_period < 0) { g_pMain->SendHelpDescription(this, "day error!"); return true; }
		period = (uint32)raw_period;
	}
	if (period && period > 1095) { g_pMain->SendHelpDescription(this, "day error!"); return true; }

	g_pMain->UserAuthorityUpdate(BanTypes::MUTE, this, strUserID,"", period);
	return true;
}
#pragma endregion

#pragma region CUser::HandleUnMuteCommand
COMMAND_HANDLER(CUser::HandleUnMuteCommand) 
{
	if (!isGM())
		return false;

	if (m_GameMastersUnMute != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +unmute CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE || !string_is_valid(strUserID))
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +unmute CharacterName");
		return true;
	}

	uint32 period = 0;
	if (!vargs.empty()) {
		int raw_period = SafeAtoi(vargs.front(), 0, 1095);
		vargs.pop_front();
		if (raw_period < 0) { g_pMain->SendHelpDescription(this, "day error!"); return true; }
		period = (uint32)raw_period;
	}
	if (period && period > 1095) { g_pMain->SendHelpDescription(this, "day error!"); return true; }

	g_pMain->UserAuthorityUpdate(BanTypes::UNMUTE, this, strUserID,"", period);
	return true;
}
#pragma endregion

#pragma region CUser::HandleFishingDropTester
COMMAND_HANDLER(CUser::HandleFishingDropTester)
{
	if (!isGM())
		return true;

	// Char name
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +fishingtest Type Hour");
		return true;
	}

	int8 WeaponType = -1;
	if (!vargs.empty()) { WeaponType = SafeAtoi(vargs.front(), -1, 127); vargs.pop_front(); }
	if (WeaponType != 0 && WeaponType != 1 && WeaponType != 5) {
		g_pMain->SendHelpDescription(this, "Invalid Weapon Type");
		return true;
	}

	uint8 Hour = 0;
	if (!vargs.empty()) { Hour = SafeAtoi(vargs.front(), 0, 24); vargs.pop_front(); }
	if (Hour > 24 || !Hour) {
		g_pMain->SendHelpDescription(this, "Invalid Hour");
		return true;
	}

	FishingDropTester(WeaponType, Hour);

	return true;
}
#pragma endregion

#pragma region CUser::HandleMiningDropTester
COMMAND_HANDLER(CUser::HandleMiningDropTester)
{
	if (!isGM())
		return true;

	// Char name
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +miningtest WeaponType Hour");
		return true;
	}

	uint8 WeaponType = 0;
	if (!vargs.empty()) { WeaponType = SafeAtoi(vargs.front(), 0, 255); vargs.pop_front(); }
	if (WeaponType != 0 && WeaponType != 1 && WeaponType != 4) {
		g_pMain->SendHelpDescription(this, "Invalid Weapon Type");
		return true;
	}

	uint8 Hour = 0;
	if (!vargs.empty()) { Hour = SafeAtoi(vargs.front(), 0, 24); vargs.pop_front(); }
	if (Hour > 24 || !Hour) {
		g_pMain->SendHelpDescription(this, "Invalid Hour");
		return true;
	}

	MiningDropTester(WeaponType, Hour);
	return true;
}

// DropTest Düzeltildi.
#pragma region CUser::HandleReloadTable
COMMAND_HANDLER(CUser::HandleReloadTable)
{
	if (!isGM())
		return true;
	// Char name
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +reload_table Table Name");
		return true;
	}
	std::string strTable = vargs.front();
	vargs.pop_front();

	if (strTable.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this,  "Using Sample : +reload_table Table Name");
		return true;
	}
	bool isResult = false;

	if (strTable == "ITEM")
	{
		g_pMain->m_ItemtableArray.DeleteAllData();
		g_pMain->LoadItemTable();
		isResult = true;
	}
	else if (strTable == "JACKPOT_SETTINGS")
	{
		for (int i = 0; i < 2; i++)
			g_pMain->pJackPot[i].Initialize();
		g_pMain->LoadJackPotSettingTable();
		isResult = true;
	}else if (strTable == "RIGHT_CLICK_EXCHANGE")
	{
		g_pMain->m_LoadRightClickExchange.DeleteAllData();
		g_pMain->m_RightClickItemListLock.lock();
		g_pMain->RightClickExchangeItemList.clear();
		g_pMain->m_RightClickItemListLock.unlock();
		g_pMain->LoadItemRightClickExchangeTable();
		if (g_pMain->RightClickExchangeItemList.size())
		{
			Packet pRightClickExchange(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
			pRightClickExchange << uint8(RightClickExchangeSubcode::SendItemExchangeList) << uint16(g_pMain->RightClickExchangeItemList.size());
			foreach(itr, g_pMain->RightClickExchangeItemList)
				pRightClickExchange << uint32(*itr);

			g_pMain->Send_All(&pRightClickExchange);
		}
		isResult = true;
	}
	else if (strTable == "MAGIC")
	{
		g_pMain->m_IsMagicTableInUpdateProcess = true;
		g_pMain->m_MagictableArray.DeleteAllData();
		g_pMain->m_Magictype1Array.DeleteAllData();
		g_pMain->m_Magictype2Array.DeleteAllData();
		g_pMain->m_Magictype3Array.DeleteAllData();
		g_pMain->m_Magictype4Array.DeleteAllData();
		g_pMain->m_Magictype5Array.DeleteAllData();
		g_pMain->m_Magictype6Array.DeleteAllData();
		g_pMain->m_Magictype8Array.DeleteAllData();
		g_pMain->m_Magictype9Array.DeleteAllData();
		g_pMain->LoadMagicTable();
		g_pMain->LoadMagicType1();
		g_pMain->LoadMagicType2();
		g_pMain->LoadMagicType3();
		g_pMain->LoadMagicType4();
		g_pMain->LoadMagicType5();
		g_pMain->LoadMagicType6();
		g_pMain->LoadMagicType7();
		g_pMain->LoadMagicType8();
		g_pMain->LoadMagicType9();
		g_pMain->m_IsMagicTableInUpdateProcess = false;
		isResult = true;
	}else
		if (strTable == "COLLECTION_RACE_EVENT_SETTINGS")
		{
			g_pMain->m_CollectionRaceListArray.DeleteAllData();
			g_pMain->LoadCollectionRaceEventTable();
			isResult = true;
		}
		else
			if (strTable == "ITEM_EXCHANGE")
			{
				g_pMain->m_ItemExchangeArray.DeleteAllData();
				g_pMain->LoadItemExchangeTable();
				isResult = true;
			}
			else if (strTable == "ITEM_RANDOM")
				{
					g_pMain->m_RandomItemArray.clear();
					g_pMain->LoadGiftEventArray();
					isResult = true;
				}
			else if (strTable == "ITEM_SHOZIN")
			{
				g_pMain->m_ItemSpecialExchangeArray.DeleteAllData();
				g_pMain->LoadItemSpecialExchangeTable();
				isResult = true;
			}
			else if (strTable == "ITEM_SMASH")
			{
				g_pMain->m_ItemExchangeCrashArray.DeleteAllData();
				g_pMain->LoadItemExchangeCrashTable();
				isResult = true;
			}else if (strTable == "K_SPECIAL_STONE")
			{
				g_DBAgent.SpecialStone();
				isResult = true;
			}else if (strTable == "MONSTER_BOSS_RANDOM_SPAWN")
			{
				g_pMain->m_MonsterBossRandom.DeleteAllData();
				g_pMain->LoadRandomBossTable();
				isResult = true;
			}
			else if (strTable == "NEW_UPGRADE")
			{
				g_DBAgent.LoadUpgrade();
				isResult = true;
			}else if (strTable == "ITEM_UPGRADE_SETTINGS")
			{
				g_DBAgent.LoadItemUpgradeSettings();
				isResult = true;
			}else if (strTable == "ZONE_KILL_REWARD")
			{
				g_pMain->m_ZoneKillReward.clear();
				g_pMain->LoadZoneKillReward();
				isResult = true;
			}
			else if (strTable == "MONSTER_BOSS_RANDOM_STAGES")
			{
				g_pMain->m_MonsterBossStage.DeleteAllData();
				g_pMain->LoadRandomBoosStageTable();
				isResult = true;
			}else if (strTable == "DAILY_QUESTS")
			{
				g_pMain->m_DailyQuestArray.DeleteAllData();
				g_pMain->LoadDailyQuestListTable();
				isResult = true;
			}else if (strTable == "PUS_ITEMS")
			{
				g_pMain->m_PusItemArray.DeleteAllData();
				g_pMain->LoadPusItemsTable();
				isResult = true;
			}else if (strTable == "PUS_CATEGORY")
			{
				g_pMain->m_PusCategoryArray.DeleteAllData();
				g_pMain->LoadPusCategoryTable();
				isResult = true;
			}
			else if (strTable == "LOTTERY_EVENT_SETTINGS")
			{
				g_pMain->m_RimaLotteryArray.DeleteAllData();
				g_pMain->LoadRimaLotteryEventTable();
				isResult = true;
			}
			else if (strTable == "SERVER_SETTINGS") {
				g_pMain->LoadServerSettingsData();
				isResult = true;
			}
			else if (strTable == "MONSTER_RESPAWNLOOP_LIST") {
				g_pMain->m_MonsterRespawnLoopArray.DeleteAllData();
				g_pMain->LoadMonsterRespawnLoopListTable();
				isResult = true;
			}
			else if (strTable == "DAMAGE_SETTINGS") {
				g_pMain->LoadDamageSettingTable();
				isResult = true;
			}
			else if (strTable == "START_POSITION_RANDOM") {
				g_pMain->m_StartPositionRandomArray.DeleteAllData();
				g_pMain->LoadStartPositionRandomTable();
				isResult = true;
			}
			else if (strTable == "SET_ITEM")
			{
				g_pMain->m_SetItemArray.DeleteAllData();
				g_pMain->LoadSetItemTable();
				isResult = true;
			}
			else if (strTable == "SET_ITEM")
			{
				g_pMain->m_SetItemArray.DeleteAllData();
				g_pMain->LoadSetItemTable();
				isResult = true;
			}else if (strTable == "WHEEL_SETTINGS")
			{
				g_pMain->m_ItemWheelArray.DeleteAllData();
				g_pMain->LoadWheelDataSetTable();
				isResult = true;
			}
			else if (strTable == "EVENT_BEEF_PLAY_TIMER")
			{
				g_pMain->m_BeefEventPlayTimerArray.DeleteAllData();
				g_pMain->LoadBeefEventPlayTimerTable();
				isResult = true;
			}else if (strTable == "EVENT_REWARD")
			{
				g_pMain->GetEventAwardsIni();
				isResult = true;
			}
			else if (strTable == "EVENT_REWARDS")
			{
				g_pMain->m_EventRewardArray.DeleteAllData();
				g_pMain->LoadEventRewardTable();
				isResult = true;
			}
			else if (strTable == "KNIGHTS_CSW_OPT")
			{
				g_pMain->pCswEvent.poptions.Initialize();
				g_pMain->LoadCSWTables();
				isResult = true;
			}
			else if (strTable == "USER_BOTS")
			{
				g_pMain->m_BotArray.m_lock.lock();
				foreach_stlmap_nolock(itr, g_pMain->m_BotArray)
				{
					CBot* pBot = itr->second;
					if (pBot == nullptr)
						continue;

					if (!pBot->isInGame())
						continue;

					pBot->UserInOut(INOUT_OUT);
				}
				g_pMain->m_BotArray.m_lock.unlock();

				g_pMain->m_BotcharacterNameLock.lock();
				g_pMain->m_BotcharacterNameMap.clear();
				g_pMain->m_BotcharacterNameLock.unlock();
				g_pMain->m_BotArray.DeleteAllData();
				g_DBAgent.LoadBotTable();
				isResult = true;
			}

			if (isResult)
				g_pMain->SendHelpDescription(this, string_format("Reload Table [%s].", strTable.c_str()));
			else
				g_pMain->SendHelpDescription(this, string_format("Invalid table name."));

	return true;
}
// DropTest Düzeltildi.
#pragma region CUser::HandleNpcDropTester
COMMAND_HANDLER(CUser::HandleNpcDropTester)
{
	if (!isGM())
		return true;

	// Char name
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +droptest and Count");
		return true;
	}

	uint16 Count = SafeAtoi(vargs.front(), 0, 65535);
	vargs.pop_front();

	CNpc* pNpc = g_pMain->GetNpcPtr(GetTargetID(), GetZoneID());
	if (pNpc == nullptr)
		return false;

	if (Count > 9999)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Count max value (9999).");
		return true;
	}

	pNpc->DropTesterHaveItem(this, Count);
	//printf("FoundName = %s - FoundGetID = %d - FoundProtoID = %d - FoundZoneID = %d - Count = %d\n", FoundNpc->GetName().c_str(), FoundNpc->GetID(), FoundNpc->GetProtoID(), FoundNpc->GetZoneID(), Count);
	return true;
}
#pragma endregion
// DropTest Düzeltildi.

#pragma region CUser::HandleAllowAttackCommand
COMMAND_HANDLER(CUser::HandleAllowAttackCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersAllowAttack != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +allow CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE || !string_is_valid(strUserID))
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +allow CharacterName");
		return true;
	}

	uint32 period = 0;
	if (!vargs.empty()) {
		int raw_period = SafeAtoi(vargs.front(), 0, 1095);
		vargs.pop_front();
		if (raw_period < 0) { g_pMain->SendHelpDescription(this, "day error!"); return true; }
		period = (uint32)raw_period;
	}
	if (period && period > 1095) { g_pMain->SendHelpDescription(this, "day error!"); return true; }

	g_pMain->UserAuthorityUpdate(BanTypes::ALLOW_ATTACK, this, strUserID,"",period);
	return true;
}
#pragma endregion

#pragma region CUser::HandleDisableCommand
COMMAND_HANDLER(CUser::HandleDisableCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersDisabledAttack != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +disable CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE || !string_is_valid(strUserID))
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +disable CharacterName");
		return true;
	}

	uint32 period = 0;
	if (!vargs.empty()) {
		int raw_period = SafeAtoi(vargs.front(), 0, 1095);
		vargs.pop_front();
		if (raw_period < 0) { g_pMain->SendHelpDescription(this, "day error!"); return true; }
		period = (uint32)raw_period;
	}
	if (period && period > 1095) { g_pMain->SendHelpDescription(this, "day error!"); return true; }

	g_pMain->UserAuthorityUpdate(BanTypes::DIS_ATTACK, this, strUserID,"", period);
	return true;
}
#pragma endregion

void CUser::Clean_expnoahdropnp_notice(uint8 id) {
	Packet result(WIZ_NOTICE);
	result.DByte();
	result << uint8(4) << uint8(2);
	if (id == 1) result << "Exp Event";
	else if (id == 2) result << "Noah Event";
	else if (id == 3) result << "Np Event";
	else if (id == 4) result << "Drop Event";
	Send(&result);
}

#pragma region CUser::HandleExpAddCommand
COMMAND_HANDLER(CUser::HandleExpAddCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersExpAdd != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Expects the bonus XP percent, e.g. '+exp_add' for a +15 XP boost.
	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +exp_add Percent");
		return true;
	}

	g_pMain->m_byExpEventAmount = (uint16) SafeAtoi(vargs.front(), 0, 1000); // CHI-20: limit 255→1000
	
	bool clean = false;
	if (g_pMain->m_byExpEventAmount == 0) clean = true;

	for (uint16 i = 0; i < MAX_USER; i++) {
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;

		if (clean)
			pUser->Clean_expnoahdropnp_notice(1);
		else 
			pUser->SendNotice();
	}

	if (clean)
		return true;

	g_pMain->SendFormattedResource(IDS_EXP_REPAY_EVENT, (uint8)Nation::ALL, false, (uint16)g_pMain->m_byExpEventAmount);
	return true;
}
#pragma endregion

#pragma region CUser::HandleMoneyAddCommand
// Starts/stops the server coin event & sets its server-wide bonus.
COMMAND_HANDLER(CUser::HandleMoneyAddCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersMoneyAdd != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Expects the bonus coin percent, e.g. '+money_add' for a +15 dropped coin boost.
	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +money_add Percent");
		return true;
	}

	g_pMain->m_byCoinEventAmount = (uint8) SafeAtoi(vargs.front(), 0, 255);

	bool clean = false;
	if (g_pMain->m_byCoinEventAmount == 0) clean = true;

	for (uint16 i = 0; i < MAX_USER; i++) {
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;

		if (clean)
			pUser->Clean_expnoahdropnp_notice(2);
		else
			pUser->SendNotice();
	}

	if (clean)
		return true;

	g_pMain->SendFormattedResource(IDS_MONEY_REPAY_EVENT, (uint8)Nation::ALL, false, g_pMain->m_byCoinEventAmount);

	return true;
}
#pragma endregion

#pragma region CUser::HandleNPAddCommand
COMMAND_HANDLER(CUser::HandleNPAddCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersNpAdd != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Expects the bonus np percent, e.g. '+np_add' for a +15 np boost.
	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +np_add Percent");
		return true;
	}

	g_pMain->m_byNPEventAmount = (uint8)SafeAtoi(vargs.front(), 0, 255);

	bool clean = false;
	if (g_pMain->m_byNPEventAmount == 0) clean = true;

	for (uint16 i = 0; i < MAX_USER; i++) {
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;

		if (clean)
			pUser->Clean_expnoahdropnp_notice(3);
		else
			pUser->SendNotice();
	}

	if (clean)
		return true;

	g_pMain->SendFormattedResource(IDS_NP_REPAY_EVENT, (uint8)Nation::ALL, false, g_pMain->m_byNPEventAmount);
	return true;
}
#pragma endregion

#pragma region CUser::HandleDropAddCommand
// Starts/stops the server coin event & sets its server-wide bonus.
COMMAND_HANDLER(CUser::HandleDropAddCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersDropAdd != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Expects the bonus coin percent, e.g. '+money_add' for a +15 dropped coin boost.
	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +drop_add Percent");
		return true;
	}

	g_pMain->m_byDropEventAmount = (uint8)SafeAtoi(vargs.front(), 0, 255);

	bool clean = false;
	if (g_pMain->m_byDropEventAmount == 0) clean = true;

	for (uint16 i = 0; i < MAX_USER; i++) {
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;

		if (clean)
			pUser->Clean_expnoahdropnp_notice(4);
		else
			pUser->SendNotice();
	}

	if (clean)
		return true;

	g_pMain->SendFormattedResource(IDS_DROP_REPAY_EVENT, (uint8)Nation::ALL, false, g_pMain->m_byDropEventAmount);
	return true;
}
#pragma endregion

#pragma region CUser::HandleLoyaltyChangeCommand
COMMAND_HANDLER(CUser::HandleLoyaltyChangeCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersLoyaltyChange != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | loyalty
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +np_change CharacterName Loyalty(+/-)");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE || !string_is_valid(strUserID))
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +np_change CharacterName Loyalty(+/-)");
		return true;
	}

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint32 nLoyalty = SafeAtoi(vargs.front(), 0, LOYALTY_MAX);

	if (nLoyalty != 0)
		pUser->SendLoyaltyChange("Loyalty Command", nLoyalty, false);

	return true;
}
#pragma endregion

#pragma region CUser::HandleExpChangeCommand
COMMAND_HANDLER(CUser::HandleExpChangeCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersExpChange != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | exp
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +exp_change CharacterName Exp(+/-)");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	int64 nExp = SafeAtoi(vargs.front(), 0, 2000000000);

	if (nExp != 0)
		pUser->ExpChange("gm command",nExp, true);

	return true;
}

#pragma endregion

#pragma region CUser::HandleGoldChangeCommand
COMMAND_HANDLER(CUser::HandleGoldChangeCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersMoneyChange != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | coins
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +noah CharacterName Gold(+/-)");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	int32 nGold = SafeAtoi(vargs.front(), 0, COIN_MAX);

	if (nGold != 0)
	{
		if (nGold > 0)
		{
			pUser->GoldGain(nGold, true);
			g_pMain->SendHelpDescription(this, "User has received coins.");
		}
		else
		{
			pUser->GoldLose(-nGold, true);
			g_pMain->SendHelpDescription(this, "Coins was taken from the user.");
		}
	}

	return true;
}
#pragma endregion

#pragma region CUser::HandleOnlineZoneGiveItemCommand
COMMAND_HANDLER(CUser::HandleOnlineZoneGiveItemCommand)
{
	if (!isGM())
		return false;

	// item ID | [stack size]
	if (vargs.size() < 4)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +online_zone_give_item ZoneID ItemID Count Time");
		return true;
	}

	uint8 sZoneID = 21;
	if (!vargs.empty())
		sZoneID = SafeAtoi(vargs.front(), 0, 255);

	uint32 nItemID = SafeAtoi(vargs.front(), 0, 999999999);
	vargs.pop_front();
	_ITEM_TABLE pData = g_pMain->GetItemPtr(nItemID);
	if (pData.isnull())
	{
		// send error message saying the item does not exist
		g_pMain->SendHelpDescription(this, "Error : Item does not exist");
		return true;
	}

	uint16 sCount = 1;
	if (!vargs.empty())
		sCount = SafeAtoi(vargs.front(), 1, MAX_ITEM_COUNT);
	vargs.pop_front();

	uint32 sExpirationTime = 0;
	if (!vargs.empty())
		sExpirationTime = SafeAtoi(vargs.front(), 0, INT_MAX);

	Packet newpkt(WIZ_DB_SAVE, uint8(ProcDbServerType::GmCommandGiveLetterItem));
	newpkt << nItemID << sZoneID << sCount << sExpirationTime;
	g_pMain->AddDatabaseRequest(newpkt);

	return true;
}
#pragma endregion


#pragma region CUser::HandleOnlineGiveItemCommand
COMMAND_HANDLER(CUser::HandleOnlineGiveItemCommand)
{
	if (!isGM())
		return false;

	// item ID | [stack size]
	if (vargs.size() < 3)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +online_give_item ItemID Count Time");
		return true;
	}

	uint32 nItemID = SafeAtoi(vargs.front(), 0, 999999999);
	vargs.pop_front();

	_ITEM_TABLE pData = g_pMain->GetItemPtr(nItemID);
	if (pData.isnull())
	{
		// send error message saying the item does not exist
		g_pMain->SendHelpDescription(this, "Error : Item does not exist");
		return true;
	}

	uint16 sCount = 1;
	if (!vargs.empty())
		sCount = SafeAtoi(vargs.front(), 1, MAX_ITEM_COUNT);

	if (!sCount)
		return true;

	vargs.pop_front();

	uint32 sExpirationTime = 0;
	if (!vargs.empty())
		sExpirationTime = SafeAtoi(vargs.front(), 0, INT_MAX);

	Packet newpkt(WIZ_DB_SAVE, uint8(ProcDbServerType::GmCommandGiveLetterItem));
	newpkt << nItemID << uint8(0) << sCount << sExpirationTime;
	g_pMain->AddDatabaseRequest(newpkt);

	return true;
}
#pragma endregion

#pragma region CUser::HandleGiveItemCommand
COMMAND_HANDLER(CUser::HandleGiveItemCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersGiveItem != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | item ID | [stack size]
	if (vargs.size() < 4)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +give CharacterName ItemID Count Time");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint32 nItemID = SafeAtoi(vargs.front(), 0, 999999999);
	vargs.pop_front();
	_ITEM_TABLE pItem = g_pMain->GetItemPtr(nItemID);
	if (pItem.isnull())
	{
		// send error message saying the item does not exist
		g_pMain->SendHelpDescription(this, "Error : Item does not exist");
		return true;
	}

	uint16 sCount = 1;
	if (!vargs.empty())
		sCount = SafeAtoi(vargs.front(), 1, MAX_ITEM_COUNT);
	vargs.pop_front();

	uint16 sExpirationTime = 0;
	if (!vargs.empty())
		sExpirationTime = SafeAtoi(vargs.front(), 0, 65535);

	if (!pUser->GiveItem("Give Item Command", nItemID, sCount, true, sExpirationTime))
		g_pMain->SendHelpDescription(this, "Error : Item could not be added");
	else
		g_pMain->SendHelpDescription(this, string_format("Item added successfully to %s!", pUser->GetName().c_str()).c_str());
	return true;
}
#pragma endregion

#pragma region CUser::HandleNationChangeCommand
COMMAND_HANDLER(CUser::HandleNationChangeCommand)
{
	if (!isGM())
		return false;


	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Using Sample : +nation_change CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE || !string_is_valid(strUserID)) {
		g_pMain->SendHelpDescription(this, "Using Sample : +nation_change CharacterName");
		return true;
	}

	g_pMain->SendHelpDescription(this, "Plase waiting in Process!");
	Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::NtsCommand));
	newpkt << strUserID;
	g_pMain->AddDatabaseRequest(newpkt, this);
	return true;
}
#pragma endregion

#pragma region CUser::HandleGiveItemSelfCommand
COMMAND_HANDLER(CUser::HandleGiveItemSelfCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersGiveItemSelf != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | item ID | [stack size]
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +item itemID stackSize");
		return true;
	}

	uint32 nItemID = SafeAtoi(vargs.front(), 0, 999999999);
	vargs.pop_front();
	_ITEM_TABLE pItem = g_pMain->GetItemPtr(nItemID);
	if (pItem.isnull())
	{
		// send error message saying the item does not exist
		g_pMain->SendHelpDescription(this, "Error : Item does not exist");
		return true;
	}

	uint16 sCount = 1;
	if (!vargs.empty())
		sCount = SafeAtoi(vargs.front(), 1, MAX_ITEM_COUNT);

	if (!GiveItem("Give Item Self Command", nItemID, sCount))
		g_pMain->SendHelpDescription(this, "Error : Item could not be added");

	return true;
}
#pragma endregion

#pragma region CUser::HandleSummonUserCommand
COMMAND_HANDLER(CUser::HandleSummonUserCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersSummonUser != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | item ID | [stack size]
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : /summonuser CharacterName ");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	pUser->ZoneChange(GetZoneID(), GetX(), GetZ());
	return true;
}
#pragma endregion

#pragma region CUser::HandleTpOnUserCommand
COMMAND_HANDLER(CUser::HandleTpOnUserCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersTpOnUser != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | item ID | [stack size]
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +tpon CharacterName ");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	ZoneChange(pUser->GetZoneID(), pUser->GetX(), pUser->GetZ());
	return true;
}

#pragma endregion

#pragma region CUser::HandleZoneChangeCommand
COMMAND_HANDLER(CUser::HandleZoneChangeCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersZoneChange != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +zone ZoneNumber");
		return true;
	}

	// Behave as in official (we'll fix this later)
	int nZoneID = SafeAtoi(vargs.front(), 0, 255);

	_START_POSITION * pStartPosition = g_pMain->GetStartPosition(nZoneID);
	if (pStartPosition == nullptr) 
		return false;
	
	ZoneChange(nZoneID, 
		(float)(GetNation() == (uint8)Nation::KARUS ? pStartPosition->sKarusX : pStartPosition->sElmoradX + myrand(0, pStartPosition->bRangeX)),
		(float)(GetNation() == (uint8)Nation::KARUS ? pStartPosition->sKarusZ : pStartPosition->sElmoradZ + myrand(0, pStartPosition->bRangeZ)));

	return true;
}
#pragma endregion

#pragma region CUser::HandleLocationChange
COMMAND_HANDLER(CUser::HandleLocationChange)
{	
	if (!isGM())
		return false;

	if (m_GameMastersLocationChange != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | item ID | [stack size]
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : /goto X Y - Note: X and Y cordinate ");
		return true;
	}

	uint32 X = SafeAtoi(vargs.front(), 0, INT_MAX);
	vargs.pop_front();

	uint32 Y = SafeAtoi(vargs.front(), 0, INT_MAX);
	vargs.pop_front();

	if (X < 0 || Y < 0)
	{
		g_pMain->SendHelpDescription(this, "Error : Invalid coordinate.");
		return true;
	}

	ZoneChange(GetZoneID(), (float)X, (float)Y);
	return true;
}
#pragma endregion

#pragma region CUser::HandleMonsterSummonCommand
COMMAND_HANDLER(CUser::HandleMonsterSummonCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersMonsterSummon != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +mon MonsterSID Count");
		return true;
	}

	int sSid = 0;
	uint16 sCount = 1;

	if (vargs.size() == 1)
		sSid = SafeAtoi(vargs.front());

	if (vargs.size() == 2)
	{
		sSid = SafeAtoi(vargs.front());
		vargs.pop_front();
		sCount = SafeAtoi(vargs.front(), 1, MAX_ITEM_COUNT);
	}
	
	g_pMain->SpawnEventNpc(sSid, true, GetZoneID(), GetX(), GetY(), GetZ(), sCount);
	// Boss Atınca Notice Gecmesi Engellendi start
	g_pMain->IsSummon = true;
	g_pMain->NameGm = GetName().c_str();
	// Boss Atınca Notice Gecmesi Engellendi end
	return true;
}
#pragma endregion

#pragma region CUser::HandleNPCSummonCommand
COMMAND_HANDLER(CUser::HandleNPCSummonCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersNpcSummon != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	if (vargs.empty())
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +npc NPCSID");
		return true;
	}

	int sSid = SafeAtoi(vargs.front());
	g_pMain->SpawnEventNpc(sSid, false, GetZoneID(), GetX(), GetY(), GetZ());

	return true;
}

#pragma endregion

#pragma region CUser::HandleMonKillCommand
COMMAND_HANDLER(CUser::HandleMonKillCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersMonKilled != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}
	
	if (GetTargetID() == 0 && GetTargetID() < NPC_BAND)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : Select a NPC or Monster than use +kill");
		return false;
	}
	
	CNpc *pNpc = g_pMain->GetNpcPtr(GetTargetID(), GetZoneID());
	
	if (pNpc != nullptr)
		g_pMain->KillNpc(GetTargetID(), GetZoneID(), this);
	
	return true;
}

#pragma endregion

#pragma region CUser::HandleTeleportAllCommand
COMMAND_HANDLER(CUser::HandleTeleportAllCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersTeleportAllUser != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Zone number
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +tp_all ZoneNumber | +tp_all ZoneNumber TargetZoneNumber");
		return true;
	}

	int nZoneID = 0;
	int nTargetZoneID = 0;

	if (vargs.size() == 1)
		nZoneID = SafeAtoi(vargs.front(), 0, 255);

	if (vargs.size() == 2)
	{
		nZoneID = SafeAtoi(vargs.front(), 0, 255);
		vargs.pop_front();
		nTargetZoneID = SafeAtoi(vargs.front(), 0, 255);
	}

	if (nZoneID > 0 || nTargetZoneID > 0)
		g_pMain->KickOutZoneUsers(nZoneID,nTargetZoneID);
	return true;
}
#pragma endregion

#pragma region CUser::HandleKnightsSummonCommand
COMMAND_HANDLER(CUser::HandleKnightsSummonCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersClanSummon != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Clan name
	if(vargs.empty())
	{
		// Send description
		g_pMain->SendHelpDescription(this, "Using Sample : +summonknights ClanName");
		return true;
	}

	std::string strClanName = "";
	if (!vargs.empty()) { strClanName = vargs.front(); vargs.pop_front(); }
	if (strClanName.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +summonknights ClanName ");
		return true;
	}

	CKnights* pKnights = nullptr;

	g_pMain->m_KnightsArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_KnightsArray) {
		if (itr->second && itr->second->GetName() == strClanName) {
			pKnights = itr->second;
			break;
		}
	}
	g_pMain->m_KnightsArray.m_lock.unlock();

	if (pKnights == nullptr)
		return true;

	std::vector<std::string> willTP;
	pKnights->m_arKnightsUser.m_lock.lock();
	foreach_stlmap_nolock(i, pKnights->m_arKnightsUser) {
		auto* p = i->second;
		if (p)willTP.push_back(p->strUserName);
	}
	pKnights->m_arKnightsUser.m_lock.unlock();

	foreach(itr, willTP) {
		CUser* pTUser = g_pMain->GetUserPtr((*itr), NameType::TYPE_CHARACTER);
		if (pTUser == nullptr || !pTUser->isInGame() || pTUser->GetName() == GetName())
			continue;

		pTUser->ZoneChange(GetZoneID(), m_curx, m_curz);
		std::string helpstring = string_format("[%s] %s is teleported.", pKnights->GetName().c_str(), pTUser->GetName().c_str());
		g_pMain->SendHelpDescription(this, helpstring);
	}

	return true;
}
#pragma endregion

#pragma region CUser::HandleResetPlayerRankingCommand
COMMAND_HANDLER(CUser::HandleResetPlayerRankingCommand)
{
	if (!isGM())
		return false;

	if (m_GameMastersResetPlayerRanking != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	if (vargs.empty())
	{
		// Send description
		g_pMain->SendHelpDescription(this, "Using Sample : +resetranking");
		return true;
	}

	g_pMain->ResetPlayerRankings();

	return true;
}
#pragma endregion

#pragma region CUser::HandleChangeRoom
COMMAND_HANDLER(CUser::HandleChangeRoom)
{
	if (!isGM())
		return false;

	if (m_GameMastersChangeEventRoom != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Room number
	if (vargs.size() < 3)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +changeroom username ZoneId RoomNumber");
		return true;
	}

	uint16 ZoneID;
	uint16 Room;

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);

	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	ZoneID = SafeAtoi(vargs.front(), 0, 255);
	vargs.pop_front();
	Room = SafeAtoi(vargs.front(), 0, 255);
	vargs.pop_front();

	if (ZoneID < 0
		|| Room < 0
		|| ZoneID > 255
		|| Room > 255)
	{
		g_pMain->SendHelpDescription(this, "Using Sample : Invalid Zone Or Invalid Room");
		return true;
	}

	if (ZoneID != 84 
		&& ZoneID != 85 
		&& ZoneID != 87)
	{
		g_pMain->SendHelpDescription(this, "Using Sample : Invalid Zone");
		return true;
	}

	bool RoomNumber = (Room >= 1 
		&& Room <= 60);

	if (!RoomNumber)
	{
		g_pMain->SendHelpDescription(this, "Using Sample : Invalid Room");
		return true;
	}

	pUser->m_bEventRoom = Room;
	pUser->ZoneChange(ZoneID, 0.0f, 0.0f);
	return true;
}
#pragma endregion

COMMAND_HANDLER(CUser::HandleChaosExpansionOpen)
{
	if (!isGM()) return false;
	if (!m_GameMasterChaosEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleChaosExpansionOpen(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleBorderDefenceWarOpen)
{
	if (!isGM()) return false;
	if (!m_GameMasterBdwEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleBorderDefenceWar(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleJuraidMountainOpen)
{
	if (!isGM()) return false;
	if (!m_GameMasterJrEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleJuraidMountain(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleBorderDefenceWarClosed)
{
	if (!isGM()) return false;
	if (!m_GameMasterRoyalEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleBorderDefenceWarClose(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleJuraidMountainClosed)
{
	if (!isGM()) return false;
	if (!m_GameMasterRoyalEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleJuraidMountainClose(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadClanPremiumTable)
{
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadClanPremiumTable(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleBeefEventOpen)
{
	if (!isGM()) return false;
	if (!m_GameMasterBeefEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleBeefEvent(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleInventoryClear)
{
	if (!isGM())
		return false;

	// Char name
	if (vargs.size() < 1)
	{
		InventoryClear(this); // InventoryClear Düzeltildi.
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : for GM +clear or for user +clear CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Nickname error");
		return true;
	}

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !pUser->isInGame())
	{
		// send description
		g_pMain->SendHelpDescription(this, "User is not online");
		return true;
	}

	InventoryClear(pUser);
	return true;
}

COMMAND_HANDLER(CUser::HandleLotteryStart)
{
	if (!isGM())
		return false;

	// Char name | item ID | [stack size]
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +lottery ID on Database ");
		return true;
	}

	uint32 ID = SafeAtoi(vargs.front(), 0, INT_MAX);

	_RIMA_LOTTERY_DB *pLottery = g_pMain->m_RimaLotteryArray.GetData(ID);
	if (pLottery == nullptr)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Lottery System Not Found ID");
		return true;
	}

	g_pMain->LotterySystemStart(ID);

	return true;
}

void CGameServerDlg::LotteryClose(CUser* pUser /*=nullptr*/)
{
	if (!pLotteryProc.LotteryStart)
	{
		if (pUser)
			SendHelpDescription(pUser, "Lottery Zeten Kapali");
		else
			printf("Lottery Zaten Kapali\n");
		return;
	}

	std::vector< _RIMA_LOOTERY_USER_INFO> mList;
	m_RimaLotteryUserInfo.m_lock.lock();
	foreach_stlmap_nolock(itr, m_RimaLotteryUserInfo)
		if (itr->second)
			mList.push_back(*itr->second);
	m_RimaLotteryUserInfo.m_lock.unlock();

	bool cont = true;
	if (mList.empty())
		cont = false;

	if (cont)
	{
		foreach(itr, mList)
		{
			if (!itr->TicketCount)
				continue;

			CUser* pUser = GetUserPtr(itr->Name, NameType::TYPE_CHARACTER);
			if (!pUser || !pUser->isInGame())
				continue;

			uint32 nReqGold = 0;

			for (int x = 0; x < static_cast<int>(itr->TicketCount); x++) {
				for (int i = 0; i < 5; i++) {
					uint32 ItemID = g_pMain->pLotteryProc.nReqItem[i];
					uint32 ItemCount = g_pMain->pLotteryProc.nReqItemCount[i];
					if (!ItemID || !ItemCount) continue;
					if (ItemID == ITEM_GOLD)
						nReqGold += ItemCount;
					else
						pUser->GiveItem("Lottery give back", ItemID, ItemCount, true);
				}
			}

			if (nReqGold)
				pUser->GoldGain(nReqGold, true, false);
		}
	}

	LotterySystemReset();
	if (pUser)
		SendHelpDescription(pUser, "Lottery Kapatildi");
	else
		printf("Lottery Kapatildi\n");
}

COMMAND_HANDLER(CUser::HandleLotteryClose)
{
	if (!isGM())
		return false;

	g_pMain->LotteryClose(this);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleLotteryClose)
{
	LotteryClose();
	return true;
}

#pragma region CUser::HandleSummonPrison
COMMAND_HANDLER(CUser::HandleSummonPrison)
{
	if (!isGM())
		return false;

	if (m_GameMastersSendPrsion != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | item ID | [stack size]
	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +Hapis CharacterNick ");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	pUser->ZoneChange(ZONE_PRISON, 170, 146);
	std::string sNoticeMessage;
	sNoticeMessage = string_format("%s Currently send to prison", pUser->GetName().c_str()); //02.10.2020 Hapise Gonderince Notice Gecmesi
	g_pMain->SendNotice(sNoticeMessage.c_str(), (uint8)Nation::ALL);
	return true;
}
COMMAND_HANDLER(CUser::HandlePrivateAllCommand) 
{
	if (!isGM())
		return false;

	// string & atoi size
	if (vargs.size() < 2)
	{
		// Send Game Server Description
		printf("Using Sample : +pmall Title Message \n");
		return true;
	}

	Packet result;
	std::string sTitle = vargs.front();
	vargs.pop_front();

	uint16 vargsize = (uint16)vargs.size();

	if (vargsize >= (1 + 50))
	{
		// send description
		g_pMain->SendHelpDescription(this, "Error : long word!");
		return true;
	}

	std::string word[50];
	std::string guncelword = "";
	for (int x = 1; x <= vargsize; x++)
	{
		word[x] = vargs.front();
		vargs.pop_front();

		if (!word[x].empty())
		{
			if (word[x].size() > 75)
			{
				// send description
				g_pMain->SendHelpDescription(this, "Error : long word!");
				return true;
			}
		}
		else
		{
			word[x] = "";
		}
		guncelword += word[x] + ' ';
	}
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		ChatPacket::Construct(&result, (uint8)ChatType::PRIVATE_CHAT, guncelword.c_str(), sTitle.c_str(), pUser->GetNation(), -1, -1, 21);
		pUser->Send(&result);
	}

	return true;
}
COMMAND_HANDLER(CUser::HandleAnindaGM) //aninda gm ol cýk
{
	if (!isGM() && !isGMUser())
		return false;

	if (vargs.size() < 1)
	{
		if (m_bResHpType == USER_STANDING)
		{
			m_bAuthority = GetAuthority() == (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER ? (uint8)AuthorityTypes::AUTHORITY_GM_USER : (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER;
			if (isGM())
				g_pMain->SendHelpDescription(this, "You are a gamemaster.");
			else if (isGMUser())
				g_pMain->SendHelpDescription(this, "You are a user.");

			if (GetHealth() < (GetMaxHealth() / 2))
				HpChange(GetMaxHealth());

			bool bBecomingGM = (GetAuthority() == (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER);

			// GM moduna gecince diger clientlara kaybolma paketi gonder (donukluk onlenir)
			// User moduna gecince once visible yap, sonra ABNORMAL_NORMAL set et
			if (bBecomingGM)
			{
				// Once bolgedeki herkese kaybol paketi gonder
				UserInOut(INOUT_OUT);
				m_bAbnormalType = ABNORMAL_INVISIBLE;
			}
			else
			{
				// Once invisible state'i temizle, sonra bolgeye gir
				m_bAbnormalType = ABNORMAL_NORMAL;
			}

			SendMyInfo();

			if (!bBecomingGM)
				UserInOut(INOUT_OUT);

			RegisterRegion();
			SetRegion(GetNewRegionX(), GetNewRegionZ());
			UserInOut(INOUT_WARP);

			g_pMain->UserInOutForMe(this);
			NpcInOutForMe();
			g_pMain->MerchantUserInOutForMe(this);

			ResetWindows();

			InitType4();
			RecastSavedMagic();
			ZoneChange(GetZoneID(), uint16(GetX()), uint16(GetZ()));
		}

		if (m_bResHpType == USER_SITDOWN)
			g_pMain->SendHelpDescription(this, "Kodu kullanabilmek icin ayaga kalkiniz");
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleJobChangeGM)
{
	if (!isGM())
		return false;

	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +jobchange nickname 1/2/3/4 ");
		g_pMain->SendHelpDescription(this, "jobchange nickname 1 Warrior");
		g_pMain->SendHelpDescription(this, "jobchange nickname 2 Rogue");
		g_pMain->SendHelpDescription(this, "jobchange nickname 3 Mage");
		g_pMain->SendHelpDescription(this, "jobchange nickname 4 Priest");
		return true;
	}

	std::string strUserID = "";
	if (!vargs.empty()) {strUserID = vargs.front(); vargs.pop_front();}
	if (strUserID.empty() || strUserID.length() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Error : nickname");
		return true;
	}

	uint8 NewJob = SafeAtoi(vargs.front(), 1, 4);
	bool check = NewJob >= 1 && NewJob <= 4;
	if (!check) {
		g_pMain->SendHelpDescription(this, "Error : job id");
		return true;
	}

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !isInGame()) {
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	pUser->JobChangeGM(NewJob);
	return true;
}

COMMAND_HANDLER(CUser::HandleGenderChangeGM)
{
	if (!isGM())
		return false;

	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +gender nickname 1/2/3");
		g_pMain->SendHelpDescription(this, "gender nickname 1 Male");
		g_pMain->SendHelpDescription(this, "gender nickname 2 Female");
		g_pMain->SendHelpDescription(this, "gender nickname 3 Barbarian");
		return true;
	}

	std::string strUserID = "";
	if (!vargs.empty()) { strUserID = vargs.front(); vargs.pop_front(); }
	if (strUserID.empty() || strUserID.length() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Error : nickname");
		return true;
	}

	uint8 Race = 0;
	if (!vargs.empty()) { Race = SafeAtoi(vargs.front(), 1, 3); vargs.pop_front(); }
	bool check = Race >= 1 && Race <= 3;
	if (!check) {
		g_pMain->SendHelpDescription(this, "Error : race id");
		return true;
	}

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !isInGame()) {
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	if (pUser->GetNation() == (uint8)Nation::KARUS)
	{
		if (pUser->isBeginnerWarrior() || pUser->isNoviceWarrior() || pUser->isMasteredWarrior())
		{
			g_pMain->SendHelpDescription(this, "In Karus Warrior Gender Change is invalid");
		}
		else if (pUser->isBeginnerRogue() || pUser->isNoviceRogue() || pUser->isMasteredRogue())
		{
			g_pMain->SendHelpDescription(this, "In Karus Rogue Gender Change is invalid");
		}
		else if (pUser->isBeginnerMage() || pUser->isNoviceMage() || pUser->isMasteredMage())
		{
			if (Race == 1)
			{
				pUser->GenderChangeGM(3);
			}
			else if (Race == 2)
			{
				pUser->GenderChangeGM(4);
			}
			else
			{
				g_pMain->SendHelpDescription(this, string_format("Invalid Mage Race Type %d", Race));
			}
		}
		else if (pUser->isBeginnerPriest() || pUser->isNovicePriest() || pUser->isMasteredPriest())
		{
			if (Race == 1)
			{
				pUser->GenderChangeGM(2);
			}
			else if (Race == 2)
			{
				pUser->GenderChangeGM(4);
			}
			else
			{
				g_pMain->SendHelpDescription(this, string_format("Invalid Priest Race Type %d", Race));
			}
		}
		else
		{
			g_pMain->SendHelpDescription(this, "Invalid job to Use Gender Change");
		}
	}
	else
	{
		/*duzenleme*/
		if (pUser->isBeginnerWarrior() || pUser->isNoviceWarrior() || pUser->isMasteredWarrior())
		{
			if (Race == 1)
			{
				pUser->GenderChangeGM(12);
			}
			else if (Race == 2)
			{
				pUser->GenderChangeGM(13);
			}
			else if (Race == 3)
			{
				pUser->GenderChangeGM(11);
			}
			else
			{
				g_pMain->SendHelpDescription(this, string_format("Invalid Warrior Race Type %d", Race));
			}
		}
		else if (pUser->isBeginnerRogue() || pUser->isNoviceRogue() || pUser->isMasteredRogue())
		{
			if (Race == 1)
			{
				pUser->GenderChangeGM(12);
			}
			else if (Race == 2)
			{
				pUser->GenderChangeGM(13);
			}
			else
			{
				g_pMain->SendHelpDescription(this, string_format("Invalid Rogue Race Type %d", Race));
			}
		}
		else if (pUser->isBeginnerMage() || pUser->isNoviceMage() || pUser->isMasteredMage())
		{
			if (Race == 1)
			{
				pUser->GenderChangeGM(12);
			}
			else if (Race == 2)
			{
				pUser->GenderChangeGM(13);
			}
			else
			{
				g_pMain->SendHelpDescription(this, string_format("Invalid Mage Race Type %d", Race));
			}
		}
		else if (pUser->isBeginnerPriest() || pUser->isNovicePriest() || pUser->isMasteredPriest())
		{
			if (Race == 1)
			{
				pUser->GenderChangeGM(12);
			}
			else if (Race == 2)
			{
				pUser->GenderChangeGM(13);
			}
			else
			{
				g_pMain->SendHelpDescription(this, string_format("Invalid Priest Race Type %d", Race));
			}
		}
		else
		{
			g_pMain->SendHelpDescription(this, "Invalid job to Use Gender Change");
		}
		/*duzenleme*/
	}

	return true;
}

#pragma region CUser::HandleKcChangeCommand
COMMAND_HANDLER(CUser::HandleKcChangeCommand) // gm koduyla kc yukleme veya eksiltme
{
	if (!isGM())
		return false;

	if (m_GameMastersKcChange != 1) // Kc Verme Yetkisi 
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | KC
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +kc CharacterName KC(+/-)");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint32 KC = SafeAtoi(vargs.front(), 0, 999999999);

	if (KC <= 0)
		return true;

	if (KC != 0)
	{
		if (KC > 0)
		{
			pUser->GiveBalance(KC);
			g_pMain->SendHelpDescription(this, "User has received KC.");
		}

		KC += pUser->m_nKnightCash;

		Packet result(XSafe);
		result << uint8(XSafeOpCodes::KCUPDATE) << uint32(KC) <<  pUser->m_nTLBalance;
		pUser->Send(&result);
	}

	return true;
}
#pragma endregion

#pragma region CUser::HandleKcChangeCommand
COMMAND_HANDLER(CUser::HandleTLBalanceCommand) //16.09.2020 gm koduyla kc yukleme veya eksiltme
{
	if (!isGM())
		return false;

	if (m_GameMastersKcChange != 1) // Kc Verme Yetkisi 27.09.2020
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	// Char name | KC
	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +tl CharacterName KC(+/-)");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint32 KC = SafeAtoi(vargs.front(), 0, 999999999);

	if (KC <= 0)
		return true;

	if (KC != 0)
	{
		if (KC > 0)
		{
		
			g_pMain->SendHelpDescription(this, "User has received TL.");
		}

		pUser->GiveBalance(0, KC, GetSocketID());

		/*pUser->m_nTLBalance += KC;
		g_DBAgent.UpdateKnightCash(pUser);
		Packet result(XSafe);
		result << uint8(XSafeOpCodes::KCUPDATE) << uint32(pUser->m_nKnightCash) << pUser->m_nTLBalance;
		pUser->Send(&result);*/
	}

	return true;
}
#pragma endregion
COMMAND_HANDLER(CUser::HandleChaosExpansionClosed)
{
	if (!isGM()) return false;
	if (!m_GameMasterRoyalEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleBeefEventClose(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleBeefEventClose)
{
	if (!isGM()) return false;
	if (!m_GameMasterBeefEvent) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return g_pMain->HandleBeefEventClose(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadAllTabCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadAllTabCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadNoticeCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadNoticeCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadTablesCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadTablesCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadTables2Command) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadTables2Command(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadTables3Command) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadTables3Command(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadMagicsCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadMagicsCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadQuestCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadQuestCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadRanksCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadRanksCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadDropsCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadDropsCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadDropsRandomCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadDropsRandomCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadKingsCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadKingsCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadRightTopTitleCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadRightTopTitleCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadPusItemCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadPusItemCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadItemsCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadItemsCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadDungeonDefenceTables) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadDungeonDefenceTables(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadDrakiTowerTables) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadDrakiTowerTables(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleEventScheduleResetTable) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleEventScheduleResetTable(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleTopLeftCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleTopLeftCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadBonusNotice) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadBonusNotice(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadItems) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadItems(vargs, args, description);
}

#pragma region CUser::HandleOpenMaster
COMMAND_HANDLER(CUser::HandleOpenMaster)
{
	if (!isGM()) 
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +open_master CharacterName");
		return true;
	}

	std::string strUserID = "";
	if (!vargs.empty()) { strUserID = vargs.front(); vargs.pop_front(); }
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Using Sample : +open_master CharacterName");
		return true;
	}

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !pUser->isInGame()) {
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	if (pUser->isMastered()) {
		g_pMain->SendHelpDescription(this, "Error : Already master!");
		return true;
	}
	pUser->PromoteUser();
	return true;
}
#pragma endregion

#pragma region CUser::HandleOpenSkill
COMMAND_HANDLER(CUser::HandleOpenSkill)
{
	if (!isGM()) 
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +open_skill CharacterName");
		return true;
	}

	std::string strUserID = "";
	if (!vargs.empty()) { strUserID = vargs.front(); vargs.pop_front(); }
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Using Sample : +open_skill CharacterName");
		return true;
	}

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !pUser->isInGame()) {
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	if (pUser->isNovice()) {
		g_pMain->SendHelpDescription(this, "Error : Already open skill!");
		return true;
	}
	pUser->PromoteUserNovice();
	return true;
}
#pragma endregion

#pragma region CUser::HandleOpenQuestSkill
COMMAND_HANDLER(CUser::HandleOpenQuestSkill)
{
	if (!isGM()) 
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +open_questskill CharacterName");
		return true;
	}

	std::string strUserID = "";
	if (!vargs.empty()) { strUserID = vargs.front(); vargs.pop_front(); }
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Using Sample : +open_questskill CharacterName");
		return true;
	}

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !pUser->isInGame()) {
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	if (pUser->isNovice()) {
		g_pMain->SendHelpDescription(this, "Error : Already open skill!");
		return true;
	}

	std::vector<uint32> metclist[5], metclist2;
	metclist[0] = { 334,359,365, 273 };
	metclist[1] = { 335 ,347 ,360 ,366, 273 };
	metclist[2] = { 336 ,348 ,361 ,367, 273 };
	metclist[3] = { 337 ,349 ,357 ,362,363,364,368, 273 };
	metclist[4] = { 1377 ,1378, 273 };

	if (pUser->isWarrior()) {
		foreach(itr, metclist[0])
			metclist2.push_back(*itr);
	}
	else if (pUser->isRogue()) {
		foreach(itr, metclist[1])
			metclist2.push_back(*itr);
	}
	else if (pUser->isMage()) {
		foreach(itr, metclist[2])
			metclist2.push_back(*itr);
	}
	else if (pUser->isPriest()) {
		foreach(itr, metclist[3])
			metclist2.push_back(*itr);
	}
	else if (pUser->isPortuKurian()) {
		foreach(itr, metclist[4])
			metclist2.push_back(*itr);
	}
	foreach(itr, metclist2)
		pUser->SaveEvent(*itr, 2);

	return true;
}
#pragma endregion

COMMAND_HANDLER(CUser::HandleLoadMerchant)
{
	if (!isGM() && !isGMUser())
		return false;

	Packet newpkt(WIZ_DB_SAVE, uint8(ProcDbServerType::BotSaveLoad));
	newpkt << uint8(2) << GetZoneID() << GetSocketID();
	g_pMain->AddDatabaseRequest(newpkt);
	return true;
}
COMMAND_HANDLER(CUser::HandleSaveMerchant)
{
	if (!isGM() && !isGMUser())
		return false;

	Packet newpkt(WIZ_DB_SAVE, uint8(ProcDbServerType::BotSaveLoad));
	newpkt << uint8(1) << GetZoneID() << GetSocketID();
	g_pMain->AddDatabaseRequest(newpkt);
	return true;
}
COMMAND_HANDLER(CUser::HandleAIResetCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleAIResetCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadUpgradeCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadUpgradeCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadRankBugCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadRankBugCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadLevelRewardCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadLevelRewardCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadMerchantLevelRewardCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadMerchantLevelRewardCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadBotInfoCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadBotInfoCommand(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleReloadZoneOnlineRewardCommand) {
	if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Gerekli Yetki Yok. Lütfen Yetki Sınırlandırması için Yönetici ile konuşunuz."); return false; }
	return !isGM() ? false : g_pMain->HandleReloadZoneOnlineRewardCommand(vargs, args, description);
}

#pragma region CUser::HandleCountZoneCommand
COMMAND_HANDLER(CUser::HandleCountZoneCommand)
{
	if (!isGM())
		return false;


	uint16 sZoneID = GetZoneID();
	uint16 count = 0, elmoradcount = 0, karuscount = 0;

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;
		if (pUser->GetZoneID() != sZoneID) continue;
		if (pUser->GetNation() == (uint8)Nation::KARUS)karuscount++;
		else elmoradcount++;
		count++;
	}

	Packet test(WIZ_CHAT, uint8(ChatType::GM_INFO_CHAT));
	test << uint8(1) << sZoneID << count << karuscount << elmoradcount;
	Send(&test);
	return true;
}
#pragma endregion

#pragma region CUser::HandleCountLevelCommand
COMMAND_HANDLER(CUser::HandleCountLevelCommand)
{
	if (!isGM())
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +countlevel Lwl giriniz.");
		return true;
	}

	uint8 Level = 0;
	if (!vargs.empty()) {
		Level = SafeAtoi(vargs.front(), 1, 83);
		vargs.pop_front();
	}


	if (Level < 1 || Level > 83) {
		g_pMain->SendHelpDescription(this, "Error : Invalid Level");
		return true;
	}

	uint16 count = 0;
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;
		if (pUser->GetLevel() != Level) continue;
		count++;
	}


	Packet test(WIZ_CHAT, uint8(ChatType::GM_INFO_CHAT));
	test << uint8(2) << (uint16)Level << count;
	Send(&test);
	return true;
}
#pragma endregion

#pragma region CUser::HandleBowlEvent
COMMAND_HANDLER(CUser::HandleBowlEvent)
{
	if(!isGM())
		return true;
	if (vargs.size() < 2)
	{

		return true;
	}
	uint8 sZone;
	sZone = SafeAtoi(vargs.front(), 0, 255);
	vargs.pop_front();
	uint16 sTime;
	sTime = SafeAtoi(vargs.front(), 0, 65535);
	vargs.pop_front();
	if (g_pMain->isBowlEventActive && sTime == 0)
	{
		std::string zonename = "None";
		auto* pzone = g_pMain->m_ZoneArray.GetData(sZone);
		if (pzone) zonename = pzone->m_nZoneName;
		string helpstring = string_format("Bowl Event %s Bölgesinde sona erdi.", zonename.c_str());
		g_pMain->CloseBowlEvent();
		g_pMain->SendAnnouncement(helpstring.c_str());
		g_pMain->SendNotice(helpstring.c_str());
		return true;
	}
	g_pMain->isBowlEventActive = true;
	g_pMain->tBowlEventTime = sTime;
	g_pMain->tBowlEventZone = sZone;
	std::string zonename = "None";
	auto* pzone = g_pMain->m_ZoneArray.GetData(sZone);
	if (pzone) zonename = pzone->m_nZoneName;
	string helpstring = string_format("Bowl Event %s Bölgesinde Başladı. Event Süresi %d Dakikadır.", zonename.c_str(), uint16(sTime / 60));
	g_pMain->SendAnnouncement(helpstring.c_str());
	g_pMain->SendNotice(helpstring.c_str());
	return true;
}
#pragma region CUser::HandleGiveGenieTime
COMMAND_HANDLER(CUser::HandleGiveGenieTime)
{
	if (!isGM())
		return false;


	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +givegenietime UserID Time");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint32 KC = SafeAtoi(vargs.front(), 0, 999999999);

	int remtime = int(pUser->m_1098GenieTime > UNIXTIME ? pUser->m_1098GenieTime - UNIXTIME : 0);

	pUser->m_1098GenieTime = UNIXTIME + (KC * HOUR) + (remtime > 0 ? remtime : 0);

	Packet result(WIZ_GENIE, uint8(GenieUseSpiringPotion));
	result << uint8(GenieUseSpiringPotion) << GetGenieTime();
	pUser->Send(&result);
	g_pMain->SendHelpDescription(this, "Give Genie succesfully");
	return true;
}

COMMAND_HANDLER(CUser::HandleCindirellaWarOpen)
{
	if (!isGM())
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +madclas settingid");
		return true;
	}

	int8 settingid = -1;
	if (!vargs.empty()) { settingid = SafeAtoi(vargs.front(), -1, 5); vargs.pop_front(); }

	if (settingid < 0 || settingid > 5) {
		g_pMain->SendHelpDescription(this, "invalid settingid");
		return true;
	}

	return g_pMain->CindirellaCommand(true, settingid, this);
}

COMMAND_HANDLER(CUser::HandleReloadCindirellaCommand)
{
	if (!isGM()) return false;
	/*if (!pGMSetting.ReloadTable) {
		g_pMain->SendHelpDescription(this, GAMEMASTER_NOT);
		return true;
	}*/
	
	return g_pMain->HandleReloadCindirellaCommand(vargs, args, description);
}

#pragma region CUser::HandleSpecialEventOpenCommand
COMMAND_HANDLER(CUser::HandleSpecialEventOpenCommand)
{
	if (!isGM())
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Using Sample : +event status eventid");
		return true;
	}

	bool status = 0;
	if (!vargs.empty()) { status = SafeAtoi(vargs.front(), 0, 1); vargs.pop_front(); }
	if (status != 1 && status != 0) {
		g_pMain->SendHelpDescription(this, "invalid status id");
		return true;
	}

	if (!status) {
		g_pMain->SpecialEventProcess(this, g_pMain->pSpecialEvent.zoneid, status);
		return true;
	}

	uint8 eventid = 0;
	if (!vargs.empty()) { eventid = SafeAtoi(vargs.front(), 0, 255); vargs.pop_front(); }

	int zoneid = eventid + ZONE_SPBATTLE_BASE;
	if (!isInSpecialEventZone(zoneid)) {
		g_pMain->SendHelpDescription(this, "invalid event id");
		return true;
	}
	g_pMain->SpecialEventProcess(this, zoneid, status);
	return true;
}
#pragma endregion

void CGameServerDlg::SpecialEventProcess(CUser* pUser, uint8 zoneid, bool opened /*= false*/) {

	if (!pUser || !zoneid)
		return;

	if (pSpecialEvent.opened && opened)
		return g_pMain->SendHelpDescription(pUser, "already opened");
	else if (!pSpecialEvent.opened && !opened)
		return g_pMain->SendHelpDescription(pUser, "already closed");

	KickOutZoneUsers(ZONE_RONARK_LAND);
	KickOutZoneUsers(ZONE_ARDREAM);
	KickOutZoneUsers(ZONE_RONARK_LAND_BASE);
	KickOutZoneUsers(zoneid);
	pSpecialEvent.Initialize();

	std::string zone_name = "";
	auto* pzone = GetZoneByID(zoneid);
	if (pzone) 
		zone_name = pzone->m_nZoneName;

	if (opened) {
		if (zoneid == ZONE_SPBATTLE1) {
			ZindanWarStartTimeTickCount = (uint32)UNIXTIME;
			ZindanWarSummonCheck = false;
			ZindanLastSummonTime = 0;
			ZindanWarSummonStage = 0;
			ZindanWarisSummon = false;
			ZindanWarLastSummon = false;
			ZindanSpawnList.clear();
		}

		pSpecialEvent.opened = true;
		pSpecialEvent.starttime = UNIXTIME;
		pSpecialEvent.finishtime = pSpecialEvent.starttime + (60 * 60 * 1);
		pSpecialEvent.zoneid = zoneid;
		pSpecialEvent.eventname = zone_name;
	}
	else if (!opened && zoneid == ZONE_SPBATTLE1) {
		ZindanWarStartTimeTickCount = 0;
		ZindanWarSummonCheck = false;
		ZindanLastSummonTime = 0;
		ZindanWarSummonStage = 0;
		ZindanWarisSummon = false;
		ZindanWarLastSummon = false;
		ZindanSpawnList.clear();
	}

	std::string notice = string_format("%s event is %s", zone_name.empty() ? "-" : zone_name.c_str(), !opened ? "closed. Have Fun." : "opened. Entrances are from Moradon Gates. Have Fun.");
	if (!notice.empty()) {
		g_pMain->SendChat<ChatType::PUBLIC_CHAT>(notice.c_str(), (uint8)Nation::ALL, true);
		g_pMain->SendChat<ChatType::WAR_SYSTEM_CHAT>(notice.c_str(), (uint8)Nation::ALL, true);
	}
}

#pragma region CUser::HandleBugdanKurtarCommand
COMMAND_HANDLER(CUser::HandleBugdanKurtarCommand)
{
	if (!isGM())
		return false;

	return g_pMain->HandleBugdanKurtarCommand(vargs, args, description);
}
#pragma endregion

COMMAND_HANDLER(CGameServerDlg::HandleBugdanKurtarCommand)
{
	if (vargs.size() < 1) {
		printf("Using Sample : /bug strAccountID \n");
		return true;
	}

	std::string strAccountID = "";
	if (!vargs.empty()) { strAccountID = vargs.front(); vargs.pop_front(); }

	if (strAccountID.empty() || strAccountID.size() > MAX_ID_SIZE) {
		printf("Error: strAccountID\n");
		return true;
	}

	CUser* pUser = GetUserPtr(strAccountID, NameType::TYPE_ACCOUNT);
	if (pUser == nullptr) {
		printf("Error : User is not online\n");
		return true;
	}

	RemoveSessionNames(pUser);
	return true;
}

COMMAND_HANDLER(CUser::HandleForgettenTempleEvent)
{
	if (!isGM()) return false;
	return g_pMain->HandleForgettenTempleEvent(vargs, args, description);
}

COMMAND_HANDLER(CUser::HandleForgettenTempleEventClose)
{
	if (!isGM()) return false;
	return g_pMain->HandleForgettenTempleEventClose(vargs, args, description);
}

#pragma region CUser::HandleServerConfigCommand
COMMAND_HANDLER(CUser::HandleServerConfigCommand)
{
	if (!isGM())
		return false;

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Usage: +config key value");
		g_pMain->SendHelpDescription(this, "  +config list — show all");
		g_pMain->SendHelpDescription(this, "  +config SERVER.packet_rate_limit 200");
		return true;
	}

	std::string key = vargs.front();
	vargs.pop_front();

	if (key == "list") {
		std::string list = g_ServerConfig.GetValueList();
		g_pMain->SendHelpDescription(this, list);
		return true;
	}

	if (key == "reload") {
		g_ServerConfig.LoadFromINI("GameServer.ini");
		g_pMain->SendHelpDescription(this, "Config reloaded from GameServer.ini");
		LOG_GM("%s reloaded server config", GetName().c_str());
		return true;
	}

	if (vargs.empty()) {
		int val = g_ServerConfig.GetInt(key.c_str(), -1);
		g_pMain->SendHelpDescription(this, string_format("%s = %d", key.c_str(), val));
		return true;
	}

	std::string value = vargs.front();
	vargs.pop_front();

	g_ServerConfig.SetValue(key, value);
	g_pMain->SendHelpDescription(this, string_format("Config set: %s = %s", key.c_str(), value.c_str()));
	LOG_GM("%s set config %s=%s", GetName().c_str(), key.c_str(), value.c_str());
	return true;
}
#pragma endregion

#pragma region CUser::HandleShutdownGMCommand
COMMAND_HANDLER(CUser::HandleShutdownGMCommand)
{
	if (!isGM())
		return false;

	if (g_pMain->m_Shutdownstart) {
		g_pMain->SendHelpDescription(this, "Sunucu zaten kapaniyor!");
		return true;
	}

	uint32 minutes = 5; // varsayilan 5 dakika
	if (!vargs.empty()) {
		minutes = SafeAtoi(vargs.front(), 1, 60);
	}

	g_pMain->m_Shutdownfinishtime = UNIXTIME + (minutes * 60);
	g_pMain->m_Shutdownstart = true;
	g_pMain->m_ShutdownKickStart = false;
	g_pMain->m_Shutdownfinished = false;

	// Oyunculara hemen bildir (ust kayan yazi)
	Packet notice;
	std::string msg = string_format("#### DIKKAT: Sunucu %d dakika icinde kapanacak! ####", minutes);
	ChatPacket::Construct(&notice, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	g_pMain->Send_All(&notice, nullptr, (uint8)Nation::ALL);

	g_pMain->SendHelpDescription(this, string_format("Sunucu %d dakika sonra kapanacak.", minutes));
	printf("[Shutdown] GM %s triggered shutdown in %d minutes.\n", GetName().c_str(), minutes);
	LOG_GM("%s triggered server shutdown in %d minutes", GetName().c_str(), minutes);

	return true;
}
#pragma endregion

#pragma region CUser::HandleMaintenanceGMCommand
COMMAND_HANDLER(CUser::HandleMaintenanceGMCommand)
{
	if (!isGM())
		return false;

	if (g_pMain->m_bMaintenanceMode) {
		g_pMain->SendHelpDescription(this, "Bakim modu zaten aktif!");
		return true;
	}

	uint32 minutes = 10;
	if (!vargs.empty()) {
		minutes = SafeAtoi(vargs.front(), 1, 60);
	}

	g_pMain->m_bMaintenanceMode = true;
	g_pMain->m_MaintenanceKickTime = UNIXTIME + (minutes * 60);

	Packet notice;
	std::string msg = string_format("#### SUNUCU %d DAKIKA SONRA BAKIMA GIRECEKTIR! ####", minutes);
	ChatPacket::Construct(&notice, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	g_pMain->Send_All(&notice, nullptr, (uint8)Nation::ALL);

	g_pMain->SendHelpDescription(this, string_format("Bakim modu %d dakika sonra aktif olacak.", minutes));
	printf("[Maintenance] GM %s triggered maintenance in %d minutes.\n", GetName().c_str(), minutes);
	LOG_GM("%s triggered maintenance mode in %d minutes", GetName().c_str(), minutes);

	return true;
}
#pragma endregion

#pragma region CUser::HandleMaintenanceOffGMCommand
COMMAND_HANDLER(CUser::HandleMaintenanceOffGMCommand)
{
	if (!isGM())
		return false;

	if (!g_pMain->m_bMaintenanceMode) {
		g_pMain->SendHelpDescription(this, "Bakim modu zaten kapali.");
		return true;
	}

	g_pMain->m_bMaintenanceMode = false;
	g_pMain->m_MaintenanceKickTime = 0;

	Packet notice;
	std::string msg = "#### SUNUCU BAKIMI TAMAMLANDI — HERKES GIREBILIR ####";
	ChatPacket::Construct(&notice, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	g_pMain->Send_All(&notice, nullptr, (uint8)Nation::ALL);

	g_pMain->SendHelpDescription(this, "Bakim modu kapatildi. Herkes girebilir.");
	printf("[Maintenance] GM %s disabled maintenance mode.\n", GetName().c_str());
	LOG_GM("%s disabled maintenance mode", GetName().c_str());

	return true;
}
#pragma endregion

#pragma region Censor Commands
COMMAND_HANDLER(CUser::HandleCensorOnCommand)
{
	if (!isGM()) return false;
	g_pMain->m_bCensorEnabled = true;
	g_pMain->SendHelpDescription(this, string_format("Kufur filtresi ACIK. %d kelime aktif.", (int)g_pMain->m_CensorWords.size()));
	printf("[CENSOR] GM %s enabled censor.\n", GetName().c_str());
	return true;
}

COMMAND_HANDLER(CUser::HandleCensorOffCommand)
{
	if (!isGM()) return false;
	g_pMain->m_bCensorEnabled = false;
	g_pMain->SendHelpDescription(this, "Kufur filtresi KAPALI.");
	printf("[CENSOR] GM %s disabled censor.\n", GetName().c_str());
	return true;
}

COMMAND_HANDLER(CUser::HandleCensorAddCommand)
{
	if (!isGM()) return false;
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Kullanim: +censoradd kelime");
		return true;
	}
	std::string word = vargs.front();
	g_pMain->m_CensorWords.push_back(word);

	// Dosyaya kaydet
	FILE *fp = fopen("censor_words.txt", "a");
	if (fp) { fprintf(fp, "%s\n", word.c_str()); fclose(fp); }

	g_pMain->SendHelpDescription(this, string_format("'%s' kufur listesine eklendi. Toplam: %d", word.c_str(), (int)g_pMain->m_CensorWords.size()));
	printf("[CENSOR] GM %s added word: %s\n", GetName().c_str(), word.c_str());
	return true;
}

COMMAND_HANDLER(CUser::HandleCensorDelCommand)
{
	if (!isGM()) return false;
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "Kullanim: +censordel kelime");
		return true;
	}
	std::string word = vargs.front();
	auto &words = g_pMain->m_CensorWords;
	auto it = std::find(words.begin(), words.end(), word);
	if (it != words.end()) {
		words.erase(it);
		// Dosyayi yeniden yaz
		FILE *fp = fopen("censor_words.txt", "w");
		if (fp) { for (auto &w : words) fprintf(fp, "%s\n", w.c_str()); fclose(fp); }
		g_pMain->SendHelpDescription(this, string_format("'%s' listeden silindi. Toplam: %d", word.c_str(), (int)words.size()));
	} else {
		g_pMain->SendHelpDescription(this, string_format("'%s' listede bulunamadi.", word.c_str()));
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleCensorReloadCommand)
{
	if (!isGM()) return false;
	g_pMain->LoadCensorWords();
	g_pMain->SendHelpDescription(this, string_format("Kufur listesi yeniden yuklendi. %d kelime.", (int)g_pMain->m_CensorWords.size()));
	return true;
}

// Why: Kral H-paneli (client UI) Notice + Event ac/kapat butonlarini gostermiyor.
// Server tarafi (KingSystem.cpp) hazir, chat komutlariyla ayni packet'i uretip
// CKingSystem::PacketProcess'e besliyoruz. isKing() hem burada hem KingSystem'de kontrol edilir.
COMMAND_HANDLER(CUser::HandleKingExpCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }
	if (vargs.size() < 1) { g_pMain->SendHelpDescription(this, "Ornek: +kingexp 30  (10/30/50)"); return true; }
	uint8 amount = SafeAtoi(vargs.front(), 0, 50);
	if (amount != 10 && amount != 30 && amount != 50) { g_pMain->SendHelpDescription(this, "Sadece 10, 30 veya 50."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_EVENT));
	pkt << uint8(KING_EVENT_EXP) << amount;
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

COMMAND_HANDLER(CUser::HandleKingNoahCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }
	if (vargs.size() < 1) { g_pMain->SendHelpDescription(this, "Ornek: +kingnoah 2  (1/2/3)"); return true; }
	uint8 amount = SafeAtoi(vargs.front(), 0, 3);
	if (amount < 1 || amount > 3) { g_pMain->SendHelpDescription(this, "Sadece 1, 2 veya 3."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_EVENT));
	pkt << uint8(KING_EVENT_NOAH) << amount;
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

COMMAND_HANDLER(CUser::HandleKingNoticeCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }
	if (vargs.empty()) { g_pMain->SendHelpDescription(this, "Ornek: +kingnotice mesaj"); return true; }

	std::string strMessage;
	while (!vargs.empty()) { strMessage += vargs.front(); vargs.pop_front(); if (!vargs.empty()) strMessage += " "; }
	if (strMessage.empty() || strMessage.length() > 256) { g_pMain->SendHelpDescription(this, "Mesaj 1-256 karakter olmali."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_EVENT));
	pkt << uint8(KING_EVENT_NOTICE);
	pkt.SByte();
	pkt << strMessage;
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

COMMAND_HANDLER(CUser::HandleKingWeatherCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }
	if (vargs.size() < 2) { g_pMain->SendHelpDescription(this, "Ornek: +kingweather 2 50  (tip 1-3, siddet 1-100)"); return true; }
	uint8 bType = SafeAtoi(vargs.front(), 0, 255); vargs.pop_front();
	uint8 bAmount = SafeAtoi(vargs.front(), 0, 255); vargs.pop_front();
	if (bType < 1 || bType > WEATHER_SNOW) { g_pMain->SendHelpDescription(this, "Tip 1=Gunes 2=Yagmur 3=Kar."); return true; }
	if (bAmount < 1 || bAmount > 100) { g_pMain->SendHelpDescription(this, "Siddet 1-100."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_EVENT));
	pkt << uint8(KING_EVENT_WEATHER) << bType << bAmount;
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

COMMAND_HANDLER(CUser::HandleKingPrizeCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }
	if (vargs.size() < 2) { g_pMain->SendHelpDescription(this, "Ornek: +kingprize OyuncuAdi 1000000"); return true; }
	std::string strTarget = vargs.front(); vargs.pop_front();
	uint32 nCoins = SafeAtoi(vargs.front(), 0, COIN_MAX); vargs.pop_front();
	if (strTarget.empty() || strTarget.length() > MAX_ID_SIZE || nCoins == 0) { g_pMain->SendHelpDescription(this, "Gecersiz oyuncu veya miktar."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_EVENT));
	pkt << uint8(KING_EVENT_PRIZE);
	pkt.SByte();
	pkt << nCoins << strTarget;
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

COMMAND_HANDLER(CUser::HandleKingTaxCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }
	if (vargs.size() < 1) { g_pMain->SendHelpDescription(this, "Ornek: +kingtax 5  (0-10)"); return true; }
	uint8 byTariff = SafeAtoi(vargs.front(), 0, 10);
	if (byTariff > 10) { g_pMain->SendHelpDescription(this, "Vergi 0-10 arasi."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_TAX));
	pkt << uint8(4) << byTariff;
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

COMMAND_HANDLER(CUser::HandleKingFundCommand)
{
	if (!isKing()) { g_pMain->SendHelpDescription(this, "Sadece Kral kullanabilir."); return true; }

	Packet pkt(WIZ_KING, uint8(KING_TAX));
	pkt << uint8(2);
	CKingSystem::PacketProcess(this, pkt);
	return true;
}

// S115 Plan A — Oyun ici GM komutu: +tournamentstart (server-side CGameServerDlg::HandleTournamentStart forward)
COMMAND_HANDLER(CUser::HandleTournamentStartUserCommand)
{
	if (!isGM())
	{
		g_pMain->SendHelpDescription(this, "Sadece GM kullanabilir.");
		return true;
	}
	// Server-side handler'a forward (ayni mantik)
	g_pMain->HandleTournamentStart(vargs, args, description);
	return true;
}

// S115 Plan A — Oyun ici GM komutu: +tournamentclose
// Kullanim basitlestirildi: sadece ZoneID gerek (server-side eski 3 arg yerine 1 arg)
COMMAND_HANDLER(CUser::HandleTournamentCloseUserCommand)
{
	if (!isGM())
	{
		g_pMain->SendHelpDescription(this, "Sadece GM kullanabilir.");
		return true;
	}
	if (vargs.size() < 1)
	{
		g_pMain->SendHelpDescription(this, "Ornek: +tournamentclose 77");
		return true;
	}
	uint8 zoneID = SafeAtoi(vargs.front(), 0, 255);
	bool validZone = (zoneID == 77 || zoneID == 78 ||
	                  zoneID == 96 || zoneID == 97 ||
	                  zoneID == 98 || zoneID == 99);
	if (!validZone)
	{
		g_pMain->SendHelpDescription(this, "ZoneID gecersiz. 77/78/96-99 olmali");
		return true;
	}

	_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (info == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Bu zone'da aktif tournament yok");
		return true;
	}

	// S115 SAGLAMLIK FIX: Manuel close'da temizlik (bet iade + DB log FINISH)
	{
		extern void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID);
		ResolveTournamentBets(zoneID, 0); // iade
	}
	if (info->dbTournamentID > 0)
	{
		g_DBAgent.TournamentLogFinish(
			info->dbTournamentID,
			info->aTournamentScoreBoard[0],
			info->aTournamentScoreBoard[1],
			info->aTournamentMonumentKilled,
			0); // winnerClanID=0
	}

	g_pMain->KickOutZoneUsers(zoneID, ZONE_MORADON, (uint8)Nation::ALL);
	g_pMain->m_ClanVsDataList.DeleteData(zoneID);
	g_pMain->SendHelpDescription(this, "Tournament kapatildi (bet iade, DB log)");
	return true;
}

// =====================================================================
// S115 — Bracket Tournament komutlari (CGameServerDlg console + CUser oyun ici)
// MATRIX MSG:5914 SP'leri + BracketTournament.cpp engine
// =====================================================================

extern int32_t CreateBracket(const std::string& name, uint8 maxClans, const std::string& createdByGM);
extern bool    RegisterClanToBracket(int32_t bracketID, uint16 clanID,
                                     const std::string& clanName, const std::string& leaderName);
extern bool    StartBracket(int32_t bracketID);
extern bool    CancelBracket(int32_t bracketID);
extern void    GetBracketStatus(int32_t bracketID);

// /bracketcreate "AdSec" MaxKlan
COMMAND_HANDLER(CGameServerDlg::HandleBracketCreateCommand)
{
	if (vargs.size() < 2)
	{
		printf("Using: /bracketcreate \"AdSec\" MaxKlan (4/8/16/32)\n");
		return true;
	}

	std::string bracketName = vargs.front(); vargs.pop_front();
	uint8 maxClans = (uint8)SafeAtoi(vargs.front(), 4, 32);

	int32_t newID = CreateBracket(bracketName, maxClans, "console");
	if (newID > 0)
		printf("[BRACKET] Created: ID=%d Name='%s' MaxClans=%u\n",
			newID, bracketName.c_str(), maxClans);
	else
		printf("[BRACKET] Create FAILED (DB hata veya MATRIX SP yok)\n");
	return true;
}

// /bracketstart BracketID
COMMAND_HANDLER(CGameServerDlg::HandleBracketStartCommand)
{
	if (vargs.empty())
	{
		printf("Using: /bracketstart BracketID\n");
		return true;
	}

	int32_t bracketID = (int32_t)SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	if (StartBracket(bracketID))
		printf("[BRACKET] Started: ID=%d (Round 1 mac'lari DB'de)\n", bracketID);
	else
		printf("[BRACKET] Start FAILED ID=%d\n", bracketID);
	return true;
}

// /bracketstatus BracketID
COMMAND_HANDLER(CGameServerDlg::HandleBracketStatusCommand)
{
	if (vargs.empty())
	{
		printf("Using: /bracketstatus BracketID\n");
		return true;
	}

	int32_t bracketID = (int32_t)SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	GetBracketStatus(bracketID);
	return true;
}

// /bracketcancel BracketID
COMMAND_HANDLER(CGameServerDlg::HandleBracketCancelCommand)
{
	if (vargs.empty())
	{
		printf("Using: /bracketcancel BracketID\n");
		return true;
	}

	int32_t bracketID = (int32_t)SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	if (CancelBracket(bracketID))
		printf("[BRACKET] Cancelled: ID=%d\n", bracketID);
	else
		printf("[BRACKET] Cancel FAILED ID=%d\n", bracketID);
	return true;
}

// +bracketreg BracketID (oyun ici, klan lideri)
COMMAND_HANDLER(CUser::HandleBracketRegisterCommand)
{
	if (!isInClan())
	{
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin.");
		return true;
	}
	if (!isClanLeader())
	{
		g_pMain->SendHelpDescription(this, "Sadece klan lideri kayit yapabilir.");
		return true;
	}
	if (vargs.empty())
	{
		g_pMain->SendHelpDescription(this, "Ornek: +bracketreg 1 (BracketID)");
		return true;
	}

	int32_t bracketID = (int32_t)SafeAtoi(vargs.front(), 1, 0x7FFFFFFF);
	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Klan bulunamadi.");
		return true;
	}

	if (RegisterClanToBracket(bracketID, pClan->GetID(), pClan->GetName(), GetName()))
	{
		char buf[200] = { 0 };
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BRACKET] %s klani Bracket #%d'a kayit oldu!",
			pClan->GetName().c_str(), bracketID);
		std::string msg = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		g_pMain->Send_All(&pkt);
	}
	else
	{
		g_pMain->SendHelpDescription(this, "Kayit basarisiz (BracketID yanlis veya bracket dolu/kapali).");
	}
	return true;
}
#pragma endregion