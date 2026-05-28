#include "stdafx.h"
#include "../shared/database/OdbcConnection.h"
#include "KnightsManager.h"
#include "DBAgent.h"
#include "../shared/DateTime.h"

CDBAgent g_DBAgent;

using std::string;
using std::unique_ptr;

CDBAgent::CDBAgent() {
	for (int i = 0; i < GAME_DB_POOL_SIZE; i++)
		m_GameDBPool[i] = new OdbcConnection();
	m_AccountDB = new OdbcConnection();
	m_LogDB = new OdbcConnection();
}

CDBAgent::~CDBAgent() {
	for (int i = 0; i < GAME_DB_POOL_SIZE; i++)
		delete m_GameDBPool[i];
	delete m_AccountDB;
	delete m_LogDB;
}

OdbcConnection* CDBAgent::GetGameDB() {
	uint32_t idx = m_poolCounter.fetch_add(1, std::memory_order_relaxed) % GAME_DB_POOL_SIZE;
	return m_GameDBPool[idx];
}

bool CDBAgent::Startup(bool bMarsEnabled, tstring& strAccountDSN, tstring& strAccountUID, tstring& strAccountPWD,
	tstring& strGameDSN, tstring& strGameUID, tstring& strGamePWD, tstring& strLogDSN, tstring& strLogUID, tstring& strLogPWD)
{
	if (!Connect(bMarsEnabled, strAccountDSN, strAccountUID, strAccountPWD, strGameDSN, strGameUID, strGamePWD, strLogDSN, strLogUID, strLogPWD)) {
		printf(_T("ERROR: Failed to connect to the database server.\n"));
		return false;
	}
	return true;
}

bool CDBAgent::Connect(bool bMarsEnabled, tstring& strAccountDSN, tstring& strAccountUID, tstring& strAccountPWD,
	tstring& strGameDSN, tstring& strGameUID, tstring& strGamePWD,
	tstring& strLogDSN, tstring& strLogUID, tstring& strLogPWD)
{
	if (!m_AccountDB->Connect(strAccountDSN, strAccountUID, strAccountPWD, bMarsEnabled)) { ReportSQLError(m_AccountDB->GetError()); return false; }
	for (int i = 0; i < GAME_DB_POOL_SIZE; i++) {
		if (!m_GameDBPool[i]->Connect(strGameDSN, strGameUID, strGamePWD, bMarsEnabled)) { ReportSQLError(m_GameDBPool[i]->GetError()); return false; }
	}
	if (!m_LogDB->Connect(strLogDSN, strLogUID, strLogPWD, bMarsEnabled)) { ReportSQLError(m_LogDB->GetError()); return false; }
	return true;
}

void CDBAgent::ReportSQLError(OdbcError *pError)
{
	if (pError == nullptr)
		return;

	DateTime time;

	// This is *very* temporary.
	string errorMessage = string_format(_T("[ ODBC Error - %d.%d.%d %d:%d:%d ] ] Source: %s Error: %s Description: %s\n"),
		time.GetDay(), time.GetMonth(), time.GetYear(), time.GetHour(), time.GetMinute(), time.GetSecond(),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	// Write to structured ODBC log file (Logs/ODBC_YYYY-MM-DD.log)
	LOG_ODBC("[ODBC_ERROR] Source=%s Error=%s Description=%s",
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	Guard lock(m_lock);

	FILE *fp = fopen("./Logs/GameServer.log", "a");
	if (fp != nullptr)
	{
		fwrite(errorMessage.c_str(), errorMessage.length(), 1, fp);
		fclose(fp);
	}

	TRACE("Database error: %s\n", errorMessage.c_str());
	delete pError;
}

int8 CDBAgent::AccountLogin(string & strAccountID, string & strPasswd)
{
	if (strAccountID.length() > MAX_ID_SIZE || strPasswd.length() > MAX_PW_SIZE)
		return -1;

	int8 bRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strPasswd.c_str(), strPasswd.length());

	if (!dbCommand->Execute(_T("{? = CALL GAME_LOGIN_V2(?, ?)}")))
		ReportSQLError(GetGameDB()->GetError());

	// ODBC FIX: SELECT resultset'i tuketmeden RETURN value output param'a yazilmiyor
	while (dbCommand->MoveNext()) {}

	return bRet;
}

uint8 CDBAgent::NationSelect(string & strAccountID, uint8 bNation)
{
	if (bNation != 1 && bNation != 2) return false;

	uint8 bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL NATION_SELECT(?, %d)}"), bNation)))
		ReportSQLError(GetGameDB()->GetError());

	return (bRet > 0 ? bNation : 0);
}

#pragma region CDBAgent::GetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3, string & strCharID4)
bool CDBAgent::GetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3, string & strCharID4)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	int8 bRet = -2; // generic error

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("{? = CALL GET_ALL_CHARID(?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchString(1, strCharID1);
	dbCommand->FetchString(2, strCharID2);
	dbCommand->FetchString(3, strCharID3);
	dbCommand->FetchString(4, strCharID4);

	return true;
}
#pragma endregion

void CDBAgent::LoadCharInfo(string & strCharID, ByteBuffer & result)
{
	uint32 nHair = 0;
	uint16 sClass = 0;
	uint8 bRace = 0, bLevel = 0, bFace = 0, bZone = 0; 
	char strItem[INVENTORY_TOTAL * 8];
	char strItemExpiration[INVENTORY_TOTAL * 4];
	ByteBuffer itemData;

	// ensure it's all 0'd out initially.
	memset(strItem, 0x00, sizeof(strItem));
	memset(strItemExpiration, 0x00, sizeof(strItemExpiration));

	if (!strCharID.empty())
	{
		unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
		if (dbCommand.get() == nullptr)
			return;

		dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

		if (!dbCommand->Execute(_T("{CALL LOAD_CHAR_INFO(?)}")))
			ReportSQLError(GetGameDB()->GetError());

		if (dbCommand->hasData())
		{
			dbCommand->FetchByte(1, bRace);
			dbCommand->FetchUInt16(2, sClass);
			dbCommand->FetchUInt32(3, nHair);
			dbCommand->FetchByte(4, bLevel);
			dbCommand->FetchByte(5, bFace);
			dbCommand->FetchByte(6, bZone);
			dbCommand->FetchBinary(7, strItem, sizeof(strItem));
			dbCommand->FetchBinary(8, strItemExpiration, sizeof(strItemExpiration));
		}
	}

	itemData.append(strItem, sizeof(strItem));

	result	<< strCharID << bRace << sClass << bLevel << bFace << nHair << bZone;
	for (int i = 0; i < SLOT_MAX; i++) 
	{
		uint32 nItemID;
		uint16 sDurability, sCount;
		itemData >> nItemID >> sDurability >> sCount;
		if (i == HEAD || i == BREAST || i == SHOULDER || i == LEG || i == GLOVE || i == FOOT || i == RIGHTHAND || i == LEFTHAND)
			result << nItemID << sDurability;
	}
}

int8 CDBAgent::CreateNewChar(string & strAccountID, int index, string & strCharID, uint8 bRace, uint16 sClass, uint32 nHair, uint8 bFace, uint8 bStr, uint8 bSta, uint8 bDex, uint8 bInt, uint8 bCha)
{
	int8 bRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL CREATE_NEW_CHAR(?, %d, ?, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), 
		index, bRace, sClass, nHair, bFace, bStr, bSta, bDex, bInt, bCha)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

void CDBAgent::LoadNewCharSet(std::string& strCharID, uint16 sClass)
{
	int8 bRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL [LOAD_NEW_CHAR_SET](?,%d)}"), sClass)))
		ReportSQLError(GetGameDB()->GetError());
}

void CDBAgent::LoadNewCharValue(std::string& strCharID, uint16 sClass)
{
	int8 bRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL [LOAD_NEW_CHAR_VALUE](?,%d, %d)}"), sClass, g_pMain->m_nServerNo)))
		ReportSQLError(GetGameDB()->GetError());
}

int8 CDBAgent::ChangeHair(std::string & strAccountID, std::string & strCharID, uint8 bOpcode, uint8 bFace, uint32 nHair)
{
	int8 bRet = 1; // failed
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL CHANGE_HAIR(?, ?, %d, %d, %d)}"), 
		bOpcode, bFace, nHair)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

int8 CDBAgent::DeleteChar(string & strAccountID, int index, string & strCharID, string & strSocNo)
{
	int8 bRet = -2; // generic error
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSocNo.c_str(), strSocNo.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL DELETE_CHAR(?, %d, ?, ?)}"), index)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

#pragma region CDBAgent::VIPStorageSetPassword(std::string &strSealPasswd, std::string & strAccountID, uint8 KeyCancel)
uint8 CDBAgent::VIPStorageSetPassword(std::string &strSealPasswd, std::string & strAccountID, uint8 KeyCancel)
{
	uint8 bRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 3;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSealPasswd.c_str(), strSealPasswd.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL VIP_STORAGE_PASSWORD(?, ?, %d)}"), KeyCancel)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}
#pragma endregion

#pragma region CDBAgent::VIPStorageCancelPassword(std::string & strAccountID)
uint8 CDBAgent::VIPStorageCancelPassword(std::string & strAccountID, uint8 KeyCancel)
{
	uint8 bRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 2;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL VIP_STORAGE_CANCEL_PASSWORD(?,%d)}"), KeyCancel)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}
#pragma endregion

#pragma region CDBAgent::VIPStorageUseVaultKey(std::string &strAccountID, uint32 iExpirationTime)

uint8 CDBAgent::VIPStorageUseVaultKey(std::string &strAccountID, uint32 iExpirationTime)
{
	uint8 bRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 3;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL VIP_STORAGE_USE_VAULTKEY(?, %d)}"), iExpirationTime)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

#pragma endregion

void CDBAgent::LoadRentalData(string & strAccountID, string & strCharID, UserRentalMap & rentalData)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("{CALL LOAD_RENTAL_DATA(?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	do
	{
		_USER_RENTAL_ITEM *pItem = new _USER_RENTAL_ITEM();
		_RENTAL_ITEM *pRentalItem = nullptr;

		dbCommand->FetchString(1, pItem->strUserID);
		if (STRCASECMP(pItem->strUserID.c_str(), strCharID.c_str()) != 0)
		{
			delete pItem;
			continue;
		}

		dbCommand->FetchByte(2, pItem->byRentalType);
		dbCommand->FetchByte(3, pItem->byRegType);
		dbCommand->FetchUInt32(4, pItem->nRentalIndex);
		dbCommand->FetchUInt32(5, pItem->nItemID);
		dbCommand->FetchUInt16(6, pItem->sDurability);
		dbCommand->FetchUInt64(7, pItem->nSerialNum);
		dbCommand->FetchUInt32(8, pItem->nRentalMoney);
		dbCommand->FetchUInt16(9, pItem->sRentalTime);
		dbCommand->FetchInt16(10, pItem->sMinutesRemaining);
		dbCommand->FetchString(11, pItem->szTimeRental, sizeof(pItem->szTimeRental));

		pRentalItem = g_pMain->m_RentalItemArray.GetData(pItem->nRentalIndex);
		if (pRentalItem == nullptr
			|| rentalData.find(pItem->nSerialNum) != rentalData.end())
			delete pItem;
		else
			rentalData.insert(std::make_pair(pItem->nSerialNum, pItem));

	} while (dbCommand->MoveNext());
}

void CDBAgent::LoadItemSealData(string& strAccountID, string& strCharID, UserItemSealMap& itemSealData)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("SELECT nItemSerial, nItemID, bSealType, prelockstate FROM SEALED_ITEMS WHERE strAccountID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	do
	{
		int field = 1;
		_USER_SEAL_ITEM* pItem = new _USER_SEAL_ITEM;
		dbCommand->FetchUInt64(field++, pItem->nSerialNum);
		dbCommand->FetchUInt32(field++, pItem->nItemID);
		dbCommand->FetchByte(field++, pItem->bSealType);
		dbCommand->FetchByte(field++, pItem->prelockstate);
		itemSealData.insert(std::make_pair(pItem->nSerialNum, pItem));
	} while (dbCommand->MoveNext());
}

uint8 CDBAgent::CharacterSelectLoginChecking(std::string& AccountID, std::string& CharID)
{
	if (CharID.length() > MAX_ID_SIZE)
		return 0;

	uint8 bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, AccountID.c_str(), AccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, CharID.c_str(), CharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL CHARACTER_LOGIN_CHECKS(?, ?)}")))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}
		
	return bRet;
}

void CDBAgent::InsertTrashItemList(CUser *pUser, uint32 itemid, uint32 itemcount, uint16 duration, uint32 deleteime, uint64 serial, uint8 bflag) {
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return;
	if (pUser == nullptr) return;
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(string_format(_T("{CALL INSERT_TRASH_ITEMLIST(?,%d,%d,%d," I64FMTD ",%d,%d)}"), itemid, duration,deleteime,serial, bflag, itemcount)))
		ReportSQLError(GetGameDB()->GetError());
}

void CDBAgent::DeleteTrashItem(CUser *pUser, uint64 serial) {
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return;
	if (pUser == nullptr) return;
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(string_format(_T("{CALL DELETE_TRASH_ITEM(?," I64FMTD ")}"), serial)))
		ReportSQLError(GetGameDB()->GetError());
}

bool CDBAgent::LoadTrashItemList(CUser *pUser) {
	if (pUser == nullptr )
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(_T("{CALL LOAD_TRASH_ITEMLIST(?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return true;

	do {
		uint32 itemid, deletetime, itemcount; uint16 itemduration; uint8 flag; uint64 serialnum;  int field = 1;
		dbCommand->FetchUInt32(field++, itemid);
		dbCommand->FetchUInt32(field++, deletetime);
		dbCommand->FetchUInt16(field++, itemduration);
		dbCommand->FetchUInt32(field++, itemcount);
		dbCommand->FetchByte(field++, flag);
		dbCommand->FetchUInt64(field++, serialnum);

		if (!itemid || (uint32)UNIXTIME >= deletetime || !itemcount) continue;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr(itemid);
		if (pTable.isnull()) continue;

		if ((pTable.m_bKind == 255 && !pTable.m_bCountable) ||(!pTable.m_bCountable)) itemcount = 1;
		
		short RemoveID = pUser->GetRepurchaseFreeID();
		if (RemoveID == -1 || pUser->m_sDeletedItemMap.GetData(RemoveID) != nullptr) continue;

		_DELETED_ITEM* pDelete = new _DELETED_ITEM;
		pDelete->nNum = itemid;
		pDelete->sCount = itemcount;
		pDelete->iDeleteTime = deletetime;
		pDelete->serialnum = serialnum;
		pDelete->bflag = flag;
		pDelete->itemduration = itemduration;
		pDelete->status = false;
		if (!pUser->m_sDeletedItemMap.PutData(RemoveID, pDelete)) delete pDelete;
	} while (dbCommand->MoveNext());
	return true;
}

bool CDBAgent::LoadUserData(string & strAccountID, string & strCharID, CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (pUser == nullptr || pUser->m_bLogout || strCharID.length() > MAX_ID_SIZE)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(_T("{CALL LOAD_USER_DATA(?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
		
	if (!dbCommand->hasData())
		return false;

	char strItem[INVENTORY_TOTAL * 8] { },
		strSerial[INVENTORY_TOTAL * 8]{ },
		strItemExpiration[INVENTORY_TOTAL * 4],
		strDeletedItem[40 * 12]{};

	char strLevel[83]{};

	memset(strItem, 0, sizeof(strItem));
	memset(strSerial, 0, sizeof(strSerial));
	memset(strItemExpiration, 0, sizeof(strItemExpiration));
	memset(strDeletedItem, 0, sizeof(strDeletedItem));
	memset(strLevel, 0, sizeof(strLevel));

	int field = 1;
	dbCommand->FetchByte(field++, pUser->m_bNation);
	dbCommand->FetchByte(field++, pUser->m_bRace);
	dbCommand->FetchUInt16(field++, pUser->m_sClass);
	dbCommand->FetchUInt32(field++, pUser->m_nHair);
	dbCommand->FetchByte(field++, pUser->m_bRank);
	dbCommand->FetchByte(field++, pUser->m_bTitle);
	dbCommand->FetchByte(field++, pUser->m_bLevel);
	dbCommand->FetchByte(field++, pUser->m_bRebirthLevel);
	dbCommand->FetchInt64(field++, pUser->m_iExp);
	dbCommand->FetchUInt32(field++, pUser->m_iLoyalty);
	dbCommand->FetchByte(field++, pUser->m_bFace);
	dbCommand->FetchByte(field++, pUser->m_bCity);
	dbCommand->FetchInt16(field++, pUser->m_bKnights);
	dbCommand->FetchByte(field++, pUser->m_bFame);
	dbCommand->FetchInt16(field++, pUser->m_sHp);
	dbCommand->FetchInt16(field++, pUser->m_sMp);
	dbCommand->FetchInt16(field++, pUser->m_sSp);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_STR]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_STA]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_DEX]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_INT]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_CHA]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_STR]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_STA]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_DEX]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_INT]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_CHA]);
	dbCommand->FetchByte(field++, pUser->m_bAuthority);
	dbCommand->FetchInt16(field++, pUser->m_sPoints);
	dbCommand->FetchUInt32(field++, pUser->m_iGold);
	dbCommand->FetchByte(field++, pUser->m_bZone);
	dbCommand->FetchInt16(field++, pUser->m_sBind);
	pUser->m_curx = (float)(dbCommand->FetchInt32(field++) / 100.0f);
	pUser->m_curz = (float)(dbCommand->FetchInt32(field++) / 100.0f);
	pUser->m_cury = (float)(dbCommand->FetchInt32(field++) / 100.0f);
	pUser->m_oldx = pUser->m_curx;
	pUser->m_oldy = pUser->m_cury;
	pUser->m_oldz = pUser->m_curz;
	pUser->m_dwTime = dbCommand->FetchUInt32(field++) + 1;
	dbCommand->FetchString(field++, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->FetchBinary(field++, strItem, sizeof(strItem));
	dbCommand->FetchBinary(field++, strSerial, sizeof(strSerial));
	dbCommand->FetchBinary(field++, strItemExpiration, sizeof(strItemExpiration));
	dbCommand->FetchBinary(field++, strLevel, sizeof(strLevel));
	dbCommand->FetchUInt32(field++, pUser->m_iMannerPoint);
	dbCommand->FetchUInt32(field++, pUser->m_iLoyaltyMonthly);
	dbCommand->FetchInt32(field++, pUser->m_mutetimestatus);
	dbCommand->FetchInt32(field++, pUser->m_attacktimestatus);
	dbCommand->FetchString(field++, pUser->mytagname);
	dbCommand->FetchUInt32(field++, pUser->m_tagnamergb);
	dbCommand->FetchByte(field++, pUser->ChickenStatus);
	dbCommand->FetchUInt32(field++, pUser->m_flashtime);
	dbCommand->FetchByte(field++, pUser->m_flashcount);
	dbCommand->FetchByte(field++, pUser->m_flashtype);

	pUser->m_strUserID = strCharID;
	pUser->m_lastSaveTime = UNIXTIME + (pUser->GetSocketID() % PLAYER_SAVE_INTERVAL);
	pUser->m_lastBonusTime = UNIXTIME;
	pUser->m_TimeOnline = UNIXTIME;

	// FIX: Gold overflow protection on load
	if (pUser->m_iGold > COIN_MAX)
		pUser->m_iGold = COIN_MAX;

	// FIX: Stat validation on load
	if (pUser->m_bLevel > 83) pUser->m_bLevel = 83;
	if (pUser->m_sHp < 0) pUser->m_sHp = 0;
	if (pUser->m_sMp < 0) pUser->m_sMp = 0;
	if (pUser->m_sSp < 0) pUser->m_sSp = 0;

	ByteBuffer ItemBuffer, DeletedItemBuffer, SerialBuffer, ItemTimeBuffer;
	ItemBuffer.append(strItem, sizeof(strItem));
	DeletedItemBuffer.append(strDeletedItem, sizeof(strDeletedItem));
	SerialBuffer.append(strSerial, sizeof(strSerial));
	ItemTimeBuffer.append(strItemExpiration, sizeof(strItemExpiration));

	ByteBuffer strLevelBuffer;
	strLevelBuffer.append(strLevel, sizeof(strLevel));

	memset(pUser->m_sItemArray, 0x00, sizeof(pUser->m_sItemArray));
	memset(pUser->m_sLevelArray, 0x00, sizeof(pUser->m_sLevelArray));

	pUser->m_sDeletedItemMap.DeleteAllData();
	
	if (pUser->m_sealedItemMap.size()) {
		foreach(itr, pUser->m_sealedItemMap) {
			if (itr->second == nullptr)
				continue;
			
			delete itr->second;
		}
	}
	pUser->m_sealedItemMap.clear();

	LoadItemSealData(strAccountID, strCharID, pUser->m_sealedItemMap);
	LoadKnightCash(strAccountID, pUser);

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{ 
		uint64 nSerialNum;
		uint32 nItemID;
		int16 sDurability, sCount;
		uint32 nItemTime;

		ItemBuffer >> nItemID >> sDurability >> sCount;
		SerialBuffer >> nSerialNum;			//�TEMLER KARARIRSA AKINA HABER VER BUFLARI BA�TAN YAZICAK 26.10.2020 02.05
		ItemTimeBuffer >> nItemTime;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull() || sCount <= 0)
			continue;

		if (!pTable.m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		if (nSerialNum == 0)
			nSerialNum = g_pMain->GenerateItemSerial();

		_ITEM_DATA* pItem = pUser->GetItem(i);
		if (pItem == nullptr)
			continue;

		pItem->nNum = nItemID; 
		pItem->sDuration = pTable.isAccessory() ? pTable.m_sDuration : sDurability;
		pItem->sCount = sCount;
		pItem->nSerialNum = nSerialNum;
		pItem->nExpirationTime = nItemTime;

		if (i == SHOULDER 
			&& (nItemID == ROBIN_LOOT_ITEM || nItemID == 850680000 || nItemID == 510000000 || nItemID == 520000000))
			pUser->m_autoloot = true;
		
		if (i == CFAIRY && nItemID == ITEM_OREADS) 
			pUser->m_fairycheck = true;

		if ((pTable.m_bKind == 255 && !pTable.m_bCountable) || (!pTable.m_bCountable)) pItem->sCount = 1;

		UserItemSealMap::iterator sealitr = pUser->m_sealedItemMap.find(nSerialNum);
		if (sealitr != pUser->m_sealedItemMap.end()) {
			if (sealitr->second->bSealType == 1) pItem->bFlag = (uint8)ItemFlag::ITEM_FLAG_SEALED;
			else if (sealitr->second->bSealType == 3) pItem->bFlag = (uint8)ItemFlag::ITEM_FLAG_BOUND;
			else if (sealitr->second->bSealType == 4) pItem->bFlag = (uint8)ItemFlag::ITEM_FLAG_NOT_BOUND;
			pItem->oFlag = sealitr->second->prelockstate;
		}

		if (pUser->dupplicate_itemcheck(pItem->nSerialNum))
		{
			auto pTable = g_pMain->GetItemPtr(pItem->nNum);
			if (!pTable.isnull()) {
				printf("Dupplicate Item Inventory: AccountName:%s - UserName:%s -  [Item Name:%s, ItemNum:%d, SerialNum:%I64d, ItemSlot:%d] \n",
					pUser->GetAccountName().c_str(), pUser->GetName().c_str(), pTable.m_sName.c_str(), pTable.Getnum(), pItem->nSerialNum, i);
			}
			// FIX: Clear duplicate item slot instead of just flagging
			pItem->nNum = 0;
			pItem->sDuration = 0;
			pItem->sCount = 0;
			pItem->nSerialNum = 0;
			pItem->nExpirationTime = 0;
			pItem->bFlag = 0;
		}
	}

	for (int i = 0; i < 83; i++)
	{
		bool sCheck = false;
		strLevelBuffer >> sCheck;
		pUser->m_sLevelArray[i] = sCheck;
	}

	// FIX: Authority validation - reject invalid values
	if (pUser->m_bAuthority != (uint8)AuthorityTypes::AUTHORITY_PLAYER
		&& pUser->m_bAuthority != (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER
		&& pUser->m_bAuthority != (uint8)AuthorityTypes::AUTHORITY_GM_USER)
	{
		printf("Invalid authority value %d for AccountID: (%s) Character: (%s), resetting to PLAYER\n",
			pUser->m_bAuthority, strAccountID.c_str(), strCharID.c_str());
		pUser->m_bAuthority = (uint8)AuthorityTypes::AUTHORITY_PLAYER;
	}

	if (pUser->m_bAuthority == (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER || pUser->m_bAuthority == (uint8)AuthorityTypes::AUTHORITY_GM_USER)
	{
		if (!g_DBAgent.LoadGameMasterSetting(strCharID, pUser))
		{
			printf("�zinsiz Gm Giri� Yap�m� ! Gm AccountID: (%s) Character Name: (%s) \n", strAccountID.c_str(), strCharID.c_str());
			return false;
		}
	}

	pUser->m_bCharacterDataLoaded = true;
	return true;
}

bool CDBAgent::LoadWarehouseData(string & strAccountID, CUser *pUser)
{
	char strItem[WAREHOUSE_MAX * 8], strSerial[WAREHOUSE_MAX * 8], strItemExpiration[WAREHOUSE_MAX * 4];

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (pUser == nullptr 
		|| pUser->m_bLogout)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("SELECT nMoney, WarehouseData, strSerial, WarehouseDataTime FROM WAREHOUSE WHERE strAccountID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	memset(strItem, 0x00, sizeof(strItem));
	memset(strSerial, 0x00, sizeof(strSerial));
	memset(strItemExpiration, 0x00, sizeof(strItemExpiration));

	dbCommand->FetchUInt32(1, pUser->m_iBank);
	dbCommand->FetchBinary(2, strItem, sizeof(strItem));
	dbCommand->FetchBinary(3, strSerial, sizeof(strSerial));
	dbCommand->FetchBinary(4, strItemExpiration, sizeof(strItemExpiration));

	// FIX: Bank money overflow protection on load
	if (pUser->m_iBank > COIN_MAX)
		pUser->m_iBank = COIN_MAX;

	ByteBuffer ItemBuffer, SerialBuffer, ItemTimeBuffer;
	ItemBuffer.append(strItem, sizeof(strItem));
	SerialBuffer.append(strSerial, sizeof(strSerial));
	ItemTimeBuffer.append(strItemExpiration, sizeof(strItemExpiration));

	memset(pUser->m_sWarehouseArray, 0x00, sizeof(pUser->m_sWarehouseArray));

	for (int i = 0; i < WAREHOUSE_MAX; i++) 
	{
		uint64 nSerialNum;
		uint32 nItemID, nItemTime;
		int16 sDurability, sCount;

		ItemBuffer >> nItemID >> sDurability >> sCount;
		SerialBuffer >> nSerialNum;
		ItemTimeBuffer >> nItemTime;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull() || sCount <= 0)
			continue;

		if (!pTable.m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		pUser->m_sWarehouseArray[i].nNum = nItemID;
		pUser->m_sWarehouseArray[i].sDuration = sDurability;
		pUser->m_sWarehouseArray[i].sCount = sCount;
		pUser->m_sWarehouseArray[i].nSerialNum = nSerialNum;
		pUser->m_sWarehouseArray[i].nExpirationTime = nItemTime;

		if ((pTable.m_bKind == 255 && !pTable.m_bCountable) || (!pTable.m_bCountable)) 
			pUser->m_sWarehouseArray[i].sCount = 1;

		UserItemSealMap::iterator sealitr = pUser->m_sealedItemMap.find(nSerialNum);
		if (sealitr != pUser->m_sealedItemMap.end()) {
			if (sealitr->second->bSealType == 1) pUser->m_sWarehouseArray[i].bFlag = (uint8)ItemFlag::ITEM_FLAG_SEALED;
			else if (sealitr->second->bSealType == 3) pUser->m_sWarehouseArray[i].bFlag = (uint8)ItemFlag::ITEM_FLAG_BOUND;
			else if (sealitr->second->bSealType == 4) pUser->m_sWarehouseArray[i].bFlag = (uint8)ItemFlag::ITEM_FLAG_NOT_BOUND;
			pUser->m_sWarehouseArray[i].oFlag = sealitr->second->prelockstate;
		}

		if (pUser->dupplicate_itemcheck(pUser->m_sWarehouseArray[i].nSerialNum))
		{
			auto pTable = g_pMain->GetItemPtr(pUser->m_sWarehouseArray[i].nNum);
			if (!pTable.isnull()) {
				printf("Dupplicate Item Warehouse: AccountName:%s - UserName:%s -  [Item Name:%s, ItemNum:%d, SerialNum:%I64d, ItemSlot:%d] \n",
					pUser->GetAccountName().c_str(), pUser->GetName().c_str(), pTable.m_sName.c_str(), pTable.Getnum(), pUser->m_sWarehouseArray[i].nSerialNum, i);
			}
			// FIX H8: Clear duplicate item slot instead of just flagging (consistent with inventory handling)
			memset(&pUser->m_sWarehouseArray[i], 0, sizeof(_ITEM_DATA));
		}
	}

	return true;
}

#pragma region CDBAgent::UpdateClanWarehouseData(string strAccountID, CUser *puser)
bool CDBAgent::UpdateClanWarehouseData(std::string strClanID,  CUser* pUser)
{
	if (pUser == nullptr || !pUser->isInClan())
		return false;

	CKnights* pKnights = g_pMain->GetClanPtr(pUser->GetClanID());
	if (strClanID.length() == 0 || !pKnights)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	// This *should* be padded like the database field is (unnecessarily), but I want to see how MSSQL responds.
	ByteBuffer itemBuffer, serialBuffer;
	for (int i = 0; i < WAREHOUSE_MAX; i++) {
		_ITEM_DATA *pItem = &pKnights->m_sClanWarehouseArray[i];  //pKnights
		if (pItem == nullptr) continue;
		itemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount/* << pItem->nExpirationTime*/; // item remaining infos
		serialBuffer << pItem->nSerialNum;
	}

	//***sorunsuz94*** 27.03.2020 clana para koyma fix 0 lan yer  pUser->m_ClaniBank eklendi
	// SQL injection fix: Gold ve dwTime artik parameterized query ile gonderiliyor
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pKnights->m_nMoney);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pUser->m_dwTime);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClanID.c_str(), strClanID.length());
	if (!dbCommand->Execute(_T("UPDATE KNIGHTS SET Gold=?, dwTime=?, WarehouseData=?, strSerial=? WHERE IDName=?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return true;
}
#pragma endregion

bool CDBAgent::LoadVipWarehouseData(string & strAccountID, CUser *pUser)
{
	char strItem[VIPWAREHOUSE_MAX * 8],
		strItemExpiration[VIPWAREHOUSE_MAX * 4],
		strSerial[VIPWAREHOUSE_MAX * 8];

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (pUser == nullptr
		|| pUser->m_bLogout)
		return false;

	pUser->m_bVIPStoragePassword = "";
	pUser->m_bVIPStorageVaultExpiration = 0;
	pUser->m_bWIPStorePassowrdRequest = 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("SELECT strPassword,strExpirationDate,WipPasswordRequest, VIPWarehouseData, strSerial, VIPWarehouseDataTime FROM VIP_WAREHOUSE WHERE strAccountID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	memset(strItem, 0x00, sizeof(strItem));
	memset(strSerial, 0x00, sizeof(strSerial));
	memset(strItemExpiration, 0x00, sizeof(strItemExpiration));

	dbCommand->FetchString(1, pUser->m_bVIPStoragePassword);
	dbCommand->FetchUInt32(2, pUser->m_bVIPStorageVaultExpiration);
	dbCommand->FetchByte(3, pUser->m_bWIPStorePassowrdRequest);
	dbCommand->FetchBinary(4, strItem, sizeof(strItem));
	dbCommand->FetchBinary(5, strSerial, sizeof(strSerial));
	dbCommand->FetchBinary(6, strItemExpiration, sizeof(strItemExpiration));

	ByteBuffer vItemBuffer, vSerialBuffer, vItemExBuffer;
	vItemBuffer.append(strItem, sizeof(strItem));
	vSerialBuffer.append(strSerial, sizeof(strSerial));
	vItemExBuffer.append(strItemExpiration, sizeof(strItemExpiration));

	memset(pUser->m_sVIPWarehouseArray, 0x00, sizeof(pUser->m_sVIPWarehouseArray));

	for (int i = 0; i < VIPWAREHOUSE_MAX; i++)
	{
		uint64 nSerialNum;
		uint32 nItemID, nItemEx;
		int16 sDurability, sCount;

		vItemBuffer >> nItemID >> sDurability >> sCount;
		vSerialBuffer >> nSerialNum;
		vItemExBuffer >> nItemEx;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull() || sCount <= 0)
			continue;

		if (!pTable.m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		pUser->m_sVIPWarehouseArray[i].nNum = nItemID;
		pUser->m_sVIPWarehouseArray[i].sDuration = sDurability;
		pUser->m_sVIPWarehouseArray[i].sCount = sCount;
		pUser->m_sVIPWarehouseArray[i].nSerialNum = nSerialNum;
		pUser->m_sVIPWarehouseArray[i].nExpirationTime = nItemEx;

		if ((pTable.m_bKind == 255 && !pTable.m_bCountable) || (!pTable.m_bCountable))
			pUser->m_sVIPWarehouseArray[i].sCount = 1;

		UserItemSealMap::iterator sealitr = pUser->m_sealedItemMap.find(nSerialNum);
		if (sealitr != pUser->m_sealedItemMap.end()) {
			if (sealitr->second->bSealType == 1) pUser->m_sVIPWarehouseArray[i].bFlag = (uint8)ItemFlag::ITEM_FLAG_SEALED;
			else if (sealitr->second->bSealType == 3) pUser->m_sVIPWarehouseArray[i].bFlag = (uint8)ItemFlag::ITEM_FLAG_BOUND;
			else if (sealitr->second->bSealType == 4) pUser->m_sVIPWarehouseArray[i].bFlag = (uint8)ItemFlag::ITEM_FLAG_NOT_BOUND;
			pUser->m_sVIPWarehouseArray[i].oFlag = sealitr->second->prelockstate;
		}

		if (pUser->dupplicate_itemcheck(pUser->m_sVIPWarehouseArray[i].nSerialNum))
		{
			auto pTable = g_pMain->GetItemPtr(pUser->m_sVIPWarehouseArray[i].nNum);
			if (!pTable.isnull()) {
				printf("Dupplicate Item WipWarehouse: AccountName:%s - UserName:%s -  [Item Name:%s, ItemNum:%d, SerialNum:%I64d, ItemSlot:%d] \n",
					pUser->GetAccountName().c_str(), pUser->GetName().c_str(), pTable.m_sName.c_str(), pTable.Getnum(), pUser->m_sVIPWarehouseArray[i].nSerialNum, i);
			}
			// FIX H8: Clear duplicate item slot instead of just flagging (consistent with inventory handling)
			memset(&pUser->m_sVIPWarehouseArray[i], 0, sizeof(_ITEM_DATA));
		}
	}
	return true;
}

bool CDBAgent::LoadPremiumServiceUser(string & strAccountID, CUser *pUser)
{
	if (pUser == nullptr) return false;

	std::unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("{CALL LOAD_PREMIUM_SERVICE_USER(?)}"))) {
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	char strPremium[30];
	memset(strPremium, 0, sizeof(strPremium));
	uint8 bPremiumCount = 0;

	dbCommand->FetchByte(1, pUser->m_bPremiumInUse);
	dbCommand->FetchByte(2, bPremiumCount);
	dbCommand->FetchBinary(3, strPremium, sizeof(strPremium));

	for (int i = 0, index = 0; i < bPremiumCount; i++, index += 5) {
		uint8	bPremiumType = *(uint8*)(strPremium + index);
		uint32	iPremiumTime = *(uint32*)(strPremium + index + 1);
		if (iPremiumTime < (uint32)UNIXTIME) continue;

		_PREMIUM_DATA* pPremium = new _PREMIUM_DATA;
		pPremium->bPremiumType = bPremiumType;
		pPremium->iPremiumTime = iPremiumTime;
		if (!pUser->m_PremiumMap.PutData(bPremiumType, pPremium))
			delete pPremium;
	}

	if (pUser->m_PremiumMap.GetData(pUser->m_bPremiumInUse) == nullptr) {
		pUser->m_bPremiumInUse = NO_PREMIUM;
		foreach_stlmap(itr, pUser->m_PremiumMap) {
			_PREMIUM_DATA* uPrem = itr->second;
			if (uPrem == nullptr || uPrem->iPremiumTime == 0) continue;
			pUser->m_bPremiumInUse = uPrem->bPremiumType;
			break;
		}
	}

	// this is hardcoded because we don't really care about the other mode
	if (pUser->m_bPremiumInUse != NO_PREMIUM)
		pUser->m_bAccountStatus = 1; // normal premium with expiry time
	else
		pUser->m_bAccountStatus = 0;

	return true;
}

#pragma region CDBAgent::LoadSavedMagic(CUser *pUser)

bool CDBAgent::LoadSavedMagic(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	if (!dbCommand->Execute(_T("SELECT "
		"nSkill1, nDuring1, nSkill2, nDuring2, nSkill3, nDuring3, nSkill4, nDuring4, nSkill5, nDuring5, "
		"nSkill6, nDuring6, nSkill7, nDuring7, nSkill8, nDuring8, nSkill9, nDuring9, nSkill10, nDuring10 "
		"FROM USER_SAVED_MAGIC WHERE strCharID = ?")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}


	if (!dbCommand->hasData())
		return false;

	Guard lock(pUser->m_savedMagicLock);
	pUser->m_savedMagicMap.clear();
	for (int i = 1; i <= 20; i += 2) {
		uint32 nSkillID = 0, nExpiry = 0;
		dbCommand->FetchUInt32(i, nSkillID);
		dbCommand->FetchUInt32(i + 1, nExpiry);
		if (nSkillID > 0 && nExpiry > 5 && nExpiry < 28800)
			pUser->m_savedMagicMap[nSkillID] = (static_cast<unsigned long long>(nExpiry) * 1000 + UNIXTIME2);
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::UpdateSavedMagic(CUser *pUser)
bool CDBAgent::UpdateSavedMagic(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;
	
	std::lock_guard<std::recursive_mutex> lock(pUser->m_savedMagicLock);
	uint32 nSkillID[10] = { 0 }, tExpiryTime[10] = { 0 }, i = 0;
	foreach(itr, pUser->m_savedMagicMap) {
		if (!itr->second) continue;
		int32 sctime = 0;
		if (itr->first > 0 && (itr->second - UNIXTIME2 > 1000)) sctime = (uint32)((itr->second - UNIXTIME2) / 1000);
		if (itr->first > 0 && sctime > 0) { nSkillID[i] = itr->first; tExpiryTime[i] = sctime; }
		if (++i >= 10) break;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_SAVED_MAGIC(?, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"),
		nSkillID[0], tExpiryTime[0], nSkillID[1], tExpiryTime[1], nSkillID[2], tExpiryTime[2], nSkillID[3], tExpiryTime[3], nSkillID[4], tExpiryTime[4],
		nSkillID[5], tExpiryTime[5], nSkillID[6], tExpiryTime[6], nSkillID[7], tExpiryTime[7], nSkillID[8], tExpiryTime[8], nSkillID[9], tExpiryTime[9]))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}
#pragma endregion

bool CDBAgent::SetLogInInfo(string & strAccountID, string & strCharID, string & strServerIP, short sServerNo, string & strClientIP, uint8 bInit)
{
	uint8 result = 0;
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &result);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strServerIP.c_str(), strServerIP.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClientIP.c_str(), strClientIP.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL SET_LOGIN_INFO(?, ?, %d, ?, ?, %d)}"), sServerNo, bInit)))
		ReportSQLError(m_AccountDB->GetError());

	return (bool)(result == 0 ? false : true);
}

#pragma region CDBAgent::RemoveCurrentUser(string & strAccountID)

bool CDBAgent::RemoveCurrentUser(string & strAccountID)
{

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("DELETE FROM CURRENTUSER WHERE strAccountID = ?")))
	{
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}

	return true;
}
#pragma endregion
#pragma region CDBAgent::InsertCurrentUser(string & strAccountID)
void CDBAgent::InsertCheatLog(CUser * pUser)
{

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetRemoteIP().c_str(), pUser->GetRemoteIP().length());
	// NOTE: Cross-database reference - if log DB name changes, update here
	if (!dbCommand->Execute(_T("insert into KO_LOG.dbo.LOGS_CHEAT (strAccountID,strCharID,strClientIP,strCheatLog) values (?,?,?,'Cheat Decated BullCode !')")))
	{
		ReportSQLError(m_AccountDB->GetError());
		return;
	}

	return;
}
#pragma endregion
#pragma region CDBAgent::InsertCurrentUser(string & strAccountID)
bool CDBAgent::InsertCurrentUser(string & strAccountID, string & strCharID, const string & strServerIP /*= "127.0.0.1"*/, const string & strClientIP /*= "127.0.0.2"*/)
{
	RemoveCurrentUser(strAccountID);

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strServerIP.c_str(), strServerIP.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClientIP.c_str(), strClientIP.length());
	// CHI-AUDIT: Hardcoded IP fix — now uses actual server/client IPs
	if (!dbCommand->Execute(_T("INSERT INTO CURRENTUSER VALUES (?,?,1,?,?)")))
	{
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}

	return true;
}
#pragma endregion

void CDBAgent::DestroyPusSessions(CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr || pUser == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strAccountID.c_str(), pUser->m_strAccountID.length());

	// TODO: Add an arg for the free slot count so we only need to pull/delete what we can hold.
	if (!dbCommand->Execute(string_format(_T("DELETE FROM PUS_SESSION WHERE [strAccountID] = ?"))))
	{
		ReportSQLError(m_AccountDB->GetError());
		return;
	}

	return;
}

bool CDBAgent::CreatePusSession(uint16 sFreeCount, CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr || pUser == nullptr)
		return false;

	std::string strClass = "~Unknown~";

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strAccountID.c_str(), pUser->m_strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClass.c_str(), strClass.length());


	// TODO: Add an arg for the free slot count so we only need to pull/delete what we can hold.
	if (!dbCommand->Execute(string_format(_T("INSERT INTO PUS_SESSION([strAccountID],[strUserID],[strClass],[strLevel],[strParam],[strData],[FreeSlotCount],[SessionKey],[CreatedTime]) VALUES (?, ?, ?, %d, %d, '', GetDate())"), pUser->GetLevel(), sFreeCount)))
	{
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}

	return true;
}



bool CDBAgent::LoadWebItemMall(std::vector<_ITEM_DATA> & itemList, CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());

	// TODO: Add an arg for the free slot count so we only need to pull/delete what we can hold.
	if (!dbCommand->Execute(_T("{CALL LOAD_WEB_ITEMMALL(?)}")))
		ReportSQLError(GetGameDB()->GetError());

	if (dbCommand->hasData())
	{
		do
		{
			_ITEM_DATA item;
			dbCommand->FetchUInt32(2, item.nNum); // 1 is the account name, which we don't need to use unless we're logging	
			dbCommand->FetchUInt16(3, item.sCount);
			dbCommand->FetchUInt32(5, item.nExpirationTime);
			itemList.push_back(item);
		} while (dbCommand->MoveNext());
	}

	return true;
}

#pragma region CDBAgent::LoadSkillShortcut(Packet & result, CUser *pUser)

bool CDBAgent::LoadSkillShortcut(Packet & result, CUser *pUser)
{
	if (pUser == nullptr) return false;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;


	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(_T("SELECT nCount, strSkillData FROM USERDATA_SKILLSHORTCUT WHERE strCharID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	if (!dbCommand->hasData()) return false;

	int field = 1;
	dbCommand->FetchUInt16(field++, pUser->m_strSkillCount);
	dbCommand->FetchBinary(field++, (char*)pUser->m_strSkillData, sizeof(pUser->m_strSkillData));

	result << pUser->m_strSkillCount;
	for (uint32 i = 0; i < pUser->m_strSkillCount; i++) result << *(uint32*)(pUser->m_strSkillData + (i * sizeof(uint32)));
	return true;
}
#pragma endregion

#pragma region CDBAgent::SaveSkillShortcut(CUser *pUser)
void CDBAgent::SaveSkillShortcut(CUser* pUser)
{
	if (pUser == nullptr) return;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char*)pUser->m_strSkillData, sizeof(pUser->m_strSkillData), SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{CALL SKILLSHORTCUT_SAVE(?,%d,?)}"), pUser->m_strSkillCount)))
		ReportSQLError(GetGameDB()->GetError());
}
#pragma endregion

bool CDBAgent::UpdateClanPremiumServiceUser(CUser* pUser)
{
	if (pUser == nullptr)
		return false;

	CKnights* pKnights = g_pMain->GetClanPtr(pUser->GetClanID());
	if (pKnights == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(string_format(_T("{CALL SAVE_PREMIUM_SERVICE_CLAN(%d, %d, %d)}"),
		pKnights->GetID(), pKnights->sPremiumInUse, pKnights->sPremiumTime))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}

bool CDBAgent::UpdatePremiumServiceUser(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	int8 bRet = -2; // generic error

	char strPremium[30];
	int index = 0;
	memset(strPremium, 0, sizeof(strPremium));
	int counter = 0;

	uint32 time = 0;
	foreach_stlmap(itr, pUser->m_PremiumMap)
	{
		if (itr->second == nullptr
			|| itr->second->iPremiumTime < UNIXTIME)
			continue;

		if (time < itr->second->iPremiumTime)
			time = itr->second->iPremiumTime;

		*(uint8 *)(strPremium + index) = itr->first;
		*(uint32 *)(strPremium + 1 + index) = itr->second->iPremiumTime;
		index += 5;
		counter++;
	}

	if (time > uint32(UNIXTIME))
		time = uint32(time - UNIXTIME); // total premium seconds left

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strPremium, sizeof(strPremium), SQL_BINARY);

	if (!dbCommand->Execute(string_format(_T("{? = CALL SAVE_PREMIUM_SERVICE_USER_V2(?, ?, %d, %d, %d)}"),
		counter, pUser->m_bPremiumInUse, time)))
		ReportSQLError(m_AccountDB->GetError());

	return true;
}

uint8 CDBAgent::SealItem(string strSealPasswd, uint64 nItemSerial, uint32 nItemID, uint8 bSealType, CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 3;

	uint8 bRet = 1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSealPasswd.c_str(), strSealPasswd.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL USER_ITEM_SEAL(?, ?, ?, " I64FMTD ", %d, %d)}"), nItemSerial, nItemID, bSealType)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

void CDBAgent::RequestFriendList(std::vector<string> & friendList, CUser *pUser)
{
	if (pUser == nullptr)
		return;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(_T("SELECT * FROM FRIEND_LIST WHERE strUserID = ?")))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return;

	string strCharID;
	for (int i = 2; i <= 25; i++)
	{
		if (dbCommand->FetchString(i, strCharID)
			&& strCharID.length())
			friendList.push_back(strCharID);
	}
}

FriendAddResult CDBAgent::AddFriend(short sid, short tid)
{
	CUser *pSrcUser = g_pMain->GetUserPtr(sid), *pTargetUser = g_pMain->GetUserPtr(tid);
	if (pSrcUser == nullptr)
		return FriendAddResult::FRIEND_ADD_ERROR;

	if (pTargetUser == nullptr)
	{
		CBot * pTargetUser = g_pMain->m_BotArray.GetData(tid);

		if (pTargetUser == nullptr)
			return FriendAddResult::FRIEND_ADD_ERROR;

		unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
		if (dbCommand.get() == nullptr)
			return FriendAddResult::FRIEND_ADD_ERROR;

		int16 sRet = (int16)FriendAddResult::FRIEND_ADD_ERROR;

		dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
		dbCommand->AddParameter(SQL_PARAM_INPUT, pSrcUser->GetName().c_str(), pSrcUser->GetName().length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, pTargetUser->GetName().c_str(), pTargetUser->GetName().length());

		if (!dbCommand->Execute(_T("{? = CALL INSERT_FRIEND_LIST(?, ?)}")))
			ReportSQLError(GetGameDB()->GetError());

		return (FriendAddResult)sRet;
	}

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return FriendAddResult::FRIEND_ADD_ERROR;

	int16 sRet = (int16)FriendAddResult::FRIEND_ADD_ERROR;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pSrcUser->GetName().c_str(), pSrcUser->GetName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pTargetUser->GetName().c_str(), pTargetUser->GetName().length());

	if (!dbCommand->Execute(_T("{? = CALL INSERT_FRIEND_LIST(?, ?)}")))
		ReportSQLError(GetGameDB()->GetError());

	return (FriendAddResult)sRet;
}

FriendRemoveResult CDBAgent::RemoveFriend(string & strCharID, CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return FriendRemoveResult::FRIEND_REMOVE_ERROR;

	int16 sRet = (int16)FriendRemoveResult::FRIEND_REMOVE_ERROR;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(_T("{? = CALL DELETE_FRIEND_LIST(?, ?)}")))
		ReportSQLError(GetGameDB()->GetError());

	return (FriendRemoveResult)sRet;
}

bool CDBAgent::UpdateUser(string & strCharID, UserUpdateType type, CUser *pUser)
{
	if (pUser == nullptr || strCharID != pUser->GetName())
		return false;

	if (pUser->pCindWar.isEventUser()/* && g_pMain->isCindirellaZone(pUser->GetZoneID())*/)
		return true;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (type == UserUpdateType::UPDATE_PACKET_SAVE)
		pUser->m_dwTime++;
	else if (type == UserUpdateType::UPDATE_LOGOUT
		|| type == UserUpdateType::UPDATE_ALL_SAVE)
		pUser->m_dwTime = 0;

	ByteBuffer ItemBuffer, SerialBuffer, ItemTimeBuffer;
	ByteBuffer strLevelBuffer;
	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = pUser->GetItem(i);
		if (!pItem) 
			continue;
		
		ItemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		SerialBuffer << pItem->nSerialNum;
		ItemTimeBuffer << pItem->nExpirationTime;
	}

	for (int i = 0; i < 83; i++)
		strLevelBuffer << pUser->m_sLevelArray[i];

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)ItemBuffer.contents(), ItemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)SerialBuffer.contents(), SerialBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)ItemTimeBuffer.contents(), ItemTimeBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strLevelBuffer.contents(), strLevelBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->mytagname.c_str(), pUser->mytagname.length());
	
	if(type == UserUpdateType::UPDATE_LOGOUT)
	{
		if(pUser->isInMoradon())
			pUser->m_bZone = ZONE_MORADON;
		else if(pUser->isInLufersonCastle())
			pUser->m_bZone = ZONE_KARUS;
		else if(pUser->isInElmoradCastle())
			pUser->m_bZone = ZONE_ELMORAD;
		
		if (pUser->isInKarusEslant()) 
			pUser->m_bZone = ZONE_KARUS_ESLANT;
		else if (pUser->isInElmoradEslant()) 
			pUser->m_bZone = ZONE_ELMORAD_ESLANT;
	}

	uint32 nDonatedNP = 0;
	CKnights * pKnights = g_pMain->GetClanPtr(pUser->GetClanID());
	if (pKnights != nullptr)
	{
		std:: string userid = pUser->GetName();
		STRTOUPPER(userid);
		_KNIGHTS_USER * pKnightUser = pKnights->m_arKnightsUser.GetData(userid);
		if(pKnightUser != nullptr)
		{
			pKnightUser->bFame = pUser->GetFame();
			pKnightUser->bLevel = pUser->GetLevel();
			pKnightUser->sClass = pUser->GetClass();
			pKnightUser->nLastLogin = int32(UNIXTIME);
			pKnightUser->LoyaltyMonthly = pUser->m_iLoyaltyMonthly;
			nDonatedNP = pKnightUser->nDonatedNP;
		}
	}

	if(pUser->isKing())
	{
		CKingSystem * pKingSystem = g_pMain->m_KingSystemArray.GetData(pUser->GetNation());
		if (pKingSystem != nullptr)
			pKingSystem->m_sKingClanID = pUser->GetClanID();
	}

	// FIX: Gold overflow protection on save
	if (pUser->m_iGold > COIN_MAX)
		pUser->m_iGold = COIN_MAX;

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_DATA("
		"?, "								// strCharID 
		"%d, %d, %d, %d, %d, "				// nation, race, class, hair, rank
		"%d, %d, %d, " I64FMTD ", %d, %d, "	// title, level, levelreb, exp, loyalty, face
		"%d, %d, %d, "						// city, knights, fame
		"%d, %d, %d, "						// hp, mp, sp
		"%d, %d, %d, %d, %d, "				// str, sta, dex, int, cha
		"%d, %d, %d, %d, %d, "				// rebstr, rebsta, rebdex, rebint, rebcha
		"%d, %d, %d, %d, %d, "				// authority, free points, gold, zone, bind
		"%d, %d, %d, %d, "					// x, z, y, dwTime
		"?, ?, ?, ?, ?, "					// strSkill, strItem, strSerial, strItemExpiration, strLevelCheck
		"%d, %d, "							// manner points, monthly NP
		"%d, %d, %d, %d, ?, %d, %d, "		// LastLogin, nDonatedNP,m_mutetimestatus,m_attacktimestatus,mytagname,chickenstatus,m_tagnamergb
		"%d, %d, %d )}"),						// flashtime, flashcount,flashtype
		pUser->m_bNation, pUser->m_bRace, pUser->m_sClass, pUser->m_nHair, pUser->m_bRank,
		pUser->m_bTitle, pUser->m_bLevel, pUser->m_bRebirthLevel, pUser->m_iExp /* temp hack, database needs to support it */, pUser->m_iLoyalty, pUser->m_bFace,
		pUser->m_bCity, pUser->m_bKnights, pUser->m_bFame,
		pUser->m_sHp, pUser->m_sMp, pUser->m_sSp,
		pUser->m_bStats[(uint8)StatType::STAT_STR], pUser->m_bStats[(uint8)StatType::STAT_STA], pUser->m_bStats[(uint8)StatType::STAT_DEX], pUser->m_bStats[(uint8)StatType::STAT_INT], pUser->m_bStats[(uint8)StatType::STAT_CHA],
		pUser->m_bRebStats[(uint8)StatType::STAT_STR], pUser->m_bRebStats[(uint8)StatType::STAT_STA], pUser->m_bRebStats[(uint8)StatType::STAT_DEX], pUser->m_bRebStats[(uint8)StatType::STAT_INT], pUser->m_bRebStats[(uint8)StatType::STAT_CHA],
		pUser->m_bAuthority, pUser->m_sPoints, pUser->m_iGold, pUser->m_bZone, pUser->m_sBind,
		(int)(pUser->m_curx * 100), (int)(pUser->m_curz * 100), (int)(pUser->m_cury * 100), pUser->m_dwTime,
		pUser->m_iMannerPoint, pUser->m_iLoyaltyMonthly,
		int32(UNIXTIME), nDonatedNP, pUser->m_mutetimestatus, pUser->m_attacktimestatus, pUser->ChickenStatus, pUser->m_tagnamergb,
		pUser->m_flashtime, pUser->m_flashcount, pUser->m_flashtype)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	pUser->m_lastSaveTime = UNIXTIME;
	pUser->m_lastBonusTime = UNIXTIME;
	return true;
}

bool CDBAgent::UpdateWarehouseData(string & strAccountID, UserUpdateType type, CUser *pUser)
{
	if (strAccountID.length() == 0)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (type == UserUpdateType::UPDATE_LOGOUT
		|| type == UserUpdateType::UPDATE_ALL_SAVE)
		pUser->m_dwTime = 0;

	// This *should* be padded like the database field is (unnecessarily), but I want to see how MSSQL responds.
	ByteBuffer ItemBuffer, SerialBuffer, ItemTimeBuffer;
	for (int i = 0; i < WAREHOUSE_MAX; i++)
	{
		_ITEM_DATA *pItem = &pUser->m_sWarehouseArray[i];
		ItemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		SerialBuffer << pItem->nSerialNum;
		ItemTimeBuffer << pItem->nExpirationTime;
	}

	// FIX: Bank overflow protection on save
	if (pUser->m_iBank > COIN_MAX)
		pUser->m_iBank = COIN_MAX;

	// SQL injection fix: nMoney ve dwTime artik parameterized query ile gonderiliyor
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pUser->m_iBank);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pUser->m_dwTime);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)ItemBuffer.contents(), ItemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)SerialBuffer.contents(), SerialBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)ItemTimeBuffer.contents(), ItemTimeBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("UPDATE WAREHOUSE SET nMoney=?, dwTime=?, WarehouseData=?, strSerial=?, WarehouseDataTime=? WHERE strAccountID=?")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}

bool CDBAgent::UpdateVipWarehouseData(string & strAccountID, UserUpdateType type, CUser *pUser)
{
	if (strAccountID.length() == 0)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (type == UserUpdateType::UPDATE_LOGOUT
		|| type == UserUpdateType::UPDATE_ALL_SAVE)
		pUser->m_dwTime = 0;

	// This *should* be padded like the database field is (unnecessarily), but I want to see how MSSQL responds.
	ByteBuffer vItemBuffer, vSerialBuffer, vItemExBuffer;

	for (int i = 0; i < VIPWAREHOUSE_MAX; i++) {
		_ITEM_DATA *pItem = &pUser->m_sVIPWarehouseArray[i];
		vItemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		vSerialBuffer << pItem->nSerialNum;
		vItemExBuffer << pItem->nExpirationTime;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)vItemBuffer.contents(), vItemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)vSerialBuffer.contents(), vSerialBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)vItemExBuffer.contents(), vItemExBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(string_format(_T("UPDATE VIP_WAREHOUSE SET VIPWarehouseData=?, strSerial=?, VIPWarehouseDataTime=?  WHERE strAccountID=?"))))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}

#pragma region CDBAgent::UpdateUserAuthority(std::string GameMaster, string strUserID, uint32 iPeriod, uint8 banType, string reason, CUser *pUser/* = nullptr*/)
void CDBAgent::UpdateUserAuthority(std::string GameMaster, string strUserID, uint32 iPeriod, uint8 banType, string reason, CUser *pUser/* = nullptr*/)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return;
	
	if (iPeriod > 365) iPeriod = 365;
	uint32 day_period = iPeriod;
	if (iPeriod) iPeriod = (uint32)UNIXTIME + (60 * 60 * 24 * iPeriod);

	int sRet = -1; if (reason.empty())reason = "-";
	std::string strRealUserID = "", strAccountID = "";
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, GameMaster.c_str(), GameMaster.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, reason.c_str(), reason.length());
	if (!dbCommand->Execute(string_format(_T("{CALL AUTHORITY_CHANGE(?, ?, ?, %d, %d)}"), iPeriod, banType))) {
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData()) { return; }

	int field = 1;
	dbCommand->FetchInt32(field++, sRet);
	dbCommand->FetchString(field++, strRealUserID);
	dbCommand->FetchString(field++, strAccountID);

	if (sRet == 0) {
		std::string sNoticeMessage = "";

		CUser* pUserAccount = g_pMain->GetUserPtr(strAccountID, NameType::TYPE_ACCOUNT);
		CUser* pUserCharacter = g_pMain->GetUserPtr(strRealUserID, NameType::TYPE_CHARACTER);

		switch ((BanTypes)banType)
		{
		case BanTypes::BANNED:
			if (pUserAccount != nullptr) 
				pUserAccount->Disconnect();

			sNoticeMessage = string_format("%s is currently blocked for using illegal software.", strRealUserID.c_str());
			break;
		case BanTypes::MUTE:
			if (pUserCharacter != nullptr) {
				if (day_period)
					pUserCharacter->m_mutetimestatus = iPeriod;
				else 
					pUserCharacter->m_mutetimestatus = -1;

				sNoticeMessage = string_format("%s has been muted for %s days.", strRealUserID.c_str(), (day_period > 0 ? std::to_string(day_period).c_str() : "forever"));
			}
			break;
		case BanTypes::UNMUTE:
			if (pUserCharacter != nullptr) 
				pUserCharacter->m_mutetimestatus = 0;
			break;
		case BanTypes::DIS_ATTACK:
			if (pUserCharacter != nullptr) {
				if (iPeriod)
					pUserCharacter->m_attacktimestatus = iPeriod;
				else 
					pUserCharacter->m_attacktimestatus = -1;
			}
			break;
		case BanTypes::ALLOW_ATTACK:
			if (pUserCharacter != nullptr) 
				pUserCharacter->m_attacktimestatus = 0;
			break;
		}
		if (!sNoticeMessage.empty()) g_pMain->SendNotice(sNoticeMessage.c_str(), (uint8)Nation::ALL);
		return;
	}

	if (sRet == 1) {
		if (pUser != nullptr) g_pMain->SendHelpDescription(pUser, "Invalid player name!");
		else printf("Invalid player name! \n");
		return;
	}

	if (sRet == 2) {
		if (pUser != nullptr) g_pMain->SendHelpDescription(pUser, "You cannot change a GM's authority!");
		else printf("You cannot change a GM's authority! \n");
		return;
	}

	if (sRet == 3) {
		if (pUser != nullptr) g_pMain->SendHelpDescription(pUser, "No operation was made since user already is of that authority!");
		else printf("No operation was made since user already is of that authority! \n");
		return;
	}

	if (sRet == 6) {
		if (pUser != nullptr) g_pMain->SendHelpDescription(pUser, "Some error has occured during database process. Please try again!");
		else printf("Some error has occured during database process. Please try again! \n");
		return;
	}
}
#pragma endregion

#pragma region Enhanced Ban System - CheckBanStatus / AddBanToDB / RemoveBanFromDB

int8 CDBAgent::CheckBanStatus(std::string & strAccountID, std::string & strIPAddress)
{
	int8 bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0; // DB hatasi durumunda girise izin ver (mevcut sistem zaten kontrol ediyor)

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strIPAddress.c_str(), strIPAddress.length());

	if (!dbCommand->Execute(_T("{? = CALL CHECK_BAN_STATUS(?, ?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}

	// ODBC FIX: SELECT resultset'i tuketmeden RETURN value output param'a yazilmiyor
	while (dbCommand->MoveNext()) {}

	return bRet;
}

int8 CDBAgent::AddBanToDB(std::string strAccountID, std::string strIPAddress, uint8 nBanType, std::string strReason, std::string strBannedBy, int nDurationMinutes)
{
	int8 bRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strIPAddress.c_str(), strIPAddress.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strReason.c_str(), strReason.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strBannedBy.c_str(), strBannedBy.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL ADD_BAN(?, ?, %d, ?, ?, %d)}"), nBanType, nDurationMinutes)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return -1;
	}

	while (dbCommand->MoveNext()) {}

	return bRet;
}

// S114 K3 FAZ 5: HWID Ban — KO_LOG.dbo.SP_HWID_BAN_BY_ACCOUNT
// Returns: 0=OK, 1=NO_LOG_FOUND, 2=ALREADY_BANNED, -1=DB error
int CDBAgent::HwidBanByAccount(const std::string& account, const std::string& reason, const std::string& gm)
{
	int result = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return -1;

	dbCommand->AddParameter(SQL_PARAM_INPUT, account.c_str(), account.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, reason.c_str(), reason.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, gm.c_str(), gm.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &result);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_HWID_BAN_BY_ACCOUNT(?, ?, ?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return -1;
	}
	while (dbCommand->MoveNext()) {}
	return result;
}

// S114 K3 FAZ 5: HWID Unban — KO_LOG.dbo.SP_HWID_UNBAN
// Returns: silinen satir sayisi (0=yoktu)
int CDBAgent::HwidUnban(const std::string& hwid)
{
	int rows = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return -1;

	dbCommand->AddParameter(SQL_PARAM_INPUT, hwid.c_str(), hwid.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &rows);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_HWID_UNBAN(?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return -1;
	}
	while (dbCommand->MoveNext()) {}
	return rows;
}

int8 CDBAgent::RemoveBanFromDB(std::string strAccountID, std::string strIPAddress, std::string strRemovedBy)
{
	int8 bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strIPAddress.c_str(), strIPAddress.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strRemovedBy.c_str(), strRemovedBy.length());

	if (!dbCommand->Execute(_T("{? = CALL REMOVE_BAN(?, ?, ?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return -1;
	}

	while (dbCommand->MoveNext()) {}

	return bRet;
}

#pragma endregion

uint8 CDBAgent::UpdateCharacterName(std::string & strAccountID, std::string & strUserID, std::string & strNewUserID)
{
	uint8 sRet = 1;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return sRet;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNewUserID.c_str(), strNewUserID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL CHANGE_NEW_ID(?, ?, ?)}")))) {
		ReportSQLError(GetGameDB()->GetError());
		return 1;
	}
	return sRet;
}

uint8 CDBAgent::UpdateCharacterClanName(std::string strClanID, std::string strNewClanID, CUser* puser)
{
	if (puser == nullptr
		|| puser->GetAccountName().empty()
		|| puser->GetAccountName().length() > MAX_ID_SIZE
		|| puser->GetName().empty()
		|| puser->GetName().length() > MAX_ID_SIZE)
		return (uint8)0;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)	return (uint8)0;

	uint8 sRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, puser->GetAccountName().c_str(), puser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, puser->GetName().c_str(), puser->GetName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClanID.c_str(), strClanID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNewClanID.c_str(), strNewClanID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL CHANGE_NEW_CLANID(?, ?, ?, ?)}")))) {
		ReportSQLError(GetGameDB()->GetError());
		return (uint8)0;
	}
	return sRet;
}

void CDBAgent::UpdateBattleEvent(string & strCharID, uint8 bNation)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("UPDATE BATTLE SET byNation=%d, strUserName=? WHERE sIndex=%d"), bNation, g_pMain->m_nServerNo)))
		ReportSQLError(GetGameDB()->GetError());
}

void CDBAgent::AccountLogout(string & strAccountID)
{
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("{CALL ACCOUNT_LOGOUT(?)}")))
		ReportSQLError(m_AccountDB->GetError());
}

void CDBAgent::UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount)
{
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("UPDATE CONCURRENT SET zone%d_count = %d WHERE serverid = %d"), nZoneNo, nCount, nServerNo)))
		ReportSQLError(m_AccountDB->GetError());
}

// This is what everything says it should do, 
// but the client doesn't seem to care if it's over 1
uint8 CDBAgent::GetUnreadLetterCount(string & strCharID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	uint8 bCount = 0;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bCount);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(_T("{? = CALL MAIL_BOX_CHECK_COUNT(?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}

	return bCount;
}

bool CDBAgent::GetLetterList(string & strCharID, Packet & result, bool bNewLettersOnly /* = true*/)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	int8 bCount = 0;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bCount);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL MAIL_BOX_REQUEST_LIST(?, %d)}"), bNewLettersOnly)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	result << uint8(1);
	int offset = (int) result.wpos();
	result << bCount; // placeholder for count

	if (!dbCommand->hasData())
		return true;

	result.SByte();
	do
	{
		string strSubject, strSender;
		uint32 nLetterID, nItemID, nCoins, nDate;
		uint16 sCount, sDaysRemaining;
		uint8 bStatus, bType;

		dbCommand->FetchUInt32(1, nLetterID);
		dbCommand->FetchByte(2, bStatus);
		dbCommand->FetchByte(3, bType);
		dbCommand->FetchString(4, strSubject);
		dbCommand->FetchString(5, strSender);
		dbCommand->FetchByte(6, bType);
		dbCommand->FetchUInt32(7, nItemID);
		dbCommand->FetchUInt16(8, sCount);
		dbCommand->FetchUInt32(9, nCoins);
		dbCommand->FetchUInt32(10, nDate);
		dbCommand->FetchUInt16(11, sDaysRemaining); 

		result	<< nLetterID // letter ID
			<< bStatus  // letter status, doesn't seem to affect anything
			<< strSubject << strSender
			<< bType;	

		if (bType == 2)
			result	<< nItemID << sCount << nCoins;

		result	<< nDate // date (yy*10000 + mm*100 + dd)
			<< sDaysRemaining;

	} while (dbCommand->MoveNext());

	result.put(offset, bCount); // set count now that the result set's been read

	return true;
}

int8 CDBAgent::SendLetter(string & strSenderID, string & strRecipientID, string & strSubject, string & strMessage, uint8 bType, _ITEM_DATA * pItem, int32 nCoins)
{
	uint64 nSerialNum = 0;
	uint32 nItemID = 0, nItemExpiration = 0;
	uint16 sCount = 0, sDurability = 0;
	int8 bRet = 0;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	// This is a little bit redundant, but best to be sure.
	if (bType == 2 
		&& pItem != nullptr)
	{
		nItemID = pItem->nNum;
		sCount = pItem->sCount;
		sDurability = pItem->sDuration;
		nSerialNum = pItem->nSerialNum;
		nItemExpiration = pItem->nExpirationTime;
	}

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSenderID.c_str(), strSenderID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strRecipientID.c_str(), strRecipientID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSubject.c_str(), strSubject.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strMessage.c_str(), strMessage.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL MAIL_BOX_SEND(?, ?, ?, ?, %d, %d, %d, %d, " I64FMTD ",%d, %d)}"),
		bType, nItemID, sCount, sDurability, nSerialNum, nItemExpiration, nCoins))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}
	return bRet;
}

bool CDBAgent::ReadLetter(string & strCharID, uint32 nLetterID, string & strMessage)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL MAIL_BOX_READ(?, %d)}"), nLetterID)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchString(1, strMessage);
	return true;
}

int8 CDBAgent::GetItemFromLetter(string & strCharID, uint32 nLetterID, uint32 & nItemID, uint16 & sCount, uint16 & sDurability, uint32 & nCoins, uint64 & nSerialNum, uint32 &nExpiTime) 
{
	int8 bRet = -1; // error
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL MAIL_BOX_GET_ITEM(?, %d)}"), nLetterID)))
		ReportSQLError(GetGameDB()->GetError());

	if (dbCommand->hasData())
	{
		dbCommand->FetchUInt32(1, nItemID);
		dbCommand->FetchUInt16(2, sCount);
		dbCommand->FetchUInt16(3, sDurability);
		dbCommand->FetchUInt32(4, nCoins);
		dbCommand->FetchUInt64(5, nSerialNum);
		dbCommand->FetchUInt32(6, nExpiTime); 
	}

	return bRet;
}

void CDBAgent::DeleteLetter(string & strCharID, uint32 nLetterID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	// NOTE: The official implementation passes all 5 letter IDs.
	if (!dbCommand->Execute(string_format(_T("UPDATE MAIL_BOX SET bDeleted = 1 WHERE nLetterID = %d AND strRecipientID = ?"), nLetterID)))
		ReportSQLError(GetGameDB()->GetError());
}

#pragma region King System Related Functions

#pragma region CDBAgent::UpdateElectionStatus(uint8 byType, uint8 byNation)

/**
* @brief	Updates the election status.
*
* @param	byType  	Election status.
* @param	byNation	Electoral nation.
*/
void CDBAgent::UpdateElectionStatus(uint8 byType, uint8 byNation)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("{CALL KING_UPDATE_ELECTION_STATUS(%d, %d)}"), byType, byNation)))
		ReportSQLError(GetGameDB()->GetError());

	if(byType == ELECTION_TYPE_ELECTION)
	{
		CKingSystem * pKingSystem = g_pMain->m_KingSystemArray.GetData(byNation);
		if(pKingSystem != nullptr)
		{
			if (dbCommand->hasData())
			{
				dbCommand->FetchString(1, pKingSystem->m_strKingName);
				dbCommand->FetchUInt16(2, pKingSystem->m_sYear);
				dbCommand->FetchByte(3, pKingSystem->m_byMonth);
				dbCommand->FetchByte(4, pKingSystem->m_byDay);
				dbCommand->FetchByte(5, pKingSystem->m_byHour);
				dbCommand->FetchByte(6, pKingSystem->m_byMinute);

				CUser *pUser = g_pMain->GetUserPtr(pKingSystem->m_strKingName, NameType::TYPE_CHARACTER);

				if(pUser != nullptr && pUser->isInGame())
				{
					pUser->SendMyInfo();

					pUser->UserInOut(INOUT_OUT);
					pUser->SetRegion(pUser->GetNewRegionX(), pUser->GetNewRegionZ());
					pUser->UserInOut(INOUT_WARP);

					g_pMain->UserInOutForMe(pUser);
					pUser->NpcInOutForMe();
					g_pMain->MerchantUserInOutForMe(pUser);

					pUser->ResetWindows();

					pUser->InitType4();
					pUser->RecastSavedMagic();
				}
			}
		}
	}
}
#pragma endregion

#pragma region CDBAgent::UpdateElectionList(uint8 byDBType, uint8 byType, uint8 byNation, uint16 sKnights, uint32 nAmount, string & strNominee)

/**
* @brief	Updates the election list.
*
* @param	byDBType  	Procedure-specific database action.
* 						If 1, insert. If 2, delete.
* @param	byType	  	Flag to specify what the user's in the election list for (election, impeachment, and thereof).
* @param	byNation  	Electoral nation.
* @param	sKnights  	The nominee's clan ID.
* @param	nAmount		Vote count.
* @param	strNominee	The nominee's name.
*/
void CDBAgent::UpdateElectionList(uint8 byDBType, uint8 byType, uint8 byNation, uint16 sKnights, uint32 nAmount, string & strNominee)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strNominee.c_str(), strNominee.length());
	if (!dbCommand->Execute(string_format(_T("{CALL KING_UPDATE_ELECTION_LIST(%d, %d, %d, %d, %d, ?)}"), 
		byDBType, byType, byNation, sKnights, nAmount)))
		ReportSQLError(GetGameDB()->GetError());
}

#pragma endregion

#pragma region CDBAgent::UpdateCandidacyRecommend(std::string & strNominator, std::string & strNominee, uint8 byNation)

/**
* @brief	Nominates/recommends strNominee as King.
*
* @param	strNominator	The nominator.
* @param	strNominee  	The nominee.
* @param	byNation		Their nation.
*
* @return	.
*/
int16 CDBAgent::UpdateCandidacyRecommend(std::string & strNominator, std::string & strNominee, uint8 byNation)
{
	int16 sRet = -6;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNominator.c_str(), strNominator.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNominee.c_str(), strNominee.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KING_CANDIDACY_RECOMMEND(?, ?, %d)}"), byNation)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::UpdateElectionVoteList(uint8 byNation, std::string & strVoterAccountID, std::string & strVoterUserID, std::string & strNominee)
/**
* @brief	Updates the election vote list.
*
* @param	byNation  	Electoral nation.
* @param	strVoterAccountID  	Voter User Account ID
* @param	strVoterUserID		Voter User ID
* @param	strNominee	The nominee's name.
*/
int16 CDBAgent::UpdateElectionVoteList(uint8 byNation, std::string & strVoterAccountID, std::string & strVoterUserID, std::string & strNominee)
{
	int16 bRet = -6; // error
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strVoterAccountID.c_str(), strVoterAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strVoterUserID.c_str(), strVoterUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNominee.c_str(), strNominee.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KING_ELECTION_PROC(?, ?, %d, ?)}"), byNation)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

#pragma endregion

#pragma region CDBAgent::GetElectionResults(uint8 byNation)
/**
* @brief	Updates the election vote list.
*
* @param	byNation  	Electoral nation.
*/
void CDBAgent::GetElectionResults(uint8 byNation)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("{CALL KING_ELECTION_RESULTS(%d)}"), byNation)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	CKingSystem * pKingSystem = g_pMain->m_KingSystemArray.GetData(byNation);
	if (pKingSystem == nullptr)
		return;

	pKingSystem->m_bElectionUnderProgress = true;
	// All db processes are done.
	uint16 count = 0;
	do
	{
		string strUserID;
		uint8 bRank, bMonth;
		uint16 sClanID, sYear;
		uint32 kingvotes, totalvotes;
		// update the king system respectively.
		dbCommand->FetchString(1, strUserID);
		dbCommand->FetchByte(2, bRank);
		dbCommand->FetchUInt16(3, sClanID);
		dbCommand->FetchByte(4, bMonth);
		dbCommand->FetchUInt16(5, sYear);
		dbCommand->FetchUInt32(6, kingvotes);
		dbCommand->FetchUInt32(7, totalvotes);

		pKingSystem->m_byNextMonth = bMonth;
		pKingSystem->m_sNextYear = sYear;

		if(bRank == 2)
		{
			_KING_ELECTION_LIST * pEntry = new _KING_ELECTION_LIST;
			pEntry->sKnights = sClanID;
			pEntry->nVotes = 0;
			pKingSystem->m_senatorList.insert(make_pair(strUserID, pEntry));
		}
		else if ( bRank == 1) // this is the king!
		{
			pKingSystem->m_strNewKingName = strUserID;
			pKingSystem->m_sKingClanID = sClanID;
			pKingSystem->m_KingVotes = kingvotes;
			pKingSystem->m_TotalVotes = totalvotes;
		}
	} while (dbCommand->MoveNext());

	pKingSystem->m_bElectionUnderProgress = false;
}

#pragma endregion

#pragma region CDBAgent::UpdateCandidacyNoticeBoard(string & strCharID, uint8 byNation, string & strNotice)

/**
* @brief	Updates the candidacy notice board.
*
* @param	strCharID	Candidate's name.
* @param	byNation 	Candidate's nation.
* @param	strNotice	The notice.
*/
void CDBAgent::UpdateCandidacyNoticeBoard(string & strCharID, uint8 byNation, string & strNotice)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNotice.c_str(), strNotice.length());
	if (!dbCommand->Execute(string_format(_T("{CALL KING_CANDIDACY_NOTICE_BOARD_PROC(?, %d, ?)}"), byNation)))
		ReportSQLError(GetGameDB()->GetError());
}
#pragma endregion

#pragma region CDBAgent::UpdateNoahOrExpEvent(uint8 byType, uint8 byNation, uint8 byAmount, uint8 byDay, uint8 byHour, uint8 byMinute, uint16 sDuration, uint32 nCoins)

void CDBAgent::UpdateNoahOrExpEvent(uint8 byType, uint8 byNation, uint8 byAmount, uint8 byDay, uint8 byHour, uint8 byMinute, uint16 sDuration, uint32 nCoins)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("{CALL KING_UPDATE_NOAH_OR_EXP_EVENT(%d, %d, %d, %d, %d, %d, %d, %d)}"), 
		byType, byNation, byAmount, byDay, byHour, byMinute, sDuration, nCoins)))
		ReportSQLError(GetGameDB()->GetError());
}
#pragma endregion

#pragma region CDBAgent::UpdateKingSystemDB(uint8 byNation, uint32 nNationalTreasury, uint32 KingTax)

void CDBAgent::UpdateKingSystemDB(uint8 byNation, uint32 nNationalTreasury, uint32 KingTax)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	int16 sRet = -2;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KING_UPDATE_DATABASE(%d, %d, %d)}"), byNation, nNationalTreasury, KingTax)))
		ReportSQLError(GetGameDB()->GetError());
}

#pragma endregion

#pragma region CDBAgent::InsertPrizeEvent(uint8 byType, uint8 byNation, uint32 nCoins, std::string & strCharID)

void CDBAgent::InsertPrizeEvent(uint8 byType, uint8 byNation, uint32 nCoins, std::string & strCharID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	int16 sRet = -2;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KING_INSERT_PRIZE_EVENT(%d, %d, %d, ?)}"), 
		byType, byNation, nCoins)))
		ReportSQLError(GetGameDB()->GetError());
}

#pragma endregion

#pragma region CDBAgent::InsertTaxEvent(uint8 opcode, uint8 TerritoryTariff, uint8 Nation, uint32 TerritoryTax)
void CDBAgent::InsertTaxEvent(uint8 opcode, uint8 TerritoryTariff, uint8 Nation, uint32 TerritoryTax)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	int16 sRet = -2;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KING_CHANGE_TAX (%d, %d, %d, %d)}"),opcode, Nation, TerritoryTariff, TerritoryTax)))
		ReportSQLError(GetGameDB()->GetError());
}
#pragma endregion

#pragma region CDBAgent::UpdateNationIntro(uint32 iServerNo, uint8 nation, std::string & strKingName, std::string &strKingNotice)
void CDBAgent::UpdateNationIntro(uint32 iServerNo, uint8 nation, std::string & strKingName, std::string &strKingNotice)
{
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strKingName.c_str(), strKingName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strKingNotice.c_str(), strKingNotice.length());


	if(nation == 1)
	{
		if (!dbCommand->Execute(string_format(_T("UPDATE GAME_SERVER_LIST SET strKarusKingName = ?, strKarusNotice = ? WHERE sGroupID = %d"), iServerNo)))
		{
			ReportSQLError(m_AccountDB->GetError());
			return;
		}
	}
	else if(nation == 2)
	{
		if (!dbCommand->Execute(string_format(_T("UPDATE GAME_SERVER_LIST SET strElMoradKingName = ?, strElMoradNotice = ? WHERE sGroupID = %d"), iServerNo)))
		{
			ReportSQLError(m_AccountDB->GetError());
			return;
		}
	}
}

#pragma endregion

void CDBAgent::LoadUpgrade() {


	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;



	if (!dbCommand->Execute(_T("select nIndex,nOriginNumber,nNewNumber,nReqItem from NEW_UPGRADE")))
		ReportSQLError(GetGameDB()->GetError());

	g_pMain->m_loadUpgrade.clear();

	uint32 rCount = 1;
	uint32 nIndex;

	if (dbCommand->hasData())
	{
		do
		{
			_UPGRADE_LOAD* pEvent = new _UPGRADE_LOAD;
			dbCommand->FetchUInt32(1, nIndex);
			dbCommand->FetchUInt32(2, pEvent->ItemNumber);
			dbCommand->FetchUInt32(3, pEvent->NewItemNumber);
			dbCommand->FetchUInt32(4, pEvent->ScrollNumber);



			g_pMain->m_loadUpgrade.insert(std::make_pair(nIndex, pEvent));
			rCount++;


		} while (dbCommand->MoveNext());
	}


}


#pragma endregion
void CDBAgent::LoadItemUpgradeSettings()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;



	if (!dbCommand->Execute(_T("SELECT ReqItemID1,ReqItemID2,ItemType,ItemRate,ItemGrade,ItemReqCoins,SuccessRate FROM ITEM_UPGRADE_SETTINGS")))
		ReportSQLError(GetGameDB()->GetError());

	g_pMain->m_UpgradeSettings.clear();
	uint32 rCount = 1;


	if (dbCommand->hasData())
	{
		do
		{
			_UPGRADE_SETTINGS* pUpgrade = new _UPGRADE_SETTINGS;
			dbCommand->FetchUInt32(1, pUpgrade->nReqItem1);
			dbCommand->FetchUInt32(2, pUpgrade->nReqItem2);
			dbCommand->FetchByte(3, pUpgrade->ItemType);
			dbCommand->FetchByte(4, pUpgrade->ItemRate);
			dbCommand->FetchByte(5, pUpgrade->ItemGrade);
			dbCommand->FetchUInt32(6, pUpgrade->nReqCoins);
			dbCommand->FetchUInt32(7, pUpgrade->SuccesRate);


			


			g_pMain->m_UpgradeSettings.insert(std::make_pair(rCount, pUpgrade));
			rCount++;
			

		} while (dbCommand->MoveNext());
	}

}

void CDBAgent::LoadCollectionReward(uint16 sEventID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format("select nItemID,nItemCount,nItemTime,nRate, nItemSession from COLLECTION_RACE_EVENT_REWARD where EventID=%d", sEventID)))
		ReportSQLError(GetGameDB()->GetError());

	if (dbCommand->hasData())
	{
		int i = 0;
		do
		{
			if (i > 4) return;

			int field = 1;
			dbCommand->FetchUInt32(field++, g_pMain->pCollectionRaceEvent.RewardItemID[i]);
			dbCommand->FetchUInt32(field++, g_pMain->pCollectionRaceEvent.RewardItemCount[i]);
			dbCommand->FetchUInt32(field++, g_pMain->pCollectionRaceEvent.RewardItemTime[i]);
			dbCommand->FetchByte(field++, g_pMain->pCollectionRaceEvent.RewardItemRate[i]);
			dbCommand->FetchByte(field++, g_pMain->pCollectionRaceEvent.RewardSession[i]);

			if (g_pMain->pCollectionRaceEvent.RewardItemRate[i] > 100)
				g_pMain->pCollectionRaceEvent.RewardItemRate[i] = 100;

			i++;


		} while (dbCommand->MoveNext());
	}
}
void CDBAgent::SpecialStone()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(_T("SELECT nIndex,ZoneID,MainNPC,SummonNPC,MonsterName,SummonCount FROM K_SPECIAL_STONE where Status=1")))
		ReportSQLError(GetGameDB()->GetError());

	g_pMain->m_SpecialStonearray.clear();

	if (dbCommand->hasData())
	{
		do
		{
			SPECIAL_STONE* rStone = new SPECIAL_STONE;
			dbCommand->FetchUInt32(1, rStone->nIndex);
			dbCommand->FetchByte(2, rStone->ZoneID);
			dbCommand->FetchUInt16(3, rStone->MainNpcID);
			dbCommand->FetchUInt16(4, rStone->NpcID);
			dbCommand->FetchString(5, rStone->MonsterName);
			dbCommand->FetchByte(6, rStone->sCount);


			

			g_pMain->m_SpecialStonearray.insert(std::make_pair(rStone->nIndex, rStone));
			
		} while (dbCommand->MoveNext());
	}
	printf("Load K_SPECIAL Tables data size %d \n", (int)g_pMain->m_SpecialStonearray.size());
}
/**
* @brief	Resets the monthly NP total accumulated in the last month.
*/
void CDBAgent::ResetLoyaltyMonthly()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(_T("{CALL RESET_LOYALTY_MONTHLY}")))
		ReportSQLError(GetGameDB()->GetError());
}

/**
* @brief	Clears the remaining users who were connected to this server
from the logged in user list that may still be there as the 
result of an improper shutdown.
*/
void CDBAgent::ClearRemainUsers()
{
	_ZONE_SERVERINFO * pInfo = g_pMain->m_ServerArray.GetData(g_pMain->m_nServerNo);
	if (pInfo == nullptr)
		return;

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pInfo->strServerIP.c_str(), pInfo->strServerIP.length());
	if (!dbCommand->Execute(_T("{CALL CLEAR_REMAIN_USERS(?)}")))
		ReportSQLError(m_AccountDB->GetError());
}

void CDBAgent::InsertUserDailyOp(_USER_DAILY_OP * pUserDailyOp)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUserDailyOp->strUserId.c_str(), pUserDailyOp->strUserId.length());
	if (!dbCommand->Execute(string_format(_T("{CALL INSERT_USER_DAILY_OP(?, %d, %d, %d, %d, %d, %d, %d, %d)}"),
		pUserDailyOp->ChaosMapTime, pUserDailyOp->UserRankRewardTime, pUserDailyOp->PersonalRankRewardTime, pUserDailyOp->KingWingTime,
		pUserDailyOp->WarderKillerTime1, pUserDailyOp->WarderKillerTime2, pUserDailyOp->KeeperKillerTime, pUserDailyOp->UserLoyaltyWingRewardTime)))
		ReportSQLError(GetGameDB()->GetError());	
}

void CDBAgent::UpdateUserDailyOp(std::string strUserId, uint8 type, int32 sUnixTime)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserId.c_str(), strUserId.length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_DAILY_OP(?, %d, %d)}"), type, sUnixTime)))
		ReportSQLError(GetGameDB()->GetError());	
}

void CDBAgent::UpdateRanks()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(_T("{CALL UPDATE_RANKS}")))
		ReportSQLError(GetGameDB()->GetError());
}

int8 CDBAgent::NationTransfer(std::string strAccountID)
{
	int8 bRet = 3;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("{? = CALL ACCOUNT_NATION_TRANSFER(?)}")))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

bool CDBAgent::GetAllCharInfo(string & strCharID, uint16 & nNum, _NATION_DATA* m_NationTransfer)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(_T("SELECT USERDATA.strUserID, Race, Class, Face, HairRGB, Knights FROM USERDATA \
							   INNER JOIN USER_KNIGHTDATA ON USER_KNIGHTDATA.strUserID = USERDATA.strUserID \
							   WHERE USERDATA.strUserID = ?")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	string dCharName;
	uint8 dRace, bFace;
	uint16 dClass, sClanID;
	uint32 nHair;

	if (dbCommand->hasData())
	{
		dbCommand->FetchString(1, dCharName);
		dbCommand->FetchByte(2, dRace);
		dbCommand->FetchUInt16(3, dClass);
		dbCommand->FetchByte(4, bFace);
		dbCommand->FetchUInt32(5, nHair);
		dbCommand->FetchUInt16(6, sClanID);
	}
	else
		return false;

	m_NationTransfer->nNum = nNum;
	m_NationTransfer->bCharName = dCharName;
	m_NationTransfer->bRace = dRace;
	m_NationTransfer->sClass = dClass;
	m_NationTransfer->bFace = bFace;
	m_NationTransfer->nHair = nHair;
	m_NationTransfer->sClanID = sClanID;

	nNum++;
	return true;
}

bool CDBAgent::GetAllCharInfo(std::string strCharID, _NATION_DATA& m_NationTransfer)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(_T("SELECT strUserID, Race, Class, Face, HairRGB, Knights FROM USERDATA WHERE strUserID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData()) return false;

	std::string dCharName;
	uint8 dRace, bFace;
	uint16 dClass, sClanID;
	uint32 nHair;

	dbCommand->FetchString(1, dCharName);
	dbCommand->FetchByte(2, dRace);
	dbCommand->FetchUInt16(3, dClass);
	dbCommand->FetchByte(4, bFace);
	dbCommand->FetchUInt32(5, nHair);
	dbCommand->FetchUInt16(6, sClanID);

	m_NationTransfer.bCharName = dCharName;
	m_NationTransfer.bRace = dRace;
	m_NationTransfer.sClass = dClass;
	m_NationTransfer.bFace = bFace;
	m_NationTransfer.nHair = nHair;
	m_NationTransfer.sClanID = sClanID;
	return true;
}

int8 CDBAgent::NationTransferSaveUser(std::string strAccountID, std::string strCharID, uint16 nNum, uint8 bnewRace, uint16 bNewClass, uint8 bNewNation, uint8 bNewFace, uint32 iNewHair)
{
	int8 bRet = 3;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL NATION_TRANSFER_SAVEUSER(?, ?, %d, %d, %d, %d, %d, %d)}"),nNum, bnewRace, bNewClass, bNewNation, bNewFace, iNewHair)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

#pragma region CDBAgent::UpdateSiegeWarfareDB(uint32 nMoradonTax, uint32 nDelosTax, uint32 nDungeonCharge)

void CDBAgent::UpdateSiegeWarfareDB(uint32 nMoradonTax, uint32 nDelosTax, uint32 nDungeonCharge)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	int16 sRet = -2;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL UPDATE_SIEGE_DATABASE(%d, %d, %d)}"), nMoradonTax, nDelosTax, nDungeonCharge)))
		ReportSQLError(GetGameDB()->GetError());
}

void CDBAgent::UpdateSiege(int16 m_sCastleIndex, int16 m_sMasterKnights, int16 m_bySiegeType, int16 m_byWarDay, int16 m_byWarTime, int16 m_byWarMinute)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_SIEGE (%d, %d, %d, %d, %d, %d)}"), m_sCastleIndex, m_sMasterKnights, m_bySiegeType, m_byWarDay, m_byWarTime, m_byWarMinute)))
		ReportSQLError(GetGameDB()->GetError());	
}

void CDBAgent::UpdateSiegeTax(uint8 Zone, int16 ZoneTarrif)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return;

	if (Zone == ZONE_DELOS)
	{
		if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS_SIEGE_WARFARE SET sDellosTariff = %d"), ZoneTarrif)))
			ReportSQLError(GetGameDB()->GetError());
	}
	else if (Zone >= ZONE_MORADON && Zone <= ZONE_MORADON5)
	{
		if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS_SIEGE_WARFARE SET sMoradonTariff = %d"), ZoneTarrif)))
			ReportSQLError(GetGameDB()->GetError());
	}
	else if (Zone == 0)
	{
		if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS_SIEGE_WARFARE SET nDungeonCharge = 0, nMoradonTax = 0, nDellosTax = 0"))))
			ReportSQLError(GetGameDB()->GetError());
	}
}

void CDBAgent::UpdateSiegeRequestList(uint16 r1, uint16 r2, uint16 r3, uint16 r4, uint16 r5, uint16 r6, uint16 r7, uint16 r8, uint16 r9, uint16 r10)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS_SIEGE_WARFARE SET sRequestList_1=%d, sRequestList_2=%d, sRequestList_3=%d, sRequestList_4=%d, sRequestList_5=%d, sRequestList_6=%d, sRequestList_7=%d, sRequestList_8=%d, sRequestList_9=%d, sRequestList_10=%d"),
		r1, r2, r3, r4, r5, r6, r7, r8, r9, r10)))
		ReportSQLError(GetGameDB()->GetError());
}
#pragma endregion


uint16 CDBAgent::GetClanIDWithCharID(string & strCharID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(_T("SELECT Knights FROM USERDATA WHERE strUserId = ?")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}

	if (!dbCommand->hasData())
		return 0;

	uint16 sKnights = 0;
	dbCommand->FetchUInt16(1, sKnights);

	return sKnights;
}

void CDBAgent::LoadLadderRankList(Packet & result, uint8 bNation, uint8& count)
{
	if (bNation != 1 && bNation != 2) return;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("SELECT TOP(10)USERDATA.strUserID,  ISNULL(Knights.IDNum, 0 ), ISNULL(Knights.IDName, '' ), ISNULL(Knights.sMarkVersion, '' ) FROM USERDATA FULL OUTER JOIN Knights ON Knights.IDNum = USERDATA.Knights where USERDATA.strUserID IS NOT NULL and USERDATA.Nation = %d and USERDATA.Authority = 1 order by USERDATA.LoyaltyMonthly desc"), bNation)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	int i = 1101;
	do
	{
		uint16 sKnightsIndex, sMarkVersion;
		string strKnightsName, strChief;

		dbCommand->FetchString(1, strChief);
		dbCommand->FetchUInt16(2, sKnightsIndex);
		dbCommand->FetchString(3, strKnightsName);
		dbCommand->FetchUInt16(4, sMarkVersion);

		result << uint16(i++)
			<< strChief
			<< uint16(0)
			<< sKnightsIndex
			<< sMarkVersion
			<< strKnightsName
			<< uint16(0);
		count++;
		if (i >= 1110)
			break;
	} while (dbCommand->MoveNext());
}

void CDBAgent::LoadKnightsLeaderList(Packet & result, uint8 bNation, uint8& count)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(_T("SELECT IDNum, Chief, IDName, sMarkVersion, Nation FROM KNIGHTS WHERE Chief IN (SELECT strUserID FROM USERDATA WHERE Authority = 1) ORDER BY Flag DESC, ClanPointFund DESC, Points DESC")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	int i = 1201;
	do
	{
		uint16 sKnightsIndex, sMarkVersion;
		string strKnightsName, strChief;
		uint8 Nation;

		dbCommand->FetchUInt16(1, sKnightsIndex);
		dbCommand->FetchString(2, strChief);
		dbCommand->FetchString(3, strKnightsName);
		dbCommand->FetchUInt16(4, sMarkVersion);
		dbCommand->FetchByte(5, Nation);

		if (bNation != Nation) continue;

		result << uint16(i++)
			<< strChief
			<< uint16(0)
			<< sKnightsIndex
			<< sMarkVersion
			<< strKnightsName
			<< uint16(0);

		count++;
		if (i >= 1210)
			break;
	} while (dbCommand->MoveNext());
}

#pragma region Knights Related Database Functions
#include "Knights.h"
#include "KnightsManager.h"

#pragma region CDBAgent::LoadAllKnights(KnightsArray &m_KnightsArray)
/**
* @brief Loads all Knights data on server start.
* 
* @param m_KnightsArray	the Knights array used by the GameServerDLG.
*/
bool CDBAgent::LoadAllKnights(KnightsArray & m_KnightsArray)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(_T("{CALL KNIGHTS_LOAD}")))	
	{
		ReportSQLError(GetGameDB()->GetError());
		return false; 
	}

	if (!dbCommand->hasData())
		return true;

	do
	{
		int i = 1;
		CKnights *pData = new CKnights();
		char strItem[WAREHOUSE_MAX * 8], strSerial[WAREHOUSE_MAX * 8];//new
		dbCommand->FetchUInt16(i++, pData->m_sIndex);
		dbCommand->FetchByte(i++, pData->m_byFlag);
		dbCommand->FetchByte(i++, pData->m_byNation);
		dbCommand->FetchByte(i++, pData->m_byRanking);
		dbCommand->FetchString(i++, pData->m_strName);
		dbCommand->FetchUInt16(i++, pData->m_sDomination);
		dbCommand->FetchBinary(i++, pData->m_Image, sizeof(pData->m_Image));
		dbCommand->FetchUInt16(i++, pData->m_sMarkVersion);
		dbCommand->FetchUInt16(i++, pData->m_sMarkLen);
		dbCommand->FetchUInt16(i++, pData->m_sCape);
		dbCommand->FetchByte(i++, pData->m_bCapeR);
		dbCommand->FetchByte(i++, pData->m_bCapeG);
		dbCommand->FetchByte(i++, pData->m_bCapeB);
		dbCommand->FetchInt16(i++, pData->m_castCapeID);
		dbCommand->FetchByte(i++, pData->m_bCastCapeR);
		dbCommand->FetchByte(i++, pData->m_bCastCapeG);
		dbCommand->FetchByte(i++, pData->m_bCastCapeB);
		dbCommand->FetchUInt32(i++, pData->m_CastTime);
		dbCommand->FetchUInt16(i++, pData->m_sAlliance);
		dbCommand->FetchUInt32(i++, (uint32 &) pData->m_nClanPointFund);
		dbCommand->FetchString(i++, pData->m_strClanNotice);
		dbCommand->FetchByte(i++, pData->m_sClanPointMethod);
		dbCommand->FetchByte(i++, pData->bySiegeFlag);
		dbCommand->FetchUInt16(i++, pData->nLose);
		dbCommand->FetchUInt16(i++, pData->nVictory);
		dbCommand->FetchUInt64(i++, pData->m_nMoney);
		dbCommand->FetchInt32(i++, pData->m_dwTime);
		dbCommand->FetchBinary(i++, strItem, sizeof(strItem));
		dbCommand->FetchBinary(i++, strSerial, sizeof(strSerial));
		dbCommand->FetchUInt32(i++, pData->sPremiumTime);

		ByteBuffer itemBuffer, serialBuffer;
		itemBuffer.append(strItem, sizeof(strItem));
		serialBuffer.append(strSerial, sizeof(strSerial));

		memset(pData->m_sClanWarehouseArray, 0x00, sizeof(pData->m_sClanWarehouseArray));

		int serialcounter = 0;

		for (int i = 0; i < WAREHOUSE_MAX; i++)
		{
			uint64 nSerialNum;
			uint32 nItemID;
			int16 sDurability, sCount;

			itemBuffer >> nItemID >> sDurability >> sCount;
			serialBuffer >> nSerialNum;

			_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
			if (pTable.isnull() || sCount <= 0) continue;

			if (!pTable.m_bCountable && sCount > 1) sCount = 1;
			else if (sCount > ITEMCOUNT_MAX) sCount = ITEMCOUNT_MAX;

			pData->m_sClanWarehouseArray[i].nNum = nItemID;
			pData->m_sClanWarehouseArray[i].sDuration = sDurability;
			pData->m_sClanWarehouseArray[i].sCount = sCount;
			pData->m_sClanWarehouseArray[i].nSerialNum = nSerialNum;
			pData->m_sClanWarehouseArray[i].nExpirationTime = 0;

			/*if (pUser->dupplicate_itemcheck(pData->m_sClanWarehouseArray[i].nSerialNum))
			{
				pData->m_sClanWarehouseArray[i].bFlag = (uint8)ItemFlag::DUPLICATE;
				auto pTable = g_pMain->GetItemPtr(pData->m_sClanWarehouseArray[i].nNum);
				if (!pTable.isnull()) {
					printf("Dupplicate Item WipWarehouse: AccountName:%s - UserName:%s -  [Item Name:%s, ItemNum:%d, SerialNum:%I64d, ItemSlot:%d] \n",
						pUser->GetAccountName().c_str(), pUser->GetName().c_str(), pTable.m_sName.c_str(), pTable.Getnum(), pData->m_sClanWarehouseArray[i].nSerialNum, i);
				}
			}*/
		}

		if (pData->m_CastTime != 0 && (uint32)UNIXTIME < pData->m_CastTime)pData->castcape = true;
		if (!m_KnightsArray.PutData(pData->m_sIndex, pData))
			delete pData;



	} while (dbCommand->MoveNext());

	if (!dbCommand->Execute(_T("{CALL KNIGHTS_LOAD_MEMBERSDATA}")))	
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return true;

	uint16 sIDNum, sClass;
	std::string strUserID, strMemo, strAccountID;
	uint32 nDonatedNP, Loyalty, LoyaltyMonthly;
	uint8 fame, level;
	int32 lastLogin;
	do
	{
		int field = 1;
		dbCommand->FetchUInt16(field++, sIDNum);
		dbCommand->FetchString(field++, strAccountID);
		dbCommand->FetchString(field++, strUserID);
		dbCommand->FetchUInt32(field++, nDonatedNP);
		dbCommand->FetchString(field++, strMemo);
		dbCommand->FetchByte(field++, fame);
		dbCommand->FetchUInt16(field++, sClass);
		dbCommand->FetchByte(field++, level);
		dbCommand->FetchInt32(field++, lastLogin);
		dbCommand->FetchUInt32(field++, Loyalty);
		dbCommand->FetchUInt32(field++, LoyaltyMonthly);

		CKnightsManager::AddKnightsUser(sIDNum, strAccountID, strUserID,strMemo, fame, sClass, level, lastLogin, Loyalty, LoyaltyMonthly);
		CKnightsManager::AddUserDonatedNP(sIDNum, strUserID, nDonatedNP, false);

	} while (dbCommand->MoveNext());

	//CKnightsManager::GameLoadUpdateKnights();
	return true;
}
#pragma endregion

void CKnightsManager::GameLoadUpdateKnights()  // Clan ile ilgili t�m g�ncellemeler bu void i�ersinde olu�turulacak! OYUN A�ILIRKEN
{
	foreach_stlmap(itr, g_pMain->m_KnightsArray) {
		CKnights * pKnights = itr->second;
		if (pKnights == nullptr)
			continue;

		if (pKnights->m_byFlag < (uint8)ClanTypeFlag::ClanTypeAccredited5) {
			if (pKnights->m_byGrade > 3) {
				Packet reset(WIZ_DB_SAVE, uint8(ProcDbServerType::ResetKnights));
				reset << pKnights->GetID();
				g_pMain->AddDatabaseRequest(reset);
			}
		}
	}
}

void CDBAgent::KnightsReset(uint16 sClanID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;
	
	if (!dbCommand->Execute(string_format(_T("{CALL KNIGHTS_RESET(%d)}"), sClanID))) {
		ReportSQLError(GetGameDB()->GetError());
	}
}

#pragma region CDBAgent::CreateKnights(uint16 sClanID, uint8 bNation, string & strKnightsName, string & strChief, uint8 bFlag)

int8 CDBAgent::CreateKnights(uint16 sClanID, uint8 bNation, string & strKnightsName, string & strChief, uint8 bFlag)
{
	int8 bRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strKnightsName.c_str(), strKnightsName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strChief.c_str(), strChief.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL CREATE_KNIGHTS(%d, %d, %d, ?, ?)}"), sClanID, bNation, bFlag)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsMemberJoin(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberJoin(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_JOIN(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsMemberRemove(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberRemove(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_REMOVE(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsMemberLeave(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberLeave(string & strCharID, uint16 sClanID)
{
	uint8 sRet = 2;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_LEAVE(?, %d)}"), sClanID))) {
		ReportSQLError(GetGameDB()->GetError());
		return sRet;
	}

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsMemberAdmit(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberAdmit(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_ADMIT(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsMemberReject(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberReject(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_REJECT(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsMemberChief(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberChief(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_CHIEF(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsMemberViceChief(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberViceChief(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_VICECHIEF(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsMemberOfficer(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberOfficer(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_OFFICER(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsMemberPunish(string & strCharID, uint16 sClanID)

int CDBAgent::KnightsMemberPunish(string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_PUNISH(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsPointMethodChange(uint16 sClanID, uint8 bDomination)

int CDBAgent::KnightsPointMethodChange(uint16 sClanID, uint8 bDomination)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_POINT_METHOD_CHANGE(%d, %d)}"), sClanID, bDomination)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsDestroy(uint16 sClanID)

int CDBAgent::KnightsDestroy(uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_DESTROY (%d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsSymbolUpdate(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol)

int CDBAgent::KnightsSymbolUpdate(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol)
{
	int16 sRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, clanSymbol, MAX_KNIGHTS_MARK, SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_UPDATE_MARK(%d, ?, %d)}"), sClanID, sSymbolSize)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsCapeUpdate(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b)
/**
* @brief	Handles clan cape database updates.
*
* @param	sClanID	Identifier for the clan.
* @param	sCapeID	Identifier for the cape.
* @param	r 	Red colour component.
* @param	g 	Green colour component.
* @param	b 	Blue colour component.
*
* @return	true if it succeeds, false if it fails.
*/
int CDBAgent::KnightsCapeUpdate(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_UPDATE_CAPE (%d, %d, %d, %d, %d)}"), sClanID, sCapeID, r, g, b)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsCastellanCapeUpdate(uint8 type, uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b, uint32 time)
/**
* @brief	Handles clan cape database updates.
*
* @param	sClanID	Identifier for the clan.
* @param	sCapeID	Identifier for the cape.
* @param	r 	Red colour component.
* @param	g 	Green colour component.
* @param	b 	Blue colour component.
* @param	time 	time
*
* @return	true if it succeeds, false if it fails.
*/
int CDBAgent::KnightsCastellanCapeUpdate(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b, uint32 time)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_CASTELLAN_UPDATE_CAPE (%d, %d, %d, %d, %d, %d)}"), sClanID, sCapeID, r, g, b, time)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsHandover(std::string & strCharID, uint16 sClanID)

int CDBAgent::KnightsHandover(std::string & strCharID, uint16 sClanID)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_HANDOVER(?, %d)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsGradeUpdate(uint16 sClanID, uint8 byFlag, uint16 sCapeID)
/**
* @brief	Updates the clan grade.
*
* @param	sClanID	Identifier for the clan.
* @param	byFlag 	The clan type (training, promoted, etc).
* @param	sCapeID	Identifier for the cape.
*/
int CDBAgent::KnightsGradeUpdate(uint16 sClanID, uint8 byFlag, uint16 sCapeID)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_UPDATE_GRADE(%d, %d, %d)}"), sClanID, sCapeID, byFlag)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsClanNoticeUpdate(uint16 sClanID, std::string & strClanNotice)

/**
* @brief	Updates the clan notice.
*
* @param	sClanID		 	Identifier for the clan.
* @param	strClanNotice	The clan notice.
*/
int CDBAgent::KnightsClanNoticeUpdate(uint16 sClanID, std::string & strClanNotice)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (strClanNotice.empty())
	{
		if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS SET strClanNotice = NULL WHERE IDNum=%d"), sClanID)))
		{
			ReportSQLError(GetGameDB()->GetError());
			return sRet;
		}
		return 0;
	}
	else
	{
		dbCommand->AddParameter(SQL_PARAM_INPUT, strClanNotice.c_str(), strClanNotice.length());
		if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_UPDATE_CLANNOTICE(%d,?)}"), sClanID)))
			ReportSQLError(GetGameDB()->GetError());
	}
	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsAllianceNoticeUpdate(uint16 sClanID, std::string & strClanNotice)

/**
* @brief	Updates the clan notice.
*
* @param	sClanID		 	Identifier for the clan.
* @param	strClanNotice	The clan notice.
*/
int CDBAgent::KnightsAllianceNoticeUpdate(uint16 sClanID, std::string & strClanNotice)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClanNotice.c_str(), strClanNotice.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_ALLIANCE_UPDATE_ALLIANCENOTICE(%d,?)}"), sClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsFundUpdate(uint16 sClanID, uint32 nClanPointFund)
/**
* @brief	Updates the clan fund.
*
* @param	sClanID		  	Identifier for the clan.
* @param	nClanPointFund	The current clan point fund.
*/
int CDBAgent::KnightsFundUpdate(uint16 sClanID, uint32 nClanPointFund)
{
	int16 sRet = -1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);

	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_UPDATE_FUND(%d, %d)}"), sClanID, nClanPointFund)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsUserMemoUpdate(std::string strUserID, std::string strMemo)
/**
* @brief	Updates the user memo
*
* @param	strUserID	  	the Identifier.
* @param	strMemo			the Memo to be saved..
*/
int CDBAgent::KnightsUserMemoUpdate(std::string strUserID, std::string strMemo)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strMemo.c_str(), strMemo.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_UPDATE_USERMEMO(?,?)}"))))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsUserDonate(CUser * pUser,  uint32 amountNP, uint32 UserAmountNP)
/**
* @brief	Donates (clanPoints) clan points to the specified user's clan.
* 			Also increases the user's total NP donated.
*
* @param	pUser	  	The donor user.
* @param	amountNP  	The number of national points being donated by the user.
*
* @return	true if it succeeds, false if it fails.
*/
int CDBAgent::KnightsUserDonate(CUser * pUser,  uint32 amountNP, uint32 UserAmountNP)
{
	int16 sRet = 1;
	if (!pUser) return sRet;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_DONATE(?, %d, %d, %d)}"), pUser->GetClanID(), amountNP, UserAmountNP)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::LoadKnightsAllMembers(uint16 sClanID, Packet & result)

uint16 CDBAgent::LoadKnightsAllMembers(uint16 sClanID, Packet & result)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = -2;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);

	//if (!dbCommand->Execute(string_format(_T("SELECT strUserID, Fame, [Level], Class FROM USERDATA WHERE Knights = %d"), sClanID)))
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_LOAD_MEMBERS (%d)}"), sClanID)))	
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return 0;

	uint16 count = 0;
	do
	{
		string strCharID; uint16 sClass; uint8 bFame, bLevel;
		dbCommand->FetchString(1, strCharID);
		dbCommand->FetchByte(2, bFame);
		dbCommand->FetchByte(3, bLevel);
		dbCommand->FetchUInt16(4, sClass);

		result << strCharID << bFame << bLevel << sClass 
			// check if user's logged in (i.e. grab logged in state)
			<< uint8(g_pMain->GetUserPtr(strCharID, NameType::TYPE_CHARACTER) == nullptr ? 0 : 1);
		count++;
	} while (dbCommand->MoveNext());

	return count;
}
#pragma endregion

#pragma region CDBAgent::LoadKnightsAllList()

void CDBAgent::LoadKnightsAllList()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	const tstring szSQL = _T("SELECT IDNum, Points, Ranking FROM KNIGHTS WHERE Points != 0 ORDER BY Points DESC"); 

	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(szSQL))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	uint8 bCount = 0;
	int offset;

	do
	{
		if (bCount == 0)
		{
			result.clear();
			offset = (int)result.wpos();
			result << uint8(0);
		}

		uint32 nPoints; uint16 sClanID; uint8 bRanking;
		dbCommand->FetchUInt16(1, sClanID);
		dbCommand->FetchUInt32(2, nPoints);
		dbCommand->FetchByte(3, bRanking);

		result << sClanID << nPoints << bRanking;

		// only send 100 clans at a time (no shared memory limit, yay!)
		if (++bCount >= 100)
		{
			// overwrite the count
			result.put(offset, bCount);

			CKnightsManager::RecvKnightsAllList(result);
			bCount = 0;
		}
	} while (dbCommand->MoveNext());

	// didn't quite make it in the last batch (if any)? send the remainder.
	if (bCount < 100)
	{
		result.put(offset, bCount);
		CKnightsManager::RecvKnightsAllList(result);
	}
}
#pragma endregion

#pragma region CDBAgent::KnightsSave(uint16 sClanID, uint8 bFlag, uint32 nClanFund)
/**
* @brief	Donates (clanPoints) clan points to the specified user's clan.
* 			Also increases the user's total NP donated.
*
* @param	sClanID	  	The donor user.
* @param	bFlag  		The clan flag.
* @param	nClanFund  	The total clan fund.
*
* @return	true if it succeeds, false if it fails.
*/
int CDBAgent::KnightsSave(uint16 sClanID, uint8 bFlag, uint32 nClanFund)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_SAVE(%d, %d, %d)}"), sClanID, bFlag, nClanFund)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}

#pragma endregion

#pragma region CDBAgent::KnightsRefundNP(string & strUserID, uint32 nRefundNP)
/**
* @brief	Refunds the specified amount of NP to a logged out character.
*
* @param	strUserID	Character's name.
* @param	nRefundNP	The amount of NP to refund.
*/
int CDBAgent::KnightsRefundNP(string & strUserID, uint32 nRefundNP)
{
	int16 sRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return sRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_MEMBER_REFUNDNP(?, %d)}"), nRefundNP)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma endregion
/////////////////////////////////

/////////////////////////////////
#pragma region Knight Alliance Related Database Methods

#pragma region CDBAgent::KnightsAllianceCreate(uint16 sMainClanID, uint16 sSubClanID, uint8  byEmptyIndex)
/**
* @brief	Initiates a new alliance creation progress.
*
* @param	sMainClanID	the alliance index.
* @param	sSubClanID the knights index.
* @param	byEmptyIndex
*/
int CDBAgent::KnightsAllianceCreate(uint16 sMainClanID, uint16 sSubClanID, uint8  byEmptyIndex)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_ALLIANCE_CREATE(%d, %d, %d)}"), sMainClanID, sSubClanID, byEmptyIndex)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsAllianceInsert(uint16 sMainClanID, uint16 sSubClanID, uint8  byEmptyIndex)
/**
* @brief	Inserts a knight to an existing alliance.
*
* @param	sMainClanID	the alliance index.
* @param	sSubClanID the knights index.
* @param	byEmptyIndex
*/
int CDBAgent::KnightsAllianceInsert(uint16 sMainClanID, uint16 sSubClanID, uint8  byEmptyIndex)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_ALLIANCE_INSERT(%d, %d, %d)}"), sMainClanID, sSubClanID, byEmptyIndex)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsAllianceDestroy(uint16 sAllianceID)
/**
* @brief	Destroys the alliance.
*
* @param	sAllianceID	the alliance index.
*/
int CDBAgent::KnightsAllianceDestroy(uint16 sAllianceID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_ALLIANCE_DESTROY(%d)}"), sAllianceID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsAllianceRemove(uint16 sMainClanID, uint16 sSubClanID)
/**
* @brief	Removes a knight from the alliance.
*
* @param	sMainClanID	the alliance index.
* @param	sSubClanID the knights index.
*/
int CDBAgent::KnightsAllianceRemove(uint16 sMainClanID, uint16 sSubClanID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_ALLIANCE_REMOVE(%d, %d)}"), sMainClanID, sSubClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

#pragma region CDBAgent::KnightsAlliancePunish(uint16 sMainClanID, uint16 sSubClanID)
/**
* @brief	Punishes a knight from the alliance.
*
* @param	sMainClanID	the alliance index.
* @param	sSubClanID the knights index.
*/
int CDBAgent::KnightsAlliancePunish(uint16 sMainClanID, uint16 sSubClanID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL KNIGHTS_ALLIANCE_PUNISH(%d, %d)}"), sMainClanID, sSubClanID)))
		ReportSQLError(GetGameDB()->GetError());

	return sRet;
}
#pragma endregion

bool CDBAgent::SaveSheriffReports(std::string ReportedUser, std::string ReportedSheriff, std::string Crime, uint8 Yes, std::string SheriffA, uint8 No, std::string ReportDateTime)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, ReportedUser.c_str(), ReportedUser.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ReportedSheriff.c_str(), ReportedSheriff.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, Crime.c_str(), Crime.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, SheriffA.c_str(), SheriffA.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ReportDateTime.c_str(), ReportDateTime.length());


	if (!dbCommand->Execute(string_format(_T("{CALL SHERIF_REPORTS_SAVE(?, ?, ?, %d, ?, %d, ?)}"), Yes, No)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}

bool CDBAgent::UpdateSheriffTable(uint8 Type, std::string ReportedUser, uint8 Yes, uint8 No, std::string Sheriff)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, ReportedUser.c_str(), ReportedUser.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, Sheriff.c_str(), Sheriff.length());

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_SHERIFF_REPORTS(%d, ?, %d, %d, ?)}"), Type, Yes, No)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (Yes > 2)
	{
		CUser * pSelected = g_pMain->GetUserPtr(ReportedUser, NameType::TYPE_CHARACTER);
		if (pSelected 
			&& pSelected->isInGame()) // In game or Not we send him to the prison anyhow :)
			pSelected->ZoneChange(ZONE_PRISON, 167, 125); // Random for now

		g_pMain->m_SheriffArray.erase(ReportedUser);
	}
	else if (No >= 2)
	{
		g_pMain->m_SheriffArray.erase(ReportedUser);
	}
	return true;
}


#pragma endregion

#pragma region CDBAgent::SetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3, string & strCharID4)
bool CDBAgent::SetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3, string & strCharID4)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	string ID1, ID2, ID3, ID4;

	if (strCharID1 == "")
		ID1 = "";
	else
		ID1 = strCharID1;

	if (strCharID2 == "")
		ID2 = "";
	else
		ID2 = strCharID2;

	if (strCharID3 == "")
		ID3 = "";
	else
		ID3 = strCharID3;

	if (strCharID4 == "")
		ID4 = "";
	else
		ID4 = strCharID4;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ID1.c_str(), ID1.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ID2.c_str(), ID2.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ID3.c_str(), ID3.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ID4.c_str(), ID4.length());

	if (!dbCommand->Execute(_T("{CALL UPDATE_ALL_CHAR_ID("
		"?, ?, ?, ?, ?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::UpdateAchieveData(CUser *pUser)
bool CDBAgent::UpdateAchieveData(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	// achieve system
	char strAchieve[ACHIEVE_ARRAY_SIZE];
	memset(strAchieve, 0, sizeof(strAchieve));

	int index2 = 36;

	*(uint32 *)(strAchieve) = pUser->m_AchieveInfo.PlayTime;
	*(uint32 *)(strAchieve + 4) = pUser->m_AchieveInfo.MonsterDefeatCount;
	*(uint32 *)(strAchieve + 8) = pUser->m_AchieveInfo.UserDefeatCount;
	*(uint32 *)(strAchieve + 12) = pUser->m_AchieveInfo.UserDeathCount;
	*(uint32 *)(strAchieve + 16) = pUser->m_AchieveInfo.TotalMedal;
	*(uint16 *)(strAchieve + 20) = pUser->m_AchieveInfo.RecentAchieve[0];
	*(uint16 *)(strAchieve + 22) = pUser->m_AchieveInfo.RecentAchieve[1];
	*(uint16 *)(strAchieve + 24) = pUser->m_AchieveInfo.RecentAchieve[2];
	*(uint16 *)(strAchieve + 26) = pUser->m_AchieveInfo.NormalCount;
	*(uint16 *)(strAchieve + 28) = pUser->m_AchieveInfo.QuestCount;
	*(uint16 *)(strAchieve + 30) = pUser->m_AchieveInfo.WarCount;
	*(uint16 *)(strAchieve + 32) = pUser->m_AchieveInfo.AdvantureCount;
	*(uint16 *)(strAchieve + 34) = pUser->m_AchieveInfo.ChallangeCount;

	foreach_stlmap(itr, pUser->m_AchieveMap)
	{
		if (itr->second == nullptr)
			continue;

		if (index2 + 11 > ACHIEVE_ARRAY_SIZE) break;
		*(uint16 *)(strAchieve + index2) = itr->first;
		*(uint8 *)(strAchieve + index2 + 2) = itr->second->bStatus;
		*(uint32 *)(strAchieve + index2 + 3) = itr->second->iCount[0];
		*(uint32 *)(strAchieve + index2 + 7) = itr->second->iCount[1];
		index2 += 11;
	}

	// end of achieve system loading
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAchieve, sizeof(strAchieve), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());

	if (!dbCommand->Execute(string_format(_T("UPDATE USER_ACHIEVE_DATA SET sCoverID=%d, sSkillID=%d, sAchieveCount=%d, strAchieve=? WHERE strUserID=?"),
		pUser->m_sCoverID, pUser->m_sSkillID, pUser->m_AchieveMap.GetSize())))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::LoadAchieveData(CUser *pUser)
bool CDBAgent::LoadAchieveData(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	if (pUser->m_AchieveMap.GetSize() > 0)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	if (!dbCommand->Execute(_T("SELECT sCoverID, sSkillID, sAchieveCount, strAchieve FROM USER_ACHIEVE_DATA WHERE strUserID = ?")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return true; // No achieve data yet - not an error, just empty

	char strAchieve[ACHIEVE_ARRAY_SIZE];
	memset(strAchieve, 0, sizeof(strAchieve));
	uint16 sAchieveCount = 0;

	dbCommand->FetchUInt16(1, pUser->m_sCoverID);
	dbCommand->FetchUInt16(2, pUser->m_sSkillID);
	dbCommand->FetchUInt16(3, sAchieveCount);
	dbCommand->FetchBinary(4, strAchieve, sizeof(strAchieve));

	// Load User Achieve Data
	pUser->m_AchieveInfo.PlayTime = *(uint32*)(strAchieve);
	pUser->m_AchieveInfo.MonsterDefeatCount = *(uint32 *)(strAchieve + 4);
	pUser->m_AchieveInfo.UserDefeatCount = *(uint32 *)(strAchieve + 8);
	pUser->m_AchieveInfo.UserDeathCount = *(uint32 *)(strAchieve + 12);
	pUser->m_AchieveInfo.TotalMedal = *(uint32 *)(strAchieve + 16);
	pUser->m_AchieveInfo.RecentAchieve[0] = *(uint16 *)(strAchieve + 20);
	pUser->m_AchieveInfo.RecentAchieve[1] = *(uint16 *)(strAchieve + 22);
	pUser->m_AchieveInfo.RecentAchieve[2] = *(uint16 *)(strAchieve + 24);
	pUser->m_AchieveInfo.NormalCount = 0;
	pUser->m_AchieveInfo.QuestCount = 0;
	pUser->m_AchieveInfo.WarCount = 0;
	pUser->m_AchieveInfo.AdvantureCount = 0;
	pUser->m_AchieveInfo.ChallangeCount = 0;

	if (sAchieveCount == AchieveChalenngeInComplete)
	{
		foreach_stlmap(itr, g_pMain->m_AchieveMainArray)
		{
			_USER_ACHIEVE_INFO *pAchieve = new _USER_ACHIEVE_INFO;
			_ACHIEVE_MAIN *pAchieveMain = g_pMain->m_AchieveMainArray.GetData(itr->second->sIndex);

			if (pAchieveMain == nullptr)
				continue;

			if (pAchieveMain->byte2 == 41 
				|| pAchieveMain->byte2 == 42)
				pAchieve->bStatus = AchieveChalenngeInComplete;
			else
				pAchieve->bStatus = AchieveIncomplete;

			pAchieve->iCount[0] = AchieveChalenngeInComplete;
			pAchieve->iCount[1] = AchieveChalenngeInComplete;
			pUser->m_AchieveMap.PutData(itr->second->sIndex, pAchieve);
		}
	}
	else
	{
		for (int i = 0, index = 36; i < sAchieveCount; i++, index += 11)
		{
			uint16	sAchieveID = *(uint16*)(strAchieve + index);
			uint8	bStatus = *(uint8*)(strAchieve + index + 2);
			uint32	iCount1 = *(uint32*)(strAchieve + index + 3);
			uint32	iCount2 = *(uint32*)(strAchieve + index + 7);

			_USER_ACHIEVE_INFO *pAchieve = new _USER_ACHIEVE_INFO;
			_ACHIEVE_MAIN *pAchieveMain = g_pMain->m_AchieveMainArray.GetData(sAchieveID);
			
			if (pAchieveMain != nullptr)
			{
				if (bStatus == UserAchieveStatus::AchieveCompleted
					|| bStatus == UserAchieveStatus::AchieveFinished)
				{
					switch (pAchieveMain->AchieveType)
					{
					case 0:
						pUser->m_AchieveInfo.NormalCount++;
						break;
					case 1:
						pUser->m_AchieveInfo.QuestCount++;
						break;
					case 2:
						pUser->m_AchieveInfo.WarCount++;
						break;
					case 3:
						pUser->m_AchieveInfo.AdvantureCount++;
						break;
					case 4:
						pUser->m_AchieveInfo.ChallangeCount++;
						break;
					default:
						break;
					}
				}

				if (pAchieveMain->AchieveType == 4
					&& bStatus == UserAchieveStatus::AchieveIncomplete)
					pAchieve->bStatus = AchieveChalenngeInComplete;
				else
					pAchieve->bStatus = bStatus;

				pAchieve->iCount[0] = iCount1;
				pAchieve->iCount[1] = iCount2;
				pUser->m_AchieveMap.PutData(sAchieveID, pAchieve);
			}
		}
	}

	_ACHIEVE_MAIN *pAchieveMain = g_pMain->m_AchieveMainArray.GetData(pUser->m_sCoverID);
	if (pAchieveMain != nullptr)
		pUser->m_sCoverTitle = pAchieveMain->TitleID;

	pAchieveMain = g_pMain->m_AchieveMainArray.GetData(pUser->m_sSkillID);
	if (pAchieveMain)
	{
		pUser->m_sSkillTitle = pAchieveMain->TitleID;
		_ACHIEVE_TITLE *pAchieveTitle = g_pMain->m_AchieveTitleArray.GetData(pUser->m_sSkillTitle);
		if (pAchieveTitle != nullptr)
		{
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_STR] = pAchieveTitle->Str;
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_STA] = pAchieveTitle->Hp;
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_DEX] = pAchieveTitle->Dex;
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_INT] = pAchieveTitle->Int;
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_CHA] = pAchieveTitle->Mp;
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_ATTACK] = pAchieveTitle->Attack;
			pUser->m_sStatAchieveBonuses[(int16)UserAchieveStatTypes::ACHIEVE_STAT_DEFENCE] = pAchieveTitle->Defence;
		}
	}

	// end of achieve loading
	return true;
}
#pragma endregion

#pragma region int8 CDBAgent::CharacterSealProcess(std::string & strAcountID, std::string & strUserID, std::string & strPassword, uint64 nItemSerial)
/**
* @brief	Proceeds character seal process.
*
* @param	strAccountID	Character's Account name.
* @param	strUserID		the Character to be sealed within the Cypher Ring
* @param	strPassword		the Account Seal Password
*/
int8 CDBAgent::CharacterSealProcess(std::string & strAccountID, std::string & strUserID, std::string & strPassword, uint64 nItemSerial)
{
	uint8 bRet = 1;
	unique_ptr<OdbcCommand> dbCommand2(GetGameDB()->CreateCommand());
	if (dbCommand2.get() == nullptr)
		return 3;

	dbCommand2->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand2->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand2->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	if (!dbCommand2->Execute(string_format(_T("{? = CALL CHARACTER_SEAL_PROCEED(?, ?, " I64FMTD ")}"), nItemSerial)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}
#pragma endregion 

int8 CDBAgent::CharacterSealItemCheck(std::string& strUserID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	uint8 bRet = 1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	if (!dbCommand->Execute(_T("{? = CALL CHARACTER_SEAL_ITEM_CHECK(?)}")))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

std::string CDBAgent::getmypassword(std::string accountid)
{
	std::string my_password = "";
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return my_password;


	dbCommand->AddParameter(SQL_PARAM_INPUT, accountid.c_str(), accountid.length());
	if (!dbCommand->Execute(_T("SELECT strPasswd FROM TB_USER WHERE strAccountID = ?")))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return my_password;

	dbCommand->FetchString(1, my_password);
	return my_password;
}

bool CDBAgent::VerifyPassword(std::string accountid, std::string password)
{
	int8 bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, accountid.c_str(), accountid.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, password.c_str(), password.length());

	if (!dbCommand->Execute(_T("{? = CALL VERIFY_PASSWORD(?, ?)}")))
		ReportSQLError(GetGameDB()->GetError());

	return bRet == 1;
}

#pragma region int8 CDBAgent::CharacterUnSealProcess(std::string & strAcountID, uint8 bSelectedIndex, uint64 nItemSerial)
/**
* @brief	Proceeds character seal process.
*
* @param	strAccountID	Character's Account name.
* @param	strUserID		the Character to be sealed within the Cypher Ring
* @param	strPassword		the Account Seal Password
*/
int8 CDBAgent::CharacterUnSealProcess(std::string & strAccountID, uint8 bSelectedIndex, uint64 nItemSerial)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return 3;

	uint8 bRet = 1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL CHARACTER_UNSEAL_PROCEED(?, %d,  " I64FMTD ")}"), bSelectedIndex, nItemSerial)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

#pragma endregion

#pragma region CDBAgent::LoadAllCharacterSealItems(CharacterSealItemArray & m_CharacterSealArray)
/**
* @brief Loads all Character Seal Items data on server start.
*
* @param m_CharacterSealArray	the Character Seal array used by the GameServerDLG.
*/
bool  CDBAgent::LoadAllCharacterSealItems(CharacterSealItemArray & m_CharacterSealArray)
{
	foreach_stlmap(itr, g_pMain->m_CharacterSealItemArray)
	{
		_CHARACTER_SEAL_ITEM * pData = itr->second;
		if (pData == nullptr)
			continue;

		if (!LoadCharacterSealUserData(pData->strAccountID, pData->m_strUserID, pData))
			return false;

		_CHARACTER_SEAL_ITEM_MAPPING * pDataMapping = new _CHARACTER_SEAL_ITEM_MAPPING;
		pDataMapping->nItemSerial = pData->nSerialNum;
		pDataMapping->nUniqueID = pData->nUniqueID;

		if (!g_pMain->m_CharacterSealItemMapping.PutData(pDataMapping->nUniqueID, pDataMapping))
			return false;
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::LoadCharacterSealUserData(std::string & strAccountID, std::string & strCharID, _CHARACTER_SEAL_ITEM *pUser)

bool CDBAgent::LoadCharacterSealUserData(std::string & strAccountID, std::string & strCharID, _CHARACTER_SEAL_ITEM *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	DateTime time;

	if (dbCommand.get() == nullptr)
		return false;

	int8 bRet = -2; // generic error

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(_T("{? = CALL CHARACTER_SEAL_LOAD_USER_DATA(?, ?)}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	char strItem[INVENTORY_TOTAL * 8] = { 0 }, strSerial[INVENTORY_TOTAL * 8] = { 0 }, strItemExpiration[INVENTORY_TOTAL * 4] = { 0 };

	int field = 1;
	pUser->nUniqueID = g_pMain->GenerateItemUniqueID();
	dbCommand->FetchByte(field++, pUser->m_bRace);
	dbCommand->FetchUInt16(field++, pUser->m_sClass);
	dbCommand->FetchUInt32(field++, pUser->m_nHair);
	dbCommand->FetchByte(field++, pUser->m_bRank);
	dbCommand->FetchByte(field++, pUser->m_bTitle);
	dbCommand->FetchByte(field++, pUser->m_bLevel);
	dbCommand->FetchByte(field++, pUser->m_bRebirthLevel);
	dbCommand->FetchInt64(field++, pUser->m_iExp);
	dbCommand->FetchUInt32(field++, pUser->m_iLoyalty);
	dbCommand->FetchByte(field++, pUser->m_bFace);
	dbCommand->FetchByte(field++, pUser->m_bCity);
	dbCommand->FetchInt16(field++, pUser->m_bKnights);
	dbCommand->FetchByte(field++, pUser->m_bFame);
	dbCommand->FetchInt16(field++, pUser->m_sHp);
	dbCommand->FetchInt16(field++, pUser->m_sMp);
	dbCommand->FetchInt16(field++, pUser->m_sSp);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_STR]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_STA]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_DEX]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_INT]);
	dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_CHA]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_STR]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_STA]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_DEX]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_INT]);
	dbCommand->FetchByte(field++, pUser->m_bRebStats[(uint8)StatType::STAT_CHA]);
	dbCommand->FetchByte(field++, pUser->m_bAuthority);
	dbCommand->FetchInt16(field++, pUser->m_sPoints);
	dbCommand->FetchUInt32(field++, pUser->m_iGold);
	dbCommand->FetchByte(field++, pUser->m_bZone);
	dbCommand->FetchString(field++, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->FetchBinary(field++, strItem, sizeof(strItem));
	dbCommand->FetchBinary(field++, strSerial, sizeof(strSerial));
	dbCommand->FetchBinary(field++, strItemExpiration, sizeof(strItemExpiration));
	dbCommand->FetchUInt32(field++, pUser->m_iMannerPoint);
	dbCommand->FetchUInt32(field++, pUser->m_iLoyaltyMonthly);

	ByteBuffer itemBuffer, serialBuffer, itemexpirationbuffer;
	itemBuffer.append(strItem, sizeof(strItem));
	serialBuffer.append(strSerial, sizeof(strSerial));
	itemexpirationbuffer.append(strItemExpiration, sizeof(strItemExpiration));

	memset(pUser->m_sItemArray, 0x00, sizeof(pUser->m_sItemArray));

	UserRentalMap rentalData;

	// Until this statement is cleaned up, 
	// no other statements can be processed.
	delete dbCommand.release();

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		uint64 nSerialNum;
		uint32 nItemID;
		uint32 nExpirationTime;
		int16 sDurability, sCount;
		itemBuffer >> nItemID >> sDurability >> sCount;
		serialBuffer >> nSerialNum;
		itemexpirationbuffer >> nExpirationTime;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull()  || sCount <= 0)
			continue;

		if (!pTable.m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		if (nSerialNum == 0)
			nSerialNum = g_pMain->GenerateItemSerial();

		_ITEM_DATA *pItem = pUser->GetItem(i);
		if (pItem == nullptr)
			continue;

		pItem->nNum = nItemID;
		pItem->sDuration = pTable.isAccessory() ? pTable.m_sDuration : sDurability;
		pItem->sCount = sCount;
		pItem->nExpirationTime = nExpirationTime; // set this to 0 for now till the expiration system completed
		pItem->nSerialNum = nSerialNum;
	}
	return true;
}
#pragma endregion

#pragma region CDBAgent::LoadUserSealExpData(CUser *pUser)
bool CDBAgent::LoadUserSealExpData(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());

	if (!dbCommand->Execute(_T("{CALL LOAD_USER_SEAL_EXP(?)}")))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchUInt32(1, pUser->m_iSealedExp);

	return true;
}
#pragma endregion

#pragma region CDBAgent::UpdateUserSealExpData(CUser *pUser)
bool CDBAgent::UpdateUserSealExpData(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_SEAL_EXP(?, %d)}"), pUser->m_iSealedExp)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::DrakiSignCreateDatas(std::string & strCharID, CUser *pUser)
uint8 CDBAgent::DrakiDataCheck(CUser* pUser)
{
	uint8 bRet = 0;
	if (pUser == nullptr) return 0;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return bRet;

	uint8 Class = 0;
	if (pUser->isWarrior()) Class = 1;
	else if (pUser->isRogue()) Class = 2;
	else if (pUser->isMage()) Class = 3;
	else if (pUser->isPriest()) Class = 4;
	else if (pUser->isPortuKurian()) Class = 13;
	if (Class == 0) return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL CHECK_DRAKI_USER(?, %d)}"), Class))) {
		ReportSQLError(GetGameDB()->GetError());
		return bRet;
	}
	if (!dbCommand->hasData()) return bRet;
	return bRet;
}
#pragma endregion

#pragma region CDBAgent::LoadUserDrakiTowerData(CUser *pUser)
bool CDBAgent::LoadUserDrakiTowerData(CUser *pUser)
{
	if (pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());

	if (!dbCommand->Execute(_T("{CALL LOAD_DRAKI_TOWER(?)}")))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
	{
		pUser->m_iDrakiTime = 3600;
		pUser->m_bDrakiStage = 1;
		pUser->m_bDrakiEnteranceLimit = 3;
	}
	else
	{
		dbCommand->FetchInt32(1, pUser->m_iDrakiTime);
		dbCommand->FetchByte(2, pUser->m_bDrakiStage);
		dbCommand->FetchByte(3, pUser->m_bDrakiEnteranceLimit);
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::UpdateDaysDrakiTowerLimit()
bool CDBAgent::UpdateDaysDrakiTowerLimit()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(_T("{CALL UPDATE_DRAKI_LIMIT_DAY}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}
#pragma endregion

#pragma region CDBAgent::UpdateDrakiTowerLimitLastUpdate(std::string & strUserID, uint8 EnteranceLimit)
bool CDBAgent::UpdateDrakiTowerLimitLastUpdate(std::string & strUserID, uint8 EnteranceLimit)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_DRAKI_LIMIT(?, %d)}"), EnteranceLimit)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}
#pragma endregion


#pragma region CDBAgent::LoadDrakiTowerTables()
bool CDBAgent::LoadDrakiTowerTables()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(_T("{CALL LOAD_DRAKI_MONSTER_LIST}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	do
	{
		DRAKI_MONSTER_LIST *pData = new DRAKI_MONSTER_LIST;

		dbCommand->FetchUInt32(1, pData->nIndex);
		dbCommand->FetchByte(2, pData->bDrakiStage);
		dbCommand->FetchUInt16(3, pData->sMonsterID);
		dbCommand->FetchUInt16(4, pData->sPosX);
		dbCommand->FetchUInt16(5, pData->sPosZ);
		dbCommand->FetchUInt16(6, pData->sDirection);
		dbCommand->FetchByte(7, pData->bMonster);


		if (!g_pMain->m_DrakiMonsterListArray.PutData(pData->nIndex, pData))
			delete pData;

	} while (dbCommand->MoveNext());

	if (!dbCommand->Execute(_T("{CALL LOAD_DRAKI_TOWER_STAGES}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	do
	{
		DRAKI_ROOM_LIST *pData = new DRAKI_ROOM_LIST;

		dbCommand->FetchByte(1, pData->bIndex);
		dbCommand->FetchByte(2, pData->bDrakiStage);
		dbCommand->FetchByte(3, pData->bDrakiSubStage);
		dbCommand->FetchByte(4, pData->bDrakiNpcStage);

		if (!g_pMain->m_DrakiRoomListArray.PutData(pData->bIndex, pData))
			delete pData;

	} while (dbCommand->MoveNext());

	return true;
}
#pragma endregion

#pragma region CDBAgent::UpdateDrakiTowerData(uint32 sTime, uint8 bStage, std::string & strUserID)
void CDBAgent::UpdateDrakiTowerData(uint32 sTime, uint8 bStage, std::string & strUserID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_DRAKI_TOWER_DATA(?, %d, %d)}"), sTime, bStage)))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return;
}
#pragma endregion

#pragma region CDBAgent::ReqDrakiTowerList(Packet & pkt, CUser* pUser)
void CDBAgent::ReqDrakiTowerList(Packet & pkt, CUser* pUser)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr || pUser == nullptr)
		return;

	if (!dbCommand->Execute(_T("{CALL LOAD_DRAKI_RANK}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	pkt.SByte();
	uint8 Counter = 0;
	if (!dbCommand->hasData()) {
		for (int i = 0; i < 5; i++) {
			Counter++;
			pkt << uint8(++Counter) << "" << uint32(3600) << uint32(1);
		}
		pkt << uint8(-1) << pUser->m_strUserID << uint32(3600) << uint32(1) << uint32(1) << uint32(pUser->m_bDrakiEnteranceLimit);
		return;
	}

	std::string strRankID;
	uint32 pFinishTime, iRank, sIndex, Class;
	uint8  bStage;
	uint16 Counter2 = 0;
	do {
		int field = 1;
		dbCommand->FetchUInt32(field++, sIndex);
		dbCommand->FetchUInt32(field++, Class);
		dbCommand->FetchUInt32(field++, iRank);
		dbCommand->FetchString(field++, strRankID);
		dbCommand->FetchByte(field++, bStage);
		dbCommand->FetchUInt32(field++, pFinishTime);

		if (pUser->isWarrior()) {
			if (Class == 1) {
				Counter2++;
				pkt << uint8(iRank) << strRankID << pFinishTime << uint32(bStage);
			}
		}
		else if (pUser->isRogue()) {
			if (Class == 2) {
				Counter2++;
				pkt << uint8(iRank) << strRankID << pFinishTime << uint32(bStage);
			}
		}
		else if (pUser->isMage()) {
			if (Class == 3) {
				Counter2++;
				pkt << uint8(iRank) << strRankID << pFinishTime << uint32(bStage);
			}
		}
		else if (pUser->isPriest()) {
			if (Class == 4) {
				Counter2++;
				pkt << uint8(iRank) << strRankID << pFinishTime << uint32(bStage);
			}
		}
		else if (pUser->isPortuKurian()) {
			if (Class == 13) {
				Counter2++;
				pkt << uint8(iRank) << strRankID << pFinishTime << uint32(bStage);
			}
		}
	} while (dbCommand->MoveNext());

	int8 thyke = (5 - Counter2);
	if (thyke < 0) thyke = 0;

	if (thyke > 0) {
		for (int i = 0; i < thyke; i++) 
			pkt << uint8(++Counter2) << "" << uint32(3600) << uint32(1);
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	if (!dbCommand->Execute(_T("SELECT * FROM DRAKI_TOWER_RIFT_RANK WHERE strUserID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData()) {
		pkt << uint8(-1) << pUser->GetName() << uint32(3600) << uint32(1) << uint32(1) << uint32(pUser->m_bDrakiEnteranceLimit);
		return;
	}
	dbCommand->FetchUInt32(1, sIndex);
	dbCommand->FetchUInt32(2, Class);
	dbCommand->FetchUInt32(4, iRank);
	dbCommand->FetchString(5, strRankID);
	dbCommand->FetchByte(6, bStage);
	dbCommand->FetchUInt32(7, pFinishTime);

	if (iRank > 254) iRank = -1;

	uint8 kk = bStage % 6;
	uint32 MaxStages = (bStage - kk) / 6 + (kk > 0 ? 1 : 0);
	pkt << uint8(iRank) << pUser->GetName() << pFinishTime << uint32(bStage) << uint32(MaxStages == 0 ? 1 : MaxStages) << uint32(pUser->m_bDrakiEnteranceLimit);
}
#pragma endregion

#pragma region CDBAgent::CharacterSelectPrisonCheck(std::string & strCharID, uint8 & ZoneID)
bool CDBAgent::CharacterSelectPrisonCheck(std::string & strCharID, uint8 & ZoneID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(_T("SELECT Zone FROM USERDATA WHERE strUserID = ?")))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchByte(1, ZoneID);
	return true;
}
#pragma endregion

#pragma region CDBAgent::SelectCharacterChecking(string & strAccountID, string & strCharID)
int8 CDBAgent::SelectCharacterChecking(string & strAccountID, string & strCharID)
{
	int8 bRet = 0;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{? = CALL SELECT_CHARACTER(?, ?)}"))))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}
#pragma endregion

ServerUniteSelectingCharNameErrorOpcode CDBAgent::UpdateSelectingCharacterName(std::string & strAccountID, std::string & strUserID, std::string & strNewUserID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return ServerUniteSelectingCharNameErrorOpcode::CannotCharacterID;

	int16 bRet = 0;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strNewUserID.c_str(), strNewUserID.length());


	if (!dbCommand->Execute(string_format(_T("{? = CALL CHARACTER_SELECT_NAME_CHANGE(?, ?, ?)}"))))
		ReportSQLError(GetGameDB()->GetError());

	return ServerUniteSelectingCharNameErrorOpcode(bRet);
}

void CDBAgent::LoadPasswd(std::string AccountID, std::string & strPasswd)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return;

	char strPass[8] = { 0 };

	dbCommand->AddParameter(SQL_PARAM_INPUT, AccountID.c_str(), AccountID.length());

	if (!dbCommand->Execute(_T("{CALL LOAD_OLD_ITEM_PASSWD(?)}")))
		ReportSQLError(GetGameDB()->GetError());

	dbCommand->FetchString(1, strPasswd);
}

bool CDBAgent::LoadGameMasterSetting(string strCharID,CUser*pUser)
{
	if (!pUser || strCharID.empty())
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL LOAD_GAMEMASTER_SETTINGS(?)}"))))
		ReportSQLError(GetGameDB()->GetError());

	if (!dbCommand->hasData())
		return false;

	int field = 1; std::string charid;
	dbCommand->FetchString(field++, charid);
	dbCommand->FetchByte(field++, pUser->m_GameMastersMute);
	dbCommand->FetchByte(field++, pUser->m_GameMastersUnMute);
	dbCommand->FetchByte(field++, pUser->m_GameMastersUnBan);
	dbCommand->FetchByte(field++, pUser->m_GameMastersBanPermit);
	dbCommand->FetchByte(field++, pUser->m_GameMastersBanUnder);
	dbCommand->FetchByte(field++, pUser->m_GameMastersBanDays);
	dbCommand->FetchByte(field++, pUser->m_GameMastersBanCheating);
	dbCommand->FetchByte(field++, pUser->m_GameMastersBanScamming);
	dbCommand->FetchByte(field++, pUser->m_GameMastersAllowAttack);
	dbCommand->FetchByte(field++, pUser->m_GameMastersDisabledAttack);
	dbCommand->FetchByte(field++, pUser->m_GameMastersNpAdd);
	dbCommand->FetchByte(field++, pUser->m_GameMastersExpAdd);
	dbCommand->FetchByte(field++, pUser->m_GameMastersMoneyAdd);
	dbCommand->FetchByte(field++, pUser->m_GameMastersDropAdd);
	dbCommand->FetchByte(field++, pUser->m_GameMastersLoyaltyChange);
	dbCommand->FetchByte(field++, pUser->m_GameMastersExpChange);
	dbCommand->FetchByte(field++, pUser->m_GameMastersMoneyChange);
	dbCommand->FetchByte(field++, pUser->m_GameMastersGiveItem);
	dbCommand->FetchByte(field++, pUser->m_GameMastersGiveItemSelf);
	dbCommand->FetchByte(field++, pUser->m_GameMastersSummonUser);
	dbCommand->FetchByte(field++, pUser->m_GameMastersTpOnUser);
	dbCommand->FetchByte(field++, pUser->m_GameMastersZoneChange);
	dbCommand->FetchByte(field++, pUser->m_GameMastersLocationChange);
	dbCommand->FetchByte(field++, pUser->m_GameMastersMonsterSummon);
	dbCommand->FetchByte(field++, pUser->m_GameMastersNpcSummon);
	dbCommand->FetchByte(field++, pUser->m_GameMastersMonKilled);
	dbCommand->FetchByte(field++, pUser->m_GameMastersTeleportAllUser);
	dbCommand->FetchByte(field++, pUser->m_GameMastersClanSummon);
	dbCommand->FetchByte(field++, pUser->m_GameMastersResetPlayerRanking);
	dbCommand->FetchByte(field++, pUser->m_GameMastersChangeEventRoom);
	dbCommand->FetchByte(field++, pUser->m_GameMastersWarOpen);
	dbCommand->FetchByte(field++, pUser->m_GameMastersWarClose);
	dbCommand->FetchByte(field++, pUser->m_GameMastersCaptainElection);
	dbCommand->FetchByte(field++, pUser->m_GameMastersSendPrsion);
	dbCommand->FetchByte(field++, pUser->m_GameMastersKcChange);
	dbCommand->FetchByte(field++, pUser->m_GameMastersReloadTable);
	dbCommand->FetchByte(field++, pUser->m_GameMastersDropTest);
	dbCommand->FetchByte(field++, pUser->m_GameMasterChaosEvent);
	dbCommand->FetchByte(field++, pUser->m_GameMasterBdwEvent);
	dbCommand->FetchByte(field++, pUser->m_GameMasterJrEvent);
	dbCommand->FetchByte(field++, pUser->m_GameMasterRoyalEvent);
	dbCommand->FetchByte(field++, pUser->m_GameMasterBeefEvent);
	return true;
}

bool CDBAgent::GameStartClearRemainUser()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
	{
		printf("Clear Remain Users Error\n");
		return false;
	}

	if (!dbCommand->Execute(_T("{CALL GAME_START_CLEAR_REMAIN_USERS}")))
	{
		ReportSQLError(GetGameDB()->GetError());
		printf("Clear Remain Users Error\n");
		return false;
	}
	return true;
}

bool CDBAgent::CheckTrashItemListTime() {
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) {
		printf("CheckTrashItemListTime Error\n");
		return false;
	}

	if (!dbCommand->Execute(_T("{CALL UPDATE_TRASH_ITEMLIST}"))) {
		ReportSQLError(GetGameDB()->GetError());
		printf("CheckTrashItemListTime\n");
		return false;
	}
	return true;
}
#pragma endregion


void CDBAgent::LoadPetData(uint64 nSerial)
{
	return;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	if (!dbCommand->Execute(string_format(_T("{CALL LOAD_PET_DATA(" I64FMTD ")}"), nSerial)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	char strItem[PET_INVENTORY_TOTAL * 23];
	memset(strItem, 0x00, sizeof(strItem));

	PET_DATA * p = new PET_DATA();
	dbCommand->FetchString(1, p->strPetName);
	dbCommand->FetchByte(2, p->bLevel);
	dbCommand->FetchUInt16(3, p->sHP);
	dbCommand->FetchUInt16(4, p->sMP);
	dbCommand->FetchUInt32(5, p->nIndex);
	dbCommand->FetchInt16(6, p->sSatisfaction);
	dbCommand->FetchUInt32(7, p->nExp);
	dbCommand->FetchUInt16(8, p->sPid);
	dbCommand->FetchUInt16(9, p->sSize);
	dbCommand->FetchBinary(10, strItem, sizeof(strItem));
	
	ByteBuffer PetItemBuffer;
	PetItemBuffer.append(strItem, sizeof(strItem));

	PET_INFO * pInfo = g_pMain->m_PetInfoSystemArray.GetData(p->bLevel);
	if (pInfo == nullptr)
	{
		PET_INFO * pInfo2 = g_pMain->m_PetInfoSystemArray.GetData(PET_START_LEVEL);
		if (pInfo2 == nullptr)
		{
			printf("LoadPetData Warning!!! \n");
			TRACE("LoadPetData Warning!!! \n");
			return;
		}
	}

	memset(p->sItem, 0x00, sizeof(p->sItem));

	p->info = pInfo;
	int index = 0;
	for (int i = 0; i < PET_INVENTORY_TOTAL; i++)
	{
		uint64 nSerialNum;
		uint32 nItemID, nExpirationtime;
		int16 sDurability, sCount, PetRentTime;
		uint8 bFlag;

		PetItemBuffer >> nItemID >> sDurability >> sCount >> bFlag >> PetRentTime >> nExpirationtime >> nSerialNum;

		_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable.isnull() || sCount <= 0)
			continue;

		if (!pTable.m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		if (nSerialNum == 0)
			nSerialNum = g_pMain->GenerateItemSerial();

		_ITEM_DATA * pItem = &p->sItem[i];
		pItem->nNum = nItemID;
		pItem->sDuration = sDurability;
		pItem->sCount = sCount;
		pItem->bFlag = bFlag;
		pItem->sRemainingRentalTime = PetRentTime;	
		pItem->nExpirationTime = nExpirationtime;
		pItem->nSerialNum = nSerialNum;
	}
	
	//g_pMain->m_PetSystemlock.lock();
	g_pMain->m_PetDataSystemArray.insert(std::make_pair(nSerial, p));
	//g_pMain->m_PetSystemlock.unlock();
}

uint32 CDBAgent::CreateNewPet(uint64 nSerialID, uint8 bLevel, std::string& strPetName)
{
	uint32 bRet = 3;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strPetName.c_str(), strPetName.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL CREATE_NEW_PET(" I64FMTD ",%d, ?)}"), nSerialID, bLevel)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

void CDBAgent::UpdatingUserPet(uint64 nSerial)
{
	return;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return;

	g_pMain->m_PetSystemlock.lock();
	PetDataSystemArray::iterator itr = g_pMain->m_PetDataSystemArray.find(nSerial);
	if (itr == g_pMain->m_PetDataSystemArray.end())
	{
		g_pMain->m_PetSystemlock.unlock();
		return;
	}
	g_pMain->m_PetSystemlock.unlock();
	PET_DATA * pPet = itr->second;
	if (pPet == nullptr)
		return;

	ByteBuffer PetItemBuffer;
	for (int i = 0; i < PET_INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA * pItem = &pPet->sItem[i];
		PetItemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag << pItem->sRemainingRentalTime << pItem->nExpirationTime << pItem->nSerialNum;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, pPet->strPetName.c_str(), pPet->strPetName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char*)PetItemBuffer.contents(), PetItemBuffer.size(), SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_PET(" I64FMTD ", ?, %d, %d, %d, %d, %d, %d, %d, ?)}"), nSerial, pPet->bLevel, pPet->sHP, pPet->sMP, pPet->sSatisfaction, pPet->nExp, pPet->sPid, pPet->sSize)))
		ReportSQLError(GetGameDB()->GetError());
}

#pragma region CDBAgent::LoadBotTable()
bool CDBAgent::LoadBotTable()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());

	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(_T("{CALL LOAD_USER_BOTS}")))
	{
		printf(" Error LOAD_USER_BOTS. \n");
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
	{
		// Tablo bos - hata degil, bot olmadan devam et
		printf(" LOAD_USER_BOTS: No bots found (table empty).\n");
		return true;
	}

	do
	{
		CBot * pUser = new CBot();
		int field = 1;

		pUser->Initialize();

		char strItem[INVENTORY_TOTAL * 8];
		memset(strItem, 0x00, sizeof(strItem));

		dbCommand->FetchString(field++, pUser->m_strUserID);
		dbCommand->FetchByte(field++, pUser->m_bNation);
		dbCommand->FetchByte(field++, pUser->m_bRace);
		dbCommand->FetchUInt16(field++, pUser->m_sClass);
		dbCommand->FetchUInt32(field++, pUser->m_nHair);
		dbCommand->FetchByte(field++, pUser->m_bLevel);
		dbCommand->FetchByte(field++, pUser->m_bFace);
		dbCommand->FetchInt16(field++, pUser->m_bKnights);
		dbCommand->FetchByte(field++, pUser->m_bFame);
		dbCommand->FetchByte(field++, pUser->m_bZone);
		pUser->m_curx = (float)(dbCommand->FetchInt32(field++) / 100.0f);
		pUser->m_curz = (float)(dbCommand->FetchInt32(field++) / 100.0f);
		pUser->m_cury = (float)(dbCommand->FetchInt32(field++) / 100.0f);
		pUser->m_oldx = pUser->m_curx;
		pUser->m_oldy = pUser->m_cury;
		pUser->m_oldz = pUser->m_curz;


		memset(pUser->m_sItemArray, 0x00, sizeof(pUser->m_sItemArray));
		dbCommand->FetchBinary(field++, strItem, sizeof(strItem));
		ByteBuffer itemData;
		itemData.append(strItem, sizeof(strItem));

		for (int i = 0; i < INVENTORY_TOTAL; i++)
		{
			uint32 nItemID;
			uint16 sDurability, sCount;
			itemData >> nItemID >> sDurability >> sCount;
			_ITEM_DATA *pItem = &pUser->m_sItemArray[i];
			pItem->nNum = nItemID;
			pItem->sDuration = sDurability;
			pItem->sCount = sCount;
		}

		dbCommand->FetchUInt16(field++, pUser->m_sSid);
		dbCommand->FetchUInt16(field++, pUser->m_sAchieveCoverTitle);
		dbCommand->FetchByte(field++, pUser->m_reblvl);
		dbCommand->FetchBinary(field++, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
		dbCommand->FetchUInt32(field++, pUser->m_iGold);
		dbCommand->FetchInt16(field++, pUser->m_sPoints);
		dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_STR]);
		dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_STA]);
		dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_DEX]);
		dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_INT]);
		dbCommand->FetchByte(field++, pUser->m_bStats[(uint8)StatType::STAT_CHA]);
		pUser->m_sSid += MAX_USER;

		if (!g_pMain->m_BotArray.PutData(pUser->m_sSid, pUser))
			delete pUser;

	} while (dbCommand->MoveNext());

	return true;
}
#pragma endregion
bool CDBAgent::InsertBotMerchant(_BOT_SAVE_DATA* pData)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	int16 sRet = 1;//nNum1, nPrice1, sCount1, sDuration1
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);
	if (!dbCommand->Execute(string_format(_T("{? = CALL INSERT_BOT_MERCHANT_DATA(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d,%d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d)}"),
		pData->nNum[0], pData->nPrice[0], pData->sCount[0], pData->sDuration[0], pData->isKc[0],
		pData->nNum[1], pData->nPrice[1], pData->sCount[1], pData->sDuration[1], pData->isKc[1],
		pData->nNum[2], pData->nPrice[2], pData->sCount[2], pData->sDuration[2], pData->isKc[2],
		pData->nNum[3], pData->nPrice[3], pData->sCount[3], pData->sDuration[3], pData->isKc[3],
		pData->nNum[4], pData->nPrice[4], pData->sCount[4], pData->sDuration[4], pData->isKc[4],
		pData->nNum[5], pData->nPrice[5], pData->sCount[5], pData->sDuration[5], pData->isKc[5],
		pData->nNum[6], pData->nPrice[6], pData->sCount[6], pData->sDuration[6], pData->isKc[6],
		pData->nNum[7], pData->nPrice[7], pData->sCount[7], pData->sDuration[7], pData->isKc[7],
		pData->nNum[8], pData->nPrice[8], pData->sCount[8], pData->sDuration[8], pData->isKc[8],
		pData->nNum[9], pData->nPrice[9], pData->sCount[9], pData->sDuration[9], pData->isKc[9],
		pData->nNum[10], pData->nPrice[10], pData->sCount[10], pData->sDuration[10], pData->isKc[10],
		pData->nNum[11], pData->nPrice[11], pData->sCount[11], pData->sDuration[11], pData->isKc[11],
		(int)(pData->fX * 100), (int)(pData->fZ * 100), (int)(pData->fY * 100), pData->Minute, pData->byZone, pData->Direction, pData->sMerchanType)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}

	return true;
}

uint8 CDBAgent::LoadPromotionKey(std::string& PPCardKeys, CUser* pUser)
{
	uint8 bRet = 0; // failed key

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, PPCardKeys.c_str(), PPCardKeys.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL LOAD_PROMOTION_SYSTEM(?, ?, ?)}")))) {
		ReportSQLError(GetGameDB()->GetError());
		return 3;
	}


	return bRet;

}

#pragma region CDBAgent::LoadPPCard(std::string PPCardKeys, std::string strCharID)
uint8 CDBAgent::LoadPPCard(std::string PPCardKeys, CUser *pUser)
{
	if (PPCardKeys.empty() || !pUser || pUser->GetName().empty()) return 0;
	
	uint32 Cash = 0, tlcount = 0; uint8 cashtype = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, PPCardKeys.c_str(), PPCardKeys.length());
	if (!dbCommand->Execute(_T("SELECT KnightCash,TLBalance, CashType FROM PPCARD_LIST WHERE PPKeyCode = ? AND [STATUS] = 0"))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}
		
	if (!dbCommand->hasData())
		return 0;

	dbCommand->FetchUInt32(1, Cash);
	dbCommand->FetchUInt32(2, tlcount);
	dbCommand->FetchByte(3, cashtype);
	
	if ((cashtype != 3 && Cash == 0 && tlcount == 0) || cashtype < 1 || cashtype > 3)
		return 0;

	if (cashtype == 3) {
		uint8 pRet = LoadPromotionKey(PPCardKeys, pUser);
		if (pRet != 4)
			return 0;

		if (Cash || tlcount)pUser->GiveBalance(Cash, tlcount);
		return 1;
	}

	// FIX: Use a new command to avoid stale parameters from the SELECT above
	unique_ptr<OdbcCommand> dbUpdateCmd(GetGameDB()->CreateCommand());
	if (dbUpdateCmd.get() == nullptr)
		return 0;

	dbUpdateCmd->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbUpdateCmd->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	dbUpdateCmd->AddParameter(SQL_PARAM_INPUT, PPCardKeys.c_str(), PPCardKeys.length());
	if (!dbUpdateCmd->Execute(_T("UPDATE PPCARD_LIST SET [STATUS] = 1 , strAccountID = ? , strUserID = ? , UpdateDate = convert(varchar, getdate(), 120) WHERE PPKeyCode = ? AND [STATUS] = 0"))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}

	pUser->GiveBalance(Cash, tlcount);
	return 1;
}

#pragma endregion

bool CDBAgent::LoadKnightCash(std::string & strAccountID, CUser * pUser)
{
	if (strAccountID.empty() || pUser == nullptr)
		return false;

	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("SELECT CashPoint,BonusCashPoint FROM TB_USER WHERE strAccountID = ?")))
		ReportSQLError(m_AccountDB->GetError());

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchUInt32(1, pUser->m_nKnightCash);
	dbCommand->FetchUInt32(2, pUser->m_nTLBalance);
	return true;
}

bool CDBAgent::UpdateKnightCash(CUser * pUser)
{
	if (pUser == nullptr)
		return false;

	if (pUser->m_nKnightCash > 999999999) pUser->m_nKnightCash = 999999999;
	if (pUser->m_nTLBalance > 999999999) pUser->m_nTLBalance = 999999999;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	uint32 KnightCash = pUser->m_nKnightCash;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());

	if (!dbCommand->Execute(string_format(_T("UPDATE TB_USER SET CashPoint = %d,BonusCashPoint=%d WHERE strAccountID = ?"), KnightCash,pUser->m_nTLBalance)))
		ReportSQLError(GetGameDB()->GetError());

	return true;
}

bool CDBAgent::UpdateAccountKnightCash(CUser* pUser)
{
	if (!pUser || pUser->GetAccountName().empty()) return false;
	unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_BALANCE (?, %d, %d)}"), pUser->m_nKnightCash, pUser->m_nTLBalance))) {
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}
	return true;
}


int8 CDBAgent::SetItemFromLetter(string& strCharID, uint32 nLetterID)
{
	int8 bRet = -1; // error
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL MAIL_BOX_SET_ITEM(?, %d)}"), nLetterID))) {
		ReportSQLError(GetGameDB()->GetError());
		return -1;
	}
	return bRet;
}

bool CDBAgent::UpdateReportUser(std::string strCharID, std::string ReportType, std::string SubJect, std::string Message)
{
	if (strCharID.empty() || ReportType.empty() || SubJect.empty() || Message.empty()) return false;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, ReportType.c_str(), ReportType.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, SubJect.c_str(), SubJect.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, Message.c_str(), Message.length());
	if (!dbCommand->Execute(string_format(_T("INSERT INTO BUG_REPORT VALUES(?,?,?,?)")))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return true;
}

uint8 CDBAgent::AccountInfoShow(std::string& AccountID)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, AccountID.c_str(), AccountID.length());
	if (!dbCommand->Execute(_T("SELECT AccountCheck FROM TB_USER WHERE strAccountID = ?"))){
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}

	if (!dbCommand->hasData())
		return 0;

	uint8 Active = 0;
	dbCommand->FetchByte(1, Active);
	return Active;
}

uint8 CDBAgent::AccountInfoSave(std::string& AccountID, std::string UserID, std::string& email, std::string& phone, std::string& otp, std::string& seal)
{
	uint8 bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return 0;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, AccountID.c_str(), AccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, UserID.c_str(), UserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, email.c_str(), email.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, phone.c_str(), phone.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, otp.c_str(), otp.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, seal.c_str(), seal.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL ACCOUNT_INFO_SAVE(?,?,?,?,?,?)}")))) {
		ReportSQLError(GetGameDB()->GetError());
		return bRet;
	}

	return bRet;
}

bool CDBAgent::LoadAutoClanInfo(std::string& strCharID, uint16& Class, uint8& Level, uint16& ClanID, uint8& Fame, uint32& Loyalty, uint32& MontlyLoyalty)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(_T("SELECT Class, Level, Knights, Fame, Loyalty, LoyaltyMonthly FROM USERDATA WHERE strUserID = ?"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	int field = 1;
	dbCommand->FetchUInt16(field++, Class);
	dbCommand->FetchByte(field++, Level);
	dbCommand->FetchUInt16(field++, ClanID);
	dbCommand->FetchByte(field++, Fame);
	dbCommand->FetchUInt32(field++, Loyalty);
	dbCommand->FetchUInt32(field++, MontlyLoyalty);
	return true;
}

bool CDBAgent::LoadDrakiTowerDailyRank(std::vector<_DRAKI_TOWER_FORDAILY_RANKING>& List)
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(_T("{CALL LOAD_DRAKI_TOWER_DAILY_RANK}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	do
	{
		_DRAKI_TOWER_FORDAILY_RANKING pList;
		dbCommand->FetchString(1, pList.strUserID);
		dbCommand->FetchUInt32(2, pList.Class);
		dbCommand->FetchUInt32(3, pList.DrakiTime);
		dbCommand->FetchByte(4, pList.DrakiStage);
		List.push_back(pList);
	} while (dbCommand->MoveNext());

	return true;
}


bool CDBAgent::LoadUserDailyRank(std::string& strCharID, CUser* pUser)
{
	if (pUser == nullptr || strCharID != pUser->GetName())
		return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL LOAD_DAILY_RANK_USER(?)}"))))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	int field = 1;
	dbCommand->FetchUInt64(field++, pUser->pUserDailyRank.GMTotalSold);
	dbCommand->FetchUInt64(field++, pUser->pUserDailyRank.MHTotalKill);
	dbCommand->FetchUInt64(field++, pUser->pUserDailyRank.SHTotalExchange);
	dbCommand->FetchUInt64(field++, pUser->pUserDailyRank.CWCounterWin);
	dbCommand->FetchUInt64(field++, pUser->pUserDailyRank.UPCounterBles);

	return true;
}

bool CDBAgent::LoadDailyRank()
{
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	if (!dbCommand->Execute(_T("{CALL LOAD_DAILY_RANK}"))) { ReportSQLError(GetGameDB()->GetError()); return false; }

	do
	{
		int field = 1, diff = 0;
		_DAILY_RANK* pData = new _DAILY_RANK();
		dbCommand->FetchString(field++, pData->strUserID);
		for (int i = 0; i < 3; i++) dbCommand->FetchUInt32(field++, pData->GmRank[i]);
		dbCommand->FetchUInt64(field++, pData->GMTotalSold);
		for (int i = 0; i < 3; i++) dbCommand->FetchUInt32(field++, pData->MhRank[i]);
		dbCommand->FetchUInt64(field++, pData->MHTotalKill);
		for (int i = 0; i < 3; i++) dbCommand->FetchUInt32(field++, pData->ShRank[i]);
		dbCommand->FetchUInt64(field++, pData->SHTotalExchange);
		for (int i = 0; i < 3; i++) dbCommand->FetchUInt32(field++, pData->AkRank[i]);
		dbCommand->FetchUInt64(field++, pData->AKLoyaltyMonthly);
		for (int i = 0; i < 3; i++) dbCommand->FetchUInt32(field++, pData->CwRank[i]);
		dbCommand->FetchUInt64(field++, pData->CWCounterWin);
		for (int i = 0; i < 3; i++) dbCommand->FetchUInt32(field++, pData->UpRank[i]);
		dbCommand->FetchUInt64(field++, pData->UPCounterBles);
		if (!g_pMain->m_DailyRank.PutData(pData->strUserID, pData)) { delete pData; return false; }
	} while (dbCommand->MoveNext());

	return true;
}

bool CDBAgent::SaveDailyRank(std::string& strCharID, CUser* pUser)
{
	if (pUser == nullptr || strCharID != pUser->GetName()) return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_DAILY_RANK(?, %d, %d, %d, %d, %d)}"),
		pUser->pUserDailyRank.GMTotalSold, pUser->pUserDailyRank.MHTotalKill, pUser->pUserDailyRank.SHTotalExchange,
		pUser->pUserDailyRank.CWCounterWin, pUser->pUserDailyRank.UPCounterBles))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return true;
}

bool CDBAgent::LoadPusRefund(CUser *pthis) {
	if (!pthis || pthis->GetAccountName().empty()) return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, pthis->GetAccountName().c_str(), pthis->GetAccountName().length());
	if (!dbCommand->Execute(_T("{CALL LOAD_USER_PUSREFUND_LIST(?)}"))) { ReportSQLError(GetGameDB()->GetError()); return false; }
	if (!dbCommand->hasData()) return true;

	do
	{
		int field = 1; uint64 serial = 0;
		_PUS_REFUND *pref = new _PUS_REFUND();
		dbCommand->FetchUInt64(field++, serial);
		dbCommand->FetchString(field++, pref->accountid);
		dbCommand->FetchUInt32(field++, pref->itemid);
		dbCommand->FetchUInt16(field++, pref->itemcount);
		dbCommand->FetchUInt32(field++, pref->itemprice);
		dbCommand->FetchUInt32(field++, pref->expiredtime);
		dbCommand->FetchUInt16(field++, pref->itemduration);
		dbCommand->FetchByte(field++, pref->buytype);
		if (UNIXTIME > pref->expiredtime) { delete pref; continue; }
		if (!pthis->m_PusRefundMap.PutData(serial, pref)) { delete pref; return false; }
	} while (dbCommand->MoveNext());
	return true;
}

bool CDBAgent::PusRefundItemDelete(CUser *pthis, uint64 serial, uint32 itemid) {
	if (!pthis) return false;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, pthis->GetAccountName().c_str(), pthis->GetAccountName().length());
	if (!dbCommand->Execute(string_format(_T("{CALL PUSREFUND_ITEM_DELETE(?, %I64d, %d)}"), serial,itemid))) { ReportSQLError(GetGameDB()->GetError()); return false; }
	return true;
}

bool CDBAgent::PusRefundItemAdd(CUser *pthis, uint64 serial, uint32 itemid, uint16 itemcount, uint16 itemduration, uint32 kcprice, uint32 buyingtime, uint8 buytype) {
	if (!pthis || pthis->GetAccountName().empty()) return false;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, pthis->GetAccountName().c_str(), pthis->GetAccountName().length());
	if (!dbCommand->Execute(string_format(_T("{CALL PUSREFUND_ITEM_ADD(?, %I64d,%d,%d,%d,%d,%d,%d)}"), serial, itemid, itemcount, itemduration, kcprice, buyingtime, buytype))) { ReportSQLError(GetGameDB()->GetError()); return false; }
	return true;
}

bool CDBAgent::PusGiftCheck(CUser *pUser, std::string targetname) {
	bool bRet = false;
	if (!pUser || targetname.empty()) return bRet;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, targetname.c_str(), targetname.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL PUSGIFT_CHECK(?)}")))) { ReportSQLError(GetGameDB()->GetError()); return false; }
	return true;
}

bool CDBAgent::SaveClanNationTransferUser(std::string strUserID, std::string strAccountID, uint16 ClanID, Nation newnation, uint8 newrace, uint8 newclass) {

	int bRet = 0;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL NATION_CLANTRANSFER_SAVEUSER (?, ?, %d, %d, %d, %d)}"), ClanID, (uint8)newnation, newrace, newclass))) { ReportSQLError(GetGameDB()->GetError()); return bRet; }
	return true;
}

bool CDBAgent::SaveCNTSKnights(uint16 ClanID, Nation newnation) {
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS SET Nation = %d WHERE IDNum= %d"), (uint8)newnation, ClanID))) { ReportSQLError(GetGameDB()->GetError()); return false; }
	return true;
}

uint8 CDBAgent::UpdateNtsCommand(std::string strAccountID, std::string strCharID, uint8 bnewRace, uint16 bNewClass, uint8 bNewNation)
{
	int8 bRet = 0;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());


	if (!dbCommand->Execute(string_format(_T("{? = CALL UPDATE_NTSCOMMAND(?, ?, %d, %d, %d)}"), bnewRace, bNewClass, bNewNation)))
		ReportSQLError(GetGameDB()->GetError());

	return bRet;
}

bool CDBAgent::LoadTBUserData(std::string strAccountID, CUser* pUser)
{
	if (pUser == nullptr)  return false;

	std::unique_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == nullptr)  return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("{CALL LOAD_TBUSER(?)}"))) {
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}

	if (!dbCommand->hasData())  return false;

	int field = 1; uint8 lregister = 0;
	dbCommand->FetchString(field++, pUser->pUserTbInfo.sealpass);
	dbCommand->FetchString(field++, pUser->pUserTbInfo.accountpass);
	return true;
}

uint8 CDBAgent::UpdateUserSealItem(uint64 nItemSerial, uint32 nItemID, uint8 bSealType, uint8 prelockstate, CUser* pUser)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 3;

	uint8 bRet = 1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetAccountName().c_str(), pUser->GetAccountName().length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL UPDATE_USER_ITEM_SEAL(?, ?, " I64FMTD ", %d, %d, %d)}"), nItemSerial, nItemID, bSealType, prelockstate))) {
		ReportSQLError(GetGameDB()->GetError());
		return 3;
	}
	return bRet;
}

bool CDBAgent::BotMerchantSaveCoord(uint16 areaType, uint16 x, uint16 z, int16 y, uint8 zoneid, int16 direction) {
	
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (!dbCommand->Execute(string_format(_T("INSERT INTO MERCHANT_BOT_INFO (type,setX,setY,setZ,bZoneID,direction,isbuy) VALUES (%d,%d,%d,%d,%d,%d,%d)"), areaType,x,y,z, zoneid, direction,1)))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	return true;
}

bool CDBAgent::LoadPerksData(std::string strUserID, CUser* pUser)
{
	if (pUser == nullptr || strUserID.empty())
		return false;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strUserID.c_str(), strUserID.length());
	if (!dbCommand->Execute(_T("{CALL LOAD_PERKS_DATA(?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	int field = 1;
	std::string name;
	dbCommand->FetchString(field++, name);
	for (int i = 0; i < PERK_COUNT; i++)
		dbCommand->FetchUInt16(field++, pUser->pPerks.perkType[i]);
	dbCommand->FetchUInt16(field++, pUser->pPerks.remPerk);
	return true;
}

bool CDBAgent::UpdateUserPerks(CUser* pUser)
{
	if (!pUser || pUser->GetName().empty())
		return false;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName().c_str(), pUser->GetName().length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_PERKS(?,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)}"), pUser->pPerks.remPerk,
		pUser->pPerks.perkType[0], pUser->pPerks.perkType[1], pUser->pPerks.perkType[2], pUser->pPerks.perkType[3],
		pUser->pPerks.perkType[4], pUser->pPerks.perkType[5], pUser->pPerks.perkType[6], pUser->pPerks.perkType[7],
		pUser->pPerks.perkType[8], pUser->pPerks.perkType[9], pUser->pPerks.perkType[10], pUser->pPerks.perkType[11],
		pUser->pPerks.perkType[12]))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return true;
}

int8_t CDBAgent::ChangeClanNation(std::string& strClanName)
{
	int8_t bRet = 1;
	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr)
		return bRet;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strClanName.c_str(), strClanName.length());
	if (!dbCommand->Execute(string_format(_T("{? = CALL CLAN_NATION_TRANSFER(?)}"))))
	{
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return bRet;
}

// =====================================================================
// S115 TUR 9 — Clan Tournament DB Log (MATRIX MSG:5897 SP'leri)
// =====================================================================

// SP_CLAN_TOURNAMENT_START — yeni tournament kaydi olustur, ID dondurur
// Returns: yeni TournamentID (>0 basari, <=0 hata)
int32_t CDBAgent::TournamentLogStart(uint8 zoneID, uint16 redClanID, uint16 blueClanID,
                                     const std::string& redClanName, const std::string& blueClanName,
                                     uint16 durationMinutes, const std::string& startedByGM)
{
	// AddParameter pointer alir — local kopyalari pointer'lari icin tut
	uint8  pZoneID    = zoneID;
	int16  pRedClanID = (int16)redClanID;   // SMALLINT
	int16  pBlueClanID= (int16)blueClanID;
	int16  pDuration  = (int16)durationMinutes;
	int32  pTournamentID = 0;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRedClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBlueClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, redClanName.c_str(),  redClanName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, blueClanName.c_str(), blueClanName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pDuration);
	dbCommand->AddParameter(SQL_PARAM_INPUT, startedByGM.c_str(),  startedByGM.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pTournamentID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_CLAN_TOURNAMENT_START(?, ?, ?, ?, ?, ?, ?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}
	while (dbCommand->MoveNext()) {}
	return pTournamentID;
}

// SP_CLAN_TOURNAMENT_FINISH — tournament sonucunu DB'ye yaz
// winnerClanID = 0 ise berabere (MATRIX SP berabere icin NULL kullanir, biz 0 gonderiyoruz; sema check)
bool CDBAgent::TournamentLogFinish(int32_t tournamentID, uint16 redScore, uint16 blueScore,
                                   uint8 monumentKilled, uint16 winnerClanID)
{
	if (tournamentID <= 0) return false;

	int32 pTournamentID  = tournamentID;
	int16 pRedScore      = (int16)redScore;
	int16 pBlueScore     = (int16)blueScore;
	uint8 pMonumentKilled= monumentKilled;
	int16 pWinnerClanID  = (int16)winnerClanID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pTournamentID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRedScore);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBlueScore);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMonumentKilled);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pWinnerClanID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_CLAN_TOURNAMENT_FINISH(?, ?, ?, ?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return true;
}

// =====================================================================
// S115 TUR 8 — Tournament Bet DB (MATRIX MSG:5907 SP'leri)
// =====================================================================

// SP_TOURNAMENT_BET_PLACE — yeni bahis kaydet, ID dondurur
int32_t CDBAgent::TournamentBetPlace(uint8 zoneID, const std::string& betterAccountID,
                                     const std::string& betterCharName, uint16 betClanID,
                                     const std::string& betClanName, int32_t betAmount)
{
	uint8 pZoneID    = zoneID;
	int16 pClanID    = (int16)betClanID;
	int32 pAmount    = betAmount;
	int32 pNewBetID  = 0;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, betterAccountID.c_str(), betterAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, betterCharName.c_str(),  betterCharName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, betClanName.c_str(),     betClanName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pAmount);
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pNewBetID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_BET_PLACE(?, ?, ?, ?, ?, ?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}
	while (dbCommand->MoveNext()) {}
	return pNewBetID;
}

// SP_TOURNAMENT_BET_RESOLVE — havuzu dagit (kazananlar 2x, kaybedenler kaybetti)
// Server bunu sadece DB'ye yazıyor; oyuncuya odeme RAM'den (TournamentBet.cpp::ResolveTournamentBets) yapilir
bool CDBAgent::TournamentBetResolve(uint8 zoneID, uint16 winnerClanID)
{
	uint8 pZoneID         = zoneID;
	int16 pWinnerClanID   = (int16)winnerClanID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pWinnerClanID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_BET_RESOLVE(?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	while (dbCommand->MoveNext()) {}  // returned rows tuket
	return true;
}

// SP_TOURNAMENT_BET_REFUND — tournament iptal/berabere, bahisleri iade et
bool CDBAgent::TournamentBetRefund(uint8 zoneID)
{
	uint8 pZoneID = zoneID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_BET_REFUND(?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	while (dbCommand->MoveNext()) {}
	return true;
}

// S115 — oyuncu kendi bahsini iptal (direkt UPDATE, SP gerek yok)
// ACTIVE durumdaki bahisleri CANCELLED yapar (oyuncu +bet cancel ile)
bool CDBAgent::TournamentBetCancelUser(uint8 zoneID, const std::string& betterAccountID)
{
	uint8 pZoneID = zoneID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, betterAccountID.c_str(), betterAccountID.length());

	// Status='ACTIVE' olan bu account+zone bahislerini CANCELLED yap
	return dbCommand->Execute(_T("UPDATE KO_LOG.dbo._MK_TOURNAMENT_BETS SET Status = 'CANCELLED', ResolvedTime = GETDATE() WHERE ZoneID = ? AND BetterAccountID = ? AND Status = 'ACTIVE'"));
}

// =====================================================================
// S115 TUR 11 — Tournament Registration DB (MATRIX MSG:5907 SP'leri)
// =====================================================================

// SP_TOURNAMENT_REG_INSERT — klan kayit (NPC + Web + Chat ayni)
// Returns: 0 = ALREADY_REGISTERED veya hata, >0 = yeni RegID
int32_t CDBAgent::TournamentRegInsert(uint16 clanID, const std::string& clanName,
                                      const std::string& leaderName, const std::string& leaderAccountID,
                                      const std::string& registeredVia)
{
	int16 pClanID    = (int16)clanID;
	int32 pNewRegID  = 0;
	// Result string: NVARCHAR — biz string olarak fetch etmek istemiyoruz, varchar buffer
	char pResult[64] = {0};

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, clanName.c_str(),         clanName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, leaderName.c_str(),       leaderName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, leaderAccountID.c_str(),  leaderAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, registeredVia.c_str(),    registeredVia.length());
	// @TournamentDate NULL — pass empty/null (skip optional)
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pNewRegID);
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, pResult, sizeof(pResult));

	// MATRIX SP imzasi: @ClanID, @ClanName, @LeaderName, @LeaderAccountID, @RegisteredVia, @TournamentDate=NULL, @NewRegID OUT, @Result OUT
	// @TournamentDate icin NULL gondermek karmasik — su an default null bekleyen SP
	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_REG_INSERT(?, ?, ?, ?, ?, NULL, ?, ?)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return 0;
	}
	while (dbCommand->MoveNext()) {}
	return pNewRegID;
}

// SP_TOURNAMENT_REG_CANCEL — klan kaydini iptal et
bool CDBAgent::TournamentRegCancel(uint16 clanID)
{
	int16 pClanID = (int16)clanID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_REG_CANCEL(?, NULL)}"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}
	return true;
}

// =====================================================================
// S115 — Tournament Reward tablosu (DB-driven, hardcoded yerine)
// _MK_TOURNAMENT_REWARDS: (ZoneID, Position, ItemID, Count, Enabled)
// Position: 'WINNER', 'LOSER', 'DRAW'
// =====================================================================
bool CDBAgent::LoadTournamentRewards(uint8 zoneID, const std::string& position,
                                     std::vector<std::pair<uint32, uint16>>& rewards)
{
	rewards.clear();
	uint8 pZoneID = zoneID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, position.c_str(), position.length());

	if (!dbCommand->Execute(_T("SELECT ItemID, Count FROM KO_LOG.dbo._MK_TOURNAMENT_REWARDS WHERE ZoneID = ? AND Position = ? AND Enabled = 1"))) {
		ReportSQLError(GetGameDB()->GetError());
		return false;
	}

	while (dbCommand->MoveNext()) {
		int32 itemID = 0;
		int16 count = 0;
		dbCommand->FetchInt32(1, itemID);
		dbCommand->FetchInt16(2, count);
		if (itemID > 0 && count > 0)
			rewards.push_back(std::make_pair((uint32)itemID, (uint16)count));
	}

	return true;
}

// =====================================================================
// S115 — Bracket Tournament DB (MATRIX MSG:5914 SP'leri — 10 SP)
// SU AN COMMENT — MATRIX bittikten sonra uncomment edilecek.
// Comment'siz hata yontemli: SP yoksa Execute false dondurur, fonksiyon
// false/0 doner, RAM-only bracket iskelet calismaya devam eder.
// =====================================================================

// SP_BRACKET_CREATE — yeni bracket olustur, ID dondurur
int32_t CDBAgent::BracketCreate(const std::string& name, uint8 maxClans, const std::string& gm)
{
	int32 pMaxClans = (int32)maxClans;
	int32 pNewBracketID = 0;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, name.c_str(), name.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMaxClans);
	dbCommand->AddParameter(SQL_PARAM_INPUT, gm.c_str(), gm.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pNewBracketID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_CREATE(?, ?, ?, ?)}"))) {
		// SP yoksa hata sessiz — RAM iskelet calisir
		return 0;
	}
	while (dbCommand->MoveNext()) {}
	return pNewBracketID;
}

// SP_BRACKET_REGISTER — klan kayit (max + duplicate kontrol)
bool CDBAgent::BracketRegister(int32_t bracketID, uint16 clanID,
                                const std::string& clanName, const std::string& leaderName,
                                std::string& outResult)
{
	int32 pBracketID = bracketID;
	int16 pClanID    = (int16)clanID;
	char  pResult[64] = {0};

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, clanName.c_str(),   clanName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, leaderName.c_str(), leaderName.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, pResult, sizeof(pResult));

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_REGISTER(?, ?, ?, ?, ?)}"))) {
		outResult = "DB_ERROR";
		return false;
	}
	while (dbCommand->MoveNext()) {}
	outResult = pResult;
	return (outResult == "OK");
}

// SP_BRACKET_GENERATE_MATCHES — Round 1 maclari olustur
bool CDBAgent::BracketGenerateMatches(int32_t bracketID)
{
	int32 pBracketID = bracketID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_GENERATE_MATCHES(?)}"))) {
		return false;
	}
	while (dbCommand->MoveNext()) {}
	return true;
}

// SP_BRACKET_MATCH_FINISH — mac sonucu yaz
bool CDBAgent::BracketMatchFinish(int32_t matchID, uint16 redScore, uint16 blueScore, uint16 winnerClanID)
{
	int32 pMatchID = matchID;
	int16 pRedScore = (int16)redScore;
	int16 pBlueScore = (int16)blueScore;
	int16 pWinnerClanID = (int16)winnerClanID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMatchID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRedScore);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBlueScore);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pWinnerClanID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_MATCH_FINISH(?, ?, ?, ?)}"))) {
		return false;
	}
	return true;
}

// SP_BRACKET_NEXT_ROUND_GENERATE — sonraki tur maclari olustur
bool CDBAgent::BracketNextRoundGenerate(int32_t bracketID, uint8 currentRound)
{
	int32 pBracketID = bracketID;
	uint8 pCurrentRound = currentRound;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pCurrentRound);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_NEXT_ROUND_GENERATE(?, ?)}"))) {
		return false;
	}
	return true;
}

// SP_BRACKET_CANCEL — bracket iptal (active maclar WALKOVER)
bool CDBAgent::BracketCancel(int32_t bracketID)
{
	int32 pBracketID = bracketID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_CANCEL(?, NULL)}"))) {
		return false;
	}
	return true;
}

// SP_BRACKET_GET_REWARDS — pozisyon basina odul
bool CDBAgent::BracketGetRewards(int32_t bracketID, const std::string& position,
                                  int32_t& goldOut, int32_t& npOut, int32_t& itemIDOut,
                                  int16_t& itemCountOut, int32_t& premiumHoursOut)
{
	goldOut = npOut = itemIDOut = premiumHoursOut = 0;
	itemCountOut = 0;
	int32 pBracketID = bracketID;

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, position.c_str(), position.length());

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_GET_REWARDS(?, ?)}"))) {
		return false;
	}

	bool found = false;
	while (dbCommand->MoveNext()) {
		int32 g = 0, n = 0, iid = 0, ph = 0;
		int16 ic = 0;
		dbCommand->FetchInt32(1, g);
		dbCommand->FetchInt32(2, n);
		dbCommand->FetchInt32(3, iid);
		dbCommand->FetchInt16(4, ic);
		dbCommand->FetchInt32(5, ph);
		goldOut = g; npOut = n; itemIDOut = iid; itemCountOut = ic; premiumHoursOut = ph;
		found = true;
	}
	return found;
}

// SP_BRACKET_PARTY_MEMBER_ADD — 8v8 hazirligi (su an pasif)
bool CDBAgent::BracketPartyMemberAdd(int32_t bracketID, uint16 clanID,
                                      const std::string& memberCharName, uint8 partyNumber,
                                      const std::string& assignedBy, std::string& outResult)
{
	int32 pBracketID = bracketID;
	int16 pClanID = (int16)clanID;
	uint8 pPartyNumber = partyNumber;
	char  pResult[64] = {0};

	unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, memberCharName.c_str(), memberCharName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pPartyNumber);
	dbCommand->AddParameter(SQL_PARAM_INPUT, assignedBy.c_str(), assignedBy.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, pResult, sizeof(pResult));

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_PARTY_MEMBER_ADD(?, ?, ?, ?, ?, ?)}"))) {
		outResult = "DB_ERROR";
		return false;
	}
	while (dbCommand->MoveNext()) {}
	outResult = pResult;
	return (outResult == "OK");
}

// S115 — 8v8 zone giris kontrolu (MATRIX brief MSG:5929 108b)
// SELECT EXISTS(...) — kullanici listede mi? Tek row TEK kolon (0/1)
bool CDBAgent::BracketPartyMemberCheck(int32_t bracketID, uint16 clanID,
                                       const std::string& memberCharName)
{
	int32 pBracketID = bracketID;
	int16 pClanID = (int16)clanID;
	int32 pIsAssigned = 0;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, memberCharName.c_str(), memberCharName.length());

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_PARTY_MEMBER_CHECK(?, ?, ?)}"))) {
		// SP yok veya hata — caller fail-open/closed karari kendisi verir
		return false;
	}

	if (dbCommand->MoveNext()) {
		dbCommand->FetchInt32(1, pIsAssigned);
	}
	return (pIsAssigned != 0);
}

// S115 — 8v8 klan uye listesi (MATRIX brief MSG:5929 108b)
// SELECT N row: PartyMemberID, PartyNumber, MemberCharName, AssignedBy, AddedAt
bool CDBAgent::BracketPartyMemberList(int32_t bracketID, uint16 clanID,
                                      std::vector<_BPM_LIST_ROW>& outRows)
{
	int32 pBracketID = bracketID;
	int16 pClanID = (int16)clanID;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_BRACKET_PARTY_MEMBER_LIST(?, ?)}"))) {
		return false;
	}

	while (dbCommand->MoveNext()) {
		_BPM_LIST_ROW row;
		int32 pid = 0;
		uint8 pn = 0;
		char  mcn[64] = {0};
		char  ab[16] = {0};
		char  ad[32] = {0};

		dbCommand->FetchInt32(1, pid);
		dbCommand->FetchByte(2, pn);
		dbCommand->FetchString(3, mcn, sizeof(mcn));
		dbCommand->FetchString(4, ab, sizeof(ab));
		dbCommand->FetchString(5, ad, sizeof(ad));

		row.partyMemberID  = pid;
		row.partyNumber    = pn;
		row.memberCharName = mcn;
		row.assignedBy     = ab;
		row.addedAt        = ad;
		outRows.push_back(row);
	}
	return true;
}

// Server init aktif bracket listesi (REGISTRATION + ACTIVE durumda olanlar)
bool CDBAgent::BracketLoadActive(std::vector<_BRACKET_INFO_ROW>& outRows)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	if (!dbCommand->Execute(_T("SELECT BracketID, Name, MaxClans, CurrentRound, Status, ISNULL(WinnerClanID, 0), ISNULL(WinnerClanName, '') FROM KO_LOG.dbo._MK_BRACKET_TOURNAMENT WHERE Status IN ('REGISTRATION', 'ACTIVE')"))) {
		return false;
	}

	while (dbCommand->MoveNext()) {
		_BRACKET_INFO_ROW row;
		int32 bid = 0;
		uint8 mc = 0, cr = 0;
		int16 wcl = 0;
		char name[64] = {0}, st[32] = {0}, wname[64] = {0};

		dbCommand->FetchInt32(1, bid);
		dbCommand->FetchString(2, name, sizeof(name));
		dbCommand->FetchByte(3, mc);
		dbCommand->FetchByte(4, cr);
		dbCommand->FetchString(5, st, sizeof(st));
		dbCommand->FetchInt16(6, wcl);
		dbCommand->FetchString(7, wname, sizeof(wname));

		row.bracketID = bid;
		row.name = name;
		row.maxClans = mc;
		row.currentRound = cr;
		row.status = st;
		row.winnerClanID = (uint16)wcl;
		row.winnerClanName = wname;
		outRows.push_back(row);
	}
	return true;
}

// 8v8 uye sil — direkt DELETE (klan lideri yanlis ekledigini geri alir)
// MATRIX SP'si yok, dogrudan SQL: BracketID + ClanID + MemberCharName ile filtre.
// Returns: true=sildi (1+ row), false=hata veya yok
bool CDBAgent::BracketPartyMemberDelete(int32_t bracketID, uint16 clanID,
                                        const std::string& memberCharName)
{
	int32 pBracketID = bracketID;
	int16 pClanID = (int16)clanID;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, memberCharName.c_str(), memberCharName.length());

	return dbCommand->Execute(_T("DELETE FROM KO_LOG.dbo._MK_BRACKET_PARTY_MEMBERS WHERE BracketID = ? AND ClanID = ? AND MemberCharName = ?"));
}

// S115 BUG #2 FIX — RAM cache fill (Bracket mac listesi)
// SELECT _MK_BRACKET_MATCHES WHERE BracketID = ? ORDER BY RoundNumber, MatchOrder
bool CDBAgent::BracketLoadMatches(int32_t bracketID, std::vector<_BRACKET_MATCH_ROW>& outRows)
{
	int32 pBracketID = bracketID;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBracketID);

	if (!dbCommand->Execute(_T("SELECT MatchID, RoundNumber, MatchOrder, RedClanID, BlueClanID, ZoneID, Status, ISNULL(WinnerClanID, 0), ISNULL(DependsOnMatch1, 0) FROM KO_LOG.dbo._MK_BRACKET_MATCHES WHERE BracketID = ? ORDER BY RoundNumber, MatchOrder"))) {
		return false;
	}

	while (dbCommand->MoveNext()) {
		_BRACKET_MATCH_ROW row;
		int32 mid = 0, dep = 0;
		uint8 rnd = 0, ord = 0, zid = 0;
		int16 rcl = 0, bcl = 0, wcl = 0;
		char  st[32] = {0};

		dbCommand->FetchInt32(1, mid);
		dbCommand->FetchByte(2, rnd);
		dbCommand->FetchByte(3, ord);
		dbCommand->FetchInt16(4, rcl);
		dbCommand->FetchInt16(5, bcl);
		dbCommand->FetchByte(6, zid);
		dbCommand->FetchString(7, st, sizeof(st));
		dbCommand->FetchInt16(8, wcl);
		dbCommand->FetchInt32(9, dep);

		row.matchID = mid;
		row.roundNumber = rnd;
		row.matchOrder = ord;
		row.redClanID = (uint16)rcl;
		row.blueClanID = (uint16)bcl;
		row.zoneID = zid;
		row.status = st;
		row.winnerClanID = (uint16)wcl;
		row.dependsOnMatch1 = dep;
		outRows.push_back(row);
	}
	return true;
}

// =====================================================================
// S115 FAZ 13 — Crystal CTF DB (MATRIX migration 110_crystal_ctf_db)
// =====================================================================
int32_t CDBAgent::CTFStart(uint8 zoneID, uint16 redClanID, const std::string& redName,
                            uint16 blueClanID, const std::string& blueName)
{
	uint8 pZoneID = zoneID;
	int16 pRedClanID = (int16)redClanID;
	int16 pBlueClanID = (int16)blueClanID;
	int32 pCTFID = 0;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pZoneID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRedClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, redName.c_str(), redName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBlueClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, blueName.c_str(), blueName.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pCTFID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_CTF_START(?, ?, ?, ?, ?, ?)}"))) {
		return 0;
	}
	while (dbCommand->MoveNext()) {}
	return pCTFID;
}

bool CDBAgent::CTFFinish(int32_t ctfID, uint16 winnerClanID, uint8 redScore, uint8 blueScore)
{
	int32 pCTFID = ctfID;
	int16 pWinner = (int16)winnerClanID;
	uint8 pRed = redScore;
	uint8 pBlue = blueScore;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pCTFID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pWinner);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRed);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBlue);

	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_CTF_FINISH(?, ?, ?, ?)}"));
}

bool CDBAgent::CTFFlagLog(int32_t ctfID, int32_t userID, const std::string& userName,
                          const std::string& action)
{
	int32 pCTFID = ctfID;
	int32 pUserID = userID;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pCTFID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pUserID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, userName.c_str(), userName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, action.c_str(), action.length());

	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_CTF_FLAG_PICKUP(?, ?, ?, ?)}"));
}

// =====================================================================
// S115 FAZ 15 — Klan Sponsorlugu DB (MATRIX migration 111_clan_sponsorship)
// =====================================================================
int32_t CDBAgent::ClanSponsorDeposit(uint16 clanID, int32_t userID, const std::string& userName,
                                      int64_t noahAmount, int32_t npAmount)
{
	int16 pClanID = (int16)clanID;
	int32 pUserID = userID;
	int64 pNoah = noahAmount;
	int32 pNP = npAmount;
	int32 pNewSponsorID = 0;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pUserID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, userName.c_str(), userName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pNoah);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pNP);
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pNewSponsorID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_CLAN_SPONSOR_DEPOSIT(?, ?, ?, ?, ?, ?)}"))) {
		return 0;
	}
	while (dbCommand->MoveNext()) {}
	return pNewSponsorID;
}

// =====================================================================
// S115 FAZ 16 — 1v1 Bracket DB (MATRIX migration 112_1v1_bracket)
// =====================================================================
int32_t CDBAgent::OneVsOneCreate(const std::string& name, uint8 maxPlayers, const std::string& gm)
{
	uint8 pMax = maxPlayers;
	int32 pBID = 0;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, name.c_str(), name.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMax);
	dbCommand->AddParameter(SQL_PARAM_INPUT, gm.c_str(), gm.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pBID);

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_1V1_CREATE(?, ?, ?, ?)}"))) return 0;
	while (dbCommand->MoveNext()) {}
	return pBID;
}

// 1v1 RAM cache fill — match listesi (BracketLoadMatches mantigi)
bool CDBAgent::OneVsOneLoadMatches(int32_t bid, std::vector<_1V1_MATCH_ROW>& outRows)
{
	int32 pBID = bid;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBID);

	if (!dbCommand->Execute(_T("SELECT MatchID, RoundNumber, MatchOrder, RedUserID, BlueUserID, RedName, BlueName, ZoneID, Status, ISNULL(WinnerUserID, 0), ISNULL(DependsOnMatch1, 0) FROM KO_LOG.dbo._MK_1V1_MATCHES WHERE BID = ? ORDER BY RoundNumber, MatchOrder"))) {
		return false;
	}

	while (dbCommand->MoveNext()) {
		_1V1_MATCH_ROW row;
		int32 mid = 0, dep = 0, ruid = 0, buid = 0, wuid = 0;
		uint8 rnd = 0, ord = 0, zid = 0;
		char  rname[64] = {0}, bname[64] = {0}, st[32] = {0};

		dbCommand->FetchInt32(1, mid);
		dbCommand->FetchByte(2, rnd);
		dbCommand->FetchByte(3, ord);
		dbCommand->FetchInt32(4, ruid);
		dbCommand->FetchInt32(5, buid);
		dbCommand->FetchString(6, rname, sizeof(rname));
		dbCommand->FetchString(7, bname, sizeof(bname));
		dbCommand->FetchByte(8, zid);
		dbCommand->FetchString(9, st, sizeof(st));
		dbCommand->FetchInt32(10, wuid);
		dbCommand->FetchInt32(11, dep);

		row.matchID = mid;
		row.roundNumber = rnd;
		row.matchOrder = ord;
		row.redUserID = ruid;
		row.blueUserID = buid;
		row.redName = rname;
		row.blueName = bname;
		row.zoneID = zid;
		row.status = st;
		row.winnerUserID = wuid;
		row.dependsOnMatch1 = dep;
		outRows.push_back(row);
	}
	return true;
}

// 1v1 bracket listesi (ACTIVE / OPEN olanlar)
bool CDBAgent::OneVsOneLoadBrackets(std::vector<_1V1_INFO_ROW>& outRows)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	if (!dbCommand->Execute(_T("SELECT BID, Name, MaxPlayers, CurrentRound, Status, ISNULL(WinnerUserID, 0), ISNULL(WinnerName, '') FROM KO_LOG.dbo._MK_1V1_TOURNAMENT WHERE Status IN ('OPEN', 'STARTED')"))) {
		return false;
	}

	while (dbCommand->MoveNext()) {
		_1V1_INFO_ROW row;
		int32 bid = 0, wuid = 0;
		uint8 mp = 0, cr = 0;
		char name[64] = {0}, st[32] = {0}, wname[64] = {0};

		dbCommand->FetchInt32(1, bid);
		dbCommand->FetchString(2, name, sizeof(name));
		dbCommand->FetchByte(3, mp);
		dbCommand->FetchByte(4, cr);
		dbCommand->FetchString(5, st, sizeof(st));
		dbCommand->FetchInt32(6, wuid);
		dbCommand->FetchString(7, wname, sizeof(wname));

		row.bid = bid;
		row.name = name;
		row.maxPlayers = mp;
		row.currentRound = cr;
		row.status = st;
		row.winnerUserID = wuid;
		row.winnerName = wname;
		outRows.push_back(row);
	}
	return true;
}

bool CDBAgent::OneVsOneRegister(int32_t bid, int32_t userID, const std::string& userName,
                                 uint8 cls, int16_t level, std::string& outResult)
{
	int32 pBID = bid;
	int32 pUserID = userID;
	uint8 pCls = cls;
	int16 pLevel = level;
	char  pResult[64] = {0};

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pUserID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, userName.c_str(), userName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pCls);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pLevel);
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, pResult, sizeof(pResult));

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_1V1_REGISTER(?, ?, ?, ?, ?, ?)}"))) {
		outResult = "DB_ERROR";
		return false;
	}
	while (dbCommand->MoveNext()) {}
	outResult = pResult;
	return (outResult == "OK");
}

bool CDBAgent::OneVsOneGenerateMatches(int32_t bid)
{
	int32 pBID = bid;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBID);
	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_1V1_GENERATE_MATCHES(?)}"));
}

bool CDBAgent::OneVsOneMatchFinish(int32_t matchID, int32_t winnerUserID)
{
	int32 pMID = matchID;
	int32 pWin = winnerUserID;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pWin);
	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_1V1_MATCH_FINISH(?, ?)}"));
}

bool CDBAgent::OneVsOneNextRoundGenerate(int32_t bid, uint8 currentRound)
{
	int32 pBID = bid;
	uint8 pRound = currentRound;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRound);
	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_1V1_NEXT_ROUND_GENERATE(?, ?)}"));
}

bool CDBAgent::OneVsOneCancel(int32_t bid)
{
	int32 pBID = bid;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBID);

	// SP varsa onu kullan, yoksa direkt UPDATE fallback (MATRIX tasariminda SP atlanmis,
	// MSG:5939 ile bildirildi; SP ekleninceye kadar UPDATE garanti calisir).
	if (dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_1V1_CANCEL(?)}"))) {
		return true;
	}

	// Fallback — direkt UPDATE (SP yoksa hata almaz)
	std::unique_ptr<OdbcCommand> dbCommand2(GetGameDB()->CreateCommand());
	if (dbCommand2.get() == nullptr) return false;
	dbCommand2->AddParameter(SQL_PARAM_INPUT, &pBID);
	return dbCommand2->Execute(_T("UPDATE KO_LOG.dbo._MK_1V1_TOURNAMENT SET Status = 'CANCELLED', EndTime = GETDATE() WHERE BID = ? AND Status IN ('OPEN', 'STARTED')"));
}

// =====================================================================
// S115 — Tournament Command Queue (GM web manuel kontrol, MATRIX 115)
// =====================================================================
bool CDBAgent::TournamentCommandNextPending(std::vector<_TOURNAMENT_CMD_ROW>& outRows)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_CMD_NEXT_PENDING}"))) {
		return false;
	}

	while (dbCommand->MoveNext()) {
		_TOURNAMENT_CMD_ROW row;
		int32 cid = 0;
		char ct[40] = {0}, pp[520] = {0}, rb[40] = {0};

		dbCommand->FetchInt32(1, cid);
		dbCommand->FetchString(2, ct, sizeof(ct));
		dbCommand->FetchString(3, pp, sizeof(pp));
		dbCommand->FetchString(4, rb, sizeof(rb));

		row.commandID   = cid;
		row.commandType = ct;
		row.params      = pp;
		row.requestedBy = rb;
		outRows.push_back(row);
	}
	return true;
}

bool CDBAgent::TournamentCommandUpdateResult(int32_t commandID,
                                              const std::string& status,
                                              const std::string& result)
{
	int32 pCommandID = commandID;

	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, &pCommandID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, status.c_str(), status.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, result.c_str(), result.length());

	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_TOURNAMENT_CMD_UPDATE_RESULT(?, ?, ?)}"));
}

// =====================================================================
// S115 — League (lig) round-robin DB (MATRIX migration 118)
// =====================================================================
int32_t CDBAgent::LeagueCreate(const std::string& name, uint8 maxClans, const std::string& gm)
{
	uint8 pMax = maxClans;
	int32 pLID = 0;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return 0;
	dbCommand->AddParameter(SQL_PARAM_INPUT, name.c_str(), name.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMax);
	dbCommand->AddParameter(SQL_PARAM_INPUT, gm.c_str(), gm.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pLID);
	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_LEAGUE_CREATE(?, ?, ?, ?)}"))) return 0;
	while (dbCommand->MoveNext()) {}
	return pLID;
}

bool CDBAgent::LeagueRegister(int32_t leagueID, uint16 clanID, const std::string& clanName,
                              const std::string& leaderName, std::string& outResult)
{
	int32 pLID = leagueID;
	int16 pClanID = (int16)clanID;
	char pResult[64] = {0};
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pLID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pClanID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, clanName.c_str(), clanName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, leaderName.c_str(), leaderName.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, pResult, sizeof(pResult));
	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_LEAGUE_REGISTER(?, ?, ?, ?, ?)}"))) {
		outResult = "DB_ERROR";
		return false;
	}
	while (dbCommand->MoveNext()) {}
	outResult = pResult;
	return (outResult == "OK");
}

bool CDBAgent::LeagueGenerateFixtures(int32_t leagueID)
{
	int32 pLID = leagueID;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pLID);
	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_LEAGUE_GENERATE_FIXTURES(?)}"));
}

bool CDBAgent::LeagueMatchFinish(int32_t matchID, uint16 redScore, uint16 blueScore, uint16 winnerClanID)
{
	int32 pMID = matchID;
	int16 pRed = (int16)redScore, pBlue = (int16)blueScore, pWin = (int16)winnerClanID;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pMID);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pRed);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pBlue);
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pWin);
	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_LEAGUE_MATCH_FINISH(?, ?, ?, ?)}"));
}

bool CDBAgent::LeagueCancel(int32_t leagueID)
{
	int32 pLID = leagueID;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pLID);
	return dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_LEAGUE_CANCEL(?)}"));
}

bool CDBAgent::LeagueLoadActive(std::vector<_LEAGUE_INFO_ROW>& outRows)
{
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	if (!dbCommand->Execute(_T("SELECT LeagueID, Name, MaxClans, CurrentRound, TotalRounds, Status, ISNULL(WinnerClanID,0), ISNULL(WinnerClanName,'') FROM KO_LOG.dbo._MK_LEAGUE WHERE Status IN ('REGISTRATION','ACTIVE')"))) {
		return false;
	}
	while (dbCommand->MoveNext()) {
		_LEAGUE_INFO_ROW row;
		int32 lid = 0; uint8 mc = 0, cr = 0, tr = 0; int16 wcl = 0;
		char name[64] = {0}, st[32] = {0}, wname[64] = {0};
		dbCommand->FetchInt32(1, lid);
		dbCommand->FetchString(2, name, sizeof(name));
		dbCommand->FetchByte(3, mc);
		dbCommand->FetchByte(4, cr);
		dbCommand->FetchByte(5, tr);
		dbCommand->FetchString(6, st, sizeof(st));
		dbCommand->FetchInt16(7, wcl);
		dbCommand->FetchString(8, wname, sizeof(wname));
		row.leagueID = lid; row.name = name; row.maxClans = mc;
		row.currentRound = cr; row.totalRounds = tr; row.status = st;
		row.winnerClanID = (uint16)wcl; row.winnerClanName = wname;
		outRows.push_back(row);
	}
	return true;
}

bool CDBAgent::LeagueLoadMatches(int32_t leagueID, std::vector<_LEAGUE_MATCH_ROW>& outRows)
{
	int32 pLID = leagueID;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pLID);
	if (!dbCommand->Execute(_T("SELECT MatchID, RoundNumber, MatchOrder, RedClanID, BlueClanID, RedClanName, BlueClanName, ZoneID, Status, ISNULL(WinnerClanID,0) FROM KO_LOG.dbo._MK_LEAGUE_MATCHES WHERE LeagueID = ? ORDER BY RoundNumber, MatchOrder"))) {
		return false;
	}
	while (dbCommand->MoveNext()) {
		_LEAGUE_MATCH_ROW row;
		int32 mid = 0; uint8 rnd = 0, ord = 0, zid = 0; int16 rcl = 0, bcl = 0, wcl = 0;
		char rname[64] = {0}, bname[64] = {0}, st[32] = {0};
		dbCommand->FetchInt32(1, mid);
		dbCommand->FetchByte(2, rnd);
		dbCommand->FetchByte(3, ord);
		dbCommand->FetchInt16(4, rcl);
		dbCommand->FetchInt16(5, bcl);
		dbCommand->FetchString(6, rname, sizeof(rname));
		dbCommand->FetchString(7, bname, sizeof(bname));
		dbCommand->FetchByte(8, zid);
		dbCommand->FetchString(9, st, sizeof(st));
		dbCommand->FetchInt16(10, wcl);
		row.matchID = mid; row.roundNumber = rnd; row.matchOrder = ord;
		row.redClanID = (uint16)rcl; row.blueClanID = (uint16)bcl;
		row.redName = rname; row.blueName = bname; row.zoneID = zid;
		row.status = st; row.winnerClanID = (uint16)wcl;
		outRows.push_back(row);
	}
	return true;
}

// S115 — Lig puan tablosu (SP_LEAGUE_STANDINGS)
bool CDBAgent::LeagueStandings(int32_t leagueID, std::vector<_LEAGUE_STANDING_ROW>& outRows)
{
	int32 pLID = leagueID;
	std::unique_ptr<OdbcCommand> dbCommand(GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) return false;
	dbCommand->AddParameter(SQL_PARAM_INPUT, &pLID);
	if (!dbCommand->Execute(_T("{CALL KO_LOG.dbo.SP_LEAGUE_STANDINGS(?)}"))) return false;
	while (dbCommand->MoveNext()) {
		_LEAGUE_STANDING_ROW row;
		char cname[64] = {0};
		uint8 pl = 0, w = 0, d = 0, ls = 0;
		int32 gf = 0, ga = 0, gd = 0, pts = 0;
		dbCommand->FetchString(1, cname, sizeof(cname));
		dbCommand->FetchByte(2, pl);
		dbCommand->FetchByte(3, w);
		dbCommand->FetchByte(4, d);
		dbCommand->FetchByte(5, ls);
		dbCommand->FetchInt32(6, gf);
		dbCommand->FetchInt32(7, ga);
		dbCommand->FetchInt32(8, gd);
		dbCommand->FetchInt32(9, pts);
		row.clanName = cname; row.played = pl; row.win = w; row.draw = d; row.loss = ls;
		row.goalsFor = gf; row.goalsAgainst = ga; row.goalDiff = gd; row.points = pts;
		outRows.push_back(row);
	}
	return true;
}
