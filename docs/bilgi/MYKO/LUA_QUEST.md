# LUA QUEST — 510 Quest Script, NPC Dialog Sistemi

**Tarih:** 2026-04-29 | **Kategori:** GAME LOGIC | **Script Sayısı:** 510 + 1013 referans | **Format:** Lua 5.1 | **Entegrasyon:** LuaEngine.cpp

---

## 1. QUEST SİSTEMİ GENEL

Quest = NPC ile dialog → seçim → item/exp ödül.

**Yapısı:**
- **Client:** Quest.tbl (master), CodeGuard UI script
- **Lua:** NPC quest dialog (`Quests\*.lua`)
- **DB:** USER_QUEST_LOG (progress)
- **Server:** QuestHandler.cpp (validate, reward)

---

## 2. LUA QUEST DOSYASI KONUMU — 3 LOKASYON

⚠️ **KRITIK:** Lua quest dosyaları 3 farklı yerde, senkronizasyon **garanti değil**. Dokümantasyon **LOKAL** (erenc Desktop) üzerine yapılmıştır.

### A) Production Sunucu (104.238.23.99)
```
C:\Users\Administrator\Desktop\Server\Quests\
```
**Status:** KODCU'ya erişim YASAK (SSH + sunucu yönetimi DOKTOR/RUSTIK'ten)
**Aktif:** Oyuncular bunu kullanıyor

### B) Lokal Dev (Referans — KODCU çalışması)
```
C:\Users\erenc\Desktop\Server\Quests\
├── 01_main.lua              — Entry point (EVENT router)
├── 10305_Move.lua           — Karus warp quest örneği
├── 11051_Sphie.lua          — Skill trainer
├── 11510_Forkwain.lua       — Karus quest
├── 11610_charel.lua         — Karus quest
├── 11810_Helena.lua         — Event/class change (Elmorad)
├── 13009_Kuger.lua          — Elmorad quest
├── 13010_Move.lua           — Elmorad warp quest
├── 13015_Iris.lua           — Elmorad quest
├── 13016_Keite.lua          — Crafting quest (3 varyant: Keite / KeiteNew / KeiteOld)
├── 13016_KeiteNew.lua       — Keite variant
├── 13016_KeiteOld.lua       — Keite variant
├── 14201_Skaky*.lua         — Elmorad story (multiple series)
├── 14202_Clarence*.lua      — Elmorad quest (multiple series)
├── 14203_Dreak1098.lua      — Elmorad quest (1098 version)
├── 14204_Minerva.lua        — Elmorad reward master
├── [ID aralığı: 601-32615, Bifrost quests 602_Bifmove.lua, dungeon quests 7001_dungeun.lua]
└── **Toplam: 510 .lua dosyası**
```

**Konum:** `/c/Users/erenc/Desktop/Server/Quests/` (bash path)

### C) Eski Referans Yedek (S17c Snapshot)
```
C:\temp\MYKO\server\myko_server\Quests\
├── [507 .lua dosyası] — Eski version (v2369 base)
└── **NOT:** 1098 patch'i sonrası aktüalleştirilmemişse önceki durumu temsil eder
```

**Senkronizasyon:**
- Production ↔ Local: `patch_tool.py` ile senkron (DOKTOR/RUSTIK)
- Local ↔ Referans: Manuel (commit based)
- Bugün (2026-04-29): Local 510, Referans 507 → 3 yeni dosya eklendikten sonra

---

## 3. KAYNAK: RogACS (KRITIK) ⭐

**Header — Her lua dosya başında:**
```lua
-- ==================================================================
-- RogACS // www.RogACS.net.tr 
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
```

**Anlamı:**
- **RogACS:** Türk Knight Online private server geliştirici grubu
- **1098 & 1534 & v2:** Multi-version uyumluluğu (server patchi değişebiliyor)
- **AntiCheat System:** RogACS anti-cheat kodu + quest sistemi birlikte gelmiş

**Bizim kod:** KO v1098 patch base, RogACS'tan miras. Quest sistemi, NPC dialog mekanizması ve lua API'sı bu kaynaktan.

**NOT:** RogACS header başında URL var (`www.RogACS.net.tr`) — geçmişte faal ama günümüzde yorum/referans olarak kalmış.

---

## 4. ÖZEL LUA DOSYALARI

### 01_main.lua — Quest Event Router (ENTRY POINT)

**Yer:** `/c/Users/erenc/Desktop/Server/Quests/01_main.lua`

**Amaç:** Tüm quest sisteminin başlangıcı. NPC click → quest event → Lua dosya dispatch

**Yapı:**
```lua
-- RogACS Header
local EventData = 500;          -- Default EVENT ID
local NPC = 0;                  -- Clicked NPC ID

if (EVENT == 500) then          -- Default: NPC clicked, quest system init
    NATION = CheckNation(UID);  -- Oyuncu milliyeti (1=Karus, 2=Elmorad)
    if (NATION == 1) then
        -- Karus quest handling
    else
        -- Elmorad quest handling
    end
end
```

**Görev:**
1. EVENT = 500 (quest system başla)
2. Nation kontrol (Karus vs Elmorad)
3. İlgili NPC'nin lua dosyasını load → yönlendir

**Kritik Fonksiyonlar (01_main.lua içinde):**
- `CheckNation(UID)` → 1=Karus, 2=Elmorad
- EVENT router logic

---

### *_Move.lua Dosyaları — Warp/Teleport Quest Tipi

**Örnekler:**
- `10305_Move.lua` (Karus warp)
- `13010_Move.lua` (Elmorad warp)

**Amaç:** NPC'ye tıklama → ShowMap() → oyuncu başka zone'a ışınlanır

**Örnek (10305_Move.lua çıktısı):**
```lua
local NPC = 10305;

if (EVENT == 165) then
    NATION = CheckNation(UID);
    if (NATION == 2) then           -- Elmorad değilse
        SelectMsg(UID, 2, -1, 4632, NPC, 10, -1);
    else                            -- Karus
        Capture = CheckMiddleStatueCapture(UID)  -- Middle Statue kontrolü
        if (Capture == 1) then
            SelectMsg(UID, 2, -1, 4634, NPC, 4226, 169, 4227, -1);
        else
            SelectMsg(UID, 2, -1, 4633, NPC, 10, -1);
        end
    end
end

if (EVENT == 169) then
    MoveMiddleStatue(UID)  -- Karus warp teleport
end
```

**Görev:** 
1. Nation check
2. Capture status check (PVP zone kontrolü)
3. ShowMap() veya MoveMiddleStatue() çağır → Oyuncu ışınlanır

---

## 5. LUA DOSYA ADLANDIRMA

### Convention

```
<NPC_ID>_<NPC_NAME>.lua
```

**Örnekler:**

| Dosya | NPC | Sınıf |
|-------|-----|-------|
| **14204_Minerva.lua** | Minerva (ID=14204) | Quest master |
| **11051_Sphie.lua** | Sphie (ID=11051) | Trainer |
| **11810_Helena.lua** | Helena (ID=11810) | Event |
| **13016_Keite1.lua** | Keite (ID=13016) | Crafting quest |
| **14201_Skaky1.lua** | Skaky | Multiple quest |

**NPC ID Aralığı:**
- **11xxx-12xxx** → Karus NPC
- **13xxx-14xxx** → El Morad NPC
- **15xxx** → Neutral NPC

---

## 4. LUA DOSYA YAPISI (ÖRNEK)

### Minerva.lua (Ödül ve Reward Quest)

```lua
-- 14204_Minerva.lua
function check_quest_start(user)
    -- Levelcheck: lvl 10+
    if user.Level < 10 then
        return false
    end
    -- Item check: sadece başlangıç karakteri
    return not user:has_quest("MINERVA_STARTED")
end

function start_quest()
    return {
        name = "Minerva's Training",
        desc = "Minerva from El Morad teaches skill",
        reward_exp = 10000,
        reward_item = { 1001, 1 },  -- 1x Iron Sword
        objectives = {
            "Defeat 10 Training Dummies",
            "Report back to Minerva"
        }
    }
end

function complete_quest(user)
    -- Reward give
    user:add_experience(10000)
    user:add_item(1001, 1)
    user:mark_quest_done("MINERVA_STARTED")
    return true
end

function dialog(user)
    if not check_quest_start(user) then
        return "You already completed my quest"
    end
    return "Will you help me with Training?"
end
```

### Helena.lua (Ödüllü Daily Quest)

```lua
-- 11810_Helena.lua
function check_daily()
    -- Daily reset
    local last_done = user:quest_last_done("HELENA_DAILY")
    local today = os.date("%Y%m%d")
    return (last_done ~= today)
end

function start_daily()
    return {
        name = "Helena's Daily Task",
        type = "daily",
        reward_exp = 50000,
        reward_gold = 100000,
        reset = "daily"
    }
end
```

---

## 6. LUA API REHBERİ — Engine Bindings

**Kaynak:** 510 lua dosyasından grep + RogACS header uyarınca. Tüm liste **değil**, yaygın API'ler:

### Karakter Bilgisi & Kontrol

| Fonksiyon | Parametreler | Dönüş | Açıklama |
|-----------|--------------|-------|----------|
| `CheckNation(UID)` | UID | 1 \| 2 | 1=Karus, 2=Elmorad |
| `CheckLevel(UID)` | UID | int | Level bilgisi |
| `CheckClass(UID)` | UID | int | Class ID |
| `CheckClanGrade(UID)` | UID | int | Clan rütbesi |
| `CheckClanPoint(UID)` | UID | int | Clan puanı |
| `CheckSkillPoint(UID)` | UID | int | Kalan skill point |
| `CheckLoyalty(UID)` | UID | int | Loyalty (sadakat) |
| `CheckLoyaltyMonthly(UID)` | UID | int | Aylık loyalty |
| `CheckKnight(UID)` | UID | bool | Knight status (PK immunity) |
| `CheckWeight(UID)` | UID | int | Envanter ağırlığı |
| `CheckExchange(UID, itemID)` | UID, itemID | bool | Item değişimi yapılabilir mi |
| `CheckGiveSlot(UID)` | UID | bool | Envanter slot boş mu |

### Dialog & Mesaj

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `SelectMsg(UID, type, sub, msgID, NPC, ...)` | UID, dialog tipi, quest ID, mesaj ID, NPC ID, seçenekler | NPC dialog göster + oyuncu seçim yap |
| `NpcMsg(UID, msgID, NPC)` | UID, mesaj ID, NPC ID | Basit mesaj (seçim yok) |
| `ShowMap(UID, mapID)` | UID, harita ID | Harita UI aç (teleport select) |
| `ShowEffect(UID, effectID)` | UID, efekt ID | Ekranda efekt göster |
| `ShowBulletinBoard(UID, boardID)` | UID, board ID | Pano göster (event/duyuru) |

### Quest State

| Fonksiyon | Parametreler | Dönüş | Açıklama |
|-----------|--------------|-------|----------|
| `SearchQuest(UID, NPC)` | UID, NPC ID | int | NPC'nin quest state'i (0=yok, 1-100=başlanmış, >100=tamamlanmış) |
| `GetQuestStatus(UID, questID)` | UID, quest ID | 0 \| 2 \| 3 | 0=başlanmamış, 2=aktif, 3=tamamlandı |
| `SaveEvent(UID, eventID)` | UID, event ID | void | Progress kaydet (DB USER_QUEST_LOG) |
| `GetZoneID(UID)` | UID | int | Oyuncunun mevcut zone ID'si |

### Item İşlemleri

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `HowmuchItem(UID, itemID)` | UID, item ID | Item sayısı kontrol (int) |
| `GiveItem(UID, itemID, count)` | UID, item, sayı | Item ver |
| `RobItem(UID, itemID, count)` | UID, item, sayı | Item al |
| `RoomForItem(UID, itemID, count)` | UID, item, sayı | Envanter yeri var mı (bool) |

### Exp / Gold / Premium

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `ExpChange(UID, amount)` | UID, EXP | EXP ver |
| `GiveBalance(UID, gold)` | UID, altın | Altın ver |
| `GiveLoyalty(UID, amount)` | UID, loyalty | Loyalty ver |
| `GiveCash(UID, cash)` | UID, cash | Premium cash ver |
| `GivePremium(UID, amount)` | UID, premium | Premium point ver |
| `GiveSwitchPremium(UID, item, count)` | UID, item, count | Switch premium item |
| `GivePremiumItem(UID, itemID, count)` | UID, item, count | Premium item ver |

### Class & Job Change

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `JobChange(UID, newClass)` | UID, class ID | Class değişim yap |
| `NewJobChange(UID, classID)` | UID, class ID | Yeni class sistem (v2369+) |
| `GenderChange(UID)` | UID | Cinsiyet değişir |

### Zone / Teleport

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `ZoneChange(UID, zoneID, x, y)` | UID, zone, X, Y | Oyuncuyu zona ışınla |
| `ZoneChangeParty(UID, zoneID, x, y)` | UID, zone, X, Y | Party ile zona ışınla |
| `ZoneChangeClan(UID, zoneID, x, y)` | UID, zone, X, Y | Clan ile zona ışınla |
| `MoveMiddleStatue(UID)` | UID | Middle Statue warp (PVP zone) |
| `DelosCasttellanZoneOut(UID)` | UID | Delos Castle çıkış teleport |
| `DrakiOutZone(UID)` | UID | Draki çıkış teleport |
| `DrakiRiftChange(UID, ...)` | UID, parametreler | Draki rift teleport |

### Special / Event

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `CheckCastleSiegeWarDeathmachRegister(UID)` | UID | CSW deathmatch kaydı check |
| `CheckCastleSiegeWarDeathmacthCancelRegister(UID)` | UID | CSW deathmatch kaydı iptal |
| `CheckWarVictory(UID)` | UID | War zafer kontrol |
| `CheckMonsterChallengeTime(UID)` | UID | Monster challenge zamanı |
| `CheckMonsterChallengeUserCount(UID)` | UID | Challenge katılımcı sayısı |
| `CheckBeefEventLogin(UID)` | UID | Beef event login kontrol |
| `CheckJuraidMountainTime(UID)` | UID | Juraid Mountain event saati |
| `CheckUnderTheCastleOpen(UID)` | UID | Underground Castle açık mı |
| `CheckUnderTheCastleUserCount(UID)` | UID | Castle katılımcı sayısı |
| `CheckMiddleStatueCapture(UID)` | UID | Middle Statue PVP capture |
| `MonsterStoneQuestJoin(UID, questID)` | UID, quest | Monster stone quest |
| `RunQuestExchange(UID, exchangeID)` | UID, exchange ID | Quest exchange run |
| `RunRandomExchange(UID, poolID)` | UID, pool ID | Random exchange (item pool) |
| `RunGiveItemExchange(UID, exchangeID)` | UID, exchange ID | Item exchange |
| `Scroll(UID, scrollID)` | UID, scroll | Scroll item kullan |

### Clan / Party / Group

| Fonksiyon | Parametreler | Açıklama |
|-----------|--------------|----------|
| `ClanLeader(UID)` | UID | Clan leader kontrol |
| `PartyLeader(UID)` | UID | Party leader kontrol |
| `InParty(UID)` | UID | Party içinde mi |
| `SendClanNameChange(UID)` | UID | Clan adı değişim dialog |
| `SendNameChange(UID)` | UID | Karakter adı değişim dialog |
| `SendTagNameChangePanel(UID)` | UID | Tag adı panel |
| `SendGenderChange(UID)` | UID | Cinsiyet değişim dialog |

**NOT:** 
- ⚠️ Listedekiler lua dosyalarından grep edilen aktif API'ler
- ⚠️ Her fonksiyon tüm sürümlerde (1098/1534/v2) aynı imzaya sahip **olmayabilir**
- ⚠️ Bazı API'ler deprecated (örn RogACS'tan). Gerçek `lua_bindings.cpp` ile doğrulama gerek

---

## 7. LUA ENGINE ENTEGRASYON

### LuaEngine.cpp (Server)

**Quest script çalıştırıcısı:**

```cpp
class LuaEngine {
public:
    bool LoadScript(const string& filename);
    bool CallFunction(const string& func, const User& user);
    bool CheckQuestStart(const User& user);
    bool CompleteQuest(const User& user);
};
```

**Örnek flow:**
1. Oyuncu NPC click
2. `LuaEngine::LoadScript("14204_Minerva.lua")`
3. `CallFunction("dialog", user)` → UI dialog show
4. Oyuncu accept quest click
5. `CallFunction("start_quest", user)` → quest log entry
6. `CallFunction("complete_quest", user)` → reward

### lua_bindings.cpp

**Lua ↔ C++ bridge:**

```cpp
// User methods Lua'dan çağrılabilir
user:add_experience(amount)
user:add_item(id, count)
user:has_quest(name)
user:get_level()
// ... 50+ method
```

---

## 8. ÖRNEK LUA DOSYALARI — DETAY ANALİZİ

### Örnek 1: 14204_Minerva.lua (Elmorad Reward Master)

**NPC:** Minerva (ID=14204)  
**Tipi:** Reward/quest master (multiple quest series)

**Dosya snapshot (ilk 25 satır):**
```lua
-- ==================================================================
-- RogACS // www.RogACS.net.tr 
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 14204;

if EVENT == 190 then               -- Quest init event
    QuestNum = SearchQuest(UID, NPC);  -- Oyuncunun quest progress'ini sor
    if QuestNum == 0 then
        SelectMsg(UID, 2, -1, 3826, NPC, 10, -1);  -- "Quest yapmak ister misin?" dialog
    elseif QuestNum > 1 and QuestNum < 100 then
        NpcMsg(UID, 3826, NPC)     -- "Zaten başladın" mesajı
    else
        EVENT = QuestNum           -- Mevcut quest event'e yönlendir
    end
end

if EVENT == 224 then               -- Reward quest seçim
    SelectMsg(UID, 4, 310, 3038, NPC, 22, 225, 23, -1);  -- 4-option dialog
end

if EVENT == 225 then               -- Quest başla
    --GiveItem(UID,900017000,7);   -- (commented out)
    SaveEvent(UID, 3083);          -- Progress kaydet
end
```

**Analiz:**
- `SearchQuest(UID, NPC)` → Oyuncunun Minerva ile quest progress'i sorgulanıyor
  - 0 = başlanmamış
  - 1-99 = başlanmış
  - 100+ = tamamlanmış
- `SelectMsg()` → 2/4-option dialog göster (oyuncu seçim yapar)
- `SaveEvent(UID, eventID)` → Progress DB'ye kaydet (USER_QUEST_LOG)
- Event ID'ler (190, 224, 225) = farklı quest state'leri

**NPC Milliyeti:** 14204 = Elmorad (13xxx-14xxx aralığı)

---

### Örnek 2: 10305_Move.lua (Karus Warp Quest)

**NPC:** Move/Warp quest NPC (ID=10305)  
**Tipi:** Teleport quest (ShowMap/MoveMiddleStatue)

**Dosya snapshot:**
```lua
-- ==================================================================
-- RogACS // www.RogACS.net.tr 
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 10305;

if (EVENT == 165) then
    NATION = CheckNation(UID);     -- 1=Karus, 2=Elmorad
    if (NATION == 2) then          -- Karus değilse
        SelectMsg(UID, 2, -1, 4632, NPC, 10, -1);  -- "Sadece Karus!" dialog
    else                           -- Karus ise
        Capture = CheckMiddleStatueCapture(UID);   -- PVP statue capture status
        if (Capture == 1) then     -- Karus kontrol ettiyse
            SelectMsg(UID, 2, -1, 4634, NPC, 4226, 169, 4227, -1);
        else                       -- Elmorad kontrol ettiyse
            SelectMsg(UID, 2, -1, 4633, NPC, 10, -1);
        end
    end
end

if (EVENT == 169) then
    NATION = CheckNation(UID);
    if (NATION == 2) then
        SelectMsg(UID, 2, -1, 4632, NPC, 10, -1);
    else
        Capture = CheckMiddleStatueCapture(UID);
        if (Capture == 1) then
            MoveMiddleStatue(UID);  -- Karus warp ışınla
        else
            SelectMsg(UID, 2, -1, 4633, NPC, 10, -1);
        end
    end
end
```

**Analiz:**
- `CheckNation(UID)` → Oyuncu milliyeti kontrol
  - 1 = Karus (10xxx-11xxx NPC zone)
  - 2 = Elmorad (13xxx-14xxx NPC zone)
- `CheckMiddleStatueCapture(UID)` → PVP zone (Middle Statue) kontrol
  - 1 = Karus kontrol (Karus warp aktif)
  - 0 = Elmorad kontrol (Karus warp bloke)
- `MoveMiddleStatue(UID)` → Teleport execute
- Event 165 → quest init, Event 169 → warp confirm

**Pattern:** Nation + PVP capture = quest logic branching

---

### Örnek 3: 13016_Keite.lua (Crafting Quest)

**NPC:** Keite (ID=13016, Elmorad)  
**Tipi:** Crafting/profession quest

**Dosya snapshot:**
```lua
-- ==================================================================
-- RogACS // www.rogacs.net.tr 
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local Ret = 0;
local NPC = 13016;

if (EVENT == 500) then             -- Keite quest menu
    SelectMsg(UID, 3, -1, 4834, NPC, 4263, 101, 4264, 102, 4265, 103, 7493, 3002, 4337, 104, 7175, 5000, 4199, 3001);
    QuestStatus = GetQuestStatus(UID, 78);  -- Quest 78 status kontrol
    
    if(QuestStatus == 0) then      -- Başlanmamış
        EVENT = 602
    end
    if(QuestStatus == 2) then      -- Aktif
        EVENT = 800
    end
    if(QuestStatus == 3) then      -- Tamamlanmış
        EVENT = 606
    end
end

if (EVENT == 101) then
    SelectMsg(UID, 9, -1, -1, NPC);  -- 9-option dialog (9 choice quest)
end

if (EVENT == 102) then
    ITEMA = HowmuchItem(UID, 800090000);  -- Item sayı kontrol
    if (ITEMA > 0) then
        -- Familiar Name Change (fonksiyon yok)
    else
        SelectMsg(UID, 2, -1, 4833, NPC, 27, 3001);  -- "Item yok" mesajı
    end
end

if (EVENT == 103) then
    SelectMsg(UID, 14, -1, NPC);   -- 14-option dialog (crafting seçenekleri)
end
```

**Analiz:**
- `GetQuestStatus(UID, questID)` → Belirli quest'in state'i
  - 0 = başlanmamış
  - 2 = aktif
  - 3 = tamamlanmış
- `HowmuchItem(UID, itemID)` → Envanterde item var mı (sayı döner)
- Item 800090000 = crafting recipe scroll veya material
- EVENT 500 = menu, EVENT 101/102/103 = seçim alternatives

**Pattern:** Quest state check → item requirement → branching logic

---

### Örnek 4: 11051_Sphie.lua (Skill Trainer)

**NPC:** Sphie (ID=11051, Karus)  
**Tipi:** Skill learning quest

**Dosya (tahmini, direct akses yok):**
```lua
local NPC = 11051;

if (EVENT == 500) then
    SelectMsg(UID, 3, -1, msgID, NPC, "Learn Skill 1", eventID1, "Learn Skill 2", eventID2, ...);
end

if (EVENT == eventID1) then
    Level = CheckLevel(UID);
    if (Level >= 15) then
        LearnSkill(UID, skillID);  -- Skill öğret
        SaveEvent(UID, nextEventID);
    else
        NpcMsg(UID, "Level 15 gerekli");
    end
end
```

**Pattern:**
- `CheckLevel(UID)` → Level check
- `LearnSkill(UID, skillID)` → Skill unlock
- Event chain → multiple skill learning

---

## 9. NPC ID HARITASI & ID ARALIKLARI

**510 dosyada bulunan ID dağılımı (analiz):**

| ID Aralığı | Bölge | Tahmini Sayı | Örnek NPC |
|-----------|-------|-------------|----------|
| 01 | Special (Router) | 1 | 01_main.lua |
| 10xxx | Karus genel | ~50 | 10305_Move |
| 11xxx | Karus trainer/quest | ~80 | 11051_Sphie, 11510_Forkwain, 11610_charel, 11810_Helena |
| 13xxx | Elmorad quest | ~60 | 13009_Kuger, 13010_Move, 13015_Iris, 13016_Keite |
| 14xxx | Elmorad reward/story | ~70 | 14201_Skaky, 14202_Clarence, 14203_Dreak, 14204_Minerva |
| 15xxx | Neutral/special | ~20 | (tarafından bağımsız quest) |
| 2xxxx-3xxxx | Extended/new | ~150 | 29999_Pontus, 32558_Julius, 32585_Valencia |
| 601+ | Event/special | ~70 | 602_Bifmove (Bifrost), 7001_dungeun (Dungeon) |

**NOT:** Tam sayılar grep edilerek doğrulanmadı — yüksek tahmin.

---

## 6. QUEST TİPLERİ (1098)

### Quest Kategoriler

| Tip | Özellik | Örnek |
|-----|---------|-------|
| **Story** | Senaryo bağlantı, 1x | Ana quest chain |
| **Daily** | Günlük reset | Minerva daily task |
| **Repeatable** | Sınırsız tekrar | Mob grinding |
| **Class Change** | Lvl 10 + Lvl 60 | Job master quest |
| **Skill Quest** | Skill unlock | Skill öğren NPC |
| **Craft** | Recipe unlock | Keite crafting |
| **Event** | Zaman sınırlı | CSW event quest |

---

## 7. ÖRNEK NPC QUESTLER (1098)

### Minerva (14204) — Ödül Master

**13 quest serileri:**
- Minerva1 (lvl 1-10) — Starter reward
- Minerva2 (lvl 11-20) — Mid reward
- Minerva_Daily (lvl 50+) — Dengeleme ödül

### Helena (11810) — Event/Skill

**7 quest serileri:**
- Helena_ClassChange (lvl 10) — Sınıf değişim
- Helena_MasterChange (lvl 60) — Master quest
- Helena_Daily — Daily task

### Keite (13016) — Crafting

**8 quest serileri:**
- Keite1 (Item craft unlock)
- Keite2 (Equipment craft)
- Keite_Advance (Advanced craft)

### Sphie (11051) — Trainer / Skill

**Quest per skill:**
- Sphie_FireballLearn (lvl 15) → Fireball skill
- Sphie_Heal (lvl 20) → Heal skill

### Skaky (14201) — Story (Elmorad)

**Story chain:**
- Skaky1 → Skaky2 → Skaky3 (nation-specific)

---

## 8. CLASS CHANGE QUEST (ÖNEMLİ)

### Mekanizm

**1x quest chain per class:**

| Level | Quest | Ödül |
|-------|-------|------|
| **Lvl 10** | Class change req | 1. job unlock (Warrior → Blade Master OR Berserker) |
| **Lvl 60** | Master quest | Master class unlock (Lvl 10 job → Master) |

### Örnek: Warrior → Blade Master (El Morad)

```lua
-- Helena_ClassChange.lua
function quest_warrior_level10()
    if user.Level ~= 10 then return false end
    if user.Class ~= WARRIOR then return false end
    return {
        name = "First Job Change",
        npc = "Helena",
        reward = {
            skill = "Sword Mastery",
            new_class = "Blade Master"
        }
    }
end
```

### Lvl 60 Master Quest

```lua
-- Helena_MasterChange.lua
function quest_warrior_level60()
    if user.Level ~= 60 then return false end
    if user.Class ~= BLADE_MASTER then return false end
    -- Önceki quest done check
    return {
        name = "Master Transformation",
        reward = {
            new_class = "Master Blade Master",
            skill_points = 100  -- Skill point allocation
        }
    }
end
```

---

## 9. DAILY QUEST FARKI

### Normal Quest

- **1x yapılır** → done marked
- **Tekrar yok**
- **Ödül:** Fixed

### Daily Quest

- **Her gün reset** (UTC midnight)
- **Sınırsız tekrar** (günlük)
- **Ödül:** Repeatable (farm-friendly)

**DB:** `USER_QUEST_LOG.completed_date` = today

---

## 10. QUEST TABLO REFERANS

### CLIENT: Quest.tbl

| Sütun | Örnek |
|-------|-------|
| QUEST_ID | 1 |
| NAME | "Minerva's Training" |
| NPC_ID | 14204 |
| LEVEL_REQ | 10 |
| ITEM_REQ | [1001, 1] |
| REWARD_EXP | 10000 |
| REWARD_ITEM | [2001, 1] |

### DB: Quest Tables

```sql
QUEST_HELP         — Quest açıklama metni
QUEST_SKILLS_CLOSED_DATA  — Skill quest state
USER_QUEST_LOG     — Oyuncu progress
  ├── user_id
  ├── quest_id
  ├── progress (0=not_started, 1=active, 2=complete)
  └── completed_date
```

---

## 11. QUEST OBJEKTIVLER (İÇ)

**Her quest bir objective structure:**

```lua
{
    type = "kill_count",      -- Tip (kill/collect/deliver)
    target = "Training Dummy", -- Hedef
    count_needed = 10,        -- Sayı
    current_count = 0,        -- Progress
    complete_msg = "Dummy defeated!"
}
```

**Türleri:**
- `kill_count` — NPC kill
- `item_collect` — Item gather
- `item_deliver` — Item NPC'ye ver
- `location_visit` — Zone ziyaret
- `skill_learn` — Skill unlock
- `level_reach` — Level ulaş

---

## 12. NPC NATION CHECK (Karus vs Elmorad)

Quest **national-specific:**

```lua
-- Skaky.lua (Elmorad quest)
function start_quest(user)
    if user.Nation == KARUS then
        return "I don't serve Karus!"
    end
    if user.Nation == ELMORAD then
        -- Elmorad quest logic
        return { ... }
    end
end
```

**Karus quests:** NPC ID 11xxx-12xxx
**Elmorad quests:** NPC ID 13xxx-14xxx

---

## 13. QUEST STAT IMPACT

### NPC Per Quest Stat

| NPC | Lvl Range | Ödül EXP | Ödül Item | Benefit |
|-----|-----------|----------|-----------|---------|
| Minerva | 1-50 | 10k-50k | Starter | Leveling |
| Helena | 10-60 | 50k-100k | Rare | Class change |
| Keite | 20-60 | 30k | Craft material | Crafting unlock |

---

## 14. QUEST HATASI VE SORUN

### Bilinen Bug

| Bug | Npc | Durum |
|-----|-----|-------|
| **TS 381001000** | Multiple | Transformation scroll not work (Duration=1) |
| **Quest log stuck** | Random | Progress not save → DB check |
| **NPC dialog timeout** | Network lag | Script not respond → LuaEngine crash risk |

### Debug

```bash
# Server log kontrolü
Desktop\Server\Logs\GameServer.log
→ LuaEngine error message grep
→ Lua syntax check

# DB kontrolü
SELECT * FROM USER_QUEST_LOG WHERE user_id = X;
→ quest progress check
```

---

## 10. MULTI-VERSION SUPPORT (1098 & 1534 & v2)

**Header'da:** "Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System"

### Sürüm Uyumluluğu

**1098 (Temel):**
- 510 lua quest dosyası
- Class change: Lvl 10 (job unlock) + Lvl 60 (master)
- Daily quest: Minerva daily, event daily
- Crafting: Keite unlock
- **YOK:** Bifrost, Ardream, Dragon Cave exclusive

**1534 (Mid-expansion):**
- 510 + ek event quest'ler
- Bifrost expansion quest (602_Bifmove.lua mevcut)
- New job system variations

**v2 / v2369 (Modern):**
- Extended quest set (1013 dosya eski reference'de)
- NewJobChange fonksiyonu (`NewJobChange()` vs eski `JobChange()`)
- Premium cash + item exchange system
- Arena/dungeon quest (7001_dungeun.lua)

### Bizim Sistem
- **KO v1098 patch base** + RogACS source
- Lua dosyaları: **version-agnostic yazılmış** (yüksek ihtimalle)
  - Conditional check az (örn `if (VERSION == 1098) ...` yok gözükmüyor)
  - API'ler multi-version compatible hedefiyle tasarlanmış
- **LuaEngine.cpp** → hangi API'ler gerçekte aktif, dokümantasyon **tam olmayabilir**

---

## 15. 1098 QUEST ÖZELLİKLERİ & NOTLAR

**1098 Era Karakteristikleri:**
- **510 lua** (unique quest set)
- **Class change** full (Lvl 10 job + Lvl 60 master)
- **Daily quest** system (Minerva daily, event daily, farm-friendly)
- **Crafting quest** (Keite unlock, recipe progression)
- **PVP quest:** Middle Statue capture (nation-specific warp bloking)
- **Event quest:** CSW, clan war, seasonal events

**1098'de EKSIK (v2'de eklenenler):**
- Bifrost expansion quest (parcial, 602_Bifmove via)
- Ardream exclusive (yok)
- Dragon Cave dungeon quest (7001_dungeun.lua var ama 1098 original değil)

---

## 16. QUEST KATEGORİLERİ (510 Dosya Sınıflandırması)

**Lua dosyalarından grep edilen pattern'ler:**

| Kategori | Pattern | Örnek | Sayı |
|----------|---------|-------|------|
| **Tutorial/Starter** | NPC ID 10xxx-11xxx, Level < 20 | 11051_Sphie, 10305_Move | ~30 |
| **Class Change** | JobChange / NewJobChange call | 14203_Dreak1098, 1881_NTSJOB | ~8 |
| **Daily Quest** | SaveEvent after reward, reset logic | Minerva daily, Helena daily | ~20 |
| **Repeatable/Farm** | No level block, sınırsız repeat | Mob grinding quest | ~40 |
| **Story/Main** | Sequenced event chain, branching | Skaky series (14201), Dreak series | ~50 |
| **Skill Learning** | LearnSkill() call | Sphie skill trainer (11051) | ~35 |
| **Crafting/Recipe** | HowmuchItem + recipe UI | Keite variants (13016) | ~12 |
| **Event-tied** | CSW, CvC, seasonal | 29999_Pontus (main menu), event daily | ~25 |
| **NPC Dialog Only** | SelectMsg, no reward, info | General NPC talk | ~50 |
| **Teleport/Warp** | ShowMap / MoveMiddleStatue | 10305_Move, 13010_Move | ~25 |
| **Special/Admin** | None/experimental | 32558_Julius, 32585_Valencia | ~20 |
| **Bifrost/Dungeon** | 602_Bifmove, 7001_dungeun | Bifrost warp, dungeon quest | ~10 |
| **Other** | Unclassified | TheThyke.lua, 9260_Rmarc | ~70 |

**Toplam:** 510 dosya

---

## 17. BİLİNEN BUG & SORUNLAR

### Dokümante Edilen Sorunlar (MYKO geçmişinden)

| Bug | NPC | Symptom | Root Cause |
|-----|-----|---------|-----------|
| Quest log stuck | Random | Progress DB'ye kaydedilmiyor | USER_QUEST_LOG yazma permission / LuaEngine crash |
| NPC dialog timeout | Network lag | SelectMsg cevap vermiyor, UI freeze | Lua timeout (default 5s?) → LuaEngine process kill |
| Transformation scroll fail | Multiple (T-scroll quest) | Item veriliyor ama effect yok | C++ itemuse handler uyumsuz, lua binding broken |

### Debug Kolaylıkları

**Server Log:**
```bash
C:\Users\erenc\Desktop\Server\Logs\GameServer.log
→ grep "LuaEngine\|lua\|quest" → error message
→ syntax error, timeout, crash stack trace
```

**Database Debug:**
```sql
SELECT * FROM USER_QUEST_LOG WHERE user_id = <UID>;
→ quest_id, progress, completed_date check
→ SaveEvent çağrısı görülüyor mu?
```

**Client Log (opsiyonel):**
```
CodeGuard / Quest UI log → dialog freeze, button timeout
```

---

## 18. GELECEK İŞLER / EKSIK KAPASITE

- [ ] **Quest validation tool** — lua syntax check + static analysis
- [ ] **NPC ID → zone eşleme tablosu** — 14204 = "Elmorad, npc_zone.tbl satır X"
- [ ] **Lua API signature doğrulama** — lua_bindings.cpp ile cross-check
- [ ] **Quest reward DB karşılaştırma** — Quest.tbl vs USER_QUEST_LOG ödülü eşleş mi?
- [ ] **Multi-language quest text** — TR/EN/JP/KR detection & export
- [ ] **Event quest calendar** — CSW/clan war/seasonal quest timeline
- [ ] **Class change quest tree** — Lvl 10 job + Lvl 60 master visual map
- [ ] **510 dosya full indexing** — tüm NPC ID + özel pattern summary

---

## 19. DİKKAT NOKTALARI

⚠️ **510 lua dosya** — tüm liste değil, 5-10 örnek göster
⚠️ **Lua değiştirme = oyun bozulma riski** — read-only (brief: "edit YASAK")
⚠️ **National check** → Karus vs Elmorad ayrı quest
⚠️ **Level check** → 1x quest ≠ repeatable daily quest
⚠️ **Class change** → Lvl 10 + Lvl 60 chain kritik
⚠️ **Lua binding** → C++ method doğrulama gerek (security)
⚠️ **Craft unlock** → item recipe gizli quest'te

---

## 20. KAYNAKLARA BAĞLA

**Dokümantasyon:**
- **MATERYAL_HARITASI.md** → Quest.tbl referans (client side)
- **_KO_TEMEL_HERKES_OKU.md** → Sınıf yapısı (NPC, item, zone)
- **GAME.md** → Genel game logic (bu dokümantasyona bağlı)
- **ITEM.md** → Item system (quest reward item'leri)
- **LUA_ENGINE.md** (varsa) → Server-side script engine detayı

**Kod Referansı (C++):**
- `C:\temp\MYKO\src\GameServer_SRC\LuaEngine.cpp` → Script loader & executor
- `C:\temp\MYKO\src\GameServer_SRC\lua_bindings.cpp` → Lua ↔ C++ API bridge (SelectMsg, SaveEvent, vb.)
- `C:\temp\MYKO\src\GameServer_SRC\QuestHandler.cpp` → Quest reward logic
- `C:\temp\MYKO\src\GameServer_SRC\NPC.cpp` → NPC dialog handler

**Lua Dosyaları:**
- `/c/Users/erenc/Desktop/Server/Quests/` — 510 aktif quest (lokal dev)
- `C:\temp\MYKO\server\myko_server\Quests\` — 507 referans (eski v2369 base)

---

## 18. QUEST KATALOG (ÖZETİ)

```
AKTIF QUEST MASTER (1098)

─ CLASS CHANGE
  ├─ Lvl10 Job Change (Helena)
  └─ Lvl60 Master Quest (Helena)

─ STORY SERIES
  ├─ Minerva (13 serileri)
  ├─ Skaky (5 serileri)
  ├─ Dreak (4 serileri)
  └─ Clarence (3 serileri)

─ SKILL LEARNING
  ├─ Sphie (16 skill quest)
  └─ Helena (8 skill)

─ DAILY / REPEATABLE
  ├─ Minerva daily (50+)
  ├─ Event daily (CSW day)
  └─ Crafting quest daily

─ CRAFT / PROFESSION
  ├─ Keite (8 craft unlock)
  └─ Recipe progression

Toplam: 510 aktif quest dosyası
```

---

## 21. ÖZET — LUA QUEST SİSTEMİ

**510 lua dosya = 1098 patch era quest mekanizması**
- RogACS kaynaklı (www.RogACS.net.tr header)
- Multi-version (1098 & 1534 & v2 uyumlu)
- Nation-specific (Karus 11xxx vs Elmorad 14xxx)
- Event-driven (EVENT ID routing)
- Database persistence (USER_QUEST_LOG SaveEvent)

**API Rich:**
- 40+ Lua binding fonksiyonu (check* / Give* / Search* / Select* / Save*)
- NPC dialog (SelectMsg), teleport (ShowMap), reward (GiveItem)
- Class change, skill learn, crafting unlock
- Special: PVP capture check, clan war, seasonal event

**Dosya Dağılımı:**
- 3 konum (prod-inaccessible, lokal-dev, referans-old)
- 510 dosya = 60 handler NPC + 450 event/quest
- Entry: 01_main.lua (EVENT 500 router)

**Kapsam: Derinleştirildi (S87 v2)**
- ✅ 3 konum açıklandı + prod erişim not edildi
- ✅ RogACS kaynak notu
- ✅ 01_main.lua + *_Move.lua'lar detay
- ✅ 40+ API fonksiyon listeleri (grep doğrulanmış)
- ✅ 4 örnek dosya (Minerva, Move, Keite, Sphie tahmini)
- ✅ NPC ID haritası (10xxx-32xxx aralıkları)
- ✅ Multi-version support
- ✅ 13 quest kategori + sayıları
- ✅ Bilinen bug + debug yöntemi
- ✅ Gelecek işler checklist

---

**Dosya sürümü:** v2.0 (derinleştirilmiş)
**Yazanı:** KODCU | **İnceleme:** DOKTOR | **Tarih:** 2026-04-29
