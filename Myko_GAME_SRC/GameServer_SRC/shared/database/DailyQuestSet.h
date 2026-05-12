#pragma once

class CDailyQyestSet : public OdbcRecordset
{
public:
	CDailyQyestSet(OdbcConnection * dbConnection, DailyQuestArray *pMap)
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("DAILY_QUESTS"); }
	virtual tstring GetColumns()  {
		return _T("id,questid,timetype,killtype,"
		"MobID1,MobID2,MobID3,MobID4,"
		"KillCount,"
		"Reward1,Reward2,Reward3,Reward4,"
		"Count1,Count2,Count3,Count4,"
		"ZoneID,MinLevel,MaxLevel,replaytime, randomid,StrName");
	}

	virtual bool Fetch() {
		int field = 1;
		_DAILY_QUEST *pData = new _DAILY_QUEST;
		_dbCommand->FetchByte(field++, pData->index);
		_dbCommand->FetchUInt16(field++, pData->questid);
		_dbCommand->FetchByte(field++, pData->timetype);
		_dbCommand->FetchByte(field++, pData->killtype);
		for (uint8 i = 0; i< 4; i++) _dbCommand->FetchUInt16(field++, pData->Mobid[i]);
		_dbCommand->FetchUInt16(field++, pData->kcount);
		for (uint8 i = 0; i < 4; i++) _dbCommand->FetchUInt32(field++, pData->rewaditemid[i]);
		for (uint8 i = 0; i < 4; i++) _dbCommand->FetchUInt32(field++, pData->rewarditemcount[i]);
		_dbCommand->FetchByte(field++, pData->zoneid);
		_dbCommand->FetchByte(field++, pData->minlevel);
		_dbCommand->FetchByte(field++, pData->maxlevel);
		_dbCommand->FetchUInt32(field++, pData->replaytime);
		_dbCommand->FetchByte(field++, pData->randomid);
		_dbCommand->FetchString(field++, pData->strQuestName);
		if (!m_pMap->PutData(pData->index, pData)) {
			delete pData;
			return false;
		}
		return true;
	}
	DailyQuestArray * m_pMap;
};