// =========================================================================
// S115 FAZ 4 — Crash Fingerprint Modulu (client-side)
// Yazan: CHIP | Tarih: 2026-05-26
// =========================================================================
// AMAC: Crash anında module + exception_code + offset'ten MD5 fingerprint
//       hesapla, %LOCALAPPDATA% dosyada son 24sa ile karsilastir, duplicate
//       ise uploaddan once SKIP et.
//
// ONEMLI: Bu MD5 algoritmasi PHP lib_fingerprint.php ile AYNI olmali.
//         Algoritma: md5( lower(module) + "|" + lower(exception) + "|" + lower(offset) )
// =========================================================================
#pragma once

#include <string>
#include <windows.h>

namespace CrashFingerprint {

// ---------------------------------------------------------------------
// Crash bilgisi — exception_pointers'tan cikarilir
// ---------------------------------------------------------------------
struct CrashInfo {
    std::string moduleName;      // "KnightOnLine.exe", "d3d9.dll" vb
    std::string exceptionCode;   // "0xC0000005" vb (lowercase, 0x prefix)
    std::string crashOffset;     // "0x12345" (module + offset hex, lowercase)
    std::string fingerprint;     // 32 char MD5 hex (calculate sonrasi dolu)
};

// ---------------------------------------------------------------------
// Crash exception bilgilerini cikar
// ep: SetUnhandledExceptionFilter'dan gelen ExceptionPointers
// out: doldurulacak CrashInfo (fingerprint hala bos)
// return: TRUE = bilgi cikarildi, FALSE = bilgi cikarilamadi (skip et)
// ---------------------------------------------------------------------
bool ExtractCrashInfo(EXCEPTION_POINTERS* ep, CrashInfo& out);

// ---------------------------------------------------------------------
// Fingerprint hesapla (MD5)
// info: ExtractCrashInfo doldurmus olmali (module/exception/offset dolu)
// return: TRUE = info.fingerprint dolu (32 char hex), FALSE = hesaplanamadi
// ---------------------------------------------------------------------
bool ComputeFingerprint(CrashInfo& info);

// ---------------------------------------------------------------------
// Whitelist exception? (user-sebepli crash, sunucuya yollanmasi anlamsiz)
// exceptionCode: "0xc0000094" vb (lowercase)
// return: TRUE = whitelist, SKIP et
// ---------------------------------------------------------------------
bool IsWhitelistedException(const std::string& exceptionCode);

// ---------------------------------------------------------------------
// Duplicate kontrol (son 24sa)
// fingerprint: 32 char MD5 hex
// return: TRUE = bu fingerprint son 24sa kaydedildi, SKIP et
//         FALSE = yeni veya 24sa+ once, devam et + dosyaya yaz
// ---------------------------------------------------------------------
bool IsDuplicateInLast24Hours(const std::string& fingerprint);

// ---------------------------------------------------------------------
// Fingerprint'i kayit et (%LOCALAPPDATA%\MalaysiaKO\crash_fp.dat)
// Dosya format: <unixtime>\t<fingerprint>\n  (her satir)
// Eski (>24sa) kayitlar otomatik temizlenir.
// ---------------------------------------------------------------------
void RecordFingerprint(const std::string& fingerprint);

// ---------------------------------------------------------------------
// Dump dosyasi gecerli mi? (MDMP magic byte + 10KB-2MB)
// dumpPath: dosya yolu
// return: TRUE = gecerli MDMP, FALSE = bos/cop, SKIP et
// ---------------------------------------------------------------------
bool IsValidDump(const std::string& dumpPath);

} // namespace CrashFingerprint
