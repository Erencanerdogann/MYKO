#pragma once
#include "stdafx.h"
#include "ftpclient/FTPClient.h"
#include "zlib\zip.h"
#include "APISocket.h"
#include "hdr.h"
#include <format>
#include <winhttp.h>  // S115 AUTO-UPDATE: INTERNET_PORT typedef
#include "LauncherDiagnostic.h"  // S115 v2.7+ C plani: m_diagResults cache

// S115 AUTO-UPDATE: Bu Launcher'in build versiyonu. Her yeni build'de ARTIR.
// Sunucudaki version.txt'de bu degerden buyuk varsa, Launcher otomatik gunceller.
// v2.5: FAZ 4 — Crash fingerprint + duplicate suppression + whitelist filtre
// v2.6: FAZ 5c fix — encoding + Defender gercek sorgu + CODE opsiyonel + Sleep 8000 + timeout 3/3/5/5
// v2.7: ACIL FIX — Self-heal AutoRun DEFAULT KAPALI (Launcher.ini [SelfHeal] AutoRun=1 ile aktif)
//       Sebep: 3 paralel HTTP thread (HwidA + CheckForUpdate + Self-heal) WinSock race -> Connection failed
//       Cozum: Self-heal manuel butona kalir, otomatik kosmaz
// v2.8: B FIX — CheckForUpdate Constructor'dan Start() icine tasindi (5sn delay)
//       + C plani — Self-heal basit Dialog (1 buton ONAR) connect() fail tetikli
//       Patron lokal test PASS, sunucu deploy 2026-05-27
// v2.9: MD5-kor guncelleme bugu fix — versiyon esit + icerik (MD5) farkliysa yine indir.
//       (MalaysiaKO2 debug 2.8 vs sunucu release 2.8 ayni versiyon farkli MD5 -> cekmiyordu)
// v3.0: IP BANNED GIF — ayri ban ekrani (loading_banned.gif), 0x84 ban sorgusuna bagli.
// v3.1 (S127 TODO#241): multi-client mutex kaldirildi + TCP connect 5sn timeout (1dk donma fix)
//       + Defender/TBL-hash/cheat-scan async (acilis hizi). Auto-update tetigi icin bump.
// v3.2 (S127 TODO#241): FIX-A version-wait timeout+retry+Sleep (Checking sonsuz donma+CPU spin),
//       FIX-B connect 3 retry + 'onar' dialogu sustur, FIX-C crash upload timeout 16->7sn.
// v3.3 (S127 TODO#241): D1 auto-update inerken START engelle (m_bUpdating gate) + D2 uyari state
//       ('guncelleme iniyor, bekleyin') -> oyuncu update-restart cakismasi yasamasin.
// v3.4 (S127 TODO#241): FIX-E Defender exclusion blogu KALDIRILDI (launcher'dan) — setup.exe
//       zaten yapiyor + launcher'da ise yaramiyordu (oyuncu vakasi: AV exe sildi) + UAC/blocking
//       riski. TBL hash + cheat scan guvenlik kontrolleri KALDI. + faz-sonu H1 olu-kod temizligi.
// v3.5 (S127 TODO#241): FIX-G GDIHelper GIF crash use-after-free+OOB guard (MATRIX kanit:
//       launcher.exe+0xb514 0xc0000005 x5 TEK gercek crash) + FIX-H START update-check gate
//       (m_bUpdateChecked) + FIX-I crash dump upload sonrasi sil.
// v3.6 (S127 TODO#241): FIX-H GERI ALINDI — IsUpdateChecked() START gate REGRESYON yaratti.
//       START tekrar D1 davranisi: IsReady() && !IsUpdating(). FIX-G + FIX-I KALDI.
// v3.7 (S127 TODO#241): FIX-G GERI ALINDI — GDIHelper::run() GIF guard OYUNU ACTIRMIYORDU.
//       GDIHelper v3.4 haline geri alindi. Sadece FIX-I (crash dump sil) KALDI.
// v3.8 (S127 TODO#241): GIF crash DOGRU COZUM — DB kanit launcher.exe 0xc0000005 GDIHelper::run x6
//       (use-after-free RACE). KOK: Destroy() ONCE m_pImage siliyordu SONRA isPlayable=FALSE ->
//       run thread silinmis pointer'a erisiyordu. FIX: Destroy() SIRASI duzeltildi (once durdur+
//       Sleep(120)+sonra sil) + run()'da sleep-sonrasi TEK SATIR guard (m_pImage NULL break).
//       run mantigi DEGISMEDI (FIX-G hatasi=run kurcalamak, bu sefer minimal). SAFE GIF akisi korundu.
// v3.9 (S127 TODO#241): SADECE TEMIZLIK — FIX-H olu kodu (m_bUpdateChecked flag/getter/set/yorum)
//       silindi (v3.6'da geri alinmisti, kalinti kalmisti). DAVRANIS v3.8 ile AYNI, kod hatasiz/temiz.
// v4.0 (S131): L1 DownloadFile donus kontrolu + L2 FTP timeout 1800sn (asili indirme sonsuz takilma fix).
// v4.1 (S131): MRX Makro tespiti — sahte svchost (>1MB) + interception keyboard.sys/mouse.sys dosya+service.
//       NOT: version artirildi (4.0->4.1), yoksa gomulu=4.0 vs version.txt=4.1 -> sonsuz indirme dongusu.
// v4.2 (2026-06-12): FP FIX — interception driver kontrolu (keyboard.sys/mouse.sys dosya + keyboard/mouse
//       service) YORUM SATIRINA alindi (patron karari: interception/mouse-fix'e izin). Boran vakasi:
//       makro KULLANMADIGI halde interception driver kalintisi (mouse-fix/Interaccel) yuzunden giremedi.
//       MRX tespiti process (MRX.exe) + sahte svchost>1MB ile devam. version bump (yoksa sonsuz indirme).
// v4.3 (2026-06-12): UX FIX — auto-update Sleep 5000->1500 (ilk saniyelerde guncelleme inmeden START'a
//       basilabiliyordu, cakisma penceresi 5sn->1.5sn) + 'Guncellemeler kontrol ediliyor...' yazisi
//       (patron: 5sn dondu saniliyor). START KILITLENMEDI (FIX-H sonsuz kilit regresyonu tekrarlanmaz).
#define LAUNCHER_BUILD_VERSION_MAJOR 4
#define LAUNCHER_BUILD_VERSION_MINOR 3

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
	// v2.5 FAZ 4: fingerprint + module + exception + offset parametreleri eklendi
	static void InstallCrashHandler();
	static bool UploadCrashDump(const std::string& dumpPath,
	                            const std::string& exeName,
	                            const std::string& account = "",
	                            const std::string& version = "",
	                            const std::string& fingerprint = "",   // S115 v2.5
	                            const std::string& moduleName = "",    // S115 v2.5
	                            const std::string& exceptionCode = "", // S115 v2.5
	                            const std::string& crashOffset = "");  // S115 v2.5
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
	// TODO#241 D1 (S127 v3.3): auto-update (v->yeni exe) inerken START engelle.
	// CheckForUpdate gercekten indirme yapacaginda true; fail/bitince false. UI thread okur.
	bool IsUpdating() { return m_bUpdating; }
	volatile bool m_bUpdating = false;
	// NOT (faz-sonu H1): IsUpdateChecked()/m_bUpdateChecked (FIX-H) v3.6'da GERI ALINDI
	// (sonsuz kilit regresyonu) -> olu kod, kaldirildi. START gate = IsReady && !IsUpdating.
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

	// S115 v2.7+ C plani: Self-heal diag sonuclari (Start() icinde dolar, WM_USER+101 okur)
	std::vector<LauncherDiagnostic::CheckResult> m_diagResults;
private:
	short m_iVersion;
	short m_settingsVersion;
	std::string m_stateString;
	
	uint8 m_dPercent;
	
};

extern Launcher* Engine;