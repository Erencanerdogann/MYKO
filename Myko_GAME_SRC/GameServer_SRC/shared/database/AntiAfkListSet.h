#pragma once

class CAntifAfkListSet : public OdbcRecordset
{
public:
	CAntifAfkListSet(OdbcConnection * dbConnection, uint16 *pMap)
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("ANTIAFKLIST"); }
	virtual tstring GetColumns() {return _T("NpcID");}
	virtual bool Fetch() {
		/*uint32 index; */uint16 NpcID; int field = 1;
		//_dbCommand->FetchUInt32(field++, index);
		_dbCommand->FetchUInt16(field++, NpcID);
		g_pMain->m_AntiAfkList.push_back(NpcID);
		return true;
	}
	uint16 * m_pMap;
};