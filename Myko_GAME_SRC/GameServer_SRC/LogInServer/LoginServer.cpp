#include "stdafx.h"
#include <sstream>
#include <iostream>
#include "../GameServer/ConsoleColor.h"

#include <time.h>
#include <fstream>

#include "../shared/Ini.h"
#include "../shared/DateTime.h"

extern bool g_bRunning;
std::vector<Thread *> g_LogintimerThreads;

LoginServer::LoginServer() { Initialize(); }

void LoginServer::Initialize()
{
	m_sLastVersion = __VERSION;
	m_fpLoginServer = false;
}

bool LoginServer::Startup()
{
	DateTime time;
	//GetInfoFromIni();
	//// Server Start
	//printf("Server started on %04d-%02d-%02d at %02d:%02d\n\n", time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute());
	//if (!m_DBProcess.Connect(m_ODBCName, m_ODBCLogin, m_ODBCPwd))
	//{
	//	printf("LoginServer ODBC Connect Error\n");
	//	return false;
	//}

	//m_DBProcess.LoadVersionList();
	//m_DBProcess.LoadServerList();
	//m_DBProcess.LoadKingNotice();
	//InitPacketHandlers();
	//UpdateServerList();

	//g_LogintimerThreads.push_back(new Thread(Timer_UpdateUserCount));
	//g_LogintimerThreads.push_back(new Thread(Timer_UpdateKingNotice));
	
	
	/*// License System Stard System..
	m_HardwareIDArray.push_back(628452953932025);
	m_HardwareIDArray.push_back(254442953937629);
	m_HardwareIDArray.push_back(2517529539244);
	m_HardwareIDArray.push_back(459122953948522);
	m_HardwareIDArray.push_back(25175295392441);
	if (!g_HardwareInformation.IsValidHardwareID(m_HardwareIDArray))
	{
		printf("Server is shutdown :(\n");
		return false;
	}
	// License System End System..*/

	GetInfoFromIni();

	CreateDirectory("Logs",NULL);

	// Initialize structured LogSystem for LoginServer
	LogSystem::Instance().Initialize("./Logs");
	LOG(LogCategory::LOG_GENERAL, "LoginServer starting — LogSystem initialized");

	m_fpLoginServer = fopen("./Logs/LoginServer.log", "a");
	if (m_fpLoginServer == nullptr)
	{
		con_red(); printf("ERROR: Unable to open log file.\n"); con_white();
		return false;
	}

	m_fpUser = fopen(string_format("./Logs/Login_%d_%d_%d.log",time.GetDay(),time.GetMonth(),time.GetYear()).c_str(), "a");
	if (m_fpUser == nullptr)
	{
		con_red(); printf("ERROR: Unable to open user log file.\n"); con_white();
		return false;
	}

	if (!m_DBProcess.Connect(m_ODBCName, m_ODBCLogin, m_ODBCPwd))
	{
		con_red(); printf("HATA: ODBC ile Yapilan ayarlar nedeniyle veritabanina baglanilamiyor.\n"); con_white();
		return false;
	}

	con_green(); printf("Database connection started successfully.\n"); con_white();

	m_DBProcess.LoadKingNotice();

	if (!m_DBProcess.LoadVersionList())
	{
		con_red(); printf("HATA: Version listesi yuklenemiyor.\n"); con_white();
		return false;
	}

	printf("Latest version in database : %d\n", GetVersion());
	if (!m_DBProcess.LoadServerList())
	{
		con_red(); printf("HATA: Version listesi yuklenemiyor.\n"); con_white();
		return false;
	}

	InitPacketHandlers();

	for (int i = 0; i < 10; i++)
		if (!m_socketMgr[i].Listen(m_LoginServerPort + i, MAX_USER))
		{
			con_red(); printf("ERROR: Failed to listen on server port.\n"); con_white();
			return false;
		}

	for (int i = 0; i < 10; i++)
		m_socketMgr[i].RunServer(true);

	// GameServer socket kullanilmiyor — bind kaldirildi (CHI-15)

	UpdateServerList();
	g_LogintimerThreads.push_back(new Thread(Timer_UpdateUserCount));
	g_LogintimerThreads.push_back(new Thread(Timer_UpdateKingNotice));
	con_green(); printf("Login Server Startup\n"); con_white();
	return true;
}

uint32 LoginServer::Timer_UpdateUserCount(void * lpParam)
{
	while (g_bRunning)
	{
		g_pMain->UpdateServerList();
		sleep(60 * SECOND);
	}
	return 0;
}

uint32 LoginServer::Timer_UpdateKingNotice(void * lpParam)
{
	while (g_bRunning)
	{
		sleep(600 * SECOND);
		if (!g_bRunning) break;
		g_pMain->m_DBProcess.LoadKingNotice();
	}
	return 0;
}

void LoginServer::GetServerList(Packet & result)
{
	Guard lock(m_serverListLock);
	result.append(m_serverListPacket.contents(), m_serverListPacket.size());
}

void LoginServer::UpdateServerList()
{
	// Update the user counts first
	m_DBProcess.LoadUserCountList();

	Guard lock(m_serverListLock);
	Packet & result = m_serverListPacket;

	result.clear();
	result << uint8(m_ServerList.GetSize(false));
	foreach_stlmap_nolock (itr, m_ServerList) 
	{		
		_SERVER_INFO * pServer = itr->second;
		if (pServer == nullptr)
			continue;

		result << pServer->strLanIP << pServer->strServerIP<< pServer->strServerName;

		if (pServer->sUserCount <= pServer->sPlayerCap)
			result << pServer->sUserCount;
		else
			result << int16(-1);

		result << pServer->sServerID << pServer->sGroupID << pServer->sPlayerCap << pServer->sFreePlayerCap << uint8(0) << pServer->strScreenType;

		// we read all this stuff from ini, TODO: make this more versatile.
		result << pServer->strKarusKingName << pServer->strKarusNotice << pServer->strElMoradKingName << pServer->strElMoradNotice;
	}
}

void LoginServer::GetInfoFromIni()
{
	CIni ini(CONF_LOGIN_SERVER);

	ini.GetString("DOWNLOAD", "URL", "ftp.yoursite.net", m_strFtpUrl, false);
	ini.GetString("DOWNLOAD", "PATH", "/", m_strFilePath, false);

	// L8: ODBC credentials — env var oncelikli, fallback INI (guvenlik)
	const char* envDSN = getenv("KO_ODBC_DSN");
	const char* envUID = getenv("KO_ODBC_UID");
	const char* envPWD = getenv("KO_ODBC_PWD");
	if (envDSN) m_ODBCName  = envDSN;
	else ini.GetString("ODBC", "DSN", "KO_MAIN",   m_ODBCName,  false);
	if (envUID) m_ODBCLogin = envUID;
	else ini.GetString("ODBC", "UID", "username",  m_ODBCLogin, false);
	if (envPWD) m_ODBCPwd   = envPWD;
	else ini.GetString("ODBC", "PWD", "password",  m_ODBCPwd,   false);
	m_LoginServerPort = ini.GetInt("SETTINGS", "PORT", 15100);

	// S115 HWID ZORLA — eski Launcher (HWID rapor etmeyen) login engelle
	// Default 0 (off) — guvenli baslangic. Acilis sonrasi 1 (log), sonra 2 (zorla).
	m_HwidForceMode        = ini.GetInt("HWID", "ForceMode", 0);
	m_HwidReportWindowSec  = ini.GetInt("HWID", "ReportWindowSec", 30);
	if (m_HwidReportWindowSec < 5)  m_HwidReportWindowSec = 5;
	if (m_HwidReportWindowSec > 300) m_HwidReportWindowSec = 300;
	printf("[CONFIG] HWID ForceMode=%d, ReportWindowSec=%d\n",
	       m_HwidForceMode, m_HwidReportWindowSec);

	char key[20];

	// Read news from INI (max 3 blocks)
#define BOX_START '#' << uint8(0) << '\n'
#define BOX_END '#'

	m_news.Size = 0;
	std::stringstream ss;
	for (int i = 0; i < 3; i++)
	{
		std::string title, message;

		_snprintf(key, sizeof(key), "TITLE_%02d", i);
		ini.GetString("NEWS", key, "", title);
		if (title.empty())
			continue;

		_snprintf(key, sizeof(key), "MESSAGE_%02d", i);
		ini.GetString("NEWS", key, "", message);
		if (message.empty())
			continue;

		size_t oldPos = 0, pos = 0;
		ss << title << BOX_START << message << BOX_END;
	}

	m_news.Size = ss.str().size();
	if (m_news.Size >= sizeof(m_news.Content))
		m_news.Size = sizeof(m_news.Content) - 1;
	if (m_news.Size) {
		snprintf(reinterpret_cast<char*>(m_news.Content), sizeof(m_news.Content), "%.*s",
			static_cast<int>(m_news.Size), ss.str().c_str());
		m_news.Content[m_news.Size] = '\0';
	}
}

void LoginServer::WriteLogFile(std::string & logMessage)
{
	m_lock.lock();
	fwrite(logMessage.c_str(), logMessage.length(), 1, m_fpLoginServer);
	fflush(m_fpLoginServer);
	m_lock.unlock();
}

void LoginServer::WriteUserLogFile(std::string & logMessage)
{
	m_lock.lock();
	fwrite(logMessage.c_str(), logMessage.length(), 1, m_fpUser);
	fflush(m_fpUser);
	m_lock.unlock();
}

void LoginServer::ReportSQLError(OdbcError *pError)
{
	if (pError == nullptr)
		return;

	// This is *very* temporary.
	std::string errorMessage = string_format(_T("ODBC error occurred.\r\nSource: %s\r\nError: %s\r\nDescription: %s\n"),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	LOG_ODBC("[LOGIN_ODBC_ERROR] Source=%s Error=%s Description=%s",
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	TRACE("%s", errorMessage.c_str());
	WriteLogFile(errorMessage);
	delete pError;
}

LoginServer::~LoginServer()
{
	printf("Waiting for timer threads to exit...");
	foreach(itr, g_LogintimerThreads)
	{
		(*itr)->waitForExit();
		delete (*itr);
	}
	printf(" exited.\n");
	Sleep(1 * SECOND);

	m_ServerList.DeleteAllData();

	foreach(itr, m_VersionList)
		delete itr->second;
	m_VersionList.clear();

	if (m_fpLoginServer != nullptr)
		fclose(m_fpLoginServer);

	if (m_fpUser != nullptr)
		fclose(m_fpUser);

	printf("Shutting down socket system...");

	for (int i = 0; i < 10; i++)
		m_socketMgr[i].Shutdown();

	printf(" done.\n");
	Sleep(1 * SECOND);
}