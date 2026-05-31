#include "stdafx.h"
#include "../shared/KOSocketMgr.h"
using namespace std;

/*
1. The invitation to the party has been declined.
2. You cannot form a party because of the Level difference.
3. You cannot form a party with a user from the other nation.
4. A clan party cannot invite users from a different clan.
5. You cannot form a party with a user in a different zone.
6. You cannot form a party with a user that's not online.
7. You cannot form a party with this user.
8. Not a same clan
9. You cannot form a party in this region.
10. You cannot invite this player to a party. Invitee does not have required item
*/

#pragma region void CUser::PartySystemProcess(Packet & pkt)
void CUser::PartySystemProcess(Packet& pkt)
{
	uint8 opcode;
	pkt >> opcode;
	switch (opcode)
	{
	case PARTY_CREATE:
		PartyCreateRequest(pkt);
		break;
	case PARTY_INSERT:
		PartyInvitationRequest(pkt);
		break;
	case PARTY_PERMIT:
		PartyInsertOrCancel(pkt);
		break;
	case PARTY_PROMOTE:
		PartyLeaderPromote(pkt.read<uint16>());
		break;
	case PARTY_REMOVE:
		PartyNemberRemove(pkt.read<uint16>());
		break;
	case PARTY_DELETE:
		PartyisDelete();
		break;
	case PARTY_TARGET_NUMBER:
		PartyTargetNumber(pkt);
		break;
	case PARTY_ALERT:
		PartyAlert(pkt);
		break;
	case PARTY_COMMAND_PROMATE:
		PartyCommand(pkt);
		break;
	default:
		printf("Party unhandled packets %x \n", opcode);
		TRACE("Party unhandled packets %x \n", opcode);
		break;
	}
}
#pragma endregion

#pragma region void CUser::PartyCreateRequest(Packet & pkt)
void CUser::PartyCreateRequest(Packet& pkt)
{
	std::string strUserID = ""; int8 PartyType;
	pkt >> strUserID >> PartyType;

	/*01 0D00 48756D616E4D6167656369616E FF*/
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
		return;

	PartyCreateCheck(strUserID, PartyType);
}
#pragma endregion

#pragma region void CUser::PartyCreateCheck(uint16 UserGetSocketID, uint8 PartyType
void CUser::PartyCreateCheck(std::string strUserID, int8 PartyType)
{
	Packet party(WIZ_PARTY);
	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);

	if (pUser == nullptr || pUser == this || pUser->isInParty()
		|| pUser->isInApprovedParty()) {
		party << uint8(PARTY_INSERT) << int16(-7);
		Send(&party);
		return;
	}

	if (GetNation() != pUser->GetNation() && !isPartnerPartyZone()) {
		party << uint8(PARTY_INSERT) << int16(-3);
		Send(&party);
		return;
	}

	if (GetZoneID() != pUser->GetZoneID()) {
		party << uint8(PARTY_INSERT) << int16(-5);
		Send(&party);
		return;
	}

	if (GetZoneID() == ZONE_CHAOS_DUNGEON
		|| GetZoneID() == ZONE_PRISON
		|| GetZoneID() == ZONE_DUNGEON_DEFENCE) {
		party << uint8(PARTY_INSERT) << int16(-9);
		Send(&party);
		return;
	}

	if (!pUser->isInGame()) {
		party << uint8(PARTY_INSERT) << int16(-6);
		Send(&party);
		return;
	}

	if (PartyType == 2) {
		if (!pUser->CheckExistItem(914057000, 1)) {
			party << uint8(PARTY_INSERT) << int16(-10);
			Send(&party);
			return;
		}

		bool seekchecking = false;
		if (g_pMain->m_SeekingPartyArray.size() != 0) {
			g_pMain->m_SeekingPartyArrayLock.lock();
			foreach(itr, g_pMain->m_SeekingPartyArray) {
				if ((*itr) == nullptr || (*itr)->m_sSid != GetID())
					continue;

				if ((*itr)->m_bloginType != 2) {
					party << uint8(PARTY_INSERT) << int16(-7);
					Send(&party);
					g_pMain->m_SeekingPartyArrayLock.unlock();
					return;
				}
				seekchecking = true;
			}
			g_pMain->m_SeekingPartyArrayLock.unlock();
		}
		if (!seekchecking) {
			party << uint8(PARTY_INSERT) << int16(-7);
			Send(&party);
			return;
		}
	}

	if (g_pMain->m_byBattleSiegeWarOpen && GetZoneID() == ZONE_DELOS)
	{
		CKnights * pKnights = g_pMain->GetClanPtr(pUser->GetClanID());
		if (pKnights != nullptr)
		{ 
			if (!pKnights->isInAlliance() && GetClanID() != pUser->GetClanID())
			{
				party << uint8(PARTY_INSERT) << int16(-3);
				Send(&party);
				return;
			}
			
			_KNIGHTS_ALLIANCE * pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());
			if (pAlliance != nullptr)
			{
				if (pKnights->isInAlliance() && pKnights->GetAllianceID() != pAlliance->sMainAllianceKnights)
				{
					party << uint8(PARTY_INSERT) << int16(-3);
					Send(&party);
					return;
				}
			}
		}
	}

	if (GetZoneID() != ZONE_BORDER_DEFENSE_WAR && GetZoneID() != ZONE_JURAID_MOUNTAIN) {
		if (!m_bIsChicken && !pUser->m_bIsChicken && (!isInClan() || GetClanID() != pUser->GetClanID())) {
			int myLevel = (int)GetLevel();
			int theirLevel = (int)pUser->GetLevel();
			if (!((theirLevel <= (int)(myLevel * 1.5f) && theirLevel >= (int)(myLevel * 0.667f))
				|| (theirLevel <= (myLevel + 8) && theirLevel >= (myLevel - 8)))) {
				party << uint8(PARTY_INSERT) << int16(-2);
				Send(&party);
				return;
			}
		}
	}

	if (isInParty() || isInApprovedParty()) {
		party << uint8(PARTY_INSERT) << int16(-7);
		Send(&party);
		return;
	}

	auto* pParty = g_pMain->CreateParty(this);
	if (pParty == nullptr) {
		party << uint8(PARTY_INSERT) << int16(-7);
		Send(&party);
		return;
	}

	m_sUserPartyType = PartyType;
	pUser->m_sUserPartyType = PartyType;
	m_bPartyLeader = true;
	m_bPartyCommandLeader = true;
	StateChangeServerDirect(6, 1); // give party leader the 'P' symbol
	pUser->m_sPartyIndex = m_sPartyIndex;
	pUser->m_bInParty = true;
	party << uint8(PARTY_PERMIT) << GetSocketID() << GetName();
	pUser->Send(&party);
}
#pragma endregion

#pragma region void CUser::PartyInvitationRequest(Packet & pkt)
void CUser::PartyInvitationRequest(Packet& pkt)
{
	std::string strUserID = "";
	int8 PartyType = 0;
	pkt >> strUserID >> PartyType;

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) 
		return;

	auto* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !pUser->isInGame())
		return;
	
	PartyInvitationCheck(pUser->GetSocketID(), PartyType);
}
#pragma endregion

#pragma region void CUser::PartyInvitationCheck(uint16 UserGetSocketID, uint8 PartyType)
void CUser::PartyInvitationCheck(uint16 UserGetSocketID, uint8 PartyType)
{
	Packet party(WIZ_PARTY);
	CUser* pUser = g_pMain->GetUserPtr(UserGetSocketID);
	_PARTY_GROUP* pParty = nullptr;
	int16 PartyMemberCount = 0;

	if (pUser == nullptr || pUser == this || pUser->isInParty() || pUser->isInApprovedParty()) {
		party << uint8(PARTY_INSERT) << int16(-7);
		Send(&party);
		return;
	}

	if (GetNation() != pUser->GetNation()
		&& !isPartnerPartyZone())
	{
		party << uint8(PARTY_INSERT) << int16(-3);
		Send(&party);
		return;
	}

	if (GetZoneID() != pUser->GetZoneID())
	{
		party << uint8(PARTY_INSERT) << int16(-5);
		Send(&party);
		return;
	}

	if (GetZoneID() == ZONE_CHAOS_DUNGEON
		|| GetZoneID() == ZONE_PRISON
		|| GetZoneID() == ZONE_DUNGEON_DEFENCE)
	{
		party << uint8(PARTY_INSERT) << int16(-9);
		Send(&party);
		return;
	}

	if (!pUser->isInGame()) {
		party << uint8(PARTY_INSERT) << int16(-6);
		Send(&party);
		return;
	}

	if (PartyType == 2 || m_sUserPartyType == 2) {
		if (!pUser->CheckExistItem(914057000, 1)) {
			party << uint8(PARTY_INSERT) << int16(-10);
			Send(&party);
			return;
		}

		g_pMain->m_SeekingPartyArrayLock.lock();
		foreach(itr, g_pMain->m_SeekingPartyArray) {
			if ((*itr) == nullptr) continue;
			if ((*itr)->m_sSid == GetID()
				&& (*itr)->m_bloginType != 2) {
				party << uint8(PARTY_INSERT) << int16(-7);
				Send(&party);
				g_pMain->m_SeekingPartyArrayLock.unlock();
				return;
			}
		}
		g_pMain->m_SeekingPartyArrayLock.unlock();
	}

	if (GetZoneID() != ZONE_BORDER_DEFENSE_WAR && GetZoneID() != ZONE_JURAID_MOUNTAIN) {
		if (!m_bIsChicken && !pUser->m_bIsChicken && (!isInClan() || GetClanID() != pUser->GetClanID())) {
			int myLevel = (int)GetLevel();
			int theirLevel = (int)pUser->GetLevel();
			if (!((theirLevel <= (int)(myLevel * 1.5f) && theirLevel >= (int)(myLevel * 0.667f))
				|| (theirLevel <= (myLevel + 8) && theirLevel >= (myLevel - 8)))) {
				party << uint8(PARTY_INSERT) << int16(-2);
				Send(&party);
				return;
			}
		}
	}

	pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr) {
		party << uint8(PARTY_INSERT) << int16(-1);
		Send(&party);
		return;
	}

	for (PartyMemberCount = 0; PartyMemberCount < MAX_PARTY_USERS; PartyMemberCount++) {
		// RECONNECT PARK - graceUid kontrolu kaldirildi
		if (pParty->uid[PartyMemberCount] < 0) // && pParty->graceUid[PartyMemberCount] < 0)
			break;
	}

	if (PartyMemberCount == MAX_PARTY_USERS)
	{
		party << uint8(PARTY_INSERT) << int16(-1);
		Send(&party);
		return;
	}

	pUser->m_sPartyIndex = m_sPartyIndex;
	pUser->m_bInParty = true;

	if (m_sUserPartyType == 2)
		pUser->m_sUserPartyType = m_sUserPartyType;
	else
		pUser->m_sUserPartyType = PartyType;

	party << uint8(PARTY_PERMIT) << GetSocketID() << GetName();
	pUser->Send(&party);
}
#pragma endregion

#pragma region void CUser::PartyInsertOrCancel(Packet & pkt)
void CUser::PartyInsertOrCancel(Packet& pkt)
{
	uint8 OkorCancel;
	pkt >> OkorCancel;
	switch (OkorCancel)
	{
	case 0:
		DoNotAcceptJoiningTheParty();
		break;
	case 1:
		AgreeToJoinTheParty();
		break;
	default:
		printf("Party System Ok or Cansel unhandled opcode %d \n", OkorCancel);
		TRACE("Party System Ok or Cansel unhandled opcode %d \n", OkorCancel);
		break;
	}
}
#pragma endregion

#pragma region void CUser::AgreeToJoinTheParty()
void CUser::AgreeToJoinTheParty()
{
	Packet result(WIZ_PARTY);
	_PARTY_GROUP* pParty = nullptr;
	uint8 byIndex = 0xFF;
	int leader_id = -1;

	if (!isInApprovedParty())
		return;

	pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr)
	{
		m_bInParty = false;
		m_bInEnterParty = false;
		m_sUserPartyType = 0;
		m_sPartyIndex = -1;
		return;
	}

	CUser* pLeader = g_pMain->GetUserPtr(pParty->uid[0]);
	if (pLeader == nullptr || !pLeader->isInGame()) {
		m_bInParty = false;
		m_bInEnterParty = false;
		m_sUserPartyType = 0;
		m_sPartyIndex = -1;
		return;
	}

	if (pLeader->GetZoneID() != GetZoneID()
		|| GetPartyMemberAmount(pParty) == MAX_PARTY_USERS) {
		DoNotAcceptJoiningTheParty();
		return;
	}

	// make sure user isn't already in the array...
	// kind of slow, but it works for the moment
	foreach_array(i, pParty->uid)
	{
		if (pParty->uid[i] == GetSocketID())
		{
			m_bInParty = false;
			m_bInEnterParty = false;
			m_sUserPartyType = 0;
			m_sPartyIndex = -1;
			pParty->uid[i] = -1;
			return;
		}
	}

	int PartyNemberCount = 0;
	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		if (pParty->uid[i] >= 0)
			PartyNemberCount++;
	}

	if (PartyNemberCount == 1)
		pLeader->m_bInEnterParty = true;

	// Send the player who just joined the existing party list
	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		// No player set?
		if (pParty->uid[i] < 0)
		{
			// If we're not in the list yet, add us.
			// RECONNECT PARK - graceUid kontrolu kaldirildi
			if (byIndex == 0xFF) { // && pParty->graceUid[i] < 0) {
				pParty->uid[i] = GetSocketID();
				byIndex = i;
			}
			continue;
		}

		// For everyone else, 
		CUser* pUser = g_pMain->GetUserPtr(pParty->uid[i]);
		if (pUser == nullptr || !pUser->isInGame()) continue;

		result.clear();
		result << uint8(PARTY_INSERT) << pParty->uid[i]
			<< uint8(1) << pUser->GetName() << pUser->m_MaxHp
			<< pUser->m_sHp << pUser->GetLevel() << pUser->m_sClass
			<< pUser->m_MaxMp << pUser->m_sMp
			<< pUser->GetNation() << uint8(0)
			<< pParty->NumberTargetID << pUser->m_sUserPartyType << pUser->GetLoyaltySymbolRank();
		Send(&result);
	}

	if (pLeader->m_bNeedParty == 2 || m_bNeedParty == 2) {
		if (pLeader->m_bNeedParty == 2) {
			g_pMain->m_SeekingPartyArrayLock.lock();
			// You don't need anymore seek
			foreach(itr, g_pMain->m_SeekingPartyArray) {
				if ((*itr) == nullptr) continue;
				if ((*itr)->m_sSid == pLeader->GetID()) {
					g_pMain->m_SeekingPartyArray.erase(itr);
					break;
				}
			}
			g_pMain->m_SeekingPartyArrayLock.unlock();
		}
		if (m_bNeedParty == 2) {
			g_pMain->m_SeekingPartyArrayLock.lock();
			// You don't need anymore seek
			foreach(itr, g_pMain->m_SeekingPartyArray) {
				if ((*itr) == nullptr) continue;
				if ((*itr)->m_sSid == GetID()) {
					g_pMain->m_SeekingPartyArray.erase(itr);
					break;
				}
			}
			g_pMain->m_SeekingPartyArrayLock.unlock();
		}
	}

	m_bInEnterParty = true;
	pLeader->m_bInEnterParty = true;
	m_sPartyIndex = pLeader->m_sPartyIndex;

	if (pLeader->m_bNeedParty == 2 && pLeader->isInParty())
		pLeader->StateChangeServerDirect(2, 1);

	if (m_bNeedParty == 2 && isInParty())
		StateChangeServerDirect(2, 1);

	result.clear();
	result << uint8(PARTY_INSERT) << GetSocketID()
		<< uint8(1) << GetName()
		<< m_MaxHp << m_sHp
		<< GetLevel() << GetClass()
		<< m_MaxMp << m_sMp
		<< GetNation() << uint8(0)
		<< pParty->NumberTargetID << m_sUserPartyType
		<< GetLoyaltySymbolRank();
	g_pMain->Send_PartyMember(GetPartyID(), &result);

	CUser* pUserr = nullptr;  //17.12.2020 Partyde HP Degeri Yazi Olarak Gosterme
	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		pUserr = g_pMain->GetUserPtr(pParty->uid[i]);
		if (pUserr == nullptr)
			continue;

		pUserr->SendPartyHpManager(PartyType::Send_All); 
	}
}
#pragma endregion

#pragma region void CUser::DoNotAcceptJoiningTheParty()
void CUser::DoNotAcceptJoiningTheParty()
{
	int LeaderGetID = -1, PartyNemberCount = 0;
	if (!isInApprovedParty())
		return;

	m_sUserPartyType = 0;
	m_bInParty = false;
	m_bInEnterParty = false;

	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr) {
		m_sPartyIndex = -1;
		return;
	}

	LeaderGetID = pParty->uid[0];
	CUser* pLeader = g_pMain->GetUserPtr(pParty->uid[0]);
	if (pLeader == nullptr) return;

	for (int i = 0; i < MAX_PARTY_USERS; i++) {
		if (pParty->uid[i] >= 0)
			PartyNemberCount++;
	}

	// Grace slot'lar varsa disband yapma
	if (PartyNemberCount == 1 && pParty->GetTotalMemberCount() <= 1)
		pLeader->PartyisDelete();

	Packet result(WIZ_PARTY, uint8(PARTY_INSERT));
	result << int16(-1);
	pLeader->Send(&result);
}
#pragma endregion

#pragma region void CUser::PartyisDelete()
void CUser::PartyisDelete()
{
	if (!isInApprovedParty())
		return;

	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());

	if (pParty == nullptr) {
		m_sUserPartyType = 0;
		m_bInEnterParty = false;
		m_bInParty = false;
		m_sPartyIndex = -1;
		m_bPartyCommandLeader = false;
		return;
	}

	for (int i = 0; i < MAX_PARTY_USERS; i++) {
		CUser* pUser = g_pMain->GetUserPtr(pParty->uid[i]);
		if (pUser != nullptr) {
			pUser->m_sUserPartyType = 0;
			pUser->m_bInEnterParty = false;
			pUser->m_bInParty = false;
			pUser->m_sPartyIndex = -1;
			pUser->m_bPartyCommandLeader = false;
		}
		/* RECONNECT PARK - grace slot temizligi kaldirildi
		pParty->graceUid[i] = -1;
		pParty->graceExpiry[i] = 0;
		pParty->graceName[i] = ""; */
	}

	if (m_bNeedParty == 3) {
		// You don't need anymore seek
		g_pMain->m_SeekingPartyArrayLock.lock();
		foreach(itr, g_pMain->m_SeekingPartyArray) {
			if ((*itr) == nullptr) continue;
			if ((*itr)->m_sSid == GetID()) {
				g_pMain->m_SeekingPartyArray.erase(itr);
				break;
			}
		}
		g_pMain->m_SeekingPartyArrayLock.unlock();
		StateChangeServerDirect(2, 1);
	}

	Packet result(WIZ_PARTY, uint8(PARTY_DELETE));
	g_pMain->Send_PartyMember(pParty->wIndex, &result);

	m_bPartyLeader = false;
	StateChangeServerDirect(6, 0); // remove 'P' symbol from party leader
	g_pMain->DeleteParty(pParty->wIndex);
}
#pragma endregion

#pragma region void CUser::PartyLeaderPromote(uint16 GetLeaderID)
void CUser::PartyLeaderPromote(uint16 GetLeaderID, _PARTY_GROUP* pmyparty)
{
	if (!isPartyLeader())
		return;

	_PARTY_GROUP* pParty = nullptr;
	if (!pmyparty) 
		pParty = g_pMain->GetPartyPtr(GetPartyID());
	else 
		pParty = pmyparty;

	if (pParty == nullptr) 
		return;

	uint8 pos = 0;
	for (uint8 i = 1; i < MAX_PARTY_USERS; i++) {
		if (pParty->uid[i] != GetLeaderID)
			continue;
		pos = i;
		break;
	}

	// Didn't find it? (leader's always position 0, no need to check there)
	if (pos == 0 || pos >= MAX_PARTY_USERS)
		return;

	// Ensure this user exists and that they're in our party already.
	CUser* pUser = g_pMain->GetUserPtr(pParty->uid[pos]);
	if (pUser == nullptr
		|| pUser->GetPartyID() != GetPartyID())
		return;

	// Swap the IDs around, so they have the leadership position.
	std::lock_guard<std::recursive_mutex> partyLock(pParty->m_lock);
	std::swap(pParty->uid[0], pParty->uid[pos]);

	// Swap the seeking party & leader flags
	std::swap(m_bNeedParty, pUser->m_bNeedParty);
	std::swap(m_bPartyLeader, pUser->m_bPartyLeader);

	// Remove our leadership state from the client
	StateChangeServerDirect(6, 0); // remove 'P' symbol from old party leader	
	StateChangeServerDirect(2, m_bNeedParty); // seeking a party

	// Make them leader.
	pUser->StateChangeServerDirect(6, 1); // assign 'P' symbol to new party leader
	pUser->StateChangeServerDirect(2, pUser->m_bNeedParty); // seeking a party

	Packet result(WIZ_PARTY, uint8(PARTY_INSERT));
	result << pUser->GetSocketID()
		<< uint8(100) // reset position to leader
		<< pUser->GetName() << pUser->m_MaxHp << pUser->m_sHp
		<< pUser->GetLevel() << pUser->GetClass()
		<< pUser->m_MaxMp << pUser->m_sMp
		<< pUser->GetNation() << uint8(0)
		<< pParty->NumberTargetID
		<< pUser->m_sUserPartyType << pUser->m_bPersonalRank;
	g_pMain->Send_PartyMember(GetPartyID(), &result);

	for (int i = 0; i < MAX_PARTY_USERS; i++) //17.12.2020 Partyde HP Degeri Yazi Olarak Gosterme
	{
		pUser = g_pMain->GetUserPtr(pParty->uid[i]);
		if (pUser == nullptr)
			continue;

		pUser->SendPartyHpManager(PartyType::Send_All);
	}
}
#pragma endregion

#pragma region void CUser::PartyNemberRemove(uint16 UserID)
void CUser::PartyNemberRemove(uint16 UserID, _PARTY_GROUP* pmyparty)
{
	if (!isInApprovedParty())
		return;

	CUser* pUser = g_pMain->GetUserPtr(UserID);

	_PARTY_GROUP* pParty = nullptr;
	if (!pmyparty) pParty = g_pMain->GetPartyPtr(GetPartyID());
	else pParty = pmyparty;

	if (pParty == nullptr) {
		m_bInParty = false;
		m_bInEnterParty = false;
		m_sPartyIndex = -1;
		return;
	}

	std::lock_guard<std::recursive_mutex> partyLock(pParty->m_lock);

	// B10 fix: pUser NULL ise socketID olarak UserID kullan
	uint16 targetSocketID = (pUser != nullptr) ? pUser->GetSocketID() : UserID;

	if (targetSocketID != GetSocketID()) {
		if (pParty->uid[0] != GetSocketID())
			return;
	}
	else {
		if (pParty->uid[0] == targetSocketID) {
			PartyisDelete();
			return;
		}
	}

	int count = 0, memberPos = -1;
	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		if (pParty->uid[i] < 0)
			continue;

		else if (pParty->uid[i] == targetSocketID)
		{
			memberPos = i;
			continue;
		}
		count++;
	}

	// Disband: tek uye kaldi → party dagil
	if (count == 1)
	{
		// Tum uyelerin flag'lerini temizle
		for (int i = 0; i < MAX_PARTY_USERS; i++) {
			CUser* pMember = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pMember != nullptr) {
				pMember->m_sUserPartyType = 0;
				pMember->m_bInEnterParty = false;
				pMember->m_bInParty = false;
				pMember->m_sPartyIndex = -1;
				pMember->m_bPartyCommandLeader = false;
				pMember->m_bPartyLeader = false;
			}
		}

		// Liderin 'P' sembolunu kaldir
		CUser* pLeader = g_pMain->GetUserPtr(pParty->uid[0]);
		if (pLeader != nullptr)
			pLeader->StateChangeServerDirect(6, 0);

		Packet removeResult(WIZ_PARTY, uint8(PARTY_REMOVE));
		removeResult << targetSocketID;
		Packet delResult(WIZ_PARTY, uint8(PARTY_DELETE));

		// FIX (son uye disband bug): cikan kisiye de PARTY_DELETE gonder.
		// Eskiden asagidaki dongu cikani (targetSocketID) atliyordu -> cikanin client'i
		// "party'deyim" sanip takiliyordu (party penceresi/HP bar/buton desync).
		// Ortadaki cikista Send_PartyMember cikana da yolladigi icin sorun yoktu.
		int sentLeaver = 0;
		CUser* pLeaver = g_pMain->GetUserPtr(targetSocketID);
		if (pLeaver != nullptr && pLeaver->isInGame()) {
			pLeaver->Send(&delResult);
			sentLeaver = 1;
		}

		for (int i = 0; i < MAX_PARTY_USERS; i++) {
			if (pParty->uid[i] < 0 || pParty->uid[i] == targetSocketID)
				continue;
			CUser* pMember = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pMember == nullptr || !pMember->isInGame())
				continue;
			pMember->Send(&removeResult);
			pMember->Send(&delResult);
		}

		LOG(LogCategory::LOG_GENERAL,
			"[PARTY DISBAND] leaver=%d remaining_leader=%d sentToLeaver=%d partyIdx=%d",
			(int)targetSocketID, (int)pParty->uid[0], sentLeaver, (int)pParty->wIndex);

		g_pMain->DeleteParty(pParty->wIndex);
		return;
	}

	Packet result(WIZ_PARTY, uint8(PARTY_REMOVE));
	result << targetSocketID;
	g_pMain->Send_PartyMember(m_sPartyIndex, &result);

	if (memberPos >= 0) {
		pParty->uid[memberPos] = -1;
		if (pUser != nullptr) {
			pUser->m_sUserPartyType = 0;
			pUser->m_bInParty = false;
			pUser->m_bInEnterParty = false;
			pUser->m_sPartyIndex = -1;
		}
	}

	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		CUser* pMember = g_pMain->GetUserPtr(pParty->uid[i]);
		if (pMember == nullptr)
			continue;

		pMember->SendPartyHpManager(PartyType::Send_All);
	}
}
#pragma endregion

#pragma region void CUser::PartyAlert(Packet & pkt)
void CUser::PartyAlert(Packet& pkt)
{
	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr)
		return;

	uint8 SubOpCode = 0; uint32 EffectID = 0;
	pkt >> SubOpCode >> EffectID;

	if (m_lasttargetnumbertime > UNIXTIME2) return;
	m_lasttargetnumbertime = UNIXTIME2 + 850;

	if (!isInGame() || isDead() || !isInParty() || !isPartyCommandLeader()) return;

	Packet result(WIZ_PARTY, uint8(PARTY_ALERT));
	result << SubOpCode;
	g_pMain->Send_PartyMember(GetPartyID(), &result);
}
#pragma endregion

#pragma region void CUser::PartyCommand(Packet & pkt)
void CUser::PartyCommand(Packet& pkt)
{
	int16 NenberID = -1; pkt >> NenberID;

	if (m_lasttargetnumbertime > UNIXTIME2) return;
	m_lasttargetnumbertime = UNIXTIME2 + 850;

	if (!isInGame() || !isInParty() || !isPartyCommandLeader()) return;
	CUser* pUser = g_pMain->GetUserPtr(NenberID);
	if (pUser == nullptr || pUser->GetPartyID() != GetPartyID()) return;

	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr) return;

	std::lock_guard<std::recursive_mutex> partyLock(pParty->m_lock);
	std::swap(m_bPartyCommandLeader, pUser->m_bPartyCommandLeader);
	Packet result(WIZ_PARTY, uint8(PARTY_COMMAND_PROMATE));
	result.SByte();
	result << pUser->GetSocketID() << pUser->GetName();
	g_pMain->Send_PartyMember(GetPartyID(), &result);
}
#pragma endregion

#pragma region void CUser::PartyTargetNumber(Packet & pkt)
void CUser::PartyTargetNumber(Packet& pkt)
{
	int8 Succes = -1; int16 TargetID = -1; uint32 EffectID = 0;

	if (m_lasttargetnumbertime > UNIXTIME2) return;
	m_lasttargetnumbertime = UNIXTIME2 + 850;

	pkt >> TargetID >> EffectID >> Succes;
	if (!isInGame() || isDead() || !isInParty() || !isPartyCommandLeader()) return;

	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> partyLock(pParty->m_lock);
	pParty->NumberTargetID = TargetID;
	Packet result(WIZ_PARTY, uint8(PARTY_TARGET_NUMBER));
	result << TargetID << Succes;
	g_pMain->Send_PartyMember(GetPartyID(), &result);
}
#pragma endregion

#pragma region CGameServerDlg::DeleteParty(uint16 sIndex)
void CGameServerDlg::DeleteParty(uint16 sIndex)
{
	m_PartyArray.DeleteData(sIndex);
}
#pragma endregion

void CUser::PartyBBS(Packet& pkt)
{
	uint8 Type; uint8 opcode;
	pkt >> Type >> opcode;

	if (Type != 0 || !isInGame() || isSellingMerchantingPreparing())
		return;

	switch (opcode)
	{
	case PARTY_BBS_REGISTER:
		PartyBBSRegister(pkt);
		break;
	case PARTY_BBS_DELETE:
		PartyBBSDelete(pkt);
		break;
	case PARTY_BBS_NEEDED:
		PartyBBSNeeded(pkt);
		break;
	case PARTY_BBS_WANTED:
		PartyBBSWanted(pkt);
		break;
	case PARTY_BBS_PARTY_CHANGE:
		PartyBBSPartyChange(pkt);
		break;
	case PARTY_BBS_PARTY_DELETE:
		PartyBBSPartyDelete(pkt);
		break;
	case PARTY_BBS_LIST:
		SendPartyBBSNeeded(pkt);
		break;
	default:
		printf("Party Seeking System Type 0 unhandled Opcode %d \n", opcode);
		break;
	}
}

void CUser::PartyBBSPartyDelete(Packet& pkt)
{
	if (m_bNeedParty == 1
		|| !isPartyLeader())
		return;

	// You don't need anymore 
	g_pMain->m_SeekingPartyArrayLock.lock();
	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		if ((*itr)->m_sSid == GetID())
		{
			g_pMain->m_SeekingPartyArray.erase(itr);
			break;
		}
	}
	g_pMain->m_SeekingPartyArrayLock.unlock();

	StateChangeServerDirect(2, 1); // not looking for a party
	SendPartyBBSNeeded(pkt);
}

void CUser::PartyBBSPartyChange(Packet& pkt)
{
	uint16 page_index = 0;
	if (!isPartyLeader())
		return;

	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr)
		return;

	Packet result(WIZ_PARTY_BBS, uint8(PARTY_TYPE_SEEKING));
	pkt.DByte();
	pkt >> pParty->sWantedClass >> page_index >> pParty->WantedMessage;
	result << uint8(PARTY_BBS_WANTED) << uint8(1);

	_SEEKING_PARTY_USER* pPartyUser = nullptr;
	uint16 seeking_index = 0; // holds the index where we located at in the vector
	g_pMain->m_SeekingPartyArrayLock.lock();

	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		seeking_index++;
		if ((*itr)->m_sSid == GetID())
		{
			pPartyUser = (*itr);
			pPartyUser->m_strSeekingNote = pParty->WantedMessage;
			pPartyUser->m_sClass = pParty->sWantedClass;
			break;
		}
	}

	SendPartyBBSNeeded(pkt);

	uint16 counter = 0;
	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		counter++;
		if (counter < seeking_index)
			continue;

		result << (*itr)->m_strUserID << uint8((*itr)->m_sLevel) << (*itr)->m_sClass;
		if (counter >= 12)
			break;
	}

	g_pMain->m_SeekingPartyArrayLock.unlock();

	uint16 s = counter - seeking_index;
	// You still need to fill up ten slots.
	if (s < MAX_BBS_PAGE)
	{
		for (int j = s; j < MAX_BBS_PAGE; j++)
			result << uint16(0) << uint16(0)
			<< uint16(0) << uint8(0)
			<< uint8(0) << uint8(0)
			<< uint16(0)
			<< uint8(0);
	}
	result << uint16(0) << uint8(0) << uint16(s);
	Send(&result);
}

void CUser::PartyBBSRegister(Packet& pkt)
{
	_SEEKING_PARTY_USER* pUser = nullptr;
	string seeking_msg = "";
	Packet result(WIZ_PARTY_BBS, uint8(PARTY_TYPE_SEEKING));
	pkt.SByte();
	pkt >> seeking_msg;

	result << uint8(PARTY_BBS_REGISTER) << uint8(1);
	StateChangeServerDirect(2, 2); // seeking a party

	uint16 seeking_index = 0; // holds the index where we located at in the vector
	g_pMain->m_SeekingPartyArrayLock.lock();

	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		seeking_index++;
		if ((*itr)->m_sSid == GetID())
		{
			pUser = (*itr);
			pUser->m_strSeekingNote = seeking_msg;
			break;
		}
	}

	if (pUser == nullptr)
	{
		pUser = new _SEEKING_PARTY_USER;
		pUser->m_bNation = GetNation();
		pUser->m_strUserID = GetName();
		pUser->m_sClass = GetClass();
		pUser->m_bSeekType = 0;
		pUser->m_bZone = GetZoneID();
		pUser->m_strSeekingNote = seeking_msg;
		pUser->m_sLevel = GetLevel();
		pUser->m_sSid = GetID();
		pUser->m_sPartyID = GetPartyID();
		pUser->isPartyLeader = isPartyLeader();
		pUser->m_bloginType = 0;
		g_pMain->m_SeekingPartyArray.push_back(pUser);
	}

	SendPartyBBSNeeded(pkt);

	uint16 counter = 0;

	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		counter++;
		if (counter < seeking_index)
			continue;

		result << (*itr)->m_strUserID << uint8((*itr)->m_sLevel) << (*itr)->m_sClass;
		if (counter >= 12)
			break;
	}

	g_pMain->m_SeekingPartyArrayLock.unlock();

	uint16 s = counter - seeking_index;
	// You still need to fill up ten slots.
	if (s < MAX_BBS_PAGE)
	{
		for (int j = s; j < MAX_BBS_PAGE; j++)
			result << uint16(0) << uint16(0)
			<< uint16(0) << uint8(0)
			<< uint8(0) << uint8(0)
			<< uint16(0)
			<< uint8(0);
	}
	result << uint16(0) << uint8(0) << uint16(s);

	Send(&result);
}

void CUser::PartyBBSDelete(Packet& pkt)
{
	if (m_bNeedParty == 1)
		return;

	g_pMain->m_SeekingPartyArrayLock.lock();
	// You don't need anymore 
	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		if ((*itr)->m_sSid == GetID())
		{
			g_pMain->m_SeekingPartyArray.erase(itr);
			break;
		}
	}
	g_pMain->m_SeekingPartyArrayLock.unlock();
	StateChangeServerDirect(2, 1); // not looking for a party
	SendPartyBBSNeeded(pkt);
}

void CUser::PartyBBSNeeded(Packet& pkt)
{
	/*uint16 page_index = 0;
	uint8 typefilter = 0, locationFilter = 0, levelFilter = 0;
	pkt >> page_index >> typefilter >> locationFilter >> levelFilter;
	SendPartyBBSNeeded(page_index,type,typefilter, locationFilter, levelFilter);*/
}

void CUser::SendPartyBBSNeeded(Packet& pkt)
{
	Packet result(WIZ_PARTY_BBS);
	uint16 page_index = 0;
	uint8 typefilter = 0, locationFilter = 0, levelFilter = 0;
	pkt >> page_index >> typefilter >> locationFilter >> levelFilter;
	uint16 start_counter = 0, BBS_Counter = 0;
	start_counter = page_index * MAX_BBS_PAGE;

	if (start_counter >= MAX_USER) {
		result << uint8(PARTY_TYPE_SEEKING) << uint8(PARTY_BBS_NEEDED) << uint8(0);
		Send(&result);
		return;
	}


	int ISTIRAP_counter = 0;
	result << uint8(PARTY_TYPE_SEEKING) << uint8(11) << uint8(1) << page_index << uint16(BBS_Counter);
	Guard lock(g_pMain->m_SeekingPartyArrayLock);
	foreach(itr, g_pMain->m_SeekingPartyArray) {
		_SEEKING_PARTY_USER* pUser = (*itr);
		if (pUser == nullptr) continue;
		uint8 PartyMembers = 0;
		uint16 sClass = pUser->m_sClass;

		ISTIRAP_counter++;
		int x_size = ISTIRAP_counter > 0 ? ISTIRAP_counter - 1 : ISTIRAP_counter;
		if (x_size < page_index * MAX_BBS_PAGE) continue;

		if ((typefilter == 2 && pUser->isPartyLeader == 1)
			|| (typefilter == 3 && pUser->isPartyLeader == 0)
			|| (locationFilter > 0 && locationFilter != pUser->m_bZone)
			|| (pUser->m_bloginType == 2)
			|| (levelFilter > 0 && levelFilter == 1 && pUser->m_sLevel > 11)
			|| (levelFilter > 0 && levelFilter == 2 && (pUser->m_sLevel < 11 || pUser->m_sLevel > 20))
			|| (levelFilter > 0 && levelFilter == 3 && (pUser->m_sLevel < 21 || pUser->m_sLevel > 30))
			|| (levelFilter > 0 && levelFilter == 4 && (pUser->m_sLevel < 31 || pUser->m_sLevel > 40))
			|| (levelFilter > 0 && levelFilter == 5 && (pUser->m_sLevel < 41 || pUser->m_sLevel > 50))
			|| (levelFilter > 0 && levelFilter == 6 && (pUser->m_sLevel < 51 || pUser->m_sLevel > 60))
			|| (levelFilter > 0 && levelFilter == 7 && (pUser->m_sLevel < 61 || pUser->m_sLevel > 70))
			|| (levelFilter > 0 && levelFilter == 8 && (pUser->m_sLevel < 71 || pUser->m_sLevel > 80))
			|| (levelFilter > 0 && levelFilter == 9 && pUser->m_sLevel < 81))
			continue;

		BBS_Counter++;
		if (BBS_Counter >= MAX_BBS_PAGE) break;

		if (pUser->isPartyLeader) {
			auto* pParty = g_pMain->GetPartyPtr(pUser->m_sPartyID);
			if (pParty == nullptr) return;
			pUser->m_strSeekingNote = pParty->WantedMessage;
			PartyMembers = GetPartyMemberAmount(pParty);
			sClass = pParty->sWantedClass;
		}

		result.DByte();
		result << pUser->m_bNation << uint8(pUser->m_bSeekType) << pUser->m_strUserID << sClass
			<< uint16(0) << pUser->m_sLevel << uint8(pUser->isPartyLeader ? 3 : 2);
		result.SByte();
		result << pUser->m_strSeekingNote << pUser->m_bZone << PartyMembers;
	}

	// You still need to fill up ten slots.
	if (BBS_Counter < MAX_BBS_PAGE) {
		for (int j = BBS_Counter; j < MAX_BBS_PAGE; j++) result << uint16(0) << uint16(0) << uint16(0) << uint8(0) << uint8(0) << uint8(0) << uint16(0) << uint8(0);
	}

	int page_number = (int)g_pMain->m_SeekingPartyArray.size() / MAX_BBS_PAGE;
	if (page_number % MAX_BBS_PAGE != 0) page_number++;
	result << page_index << page_number;
	result.put(5, BBS_Counter);
	Send(&result);
}
void CUser::PartyBBSWanted(Packet& pkt)
{
	uint16 page_index = 0;
	if (!isPartyLeader())
		return;

	_PARTY_GROUP* pParty = g_pMain->GetPartyPtr(GetPartyID());
	if (pParty == nullptr)
		return;

	Packet result(WIZ_PARTY_BBS, uint8(PARTY_TYPE_SEEKING));
	pkt.DByte();
	pkt >> pParty->sWantedClass >> page_index >> pParty->WantedMessage;
	result << uint8(PARTY_BBS_WANTED) << uint8(1);

	_SEEKING_PARTY_USER* pPartyUser = nullptr;
	uint16 seeking_index = 0; // holds the index where we located at in the vector

	g_pMain->m_SeekingPartyArrayLock.lock();

	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		seeking_index++;
		if ((*itr)->m_sSid == GetID())
		{
			pPartyUser = (*itr);
			pPartyUser->m_strSeekingNote = pParty->WantedMessage;
			pPartyUser->m_sClass = pParty->sWantedClass;
			break;
		}
	}
	
	
	if (pPartyUser == nullptr)
	{
		pPartyUser = new _SEEKING_PARTY_USER;
		pPartyUser->m_bNation = GetNation();
		pPartyUser->m_strUserID = GetName();
		pPartyUser->m_sClass = pParty->sWantedClass;
		pPartyUser->m_bSeekType = 0;
		pPartyUser->m_bZone = GetZoneID();
		pPartyUser->m_strSeekingNote = pParty->WantedMessage;
		pPartyUser->m_sLevel = GetLevel();
		pPartyUser->m_sSid = GetID();
		pPartyUser->m_sPartyID = GetPartyID();
		pPartyUser->isPartyLeader = isPartyLeader();
		pPartyUser->m_bloginType = 0;
		g_pMain->m_SeekingPartyArray.push_back(pPartyUser);
	}

	StateChangeServerDirect(2, 3); // Looking for party nember
	SendPartyBBSNeeded(pkt);

	uint16 counter = 0;

	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
		if ((*itr) == nullptr)
			continue;

		counter++;
		if (counter < seeking_index)
			continue;

		result << (*itr)->m_strUserID << uint8((*itr)->m_sLevel) << (*itr)->m_sClass;
		if (counter >= 12)
			break;
	}

	g_pMain->m_SeekingPartyArrayLock.unlock();

	uint16 s = counter - seeking_index;
	// You still need to fill up ten slots.
	if (s < MAX_BBS_PAGE)
	{
		for (int j = s; j < MAX_BBS_PAGE; j++)
			result << uint16(0) << uint16(0)
			<< uint16(0) << uint8(0)
			<< uint8(0) << uint8(0)
			<< uint16(0)
			<< uint8(0);
	}
	result << uint16(0) << uint8(0) << uint16(s);
	Send(&result);
}

void CUser::PartyBBSUserLoqOut()
{
	g_pMain->m_SeekingPartyArrayLock.lock();

	if (g_pMain->m_SeekingPartyArray.size() == 0)
	{
		g_pMain->m_SeekingPartyArrayLock.unlock();
		return;
	}

	foreach(itr, g_pMain->m_SeekingPartyArray)
	{
			if ((*itr) == nullptr)
				continue;

			if ((*itr)->m_sSid == GetID())
			{
				g_pMain->m_SeekingPartyArray.erase(itr);
				break;
			}
	}
	g_pMain->m_SeekingPartyArrayLock.unlock();

}

uint8 CUser::GetPartyMemberAmount(_PARTY_GROUP* pParty)
{
	if (pParty == nullptr)
		pParty = g_pMain->GetPartyPtr(GetPartyID());

	if (pParty == nullptr)
		return 0;

	uint8 PartyMembers = 0;
	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		// RECONNECT PARK - graceUid kontrolu kaldirildi
		if (pParty->uid[i] >= 0) // || pParty->graceUid[i] >= 0)
			PartyMembers++;
	}
	return PartyMembers;
}

void CUser::SendPartyStatusUpdate(uint8 bStatus, uint8 bResult /*= 0*/)
{
	if (!isInParty()) return;

	Packet result(WIZ_PARTY, uint8(PARTY_STATUSCHANGE));
	result << GetSocketID() << bStatus << bResult;
	g_pMain->Send_PartyMember(GetPartyID(), &result);
}

void CUser::GrantChickenManner()
{
	uint8 bLevel = GetLevel(), bManner = 0;
	// No manner points if you're not a chicken anymore nor when you're not in a party.
	if (!m_bIsChicken || !isInParty())
		return;

	_PARTY_GROUP* pParty = nullptr;
	pParty = g_pMain->GetPartyPtr(GetPartyID());

	if (pParty == nullptr)
		return;

	for (int i = 0; i < MAX_PARTY_USERS; i++) {
		CUser* pTargetUser = nullptr;
		if (pParty->uid[i] != GetSocketID()) pTargetUser = g_pMain->GetUserPtr(pParty->uid[i]);

		if (pTargetUser == nullptr || pTargetUser->isDead() || pTargetUser->m_bIsChicken) continue;
		if (!isInRange(pTargetUser, RANGE_50M)) continue;

		if (pTargetUser->GetLevel() > 20 && pTargetUser->GetLevel() < 40) bManner = pTargetUser->GetLevel() / 10;
		else bManner = 1;
		pTargetUser->SendMannerChange(bManner);
	}
}

/* ===== RECONNECT PARK (Session 25) — Party grace period devre dışı =====
 * Tekrar aktif etmek için: C:\temp\MYKO\yedekler\RECONNECT_BACKUP\
 * ===================================================================== */
#if 0 // RECONNECT PARK — Party DC Grace Period
#define PARTY_GRACE_PERIOD_SEC 120

void CUser::PartyStartGracePeriod(_PARTY_GROUP* pParty)
{
	if (pParty == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lock(pParty->m_lock);

	// Oyuncunun slot pozisyonunu bul
	int slotPos = -1;
	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		if (pParty->uid[i] == GetSocketID())
		{
			slotPos = i;
			break;
		}
	}

	if (slotPos < 0)
		return;

	// Grace bilgilerini kaydet
	pParty->graceUid[slotPos] = pParty->uid[slotPos];
	pParty->graceExpiry[slotPos] = (uint32)UNIXTIME + PARTY_GRACE_PERIOD_SEC;
	pParty->graceName[slotPos] = GetName();

	// Slot'u bos isaretle (ama grace ile kilitli)
	pParty->uid[slotPos] = -1;

	// Party uyelerine bildir: "X baglantisi kesildi"
	Packet result(WIZ_PARTY, uint8(PARTY_REMOVE));
	result << GetSocketID();
	g_pMain->Send_PartyMember(pParty->wIndex, &result);

	// Kullanicinin party flag'lerini temizle (CUser recycle edilecek)
	m_bInParty = false;
	m_bInEnterParty = false;
	m_sPartyIndex = -1;
}

bool CUser::RestorePartyFromGrace()
{
	std::string charName = GetName();

	// Buyuk kucuk harf farki olmasin
	std::string charNameUpper = charName;
	STRTOUPPER(charNameUpper);

	// Tum partyleri tara — bu karakter bir grace slot'unda mi?
	foreach_stlmap(itr, g_pMain->m_PartyArray)
	{
		_PARTY_GROUP* pParty = itr->second;
		if (pParty == nullptr)
			continue;

		std::lock_guard<std::recursive_mutex> lock(pParty->m_lock);
		for (int i = 0; i < MAX_PARTY_USERS; i++)
		{
			if (pParty->graceUid[i] < 0)
				continue;

			// Sure dolmus mu kontrol et
			if (UNIXTIME > pParty->graceExpiry[i])
			{
				// Expire olmus — temizle ve devam et
				pParty->graceUid[i] = -1;
				pParty->graceExpiry[i] = 0;
				pParty->graceName[i] = "";
				continue;
			}

			// Karakter adi eslesme kontrolu
			std::string gracNameUpper = pParty->graceName[i];
			STRTOUPPER(gracNameUpper);

			if (gracNameUpper != charNameUpper)
				continue;

			// ESLESTI — party'ye geri ekle
			pParty->uid[i] = GetSocketID();
			pParty->graceUid[i] = -1;
			pParty->graceExpiry[i] = 0;
			pParty->graceName[i] = "";

			// User party flag'lerini restore et
			m_sPartyIndex = pParty->wIndex;
			m_bInParty = true;
			m_bInEnterParty = true;
			m_bPartyLeader = (i == 0); // slot 0 = lider

			// Party uyelerine bildir: oyuncu geri geldi
			Packet result(WIZ_PARTY, uint8(PARTY_INSERT));
			result << GetSocketID()
				<< uint8(1) << GetName()
				<< m_MaxHp << m_sHp
				<< GetLevel() << GetClass()
				<< m_MaxMp << m_sMp
				<< GetNation() << uint8(0)
				<< pParty->NumberTargetID << m_sUserPartyType
				<< GetLoyaltySymbolRank();
			g_pMain->Send_PartyMember(m_sPartyIndex, &result);

			// Geri donen oyuncuya mevcut party listesini gonder
			for (int j = 0; j < MAX_PARTY_USERS; j++)
			{
				if (pParty->uid[j] < 0 || pParty->uid[j] == GetSocketID())
					continue;

				CUser* pMember = g_pMain->GetUserPtr(pParty->uid[j]);
				if (pMember == nullptr || !pMember->isInGame())
					continue;

				Packet memberInfo(WIZ_PARTY, uint8(PARTY_INSERT));
				memberInfo << pMember->GetSocketID()
					<< uint8(1) << pMember->GetName()
					<< pMember->m_MaxHp << pMember->m_sHp
					<< pMember->GetLevel() << pMember->m_sClass
					<< pMember->m_MaxMp << pMember->m_sMp
					<< pMember->GetNation() << uint8(0)
					<< pParty->NumberTargetID << pMember->m_sUserPartyType
					<< pMember->GetLoyaltySymbolRank();
				Send(&memberInfo);
			}

			// Lider bilgisini goster (slot 0 = lider sembol)
			if (i == 0)
				StateChangeServerDirect(6, 1);

			// HP bilgilerini guncelle
			for (int j = 0; j < MAX_PARTY_USERS; j++)
			{
				CUser* pMember = g_pMain->GetUserPtr(pParty->uid[j]);
				if (pMember == nullptr)
					continue;
				pMember->SendPartyHpManager(PartyType::Send_All);
			}

			return true;
		}
	}

	return false;
}
#endif // RECONNECT PARK

#pragma endregion