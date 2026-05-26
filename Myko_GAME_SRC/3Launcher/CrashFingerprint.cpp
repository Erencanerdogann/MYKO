// =========================================================================
// S115 FAZ 4 — Crash Fingerprint Implementasyonu
// Yazan: CHIP | Tarih: 2026-05-26
// =========================================================================
#include "stdafx.h"
#include "CrashFingerprint.h"
#include <shlobj.h>      // SHGetFolderPathA
#include <wincrypt.h>    // MD5
#include <psapi.h>       // GetModuleFileNameExA
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <algorithm>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")

namespace CrashFingerprint {

// =====================================================================
// HELPER: AppData klasoru
// =====================================================================
static std::string GetAppDataDir() {
    char path[MAX_PATH] = { 0 };
    // CSIDL_LOCAL_APPDATA = %LOCALAPPDATA%
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string dir = std::string(path) + "\\MalaysiaKO";
        CreateDirectoryA(dir.c_str(), NULL); // varsa zaten, hatayi yutar
        return dir;
    }
    return "";
}

static std::string GetFingerprintFile() {
    std::string dir = GetAppDataDir();
    if (dir.empty()) return "";
    return dir + "\\crash_fp.dat";
}

// =====================================================================
// HELPER: lowercase
// =====================================================================
static std::string ToLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return out;
}

// =====================================================================
// HELPER: MD5 (Windows CryptoAPI, harici lib kullanmiyoruz)
// =====================================================================
static std::string Md5Hex(const std::string& input) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string result;

    if (CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
            if (CryptHashData(hHash, (BYTE*)input.data(), (DWORD)input.size(), 0)) {
                BYTE hash[16] = { 0 };
                DWORD hashLen = 16;
                if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                    char hex[33] = { 0 };
                    for (int i = 0; i < 16; i++) {
                        sprintf_s(hex + i*2, 3, "%02x", hash[i]);
                    }
                    result = hex;
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return result;
}

// =====================================================================
// ExtractCrashInfo — exception_pointers'tan module+exception+offset cikar
// =====================================================================
bool ExtractCrashInfo(EXCEPTION_POINTERS* ep, CrashInfo& out) {
    if (!ep || !ep->ExceptionRecord) return false;

    // Exception code: 0xC0000005 format (lowercase)
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    char codeBuf[16] = { 0 };
    sprintf_s(codeBuf, "0x%08x", code);
    out.exceptionCode = ToLower(codeBuf);

    // Crash adresi
    DWORD_PTR addr = (DWORD_PTR)ep->ExceptionRecord->ExceptionAddress;

    // Hangi modulden gelmis? — VirtualQuery + GetModuleFileName
    HMODULE hMod = NULL;
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        hMod = (HMODULE)mbi.AllocationBase;
    }

    if (hMod) {
        // Modul yolunu al
        char modPath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(hMod, modPath, MAX_PATH) > 0) {
            // Sadece dosya adini al (path'i at)
            const char* fileName = strrchr(modPath, '\\');
            out.moduleName = fileName ? (fileName + 1) : modPath;
            out.moduleName = ToLower(out.moduleName);
        } else {
            out.moduleName = "unknown";
        }

        // Module relative offset
        DWORD_PTR baseAddr = (DWORD_PTR)hMod;
        DWORD_PTR relOffset = addr - baseAddr;
        char offsetBuf[24] = { 0 };
        sprintf_s(offsetBuf, "0x%llx", (unsigned long long)relOffset);
        out.crashOffset = ToLower(offsetBuf);
    } else {
        // Modul bulunamadi — absolute adres kullan (gruplama daha az ise yaramaz)
        out.moduleName = "unknown";
        char offsetBuf[24] = { 0 };
        sprintf_s(offsetBuf, "0x%llx", (unsigned long long)addr);
        out.crashOffset = ToLower(offsetBuf);
    }

    return true;
}

// =====================================================================
// ComputeFingerprint — MD5 hesapla (PHP lib_fingerprint.php ile ayni)
// =====================================================================
bool ComputeFingerprint(CrashInfo& info) {
    if (info.moduleName.empty() || info.exceptionCode.empty() || info.crashOffset.empty()) {
        return false;
    }

    // Algoritma: md5( lower(module) | lower(exception) | lower(offset) )
    // PHP'deki calculateCrashFingerprint ile ayni
    std::string normalized =
        ToLower(info.moduleName) + "|" +
        ToLower(info.exceptionCode) + "|" +
        ToLower(info.crashOffset);

    info.fingerprint = Md5Hex(normalized);
    return !info.fingerprint.empty();
}

// =====================================================================
// IsWhitelistedException — user-sebepli crash kontrolu
// PHP lib_fingerprint.php isWhitelistedException ile AYNI liste
// =====================================================================
bool IsWhitelistedException(const std::string& exceptionCode) {
    std::string lower = ToLower(exceptionCode);
    static const char* whitelist[] = {
        "0xc0000094",  // INTEGER_DIVIDE_BY_ZERO
        "0xc0000095",  // INTEGER_OVERFLOW
        "0xc000008e",  // FLOAT_DIVIDE_BY_ZERO
        "0xc000008f",  // FLOAT_INEXACT_RESULT
        "0xc0000091",  // FLOAT_OVERFLOW
        "0xc0000093",  // FLOAT_UNDERFLOW
        "0x40010005",  // DBG_CONTROL_C
        "0x40010008",  // DBG_TERMINATE_PROCESS
    };
    for (const char* w : whitelist) {
        if (lower == w) return true;
    }
    return false;
}

// =====================================================================
// IsDuplicateInLast24Hours — son 24sa dosyada bu fingerprint var mi
// Dosya format: <unixtime>\t<fingerprint>\n (her satir)
// =====================================================================
bool IsDuplicateInLast24Hours(const std::string& fingerprint) {
    if (fingerprint.empty()) return false;
    std::string filePath = GetFingerprintFile();
    if (filePath.empty()) return false;

    std::ifstream f(filePath);
    if (!f.is_open()) return false; // dosya yoksa duplicate degil

    time_t now = time(nullptr);
    time_t cutoff = now - (24 * 3600); // 24 saat onceki unixtime

    std::string line;
    while (std::getline(f, line)) {
        // <unixtime>\t<fingerprint>
        size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;

        time_t ts = (time_t)atoll(line.substr(0, tab).c_str());
        if (ts < cutoff) continue; // eski, gec

        std::string fp = line.substr(tab + 1);
        if (fp == fingerprint) {
            f.close();
            return true; // duplicate bulundu
        }
    }
    f.close();
    return false;
}

// =====================================================================
// RecordFingerprint — fingerprint kaydet + eski (>24sa) temizle
// =====================================================================
void RecordFingerprint(const std::string& fingerprint) {
    if (fingerprint.empty()) return;
    std::string filePath = GetFingerprintFile();
    if (filePath.empty()) return;

    time_t now = time(nullptr);
    time_t cutoff = now - (24 * 3600);

    // 1) Eski kayitlari oku (24sa icinde olanlari sakla)
    std::vector<std::string> kept;
    {
        std::ifstream f(filePath);
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                size_t tab = line.find('\t');
                if (tab == std::string::npos) continue;
                time_t ts = (time_t)atoll(line.substr(0, tab).c_str());
                if (ts >= cutoff) {
                    kept.push_back(line);
                }
            }
            f.close();
        }
    }

    // 2) Yeni kayit ekle
    std::ostringstream newLine;
    newLine << now << "\t" << fingerprint;
    kept.push_back(newLine.str());

    // 3) Dosyaya yaz (truncate + yeni icerik)
    std::ofstream out(filePath, std::ios::trunc);
    if (out.is_open()) {
        for (const auto& l : kept) {
            out << l << "\n";
        }
        out.close();
    }
}

// =====================================================================
// IsValidDump — MDMP magic byte + 10KB-2MB kontrol
// =====================================================================
bool IsValidDump(const std::string& dumpPath) {
    HANDLE h = CreateFileA(dumpPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;

    // Boyut kontrolu
    LARGE_INTEGER size = { 0 };
    if (!GetFileSizeEx(h, &size)) {
        CloseHandle(h);
        return false;
    }
    if (size.QuadPart < 10 * 1024 || size.QuadPart > 2 * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }

    // Magic byte: ilk 4 byte "MDMP" (0x504D444D little-endian)
    char header[4] = { 0 };
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(h, header, 4, &bytesRead, NULL);
    CloseHandle(h);

    if (!ok || bytesRead != 4) return false;
    return (header[0] == 'M' && header[1] == 'D' &&
            header[2] == 'M' && header[3] == 'P');
}

} // namespace CrashFingerprint
