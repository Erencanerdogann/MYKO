#pragma once

#include <string>
#include <mutex>
#include <map>

class ServerConfig {
public:
	static ServerConfig& Instance() {
		static ServerConfig inst;
		return inst;
	}

	void LoadFromINI(const char* filePath) {
		std::lock_guard<std::mutex> lock(m_mutex);

		FILE* fp = fopen(filePath, "r");
		if (!fp) return;

		char line[512];
		std::string currentSection;
		while (fgets(line, sizeof(line), fp)) {
			std::string s(line);
			// trim
			while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
				s.pop_back();
			if (s.empty() || s[0] == ';' || s[0] == '#')
				continue;
			if (s[0] == '[') {
				size_t end = s.find(']');
				if (end != std::string::npos)
					currentSection = s.substr(1, end - 1);
				continue;
			}
			size_t eq = s.find('=');
			if (eq == std::string::npos) continue;
			std::string key = currentSection + "." + s.substr(0, eq);
			std::string val = s.substr(eq + 1);
			// trim key
			while (!key.empty() && key.back() == ' ') key.pop_back();
			// trim val leading space
			while (!val.empty() && val.front() == ' ') val.erase(val.begin());
			m_values[key] = val;
		}
		fclose(fp);
	}

	int GetInt(const char* key, int defaultVal = 0) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_values.find(key);
		if (it == m_values.end()) return defaultVal;
		return atoi(it->second.c_str());
	}

	std::string GetString(const char* key, const std::string& defaultVal = "") {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_values.find(key);
		if (it == m_values.end()) return defaultVal;
		return it->second;
	}

	bool SetValue(const std::string& key, const std::string& value) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_values[key] = value;
		return true;
	}

	std::string GetValueList() {
		std::lock_guard<std::mutex> lock(m_mutex);
		std::string result;
		for (auto& kv : m_values) {
			result += kv.first + "=" + kv.second + "\n";
		}
		return result;
	}

	// Convenience — runtime-configurable values with defaults
	int PacketRateLimit() { return GetInt("SERVER.packet_rate_limit", 150); }
	int SaveInterval() { return GetInt("SERVER.save_interval", 600); }
	int IPBanSeconds() { return GetInt("SERVER.ip_ban_seconds", 3600); }
	int IPMaxFails() { return GetInt("SERVER.ip_max_fails", 10); }
	int MoveBroadcastInterval() { return GetInt("SERVER.move_broadcast_ms", 50); }
	int NpcDamageListCap() { return GetInt("SERVER.npc_damage_cap", 30); }
	// 0=off, 1=log-only, 2=kick (3+ violate in 60s), 3=ban (10+ total)
	int WallCheatMode() { return GetInt("SERVER.wall_cheat_mode", 1); }
	int QueueMaxSize() { return GetInt("SERVER.queue_max_size", 10000); }
	int SocketBufferSize() { return GetInt("SERVER.socket_buffer_kb", 64); }
	// 0=normal (herkes girer), 1=maintenance (sadece GM girer) — acilis oncesi GM-only lock
	int MaintenanceMode() { return GetInt("SERVER.maintenance_mode", 0); }

private:
	ServerConfig() {}
	ServerConfig(const ServerConfig&) = delete;
	ServerConfig& operator=(const ServerConfig&) = delete;

	std::map<std::string, std::string> m_values;
	std::mutex m_mutex;
};

#define g_ServerConfig ServerConfig::Instance()
