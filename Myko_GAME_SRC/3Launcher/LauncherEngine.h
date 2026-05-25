#pragma once
#include "stdafx.h"
#include "ftpclient/FTPClient.h"
#include "zlib\zip.h"
#include "APISocket.h"
#include "hdr.h"
#include <format>
#include <winhttp.h>  // S115 AUTO-UPDATE: INTERNET_PORT typedef

// S115 AUTO-UPDATE: Bu Launcher'in build versiyonu. Her yeni build'de ARTIR.
// Sunucudaki version.txt'de bu degerden buyuk varsa, Launcher otomatik gunceller.
#define LAUNCHER_BUILD_VERSION_MAJOR 1
#define LAUNCHER_BUILD_VERSION_MINOR 6

class Launcher
{
public:
	CAPISocket* mSocket;
	Launcher();
	~Launcher();
	bool Start();
	void RequestVersion();
	void RequestPatch();
	void RequestNotices();
	void ReportHwid();  // S114 K3 FAZ 4: HWID'i server'a yolla, ban kontrolu
	// S115 AUTO-UPDATE: version.txt cek, yeni varsa indir, helper.bat ile restart
	bool CheckForUpdate();
	bool HttpGet(const std::wstring& host, INTERNET_PORT port,
	             const std::wstring& path, std::vector<BYTE>& outData,
	             DWORD maxSizeBytes = 50 * 1024 * 1024);
	bool DownloadUpdateFile(const std::string& expectedMd5, size_t expectedSize,
	                        std::string& outTempPath);
	bool LaunchUpdaterAndExit(const std::string& tempNewExePath);

	// S115 CRASH REPORTER: dump uret + sunucuya yolla
	static void InstallCrashHandler();
	static bool UploadCrashDump(const std::string& dumpPath,
	                            const std::string& exeName,
	                            const std::string& account = "",
	                            const std::string& version = "");
	bool HandlePacket(Packet& pkt);
	void Update();
	void Download();
	bool KnightOnlineCheck();
	bool DownloadPatch(std::string server, std::string path, std::string file, std::string expectedHash);
	void CheckTBLHashes();
	bool ScanCheatTools(std::string& detected); // S114: KOXP/cheat tool scan (process+window+DLL+driver)
	// S114 K3: HWID_A hesap (HDD volume + MAC -> MD5). Login pakete eklenir, server ban listesine bakar.
	std::string ComputeHwidA();
	std::string m_hwidA;  // Cache (her acilista 1 kez hesap)
	// S114: Scan sonucu cache (START click sirasinda GIF goster icin)
	bool m_scanThreatDetected = false;
	std::string m_scanThreatName;
	// S114 K3: HWID ban flag (server'dan 0x84 result=1 alinca set olur)
	bool m_hwidBanned = false;
	// S114: TBL hash sonucu (cheat suphesi degil ama dosya butunlugu)
	int m_tblMismatchCount = 0;
	int m_tblMissingCount = 0;
	std::string m_tblMismatchList;
	short GetVersion() { return m_iVersion; }
	std::string GetState() { return m_stateString;  }
	void SetState(std::string state) { m_stateString = state; }
	bool IsReady() { return ready; }
	uint8 GetPercent() { return m_dPercent; };
	void SetPercent(uint8 per) { m_dPercent = per; };
	bool m_bVersionGot;
	bool m_bPatchesGot;
	std::string m_currentFile;
	std::vector<std::string> m_lNotices;
	CHAR WorkingPath[MAX_PATH];
	std::string m_settingsIP;
	HWND window;
	bool ready;
	uint32 ipParam;
	std::string cmd;
	char m_strBasePath[MAX_PATH] = { 0 };
private:
	short m_iVersion;
	short m_settingsVersion;
	std::string m_stateString;
	
	uint8 m_dPercent;
	
};

extern Launcher* Engine;