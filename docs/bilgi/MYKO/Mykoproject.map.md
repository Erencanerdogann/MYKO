# MYKO Project Haritası — Bynoisee MalaysiaKO

**Tarih:** 2026-04-29
**Hazırlayan:** DOKTOR (4 paralel ajan ile lokal sistem taraması)
**Hedef:** Projedeki tüm GAME / DB / SRC / WEB / TOOLS dosya yollarının tek dosyada haritası + bulgular.
**Sunucu lansman:** 08 Mayıs 2026 (Valor)
**Versiyon:** **2369 base + 1098 patch** (Source: `GAME_SOURCE_VERSION 1098`, Console: "Knight Online Game Systems - v2369")

---

## ⚡ HIZLI EKOSİSTEM ÖZET

```
                    ┌─────────────────────────────────────────────┐
                    │           BYNOISEE MalaysiaKO               │
                    └─────────────────────────────────────────────┘
                                       │
        ┌──────────────────────────────┼──────────────────────────────┐
        │                              │                              │
   ┌────▼─────┐                  ┌─────▼─────┐                  ┌─────▼──────┐
   │  ONLINE  │                  │  OFFLINE  │                  │  ARSENAL   │
   │ (Prod.)  │                  │  (PC)     │                  │  (Tools)   │
   └──────────┘                  └───────────┘                  └────────────┘

  104.238.23.99                  C: drive lokal              src/AntiCheat/Tools
```

---

## 📦 GAME — Oyun Motoru / Server / Client

### 🌐 ONLINE (Production)

#### User Client → `C:\MalaysiaKO`
- **5.0 GB** total, oyuncuların indirdiği client
- **Ana exe'ler:**
  - `KnightOnline.exe` (15 MB) — Ana oyun client
  - `Launcher.exe` (4.8 MB)
  - `Option.exe` (332 KB) — grafik ayar
  - `KscViewer.exe` — KSC log viewer
- **Anti-cheat / Engine:**
  - `code.guard` (6.0 MB) — anti-cheat DLL (3 versiyon: code/4code/5code)
  - `KOFPL.dll` (3.8 MB) — fizik/gameplay motoru
  - `Apr_Show.dll` (456 KB) — UI/grafik
- **Asset klasörleri:**
  - `Data/` — 246 .tbl (Item, Skill, NPC, Quest, Achievement) — **36 MB**
  - `Item/` — item.src + item.hdr — **540 MB**
  - `Object/` — 3D objeler — **302 MB**
  - `UI/` — ui.src + ui.hdr — **2.9 GB** (en büyük)
  - `Chr/` — karakter modelleri (.n3chr, .n3anim) — **124 MB**
  - `Zones/` — .mob spawn, .code script, .opd terrain — **708 MB**
  - `Snd/` — Ogg Vorbis ses — **49 MB**
  - `CodeGuard/Code/` — şifreli UI/script kodları (re_*, macho_*, co_*, El_*, Ka_*) — **3.7 MB**
- **Config:**
  - `Server.ini` → `IP=104.238.23.99:15100`, **XignCode=0**, version=2377
  - `Option.ini` → 2.1 KB grafik/UI/login
  - `Path.Ini`, `Scheduler.ini` (4743 satır war scheduler)
- **Loglar:**
  - `cg_crash.log` → "MAINTENANCE MODE ACTIVE", browser auto-open: malaysiako.com
  - `log.klg` (116 KB) → KSC binary/encrypted

#### Production Server → `104.238.23.99` (Hostabil VPS)
- **GameServer + LoginServer** (uzak)
- Port 15001 (Game), 15100 (Login)
- Detay: `Desktop\Server` lokal yansımasıdır (aşağıda OFFLINE)

---

### 💻 OFFLINE (Senin PC'nde)

#### Çalışan Test Server → `C:\Users\erenc\Desktop\Server` ⭐ **ASIL TEST ORTAMI**
- **203 MB** total — DB bağlı, fonksiyonel
- **Aktif exe'ler:**
  - `GameServer.exe` (3.4 MB, 28 Mart 2026)
  - `LogInServer.exe` (496 KB, 28 Mart 2026)
  - **AIServer YOK** → 2-tier yapı (NPC AI Lua'da)
  - `code.guard` (5.9 MB) — Code Guard anticheat
- **Backup zinciri:**
  - `BACKUP_04_MART_2026/`, `BACKUP_10_MART_2026/`, `backup_28mart/`
  - `GameServer.exe.bak.22mart`, `.bak.26mart` → patch geçişleri
  - `LogInServer.exe.broken_28mart` (502 KB) → test artifact
- **Patch zip arşivi:** `patch/2370.zip`, `2371.zip`, `2372.zip`, `2373.zip` (12 MB)
- **2373_extracted/** → KnightOnline.exe + code.guard + CodeGuard/Code/ (100+ .code)
- **Klasörler:**
  - `Logs/` → 76 dosya, son: 18 Nisan 2026 19:15 (LoginServer boot test)
  - `Map/` → **82 dosya** (61 .smd + 14 .aievt)
    - **11 zone "1098" prefix:** `1098elmo2004.smd`, `1098karus2004.smd`, `1098moradon_0826.smd`, `1098war_a.smd`, `1098freezone_a/b/c.smd`, `1098In_dungeon01-03.smd`
    - Diğer: BattleZone, War_a, dungeon serisi, Code_Moradon_war, 14th_oldmoradon, 2017_flagwar
  - `Quests/` → **511 Lua dosyası** (NPC-bazlı: 11051_Sphie, 14204_Minerva, 13016_Keite, 11810_Helena vb.)
  - `DXT & UIE EDİTÖR/` → tool (UIE.exe, N3TexViewerPNG.exe)
- **Config (`GameServer.ini`):**
  - `[ODBC]` → 3 DSN: `CodeGuardMYKO_DB` (account), `CodeGuardMYKO_GAME`, `CodeGuard_LOG`
  - `UID=sa`, `PWD=password` (REDACTED — lokal dev)
  - `[SETTINGS]` → `GAMEMAXLEVEL=72`, `LOGIN_IP=127.0.0.1`, `PORT=15001`
  - `packet_rate_limit=500` (26 Mart'ta 300'den optimize)
- **Config (`LoginServer.ini`):**
  - `[DOWNLOAD]` → patch URL: `104.238.23.99`
  - `TITLE_01="1098 Myko Server"` ← **PATCH MARKER**
  - Port 15100
- **Helper scripts:**
  - `KO_ServerPanel.bat` (24 KB), `start_servers.bat`, `start_gs.bat`, `start_ls.bat`, `status.bat`, `myko_status.ps1`, `server-agent.ps1`
- **Notice metinleri:** "MalaysiaKo - Level Cap 65 - Powered By Code Guard"
- **CapeBonus.txt:** 300HP/150MP/3AP/+5NP
- **ClanPremiumNotice.txt:** Exp+30%, NP+8, Drop+1%, Noah+30%, SellPercent+50%
- **censor_words.txt** (30 kelime — TR + EN spam filter)
- **DURUM:** Aktif EXE 28 Mart, son log 18 Nisan → **şu an dormant**, kod stabil

#### Dev Client → `C:\temp\MYKO\DEV_CLIENT`
- Yapı `C:\MalaysiaKO` ile **birebir aynı** (test/geliştirici sürümü)
- `Server.ini`: IP=`104.238.23.99:15100`, version=**2377→2373**
- **`NEW_CLIENT/`** alt klasör — temizleme/yeni build hazırlığı (cg_crash.log yok, .bak yok)
- Son güncelleme: 28 Nisan 2026 → aktif test
- Launcher + KnightOnline.exe stable: Mart 2026

#### Lokal Client (deneme) → `C:\MalaysiaKOLOCAL`
- ⚠️ İsim "LOCAL" ama içerik **CLIENT** (server değil!)
- KnightOnline.exe, Launcher, KOFPL.dll, Apr_Show.dll, CodeGuard/, Data/, Item/, Object/, Chr/
- **Bırakıldı (patron talimatı: "o klasörü bırak")**

---

## 🗄️ DB — Veritabanı

### ONLINE
- **Sunucu:** `localhost\MSSQLSERVER01` (104.238.23.99 üzerinde)
- **DB:** `KO_MYKO`
- **User:** `sa` (CLAUDE.md'den)
- **Port:** 7354 (named instance)

### OFFLINE
- **Lokal MSSQL:** `127.0.0.1` (Desktop\Server bağlanır)
- **3 ODBC DSN:**
  - `CodeGuardMYKO_DB` → Account DB
  - `CodeGuardMYKO_GAME` → Game data (item/quest)
  - `CodeGuard_LOG` → Audit log
- **MDF/LDF lokasyonu:** `C:\temp\myko_db/`
  - `MYKO_LOCAL.mdf` + `MYKO_LOCAL_log.ldf`
  - `MYKO_LOCAL_LOG.mdf` + `MYKO_LOCAL_LOG_log.ldf` (duplicate log DB)
- **Eski yedekler:** `21_07_2024_DB.bak`, `21_07_2024_LOG.bak`

### BACKUP (server\myko_server\DB_BACKUP_ORIGINALS)
- `KO_MYKO_2026-03-19.bak` (252 MB)
- `MYKO_LOCAL_2026-03-19.bak` (252 MB)
- **6 stored procedure (orijinal):**
  - `LOAD_USER_DATA_ORIGINAL.sql`
  - `CHARACTER_LOGIN_CHECKS_ORIGINAL.sql`
  - `SET_LOGIN_INFO_ORIGINAL.sql`
  - `SKILLSHORTCUT_SAVE_ORIGINAL.sql`
  - `UPDATE_SAVED_MAGIC_ORIGINAL.sql`
  - `UPDATE_USER_DATA_ORIGINAL.sql`
  - `CREATE_NEW_CHAR_ORIGINAL.sql`

### Bilinen tablolar (log + SP'lerden çıkarsanan)
- `ACCOUNT_CHAR` — character selection
- `USERDATA` — Level, Exp, Gold, Zone, Pos, Skills, Items, Authority
- `CHECK_ACCOUNT` — login state
- `USER_ACHIEVE_LOAD_DATA` — achievement
- `QUEST_SKILLS_CLOSED_DATA` — quest tracking
- `USER_ITEM` (tekil) ≠ `USER_ITEMS` (boş)
- `ITEM_SELLTABLE` — PK: iSellingGroup+nIndex
- `GAME_MASTER_SETTINGS` — GM yetki
- `VERSION` — patch version
- `TB_USER` — web hesap (`strWebHash` NULL bug var)

---

## 💻 SRC — Source Code

### Ana Source → `C:\temp\MYKO\src` (Lead src)
- **Dil:** C++ %100 (Visual Studio 2022, x64)
- **Toplam:** 471 C++ dosyası, **3 modül**

#### 1. GameServer_SRC (690 MB, 132 .cpp + ~150 .h)
- **Çözümler:** `CodeGuardGameServer.sln` (ana), `ByNoiseGameServer.sln` (alt)
- **Versiyon sabit:** `Define.h` → `#define GAME_SOURCE_VERSION 1098` (alternatifler: 1098, 1534, 2369)
- **Console title:** `"Knight Online Game Systems - v2369"`
- **Mutex:** `MYKO_GameServer_Mutex` (tek instance)
- **Önemli source dosyaları:**
  - `User.cpp` (5210 satır) — oyuncu çekirdek logic
  - `MagicInstance.cpp` (6230 satır) — büyü sistemi
  - `MagicProcess.cpp` (1525 satır) — büyü işleme
  - `ChatHandler.cpp` (90 KB) — chat
  - `GameServerDlg.cpp` (96 KB) — ana event loop
  - `AttackHandler.cpp`, `BattleSystem.cpp` — savaş
  - `ItemHandler.cpp`, `ItemUpgradeSystem.cpp`, `ItemSmashSystem.cpp` — item
  - `QuestHandler.cpp`, `QuestDatabase.cpp`, `DailyQuest.cpp` — quest
  - `Knights.cpp`, `KnightsManager.cpp`, `CastleSiegeWar.cpp` — klan
  - `Map.cpp`, `Region.cpp`, `PathFind.cpp` — map
  - `Npc.cpp`, `NpcThread.cpp`, `BossHandler.cpp` — NPC
  - `LuaEngine.cpp`, `lua_bindings.cpp` — Lua scripting
  - `XGuard.cpp` — anti-cheat hook
  - `BotHandler.cpp`, `offlinemerchant.cpp` — bot/offline merchant
  - `EventMainSystem.cpp`, `BeefEventNew.cpp` — event sistemi
  - `ArenaHandler.cpp`, `TournamentSystem.cpp`, `NewRankingSystem.cpp` — PvP/arena
  - `GenieHandler.cpp`, `PetMainHandler.cpp`, `RentalHandler.cpp`, `SealHandler.cpp` — sistemler
  - `GenderJobChangeHandler.cpp`, `NationTransferHandler.cpp`
- **Database katmanı:** `shared/database/` → 138 .h ORM-style schema
- **Aktif geliştirme:** son 7 günde 24 dosya değişti (Apr 25-29)

#### 2. AntiCheat_SRC / Pearl Guard (1.3 GB, 203 .cpp+.h)
- **Çözüm:** `CodeGuardAnticheat.sln`
- **Bileşenler:**
  - **CODE Cli.710F3AD0** — kod obfuskasyonu
  - **DetourAPI 3.0** (Microsoft Detours) — API hook
  - **Virtualizer/** — kod virtualization
  - **discord-rpc-master** — Discord status
  - **RC5/** — RC5 cipher
  - **N3BASE** — 3D yapılar (kopya)
  - **SDL2** include
- **GameServer'ın AntiCheat aynası** (CastleSiegeWar.cpp, Cindirella.cpp vb.)

#### 3. 3Launcher (59 MB, 94 C++ dosyası)
- **Adı:** `ISTIRAP-Launcher-Source` (README'den)
- **Bağımlılıklar:** FTP client, zlib, libcurl
- **Klasörler:** Launcher/, ftpclient/, curl/, libs/, zlib/, Win32/

#### Build
```
cd C:\temp\MYKO\src\GameServer_SRC
# Visual Studio 2022 ile CodeGuardGameServer.sln aç + x64 Release build
# Output: GameServer.exe → Desktop\Server\GameServer.exe replace
```

⚠️ **Git yok** — leak edilmiş kod, version control ile takip edilmiyor (lokal .bak chain ile yönetiliyor).

---

## 🛠️ TOOLS — Yardımcı araçlar

### `C:\temp\MYKO\tools` — Python toolchain
- **`key_rotation/`** — şifreleme key yenileme
  - `key_generator.py` — 6 sistem için key üretir (JvCryption, RC4 MYKO, RC4 NTF, K2 XOR, K1 XOR, DES s_secret1)
  - `rc4_re_encrypt.py` — .code dosyaları yeniden şifrele
  - `tbl_re_encrypt.py` — .tbl dosyaları yeniden şifrele
  - `exe_key_patcher.py` — Client EXE'deki key'leri patch et
  - `src_encrypt.py` — Source kod şifreleme
  - `MYKO_NEW_KEYS.md` (2026-03-23) — aktif/eski key'ler **(REDACTED)**
- **`tbl/`** — TBL editörler
  - `tbl_decrypt.py`, `tbl_edit.py`, `tbl_edit_v2.py`
  - `tbl_compare.py`, `tbl_fix_*.py`, `tbl_scan_all.py`
- **`Uif-Decryptor/`** + **`Uif-Encryptor/`** — C++ Visual Studio projeleri (.xcurse → .uif, RC4 + xorstr obf)
- **`setup/`** — Inno Setup installer (`myko_setup_v3.iss` — Türkçe, v3.0 Early Access, hedef `C:\MalaysiaKO`)
- **Diğer:**
  - `patch_tool.py` — SSH + DB kayıt → otomatik deploy
  - `build_xlsx.py` — XLSX builder
  - `item_search.py` — item arama

### `C:\temp\MYKO\EncryptClient`
- KnightOnline.exe (15.2 MB) + DLL'ler — şifrelenmiş çıktı
- 6 katmanlı şifreleme: JvCryption (paket), RC4 MYKO (.code), RC4 NTF (texture), DES Feistel + K2 XOR (.tbl), XOR chain
- Veri klasörleri: Data/, Zones/, UI/, Object/, Snd/, vb.
- Config: Option.ini, Server.ini, Path.ini, Scheduler.ini, log.klg

### `C:\temp\MYKO\PearlMonitor` — Real-time debug izleme
- **PearlMonitor.exe** (164 KB) — bağımsız konsol app
- **PearlMonitor.cpp** (1159 satır) — Windows API + Named Pipe client
- **PearlPipe.h** — Pearl Guard DLL'e include edilen header
- **Pipe:** `\\.\pipe\PearlMonitor`
- **İzlediği:**
  - Paketler (SEND/RECV opcode + isim)
  - Item events (Use/Get/Drop/Trade/Upgrade/Move)
  - Skill (kullanım/alınma)
  - Stat (HP/MP/EXP/Gold/Level/SP)
  - Combat (Attack/Dead/Regen)
  - Security (CRC/Process scan/Hack)
  - Sistem (Zone/Chat/Merchant/Party/Clan/Warehouse/Quest)
- **Log:** `pearl_monitor.log` (153 KB)
- **Zero-cost:** Monitor yoksa DLL hiç işlem yapmaz

---

## 🌐 WEB

### Production
- **Web (PHP):** `104.238.23.99:8091` (PHP nginx) → `koweb2`
- **Site API (Rust):** `104.238.23.99:3001` → `orkestra-server.exe` (`/api/site/register`, `/login`, `/online`, `/rankings`, `/server-status`, `/health`)
- **Patch Server:** `104.238.23.99:80` → `patch_server.js` (Node.js, KO client patch dağıtım)
- **Forum (Flarum):** MariaDB:3307 `flarum_db`, nginx `/forum/`, eklenti sıfır, SSO yok

### Bilinen Bug'lar (memory)
- `strWebHash NULL` → Auto-register hesaplar siteye giremez (TB_USER tablosunda)
- TLS yok → MITM açığı (uzun vade fix)

---

## 🛡️ ORKESTRA-AI (Ayrı — Sistem, Oyun değil)

### `C:\temp\MYKO\orkestra-rs` — Bizim agent kontrol sistemi
- **Dil:** Rust 2024, Cargo workspace, 8 crate, 193 .rs dosyası
- **Bileşenler:**
  - `orkestra-core/` — shared types
  - `orkestra-db/` — SQLite WAL + FTS5
  - `orkestra-cli/` — `orkestra.exe` (50+ subcommand)
  - `orkestra-a2a/` — Axum HTTP (port 3010) + LLM bridge + MSSQL site API
  - `orkestra-mcp/` — Claude Code MCP entegrasyonu
  - `orkestra-service/` — Windows Service + watchdog
  - `orkestra-gui/` — Tauri "Amiral" dashboard
  - `myko-panel/` — GM panel (Tauri+Rust+TS)
- **Aktif:** son 7 gün 305 commit, son 24 saat 94 commit
- **Deploy edilmiş runtime:** `C:\orkestra\` (orkestra.exe, orkestra-service.exe, orkestra-mcp.exe, orkestra-panel.exe, amiral.exe, myko-panel.exe)

⚠️ **NOT:** Orkestra-AI **MYKO oyunuyla ilgili değil** — bizim agent koordinasyon sistemi. Bu projede ayrı bir araç. Karıştırmayalım.

---

## 🔄 GELİŞTİRME AKIŞI (Pipeline)

```
[1. KOD]
src/GameServer_SRC/ değiştir
  ↓ Visual Studio 2022 build (x64 Release)
  ↓ GameServer.exe çıktısı

[2. LOKAL TEST]
Desktop\Server\GameServer.exe replace
  ↓ start_gs.bat + start_ls.bat
  ↓ DEV_CLIENT'ten test

[3. ŞİFRELEME (gerekirse)]
tools/key_rotation/ → yeni key üret
  ↓ rc4_re_encrypt + tbl_re_encrypt + exe_key_patcher
  ↓ EncryptClient/ output

[4. PATCH HAZIRLIK]
Patch zip oluştur → patch/2374.zip (örn)
  ↓ 2374_extracted/ test

[5. DEPLOY]
tools/patch_tool.py SSH → 104.238.23.99
  ↓ MSSQL VERSION INSERT
  ↓ taskkill /IM GameServer.exe /F
  ↓ wmic process call create yeni exe
  ↓ Reboot sonrası bekle (SSH timing)

[6. CLIENT GÜNCELLEME]
patch_server.js (port 80) yeni version dağıtır
  ↓ User Client (C:\MalaysiaKO) otomatik download
```

---

## 📊 RAKAMSAL ÖZET

| Bileşen | Boyut | Dosya |
|---------|-------|-------|
| User Client (`C:\MalaysiaKO`) | 5.0 GB | 246 .tbl + 2.9 GB UI + 708 MB Zones |
| Lokal Test Server (`Desktop\Server`) | 203 MB | 82 map + 511 quest + 76 log |
| Lead Source (`src/`) | ~2.0 GB | 471 .cpp + 1.3 GB AntiCheat + 59 MB Launcher |
| Tools (`tools/`) | ~MB | Python + C++ projects |
| EncryptClient | 5+ GB | Şifreli client çıktı |
| PearlMonitor | <1 MB | exe + source |
| DEV_CLIENT | 5+ GB | Geliştirici client |

---

## 🎯 ÖNEMLİ TESPİTLER

### 1. Versiyon Stratejisi Doğrulandı
- **Source (`Define.h`):** `GAME_SOURCE_VERSION 1098`
- **Console:** "v2369"
- **Map dosyaları:** 11 dosya `1098` prefix
- **LoginServer.ini TITLE:** "1098 Myko Server"
- → **2369 base + 1098 patch giydirme** ✅ (CLAUDE.md uyumlu)

### 2. Mimari
- **2-tier yapı** (GameServer + LoginServer, AIServer yok)
- NPC AI Lua'da gömülü
- Direct ODBC MSSQL (DBServer yok)
- 1013 + 511 = **1500+ Lua quest** (mature content)

### 3. Anti-Cheat: Code Guard / Pearl Guard
- DLL: `code.guard` (5.9 MB)
- Source: `AntiCheat_SRC/` (1.3 GB)
- Detours hook + Virtualizer obfuscation + RC5 cipher
- Discord RPC entegre
- 6-layer encryption (JvCryption + RC4 + DES + XOR)

### 4. Aktif Geliştirme
- src/ son 24 saat 24 dosya
- Server son aktif: 28 Mart EXE, 18 Nisan log
- Lansman: **08 Mayıs 2026** (Valor)
- Şu an: backup mode, kod stabil, lansman hazırlığı

### 5. Güvenlik Notları (lokal dev — production öncesi düzeltilmeli)
- ⚠️ INI'lerde plaintext SA password (`PWD=password`)
- ⚠️ Test account izleri (Lv72, 1.5M gold, "tenger")
- ⚠️ "Notice.txt: Deneme1" → test artifact
- ⚠️ `LogInServer.exe.broken_28mart` → test rollback artifact

---

## 📝 EKSİK / SONRAKİ İŞ

1. **WEB lokal kaynak (`C:\koweb2`)** — taranmadı, gerekirse ayrı tarama
2. **`C:\temp\MYKO\BynoiseeMonitor`** — taranmadı (PearlMonitor benzeri olabilir)
3. **`C:\temp\MYKO\sunucu-aktarim`** — taranmadı
4. **`C:\temp\MYKO\kaynak`** — taranmadı (orijinal MGAME source mu?)
5. **`C:\temp\MYKO\test_k19*.exe/.rs`** — kök dizinde test dosyaları (ne için?)
6. **`C:\temp\MYKO\PLANLAR/`** — proje planları
7. **`C:\temp\MYKO\MD_RAPORLAR/`** — agent raporları
8. **`C:\temp\MYKO\docs/`** — dokümantasyon (bu MD'nin lokasyonu)

→ Patron isterse bu eksikler de taranabilir.

---

**MD sürümü:** v1.0
**İlk yazım:** 2026-04-29 (DOKTOR + 4 paralel ajan)
**Sonraki güncelleme:** Faz 2 — Knight Online oyun bilgi MD'leri yazıldıkça bu dosya da büyür.
