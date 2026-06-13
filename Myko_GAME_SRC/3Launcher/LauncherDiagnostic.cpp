// =========================================================================
// S115 FAZ 5 — Launcher Self-Heal Diagnostik Implementasyonu
// Yazan: CHIP | Tarih: 2026-05-26
// =========================================================================
#include "stdafx.h"
#include "LauncherDiagnostic.h"
#include "resource.h"  // FAZ 5c: dialog ID'leri
#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>  // std::transform (FAZ 5c CheckDefenderExclusion lowercase)
#include <commctrl.h>

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
// UploadDiagLogs (S133) — client diag_*.log dosyalarini sunucuya yollar.
// Multipart/form-data POST -> http://<IP>:8091/api/diag_upload.php
//   field'lar: account(text), hwid(text), log_dosyasi(file, .log, max 500KB).
// Launcher acilisinda arka planda cagrilir (onceki oturum loglarini yakalar).
// Fail-safe (KURAL 1): hata/timeout -> sessiz doner, oyun/Launcher ETKILENMEZ.
// Basarili (HTTP 200) upload edilen dosya SILINIR (tekrar gonderilmez).
// =====================================================================
static bool UploadOneDiagFile(const std::wstring& wHost, const std::string& filePath,
                              const std::string& account, const std::string& hwid)
{
    // Dosyayi oku (max 500KB — endpoint limiti)
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    std::streamsize sz = in.tellg();
    if (sz <= 0 || sz > 500 * 1024) { in.close(); return false; } // bos veya >500KB atla
    in.seekg(0, std::ios::beg);
    std::string content((size_t)sz, '\0');
    in.read(&content[0], sz);
    in.close();

    // dosya adini al (yoldan ayir)
    std::string fname = filePath;
    size_t slash = fname.find_last_of("\\/");
    if (slash != std::string::npos) fname = fname.substr(slash + 1);

    // multipart/form-data govde olustur
    const std::string boundary = "----MalaysiaKODiag7e3b91c2";
    std::string body;
    auto addField = [&](const std::string& name, const std::string& val) {
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
        body += val + "\r\n";
    };
    addField("account", account.empty() ? std::string("unknown") : account);
    addField("hwid", hwid.empty() ? std::string("unknown") : hwid);
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"log_dosyasi\"; filename=\"" + fname + "\"\r\n";
    body += "Content-Type: text/plain\r\n\r\n";
    body += content + "\r\n";
    body += "--" + boundary + "--\r\n";

    HINTERNET hSession = WinHttpOpen(L"MalaysiaKO-Diagnostic/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;
    WinHttpSetTimeouts(hSession, 3000, 3000, 5000, 8000);

    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), 8091, 0); // site portu
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/diag_upload.php",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::wstring ctype = L"Content-Type: multipart/form-data; boundary=" +
        std::wstring(boundary.begin(), boundary.end());

    bool ok = false;
    if (WinHttpSendRequest(hRequest, ctype.c_str(), (DWORD)-1L,
            (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0) &&
        WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD status = 0, statusSize = sizeof(status);
        if (WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
            ok = (status == 200);
        }
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

void UploadDiagLogs(const std::string& gamePath, const std::string& hwid, const std::string& serverIP)
{
    std::string ip = serverIP.empty() ? std::string("104.238.23.99") : serverIP;
    std::wstring wHost(ip.begin(), ip.end());

    // diag_*.log ara: oncelik gamePath, yoksa C:\MalaysiaKO (DiagLog default klasoru)
    std::string base = gamePath.empty() ? std::string("C:\\MalaysiaKO") : gamePath;
    if (!base.empty() && (base.back() == '\\' || base.back() == '/')) base.pop_back();
    std::string pattern = base + "\\diag_*.log";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return; // log yok -> sessiz cik (normal)

    int uploaded = 0, failed = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::string full = base + "\\" + fd.cFileName;
        if (UploadOneDiagFile(wHost, full, hwid, hwid)) {
            DeleteFileA(full.c_str()); // basarili -> sil (tekrar gonderilmesin)
            uploaded++;
        } else {
            failed++; // basarisiz -> dokunma, sonraki acilista tekrar denenir
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    if (uploaded > 0 || failed > 0)
        LogAction("UploadDiagLogs", "uploaded=" + std::to_string(uploaded) + " failed=" + std::to_string(failed));
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
// FAZ 5c FIX: Gercek Defender exclusion sorgu (PowerShell Get-MpPreference)
// Eski: registry HKLM\SOFTWARE\CodeGuard\PATH — aldatici, eski Launcher yazmis olabilir
// Yeni: gercekten Defender'dan oku
CheckResult CheckDefenderExclusion(const std::string& gamePath) {
    CheckResult r;
    r.name = "Windows Defender Exclusion";
    r.repairAvailable = true;

    // Once Defender aktif mi (Windows Security disabled olabilir)
    {
        HKEY hKey;
        LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                 "SOFTWARE\\Microsoft\\Windows Defender",
                                 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
        if (res != ERROR_SUCCESS) {
            r.status = CheckStatus::OK;
            r.message = "Defender bulunamadi (devre disi veya 3.parti antivirus).";
            r.action = "";
            r.repairAvailable = false;
            return r;
        }
        RegCloseKey(hKey);
    }

    // PowerShell ile gercek exclusion listesini al
    // Cikti: lokal temp dosyaya yaz, sonra oku
    char tempDir[MAX_PATH] = { 0 };
    GetTempPathA(MAX_PATH, tempDir);
    std::string outFile = std::string(tempDir) + "mk_diag_excl.txt";

    // Lower-case gamePath karsilastirma icin
    std::string gpLower = gamePath;
    std::transform(gpLower.begin(), gpLower.end(), gpLower.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });

    // PowerShell command — exclusion path list lowercase dump
    // -ErrorAction SilentlyContinue cunku Defender disabled olabilir
    std::string psCmd =
        "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
        "\"try { "
        "(Get-MpPreference -ErrorAction SilentlyContinue).ExclusionPath | "
        "ForEach-Object { $_.ToLower() } | Out-File -Encoding ASCII -FilePath '" + outFile + "' "
        "} catch { '' | Out-File -Encoding ASCII -FilePath '" + outFile + "' }\"";

    // Sessiz calistir
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    std::string psCmdMutable = psCmd;
    DeleteFileA(outFile.c_str()); // eski varsa sil

    BOOL ok = CreateProcessA(NULL, (LPSTR)psCmdMutable.c_str(), NULL, NULL, FALSE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!ok) {
        r.status = CheckStatus::WARNING;
        r.message = "Defender durumu kontrol edilemedi (PowerShell hatasi).";
        r.action = "Repair";
        return r;
    }
    WaitForSingleObject(pi.hProcess, 5000);  // max 5sn
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Cikti oku
    std::ifstream f(outFile);
    if (!f.is_open()) {
        r.status = CheckStatus::WARNING;
        r.message = "Defender exclusion sorgulanamadi.";
        r.action = "Repair";
        return r;
    }

    bool found = false;
    std::string line;
    while (std::getline(f, line)) {
        // Trim whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
                                  line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (line.empty()) continue;
        // gamePath veya altinda mi
        if (line == gpLower || line.find(gpLower) != std::string::npos) {
            found = true;
            break;
        }
    }
    f.close();
    DeleteFileA(outFile.c_str()); // temp temizle

    if (found) {
        r.status = CheckStatus::OK;
        r.message = "Defender exclusion aktif (klasor istisna listede).";
        r.action = "";
        r.repairAvailable = false;
    } else {
        r.status = CheckStatus::WARNING;
        r.message = "Defender'da bu klasor icin istisna YOK. Onar dugmesine bas.";
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
    r.name = "Dosya Butunlugu";
    r.repairAvailable = false; // file integrity icin manual Repair, otomatik degistirme YASAK

    struct FileCheck {
        std::string name;
        DWORD minSize;
        bool critical;  // critical=true ise eksiklik ERROR, false ise WARNING
    };

    // FAZ 5c FIX: CODE opsiyonel (anti-cheat package, gercek launcher klasorunde olur ama
    // development veya custom kurulumlarda olmayabilir). Sadece KnightOnLine.exe + Launcher.exe kritik.
    FileCheck critical[] = {
        { "KnightOnLine.exe", 1024 * 1024, true  },   // en az 1MB - KRITIK
        { "Launcher.exe",     500 * 1024,  true  },   // en az 500KB - KRITIK
        { "CODE",             100 * 1024,  false }    // en az 100KB - opsiyonel (anti-cheat)
    };

    std::vector<std::string> missingCrit;
    std::vector<std::string> missingOpt;
    std::vector<std::string> tooSmallCrit;
    std::vector<std::string> tooSmallOpt;

    for (const auto& fc : critical) {
        std::string filePath = gamePath + "\\" + fc.name;
        HANDLE h = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            if (fc.critical) missingCrit.push_back(fc.name);
            else             missingOpt.push_back(fc.name);
            continue;
        }
        LARGE_INTEGER size = { 0 };
        if (GetFileSizeEx(h, &size)) {
            if ((DWORD)size.QuadPart < fc.minSize) {
                if (fc.critical) tooSmallCrit.push_back(fc.name);
                else             tooSmallOpt.push_back(fc.name);
            }
        }
        CloseHandle(h);
    }

    // ERROR sadece KRITIK eksik/bozuk varsa
    if (!missingCrit.empty() || !tooSmallCrit.empty()) {
        r.status = CheckStatus::ERROR_LEVEL;
        std::string msg = "KRITIK dosya eksik/bozuk: ";
        bool first = true;
        for (const auto& m : missingCrit) { if (!first) msg += ", "; msg += m + " (EKSIK)"; first = false; }
        for (const auto& t : tooSmallCrit) { if (!first) msg += ", "; msg += t + " (BOZUK)"; first = false; }
        r.message = msg;
        r.action = "Setup'tan onar";
    }
    // WARNING sadece opsiyonel eksik (CODE gibi)
    else if (!missingOpt.empty() || !tooSmallOpt.empty()) {
        r.status = CheckStatus::WARNING;
        std::string msg = "Opsiyonel dosya eksik (anti-cheat): ";
        bool first = true;
        for (const auto& m : missingOpt) { if (!first) msg += ", "; msg += m; first = false; }
        for (const auto& t : tooSmallOpt) { if (!first) msg += ", "; msg += t + "(bozuk)"; first = false; }
        msg += ". Oyuna girince Launcher otomatik indirir.";
        r.message = msg;
        r.action = "";
    }
    else {
        r.status = CheckStatus::OK;
        r.message = "Kritik dosyalar tam.";
        r.action = "";
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

    // FAZ 5c FIX: Server.ini IP bossa default sunucu IP'sini kullan (test yapilabilsin)
    std::string ip = serverIP;
    if (ip.empty()) {
        ip = "104.238.23.99";  // Default PROD IP — Server.ini fallback
    }

    std::wstring wHost(ip.begin(), ip.end());

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
        r.message = "Sunucuya baglanilamadi: " + ip + " (firewall, ISP veya VPN engeli)";
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
        r.message = "Sunucu erisilebilir: " + ip;
        r.action = "";
    } else {
        r.status = CheckStatus::ERROR_LEVEL;
        r.message = "Sunucu yanit vermiyor: " + ip;
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

    // PowerShell argumanlari — Add-MpPreference admin gerek
    // ShellExecute -Verb runas ile UAC prompt acilir.
    // Eski LauncherEngine.cpp:131-141 ile AYNI mantik (registry state dahil)
    std::string psArgs =
        "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
        "\"try { "
        "Add-MpPreference -ExclusionPath '" + gamePath + "' -Force; "
        "Add-MpPreference -ExclusionPath '" + gamePath + "\\KnightOnLine.exe' -Force; "
        "Add-MpPreference -ExclusionPath '" + gamePath + "\\Launcher.exe' -Force; "
        "Add-MpPreference -ExclusionPath '" + gamePath + "\\CODE' -Force; "
        "Add-MpPreference -ExclusionProcess 'KnightOnLine.exe' -Force; "
        "Add-MpPreference -ExclusionProcess 'Launcher.exe' -Force "
        "} catch { }\"";

    // ShellExecuteEx with runas verb — UAC prompt acilir, admin elevation
    SHELLEXECUTEINFOA sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.fMask  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = "runas";              // ← UAC ELEVATION
    sei.lpFile = "powershell.exe";
    sei.lpParameters = psArgs.c_str();
    sei.nShow  = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            // Kullanici UAC prompt'a "Hayir" dedi — normal akis, hata degil
            LogAction("RepairAddDefenderExclusion", "user cancelled UAC");
            return false;
        }
        LogAction("RepairAddDefenderExclusion", "ShellExecuteEx failed err=" + std::to_string(err));
        return false;
    }

    if (!sei.hProcess) {
        LogAction("RepairAddDefenderExclusion", "no process handle");
        return false;
    }

    WaitForSingleObject(sei.hProcess, 30000); // max 30sn bekle
    DWORD exitCode = 0;
    GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);

    bool success = (exitCode == 0);
    LogAction("RepairAddDefenderExclusion", success ? "OK" : "PowerShell failed exit=" + std::to_string(exitCode));

    // Registry'ye state yaz (LauncherEngine.cpp:142-146 pattern)
    // Bu da admin gerek — basarili olduysa zaten PS admin'di, ama yine de elevated process oldu o
    // Bu Launcher non-admin oldugu icin direkt yazamaz, ayri PS gerek
    if (success) {
        std::string regCmd =
            "-NoProfile -Command \"New-Item -Path 'HKLM:\\SOFTWARE\\CodeGuard' -Force | Out-Null; "
            "Set-ItemProperty -Path 'HKLM:\\SOFTWARE\\CodeGuard' -Name 'PATH' -Value '" + gamePath + "' -Type String\"";

        SHELLEXECUTEINFOA seiReg = { 0 };
        seiReg.cbSize = sizeof(seiReg);
        seiReg.fMask  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        seiReg.lpVerb = "runas";
        seiReg.lpFile = "powershell.exe";
        seiReg.lpParameters = regCmd.c_str();
        seiReg.nShow  = SW_HIDE;

        if (ShellExecuteExA(&seiReg) && seiReg.hProcess) {
            WaitForSingleObject(seiReg.hProcess, 10000);
            CloseHandle(seiReg.hProcess);
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

// =====================================================================
// FAZ 5c: DIAGNOSTIC DIALOG (Glasmorphism stil + insan onayli Repair)
// Cyberpunk dersi: kor degistirme YASAK, her aksiyon kullanici onayli
// =====================================================================

// Dialog state — sadece dialog acik iken kullanilir
struct DiagDialogState {
    std::string gamePath;
    std::string serverIP;
    std::vector<CheckResult> results;
};
static DiagDialogState* g_dlgState = nullptr;

// Glasmorphism renkler (koyu cam etkisi)
static const COLORREF DLG_BG          = RGB(20, 14, 11);      // Launcher TR resource $140E0B ile uyumlu
static const COLORREF DLG_TEXT        = RGB(230, 230, 230);   // beyaz ish
static const COLORREF DLG_TEXT_MUTED  = RGB(160, 160, 160);   // status bar gri
static const COLORREF DLG_ACCENT      = RGB(200, 60, 50);     // MK kirmizi (kritik durum)
static const COLORREF DLG_OK          = RGB(80, 200, 100);    // yesil OK
static const COLORREF DLG_WARN        = RGB(255, 180, 60);    // turuncu uyari
static const COLORREF DLG_LIST_BG     = RGB(28, 22, 19);      // listbox arkaplani (bg'den biraz acik)
static const COLORREF DLG_BTN_BG      = RGB(40, 32, 28);      // buton arkaplani

static HBRUSH g_brushDlgBg     = NULL;
static HBRUSH g_brushListBg    = NULL;
static HBRUSH g_brushBtnBg     = NULL;
static HFONT  g_fontSegoeUI    = NULL;
static HFONT  g_fontSegoeBold  = NULL;

// Glasmorphism brush'lari olustur (dialog acilirken)
static void CreateDlgBrushes() {
    if (!g_brushDlgBg)    g_brushDlgBg    = CreateSolidBrush(DLG_BG);
    if (!g_brushListBg)   g_brushListBg   = CreateSolidBrush(DLG_LIST_BG);
    if (!g_brushBtnBg)    g_brushBtnBg    = CreateSolidBrush(DLG_BTN_BG);
    if (!g_fontSegoeUI) {
        g_fontSegoeUI = CreateFontA(
            -13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    }
    if (!g_fontSegoeBold) {
        g_fontSegoeBold = CreateFontA(
            -13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    }
}

static void DestroyDlgBrushes() {
    if (g_brushDlgBg)    { DeleteObject(g_brushDlgBg);    g_brushDlgBg    = NULL; }
    if (g_brushListBg)   { DeleteObject(g_brushListBg);   g_brushListBg   = NULL; }
    if (g_brushBtnBg)    { DeleteObject(g_brushBtnBg);    g_brushBtnBg    = NULL; }
    if (g_fontSegoeUI)   { DeleteObject(g_fontSegoeUI);   g_fontSegoeUI   = NULL; }
    if (g_fontSegoeBold) { DeleteObject(g_fontSegoeBold); g_fontSegoeBold = NULL; }
}

// Status string'i icon prefix ile
static std::string FormatCheckLine(const CheckResult& r) {
    std::string prefix;
    switch (r.status) {
        case CheckStatus::OK:           prefix = "[OK]   "; break;
        case CheckStatus::WARNING:      prefix = "[WARN] "; break;
        case CheckStatus::ERROR_LEVEL:  prefix = "[ERR]  "; break;
        case CheckStatus::DISABLED:     prefix = "[OFF]  "; break;
        default:                        prefix = "[--]   "; break;
    }
    return prefix + r.name + ": " + r.message;
}

// Listbox'a sonuclari yaz
static void FillListBox(HWND hList, const std::vector<CheckResult>& results) {
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    for (const auto& r : results) {
        std::string line = FormatCheckLine(r);
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    }
}

// Status bar guncelle
static void SetDlgStatus(HWND hDlg, const std::string& msg) {
    SetDlgItemTextA(hDlg, IDC_DIAG_STATUS, msg.c_str());
}

// Repair butonlarinin enabled durumunu guncel sonuca gore set et
static void UpdateRepairButtons(HWND hDlg) {
    if (!g_dlgState) return;
    bool needDefender = false, needVCRedist = false, needServerIni = false;
    for (const auto& r : g_dlgState->results) {
        if (r.name.find("Defender") != std::string::npos &&
            r.status != CheckStatus::OK) needDefender = true;
        if (r.name.find("Visual C++") != std::string::npos &&
            r.status != CheckStatus::OK) needVCRedist = true;
        if (r.name.find("Server.ini") != std::string::npos &&
            r.status != CheckStatus::OK) needServerIni = true;
    }
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REPAIR_DEFENDER), needDefender);
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REPAIR_VCREDIST), needVCRedist);
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REPAIR_SERVERINI), needServerIni);
}

// Pump pending Windows messages (UI donmasin)
static void PumpMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Yeniden tara — RunAllChecks cagir, her adim listbox'a "kontrol ediliyor..." gozuksun
// FAZ 5c FIX: buton press feedback + adim adim ilerleme
static void RefreshChecks(HWND hDlg) {
    if (!g_dlgState) return;

    // Tum butonlari disable et (tarama suresince)
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REPAIR_DEFENDER), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REPAIR_VCREDIST), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REPAIR_SERVERINI), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REFRESH), FALSE);

    HWND hList = GetDlgItem(hDlg, IDC_DIAG_LIST);

    // Sahte 5 "kontrol ediliyor..." satiri ile listbox temizle (kullanici GORSUN)
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    std::vector<std::string> labels = {
        "[..]   Windows Defender Exclusion: kontrol ediliyor...",
        "[..]   Dosya Butunlugu: kontrol ediliyor...",
        "[..]   Sunucu Baglantisi: kontrol ediliyor...",
        "[..]   Visual C++ Redistributable: kontrol ediliyor...",
        "[..]   Server.ini: kontrol ediliyor..."
    };

    // Tum CheckResult'larin OWNER-DRAWN icin g_dlgState'e kismi push edelim:
    // Owner-draw yerine basit FillListBox kullaniyoruz (status renkleri olmadan, gri yazi)
    // Geçici: results'i kismi doldur
    g_dlgState->results.clear();
    for (const auto& lbl : labels) {
        CheckResult tmp;
        tmp.name = "...";
        tmp.message = lbl;
        tmp.status = CheckStatus::SKIPPED;
        tmp.action = "";
        tmp.repairAvailable = false;
        g_dlgState->results.push_back(tmp);
        // Listbox'a ekle (FillListBox kullanmadan dogrudan)
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)lbl.c_str());
    }
    SetDlgStatus(hDlg, "Tarama basliyor...");
    PumpMessages();
    InvalidateRect(hDlg, NULL, FALSE);
    UpdateWindow(hDlg);

    // Her check'i sirayla kos, sonucu hemen listbox'a guncel sat olarak yaz
    std::string gp = g_dlgState->gamePath;
    std::string sp = g_dlgState->serverIP;

    std::vector<CheckResult> finalResults;

    // 1. Defender
    SetDlgStatus(hDlg, "1/5  Windows Defender kontrol ediliyor...");
    PumpMessages();
    finalResults.push_back(CheckDefenderExclusion(gp));
    g_dlgState->results[0] = finalResults[0];
    SendMessage(hList, LB_DELETESTRING, 0, 0);
    SendMessageA(hList, LB_INSERTSTRING, 0, (LPARAM)FormatCheckLine(finalResults[0]).c_str());
    PumpMessages();

    // 2. File Integrity
    SetDlgStatus(hDlg, "2/5  Dosya butunlugu kontrol ediliyor...");
    PumpMessages();
    finalResults.push_back(CheckFileIntegrity(gp));
    g_dlgState->results[1] = finalResults[1];
    SendMessage(hList, LB_DELETESTRING, 1, 0);
    SendMessageA(hList, LB_INSERTSTRING, 1, (LPARAM)FormatCheckLine(finalResults[1]).c_str());
    PumpMessages();

    // 3. Connectivity (en yavas — 5sn timeout)
    SetDlgStatus(hDlg, "3/5  Sunucu baglantisi kontrol ediliyor (5sn surebilir)...");
    PumpMessages();
    finalResults.push_back(CheckConnectivity(sp));
    g_dlgState->results[2] = finalResults[2];
    SendMessage(hList, LB_DELETESTRING, 2, 0);
    SendMessageA(hList, LB_INSERTSTRING, 2, (LPARAM)FormatCheckLine(finalResults[2]).c_str());
    PumpMessages();

    // 4. VC++ Redist
    SetDlgStatus(hDlg, "4/5  Visual C++ Redistributable kontrol ediliyor...");
    PumpMessages();
    finalResults.push_back(CheckVCRedist());
    g_dlgState->results[3] = finalResults[3];
    SendMessage(hList, LB_DELETESTRING, 3, 0);
    SendMessageA(hList, LB_INSERTSTRING, 3, (LPARAM)FormatCheckLine(finalResults[3]).c_str());
    PumpMessages();

    // 5. Server.ini
    SetDlgStatus(hDlg, "5/5  Server.ini kontrol ediliyor...");
    PumpMessages();
    finalResults.push_back(CheckServerIni(gp));
    g_dlgState->results[4] = finalResults[4];
    SendMessage(hList, LB_DELETESTRING, 4, 0);
    SendMessageA(hList, LB_INSERTSTRING, 4, (LPARAM)FormatCheckLine(finalResults[4]).c_str());
    PumpMessages();

    // Buton state guncel (Repair sorunlu olanlarda enable)
    g_dlgState->results = finalResults;
    UpdateRepairButtons(hDlg);
    EnableWindow(GetDlgItem(hDlg, IDC_DIAG_REFRESH), TRUE);  // tara butonu hep aktif

    int ok = 0, warn = 0, err = 0;
    for (const auto& r : finalResults) {
        switch (r.status) {
            case CheckStatus::OK: ok++; break;
            case CheckStatus::WARNING: warn++; break;
            case CheckStatus::ERROR_LEVEL: err++; break;
            default: break;
        }
    }
    char status[128];
    snprintf(status, sizeof(status), "Tarama tamam — OK: %d, Uyari: %d, Hata: %d", ok, warn, err);
    SetDlgStatus(hDlg, status);
    LogAction("RefreshChecks", status);
}

// Dialog procedure
static INT_PTR CALLBACK DiagDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // FAZ 5c GLASMORPHISM: brush + font hazirla
            CreateDlgBrushes();

            // Dialog transparancy (WS_EX_LAYERED): hafif opacity (ferah cam etkisi)
            SetLayeredWindowAttributes(hDlg, 0, 240, LWA_ALPHA);

            // Tum child kontrollere Segoe UI font ata
            EnumChildWindows(hDlg, [](HWND hChild, LPARAM /*lp*/) -> BOOL {
                SendMessage(hChild, WM_SETFONT, (WPARAM)g_fontSegoeUI, TRUE);
                return TRUE;
            }, 0);

            // Listbox icin owner-drawn mod (renkli status icin)
            HWND hList = GetDlgItem(hDlg, IDC_DIAG_LIST);
            LONG style = GetWindowLong(hList, GWL_STYLE);
            SetWindowLong(hList, GWL_STYLE, style | LBS_OWNERDRAWFIXED);
            // Listbox font biraz daha buyuk + monospace ikon icin
            SendMessage(hList, LB_SETITEMHEIGHT, 0, MAKELPARAM(22, 0));

            if (g_dlgState) {
                FillListBox(hList, g_dlgState->results);
                UpdateRepairButtons(hDlg);
                SetDlgStatus(hDlg, "Hazir — sorunlu satira tikla, alttaki Onarim butonuna bas.");
            }
            return TRUE;
        }

        // FAZ 5c GLASMORPHISM: dialog ve static text arkaplan rengi
        case WM_CTLCOLORDLG: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, DLG_TEXT);
            SetBkColor(hdc, DLG_BG);
            return (INT_PTR)g_brushDlgBg;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hCtl = (HWND)lParam;
            int ctlId = GetDlgCtrlID(hCtl);

            // Status text muted gri, digerleri beyaz
            if (ctlId == IDC_DIAG_STATUS) {
                SetTextColor(hdc, DLG_TEXT_MUTED);
            } else {
                SetTextColor(hdc, DLG_TEXT);
            }
            SetBkColor(hdc, DLG_BG);
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)g_brushDlgBg;
        }
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, DLG_TEXT);
            SetBkColor(hdc, DLG_LIST_BG);
            return (INT_PTR)g_brushListBg;
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, DLG_TEXT);
            SetBkColor(hdc, DLG_BTN_BG);
            return (INT_PTR)g_brushBtnBg;
        }

        // FAZ 5c GLASMORPHISM: listbox satirlarini ve butonlari custom ciz
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;

            // BUTON custom paint (BS_OWNERDRAW)
            if (dis->CtlType == ODT_BUTTON) {
                bool selected = (dis->itemState & ODS_SELECTED) != 0;   // tikla anında
                bool focused  = (dis->itemState & ODS_FOCUS) != 0;
                bool disabled = (dis->itemState & ODS_DISABLED) != 0;
                bool hot      = (dis->itemState & ODS_HOTLIGHT) != 0;

                // Arkaplan: normal=koyu, basili=accent kirmizi, hover=biraz acik, disabled=cok soluk
                COLORREF bgColor;
                COLORREF txtColor = DLG_TEXT;
                if (disabled) {
                    bgColor = RGB(25, 20, 18);
                    txtColor = RGB(90, 90, 90);
                } else if (selected) {
                    bgColor = DLG_ACCENT;       // BASILI: kirmizi MK rengi (NET FEEDBACK)
                    txtColor = RGB(255, 255, 255);
                } else if (hot) {
                    bgColor = RGB(60, 48, 42);  // HOVER: bg'den acik
                } else {
                    bgColor = DLG_BTN_BG;       // NORMAL
                }

                // Dikdortgen ciz (1px border)
                HBRUSH bg = CreateSolidBrush(bgColor);
                FillRect(dis->hDC, &dis->rcItem, bg);
                DeleteObject(bg);

                // Border (focused ise accent kirmizi, normal ise gri)
                HPEN borderPen = CreatePen(PS_SOLID, 1, focused ? DLG_ACCENT : RGB(70, 56, 50));
                HGDIOBJ oldPen = SelectObject(dis->hDC, borderPen);
                HGDIOBJ oldBrush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
                Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top,
                          dis->rcItem.right, dis->rcItem.bottom);
                SelectObject(dis->hDC, oldPen);
                SelectObject(dis->hDC, oldBrush);
                DeleteObject(borderPen);

                // Buton yazisi
                char btnText[64] = { 0 };
                GetWindowTextA(dis->hwndItem, btnText, sizeof(btnText));
                SetTextColor(dis->hDC, txtColor);
                SetBkMode(dis->hDC, TRANSPARENT);
                SelectObject(dis->hDC, g_fontSegoeUI);

                // Basili durumda text 1px asagi-saga (3D feedback)
                RECT rcText = dis->rcItem;
                if (selected) { rcText.left += 1; rcText.top += 1; }
                DrawTextA(dis->hDC, btnText, -1, &rcText,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                return TRUE;
            }

            // LISTBOX satirlari (mevcut kod)
            if (dis->CtlID == IDC_DIAG_LIST && dis->itemID != (UINT)-1) {
                // Hangi satir (index)
                int idx = (int)dis->itemID;

                // Arkaplan (selected/normal)
                bool selected = (dis->itemState & ODS_SELECTED) != 0;
                HBRUSH bg = CreateSolidBrush(selected ? RGB(50, 38, 32) : DLG_LIST_BG);
                FillRect(dis->hDC, &dis->rcItem, bg);
                DeleteObject(bg);

                // Satir text + status rengi
                if (g_dlgState && idx >= 0 && idx < (int)g_dlgState->results.size()) {
                    const CheckResult& r = g_dlgState->results[idx];
                    COLORREF txtColor = DLG_TEXT;
                    switch (r.status) {
                        case CheckStatus::OK:           txtColor = DLG_OK; break;
                        case CheckStatus::WARNING:      txtColor = DLG_WARN; break;
                        case CheckStatus::ERROR_LEVEL:  txtColor = DLG_ACCENT; break;
                        default: break;
                    }
                    SetTextColor(dis->hDC, txtColor);
                    SetBkMode(dis->hDC, TRANSPARENT);
                    SelectObject(dis->hDC, g_fontSegoeUI);

                    std::string line = FormatCheckLine(r);
                    RECT rcText = dis->rcItem;
                    rcText.left += 8;  // padding
                    DrawTextA(dis->hDC, line.c_str(), -1, &rcText,
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }

                // Focus rect (selected ise)
                if (dis->itemState & ODS_FOCUS) {
                    DrawFocusRect(dis->hDC, &dis->rcItem);
                }
                return TRUE;
            }
            return FALSE;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            switch (id) {
                case IDC_DIAG_REPAIR_DEFENDER: {
                    if (MessageBoxA(hDlg,
                        "Windows Defender'a MalaysiaKO klasoru ve 3 process icin istisna eklenecek.\n"
                        "Sistem 'Yonetici izni' isteyebilir. Devam edilsin mi?",
                        "Defender Onayla", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        SetDlgStatus(hDlg, "Defender exclusion ekleniyor (UAC bekliyor)...");
                        bool ok = RepairAddDefenderExclusion(g_dlgState->gamePath);
                        SetDlgStatus(hDlg, ok ?
                            "Defender exclusion eklendi. Yeniden tarayin." :
                            "Defender exclusion BASARISIZ (kullanici reddi veya hata).");
                    }
                    return TRUE;
                }
                case IDC_DIAG_REPAIR_VCREDIST: {
                    if (MessageBoxA(hDlg,
                        "Microsoft VC++ Redistributable indirme sayfasi tarayicida acilacak.\n"
                        "Inen dosyayi calistirin (Run as Admin), sonra yeniden tarayin.",
                        "VC++ Indir", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK) {
                        RepairInstallVCRedist();
                        SetDlgStatus(hDlg, "Tarayicida MS resmi VC++ Redist sayfasi acildi.");
                    }
                    return TRUE;
                }
                case IDC_DIAG_REPAIR_SERVERINI: {
                    if (MessageBoxA(hDlg,
                        "Server.ini varsayilan degerlere SIFIRLANACAK.\n"
                        "Mevcut Server.ini uzerine yazilacak. Devam edilsin mi?",
                        "Server.ini Sifirla", MB_YESNO | MB_ICONWARNING) == IDYES) {
                        bool ok = RepairServerIni(g_dlgState->gamePath);
                        SetDlgStatus(hDlg, ok ?
                            "Server.ini sifirlandi. Yeniden tarayin." :
                            "Server.ini sifirlama BASARISIZ.");
                    }
                    return TRUE;
                }
                case IDC_DIAG_REFRESH: {
                    RefreshChecks(hDlg);
                    return TRUE;
                }
                case IDC_DIAG_CLOSE:
                case IDCANCEL: {
                    EndDialog(hDlg, 0);
                    return TRUE;
                }
            }
            return FALSE;
        }

        case WM_CLOSE: {
            EndDialog(hDlg, 0);
            return TRUE;
        }

        case WM_DESTROY: {
            // FAZ 5c: brush/font temizle
            DestroyDlgBrushes();
            return FALSE;
        }
    }
    return FALSE;
}

// Public: Dialog'u modal goster
void ShowDiagnosticDialog(HWND hwndParent, const std::string& gamePath, const std::string& serverIP) {
    LogAction("ShowDiagnosticDialog", "opening");

    DiagDialogState state;
    state.gamePath = gamePath;
    state.serverIP = serverIP;
    state.results  = RunAllChecks(gamePath, serverIP);
    g_dlgState = &state;

    DialogBoxParamA(
        GetModuleHandle(NULL),
        MAKEINTRESOURCEA(IDD_DIAGNOSTIC),
        hwndParent,
        DiagDialogProc,
        0
    );

    g_dlgState = nullptr;
    LogAction("ShowDiagnosticDialog", "closed");
}

// =====================================================================
// S115 v2.7+ C plani: BASIT Self-heal Dialog (1 buton ONAR)
// =====================================================================

struct SimpleDlgState {
    std::string gamePath;
    std::vector<CheckResult> results;
    bool needDefender   = false;
    bool needVCRedist   = false;
    bool needServerIni  = false;
};

static SimpleDlgState* g_simpleState = nullptr;

// Sorun listesini insan-dostu metne cevir (bullet list)
static std::string BuildProblemsText(const std::vector<CheckResult>& results,
                                     bool& needDefender,
                                     bool& needVCRedist,
                                     bool& needServerIni) {
    needDefender = needVCRedist = needServerIni = false;
    std::string out;
    for (const auto& r : results) {
        if (r.status != CheckStatus::ERROR_LEVEL && r.status != CheckStatus::WARNING) continue;

        // Hangi repair gerek?
        std::string name_lower = r.name;
        for (auto& c : name_lower) c = (char)tolower((unsigned char)c);

        if (name_lower.find("defender") != std::string::npos) {
            out += "* Antivirus oyunu engelliyor olabilir\r\n";
            needDefender = true;
        } else if (name_lower.find("vcredist") != std::string::npos || name_lower.find("vc++") != std::string::npos) {
            out += "* VC++ Redistributable eksik\r\n";
            needVCRedist = true;
        } else if (name_lower.find("server.ini") != std::string::npos || name_lower.find("serverini") != std::string::npos) {
            out += "* Server.ini ayar dosyasi bozuk\r\n";
            needServerIni = true;
        } else if (name_lower.find("connect") != std::string::npos) {
            out += "* Sunucuya baglanti kurulamadi\r\n";
        } else if (name_lower.find("file") != std::string::npos || name_lower.find("integrity") != std::string::npos) {
            out += "* Oyun dosyalari eksik veya bozuk\r\n";
        } else {
            out += "* " + r.name + ": " + r.message + "\r\n";
        }
    }
    if (out.empty()) out = "Sorun bulundu ama detay belirsiz.\r\n";
    return out;
}

static INT_PTR CALLBACK SimpleDiagProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            if (!g_simpleState) return TRUE;

            std::string text = BuildProblemsText(
                g_simpleState->results,
                g_simpleState->needDefender,
                g_simpleState->needVCRedist,
                g_simpleState->needServerIni
            );
            SetDlgItemTextA(hDlg, IDC_DIAG_S_PROBLEMS, text.c_str());

            // Eger hicbir repair-able sorun yoksa ONAR'i devre disi birak
            bool canRepair = g_simpleState->needDefender ||
                             g_simpleState->needVCRedist ||
                             g_simpleState->needServerIni;
            EnableWindow(GetDlgItem(hDlg, IDC_DIAG_S_ONAR), canRepair);
            return TRUE;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDC_DIAG_S_ONAR) {
                if (!g_simpleState) return TRUE;

                // ONAR butonu basildi -> hangi sorun varsa o repair'lar sirayla
                EnableWindow(GetDlgItem(hDlg, IDC_DIAG_S_ONAR), FALSE);
                SetDlgItemTextA(hDlg, IDC_DIAG_S_PROBLEMS, "Onariliyor, lutfen bekleyin...");

                // Async thread'de kos — UI donmasin
                HWND hwndDlg = hDlg;
                SimpleDlgState* st = g_simpleState;
                std::thread([hwndDlg, st]() {
                    if (st->needDefender)  RepairAddDefenderExclusion(st->gamePath);
                    if (st->needVCRedist)  RepairInstallVCRedist();
                    if (st->needServerIni) RepairServerIni(st->gamePath);

                    // Bitti
                    if (IsWindow(hwndDlg)) {
                        SetDlgItemTextA(hwndDlg, IDC_DIAG_S_PROBLEMS,
                            "Onarim tamamlandi.\r\n\r\nLauncher'i kapatip tekrar acin.");
                    }
                }).detach();

                return TRUE;
            }
            if (id == IDC_DIAG_S_CLOSE || id == IDCANCEL) {
                EndDialog(hDlg, 0);
                return TRUE;
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;
    }
    return FALSE;
}

void ShowSimpleDiagnosticDialog(HWND hwndParent,
                                const std::string& gamePath,
                                const std::vector<CheckResult>& results) {
    LogAction("ShowSimpleDiagnosticDialog", "opening");

    SimpleDlgState state;
    state.gamePath = gamePath;
    state.results  = results;
    g_simpleState  = &state;

    DialogBoxParamA(
        GetModuleHandle(NULL),
        MAKEINTRESOURCEA(IDD_DIAGNOSTIC_SIMPLE),
        hwndParent,
        SimpleDiagProc,
        0
    );

    g_simpleState = nullptr;
    LogAction("ShowSimpleDiagnosticDialog", "closed");
}

} // namespace LauncherDiagnostic
