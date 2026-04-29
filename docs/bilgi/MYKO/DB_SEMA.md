# DB Şema — KO_MYKO (MalaysiaKO 1098)

**Tarih:** 2026-04-29 | **Versiyon:** 1.0 | **Agent:** MATRIX | **Session:** S87+

---

## Bağlantı Bilgisi

| Bilgi | Değer |
|-------|-------|
| **Server** | `localhost\MSSQLSERVER01` |
| **Port** | 1433 (default) |
| **Veritabanı** | `KO_MYKO` (oyun), `KO_MYKO_LOG` (log) |
| **ODBC DSN** | `KO_MAIN` (oyun), `KO_GAME` (game), `KO_LOG` (log) |
| **Driver** | SQL Server Native Client 11.0 |
| **User** | `sa` (sysadmin) / `semih_dev` / `myko_app` (limited) |
| **Auth** | SQL Server Authentication (password in env/config) |

---

## Ana Tablolar — Harita

| Tablo | Amaç | Row Count ~1098 | Kritik Kolonlar |
|-------|------|-----------------|-----------------|
| **USERDATA** | Oyuncu karakter | ~2K (testing) | CharId, AccountID, Level, Exp, Gold, Class, Nation, Authority |
| **USER_ITEM** | Envanter (tekil) | ~2K | CharId, bItem[144] (binary envanter) |
| **USER_ITEMS** | (BOŞTUR) | 0 | — |
| **USER_SKILL_BAR** | Skill kısayol | ~2K | CharId, SkillBar (binary) |
| **ITEM** | Item master | ~2100 | Num, ItemType, Kind, Class, Plus, Damage, Defense, BuyPrice, SellPrice, RequiredLevel |
| **ITEM_EXTENSION** | Item effect | ~600 | strEffect, iEffect |
| **ITEM_SELLTABLE** | NPC alış-satış | ~2K | iSellingGroup (PK1), nIndex (PK2), BuyPrice, SellPrice |
| **ITEM_UPGRADE** | Anvil +1..+9 | ~20 | nUpgradeLevel, SuccessRatio, DestroyRatio |
| **MAGIC_TABLE** | Skill ana | ~300 | dwMagicID, szMagicName, byMagicType, iMp, iCoolTime |
| **MAGIC_TYPE1-9** | Skill tipleri | ~300 | Skill effect detay (damage, heal, duration) |
| **KNIGHTS** | Klan master | ~50 | KnightsID, szName, MasterID, Notice |
| **KNIGHTS_USER** | Klan üyesi | ~200 | CharId, KnightsID, Rank (0=Knight..3=ChiefKnight) |
| **KNIGHTS_ALLIANCE** | Klan ittifak | ~5 | —— |
| **KNIGHTS_RANK** | Klan rütbe tanım | 4 | szRankName (Knight, Senior, Chief, Leader) |
| **CLAN_BANK** | Klan hesabı | ~50 | KnightsID, Gold, GB |
| **K_NPC** | NPC master | ~200 | MobNum, szName, byLevel, iHp, iMp, iDamage |
| **MONSTER** | Monster alt | ~200 | MobNum, Drop tablo ref |
| **NPC_ITEM** | NPC drop | ~500 | MobNum, ItemNum, Probability, Count |
| **ACCOUNT_CHAR** | Hesap-karakter link | ~2K | AccountID, CharId, Class, Nation, Level |
| **TB_USER** | Web login | ~200 | strID, strPassword, strWebHash (⚠️ NULL bug) |
| **GAME_MASTER_SETTINGS** | GM ayarı | 0-5 | CharId, Authority level log |
| **EVENT_OPEN** | Event durum | ~10 | szEvent, byEvent (0=off, 1=on) |
| **EVENT_REWARDS** | Event ödül | ~50 | EventId, ItemNum, Probability |
| **ZONE_INFO** | Zone detay | ~40 | ZoneID, szZoneName, iMinLevel, iMaxLevel, PvP flag |
| **BEGINNER_ITEM** | Başlangıç eşyası | ~10 | Class, ItemNum, Count |
| **USERDATA_FRIENDS** | Arkadaş listesi | ~5K | CharId, FriendCharId |
| **MAIL_BOX** | Oyuncu postası | ~2K | CharId, From, ItemNum, Gold, ReadFlag |
| **BANK** | Bank depo | ~1K | CharId, ItemNum, Count |
| **QUEST_HELP** | Quest açıkl. | ~500 | QuestID, szDesc (Lua tarafından ref) |
| **QUEST_SKILLS_CLOSED_DATA** | Skill quest | ~20 | CharId, SkillID, CompletedFlag |
| **USER_QUEST_LOG** | Quest progress | ~10K | CharId, QuestID, CompleteFlag, UpdateDate |
| **VERSION** | Patch version | ~5 | iCurVersion, iLastVersion, sFile (patch URL) |
| **LOG_CHAT** | Chat log | ~100K | CharId, szChat, TimeStamp, Nation |
| **LOG_TRADE** | Trade log | ~50K | FromCharId, ToCharId, ItemNum, Gold, TimeStamp |
| **LOG_QUEST** | Quest log | ~100K | CharId, QuestID, TimeStamp, Status |

---

## 🔴 USERDATA — En Kritik Tablo

### Kolon Listesi (tam)

```sql
CharId              INT PRIMARY KEY    — Oyuncu unique ID
AccountID           INT                — ACCOUNT_CHAR.AccountID foreign
wUserType           SMALLINT           — 0=normal, 9=admin
szName              VARCHAR(32)        — Karakter adı (unique)
byClass             TINYINT            — 0=Warrior, 1=Rogue, 2=Priest, 3=Mage, 4=Kurian, 5=Porutu
byRace              TINYINT            — 0=Karus, 1=El Morad/Humans
byNation            TINYINT            — Aynı byRace (1098'de ırk=nation)
iLevel              INT                — 1-72 (1098 cap)
iExp                INT / BIGINT       — Exp bar (0 → max, her level farklı)
iMaxExp             INT / BIGINT       — Level max exp
iHp                 INT                — Current HP
iMaxHp              INT                — Max HP
iMp                 INT                — Current MP
iMaxMp              INT                — Max MP
iStamina            INT / SMALLINT     — Enerji (hareket için)
iStaminaMax         INT / SMALLINT     — Max enerji
sMp                 SMALLINT           — Skill point (SP)
dMp                 SMALLINT           — Skill point dağılımı
byStr               TINYINT            — Strength stat (1-255)
byDex               TINYINT            — Dexterity
byInt               TINYINT            — Intelligence
byVit               TINYINT            — Vitality
byCha               TINYINT            — Charisma
byConc              TINYINT            — Concentration (mage)
fStr, fDex, fInt, fVit, fCha  FLOAT  — Decimal fractions
iDamage             INT                — Attack power
iAC                 INT / SMALLINT     — Armor class (defense)
iGold               BIGINT             — Noah (gold) quantity
iGoldBank           BIGINT             — Bank gold
iNationalPoint      INT                — National Point (PvP kıll sayı)
byAuthority         TINYINT            — 0=normal, 2=GM, 9=admin
iZoneID             INT                — Mevcut zone (1098war_a, etc)
iX, iY, iZ          INT                — Harita koordinat (binary coord sistem)
iDir                SMALLINT           — Yüz yönü (0-360)
byGender            TINYINT            — 0=Male, 1=Female
byHair              TINYINT            — Saç modeli
bySkinColor         TINYINT            — Cilt rengi
strExtraUniqueItemList  VARCHAR(MAX)  — JSON/binary: unique item liste
iClanID             INT                — KNIGHTS.KnightsID foreign (0=no clan)
sClanLevel          SMALLINT           — Klan rank (0=Knight, 3=ChiefKnight)
iClanRank           SMALLINT           — Klan rütbe (deprecated?)
iLoyalty            INT                — Klan sadakat
cMagnetArea         TINYINT            — Magnet loot range
iLastLogOut         INT / BIGINT       — Logout timestamp (UNIX)
iLastLogIn          INT / BIGINT       — Login timestamp
iMarkedZoneID       INT                — Mark (recall) noktası zone
iMarkedX, iMarkedY, iMarkedZ  INT    — Mark koordinat
iChecksum           INT                — Data integrity check
bRebirth            TINYINT            — Rebirth sayı (1098'de 0 ama struct yok)
bRebStr, bRebSta, bRebDex, bRebInt, bRebCha  TINYINT  — Rebirth stat bonus
iWeaponLv           INT                — Silah level (deprecated?)
iArmorLv            INT                — Zırh level
prgName             VARCHAR(5)         — —
piCount             INT                — —
iWarhouse_Slot      INT / TINYINT      — Bank slot kullanım
iLetterMark         INT                — Başlık işareti
SilverEXP           INT                — Gümüş deneyim (premium?)
GoldEXP             INT / FLOAT        — Altın deneyim bonus
bItem[144]          BINARY(144)        — ⚠️ ENVANTER BINARY (bRaw tüm item)

INDEXES:
  PK: CharId
  UK: szName
  FK: AccountID → ACCOUNT_CHAR
  FK: iClanID → KNIGHTS
  INDEX: byAuthority (GM scan)
  INDEX: iLevel (level sort)
  INDEX: iZoneID (zone scan)
```

### Tuzaklar ve Kurallar

1. **bItem[144] = Envanter binary**
   - Her slot: Num (4b) + Plus (1b) + Durability (2b) + ... = 12 byte/slot
   - 12 slot × 12 = 144 byte
   - UPDATE YASAK — kullan TBL edit tool
   - Yanlış parse → item kayıp

2. **⚠️ USER_ITEM vs USER_ITEMS**
   - Kodda `USER_ITEM` (tekil) kullanılıyor
   - Tablo `USER_ITEMS` boş (schema leftover)
   - Envanter `USERDATA.bItem` içinde (binary)

3. **iExp / iMaxExp overflow**
   - 1098'de max exp ~10B (INT 32-bit borderline)
   - SQL Int32: max 2.1B
   - Çözüm: BIGINT (S71+ update)

4. **byNation = byRace (1098'de ırk savaşı, nation ayrı değil)**
   - 0 = Karus
   - 1 = El Morad
   - Nation system (später expansion) burada yok

5. **Authority seviyeleri**
   - 0 = Normal oyuncu
   - 2 = GM (Game Master)
   - 9 = Admin
   - Web panel role != DB authority

6. **Rebirth sistem (1098'de minimal)**
   - `bRebirth = 0` (yoksa feature off)
   - bRebStr/Sta/Dex/Int/Cha = bonus stat (ama 1098'de 0)
   - Şu an kullanılmıyor

---

## ITEM Tablosu

```sql
Num                 INT PRIMARY KEY   — Item unique ID (0-9999 arası, vb)
ItemType            TINYINT           — 0=Weapon, 1=Armor, 2=Accessory, 3=Quest, 4=Consumable
Kind                TINYINT           — Subtype (class, slot)
szName              VARCHAR(100)      — Item display name
byClass             TINYINT           — Gear class (0=all, 1=warrior, etc)
byRace              TINYINT           — 0=all, 1=Karus, 2=Human
byGender            TINYINT           — 0=all, 1=male, 2=female
byRequiredLevel     TINYINT           — Min level
byRequiredClass     TINYINT           — Min class
byMagic             TINYINT           — 0=normal, 1=magic, 2=rare, 3=legendary
byWeight            TINYINT           — Weight class (light/heavy)
iDamage             INT               — Silah damage (weapon), 0 (armor)
iDefense            INT               — Armor defense, 0 (weapon)
iDurability         INT               — Max durability
iBuyPrice           BIGINT            — NPC satın alma (Noah)
iSellPrice          BIGINT            — NPC satış
byFireResist, byColdResist, ...  TINYINT  — Resistans değeri
iEffectID           INT               — ITEM_EXTENSION ref
iEffectValue        INT               — Effect param
bySpecialType       TINYINT           — Special effect flag
```

---

## MAGIC_TABLE Tablosu

```sql
dwMagicID           INT PRIMARY KEY    — Skill unique ID
szMagicName         VARCHAR(100)       — Skill name
byMagicType         TINYINT            — 1=Direct, 2=Heal, 3=Buff, 4=DoT, vb
iMp                 INT                — MP cost
iCoolTime           INT                — Cooldown ms
iDuration           INT                — Effect duration ms (buff/debuff)
iRange              INT                — Ability range
iCastTime           INT                — Cast duration ms
iEffectValue        INT                — Damage / Heal amount
byLevel             TINYINT            — Spell level (1-10)
byRequiredLevel     TINYINT            — Min karakter level
szDescription       VARCHAR(255)       — Flavor text
```

---

## KNIGHTS (Klan) Tablosu

```sql
KnightsID           INT PRIMARY KEY   — Klan unique ID
szName              VARCHAR(50)       — Klan adı
dwMasterID          INT               — Master oyuncu CharId
dwMasterName        VARCHAR(32)       — Master adı (denorm)
Notice              VARCHAR(255)      — Klan announcemet
Grade               TINYINT           — Klan sıralama (5 = CSW qualified)
Alliance            TINYINT           — Ittifak dışında: 0
FoundDate           INT               — Kuruluş timestamp
TotalMembers        INT               — Toplam üye (denorm)
iGold               BIGINT            — Klan hazinesi (Noah)
iGoldBank           BIGINT            — Klan bank
```

---

## TB_USER (Web Login)

```sql
AccountID           INT PRIMARY KEY   — Hesap unique ID
strID               VARCHAR(50)       — Login username
strPassword         VARCHAR(255)      — Hashed password (bcrypt / sha256)
strWebHash          VARCHAR(255)      — ⚠️ WEB LOGIN TOKEN (NULL BUG)
LoginCheck          TINYINT           — 0=off, 1=on
LastIP              VARCHAR(15)       — Last login IP
LastDate            DATETIME          — Last login time
AuthProvider        VARCHAR(50)       — Provider (google, discord, etc)
```

**⚠️ strWebHash NULL Bug:** Web login tokenı NULL tutulursa web portal login başarısız. Fix: Default token generate.

---

## QUOTED_IDENTIFIER Bug (199 SP)

**Sorun:** 199 Stored Procedure'de `SET QUOTED_IDENTIFIER OFF` → `"` (quote) attribute adı parse hatası → LoginServer crash.

**Tuzak:** Bazı SP'ler SQL Server 2008 yazıldı (QI kullanmayan), şu an 2019'da çalışmıyor.

**Çözüm:** ALTER PROC + SET QUOTED_IDENTIFIER ON başında.

**Memory:** `feedback_qi_rule.md` — Her SP'de SET QUOTED_IDENTIFIER ON zorunlu.

---

## VERSION Tablosu (Patch)

```sql
VersionID           INT PRIMARY KEY
iCurVersion         INT               — Şu anki patch version
iLastVersion        INT               — Son official version
sFile               VARCHAR(255)      — Patch dosya URL
FileHash            VARCHAR(64)       — SHA256 hash
ReleaseDate         DATETIME          — Yayın tarihi
```

---

## Backup / Restore

| Dosya | Boyut | Tarih | Amacı |
|-------|-------|-------|-------|
| `KO_MYKO_2026-03-19.bak` | 252M | 2026-03-19 | Full backup |
| `MYKO_LOCAL_2026-03-19.bak` | 252M | 2026-03-19 | Lokal kopy |

**Restore Yasak:** Yalnızca SELECT doğrulama için (DOKTOR onayı ile).

---

## Erişim Kısıtlamaları

| İşlem | Kime | Kuralı |
|-------|------|--------|
| SELECT | Herkese | Serbest |
| INSERT | sa / myko_app | DOKTOR onayı gerekli |
| UPDATE | sa / myko_app | DOKTOR + Erencan çift onay |
| DELETE | YASAK | Hiç kimseye — veri kurtarılamaz |
| DROP / ALTER | YASAK | Hiç kimseye — backup yok |
| TBL edit | (local) | DOKTOR onayı + backup mandatory |

---

## Kaynak Referansları

- `_KO_TEMEL_HERKES_OKU.md` — KO oyun bilgisi
- `MATERYAL_HARITASI.md` — Element ↔ dosya mapping
- `Desktop\Server\DB_BACKUP_ORIGINALS\*.sql` — Orijinal SP'ler
- `GameServer.ini [ODBC]` — Connection string
- `tools\tbl\tbl_decrypt.py` — TBL binary parse
- Memory: `feedback_qi_rule.md`, `feedback_binary_kolonlar.md`

---

**Son Güncelleme:** 2026-04-29 | **Versiyon:** 1.0 | **MATRIX**
