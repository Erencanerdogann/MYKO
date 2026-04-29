# MYKO Materyal Haritası — Hangi Bilgi Hangi Dosyada

**Tarih:** 2026-04-29
**Hedef:** Oyun elementlerini değiştirirken/öğrenirken **bakılacak dosyalar** tek tablo.
**Yapı:** Her element için → `User Client` (.tbl) + `DB tablo` + `Source code` + `Lua script` + `Map data`

---

## 🎯 NASIL OKUNUR

```
[ELEMENT] = ne öğrenmek/değiştirmek istiyorsun?
    ↓
[CLIENT]  = oyuncunun gördüğü görsel/data (.tbl, .n3chr, .smd)
[DB]      = MSSQL tablosu (kalıcı state)
[SRC]     = C++ kod (logic, formül, hesap)
[LUA]     = quest/event scripti (logic dışında)
[CONFIG]  = .ini ayarı (rate, balance)
```

---

## 1️⃣ ITEM (Eşya)

### Client Data (`C:\MalaysiaKO\Data\`)
| Dosya | İçerik |
|-------|--------|
| `Item.tbl` | Tüm item temel bilgi (ID, isim, tier, cap) |
| `Item_Ext.tbl` | Item extension data (effect, special) |
| `Item_Ext_*.tbl` | Item extension parçalı (level/class) |
| `ITEM_SELL.tbl` | NPC alış-satış fiyat |
| `ITEM_UPGRADE.tbl` | Anvil +1...+9 oran tablosu |

### Asset (`C:\MalaysiaKO\Item\`)
- `item.src` (540 MB) + `item.hdr` — 3D model + texture binary

### DB Tabloları (`KO_MYKO`)
| Tablo | Ne tutar |
|-------|----------|
| `ITEM` | Item master (statik) |
| `ITEM_EXTENSION` | Effect/special |
| `ITEM_SELLTABLE` | NPC fiyat (PK: iSellingGroup+nIndex) |
| `USER_ITEM` | Oyuncu envanteri (tekil) — ⚠️ `USER_ITEMS` boş |
| `ITEM_UPGRADE` | Upgrade oran |

### Source (`C:\temp\MYKO\src\GameServer_SRC\GameServer\`)
| Dosya | Ne yapar |
|-------|----------|
| `ItemHandler.cpp` | Item logic (use, drop, get) |
| `ItemUpgradeSystem.cpp` | +1..+9 anvil mantığı |
| `ItemSmashSystem.cpp` | Item kırma |
| `WareHouse.cpp` | Bank deposito |
| `VipWareHouse.cpp` | VIP bank |
| `MerchantHandler.cpp` | NPC trade |
| `ShoppingMallHandler.cpp` | PUS (Power Up Store) |
| `RentalHandler.cpp` | Item kiralama |

### Bilinen Bug
- `TS 381001000` (Transformation Scroll) → çalışmıyor + sell 1.3M (400k olmalı). Duration=1, SellPrice=0 şüpheli.

### Tools
- `tools\tbl\tbl_decrypt.py` → .tbl şifre çöz
- `tools\tbl\tbl_edit.py` → .tbl tek satır düzenle
- `tools\item_search.py` → item arama

---

## 2️⃣ SKILL / MAGIC (Beceri / Büyü)

### Client Data
| Dosya | İçerik |
|-------|--------|
| `MagicTable.tbl` | Skill ana tablo (ID, isim, MP, cooldown) |
| `MagicType1.tbl` | Direct damage |
| `MagicType2.tbl` | Heal/buff |
| `MagicType3.tbl` | DoT/HoT |
| `MagicType4.tbl` | Summon/transform |
| `MagicType5-9.tbl` | Diğer skill tipleri |

### DB
| Tablo | Ne tutar |
|-------|----------|
| `MAGIC_TABLE` | Skill master |
| `MAGIC_TYPE1-9` | Skill tipleri |
| `USER_SKILL_BAR` | Oyuncu skill shortcut (SP `SKILLSHORTCUT_SAVE`) |
| `USER_DURATION_SKILL` | Aktif buff/debuff |

### Source
| Dosya | Ne yapar |
|-------|----------|
| `MagicInstance.cpp` (6230 satır) | Skill instance, target, hit |
| `MagicProcess.cpp` (1525 satır) | Skill effect uygulama |
| `UserDurationSkillSystem.cpp` | Buff timer |
| `UserSkillStatPointSystem.cpp` | Skill point dağılımı |

### Lua (skill ile ilgili quest)
- Skill açma quest dialogları (Quests/*.lua) — örn class change

### Tools
- [Kobugda Skill Calculator](https://kobugda.com/skill-calculator) — build planlama (referans)

---

## 3️⃣ QUEST (Görev)

### Client Data
| Dosya | İçerik |
|-------|--------|
| `Quest.tbl` | Quest master (ID, name, reward, prereq) |

### Source
| Dosya | Ne yapar |
|-------|----------|
| `QuestHandler.cpp` | Quest start/finish/reward |
| `QuestDatabase.cpp` | Quest state DB ops |
| `DailyQuest.cpp` | Daily reset quest |

### Lua Scripts (asıl quest mantığı)
- **`Desktop\Server\Quests\` → 511 .lua dosyası** (aktif test server)
- **`server\myko_server\Quests\` → 1013 .lua dosyası** (referans)
- **İsimlendirme:** `<NPC_ID>_<NPC_NAME>.lua`
  - Örnek: `14204_Minerva.lua`, `11051_Sphie.lua`, `11810_Helena.lua`, `13016_Keite.lua`
- **İçerik:** Events, nation check (Karus/Humans vs Elmorad), dialog, map teleport, item give

### DB
| Tablo | Ne tutar |
|-------|----------|
| `QUEST_HELP` | Quest açıklama metni |
| `QUEST_SKILLS_CLOSED_DATA` | Skill quest durumu |
| `USER_QUEST_LOG` | Oyuncu quest progress |

### Lua Engine
- `LuaEngine.cpp` + `lua_bindings.cpp` (src) → Quest .lua scriptleri çalıştırır

---

## 4️⃣ NPC

### Client Data
| Dosya | İçerik |
|-------|--------|
| `NPC.tbl` | NPC master (ID, isim, level, HP, drop) |
| `NPC_Pos.tbl` | NPC spawn lokasyon |

### Asset (`C:\MalaysiaKO\Chr\`)
- `*.n3chr` — NPC 3D model
- `*.n3anim` — NPC animasyon

### DB
| Tablo | Ne tutar |
|-------|----------|
| `K_NPC` | NPC master |
| `MONSTER` | Monster tablosu (ayrı) |
| `NPC_ITEM` | NPC drop tablo |

### Source
| Dosya | Ne yapar |
|-------|----------|
| `Npc.cpp` | NPC entity sınıf |
| `NPCHandler.cpp` | NPC etkileşim |
| `NpcTable.cpp` | NPC veri yükle |
| `NpcThread.cpp` | NPC AI thread (AIServer yok, bu kod onun yerine) |
| `NpcEventSystem.cpp` | NPC event tetikleyici |
| `BossHandler.cpp` | Boss spawn/drop |

### Map data
- `Map\*.aievt` — NPC AI event/spawn (binary)
- `Map\<zone>.aievt` — zone-bazlı spawn

---

## 5️⃣ MAP / ZONE

### Map Data (`Desktop\Server\Map\`)
**61 .smd zone dosyası — 1098 prefix önemli:**

| Dosya | Zone |
|-------|------|
| `1098elmo2004.smd` | El Morad (3.8 MB) |
| `1098karus2004.smd` | Karus (3.3 MB) |
| `1098moradon_0826.smd` | Moradon — neutral hub (2.9 MB) |
| `1098war_a.smd` | War zone PvP (1.0 MB) |
| `1098freezone_a/b/c.smd` | Free zone 3 varyant |
| `1098In_dungeon01-03.smd` | Dungeon iç |
| `BattleZone.smd` | Ana PvP arena |
| `Code_Moradon_war.smd` | Moradon war event |
| `dungeon_a/b1th/b2th/b3th2015/d.smd` | Dungeon serisi |
| `arena.smd`, `bossmode.smd` | Arena/Boss event |
| `LK_war_a01.smd` | LK war |
| `2017_flagwar.smd` | Flag war |
| `14th_oldmoradon.smd` | Eski moradon |

### AI Event (`Map\*.aievt`)
- 14 .aievt — NPC spawn/behavior tanımı

### Source
| Dosya | Ne yapar |
|-------|----------|
| `Map.cpp` | Map yapısı, region |
| `Region.cpp` | Region (chunk) yönetim |
| `RegionHandler.cpp` | Region event |
| `PathFind.cpp` | NPC pathfinding |
| `ZoneChangeWarpHandler.cpp` | Zone geçişi |

### DB
| Tablo | Ne tutar |
|-------|----------|
| `ZONE_INFO` | Zone master (ID, isim, max_user) |
| `WARP_LIST` | Warp gate noktaları |

### Tools
- `Map\WarpGateEditor.exe` — warp düzenleyici
- `Server.ini [ZONE_INFO]` — zone server count, IP

### Bilinen Bug (memory)
- **Moradon harita bug:** M-key Moradon'da yanlış konum gösteriyor. ui.src farklı.

---

## 6️⃣ KARAKTER / SINIF

### Client Data
| Dosya | İçerik |
|-------|--------|
| `Class.tbl` | Sınıf master (Warrior/Rogue/Mage/Priest) |
| `JobClass.tbl` | Master sınıf (Blade Master, Berserker, vb.) |

### Asset (`C:\MalaysiaKO\Chr\`)
- Karakter modelleri (race + class kombinasyonu)
- `ChrSelect/` → karakter seçim ekranı

### DB
| Tablo | Ne tutar |
|-------|----------|
| `USERDATA` | Karakter ana (Level, Exp, Gold, Zone, Stat) |
| `USER_ITEM` | Envanter |
| `USER_SKILL_POINT` | Skill point |
| `USER_RIVAL` | Rakip listesi |
| `USERDATA.Authority` | 2=GM (GAME_MASTER_SETTINGS INSERT gerek) |

### Source
| Dosya | Ne yapar |
|-------|----------|
| `User.cpp` (5210 satır) | Oyuncu çekirdek logic |
| `LoginHandler.cpp` | Login akışı |
| `CharacterSelectionHandler.cpp` | Karakter seçim |
| `CharacterMovementHandler.cpp` | Hareket |
| `UserInfoSystem.cpp` | Stat/info ekran |
| `GenderJobChangeHandler.cpp` | Cinsiyet/sınıf değiştirme |
| `NationTransferHandler.cpp` | Karus ↔ El Morad geçiş |

### Stored Procedures
- `LOAD_USER_DATA` → karakter yükle
- `CREATE_NEW_CHAR` → yeni karakter
- `UPDATE_USER_DATA` → karakter kaydet
- `SET_LOGIN_INFO` → login state

---

## 7️⃣ KLAN / GUILD

### Source
| Dosya | Ne yapar |
|-------|----------|
| `Knights.cpp` | Klan logic |
| `KnightsManager.cpp` | Klan listesi yönetim |
| `KnightsDatabaseHandler.cpp` | Klan DB ops |
| `ClanBank.cpp` | Klan bankası |
| `KnightsAllianceHandler.cpp` | Klan ittifak |
| `CastleSiegeWar.cpp` | Delos kale savaşı |

### DB
| Tablo | Ne tutar |
|-------|----------|
| `KNIGHTS` | Klan master |
| `KNIGHTS_USER` | Klan üyeleri |
| `KNIGHTS_RANK` | Rütbe |
| `CLAN_BANK` | Banka envanteri |

### Config (`Desktop\Server\GameServer.ini`)
- `[CLAN_GRADE]` → GRADE1-4 (720k, 360k, 144k, 72k loyalty)
- `[CLAN_PREMIUM]` → EXPBONUS=10, GOLDBONUS=2, NPBONUS=100, MERCHANT=1
- `[CLAN_BANK]` → STATUS=1
- `[CASTLE]` → NATION=1

---

## 8️⃣ EVENT / SAVAŞ

### Source
| Dosya | Ne yapar |
|-------|----------|
| `EventMainSystem.cpp` | Event ana |
| `EventMainTimer.cpp` | Event zamanlayıcı |
| `EventTrapSystem.cpp` | Trap event |
| `EventSigningSystem.cpp` | Event kayıt |
| `BeefEventNew.cpp` | Beef event (custom) |
| `CastleSiegeWar.cpp` | CSW Delos |

### Config Files
**`Desktop\Server\EventAwards.ini`** (2.2 KB):
- `[BORDER_DEFENSE_WAR]` → loyalty/cash levels
- `[CASTLE_SIEGE_WARFARE]` → Loser=100/500, Winner=300/3000
- `[CHAOS_EXPANSION]` → ranking 1-3, 4-7, 8-10 ödülleri
- `[JURAID_MOUNTION_DEFENSE]` → Loser=389205000, Winner=389196000

**`Desktop\Server\EventSettings.ini`** (228 B):
- `[CLAN_BUFF_SYSTEM]` → 5/10/15/20/25/30 lvl ONLINE_EXP/NP buff
- `STATUS=1`

### Scheduler
- `C:\MalaysiaKO\Scheduler.ini` (141 KB, 4743 satır) — war scheduler binary/hex

### Bilinen events
- Border Defense War (BDW)
- Castle Siege War (CSW Delos)
- Chaos Expansion / Chaos Stone
- Juraid Mountain Defense
- Lunar War
- Felankor / Bifrost (1098'de Bifrost YOK)

---

## 9️⃣ PVP / NP

### DB
| Tablo | Ne tutar |
|-------|----------|
| `USERDATA.sLoyalty` | NP (National Point) |
| `USERDATA.sLoyaltyMonthly` | Aylık NP |
| `KNIGHTS.sNP` | Klan NP |

### Source
| Dosya | Ne yapar |
|-------|----------|
| `AttackHandler.cpp` | PvP saldırı |
| `BattleSystem.cpp` | Savaş hesap |
| `UserRivalSystem.cpp` | Rakip takibi |
| `NewRankingSystem.cpp` | Sıralama |
| `ArenaHandler.cpp` | Arena |
| `TournamentSystem.cpp` | Turnuva |

### NP kuralları (KAYNAK_HAVUZU)
- Solo kill = 50 NP
- PvP zone = Ardream / Ronark Land Base / Ronark Land
- 1098'de Ardream YOK → CZ (Colony Zone) ana

---

## 🔟 PARTI / RANK / ACHIEVEMENT

### Source
- `PartyHandler.cpp` — parti
- `NewRankingSystem.cpp` — günlük rank
- `AchieveCom.h`, `AchieveMain.h`, `AchieveMonster.h`, `AchieveNormal.h`, `AchieveTitle.h`, `AchieveWar.h` (shared/database)

### DB
- `USER_ACHIEVE_LOAD_DATA`
- `RANK_DAY` / `RANK_MONTH`

---

## 1️⃣1️⃣ PREMIUM / CASH / PUS

### Source
| Dosya | Ne yapar |
|-------|----------|
| `PremiumSystem.cpp` | Premium üyelik |
| `KnightCashSystem.cpp` | Cash item |
| `ShoppingMallHandler.cpp` | PUS satın alma |
| `KnightCape.cpp` | Cape sistemi |
| `SealHandler.cpp` | Item seal/binding |

### Config
- `Desktop\Server\CapeBonus.txt` → 300HP/150MP/3AP/+5NP
- `Desktop\Server\ClanPremiumNotice.txt` → Exp+30%/NP+8/Drop+1%/Noah+30%/SellPercent+50%

---

## 1️⃣2️⃣ PET / GENIE / RENTAL

### Source
- `PetMainHandler.cpp` — Pet
- `GenieHandler.cpp` — Genie (auto-pickup, auto-loot)
- `RentalHandler.cpp` — Item kiralama

---

## 1️⃣3️⃣ CHAT / İLETİŞİM

### Source
| Dosya | Ne yapar |
|-------|----------|
| `ChatHandler.cpp` (90 KB) | Chat ana |
| `ChatRoomHandler.cpp` | Chat odası |
| `LetterHandler.cpp` | Mektup sistemi |
| `InfoNotice.cpp` | Sistem mesajı |
| `NewPacketsNotice.cpp` | Paket bildirimi |

### Config
- `Desktop\Server\censor_words.txt` → 30 spam filter (TR + EN)

---

## 1️⃣4️⃣ TICARET / EKONOMI

### Source
| Dosya | Ne yapar |
|-------|----------|
| `TradeHandler.cpp` | Oyuncu ticaret |
| `MerchantHandler.cpp` | NPC merchant |
| `offlinemerchant.cpp` | Offline AFK merchant |

### DB
- `USERDATA.iGold` (Noah)
- `BOT_MERCHANT_DATA` — offline merchant

---

## 1️⃣5️⃣ ANTI-CHEAT / GUARD

### Source (Pearl Guard)
**`C:\temp\MYKO\src\AntiCheat_SRC\`** (1.3 GB):
- `XGuard.cpp` (GameServer side hook)
- `Pearl.cpp` (DLL ana)
- `CODE Cli.710F3AD0/` — code obfuscation
- `DetourAPI 3.0/` — Microsoft Detours hook
- `Virtualizer/` — kod virtualization
- `RC5/` — RC5 cipher
- `discord-rpc-master/` — Discord status

### Client DLL
- `C:\MalaysiaKO\code.guard` (6.0 MB)
- `C:\MalaysiaKO\CodeGuard\Code\*.code` — şifreli UI/script (re_*, macho_*, co_*, El_*, Ka_*)

### Monitor (debug)
- `C:\temp\MYKO\PearlMonitor\` → real-time paket/stat izleme
- Pipe: `\\.\pipe\PearlMonitor`

### Şifreleme katmanları (6 sistem)
1. **JvCryption** — paket (uint64)
2. **RC4 MYKO** — .code dosya (SHA-1 derivation)
3. **RC4 NTF** — texture
4. **K2 XOR** — TBL layer 2
5. **K1 XOR** — OpenKO TBL
6. **DES s_secret1** — TBL layer 1

### Tools (key rotation)
- `tools\key_rotation\key_generator.py` — yeni key üret
- `tools\key_rotation\rc4_re_encrypt.py` — .code yeniden şifrele
- `tools\key_rotation\tbl_re_encrypt.py` — .tbl yeniden şifrele
- `tools\key_rotation\exe_key_patcher.py` — exe içindeki key patch

---

## 1️⃣6️⃣ PATCH / DEPLOY

### Patch Server
- **Production:** `104.238.23.99:80` → `patch_server.js` (Node.js)
- **DB:** `VERSION` tablosu → `INSERT INTO VERSION VALUES (X, X-1, 'X.zip')`

### Tools
- `tools\patch_tool.py` — SSH + DB kayıt → otomatik deploy

### Patch Zip Arşivi
- `Desktop\Server\patch\2370.zip` (984 KB)
- `Desktop\Server\patch\2371.zip` (985 KB)
- `Desktop\Server\patch\2372.zip` (984 KB)
- `Desktop\Server\patch\2373.zip` (12 MB) ← son patch

### Reboot Akışı (CLAUDE.md)
```
sqlcmd → INSERT VERSION
taskkill /IM LogInServer.exe /F
wmic process call create yeni exe
# Reboot sonrası BEKLE - task "completed" ≠ sunucu hazır
```

---

## 1️⃣7️⃣ WEB / SITE API

### Production endpoints
- `104.238.23.99:8091` (PHP nginx) — koweb2
- `104.238.23.99:3001` (Rust orkestra-server) — site API
  - `POST /api/site/register` (3/dk)
  - `POST /api/site/login` (10/dk) → token, yetki(1/2/9)
  - `GET /api/site/online` / `/rankings` / `/server-status` / `/health`
- `104.238.23.99:80` — patch server (Node.js)
- MariaDB:3307 → Flarum forum DB

### DB tabloları
- `TB_USER` — web hesap
  - ⚠️ `strWebHash` NULL bug → auto-register hesap siteye giremez

---

## 1️⃣8️⃣ LOG SİSTEMİ

### Server logs (`Desktop\Server\Logs\`)
- `GameServer.log` — ana server log
- `LoginServer.log` — auth log
- `GENERAL_<tarih>.log` — günlük genel
- `DISCONNECT_<tarih>.log` — disconnect olayları
- `GM_<tarih>.log` — GM komut logu
- `Login_<tarih>.log` — login geçmişi
- `diag.log` (1.1 MB) — diagnostic detaylı

### Diğer
- `patch_http.log` — patch download
- `gameserver_output.txt`, `gs_output.txt` — stdout
- `crashfiles.dmp` — crash dump

---

## 1️⃣9️⃣ TBL / DATA YÖNETİMİ

### .tbl Dosya Sistemi
- **Konum:** `C:\MalaysiaKO\Data\` (246 dosya, 36 MB)
- **Şifreleme:** DES Feistel + K2 XOR
- **Tools:**
  - `tbl_decrypt.py` → şifre çöz
  - `tbl_edit.py` / `tbl_edit_v2.py` → satır düzenle
  - `tbl_compare.py` → karşılaştır
  - `tbl_fix_*.py` → encoding/ACS düzelt
  - `tbl_scan_all.py` → toplu tarama
  - `tbl_re_encrypt.py` → yeniden şifrele

### Önemli .tbl dosyaları (örnek)
| Dosya | İçerik |
|-------|--------|
| `Item.tbl` + `Item_Ext_*.tbl` | Item master |
| `MagicTable.tbl` + `MagicType1-9.tbl` | Skill |
| `NPC.tbl` + `NPC_Pos.tbl` | NPC |
| `Quest.tbl` | Quest |
| `Class.tbl` + `JobClass.tbl` | Sınıf |
| `ZONE_INFO.tbl` | Zone |
| `ITEM_SELL.tbl` | NPC alış-satış |
| `ACHIEVE_main.tbl` | Achievement |

### TBL_HASH (validation)
`Desktop\Server\GameServer.ini`:
```
[TBL_HASH]
ITEM_ORG = <hash>
MAGIC_MAIN = <hash>
ZONES = <hash>
```
→ Server boot'ta .tbl integrity check

---

## 2️⃣0️⃣ UI

### Asset
- `C:\MalaysiaKO\UI\ui.src` (2.9 GB) + `ui.hdr`
- **En büyük asset** — UI layout/resource paketi

### CodeGuard UI scripts
- `C:\MalaysiaKO\CodeGuard\Code\*.code` (3.7 MB)
  - `re_*.code` — re_login_intro, re_reconnect (login akışı)
  - `macho_*.code` — NPC menu
  - `co_*.code` — character (co_character_seal)
  - `El_*.code` — El Morad spesifik
  - `Ka_*.code` — Karus spesifik
- **Şifreleme:** RC4 MYKO (Windows CryptoAPI uyumlu)

### Tools
- `tools\Uif-Decryptor\` — UIF (UI dosyası) şifre çöz
- `tools\Uif-Encryptor\` — şifrele

### Ekstra
- `C:\temp\MYKO\tools\DXT & UIE EDİTÖR\UIE.exe`
- `N3TexViewerPNG.exe` — texture viewer

---

## 🚀 HIZLI BAŞVURU TABLOSU

**"X özelliği için bakacağım yerler" hızlı kart:**

| Özellik | Client (.tbl) | DB | Source | Lua |
|---------|---------------|-----|--------|-----|
| Item ekle | Item.tbl | ITEM | ItemHandler.cpp | — |
| Item upgrade oran | ITEM_UPGRADE.tbl | ITEM_UPGRADE | ItemUpgradeSystem.cpp | — |
| Skill yarat/değiştir | MagicTable.tbl + Type1-9 | MAGIC_TABLE/TYPE | MagicInstance.cpp | — |
| Skill formül | — | — | MagicProcess.cpp | — |
| Quest yarat | Quest.tbl | QUEST_HELP | QuestHandler.cpp | `Quests/<NPC_ID>_<NAME>.lua` |
| NPC ekle/spawn | NPC.tbl + NPC_Pos.tbl | K_NPC | Npc.cpp | aievt + Lua |
| NPC AI | — | — | NpcThread.cpp | aievt |
| Map ekle | — | ZONE_INFO | Map.cpp | smd dosyası |
| Sınıf değiştir | Class.tbl | USERDATA | GenderJobChangeHandler.cpp | — |
| Klan rütbe | — | KNIGHTS_RANK | Knights.cpp | — |
| CSW kuralı | — | — | CastleSiegeWar.cpp | EventAwards.ini |
| Drop oran | — | NPC_ITEM | LootHandler? | — |
| Buff timer | — | USER_DURATION_SKILL | UserDurationSkillSystem.cpp | — |
| Chat filter | — | — | ChatHandler.cpp | censor_words.txt |
| PUS item | — | — | ShoppingMallHandler.cpp | — |

---

## ⚠️ DİKKAT NOKTALARI

1. **2369 base + 1098 patch** — bazı modern özellikler (Bifrost, Ardream, Dragon Cave) **YOK**
2. **AIServer YOK** — NPC AI Lua + NpcThread.cpp'de
3. **DBServer YOK** — direct ODBC MSSQL
4. **Git YOK** (src/) — version control sadece .bak chain
5. **TBL_HASH validation** — .tbl değiştirince GameServer.ini'deki hash güncellenmeli
6. **Şifre 6 katman** — .tbl/.code/.uif değiştirip deploy etmeden ÖNCE re-encrypt
7. **TS 381001000 transformation scroll BUG** — Duration=1, SellPrice=0 (not düşülmüş)

---

## 📁 SONRAKI ADIM

Bu MD haritayı verir, **konu özelinde** yazılacak MD'ler:
- `01_ITEM_DETAY.md` — item ID aralığı, drop oran formülü, anvil oran
- `02_SKILL_DETAY.md` — skill type açıklama, formula, master quest
- `03_QUEST_KATALOG.md` — 1500 Lua quest gruplandırma
- `04_NPC_KATALOG.md` — Minerva, Helena, Keite vb. NPC isim → fonksiyon
- `05_MAP_KATALOG.md` — 11 zone detayı (1098 prefix)
- `06_CLAN_DETAY.md` — KNIGHTS_RANK, CSW, alliance
- `07_EVENT_DETAY.md` — BDW, CSW, Chaos, Juraid kuralları

Her detay MD'si yazılınca bu haritada **link** eklenir.

---

**MD sürümü:** v1.0
**İlk yazım:** 2026-04-29 (DOKTOR — Mykoproject.map.md + KAYNAK_HAVUZU veri birleşimi)
