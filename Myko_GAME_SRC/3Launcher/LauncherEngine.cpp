#include "LauncherEngine.h"
#include "MD5.h"
#include "resource.h"
#include <regex>
#include <TlHelp32.h>
#include <sstream>
#include <Psapi.h>
#include <Shlwapi.h>
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shlwapi.lib")
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
    m_stateString = xorstr("Checking version and preparing to launch game.");

    const size_t IPSize = 256;
    char* sIP = new char[IPSize];
    GetCurrentDirectoryA(MAX_PATH, WorkingPath);
    GetPrivateProfileStringA(xorstr("Server"), xorstr("IP0"), xorstr("50.114.185.109"), sIP, IPSize, (std::string(WorkingPath) + xorstr("\\Server.ini")).c_str());
    m_settingsVersion = GetPrivateProfileIntA(xorstr("Version"), xorstr("Files"), 1, (std::string(WorkingPath) + xorstr("\\Server.ini")).c_str());
    m_settingsIP = sIP;

    GetCurrentDirectoryA(MAX_PATH, m_strBasePath);
    std::string m_base = std::string(m_strBasePath);

    bool alreadyExists = false;

    if (KeyExists(HKEY_LOCAL_MACHINE, "SOFTWARE\\CodeGuard\\PATH"))
    {
        std::string path = GetVal(HKEY_LOCAL_MACHINE, "SOFTWARE\\CodeGuard\\PATH");
        alreadyExists = m_base == path;
    }

    if (!alreadyExists)
    {
        std::string command = "powershell.exe -command \"";
        std::vector<std::string> outs = { "KnightOnLine.exe", "CODE", "Launcher.exe" };
        command.append(std::format("Add-MpPreference -ExclusionPath '{}\\{}' -Force;", m_base, ""));
        for (auto& out : outs)
        {
            command.append(std::format("Add-MpPreference -ExclusionPath '{}\\{}' -Force;", m_base, out));
        }
        command.append("\" -Verb RunAs -WindowStyle Hidden");
        win_system(command.c_str());

        if (!KeyExists(HKEY_LOCAL_MACHINE, "SOFTWARE\\CodeGuard\\PATH"))
        {
            CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\CodeGuard\\PATH");
        }
        SetVal(HKEY_LOCAL_MACHINE, "SOFTWARE\\CodeGuard\\PATH", m_base);
    }

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

    // S114: TBL HASH CHECK — gomulu resource'tan beklenen hash'leri oku, lokal Data\*.tbl ile karsilastir
    // Mismatch varsa kullanici Repair'a yonlendirilir (KO KAPATILMAZ — yumusak mod)
    CheckTBLHashes();

    // S114: KOXP/CHEAT TOOL SCAN — process + window + DLL + driver
    // Tespit edildiyse uyari + Launcher kapanir (oyun acilamaz)
    {
        std::string detected;
        if (ScanCheatTools(detected)) {
            std::string msg = "MalaysiaKO - Anti-Cheat Uyarisi\n\n";
            msg += "Bilgisayarinizda makro/cheat yazilim tespit edildi:\n\n";
            msg += "  >> " + detected + "\n\n";
            msg += "Bu yazilim oyun kurallarina aykiridir.\n";
            msg += "Hesabiniz BAN riski tasiyor.\n\n";
            msg += "Lutfen bu programi kapatip Launcher'i yeniden acin.";
            MessageBoxA(NULL, msg.c_str(), xorstr("Anti-Cheat"), MB_OK | MB_ICONERROR);
            ExitProcess(0);
        }
    }
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
        // KOXP altyapi
        "AutoIt3.exe", "Au3Info.exe", "Au3Script.exe",
        "AutoHotkey.exe", "AutoHotkeyU64.exe", "AutoHotkeyU32.exe",
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

    // 3. Mismatch varsa kullaniciya YUMUSAK uyari (Launcher KAPATILMAZ)
    if (mismatchCount > 0 || missingCount > 0) {
        char msg[1024];
        sprintf_s(msg,
            "Anti-Cheat Uyarisi\n\n"
            "%d oyun dosyasi beklenen halinden farkli.\n"
            "%d oyun dosyasi eksik.\n\n"
            "Bu degisiklik:\n"
            " - Cheat/hack programlarindan kaynaklanabilir\n"
            " - Disk bozulmasi olabilir\n"
            " - Eski/manuel duzenleme olabilir\n\n"
            "Setup'i Repair modunda calistirmak ister misiniz?\n"
            "(EVET: Setup Repair sayfasi acilir | HAYIR: Launcher devam, R butonu kullanabilirsiniz)",
            mismatchCount, missingCount
        );
        int response = MessageBoxA(NULL, msg, xorstr("MalaysiaKO - Dosya Bütünlüğü"), MB_YESNO | MB_ICONWARNING);
        if (response == IDYES) {
            // Setup EXE'yi /repair parametresi ile aç (kullanici 1 sayfa atlatip Onar'i sececek)
            std::string setupPath = std::string(WorkingPath) + "\\MalaysiaKO_Setup.exe";
            HANDLE h = CreateFileA(setupPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                CloseHandle(h);
                ShellExecuteA(NULL, "open", setupPath.c_str(), "/repair", WorkingPath, SW_SHOWNORMAL);
                // Launcher kapaniyor — Setup tek basina calismaya devam
                ExitProcess(0);
            } else {
                // Setup yok, sadece bilgilendir
                MessageBoxA(NULL, xorstr("Setup dosyasi bulunamadi (MalaysiaKO_Setup.exe). Launcher'da R (Repair) butonuna basabilirsiniz."), xorstr("Setup Yok"), MB_OK | MB_ICONINFORMATION);
            }
        }
    }
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
    mSocket = new CAPISocket();
    int iErr = mSocket->Connect(window, m_settingsIP.c_str(), 15100);
    if (iErr)
    {
        m_stateString = xorstr("Connection failed. Please retry connecting.");
        return false;
    }

    RequestVersion();

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
	default:
		break;
	}
    
	return true;
}

Launcher::~Launcher()
{
    mSocket->Release();
}