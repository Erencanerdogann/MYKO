// ============================================================================
// DiagLog.h — MERKEZI CLIENT TESHIS LOG SISTEMI (2026-06-12, patron istegi)
// ----------------------------------------------------------------------------
// AMAC: Client'ta olan TUM hatalari (donma/crash/DC/D3D) tek dosyada gor.
//   Client KnightOnline.exe PACKED + source bizde YOK -> kendi logic'ini goremeyiz.
//   AMA code.guard client'a enjekte -> hook'lu kapilari (D3D/input/net/exception)
//   loglayabiliriz. Bu dosya o kapilari TEK MERKEZE toplar.
//
// AC/KAPA: code.guard yanindaki Option.ini -> [Diag] Enabled=1 (test PC) / =0 (prod).
//   Kapaliyken DIAG() no-op (sifir maliyet). g_DiagEnabled ilk cagrida INI'den okunur.
//
// DOSYA: C:\MalaysiaKO\diag.log (client klasoru — C:\ koku UAC YASAK, feedback dersi).
//   INI'de [Diag] Path= ile degistirilebilir.
//
// KATEGORILER: CRASH (sart) / FREEZE / D3D / WND / NET / DC / GUARD / MRX / INFO
// GERI-ACMA: production'da Enabled=0 yeter; kod kalir (KURAL 0-B).
// ============================================================================
#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>

// -1 = henuz okunmadi, 0 = kapali, 1 = acik
extern int  g_DiagEnabled;
extern char g_DiagPath[MAX_PATH];

// INI'den oku (ilk DIAG cagrisinda 1 kez). Option.ini code.guard ile ayni klasorde.
static inline void DiagLoadConfig()
{
	char iniPath[MAX_PATH] = { 0 };
	// code.guard'in yuklu oldugu modul klasoru -> Option.ini ara (yoksa client koku)
	GetModuleFileNameA(NULL, iniPath, MAX_PATH); // host exe (KnightOnline.exe) yolu
	char* slash = strrchr(iniPath, '\\');
	if (slash) *(slash + 1) = '\0';
	else strcpy(iniPath, ".\\");

	char iniFull[MAX_PATH];
	sprintf(iniFull, "%sOption.ini", iniPath);

	g_DiagEnabled = GetPrivateProfileIntA("Diag", "Enabled", 0, iniFull); // default KAPALI

	char defPath[MAX_PATH];
	sprintf(defPath, "%sdiag.log", iniPath); // default: client klasoru\diag.log
	GetPrivateProfileStringA("Diag", "Path", defPath, g_DiagPath, MAX_PATH, iniFull);
}

// Merkezi log fonksiyonu. Kapaliyken aninda doner (no-op).
static inline void DIAG(const char* kategori, const char* fmt, ...)
{
	if (g_DiagEnabled == -1)
		DiagLoadConfig();
	if (g_DiagEnabled <= 0)
		return;

	char body[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(body, sizeof(body) - 1, fmt, ap);
	va_end(ap);
	body[sizeof(body) - 1] = '\0';

	SYSTEMTIME st;
	GetLocalTime(&st);

	FILE* f = fopen(g_DiagPath, "a");
	if (!f) return;
	fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] [%s] %s\n",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		kategori, body);
	fclose(f);
}

// ---- CRASH yakalama: SetUnhandledExceptionFilter ile son-care -------------
// Client cokerse cokme adresi + exception kodu + modul yazilir. Acilista
// kapanma sorununun kok kaniti BURADA cikar. Filtre CONTINUE_SEARCH doner ->
// BugTrap/varsa diger handler'lar da calisir (zincir bozulmaz).
LONG WINAPI DiagUnhandledFilter(EXCEPTION_POINTERS* ep);

// DllMain/Init'te 1 kez kurulur.
static inline void DiagInstallCrashHandler()
{
	SetUnhandledExceptionFilter(DiagUnhandledFilter);
}
