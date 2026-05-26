// =========================================================================
// S115 FAZ 5 — Launcher Self-Heal Diagnostik
// Yazan: CHIP | Tarih: 2026-05-26
// =========================================================================
// AMAC: Launcher acilirken 5 onemli kontrol yapar, sorun varsa BILDIR + REPAIR butonu
//
// AKTIF MOD GUVENLIK (Cyberpunk launch dersi):
//   - Hash mismatch'te direkt indirmek YASAK -> insan onayli "Repair" butonu
//   - Backup yoksa SADECE BILDIR, otomatik degistirme yok
//   - Defender UAC prompt: kullanici Hayir derse Launcher devam (hata vermez)
//   - Tum aksiyonlar %LOCALAPPDATA%\MalaysiaKO\selfheal.log dosyasina yazilir
//
// FLAG: LAUNCHER.ini [SelfHeal] Enabled=1  -> 0 yapinca anlik kapanir
// =========================================================================
#pragma once

#include <string>
#include <vector>

namespace LauncherDiagnostic {

// ---------------------------------------------------------------------
// Diagnostik kontrol sonucu
// ---------------------------------------------------------------------
enum class CheckStatus {
    OK,             // ✅ sorun yok
    WARNING,        // ⚠ sorun var ama oyun acilabilir (uyari)
    ERROR_LEVEL,    // ❌ kritik sorun, oyun acilmayabilir (Repair butonu)
    DISABLED,       // SelfHeal flag kapali
    SKIPPED         // bu kontrol bu PC'de uygulanmiyor (eski Windows vs)
};

struct CheckResult {
    std::string name;          // "Defender Exclusion" vb
    CheckStatus status;
    std::string message;       // Kullaniciya gosterilen yazi
    std::string action;        // "Repair" / "Manual Fix" / "Continue"
    bool repairAvailable;      // TRUE = Repair butonu aktif
};

// ---------------------------------------------------------------------
// SelfHeal etkin mi? (LAUNCHER.ini [SelfHeal] Enabled=1)
// ---------------------------------------------------------------------
bool IsEnabled();

// ---------------------------------------------------------------------
// 5 Diagnostik fonksiyonu
// ---------------------------------------------------------------------

// 1. Defender exclusion eklenmis mi (HKLM\SOFTWARE\CodeGuard\PATH var mi)
CheckResult CheckDefenderExclusion(const std::string& gamePath);

// 2. Kritik dosyalar (KnightOnLine.exe + Launcher.exe + CODE) var ve boyutu makul mi
CheckResult CheckFileIntegrity(const std::string& gamePath);

// 3. Sunucuya HTTP baglanti acik mi (crash endpoint ping)
CheckResult CheckConnectivity(const std::string& serverIP);

// 4. VC++ Redistributable kurulu mu (registry kontrol)
CheckResult CheckVCRedist();

// 5. Server.ini var ve gecerli formata sahip mi
CheckResult CheckServerIni(const std::string& gamePath);

// ---------------------------------------------------------------------
// Tum kontrolleri tek seferde kos
// ---------------------------------------------------------------------
std::vector<CheckResult> RunAllChecks(const std::string& gamePath, const std::string& serverIP);

// ---------------------------------------------------------------------
// Repair fonksiyonlari (insan onayli — Launcher UI'dan tetiklenir)
// ---------------------------------------------------------------------

// Defender exclusion eklemek icin PowerShell calistir (UAC isteyebilir)
// return: TRUE = basarili, FALSE = kullanici reddetti veya hata
bool RepairAddDefenderExclusion(const std::string& gamePath);

// VC++ Redistributable indir+kur (eger Server'da VC++ link varsa)
bool RepairInstallVCRedist();

// Server.ini sifirla (default degerlerle yeniden olustur)
bool RepairServerIni(const std::string& gamePath);

// ---------------------------------------------------------------------
// Log fonksiyonu (selfheal.log dosyasina yazar)
// ---------------------------------------------------------------------
void LogAction(const std::string& action, const std::string& result);

// ---------------------------------------------------------------------
// FAZ 5c: Diagnostik Dialog'u goster (modal popup)
// hwndParent: Launcher ana penceresi
// gamePath: Game klasor yolu
// serverIP: Server.ini IP0 degeri
// ---------------------------------------------------------------------
void ShowDiagnosticDialog(HWND hwndParent, const std::string& gamePath, const std::string& serverIP);

// ---------------------------------------------------------------------
// S115 v2.7+ C plani: BASIT Self-heal Dialog (1 buton ONAR)
// Onceden RunAllChecks calistirilmis, problems listesi hazir gelir.
// Kullanici ONAR'a basinca hangi sorun varsa o repair'lar sirayla cagrilir.
// ---------------------------------------------------------------------
void ShowSimpleDiagnosticDialog(HWND hwndParent,
                                const std::string& gamePath,
                                const std::vector<CheckResult>& results);

} // namespace LauncherDiagnostic
