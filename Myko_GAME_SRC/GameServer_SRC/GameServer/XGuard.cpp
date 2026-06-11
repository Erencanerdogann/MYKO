#include "stdafx.h"
#include "stdafx.h"
#include "DBAgent.h"
#include <vector>
#include "md5.h"

using namespace std;
#define XSafe_ACTIVE 1
#define XSafe_VERSION	5				// Anticheat derlendi�inde buras� de�i�tirilmeli. 
#define XSafe_ALIVE_TIMEOUT 60
#define XSafe_SUPPORT_CHEK 30

time_t XSafe_LASTSUPPORT = 0;

struct ProcessInfo
{
	int id;
	string name;
	vector<string> windows;
	ProcessInfo(int _id, string _name, vector<string> _windows)
	{
		id = _id;
		name = _name;
		windows = _windows;
	}
};

enum class PUS_CATEGORY
{
	SPECIAL = 1,
	POWER_UP = 2,
	COSTUME = 3,
	BUNDLE = 4
};

struct PUSItem
{
	uint32 sID;
	string Name;
	string Desc1;
	string Desc2;
	uint32 Price;
	uint32 Count;
	PUS_CATEGORY Category;
	PUSItem(uint32 sid, string name, string desc1, string desc2, uint32 price, uint32 count, PUS_CATEGORY category) {
		sID = sid;
		Name = name;
		Desc1 = desc1;
		Desc2 = desc2;
		Price = price;
		Count = count;
		Category = category;
	}
};

struct _TempItem
{
	uint32 nItemID;
	uint32 time;
	uint8 pos, slot;
};

void CUser::XSafe_SendLifeSkills()
{
	LF_VEC2 m_War = GetLifeSkillCurrent(LIFE_SKILL::WAR);
	LF_VEC2 m_Hunting = GetLifeSkillCurrent(LIFE_SKILL::HUNTING);
	LF_VEC2 m_Smithery = GetLifeSkillCurrent(LIFE_SKILL::SMITHERY);
	LF_VEC2 m_Karma = GetLifeSkillCurrent(LIFE_SKILL::KARMA);

	Packet pkt(XSafe);
	pkt << uint8(XSafeOpCodes::LIFESKILL)
		<< GetLifeSkillLevel(LIFE_SKILL::WAR) << m_War.EXP << m_War.TargetEXP
		<< GetLifeSkillLevel(LIFE_SKILL::HUNTING) << m_Hunting.EXP << m_Hunting.TargetEXP
		<< GetLifeSkillLevel(LIFE_SKILL::SMITHERY) << m_Smithery.EXP << m_Smithery.TargetEXP
		<< GetLifeSkillLevel(LIFE_SKILL::KARMA) << m_Karma.EXP << m_Karma.TargetEXP;

	Send(&pkt);
}

void CUser::XSafe_HandleLifeSkill(Packet& pkt)
{
	XSafe_SendLifeSkills();
}

void CUser::XSafe_ItemNotice(uint8 type, uint32 nItemID)
{
	Packet pkt(XSafe);
	pkt << uint8_t(XSafeOpCodes::AUTODROP) << type << nItemID;
	Send(&pkt);
}

void CUser::XSafe_SendMessageBox(string title, string message)
{
	Packet pkt(XSafe);
	pkt << uint8_t(XSafeOpCodes::MESSAGE) << title << message;
	Send(&pkt);
}

void CUser::XSafe_ReqMerchantList(Packet& pkt)
{
	std::vector<uint16_t> MerchantListUser;
	uint8 subCode;
	pkt >> subCode;
	
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_MENISIA_LIST));
	if (!CheckExistItem(810166000))
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	if (subCode == 0)
	{
		Packet result(XSafe);
		result << uint8(XSafeOpCodes::MERCHANTLIST);

		uint32 merchantCount = 0;

		for (uint16 i = 0; i < MAX_USER; i++)
		{
			CUser * pMerchant = g_pMain->GetUserPtr(i);
			if (pMerchant == nullptr)
				continue;

			if (!pMerchant->isMerchanting())
				continue;

			for (int j = 0; j < MAX_MERCH_ITEMS; j++) 
			{
				_MERCH_DATA * pMerch = &pMerchant->m_arMerchantItems[j];
				if (pMerch->nNum == 0 || pMerch->nPrice == 0)
					continue;

				merchantCount++;
			}

			MerchantListUser.push_back(pMerchant->GetID());
		}


		std::vector<CBot*> mlist;
		g_pMain->m_BotArray.m_lock.lock();
		foreach_stlmap_nolock(itr, g_pMain->m_BotArray)
		{
			CBot * pBotMerchant = itr->second;
			if (pBotMerchant == nullptr || !pBotMerchant->isInGame())
				continue;

			if (!pBotMerchant->isMerchanting())
				continue;

			mlist.push_back(pBotMerchant);
		}
		g_pMain->m_BotArray.m_lock.unlock();

		foreach(itr, mlist)
		{
			auto* pBotMerchant = *itr;
			if (!pBotMerchant)
				continue;

			for (int j = 0; j < MAX_MERCH_ITEMS; j++)
			{
				_MERCH_DATA* pMerch = &pBotMerchant->m_arMerchantItems[j];
				if (pMerch->nNum == 0 || pMerch->nPrice == 0)
					continue;

				merchantCount++;
			}

			MerchantListUser.push_back(pBotMerchant->GetID());
		}

		result << merchantCount;

		foreach(itr, MerchantListUser)
		{
			auto UserList = (*itr);
			if (UserList < 0) continue;

			CUser * pMerchant = g_pMain->GetUserPtr(UserList);
			CBot * pMerchantBot = g_pMain->m_BotArray.GetData(UserList);
			if (pMerchant != nullptr)
			{
				for (int j = 0; j < MAX_MERCH_ITEMS; j++)
				{
					_MERCH_DATA * pMerch = &pMerchant->m_arMerchantItems[j];
					if (pMerch->nNum == 0 || pMerch->nPrice == 0)
						continue;

					result << pMerchant->GetID() << uint32(UserList) << pMerchant->GetName() 
						<< uint8((pMerchant->isSellingMerchant() ? uint8(0) : uint8(1)))
						<< pMerch->nNum << pMerch->sCount << pMerch->nPrice << pMerch->isKC 
						<< pMerchant->m_curx << pMerchant->m_cury << pMerchant->m_curz;
				}
			}
			else if (pMerchantBot != nullptr)
			{
				for (int j = 0; j < MAX_MERCH_ITEMS; j++)
				{
					_MERCH_DATA * pMerchbot = &pMerchantBot->m_arMerchantItems[j];
					if (pMerchbot->nNum == 0 || pMerchbot->nPrice == 0)
						continue;

					result << pMerchantBot->GetID() << uint32(UserList) << pMerchantBot->GetName()
						<< uint8((pMerchantBot->isSellingMerchant() ? uint8(0) : uint8(1)))
						<< pMerchbot->nNum << pMerchbot->sCount << pMerchbot->nPrice << pMerchbot->isKC
						<< pMerchantBot->m_curx << pMerchantBot->m_cury << pMerchantBot->m_curz;
				}
			}
		}

		Send(&result);
	}
	else if (subCode == 1)
	{
		string target;
		pkt >> target;

		CUser * pUser = g_pMain->GetUserPtr(target, NameType::TYPE_CHARACTER);
		CBot * pBotUser = g_pMain->GetBotPtr(target, NameType::TYPE_CHARACTER);
		if (pUser != nullptr && pUser->isMerchanting())
			ZoneChange(pUser->m_bZone, pUser->m_curx, pUser->m_curz);
		else if (pBotUser != nullptr && pBotUser->isMerchanting())
			ZoneChange(pBotUser->m_bZone, pBotUser->m_curx, pBotUser->m_curz);
	}
	else if (subCode == 2)
	{
		string target;
		pkt >> target;

		CUser * pUser = g_pMain->GetUserPtr(target, NameType::TYPE_CHARACTER);
		CBot * pBotUser = g_pMain->GetBotPtr(target, NameType::TYPE_CHARACTER);
		if (pUser != nullptr)
		{
			Packet PM;
			string msg = " ";
			ChatPacket::Construct(&PM, (uint8)ChatType::PRIVATE_CHAT, &msg, &target);
			Send(&PM);
		}
		else if (pBotUser != nullptr)
		{
			Packet PM;
			string msg = " ";
			ChatPacket::Construct(&PM, (uint8)ChatType::PRIVATE_CHAT, &msg, &target);
			Send(&PM);
		}
	}
}

void CUser::MyInfo()
{
	C3DMap * pMap = GetMap();
	if (pMap == nullptr)
		return;

	CKnights * pKnights = nullptr;
	if (!pMap->IsValidPosition(GetX(), GetZ(), 0.0f))
	{
		short x = 0, z = 0;
		GetStartPosition(x, z);

		m_curx = (float)x;
		m_curz = (float)z;
	}

	AchieveNormalCountAdd(UserAchieveNormalTypes::AchieveBecomeKing, 0, nullptr);

	Packet result(WIZ_MYINFO);

	// Load up our user rankings (for our NP symbols).
	g_pMain->GetUserRank(this);

	// Are we the King? Let's see, shall we?
	CKingSystem * pData = g_pMain->m_KingSystemArray.GetData(GetNation());
	if (pData != nullptr && STRCASECMP(pData->m_strKingName.c_str(), m_strUserID.c_str()) == 0)
		m_bRank = 1; // We're da King, man.
	else
		m_bRank = 0; // totally not da King.

	result.SByte(); // character name has a single byte length
	result << GetSocketID()
		<< GetName()
		<< GetSPosX()
		<< GetSPosZ()
		<< GetSPosY()
		<< GetNation()
		<< m_bRace
		<< m_sClass
		<< m_bFace
		<< m_nHair
		<< m_bRank
		<< m_bTitle
		<< m_bIsHidingHelmet
		<< m_bIsHidingCospre
		<< GetLevel()
		<< m_sPoints
		<< m_iMaxExp
		<< m_iExp
		<< GetLoyalty()
		<< GetMonthlyLoyalty()
		<< GetClanID()
		<< GetFame();

	if (isInClan())
		pKnights = g_pMain->GetClanPtr(GetClanID());

	if (pKnights == nullptr)
	{
		if (isKing())
		{
			if (GetNation() == (uint8)Nation::ELMORAD)
				result << uint64(0) << uint16(KingCapeType::KNIGHTS_HUMAN_KING_CAPE) << uint32(0);
			else
				result << uint64(0) << uint16(KingCapeType::KNIGHTS_KARUS_KING_CAPE) << uint32(0);
		}
		else
			result << uint64(0) << uint16(-1) << uint32(0);
	}
	else
	{
		result << pKnights->GetAllianceID() << pKnights->m_byFlag << pKnights->GetName() << pKnights->m_byGrade << pKnights->m_byRanking << pKnights->m_sMarkVersion;

		CKnights * pMainClan = g_pMain->GetClanPtr(pKnights->GetAllianceID());
		_KNIGHTS_ALLIANCE * pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());

		if (pKnights->isInAlliance() && pMainClan != nullptr && pAlliance != nullptr)
		{
			if (isKing())
			{
				if (GetNation() == (uint8)Nation::ELMORAD)
					result << uint16(KingCapeType::KNIGHTS_HUMAN_KING_CAPE) << uint32(0);
				else
					result << uint16(KingCapeType::KNIGHTS_KARUS_KING_CAPE) << uint32(0);
			}
			else
			{
				if (pMainClan->isCastellanCape()) {
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						result << pMainClan->m_castCapeID << pMainClan->m_bCastCapeR << pMainClan->m_bCastCapeG << pMainClan->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						result << pMainClan->m_castCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
					else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
						result << pMainClan->m_castCapeID << uint32(0); // only the cape will be present
				}
				else {
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						result << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						result << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
					else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
						result << pMainClan->GetCapeID() << uint32(0); // only the cape will be present
				}
			}
		}
		else
		{
			if (isKing())
			{
				if (GetNation() == (uint8)Nation::ELMORAD)
					result << uint16(KingCapeType::KNIGHTS_HUMAN_KING_CAPE) << uint32(0);
				else
					result << uint16(KingCapeType::KNIGHTS_KARUS_KING_CAPE) << uint32(0);
			}
			else
			{
				if (pKnights->isCastellanCape())
					result << pKnights->m_castCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
				else
					result << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(0));
			}
		}
	}
	result << uint8(0) << uint8(0) << uint8(0) << uint8(0) // unknown
		<< m_MaxHp << m_sHp
		<< m_MaxMp << m_sMp
		<< m_sMaxWeight << m_sItemWeight
		<< GetStat(StatType::STAT_STR) << uint8(GetStatItemBonus(StatType::STAT_STR))
		<< GetStat(StatType::STAT_STA) << uint8(GetStatItemBonus(StatType::STAT_STA))
		<< GetStat(StatType::STAT_DEX) << uint8(GetStatItemBonus(StatType::STAT_DEX))
		<< GetStat(StatType::STAT_INT) << uint8(GetStatItemBonus(StatType::STAT_INT))
		<< GetStat(StatType::STAT_CHA) << uint8(GetStatItemBonus(StatType::STAT_CHA))
		<< m_sTotalHit << m_sTotalAc
		<< uint8(m_sFireR) << uint8(m_sColdR) << uint8(m_sLightningR)
		<< uint8(m_sMagicR) << uint8(m_sDiseaseR) << uint8(m_sPoisonR)
		<< m_iGold
		<< m_bAuthority;
	if (GetLevel() < 30)
	{
		result << int8(150) << int8(150);
	}
	else
	{
		result << (m_bKnightsRank <= m_bPersonalRank ? m_bKnightsRank : int8(-1))		// national rank, leader rank
			<< (m_bPersonalRank <= m_bKnightsRank ? m_bPersonalRank : int8(-1));	// national rank, leader rank
	}


	result.append(m_bstrSkill, 9);

	for (int i = 0; i < INVENTORY_TOTAL; i++) {
		_ITEM_DATA *pItem = GetItem(i);
		if (pItem == nullptr) continue;

		result << pItem->nNum
			<< pItem->sDuration
			<< pItem->sCount
			<< pItem->bFlag
			<< pItem->sRemainingRentalTime;	// remaining time

		_ITEM_TABLE pItemTable = g_pMain->GetItemPtr(pItem->nNum);
		if (!pItemTable.isnull())
		{
			if (pItemTable.isPetItem())
				ShowPetItemInfo(result, pItem->nSerialNum);
			else if (pItemTable.Getnum() == ITEM_CYPHER_RING)
				ShowCyperRingItemInfo(result, pItem->nSerialNum);
			else
				result << uint32(0); // Item Unique ID
		}
		else
			result << uint32(0); // Item Unique ID*/

		result << pItem->nExpirationTime; // expiration date in unix time
	}
	
	result.SByte();
	m_bIsChicken = GetLevel() < 30 ? true : false;// CheckExistEvent(50, 1);
	result << uint8(m_bAccountStatus)	// account status (0 = none, 1 = normal prem with expiry in hours, 2 = pc room)
		<< uint8(m_PremiumMap.GetSize());

	foreach_stlmap(itr, m_PremiumMap)
	{
		_PREMIUM_DATA * pPreData = itr->second;
		if (pPreData == nullptr
			|| pPreData->iPremiumTime == 0)
			continue;

		uint32 TimeRest;
		uint16 TimeShow;
		TimeRest = uint32(pPreData->iPremiumTime - UNIXTIME);

		if (TimeRest >= 1 && TimeRest <= 3600)
			TimeShow = 1;
		else
			TimeShow = TimeRest / 3600;

		result << pPreData->bPremiumType
			<< TimeShow;
	}
	result << m_bPremiumInUse
		<< uint8(m_bIsChicken) // chicken/beginner flag
		<< m_iMannerPoint
		<< g_pMain->m_sKarusMilitary
		<< g_pMain->m_sHumanMilitary
		<< g_pMain->m_sKarusEslantMilitary
		<< g_pMain->m_sHumanEslantMilitary
		<< g_pMain->m_sMoradonMilitary
		<< uint8(0)
		<< uint16(GetGenieTime())
		<< GetRebirthLevel()
		<< GetRebStatBuff(StatType::STAT_STR)
		<< GetRebStatBuff(StatType::STAT_STA)
		<< GetRebStatBuff(StatType::STAT_DEX)
		<< GetRebStatBuff(StatType::STAT_INT)
		<< GetRebStatBuff(StatType::STAT_CHA)
		<< uint64(m_iSealedExp)
		<< uint16(m_sCoverTitle)
		<< uint16(m_sSkillTitle)
		<< ReturnSymbolisOK
		<< m_bIsHidingWings;

	SendCompressed(&result);
	SetZoneAbilityChange(GetZoneID());

	if (g_pMain->ClanPrePazar && sClanPremStatus)
		m_bPremiumMerchant = true; 
}

void CUser::XSafe_Reset(Packet& pkt) 
{
	if (!isInGame() || isMerchanting()
		|| isMining() || isDead() || isFishing()
		|| GetZoneID() == ZONE_CHAOS_DUNGEON
		|| GetZoneID() == ZONE_KNIGHT_ROYALE
		|| GetZoneID() == ZONE_DUNGEON_DEFENCE
		|| GetZoneID() == ZONE_PRISON)
		return;

	uint8 subCode = pkt.read<uint8>();
	if (subCode == 1)  AllSkillPointChange();
	else if(subCode == 2) AllPointChange();
}

void CUser::XSafe_StayAlive(Packet & pkt)
{
	if (isCheckDecated)
		return;

	// Spawn sonrasi 5 saniye grace period — client henuz tam yuklenmediyse false positive onle
	if (m_bSelectedCharacter && (UNIXTIME2 - m_lastZoneChangeTime) < 5000)
		return;

	uint8 ischeckdecated2;
	uint32 clock1, clock2, clock3;
	std::string public_key, uPublic_key;

	std::string accountid = GetAccountName();

	pkt.DByte();
	pkt >>  clock1 >> clock2 >> uPublic_key >> ischeckdecated2 >> clock3;
	if(accountid.size()) STRTOUPPER(accountid);

	// #242 idle-FP kapisi (2026-06-10): multibox arka-plan client paket uretmez -> client
	// Real_SendTime bayatlar -> heartbeat cheat-flag/stale-clock1 tasir = FALSE POSITIVE.
	// Gercek hile (Real_Send unhook / injected send / overlay) OYNAYAN client'ta olur ->
	// server XSafe-disi paket ALIR (<5sn) -> kapi ACIK -> DC aynen calisir.
	// 5sn < heartbeat periyodu (7sn): idle'da stale heartbeat daima kapi DISINDA kalir.
	// Client ischeatactive STICKY (hic sifirlanmaz) -> tolere edilen hileci harekete gecince DC.
	bool bRecentRealPacket = (m_lastRealPacketTime != 0 && (UNIXTIME2 - m_lastRealPacketTime) <= 5000);

	if (ischeckdecated2) {
		if (bRecentRealPacket) {
			isCheckDecated = true;
			Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CheatLog));
			g_pMain->AddDatabaseRequest(newpkt, this);
			return goDisconnect("checkdecated1", __FUNCTION__);
		}
		// idle tolerans: DC yok, iz birak (test dogrulamasi + gercek-hile takip)
		LOG_HACK("XSafe TOLERATED (idle) cheatflag: %s (%s) lastRealPkt=%llums ago IP=%s",
			GetName().c_str(), GetAccountName().c_str(),
			(ULONGLONG)(UNIXTIME2 - m_lastRealPacketTime), GetRemoteIP().c_str());
	}

	public_key = md5("1X" + std::to_string(XSafe_VERSION) + "10001" + std::to_string(clock1) + std::to_string(ischeckdecated2) + accountid);

	if (public_key.empty() || uPublic_key.empty())
		return goDisconnect("heart beat md5 encrypt error", __FUNCTION__);

	if (m_bSelectedCharacter && public_key != uPublic_key) {
		g_pMain->SendHelpDescription(this, "Version mismatch. Please update your game or reinstall.");
		return goDisconnect("heart beat md5 encrypt2 error", __FUNCTION__);
	}

	if (isInGame())
	{
		if (lastTickTime == clock1)
		{
			// #242: ayni kapi — idle'da ayni clock1 BEKLENEN durum (paket yok = Real_SendTime sabit)
			if (bRecentRealPacket)
			{
				Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CheatLog));
				g_pMain->AddDatabaseRequest(newpkt, this);
				return goDisconnect("lastticktime1", __FUNCTION__);
			}
			LOG_HACK("XSafe TOLERATED (idle) staletick: %s (%s) lastRealPkt=%llums ago IP=%s",
				GetName().c_str(), GetAccountName().c_str(),
				(ULONGLONG)(UNIXTIME2 - m_lastRealPacketTime), GetRemoteIP().c_str());
		}
		else lastTickTime = clock1;
	}

	XSafe_LASTCHECK = UNIXTIME;
}

void CUser::XSafe_ReqUserInfo(Packet& pkt) 
{
	uint32 moneyReq = 0;
	if (GetPremium() == 12)
		moneyReq = 0;
	else
	{
		moneyReq = (int)pow((GetLevel() * 2.0f), 3.4f);
		if (GetLevel() < 30)
			moneyReq = (int)(moneyReq * 0.4f);
		else if (GetLevel() >= 60)
			moneyReq = (int)(moneyReq * 1.5f);

		if ((g_pMain->m_sDiscount == 1
			&& g_pMain->m_byOldVictory == GetNation())
			|| g_pMain->m_sDiscount == 2)
			moneyReq /= 2;
	}

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::USERINFO) << uint32(m_nKnightCash) << m_nTLBalance << uint16(m_sDaggerR) << uint16(m_sAxeR) << uint16(m_sSwordR) << uint16(m_sClubR) << uint16(m_sSpearR) << uint16(m_sBowR) << uint16(m_sJamadarR) << uint32(moneyReq);
	result << m_flameilevel;
	Send(&result);
}

void CUser::XSafe_AuthInfo(Packet & pkt) 
{
	uint8 u_auth;
	pkt >> u_auth;
	if (&u_auth != nullptr) 
	{
		if (u_auth == (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER && !isGM())
		{
			string graphicCards, processor, ip;
			pkt >> graphicCards >> processor;
			ip = GetRemoteIP();
			printf("%s is currently disconnect due to editing memory.", GetName().c_str());
			g_pMain->SendFormattedNotice("%s is currently disconnect due to editing memory.", (uint8)Nation::ALL, GetName().c_str());
			Disconnect();
		}
	}
	else 
		printf("%s is currently disconnect due to editing memory2.", GetName().c_str());
}

void CUser::XSafe_ProcInfo(Packet& pkt)
{
	bool senddesc = false;
	uint16 whoID = 0;
	pkt >> whoID;
	CUser* toWHO = nullptr;

	if(whoID) 
		toWHO = g_pMain->GetUserPtr(whoID);
	
	if (toWHO == nullptr) //17.03.2024
		return;

	if (toWHO->isGM())
		senddesc = true;

	vector<ProcessInfo> process;
	uint32 size;
	pkt >> size;
	// FIX K1 (S131): client-kontrollu 'size'/'windowsize' icin ust sinir YOKTU. ByteBuffer
	// read tukenince exception ATMAZ (0 doner, _rpos koşulsuz ilerler) -> dongu size kez doner.
	// size=0xFFFFFFFF -> milyarlarca push_back -> OOM/CPU -> sunucu-geneli DoS (tek paketle).
	// Cap + buffer-tukendi guard ekle (HandleChestBlock:2097 clamp deseni). Mesru proc-info << 1024.
	if (size > 1024)
		return;
	for (uint32 i = 0; i < size; i++)
	{
		if (pkt.rpos() >= pkt.size()) // paket bitti -> bos eleman uretme, kir
			break;

		int id, windowsize;
		string name;
		vector<string> windows;
		pkt >> id >> name >> windowsize;

		if (windowsize < 0 || windowsize > 256) // ikinci OOM vektoru + signed-int guard
			windowsize = 0;

		for (int j = 0; j < windowsize; j++)
		{
			if (pkt.rpos() >= pkt.size())
				break;
			string windowtitle;
			pkt >> windowtitle;
			windows.push_back(windowtitle);
		}
		process.push_back(ProcessInfo(id, name, windows));
	}

	if (senddesc)
		g_pMain->SendHelpDescription(toWHO, string_format("----- [%s] Processes ----", GetName().c_str()));

	//printf("\n----- [%s] Processes ----\n", GetName().c_str());
	for (ProcessInfo proc : process)
	{
		//if (senddesc) g_pMain->SendHelpDescription(toWHO, string_format("Name: %s - ID : %d - Windows : %d", proc.name.c_str(), proc.id, proc.windows.size()));
		for (string window : proc.windows)
		{
			if (senddesc) g_pMain->SendHelpDescription(toWHO, string_format("    -- %s", window.c_str()));
			//printf("    -- %s\n", window.c_str());
		}
	}

	if (senddesc)
		g_pMain->SendHelpDescription(toWHO, "----- End of processes ----");
	//printf("----- End of processes ----\n\n");
}

void CUser::XSafe_SendProcessInfoRequest(CUser* toWHO) 
{
	Packet pkt(XSafe);
	pkt << uint8(XSafeOpCodes::PROCINFO);

	if (toWHO != nullptr)
		pkt << uint16(toWHO->GetID());
	else
		pkt << uint16(0);
	
	Send(&pkt);
}

void CUser::XSafe_Log(Packet& pkt)
{
	string log, graphicCards, processor, ip;
	pkt >> log >> graphicCards >> processor;
	ip = GetRemoteIP();

	// HACK log dosyaya yaz (kanit + GM gorunur). HACK_YYYY-MM-DD.log
	LOG_HACK("[XGuard] %s | char=%s acc=%s ip=%s gpu=%s cpu=%s",
		log.c_str(), GetName().c_str(), GetAccountName().c_str(), ip.c_str(),
		graphicCards.c_str(), processor.c_str());

	// MRX Makro tespiti: code.guard "[MRX]" etiketli log gonderir -> OTOMATIK HWID ban.
	// (code.guard sahte svchost / interception driver / MRX.exe yakalaninca LM_Shutdown([MRX]...))
	// HwidBanByAccount: 0=ban OK, 1=HWID log yok (yeni launcher login olmadi), 2=zaten banli.
	if (log.find("[MRX]") != std::string::npos)
	{
		std::string strAccountID = GetAccountName();
		if (!strAccountID.empty())
		{
			extern CDBAgent g_DBAgent;
			std::string strReason = string_format("MRX Makro otomatik tespit (XGuard): %s", log.c_str());
			int ret = g_DBAgent.HwidBanByAccount(strAccountID, strReason, "XGuard-AUTO");
			if (ret == 0)
			{
				LOG_HACK("[XGuard] MRX OTOMATIK HWID BAN: %s (acc=%s)", GetName().c_str(), strAccountID.c_str());
				std::string notice = string_format("%s MRX makro kullandigi icin PC (HWID) banlandi.", GetName().c_str());
				g_pMain->SendNotice(notice.c_str(), (uint8)Nation::ALL);
			}
			else
			{
				LOG_HACK("[XGuard] MRX HWID ban atlandi (ret=%d) char=%s acc=%s", ret, GetName().c_str(), strAccountID.c_str());
			}
		}
	}
}

void CUser::XSafe_Support(Packet& pkt)
{
	if (XSafe_LASTSUPPORT + (XSafe_SUPPORT_CHEK) >= UNIXTIME)
		return;

	uint8 subCode;
	pkt >> subCode;

	string subject, message, ReportType;
	pkt >> subject >> message;
	if (subject.size() > 25 || message.size() > 40)
		return;

	if (subCode == 0x00) ReportType = "Bug";
	else if (subCode == 0x01) ReportType = "Koxp";

	XSafe_LASTSUPPORT = UNIXTIME;
	Packet report(WIZ_DB_SAVE_USER,uint8(ProcDbType::UserReportDbSave));
	report << GetName() << ReportType << subject << message;
	g_pMain->AddDatabaseRequest(report,this);
}

void CUser::XSafe_UIRequest(Packet& pkt)
{
	int moneyReq = 0;
	if (GetPremium() == 12)
		moneyReq = 0;
	else
	{
		moneyReq = (int)pow((GetLevel() * 2.0f), 3.4f);
		if (GetLevel() < 30)
			moneyReq = (int)(moneyReq * 0.4f);
		else if (GetLevel() >= 60)
			moneyReq = (int)(moneyReq * 1.5f);

		if ((g_pMain->m_sDiscount == 1 && g_pMain->m_byOldVictory == GetNation()) || g_pMain->m_sDiscount == 2)
			moneyReq /= 2;
	}

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::UIINFO) << uint32(m_nKnightCash) << m_nTLBalance << uint16(m_sDaggerR) << uint16(m_sAxeR) << uint16(m_sSwordR) << uint16(m_sClubR) << uint16(m_sSpearR) << uint16(m_sBowR) << uint16(m_sJamadarR) << uint32(moneyReq);
	result << m_iExp << m_iMaxExp << GetSocketID() << GetName() << GetClass() << GetRace() << GetLevel();
	for (uint8 i = 0; i < (uint8)StatType::STAT_COUNT; i++) result << m_bStats[i];

	uint8 r = GetRValue(GetTagNameColour()), g = GetGValue(GetTagNameColour()), b = GetBValue(GetTagNameColour());
	result << mytagname << r << g << b << GetZoneID() << m_flameilevel;
	Send(&result);

	Packet pus(XSafeOpCodes::PUS);
	pus << uint8(0);
	XSafe_PusRequest(pus);

	XSafe_SendTempItems();
}

void CUser::XSafe_SendTempItems()
{
	if (!TempItemsControl)
	{
		Packet result(XSafe);
		result << uint8(XSafeOpCodes::TEMPITEMS);
		vector<_TempItem> items;

		for (int Inventory = SLOT_MAX; Inventory < INVENTORY_COSP; Inventory++)
		{
			auto * pData = GetItem(Inventory);
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 0;
			item.nItemID = pData->nNum;
			item.pos = Inventory;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int Cospre = INVENTORY_COSP; Cospre < INVENTORY_MBAG; Cospre++)
		{
			auto * pData = GetItem(Cospre);
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 1;
			item.nItemID = pData->nNum;
			item.pos = Cospre;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int MagicBag = INVENTORY_MBAG1; MagicBag < INVENTORY_MBAG2; MagicBag++)
		{
			auto * pData = GetItem(MagicBag);
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 2;
			item.nItemID = pData->nNum;
			item.pos = MagicBag;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int Warehouse = 0; Warehouse < WAREHOUSE_MAX; Warehouse++)
		{
			auto * pData = &m_sWarehouseArray[Warehouse];
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 3;
			item.nItemID = pData->nNum;
			item.pos = Warehouse;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int Vip = 0; Vip < VIPWAREHOUSE_MAX; Vip++)
		{
			auto * pData = &m_sVIPWarehouseArray[Vip];
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 4;
			item.nItemID = pData->nNum;
			item.pos = Vip;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		result << uint8(items.size());
		foreach(itr, items)
			result << itr->slot << itr->nItemID << itr->pos << itr->time;

		Send(&result);
		TempItemsControl = true;
	}
}

void CUser::XSafe_SaveLootSettings(Packet & pkt) 
{
	uint8 iWeapon, iArmor, iAccessory, iNormal, iUpgrade, iCraft, iRare, iMagic, iUnique, iConsumable;
	uint32 iPrice = 0;

	pkt >> iWeapon >> iArmor >> iAccessory >> iNormal >> iUpgrade >> iCraft >> iRare >> iMagic >> iUnique >> iConsumable >> iPrice;

	if (iWeapon > 1 || iWeapon < 0) iWeapon = 0;
	if (iArmor > 1 || iArmor < 0) iArmor = 0;
	if (iAccessory > 1 || iAccessory < 0) iAccessory = 0;

	if (iNormal > 1 || iNormal < 0) iNormal = 0;
	if (iUpgrade > 1 || iUpgrade < 0) iUpgrade = 0;
	if (iCraft > 1 || iCraft < 0) iCraft = 0;
	if (iRare > 1 || iRare < 0) iRare = 0;
	if (iMagic > 1 || iMagic < 0) iMagic = 0;
	if (iUnique > 1 || iUnique < 0) iUnique = 0;

	if (iConsumable > 1 || iConsumable < 0) iConsumable = 0;

	if (iPrice > 999999999) iPrice = 999999999;

	pUserLootSetting.iWeapon = iWeapon;
	pUserLootSetting.iArmor = iArmor;
	pUserLootSetting.iAccessory = iAccessory;
	pUserLootSetting.iNormal = iNormal;
	pUserLootSetting.iUpgrade = iUpgrade;
	pUserLootSetting.iCraft = iCraft;
	pUserLootSetting.iRare = iRare;
	pUserLootSetting.iMagic = iMagic;
	pUserLootSetting.iUnique = iUnique;
	pUserLootSetting.iConsumable = iConsumable;
	pUserLootSetting.iPrice = iPrice;
}

void CUser::XSafe_ReqUserData() 
{
	Packet ret(XSafe);
	ret << uint8(XSafeOpCodes::USERDATA) << GetSocketID() << GetName() << GetClass() << GetRace() << GetLevel() << m_bStats[0] << m_bStats[1] << m_bStats[2] << m_bStats[3] << m_bStats[4];
	Send(&ret);
}

void CUser::XSafe_Merchant(Packet & pkt) 
{
	if (!isSellingMerchantingPreparing() || isBuyingMerchantingPreparing())
		return;

	_ITEM_TABLE pTable = _ITEM_TABLE();
	uint8 subCode;
	pkt >> subCode;

	if (subCode == MERCHANT_ITEM_ADD)
	{
		Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_ADD));
		uint32 nGold, nItemID;
		uint16 sCount;
		uint8 bSrcPos, bDstPos, bMode, isKC;

		pkt >> nItemID >> sCount >> nGold >> bSrcPos >> bDstPos >> bMode >> isKC;
		// TODO: Implement the possible error codes for these various error cases.
		if (!nItemID || !sCount || bSrcPos >= HAVE_MAX || bDstPos >= MAX_MERCH_ITEMS) goto fail_return;

		for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			if (m_arMerchantItems[i].nNum && m_arMerchantItems[i].bOriginalSlot == (bSrcPos + SLOT_MAX))
				goto fail_return;

		uint16 minreqcash = g_pMain->pServerSetting.MinKnightCash;
		if (isKC && minreqcash && nGold < minreqcash) {
			g_pMain->SendHelpDescription(this, string_format("You must enter a minimum of %d Knight Cash to establish a market.", minreqcash).c_str());
			goto fail_return;
		}

		_ITEM_DATA* pSrcItem = GetItem(bSrcPos + SLOT_MAX);
		pTable = g_pMain->GetItemPtr(nItemID);

		if (pTable.isnull() || !pSrcItem)
			goto fail_return;

		if (nItemID >= ITEM_NO_TRADE_MIN && nItemID <= ITEM_NO_TRADE_MAX // Cannot be traded, sold or stored.
			|| pTable.m_bRace == RACE_UNTRADEABLE // Cannot be traded or sold.
			|| pTable.m_bCountable == 2
			|| (pTable.m_bKind == 255 && sCount != 1)) // Cannot be traded or sold.
			goto fail_return;

		if (pTable.m_bCountable == 1 && sCount > MAX_ITEM_COUNT) goto fail_return;

		if (pSrcItem->nNum != nItemID
			|| pSrcItem->sCount == 0
			|| sCount == 0
			|| pSrcItem->sCount < sCount
			|| pSrcItem->isRented()
			|| pSrcItem->isSealed()
			|| pSrcItem->isBound()
			|| pSrcItem->isDuplicate()
			|| pSrcItem->isExpirationTime())
			goto fail_return;

		_MERCH_DATA* pMerch = &m_arMerchantItems[bDstPos];
		if (pMerch == nullptr || pMerch->nNum != 0)
			goto fail_return;

		pSrcItem->MerchItem = true;

		pMerch->nNum = nItemID;
		pMerch->nPrice = nGold;
		pMerch->sCount = sCount; // Selling Count
		pMerch->orgcount = pSrcItem->sCount;

		pMerch->sDuration = pSrcItem->sDuration;
		pMerch->nSerialNum = pSrcItem->nSerialNum; // NOTE: Stackable items will have an issue with this.
		pMerch->bOriginalSlot = bSrcPos + SLOT_MAX;
		pMerch->IsSoldOut = false;
		pMerch->isKC = isKC;

		if (pTable.m_bKind == 255 && !pTable.m_bCountable) pMerch->sCount = 1;

		result << uint16(1) << nItemID << sCount << pMerch->sDuration << nGold << bSrcPos << bDstPos;
		Send(&result);
		return;

	fail_return:
		result << uint16(0) << nItemID << sCount << (uint16)bSrcPos + bDstPos << nGold << bSrcPos << bDstPos;
		Send(&result);
	}
}

void CUser::SendChaoticResult(uint8 result) {
	Packet newpkt(XSafe, uint8(XSafeOpCodes::CHAOTIC_EXCHANGE));
	newpkt << result;
	Send(&newpkt);
}

void CUser::XSafe_ChaoticExchange(Packet & pkt)
{
	uint16 sNpcID; uint32 nExchangeItemID; uint8 bSrcPos, errorcode = 2, bank = 0, sell = 0, count = 0;
	pkt >> sNpcID >> nExchangeItemID >> bSrcPos >> bank >> sell >> count;

	uint32 coinsreq = g_pMain->pServerSetting.chaoticcoins;
	if (coinsreq && !hasCoins(coinsreq)) {
		g_pMain->SendHelpDescription(this, string_format("You need %d coins to do the exchange process.", coinsreq));
		return BifrostPieceSendFail(errorcode);
	}

	if (count > 100) {
		g_pMain->SendHelpDescription(this, "maximum number of pieces 100.");
		return BifrostPieceSendFail(errorcode);
	}

	bool multiple = count > 1 ? true : false;

	if (!CheckExistItem(nExchangeItemID, count)) {
		g_pMain->SendHelpDescription(this, "You are not equal to the sufficient number of items.");
		return BifrostPieceSendFail(errorcode);
	}

	for (int i = 0; i < count; i++) {

		Packet result(WIZ_ITEM_UPGRADE, (uint8)ItemUpgradeOpcodes::ITEM_BIFROST_EXCHANGE);

		if (!multiple) {
			if (m_BeefExchangeTime > UNIXTIME2 || m_sItemWeight >= m_sMaxWeight)
				return BifrostPieceSendFail(errorcode);
			m_BeefExchangeTime = UNIXTIME2 + 750;
		}
		
		CNpc* pNpc = g_pMain->GetNpcPtr(sNpcID, GetZoneID());
		if (pNpc == nullptr || !isInGame()
			|| !isInRange(pNpc, MAX_NPC_RANGE)
			|| isTrading() || isMerchanting()
			|| isSellingMerchantingPreparing() || isMining()
			|| isFishing() || isDead() || !isInMoradon())
			return BifrostPieceSendFail(errorcode);

		if (pNpc->GetType() != NPC_CHAOTIC_GENERATOR && pNpc->GetType() != NPC_CHAOTIC_GENERATOR2)
			return BifrostPieceSendFail(errorcode);

		auto pTable = g_pMain->GetItemPtr(nExchangeItemID);
		if (pTable.isnull() || !pTable.m_bCountable || pTable.m_iEffect2 != 251)
			return BifrostPieceSendFail(errorcode);

		auto* pItem = GetItem(SLOT_MAX + bSrcPos);
		if (pItem == nullptr || bSrcPos >= HAVE_MAX
			|| pItem->nNum != nExchangeItemID
			|| !pItem->sCount || pItem->isRented()
			|| pItem->isSealed() || pItem->isDuplicate())
			return BifrostPieceSendFail(errorcode);

		uint8 bFreeSlots = 0;
		for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			if (GetItem(i)->nNum == 0 && ++bFreeSlots >= ITEMS_IN_EXCHANGE_GROUP) break;

		if (bFreeSlots < 1) {
			g_pMain->SendHelpDescription(this, "there is not enough space left.");
			return BifrostPieceSendFail(errorcode);
		}

		std::vector<_ITEM_EXCHANGE> mlist;
		foreach_stlmap(itr, g_pMain->m_ItemExchangeArray) {
			if (itr->second == nullptr) continue;
			if (itr->second->bRandomFlag != 1 && itr->second->bRandomFlag != 2 && itr->second->bRandomFlag != 3) continue;
			if (itr->second->nOriginItemNum[0] == nExchangeItemID) mlist.push_back(*itr->second);
		}

		if (mlist.empty())
			return BifrostPieceSendFail(errorcode);

		int bRandArray[10000]{ 0 }; int offset = 0;
		memset(&bRandArray, 0, sizeof(bRandArray));
		foreach(itr, mlist) {
			if (!BifrostCheckExchange(itr->nIndex))
				return BifrostPieceSendFail(errorcode);

			if (itr->bRandomFlag >= 101
				|| !CheckExistItemAnd(itr->nOriginItemNum[0], itr->sOriginItemCount[0], 0, 0, 0, 0, 0, 0, 0, 0))
				continue;

			if (offset >= 9999) break;
			for (int i = 0; i < int(itr->sExchangeItemCount[0] / 5); i++) {
				if (i + offset >= 9999) break;
				bRandArray[offset + i] = itr->nExchangeItemNum[0];
			}
			offset += int(itr->sExchangeItemCount[0] / 5);
		}

		if (offset > 9999) offset = 9999;
		uint32 bRandSlot = myrand(0, offset);
		uint32 nItemID = bRandArray[bRandSlot];

		auto pGiveTable = g_pMain->GetItemPtr(nItemID);
		if (pGiveTable.isnull())
			return BifrostPieceSendFail(errorcode);

		bool seslling = false;
		if (sell && pGiveTable.Gettype() != 4) seslling = true;

		if (!seslling && !bank && pGiveTable.m_sWeight + m_sItemWeight >= m_sMaxWeight) {
			g_pMain->SendHelpDescription(this, "too much weight in your inventory.");
			return BifrostPieceSendFail(errorcode);
		}

		int8 sItemSlot = FindSlotForItem(nItemID, 1);
		if (sItemSlot < 1)
			return BifrostPieceSendFail(errorcode);

		if (!RobItem(SLOT_MAX + bSrcPos, pTable, 1))
			return BifrostPieceSendFail(errorcode);

		if (seslling) {
			uint32 transactionPrice = 0;
			if(pGiveTable.m_iSellNpcType == 1)
				transactionPrice = pGiveTable.m_iSellNpcPrice;
			else if (GetPremium() == 0 && pGiveTable.m_iSellPrice == SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice;
			else if (GetPremium() > 0 && pGiveTable.m_iSellPrice == SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice;
			else if (GetPremium() == 0 && pGiveTable.m_iSellPrice != SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice / 6;
			else if (GetPremium() > 0 && pGiveTable.m_iSellPrice != SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice / 4;

			if (pGiveTable.GetKind() == 255) transactionPrice = 1;

			if (GetCoins() + transactionPrice > COIN_MAX) {
				g_pMain->SendHelpDescription(this, "You have too many coins in your inventory.");
				return BifrostPieceSendFail(errorcode);
			}

			GoldGain(transactionPrice);
			SendChaoticResult(2);
			BifrostPieceSendFail(1);
			continue;
		}

		if (bank) {
			if (!CheckEmptyWareHouseSlot()) {
				g_pMain->SendHelpDescription(this, "There is not enough space in your bank.");
				return BifrostPieceSendFail(errorcode);
			}

			WareHouseAddItemProcess(nItemID, 1);
			SendChaoticResult(2);
			BifrostPieceSendFail(1);

			std::string message = string_format("%s has been stored.", pGiveTable.m_sName.c_str());
			SendAcsMessage(message);
			continue;
		}

		if (!GiveItem("Bifrost Exchange", nItemID, 1)) {
			errorcode = 0;
			return BifrostPieceSendFail(errorcode);
		}

		BeefEffectType beefEffectType = BeefEffectType::EffectNone;

		if (pGiveTable.m_ItemType == 4) beefEffectType = BeefEffectType::EffectWhite;
		else if (pGiveTable.m_ItemType == 5) beefEffectType = BeefEffectType::EffectGreen;
		else beefEffectType = BeefEffectType::EffectRed;
		
		result << uint8(1) << nItemID << int8(sItemSlot - SLOT_MAX) << nExchangeItemID << bSrcPos << (uint8)beefEffectType;
		Send(&result);
		
		SendChaoticResult(2);
		if (!multiple) {
			Packet newpkt(WIZ_OBJECT_EVENT, (uint8)OBJECT_ARTIFACT);
			newpkt << (uint8)beefEffectType << sNpcID;
			SendToRegion(&newpkt, nullptr, GetEventRoom());
		}
		if (pGiveTable.m_ItemType == 4 || pGiveTable.Getnum() == 379068000) LogosItemNotice(pGiveTable);

		if (coinsreq)
			GoldLose(coinsreq);

		BifrostPieceSendFail(2);
	}
}

void CUser::XSafe_PusRequest(Packet& pkt) 
{
	uint8 process;
	pkt >> process;
	switch (process)
	{
	case 0:
		XSafe_SendPUS();
		break;
	case 1:
		XSafe_PUSPurchase(pkt);
		break;
	case 2:
		break;
	case 3:
		PUSGiftPurchase(pkt);
		break;
	default:
		break;
	}

	if (process == 0) 
	{
		XSafe_SendPUS();
		return;
	}
	else if (process == 1) 
	{
		XSafe_PUSPurchase(pkt);
		return;
	}
	else if (process == 2) 
	{
		return;
	}
}

void CUser::XSafe_PUSPurchase(Packet & pkt) 
{
	if (pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID())) {
		g_pMain->SendHelpDescription(this, "You can't use power-up-store while Fun Class Event.");
		return;
	}
	if (isMerchanting())
	{
		g_pMain->SendHelpDescription(this, "You can't use power-up-store while merchanting.");
		return;
	}

	if (isTrading())
	{
		g_pMain->SendHelpDescription(this, "You can't use power-up-store while trading.");
		return;
	}

	if (isDead())
		return;

	uint32 item_id;
	uint8 count;
	pkt >> item_id >> count; 

	if (count > 28)
		return;

	_PUS_ITEM * item = g_pMain->m_PusItemArray.GetData(item_id);
	if (item == nullptr)
		return;

	if (count > 0 && item->Price > UINT32_MAX / count)
		return;

	uint32 totalCash = item->Price * count;

	if (!item->PriceType)
	{
		if (m_nKnightCash >= totalCash)
		{
			_ITEM_TABLE itemdata = g_pMain->GetItemPtr(item->ItemID);
			if (itemdata.isnull())
				return;

			uint32 sFreeSlot = 0;
			for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			{
				if (GetItem(i)->nNum == 0)
					sFreeSlot++;
			}

			if (sFreeSlot <= count)
				return;

			bool notice = true;
			for (size_t i = 1; i <= count; i++)
			{
				if (!GiveItem("Pus Repurchase", item->ItemID, item->BuyCount, true, 0, _giveitempusinfo(true, item->PriceType, item->Price)))
					continue;

				m_nKnightCash -= item->Price;

				if (notice)
					g_pMain->SendHelpDescription(this, string_format("You have purchased %s for %d Knight Cash.", itemdata.m_sName.c_str(), item->Price).c_str());

				Packet result(XSafe);
				result << uint8(XSafeOpCodes::CASHCHANGE) << uint32(m_nKnightCash) << uint32(m_nTLBalance);
				Send(&result);

				Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateKnightCash));
				g_pMain->AddDatabaseRequest(Save, this);

				notice = false;
			}
		}
	}
	else if (item->PriceType)
	{
		if (m_nTLBalance >= totalCash)
		{
			_ITEM_TABLE itemdata = g_pMain->GetItemPtr(item->ItemID);
			if (itemdata.isnull())
				return;

			uint32 sFreeSlot = 0;
			for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			{
				if (GetItem(i)->nNum == 0)
					sFreeSlot++;
			}

			if (sFreeSlot <= count)
				return;

			bool notice = true;
			for (size_t i = 1; i <= count; i++)
			{
				if (item->ItemID >= 489500000 && item->ItemID <= 489600000)
				{
					m_nTLBalance -= item->Price;
				
					GiveBalance(item->BuyCount);
					//g_DBAgent.UpdateKnightCash(this);
					Packet result(XSafe);
					result << uint8(XSafeOpCodes::CASHCHANGE) << uint32(m_nKnightCash) << uint32(m_nTLBalance);
					Send(&result);
					g_pMain->SendHelpDescription(this, string_format("1_You have purchased Change %d Balance to %d knight cash",item->Price, item->BuyCount).c_str());
					return;
				}

				if (!GiveItem("Pus Repurchase", item->ItemID, item->BuyCount, true, 0, _giveitempusinfo(true, item->PriceType, item->Price)))
					continue;

				m_nTLBalance -= item->Price;

				if (notice)
					g_pMain->SendHelpDescription(this, string_format("You have purchased %s for %d Balance Cash.", itemdata.m_sName.c_str(), item->Price).c_str());

				Packet result(XSafe);
				result << uint8(XSafeOpCodes::CASHCHANGE) << uint32(m_nKnightCash) << uint32(m_nTLBalance);
				Send(&result);

				Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateKnightCash));
				g_pMain->AddDatabaseRequest(Save, this);


				notice = false;
			}
		}
	}
}

void CUser::XSafe_SendPUS() 
{
	if (!PusArrayControl)
	{
		Packet result(XSafe);
		result << uint8(XSafeOpCodes::PUS);

		uint32 PusItemCount = g_pMain->m_PusItemArray.GetSize();

		result << PusItemCount;
		g_pMain->m_PusItemArray.m_lock.lock();
		foreach_stlmap_nolock(itr, g_pMain->m_PusItemArray)
		{
			if (itr->second == nullptr)
				continue;

			result << itr->second->ID << itr->second->ItemID << itr->second->Price << itr->second->Cat << itr->second->BuyCount << itr->second->PriceType;
		}
		g_pMain->m_PusItemArray.m_lock.unlock();

		Send(&result);

		if (true)
		{
			Packet result(XSafe);
			result << uint8(XSafeOpCodes::PusCat);


			result << uint32(g_pMain->m_PusCategoryArray.GetSize());

			foreach_stlmap_nolock(itr, g_pMain->m_PusCategoryArray)
			{
				if (itr->second == nullptr)
					continue;

				result << uint32(itr->second->ID) << itr->second->CategoryName  << itr->second->Status;
			}

			Send(&result);
		}
		PusArrayControl = true;
	}
	else
	{
		PusArrayControl = true;
		return;
	}
	PusArrayControl = true;
}

void CUser::XSafe_SendAliveRequest() 
{
	Packet pkt(XSafe);
	pkt << uint8(XSafeOpCodes::xALIVE) << uint8(1);
	Send(&pkt);
}

void CUser::XSafe_OpenWeb(std::string url) 
{
	Packet pkt(XSafe);
	pkt << uint8(XSafeOpCodes::OPEN) << url;
	Send(&pkt);
}

void CUser::XSafe_DropRequest(Packet& pkt) 
{
	Packet result(XSafe);
	
	uint8 command;
	pkt >> command;

	if (command == 1) 
	{
		uint8 iasMonster = 0;
		result << uint8_t(XSafeOpCodes::DROP_REQUEST) << uint8(1);
		uint32 target;
		uint8 isRandom = 0;
		pkt >> target;
		
		CNpc * pNpc = g_pMain->GetNpcPtr(m_targetID, GetZoneID());
		if (pNpc == nullptr)
			return;

		if (pNpc->isMonster()) iasMonster = 1;

		if (pNpc->isMonster()) {
			iasMonster = 1;
			_K_MONSTER_ITEM * pItem = g_pMain->m_MonsterItemArray.GetData(pNpc->GetProtoID());
			if (pItem != nullptr)
			{
				result << pNpc->GetProtoID();
				for (int j = 0; j < LOOT_DROP_ITEMS; j++)
					result << uint32(pItem->iItem[j]) << uint16(pItem->sPercent[j]);
			}
		}
		else {
			_K_NPC_ITEM * pItem = g_pMain->m_NpcItemArray.GetData(pNpc->GetProtoID());
			if (pItem != nullptr)
			{
				result << pNpc->GetProtoID();
				for (int j = 0; j < LOOT_DROP_ITEMS; j++)
					result << uint32(pItem->iItem[j]) << uint16(pItem->sPercent[j]);
			}
		}

		result << iasMonster;
		Send(&result);
	}
	else if (command == 2) 
	{
		result << uint8_t(XSafeOpCodes::DROP_LIST) << uint8(2);

		uint32 groupID;
		pkt >> groupID;

		_MAKE_ITEM_GROUP * itemGroup = g_pMain->m_MakeItemGroupArray.GetData(groupID);
		if (itemGroup != nullptr) 
		{
			result << uint8(itemGroup->iItems.size());
			for (uint8 i = 0; i < (uint8)itemGroup->iItems.size(); i++)
				result << itemGroup->iItems[i];
			
			Send(&result);
		}
	}
	else if (command == 3)
	{
		result << uint8_t(XSafeOpCodes::DROP_REQUEST) << uint8(1);
		uint16 mob;
		pkt >> mob;

		std::map<uint32, uint16> mlist; //13.03.2024 drop y�zde
		result << mob;

		_K_MONSTER_ITEM * pItem = g_pMain->m_MonsterItemArray.GetData(mob);
		if (pItem) {

			bool added_random = false;
			for (int j = 0; j < 12; j++) {
				if (!pItem->iItem[j])
					continue;
				
				if (pItem->iItem[j] < MIN_ITEM_ID) {
					
					if (!added_random) {
						g_pMain->m_MakeItemGroupRandomArray.m_lock.lock();
						foreach_stlmap_nolock(itr, g_pMain->m_MakeItemGroupRandomArray) {
							if (itr->second && itr->second->GroupNo == pItem->iItem[j]) {
								added_random = true;
								mlist.insert(std::make_pair(900004000,2500)); //13.03.2024 drop y�zde
								break;
							}
						}
						g_pMain->m_MakeItemGroupRandomArray.m_lock.unlock();
					}

					auto* itemGroup = g_pMain->m_MakeItemGroupArray.GetData(pItem->iItem[j]);
					if (!itemGroup || itemGroup->iItems.empty())
						continue;

					for (int i = 0; i < (int)itemGroup->iItems.size(); i++) {
						if (mlist.find(itemGroup->iItems[i]) == mlist.end()) //13.03.2024 drop y�zde
							mlist.insert(std::make_pair(itemGroup->iItems[i], pItem->sPercent[j])); //13.03.2024 drop y�zde
					}
				}
				else {
					if (mlist.find(pItem->iItem[j]) == mlist.end()) //13.03.2024 drop y�zde
						mlist.insert(std::make_pair(pItem->iItem[j], pItem->sPercent[j])); //13.03.2024 drop y�zde
				}
			}
		}
	
		/*if (mlist.empty()) 
			return;*/

		result << (uint32)mlist.size();
		for (auto it : mlist) result << it.first << (uint16)it.second; //13.03.2024 drop y�zde
		result << uint8(1);
		Send(&result);
	}
	else if (command == 4)
	{
		uint8 crSelect;
		pkt >> crSelect;
		result << uint8_t(XSafeOpCodes::DROP_REQUEST) << uint8(1);
		if (g_pMain->pCollectionRaceEvent.RewardItemID[crSelect] == 0)
			return;

		std::list<uint32> mlist;
		result << uint16(2);
		foreach(itr, g_pMain->m_RandomItemArray)
		{

			_RANDOM_ITEM* pRandom = *itr;
			if (pRandom->SessionID != g_pMain->pCollectionRaceEvent.RewardSession[crSelect])
				continue;
			mlist.push_back(pRandom->ItemID);
		
		}
		if (mlist.empty()) return;

		result << (uint32)mlist.size();
		for (auto it : mlist) result << it;
		result << uint8(2);
		Send(&result);
	}

}

void CUser::XSafe_Main() 
{
	if (!isInGame() || IsOffCharacter()) return;

	if (UNIXTIME - XSafe_LASTCHECK > XSafe_ALIVE_TIMEOUT)
	{
		if (XSafe_ACTIVE == 1)
		{
			Disconnect();
		}
	}
}

void CUser::XSafe_HandlePacket(Packet & pkt)
{
	uint8 SubOpCode;
	pkt >> SubOpCode;

	// SECURITY: Only allow handshake/heartbeat packets before login
	if (m_strAccountID.empty())
	{
		if (SubOpCode != XSafeOpCodes::xALIVE
			&& SubOpCode != XSafeOpCodes::AUTHINFO
			&& SubOpCode != 22)
			return;
	}

	switch (SubOpCode)
	{
	case XSafeOpCodes::PERKS:
		HandlePerks(pkt);
		break;
	case XSafeOpCodes::CINDIRELLA:
		CindirillaHandler(pkt);
		break;
	case XSafeOpCodes::xALIVE:
		XSafe_StayAlive(pkt);
		break;
	case XSafeOpCodes::AUTHINFO:
		XSafe_AuthInfo(pkt);
		break;
	case XSafeOpCodes::PROCINFO:
		XSafe_ProcInfo(pkt);
		break;
	case XSafeOpCodes::LOG:
		XSafe_Log(pkt);
		break;	
	case XSafeOpCodes::RIGHT_CLICK_EXCHANGE:
		HandleRightClickExchange(pkt);
		break;
	case XSafeOpCodes::UIINFO:
		XSafe_UIRequest(pkt);
		break;
	case XSafeOpCodes::PUS:
		XSafe_PusRequest(pkt);
		break;
	case XSafeOpCodes::RESET:
		XSafe_Reset(pkt);
		break;
	case XSafeOpCodes::DROP_LIST:
		XSafe_DropRequest(pkt);
		break;
	case XSafeOpCodes::DROP_REQUEST:
		XSafe_DropRequest(pkt);
		break;
	case XSafeOpCodes::USERINFO:
		XSafe_ReqUserInfo(pkt);
		break;
	case XSafeOpCodes::LOOT_SETTINS:
		XSafe_SaveLootSettings(pkt);
		break;
	case XSafeOpCodes::CHAOTIC_EXCHANGE:
		XSafe_ChaoticExchange(pkt);
		break;
	case XSafeOpCodes::MERCHANT:
		XSafe_Merchant(pkt);
		break;
	case XSafeOpCodes::TEMPITEMS:
		XSafe_SendTempItems();
		break;
	case XSafeOpCodes::SUPPORT:
		XSafe_Support(pkt);
		break;
	case XSafeOpCodes::MERCHANTLIST:
		XSafe_ReqMerchantList(pkt);
		break;
	case XSafeOpCodes::LIFESKILL:
		XSafe_HandleLifeSkill(pkt);
		break;
	case 22:
		XSafe_General(pkt);
		break;
	case XSafeOpCodes::LOTTERY:
		XSafe_LotteryJoinFunction(pkt);
		break;
	case XSafeOpCodes::RESETREBSTAT:
		{
			int32_t menuButtonText[MAX_MESSAGE_EVENT], menuButtonEvents[MAX_MESSAGE_EVENT];
			m_sEventNid = 14446;
			m_sEventSid = 18004;
			m_nQuestHelperID = 4324;
			foreach_array_n(i, menuButtonText, MAX_MESSAGE_EVENT)
			{
				menuButtonText[i] = -1;
			}

			foreach_array_n(i, menuButtonEvents, MAX_MESSAGE_EVENT)
			{
				menuButtonEvents[i] = -1;
			}

			SelectMsg(52, -1, -1, menuButtonText, menuButtonEvents);
		}
		break;
	case XSafeOpCodes::ACCOUNT_INFO_SAVE:
		XSafe_AccountInfoSave(pkt);
		break;
	case XSafeOpCodes::CHAT_LASTSEEN:
		XSafe_LastSeenProcess(pkt);
		break;
	case XSafeOpCodes::SendRepurchaseMsg:
		XSafe_SendRepurchaseMsg(pkt);
		break;
	case XSafeOpCodes::Send1299SkillAndStatReset:
		XSafe_Send1299SkillAndStatReset(pkt);
		break;
	case XSafeOpCodes::TagInfo:
		HandleTagChange(pkt);
		break;
	case XSafeOpCodes::PusRefund:
		HandleItemReturn(pkt);
		break;
	case XSafeOpCodes::WheelData:
		WheelOfFun(pkt);
		break;
	case XSafeOpCodes::CHEST_BLOCKITEM:
		HandleChestBlock(pkt);
		break;
	default:
		printf("XSafe_HandlePacket unhandled opcode %d\n", SubOpCode);
		break;
	}
}

bool CUser::CheckChestBlockItem(uint32 itemid)
{
	if (m_reloadChestBlock)
		return false;

	Guard lock(mChestBlockItemLock);
	return std::find(mChestBlockItem.begin(), mChestBlockItem.end(), itemid) != mChestBlockItem.end();
}
bool CUser::CheckRightClickExchange(uint32 nExchangeID)
{
	// Does the exchange exist?

	if (isDead()
		|| isTrading()
		|| isMerchanting()
		|| isStoreOpen()
		|| isSellingMerchant()
		|| isBuyingMerchant()
		|| isMining())
		return false;

	_RIGHT_CLICK_EXCHANGE* pExchange = g_pMain->m_LoadRightClickExchange.GetData(nExchangeID);
	if (pExchange == nullptr)
		return false;
	uint16 sFreeSlot = -1;

	// Find free slots in the inventory, so that we can check against this later.
	uint8 bFreeSlots = 0;
	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		if (m_sItemArray[i].nNum == 0
			&& ++bFreeSlots >= ITEMS_RIGHT_CLICK_EXCHANGE_GROUP)
			break;
	}


	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		if (m_sItemArray[i].nNum == 0)
		{
			sFreeSlot = i;
			break;
		}
	}

	if (sFreeSlot < 0)
	{
		return false;
	}
	std::map<uint32, uint32> GiveItemList;
	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pExchange->nItemID[i] > 0)
		{
			GiveItemList.emplace(std::make_pair(pExchange->nItemID[i], pExchange->nCount[i]));
			if (pExchange->bSelection) break;
		}
	}

	// Can we hold all of these items? If we can't, we have a problem.
	uint8 bReqSlots = 0;
	uint32 nReqWeight = 0;
	foreach(itr, GiveItemList)
	{
		_ITEM_TABLE pTable = g_pMain->GetItemPtr(itr->first);
		if (pTable.isnull())
			return false;

		bReqSlots++;
		nReqWeight += pTable.m_sWeight;
	}


	// Holding too much already?
	if (m_sItemWeight + nReqWeight > m_sMaxWeight)
		return false;

	if (isTrading() || isMerchanting() || isMining())
		return false;

	// Do we have enough slots?
	return (bFreeSlots >= bReqSlots);
}
void CUser::HandleRightClickExchange(Packet& pkt)
{
	uint8 bSubCode;
	pkt >> bSubCode;
	switch (bSubCode)
	{
	case RightClickExchangeSubcode::GetCurrentItemList:
	{
		uint32 nItemID;
		pkt >> nItemID;
		if (nItemID == 0)
			break;
		_ITEM_TABLE  pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull())
			break;
		if (!CheckExistItem(nItemID, 1))
			break;
		_RIGHT_CLICK_EXCHANGE* pData = g_pMain->m_LoadRightClickExchange.GetData(nItemID);
		if (pData == nullptr)
		{
			g_pMain->SendHelpDescription(this, "ItemID not-found database.");
			break;
		}
		Packet result(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
		result << uint8(RightClickExchangeSubcode::GetCurrentItemList);
		for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			result << pData->nItemID[i] << pData->nRentalTime[i];
		result << nItemID << pData->bSelection;
		Send(&result);
	}
	break;
	case RightClickExchangeSubcode::SendItemExchange:
	{
		if (!isInGame() || m_bIsLoggingOut )
			break;

		if (isStoreOpen() || isMerchanting() || isSellingMerchant() || isBuyingMerchant() || isDead())
			break;

		uint32 nItemID = 0;
		pkt >> nItemID;
		_RIGHT_CLICK_EXCHANGE* pData = g_pMain->m_LoadRightClickExchange.GetData(nItemID);
		if (pData == nullptr)
		{
			g_pMain->SendHelpDescription(this, "ItemID not-found database.");
			break;
		}
		if (!CheckExistItem(pData->nBaseItemID, 1))
			break;

		uint32 nSelectionItem = 0;
		if (pData->bSelection == 1) /*16.04.2024*/
			pkt >> nSelectionItem;
		if (pData->bSelection == 1 && nSelectionItem == 0) /*16.04.2024*/
		{
			g_pMain->SendHelpDescription(this, "Select ItemID not-found database.");
			break;
		}
		if (pData->bSelection == 1) /*16.04.2024*/
		{
			int8 bSelectIndex = -1;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pData->nItemID[i] == nSelectionItem)
				{
					bSelectIndex = i;
					break;
				}
				else continue;
			}
			uint32 SelectItem = pData->nItemID[bSelectIndex];
			uint32 SelectItemCount = pData->nCount[bSelectIndex];
			uint32 SelectRentalTime = pData->nRentalTime[bSelectIndex];
			uint8  SelectItemFlag = pData->bFlag[bSelectIndex];
			int pos = -1;
			if (!CheckRightClickExchange(nItemID))
			{
				XSafe_SendMessageBox("Right Click Exchange", "Not enough space on�you");
				break;
			}
			if (!RobItem(pData->nBaseItemID, 1))
				break;
			GiveItem("Right Click Exchange-2",SelectItem, SelectItemCount, true,SelectRentalTime);
			Packet succes(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
			succes << uint8(RightClickExchangeSubcode::GetExchangeMessage) << uint8(1);
			Send(&succes);
		}
		else if (pData->bSelection == 3 || pData->bSelection == 4) /*27.05.2024*/
		{
			uint32 Miktar = 0;
			//---------------------------------------------------// KC K�rma Semih Hay�rl�//
			if (pData->bSelection == 3)
			{
				if (pData->nBaseItemID == 700082000) Miktar = 100;
				if (pData->nBaseItemID == 700083000) Miktar = 350;
				if (pData->nBaseItemID == 700084000) Miktar = 700;
				if (pData->nBaseItemID == 700085000) Miktar = 1000;
				if (pData->nBaseItemID == 700089000) Miktar = 2000;
				if (pData->nBaseItemID == 700086000) Miktar = 5000;
				if (pData->nBaseItemID == 700079000) Miktar = 10000;
				if (Miktar > 0)
				{
					if (RobItem(pData->nBaseItemID, 1))
						GiveBalance(Miktar);
				}
			}
			//--------------------------------------------------- TL k�rd�rma semih hay�rl�
			if (pData->bSelection == 4)
			{
				if (pData->nBaseItemID == 346600000) Miktar = 1;
				if (pData->nBaseItemID == 346700000) Miktar = 5;
				if (pData->nBaseItemID == 346800000) Miktar = 10;
				if (pData->nBaseItemID == 346900000) Miktar = 20;
				if (pData->nBaseItemID == 347000000) Miktar = 50;
				if (pData->nBaseItemID == 347100000) Miktar = 100;
				if (Miktar > 0)
				{
					if (RobItem(pData->nBaseItemID, 1))
						GiveBalance(0, Miktar);
				}
			}
			//---------------------------------------------------
			if (GetItemCount(pData->nBaseItemID) == 0)
			{
				Packet succes(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
				succes << uint8(RightClickExchangeSubcode::GetExchangeMessage) << uint8(1);
				Send(&succes);
			}
		}
		else if (pData->bSelection == 5) /*27.25.2024*/ // Premium K�rma Semih Hay�rl�//
		{
			bool clanpre = false;
			int8 bSelectIndex = 0;
			if (pData->nItemID[bSelectIndex] >= MIN_ITEM_ID)
				break;

			if (pData->nItemID[bSelectIndex] == 2 && pData->nCount[bSelectIndex] == 30) //clan premium
			{
				if (!isClanLeader())
				{
					XSafe_SendMessageBox("Right Click Exchange", "If you are not the clan leader, you can't exchange�it.");
					break;
				}

				GiveClanPremium(pData->nItemID[bSelectIndex], pData->nCount[bSelectIndex]);
				clanpre = true;
			}
			if (!clanpre)
			{
				if (!RobItem(pData->nBaseItemID, 1))
					break;

				if (pData->nItemID[bSelectIndex] == 13)
				{
					GiveSwitchPremium(10, pData->nCount[bSelectIndex]);
					GiveSwitchPremium(11, pData->nCount[bSelectIndex]);
					GiveSwitchPremium(12, pData->nCount[bSelectIndex]);
				}
				else
				{
					GivePremium(pData->nItemID[bSelectIndex], pData->nCount[bSelectIndex]);
				}
				if (pData->nItemID[bSelectIndex] == 4 && pData->nCount[bSelectIndex] == 30) // Silver Premium 28.05.2024 Semih Hay�rl�
					GivePremiumItem(10);

				if (pData->nItemID[bSelectIndex] == 5 && pData->nCount[bSelectIndex] == 30) // Gold Premium 28.05.2024 Semih Hay�rl�
					GivePremiumItem(10);

				if (pData->nItemID[bSelectIndex] == 7 && pData->nCount[bSelectIndex] == 30) // Platinum Premium 28.05.2024 Semih Hay�rl�
					GivePremiumItem(10);

				if (pData->nItemID[bSelectIndex] == 10 && pData->nCount[bSelectIndex] == 30) //dc premium
					GivePremiumItem(10);

				if (pData->nItemID[bSelectIndex] == 11 && pData->nCount[bSelectIndex] == 30) //exp premium
					GivePremiumItem(11);

				if (pData->nItemID[bSelectIndex] == 12 && pData->nCount[bSelectIndex] == 30) //war premium
					GivePremiumItem(12);

				if (pData->nItemID[bSelectIndex] == 13 && pData->nCount[bSelectIndex] == 30) //switch premium
				{
					if (CheckClass(1) || CheckClass(5) || CheckClass(6))
						GivePremiumItem(15);
					else if (CheckClass(2) || CheckClass(7) || CheckClass(8))
						GivePremiumItem(15);
					else if (CheckClass(3) || CheckClass(9) || CheckClass(10))
						GivePremiumItem(13);
					else if (CheckClass(4) || CheckClass(11) || CheckClass(12))
						GivePremiumItem(13);
				}
			}
			Packet succes(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
			succes << uint8(RightClickExchangeSubcode::GetExchangeMessage) << uint8(1);
			Send(&succes);
		}
		else
		{
			if (!CheckRightClickExchange(nItemID))
			{
				XSafe_SendMessageBox("Right Click Exchange", "Not enough space on�you");
				break;
			}
			if (!RobItem(pData->nBaseItemID, 1))
				break;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pData->nItemID[i] > 0)
					GiveItem("Right Click Exchange-1", pData->nItemID[i], pData->nCount[i], true, pData->nRentalTime[i]);
				else continue;
			}
			Packet succes(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
			succes << uint8(RightClickExchangeSubcode::GetExchangeMessage) << uint8(1);
			Send(&succes);
		}
	}
	break;
	case RightClickExchangeSubcode::GetExchangeInformation:
	{
		uint32 nExchangeItemID;
		_ITEM_EXCHANGE* pExchange = nullptr;
		pkt >> nExchangeItemID;
		int offset = 0;
		std::vector<uint32> ExchangeIndexList;
		if (g_pMain->m_ItemExchangeArray.GetSize() > 0)
		{
			foreach_stlmap_nolock(itr, g_pMain->m_ItemExchangeArray)
			{
				if (itr->second->nOriginItemNum[0] == nExchangeItemID)
					ExchangeIndexList.push_back(itr->second->nIndex);
				else
					continue;
			}
		}
		if (ExchangeIndexList.empty())
			break;
		Packet result(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
		result << uint8(RightClickExchangeSubcode::SendExchangeInformation) << nExchangeItemID;
		result << uint16(ExchangeIndexList.size());
		foreach(itr, ExchangeIndexList)
		{
			pExchange = g_pMain->m_ItemExchangeArray.GetData(*itr);

			if (pExchange == nullptr
				|| !(pExchange->bRandomFlag == 77 || pExchange->bRandomFlag == 3))
				continue;

			if (offset >= 10000)
				break;
			for (int i = 0; i < int(pExchange->sExchangeItemCount[0] / 5); i++)
			{
				if (i + offset >= 10000)
					break;
			}

			offset += int(pExchange->sExchangeItemCount[0] / 5);
			result << uint32(pExchange->nExchangeItemNum[0]) << uint16(offset);
		}
		Send(&result);
	}
	break;
	case RightClickExchangeSubcode::GetExchangeChestInformation:
	{
		uint32 nExchangeItemID;
		_ITEM_EXCHANGE* pExchange = nullptr;
		pkt >> nExchangeItemID;
		int offset = 0;
		std::vector<uint32> ExchangeIndexList;
		if (g_pMain->m_ItemExchangeArray.GetSize() > 0)
		{
			foreach_stlmap_nolock(itr, g_pMain->m_ItemExchangeArray)
			{
				if (itr->second->nOriginItemNum[0] == nExchangeItemID)
					ExchangeIndexList.push_back(itr->second->nIndex);
				else
					continue;
			}
		}
		if (ExchangeIndexList.empty())
			break;
		Packet result(XSafe, uint8(XSafeOpCodes::RIGHT_CLICK_EXCHANGE));
		result << uint8(RightClickExchangeSubcode::SendExchangeChestInformation) << nExchangeItemID;
		result << uint16(ExchangeIndexList.size() * ITEMS_IN_EXCHANGE_GROUP);
		foreach(itr, ExchangeIndexList)
		{
			pExchange = g_pMain->m_ItemExchangeArray.GetData(*itr);
			if (pExchange == nullptr
				|| !(pExchange->bRandomFlag == 101))
				continue;
			for (int i = 0; i < ITEMS_IN_EXCHANGE_GROUP; i++)
			{
				if (pExchange->nExchangeItemNum[i] == 0)
				{
					result << uint32(0) << uint16(0);
					continue;
				}



				result << uint32(pExchange->nExchangeItemNum[i]) << uint16(pExchange->sExchangeItemCount[i]);
			}
		}
		Send(&result);
	}
	break;
	default:
		break;
	}
}
void CUser::HandleChestBlock(Packet& pkt)
{
	uint8 subcode = pkt.read<uint8>();

	m_reloadChestBlock = true;
	mChestBlockItemLock.lock();
	mChestBlockItem.clear();
	if (subcode == 0) {
		uint16 size;
		pkt >> size;
		if (size > 100) size = 100;
		for (uint16 i = 0; i < size; i++) {
			uint32 itemid = pkt.read<uint32>();

			auto pItem = g_pMain->GetItemPtr(itemid);
			if (pItem.isnull() || pItem.m_bCountable == 2)
				continue;

			if (itemid)
				mChestBlockItem.push_back(itemid);
		}
	}
	mChestBlockItemLock.unlock();
	m_reloadChestBlock = false;
	if (subcode != 1)
		return;

	Packet newpkt(XSafe, uint8(XSafeOpCodes::CHEST_BLOCKITEM));
	Send(&newpkt);
}

void CUser::xSafe_SendAccountRegister()
{
	/*if (isGM())
		return;*/

	Packet test(WIZ_DB_SAVE_USER, uint8(ProcDbType::AccountInfoShow));
	g_pMain->AddDatabaseRequest(test, this);
}

bool NumberValid(const std::string& str2)
{
	std::string str = str2;
	STRTOLOWER(str);
	char a[10] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

	int size = (int)str.length();
	for (int i = 0; i < size; i++)
	{
		if (str.at(i) != a[0]
			&& str.at(i) != a[1]
			&& str.at(i) != a[2]
			&& str.at(i) != a[3]
			&& str.at(i) != a[4]
			&& str.at(i) != a[5]
			&& str.at(i) != a[6]
			&& str.at(i) != a[7]
			&& str.at(i) != a[8]
			&& str.at(i) != a[9])
			return false;
	}
	return true;
}

void CUser::XSafe_AccountInfoSave(Packet& pkt)
{
	uint8 Opcode;
	pkt >> Opcode;

	switch (Opcode)
	{
	case 1:
	{
		std::string txt_email, txt_phone, txt_seal, txt_otp;
		pkt >> txt_email >> txt_phone >> txt_seal >> txt_otp;

		Packet test(XSafe, uint8(XSafeOpCodes::ACCOUNT_INFO_SAVE));
		if (txt_email.empty() || txt_email.size() > 250) {
			test << uint8(2) << uint8(0);
			Send(&test);
			return;
		}

		if (txt_phone.empty() || txt_phone.size() != 11 || !NumberValid(txt_phone)) {
			test << uint8(2) << uint8(0);
			Send(&test);
			return;
		}

		if (txt_seal.empty() || txt_seal.size() != 8 || !NumberValid(txt_seal)) {
			test << uint8(2) << uint8(0);
			Send(&test);
			return;
		}

		if (txt_otp.empty() || txt_otp.size() != 6 || !NumberValid(txt_otp)) {
			test << uint8(2) << uint8(0);
			Send(&test);
			return;
		}

		Packet Datasave(WIZ_DB_SAVE_USER, uint8(ProcDbType::AccountInfoSave));
		Datasave.DByte();
		Datasave << txt_email << txt_phone << txt_seal << txt_otp;
		g_pMain->AddDatabaseRequest(Datasave,this);
	}
	break;
	//case 2:
	//	Disconnect();
	//	break;
	default:
		break;
	}
}

void CUser::XSafe_General(Packet & pkt)
{
	DateTime time;
	using std::string;

	std::string Account;

	pkt >> Account >> itemorg >> skillmagic >> zones >> skillmagictk >> srcversion; // dllversion uyumsuz ise dc eder
	if (!isGM() && !isGMUser())
	{
		if (itemorg != g_pMain->server_itemorg
			|| skillmagic != g_pMain->server_skillmagic
			|| zones != g_pMain->server_zones
			|| skillmagictk != g_pMain->server_skillmagictk)
			return goDisconnect("tbl data does not match.", __FUNCTION__);
	}

	//if (srcversion != "08a406419fb655942c4e319530734b14") // dllversion uyumsuz ise dc eder
	//if (srcversion != "ef97679f28771ba35b9e1a39be5cd733") // dllversion uyumsuz ise dc eder
	if (srcversion != "f5h4y7r8d5v3sd1s696g9y7r5w5q1a2d23gf3e625q4") // dllversion uyumsuz ise dc eder
	{
		printf("Version Eslesmedi AccountID : %s Nick : %s\n", GetAccountName().c_str(), GetName().c_str());
		return goDisconnect("version did not match.", __FUNCTION__);
	}

	if (m_strAccountID != Account){
		//printf("HWID Unmatch Account: %s - %s\n", GetRemoteIP().c_str(), GetAccountName().c_str());
		//Disconnect();
	}
}

uint8_t CUser::CheckEmptyWareHouseSlot()
{
	uint8_t EmptyCount = 0;

	for (uint8_t i = 0; i < WAREHOUSE_MAX; i++)
	{
		_ITEM_DATA * pItem = &m_sWarehouseArray[i];
		if (pItem == nullptr)
			continue;

		if (pItem->nNum == 0)
			EmptyCount++;
		
	}
	return EmptyCount;
}

void CUser::WareHouseAddItemProcess(uint32 ItemID, uint32 Count)
{
	//if (WareHouseOpen) // Banka A��k �ptal Ediyoruz
	//	return;

	/*if (!isAutoFishing() && !isAutoMining())
		return;*/

	_ITEM_TABLE pTable = g_pMain->GetItemPtr(ItemID);
	if (pTable.isnull() || (!pTable.isStackable() && Count > 1))
		return;

	for (int i = 0; i < WAREHOUSE_MAX; i++)
	{
		_ITEM_DATA * pItem = &m_sWarehouseArray[i];
		if (pItem == nullptr || (pItem->nNum != 0 && pItem->nNum != ItemID))
			continue;

		if (pItem->nNum == ItemID && pTable.m_bCountable != 0)
		{
			pItem->sCount++;
			break;
		}
		else if (pItem->nNum == ItemID && pTable.m_bCountable == 0)
			continue;
		else
		{
			pItem->nNum = ItemID;
			pItem->sDuration = pTable.m_sDuration;
			pItem->sCount = Count;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->bFlag = ITEM_FLAG_NONE;	// ITEM_FLAG_NONE
			break;
		}
	}
}



void CUser::XSafe_LastSeenProcess(Packet& pkt)
{
	uint8 opcode = pkt.read<uint8>();

	switch (opcode)
	{
	case 1:
	{
		DateTime dt;
		m_LastSeen[0] = dt.GetHour();
		m_LastSeen[1] = dt.GetMinute();

		//printf("Last Seen Updated %d:%d\n", m_LastSeen[0], m_LastSeen[1]);
	}
	break;
	case 2:
	{
		return;
		std::string targetUser = "";
		pkt >> targetUser;
		if (targetUser.empty() || targetUser == "")
			return;
		CUser * pTUser = g_pMain->GetUserPtr(targetUser, NameType::TYPE_CHARACTER);
		if (pTUser == nullptr || !pTUser->isInGame())
			return;

		Packet result(XSafe, uint8(XSafeOpCodes::CHAT_LASTSEEN));
		result << pTUser->GetName() << uint8(pTUser->m_LastSeen[0]) << uint8(pTUser->m_LastSeen[1]);
		Send(&result);
	}
	default:
		break;
	}
}

void CUser::XSafe_SendRepurchaseMsg(Packet& pkt)
{
	SendRepurchaseMsg();
}

void CUser::XSafe_Send1299SkillAndStatReset(Packet& pkt)
{
	uint8 opcode = pkt.read<uint8>();

	switch (opcode)
	{
	case 1:
	{
		AllPointChange();
	}
	break;
	case 2:
	{
		AllSkillPointChange();
	}
	break;
	default:
		break;
	}
}

void CUser::SendAcsMessage(std::string message) {
	if (message.empty()) 
		return;

	Packet newpkt(XSafe, uint8(XSafeOpCodes::INFOMESSAGE));
	newpkt << message; Send(&newpkt);
}