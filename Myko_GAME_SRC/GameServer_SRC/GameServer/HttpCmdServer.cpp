#include "stdafx.h"

#ifdef HTTP_CMD_SERVER

#include "HttpCmdServer.h"
#include "GameServerDlg.h"
#include <winsock2.h>
#include <fstream>
#include <sstream>
#include <set>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> CHttpCmdServer::s_running(false);
SOCKET CHttpCmdServer::s_listenSocket = INVALID_SOCKET;
std::thread CHttpCmdServer::s_thread;
std::string CHttpCmdServer::s_token;

static const std::set<std::string> s_whitelist = {
	"permanent", "offpermanent", "notice", "noticeall",
	"reloadnotice", "reloadalltables", "reloaditems", "reloaddrops",
	"reloadquests", "reloadmagics", "reloadkings", "reloadevent",
	"reloadtables", "reloadtables2", "reloadtables3",
	"open1", "open2", "open3", "open4", "open5", "open6",
	"snow", "csw", "close", "cswclose",
	"santa", "santaclose", "angel", "discount", "alldiscount", "offdiscount"
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

void CHttpCmdServer::HandleRequest(SOCKET client)
{
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
	if (!VerifyToken(token))
	{
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

	if (!IsWhitelisted(cmdName))
	{
		const char* resp = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
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
		const char* resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
		send(client, resp, (int)strlen(resp), 0);
		return;
	}

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
