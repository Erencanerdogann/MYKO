# SRC Haritası — GameServer_SRC

**Tarih:** 2026-04-29 | **Hazırlayan:** CHIP | **Kaynak:** Gerçek dosya okuma

---

## Çözüm Dosyaları

| Dosya | Amaç |
|-------|------|
| `CodeGuardGameServer.sln` | Ana çözüm (Pearl Guard entegre) |
| `ByNoiseGameServer.sln` | Alt çözüm (standalone build) |

---

## Klasör Yapısı

```
C:\temp\MYKO\src\GameServer_SRC\
│
├── CodeGuardGameServer.sln          ← Ana çözüm
├── ByNoiseGameServer.sln            ← Alt çözüm
├── Hdd Unban.bat                    ← Yardımcı script
├── clean.bat                        ← Build temizlik
│
├── GameServer\                      ← 132 .cpp + 34 .h (oyun logic çekirdeği)
│   └── [detay: SRC_ONEMLI_CPP.md]
│
├── LogInServer\                     ← 7 .cpp (auth/login)
│   ├── main.cpp
│   ├── LoginServer.cpp / .h
│   ├── LoginHandler.cpp / .h
│   ├── LoginSession.cpp / .h
│   ├── GameSocket.cpp / .h
│   ├── DBProcess.cpp / .h
│   └── stdafx.cpp
│
├── shared\                          ← 232 MB ortak kütüphane
│   ├── database\                   ← 138 .h ORM şema (her .h = 1 DB tablo)
│   ├── signal_handler.h            ← SIGTERM/SIGINT hook
│   ├── CrashHandler.h              ← BugTrap entegrasyon
│   ├── Condition.h                 ← Thread senkronizasyon
│   └── ServerConfig.h              ← INI okuma
│
├── scripting\                       ← Lua betik dosyaları
│   └── [quest script, event script]
│
├── N3BASE\                          ← 3D shape/mesh kütüphane
│
├── libcurl\                         ← HTTP kütüphane (statik)
│
└── x64\Release\                     ← Build çıktı dizini
    ├── GameServer.exe               ← Ana çıktı (~3.4 MB)
    └── LogInServer.exe              ← Login çıktı (~496 KB)
```

---

## Modül Listesi

| Modül | Klasör | .cpp | .h | Amaç |
|-------|--------|------|----|------|
| GameServer | GameServer\ | 132 | 34 | Oyun logic çekirdeği |
| LogInServer | LogInServer\ | 7 | 5 | Auth / kimlik doğrulama |
| shared | shared\ | ~10 | 138+ | Ortak DB ORM + yardımcılar |
| scripting | scripting\ | - | - | Lua quest ve event |
| N3BASE | N3BASE\ | - | - | 3D yapı (read-only) |
| libcurl | libcurl\ | - | - | HTTP bağlantı (read-only) |

**Toplam GameServer klasörü:** 104,469 satır kaynak kod

---

## Database Header'ları (shared\database\)

- 138 adet `.h` dosyası
- Her `.h` = 1 DB tablosu ORM şeması
- MSSQL → ODBC → `ServerConfig.h` bağlantı string
- DSN adı: `CodeGuardMYKO_DB` (Türkçe karakter YASAK — bak: Patlama Dersleri)

---

## Versiyon Bilgisi

| Alan | Değer | Dosya |
|------|-------|-------|
| GAME_SOURCE_VERSION | 1098 | `Define.h:3` |
| Console başlık | "Knight Online Game Systems - v2369" | `main.cpp` |
| Mutex adı | `MYKO_GameServer_Mutex` | `main.cpp:15` |
| Bynoisee panel | v2369 string | `main.cpp` startup |
| Max level | 83 | `Define.h`, GameServer.ini |
| Max HP | 14000 | `Define.h` |
| Max Damage | 32000 | `Define.h` |

---

## Port Konfigürasyonu (GameServer.ini)

| INI Anahtarı | Varsayılan | Gerçek | Dosya |
|--------------|-----------|--------|-------|
| `[SETTINGS] PORT` | 15001 | — | GameServerDlg.cpp:677 |
| `[SETTINGS] LOGIN_PORT` | 15100 | **15100** | GameServerDlg.cpp:680 |
| `[SETTINGS] LOGIN_IP` | 127.0.0.1 | — | GameServerDlg.cpp:679 |

⚠️ **PATLAMA UYARISI:** GameServer.ini `LOGIN_PORT` ile LogInServer.ini `PORT` **AYNI OLMALI**.
Eski projede: GameServer=15200, LogInServer=15100 → silent death (boot oluyor, bağlantı yok).

---

## Build Sistemi

| Alan | Değer |
|------|-------|
| IDE | Visual Studio 2022 |
| Toolset | v143 (PlatformToolset) |
| Platform | x64 |
| Config | Release / Debug |
| Kütüphaneler | libcurl (statik), zlib, BugTrap, Lua 5.1 |
| Build komutu | `MSBuild ByNoiseGameServer.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -m` |

---

## Son 7 Gün Değişen Dosyalar (29 Nisan 2026)

Aktif geliştirme devam ediyor. Hangi dosyanın ne zaman değiştiğini görmek için:
```
git log --oneline --name-only -20
```

---

## ODBC DSN (Kritik)

| Alan | Doğru Değer |
|------|-------------|
| DSN adı | `CodeGuardMYKO_DB` |
| DB adı | `KO_MYKO` |
| Server | `localhost\MSSQLSERVER01` |
| Karakter | **ASCII only** (Türkçe İ, Ş, Ü YASAK) |

---

## İlgili Dosyalar

- Build detayı: `BUILD.md`
- Önemli CPP'ler: `SRC_ONEMLI_CPP.md`
- Anti-Cheat: `ANTI_CHEAT.md`
- Patlama dersleri: `PROJE_TARIHCESI_VE_DERSLER.md`
