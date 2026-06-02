#pragma once

#ifdef HTTP_CMD_SERVER

#include <string>
#include <thread>
#include <atomic>

class CHttpCmdServer
{
public:
	static void Start();
	static void Stop();

private:
	static void ListenerThread();
	static void HandleRequest(SOCKET client);

	static bool VerifyToken(const std::string& token);
	static bool IsWhitelisted(const std::string& cmd);

	// Audit log (dosya + DB _MK_GM_AUDIT). FAIL-SAFE: hata komut akisini durdurmaz (KURAL 1).
	static void WriteAudit(const std::string& tokenShort, const std::string& clientIP,
		const std::string& cmd, const std::string& params, int httpCode,
		const std::string& durum, bool yikici);

	static std::string ExtractHeader(const std::string& request, const std::string& headerName);
	static std::string ExtractBody(const std::string& request);
	static std::string ExtractCommandFromJson(const std::string& body);

	static std::atomic<bool> s_running;
	static SOCKET s_listenSocket;
	static std::thread s_thread;
	static std::string s_token;
};

#endif // HTTP_CMD_SERVER
