#pragma once

#include <cstdio>
#include <ctime>
#include <string>
#include <mutex>

enum class LogCategory : uint8_t {
	LOG_GENERAL,
	LOG_TRADE,
	LOG_HACK,
	LOG_GM,
	LOG_ERROR,
	LOG_MERCHANT,
	LOG_LOGIN,
	LOG_DISCONNECT,
	LOG_ZONE,
	LOG_ODBC,
	LOG_COUNT
};

static const char* LogCategoryNames[] = {
	"GENERAL",
	"TRADE",
	"HACK",
	"GM",
	"ERROR",
	"MERCHANT",
	"LOGIN",
	"DISCONNECT",
	"ZONE",
	"ODBC"
};

class LogSystem {
public:
	static LogSystem& Instance() {
		static LogSystem inst;
		return inst;
	}

	void Initialize(const std::string& logDir = "./Logs") {
		m_logDir = logDir;
	}

	void Log(LogCategory cat, const char* fmt, ...) {
		char msgBuf[2048];
		va_list args;
		va_start(args, fmt);
		vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
		va_end(args);

		time_t now = time(nullptr);
		struct tm t;
		localtime_s(&t, &now);

		char timeBuf[64];
		snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d",
			t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
			t.tm_hour, t.tm_min, t.tm_sec);

		char dateBuf[16];
		snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d",
			t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

		int catIdx = (int)cat;
		if (catIdx < 0 || catIdx >= (int)LogCategory::LOG_COUNT)
			catIdx = 0;

		char filePath[512];
		snprintf(filePath, sizeof(filePath), "%s/%s_%s.log",
			m_logDir.c_str(), LogCategoryNames[catIdx], dateBuf);

		std::lock_guard<std::mutex> lock(m_mutex);

		FILE* fp = fopen(filePath, "a");
		if (fp) {
			fprintf(fp, "[%s] [%s] %s\n", timeBuf, LogCategoryNames[catIdx], msgBuf);
			fclose(fp);
		}
	}

private:
	LogSystem() : m_logDir("./Logs") {}
	LogSystem(const LogSystem&) = delete;
	LogSystem& operator=(const LogSystem&) = delete;

	std::string m_logDir;
	std::mutex m_mutex;
};

#define LOG(cat, ...) LogSystem::Instance().Log(cat, __VA_ARGS__)
#define LOG_TRADE(...) LOG(LogCategory::LOG_TRADE, __VA_ARGS__)
#define LOG_HACK(...) LOG(LogCategory::LOG_HACK, __VA_ARGS__)
#define LOG_GM(...) LOG(LogCategory::LOG_GM, __VA_ARGS__)
#define LOG_ERROR(...) LOG(LogCategory::LOG_ERROR, __VA_ARGS__)
#define LOG_MERCHANT(...) LOG(LogCategory::LOG_MERCHANT, __VA_ARGS__)
#define LOG_LOGIN(...) LOG(LogCategory::LOG_LOGIN, __VA_ARGS__)
#define LOG_DISCONNECT(...) LOG(LogCategory::LOG_DISCONNECT, __VA_ARGS__)
#define LOG_ZONE(...) LOG(LogCategory::LOG_ZONE, __VA_ARGS__)
#define LOG_ODBC(...) LOG(LogCategory::LOG_ODBC, __VA_ARGS__)
