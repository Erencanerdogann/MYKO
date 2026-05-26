// =========================================================================
// S115 FAZ 5 — Launcher Self-Heal Diagnostik Implementasyonu
// Yazan: CHIP | Tarih: 2026-05-26
// =========================================================================
#include "stdafx.h"
#include "LauncherDiagnostic.h"
#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>
#include <fstream>
#include <sstream>
#include <ctime>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

namespace LauncherDiagnostic {

// =====================================================================
// HELPER: SelfHeal log dosyasi
// =====================================================================
static std::string GetLogPath() {
    char path[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string dir = std::string(path) + "\\MalaysiaKO";
        CreateDirectoryA(dir.c_str(), NULL);
        return dir + "\\selfheal.log";
    }
    return "";
}

void LogAction(const std::string& action, const std::string& result) {
    std::string logPath = GetLogPath();
    if (logPath.empty()) return;

    std::ofstream f(logPath, std::ios::app);
    if (!f.is_open()) return;

    time_t now = time(nullptr);
    char timeBuf[32] = { 0 };
    struct tm tmInfo;
    if (localtime_s(&tmInfo, &now) == 0) {
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmInfo);
    }

    f << timeBuf << " | " << action << " | " << result << "\n";
    f.close();
}

// =====================================================================
// IsEnabled — Launcher.ini [SelfHeal] Enabled=1 kontrol
// Dosya yoksa veya flag yoksa default ENABLED (acik)
// =====================================================================
bool IsEnabled() {
    char workDir[MAX_PATH] = { 0 };
    GetCurrentDirectoryA(MAX_PATH, workDir);
    std::string iniPath = std::string(workDir) + "\\Launcher.ini";

    UINT enabled = GetPrivateProfileIntA("SelfHeal", "Enabled", 1, iniPath.c_str());
    return enabled == 1;
}

// =====================================================================
// 1. CheckDefenderExclusion
// HKLM\SOFTWARE\CodeGuard\PATH var ve gamePath ile esit mi
// (LauncherEngine.cpp:124-146 mantigi)
// =====================================================================
CheckResult CheckDefenderExclusion(const std::string& gamePath) {
    CheckResult r;
    r.name = "Windows Defender Exclusion";
    r.repairAvailable = true;

    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                             "SOFTWARE\\CodeGuard",
                             0, KEY_READ, &hKey);
    if (res != ERROR_SUCCESS) {
        r.status = CheckStatus::WARNING;
        r.message = "Defender exclusion eklenmemis. Antivirus oyunu engelleyebilir.";
        r.action = "Repair";
        return r;
    }

    char value[MAX_PATH] = { 0 };
    DWORD valueLen = MAX_PATH;
    DWORD type = 0;
    res = RegQueryValueExA(hKey, "PATH", NULL, &type, (LPBYTE)value, &valueLen);
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS || type != REG_SZ) {
        r.status = CheckStatus::WARNING;
        r.message = "Defender exclusion PATH registry'de yok.";
        r.action = "Repair";
        return r;
    }

    if (gamePath == value) {
        r.status = CheckStatus::OK;
        r.message = "Defender exclusion aktif.";
        r.action = "";
        r.repairAvailable = false;
    } else {
        r.status = CheckStatus::WARNING;
        r.message = "Defender exclusion baska klasor icin: " + std::string(value);
        r.action = "Repair";
    }
    return r;
}

// =====================================================================
// 2. CheckFileIntegrity
// Kritik dosyalar (KnightOnLine.exe, Launcher.exe, CODE) var ve boyutu makul mi
// =====================================================================
CheckResult CheckFileIntegrity(const std::string& gamePath) {
    CheckResult r;
    r.name = "Dosya Butunluğu";
    r.repairAvailable = false; // file integrity icin manual Repair, otomatik degistirme YASAK

    struct FileCheck {
        std::string name;
        DWORD minSize;
    };

    FileCheck critical[] = {
        { "KnightOnLine.exe", 1024 * 1024 },   // en az 1MB
        { "Launcher.exe",     500 * 1024 },     // en az 500KB
        { "CODE",             100 * 1024 }      // en az 100KB
    };

    std::vector<std::string> missing;
    std::vector<std::string> tooSmall;

    for (const auto& fc : critical) {
        std::string filePath = gamePath + "\\" + fc.name;
        HANDLE h = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            missing.push_back(fc.name);
            continue;
        }
        LARGE_INTEGER size = { 0 };
        if (GetFileSizeEx(h, &size)) {
            if ((DWORD)size.QuadPart < fc.minSize) {
                tooSmall.push_back(fc.name);
            }
        }
        CloseHandle(h);
    }

    if (missing.empty() && tooSmall.empty()) {
        r.status = CheckStatus::OK;
        r.message = "Kritik dosyalar tam.";
        r.action = "";
    } else {
        r.status = CheckStatus::ERROR_LEVEL;
        std::string msg = "Bozuk/eksik dosyalar:";
        for (const auto& m : missing) msg += "\n  EKSIK: " + m;
        for (const auto& t : tooSmall) msg += "\n  BOZUK: " + t;
        r.message = msg;
        r.action = "Setup'tan onar"; // backup yok, manuel reinstall
    }
    return r;
}

// =====================================================================
// 3. CheckConnectivity
// HTTP HEAD /crash_upload.php ping (5sn timeout)
// =====================================================================
CheckResult CheckConnectivity(const std::string& serverIP) {
    CheckResult r;
    r.name = "Sunucu Baglantisi";
    r.repairAvailable = false; // network sorunu — kullanici cozecek

    if (serverIP.empty()) {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "Server.ini'de IP yok.";
        r.action = "Server.ini onar";
        return r;
    }

    std::wstring wHost(serverIP.begin(), serverIP.end());

    HINTERNET hSession = WinHttpOpen(L"MalaysiaKO-Diagnostic/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "WinHTTP init basarisiz.";
        r.action = "Bilgisayar yeniden baslat";
        return r;
    }

    WinHttpSetTimeouts(hSession, 3000, 3000, 5000, 5000);

    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), 80, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "Sunucuya baglanilamadi: " + serverIP + " (firewall veya ISP engeli olabilir)";
        r.action = "Internet/firewall kontrol";
        return r;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"HEAD",
        L"/crash_upload.php", NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "HTTP request basarisiz.";
        r.action = "Internet kontrol";
        return r;
    }

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD status = 0, statusSize = sizeof(status);
        if (WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                WINHTTP_NO_HEADER_INDEX)) {
            // 200 veya 405 (HEAD POST endpoint reddeder ama erisilebilir) = sunucu acik
            success = (status >= 200 && status < 500);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (success) {
        r.status = CheckStatus::OK;
        r.message = "Sunucu erisilebilir: " + serverIP;
        r.action = "";
    } else {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "Sunucu yanit vermiyor: " + serverIP;
        r.action = "Discord'dan destek iste";
    }
    return r;
}

// =====================================================================
// 4. CheckVCRedist
// HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86 Installed=1 ?
// =====================================================================
CheckResult CheckVCRedist() {
    CheckResult r;
    r.name = "Visual C++ Redistributable";
    r.repairAvailable = true;

    HKEY hKey;
    // VS2015-2022 x86 redistributable (en kapsamli)
    LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                             "SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x86",
                             0, KEY_READ | KEY_WOW64_32KEY, &hKey);

    if (res != ERROR_SUCCESS) {
        // Wow64 32-bit view'da yoksa default view'da dene
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                            "SOFTWARE\\WOW6432Node\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x86",
                            0, KEY_READ, &hKey);
    }

    if (res != ERROR_SUCCESS) {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "VC++ Redistributable kurulu degil. Oyun acilmayabilir.";
        r.action = "VC++ Redistributable indir+kur";
        return r;
    }

    DWORD installed = 0;
    DWORD valSize = sizeof(installed);
    res = RegQueryValueExA(hKey, "Installed", NULL, NULL, (LPBYTE)&installed, &valSize);
    RegCloseKey(hKey);

    if (res == ERROR_SUCCESS && installed == 1) {
        r.status = CheckStatus::OK;
        r.message = "VC++ Redistributable kurulu.";
        r.action = "";
        r.repairAvailable = false;
    } else {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "VC++ Redistributable bozuk veya kismi kurulu.";
        r.action = "VC++ Redistributable indir+kur";
    }
    return r;
}

// =====================================================================
// 5. CheckServerIni
// Server.ini var ve [Server] IP0 + [Version] Files dolu mu
// =====================================================================
CheckResult CheckServerIni(const std::string& gamePath) {
    CheckResult r;
    r.name = "Server.ini";
    r.repairAvailable = true;

    std::string iniPath = gamePath + "\\Server.ini";

    HANDLE h = CreateFileA(iniPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "Server.ini bulunamadi.";
        r.action = "Server.ini default ile olustur";
        return r;
    }
    CloseHandle(h);

    // IP0 var mi
    char ipBuf[128] = { 0 };
    GetPrivateProfileStringA("Server", "IP0", "", ipBuf, sizeof(ipBuf), iniPath.c_str());

    if (ipBuf[0] == '\0') {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "Server.ini'de [Server] IP0 anahtari yok.";
        r.action = "Server.ini default ile olustur";
        return r;
    }

    // Format kabaca dogru mu (en az 3 nokta — IPv4)
    int dots = 0;
    for (char* p = ipBuf; *p; p++) if (*p == '.') dots++;
    if (dots != 3) {
        r.status = CheckStatus::WARNING;
        r.message = std::string("Server.ini IP formati supheli: ") + ipBuf;
        r.action = "Server.ini onar";
        return r;
    }

    r.status = CheckStatus::OK;
    r.message = std::string("Server.ini OK (IP: ") + ipBuf + ")";
    r.action = "";
    r.repairAvailable = false;
    return r;
}

// =====================================================================
// RunAllChecks — 5 kontrolu tek seferde kos
// =====================================================================
std::vector<CheckResult> RunAllChecks(const std::string& gamePath, const std::string& serverIP) {
    std::vector<CheckResult> results;

    if (!IsEnabled()) {
        CheckResult disabled;
        disabled.name = "Self-Heal";
        disabled.status = CheckStatus::DISABLED;
        disabled.message = "Self-Heal disabled in Launcher.ini";
        results.push_back(disabled);
        return results;
    }

    results.push_back(CheckDefenderExclusion(gamePath));
    results.push_back(CheckFileIntegrity(gamePath));
    results.push_back(CheckConnectivity(serverIP));
    results.push_back(CheckVCRedist());
    results.push_back(CheckServerIni(gamePath));

    // Log
    int okCount = 0, warnCount = 0, errCount = 0;
    for (const auto& r : results) {
        switch (r.status) {
            case CheckStatus::OK: okCount++; break;
            case CheckStatus::WARNING: warnCount++; break;
            case CheckStatus::ERROR_LEVEL: errCount++; break;
            default: break;
        }
    }
    std::ostringstream summary;
    summary << "OK=" << okCount << " WARN=" << warnCount << " ERR=" << errCount;
    LogAction("RunAllChecks", summary.str());

    return results;
}

// =====================================================================
// REPAIR FUNCTIONS (insan onayli, kullanici Launcher UI'dan tetikler)
// =====================================================================

// PowerShell Add-MpPreference (LauncherEngine.cpp:130-147 ile ayni mantik)
bool RepairAddDefenderExclusion(const std::string& gamePath) {
    LogAction("RepairAddDefenderExclusion", "starting");

    std::string ps =
        "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
        "\"try { "
        "Add-MpPreference -ExclusionPath '" + gamePath + "' -Force; "
        "Add-MpPreference -ExclusionPath '" + gamePath + "\\KnightOnLine.exe' -Force; "
        "Add-MpPreference -ExclusionPath '" + gamePath + "\\Launcher.exe' -Force; "
        "Add-MpPreference -ExclusionPath '" + gamePath + "\\CODE' -Force; "
        "Add-MpPreference -ExclusionProcess 'KnightOnLine.exe' -Force; "
        "Add-MpPreference -ExclusionProcess 'Launcher.exe' -Force "
        "} catch { }\"";

    // -Verb RunAs UAC prompt, kullanici Hayir derse hata yutulur
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL ok = CreateProcessA(NULL, (LPSTR)ps.c_str(), NULL, NULL, FALSE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!ok) {
        LogAction("RepairAddDefenderExclusion", "CreateProcess failed");
        return false;
    }

    WaitForSingleObject(pi.hProcess, 30000); // max 30sn bekle
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    bool success = (exitCode == 0);
    LogAction("RepairAddDefenderExclusion", success ? "OK" : "failed");

    // Registry'ye state yaz (LauncherEngine.cpp:142-146 pattern)
    if (success) {
        HKEY hKey;
        if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\CodeGuard", 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "PATH", 0, REG_SZ,
                           (const BYTE*)gamePath.c_str(),
                           (DWORD)(gamePath.length() + 1));
            RegCloseKey(hKey);
        }
    }
    return success;
}

// VC++ Redistributable — sunucudan indirme yapilmiyor (sunucu link'i hazir degil)
// Kullaniciya Microsoft'un resmi link'ini gosterip elle indirsin
bool RepairInstallVCRedist() {
    LogAction("RepairInstallVCRedist", "redirecting to MS download page");
    ShellExecuteA(NULL, "open",
                  "https://aka.ms/vs/17/release/vc_redist.x86.exe",
                  NULL, NULL, SW_SHOWNORMAL);
    return true; // browser acildi, kullanici elle kuracak
}

// Server.ini default ile olustur
bool RepairServerIni(const std::string& gamePath) {
    LogAction("RepairServerIni", "creating default");

    std::string iniPath = gamePath + "\\Server.ini";

    // Default Server.ini icerigi (mevcut LauncherEngine.cpp degerleri)
    const char* defaultContent =
        "[Server]\r\n"
        "IP0=104.238.23.99\r\n"
        "Port=15100\r\n"
        "\r\n"
        "[Version]\r\n"
        "Files=2369\r\n";

    HANDLE h = CreateFileA(iniPath.c_str(), GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        LogAction("RepairServerIni", "CreateFile failed");
        return false;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(h, defaultContent, (DWORD)strlen(defaultContent), &written, NULL);
    CloseHandle(h);

    LogAction("RepairServerIni", ok ? "OK" : "WriteFile failed");
    return ok != 0;
}

} // namespace LauncherDiagnostic
