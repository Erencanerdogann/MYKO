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
	// S114 K3 FAZ 5: KO login basarisi sonrasi _pending_ HWID kaydini account ile esleStir
	int HwidLinkAccount(const std::string & account, const std::string & ip);
	// S115 HWID ZORLA: Bizim Launcher haricinde login denemesi yapani log'la
	// action: "LOG" (mode 1) / "KICK" (mode 2). reason: "NO_HWID_REPORT" vb
	int HwidForceLog(const std::string & account, const std::string & ip,
	                 uint8 mode, const std::string & action, const std::string & reason);

	CDBProcess();
	~CDBProcess();

private:
	OdbcConnection* GetConnection();

	OdbcConnection* m_dbConnections[LOGIN_DB_POOL_SIZE];
	std::atomic<uint32_t> m_poolCounter{0};
};