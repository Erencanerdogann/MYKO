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
| shared | shared\ | ~10 | 144+ | Ortak DB ORM + yardımcılar |
| scripting | scripting\ | - | - | Lua quest ve event |
| N3BASE | N3BASE\ | - | - | 3D yapı (read-only) |
| libcurl | libcurl\ | - | - | HTTP bağlantı (read-only) |

**Toplam GameServer klasörü:** 104,469 satır kaynak kod

---

## TAM MODÜL DÖKÜMÜ — GameServer/ (Büyükten küçüğe, gerçek satır sayıları)

### Çekirdek Motoru (>2000 satır)

| Dosya | Satır | Amaç |
|-------|-------|------|
| `Npc.cpp` | 7,086 | NPC AI, vendor, spawn mantığı |
| `MagicInstance.cpp` | 6,230 | Skill/büyü instans yönetimi |
| `DBAgent.cpp` | 5,658 | MSSQL ODBC köprüsü, tüm DB sorguları |
| `User.cpp` | 5,210 | Oyuncu oturumu — ProcessLogin, SendUserInfo, ChangeZone |
| `GMCommandsHandler.cpp` | 3,075 | GM komutları (kick, ban, spawn, event) |
| `GameServerDlg.cpp` | 3,006 | Ana dialog — Startup, INI yükle, timer |
| `ChatHandler.cpp` | 2,770 | Sohbet, kanal yönetimi, GM chat |
| `ItemHandler.cpp` | 2,409 | Item alma, bırakma, ekipman |
| `XGuard.cpp` | 2,334 | AC server hook — XSafe heartbeat/challenge |
| `BotHandler.cpp` | 2,327 | Bot engelleme + bot merchant |
| `Unit.cpp` | 2,262 | Base entity (User/Npc ortak) |
| `NPCHandler.cpp` | 2,135 | NPC paket handler (click, konuşma) |
| `EventMainSystem.cpp` | 2,084 | Event zamanlama motoru |

### Oyun Sistemleri (1000–2000 satır)

| Dosya | Satır | Amaç |
|-------|-------|------|
| `MerchantHandler.cpp` | 1,999 | Kişisel dükkan / merchant |
| `KnightsManager.cpp` | 1,867 | Klan yönetim merkezi |
| `DatabaseThread.cpp` | 1,813 | Asenkron DB iş kuyruğu |
| `KingSystem.cpp` | 1,713 | Kral sistemi (seçim, görev) |
| `MagicProcess.cpp` | 1,525 | Skill hesap pipeline |
| `PartyHandler.cpp` | 1,507 | Parti kurma, exp paylaşım |
| `ItemUpgradeSystem.cpp` | 1,326 | Eşya +lu yapma |
| `UserSkillStatPointSystem.cpp` | 1,283 | Stat/skill puanı dağıtımı |
| `CharacterSelectionHandler.cpp` | 1,283 | Karakter seçimi / oluşturma |
| `NewRankingSystem.cpp` | 1,201 | PK/seviye ranking |
| `AchieveHandler.cpp` | 1,184 | Başarı sistemi |
| `NpcThread.cpp` | 1,166 | NPC AI timer thread |
| `GiveItemLuA.cpp` | 1,140 | Lua → C++ item verme köprüsü |
| `FundamentalMethods.cpp` | 1,123 | Temel yardımcı metodlar |
| `DungeonDefenceSystem.cpp` | 1,023 | Zindan savunma modu |
| `UserHealtMagicSpSystem.cpp` | 1,016 | HP/MP yönetimi |

### Handler Katmanı (500–1000 satır)

| Dosya | Amaç |
|-------|------|
| `AttackHandler.cpp` | Fiziksel saldırı hesabı |
| `BattleSystem.cpp` | Savaş motoru |
| `CastleSiegeWar.cpp` | Kale kuşatma (CSW) |
| `KnightsDatabaseHandler.cpp` | Klan DB işlemleri |
| `QuestHandler.cpp` | Quest verme/teslim/kontrol |
| `QuestDatabase.cpp` | Quest tablo yükleme |
| `TradeHandler.cpp` | Oyuncu-oyuncu takas |
| `WareHouse.cpp` + `VipWareHouse.cpp` | Depo sistemi |
| `ArenaHandler.cpp` | Arena/PvP modu |
| `TournamentSystem.cpp` | Turnuva sistemi |
| `UserRivalSystem.cpp` | Rakip/düşman sistemi |
| `PetMainHandler.cpp` | Evcil hayvan sistemi |
| `PerksHandler.cpp` | Özellik/avantaj sistemi |
| `PremiumSystem.cpp` | Premium üyelik |
| `KnightCashSystem.cpp` | KC (Knight Cash) sistemi |
| `GenieHandler.cpp` | Genie item sistemi |
| `ShoppingMallHandler.cpp` | Oyun içi mağaza |
| `LetterHandler.cpp` | Oyuncu postası |
| `InfoNotice.cpp` + `NewPacketsNotice.cpp` | Sunucu duyuru sistemi |
| `Map.cpp` + `Region.cpp` + `RegionHandler.cpp` | Harita bölge yönetimi |
| `PathFind.cpp` | NPC yol bulma algoritması |
| `ZoneChangeWarpHandler.cpp` | Bölge geçişi (ışınlanma) |
| `NpcEventSystem.cpp` + `NpcTable.cpp` | NPC event + tablo |
| `BossHandler.cpp` | Boss spawn/kontrol |
| `LuaEngine.cpp` + `lua_bindings.cpp` | Lua VM + binding |
| `LoadServerData.cpp` | Boot: .tbl + DB yükleme |
| `ConsoleInputThread.cpp` | Konsol komut dinleyici |
| `GameSocket.cpp` | Network soket manager |

### Özel Sistemler

| Dosya | Amaç |
|-------|------|
| `CastleSiegeWar.cpp` + `thyke_csw.cpp` | Thyke + normal kale |
| `CindirellaWar.cpp` | Cinderella event |
| `BeefEventNew.cpp` | Beef event |
| `SoccerSystem.cpp` | Futbol event |
| `DrakiTowerSystem.cpp` | Draki kule sistemi |
| `WheelOfFun.cpp` | Çark sistemi |
| `LotterySystem.cpp` | Piyango |
| `ChaosStone.cpp` | Kaos taşı event |
| `MonsterStoneSystem.cpp` | Canavar taşı |
| `BifrostPieceSmashSystem.cpp` | Bifrost parça sistemi |
| `MiningFishingSystem.cpp` + `MiningExchange.cpp` | Madencilik + balıkçılık |
| `CraftingSystem.cpp` | Eşya üretim sistemi |
| `ExchangeSystemMain.cpp` | Gelişmiş takas |
| `CollectionRaceHandler.cpp` | Koleksiyon yarışı |
| `GenderJobChangeHandler.cpp` | Cinsiyet/sınıf değiştirme |
| `NationTransferHandler.cpp` | Millet transferi |
| `KnightCape.cpp` | Klan pelerin sistemi |
| `KnightsAllianceHandler.cpp` | Klan ittifak sistemi |
| `KnightCrownGuard.cpp` | Klan taç koruma |
| `KnightUserReturnSystem.cpp` | Geri dönen oyuncu sistemi |
| `TagChange.cpp` | Tag değiştirme |
| `SealHandler.cpp` | Mühür sistemi |
| `RentalHandler.cpp` | Kiralık eşya |
| `SheriffHandler.cpp` | Şerif sistemi |
| `FriendHandler.cpp` | Arkadaş listesi |
| `ChatRoomHandler.cpp` | Sohbet odaları |
| `dailyrank.cpp` | Günlük ranking |
| `UserDailyOpSystem.cpp` | Günlük görev sistemi |
| `UserGoldSystem.cpp` | Altın yönetimi |
| `UserLoyaltySystem.cpp` | Sadakat puanı |
| `UserRankingSystem.cpp` | Oyuncu ranking |
| `UserObjectSystem.cpp` | Obje etkileşim |
| `UserInfoSystem.cpp` | Oyuncu bilgi sistemi |
| `UserLevelExperienceSystem.cpp` | Deneyim/seviye |
| `UserSkillShortcutSystem.cpp` | Skill kısayol |
| `UserAbilityHandler.cpp` | Yetenek handler |
| `ItemSmashSystem.cpp` | Eşya kırma |
| `Clientless.cpp` | Clientsiz bot desteği |
| `offlinemerchant.cpp` | Çevrimdışı dükkan |
| `FerihaLogHandler.cpp` + `FerihaQueque.cpp` | Feriha olay kuyruğu |
| `FTHandler.cpp` | FT (özel sistem) |
| `WandetEvent.cpp` | Wandet event |
| `JuraidBdwFragSystem.cpp` | Juraid fragment |
| `UnderTheCastleSystem.cpp` | Kale altı sistemi |
| `TowerTransformationProcess.cpp` | Kule dönüşüm |
| `OtherExchange.cpp` | Diğer takas |
| `BundleSystem.cpp` | Demet/paket sistemi |
| `PusRefund.cpp` | PUS iade sistemi |
| `BottomUserList.cpp` | Oyuncu listesi |
| `ServerStartStopHandler.cpp` | Server başlat/durdur |
| `ClanNtsHandler.cpp` | Klan NTS |

### Yardımcılar

| Dosya | Amaç |
|-------|------|
| `main.cpp` | Giriş, mutex, crash handler, console |
| `md5.cpp` | MD5 hash (XSafe challenge için) |
| `FundamentalMethods.cpp` | Temel yardımcılar |
| `HelperMethods.cpp` | Ek yardımcılar |
| `GiveItemExchange.cpp` | Item exchange köprüsü |
| `stdafx.cpp` | Precompiled header |

---

## Header Dosyaları (GameServer/ — 34 adet)

```
BotHandler.h         ChatHandler.h        ConsoleColor.h
ConsoleInputThread.h DBAgent.h            Define.h
FerihaQueque.h       GameDefine.h         GameEvent.h
GameServerDlg.h      KingSystem.h         Knights.h
KnightsManager.h     LoadServerData.h     LuaEngine.h
MagicInstance.h      MagicProcess.h       Map.h
Npc.h                NpcDefines.h         NpcSignalling.h
NpcTable.h           NpcThread.h          PathFind.h
PusCategorty.h       PusItemSet.h         Region.h
Resource.h           SheriffReportListSet.h StdAfx.h
Unit.h               User.h               lua_bindings.h
md5.h
```

---

## shared/ Modülü (Ortak Kütüphane)

### Ağ Katmanı
```
KOSocket.cpp/h          — Temel soket sınıfı
KOSocketMgr.h           — Soket yönetici
ClientSocketMgr.h       — İstemci soket yönetici  
ListenSocketWin32.h     — Windows dinleme soketi
Socket.cpp/h            — Düşük seviye soket
SocketMgr.cpp/h         — Soket havuzu
SocketOps.h + Win32     — Platform spesifik operasyonlar
SocketPoll.cpp/h        — Poll/select mantığı
```

### Kriptografi & Güvenlik
```
JvCryption.cpp/h        — Paket şifreleme (JvCryption algoritması)
crc32.c/h               — CRC32 kontrol
```

### Yardımcılar
```
Ini.cpp/h               — INI dosya okuma
DateTime.h              — Tarih/saat yardımcısı
Thread.cpp/h            — Thread wrapper
Condition.cpp/h         — Thread senkronizasyon (mutex/event)
TimeThread.cpp/h        — Global zaman güncelleyici
ByteBuffer.h            — Binary buffer
CircularBuffer.cpp/h    — Dairesel tampon
ServerConfig.h          — INI → config struct mapping
signal_handler.cpp/h    — SIGTERM/SIGINT yakalama
CrashHandler.cpp/h      — BugTrap entegrasyon
```

### Diğer
```
SMDFile.cpp/h           — SMD (3D) dosya okuma
lzf.cpp/h               — LZF sıkıştırma
globals.cpp/h           — Global değişkenler
DebugUtils.cpp/h        — Debug yardımcıları
LogSystem.h             — Log sistemi
tstring.cpp/h           — Unicode/ANSI string
version.h               — #define __VERSION 2369
Explosion.cpp/h         — BugTrap exception handler
HardwareInformation.cpp/h — Donanım bilgisi
Atomic.h                — Atomik operasyonlar
```

### Database Header'ları (shared\database\ — 144 dosya)

- **144 adet dosya** (138 `.h` ORM şeması + 8 .cpp ODBC + 2 stdafx + structs + thykedb)
- Her `.h` = 1 DB tablosu ORM şeması
- MSSQL → ODBC → `ServerConfig.h` bağlantı string
- DSN adı: `CodeGuardMYKO_DB` (Türkçe karakter YASAK — bak: Patlama Dersleri)

**Kategori dökümü:**

| Kategori | Dosyalar (örnek) |
|----------|-----------------|
| Achieve | AchieveCom, AchieveMain, AchieveMonster, AchieveNormal, AchieveTitle, AchieveWar |
| Event | EventAllSet, EventSet, EventTimes, EventTriggerSet, EventScheduleStatusSet, EventTimerShowListSet |
| Item | ItemTableSet, ItemUpgradeSet, ItemExchangeSet, ItemOpSet, ItemSellTableSet, ItemPremiumGift, vb. |
| Monster | MonsterItemSet, MonsterResourceSet, MonsterRespawnListSet, MonsterSummonListSet, vb. |
| Knight/Klan | KnightsSet, KnightsUserSet, KnightsAllianceSet, KnightsCapeSet, KnightsSiegeWar, vb. |
| Magic | MagicTableSet, MagicType1Set–MagicType9Set |
| User | UserItemSet, UserDailyOpSet, UserKnightsRankSet, UserPersonalRankSet, UserLootSettingsSet |
| Quest | QuestHelperSet, QuestMonsterSet, DailyQuestSet |
| King | KingSystemSet, KingElectionListSet, KingNominationListSet, KingCandidacyNoticeBoardSet |
| NPC | NpcItemSet, NpcPosSet, NpcTableSet, ObjectPosSet |
| Crafting | MakeItemGroupSet, MakeWeaponTableSet, MakeGradeItemTableSet, vb. |
| ODBC | OdbcCommand, OdbcConnection, OdbcParameter, OdbcRecordset |
| Diğer | BattleSet, ZoneInfoSet, ServerSettingSet, SetItemTableSet, PetDataInfo, vb. |

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

## ASCII Klasör Diyagramı

```
GameServer_SRC/
├── CodeGuardGameServer.sln          ← Ana çözüm (Pearl Guard entegre)
├── ByNoiseGameServer.sln            ← Alt çözüm (standalone)
├── Hdd Unban.bat                    ← Yardımcı script
├── clean.bat                        ← Build temizlik
│
├── GameServer/                      ← 132 .cpp + 34 .h (104,469 satır)
│   ├── main.cpp                     ← Mutex, crash handler, console thread
│   ├── Define.h                     ← GAME_SOURCE_VERSION 1098, MAX_LEVEL 83
│   ├── User.cpp        (5,210)      ← ProcessLogin, SendUserInfo, ChangeZone
│   ├── Npc.cpp         (7,086)      ← NPC AI + vendor
│   ├── MagicInstance.cpp (6,230)    ← Skill/büyü instans
│   ├── DBAgent.cpp     (5,658)      ← ODBC köprüsü
│   ├── GameServerDlg.cpp (3,006)    ← Ana startup + timer
│   ├── ChatHandler.cpp (2,770)      ← Sohbet + GM chat
│   ├── XGuard.cpp      (2,334)      ← AC server hook
│   └── [128 diğer .cpp]
│
├── LogInServer/                     ← 7 .cpp + 5 .h (auth/login)
│   ├── main.cpp
│   ├── LoginServer.cpp/h
│   ├── LoginHandler.cpp/h
│   ├── LoginSession.cpp/h
│   ├── GameSocket.cpp/h
│   ├── DBProcess.cpp/h
│   └── stdafx.cpp
│
├── shared/                          ← 232 MB ortak kütüphane
│   ├── version.h                    ← #define __VERSION 2369
│   ├── ServerConfig.h               ← INI config struct
│   ├── JvCryption.cpp/h             ← Paket şifreleme
│   ├── KOSocket.cpp/h               ← Ağ katmanı
│   ├── Condition.cpp/h              ← Thread senkron
│   ├── CrashHandler.cpp/h           ← BugTrap
│   └── database/                   ← 144 dosya
│       ├── OdbcCommand/Connection/Parameter/Recordset  ← ODBC wrapper
│       ├── [138 .h]                 ← Her .h = 1 DB tablo ORM
│       └── structs.h                ← Paylaşılan struct'lar
│
├── scripting/                       ← Lua betik dosyaları
├── N3BASE/                          ← 3D shape/mesh (read-only)
├── libcurl/                         ← HTTP kütüphane (statik, read-only)
└── x64/Release/                     ← Build çıktı
    ├── GameServer.exe               ← ~3.4 MB
    └── LogInServer.exe              ← ~496 KB
```

---

## Aktif Geliştirme (29 Nisan 2026)

Son commit: `casus yol fix` (29 Apr 02:09)
En aktif dosyalar: `User.cpp`, `MagicInstance.cpp`, `GameServerDlg.cpp`, `CharacterMovementHandler.cpp`

Aktif değişiklik listesi:
```bash
git log --oneline --name-only -20
```

---

## Versiyon Çiftleme (1098 Patch Notu)

Bu source **v2369 base** — ancak `GAME_SOURCE_VERSION 1098` ile çalışır:
- `shared/version.h`: `#define __VERSION 2369` (konsol başlığı)  
- `GameServer/Define.h`: `#define GAME_SOURCE_VERSION 1098` (protokol/patch seviyesi)
- 1098 patch: lvl cap 83, Bifrost sistemi YOK, Ardream 59 cap
- Alternatif sürümler comment'te: `// 1098, 1534, 2369`

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
