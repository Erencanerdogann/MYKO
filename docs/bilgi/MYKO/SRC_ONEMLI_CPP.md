# Önemli CPP Dosyaları — GameServer_SRC

**Tarih:** 2026-04-29 | **Hazırlayan:** CHIP | **Kaynak:** Gerçek dosya okuma

---

## En Büyük Dosyalar (Satır Sayısı)

| Dosya | Satır | Amaç |
|-------|-------|------|
| Npc.cpp | 7,086 | NPC AI + davranış |
| MagicInstance.cpp | 6,230 | Skill instance + target |
| DBAgent.cpp | 5,658 | Tüm DB operasyonları |
| User.cpp | 5,210 | Oyuncu çekirdek logic |
| GMCommandsHandler.cpp | 3,075 | GM komutları |
| GameServerDlg.cpp | 3,006 | Ana event loop + init |
| ChatHandler.cpp | 2,770 | Chat + server komutları |
| ItemHandler.cpp | 2,409 | Item alma/satma/taşıma |
| XGuard.cpp | 2,334 | Anti-cheat server hook |
| BotHandler.cpp | 2,327 | Bot (offline merchant + AI) |
| Unit.cpp | 2,262 | Temel entity (oyuncu+NPC ortak) |
| NPCHandler.cpp | 2,135 | NPC paket işleme |
| EventMainSystem.cpp | 2,084 | Event scheduler |
| MerchantHandler.cpp | 1,999 | Dükkan sistemi |
| KnightsManager.cpp | 1,867 | Klan yönetimi |
| DatabaseThread.cpp | 1,813 | Async DB kuyruğu |
| KingSystem.cpp | 1,713 | King of the Hill event |
| MagicProcess.cpp | 1,525 | Skill kullanma + efekt |

---

## Dosya Detayları

### main.cpp
**Amaç:** GameServer entry point
**Önemli:**
- `MYKO_GameServer_Mutex` → aynı anda 1 instance zorunlu
- `SetConsoleTitle("Knight Online Game Systems - v" STRINGIFY(__VERSION))`
- `_ConsoleHandler` → Ctrl+C yakalar, graceful shutdown başlatır
- Graceful shutdown: `s_hEvent.Signal()` → tüm thread'ler durur → DB save → exit

### Define.h
**Amaç:** Tüm proje sabitleri
**Önemli sabitleri:**
- `GAME_SOURCE_VERSION 1098` → paket versiyon kontrolü
- `MAX_LEVEL 83`, `MAX_PLAYER_HP 14000`, `MAX_DAMAGE 32000`
- `MAX_ITEM 28` → envanter slot sayısı
- `CONF_GAME_SERVER "./GameServer.ini"` → config yolu

### User.cpp (5,210 satır)
**Amaç:** Oyuncu çekirdek logic
**Bağlı:** USERDATA tablo, LoginHandler, AttackHandler, XGuard
**Önemli fonksiyonlar:**
- `ProcessLogin` → kullanıcı giriş
- `SendUserInfo` → client'a karakter bilgisi gönder
- `ChangeZone` → harita geçişi
- `XSafe_*` → anti-cheat paket handler'ları (bak: XGuard.cpp)
- `HandleShutdownGMCommand` → GM "down" komutu

### MagicInstance.cpp (6,230 satır)
**Amaç:** Skill instance + hedef sistemi
**Bağlı:** MAGIC_TABLE, MAGIC_TYPE1-9, BattleSystem
**Önemli:**
- `ExecuteType1` → damage skill
- `ExecuteType2` → heal
- `ExecuteType3` → buff/debuff
- Her skill tipi (1-9) ayrı execute metodu

### MagicProcess.cpp (1,525 satır)
**Amaç:** Skill kullanma sürecini yönetir (cast time, cool, efekt)
**Bağlı:** MagicInstance, User, Packet

### DBAgent.cpp (5,658 satır)
**Amaç:** TÜM DB operasyonları tek dosyada
**Bağlı:** shared\database\*.h ORM, DatabaseThread
**Önemli:**
- SP çağrıları (item save, user save, kill log)
- Async queue: `AddDatabaseRequest()` → DatabaseThread işler
- Batch save mekanizması

### DatabaseThread.cpp (1,813 satır)
**Amaç:** Async DB kuyruğu — ana thread'i bloklamaz
**Bağlı:** DBAgent, shared ODBC
**Önemli:**
- Shutdown sırasında kuyruk boşaltılır
- `[Shutdown] Waiting for DB...` log satırı → buradan

### GameServerDlg.cpp (3,006 satır)
**Amaç:** Ana event loop + server init + timer'lar
**Önemli:**
- `Startup()` → tüm modülleri yükler
- `HandleConsoleCommand()` → `/down`, `/notice` vb.
- `ProcessServerCommand()` → command router (ChatHandler.cpp:781'de tanımlı)
- `ShutdownTimer()` → graceful shutdown zamanlayıcısı
- Port config: `m_GameServerPort`, `m_LoginServerPort`
- `ResetBattleZone()` → startup'ta BZ temizle

### ChatHandler.cpp (2,770 satır)
**Amaç:** Chat sistemi + server/GM komut tablosu
**Önemli:**
- `s_commandTable` → server komut kaydı (satır 8)
- `/down` → `HandleShutdownCommand` (satır 30)
- `+down` → `HandleShutdownGMCommand` (satır 247, GM komut)
- `ProcessServerCommand()` → server prefix ile komut çalıştır (satır 781)
- `HandleShutdownCommand` (satır 1315) → `m_Shutdownstart=true`, `m_Shutdownfinishtime=UNIXTIME+1`

**Shutdown komutları:**
```
Server console: /down   → HandleShutdownCommand (anında)
GM in-game:     +down 5 → HandleShutdownGMCommand (5 dakika sonra)
```

### AttackHandler.cpp + BattleSystem.cpp
**Amaç:** Melee/range saldırı hesabı + savaş sistemi
**Bağlı:** User, MagicInstance, Region

### ItemHandler.cpp (2,409 satır)
**Amaç:** Item operasyonları (al, sat, taşı, kullan)
**Bağlı:** USER_ITEMS, ITEM_TABLE, DBAgent

### ItemUpgradeSystem.cpp + ItemSmashSystem.cpp
**Amaç:** +1→+9 upgrade, item smash (yıkım bonusu)
**Bağlı:** ITEM_TABLE, DBAgent, Chat (sonuç bildirim)

### QuestHandler.cpp + QuestDatabase.cpp
**Amaç:** Quest kabul, tamamlama, ödül
**Bağlı:** Lua engine (`lua_bindings.cpp`), QUEST tablo

### LuaEngine.cpp + lua_bindings.cpp
**Amaç:** Lua 5.1 quest/event entegrasyonu
**Bağlı:** QuestHandler, scripting\ klasörü
**Önemli:**
- Quest mantığı Lua'da yazılı → C++ sadece bridge
- `m_luaEngine.Shutdown()` → graceful shutdown sırasında çağrılır

### Knights.cpp + KnightsManager.cpp + CastleSiegeWar.cpp
**Amaç:** Klan sistemi + CSW (Delos kalesi savaşı)
**Bağlı:** KNIGHTS tablo, KNIGHTS_USER, CastleSiegeWarTimer

### Map.cpp + Region.cpp + PathFind.cpp
**Amaç:** Harita yönetimi, bölge sistemi, yol bulma
**Bağlı:** Zone tablo, NpcThread

### Npc.cpp + NpcThread.cpp + BossHandler.cpp (7,086 + 1,166 satır)
**Amaç:** NPC AI + thread yönetimi + boss spawn
**Önemli:** AIServer YOK — tüm NPC AI GameServer'da
- `NpcThread`: Her zone için ayrı thread
- `BossHandler`: Boss spawn zamanlaması

### XGuard.cpp (2,334 satır)
**Amaç:** Anti-cheat server tarafı hook (Pearl Guard iletişim)
**Önemli:**
- `XSafe_ACTIVE 1` → aktif
- `XSafe_VERSION 5` → client versiyon eşleşmesi
- `XSafe_ALIVE_TIMEOUT 60` → 60 sn yanıt gelmezse kick
- `XSafe_StayAlive()` → heartbeat + MD5 challenge (satır 496)
- `XSafe_ProcInfo()` → çalışan process listesi sorgula (satır 591)
- `XSafe_Log()` → cheat log (satır 658)
- `XSafe_ReqMerchantList()` → güvenli merchant list
- MD5 challenge: `md5("1X" + VERSION + "10001" + clock + ischeckdecated2 + accountid)`

### BotHandler.cpp + offlinemerchant.cpp (2,327 satır)
**Amaç:** 1098 normal feature olan offline merchant + bot sistemi
**Önemli:** Production feature, cheat değil

### ServerStartStopHandler.cpp
**Amaç:** Timer thread + event timer loop
**Bağlı:** GameServerDlg, NpcThread, EventMainSystem

### ConsoleInputThread.cpp
**Amaç:** Console'dan komut okuma (stdin)
**Önemli:**
- `HandleConsoleCommand(cmd)` → her satırı GameServerDlg'a yönlendir
- Ctrl+C → thread kapanır (fgets null döner)

### CharacterMovementHandler.cpp
**Amaç:** Karakter hareket + wall cheat detection
**⚠️ UYARI (satır 197):**
```cpp
//UserWallCheatCheckRegion();   ← YORUM SATIRI, production'da AÇILMALI
```
Wall hack detection deaktif! Patlama projesinden kalma. **Doğrula ve aç.**

---

## Önemli Header Dosyaları

| Header | Amaç |
|--------|------|
| `Define.h` | Tüm sabitler |
| `GameDefine.h` | Game-specific sabitler |
| `StdAfx.h` | Precompiled header |
| `User.h` | CUser sınıf tanımı |
| `GameServerDlg.h` | CGameServerDlg (1313+ satır) |
| `DBAgent.h` | DB fonksiyon prototipleri |
| `ChatHandler.h` | Chat sınıf |
| `Knights.h` + `KnightsManager.h` | Klan sınıfları |
| `json.hpp` | nlohmann/json (tek header) |
| `md5.h` | MD5 (XGuard challenge için) |

---

## İlişki Diyagramı

```
main.cpp
  └── CGameServerDlg (GameServerDlg.cpp)
        ├── CUser[] (User.cpp)          ← oyuncu
        │     ├── XGuard/XSafe          ← AC hook
        │     ├── MagicInstance         ← skill
        │     └── ItemHandler           ← item
        ├── CNpc[] (Npc.cpp)            ← NPC
        │     └── NpcThread             ← AI thread
        ├── DBAgent (DBAgent.cpp)       ← DB iletişim
        │     └── DatabaseThread        ← async kuyruk
        ├── LuaEngine                   ← Lua bridge
        ├── EventMainSystem             ← event timer
        └── ConsoleInputThread          ← console komut
```

---

## İlgili Dosyalar

- Genel harita: `SRC_HARITA.md`
- Anti-cheat: `ANTI_CHEAT.md`
- Build: `BUILD.md`
