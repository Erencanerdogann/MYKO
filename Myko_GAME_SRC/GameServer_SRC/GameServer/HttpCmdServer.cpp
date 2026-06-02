#include "stdafx.h"

#ifdef HTTP_CMD_SERVER

#include "HttpCmdServer.h"
#include "GameServerDlg.h"
#include "DBAgent.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <sstream>
#include <set>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> CHttpCmdServer::s_running(false);
SOCKET CHttpCmdServer::s_listenSocket = INVALID_SOCKET;
std::thread CHttpCmdServer::s_thread;
std::string CHttpCmdServer::s_token;

static const std::set<std::string> s_whitelist = {
	// Duyuru
	"permanent", "offpermanent", "notice", "noticeall",
	// Reload
	"reloadnotice", "reloadalltables", "reloaditems", "reloaddrops",
	"reloadquests", "reloadmagics", "reloadkings", "reloadcsw", "reloadevent",
	"reloadtables", "reloadtables2", "reloadtables3",
	// Savas/Etkinlik
	"open1", "open2", "open3", "open4", "open5", "open6",
	"snow", "csw", "close", "cswclose",
	"santa", "santaclose", "angel", "angelclose", "discount", "alldiscount", "offdiscount",
	// Bakim / Kapanma
	"caremode", "caremodeoff", "shutdown",
	// Etkinlikler
	"madclas", "madclasclose",
	"ftopen", "ftclose",
	"utc",
	"chaosopen", "chaosclose",
	"borderopen", "borderclose",
	"juraidopen", "juraidclose",
	"beefopen", "beefclose",
	"tournamentstart", "tournamentclose",
	"lottery", "lotteryclose",
	// Diger
	"count", "aireset", "bug",
	// GM_MOD (MATRIX — turnuva/lig/bracket otomasyon, server-form CGameServerDlg'de hazir)
	"bracketcreate", "bracketstart", "bracketcancel", "bracketstatus",
	"leaguecreate", "leaguestart", "leaguecancel", "leaguestatus",
	"1v1create", "1v1start", "1v1cancel", "1v1status",
	"ctfstart", "ctfclose",
	"partyvs",
	"eventcreate", "eventcancel", "eventconfig", "eventlist",
	"partybracketcreate", "partybracketstart", "partyleaguecreate", "partyleaguestart",
	"tournamentreglist", "tournamentreward",
	// GM_MOD bahis
	"betlimits", "betwindow", "betcommission", "betcancel", "betstatus",
	// GM_MOD aksiyon (server-form CGameServerDlg'de hazir)
	"kill", "tpall", "captain", "warresult", "block", "user_bots", "setweather"
};

void CHttpCmdServer::Start()
{
	try
	{
		// Token oku
		std::ifstream tokenFile("HttpCmdToken.txt");
		if (!tokenFile.is_open())
		{
			printf("[HttpCmd] HttpCmdToken.txt bulunamadi, listener baslatilmiyor.\n");
			return;
		}
		std::getline(tokenFile, s_token);
		tokenFile.close();

		// Bosluk temizle
		while (!s_token.empty() && (s_token.back() == '\r' || s_token.back() == '\n' || s_token.back() == ' '))
			s_token.pop_back();

		if (s_token.size() < 20)
		{
			printf("[HttpCmd] Token cok kisa (min 20 char), listener baslatilmiyor.\n");
			return;
		}

		// Socket
		s_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s_listenSocket == INVALID_SOCKET)
		{
			printf("[HttpCmd] socket() hatasi: %d\n", WSAGetLastError());
			return;
		}

		int opt = 1;
		setsockopt(s_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		addr.sin_port = htons(19999);

		if (bind(s_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			printf("[HttpCmd] bind() hatasi: %d\n", WSAGetLastError());
			closesocket(s_listenSocket);
			s_listenSocket = INVALID_SOCKET;
			return;
		}

		if (listen(s_listenSocket, 5) == SOCKET_ERROR)
		{
			printf("[HttpCmd] listen() hatasi: %d\n", WSAGetLastError());
			closesocket(s_listenSocket);
			s_listenSocket = INVALID_SOCKET;
			return;
		}

		s_running = true;
		s_thread = std::thread(ListenerThread);
		printf("[HttpCmd] listener basladi: 127.0.0.1:19999\n");
	}
	catch (...)
	{
		printf("[HttpCmd] Start() beklenmedik istisna.\n");
	}
}

void CHttpCmdServer::Stop()
{
	try
	{
		s_running = false;
		if (s_listenSocket != INVALID_SOCKET)
		{
			closesocket(s_listenSocket);
			s_listenSocket = INVALID_SOCKET;
		}
		if (s_thread.joinable())
			s_thread.join();
	}
	catch (...) {}
}

void CHttpCmdServer::ListenerThread()
{
	while (s_running)
	{
		try
		{
			SOCKET client = accept(s_listenSocket, nullptr, nullptr);
			if (client == INVALID_SOCKET)
				break;

			// Her istek inline isle (dusuk trafik bekleniyor)
			try
			{
				HandleRequest(client);
			}
			catch (...)
			{
				printf("[HttpCmd] HandleRequest istisna.\n");
			}
			closesocket(client);
		}
		catch (...)
		{
			printf("[HttpCmd] ListenerThread istisna.\n");
		}
	}
}

// YIKICI komutlar — confirm:true sart, audit'te yikici=1. (S120 GM_MOD guvenlik)
static const std::set<std::string> s_destructive = {
	"shutdown", "down", "ipban", "hwidban", "block", "kill"
};

// KURAL 1 (BLOCKING): Audit log FAIL-SAFE. DB/log basarisiz olsa bile komut AKISI DURMAZ.
// AI/MATRIX/DB yoksa sistem hataya dusmeden calismaya devam eder.
void CHttpCmdServer::WriteAudit(const std::string& tokenShort, const std::string& clientIP,
	const std::string& cmd, const std::string& params, int httpCode, const std::string& durum, bool yikici)
{
	// Dosya log (her zaman, DB'den bagimsiz)
	try {
		LOG(LogCategory::LOG_GENERAL, "GM_HTTP: ip=%s token=%s cmd=%s params=[%s] http=%d durum=%s yikici=%d",
			clientIP.c_str(), tokenShort.c_str(), cmd.c_str(), params.c_str(), httpCode, durum.c_str(), (int)yikici);
	} catch (...) {}

	// DB log (_MK_GM_AUDIT, KO_MYKO). FAIL-SAFE: hata olsa bile komut akisi etkilenmez (KURAL 1).
	try {
		OdbcConnection* db = g_DBAgent.GetGameDB();
		if (db != nullptr) {
			std::unique_ptr<OdbcCommand> dbCommand(db->CreateCommand());
			if (dbCommand.get() != nullptr) {
				dbCommand->AddParameter(SQL_PARAM_INPUT, tokenShort.c_str(), tokenShort.length() ? tokenShort.length() : 1);
				dbCommand->AddParameter(SQL_PARAM_INPUT, clientIP.c_str(), clientIP.length() ? clientIP.length() : 1);
				dbCommand->AddParameter(SQL_PARAM_INPUT, cmd.c_str(), cmd.length() ? cmd.length() : 1);
				dbCommand->AddParameter(SQL_PARAM_INPUT, params.c_str(), params.length() ? params.length() : 1);
				dbCommand->AddParameter(SQL_PARAM_INPUT, &httpCode);
				dbCommand->AddParameter(SQL_PARAM_INPUT, durum.c_str(), durum.length() ? durum.length() : 1);
				uint8 by = yikici ? 1 : 0;
				dbCommand->AddParameter(SQL_PARAM_INPUT, &by);
				dbCommand->Execute(_T("INSERT INTO KO_MYKO.dbo._MK_GM_AUDIT (token_kisa,clientIP,komut,params,sonuc,durum,yikici) VALUES (?,?,?,?,?,?,?)"));
			}
		}
	} catch (...) {
		// DB log patlasa bile SESSIZ gec — komut zaten islendi, sistem durmaz (KURAL 1)
	}
}

void CHttpCmdServer::HandleRequest(SOCKET client)
{
	// Client IP (audit icin) — fail olsa "?" (KURAL 1: hata akisi durdurmaz)
	std::string clientIP = "?";
	try {
		sockaddr_in addr; int alen = sizeof(addr);
		if (getpeername(client, (sockaddr*)&addr, &alen) == 0) {
			char ipbuf[64] = {};
			const char* p = inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf));
			if (p) clientIP = p;
		}
	} catch (...) {}

	// Recv
	char buf[4096] = {};
	int received = recv(client, buf, sizeof(buf) - 1, 0);
	if (received <= 0)
	{
		const char* resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	std::string request(buf, received);

	// Token kontrol
	std::string token = ExtractHeader(request, "X-Token");
	std::string tokenShort = token.substr(0, token.length() < 8 ? token.length() : 8);
	if (!VerifyToken(token))
	{
		WriteAudit(tokenShort, clientIP, "?", "", 401, "DENIED", false);
		const char* resp = "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	// Body parse
	std::string body = ExtractBody(request);
	if (body.empty())
	{
		const char* resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	std::string cmd = ExtractCommandFromJson(body);
	if (cmd.empty())
	{
		const char* resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	// cmd "/" ile baslamali
	if (cmd[0] != '/')
		cmd = "/" + cmd;

	// Whitelist kontrol: ikinci kelimeye kadar al (orn "/permanent metin" -> "permanent")
	std::string cmdName = cmd.substr(1);
	size_t spacePos = cmdName.find(' ');
	if (spacePos != std::string::npos)
		cmdName = cmdName.substr(0, spacePos);

	// Kucuk harf
	for (auto& c : cmdName) c = (char)tolower((unsigned char)c);

	bool bYikici = (s_destructive.find(cmdName) != s_destructive.end());

	if (!IsWhitelisted(cmdName))
	{
		WriteAudit(tokenShort, clientIP, cmdName, "", 403, "DENIED", bYikici);
		const char* resp = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	// YIKICI komut -> body'de "confirm":true SART (S120 guvenlik)
	if (bYikici && body.find("\"confirm\"") == std::string::npos)
	{
		WriteAudit(tokenShort, clientIP, cmdName, "", 400, "CONFIRM_REQ", true);
		const char* resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	// ProcessServerCommand
	try
	{
		g_pMain->ProcessServerCommand(cmd);
	}
	catch (...)
	{
		printf("[HttpCmd] ProcessServerCommand istisna: %s\n", cmdName.c_str());
		WriteAudit(tokenShort, clientIP, cmdName, cmd, 500, "FAIL", bYikici);
		const char* resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

	WriteAudit(tokenShort, clientIP, cmdName, cmd, 200, "OK", bYikici);
	const char* resp = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
	send(client, resp, (int)strlen(resp), 0);
}

bool CHttpCmdServer::VerifyToken(const std::string& token)
{
	return !token.empty() && token == s_token;
}

bool CHttpCmdServer::IsWhitelisted(const std::string& cmd)
{
	return s_whitelist.find(cmd) != s_whitelist.end();
}

std::string CHttpCmdServer::ExtractHeader(const std::string& request, const std::string& headerName)
{
	std::string search = headerName + ": ";
	size_t pos = request.find(search);
	if (pos == std::string::npos)
		return "";
	pos += search.size();
	size_t end = request.find("\r\n", pos);
	if (end == std::string::npos)
		end = request.find("\n", pos);
	if (end == std::string::npos)
		return request.substr(pos);
	return request.substr(pos, end - pos);
}

std::string CHttpCmdServer::ExtractBody(const std::string& request)
{
	size_t pos = request.find("\r\n\r\n");
	if (pos == std::string::npos)
		pos = request.find("\n\n");
	if (pos == std::string::npos)
		return "";
	return request.substr(pos + (request[pos + 2] == '\n' ? 2 : 4));
}

std::string CHttpCmdServer::ExtractCommandFromJson(const std::string& body)
{
	// Basit "cmd" field parse: {"cmd":"/permanent metin"}
	size_t pos = body.find("\"cmd\"");
	if (pos == std::string::npos)
		return "";
	pos = body.find(":", pos);
	if (pos == std::string::npos)
		return "";
	pos = body.find("\"", pos);
	if (pos == std::string::npos)
		return "";
	pos++;
	size_t end = body.find("\"", pos);
	if (end == std::string::npos)
		return "";
	return body.substr(pos, end - pos);
}

#endif // HTTP_CMD_SERVER
