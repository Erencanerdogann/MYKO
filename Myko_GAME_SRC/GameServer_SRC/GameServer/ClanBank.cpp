#include "stdafx.h"
#include "DBAgent.h"

#pragma region CUser::ClanWarehouseProcess(Packet & pkt)

void CUser::ClanWareHouseProcess(Packet& pkt)
{
	auto OpCode = pkt.read<uint8_t>();
	switch (OpCode)
	{
	case ClanBankOpcodes::ClanBankOpen:
		ClanWarehouseOpen(pkt);
		break;
	case ClanBankOpcodes::ClanBankInput:
		ClanWarehouseItemInput(pkt);
		break;
	case ClanBankOpcodes::ClanBankOutPut:
		ClanWarehouseItemOutput(pkt);
		break;
	case ClanBankOpcodes::ClanBankMove:
		ClanWarehouseItemMove(pkt);
		break;
	case ClanBankOpcodes::ClanBankInventoryMove:
		ClanWarehouseInventoryItemMove(pkt);
		break;
	}
}

void CUser::ClanWarehouseOpen(Packet& pkt)
{
	Packet result(WIZ_CLANWAREHOUSE);
	result << uint8_t(ClanBankOpcodes::ClanBankOpen);

	uint8_t ReturnValue = 1;

	if (!isInGame() || isDead() || isTrading()
		/*|| isStoreOpen()*/ || isMerchanting()
		|| isMining() || isFishing()
		|| isInAutoClan())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16_t sNpcID;
	pkt >> sNpcID;

	if (g_pMain->pServerSetting.ClanBankWithPremium)
	{
		if (m_bIsLoggingOut || !isInClan() || !sClanPremStatus || !g_pMain->ClanBankStatus)
		{
			ReturnValue = 0;
			goto fail_return;
		}
	}
	else
	{
		if (m_bIsLoggingOut || !isInClan() || !g_pMain->ClanBankStatus)
		{
			ReturnValue = 0;
			goto fail_return;
		}
	}

	auto* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Clan Bank : Couldn't Identify Clan.");
		ReturnValue = 0;
		goto fail_return;
	}

	{
		std::lock_guard<std::recursive_mutex> whLock(pKnights->m_warehouseLock);

		result << ReturnValue << pKnights->GetClanInnCoins();

		for (int32_t i = 0; i < WAREHOUSE_MAX; i++)
		{
			auto* pItem = &pKnights->m_sClanWarehouseArray[i];
			if (pItem == nullptr)
				continue;

			if (pItem->nExpirationTime != 0 && (pItem->nExpirationTime < (uint32_t)UNIXTIME))
				memset(pItem, 0, sizeof(_ITEM_DATA));

			result << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag << uint32_t(0x00) << pItem->nExpirationTime;
		}

		Send(&result);
		g_pMain->SendHelpDescription(this, "Clan Bank : Your clan bank is opened.");
		return;
	}

fail_return:
	result << ReturnValue;
	Send(&result);
}

void CUser::ClanWarehouseItemInput(Packet& pkt)
{
	_ITEM_TABLE pTable = _ITEM_TABLE();
	Packet result(WIZ_CLANWAREHOUSE);
	result << uint8_t(ClanBankOpcodes::ClanBankInput);

	Packet DBSave(WIZ_DB_SAVE_USER, uint8(ProcDbType::ClanBankSave));
	uint8_t ReturnValue = 1;

	if (!isInGame() || isDead() || isTrading() || isStoreOpen() || isMerchanting() || isMining() || isFishing())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16_t sNpcID;
	uint32_t nItemID, nCount;
	uint8_t Page, bSrcPos, bDstPos;

	pkt >> sNpcID;
	{
		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (m_bIsLoggingOut || !isInClan() || !sClanPremStatus || !g_pMain->ClanBankStatus)
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
		else
		{
			if (m_bIsLoggingOut || !isInClan() || !g_pMain->ClanBankStatus)
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
	}

	auto* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Clan Bank : Couldn't Identify Clan.");
		ReturnValue = 0;
		goto fail_return;
	}

	do {
		std::lock_guard<std::recursive_mutex> whLock(pKnights->m_warehouseLock);

		pkt >> nItemID >> Page >> bSrcPos >> bDstPos >> nCount;
		pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull()) { ReturnValue = 0; break; }

		if (nItemID == ITEM_GOLD)
		{
			{ // CHI-AUDIT: Gold lock — clan bank deposit
				std::lock_guard<std::recursive_mutex> lock(m_goldLock);
				if (m_iGold < nCount || pKnights->GetClanInnCoins() + nCount > COIN_MAX) { ReturnValue = 0; break; }
				pKnights->m_nMoney += nCount;
				m_iGold -= nCount;
			}
			result << ReturnValue;
			Send(&result);
			ClanBankInsertLog(pKnights, 0, 0, nCount, true);
			DBSave << pKnights->GetName();
			g_pMain->AddDatabaseRequest(DBSave, this);
			return;
		}

		if (Page > 3) { ReturnValue = 0; break; }
		uint16_t reference_pos = 24 * Page;

		if (bSrcPos > HAVE_MAX || reference_pos + bDstPos > WAREHOUSE_MAX
			|| (nItemID >= ITEM_NO_TRADE_MIN && nItemID <= ITEM_NO_TRADE_MAX)) { ReturnValue = 0; break; }

		auto* pSrcItem = GetItem(SLOT_MAX + bSrcPos);
		if (pSrcItem == nullptr || pSrcItem->nNum != nItemID || pSrcItem->sCount < nCount
			|| pSrcItem->isRented() || pSrcItem->isExpirationTime() || pSrcItem->isSealed()) { ReturnValue = 0; break; }

		if (pSrcItem->isDuplicate()) { ReturnValue = 2; break; }

		auto* pDstItem = &pKnights->m_sClanWarehouseArray[reference_pos + bDstPos];
		if (pDstItem == nullptr) { ReturnValue = 0; break; }

		if ((!pTable.isStackable() && pDstItem->nNum != 0)
			|| (pTable.isStackable() && pDstItem->nNum != 0 && pDstItem->nNum != pSrcItem->nNum)
			|| pSrcItem->sCount < nCount) { ReturnValue = 0; break; }

		if (pTable.isStackable())
			pDstItem->sCount += (uint16)nCount;
		else
			pDstItem->sCount = (uint16)nCount;

		if (pTable.isStackable())
			pSrcItem->sCount -= nCount;
		else
			pSrcItem->sCount = 0;

		uint64 serial = pSrcItem->nSerialNum;
		if (!serial) serial = g_pMain->GenerateItemSerial();

		if (pTable.isStackable())
			pDstItem->nSerialNum = g_pMain->GenerateItemSerial();
		else
			pDstItem->nSerialNum = serial;

		pDstItem->sDuration = pSrcItem->sDuration;
		pDstItem->bFlag = pSrcItem->bFlag;
		pDstItem->sRemainingRentalTime = pSrcItem->sRemainingRentalTime;
		pDstItem->nExpirationTime = pSrcItem->nExpirationTime;
		pDstItem->nNum = pSrcItem->nNum;

		if (pDstItem->sCount > MAX_ITEM_COUNT)
			pDstItem->sCount = MAX_ITEM_COUNT;

		if ((!pSrcItem->sCount) || (!pTable.m_bCountable) || (pTable.m_bKind == 255 && !pTable.m_bCountable))
			memset(pSrcItem, 0, sizeof(_ITEM_DATA));

		SetUserAbility(false);
		SendItemWeight();

		result << ReturnValue;
		Send(&result);

		if (pDstItem != nullptr)
			ClanBankInsertLog(pKnights, nItemID, nCount, 0, true);

		DBSave << pKnights->GetName();
		g_pMain->AddDatabaseRequest(DBSave, this);

		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (sClanPremStatus && pKnights->GetID() == GetClanID())
			{
				Packet ClanNotices;
				std::string buffer = pDstItem->sCount > 1
					? string_format("%s Clan from bank %d Count %s stopped.!", GetName().c_str(), pDstItem->sCount, pTable.m_sName.c_str())
					: string_format("%s Clan from bank %s stopped.!", GetName().c_str(), pTable.m_sName.c_str());
				ChatPacket::Construct(&ClanNotices, (uint8)ChatType::GM_CHAT, &buffer);
				pKnights->Send(&ClanNotices);
			}
			else if (sClanPremStatus) goDisconnect("1", __FUNCTION__);
		}
		else
		{
			if (pKnights->GetID() == GetClanID())
			{
				Packet ClanNotices;
				std::string buffer = pDstItem->sCount > 1
					? string_format("%s Clan from bank %d Count %s stopped.!", GetName().c_str(), pDstItem->sCount, pTable.m_sName.c_str())
					: string_format("%s Clan from bank %s stopped.!", GetName().c_str(), pTable.m_sName.c_str());
				ChatPacket::Construct(&ClanNotices, (uint8)ChatType::GM_CHAT, &buffer);
				pKnights->Send(&ClanNotices);
			}
			else goDisconnect("2", __FUNCTION__);
		}
		return;
	} while(0);

fail_return:
	result << ReturnValue;
	Send(&result);
}

void CUser::ClanWarehouseItemOutput(Packet& pkt)
{
	_ITEM_TABLE pTable = _ITEM_TABLE();
	Packet result(WIZ_CLANWAREHOUSE);
	result << uint8_t(ClanBankOpcodes::ClanBankOutPut);

	Packet DBSave(WIZ_DB_SAVE_USER, uint8(ProcDbType::ClanBankSave));
	uint8_t ReturnValue = 1;

	if (!isInGame() || isDead() || isTrading() || isStoreOpen() || isMerchanting() || isMining() || isFishing())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16_t sNpcID;
	uint32_t nItemID, nCount;
	uint8_t Page, bSrcPos, bDstPos;

	pkt >> sNpcID;
	//Clan Bankası Premiumlu veya Premiumsuz Acilsin 15.07.2020 start
	{
		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (m_bIsLoggingOut || !isInClan() || !sClanPremStatus || !g_pMain->ClanBankStatus || (!isClanLeader() && !isClanAssistant()))
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
		else
		{
			if (m_bIsLoggingOut || !isInClan() || !g_pMain->ClanBankStatus || (!isClanLeader() && !isClanAssistant()))
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
	}
	//Clan Bankası Premiumlu veya Premiumsuz Acilsin 15.07.2020 end


	auto* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Clan Bank : Couldn't Identify Clan.");
		ReturnValue = 0;
		goto fail_return;
	}

	do {
		std::lock_guard<std::recursive_mutex> whLock(pKnights->m_warehouseLock);

		pkt >> nItemID >> Page >> bSrcPos >> bDstPos >> nCount;
		pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull()) { ReturnValue = 0; break; }

		if (nItemID == ITEM_GOLD)
		{
			{ // clan bank withdraw gold lock
				std::lock_guard<std::recursive_mutex> lock(m_goldLock);
				// AS22 fix: bankada olmayan parayi cekme - uint64 underflow = gold dupe
				if (pKnights->m_nMoney < nCount || m_iGold + nCount > COIN_MAX)
				{
					ReturnValue = 0;
					break;
				}
				pKnights->m_nMoney -= nCount;
				m_iGold += nCount;
			}
			result << ReturnValue;
			Send(&result);
			UserDataSaveToAgent();
			ClanBankInsertLog(pKnights, 0, 0, nCount, false);
			DBSave << pKnights->GetName();
			g_pMain->AddDatabaseRequest(DBSave, this);
			return;
		}

		if (Page > 3) { ReturnValue = 0; break; }
		if (pTable.m_bCountable) { if (((pTable.m_sWeight * nCount) + m_sItemWeight) > m_sMaxWeight) { ReturnValue = 3; break; } }
		else { if ((pTable.m_sWeight + m_sItemWeight) > m_sMaxWeight) { ReturnValue = 3; break; } }

		uint16_t reference_pos = 24 * Page;
		if (reference_pos + bSrcPos > WAREHOUSE_MAX || bDstPos > HAVE_MAX) { ReturnValue = 0; break; }

		auto* pSrcItem = &pKnights->m_sClanWarehouseArray[reference_pos + bSrcPos];
		if (pSrcItem == nullptr || pSrcItem->nNum != nItemID || pSrcItem->sCount < nCount) { ReturnValue = 0; break; }
		if (pSrcItem->isDuplicate()) { ReturnValue = 2; break; }

		auto* pDstItem = GetItem(SLOT_MAX + bDstPos);
		if (pDstItem == nullptr) { ReturnValue = 0; break; }

		if ((!pTable.isStackable() && pDstItem->nNum != 0)
			|| (pTable.isStackable() && pDstItem->nNum != 0 && pDstItem->nNum != pSrcItem->nNum)
			|| pSrcItem->sCount < nCount) { ReturnValue = 0; break; }

		if (pTable.isStackable()) pDstItem->sCount += (uint16)nCount;
		else pDstItem->sCount = (uint16)nCount;

		if (pTable.isStackable()) pSrcItem->sCount -= nCount;
		else pSrcItem->sCount = 0;

		uint64 serial = pSrcItem->nSerialNum;
		if (!serial) serial = g_pMain->GenerateItemSerial();
		if (pTable.isStackable()) pDstItem->nSerialNum = g_pMain->GenerateItemSerial();
		else pDstItem->nSerialNum = serial;

		pDstItem->sDuration = pSrcItem->sDuration;
		pDstItem->bFlag = pSrcItem->bFlag;
		pDstItem->sRemainingRentalTime = pSrcItem->sRemainingRentalTime;
		pDstItem->nExpirationTime = pSrcItem->nExpirationTime;
		pDstItem->nNum = pSrcItem->nNum;

		if (pDstItem->sCount > MAX_ITEM_COUNT) pDstItem->sCount = MAX_ITEM_COUNT;
		if (!pSrcItem->sCount || !pTable.m_bCountable || (pTable.m_bKind == 255 && !pTable.m_bCountable))
			memset(pSrcItem, 0, sizeof(_ITEM_DATA));

		SetUserAbility(false);
		SendItemWeight();
		result << ReturnValue;
		Send(&result);

		if (pDstItem != nullptr)
			ClanBankInsertLog(pKnights, nItemID, nCount, 0, false);

		DBSave << pKnights->GetName();
		g_pMain->AddDatabaseRequest(DBSave, this);

		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (sClanPremStatus && pKnights->GetID() == GetClanID())
			{
				Packet ClanNotices;
				std::string buffer = pDstItem->sCount > 1
					? string_format("%s Clan from bank %d Count %s took.!", GetName().c_str(), pDstItem->sCount, pTable.m_sName.c_str())
					: string_format("%s Clan from bank %s took.!", GetName().c_str(), pTable.m_sName.c_str());
				ChatPacket::Construct(&ClanNotices, (uint8)ChatType::GM_CHAT, &buffer);
				pKnights->Send(&ClanNotices);
			}
			else if (sClanPremStatus) goDisconnect("3", __FUNCTION__);
		}
		else
		{
			if (pKnights->GetID() == GetClanID())
			{
				Packet ClanNotices;
				std::string buffer = pDstItem->sCount > 1
					? string_format("%s Clan from bank %d Count %s took.!", GetName().c_str(), pDstItem->sCount, pTable.m_sName.c_str())
					: string_format("%s Clan from bank %s took.!", GetName().c_str(), pTable.m_sName.c_str());
				ChatPacket::Construct(&ClanNotices, (uint8)ChatType::GM_CHAT, &buffer);
				pKnights->Send(&ClanNotices);
			}
			else goDisconnect("4", __FUNCTION__);
		}
		return;
	} while(0);

fail_return:
	result << ReturnValue;
	Send(&result);
}

void CUser::ClanWarehouseItemMove(Packet& pkt)
{
	_ITEM_TABLE pTable = _ITEM_TABLE();
	Packet result(WIZ_CLANWAREHOUSE);
	result << uint8_t(ClanBankOpcodes::ClanBankMove);

	Packet DBSave(WIZ_DB_SAVE_USER, uint8(ProcDbType::ClanBankSave));
	uint8_t ReturnValue = 1;

	if (!isInGame() || isDead() || isTrading() || isStoreOpen() || isMerchanting() || isMining() || isFishing())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16_t sNpcID;
	uint32_t nItemID;
	uint8_t Page, bSrcPos, bDstPos;

	pkt >> sNpcID;
	//Clan Bankası Premiumlu veya Premiumsuz Acilsin 15.07.2020 start
	{
		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (m_bIsLoggingOut || !isInClan() || !sClanPremStatus || !g_pMain->ClanBankStatus || (!isClanLeader() && !isClanAssistant()))
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
		else
		{
			if (m_bIsLoggingOut || !isInClan() || !g_pMain->ClanBankStatus || (!isClanLeader() && !isClanAssistant()))
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
	}
	//Clan Bankası Premiumlu veya Premiumsuz Acilsin 15.07.2020 end


	auto* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Clan Bank : Couldn't Identify Clan.");
		ReturnValue = 0;
		goto fail_return;
	}

	pkt >> nItemID >> Page >> bSrcPos >> bDstPos;
	if (Page > 3)
	{
		ReturnValue = 0;
		goto fail_return;
	}
	pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16_t reference_pos = 24 * Page;

	if (reference_pos + bSrcPos > WAREHOUSE_MAX || reference_pos + bDstPos > WAREHOUSE_MAX)
	{
		ReturnValue = 0;
		goto fail_return;
	}

	do {
		std::lock_guard<std::recursive_mutex> lock(pKnights->m_warehouseLock);

		auto* pSrcItem = &pKnights->m_sClanWarehouseArray[reference_pos + bSrcPos];
		auto* pDstItem = &pKnights->m_sClanWarehouseArray[reference_pos + bDstPos];
		if (pSrcItem == nullptr || pDstItem == nullptr)
		{
			ReturnValue = 0;
			break;
		}

		if (pSrcItem->nNum != nItemID || pDstItem->nNum != 0)
		{
			ReturnValue = 0;
			break;
		}

		if (pSrcItem->isDuplicate() || pDstItem->isDuplicate())
		{
			ReturnValue = 2;
			break;
		}

		memcpy(pDstItem, pSrcItem, sizeof(_ITEM_DATA));
		memset(pSrcItem, 0, sizeof(_ITEM_DATA));

		result << ReturnValue;
		Send(&result);

		DBSave << pKnights->GetName();
		g_pMain->AddDatabaseRequest(DBSave, this);
		return;
	} while (0);

fail_return:
	result << ReturnValue;
	Send(&result);
}

void CUser::ClanWarehouseInventoryItemMove(Packet& pkt)
{
	_ITEM_TABLE pTable = _ITEM_TABLE();
	Packet result(WIZ_CLANWAREHOUSE);
	result << uint8_t(ClanBankOpcodes::ClanBankInventoryMove);

	uint8_t ReturnValue = 1;

	if (!isInGame() || isDead() || isTrading() || isStoreOpen() || isMerchanting() || isMining() || isFishing())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16_t sNpcID;
	uint32_t nItemID;
	uint8_t Page, bSrcPos, bDstPos;

	pkt >> sNpcID;
	//Clan Bankası Premiumlu veya Premiumsuz Acilsin 15.07.2020 start
	{
		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (m_bIsLoggingOut || !isInClan() || !sClanPremStatus || !g_pMain->ClanBankStatus || (!isClanLeader() && !isClanAssistant()))
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
		else
		{
			if (m_bIsLoggingOut || !isInClan() || !g_pMain->ClanBankStatus || (!isClanLeader() && !isClanAssistant()))
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
	}
	//Clan Bankası Premiumlu veya Premiumsuz Acilsin 15.07.2020 end


	auto* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Clan Bank : Couldn't Identify Clan.");
		ReturnValue = 0;
		goto fail_return;
	}

	pkt >> nItemID >> Page >> bSrcPos >> bDstPos;
	pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	if (bSrcPos > HAVE_MAX || bDstPos > HAVE_MAX)
		goto fail_return;

	auto* pSrcItem = GetItem(SLOT_MAX + bSrcPos);
	auto* pDstItem = GetItem(SLOT_MAX + bDstPos);
	if (pSrcItem == nullptr || pDstItem == nullptr)
	{
		ReturnValue = 0;
		goto fail_return;
	}

	if (pSrcItem->nNum != nItemID)
	{
		ReturnValue = 0;
		goto fail_return;
	}

	if (pSrcItem->isDuplicate() || pDstItem->isDuplicate())
	{
		ReturnValue = 2;
		goto fail_return;
	}

	_ITEM_DATA PositionItem;
	memcpy(&PositionItem, pDstItem, sizeof(_ITEM_DATA));
	memcpy(pDstItem, pSrcItem, sizeof(_ITEM_DATA));
	memcpy(pSrcItem, &PositionItem, sizeof(_ITEM_DATA));

	result << ReturnValue;
	Send(&result);
	return;

fail_return:
	result << ReturnValue;
	Send(&result);
}
#pragma endregion
