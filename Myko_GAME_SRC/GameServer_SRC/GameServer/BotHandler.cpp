#include "stdafx.h"
#include "Map.h"
#include "../shared/DateTime.h"
#include "DBAgent.h"
#include <random>   
#include <chrono>   
#include <algorithm>

CBot::CBot()
{
	Initialize();
}

void CBot::Initialize()
{
	memset(m_arMerchantItems, 0, sizeof(m_arMerchantItems));
	m_merchantindex = 0;
	m_merchantareatype = 0;
	MerchantChat.clear();
	LastWarpTime = 0;
	m_tLastKillTime = 0;
	m_sChallangeAchieveID = 0;
	m_pktcount = 0;
	m_strAccountID.clear();
	m_strUserID.clear();
	m_strMemo.clear();
	nSkillTime = 0;
	for (int i = 0; i < (int)StatType::STAT_COUNT; i++)
		m_bRebStats[i] = 0;

	m_sAchieveCoverTitle = 0;

	m_bMerchantState = MERCHANT_STATE_NONE;
	m_sBind = -1;
	m_state = GameState::GAME_STATE_CONNECTED;
	m_bPartyLeader = false;
	m_bIsChicken = false;
	m_bIsHidingHelmet = false;
	m_bIsHidingCospre = false;

	m_bInParty = false;

	m_bInvisibilityType = (uint8)InvisibilityType::INVIS_NONE;

	m_sDirection = ReturnSymbolisOK = 0;

	memset(&m_bStats, 0, sizeof(m_bStats));
	memset(&m_bstrSkill, 0, sizeof(m_bstrSkill));
	m_for_bottime = 0;
	m_bAuthority = (uint8)AuthorityTypes::AUTHORITY_PLAYER;
	m_bLevel = 1;
	m_iExp = 0;
	m_iBank = m_iGold = 0;
	m_iLoyalty = 100;
	m_iLoyaltyMonthly = 0;
	m_iMannerPoint = 0;
	m_sHp = m_sMp = m_sSp = 0;

	memset(&m_arMerchantItems, 0, sizeof(m_arMerchantItems));
	m_bSellingMerchantPreparing = false;
	m_bBuyingMerchantPreparing = false;
	m_bPremiumMerchant = false;
	plookersocketid = m_sMerchantsSocketID = -1;

	m_MaxHP = 0;
	m_MaxMp = 1;
	m_MaxSp = 120;

	m_bResHpType = USER_STANDING;
	m_bBlockPrivateChat = false;
	m_sPrivateChatUser = 0;
	m_bNeedParty = 0x01;
	m_bAbnormalType = ABNORMAL_NORMAL;	// User starts out in normal size.
	m_nOldAbnormalType = m_bAbnormalType;
	m_teamColour = TeamColour::TeamColourNone;
	m_bGenieStatus = m_bIsDevil = m_bIsHidingWings = false;
	m_bRank = 0;
	m_Step = 0;
	m_Move_Time = 0;
	LoginBotType = 0;
	m_bRegeneType = 0;
	m_bResHpType = USER_STANDING;
	m_bPersonalRank = m_bKnightsRank = -1;
}
void CBot::GetMerchantSlipList(_MERCH_DATA list[MAX_MERCH_ITEMS], CBot* pownermerch) {
	if (!pownermerch) return;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++) {
		if (pownermerch->m_arMerchantItems[i].nNum) {
			for (int x = 0; x < MAX_MERCH_ITEMS; x++) {
				if (!list[x].nNum) { list[x] = pownermerch->m_arMerchantItems[i]; break; }
			}
		}
	}
}
void CBot::MerchantSlipRefList(CBot* pM, bool merchcrea) {
	if (!pM) return;

	if (!merchcrea) {
		Packet newpkt1(WIZ_MERCHANT_INOUT, uint8(1));
		newpkt1 << uint16(1) << pM->GetID() << pM->GetMerchantState() << (pM->GetMerchantState() == 1 ? false : pM->m_bPremiumMerchant);
		pM->SendToRegion(&newpkt1);
	}

	Packet newpkt2(WIZ_MERCHANT, uint8(MERCHANT_LIST));
	uint8 PremiumState = 0;
	newpkt2 << uint8(1) << pM->GetID() << pM->GetMerchantState() << PremiumState;
	_MERCH_DATA pnewlist[MAX_MERCH_ITEMS] = {};
	memset(pnewlist, 0, sizeof(pnewlist));
	GetMerchantSlipList(pnewlist, pM);
	for (int i = 0, x = PremiumState == 1 ? 8 : 4; i < x; i++) if (!pnewlist[i].IsSoldOut) newpkt2 << pnewlist[i].nNum;
	pM->SendToRegion(&newpkt2);

	for (int i = 0, x = PremiumState == 1 ? 8 : 4; i < x; i++) {
		if (!pnewlist[i].nNum) continue;
		Packet newpkt3(WIZ_MERCHANT, uint8(MERCHANT_ITEM_ADD));
		newpkt3 << uint16(1) << pnewlist[i].nNum << pnewlist[i].sCount << pnewlist[i].sDuration << pnewlist[i].nPrice << pnewlist[i].bOriginalSlot << uint8(i);
		pM->SendToRegion(&newpkt3);
	}
}
bool CBot::RegisterRegion()
{
	uint16
		new_region_x = GetNewRegionX(), new_region_z = GetNewRegionZ(),
		old_region_x = GetRegionX(), old_region_z = GetRegionZ();

	if (GetRegion() == nullptr
		|| (old_region_x == new_region_x
			&& old_region_z == new_region_z))
		return false;

	AddToRegion(new_region_x, new_region_z);

	RemoveRegion(old_region_x - new_region_x, old_region_z - new_region_z);
	InsertRegion(new_region_x - old_region_x, new_region_z - old_region_z);

	return true;
}

void CBot::AddToRegion(int16 new_region_x, int16 new_region_z)
{
	if (GetRegion())
		GetRegion()->Remove(this);

	SetRegion(new_region_x, new_region_z);

	if (GetRegion())
		GetRegion()->Add(this);
}

void CBot::RemoveRegion(int16 del_x, int16 del_z)
{
	if (GetMap() == nullptr)
		return;

	Packet result;
	GetInOut(result, INOUT_OUT);
	g_pMain->Send_OldRegions(&result, del_x, del_z, GetMap(), GetRegionX(), GetRegionZ(), nullptr, 0);
}

void CBot::InsertRegion(int16 insert_x, int16 insert_z)
{
	if (GetMap() == nullptr)
		return;

	Packet result;
	GetInOut(result, INOUT_IN);
	g_pMain->Send_NewRegions(&result, insert_x, insert_z, GetMap(), GetRegionX(), GetRegionZ(), nullptr, 0);
}

void CBot::SetRegion(uint16 x /*= -1*/, uint16 z /*= -1*/)
{
	m_sRegionX = x; m_sRegionZ = z;
	m_pRegion = m_pMap->GetRegion(x, z); // TODO: Clean this up
}

void CBot::StateChangeServerDirect(uint8 bType, uint32 nBuff)
{
	uint8 buff = *(uint8 *)&nBuff; // don't ask
	switch (bType)
	{
	case 1:
		m_bResHpType = buff;
		break;

	case 2:
		m_bNeedParty = buff;
		break;

	case 3:
		m_nOldAbnormalType = m_bAbnormalType;


		m_bAbnormalType = nBuff;
		break;

	case 5:
		m_bAbnormalType = nBuff;
		break;

	case 6:
		nBuff = m_bPartyLeader; // we don't set this here.
		break;

	case 7:
		break;

	case 8: // beginner quest
		break;

	case 14:
		break;
	}

	Packet result(WIZ_STATE_CHANGE);
	result << GetID() << bType << nBuff;
	SendToRegion(&result);
}

void CBot::GetInOut(Packet & result, uint8 bType)
{
	result.Initialize(WIZ_USER_INOUT);
	result << uint16(bType) << GetID();
	if (bType != INOUT_OUT)
		GetUserInfo(result);
}

void CBot::UserInOut(uint8 bType)
{
	Packet result;
	GetInOut(result, bType);

	if (bType == INOUT_OUT)
		Remove();
	else
		Add();

	if (bType == INOUT_OUT && m_merchantindex)
	{
		auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(m_merchantindex);
		if (pCoord) pCoord->used = false;
	}

	if (bType == INOUT_OUT)
	{
		memset(m_arMerchantItems, 0, sizeof(m_arMerchantItems));
	}
	
	SendToRegion(&result);
}

void CBot::Add()
{
	if (GetRegion())
		GetRegion()->Add(this);

	m_state = GameState::GAME_STATE_INGAME;
	
	Packet pkt(WIZ_DB_SAVE, uint8(ProcDbServerType::BotRegion));
	pkt << uint8(1) << GetName();
	g_pMain->AddDatabaseRequest(pkt);


	std::string upperName = GetName();
	STRTOUPPER(upperName);
	g_pMain->m_BotcharacterNameLock.lock();
	g_pMain->m_BotcharacterNameMap[upperName] = this;
	g_pMain->m_BotcharacterNameLock.unlock();
}

void CBot::Remove()
{
	if (GetRegion())
		GetRegion()->Remove(this);

	m_state = GameState::GAME_STATE_CONNECTED;

	Packet pkt(WIZ_DB_SAVE, uint8(ProcDbServerType::BotRegion));
	pkt << uint8(0) << GetName();
	g_pMain->AddDatabaseRequest(pkt);

	std::string upperName = GetName();
	STRTOUPPER(upperName);
	g_pMain->m_BotcharacterNameLock.lock();
	g_pMain->m_BotcharacterNameMap.erase(upperName);
	g_pMain->m_BotcharacterNameLock.unlock();
}

void CBot::SendToRegion(Packet *pkt)
{
	g_pMain->Send_Region(pkt, GetMap(), GetRegionX(), GetRegionZ(), nullptr, 0);
}

void CBot::GetUserInfo(Packet & pkt)
{
	pkt.SByte();
	pkt << GetName() << uint16(GetNation()) << uint8(0);
	pkt << GetClanID() << GetFame();

	CKnights * pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		if (isKing())
			pkt << uint32(0) << uint16(0) << uint8(0) << uint16(KingCapeType::KNIGHTS_KNIG_CAPE) << uint32(0) << uint8(0);
		else
			//pkt << uint32(0) << uint16(0) << uint8(0) << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0) << uint8(0);  // Botlarda Irk Kral Pelerinleri g�z�k�yor
			pkt << uint32(0) << uint16(0) << uint8(0) << uint16(-1) << uint32(0) << uint8(0);
	}
	else
	{
		pkt << pKnights->GetAllianceID() << pKnights->GetName() << pKnights->m_byGrade << pKnights->m_byRanking << pKnights->m_sMarkVersion; // symbol/mark version

		CKnights * pMainClan = g_pMain->GetClanPtr(pKnights->GetAllianceID());
		_KNIGHTS_ALLIANCE* pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());

		if (pKnights->isInAlliance() && pMainClan != nullptr && pAlliance != nullptr)
		{
			if (!isKing())
			{
				if (pMainClan->isCastellanCape()) {
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						pkt << pMainClan->m_castCapeID << pMainClan->m_bCastCapeR << pMainClan->m_bCastCapeG << pMainClan->m_bCastCapeB;
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						pkt << pMainClan->m_castCapeID << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB;
					else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
						pkt << pMainClan->m_castCapeID << uint32(0);
					else
						pkt << pMainClan->m_castCapeID << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB;
					pkt << uint8(0);
				}
				else {
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB;
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						pkt << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB;
					else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
						pkt << pMainClan->GetCapeID() << uint32(0);
					else
						pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB;
					pkt << uint8(0);
				}
			}
			else
				//pkt << uint16(KNIGHTS_KNIG_CAPE) << uint32(0); // cape ID //King White Cape 20.05.2020	
				pkt << uint16(GetNation() == (uint8)Nation::KARUS ? 99 : 99) << uint32(0); //King New Cape	20.05.2020	

			// not sure what this is, but it (just?) enables the clan symbol on the cape 
			// value in dump was 9, but everything tested seems to behave as equally well...
			// we'll probably have to implement logic to respect requirements.
			//pkt << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
			pkt << pKnights->m_byFlag;
		}
		else
		{
			if (!isKing())
			{
				if (pKnights->isCastellanCape())
					pkt << pKnights->m_castCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB;
				else
					pkt << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB;
				pkt << uint8(0);
			}
			else
				//pkt << uint16(KNIGHTS_KNIG_CAPE) << uint32(0); // cape ID //King White Cape 20.05.2020	
				pkt << uint16(GetNation() == (uint8)Nation::KARUS ? 99 : 99) << uint32(0); //King New Cape	19.06.2024		

			// not sure what this is, but it (just?) enables the clan symbol on the cape 
			// value in dump was 9, but everything tested seems to behave as equally well...
			// we'll probably have to implement logic to respect requirements.
			//pkt << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
			pkt << pKnights->m_byFlag;
		}
	}
	// There are two event-driven invisibility states; dispel on attack, and dispel on move.
	// These are handled primarily server-side; from memory the client only cares about value 1 (which we class as 'dispel on move').
	// As this is the only place where this flag is actually sent to the client, we'll just convert 'dispel on attack' 
	// back to 'dispel on move' as the client expects.
	uint8 bInvisibilityType = m_bInvisibilityType;
	if (bInvisibilityType != (uint8)InvisibilityType::INVIS_NONE)
		bInvisibilityType = (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE;

	pkt << GetLevel() << m_bRace << m_sClass
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< m_bFace << m_nHair
		<< m_bResHpType << uint32(m_bAbnormalType)
		<< m_bNeedParty
		<< m_bAuthority
		<< m_bPartyLeader
		<< bInvisibilityType
		<< uint8(m_teamColour)
		<< m_bIsHidingHelmet
		<< m_bIsHidingCospre
		<< m_sDirection
		<< m_bIsDevil
		<< m_bIsHidingWings
		<< m_bIsChicken
		<< m_bRank;

		if (GetLevel() < 30)
			pkt << int8(50) << int8(250);
		else
			pkt << (m_bKnightsRank <= m_bPersonalRank ? m_bKnightsRank : int8(-1)) << (m_bPersonalRank <= m_bKnightsRank ? m_bPersonalRank : int8(-1));

	uint8 equippedItems[] =
	{
		BREAST, LEG, HEAD, GLOVE, FOOT, SHOULDER, RIGHTHAND, LEFTHAND,
		CWING, CHELMET, CLEFT, CRIGHT, CTOP, CFAIRY, CTATTOO
	};


	bool isRoyaleSignEvent = (GetZoneID() == ZONE_KNIGHT_ROYALE);
	bool isWarOpen = (g_pMain->m_byBattleOpen == NATION_BATTLE && g_pMain->m_byBattleZone + ZONE_BATTLE_BASE != ZONE_BATTLE3);
	_ITEM_DATA *pItem = nullptr;


	foreach_array(i, equippedItems)
	{
		pItem = GetItem(equippedItems[i]);
		if (pItem == nullptr)
			continue;

		if (isWarOpen)
		{
			if (isWarrior())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isRogue())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isMage())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)MAGE_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)MAGE_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)MAGE_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)MAGE_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)MAGE_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isPriest())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isPortuKurian())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
		}
		else
		{
			pItem = GetItem(equippedItems[i]);
			if (pItem == nullptr)
				continue;

			pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
		}
	}
	
	pkt << GetZoneID()
		<< uint8(-1)
		<< uint8(-1)
		<< uint32(0)
		<< m_bIsHidingWings
		<< m_bIsHidingHelmet
		<< m_bIsHidingCospre
		<< isGenieActive()
		<< GetRebirthLevel() /*// is reb exp 83+ thing << uint8(m_bLevel == 83) // is reb exp 83+ thing*/
		<< uint16(m_sAchieveCoverTitle)
		<< ReturnSymbolisOK // R symbol after name returned?
		<< uint32(0)
		<< uint16(0);
}
void CBot::FindMonsterAttackSlot()
{
	if (m_bResHpType == USER_DEAD)
		return;

	CNpc* pNpc = nullptr;
	KOMap* pMap = GetMap();
	if (pMap == nullptr)
		return;

	foreach_region(rx, rz)
		FindMonsterAttack(rx + GetRegionX(), rz + GetRegionZ(), pMap);
}

void CBot::FindMonsterAttack(int x, int z, C3DMap* pMap)
{
	if (x < 0 || z < 0 || x > pMap->GetXRegionMax() || z > pMap->GetZRegionMax())
		return;

	//if (nSkillTime > 0 && UNIXTIME < nSkillTime)
	//	return;
	/*if (nSkillTime > 0 && nSkillTime - UNIXTIME2 > 800)
		nSkillTime = 0;*/

	float fDis = 0.0f, fSearchRange = 30;
	int iValue = 0;

	float monster_x, monster_y, monster_z;

	CRegion* pRegion = pMap->GetRegion(x, z);
	if (pRegion == nullptr)
		return;

	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = 0;

	std::vector<uint16> willDel;

	__Vector3 vBot, vUser, vDistance, vRealDistance;

	pRegion->m_lockNpcArray.lock_shared();
	ZoneNpcArray cm_RegionNpcArray = pRegion->m_RegionNpcArray;
	if (cm_RegionNpcArray.empty()) {
		pRegion->m_lockNpcArray.unlock_shared();
		return;
	}
	pRegion->m_lockNpcArray.unlock_shared();

	CNpc* pNpc = nullptr;
	foreach(itr, cm_RegionNpcArray)
	{
		pNpc = g_pMain->GetNpcPtr(*itr, pMap->GetID());
		if (pNpc == nullptr
			|| pNpc->isDead()
			|| !pNpc->isMonster()
			|| pNpc->m_bZone != m_bZone)
			continue;

		if (BestTargetID > 0)
			continue;

		if (pow(m_curx - pNpc->GetX(), 2.0f) + pow(m_curz - pNpc->GetZ(), 2.0f) > 400) //bot skill range
			continue;

		Mesafe = pow(pNpc->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(pNpc->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);

		if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
			continue;
		monster_x = pNpc->GetX();
		monster_z = pNpc->GetZ();
		monster_y = pNpc->GetY();

		vBot.Set(GetX(), pNpc->GetY(), GetZ());
		vUser.Set(pNpc->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f), pNpc->GetY(), pNpc->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f));

		BestTargetID = pNpc->GetID();
	

		EnYakinMesafe = Mesafe;

		if (pNpc->m_Target.id < 0)
		{
			pNpc->m_Target.id = GetID();
			pNpc->m_Target.bSet = true;
		}
	}


	if (BestTargetID == uint16(0)) {
		m_tLastKillTime = UNIXTIME2 + 5000;
		return;
	}

	if (m_TargetID != BestTargetID)
	{
		m_TargetChanged = true;
		m_TargetID = BestTargetID;
		m_Tcurx = vUser.x + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_Tcurz = vUser.z + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_Tcury = vUser.y;
	}

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 45.0f;
	uint8 KosuSuresi = 1;
	bool KosuBitirme = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0) // Duruyorsa
		&& vDistance.Magnitude() < vRealDistance.Magnitude() // Ger�ek mesafe tek saniyelik ko�u mesafesinden b�y�kse
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())// Ger�ek mesafe 2 saniyelik ko�u mesafesinden b�y�kse
	{
		vDistance *= 2.0f;// 2 saniyelik ko�u yapt�ral�m
		KosuSuresi = 2;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude() // Ger�ek mesafe ko�u mesafesinden k���kse
		|| vDistance.Magnitude() == vRealDistance.Magnitude())// Ger�ek mesafe ko�u mesafesine e�itse
	{
		KosuBitirme = true;
		vDistance = vRealDistance;
	}

	uint8 bResult = ATTACK_SUCCESS;
	Packet result(WIZ_ATTACK, uint8(LONG_ATTACK));
	result << bResult << pNpc->GetID() << GetID();
	SendToRegion(&result);

	if (nSkillTime == 0)
	{
		Packet result(WIZ_ATTACK, uint8(MAGIC_ATTACK));
		result << bResult << GetID() << pNpc->GetID();
		SendToRegion(&result);
	}
	uint16 will_x, will_z, will_y;
	if (m_TargetChanged)
	{
		m_TargetChanged = false;
		echo = uint8(1);
		m_tLastKillTime = UNIXTIME2 + KosuSuresi * 995;
	}
	else if (KosuBitirme)
	{
		echo = uint8(0);
		//uint32* Time = new uint32(UNIXTIME2 + KosuSuresi * 995);
		m_tLastKillTime = UNIXTIME2 + (myrand(1, m_maxStop) * 995);
	}
	else
	{
		echo = uint8(3);
		m_tLastKillTime = UNIXTIME2 + KosuSuresi * 995;
	}

	m_tLastKillTime = UNIXTIME2 + 1500;
	if (isRogue())
	{
		if (nSkillTime == 0)
		{
			Packet result(WIZ_MAGIC_PROCESS, uint8(MagicOpcode::MAGIC_CASTING)); // here we emulate a skill packet to be handled.
			result << uint32(208500) << GetID() << GetID() << m_curx << m_cury << m_curz;
			SendToRegion(&result);
			nSkillTime = UNIXTIME2 + myrand(1100, 1500);

		}
		else if (nSkillTime > 0 && UNIXTIME2 > nSkillTime)
		{
			Packet result2(WIZ_MAGIC_PROCESS, uint8(MagicOpcode::MAGIC_EFFECTING)); // here we emulate a skill packet to be handled.
			result2 << uint32(208500) << GetID() << pNpc->GetID() << m_curx << m_cury << m_curz;
			SendToRegion(&result2);
			if (!pNpc->isDead())
				pNpc->HpChange(-300);

			nSkillTime = 0;
		}
	}

	if (isMage())
	{

		if (nSkillTime == 0)
		{
			Packet result(WIZ_MAGIC_PROCESS, uint8(MagicOpcode::MAGIC_CASTING)); // here we emulate a skill packet to be handled.
			result << uint32(109518) << GetID() << GetID() << m_curx << m_cury << m_curz;
			SendToRegion(&result);
			nSkillTime = UNIXTIME2 + myrand(1200, 1700);

		}
		else if (nSkillTime > 0 && UNIXTIME2 > nSkillTime)
		{
			Packet result2(WIZ_MAGIC_PROCESS, uint8(MagicOpcode::MAGIC_EFFECTING)); // here we emulate a skill packet to be handled.
			result2 << uint32(109518) << GetID() << pNpc->GetID() << m_curx << m_cury << m_curz;
			SendToRegion(&result2);
			nSkillTime = 0;

			if (!pNpc->isDead())
				pNpc->HpChange(-300);
		}

	}


	if (isWarrior())
	{
		bool isVardi = false;
		if (pow(m_curx - pNpc->GetX(), 2.0f) + pow(m_curz - pNpc->GetZ(), 2.0f) < 70)
			isVardi = true;

		if (!isVardi)
		{
			will_x = uint16((vBot).x * 10.0f);
			will_y = uint16(vUser.y * 10.0f);
			will_z = uint16((vBot).z * 10.0f);

			Packet result(WIZ_MOVE);
			result << GetID() << will_x << will_z << will_y << speed << uint8(echo);
			SendToRegion(&result);
		}
		m_tLastKillTime = UNIXTIME2;

		m_curx = (vBot + vDistance).x;
		m_curz = (vBot + vDistance).z;
		m_cury = vUser.y;
		if (isVardi)
		{
			if (nSkillTime == 0)
			{

				nSkillTime = UNIXTIME2 + myrand(1100, 1500);

			}
			else if (nSkillTime > 0 && UNIXTIME2 > nSkillTime)
			{
				Packet result2(WIZ_MAGIC_PROCESS, uint8(MagicOpcode::MAGIC_EFFECTING)); // here we emula3te a skill packet to be handled.
				result2 << uint32(105505) << GetID() << pNpc->GetID() << m_curx << m_cury << m_curz;
				SendToRegion(&result2);
				nSkillTime = 0;

				if (!pNpc->isDead())
					pNpc->HpChange(-300);
			}

		}

		RegisterRegion();
	}

}

void CBot::SetMaxHp(int iFlag)
{
	_CLASS_COEFFICIENT* p_TableCoefficient = nullptr;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData(m_sClass);
	if (!p_TableCoefficient) return;

	int temp_sta = GetStat(StatType::STAT_STA);

	if (GetZoneID() == ZONE_SNOW_BATTLE && iFlag == 0)
	{
		if (GetFame() == COMMAND_CAPTAIN || isKing())
			m_MaxHP = 300;
		else
			m_MaxHP = 100;
	}
	else if (m_bZone == ZONE_CHAOS_DUNGEON && iFlag == 0)
		m_MaxHP = 10000 / 10;
	else
	{
		m_MaxHP = (short)(((p_TableCoefficient->HP * GetLevel() * GetLevel() * temp_sta)
			+ 0.1 * (GetLevel() * temp_sta) + (temp_sta / 5)) + 20);

		// A player's max HP should be capped at (currently) 14,000 HP.
		if (m_MaxHP > g_pMain->pServerSetting.maxplayerhp)
			m_MaxHP = g_pMain->pServerSetting.maxplayerhp;

		if (iFlag == 1)
			m_sHp = m_MaxHP;
		else if (iFlag == 2)
			m_MaxHP = 100;
	}

	if (m_MaxHP < m_sHp)
		m_sHp = m_MaxHP;
}

void CBot::SetMaxMp()
{
	_CLASS_COEFFICIENT* p_TableCoefficient = nullptr;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData(m_sClass);
	if (!p_TableCoefficient)
		return;

	int temp_intel = 0, temp_sta = 0;
	temp_intel = GetStat(StatType::STAT_INT) + 30;

	if (temp_intel > 255)
		temp_intel = 255;

	temp_sta = GetStat(StatType::STAT_STA);
	if (temp_sta > 255)
		temp_sta = 255;

	if (p_TableCoefficient->MP != 0)
	{
		m_MaxMp = (short)((p_TableCoefficient->MP * GetLevel() * GetLevel() * temp_intel)
			+ (0.1f * GetLevel() * 2 * temp_intel) + (temp_intel / 5) + 20);
	}
	else if (p_TableCoefficient->SP != 0)
	{
		m_MaxMp = (short)((p_TableCoefficient->SP * GetLevel() * GetLevel() * temp_sta)
			+ (0.1f * GetLevel() * temp_sta) + (temp_sta / 5));
	}

	if (m_MaxMp < m_sMp)
		m_sMp = m_MaxMp;
}

int CBot::FindSlotForItem(uint32 nItemID, uint16 sCount /*= 1*/)
{
	int result = -1;
	_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull())
		return result;

	_ITEM_DATA* pItem = nullptr;
	if (pTable.m_bCountable > 0) {
		for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++) {
			pItem = GetItem(i);
			if (pItem == nullptr)
				continue;

			if (pItem->nNum == nItemID && pItem->sCount + sCount <= ITEMCOUNT_MAX)
				return i;

			// Found a free slot, we'd prefer to stack it though
			// so store the first free slot, and ignore it.
			if (pItem->nNum == 0 && result < 0)
				result = i;
		}
		// If we didn't find a slot countaining our stackable item, it's possible we found
		// an empty slot. So return that (or -1 if it none was found; no point searching again).
		return result;
	}

	// If it's not stackable, don't need any additional logic.
	// Just find the first free slot.
	return GetEmptySlot();
}

int CBot::GetEmptySlot()
{
	_ITEM_DATA *pItem = nullptr;
	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		pItem = GetItem(i);
		if (pItem == nullptr)
			continue;

		if (pItem->nNum == 0)
			return i;
	}
	return -1;
}

bool CBot::GiveItemChecks(uint32 itemid, uint16 count)
{
	_ITEM_TABLE pTable = g_pMain->GetItemPtr(itemid);
	if (pTable.isnull() || count <= 0)
		return false;

	int8 pos = FindSlotForItem(itemid, count);
	if (pos < 0)
		return false;

	_ITEM_DATA* pItem = GetItem(pos);
	if (pItem == nullptr)
		return false;

	if (pTable.m_bCountable == 2) {
		return false;
	}

	return true;
}

#pragma region CUser::GiveItem(uint32 itemid, uint16 count, bool send_packet /*= true*/, int isRentalHour, bool isQuest)
bool CBot::GiveItem(uint32 itemid, uint16 count, bool send_packet /*= true*/, uint32 Time)
{
	if (!GiveItemChecks(itemid, count))
		return false;

	bool bNewItem = true;
	_ITEM_TABLE pTable = g_pMain->GetItemPtr(itemid);
	if (pTable.isnull())
		return false;

	int8 pos = FindSlotForItem(itemid, count);
	if (pos < 0)
		return false;

	_ITEM_DATA* pItem = GetItem(pos);
	if (pItem == nullptr)
		return false;

	if (pItem->nNum != 0)
		bNewItem = false;

	if (bNewItem)
		pItem->nSerialNum = g_pMain->GenerateItemSerial();

	pItem->nNum = itemid;
	pItem->sCount += count;
	pItem->sDuration = pTable.m_sDuration;

	if (pItem->sCount > MAX_ITEM_COUNT)
		pItem->sCount = MAX_ITEM_COUNT;

	if (Time != 0)
		pItem->nExpirationTime = int32(UNIXTIME) + ((60 * 60 * 24) * Time);
	else
		pItem->nExpirationTime = 0;

	// This is really silly, but match the count up with the duration
	// for this special items that behave this way.
	if (pTable.m_bKind == 255)
		pItem->sCount = 1;


	return true;
}

#pragma endregion


bool CBot::JobGroupCheck(short jobgroupid)
{
	if (jobgroupid > 100)
		return GetClass() == jobgroupid;

	ClassType subClass = GetBaseClassType();
	switch (jobgroupid)
	{
	case GROUP_WARRIOR:
		return (subClass == ClassType::ClassWarrior || subClass == ClassType::ClassWarriorNovice || subClass == ClassType::ClassWarriorMaster);

	case GROUP_ROGUE:
		return (subClass == ClassType::ClassRogue || subClass == ClassType::ClassRogueNovice || subClass == ClassType::ClassRogueMaster);

	case GROUP_MAGE:
		return (subClass == ClassType::ClassMage || subClass == ClassType::ClassMageNovice || subClass == ClassType::ClassMageMaster);

	case GROUP_CLERIC:
		return (subClass == ClassType::ClassPriest || subClass == ClassType::ClassPriestNovice || subClass == ClassType::ClassPriestMaster);

	case GROUP_PORTU_KURIAN:
		return (subClass == ClassType::ClassPortuKurian || subClass == ClassType::ClassPortuKurianNovice || subClass == ClassType::ClassPortuKurianMaster);
	}

	return (subClass == (ClassType)jobgroupid);
}

COMMAND_HANDLER(CUser::HandleServerBotCommand)
{
	if (!isGM())
		return false;

	if (vargs.size() < 4)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +user_bots Count Time(AS MINUTE) ResType(1 Mining 2 Fishing 3 Standing 4 Sitting) minlevel");
		return true;
	}

	int Minute = 0, Restie = 0, minlevel = 0, sCount = 0;

	sCount = SafeAtoi(vargs.front(), 0, 100);

	vargs.pop_front();

	Minute = SafeAtoi(vargs.front(), 0, 1440);

	vargs.pop_front();

	Restie = SafeAtoi(vargs.front(), 0, 10);

	vargs.pop_front();

	minlevel = SafeAtoi(vargs.front(), 0, 83);
	if (sCount > 25) // 25ten fazla bot yaz�l�rsa otomatik 25e �eker
		sCount = 25;

	for (int i = 0; i < sCount; i++)
	{
		float Bonc  = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;
		float Bonc2 = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;

		g_pMain->SpawnUserBot(Minute, GetZoneID(), GetX() + Bonc, GetY(), GetZ() + Bonc2, Restie, minlevel,m_sDirection);
	}
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleServerBotCommand)
{
	if (vargs.size() < 4)
	{
		// send description
		printf("Using Sample : /user_bots Count Time(AS MINUTE) ResType(1 Mining 2 Fishing 3 Standing 4 Sitting) minlevel");
		return true;
	}

	int Minute = 0, Restie = 0, minlevel = 0, sCount = 0;

	sCount = SafeAtoi(vargs.front(), 0, 100);

	vargs.pop_front();

	Minute = SafeAtoi(vargs.front(), 0, 1440);

	vargs.pop_front();

	Restie = SafeAtoi(vargs.front(), 0, 10);

	vargs.pop_front();

	minlevel = SafeAtoi(vargs.front(), 0, 83);
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);

		if (pUser == nullptr || !pUser->isInGame())
			continue;

		if (pUser->isInPKZone())
			continue;

		for (int i = 0; i < sCount; i++)
		{
			float Bonc = myrand(1, 15) * 1.0f;
			float Bonc2 = myrand(1, 15) * 1.0f;

			if (sCount > 25) //25ten fazla bot yaz�l�rsa otomatik 25e �eker
				sCount = 25;
			g_pMain->SpawnUserBot(Minute, pUser->GetZoneID(), pUser->GetX() + Bonc, pUser->GetY(), pUser->GetZ() + Bonc2, Restie, minlevel, pUser->m_sDirection);
		}
	}

	return true;
}

uint16 CGameServerDlg::SpawnUserBot(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 Restipi, 
	uint8 minlevel /* = 1*/, int16 direction, uint32 SaveID, uint8 Class, _bot_merchant _merchant)
{

	std::vector<CBot*> mBotList;

	m_BotArray.m_lock.lock();
	foreach_stlmap_nolock(itr, m_BotArray)
	{
		CBot* pUser = itr->second;
		if (pUser == nullptr || pUser->m_state == GameState::GAME_STATE_INGAME)
			continue;

		if ((byZone <= ZONE_ELMORAD && byZone != pUser->m_bNation)
			|| (byZone >= ZONE_KARUS_ESLANT && byZone <= ZONE_ELMORAD_ESLANT && byZone != (pUser->m_bNation + 10)))
			continue;

		mBotList.push_back(pUser);
	}
	m_BotArray.m_lock.unlock();

	if (mBotList.empty())
	{
		if (_merchant.index) {
			auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(_merchant.index);
			if (pCoord)
				pCoord->used = false;
		}
		return 0;
	}

	foreach(itr, mBotList)
	{
		auto* pUser = *itr;
		if (!pUser) continue;

		pUser->m_bMerchantState = (SaveID > 0 ? MERCHANT_STATE_SELLING : MERCHANT_STATE_NONE);
		pUser->LastWarpTime = 0;
		pUser->m_merchantareatype = 0;

		if (Minute > 0)
			pUser->LastWarpTime = UNIXTIME + (Minute * 60);

		pUser->m_sDirection = direction;
		pUser->m_pMap = GetZoneByID(byZone);
		pUser->m_bZone = byZone;
		pUser->m_merchantindex = 0;
		pUser->m_iGold = myrand(1000, 5000000);

		if (Restipi == 13 || Restipi == 14)
		{
			_ITEM_DATA* pItem = &pUser->m_sItemArray[RIGHTHAND];

			_ITEM_TABLE pTable = GetItemPtr(pItem->nNum);
			if (pTable.isnull())
				continue;

			if (Class == 1 && !pUser->isWarrior())
				continue;

			if (Class == 2 && !pUser->isRogue())
				continue;

			if (Class == 3 && !pUser->isMage())
				continue;

			if (Class == 4 && !pUser->isPriest())
				continue;

			if (pUser->isRogue() && !pTable.isBow())
				continue;

			if (pUser->isWarrior() || pTable.isShield() || pTable.isPickaxe() || pTable.isFishing())
				continue;

			pUser->restyping = Restipi;
			pUser->m_bGenieStatus = 1;

			if (Restipi == 14)
				pUser->m_bPartyLeader = true;
			else
				pUser->m_bPartyLeader = false;
		}
		else if (Restipi == 50)
		{
			_BOT_SAVE_DATA* pAuto = g_pMain->m_BotSaveDataArray.GetData(SaveID);
			if (pAuto == nullptr)
			{
				return 0;
			}

			std::string advertMessage = "test";

			uint16 bResult = 0;
			uint8 MerchantItemleri = 0;
			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			{
				if (pAuto->nNum[i] != 0)
					MerchantItemleri++;
			}

			if (MerchantItemleri == 0)
				return false;

			bResult = 1;
			_MERCH_DATA	m_arNewItems[MAX_MERCH_ITEMS]{};

			if (pAuto->sMerchanType == 0) {
				Packet result(WIZ_MERCHANT, uint8(MERCHANT_INSERT));

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				{
					int8 sItemSlot = pUser->FindSlotForItem(pAuto->nNum[i], pAuto->sCount[i]);
					if (sItemSlot < 0)
						continue;

					auto* pData = pUser->GetItem(sItemSlot);
					if (!pData || pData->nNum != 0)
						continue;

					pData->nNum = pAuto->nNum[i];
					pData->sCount = pAuto->sCount[i];
					pData->sDuration = pAuto->sDuration[i];
					pData->nSerialNum = pAuto->nSerialNum[i];
					pData->MerchItem = true;

					m_arNewItems[i].sCount = pAuto->sCount[i];
					m_arNewItems[i].orgcount = pAuto->sCount[i];
					m_arNewItems[i].nNum = pAuto->nNum[i];
					m_arNewItems[i].IsSoldOut = pAuto->IsSoldOut[i];
					m_arNewItems[i].sDuration = pAuto->sDuration[i];
					m_arNewItems[i].nPrice = pAuto->nPrice[i];
					m_arNewItems[i].nSerialNum = pAuto->nSerialNum[i];
					m_arNewItems[i].bOriginalSlot = sItemSlot;
					m_arNewItems[i].isKC = pAuto->isKc[i];
				}

				uint8 reqcount = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					if (m_arNewItems[i].nNum)
						reqcount++;

				if (!reqcount)
					return false;

				uint8 nRandom = 3;
				std::string asdasd = "";
				if (!asdasd.empty())
					pUser->MerchantChat = string_format("%s(Location:%d,%d)", asdasd.c_str(), pUser->GetSPosX() / 10, pUser->GetSPosZ() / 10);

				pUser->MerchantChat.clear();
				pUser->m_iLoyalty = myrand(3000, 5000);
				_MERCH_DATA* pMerch = new _MERCH_DATA();
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				{
					pMerch->sCount = pAuto->sCount[i];
					pMerch->nNum = pAuto->nNum[i];
					pMerch->sDuration = pAuto->sDuration[i];
					pMerch->nPrice = pAuto->nPrice[i];
					pMerch->nSerialNum = pAuto->nSerialNum[i];
					pMerch->bOriginalSlot = i;
					pMerch->IsSoldOut = false;
					pMerch->isKC = pAuto->isKc[i];
					bResult = 1;
				}

				pUser->m_bPremiumMerchant = 0;
				pUser->m_bMerchantState = MERCHANT_STATE_SELLING;
				result << bResult << advertMessage << pUser->GetID()
					<< pUser->m_bPremiumMerchant;

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					pUser->m_arMerchantItems[i] = m_arNewItems[i];

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					result << pUser->m_arMerchantItems[i].nNum;

				pUser->SendToRegion(&result);
			}

			if (pAuto->sMerchanType == 1) {
				Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_REGION_INSERT));

				_MERCH_DATA* pMerch = new _MERCH_DATA();
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				{
					pUser->m_arMerchantItems[i].nNum = pAuto->nNum[i];
					pUser->m_arMerchantItems[i].sCount = pAuto->sCount[i];

					pUser->m_arMerchantItems[i].nPrice = pAuto->nPrice[i];
					pUser->m_arMerchantItems[i].sDuration = pAuto->sDuration[i];
					pUser->m_arMerchantItems[i].isKC = pAuto->isKc[i];
				}
				pUser->m_bMerchantState = MERCHANT_STATE_BUYING;

				result << pUser->GetID();
				for (int i = 0; i < 4; i++)
					result << pUser->m_arMerchantItems[i].nNum;


				pUser->SendToRegion(&result);
			}
		}
		else if (Restipi == 1)
		{
			_ITEM_DATA* pItem = &pUser->m_sItemArray[RIGHTHAND];
			if (pItem)
			{
				auto pTable = GetItemPtr(myrand(0, 100) > 50 ? GOLDEN_MATTOCK : MATTOCK);
				if (pTable.isnull())
					continue;

				_ITEM_DATA* pTItem = &pUser->m_sItemArray[LEFTHAND];
				if (pTItem) memset(pTItem, 0x00, sizeof(_ITEM_DATA));

				memset(pItem, 0x00, sizeof(_ITEM_DATA));
				pItem->nNum = pTable.m_iNum;
				pItem->nSerialNum = GenerateItemSerial();
				pItem->sCount = 1;
				pItem->sDuration = pTable.m_sDuration;
				pUser->m_bResHpType = USER_MINING;
			}
		}
		else if (Restipi == 2)
		{
			_ITEM_DATA* pItem = &pUser->m_sItemArray[RIGHTHAND];
			if (pItem)
			{
				auto pTable = GetItemPtr(myrand(0, 100) > 50 ? GOLDEN_FISHING : FISHING);
				if (pTable.isnull())
					continue;

				auto* pTItem = &pUser->m_sItemArray[LEFTHAND];
				if (pTItem != nullptr)
					memset(pTItem, 0x00, sizeof(_ITEM_DATA));

				memset(pItem, 0x00, sizeof(_ITEM_DATA));
				pItem->nNum = pTable.m_iNum;
				pItem->nSerialNum = GenerateItemSerial();
				pItem->sCount = 1;
				pItem->sDuration = pTable.m_sDuration;
				pUser->m_bResHpType = USER_FLASHING;
			}
		}
		else if (Restipi == 3 || Restipi == 4)
			pUser->m_bResHpType = Restipi == 3 ? USER_STANDING : USER_SITDOWN;
		else if (Restipi == 5)
		{
			//int Random = myrand(0, 10000);
			pUser->m_bResHpType = USER_STANDING;// Random > 5000 ? USER_STANDING : USER_SITDOWN;
		}
		else if (Restipi == 6) {}
		else continue;

		if (Restipi == 6)
		{
			pUser->m_merchantindex = _merchant.index;
			pUser->m_merchantareatype = _merchant.areaType;

			uint8 itemcount = 0;
			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			{
				if (_merchant.merc[i].itemid)
					itemcount++;
			}

			_MERCH_DATA	m_arNewItems[MAX_MERCH_ITEMS]{};
			memset(m_arNewItems, 0, sizeof(m_arNewItems));

			if (!_merchant.isBuy) {

				for (int i = 0; i < MAX_MERCH_ITEMS; i++) {
					if (!_merchant.merc[i].itemid)
						continue;

					int8 sItemSlot = pUser->FindSlotForItem(_merchant.merc[i].itemid, _merchant.merc[i].count);
					if (sItemSlot < 0)
						continue;

					auto pItem = g_pMain->GetItemPtr(_merchant.merc[i].itemid);
					if (pItem.isnull())
						continue;

					auto* pData = pUser->GetItem(sItemSlot);
					if (!pData || (pData->nNum != 0 && !pItem.m_bCountable))
						continue;

					pData->nNum = _merchant.merc[i].itemid;
					pData->sCount += _merchant.merc[i].count;
					pData->sDuration = _merchant.merc[i].pTable.m_sDuration;
					pData->nSerialNum = g_pMain->GenerateItemSerial();
					pData->MerchItem = true;

					m_arNewItems[i].sCount = pData->sCount;
					m_arNewItems[i].orgcount = pData->sCount;
					m_arNewItems[i].nNum = pData->nNum;
					m_arNewItems[i].IsSoldOut = false;
					m_arNewItems[i].sDuration = pData->sDuration;
					m_arNewItems[i].nPrice = _merchant.merc[i].price;
					m_arNewItems[i].nSerialNum = pData->nSerialNum;
					m_arNewItems[i].bOriginalSlot = sItemSlot;
					m_arNewItems[i].isKC = _merchant.merc[i].iskc;
				}

				uint8 reqcount = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					if (m_arNewItems[i].nNum)
						reqcount++;

				if (!reqcount)
				{
					if (_merchant.index) {
						auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(_merchant.index);
						if (pCoord)
							pCoord->used = false;
					}
					return 0;
				}
					
				pUser->m_bPremiumMerchant = myrand(0, 100) < 15;
				pUser->m_bMerchantState = MERCHANT_STATE_SELLING;
			}
			else {

				uint32 total_price = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++) {
					m_arNewItems[i].sCount = _merchant.merc[i].count;
					m_arNewItems[i].orgcount = _merchant.merc[i].count;
					m_arNewItems[i].nNum = _merchant.merc[i].itemid;
					m_arNewItems[i].sDuration = _merchant.merc[i].pTable.m_sDuration;
					m_arNewItems[i].nPrice = _merchant.merc[i].price;
					m_arNewItems[i].isKC = _merchant.merc[i].iskc;
					m_arNewItems[i].bOriginalSlot = i;
					total_price += _merchant.merc[i].price;
				}

				uint8 reqcount = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					if (m_arNewItems[i].nNum)
						reqcount++;

				if (!reqcount)
				{
					if (_merchant.index) {
						auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(_merchant.index);
						if (pCoord)
							pCoord->used = false;
					}
					return 0;
				}

				pUser->m_bPremiumMerchant = myrand(0, 100) < 15;
				pUser->m_bMerchantState = MERCHANT_STATE_BUYING;

				if (pUser->m_iGold < total_price)
					pUser->m_iGold = myrand(total_price, total_price + 5000000);
			}

			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				pUser->m_arMerchantItems[i] = m_arNewItems[i];
		}

		pUser->SetPosition(fX, fY, fZ);
		pUser->SetRegion(pUser->GetNewRegionX(), pUser->GetNewRegionZ());
		pUser->SetMaxHp(1);
		pUser->SetMaxMp();
		pUser->UserInOut(INOUT_IN);

		if (Restipi == 6) {

			Packet result(WIZ_MERCHANT);
			if (!_merchant.isBuy)
				result << uint8(MERCHANT_INSERT) << uint16(1) << "advertMessage" << pUser->GetID() << pUser->m_bPremiumMerchant;
			else
				result << uint8(MERCHANT_BUY_REGION_INSERT) << pUser->GetID();

			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				result << pUser->m_arMerchantItems[i].nNum;

			pUser->SendToRegion(&result);
		}

		return pUser->GetID();
	}
	
	return 0;
}

void CBot::SendMerchantChat() {
	if (MerchantChat.empty()) return;

	Packet result(WIZ_CHAT);
	ChatPacket::Construct(&result,(uint8)ChatType::MERCHANT_CHAT, &MerchantChat, &GetName(), m_bNation, GetID());
	SendToRegion(&result);
	m_lastmerchanttime = UNIXTIME;
}

//login bot start
COMMAND_HANDLER(CUser::HandleLoginBotCommand)
{
	if (!isGM())
		return false;

	if (isInPKZone())
		return true;

	if (vargs.size() < 4)
	{
		g_pMain->SendHelpDescription(this, "Town [1] - Hemes [2] - Count - StepCount - Time");
		return true;
	}

	uint8 type = SafeAtoi(vargs.front(), 0, 255);
	vargs.pop_front();

	uint16 scount = SafeAtoi(vargs.front(), 0, 65535);
	vargs.pop_front();

	uint16 step = SafeAtoi(vargs.front(), 0, 65535);
	vargs.pop_front();

	uint16 xtime = SafeAtoi(vargs.front(), 0, 65535);
	vargs.pop_front();

	if (type > 2 || type <= 0)
	{
		g_pMain->SendHelpDescription(this, "Bot type not found!");
		return true;
	}

	uint8 RandomX = myrand(1, 2);

	if (scount > 1 && step > 0 && xtime > 0)
	{
		if (g_pMain->MaxSpawnCount > 0)
		{
			g_pMain->SendHelpDescription(this, "Login Bot Start Active.");
			return true;
		}

		g_pMain->MaxSpawnCount = scount;
		g_pMain->BotStepCount = step;
		g_pMain->BotTimeNext = xtime;
		g_pMain->BotAutoType = type;

		if (RandomX == 1)
		{
			g_pMain->botautoX = 264;
			g_pMain->botautoZ = 302;
		}
		else
		{
			g_pMain->botautoX = 264;
			g_pMain->botautoZ = 302;
		}

		g_pMain->SendHelpDescription(this, "Login Bot Started.");
		return true;
	}

	if (RandomX == 1)
	{
		if (scount > 1 && step == 0 && xtime == 0)
		{
			for (size_t i = 0; i < scount; i++)
			{
				if (g_pMain->SpawnLoginBot(264, 4, 302, type) > 0)
					g_pMain->SendHelpDescription(this, "Login bot added");
			}
		}
		else
		{
			if (g_pMain->SpawnLoginBot(264, 4, 302, type) > 0)
				g_pMain->SendHelpDescription(this, "Login bot added");
		}
	}
	else
	{
		if (scount > 1 && step == 0 && xtime == 0)
		{
			for (size_t i = 0; i < scount; i++)
			{
				if (g_pMain->SpawnLoginBot(264, 4, 302, type) > 0)
					g_pMain->SendHelpDescription(this, "Login bot added");
			}
		}
		else
		{
			if (g_pMain->SpawnLoginBot(264, 4, 302, type) > 0)
				g_pMain->SendHelpDescription(this, "Login bot added");
		}
	}

	return true;
}

uint16 CGameServerDlg::SpawnLoginBot(float fX, float fY, float fZ, uint8 type)
{
	std::vector<CBot*> mBotList;
	m_BotArray.m_lock.lock();
	foreach_stlmap_nolock(itr, m_BotArray)
	{
		CBot* pBot = itr->second;

		if (pBot == nullptr)
			continue;

		if (pBot->m_state == GameState::GAME_STATE_INGAME)
			continue;

		mBotList.push_back(pBot);
	}
	m_BotArray.m_lock.unlock();

	foreach(itr, mBotList)
	{
		auto* pBot = *itr;
		if (!pBot) continue;

		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (10 * 60);
		pBot->m_Step = 0;
		pBot->m_pMap = GetZoneByID(ZONE_MORADON);
		pBot->m_bZone = ZONE_MORADON;
		pBot->m_bGenieStatus = false;
		pBot->m_bPartyLeader = false;
		pBot->m_bResHpType = USER_STANDING;
		pBot->m_sHp = pBot->m_MaxHP;
		pBot->LoginBotType = type;
		pBot->m_bAbnormalType = ABNORMAL_BLINKING;
		pBot->m_tBlinkExpiryTime = UNIXTIME + BLINK_TIME;
		uint8 randtime = myrand(3, 10);
		pBot->m_StartMoveTime = UNIXTIME + randtime;
		pBot->m_bRegeneType = REGENE_ZONECHANGE;
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetMaxHp(1);
		pBot->SetMaxMp();
		pBot->UserInOut(INOUT_RESPAWN);
		pBot->StateChangeServerDirect(3, ABNORMAL_BLINKING);
		return pBot->GetID();
	}
	return 0;
}

uint32 CGameServerDlg::Timer_BotMoving(void* lpParam)
{
	std::set<CBot*> mlist;
	while (g_bRunning)
	{
		mlist.clear();
		g_pMain->m_BotArray.m_lock.lock();
		foreach_stlmap_nolock(itr, g_pMain->m_BotArray) //forearch_stlmap yerine kullaabilir duruma gore.
		{
			CBot* pBot = itr->second;
			if (pBot == nullptr)
				continue;

			if (pBot->m_state != GameState::GAME_STATE_INGAME)
				continue;

			mlist.insert(pBot);
		}
		g_pMain->m_BotArray.m_lock.unlock();

		foreach(itr, mlist)
		{
			auto* pBot = *itr;
			if (!pBot) continue;

			if (pBot->LoginBotType == 1 && pBot->m_StartMoveTime > 0 && (UNIXTIME > pBot->m_StartMoveTime))
			{
				ULONGLONG time = GetTickCount64();

				if (time > pBot->m_Move_Time)
					pBot->BotTownGo();
			}
			else if (pBot->LoginBotType == 2 && pBot->m_StartMoveTime > 0 && (UNIXTIME > pBot->m_StartMoveTime))
			{
				ULONGLONG time = GetTickCount64();

				if (time > pBot->m_Move_Time)
					pBot->BotHemesGo();
			}
			else if (pBot->LoginBotType == 7 && pBot->m_StartMoveTime > 0 && (UNIXTIME > pBot->m_StartMoveTime))
			{
				ULONGLONG time = GetTickCount64();

				if (time > pBot->m_Move_Time)
					pBot->BotHemesGoStop();
			}

			if (pBot->m_tBlinkExpiryTime > 0 && UNIXTIME > pBot->m_tBlinkExpiryTime)
			{
				pBot->m_bRegeneType = REGENE_NORMAL;
				pBot->StateChangeServerDirect(3, ABNORMAL_NORMAL);
				pBot->m_tBlinkExpiryTime = 0;
			}
		}

		Sleep(100);
	}

	return 0;
}

COMMAND_HANDLER(CUser::HandleMerchantBotSCommand) {
	if (!isGM())
		return true;

	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Using Sample : /sbot AreaType.");
		return true;
	}

	uint16 areaType = 0;
	if (!vargs.empty()) { areaType = SafeAtoi(vargs.front(), 0, 65535); vargs.pop_front(); }

	std::vector<CBot*> mlist;
	g_pMain->m_BotArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_BotArray)
	{
		CBot* pUser = itr->second;
		if (pUser == nullptr || pUser->m_state != GameState::GAME_STATE_INGAME)
			continue;

		if (areaType != pUser->m_merchantareatype)
			continue;

		mlist.push_back(pUser);
	}
	g_pMain->m_BotArray.m_lock.unlock();

	foreach(itr, mlist)
		(*itr)->UserInOut(INOUT_OUT);
	return true;
}

COMMAND_HANDLER(CUser::HandleMerchantBotSaveCommand)
{
	if (!isGM())
		return true;

	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Using Sample : /mbotsave AreaType.");
		return true;
	}

	uint16 botArea = 0;
	if (!vargs.empty()) { botArea = SafeAtoi(vargs.front(), 0, 65535); vargs.pop_front(); }

	if (!botArea) {
		g_pMain->SendHelpDescription(this, "Using Sample : /mbotsave AreaType.");
		return true;
	}

	Packet pkt(WIZ_DB_SAVE, uint8(ProcDbServerType::BotMerchantCoordSave));
	pkt << botArea << (int16)GetX() << (int16)GetZ() << (int16)GetY() << m_sDirection << GetZoneID();
	g_pMain->AddDatabaseRequest(pkt);
	g_pMain->SendHelpDescription(this, "bot registration request received. Please wait.");
	return true;
}

COMMAND_HANDLER(CUser::HandleMerchantBotCommand)
{
	if (!isGM())
		return true;

	if (g_pMain->tar_botinfo) {
		g_pMain->SendHelpDescription(this, "Please reloading table.");
		return true;
	}

	if (vargs.size() < 3) {
		g_pMain->SendHelpDescription(this, "Using Sample : /mbot Count Time AreaType.");
		return true;
	}

	uint16 botCount = 0;
	if (!vargs.empty()) { botCount = SafeAtoi(vargs.front(), 0, 100); vargs.pop_front(); }

	if (!botCount || botCount > 100) {
		g_pMain->SendHelpDescription(this, "The maximum number of robots you can buy cannot exceed 100.");
		return true;
	}

	uint32 botTime = 0;
	if (!vargs.empty()) { botTime = SafeAtoi(vargs.front(), 0, 1440); vargs.pop_front(); }

	botTime = botTime * 1000;

	uint32 botArea = 0;
	if (!vargs.empty()) { botArea = SafeAtoi(vargs.front(), 0, 65535); vargs.pop_front(); }
	if (!botArea) {
		g_pMain->SendHelpDescription(this, "Bot Area Error.");
		return true;
	}

	BotMerchantAdd(botCount, botTime, botArea);
	return true;
}

void CUser::BotMerchantAdd(uint16 count, uint32 bTime, uint16 type)
{
	struct _list {
		uint32 index;
		_MERCHANT_BOT_INFO pInfo;
		_list(uint32 index, _MERCHANT_BOT_INFO pInfo) {
			this->index = index;
			this->pInfo = pInfo;
		}
	};
	std::vector<_list> mList;

	uint16 counter = 0; uint32 index = 0;
	g_pMain->pBotInfo.mCoordinate.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->pBotInfo.mCoordinate) {
		auto* p = itr->second;
		if (!p || p->used || p->type != type)
			continue;

		if (counter >= count)
			break;

		counter++;
		index = itr->first;
		p->used = true;
		mList.push_back(_list(itr->first, *p));
	}
	g_pMain->pBotInfo.mCoordinate.m_lock.unlock();

	if (mList.empty())
	{
		if (index) {
			auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(index);
			if (pCoord)
				pCoord->used = false;
		}
		g_pMain->SendHelpDescription(this, "List is empty!");
		return;
	}

	/*srand(unsigned(time(NULL)));
	std::random_shuffle(mList.begin(), mList.end());*/

	int atime = 1; ULONGLONG btime = UNIXTIME2 + (ULONGLONG)(atime * bTime);
	g_pMain->m_addbotlistLock.lock();
	for (auto& itr : mList) {
		g_pMain->m_addbotlist.insert(std::make_pair(itr.index, _botadd(itr.index, btime, (uint8)type)));
		btime += (ULONGLONG)(atime * bTime);
	}
	g_pMain->m_addbotlistLock.unlock();
}

void CGameServerDlg::t_addBot(uint32 index, uint8 type) {
	
	auto* pCoor = pBotInfo.mCoordinate.GetData(index);
	if (!pCoor || !pCoor->used || pCoor->type != type)
		return;

	bool isBuy = pCoor->isBuy;
	uint8 bZone = pCoor->bZoneID;
	uint16 setX = pCoor->setX, setZ = pCoor->setZ;
	int32 direction = pCoor->direction;
	int16 setY = pCoor->setY;

	std::map<uint32, _MERCHANT_BOT_ITEM> mItems;

	pBotInfo.mItem.m_lock.lock();
	foreach_stlmap_nolock(itr, pBotInfo.mItem) {
		auto* p = itr->second;
		if (!p || p->type != type) 
			continue;

		if (mItems.find(itr->first) != mItems.end())
			continue;

		mItems.insert(std::make_pair(itr->first, *p));
	}
	pBotInfo.mItem.m_lock.unlock();

	if (mItems.empty()) {
		pCoor->used = false;
		return;
	}

	_bot_merchant _merchant{}; 
	_merchant.isBuy = isBuy;
	_merchant.index = index;
	_merchant.areaType = type;
	memset(_merchant.merc, 0, sizeof(_merchant.merc));

	uint8 max_itemcount = myrand(4, 10);
	uint64 totalprice = 0;
	int my_index = 0;

	for (int i = 0; i < max_itemcount; i++) {

		std::vector<uint32> mlist;
		foreach(itr, mItems)
			mlist.push_back(itr->second.itemid);

		if (mlist.empty())
			continue;

		uint32 itemid = mlist[myrand(0, (int32)mlist.size() - 1)];
		auto pBaz = mItems.find(itemid);
		if (pBaz == mItems.end())
			continue;

		auto pTable = g_pMain->GetItemPtr(itemid);
		if (pTable.isnull())
			continue;

		bool iskc = myrand(0, 100) < 50;
		uint32 min_price = 0; // iskc ? pBaz->second.minKc : pBaz->second.minPrice;
		uint32 max_price = 0; // iskc ? pBaz->second.maxKc : pBaz->second.maxPrice;

		uint8 money_type = 0;
		if (pBaz->second.moneytype == 0) { //0: random - 1:coins  - 2:kc
			if (myrand(0, 100) < 50) money_type = 2;
			else money_type = 1;
		}
		else if (pBaz->second.moneytype == 1)
			money_type = 1;
		else if (pBaz->second.moneytype == 2)
			money_type = 2;

		if (money_type == 1) {
			min_price = pBaz->second.minPrice;
			max_price = pBaz->second.maxPrice;
		}
		else {
			min_price = pBaz->second.minKc;
			max_price = pBaz->second.maxKc;
		}
		
		if (!min_price && !max_price)
			continue;

		if (min_price > max_price) 
			min_price = max_price;
		
		uint32 price = myrand(min_price, max_price);
		if (price > COIN_MAX)
			price = COIN_MAX;

		std::string real_price = std::to_string(price);
		if (real_price.empty()) continue;
		
		if (real_price.size() > 3 && real_price.size() < 12)
		{
			std::string final_price = "";
			for (int i = 0; i < real_price.size(); i++)
				final_price.append("0");

			int z = (int)real_price.size();
			if (real_price.size() == 4 || real_price.size() == 5 || real_price.size() == 11) z -= 2;
			else if (real_price.size() == 6 || real_price.size() == 7) z -= 3;
			else if (real_price.size() == 8 || real_price.size() == 9) z -= 3;
			else if (real_price.size() == 10) z -= 4;
			
			if (z > 0 && real_price.size() > z)
			{
				for (int i = 0; i < real_price.size() - z; i++)
					final_price[i] = real_price[i];

				price = (uint32)std::stoull(final_price);

				if (myrand(0, 100) < 15)
					price -= myrand(1, 5);
			}
			
		}

		if (totalprice + price >= COIN_MAX)
			break;

		totalprice += price;

		uint32 minItemCount = pBaz->second.minItemCount, maxItemCount = pBaz->second.maxItemCount;
		if (minItemCount > maxItemCount)
			minItemCount = maxItemCount;

		if (!minItemCount) minItemCount = 1;
		if (!maxItemCount) maxItemCount = 1;

		_merchant.merc[my_index].itemid = itemid;
		_merchant.merc[my_index].price = price;
		_merchant.merc[my_index].iskc = money_type == 2 ? true : false;
		_merchant.merc[my_index].count = myrand(minItemCount, maxItemCount);
		_merchant.merc[my_index].pTable = pTable;
		my_index++;

		//if (pTable.m_bCountable)
		mItems.erase(pBaz->second.itemid);
	}

	uint8 m_bzaa_count = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
		if (_merchant.merc[i].itemid)
			m_bzaa_count++;

	if (!m_bzaa_count)
		return;

	g_pMain->SpawnUserBot(30, bZone, (float)setX, (float)setY, (float)setZ, 6, 1, direction, 0, 0, _merchant);
}

uint32 CGameServerDlg::Timer_BotAutoSpawn(void* lpParam)
{
	while (true)
	{
		{
			std::set<uint32> removelist;  removelist.clear();
			g_pMain->m_addbotlistLock.lock();
			foreach(itr, g_pMain->m_addbotlist) {
				if (itr->second.addtime > UNIXTIME2)
					continue;

				g_pMain->t_addBot(itr->first, itr->second.type);
				removelist.insert(itr->first);
			}
			foreach(itr, removelist) g_pMain->m_addbotlist.erase(*itr);
			g_pMain->m_addbotlistLock.unlock();
		}

		if (g_pMain->MaxSpawnCount > 0 && g_pMain->MaxBotFinish >= g_pMain->MaxSpawnCount)
		{
			g_pMain->MaxSpawnCount = 0;
			g_pMain->BotStepCount = 0;
			g_pMain->BotRespawnTick = 0;
			g_pMain->BotTimeNext = 0;
			g_pMain->MaxBotFinish = 0;
		}

		if (g_pMain->MaxSpawnCount > 0)
		{
			if (GetTickCount64() > g_pMain->BotRespawnTick)
			{
				for (size_t i = 0; i < g_pMain->BotStepCount; i++)
				{
					if (g_pMain->MaxBotFinish >= g_pMain->MaxSpawnCount)
					{
						g_pMain->MaxSpawnCount = 0;
						g_pMain->BotStepCount = 0;
						g_pMain->BotRespawnTick = 0;
						g_pMain->BotTimeNext = 0;
						g_pMain->MaxBotFinish = 0;
						break;
					}

					if (g_pMain->SpawnLoginBot((float)g_pMain->botautoX, (float)4, (float)g_pMain->botautoZ, g_pMain->BotAutoType) > 0)
						g_pMain->MaxBotFinish++;
				}

				g_pMain->BotRespawnTick = (DWORD)GetTickCount64() + g_pMain->BotTimeNext;
			}
		}

		Sleep(1 * SECOND);
	}

	return 0;
}

void CBot::BotHemesGo()
{
	uint8 echo = 0;
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = uint16(-1);
	__Vector3 vBot, vUser, vDistance, vRealDistance;

	uint8 RandomX = myrand(1, 2);
	float x, z;

	if (RandomX == 1)
	{
		x = 327.0f;
		z = 367.0f;
	}
	else
	{
		x = 330.0f;
		z = 365.0f;
	}

	Mesafe = pow(x + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(z + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
	if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
		return;


	if (RandomX == 1)
		vBot.Set(GetX() + 2, 0, GetZ());
	else
		vBot.Set(GetX() - 1, 0, GetZ());

	vUser.Set(x + ((myrand(0, 2000) - 1000.0f) / 500.0f), 0, z + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 40.0f;
	uint8 KosuSuresi = 1;
	bool Finished = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		KosuSuresi = 1;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		Finished = true;
		vDistance = vRealDistance;
	}

	if (Finished)
	{
		LoginBotType = 7;
		echo = uint8(3);
		m_Move_Time = 0;
	}
	else
	{
		echo = uint8(3);
		m_Move_Time = GetTickCount64() + 1700;
	}

	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	Packet result(WIZ_MOVE);
	result << GetID() << will_x << will_z << will_y << uint16(40) << uint8(echo);
	SendToRegion(&result);

	m_curx = (vBot + vDistance).x;
	m_curz = (vBot + vDistance).z;
	m_cury = vUser.y;
	RegisterRegion();
}

void CBot::BotHemesGoStop()
{
	uint8 echo = 0;
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = uint16(-1);
	__Vector3 vBot, vUser, vDistance, vRealDistance;
	uint8 RandomX = myrand(1, 13);
	float CX = 0.0f, CZ = 0.0f;

	if (RandomX == 1)
	{
		CX = 309.0f;
		CZ = 404.0f;
	}
	else if (RandomX == 2)
	{
		CX = 305.0f;
		CZ = 403.0f;
	}
	else if (RandomX == 3)
	{
		CX = 305.0f;
		CZ = 399.0f;
	}
	else if (RandomX == 4)
	{
		CX = 306.0f;
		CZ = 397.0f;
	}
	else if (RandomX == 5)
	{
		CX = 310.0f;
		CZ = 396.0f;
	}
	else if (RandomX == 6)
	{
		CX = 312.0f;
		CZ = 397.0f;
	}
	else if (RandomX == 7)
	{
		CX = 310.0f;
		CZ = 399.0f;
	}
	else if (RandomX == 8)
	{
		CX = 309.0f;
		CZ = 401.0f;
	}
	else if (RandomX == 9)
	{
		CX = 308.0f;
		CZ = 399.0f;
	}
	else if (RandomX == 10)
	{
		CX = 313.0f;
		CZ = 401.0f;
	}
	else if (RandomX == 11)
	{
		CX = 314.0f;
		CZ = 398.0f;
	}
	else if (RandomX == 12)
	{
		CX = 316.0f;
		CZ = 394.0f;
	}
	else if (RandomX == 13)
	{
		CX = 309.0f;
		CZ = 400.0f;
	}

	Mesafe = pow(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
	if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
		return;

	vBot.Set(GetX(), 0, GetZ());
	vUser.Set(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f), 0, CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 40.0f;
	uint8 KosuSuresi = 1;
	bool Finished = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		KosuSuresi = 1;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		Finished = true;
		vDistance = vRealDistance;
	}

	if (Finished)
	{
		m_StartMoveTime = 0;
		LoginBotType = 0;
		echo = uint8(0);
		m_Move_Time = GetTickCount64() + 1700;
	}
	else
	{
		echo = uint8(3);
		m_Move_Time = GetTickCount64() + 1700;
	}

	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	Packet result(WIZ_MOVE);
	result << GetID() << will_x << will_z << will_y << uint16(40) << uint8(echo);
	SendToRegion(&result);

	m_curx = (vBot + vDistance).x;
	m_curz = (vBot + vDistance).z;
	m_cury = vUser.y;
	RegisterRegion();
}

void CBot::BotTownGo()
{
	uint8 echo = 0;
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = uint16(-1);
	__Vector3 vBot, vUser, vDistance, vRealDistance;
	float CX = 0.0f, CZ = 0.0f;
	uint32 RandomX = myrand(248, 268);//town kordinat 1.
	uint32 RandomZ = myrand(286, 309);// town kordinat 2.

	CX = (float)RandomX;
	CZ = (float)RandomZ;

	Mesafe = pow(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
	if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
		return;

	vBot.Set(GetX(), 0, GetZ());
	vUser.Set(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f), 0, CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 40.0f;
	uint8 KosuSuresi = 1;
	bool Finished = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		KosuSuresi = 1;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		Finished = true;
		vDistance = vRealDistance;
	}

	if (Finished)
	{
		m_StartMoveTime = 0;
		LoginBotType = 0;
		echo = uint8(0);
		m_Move_Time = GetTickCount64() + 1700;
	}
	else
	{
		echo = uint8(3);
		m_Move_Time = GetTickCount64() + 1700;
	}

	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	Packet result(WIZ_MOVE);
	result << GetID() << will_x << will_z << will_y << uint16(40) << uint8(echo);
	SendToRegion(&result);

	m_curx = (vBot + vDistance).x;
	m_curz = (vBot + vDistance).z;
	m_cury = vUser.y;
	RegisterRegion();
}



//login bot end