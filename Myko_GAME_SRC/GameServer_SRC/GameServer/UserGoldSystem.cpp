#include "stdafx.h"

void CUser::GoldChange(short tid, int gold)
{
	if (GetZoneID() == ZONE_SNOW_BATTLE
		|| GetZoneID() == ZONE_DESPERATION_ABYSS
		|| GetZoneID() == ZONE_HELL_ABYSS
		|| GetZoneID() == ZONE_DRAGON_CAVE
		|| GetZoneID() == ZONE_CAITHAROS_ARENA
		|| GetZoneID() == ZONE_ISILOON_ARENA
		|| GetZoneID() == ZONE_FELANKOR_ARENA)
		return;

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (pTUser == nullptr || pTUser->GetCoins() <= 0)
		return;

	// Reward money in war zone
	if (gold == 0)
	{
		// If we're not in a party, we can distribute cleanly.
		if (!isInParty())
		{
			uint32 targetCoins = pTUser->GetCoins();
			GoldGain((targetCoins * 4) / 10);
			pTUser->GoldLose(targetCoins / 2);
			return;
		}

		// Otherwise, if we're in a party, we need to divide it up.
		_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
		if (pParty == nullptr)
			return;

		uint32 targetCoins = pTUser->GetCoins();
		int userCount = 0, levelSum = 0, temp_gold = (targetCoins * 4) / 10;
		pTUser->GoldLose(targetCoins / 2);

		// TODO: Clean up the party system. 
		for (int i = 0; i < MAX_PARTY_USERS; i++)
		{
			CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pUser == nullptr)
				continue;

			userCount++;
			levelSum += pUser->GetLevel();
		}

		// No users (this should never happen! Needs to be cleaned up...), don't bother with the below loop.
		if (userCount == 0)
			return;

		for (int i = 0; i < MAX_PARTY_USERS; i++)
		{
			CUser * pUser = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pUser == nullptr)
				continue;

			pUser->GoldGain((int)(temp_gold * (float)(pUser->GetLevel() / (float)levelSum)));
		}
		return;
	}

	// Otherwise, use the coin amount provided.
	// Reject negative values: negative int cast to uint32 causes catastrophic overflow.
	if (gold <= 0)
		return;

	GoldGain(gold);
	pTUser->GoldLose(gold);
}

void CUser::GoldGain(uint32 gold, bool bSendPacket /*= true*/, bool bApplyBonus /*= false*/)
{
	// Assuming it works like this, although this affects (probably) all gold gained (including kills in PvP zones)
	// If this is wrong and it should ONLY affect gold gained from monsters, let us know!
	if (bApplyBonus)
	{
		if (sClanPremStatus && g_pMain->ClanPreFazlagold >= 1)
			gold = gold * (m_bNoahGainAmount + m_bItemNoahGainAmount + g_pMain->ClanPreFazlagold) / 100;
		else
			gold = gold * (m_bNoahGainAmount + m_bItemNoahGainAmount) / 100;

		if (m_flameilevel > 0 && m_flameilevel <= 3 && g_pMain->pBurningFea[m_flameilevel - 1].moneyrate)
			gold = gold * (100 + g_pMain->pBurningFea[m_flameilevel - 1].moneyrate) / 100;

		uint32 perkCoin = 0;
		if (pPerks.perkType[(int)perks::percentCoinsMon] > 0) {
			auto* perks = g_pMain->m_PerksArray.GetData((int)perks::percentCoinsMon);
			if (perks && perks->perkCount)
				perkCoin += perks->perkCount * pPerks.perkType[(int)perks::percentCoinsMon];
		}

		if (perkCoin)
			gold += (gold * perkCoin) / 100;
	}

	{ // CHI-AUDIT: Gold lock — race condition fix
		std::lock_guard<std::recursive_mutex> lock(m_goldLock);
		if (m_iGold + gold > COIN_MAX)
			m_iGold = COIN_MAX;
		else
			m_iGold += gold;
	}

	if (bSendPacket)
	{
		Packet result(WIZ_GOLD_CHANGE, uint8(CoinGain));
		result << gold << GetCoins();
		Send(&result);
	}
}

void CUser::LuaGoldGain(uint32 gold, bool bSendPacket /*= true*/)
{
	Packet result;

	{ // CHI-AUDIT: Gold lock — race condition fix
		std::lock_guard<std::recursive_mutex> lock(m_goldLock);
		if (m_iGold + gold > COIN_MAX)
		{
			result.Initialize(WIZ_QUEST);
			result << uint8(13) << uint8(2);
			Send(&result);
			return;
		}
		else
			m_iGold += gold;
	}

	if (bSendPacket)
	{
		result.Initialize(WIZ_GOLD_CHANGE);
		result << uint8(CoinGain) << gold << GetCoins();
		Send(&result);
	}
}

bool CUser::GoldLose(uint32 gold, bool bSendPacket /*= true*/)
{
	{ // CHI-AUDIT: Gold lock — race condition fix
		std::lock_guard<std::recursive_mutex> lock(m_goldLock);
		if (m_iGold < gold)
			return false;
		m_iGold -= gold;
	}

	if (bSendPacket)
	{
		Packet result(WIZ_GOLD_CHANGE, uint8(CoinLoss));
		result << gold << GetCoins();
		Send(&result);
	}
	return true;
}

bool CUser::CashLose(uint32 cash)
{
	if (m_nKnightCash < cash)
		return false;

	m_nKnightCash -= cash;

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::CASHCHANGE) << m_nKnightCash <<  m_nTLBalance;
	Send(&result);
	
	Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateKnightCash));
	g_pMain->AddDatabaseRequest(Save, this);
	return true;
}

void CUser::CashGain(uint32 cash)
{
	if (m_nKnightCash + cash > 999999999) m_nKnightCash = 999999999;
	else m_nKnightCash += cash;

	Packet result(XSafe);
	result << uint8(XSafeOpCodes::CASHCHANGE) << m_nKnightCash <<  m_nTLBalance;
	Send(&result);
	Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateKnightCash));
	g_pMain->AddDatabaseRequest(Save, this);
}