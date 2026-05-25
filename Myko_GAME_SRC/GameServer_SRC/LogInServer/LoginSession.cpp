#pragma region include
#include "stdafx.h"
#include "../shared/DateTime.h"
#include <chrono>
#include <unordered_map>  // S115 HWID ZORLA: g_hwidReportTimes
#include <mutex>          // S115 HWID ZORLA: g_hwidReportMutex
#include "../GameServer/ConsoleColor.h"

LSPacketHandler PacketHandlers[NUM_LS_OPCODES];

#pragma region IP Rate Limiting
struct IPFailRecord {
	int failCount;
	std::chrono::steady_clock::time_point banUntil;
	IPFailRecord() : failCount(0), banUntil(std::chrono::steady_clock::time_point::min()) {}
};
static std::recursive_mutex s_ipRateMutex;
static std::map<std::string, IPFailRecord> s_ipFailMap;

// AS14: key = ip+account bazli, 15 fail/2dk (farkli hesap denemeleri ayri sayilir)
static const int IP_MAX_FAILS = 15;
static const int IP_BAN_SECONDS = 120;

static std::string MakeKey(const std::string &ip, const std::string &account)
{
	return ip + ":" + account;
}

static bool IsIPBanned(const std::string &ip, const std::string &account = "")
{
	std::lock_guard<std::recursive_mutex> lock(s_ipRateMutex);
	std::string key = account.empty() ? ip : MakeKey(ip, account);
	auto it = s_ipFailMap.find(key);
	if (it == s_ipFailMap.end())
		return false;
	if (it->second.failCount >= IP_MAX_FAILS) {
		if (std::chrono::steady_clock::now() < it->second.banUntil)
			return true;
		s_ipFailMap.erase(it);
		return false;
	}
	return false;
}

static int RecordIPLoginFail(const std::string &ip, const std::string &account = "")
{
	std::lock_guard<std::recursive_mutex> lock(s_ipRateMutex);
	std::string key = account.empty() ? ip : MakeKey(ip, account);
	auto &rec = s_ipFailMap[key];
	rec.failCount++;
	if (rec.failCount >= IP_MAX_FAILS)
		rec.banUntil = std::chrono::steady_clock::now() + std::chrono::seconds(IP_BAN_SECONDS);
	return rec.failCount;
}

static void ResetIPLoginFails(const std::string &ip, const std::string &account = "")
{
	std::lock_guard<std::recursive_mutex> lock(s_ipRateMutex);
	std::string key = account.empty() ? ip : MakeKey(ip, account);
	s_ipFailMap.erase(key);
}
#pragma endregion

#pragma endregion

// =====================================================================
// S115 HWID ZORLA — IP basina son HWID rapor zamani
// =====================================================================
// Map: ip -> son HWID report time (epoch sn)
// HandleLogin: m_HwidForceMode > 0 ise, IP'nin son HWID raporu pencerede mi
//             kontrol et. Yoksa log (mode 1) veya reject (mode 2).
// HandleHwidReport: her rapor geldikce time guncellenir.
// Memory: 1000+ kayit varsa 5 dk eski cleanup.
// =====================================================================
static std::mutex g_hwidReportMutex;
static std::unordered_map<std::string, time_t> g_hwidReportTimes;

static void RecordHwidReport(const std::string& ip)
{
	std::lock_guard<std::mutex> lk(g_hwidReportMutex);
	g_hwidReportTimes[ip] = time(nullptr);

	if (g_hwidReportTimes.size() > 1000) {
		time_t cutoff = time(nullptr) - 300;
		for (auto it = g_hwidReportTimes.begin(); it != g_hwidReportTimes.end(); ) {
			if (it->second < cutoff) it = g_hwidReportTimes.erase(it);
			else ++it;
		}
	}
}

static bool HasRecentHwidReport(const std::string& ip, int windowSec)
{
	std::lock_guard<std::mutex> lk(g_hwidReportMutex);
	auto it = g_hwidReportTimes.find(ip);
	if (it == g_hwidReportTimes.end()) return false;
	return (time(nullptr) - it->second) <= windowSec;
}
// =====================================================================

#pragma region InitPacketHandlers
void InitPacketHandlers(void)
{
	memset(&PacketHandlers, 0, sizeof(LSPacketHandler) * NUM_LS_OPCODES);
	PacketHandlers[LS_VERSION_REQ]			= &LoginSession::HandleVersion;
	PacketHandlers[LS_DOWNLOADINFO_REQ]		= &LoginSession::HandlePatches;
	PacketHandlers[LS_HWID_REPORT]			= &LoginSession::HandleHwidReport;  // S114 K3 FAZ 4
	PacketHandlers[LS_LOGIN_REQ]			= &LoginSession::HandleLogin;
	PacketHandlers[LS_SERVERLIST]			= &LoginSession::HandleServerlist;
	PacketHandlers[LS_NEWS]					= &LoginSession::HandleNews;
	PacketHandlers[LS_CRYPTION]				= &LoginSession::HandleSetEncryptionPublicKey;
	PacketHandlers[LS_UNKF7]				= &LoginSession::HandleUnkF7;
	PacketHandlers[LS_OTP]					= &LoginSession::HandleOTP;
	PacketHandlers[LS_OTP_SYNC]				= &LoginSession::HandleOTPSync;


	PacketHandlers[LS_KOREAKOLAUNCHER_PING] = &LoginSession::HandleLauncherPing;
}
#pragma endregion

#pragma region Constructor
/**
* @brief The constructor.
*/
LoginSession::LoginSession(uint16 socketID, SocketMgr *mgr) : KOSocket(socketID, mgr, -1, 2048, 256), OTPTrials(0), isOTPPassed(false) {}

#pragma endregion

#pragma region LoginSession::OnConnect()
/**
* @brief	Executes the connect action.
*/
void LoginSession::OnConnect()
{
	KOSocket::OnConnect();

	// Reject banned IPs immediately on connect — DoS protection
	std::string connectIP = GetRemoteIP();
	if (IsIPBanned(connectIP))
	{
		con_red(); printf("[CONN_BANNED] IP=%s rejected on connect (IP is temporarily banned)\n", connectIP.c_str()); con_white();
		Disconnect();
		return;
	}

	Initialize();
}
#pragma endregion

#pragma region LoginSession::Initialize()
/**
* @brief	Initializes this object.
*/
void LoginSession::Initialize()
{
	OTPKey = "";
	OTPTrials = 0;
	isOTPPassed = false;
}
#pragma endregion

#pragma region LoginSession::OnDisconnect()
/**
* @brief	Executes the disconnect action.
*/
void LoginSession::OnDisconnect()
{
	KOSocket::OnDisconnect();
}
#pragma endregion

#pragma region LoginSession::HandlePacket(Packet & pkt)

/**
* @brief	Handles an incoming user packet.
*
* @param	pkt	The packet.
*
* @return	true if it succeeds, false if it fails.
*/
bool LoginSession::HandlePacket(Packet & pkt)
{
	uint8 opcode = pkt.GetOpcode();

	// Unknown packetWIZ
	if (opcode >= NUM_LS_OPCODES
		|| PacketHandlers[opcode] == nullptr)
		return false;

	(this->*PacketHandlers[opcode])(pkt);
	return true;
}
#pragma endregion

#pragma region LoginSession::HandleVersion(Packet & pkt)
void LoginSession::HandleVersion(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << g_pMain->GetVersion();
	Send(&result);
}
#pragma endregion

#pragma region LoginSession::HandlePatches(Packet & pkt)
void LoginSession::HandlePatches(Packet & pkt)
{
	/*Packet result(pkt.GetOpcode());
	std::set<std::string> downloadset;
	uint16 version;
	pkt >> version;

	foreach(itr, (*g_pMain->GetPatchList()))
	{
		_VERSION_INFO* pInfo = itr->second;
		if (pInfo->sVersion > version)
			downloadset.insert(pInfo->strFilename);
	}

	result << g_pMain->GetFTPUrl() << g_pMain->GetFTPPath();
	result << uint16(downloadset.size());
	foreach(itr, downloadset)
		result << (*itr);
	printf("IP: %s Client Version: %d < Will download %d patch(es).>\n", GetRemoteIP().c_str(), version, uint8(downloadset.size()));
	Send(&result);*/


	Packet result(pkt.GetOpcode());
	uint16 version = 0;
	// L10: Paket validation — version alani icin minimum boyut kontrolu
	if (pkt.rpos() + sizeof(uint16) > pkt.wpos())
		return;
	pkt >> version;

	// Session 20: filename + hash ciftleri gonder
	struct PatchEntry {
		std::string filename;
		std::string hash;
	};
	std::vector<PatchEntry> downloadset;
	foreach(itr, (*g_pMain->GetPatchList())) {
		auto* pInfo = itr->second;
		if (pInfo != nullptr && pInfo->sVersion > version)
			downloadset.push_back({ pInfo->strFilename, pInfo->strFileHash });
	}

	result << g_pMain->GetFTPUrl() << g_pMain->GetFTPPath();

	if (downloadset.empty()) {
		result << uint16(0);
		Send(&result);
		return;
	}

	std::sort(downloadset.begin(), downloadset.end(), [](const PatchEntry& a, const PatchEntry& b) {
		return a.filename < b.filename;
	});
	result << uint16(downloadset.size());
	for (const auto& entry : downloadset) {
		result << entry.filename << entry.hash;
	}
	printf ("IP: %s Client Version: %d < Will download %d patch(es).>\n", GetRemoteIP().c_str(), version,uint8(downloadset.size()));
	Send(&result);
}
#pragma endregion


void LoginSession::HandleLauncherPing(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	Send(&result);
}

#pragma region LoginSession::HandleLogin(Packet & pkt)
void LoginSession::HandleLogin(Packet& pkt)
{
	enum LoginErrorCode
	{
		AUTH_SUCCESS = 0x01,
		AUTH_NOT_FOUND = 0x02,
		AUTH_INVALID = 0x03,
		AUTH_BANNED = 0x04,
		AUTH_IN_GAME = 0x05,
		AUTH_ERROR = 0x06,
		AUTH_AGREEMENT = 0xF,
		AUTH_OTP = 0x10,
		AUTH_FAILED = 0xFF
	};

	std::string clientIP = GetRemoteIP();
	Packet result(pkt.GetOpcode());
	uint16 resultCode = 0;
	std::string account, password, OTP_Key;
	DateTime time;

	pkt >> account >> password;

	// IP+account bazli brute force korumasi
	if (IsIPBanned(clientIP, account))
	{
		DateTime banTime;
		con_red(); printf("[LOGIN_BLOCKED] IP=%s Account=%s Time=%02d:%02d:%02d (too many failed attempts)\n",
			clientIP.c_str(), account.c_str(), banTime.GetHour(), banTime.GetMinute(), banTime.GetSecond()); con_white();
		Packet banResult(pkt.GetOpcode());
		banResult << uint16(0) << uint8(0x04); // AUTH_BANNED
		Send(&banResult);
		return;
	}

	// S115 HWID ZORLA — eski Launcher (HWID rapor etmeyen) login engelle
	// Mode 0: off | Mode 1: sadece log | Mode 2: zorla (AUTH_BANNED don)
	int hwidMode = g_pMain->m_HwidForceMode;
	if (hwidMode > 0)
	{
		bool hwidRecent = HasRecentHwidReport(clientIP,
			g_pMain->m_HwidReportWindowSec);
		if (!hwidRecent)
		{
			if (hwidMode == 1)
			{
				// LOG ONLY — login normal devam etsin ama kayit at
				con_yellow(); printf("[HWID_FORCE_LOG] No recent HWID for IP=%s account=%s (mode=1, would be kicked in mode 2)\n",
					clientIP.c_str(), account.c_str()); con_white();
				LOG_LOGIN("[HWID_FORCE_LOG] No HWID IP=%s Account=%s", clientIP.c_str(), account.c_str());
			}
			else
			{
				// MODE 2 — REJECT
				con_red(); printf("[HWID_FORCE_KICK] No recent HWID for IP=%s account=%s (mode=2)\n",
					clientIP.c_str(), account.c_str()); con_white();
				LOG_LOGIN("[HWID_FORCE_KICK] IP=%s Account=%s rejected", clientIP.c_str(), account.c_str());
				Packet banResult(pkt.GetOpcode());
				banResult << uint16(0) << uint8(0x04); // AUTH_BANNED
				Send(&banResult);
				return;
			}
		}
	}

	if (account.size() == 0
		|| account.size() > MAX_ID_SIZE
		|| password.size() == 0
		|| password.size() > MAX_PW_SIZE
		|| !string_is_valid(account))
		resultCode = AUTH_NOT_FOUND;
	else
		resultCode = g_pMain->m_DBProcess.AccountLogin(account, password, OTP_Key);

	std::string sAuthMessage;

	switch (resultCode)
	{
	case AUTH_SUCCESS:
		sAuthMessage = "SUCCESS";
		break;
	case AUTH_NOT_FOUND:
		sAuthMessage = "NOT FOUND";
		break;
	case AUTH_INVALID:
		sAuthMessage = "INVALID";
		break;
	case AUTH_BANNED:
		sAuthMessage = "BANNED";
		break;
	case AUTH_IN_GAME:
		sAuthMessage = "IN GAME";
		break;
	case AUTH_ERROR:
		sAuthMessage = "ERROR";
		break;
	case AUTH_AGREEMENT:
		sAuthMessage = "USER AGREEMENT";
		break;
	case AUTH_FAILED:
		sAuthMessage = "FAILED";
		break;
	case AUTH_OTP:
	{
		sAuthMessage = "OTP VALIDATION";
		OTPKey = OTP_Key;
	}
	break;
	default:
		sAuthMessage = string_format("UNKNOWN (%d)", resultCode);
		break;
	}

	con_cyan(); printf("[ LOGIN - %d:%d:%d ] ID=%s Authentication=%s IP=%s\n",
		time.GetHour(), time.GetMinute(), time.GetSecond(),
		account.c_str(), sAuthMessage.c_str(), clientIP.c_str()); con_white();

	LOG_LOGIN("[LOGIN_AUTH] Account=%s Result=%s IP=%s Code=%d",
		account.c_str(), sAuthMessage.c_str(), clientIP.c_str(), resultCode);

	// Track login success/failure for rate limiting
	if (resultCode == AUTH_SUCCESS || resultCode == AUTH_OTP || resultCode == AUTH_AGREEMENT)
	{
		ResetIPLoginFails(clientIP, account);
		// S114 K3 FAZ 5: Login basarisi sonrasi _pending_ HWID kaydini account ile esleStir
		// (Launcher 0x04 ile account="" yazdi, simdi gercek account ile UPDATE)
		g_pMain->m_DBProcess.HwidLinkAccount(account, clientIP);
	}
	else if (resultCode == AUTH_NOT_FOUND || resultCode == AUTH_INVALID || resultCode == AUTH_FAILED)
	{
		int currentFails = RecordIPLoginFail(clientIP, account);
		int remaining = IP_MAX_FAILS - currentFails;
		con_red(); printf("[LOGIN_FAIL] IP=%s Account=%s Time=%02d:%02d:%02d Reason=%s FailCount=%d/%d Kalan=%d\n",
			clientIP.c_str(), account.c_str(),
			time.GetHour(), time.GetMinute(), time.GetSecond(),
			sAuthMessage.c_str(), currentFails, IP_MAX_FAILS, remaining > 0 ? remaining : 0); con_white();
		if (currentFails >= IP_MAX_FAILS) {
			LOG_LOGIN("[LOGIN_BANNED] IP=%s Account=%s Ban=%d sn", clientIP.c_str(), account.c_str(), IP_BAN_SECONDS);
		}
	}

	// L4: Account enumeration onlemi — NOT_FOUND ve INVALID ayni kod olarak gonder
	uint8 clientResultCode = static_cast<uint8>(resultCode);
	if (resultCode == AUTH_NOT_FOUND || resultCode == AUTH_INVALID)
		clientResultCode = AUTH_INVALID;  // client "gecersiz kullanici adi veya sifre" goster
	result << uint16(0) << clientResultCode;
	if (resultCode == AUTH_SUCCESS)
	{
		result << g_pMain->m_DBProcess.AccountPremium(account);
		result << account;
	}
	if (resultCode == AUTH_OTP)
	{
		result << uint32(0);
	}
	else if (resultCode == AUTH_IN_GAME)
	{
		/*Packet newpkt(LOGIN_ADD_ACCOUNT);
		newpkt << account << password;
		g_pMain->m_GameServerSocketMgr.SendAll(&newpkt);*/

		std::string iTarIP = g_pMain->m_DBProcess.GetConnectedServerIP(account);
		if (!iTarIP.empty())
		{
			result << iTarIP;
			result << uint16(15001);
			
			result << account;
		}
	}
	// L15: AUTH_AGREEMENT icin ek data gonderilmiyor — else if blogu kaldirildi

	//g_pMain->WriteUserLogFile(string_format("LOGIN - %d:%d:%d || ID=%s,PDW=%s,Authentication=%s,IP=%s\n", time.GetHour(), time.GetMinute(), time.GetSecond(), account.c_str(), password.c_str(), sAuthMessage.c_str(), GetRemoteIP().c_str()));

	Send(&result);
}
#pragma endregion

#pragma region LoginSession::HandleServerlist(Packet & pkt)
void LoginSession::HandleServerlist(Packet & pkt)
{
	Packet result(pkt.GetOpcode());

	uint16 echo;
	pkt >> echo;
	result << echo;

	g_pMain->GetServerList(result);
	Send(&result);
}
#pragma endregion

#pragma region LoginSession::HandleHwidReport(Packet & pkt)
// S114 K3 FAZ 4: Launcher -> server HWID rapor.
// Format gelen:  [0x04] [hwid_string]
// Format giden:  [0x84] [uint8 result]  // 0=OK, 1=BANNED
void LoginSession::HandleHwidReport(Packet & pkt)
{
	// Launcher MP_AddString ile RAW 32 byte yolluyor (length prefix YOK)
	// Fixed 32 byte oku
	char hwidBuf[33] = {0};
	if (pkt.rpos() + 32 > pkt.size()) {
		// Paket eksik
		Packet result(0x84);
		result << uint8(0);
		Send(&result);
		return;
	}
	pkt.read(hwidBuf, 32);
	std::string hwid(hwidBuf, 32);

	// HWID format check (32 char hex MD5)
	if (hwid.length() != 32) {
		Packet result(0x84);
		result << uint8(0);
		Send(&result);
		return;
	}

	std::string ip = GetRemoteIP();
	int banResult = g_pMain->m_DBProcess.HwidCheckLogin(hwid, ip);

	Packet result(0x84);
	result << uint8(banResult);  // 0=OK, 1=BANNED
	Send(&result);

	if (banResult == 1) {
		printf("[HWID BAN] Reject login: hwid=%s ip=%s\n", hwid.c_str(), ip.c_str());
	}
	else {
		// S115 HWID ZORLA: Bu IP'nin son HWID rapor zamanini guncelle.
		// HandleLogin'de bu kayit pencerede mi diye kontrol edilir (eski Launcher tespiti).
		RecordHwidReport(ip);
	}
}
#pragma endregion

#pragma region LoginSession::HandleNews(Packet & pkt)
void LoginSession::HandleNews(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	News *pNews = g_pMain->GetNews();

	if (pNews->Size)
	{
		result << "Login Notice" << uint16(pNews->Size);
		result.append(pNews->Content, pNews->Size);
	}
	else // dummy news, will skip past it
	{
		result << "Login Notice" << "<empty>";
	}
	Send(&result);
}
#pragma endregion

#pragma region LoginSession::HandleSetEncryptionPublicKey(Packet & pkt)
void LoginSession::HandleSetEncryptionPublicKey(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << m_crypto.GenerateKey();
	Send(&result);
	EnableCrypto();
}
#pragma endregion

#pragma region LoginSession::HandleUnkF7(Packet & pkt)
void LoginSession::HandleUnkF7(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint16(0);
	Send(&result);
}
#pragma endregion

#pragma region LoginSession::HandleOTP(Packet & pkt)
void LoginSession::HandleOTP(Packet & pkt)
{
	enum LoginErrorCode
	{
		AUTH_SUCCESS = 0x01,
		AUTH_NOT_FOUND = 0x02,
		AUTH_INVALID = 0x03,
		AUTH_BANNED = 0x04,
		AUTH_IN_GAME = 0x05,
		AUTH_ERROR = 0x06,
		AUTH_AGREEMENT = 0xF,
		AUTH_FAILED = 0xFF
	};

	// IP rate limiting check for OTP — brute force protection
	std::string otpClientIP = GetRemoteIP();
	if (IsIPBanned(otpClientIP))
	{
		DateTime banTime;
		con_red(); printf("[LOGIN_BLOCKED] IP=%s Time=%02d:%02d:%02d (IP temporarily banned - OTP attempt blocked)\n",
			otpClientIP.c_str(), banTime.GetHour(), banTime.GetMinute(), banTime.GetSecond()); con_white();
		Disconnect();
		return;
	}

	Packet result(pkt.GetOpcode());
	uint16 resultCode = 0;
	std::string account, password, OTPCode;
	DateTime time;

	pkt >> account >> password >> OTPCode;

	if (account.size() == 0
		|| account.size() > MAX_ID_SIZE
		|| password.size() == 0
		|| password.size() > MAX_PW_SIZE
		|| !string_is_valid(account))
		return;
	else
		resultCode = g_pMain->m_DBProcess.AccountLoginOTP(account, password);

	std::string sAuthMessage;

	if (resultCode == AUTH_NOT_FOUND
		|| resultCode == AUTH_INVALID
		|| resultCode == AUTH_BANNED
		|| resultCode == AUTH_ERROR
		|| resultCode == AUTH_FAILED)
		return;

	if (OTPKey == OTPCode)
	{
		isOTPPassed = true;
		sAuthMessage = "SUCCESS";
		ResetIPLoginFails(otpClientIP);
	}
	else if (OTPKey != OTPCode)
	{
		isOTPPassed = false;
		OTPTrials++;
		sAuthMessage = "INVALID OTP";
		RecordIPLoginFail(otpClientIP);
		con_red(); printf("[LOGIN_FAIL] IP=%s Account=%s Time=%02d:%02d:%02d Reason=INVALID_OTP\n",
			otpClientIP.c_str(), account.c_str(),
			time.GetHour(), time.GetMinute(), time.GetSecond()); con_white();
	}

	con_cyan(); printf("[ LOGIN OTP - %d:%d:%d ] ID=%s Authentication=%s IP=%s\n",
		time.GetHour(), time.GetMinute(), time.GetSecond(),
		account.c_str(), sAuthMessage.c_str(), GetRemoteIP().c_str()); con_white();

	if (isOTPPassed)
	{
		result.clear();
		result.Initialize(LS_LOGIN_REQ);
		result << uint16(0) << uint8(resultCode);// << uint8(0) << uint8(0);
		
		if (resultCode == AUTH_SUCCESS)
		{
			result << g_pMain->m_DBProcess.AccountPremium(account);
			result << account;
		}
	}
	else
	{
		if (OTPTrials > 2)
		{
			result << uint16(0) << OTPCode;
			Send(&result);
			Disconnect();
			return;
		}

		result << uint16(2) << OTPCode;
	}

//	g_pMain->WriteUserLogFile(string_format("LOGIN OTP - %d:%d:%d || ID=%s,PDW=%s,Authentication=%s,IP=%s\n", time.GetHour(), time.GetMinute(), time.GetSecond(), account.c_str(), password.c_str(), sAuthMessage.c_str(), GetRemoteIP().c_str()));

	Send(&result);
}
#pragma endregion

#pragma region LoginSession::HandleOTPSync(Packet & pkt)
void LoginSession::HandleOTPSync(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint16(0);
	Send(&result);
}
#pragma endregion