#include "stdafx.h"

/**
* @brief	Checks whether the specified ItemID exists in the map passed by.
*			And returns the number of instance on success.
*
* @return	Returns the number of instance found in the map.
*/
static uint16 IsExistInMap(UpgradeItemsMap &map, int32 ItemID)
{
	uint16 temp = 0;
	foreach(itr, map)
	{
		if (itr->second.nItemID == ItemID)
			temp++;
	}
	return temp;
}

static UpgradeScrollType GetScrollType(uint32 ScrollNum)
{
	if (ScrollNum == 379221000
		|| ScrollNum == 379222000
		|| ScrollNum == 379223000
		|| ScrollNum == 379224000
		|| ScrollNum == 379225000
		|| ScrollNum == 379226000
		|| ScrollNum == 379227000
		|| ScrollNum == 379228000
		|| ScrollNum == 379229000
		|| ScrollNum == 379230000
		|| ScrollNum == 379231000
		|| ScrollNum == 379232000
		|| ScrollNum == 379233000
		|| ScrollNum == 379234000
		|| ScrollNum == 379235000
		|| ScrollNum == 379255000)
		return UpgradeScrollType::LowClassScroll;

	else if (ScrollNum == 379205000
		|| ScrollNum == 379206000
		|| ScrollNum == 379208000
		|| ScrollNum == 379209000
		|| ScrollNum == 379210000
		|| ScrollNum == 379211000
		|| ScrollNum == 379212000
		|| ScrollNum == 379213000
		|| ScrollNum == 379214000
		|| ScrollNum == 379215000
		|| ScrollNum == 379216000
		|| ScrollNum == 379217000
		|| ScrollNum == 379218000
		|| ScrollNum == 379219000
		|| ScrollNum == 379220000)
		return UpgradeScrollType::MiddleClassScroll;

	else if (ScrollNum == 379021000
		|| ScrollNum == 379022000
		|| ScrollNum == 379023000
		|| ScrollNum == 379024000
		|| ScrollNum == 379025000
		|| ScrollNum == 379030000
		|| ScrollNum == 379031000
		|| ScrollNum == 379032000
		|| ScrollNum == 379033000
		|| ScrollNum == 379034000
		|| ScrollNum == 379035000
		|| ScrollNum == 379138000
		|| ScrollNum == 379139000
		|| ScrollNum == 379140000
		|| ScrollNum == 379141000
		|| ScrollNum == 379016000
		|| ScrollNum == 379020000
		|| ScrollNum == 379018000
		|| ScrollNum == 379019000)
		return UpgradeScrollType::HighClassScroll;

	else if (ScrollNum == 379256000)
		return UpgradeScrollType::HighToRebirthScroll;

	else if (ScrollNum == 379257000)
		return UpgradeScrollType::RebirthClassScroll;

	else if (ScrollNum == 379159000
		|| ScrollNum == 379160000
		|| ScrollNum == 379161000
		|| ScrollNum == 379162000
		|| ScrollNum == 379163000
		|| ScrollNum == 379164000)
		return UpgradeScrollType::AccessoriesClassScroll;

	else if (ScrollNum == 379152000)
		return UpgradeScrollType::ClassScroll;

	return UpgradeScrollType::InvalidScroll;
}

/**
* @brief	Packet handler for the standard item upgrade system.
*
* @param	pkt	The packet.
*/
//void CUser::ItemUpgrade(Packet & pkt, uint8 nUpgradeType)
//{
//	Packet result(WIZ_ITEM_UPGRADE, nUpgradeType);
//	_ITEM_DATA  * pOriginItem;
//	_ITEM_TABLE * proto;
//	CNpc * pNpc;
//	DateTime time;
//	UpgradeScrollType m_ScrollType; // the Scroll type that shall be used for this upgrade.
//	UpgradeItemsMap nItems; // the items sent by the client 
//	UpgradeIterator OriginItem;
//	uint16 sNpcID;
//	int32 nItemID; int8 bPos;
//	UpgradeScrollType pUserScroll = UpgradeScrollType::InvalidScroll;
//
//	int8 bType = UpgradeType::UpgradeTypeNormal, bResult = UpgradeErrorCodes::UpgradeNoMatch, ItemClass = 0;
//	bool trina = false, karivdis = false, logositem = false, evilingot = false; 
//
//	if (isTrading() || isMerchanting() || isMining() || isFishing())
//	{
//		bResult = UpgradeTrading;
//		goto fail_return;
//	}
//
//	pkt >> bType >> sNpcID;
//	if (bType != UpgradeType::UpgradeTypeNormal && bType != UpgradeType::UpgradeTypePreview) goto fail_return;
//
//	pNpc = g_pMain->GetNpcPtr(sNpcID, GetZoneID());
//	if (pNpc == nullptr || pNpc->GetType() != NPC_ANVIL) goto fail_return;
//
//	if (m_sUserUpgradeCount >= g_pMain->pServerSetting.UserMaxUpgradeCount)
//		goto fail_return;
//
//	if ((UNIXTIME - m_tUpgradeDelay) < UPGRADE_DELAY) //30.08.2020 Seri Upgrade Fix
//	{
//		Packet pkt;
//		pkt.clear();
//		pkt.Initialize(XSafe);
//		pkt << uint8(XSafeOpCodes::ERRORMSG) << uint8(1) << uint8(1) << std::string("Magic Anvil") << std::string("You are doing item upgrade very fast");
//		Send(&pkt);
//		goto fail_return;
//	}
//
//	uint32 OriginItemID[3] = { 0 }; int8 OriginItemPos[3] = { -1 };
//	int counter = 0;
//	for (int i = 0; i < 10; i++) // client sends 10 items in a row, starting with the item to be upgraded and latter the required items
//	{
//		pkt >> nItemID >> bPos;
//
//		if (nUpgradeType == ITEM_ACCESSORIES && i < 3) {
//			OriginItemID[i] = nItemID; OriginItemPos[i] = bPos;
//			if (OriginItemID[0] != OriginItemID[i]) goto fail_return;
//		}
//
//
//		// Invalid slot ID
//		if (bPos != -1 && bPos >= HAVE_MAX)
//			goto fail_return;
//
//		// We do not care where the user put the scroll in his screen. We do only care
//		// is that whether the received Item IDs and POSs are valid.
//		if (nItemID > 0 && bPos >= 0 && bPos < HAVE_MAX)
//		{
//			ITEM_UPGRADE_ITEM pUpgradeItem;
//			pUpgradeItem.nItemID = nItemID;
//			pUpgradeItem.bPos = bPos;
//			nItems.insert(std::make_pair(counter++, pUpgradeItem));
//		}
//	}
//
//	if (nItems.empty() 
//		|| ((nUpgradeType == ItemUpgradeOpcodes::ITEM_UPGRADE 
//			|| nUpgradeType == ItemUpgradeOpcodes::ITEM_UPGRADE_REBIRTH) && nItems.size() > 3)
//		|| (nUpgradeType == ItemUpgradeOpcodes::ITEM_ACCESSORIES && (nItems.size() < 4 || nItems.size() > 4))) // if no items are available, then it should fail.
//		goto fail_return;
//
//	if (nUpgradeType == ITEM_ACCESSORIES) {
//		for (int i = 0; i < 3; i++) {
//			if (!OriginItemID[i]) goto fail_return;
//			for (int j = 0; j < 3; j++) {
//				if (OriginItemPos[i] == OriginItemPos[j] && i != j) goto fail_return;
//			}
//		}
//	}
//
//	foreach(itr, nItems) // we shall check if the items sent by the client do really exist on the user.
//	{
//		_ITEM_DATA  * pItem = GetItem(SLOT_MAX + itr->second.bPos);
//		_ITEM_TABLE * pTable = g_pMain->GetItemPtr(itr->second.nItemID);
//		// Here we check and iterate through the users inventory and check
//		// whether the item ids and positions are valid in the users inventory.
//		// The reason why we do that is that we never trust a player.
//		if (pItem == nullptr
//			|| pTable == nullptr
//			|| pTable->m_bKind == 250
//			|| pTable->m_bKind == 252
//			|| pTable->m_bKind == 253
//			|| pTable->m_bKind == 254
//			//|| pTable->m_bKind == 255
//			|| pItem->nNum != itr->second.nItemID)
//			goto fail_return;
//		// We shall allow no user to upgrade items with protected types like bounded, sealed etc. 
//		// Even worse trying to upgrade a duplicate item cant be accepted.
//		// The official behavious is to check the origin item only but c'mon
//		// a user shouldnt upgrade an item by using a sealed scroll too.
//		else if (pItem->isDuplicate()
//			|| pItem->isRented()
//			|| pItem->isSealed()
//			|| pItem->isExpirationTime())
//		{
//			bResult = UpgradeErrorCodes::UpgradeRental;
//			goto fail_return;
//		}
//	}
//
//	// Now we are about to put the data on our pointers, iterators that
//	// we are going to use for the rest of upgrade process.
//	OriginItem = nItems.begin(); // this is our first item in the map which is going to be upgraded.
//	if (OriginItem == nItems.end())
//		goto fail_return;
//
//	proto = g_pMain->GetItemPtr(OriginItem->second.nItemID);
//	pOriginItem = GetItem(SLOT_MAX + OriginItem->second.bPos);
//	if (proto == nullptr || pOriginItem == nullptr)
//		goto fail_return;
//
//	foreach(itr, nItems)
//	{
//		pUserScroll = GetScrollType(itr->second.nItemID);
//		if (pUserScroll != UpgradeScrollType::InvalidScroll)
//			break;
//	}
//
//	if (pUserScroll == UpgradeScrollType::InvalidScroll)
//		goto fail_return;
//
//	int nReqOriginItem = OriginItem->second.nItemID; bool logosactive = false;
//	{
//		_ITEM_UPGRADE * pUpgrade = nullptr;
//		foreach_stlmap(itr, g_pMain->m_ItemUpgradeArray)
//		{
//			pUpgrade = itr->second;
//			if (pUpgrade == nullptr)
//				continue;
//
//			if (pUpgrade->sOriginItem != nReqOriginItem)
//				continue;
//
//			if ((OriginItem->second.nItemID / MIN_ITEM_ID) != pUpgrade->nIndex / 100000 && pUpgrade->nIndex < 300000)
//				continue;
//
//			if (IsExistInMap(nItems, ITEM_TRINA)) // Trina Piece 
//				trina = true;
//
//			if (IsExistInMap(nItems, ITEM_KARIVDIS)) // Karvis Rebirth Trina
//				karivdis = true;
//
//			if (IsExistInMap(nItems, ITEM_LOGOS)) logositem = true;
//
//			if ((trina && karivdis)
//				|| IsExistInMap(nItems, ITEM_TRINA) > 1
//				|| IsExistInMap(nItems, ITEM_KARIVDIS) > 1)
//				goto fail_return;
//
//			if (logositem && (trina || karivdis)) goto fail_return;
//			if (trina && (logositem || karivdis)) goto fail_return;
//			if (karivdis && (logositem || trina)) goto fail_return;
//
//			// Up to this point, we have checked basic requirements and whether
//			// the user is trying to fool us with dummy packets. Yet, it still
//			// needs to be checked whether the items are valid for this kind
//			// of upgrade.
//
//			if (pUpgrade->bOriginType != -1)
//			{
//				switch (pUpgrade->bOriginType)
//				{
//				case 0:
//					if (proto.m_bKind != 11 && proto.m_bKind != 21 && proto.m_bKind != 52)
//						continue;
//					break;
//				case 1:
//					if (proto.m_bKind != 21 && proto.m_bKind != 22)
//						continue;
//					break;
//				case 2:
//				case 26:
//					if (proto.m_bKind != 22)
//						continue;
//					break;
//				case 3:
//					if (proto.m_bKind != 31)
//						continue;
//					break;
//				case 4:
//				case 28:
//					if (proto.m_bKind != 32)
//						continue;
//					break;
//				case 5:
//					if (proto.m_bKind != 41 && proto.m_bKind != 62)
//						continue;
//					break;
//				case 6:
//				case 30:
//					if (proto.m_bKind != 42)
//						continue;
//					break;
//				case 7:
//				case 31:
//					if (proto.m_bKind != 51)
//						continue;
//					break;
//				case 8:
//				case 32:
//					if (proto.m_bKind != 52)
//						continue;
//					break;
//				case 9:
//					if (proto.m_bKind != 70 && proto.m_bKind != 71)
//						continue;
//					break;
//				case 10:
//					if (proto.m_bKind != 21 && proto.m_bKind != 110)
//						continue;
//					break;
//				case 11:
//					if (proto.m_bKind != 41 && proto.m_bKind != 181)
//						continue;
//					break;
//				case 12:
//				case 36:
//					if (proto.m_bKind != 60)
//						continue;
//					break;
//				case 13:
//				case 14:
//				case 15:
//				case 16:
//				case 17:
//				case 37:
//				case 38:
//				case 39:
//				case 40:
//				case 41:
//					if (proto.m_bKind != 210 && proto.m_bKind != 220 && proto.m_bKind != 230 && proto.m_bKind != 240)
//						continue;
//					break;
//				case 18:
//					if (proto.m_bKind != 91)
//						continue;
//					break;
//				case 19:
//					if (proto.m_bKind != 92)
//						continue;
//					break;
//				case 20:
//					if (proto.m_bKind != 93)
//						continue;
//					break;
//				case 21:
//					if (proto.m_bKind != 94)
//						continue;
//					break;
//				case 23:
//					if (proto.m_bKind != 91
//						&& proto.m_bKind != 92
//						&& proto.m_bKind != 93
//						&& proto.m_bKind != 94
//						&& proto.m_bKind != 11
//						&& proto.m_bKind != 21
//						&& proto.m_bKind != 22
//						&& proto.m_bKind != 32
//						&& proto.m_bKind != 51
//						&& proto.m_bKind != 42
//						&& proto.m_bKind != 70
//						&& proto.m_bKind != 71
//						&& proto.m_bKind != 60
//						&& proto.m_bKind != 110
//						&& proto.m_bKind != 181)
//						continue;
//					break;
//				case 24:
//					if (proto.m_bKind != 11)
//						continue;
//					break;
//				case 25:
//					if (proto.m_bKind != 21)
//						continue;
//					break;
//				case 27:
//					if (proto.m_bKind != 110 && proto.m_bKind != 31)
//						continue;
//					break;
//				case 29:
//					if (proto.m_bKind != 41)
//						continue;
//					break;
//				case 33:
//					if (proto.m_bKind != 70 && proto.m_bKind != 71 && proto.m_bKind != 110)
//						continue;
//					break;
//				case 34:
//					if (proto.m_bKind != 110)
//						continue;
//					break;
//				case 35:
//					if (proto.m_bKind != 181 && proto.m_bKind != 110)
//						continue;
//					break;
//				case 43:
//				case 44:
//					if (proto.m_bKind != 140)
//						continue;
//					break;
//				}
//			}
//
//			if ((OriginItem->second.nItemID / MIN_ITEM_ID) != (pUpgrade->nIndex / 100000) && ((pUpgrade->nIndex / 100000) == 1 || (pUpgrade->nIndex / 100000) == 2))
//				continue;
//
//			bool isValidMatch = false;
//			// Does our upgrade attempt match the requirements for this upgrade entry?
//			for (int x = 1; x < MAX_ITEMS_REQ_FOR_UPGRADE; x++)
//			{
//				if (pUpgrade->nReqItem[x - 1] == 0)
//					continue;
//
//				if (IsExistInMap(nItems, pUpgrade->nReqItem[x - 1]))
//				{
//					isValidMatch = true;
//					break;
//				}
//			}
//
//			// Not a valid match, try another row.
//			if (!isValidMatch)
//				continue;
//
//			switch (proto.ItemClass)
//			{
//			case 0:
//				if (proto.ItemClass == 0)
//				{
//					if (proto.m_bKind == 91
//						|| proto.m_bKind == 92
//						|| proto.m_bKind == 93
//						|| proto.m_bKind == 94)
//						m_ScrollType = UpgradeScrollType::AccessoriesClassScroll;
//					else
//						m_ScrollType = UpgradeScrollType::LowClassScroll;
//					break;
//				}
//			case 1:
//				if (proto.ItemClass == 1)
//					m_ScrollType = UpgradeScrollType::LowClassScroll;
//				break;
//			case 2:
//				if (proto.ItemClass == 2)
//					m_ScrollType = UpgradeScrollType::MiddleClassScroll;
//				break;
//			case 3:
//				if (proto.ItemClass == 3)
//					m_ScrollType = UpgradeScrollType::HighClassScroll;
//				break;
//			case 4:
//				if (proto.ItemClass == 4)
//					m_ScrollType = UpgradeScrollType::HighToRebirthScroll;
//				break;
//			case 5:
//				if (proto.ItemClass == 5)
//					m_ScrollType = UpgradeScrollType::RebirthClassScroll;
//				break;
//			case 7:
//				if (proto.ItemClass == 7)
//					m_ScrollType = UpgradeScrollType::MiddleClassScroll;
//				break;
//			case 8:
//				if (proto.ItemClass == 8)
//					m_ScrollType = UpgradeScrollType::HighToRebirthScroll;
//				break;
//			case 22:
//				if (proto.ItemClass == 22)
//					m_ScrollType = UpgradeScrollType::AccessoriesClassScroll;
//				break;
//			case 33:
//				if (proto.ItemClass == 33)
//					m_ScrollType = UpgradeScrollType::HighClassScroll;
//				break;
//			case 34:
//				if (proto.ItemClass == 34)
//					m_ScrollType = UpgradeScrollType::HighToRebirthScroll;
//				break;
//			case 35:
//				if (proto.ItemClass == 35)
//					m_ScrollType = UpgradeScrollType::RebirthClassScroll;
//				break;
//			default:
//				m_ScrollType = UpgradeScrollType::InvalidScroll;
//				break;
//			}
//
//			if (m_ScrollType == UpgradeScrollType::LowClassScroll
//				&& (pUserScroll != UpgradeScrollType::LowClassScroll
//					&& pUserScroll != UpgradeScrollType::MiddleClassScroll
//					&& pUserScroll != UpgradeScrollType::HighClassScroll
//					&& pUserScroll != UpgradeScrollType::ClassScroll))
//				goto fail_return;
//			else if (m_ScrollType == UpgradeScrollType::MiddleClassScroll
//				&& (pUserScroll != UpgradeScrollType::MiddleClassScroll
//					&& pUserScroll != UpgradeScrollType::HighClassScroll
//					&& pUserScroll != UpgradeScrollType::ClassScroll))
//				goto fail_return;
//			else if (m_ScrollType == UpgradeScrollType::HighClassScroll
//				&& (pUserScroll != UpgradeScrollType::HighClassScroll
//					&& pUserScroll != UpgradeScrollType::HighToRebirthScroll
//					&& pUserScroll != UpgradeScrollType::ClassScroll))
//				goto fail_return;
//			else if (m_ScrollType == UpgradeScrollType::HighToRebirthScroll
//				&& (pUserScroll != UpgradeScrollType::HighToRebirthScroll
//					&& pUserScroll != UpgradeScrollType::HighClassScroll))
//				goto fail_return;
//			else if (m_ScrollType == UpgradeScrollType::RebirthClassScroll
//				&& pUserScroll != UpgradeScrollType::RebirthClassScroll)
//				goto fail_return;
//			else if (m_ScrollType == UpgradeScrollType::AccessoriesClassScroll
//				&& pUserScroll != UpgradeScrollType::AccessoriesClassScroll)
//				goto fail_return;
//
//			if (!hasCoins(pUpgrade->nReqNoah))
//			{
//				bResult = UpgradeNeedCoins;
//				goto fail_return;
//			}
//
//			bResult = UpgradeSucceeded;
//			break;
//		}
//
//		// If we ran out of upgrades to search through, it failed.
//		if (bResult != UpgradeSucceeded || pUpgrade == nullptr)
//			goto fail_return;
//
//		if (karivdis)
//		{
//			if (pUserScroll != UpgradeScrollType::RebirthClassScroll)
//			{
//				bResult = UpgradeNoMatch;
//				goto fail_return;
//			}
//		}
//		
//		if (trina)
//		{
//			if (pUserScroll == UpgradeScrollType::RebirthClassScroll)
//			{
//				bResult = UpgradeNoMatch;
//				goto fail_return;
//			}
//		}
//
//
//		//int rand = myrand(0, myrand(9000, 10000)), GenRate;
//		//
//		//GenRate = pUpgrade->sGenRate;
//
//		/*if (GenRate == UpgradeFailed)
//		{
//			bResult = UpgradeNoMatch;
//			goto fail_return;
//		}*/
//
//		////Trina & Kravdis Upgrade Rate
//		//switch (pUserScroll)
//		//{
//		//case RebirthClassScroll:
//		//	if (GenRate > 2000)   // Karvdis oran�.
//		//		GenRate = 2000;   // Karvdis oran�.
//
//		//	if (karivdis)
//		//		GenRate += 3000;     // Karvdis oran� %30=3000
//		//	break;
//
//		//case AccessoriesClassScroll:
//		//	break;
//
//		//default:
//		//	if (trina)
//		//		GenRate += (GenRate * 50) / 100;   // Trina Oran� �rne�in 50 olursa - var olan upgradenin oran�na %50 ekler.
//		//	break;
//		//}
//
//		//if (GenRate > 10000)
//		//	GenRate = 10000;
//

//		// Generate a random number, test if the item burned.
//		int rand = myrand(1, 10000), GenRate = 0;
//		if (trina || karivdis)
//		{
//			if (trina)
//			{
//				GenRate += pUpgrade->sGenRate + pUpgrade->sTrinaRate;
//				if (GenRate > 10000)
//					GenRate = 10000;
//			}
//
//			if (karivdis)
//			{
//				GenRate += pUpgrade->sGenRate + pUpgrade->sKarivdisRate;
//				if (GenRate > 10000)
//					GenRate = 10000;
//			}
//		}
//		else
//			GenRate = pUpgrade->sGenRate;
//		
//		if (GenRate == UpgradeFailed)
//		{
//			bResult = UpgradeNoMatch;
//			goto fail_return;
//		}
//
//		if (bType == UpgradeTypeNormal && GenRate < rand)
//		{
//			bResult = UpgradeFailed;
//			if (logositem) { 
//				if (myrand(0, 10000) > 5000) { logosactive = true; goto devamet; }
//			}
//
//			if (!logositem && pUserScroll == UpgradeScrollType::RebirthClassScroll) {
//				if (myrand(0, 10000) > 5000) {
//					pOriginItem->nNum = 810671000;
//					pOriginItem->sDuration = 1;
//
//					OriginItem->second.nItemID = 810671000;
//					evilingot = true;
//					goto devamet;
//				}
//			}
//
//			memset(pOriginItem, 0, sizeof(_ITEM_DATA));
//
//			// Send upgrade notice.
//			ItemUpgradeNotice(proto, UpgradeFailed);
//		
//		devamet:
//			// Rob gold upgrade noah
//			GoldLose(pUpgrade->nReqNoah, true);
//			m_sUserUpgradeCount++;
//			m_tUpgradeDelay = UNIXTIME; //30.08.2020 Seri Upgrade Fix
//		}
//		else
//		{
//			// Generate the new item ID
//			int nNewItemID = pOriginItem->nNum + pUpgrade->nGiveItem;
//
//			int nNewItemLevel = nNewItemID % 10;
//
//			// Does this new item exist?
//			_ITEM_TABLE * newProto = g_pMain->GetItemPtr(nNewItemID);
//			if (newProto == nullptr)
//			{ // if not, just say it doesn't match. No point removing the item anyway (like official :/).
//				bResult = UpgradeNoMatch;
//				goto fail_return;
//			}
//
//			if (bType == UpgradeType::UpgradeTypePreview)
//			{
//				Packet UpgradeRate(XSafe);
//				UpgradeRate << uint8(XSafeOpCodes::UPGRADE_RATE) << uint32((GenRate) / 100);
//				Send(&UpgradeRate);
//			}
//
//			if (bType != UpgradeTypePreview)
//			{
//				// Update the user's item in their inventory with the new item
//				pOriginItem->nNum = nNewItemID;
//
//				// Reset the durability also, to the new cap.
//				pOriginItem->sDuration = newProto->m_sDuration;
//
//				// Send upgrade notice.
//				ItemUpgradeNotice(newProto, UpgradeSucceeded);
//
//				// Rob gold upgrade noah
//				GoldLose(pUpgrade->nReqNoah, true);
//				m_sUserUpgradeCount++;
//				m_tUpgradeDelay = UNIXTIME; //30.08.2020 Seri Upgrade Fix
//
//				if(pUserScroll == UpgradeScrollType::HighClassScroll) pUserDailyRank.UPCounterBles++;
//			}
//
//			// Replace the item ID in the list for the packet
//			OriginItem->second.nItemID = nNewItemID;
//		}
//
//		uint32 logitemid[9]{ 0 }; uint16 logitemcount[9]{ 0 };
//		// Remove the source item 
//		if (bType != UpgradeTypePreview)
//		{
//			// Remove all required items, if applicable.
//			for (int i = 1; i < MAX_ITEMS_REQ_FOR_UPGRADE; i++)
//			{
//				auto xitr = nItems.find(i);
//				if (xitr == nItems.end())
//					continue;
//
//				_ITEM_DATA * pItem = GetItem(SLOT_MAX + xitr->second.bPos);
//				if (pItem == nullptr
//					|| pItem->nNum == 0
//					|| pItem->sCount == 0)
//					continue;
//
//				logitemid[i] = xitr->second.nItemID;
//				logitemcount[i] = 1;
//
//				pItem->sCount--;
//				if (pItem->sCount == 0 && pItem->nNum == xitr->second.nItemID)
//					memset(pItem, 0, sizeof(pItem));
//			}
//		}
//
//		result << bType << bResult;
//		if (!bResult) { bool recover = evilingot || logosactive; result << recover; }
//		foreach(i, nItems) result << i->second.nItemID << i->second.bPos;
//		Send(&result);
//
//		// Send the result to everyone in the area
//		// (i.e. make the anvil do its burned/upgraded indicator thing)
//		result.Initialize(WIZ_OBJECT_EVENT);
//		result << uint8(OBJECT_ANVIL) << bResult << sNpcID;
//		SendToRegion(&result, nullptr, GetEventRoom());
//
//		if (bType != UpgradeTypePreview) UpgradeInsertLog(proto.Getnum(), pUpgrade->nReqNoah, (uint8)pUserScroll, bResult, logitemid, logitemcount);
//	}
//
//	return;
//fail_return:
//	result << bType << bResult;
//	if (!bResult) result << uint8(0);
//	foreach(i, nItems)
//	result << i->second.nItemID << i->second.bPos;
//	Send(&result);
//}
#
void CUser::ItemUpgrade(Packet& pkt, uint8 nUpgradeType)
{
#define ITEM_LOW_CLASS_TRINA 353000000
#define ITEM_MIDDLE_CLASS_TRINA 352900000
	Packet result(WIZ_ITEM_UPGRADE, nUpgradeType);
	_ITEM_DATA* pOriginItem;
	_ITEM_TABLE  proto = _ITEM_TABLE();
	CNpc* pNpc;
	UpgradeScrollType m_ScrollType = UpgradeScrollType::InvalidScroll;; // the Scroll type that shall be used for this upgrade.
	UpgradeItemsMap nItems; // the items sent by the client 
	UpgradeIterator OriginItem;
	uint16 sNpcID;
	int32 nItemID; int8 bPos;
	uint32 ScrollID = 0;
	UpgradeScrollType pUserScroll = UpgradeScrollType::InvalidScroll;
	/*eneskcbs*/
	uint8 byGrade = 0;
	uint32 byGenRate = 0;
	uint32 NewItemID = 0;
	uint32 nReqCoins = 0;
	int8 bType = UpgradeType::UpgradeTypeNormal, bResult = UpgradeErrorCodes::UpgradeNoMatch, ItemClass = 0;
	bool trina = false, karivdis = false, Accessories = false, isLowClassTrina = false,isMiddleClassTrina=false;
	bool ringtrina = false;
	DWORD CheckTime = GetTickCount();

	bool isrecycle = false, logositemac = false;

	if (isDead() || isTrading() || isStoreOpen() || isMerchanting() || isSellingMerchant() || isBuyingMerchant() || isMining())
	{
		bResult = UpgradeTrading;
		goto fail_return;
	}

	if (bType != UpgradeTypeNormal && bType != UpgradeTypePreview) {
		bResult = UpgradeNoMatch;
		goto fail_return;
	}

	if (m_sUserUpgradeCount >= g_pMain->pServerSetting.UserMaxUpgradeCount)
		goto fail_return;
	
	if ((UNIXTIME - m_tUpgradeDelay) < UPGRADE_DELAY) //30.08.2020 Seri Upgrade Fix
	{
		Packet newpkt(XSafe, uint8(XSafeOpCodes::ERRORMSG));
		newpkt << uint8(1) << uint8(1) << std::string("Magic Anvil") << std::string("You are doing item upgrade very fast");
		Send(&newpkt);
		goto fail_return;
	}

	m_tUpgradeDelay = UNIXTIME;
	pkt >> bType; // either preview or upgrade, need to allow for these types
	pkt >> sNpcID;

	if (bType > 2 || bType < 1)
		goto fail_return;
	pNpc = g_pMain->GetNpcPtr(sNpcID, GetZoneID());

	if (pNpc == nullptr
		|| pNpc->GetType() != NPC_ANVIL)
		goto fail_return;

	int32 snItemID[10];
	int counter = 0;
	for (int i = 0; i < 10; i++) // client sends 10 items in a row, starting with the item to be upgraded and latter the required items
	{
		pkt >> nItemID >> bPos;

		// Invalid slot ID
		if (bPos != -1 && bPos >= HAVE_MAX)
			goto fail_return;

		snItemID[i] = nItemID;

		// We do not care where the user put the scroll in his screen. We do only care
		// is that whether the received Item IDs and POSs are valid.
		if (nItemID > 0 && bPos >= 0 && bPos < HAVE_MAX)
		{

			ITEM_UPGRADE_ITEM pUpgradeItem;
			pUpgradeItem.nItemID = nItemID;
			pUpgradeItem.bPos = bPos;
			nItems.insert(std::make_pair(counter++, pUpgradeItem));
		}
	}

	if (nItems.empty()) // if no items are available, then it should fail.
		goto fail_return;

	foreach(itr, nItems) // we shall check if the items sent by the client do really exist on the user.
	{
		_ITEM_DATA* pItem = GetItem(SLOT_MAX + itr->second.bPos);
		_ITEM_TABLE  pTable = g_pMain->GetItemPtr(itr->second.nItemID);

		// Here we check and iterate through the users inventory and check
		// whether the item ids and positions are valid in the users inventory.
		// The reason why we do that is that we never trust a player.
		if (pItem == nullptr
			|| pTable.isnull()
			|| pItem->nNum != itr->second.nItemID)
			goto fail_return;
		// We shall allow no user to upgrade items with protected types like bounded, sealed etc. 
		// Even worse trying to upgrade a duplicate item cant be accepted.
		// The official behavious is to check the origin item only but c'mon
		// a user shouldnt upgrade an item by using a sealed scroll too.
		else if (pItem->isBound()
			|| pItem->isDuplicate()
			|| pItem->isRented()
			|| pItem->isSealed())
		{
			bResult = UpgradeErrorCodes::UpgradeRental;
			goto fail_return;
		}
	}


	// Now we are about to put the data on our pointers, iterators that
	// we are going to use for the rest of upgrade process.
	OriginItem = nItems.begin(); // this is our first item in the map which is going to be upgraded.
	if (OriginItem == nItems.end())
		goto fail_return;

	proto = g_pMain->GetItemPtr(OriginItem->second.nItemID);
	pOriginItem = GetItem(SLOT_MAX + OriginItem->second.bPos);
	if (proto.isnull() || pOriginItem == nullptr)
		goto fail_return;

	uint32 scroll_id = 0;
	foreach(itr, nItems)
	{
		pUserScroll = GetScrollType(itr->second.nItemID);
		scroll_id = itr->second.nItemID;
		if (pUserScroll != UpgradeScrollType::InvalidScroll)
			break;
	}

	if (pUserScroll == UpgradeScrollType::InvalidScroll)
		goto fail_return;

	int nReqOriginItem = OriginItem->second.nItemID;
	{
		foreach(itr, g_pMain->m_loadUpgrade) {

			if (nReqOriginItem != itr->second->ItemNumber)
				continue;
			if (pUserScroll != UpgradeScrollType::AccessoriesClassScroll) {
				if (snItemID[1] > 0
					&& snItemID[1] != ITEM_TRINA
					&& snItemID[1] != ITEM_KARIVDIS
					&& snItemID[1] != ITEM_MIDDLE_CLASS_TRINA
					&& snItemID[1] != ITEM_LOW_CLASS_TRINA
					&& snItemID[1] != 890092000
					&& itr->second->ScrollNumber != snItemID[1])
					continue;

				if (snItemID[2] > 0
					&& snItemID[2] != ITEM_TRINA
					&& snItemID[2] != ITEM_KARIVDIS
					&& snItemID[2] != ITEM_MIDDLE_CLASS_TRINA
					&& snItemID[2] != ITEM_LOW_CLASS_TRINA
					&& snItemID[2] != 890092000
					&& itr->second->ScrollNumber != snItemID[2])
					continue;
			}

			if (pUserScroll == UpgradeScrollType::AccessoriesClassScroll)
			{
				if(snItemID[3] != 354000000
					&& itr->second->ScrollNumber != snItemID[3])
				continue;
			}
				
			if (IsExistInMap(nItems, ITEM_TRINA)) // Trina Piece 
				trina = true;

			if (IsExistInMap(nItems, ITEM_KARIVDIS)) // Karvis Rebirth Trina
				karivdis = true;	

			if (IsExistInMap(nItems, 890092000)) //ITEM_BLESSINGLOGOS
				logositemac = true;	
			
			if (IsExistInMap(nItems, ITEM_MIDDLE_CLASS_TRINA)) // Karvis Rebirth Trina
				isMiddleClassTrina = true;

			if (IsExistInMap(nItems, ITEM_LOW_CLASS_TRINA)) // Karvis Rebirth Trina
				isLowClassTrina = true;
			
			if (IsExistInMap(nItems, 354000000))
				ringtrina = true;

			if (ringtrina && IsExistInMap(nItems, 354000000) > 1)
				goto fail_return;

			if ((trina && karivdis)
				|| IsExistInMap(nItems, ITEM_TRINA) > 1
				|| IsExistInMap(nItems, ITEM_KARIVDIS) > 1)
				goto fail_return;

			if((isLowClassTrina && isMiddleClassTrina)
				|| IsExistInMap(nItems, ITEM_MIDDLE_CLASS_TRINA) > 1
				|| IsExistInMap(nItems, ITEM_LOW_CLASS_TRINA) > 1)
				goto fail_return;

			if (logositemac && (trina || karivdis || isLowClassTrina || isMiddleClassTrina))
				goto fail_return;

			if (logositemac
				&& proto.m_ItemType != 11 && proto.m_ItemType != 12
				&& proto.m_ItemType != 4 && proto.m_ItemType != 5)
				goto fail_return;

			bool isValidMatch = false;

			m_ScrollType = UpgradeScrollType::InvalidScroll;
			if (proto.ItemClass == 1) // low class scroll
				m_ScrollType = UpgradeScrollType::LowClassScroll;
			else if (proto.ItemClass == 2) // middle class scroll
				m_ScrollType = UpgradeScrollType::MiddleClassScroll;
			else if (proto.ItemClass == 3) // high class scroll
				m_ScrollType = UpgradeScrollType::HighClassScroll;
			else if (proto.ItemClass == 4) // rebirth scroll
				m_ScrollType = UpgradeScrollType::RebirthClassScroll;
			else if (proto.ItemClass == 8) // accessories class scroll
				m_ScrollType = UpgradeScrollType::AccessoriesClassScroll;

			if (m_ScrollType == UpgradeScrollType::LowClassScroll &&
				(pUserScroll != UpgradeScrollType::LowClassScroll
					&& pUserScroll != UpgradeScrollType::MiddleClassScroll
					&& pUserScroll != UpgradeScrollType::HighClassScroll))
				goto fail_return;
			// Middle Class Scroll Match
			else if (m_ScrollType == UpgradeScrollType::MiddleClassScroll &&
				(pUserScroll != UpgradeScrollType::MiddleClassScroll
					&& pUserScroll != UpgradeScrollType::HighClassScroll && pUserScroll != UpgradeScrollType::HighToRebirthScroll))
				goto fail_return;
			// High Class Scroll Match && High to Rebirth Upgrade Class Scroll Match
			else if (m_ScrollType == UpgradeScrollType::HighClassScroll &&
				(pUserScroll != UpgradeScrollType::HighClassScroll
					&& pUserScroll != UpgradeScrollType::HighToRebirthScroll))
				goto fail_return;
			// Rebirth Upgrade Class Scroll Match
			else if (m_ScrollType == UpgradeScrollType::RebirthClassScroll && pUserScroll != UpgradeScrollType::RebirthClassScroll) // rebirth + basma
				goto fail_return;
			else if (m_ScrollType == UpgradeScrollType::AccessoriesClassScroll && pUserScroll != UpgradeScrollType::AccessoriesClassScroll) // tak� basma
				goto fail_return;

			if (logositemac
				&& (pUserScroll == UpgradeScrollType::HighToRebirthScroll
					|| pUserScroll == UpgradeScrollType::AccessoriesClassScroll))
				goto fail_return;

			if (logositemac)
			{
				if ((proto.m_ItemType == 4 || proto.m_ItemType == 5) && proto.m_byGrade >= g_pMain->pServerSetting.maxBlessingUp)
					goto fail_return;

				if ((proto.m_ItemType == 11 || proto.m_ItemType == 12) && proto.m_byGrade >= g_pMain->pServerSetting.maxBlessingUpReb)
					goto fail_return;
			}

		/*	if (!(proto.ItemClass <= ItemClass)
				&& m_ScrollType == UpgradeScrollType::AccessoriesClassScroll)
				continue;*/

			if (pUserScroll == UpgradeScrollType::AccessoriesClassScroll) {  // eneskcbs tak� kontrolu 14.08.2020
				if ((snItemID[0] != snItemID[1] || snItemID[0] != snItemID[2] || snItemID[1] != snItemID[0]
					|| snItemID[1] != snItemID[2] || snItemID[2] != snItemID[0] || snItemID[2] != snItemID[1])
					|| (snItemID[0] <= 0 || snItemID[1] <= 0 || snItemID[2] <= 0 || snItemID[3] <= 0))
					//|| (!snItemID[0] > 0 || !snItemID[1] > 0 || !snItemID[2] > 0 || !snItemID[3] > 0))
					goto fail_return;
			}

			NewItemID = itr->second->NewItemNumber;
			bResult = UpgradeSucceeded;
			break;
		}

		// If we ran out of upgrades to search through, it failed.
		if (bResult != UpgradeSucceeded)
			goto fail_return;

		uint32 ReqItem1 = 0;
		uint32 ReqItem2 = 0;
		uint8 rtCount = 0;
		if (logositemac)
		{
			ReqItem1 = 379257000;
			ReqItem1 = 890092000;
			rtCount++;
		}
		else
		{
			foreach(itr, g_pMain->m_UpgradeSettings)
			{
				if (m_ScrollType == UpgradeScrollType::AccessoriesClassScroll) {
					if (itr->second->nReqItem1 != snItemID[4]
						&& itr->second->nReqItem2 != snItemID[4])
						continue;
					if (itr->second->nReqItem1 != snItemID[3]
						&& itr->second->nReqItem2 != snItemID[3])
						continue;
					if (itr->second->ItemGrade != proto.m_byGrade && itr->second->ItemGrade != 99)
						continue;
				}
				else {
					if (itr->second->nReqItem1 != snItemID[1]
						&& itr->second->nReqItem2 != snItemID[1])
						continue;
					if (itr->second->nReqItem1 != snItemID[2]
						&& itr->second->nReqItem2 != snItemID[2])
						continue;
					if (itr->second->ItemGrade != proto.m_byGrade && itr->second->ItemGrade != 99)
						continue;
				}
				if (proto.m_ItemType != itr->second->ItemType)
					continue;

				ReqItem1 = itr->second->nReqItem1;
				ReqItem2 = itr->second->nReqItem2;
				byGenRate = itr->second->SuccesRate;
				nReqCoins = itr->second->nReqCoins;
				rtCount++;
			}
		}

		if (!hasCoins(nReqCoins))
		{
			bResult = UpgradeNeedCoins;
			goto fail_return;
		}
		// Deduct gold early to prevent race condition
		GoldLose(nReqCoins, true);
		if (!IsExistInMap(nItems, ReqItem2) && ReqItem2 > 0) {
			bResult = UpgradeNoMatch;
			goto fail_return;
		}
		if (!IsExistInMap(nItems, ReqItem1) && ReqItem1 > 0) {
			bResult = UpgradeNoMatch;
			goto fail_return;
		}
		if (!CheckExistItem(ReqItem1, 1) && ReqItem1 > 0) {
			bResult = UpgradeNoMatch;
			goto fail_return;
		}
		if (!CheckExistItem(ReqItem2, 1) && ReqItem2 > 0) {
			bResult = UpgradeNoMatch;
			goto fail_return;
		}

		if (rtCount == 0) {
			bResult = UpgradeNoMatch;
			goto fail_return;
		}

		// Generate a random number, test if the item burned.
		int rand = myrand(0, 10000/* myrand(9000, 10000)*/), GenRate;
		if (trina || karivdis || isLowClassTrina || isMiddleClassTrina)
		{
			GenRate = byGenRate;
			if (GenRate > 10000)
				GenRate = 10000;
		}
		else
			GenRate = byGenRate;

		if (logositemac)
			GenRate = 33 * 100;

		if (bType == UpgradeTypeNormal && GenRate < rand)
		{
			if (logositemac)
			{
				uint32 rand_id = myrand(0, 6700);
				if (static_cast<int>(rand_id) < rand && proto.m_byGrade > 1)
				{
					uint32 newid = pOriginItem->nNum -= 1;
					_ITEM_TABLE newProto = g_pMain->GetItemPtr(newid);
					if (newProto.isnull()) {
						bResult = UpgradeNoMatch;
						goto fail_return;
					}

					pOriginItem->nNum = newid;
					OriginItem->second.nItemID = pOriginItem->nNum;
				}
			}
			else
			{
				memset(pOriginItem, 0, sizeof(_ITEM_DATA));
			}

			bResult = UpgradeFailed;
			ItemUpgradeNotice(proto, UpgradeFailed, scroll_id);
			
			std::string UP = "ItemUpgrade";
			std::string STA = "FAILED";
			
			if (isGM()) g_pMain->SendHelpDescription(this, string_format("Item Upgrade Failed : (+%d -> +%d) SuccesRate : %d ScrollID : %d , %d ",
				proto.m_byGrade, proto.m_byGrade + 1, byGenRate, snItemID[1], snItemID[2]));
		}
		else
		{
			// Generate the new item ID
			int nNewItemID = NewItemID;

			int nNewItemLevel = nNewItemID % 10;

			/*if((m_ScrollType != UpgradeScrollType::RebirthClassScroll && nNewItemLevel > 8)
			|| (m_ScrollType == UpgradeScrollType::RebirthClassScroll && nNewItemLevel > 5))
			goto fail_return; */

			// Does this new item exist?
			_ITEM_TABLE  newProto = g_pMain->GetItemPtr(nNewItemID);

			if (newProto.isnull())
			{ // if not, just say it doesn't match. No point removing the item anyway (like official :/).
				bResult = UpgradeNoMatch;
				goto fail_return;
			}

			if (bType != UpgradeTypePreview)
			{
				// Update the user's item in their inventory with the new item
				pOriginItem->nNum = nNewItemID;

				// Reset the durability also, to the new cap.
				pOriginItem->sDuration = newProto.m_sDuration;

				// Send upgrade notice.
				ItemUpgradeNotice(newProto, UpgradeSucceeded, scroll_id);
				std::string UP = "ItemUpgrade";
				// Gold already deducted early (line 1078)
					if (isGM())
					g_pMain->SendHelpDescription(this, string_format("Item Upgrade Succesfully : (+%d -> +%d) SuccesRate : %d ScrollID : %d , %d ",proto.m_byGrade, newProto.m_byGrade, byGenRate, snItemID[1], snItemID[2]));
			}

			// Replace the item ID in the list for the packet
			OriginItem->second.nItemID = nNewItemID;
		}

		uint32 logitemid[9]{ 0 }; uint16 logitemcount[9]{ 0 };
		// Remove the source item 
		if (bType != UpgradeTypePreview)
		{
			// Remove all required items, if applicable.
			for (int i = 1; i < MAX_ITEMS_REQ_FOR_UPGRADE; i++)
			{
				auto xitr = nItems.find(i);
				if (xitr == nItems.end())
					continue;

				_ITEM_DATA* pItem = GetItem(SLOT_MAX + xitr->second.bPos);
				if (pItem == nullptr
					|| pItem->nNum == 0
					|| pItem->sCount == 0)
					continue;

				logitemid[i] = xitr->second.nItemID;
				logitemcount[i] = 1;
				
				pItem->sCount--;
				if (pItem->sCount == 0 && pItem->nNum == xitr->second.nItemID) {
					pItem->nExpirationTime = 0;
					memset(pItem, 0, sizeof(_ITEM_DATA));
				}
			}
		}

		if (bType != UpgradeTypePreview)
			UpgradeInsertLog(proto.Getnum(), nReqCoins, (uint8)pUserScroll, bResult, logitemid, logitemcount);
	}
	result << bType << bResult;
	if (bResult == UpgradeFailed && nUpgradeType != 3)
		result << logositemac;
	
	foreach(i, nItems)
		result << i->second.nItemID << i->second.bPos;
	Send(&result);

	// Send the result to everyone in the area
	// (i.e. make the anvil do its burned/upgraded indicator thing)
	if (bType != UpgradeTypePreview) {
		result.Initialize(WIZ_OBJECT_EVENT);
		result << uint8(OBJECT_ANVIL) << bResult << sNpcID;
		SendToRegion(&result, nullptr, GetEventRoom());
		UserDataSaveToAgent(); // Upgrade sonrasi aninda kaydet
	}

	return;
fail_return:
	if (bResult == UpgradeFailed)
		result << logositemac;
	result << bType << bResult;
	foreach(i, nItems)
		result << i->second.nItemID << i->second.bPos;
	Send(&result);
}

void CUser::ItemUpgradeNotice(_ITEM_TABLE pItem, uint8 UpgradeResult, uint32 scroll_id)
{
	if (pItem.isnull() || isGM() || !g_pMain->pServerSetting.UpgradeNotice)
		return;

	if (!pItem.m_isUpgradeNotice)
		return;

	bool is_blue = (scroll_id == 379222000 || scroll_id == 379224000 || scroll_id == 379223000 || scroll_id == 379225000 ||
		scroll_id == 379226000 || scroll_id == 379206000 || scroll_id == 379209000 || scroll_id == 379208000 ||
		scroll_id == 379210000 || scroll_id == 379211000 || scroll_id == 379017000 || scroll_id == 379027000 ||
		scroll_id == 379026000 || scroll_id == 379028000 || scroll_id == 379029000 || scroll_id == 379022000 ||
		scroll_id == 379031000 || scroll_id == 379030000 || scroll_id == 379032000 || scroll_id == 379033000);

	bool is_orange = (scroll_id == 379020000 || scroll_id == 379229000 || scroll_id == 379214000 || scroll_id == 379025000);
	bool is_upgrade = (scroll_id == 379205000 || scroll_id == 379016000 || scroll_id == 379221000 || scroll_id == 379021000);

	bool i_staff_trans = (
		scroll_id == 379230000 || scroll_id == 379231000 || scroll_id == 379232000 || scroll_id == 379233000 ||
		scroll_id == 379234000 || scroll_id == 379235000 || scroll_id == 379215000 || scroll_id == 379216000 ||
		scroll_id == 379217000 || scroll_id == 379218000 || scroll_id == 379219000 || scroll_id == 379220000 ||
		scroll_id == 379034000 || scroll_id == 379035000 || scroll_id == 379138000 || scroll_id == 379139000 ||
		scroll_id == 379140000 || scroll_id == 379141000);

	if (is_upgrade || UpgradeResult == 0 || pItem.m_ItemType == 11 || pItem.m_ItemType == 12 || scroll_id == 379159000)
	{
		Packet result(WIZ_LOGOSSHOUT, uint8(0x02));
		result.SByte();
		result << uint8(0x05) << uint8(UpgradeResult) << GetName() << pItem.m_iNum << GetLoyaltySymbolRank();
		g_pMain->Send_All(&result);
		return;
	}

	std::string message = string_format("%s has %s to upgrade %s", GetName().c_str(), (UpgradeResult == 1 ? "succedeed" : "Failed"), pItem.m_sName.c_str());
	uint8 chatType = 0;
	if (is_blue)
		chatType = (uint8)ChatType::ALLIANCE_NOTICE;
	else if (is_orange)
		chatType = (uint8)ChatType::SHOUT_CHAT;
	else if (i_staff_trans)
		chatType = (uint8)ChatType::MERCHANT_CHAT;
	else
		chatType = (uint8)ChatType::KNIGHTS_CHAT;

	if (!chatType)
		return;

	std::string test = "";
	Packet newpkt;
	ChatPacket::Construct(&newpkt, chatType, &message, &test, GetNation());
	g_pMain->SendGameNotice((ChatType)chatType, message, "", true, nullptr, false);

	/*
	PARTY_CHAT saklement
	SHOUT_CHAT turuncu
	KNIGHTS_CHAT mavi
	PUBLIC_CHAT sar�
	GENERAL_CHAT beyaz
	MERCHANT_CHAT eflatun a��k pembemsi
	ALLIANCE_NOTICE mavi 
	*/
	return;
	
}
/**
* @brief	Packet handler for the accessory upgrade system.
*
* @param	pkt	The packet.
*/
void CUser::ItemUpgradeAccessories(Packet & pkt)
{
	ItemUpgrade(pkt, ITEM_ACCESSORIES);
}

/**
* @brief	Packet handler for the upgrading of 'rebirthed' items.
*
* @param	pkt	The packet.
*/
void CUser::ItemUpgradeRebirth(Packet & pkt)
{
	ItemUpgrade(pkt, ITEM_UPGRADE_REBIRTH);
}