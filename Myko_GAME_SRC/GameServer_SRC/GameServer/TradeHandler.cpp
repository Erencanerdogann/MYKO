#include "stdafx.h"
#include "Map.h"
#include "../shared/DateTime.h"

using namespace std;

void CUser::ExchangeProcess(Packet & pkt)
{
	if (!isInGame() || m_bIsLoggingOut)
		return;

	if (isMerchanting() /*|| isStoreOpen()*/ || isSellingMerchant() || isBuyingMerchant() || isDead()) 
		return;

	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case EXCHANGE_REQ:
		ExchangeReq(pkt);
		break;
	case EXCHANGE_AGREE:
		ExchangeAgree(pkt);
		break;
	case EXCHANGE_ADD:
		ExchangeAdd(pkt);
		break;
	case EXCHANGE_DECIDE:
		ExchangeDecide();
		break;
	case EXCHANGE_CANCEL:
		ExchangeCancel();
		break;
	default:
		printf("Trade unhanled packets %d \n", opcode);
		break;
	}
}

void CUser::ExchangeReq(Packet & pkt)
{
	Packet result(WIZ_EXCHANGE);
	CUser * pUser;
	uint16 destid;
	DateTime time;

	if (pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID()))
		goto fail_return;

	if (isDead()
		|| isMerchanting()
		|| isMining()
		|| isFishing()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing()
		|| !isInGame())
		goto fail_return;
	else if (isTrading())
	{
		ExchangeCancel();
		return;
	}

	/*if (GetLevel() < 30)
	{
		result.clear();
		result.Initialize(WIZ_EXCHANGE);
		result << uint8(8) << uint8(2);
		Send(&result);
		return;
	}*/

	destid = pkt.read<uint16>();
	pUser = g_pMain->GetUserPtr(destid);

	if (pUser == nullptr)
		goto fail_return;

	if (pUser->isDead()
		|| !pUser->isInGame()
		|| pUser->isTrading()
		|| pUser->isMining()
		|| pUser->isFishing()
		|| pUser->isSellingMerchantingPreparing()
		|| pUser->isBuyingMerchantingPreparing()
		|| pUser->isMerchanting()
		|| pUser->m_bGenieStatus
		|| pUser->GetZoneID() != GetZoneID()
		|| (pUser->GetNation() != GetNation() && GetMap() && !GetMap()->canTradeWithOtherNation()))
		goto fail_return;

	if (pUser->GetAccountName() == GetAccountName()
		|| pUser->isGM() || pUser == this
		|| pUser->GetSocketID() == GetSocketID()) 
		goto fail_return;


	if (GetLevel() < g_pMain->pServerSetting.TradeLevel) {
		g_pMain->SendHelpDescription(this, string_format("Your level is insufficient to trade. You must be at least level %d .", g_pMain->pServerSetting.TradeLevel));
		goto fail_return;
	}

	m_sExchangeUser = destid;
	m_sTradeStatue = 2;
	pUser->m_sExchangeUser = GetSocketID();
	pUser->m_sTradeStatue = 3;
	m_isRequestSender = true;

	result << uint8(EXCHANGE_REQ)
		<< GetSocketID();
	pUser->Send(&result);
	return;

fail_return:
	result << uint8(EXCHANGE_CANCEL);
	Send(&result);
}

void CUser::ExchangeAgree(Packet & pkt)
{
	Packet result(WIZ_EXCHANGE);
	DateTime time;

	if (m_sTradeStatue != 3) goto fail_return;

	if (!isTrading()
		|| isDead()
		|| isMining()
		|| isFishing()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing()
		|| isMerchanting()
		|| !isInGame())
	{
		NewTradeAgreeErrorReset();
		goto fail_return;
	}

	uint8 bResult = pkt.read<uint8>();
	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);

	if (pUser == nullptr) {
		NewTradeAgreeErrorReset();
		goto fail_return;
	}
		
	if (pUser->GetZoneID() != GetZoneID()
		|| pUser->isMining()
		|| pUser->isFishing()
		|| pUser->m_sExchangeUser != GetSocketID()
		|| pUser->isSellingMerchantingPreparing()
		|| pUser->isBuyingMerchantingPreparing()
		|| pUser->isMerchanting()
		|| pUser->m_sTradeStatue != 2
		|| (pUser->GetNation() != GetNation() && GetMap() && !GetMap()->canTradeWithOtherNation()))
	{
		NewTradeAgreeErrorReset();
		/*if (pUser) 
			pUser->m_sExchangeUser = -1;*/
		goto fail_return;
	}

	if (pUser->GetAccountName() == GetAccountName()
		|| pUser->GetSocketID() == GetSocketID() || this == pUser || m_isRequestSender == true)
	{
		goDisconnect("trade level accountid or socketid check.", __FUNCTION__);
		pUser->goDisconnect("trade level accountid or socketid check.", __FUNCTION__);
		return;
	}

	// CHI-18: Her iki taraf da level kontrolunden gecmeli (asimetrik durum duzeltildi)
	if (GetLevel() < g_pMain->pServerSetting.TradeLevel) {
		g_pMain->SendHelpDescription(this, string_format("Your level is insufficient to trade. You must be at least level %d .", g_pMain->pServerSetting.TradeLevel));
		NewTradeAgreeErrorReset();
		goto fail_return;
	}
	if (pUser->GetLevel() < g_pMain->pServerSetting.TradeLevel) {
		g_pMain->SendHelpDescription(this, string_format("The other player's level is insufficient to trade. They must be at least level %d .", g_pMain->pServerSetting.TradeLevel));
		NewTradeAgreeErrorReset();
		goto fail_return;
	}

	if (!bResult) {
		m_sExchangeUser = -1;
		m_sTradeStatue = 1;
		pUser->m_sExchangeUser = -1;
		pUser->m_sTradeStatue = 1;
		bResult = 0;
		pUser->m_isRequestSender = false;
	}
	else {
		m_sTradeStatue = 4;
		pUser->m_sTradeStatue = 4;
	}

	if (m_sMerchantsSocketID >= 0) RemoveFromMerchantLookers();

	result << uint8(EXCHANGE_AGREE)<< uint16(bResult);
	pUser->Send(&result);

	return;

fail_return:
	result << uint8(EXCHANGE_CANCEL);
	Send(&result);
}

#pragma region NewTradeAgreeErrorReset()
void CUser::NewTradeAgreeErrorReset() {
	m_sExchangeUser = -1; m_sTradeStatue = 1;
}
#pragma endregion

/**
* @brief	Exchange item adding.
*/
void CUser::ExchangeAdd(Packet & pkt)
{
	if (!isTrading()
		|| isDead()
		|| isMining()
		|| isFishing()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing()
		|| isMerchanting()
		|| !isInGame())
		return;

	std::lock_guard<std::recursive_mutex> inventoryGuard(m_inventoryLock);

	Packet result(WIZ_EXCHANGE, uint8(EXCHANGE_ADD));
	uint64 nSerialNum = 0;
	DateTime time;
	uint32 nItemID, count = 0;
	uint16 duration = 0;
	_ITEM_DATA * pSrcItem = nullptr;
	list<_EXCHANGE_ITEM*>::iterator	Iter;
	uint8 pos;
	bool bAdd = true, bGold = false, bCountDeducted = false;
	_ITEM_TABLE pItemTable = _ITEM_TABLE();
	_ITEM_TABLE pTable = _ITEM_TABLE();

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);

	if (pUser == nullptr)
		return;

	if (m_sTradeStatue != 4) goto add_fail;

	if (!pUser->isInGame()
		|| pUser->m_sExchangeUser != GetSocketID()
		|| pUser->isDead()
		|| pUser->GetZoneID() != GetZoneID()
		|| pUser->isMining()
		|| pUser->isFishing()
		|| pUser->isSellingMerchantingPreparing()
		|| pUser->isBuyingMerchantingPreparing()
		|| pUser->isMerchanting()
		|| (pUser->GetNation() != GetNation() && GetMap() && !GetMap()->canTradeWithOtherNation()))
		return;

	if (pUser->m_sTradeStatue != 4
		&& pUser->m_sTradeStatue != 5) goto add_fail;

	if (pUser->GetAccountName() == GetAccountName())
	{
		goDisconnect("It uses the same account name as the other party.", __FUNCTION__);
		pUser->goDisconnect("It uses the same account name as the other party.", __FUNCTION__);
		return;
	}

	pkt >> pos >> nItemID >> count;
	if (count == 0 || !nItemID)
	{
		LOG_HACK("[PACKET_INVALID] User=%s Type=TradeAdd count=%u itemID=%u", GetName().c_str(), count, nItemID);
		return goDisconnect("trade item add count check.", __FUNCTION__);
	}
	// Gold ve item icin ayri limit: gold COIN_MAX, item MAX_ITEM_COUNT
	if (nItemID == ITEM_GOLD && count > COIN_MAX)
	{
		LOG_HACK("[PACKET_INVALID] User=%s Type=TradeAdd gold overflow count=%u", GetName().c_str(), count);
		return goDisconnect("trade gold overflow.", __FUNCTION__);
	}
	if (nItemID != ITEM_GOLD && count > MAX_ITEM_COUNT)
	{
		LOG_HACK("[PACKET_INVALID] User=%s Type=TradeAdd count=%u itemID=%u", GetName().c_str(), count, nItemID);
		return goDisconnect("trade item count check.", __FUNCTION__);
	}

	// Validate pos is within inventory range (not gold)
	if (nItemID != ITEM_GOLD && pos >= HAVE_MAX)
	{
		LOG_HACK("[PACKET_INVALID] User=%s Type=TradeAdd pos=%u out of range", GetName().c_str(), pos);
		goto add_fail;
	}

	if (nItemID == ITEM_GOLD && pos != 255) goto add_fail;

	pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull())
		goto add_fail;

	if ((nItemID != ITEM_GOLD
		&& (pos >= HAVE_MAX || (nItemID >= ITEM_NO_TRADE_MIN && nItemID <= ITEM_NO_TRADE_MAX) // Cannot be traded, stored or sold.
			|| pTable.m_bRace == RACE_UNTRADEABLE)))
		goto add_fail;

	if (nItemID == ITEM_GOLD)
	{
		// CHI-AUDIT: Gold lock — trade add
		m_goldLock.lock();
		if (!count  || count > m_iGold)
		{
			m_goldLock.unlock();
			goto add_fail;
		}

		// If we have coins in the list already
		// add to the amount of coins listed.
		foreach(itr, m_ExchangeItemList)
		{
			if ((*itr)->nItemID == ITEM_GOLD)
			{
				// Overflow protection: ensure accumulated gold doesn't exceed COIN_MAX
				if ((*itr)->nCount + count > COIN_MAX)
				{
					LOG_HACK("[PACKET_INVALID] User=%s Type=TradeGoldOverflow current=%u adding=%u", GetName().c_str(), (*itr)->nCount, count);
					m_goldLock.unlock();
					goto add_fail;
				}
				(*itr)->nCount += count;
				bAdd = false; /* don't need to add a new entry */
				break;
			}
		}

		m_iGold -= count;
		bCountDeducted = true;
		m_goldLock.unlock();
	}
	else if ((pSrcItem = GetItem(SLOT_MAX + pos)) != nullptr && pSrcItem->nNum == nItemID)
	{
		if (pSrcItem->sCount < count
			|| pSrcItem->sCount == 0
			|| pSrcItem->isRented()
			|| pSrcItem->isSealed()
			|| pSrcItem->isBound()
			|| pSrcItem->isDuplicate()
			|| pSrcItem->isExpirationTime())
			goto add_fail;

		if (pTable.m_bCountable)
		{
			foreach(itr, m_ExchangeItemList)
			{
				if ((*itr)->nItemID == nItemID)
				{
					(*itr)->nCount += count;
					bAdd = false;
					break;
				}
			}
		}

		pSrcItem->sCount -= count;
		bCountDeducted = true;

		duration = pSrcItem->sDuration;
		nSerialNum = pSrcItem->nSerialNum;
	}
	else
		goto add_fail;

	foreach(itr, m_ExchangeItemList)
	{
		if ((*itr)->nItemID == ITEM_GOLD)
		{
			bGold = true;
			break;
		}
	}

	if ((int)m_ExchangeItemList.size() > (bGold ? 13 : 12))
		goto add_fail;

	if (bAdd)
	{
		_EXCHANGE_ITEM * pItem = new _EXCHANGE_ITEM;
		pItem->nItemID = nItemID;
		pItem->sDurability = duration;
		pItem->nCount = count;
		pItem->nSerialNum = nSerialNum;
		pItem->bSrcPos = SLOT_MAX + pos;
		m_ExchangeItemList.push_back(pItem);
	}

	result << uint8(1);
	Send(&result);

	result.clear();

	result << uint8(EXCHANGE_OTHERADD)
		<< nItemID
		<< count
		<< duration;

	pItemTable = g_pMain->GetItemPtr(nItemID);
	if (!pItemTable.isnull())
	{
		if (pItemTable.isPetItem() && bAdd)
			ShowPetItemInfo(result, nSerialNum);
		else if (pItemTable.Getnum() == ITEM_CYPHER_RING && bAdd)
			ShowCyperRingItemInfo(result, nSerialNum);
		else
			result << uint32(0); // Item Unique ID
	}
	else
		result << uint32(0); // Item Unique ID*/

	/*if (nItemID == ITEM_CYPHER_RING && bAdd)
		ShowCyperRingItemInfo(result, nSerialNum);
	else
		result << uint32(0); // Item Unique ID*/

	pUser->Send(&result);
	return;

add_fail:
	// Restore item count / gold that was already decremented before this failure
	if (bCountDeducted)
	{
		if (pSrcItem != nullptr && pSrcItem->nNum == nItemID)
			pSrcItem->sCount += count;
		else if (nItemID == ITEM_GOLD)
		{
			std::lock_guard<std::recursive_mutex> lock(m_goldLock);
			m_iGold += count;
		}

		// If we added to an existing exchange list entry (bAdd == false),
		// undo that increment as well
		if (!bAdd)
		{
			foreach(itr, m_ExchangeItemList)
			{
				if ((*itr)->nItemID == nItemID)
				{
					(*itr)->nCount -= count;
					break;
				}
			}
		}
	}

	result << uint8(0);
	Send(&result);
}

void CUser::ExchangeDecide()
{
	if (m_sTradeStatue != 4
		|| !isTrading()
		|| isDead()
		|| isMining()
		|| isFishing()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing()
		|| isMerchanting()
		|| !isInGame())
		return;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);

	if (pUser == nullptr)
		return;

	// Lock both users' inventories in socketID order to prevent deadlock
	CUser* pFirst = (GetSocketID() < pUser->GetSocketID()) ? this : pUser;
	CUser* pSecond = (GetSocketID() < pUser->GetSocketID()) ? pUser : this;
	std::lock_guard<std::recursive_mutex> lock1(pFirst->m_inventoryLock);
	std::lock_guard<std::recursive_mutex> lock2(pSecond->m_inventoryLock);

	if (!pUser->isInGame()
		|| pUser->m_sExchangeUser != GetSocketID()
		|| pUser->isDead()
		|| pUser->GetZoneID() != GetZoneID()
		|| pUser->isMining()
		|| pUser->isFishing()
		|| pUser->isSellingMerchantingPreparing()
		|| pUser->isBuyingMerchantingPreparing()
		|| pUser->isMerchanting()
		|| (pUser->GetNation() != GetNation() && GetMap() && !GetMap()->canTradeWithOtherNation()))
		return;

	if (pUser->m_sTradeStatue != 5 && pUser->m_sTradeStatue != 4) return;

	Packet result(WIZ_EXCHANGE);
	if (pUser->m_sTradeStatue != 5) {
		m_sTradeStatue = 5;
		result << uint8(EXCHANGE_OTHERDECIDE);
		pUser->Send(&result);
		return;
	}

	m_sTradeStatue = 5;

	// Did the exchange requirements fail?
	if (!CheckExchange() || !pUser->CheckExchange())
	{
		// At this stage, neither user has their items exchanged.
		// However, their coins were removed -- these will be removed by ExchangeFinish().
		result << uint8(EXCHANGE_DONE) << uint8(0);
		Send(&result);

		if (!pUser->CheckExchange())
			pUser->Send(&result);

		ExchangeCancel();

		if (!pUser->CheckExchange())
			pUser->ExchangeCancel();
	}
	else
	{
		if (!CheckExecuteExchange() || !pUser->CheckExecuteExchange())
		{
			// At this stage, neither user has their items exchanged.
			// However, their coins were removed -- these will be removed by ExchangeFinish().
			result << uint8(EXCHANGE_DONE) << uint8(0);
			Send(&result);

			if (!pUser->CheckExecuteExchange())
				pUser->Send(&result);

			ExchangeCancel();

			if (!pUser->CheckExecuteExchange())
				pUser->ExchangeCancel();
		}
		else
		{
			uint32 itemid[9], titemid[9], money = 0, tmoney = 0; uint16 itemcount[9], titemcount[9];
			uint16 counter = 0;
			
			ExecuteExchange(money);
			pUser->ExecuteExchange(tmoney);

			memset(itemid, 0, sizeof(itemid));
			memset(titemid, 0, sizeof(titemid));
			memset(itemcount, 0, sizeof(itemcount));
			memset(titemcount, 0, sizeof(titemcount));

			Packet result(WIZ_EXCHANGE);
			result << uint8(EXCHANGE_DONE) << uint8(1)
				<< GetCoins()
				<< uint16(pUser->m_ExchangeItemList.size());
			DateTime time;
			foreach(itr, pUser->m_ExchangeItemList)
			{
				if ((*itr) == nullptr)
					continue;

				result << (*itr)->bDstPos << (*itr)->nItemID << uint16((*itr)->nCount) << (*itr)->sDurability << uint8(0);

				_ITEM_TABLE pItemTable = g_pMain->GetItemPtr((*itr)->nItemID);
				if (!pItemTable.isnull())
				{
					if (pItemTable.isPetItem())
						ShowPetItemInfo(result, (*itr)->nSerialNum);
					else if (pItemTable.Getnum() == ITEM_CYPHER_RING)
						ShowCyperRingItemInfo(result, (*itr)->nSerialNum);
					else
						result << uint32(0); // Item Unique ID
				}
				//else result << uint32(0); // Item Unique ID*/

				if ((*itr)->nItemID != ITEM_GOLD) {
					if (counter < 9) {
						itemid[counter] = (*itr)->nItemID;
						itemcount[counter++] = (*itr)->nCount;
					}
				}
				//else money = (*itr)->nCount;

				/*if ((*itr)->nItemID == ITEM_CYPHER_RING)
					ShowCyperRingItemInfo(result, (*itr)->nSerialNum);
				else
					result << uint32(0); // Item Unique ID*/

			}
			Send(&result);

			result.clear();

			result << uint8(EXCHANGE_DONE) << uint8(1)
				<< pUser->GetCoins()
				<< uint16(m_ExchangeItemList.size());

			counter = 0;

			foreach(itr, m_ExchangeItemList)
			{
				if ((*itr) == nullptr)
					continue;

				result << (*itr)->bDstPos << (*itr)->nItemID << uint16((*itr)->nCount) << (*itr)->sDurability << uint8(0);

				_ITEM_TABLE pItemTable = g_pMain->GetItemPtr((*itr)->nItemID);
				if (!pItemTable.isnull())
				{
					if (pItemTable.isPetItem())
						ShowPetItemInfo(result, (*itr)->nSerialNum);
					else if (pItemTable.Getnum() == ITEM_CYPHER_RING)
						ShowCyperRingItemInfo(result, (*itr)->nSerialNum);
					else
						result << uint32(0); // Item Unique ID
				}
				else
					result << uint32(0); // Item Unique ID*/

				if ((*itr)->nItemID != ITEM_GOLD) {
					if (counter < 9) {
						titemid[counter] = (*itr)->nItemID;
						titemcount[counter++] = (*itr)->nCount;
					}
				}
				else tmoney = (*itr)->nCount;

				/*if ((*itr)->nItemID == ITEM_CYPHER_RING)
					ShowCyperRingItemInfo(result, (*itr)->nSerialNum);
				else
					result << uint32(0); // Item Unique ID*/

			}
			pUser->Send(&result);

			TradeInsertLog(pUser, money, tmoney, itemid, itemcount, titemid, titemcount);

			SetUserAbility(false);
			SendItemWeight();
			ExchangeFinish();
			UserDataSaveToAgent();

			pUser->SetUserAbility(false);
			pUser->SendItemWeight();
			pUser->ExchangeFinish();
			pUser->UserDataSaveToAgent();

			LOG_TRADE("[TRADE] User1=%s User2=%s Gold1=%u Gold2=%u Items1=[%u:%u,%u:%u,%u:%u] Items2=[%u:%u,%u:%u,%u:%u]",
				GetName().c_str(), pUser->GetName().c_str(),
				money, tmoney,
				itemid[0], itemcount[0], itemid[1], itemcount[1], itemid[2], itemcount[2],
				titemid[0], titemcount[0], titemid[1], titemcount[1], titemid[2], titemcount[2]);
		}
	}
}

void CUser::ExchangeCancel(bool bIsOnDeath)
{
	if (m_sTradeStatue == 1) return;

	short ExchangeUserID = m_sExchangeUser;
	CUser* pUser = g_pMain->GetUserPtr(ExchangeUserID);

	// Lock both users' inventories in socketID order to prevent deadlock
	// If pUser is null, only lock this user
	if (pUser != nullptr && pUser->isInGame() && pUser->m_sExchangeUser == GetSocketID()) {
		CUser* pFirst = (GetSocketID() < pUser->GetSocketID()) ? this : pUser;
		CUser* pSecond = (GetSocketID() < pUser->GetSocketID()) ? pUser : this;
		std::lock_guard<std::recursive_mutex> lock1(pFirst->m_inventoryLock);
		std::lock_guard<std::recursive_mutex> lock2(pSecond->m_inventoryLock);

		Packet result(WIZ_EXCHANGE, uint8(EXCHANGE_CANCEL));
		ExchangeGiveItemsBack();
		ExchangeFinish();
		Send(&result);

		if (pUser->m_sTradeStatue != 1) {
			pUser->ExchangeGiveItemsBack();
			pUser->ExchangeFinish();
			pUser->Send(&result);
		}
	}
	else {
		std::lock_guard<std::recursive_mutex> inventoryGuard(m_inventoryLock);

		Packet result(WIZ_EXCHANGE, uint8(EXCHANGE_CANCEL));
		ExchangeGiveItemsBack();
		ExchangeFinish();
		Send(&result);
	}
}

void CUser::ExchangeFinish()
{
	m_sTradeStatue = 1;
	m_sExchangeUser = -1;
	m_ExchangeItemList.clear();
	m_isRequestSender = false;
}

void CUser::ExchangeGiveItemsBack()
{
	// Restore coins and items...
	while (m_ExchangeItemList.size())
	{
		_EXCHANGE_ITEM *pItem = m_ExchangeItemList.front();
		if (pItem != nullptr)
		{
			// Restore coins to owner
			if (pItem->nItemID == ITEM_GOLD) {
				std::lock_guard<std::recursive_mutex> lock(m_goldLock);
				if (m_iGold + pItem->nCount > COIN_MAX) m_iGold = COIN_MAX;
				else m_iGold += pItem->nCount;
			}
			// Restore items to owner
			else
			{
				_ITEM_DATA * pItems = GetItem(pItem->bSrcPos);
				if (pItems != nullptr && pItems->nNum == pItem->nItemID)
					GetItem(pItem->bSrcPos)->sCount += pItem->nCount;
			}
			delete pItem;
		}

		m_ExchangeItemList.pop_front();
	}
}

bool CUser::CheckExchange()
{
	if (!isTrading()
		|| isDead()
		|| isMining()
		|| isFishing()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing()
		|| isMerchanting()
		|| !isInGame())
		return false;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);

	if (pUser == nullptr)
		return false;

	if (!pUser->isInGame()
		|| pUser->m_sExchangeUser != GetSocketID()
		|| pUser->GetZoneID() != GetZoneID()
		|| pUser->isMining()
		|| pUser->isFishing()
		|| pUser->isSellingMerchantingPreparing()
		|| pUser->isBuyingMerchantingPreparing()
		|| pUser->isMerchanting()
		|| (pUser->GetNation() != GetNation() && GetMap() && !GetMap()->canTradeWithOtherNation()))
		return false;

	uint32 money = 0;
	uint32 weight = 0;
	// Get the total number of free slots in the player's inventory
	uint8 bFreeSlots = 0, bItemCount = 0;
	for (uint8 i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		_ITEM_DATA * pItem = GetItem(i);

		if (!pItem)
			continue;

		if (pItem->nNum == 0)
			bFreeSlots++;
	}

	// Loop through the other user's list of items up for trade.
	foreach(Iter, pUser->m_ExchangeItemList)
	{
		// If we're trading coins, ensure we don't exceed our limit.
		if ((*Iter)->nItemID == ITEM_GOLD)
		{
			money += (*Iter)->nCount;
			if ((GetCoins() + money) > COIN_MAX)
				return false;

			continue;
		}

		// Does this item exist?
		_ITEM_TABLE pTable = g_pMain->GetItemPtr((*Iter)->nItemID);
		if (pTable.isnull())
			return false;

		// Is there enough room for this item?
		// NOTE: Also ensures we have enough space in our inventory (with one exchange in mind anyway)
		if (!CheckWeight(pTable, (*Iter)->nItemID, (*Iter)->nCount))
			return false;

		// Total up the weight (multiply by count for stackable items).
		weight += (uint32)pTable.m_sWeight * (*Iter)->nCount;
		bItemCount++;
	}

	// Do we have enough free slots for all these items?
	if (bItemCount > bFreeSlots)
		return false; /* note: ignores item stacks for now */

	// Ensure the total combined item weight does not exceed our weight limit
	return ((weight + m_sItemWeight) <= m_sMaxWeight);
}

void CUser::ExecuteExchange(uint32& get_money)
{
	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);

	if (pUser == nullptr)
		return;

	if (!pUser->isInGame()
		|| pUser->m_sExchangeUser != GetSocketID())
		return;

	ItemList::iterator coinItr = pUser->m_ExchangeItemList.end();
	foreach(Iter, pUser->m_ExchangeItemList)
	{
		if ((*Iter) == nullptr)
			continue;

		if ((*Iter)->nItemID == ITEM_GOLD)
		{
			get_money = (*Iter)->nCount;
			{ // CHI-AUDIT: Gold lock — trade execute
				std::lock_guard<std::recursive_mutex> lock(m_goldLock);
				if (m_iGold + (*Iter)->nCount > COIN_MAX) m_iGold = COIN_MAX;
				else m_iGold += (*Iter)->nCount;
			}
			coinItr = Iter;
			continue;
		}

		_ITEM_TABLE pTable = g_pMain->GetItemPtr((*Iter)->nItemID);
		if (pTable.isnull())
			continue;

		int nSlot = FindSlotForItem((*Iter)->nItemID, (*Iter)->nCount);

		if (nSlot < 0 || nSlot >= SLOT_MAX + HAVE_MAX)
			return;

		_ITEM_DATA * pDstItem = GetItem(nSlot);
		_ITEM_DATA * pSrcItem = pUser->GetItem((*Iter)->bSrcPos);

		if (pDstItem == nullptr || pSrcItem == nullptr || pSrcItem && pSrcItem->nNum != pTable.m_iNum)
			return;

		if (pTable.m_bKind == 255 && pDstItem->nNum != 0)
			return;

		if (pDstItem->nNum == pSrcItem->nNum || pDstItem->nNum == 0)
		{
			if (pTable.isStackable())
				pDstItem->sCount += (*Iter)->nCount;
			else
				pDstItem->sCount = (*Iter)->nCount;

			if (pDstItem->sCount > MAX_ITEM_COUNT)
				pDstItem->sCount = MAX_ITEM_COUNT;

			uint64 serial = pSrcItem->nSerialNum;
			if (!serial) serial = g_pMain->GenerateItemSerial();

			if (pTable.isStackable())
			{
				if (!pSrcItem->sCount && !pDstItem->nNum)
					pDstItem->nSerialNum = serial;
				else if (!pDstItem->nNum)
					pDstItem->nSerialNum = g_pMain->GenerateItemSerial();
			}
			else pDstItem->nSerialNum = serial;

			pDstItem->sDuration = pSrcItem->sDuration;
			pDstItem->nNum = pSrcItem->nNum;

			if (pTable.m_bKind == 255 && pDstItem->sCount != 1 && pTable.m_bCountable == 0) {
				pDstItem->sCount = 1;
			}
			if (!pTable.m_bCountable) pDstItem->sCount = 1;

			// Set destination position for use in packet to client
			// to let them know where the item is.
			(*Iter)->bDstPos = (uint8)(nSlot - SLOT_MAX);

			// Remove the item from the other player.
			if (pSrcItem->sCount <= 0)
				memset(pSrcItem, 0, sizeof(_ITEM_DATA));
		}
		else
			return;
	}

	// Remove coins from the list so it doesn't get sent
	// with the rest of the packet.
	if (coinItr != pUser->m_ExchangeItemList.end())
	{
		delete *coinItr;
		pUser->m_ExchangeItemList.erase(coinItr);
	}
}

bool CUser::CheckExecuteExchange()
{
	if (m_sTradeStatue != 5
		|| !isTrading()
		|| isMining()
		|| isFishing()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing()
		|| isMerchanting()
		|| !isInGame())
		return false;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);

	if (pUser == nullptr)
		return false;

	if (!pUser->isInGame()
		|| pUser->m_sExchangeUser != GetSocketID()
		|| pUser->GetZoneID() != GetZoneID()
		|| pUser->isMining()
		|| pUser->isFishing()
		|| pUser->isSellingMerchantingPreparing()
		|| pUser->isBuyingMerchantingPreparing()
		|| pUser->isMerchanting()
		|| (pUser->GetNation() != GetNation() && !GetMap()->canTradeWithOtherNation()))
		return false;

	foreach(Iter, pUser->m_ExchangeItemList)
	{
		if ((*Iter) == nullptr)
			continue;

		if ((*Iter)->nItemID == ITEM_GOLD)
			continue;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr((*Iter)->nItemID);
		if (pTable.isnull())
			return false;

		int nSlot = FindSlotForItem((*Iter)->nItemID, (*Iter)->nCount);

		if (nSlot < 0 || nSlot >= SLOT_MAX + HAVE_MAX)
			return false;

		_ITEM_DATA * pDstItem = GetItem(nSlot);
		_ITEM_DATA * pSrcItem = pUser->GetItem((*Iter)->bSrcPos);

		if (pDstItem == nullptr || pSrcItem == nullptr || pSrcItem && pSrcItem->nNum != pTable.m_iNum)
			return false;

		if (pTable.m_bKind == 255 && pDstItem->nNum != 0)
			return false;

		if (pDstItem->nNum != pSrcItem->nNum && pDstItem->nNum != 0)
			return false;
	}

	return true;
}