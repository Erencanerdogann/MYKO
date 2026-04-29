# Stored Procedure'ler — KO_MYKO

**Tarih:** 2026-04-29 | **Versiyon:** 1.0 | **Agent:** MATRIX | **Session:** S87+

---

## Özet

MalaysiaKO 1098, oyuncu giriş (login) ve karakter oluşturma (character creation) işlemleri **6 kritik SP** ile yönetilir. Ayrıca **199 SP'de QUOTED_IDENTIFIER bug** vardır (LoginServer crash riski).

---

## 1️⃣ LOAD_USER_DATA

### Amaç
Oyuncu login'de: AccountID + CharID verilen, o karakterin **tüm verisi** (USERDATA + USER_ITEM + USER_SKILL) getir.

### Input
```sql
@strAccountID VARCHAR(21)  — Hesap unique ID (örnek: "100")
@strCharID    VARCHAR(21)  — Karakter unique ID (örnek: "100_1")
```

### Output
**USERDATA satırı** (50+ kolon):
```
Nation, Race, Class, HairRGB, Rank, Title, Level,
Exp, Loyalty, Face, City, Knights, Fame, Hp, Mp, Sp,
Strong, Sta, Dex, Intel, Cha, bRebStr, bRebSta, bRebDex, bRebIntel, bRebCha,
Authority, Points, Gold, Zone, Bind, PX, PZ, PY, dwTime,
strSkill, strItem, strSerial, strItemTime, strLevel,
MannerPoint, LoyaltyMonthly, mutestatus, attackstatus, tagname, tagnamergb,
ChickenStatus, flashtime, flashcount, flashtype
```

### Mantık
1. Hesap (`ACCOUNT_CHAR`) kontrol: CharID valid mi?
2. Zone fix: Closed zone'dan → spawn zone'a (dungeon → Moradon)
3. USERDATA SELECT → Client'e gönder

### Kaynak
- `Desktop\Server\DB_BACKUP_ORIGINALS\LOAD_USER_DATA_ORIGINAL.sql`
- **QI Bug:** `SET QUOTED_IDENTIFIER OFF` → Fix: `SET QUOTED_IDENTIFIER ON` ekle

### Çağrılan Yer
- **GameServer**: Login flow (character select menüsünde)

---

## 2️⃣ CREATE_NEW_CHAR

### Amaç
Yeni karakter yarat. ACCOUNT_CHAR + USERDATA INSERT, başlangıç eşyası (START_ITEM_PROC).

### Input
```sql
@nRet OUTPUT SMALLINT              — Return code (0=ok, 1=5char limit, 2=race mismatch, 3=duplicate, 4=error)
@AccountID CHAR(21)                — Hesap ID
@index TINYINT                     — Slot (0-4, 5 karekter limit)
@CharID CHAR(21)                   — Yeni karakter ID (örnek: "100_1")
@Race TINYINT                      — 0=Karus, 1=Human
@Class SMALLINT                    — 0=Warrior, 1=Rogue, 2=Priest, 3=Mage, 4=Kurian, 5=Porutu
@Hair TINYINT                      — Saç modeli (0-10)
@Face TINYINT                      — Yüz modeli (0-5)
@Str, @Sta, @Dex, @Intel, @Cha TINYINT  — İlk stat
```

### Output
- Return code: 0 (başarılı), 1 (limit), 2 (ırk hatası), 3 (duplicate), 4 (DB error)

### Mantık
1. **Slot kontrol:** ACCOUNT_CHAR.bCharNum >= 5? → return 1
2. **Irk kontrol:** Nation vs Race uyum? → return 2
3. **Duplicate kontrol:** USERDATA'da zaten var mı? → return 3
4. **INSERT ACCOUNT_CHAR:** strCharID1/2/3/4/5 güncelle, bCharNum++ (transaction)
5. **INSERT USERDATA:** Zone 21 (Moradon), init stat, PX/PZ (zone başlangıç)
6. **EXEC START_ITEM_PROC:** Başlangıç eşyası (`BEGINNER_ITEM`)

### Kaynak
- `Desktop\Server\DB_BACKUP_ORIGINALS\CREATE_NEW_CHAR_ORIGINAL.sql`
- Alternatif: `myko-panel/kaynak/SP/CREATE_NEW_CHAR.sql`
- **QI Bug:** Var

### Çağrılan Yer
- **GameServer**: Character creation window

---

## 3️⃣ SET_LOGIN_INFO

### Amaç
Login sırasında oyuncu state güncellemesi (Zone, koordinat, last login time).

### Input
```sql
@CharID VARCHAR(21)                — Oyuncu ID
@iZone INT                         — Mevcut zone
@iPX, @iPY, @iPZ INT               — Koordinat
```

### Output
Yok (UPDATE sadece)

### Mantık
1. USERDATA.Zone = @iZone
2. USERDATA.PX/PY/PZ = input koordinat
3. USERDATA.dwTime = GETDATE()
4. Closed zone kontrol (zone fix apply)

### Kaynak
- `Desktop\Server\DB_BACKUP_ORIGINALS\SET_LOGIN_INFO_ORIGINAL.sql`
- **QI Bug:** Var

### Çağrılan Yer
- **GameServer**: Login başında (state load sonrası)

---

## 4️⃣ UPDATE_USER_DATA

### Amaç
Oyunun devamında oyuncu state kaydet (logout veya periodic save).

### Input
```sql
@CharID VARCHAR(21)
@iLevel, @iExp, @iHp, @iMp INT
@Str, @Sta, @Dex, @Intel, @Cha TINYINT
@Gold BIGINT
@Zone INT, @PX, @PY, @PZ INT
@strItem VARBINARY(MAX)            — Envanter binary (bItem[144])
@strSkill VARBINARY(MAX)           — Skill shortcut binary
— ...diğer state
```

### Output
Yok (UPDATE)

### Mantık
1. USERDATA update: level, exp, gold, stat, zone, coord
2. **bItem UPDATE YASAK** — strItem direkt yazma (binary parse bug riski)
3. **USER_ITEM tekil** — USERDATA.bItem kullanılıyor

### Kaynak
- `Desktop\Server\DB_BACKUP_ORIGINALS\UPDATE_USER_DATA_ORIGINAL.sql`
- **QI Bug:** Var

### Çağrılan Yer
- **GameServer**: Logout flow, periodic save (60sn)

---

## 5️⃣ SKILLSHORTCUT_SAVE

### Amaç
Oyuncu skill bar sıralaması kaydet (U tuşu shortcut).

### Input
```sql
@CharID VARCHAR(21)
@SkillBar VARBINARY(MAX)           — Skill shortcut binary array
```

### Output
Yok

### Mantık
1. USER_SKILL_BAR.SkillBar = @SkillBar
2. Binary format: [MagicID (4b), SlotNum (1b)] × N slots

### Kaynak
- `Desktop\Server\DB_BACKUP_ORIGINALS\SKILLSHORTCUT_SAVE_ORIGINAL.sql`
- **QI Bug:** Var

### Çağrılan Yer
- **GameServer**: Skill bar değiştirilince (event listener)

---

## 6️⃣ UPDATE_SAVED_MAGIC

### Amaç
Oyuncu aktif buff/debuff durumu kaydet (USER_DURATION_SKILL).

### Input
```sql
@CharID VARCHAR(21)
@MagicID INT
@Duration INT                      — Remaining time (ms)
@Level TINYINT                     — Magic level
```

### Output
Yok

### Mantık
1. USER_DURATION_SKILL INSERT/UPDATE (CHARACTER_ID, MAGIC_ID, DURATION)
2. Duration = 0 → DELETE (buff bitti)
3. Logout'da silme → relogin'de buff reset

### Kaynak
- `Desktop\Server\DB_BACKUP_ORIGINALS\UPDATE_SAVED_MAGIC_ORIGINAL.sql`
- **QI Bug:** Var

### Çağrılan Yer
- **GameServer**: Buff apply/remove event

---

## 7️⃣ CHARACTER_LOGIN_CHECKS (isteğe bağlı)

### Amaç
Login öncesi güvenlik kontrol (banned account, muted, etc).

### Input
```sql
@CharID VARCHAR(21)
```

### Output
```
bIsBanned TINYINT              — 0=ok, 1=banned
iMuteTime INT                  — Mute duration (0=not muted)
iLockTime INT                  — Account lock duration
```

### Mantık
1. BANNED_LIST → isBanned check
2. MUTE_LOG → active mute check
3. ACCOUNT_LOCK → account lock check
4. Hepsi 0 ise login allow

### Kaynak
- (Var mı kontrol gerekli — S87'de rapor yazılacak)

---

## 🔴 QUOTED_IDENTIFIER Bug — BLOCKING

### Sorun
**199 SP'de `SET QUOTED_IDENTIFIER OFF` → LoginServer crash**

LoginServer, SP sonucu parse ederken `"` (double quote) attribute adlarını SQL quote syntax'ı olarak okur → bozulur.

### Tuzak
```sql
SET QUOTED_IDENTIFIER OFF          ← PROBLEMLI
...
SELECT "Level", "Class" FROM ...   ← " = delimiter mi, string mi?
```

### Çözüm
```sql
SET QUOTED_IDENTIFIER ON           ← FIX
```

### Etkilenen SP'ler
- CREATE_NEW_CHAR
- LOAD_USER_DATA
- UPDATE_USER_DATA
- SET_LOGIN_INFO
- SKILLSHORTCUT_SAVE
- UPDATE_SAVED_MAGIC
- (+ 193 diğer SP)

### Memory
- `feedback_qi_rule.md` — Her SP'de SET QUOTED_IDENTIFIER ON zorunlu

### Yaptırım
- İhlal: -15 puan (critical bug)
- Fix: ALTER PROC + SET QUOTED_IDENTIFIER ON başında + test

---

## İşlem Sırası — Login Flow

```
1. Character selection (SELECT USERDATA.CharId, szName, Level)
                           ↓
2. LOAD_USER_DATA @AccountID, @CharID
                           ↓
3. CHARACTER_LOGIN_CHECKS @CharID (banned? muted?)
                           ↓
4. SET_LOGIN_INFO (zone, coord update)
                           ↓
5. GameServer → Map load, spawn
                           ↓
6. [GAMEPLAY]
                           ↓
7. Logout → UPDATE_USER_DATA (state save)
                           ↓
8. SKILLSHORTCUT_SAVE, UPDATE_SAVED_MAGIC (cleanup)
                           ↓
9. Disconnect
```

---

## İşlem Sırası — Character Creation

```
1. Character creation form (input: class, hair, face, stat)
                           ↓
2. CREATE_NEW_CHAR @AccountID, @index, @CharID, @Race, @Class, ...
   ├─ ACCOUNT_CHAR INSERT (slot kaydı)
   ├─ USERDATA INSERT (init stat, zone 21)
   └─ START_ITEM_PROC (başlangıç eşya)
                           ↓
3. SUCCESS → Character list'e yeni karakter görün
```

---

## Backup / Restore

**Orijinal SP'ler** (kaynak):
- `Desktop\Server\DB_BACKUP_ORIGINALS\<SP>_ORIGINAL.sql`

**Buggy versiyonlar** (1098 olduğu gibi):
- `KO_MYKO` → var olan SP'ler (QI bug + diğer eski syntax)

**Fix prosedürü:**
1. LOAD_USER_DATA_ORIGINAL.sql oku
2. SET QUOTED_IDENTIFIER ON ekle
3. KO_MYKO'da DROP PROCEDURE
4. Fixed version ile CREATE PROCEDURE
5. Test: GameServer login flow

---

## Kaynak Referansları

- `Desktop\Server\DB_BACKUP_ORIGINALS\*.sql` — Orijinal SP'ler
- `myko-panel/kaynak/SP/CREATE_NEW_CHAR.sql` — Alternatif versiyon
- `LOAD_USER_DATA_ORIGINAL.sql` — Login core
- Memory: `feedback_qi_rule.md` — QUOTED_IDENTIFIER rule

---

**Son Güncelleme:** 2026-04-29 | **Versiyon:** 1.0 | **MATRIX**
