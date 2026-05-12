#include "stdafx.h"
#include "DBAgent.h"

using std::string;
using std::unique_ptr;
extern CDBAgent g_DBAgent;

#pragma region CUser::HandleGenie(Packet & pkt)
void CUser::HandleGenie(Packet & pkt)
{
	uint8 command = pkt.read<uint8>();

	switch (command)
	{
	case GenieInfoRequest:
		GenieNonAttackProgress(pkt);
		break;
	case GenieUpdateRequest:
		GenieAttackProgress(pkt);
		break;
	case 25:
		GenieNotice(pkt);
		break;
	default:
		TRACE("Genie OpCode %u \n", command);
		break;
	}
}
#pragma endregion

void CUser::GenieNotice(Packet& pkt)
{
	bool status = pkt.read<bool>();
	std::string notice = "Running towards to mob deactivated.";
	if (status) notice = "Running towards to mob activated.";
	g_pMain->SendGameNotice(ChatType::GENERAL_CHAT, notice, "", false, this, false);
}

#pragma region CUser::GenieNonAttackProgress(Packet & pkt)
void CUser::GenieNonAttackProgress(Packet & pkt)
{
	uint8 command = pkt.read<uint8>();

	switch (command)
	{
	case GenieUseSpiringPotion:
		GenieUseGenieSpirint(pkt);
		break;
	case GenieLoadOptions:
		HandleGenieLoadOptions();
		break;
	case GenielSaveOptions:
		HandleGenieSaveOptions(pkt);
		break;
	case GenieStartHandle:
		GenieStart();
		break;
	case GenieStopHandle:
		GenieStop();
		break;
	default:
		TRACE("GenieNonAttackProgress Unknow Attack Handle %d\n", command);
		//printf("GenieNonAttackProgress Unknow Attack Handle %d\n", command);		// Hata ! Buras� s�rekli uyar� veriyor.  - Handle 155 & 22 & 130 & 90 & 87 Uyar�s�
		break;
	}

}
#pragma endregion

#pragma region CUser::HandleGenieLoadOptions()
void CUser::HandleGenieLoadOptions()
{
	Packet result(WIZ_GENIE, uint8(GenieInfoRequest));
	result << uint8(GenieLoadOptions);

	for (int i = 0; i < sizeof(m_GenieOptions); i++)
		result << uint8(*(uint8 *)(m_GenieOptions + i));
	Send(&result);
}
#pragma endregion

#pragma region CUser::HandleGenieSaveOptions(Packet & pkt)
void CUser::HandleGenieSaveOptions(Packet & pkt)
{
	for (int i = 0; i < sizeof(m_GenieOptions); i++)
		*(uint8 *)(m_GenieOptions + i) = pkt.read<uint8>();
}
#pragma endregion

#pragma region CUser::GenieAttackProgress(Packet & pkt)
void CUser::GenieAttackProgress(Packet & pkt)
{
	uint8 command = pkt.read<uint8>();
	if (UNIXTIME > m_1098GenieTime) return SendGenieStop(true);

	switch (command)
	{
	case GenieMove:
		MoveProcess(pkt);
		break;
	case GenieRotate:
		Rotate(pkt);
		break;
	case GenieMainAttack:
		Attack(pkt);
		break;
	case GenieMagic:
		CMagicProcess::MagicPacket(pkt, this);
		break;
	default:
		TRACE("Genie Unknow Attack Handle %d\n", command);
		break;
	}
}
#pragma endregion

#pragma region CUser::GenieStart()
void CUser::GenieStart()
{
	if ((g_pMain->pServerSetting.LootandGeniePremium && GetPremium() == 0) || UNIXTIME > m_1098GenieTime)
		return;

	Packet result(WIZ_GENIE, uint8(GenieStatusActive));
	result << uint8(4) << uint16(1) << GetGenieTime();
	m_bGenieStatus = true;
	Send(&result);
	SendGenieStart(true);
}
#pragma endregion

#pragma region CUser::SendGenieStart(bool isToRegion /* = false */)
void CUser::SendGenieStart(bool isToRegion /* = false */)
{
	Packet result(WIZ_GENIE, uint8(GenieInfoRequest));
	result << uint8(GenieStartHandle) << uint16(1) << GetGenieTime();
	Send(&result);
	Packet newpkt(XSafe, uint8(0xDB));
	newpkt << uint8(1);
	Send(&newpkt);
	Packet result2(WIZ_GENIE, uint8(GenieInfoRequest));
	result2 << uint8(GenieActivated) << uint16(GetID()) << uint8(1);

	if (isToRegion) SendToRegion(&result2, nullptr, GetEventRoom());
	else Send(&result2);
	
	if (m_bGenieStatus == GenieStatusInactive) GenieStop();
	//SetOffCha(_choffstatus::ACTIVE, offcharactertype::genie);
}
#pragma endregion

#pragma region CUser::GenieStop()
void CUser::GenieStop()
{
	if (m_bGenieStatus == GenieStatusInactive) return;

	Packet newpkt(XSafe, uint8(0xDB));
	newpkt << uint8(0);
	Send(&newpkt);
	Packet result(WIZ_GENIE, uint8(1));
	result << uint8(5) << uint16(1) << GetGenieTime();
	m_bGenieStatus = false;
	// B9 fix: Zone gecisi sirasinda SendToRegion eski zone'a broadcast yapiyor
	// ve state tutarsizligi olusturuyor. Zone gecisindeyken region broadcast
	// yapma — oyuncu zaten INOUT_WARP ile eski zone'dan cikacak.
	SendGenieStop(!m_bIsZoneChanging);
	Send(&result);
	{
		Packet newpkt(XSafe, uint8(0xDB));
		newpkt << uint8(0);
		Send(&newpkt);
	}
}
#pragma endregion

void CUser::SendGenieStop(bool isToRegion /* = false */)
{
	Packet result(WIZ_GENIE, uint8(GenieInfoRequest));
	if (GetGenieTime() > 0) 
		result << uint8(GenieStopHandle) << uint16(1) << GetGenieTime();
	Send(&result);
	Packet newpkt(XSafe, uint8(0xDB));
	newpkt << uint8(0);
	Send(&newpkt);
	Packet result2(WIZ_GENIE, uint8(GenieInfoRequest));
	result2 << uint8(GenieActivated) << uint16(GetID()) << uint8(0);
	if (isToRegion) SendToRegion(&result2, nullptr, GetEventRoom());
	else Send(&result2);
	//SetOffCha(_choffstatus::DEACTIVE, offcharactertype::genie);
}

#pragma region CUser::UpdateGenieTime(uint16 m_sTime)
void CUser::UpdateGenieTime(uint16 m_sTime)
{
	Packet result(WIZ_GENIE, uint8(GenieInfoRequest));
	result << uint8(GenieRemainingTime) << m_sTime;
	Send(&result);
	if (m_sTime == GenieStatusInactive) GenieStop();
}
#pragma endregion

#pragma region CUser::GenieUseGenieSpirint()
void CUser::GenieUseGenieSpirint(Packet & pkt)
{

#if 0
	if (isTrading() || isMerchanting() || isMining() || isFishing()) return;

	uint32 nItemID; uint16 GenieItem;
	pkt >> nItemID;

	_ITEM_TABLE* ItemTable = g_pMain->GetItemPtr(nItemID);
	if (ItemTable == nullptr)
		return;

	GenieItem = GetItemCount(nItemID);

	if (!CheckExistItem(nItemID))
		return;

	if (nItemID != 810305000 && nItemID != 810378000  && nItemID != 900772000)
		return;

	if (RobItem(nItemID))
		m_GenieTime = 120;

	if (m_sFirstUsingGenie <= 0)
		m_sFirstUsingGenie = 1;

	Packet result(WIZ_GENIE, uint8(GenieUseSpiringPotion));
	result << uint8(GenieUseSpiringPotion) << GetGenieTime();
	Send(&result);
#endif // 0
}
#pragma endregion

bool CDBAgent::UpdateGenieData(string & strCharID, CUser *pUser)
{
	if (strCharID != pUser->GetName())
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_GenieOptions, sizeof(pUser->m_GenieOptions), SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_GENIE_DATA(?,?, ?, %d,%d)}"), (uint32)pUser->m_1098GenieTime, pUser->m_sFirstUsingGenie))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}

bool CDBAgent::LoadGenieData(string & strCharID, CUser *pUser)
{
	if (pUser == nullptr
		|| strCharID != pUser->GetName())
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());

	if (!dbCommand->Execute(_T("{CALL LOAD_GENIE_DATA(?,?)}")))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return false;

	int field = 1; uint32 genietime = 0;
	dbCommand->FetchUInt32(field++, genietime);
	dbCommand->FetchBinary(field++, (char *)pUser->m_GenieOptions, sizeof(pUser->m_GenieOptions));
	dbCommand->FetchByte(field++, pUser->m_sFirstUsingGenie);

	pUser->m_1098GenieTime = genietime;
	return true;
}
#pragma endregion

bool CUser::GenieExchange(uint32 itemid, uint32 time, bool newChar) {

	if (!time || (!newChar && !itemid))
		return false;

	if (time > 720) time = 720;

	if (!newChar && (!CheckExistItem(itemid) || !RobItem(itemid)))
		return false;

	if (!m_sFirstUsingGenie) m_sFirstUsingGenie = 1;
	int remtime = int(m_1098GenieTime > UNIXTIME ? m_1098GenieTime - UNIXTIME : 0);

	m_1098GenieTime = UNIXTIME + (time * HOUR) + (remtime > 0 ? remtime : 0);

	if (!newChar) {
		Packet result(WIZ_GENIE, uint8(GenieUseSpiringPotion));
		result << uint8(GenieUseSpiringPotion) << GetGenieTime();
		Send(&result);
	}

	return true;
}

void CUser::CheckGenieTime() {
	Packet result(WIZ_GENIE, uint8(GenieInfoRequest));
	result << uint8(GenieRemainingTime) << GetGenieTime();
	Send(&result);
	if (UNIXTIME > m_1098GenieTime) GenieStop();
}
