#include "LauncherEngine.h"
#include "MD5.h"
#include "resource.h"
#include <regex>
#include <TlHelp32.h>
#include <sstream>
#include <Psapi.h>
#include <Shlwapi.h>
#include <iphlpapi.h>  // S114 K3: GetAdaptersInfo (HWID MAC)
#include <thread>      // S114 K3 FIX: async HWID
#include <winhttp.h>   // S115 AUTO-UPDATE: HTTP download
#include <fstream>     // S115 AUTO-UPDATE: dosya yazma
#include <dbghelp.h>   // S115 CRASH REPORTER: MiniDumpWriteDump
#include "CrashFingerprint.h"  // S115 v2.5 FAZ 4: client filtre
#include "LauncherDiagnostic.h" // S115 v2.5 FAZ 5: self-heal diagnostik
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "winhttp.lib")  // S115 AUTO-UPDATE
#pragma comment(lib, "dbghelp.lib")  // S115 CRASH REPORTER
#define CURL_ICONV_CODESET_FOR_UTF8 "UTF-8"
#define PRINT_LOG [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl;  }

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos - prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

int win_system(const char* command)
{
    char* tmp_command, * cmd_exe_path;
    DWORD ret_val = 0;
    size_t len = strlen(command);
    PROCESS_INFORMATION process_info = { 0 };
    STARTUPINFOA        startup_info = { 0 };
    tmp_command = (char*)malloc(len + 4);
    if (tmp_command) {
        tmp_command[0] = 0x2F;
        tmp_command[1] = 0x63;
        tmp_command[2] = 0x20;
        memcpy(tmp_command + 3, command, len + 1);

        startup_info.cb = sizeof(STARTUPINFOA);
        cmd_exe_path = getenv("COMSPEC");
        _flushall();
        if (CreateProcessA(cmd_exe_path, tmp_command, NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_info)) {
            WaitForSingleObject(process_info.hProcess, 5000);
            GetExitCodeProcess(process_info.hProcess, &ret_val);
            CloseHandle(process_info.hProcess);
            CloseHandle(process_info.hThread);
        }
        free((void*)tmp_command);
    }
    return(ret_val);
}

bool KeyExists(HKEY hRootKey, LPCSTR strKey)
{
    HKEY hKey;
    LONG nError = RegOpenKeyEx(hRootKey, strKey, NULL, KEY_ALL_ACCESS, &hKey);
    return nError != ERROR_FILE_NOT_FOUND;
}

std::string GetVal(HKEY hKey, LPCTSTR strKey)
{
    char str[255]{ 0 };
    DWORD size = 255;
    DWORD type = REG_SZ;

    RegQueryValueExA(hKey, strKey, NULL, &type, (LPBYTE)str, &size);

    return std::string(str);
}

HKEY CreateKey(HKEY hRootKey, LPCSTR strKey)
{
    HKEY hKey;
    LONG nError = RegCreateKeyExA(hRootKey, strKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    return hKey;
}

void SetVal(HKEY hKey, LPCTSTR lpValue, std::string data)
{
    LONG nError = RegSetValueEx(hKey, lpValue, NULL, REG_SZ, (LPBYTE)data.data(), (data.size() + 1) * sizeof(wchar_t));
}

Launcher::Launcher()
{
    mSocket = NULL;
    ready = 0;
    m_dPercent = 0;
    m_bVersionGot = false;
    m_bPatchesGot = false;
    m_iVersion = 0;
    // S115: Launcher kendi build versiyonu state'e eklendi (gozle gorulebilir surum farki icin)
    char verBuf[64];
    snprintf(verBuf, sizeof(verBuf), "[Launcher v%d.%d] Checking version...",
             LAUNCHER_BUILD_VERSION_MAJOR, LAUNCHER_BUILD_VERSION_MINOR);
    m_stateString = verBuf;

    // S114 K3 FIX: HWID hesabi ASYNC thread'de (GetAdaptersInfo yavas — Launcher UI bloklamasin)
    std::thread([this]() { this->ComputeHwidA(); }).detach();

    // S115 v2.7 ACIL FIX: Self-heal DEVRE DISI (Launcher.ini [SelfHeal] AutoRun=1 ile acilabilir)
    // SEBEP: Constructor'da 3 async HTTP/WinSock thread (HwidA + CheckForUpdate + Self-heal)
    //        WinSock/WinHTTP race condition yaratiyor, baglanti kuruluyor olmasi gerekirken
    //        bazi PC'lerde 'Connection failed' veriyor (patron PC ornek).
    // COZUM: Self-heal AUTO-RUN kapali (Diagnostic Dialog hala manuel acilabilir).
    //        Patron isterse Launcher.ini [SelfHeal] AutoRun=1 ile acabilir.
    char workDirInit[MAX_PATH] = { 0 };
    GetCurrentDirectoryA(MAX_PATH, workDirInit);
    std::string iniPath = std::string(workDirInit) + "\\Launcher.ini";
    UINT autoRun = GetPrivateProfileIntA("SelfHeal", "AutoRun", 0, iniPath.c_str());

    if (autoRun == 1) {
        // Sadece kullanici acikca aktiv ederse arka planda calis
        std::thread([this]() {
            Sleep(15000);  // 15sn cok gec — Auto-update + Start() bitmis olur

            char workDir[MAX_PATH] = { 0 };
            GetCurrentDirectoryA(MAX_PATH, workDir);
            std::string gamePath(workDir);

            char ipBuf[128] = { 0 };
            GetPrivateProfileStringA("Server", "IP0", "", ipBuf, sizeof(ipBuf),
                                     (gamePath + "\\Server.ini").c_str());

            auto results = LauncherDiagnostic::RunAllChecks(gamePath, ipBuf);
            int errors = 0;
            for (const auto& r : results) {
                if (r.status == LauncherDiagnostic::CheckStatus::ERROR_LEVEL) errors++;
                std::string statusStr =
                    (r.status == LauncherDiagnostic::CheckStatus::OK)          ? "OK" :
                    (r.status == LauncherDiagnostic::CheckStatus::WARNING)     ? "WARN" :
                    (r.status == LauncherDiagnostic::CheckStatus::ERROR_LEVEL) ? "ERR" :
                    (r.status == LauncherDiagnostic::CheckStatus::DISABLED)    ? "DISABLED" : "SKIP";
                LauncherDiagnostic::LogAction(r.name, statusStr + " | " + r.message);
            }
            // Sadece ERROR varsa kullaniciya UI goster
            if (errors > 0) {
                extern HWND mainWindow;
                if (mainWindow && IsWindow(mainWindow)) {
                    PostMessageA(mainWindow, WM_USER + 100, 0, 0);
                }
            }
        }).detach();
    }

    const size_t IPSize = 256;
    char* sIP = new char[IPSize];
    GetCurrentDirectoryA(MAX_PATH, WorkingPath);
    GetPrivateProfileStringA(xorstr("Server"), xorstr("IP0"), xorstr("104.238.23.99"), sIP, IPSize, (std::string(WorkingPath) + xorstr("\\Server.ini")).c_str());
    m_settingsVersion = GetPrivateProfileIntA(xorstr("Version"), xorstr("Files"), 1, (std::string(WorkingPath) + xorstr("\\Server.ini")).c_str());
    m_settingsIP = sIP;

    GetCurrentDirectoryA(MAX_PATH, m_strBasePath);
    // NOT (FIX-E): lokal 'm_base' degiskeni kaldirildi — Defender exclusion blogu silinince
    // kullanansiz kaldi (m_strBasePath member set edilmeye devam ediyor, baska yerde kullanilabilir).

    // TODO#241 FIX-E (S127 v3.4): DEFENDER EXCLUSION BLOGU KALDIRILDI (launcher'dan).
    // SEBEP: (1) setup.exe kurulumda zaten exclusion ekliyor -> launcher'da tekrar GEREKSIZ.
    //   (2) Launcher'daki exclusion ISE YARAMIYORDU: oyuncu vakasi (S127) — AV acikti, exe SILINDI.
    //       Defender exclusion 3.parti AV'da (Avast/Kaspersky) etkisiz + UAC reddinde sessiz fail.
    //   (3) win_system 'powershell -Verb RunAs' -> UAC penceresi + 5sn blocking (async olsa da
    //       UAC prompt oyuncuyu sasirtir, bazen reddedilir).
    // YAN ETKI YOK: HKLM\SOFTWARE\CodeGuard\PATH registry'sine baska kritik bagimlilik yok —
    //   LauncherDiagnostic.cpp:73 zaten 'bu registry ALDATICI, eski launcher yazmis olabilir'
    //   deyip ona guvenmiyor, gercek Defender'dan PowerShell ile soruyor. Registry yazimi kaldirildi.
    // GUVENLIK KONTROLLERI KALDI: (b) TBL hash + (c) cheat scan ASYNC calismaya devam.

    // TODO#241 F3 (S127): ACILIS HIZI — CheckTBLHashes + ScanCheatTools ASYNC thread'de.
    // Pencere ANINDA acilir, bu isler arka planda kosar. Sonuclar START click GIF fazinda
    // TAZE yeniden taraniyor (Launcher.cpp 'Faz 0: Taze scan') -> ctor taramasi cache on-isi.
    std::thread([this]() {
        // (b) TBL hash check (Data\*.tbl senkron MD5) — arka plan, sonuc cache'e yazilir
        this->CheckTBLHashes();

        // (c) KOXP/cheat tool scan — arka plan, sonuc m_scanThreatDetected/Name cache
        {
            std::string detected;
            this->m_scanThreatDetected = this->ScanCheatTools(detected);
            this->m_scanThreatName = detected;
        }
    }).detach();

    // S113: L1 SELF-HEAL — KRITIK kontrol: sadece UI/ui.src boyut
    // Bos veya cok kucuk (<1 MB) ise Server.ini'yi geriye sar (Launcher yeniden indirir)
    // NOT: Sadece UI bakiliyor, baska klasor (Data/fx vs) bazi KO versiyonlarinda yok
    {
        std::string p = std::string(WorkingPath) + "\\UI\\ui.src";
        HANDLE h = CreateFileA(p.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        bool needsHeal = false;
        if (h == INVALID_HANDLE_VALUE) {
            needsHeal = true;
        } else {
            DWORD szHi = 0;
            DWORD szLo = GetFileSize(h, &szHi);
            CloseHandle(h);
            int64_t size = ((int64_t)szHi << 32) | szLo;
            if (size < 1024 * 1024) needsHeal = true;  // <1 MB = bozuk
        }
        if (needsHeal) {
            WritePrivateProfileStringA(xorstr("Version"), xorstr("Files"), "2369", (std::string(WorkingPath) + xorstr("\\Server.ini")).c_str());
            m_settingsVersion = 2369;
        }
    }

    // TODO#241 F3 (S127): CheckTBLHashes() + ScanCheatTools() YUKARIDAKI async thread'e
    // tasindi (Defender ile birlikte). Burada SENKRON cagri YOK — ctor bloklamiyor.

    // S115 v2.7+ FIX: AUTO-UPDATE thread BURADAN KALDIRILDI.
    // Sebep: WinHTTP + WinSock race ('Connection failed' patron PC bug, S115).
    // Yeni yer: Launcher::Start() — connect() basarili olduktan SONRA fork ediliyor.
}

// S114: KOXP/cheat tool tarayicisi — 4 katman (process + window + DLL disk + driver service)
// True dondururse 'detected' degiskeninde tespit edilen sey adi vardir
bool Launcher::ScanCheatTools(std::string& detected)
{
    detected = "";

    // 1) PROCESS scan (case-insensitive isim)
    const char* processes[] = {
        // MRX
        "MRX.exe", "MRXMAKRO.exe",
        // Private/Hidden bots
        "PrivateKoxp.exe", "PrivateBot.exe", "HiddenCore.exe", "HiddenCoreKoxp.exe",
        // Yaygin makrolar
        "CL4X3S.exe", "Cl4x3sMacro.exe",
        "ZenAutobot.exe", "ZenMacro.exe",
        "AnnihilatorPedal.exe", "Annihilator.exe",
        "KozyMacro.exe", "Kozy.exe",
        "FreevarBot.exe", "Freevar.exe",
        "HeavenFire.exe", "HeavenFireKoxp.exe",
        "EXEBOT.exe", "ExeBotKoxp.exe",
        "Asiturk.exe", "AsiturkKoxp.exe",
        "PandoraBot.exe", "AgarthaBot.exe", "MadenBot.exe",
        "PkBot.exe", "SeriMinor.exe", "KoHack.exe", "DryardsBot.exe",
        "CesarBot.exe", "CheapBot.exe", "MotaBot.exe",
        "Koreabot.exe", "KOREABOT.exe",
        // NOT: AutoIt3/AutoHotkey listesinden cikarildi — PvP'de makro caiz (text expansion, mouse pedal vb. cift kullanim)
        // Gercek KOXP'lar zaten kendi exe icine derliyor (MRX gibi)
        // KOXP altyapi
        "cheatengine-x86_64.exe", "cheatengine-i386.exe", "CheatEngine.exe",
        "interception.exe",
        NULL
    };

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe{}; pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe)) {
            do {
                for (int i = 0; processes[i] != NULL; i++) {
                    if (_stricmp(pe.szExeFile, processes[i]) == 0) {
                        detected = std::string("Process: ") + pe.szExeFile;
                        CloseHandle(snap);
                        return true;
                    }
                }
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
    }

    // 2) WINDOW TITLE scan
    struct WindowScanData {
        const char** titles;
        std::string* detected;
    };
    const char* windowTitles[] = {
        "MRX MAKRO", "MRX MAKRO ",
        "Private Koxp", "PrivateBot",
        "HiddenCore",
        "CL4X3S",
        "Zen Autobot",
        "AnnihilatorPedal",
        "Kozy Macro",
        "FreevarBot",
        "Heaven Fire",
        "EXEBOT",
        "Asiturk",
        "Cheat Engine",
        NULL
    };
    WindowScanData wsd { windowTitles, &detected };

    auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        WindowScanData* d = (WindowScanData*)lParam;
        char title[256] = {0};
        if (GetWindowTextA(hwnd, title, sizeof(title)) > 0) {
            for (int i = 0; d->titles[i] != NULL; i++) {
                // Strstr ile partial match — KOXP'lar pencere title'ina ek metin ekleyebilir
                if (StrStrIA(title, d->titles[i]) != NULL) {
                    *(d->detected) = std::string("Window: ") + title;
                    return FALSE;  // Bul, dur
                }
            }
        }
        return TRUE;  // Devam
    };
    EnumWindows(enumProc, (LPARAM)&wsd);
    if (!detected.empty()) return true;

    // 3) interception.dll DISK SCAN — Windows klasoru DISINDA (gercek interception kullanan az)
    // Standart konum kontrol: temp, downloads, desktop, common
    const char* dllSearchDirs[] = {
        "C:\\Windows\\interception.dll",  // Bazi makrolar Windows koklerine atar
        NULL
    };
    for (int i = 0; dllSearchDirs[i] != NULL; i++) {
        HANDLE h = CreateFileA(dllSearchDirs[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            detected = std::string("DLL: ") + dllSearchDirs[i];
            return true;
        }
    }

    // 4) DRIVER SERVICE — "interception" kernel driver kurulu mu?
    SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCM != NULL) {
        SC_HANDLE hSvc = OpenServiceA(hSCM, "interception", SERVICE_QUERY_STATUS);
        if (hSvc != NULL) {
            // Servis mevcut — KOXP imzasi (gercek oyuncu interception driver kullanmaz)
            detected = "Driver: interception (kernel mouse/keyboard hook)";
            CloseServiceHandle(hSvc);
            CloseServiceHandle(hSCM);
            return true;
        }
        CloseServiceHandle(hSCM);
    }

    return false;
}

// =============================================================
// S114 K3: HWID_A hesap (HDD volume serial + MAC adresi -> MD5)
// 1 kez hesaplanir (m_hwidA cache), login paketinde server'a yollanir.
// Server TB_HWID_BANS_A tablosunda check eder, banli ise login reddedilir.
// =============================================================
// S114 K3 FAZ 2: HWID_A hesap (volume + MAC + salt -> MD5)
std::string Launcher::ComputeHwidA()
{
    if (!m_hwidA.empty()) return m_hwidA;
    char volBuf[16] = {0};
    DWORD volSerial = 0;
    if (GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0)) {
        sprintf_s(volBuf, "%08X", volSerial);
    }
    char macBuf[32] = {0};
    IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ULONG bufLen = sizeof(IP_ADAPTER_INFO);
    if (GetAdaptersInfo(pAdapterInfo, &bufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
    }
    if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &bufLen) == NO_ERROR) {
        IP_ADAPTER_INFO* p = pAdapterInfo;
        while (p) {
            if (p->Type != MIB_IF_TYPE_LOOPBACK && p->AddressLength == 6) {
                sprintf_s(macBuf, "%02X%02X%02X%02X%02X%02X",
                    p->Address[0], p->Address[1], p->Address[2],
                    p->Address[3], p->Address[4], p->Address[5]);
                break;
            }
            p = p->Next;
        }
    }
    if (pAdapterInfo) free(pAdapterInfo);
    std::string combined = std::string(volBuf) + "|" + macBuf + "|MYKO_K3_SALT";
    MD5 md5;
    m_hwidA = std::string(md5.digestMemory((BYTE*)combined.c_str(), (int)combined.size()));
    return m_hwidA;
}

// S114: TBL hash kontrolu — gomulu hash listesi vs disk MD5
void Launcher::CheckTBLHashes()
{
    // 1. Resource'tan gomulu hash listesi (text formati: filename=md5\n)
    HRSRC hRes = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TBL_HASHES), RT_RCDATA);
    if (!hRes) return;
    HGLOBAL hMem = LoadResource(GetModuleHandle(NULL), hRes);
    if (!hMem) return;
    DWORD resSize = SizeofResource(GetModuleHandle(NULL), hRes);
    if (resSize == 0) return;
    const char* resData = (const char*)LockResource(hMem);
    if (!resData) return;

    std::string hashList(resData, resSize);

    // 2. Her satir: filename=md5
    std::stringstream ss(hashList);
    std::string line;
    int mismatchCount = 0;
    int missingCount = 0;
    std::string mismatchList;

    while (std::getline(ss, line)) {
        // Boş satir veya satir sonu temizle
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // filename=md5 ayir
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string fname = line.substr(0, eq);
        std::string expectedHash = line.substr(eq + 1);
        if (expectedHash.length() != 32) continue;

        // Disk path
        std::string diskPath = std::string(WorkingPath) + "\\Data\\" + fname;

        // Dosya yoksa missing
        HANDLE h = CreateFileA(diskPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            missingCount++;
            continue;
        }
        CloseHandle(h);

        // MD5 hesapla
        MD5 md5check;
        std::string actualHash = md5check.FileMD5Check(diskPath.c_str());
        // Lowercase karsilastir
        for (auto& c : actualHash) c = tolower(c);
        for (auto& c : expectedHash) c = tolower(c);

        if (actualHash != expectedHash) {
            mismatchCount++;
            if (mismatchList.length() < 300) {
                mismatchList += fname + "\n";
            }
        }
    }

    // S114: Sonuc cache (MessageBox YOK — START sonrasi GIF + UI ile gosterilir)
    m_tblMismatchCount = mismatchCount;
    m_tblMissingCount  = missingCount;
    m_tblMismatchList  = mismatchList;
}

static size_t my_write(void* buffer, size_t size, size_t nmemb, void* param)
{
    std::string& text = *static_cast<std::string*>(param);
    size_t totalsize = size * nmemb;
    text.append(static_cast<char*>(buffer), totalsize);
    return totalsize;
}

void Launcher::RequestVersion()
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return;

    int iOffset = 0;
    uint8_t byBuffs[1];
    CAPISocket::MP_AddByte(byBuffs, iOffset, 0x1);
    mSocket->Send(byBuffs, iOffset);
}

void Launcher::RequestPatch()
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return;

    int iOffset = 0;
    uint8_t byBuffs[3];
    CAPISocket::MP_AddByte(byBuffs, iOffset, 0x2);
    CAPISocket::MP_AddShort(byBuffs, iOffset, m_settingsVersion);
    mSocket->Send(byBuffs, iOffset);
}

// S114 K3 FAZ 4: HWID_REPORT paketi (opcode 0x4)
// Format: [0x4][hwid_string]
// Server: TB_HWID_BANS_A check -> 0x84 cevap (0=OK, 1=BANNED)
void Launcher::ReportHwid()
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return;

    std::string hwid = ComputeHwidA();
    if (hwid.empty()) return;

    int iOffset = 0;
    uint8_t byBuffs[64];  // 1 opcode + 2 len + 32 hwid + buffer
    CAPISocket::MP_AddByte(byBuffs, iOffset, 0x4);
    CAPISocket::MP_AddString(byBuffs, iOffset, hwid);
    mSocket->Send(byBuffs, iOffset);
}

void Launcher::RequestNotices()
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return;

    int iOffset = 0;
    uint8_t byBuffs[1];
    CAPISocket::MP_AddByte(byBuffs, iOffset, 0x3);
    mSocket->Send(byBuffs, iOffset);
}

bool Launcher::Start()
{
    // TODO#241 FIX-B (S127 v3.2): connect RETRY + 'onar' dialogu sustur.
    // ESKI: connect tek deneme; fail -> 'Connection failed' + self-heal RunAllChecks thread
    //       -> ERROR/WARN varsa 'ONAR' dialogu (WM_USER+101). connect-fail self-heal DEFAULT
    //       ACIK (Launcher.ini PROD'da yok -> [SelfHeal] Enabled=1). Ag anlik koptu mu herkeste
    //       'onar' cikip kafa karistiriyor + RunAllChecks ~10sn takiliyor, AMA sorun ag -> cozmuyor.
    // YENI: (1) connect 2-3 retry (F2'nin 5sn timeout'u ile, kisa backoff). Ag anlik toparlarsa
    //       launcher KENDI baglanir (kapat-ac gerekmez). (2) hala fail -> SADE mesaj, 'onar'
    //       dialogu/RunAllChecks YOK (ag sorununu onar cozmuyordu, sadece kasiyordu).
    mSocket = new CAPISocket();
    int iErr = mSocket->Connect(window, m_settingsIP.c_str(), 15100);
    if (iErr)
    {
        const int CONN_RETRY_MAX = 3;
        const DWORD CONN_BACKOFF[] = { 2000, 3000, 4000 }; // artan bekleme
        // NOT (faz-sonu H1): Connect() fail edince APISocket m_hSocket=INVALID_SOCKET set ediyor
        // (APISocket.cpp:149-150), ayrica Connect basinda eski socket'i Disconnect ediyor (94-95)
        // -> retry'da socket leak YOK. Onceki 'GetSocket()!=INVALID break' olu koddu (fail sonrasi
        // socket hep INVALID, tetiklenmezdi) -> kaldirildi. Dogrudan Connect retry yeterli.
        for (int r = 0; r < CONN_RETRY_MAX && iErr; r++)
        {
            SetState(std::format("Baglaniliyor... (deneme {}/{})", r + 1, CONN_RETRY_MAX));
            Sleep(CONN_BACKOFF[r]);
            iErr = mSocket->Connect(window, m_settingsIP.c_str(), 15100);
        }

        if (iErr)
        {
            // Tum denemeler basarisiz -> SADE mesaj (onar dialogu / RunAllChecks YOK)
            SetState(xorstr("Sunucuya baglanilamadi. Lutfen tekrar deneyin."));
            return false;
        }
    }

    RequestVersion();
    // S114 K3 FIX: ReportHwid ASYNC (Launcher acilis bloklamasin)
    std::thread([this]() {
        Sleep(200);  // Rate limiter (100ms/10 paket)
        this->ReportHwid();
    }).detach();

    // S115 v2.7+ FIX: AUTO-UPDATE thread Constructor'dan buraya tasindi.
    // SEBEP: Constructor'da WinHTTP thread + WinSock connect ayni anda baslayinca
    //        race yaratip 'Connection failed' veriyordu (patron PC test S115).
    // COZUM: connect() basarili olduktan SONRA 5sn bekle + WinHTTP thread fork.
    //        Bu noktada WinSock tam settle, WinHTTP race etmez.
    std::thread([this]() {
        Sleep(5000);  // WinSock settle + ilk paket akisi tamamlansin
        try {
            this->CheckForUpdate();
        } catch (...) {
            // Auto-update fail oldu — sessizce gec, normal Launcher akisi devam
        }
        // TODO#241 FIX-H (S127 v3.5): update KONTROLU bitti (sonuc ne olursa olsun: update yok /
        // fail / indirme baslayacak). m_bUpdateChecked=true -> START artik serbest (update varsa
        // m_bUpdating zaten true kalir, D1 gate kapatir). TEK NOKTA: CheckForUpdate'in tum
        // return dallarini tek tek isaretlemeye gerek yok, caller'da garanti edilir (kacak yok).
        this->m_bUpdateChecked = true;
    }).detach();
    // FIX-H: T=1-5sn cakisma penceresi KAPANDI. ready=true olsa bile update-check (m_bUpdateChecked)
    // bitmeden START tetiklenmez (StartClick/HandleMouse gate). Oyuncu o sirada 'Guncelleme kontrol
    // ediliyor' gorur (~5sn + version.txt). Guncelse hemen serbest, update varsa indirme gate'ler.

    // TODO#241 FIX-A (S127 v3.2): version-wait busy-loop -> CPU spin + sonsuz donma duzeltmesi.
    // ESKI: while(true) bos dongu (Sleep yok) -> CPU %100 spin; version cevabi (0x1) gelmezse
    //       'Checking version...' SONSUZA kadar donar (timeout/retry YOK). Paket dusen oyuncu
    //       ekranda kilitli kalir (patron kaniti: ag-zayif oyuncuda surekli takilma).
    // YENI: (1) her tur Sleep(50) -> busy-spin biter. (2) version (m_bVersionGot) belli surede
    //       gelmezse RequestVersion'i tekrar gonder (3 deneme, 7sn arayla). (3) hala gelmezse
    //       'Sunucuya ulasilamadi, tekrar deneyin' state -> sonsuz donma YOK, oyuncu bilgilenir.
    clock_t verSentAt = clock();
    int verRetry = 0;
    const int VER_RETRY_MAX = 3;            // toplam 3 ek deneme
    const clock_t VER_RETRY_INTERVAL = 7000; // 7sn cevap yoksa tekrar gonder

    while (true)
    {
        if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
            return false;

        while (!mSocket->m_qRecvPkt.empty())
        {
            auto pkt = mSocket->m_qRecvPkt.front();
            if (!HandlePacket(*pkt))
                break;

            delete pkt;
            mSocket->m_qRecvPkt.pop();
        }

        // Version henuz gelmediyse: belli arayla tekrar iste, limitte uyari ver (sonsuz donma yok)
        if (!m_bVersionGot && (clock() - verSentAt) > VER_RETRY_INTERVAL)
        {
            if (verRetry < VER_RETRY_MAX)
            {
                verRetry++;
                SetState(std::format("Checking version... (retry {}/{})", verRetry, VER_RETRY_MAX));
                RequestVersion();
                verSentAt = clock();
            }
            else
            {
                SetState(xorstr("Sunucuya ulasilamadi. Lutfen launcher'i tekrar acin."));
                // version alinamadi -> sonsuz spin yerine cik (oyuncu tekrar dener / retry'lar tukendi)
                return false;
            }
        }

        Sleep(50); // CPU busy-spin engelle (ana akis event-driven, recv WSAAsyncSelect ile gelir)
    }
}

void Launcher::Update()
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return;

    RequestNotices();

    m_bVersionGot = true;
    if (!m_bPatchesGot)
        Download();
}

double parseMB(double bytes)
{
    return bytes / 1024 / 1024;
}

int ProgCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded)
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return 0;

    Engine->SetPercent(static_cast<uint8>(round(dNowDownloaded * 100 / dTotalToDownload)));
    Engine->SetState(std::format("Downloading {}: {:.2f}/{:.2f} MB.", Engine->m_currentFile.c_str(), parseMB(dNowDownloaded), parseMB(dTotalToDownload)));
    return 0;
}

bool is_file_exist(const char* fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

int on_extract_entry(const char *filename, void *arg) {
	Engine->SetState(std::format("Extracting: {}", filename));
	return 0;
}

bool Launcher::KnightOnlineCheck()
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            std::string a = entry.szExeFile;
            std::size_t pos = a.find(xorstr("KnightOnLine.exe"));

            if (pos != -1)
                return true;
        }
    }

    return false;
}

bool Launcher::DownloadPatch(std::string server, std::string path, std::string file, std::string expectedHash)
{
    if (Engine->mSocket->GetSocket() == (void*)INVALID_SOCKET)
        return false;

    if (KnightOnlineCheck())
    {
        MessageBoxA(NULL, xorstr("KnightOnLine.exe turn it off please try again"), "CodeGuard", MB_ICONEXCLAMATION);
        ExitProcess(0);
        return false;
    }

	if (m_settingsVersion < m_iVersion)
	{
		CFTPClient FTPClient(PRINT_LOG);
		FTPClient.InitSession(server, 21, "", "", CFTPClient::FTP_PROTOCOL::HTTP, CFTPClient::ENABLE_LOG);
		FTPClient.SetProgressFnCallback(reinterpret_cast<void*>(0xFFFFFFFF), &ProgCallback);
		m_currentFile = file;
		FTPClient.DownloadFile(file, path + "/" + file);
		FTPClient.CleanupSession();

		// MD5 hash dogrulama — DB hash bos olabilir (geri donus uyumlu), o zaman atla
		if (!expectedHash.empty())
		{
			MD5 md5check;
			std::string fileHash = md5check.FileMD5Check(file.c_str());
			if (fileHash != expectedHash)
			{
				SetState(xorstr("Patch hash mismatch: ") + file);
				std::remove(file.c_str());
				return false;
			}
		}

        Sleep(50);
		// S113: zip_extract return value KONTROL et — fail ise Server.ini'ye YAZMA, dosya kalsin (retry icin)
		int extract_result = zip_extract(file.c_str(), WorkingPath, on_extract_entry, NULL);
		if (extract_result != 0) {
			SetState(xorstr("Patch extract failed: ") + file);
			// zip dosyasini SILME — bir sonraki acilista yeniden dene (L1 acilista UI bozulmasini tespit eder)
			return false;
		}
		std::remove(file.c_str());
		// Extract OK ise Server.ini'ye yaz
		std::string versionFromFile = m_currentFile.substr(0, m_currentFile.length() - 4);
		m_settingsVersion = atoi(versionFromFile.c_str());
		WritePrivateProfileStringA(xorstr("Version"), xorstr("Files"), std::to_string(m_settingsVersion).c_str(), (std::string(WorkingPath) + xorstr("\\Server.ini")).c_str());
		if (m_settingsVersion == m_iVersion)
			return true;
		else
			return false;
	}
	return true;
}

void Launcher::Download()
{
	if (m_settingsVersion > m_iVersion)
	{
		SetState(xorstr("Version invalid."));
		ready = false;
		return;
	}

    RequestPatch();
}

void str_tolower(std::string& str)
{
    for (size_t i = 0; i < str.length(); ++i)
        str[i] = (char)tolower(str[i]);
}

bool str_contains(std::string str, std::string find)
{
    std::string s = str;
    str_tolower(s);

    std::string f = find;
    str_tolower(f);

    if (s.find(f) != std::string::npos)
        return true;
    return false;
}

void str_replace(std::string& str, std::string find, std::string replace)
{
    if (find.empty())
        return;

    size_t start_pos = 0;
    while ((start_pos = str.find(find, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, find.length(), replace);
        start_pos += replace.length();
    }
}

void str_split(std::string str, std::string delim, std::vector<std::string>& out)
{
    size_t pos_start = 0, pos_end, delim_len = delim.length();
    std::string token;

    while ((pos_end = str.find(delim, pos_start)) != std::string::npos)
    {
        token = str.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        out.push_back(token);
    }

    out.push_back(str.substr(pos_start));
}

bool Launcher::HandlePacket(Packet& pkt)
{
    int opCode = pkt.GetOpcode();

	switch (opCode)
	{
	case 0x1:
	{
		if (!m_bVersionGot)
        {
            pkt >> m_iVersion;
			Update();
		}
	}
	break;
	case 0x2:
	{
		std::string ftpURL, ftpPATH;
		uint16 fileCount = 0;
		pkt >> ftpURL >> ftpPATH >> fileCount;
		for (int i = 0; i < fileCount; i++)
		{
			std::string file, fileHash;
			pkt >> file >> fileHash;
			DownloadPatch(ftpURL, ftpPATH, file, fileHash);
		}
        SetState(xorstr("Files are being packed..."));
        CHDRSystem* hdrPacker = new CHDRSystem;
        hdrPacker->Pack();
        delete hdrPacker;
        Engine->SetPercent(100);
		SetState(xorstr("Update Completed..."));
		ready = true;
	}
	break;
    case 0x3:
    {
        uint16 noticeCount;
        pkt >> noticeCount;
        std::string notice = "";
        for (uint16 i = 0; i < noticeCount; i++)
        {
            pkt >> notice;
            m_lNotices.push_back(notice);
        }
        std::reverse(m_lNotices.begin(), m_lNotices.end());
    }
    break;
    // S114 K3 FAZ 4: HWID_REPORT cevap (opcode 0x84)
    // 0=OK (devam), 1=BANNED (Launcher kapanir)
    case 0x84:
    {
        uint8 result = 0;
        pkt >> result;
        if (result == 1) {
            // S114 K3: HWID banli — flag set, Launcher.cpp ana loop'u ERROR GIF gosterip cikar
            m_hwidBanned = true;
        }
    }
    break;
	default:
		break;
	}
    
	return true;
}

// ============================================================
// S115 AUTO-UPDATE LAUNCHER
// ============================================================
// Akis: version.txt cek -> versiyon karsilastir -> indir -> md5 dogrula
//       -> helper.bat olustur -> calistir -> ExitProcess
// Risk: MITM/sahte exe icin MD5 dogrulama; fail -> normal akis devam
// ============================================================

// HTTP GET wrapper (WinHTTP, ekstra DLL gerekmiyor — Windows built-in)
bool Launcher::HttpGet(const std::wstring& host, INTERNET_PORT port,
                       const std::wstring& path, std::vector<BYTE>& outData,
                       DWORD maxSizeBytes)
{
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    bool success = false;
    outData.clear();

    do {
        hSession = WinHttpOpen(L"MalaysiaKO-Launcher/1.5",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) break;

        // Timeouts: resolve=5s, connect=5s, send=10s, receive=10s
        // FAZ 5c: Auto-update timeout kucult — version.txt 100 byte yeter
        // Eski: 5/5/10/10 (max 30sn beklerdi)
        // Yeni: 3/3/5/5 (max 16sn) — patron "version cekiliyor" uzun gozukmesin
        WinHttpSetTimeouts(hSession, 3000, 3000, 5000, 5000);

        hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect) break;

        hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) break;

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) break;
        if (!WinHttpReceiveResponse(hRequest, NULL)) break;

        // HTTP status code kontrol
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (!WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                WINHTTP_NO_HEADER_INDEX)) break;
        if (statusCode != 200) break;

        // Veri oku
        DWORD bytesAvail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0)
        {
            if (outData.size() + bytesAvail > maxSizeBytes)
            {
                // Boyut limit asildi - guvenlik
                outData.clear();
                break;
            }
            size_t offset = outData.size();
            outData.resize(offset + bytesAvail);
            DWORD bytesRead = 0;
            if (!WinHttpReadData(hRequest, outData.data() + offset,
                                 bytesAvail, &bytesRead))
            {
                outData.clear();
                break;
            }
            if (bytesRead == 0) break;
            outData.resize(offset + bytesRead);
        }

        success = !outData.empty();
    } while (false);

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return success;
}

// MD5 hex string hesapla (bayt arrayden) - lowercase 32 hex char
static std::string ComputeMd5Hex(const BYTE* data, size_t len)
{
    MD5 md5;
    char* hex = md5.digestMemory((BYTE*)data, (int)len);
    std::string out = hex ? std::string(hex) : std::string();
    // Zaten lowercase ama emin olalim
    for (char& c : out) c = (char)tolower((unsigned char)c);
    return out;
}

// version.txt parse: "1.6|md5|size"
static bool ParseVersionTxt(const std::string& content,
                            int& outMaj, int& outMin,
                            std::string& outMd5, size_t& outSize)
{
    // Beyaz bosluklari temizle
    std::string s = content;
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                          s.back() == ' ' || s.back() == '\t'))
        s.pop_back();

    auto p1 = s.find('|');
    if (p1 == std::string::npos) return false;
    auto p2 = s.find('|', p1 + 1);
    if (p2 == std::string::npos) return false;

    std::string ver = s.substr(0, p1);
    outMd5 = s.substr(p1 + 1, p2 - p1 - 1);
    std::string sizeStr = s.substr(p2 + 1);

    // Versiyon parse (MAJOR.MINOR)
    auto dot = ver.find('.');
    if (dot == std::string::npos) return false;
    try {
        outMaj = std::stoi(ver.substr(0, dot));
        outMin = std::stoi(ver.substr(dot + 1));
        outSize = (size_t)std::stoull(sizeStr);
    } catch (...) { return false; }

    // MD5 32 hex char olmali
    if (outMd5.length() != 32) return false;
    for (char c : outMd5) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    // Lowercase normalize
    for (char& c : outMd5) c = (char)tolower((unsigned char)c);

    return true;
}

bool Launcher::CheckForUpdate()
{
    // Sunucu IP'sini Server.ini'den oku (HWID/Patch ile ayni)
    std::string serverIP = m_settingsIP;
    if (serverIP.empty()) return false;

    // ASCII -> wide
    std::wstring wHost(serverIP.begin(), serverIP.end());

    // 1) version.txt cek (10 KB limit yeter)
    std::vector<BYTE> verData;
    if (!HttpGet(wHost, 80, L"/patch/launcher/version.txt", verData, 10 * 1024))
        return false;

    std::string verStr((const char*)verData.data(), verData.size());
    int remoteMaj = 0, remoteMin = 0;
    std::string remoteMd5;
    size_t remoteSize = 0;
    if (!ParseVersionTxt(verStr, remoteMaj, remoteMin, remoteMd5, remoteSize))
        return false;

    // 2) Local versiyonla karsilastir
    int remoteVal = remoteMaj * 1000 + remoteMin;
    int localVal = LAUNCHER_BUILD_VERSION_MAJOR * 1000 + LAUNCHER_BUILD_VERSION_MINOR;

    // Boyut sanity (50 MB hard limit) — once kontrol (MD5 dali da kullanir)
    if (remoteSize == 0 || remoteSize > 50 * 1024 * 1024) return false;

    // S115 FIX (MD5-kor guncelleme bugu): versiyon kontrolu TEK BASINA yetmez.
    // Ayni versiyon etiketi (orn 2.8) ama FARKLI icerik (debug build vs release) durumunda
    // eski kod "guncelim" deyip yeni patch'i CEKMIYORDU. Kanit: MalaysiaKO2 debug 2.8 (4.5MB)
    // vs sunucu release 2.8 (2.5MB) -> ayni versiyon, farkli MD5, guncelleme atlandi.
    // COZUM: remote versiyon ESKI degilse (>=local), kendi exe MD5'ini remote MD5 ile karsilastir.
    //        MD5 farkliysa (icerik degismis) yine indir. Sadece remote ESKI ise (remoteVal<localVal) gec.
    if (remoteVal < localVal)
        return false; // Remote gercekten ESKI — gec (downgrade yok)

    if (remoteVal == localVal)
    {
        // Versiyon esit — icerik (MD5) ayni mi kontrol et. Ayni ise guncelim, gec.
        char selfPath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(NULL, selfPath, MAX_PATH) == 0)
            return false; // kendi yolu alinamadi — guvenli taraf, gec
        MD5 md5self;
        std::string selfMd5 = md5self.FileMD5Check(selfPath);
        if (!selfMd5.empty() &&
            !remoteMd5.empty() &&
            _stricmp(selfMd5.c_str(), remoteMd5.c_str()) == 0)
        {
            return false; // MD5 ESIT — gercekten guncel, gec
        }
        // MD5 farkli (veya hesaplanamadi + remote MD5 var) -> icerik degismis, INDIR
    }
    // remoteVal > localVal -> yeni versiyon, dogrudan indir

    // TODO#241 D1 (S127 v3.3): buraya gelindiyse GERCEKTEN guncelleme inecek (remote yeni VEYA
    // MD5 farkli; version-esit+MD5-ayni dallari yukarida 'return false' ile elendi). m_bUpdating=true
    // -> oyuncu START'a basamaz (StartClick/HandleMouse gate), guncelleme-restart cakismasi onlenir.
    m_bUpdating = true;
    SetState(xorstr("Guncelleme indiriliyor, lutfen bekleyin..."));

    // 3) Yeni exe indir + MD5 dogrula
    std::string tempPath;
    if (!DownloadUpdateFile(remoteMd5, remoteSize, tempPath))
    {
        // Indirme basarisiz -> kilidi ac (oyuncu mevcut surumle oynayabilsin, sonsuz kilit YOK)
        m_bUpdating = false;
        SetState(xorstr("Guncelleme indirilemedi. Mevcut surumle devam ediliyor."));
        return false;
    }

    // 4) Helper batch olustur ve calistir, exit (m_bUpdating true kalir — zaten kapaniyoruz)
    return LaunchUpdaterAndExit(tempPath);
}

bool Launcher::DownloadUpdateFile(const std::string& expectedMd5,
                                  size_t expectedSize,
                                  std::string& outTempPath)
{
    std::string serverIP = m_settingsIP;
    if (serverIP.empty()) return false;
    std::wstring wHost(serverIP.begin(), serverIP.end());

    std::vector<BYTE> exeData;
    if (!HttpGet(wHost, 80, L"/patch/launcher/Launcher.exe", exeData,
                 (DWORD)(expectedSize + 1024)))
        return false;

    // Boyut dogrulama
    if (exeData.size() != expectedSize) return false;

    // MD5 dogrulama
    std::string actualMd5 = ComputeMd5Hex(exeData.data(), exeData.size());
    if (actualMd5 != expectedMd5) return false;

    // %TEMP%\Launcher_new_<pid>.tmp
    char tempDir[MAX_PATH] = { 0 };
    if (GetTempPathA(MAX_PATH, tempDir) == 0) return false;
    char tempFile[MAX_PATH] = { 0 };
    snprintf(tempFile, MAX_PATH, "%sMalaysiaKO_Launcher_new_%lu.tmp",
             tempDir, GetCurrentProcessId());

    std::ofstream ofs(tempFile, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs.write((const char*)exeData.data(), exeData.size());
    ofs.close();
    if (!ofs.good()) return false;

    outTempPath = tempFile;
    return true;
}

bool Launcher::LaunchUpdaterAndExit(const std::string& tempNewExePath)
{
    // Mevcut Launcher.exe yolu
    char selfPath[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, selfPath, MAX_PATH) == 0) return false;

    // Helper batch yolu (%TEMP%)
    char tempDir[MAX_PATH] = { 0 };
    if (GetTempPathA(MAX_PATH, tempDir) == 0) return false;
    char batPath[MAX_PATH] = { 0 };
    snprintf(batPath, MAX_PATH, "%sMalaysiaKO_updater_%lu.bat",
             tempDir, GetCurrentProcessId());

    // Helper batch icerik
    // 1) 2sn bekle (Launcher kapansin)
    // 2) Eski exe sil
    // 3) tmp -> Launcher.exe
    // 4) Yeni Launcher.exe baslat (cwd = eski klasor)
    // 5) Self-delete
    std::ofstream bat(batPath, std::ios::trunc);
    if (!bat) return false;
    bat << "@echo off\r\n"
        << "timeout /t 2 /nobreak > nul\r\n"
        << ":retry\r\n"
        << "del /F /Q \"" << selfPath << "\" 2>nul\r\n"
        << "if exist \"" << selfPath << "\" (\r\n"
        << "    timeout /t 1 /nobreak > nul\r\n"
        << "    goto retry\r\n"
        << ")\r\n"
        << "move /Y \"" << tempNewExePath << "\" \"" << selfPath << "\" > nul\r\n"
        << "if not exist \"" << selfPath << "\" (\r\n"
        << "    exit /b 1\r\n"
        << ")\r\n"
        << "start \"\" \"" << selfPath << "\"\r\n"
        << "del \"%~f0\"\r\n";
    bat.close();
    if (!bat.good()) return false;

    // Helper'i sessizce baslat
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = { 0 };

    std::string cmdLine = std::string("\"") + batPath + "\"";
    char cmdBuf[MAX_PATH * 2] = { 0 };
    strncpy_s(cmdBuf, cmdLine.c_str(), MAX_PATH * 2 - 1);

    if (!CreateProcessA(NULL, cmdBuf, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return false;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Launcher'i sonlandir - helper devraldi
    ExitProcess(0);
    return true;
}

// ============================================================
// S115 CRASH REPORTER
// ============================================================
// Crash anında MiniDumpWriteDump ile dump uret, sunucuya HTTP POST.
// Sunucu: nginx -> PHP /crash_upload.php -> disk + DB
// ============================================================

// Server IP'sini al (Launcher kapanmis olabilir, Server.ini'den oku)
static std::string GetServerIPForCrash()
{
    char path[MAX_PATH] = { 0 };
    GetCurrentDirectoryA(MAX_PATH, path);
    std::string iniPath = std::string(path) + "\\Server.ini";
    char buf[128] = { 0 };
    GetPrivateProfileStringA("Server", "IP0", "104.238.23.99", buf, 128, iniPath.c_str());
    return std::string(buf);
}

bool Launcher::UploadCrashDump(const std::string& dumpPath,
                               const std::string& exeName,
                               const std::string& account,
                               const std::string& version,
                               const std::string& fingerprint,    // S115 v2.5 FAZ 4
                               const std::string& moduleName,     // S115 v2.5 FAZ 4
                               const std::string& exceptionCode,  // S115 v2.5 FAZ 4
                               const std::string& crashOffset)    // S115 v2.5 FAZ 4
{
    // Dump dosyasini oku
    std::ifstream ifs(dumpPath, std::ios::binary | std::ios::ate);
    if (!ifs) return false;
    std::streamsize sz = ifs.tellg();
    if (sz <= 0 || sz > 2 * 1024 * 1024) return false; // boyut limit
    ifs.seekg(0, std::ios::beg);
    std::vector<char> fileData((size_t)sz);
    if (!ifs.read(fileData.data(), sz)) return false;
    ifs.close();

    // OS versiyon
    char osBuf[128] = { 0 };
    OSVERSIONINFOA osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(osvi);
#pragma warning(push)
#pragma warning(disable: 4996) // GetVersionEx deprecated
    if (GetVersionExA(&osvi))
    {
        snprintf(osBuf, 128, "Windows %lu.%lu build %lu",
                 osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    }
#pragma warning(pop)

    // Multipart form-data hazirla
    std::string boundary = "----MalaysiaKOCrashBoundary7d8f3a";
    std::string body;
    body.reserve(fileData.size() + 1024);

    auto addField = [&](const std::string& name, const std::string& val) {
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
        body += val + "\r\n";
    };

    addField("exe_name", exeName);
    addField("account",  account);
    addField("version",  version);
    addField("os",       osBuf);

    // S115 v2.5 FAZ 4: Yeni fingerprint metadata (bos string ise eklenmez)
    if (!fingerprint.empty())   addField("fingerprint",    fingerprint);
    if (!moduleName.empty())    addField("module_name",    moduleName);
    if (!exceptionCode.empty()) addField("exception_code", exceptionCode);
    if (!crashOffset.empty())   addField("crash_offset",   crashOffset);

    // Dosya field
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"crash.dmp\"\r\n";
    body += "Content-Type: application/octet-stream\r\n\r\n";
    body.append(fileData.data(), fileData.size());
    body += "\r\n--" + boundary + "--\r\n";

    // HTTP POST
    std::string serverIP = GetServerIPForCrash();
    if (serverIP.empty()) return false;
    std::wstring wHost(serverIP.begin(), serverIP.end());

    HINTERNET hSession = WinHttpOpen(L"MalaysiaKO-Launcher-CrashReporter/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;
    // TODO#241 FIX-C (S127 v3.2): crash upload timeout kisalt (5sn->2sn).
    // ESKI: 3000/3000/5000/5000 -> worst-case ~16sn. Crash crash-filter ICINDE SYNC cagriliyor
    //       (LauncherCrashFilter ~1361) -> launcher crash'inde oyuncu 16sn DONMUS gorur,
    //       'acilmiyor' deyip force-close yapar. YENI: resolve/connect 1.5sn, send/recv 2sn
    //       -> worst-case ~7sn. Sunucu ag yoksa hizli pes eder, crash raporu best-effort.
    WinHttpSetTimeouts(hSession, 1500, 1500, 2000, 2000);

    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), 80, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
        L"/crash_upload.php", NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring wHeader = L"Content-Type: multipart/form-data; boundary=";
    wHeader.append(boundary.begin(), boundary.end());

    BOOL sendOk = WinHttpSendRequest(hRequest, wHeader.c_str(),
        (DWORD)-1, (LPVOID)body.data(), (DWORD)body.size(),
        (DWORD)body.size(), 0);

    bool success = false;
    if (sendOk && WinHttpReceiveResponse(hRequest, NULL))
    {
        DWORD status = 0, statusSize = sizeof(status);
        if (WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                WINHTTP_NO_HEADER_INDEX))
        {
            success = (status == 200);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

// Unhandled exception handler - SetUnhandledExceptionFilter ile kurulur
// S115 v2.5 FAZ 4: Sentry-stili "on_crash" filtre — whitelist + duplicate + bos dump
static LONG WINAPI LauncherCrashFilter(EXCEPTION_POINTERS* ep)
{
    // ---------------------------------------------------------------
    // FAZ 4 (S115 v2.5): Crash bilgilerini cikar + fingerprint hesapla
    // ---------------------------------------------------------------
    CrashFingerprint::CrashInfo info;
    bool haveInfo = CrashFingerprint::ExtractCrashInfo(ep, info);

    // ---------------------------------------------------------------
    // FAZ 4 FILTRE 1: Whitelist exception (user-sebepli crash)
    // STATUS_INTEGER_DIVIDE_BY_ZERO, FLOAT_OVERFLOW, DBG_CONTROL_C vs
    // → dump uretme, sunucuya yollama, sessizce gec
    // ---------------------------------------------------------------
    if (haveInfo && CrashFingerprint::IsWhitelistedException(info.exceptionCode)) {
        return EXCEPTION_EXECUTE_HANDLER; // OS'a "ben hallettim" de, normal exit
    }

    // ---------------------------------------------------------------
    // FAZ 4 FILTRE 2: Duplicate (son 24sa ayni fingerprint)
    // → dump uretme, gec
    // ---------------------------------------------------------------
    if (haveInfo && CrashFingerprint::ComputeFingerprint(info)) {
        if (CrashFingerprint::IsDuplicateInLast24Hours(info.fingerprint)) {
            return EXCEPTION_EXECUTE_HANDLER; // duplicate, sessizce gec
        }
    }

    // ---------------------------------------------------------------
    // Dump dosyasini uret (MiniDumpWriteDump)
    // ---------------------------------------------------------------
    char tempDir[MAX_PATH] = { 0 };
    GetTempPathA(MAX_PATH, tempDir);
    char dumpPath[MAX_PATH] = { 0 };
    snprintf(dumpPath, MAX_PATH, "%smalaysiako_launcher_crash_%lu_%lu.dmp",
             tempDir, GetCurrentProcessId(), GetTickCount());

    HANDLE hFile = CreateFileA(dumpPath, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION mei = { 0 };
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = ep;
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                          hFile, MiniDumpNormal,
                          ep ? &mei : NULL, NULL, NULL);
        CloseHandle(hFile);

        // -----------------------------------------------------------
        // FAZ 4 FILTRE 3: Dump dosyasi gecerli mi? (MDMP magic + size)
        // → bos/cop ise gec
        // -----------------------------------------------------------
        if (!CrashFingerprint::IsValidDump(dumpPath)) {
            DeleteFileA(dumpPath); // cop dosyayi temizle
            return EXCEPTION_EXECUTE_HANDLER;
        }

        // -----------------------------------------------------------
        // FAZ 4: Fingerprint'i kayit et (gelecekte duplicate yakalansin)
        // -----------------------------------------------------------
        if (haveInfo && !info.fingerprint.empty()) {
            CrashFingerprint::RecordFingerprint(info.fingerprint);
        }

        // Account ve version Engine'den okumayi dene (varsa)
        std::string account, version;
        // Engine objesi crash sirasinda erisilebilir olmayabilir -> try/catch
        try {
            if (Engine) {
                version = std::to_string(LAUNCHER_BUILD_VERSION_MAJOR) + "." +
                          std::to_string(LAUNCHER_BUILD_VERSION_MINOR);
            }
        } catch (...) { /* gec */ }

        // -----------------------------------------------------------
        // Upload (5sn timeout, fail olursa gec)
        // FAZ 4: fingerprint + module + exception + offset eklenmis
        // -----------------------------------------------------------
        Launcher::UploadCrashDump(dumpPath, "Launcher.exe", account, version,
                                  haveInfo ? info.fingerprint : "",
                                  haveInfo ? info.moduleName : "",
                                  haveInfo ? info.exceptionCode : "",
                                  haveInfo ? info.crashOffset : "");
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

void Launcher::InstallCrashHandler()
{
    SetUnhandledExceptionFilter(LauncherCrashFilter);
}

Launcher::~Launcher()
{
    mSocket->Release();
}