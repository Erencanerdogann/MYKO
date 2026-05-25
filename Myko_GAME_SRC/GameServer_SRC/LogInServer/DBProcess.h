#pragma once
#include "../shared/database/OdbcConnection.h"
#include <atomic>

static constexpr int LOGIN_DB_POOL_SIZE = 4;

class CDBProcess
{
public:
	bool Connect(std::string & szDSN, std::string & szUser, std::string & szPass);

	bool LoadVersionList();
	bool LoadServerList();
	bool LoadKingNotice();
	bool LoadUserCountList();
	uint32 GetPatchVersion();

	uint16 AccountLogin(std::string & strAccountID, std::string & strPasswd, std::string & OTP_Key);
	uint16 AccountLoginOTP(std::string & strAccountID, std::string & strPasswd);
	int16 AccountPremium(std::string & strAccountID);
	std::string GetConnectedServerIP(std::string & strAccountID);
	// S114 K3 FAZ 4: HWID ban check (Launcher 0x4 paket icin)
	// Returns: 0=OK, 1=BANNED
	int HwidCheckLogin(const std::string & hwid, const std::string & ip);

	CDBProcess();
	~CDBProcess();

private:
	OdbcConnection* GetConnection();

	OdbcConnection* m_dbConnections[LOGIN_DB_POOL_SIZE];
	std::atomic<uint32_t> m_poolCounter{0};
};