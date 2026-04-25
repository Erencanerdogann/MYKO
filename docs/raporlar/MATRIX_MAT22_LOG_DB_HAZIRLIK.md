# MATRIX_MAT22_LOG_DB_HAZIRLIK
# Tarih: 2026-04-25 | Agent: MATRIX | Session: S83 | Görev: MAT-22
# Durum: TAMAMLANDI

---

## BÖLÜM C — GAMESERVER LOG ANALİZİ

### C.1 Log Dosyası Envanteri (23-25 Nisan)

| Dosya | Tarih | İçerik Türü |
|-------|-------|-------------|
| ODBC_2026-04-23.log | 23 Nis | ODBC hataları |
| ODBC_2026-04-25.log | 25 Nis | ODBC hataları |
| GENERAL_2026-04-23.log | 23 Nis | Sunucu başlatma, SKILL_FAIL, WAREHOUSE |
| GENERAL_2026-04-24.log | 24 Nis | Sunucu başlatma (az kayıt) |
| GENERAL_2026-04-25.log | 25 Nis | Sunucu başlatma (4 yeniden başlatma) |
| HACK_2026-04-23.log | 23 Nis | FLY_HACK: 1074 kayıt |
| HACK_2026-04-25.log | 25 Nis | GENIE_DISPATCH: 361 + FLY_HACK: 7 kayıt |
| LOGIN_2026-04-25.log | 25 Nis | Giriş/çıkış, LOGIN_AUTH |
| DISCONNECT_2026-04-25.log | 25 Nis | KICK (timeout) + NormalLogout |
| GameServer.log | sürekli | ODBC logları (GENERAL ile aynı içerik) |
| **crashfiles.dmp** | 25 Nis 18:16 | **CRASH DUMP — 44 KB** |

**24 Nisan HACK logu yok** — sunucu o gün az çalışmış (04:37'de tek başlatma).

---

### C.2 Hata Pattern Bazlı Sayım

| Pattern | Tarih | Sayı | Ciddiyet |
|---------|-------|------|----------|
| FLY_HACK | 23 Nis | 1074 | 🔴 KRİTİK |
| GENIE_DISPATCH | 25 Nis | 361 | 🔴 KRİTİK |
| FLY_HACK | 25 Nis | 7 | 🔴 KRİTİK |
| ODBC Login failed for user 'sa' | 25 Nis 18:15-16 | 2 | 🔴 KRİTİK |
| SQLBindParameter Invalid precision | 23 Nis | 2 | 🟡 YÜKSEK |
| SKILL_FAIL R17_usercancast_fail | 23 Nis | ~40+ | 🟢 NORMAL |
| SKILL_FAIL UCC_not_available | 23 Nis | ~40+ | 🟢 NORMAL |
| SKILL_FAIL R6_instant_spam_300ms | 23 Nis | az | 🟢 NORMAL |
| KICK timeout | 25 Nis | 1 | 🟢 NORMAL |
| Sunucu restart (GENERAL init) | 25 Nis | 4x | 🟡 YÜKSEK |

---

### C.3 Kritik Bulgular

#### 🔴 BULGU 1 — CRASH DUMP (25 Nisan 18:16)

```
Dosya: C:\Users\Administrator\Desktop\Server\crashfiles.dmp
Boyut: 44.061 bytes
Tarih: 25.04.2026 18:16
```

**Zaman korelasyonu:**
- 18:15:44 → ODBC Login failed for 'sa'
- 18:16:01 → ODBC Login failed for 'sa' (2. hata)
- 18:16 → crashfiles.dmp oluştu
- 18:18:45 → GENERAL: Server starting (yeniden başlatma)

**Değerlendirme:** GameServer DB bağlantısı koptuğunda çöktü. ODBC hatası → crash zinciri.
**Kök sebep:** sa hesabı şifresi o anda yanlıştı/değiştirilmişti. RUSTIK sqlcmd fix'i (18:37) bu sonradan çözdü.

---

#### 🔴 BULGU 2 — HACKER: System32 / System63 (IP: 88.254.5.104)

**23 Nisan:**
- System32 → FLY_HACK: 1074 kayıt (01:47-05:13 arası), Zone=1, Y=6525.1 (max 250)
- SKILL_FAIL'ler de aynı dönemde: System63 SkillID=111004

**25 Nisan:**
- System32 → birden fazla login (15:42, 15:44, 18:20, 20:53, 21:48)
- System321 login 21:53 → System63 karakterini çalıştırıyor
- GENIE_DISPATCH: 361 kayıt (21:52-21:54)
- FLY_HACK: 7 kayıt (21:53-21:54), System63, Zone=11

**Hesap ilişkisi:**
```
IP: 88.254.5.104
Account=System32  → User=System32  (Level 72, Nation 1, Zone 1)
Account=System321 → User=System63  (Level 72, Nation 1, Zone 11)
```

**GENIE_DISPATCH nedir:** Genie/Companion item bot komutu. cmd=1 (hareket), cmd=2 (saldır), cmd=4 (toplama) döngüsü — otomatik farm botu.

**Bu hesaplar ban edilmemiş** — GENERAL logda 23 Nisan 03:36-03:38 yeniden başlatmadan sonra System63 skill spam tekrar başlamış.

---

#### 🟡 BULGU 3 — SQLBindParameter Invalid Precision (23 Nisan)

```
23.4.2026 03:56 → SQLBindParameter Error: Invalid precision value
23.4.2026 04:02 → SQLBindParameter Error: Invalid precision value
```

**Değerlendirme:** Sunucu başlatma (01:05, 03:36, 03:38) sonrası 2 ODBC parametre hatası. Büyük ihtimalle bir SP'ye yanlış tip/boyut gönderildi. Sonrasında tekrar etmemiş — önemli değil ama bir SP'nin parametre tanımıyla tip uyumsuzluğu var.

---

#### 🟡 BULGU 4 — 25 Nisan 4x Sunucu Yeniden Başlatma

```
13:50:42 → LoginServer starting
13:50:57 → Server starting
15:41:57 → LoginServer starting
15:42:06 → Server starting
18:18:45 → Server starting (crash sonrası)
18:19:15 → LoginServer starting
21:38:57 → LoginServer starting
21:39:18 → Server starting
21:40:12 → LoginServer starting
21:40:21 → Server starting
```

25 Nisan'da 5 server start kaydı var (birden çok çift login/game). Normal test/deploy olabilir ama sık restart işareti.

---

### C.4 Trend Analizi

| Kategori | 23 Nis | 24 Nis | 25 Nis | Trend |
|----------|--------|--------|--------|-------|
| ODBC hata | 2 | 0 | 2 | Sabit |
| Crash | 0 | 0 | 1 | ⚠️ Yeni |
| FLY_HACK | 1074 | 0 | 7 | Düşüyor |
| GENIE (bot) | 0 | 0 | 361 | ⚠️ Yeni tip |
| Sunucu restart | ~4 | 1 | 5 | Artıyor |

**FLY_HACK azalmış** (1074→7) ama GENIE_DISPATCH yeni pattern olarak ortaya çıktı.
**Aynı IP (88.254.5.104)** — farklı hack aracına geçmiş.

---

### C.5 Acılış Öncesi Fix Listesi

| Öncelik | Aksiyon | Kime |
|---------|---------|------|
| 🔴 ACİL | System32 + System321 hesaplarını BAN et (IP: 88.254.5.104) | DOKTOR / Erencan |
| 🔴 ACİL | crashfiles.dmp analiz et — ne zaman oluştu, kayıp var mı | CHIP (crash analiz) |
| 🟡 YÜKSEK | GENIE_DISPATCH rate limiting / ban otomasyonu | RUS-66 FAZ-4 kapsamına ekle |
| 🟡 YÜKSEK | SQLBindParameter hatasını tetikleyen SP'yi bul | MATRIX (ayrı görev) |
| 🟢 NORMAL | SKILL_FAIL R17 — System63 skill kullanamıyor ama denemekten durmuyor | Config ayarı |

---

## BÖLÜM D — TRILOGY DB HAZIRLIK (RUS-66 FAZ-0/4)

### D.1 Tablo Varlık Kontrolü

Tüm hedef tablolar KO_MYKO'da mevcut ✅:

| Tablo | Var mı | Not |
|-------|--------|-----|
| BANNED_LIST | ✅ | 10 kolon |
| USER_HDD_BAN_LIST | ✅ | 4 kolon (HDD ban) |
| BANISH_OF_WINNER | ✅ | (var, kolon sorulmadı) |
| _MALAYSIAKO_banks | ✅ | (custom tablo) |
| MONSTER_RESPAWN_STABLE_LIST | ✅ | 8 kolon |
| ITEM_SELLTABLE | ✅ | 26 kolon |
| QUEST_MENU_US | ✅ | 2 kolon |
| QUEST_TALK_US | ✅ | 2 kolon |
| DAILY_QUESTS | ✅ | 23 kolon (nID şeması) |
| CREATE_NEW_CHAR_SET | ✅ | 8 kolon |
| CREATE_NEW_CHAR_VALUE | ✅ | 17 kolon |

---

### D.2 BAN SİSTEMİ (FAZ-4 için)

#### SP Listesi:
```
ADD_BAN
CHECK_BAN_STATUS
CHECK_BANNED_ACCOUNTS
GET_BAN_LIST
REMOVE_BAN
```

#### ADD_BAN İmzası:
```sql
EXEC ADD_BAN
    @strAccountID VARCHAR(50),   -- hesap adı (boş bırakılabilir IP ban için)
    @strIPAddress VARCHAR(50),   -- IP adresi (boş bırakılabilir account ban için)
    @nBanType     TINYINT,       -- 1=account, 2=IP, 3=her ikisi
    @strReason    NVARCHAR(200), -- sebep metni
    @strBannedBy  VARCHAR(50),   -- banlayan GM adı
    @nDurationMinutes INT        -- 0=kalıcı, >0=geçici (dakika)
```

**Return kodları:** 0=başarı, -1=geçersiz banType, -2=account boş(type 1/3), -3=IP boş(type 2/3)

**ADD_BAN davranışı:**
1. Aynı hesap/IP'nin aktif banını kapatır (UPDATE bActive=0)
2. Yeni ban satırı ekler BANNED_LIST'e
3. Account ban ise CHECK_ACCOUNT.logintimestatus günceller:
   - Kalıcı: -1
   - Süreli: UNIX timestamp (expire zamanı)

#### REMOVE_BAN İmzası:
```sql
EXEC REMOVE_BAN
    @strAccountID VARCHAR(50),  -- hesap (veya boş)
    @strIPAddress VARCHAR(50),  -- IP (veya boş)
    @strRemovedBy VARCHAR(50)   -- kaldıran GM
```
**Return:** etkilenen satır sayısı (@@ROWCOUNT)

#### BANNED_LIST Şeması:
```
nIndex        int         NOT NULL  (PK, auto)
strAccountID  varchar(50) NOT NULL
strCharID     varchar(50) NOT NULL
strIPAddress  varchar(50) NOT NULL
nBanType      tinyint     NOT NULL  (1/2/3)
strReason     nvarchar(200) NOT NULL
strBannedBy   varchar(50) NOT NULL
dtBanStart    datetime    NOT NULL
dtBanEnd      datetime    NULL      (NULL=kalıcı)
bActive       bit         NOT NULL
```

**⚠️ DİKKAT:** ADD_BAN imzasında `@strCharID` parametresi YOK ama BANNED_LIST tablosunda `strCharID` kolonu var. SP char_id'yi boş string olarak insert ediyor (`''`). myko-panel gm.rs'deki `oyuncu_ban()` fonksiyonu SP'yi `@strCharID` parametre pozisyonuyla çağırıyor — SP imzasıyla uyumsuz.

**gm.rs mevcut çağrı (yanlış):**
```rust
"EXEC ADD_BAN N'{acc}', N'{chr}', N'{ip}', {bt}, N'{rsn}', N'{gm}', {days}"
// 7 parametre geçiyor ama SP 6 parametre alıyor
```

**Doğru çağrı:**
```rust
"EXEC ADD_BAN N'{acc}', N'{ip}', {bt}, N'{rsn}', N'{gm}', {days}"
// 6 parametre — charID yok, SP bunu BANNED_LIST'e '' olarak yazar
```

#### USER_HDD_BAN_LIST Şeması (HDD/MAC ban):
```
UserID    varchar(21)  -- karakter adı
HwidKey   bigint       -- HDD seri no hash
MacKey    int          -- MAC adresi hash
Banned    tinyint      -- 0/1
```

**Örnek sorgu:**
```sql
SELECT * FROM KO_MYKO.dbo.USER_HDD_BAN_LIST WHERE UserID='KullaniciAdi'
INSERT INTO KO_MYKO.dbo.USER_HDD_BAN_LIST (UserID,HwidKey,MacKey,Banned)
VALUES ('System32', 0, 0, 1)  -- HWID/MAC bilinmiyorsa 0
```

---

### D.3 MONSTER RESPAWN (FAZ-0.3)

#### MONSTER_RESPAWN_STABLE_LIST Şeması:
```
sIndex       smallint  -- spawn kaydı ID
GroupNumber  smallint  -- spawn grubu (zone bazlı gruplar)
sSid         smallint  -- K_MONSTER.sSid (FK)
isNpc        tinyint   -- 0=monster, 1=NPC
sZoneID      tinyint   -- zone ID
sCount       smallint  -- spawn sayısı
sRadius      tinyint   -- spawn yarıçapı
isDeadTime   int       -- respawn süresi (saniye)
```

**Örnek veri:**
```
sIndex=5801, GroupNumber=1, sSid=5901, isNpc=0, sZoneID=71, sCount=1, sRadius=3, isDeadTime=10
```

**K_MONSTER ilişkisi:** `sSid` kolonu K_MONSTER.sSid'e referans veriyor (FK enforce edilmemiş olabilir ama mantıksal FK).

**Spawn yapısı:** GroupNumber ile gruplandırılmış, her grup bir zone'daki spawn bölgesini temsil ediyor.

**RUSTIK için örnek SELECT:**
```sql
SELECT r.sIndex, r.GroupNumber, r.sSid, m.strName, r.isNpc, r.sZoneID, r.sCount, r.sRadius, r.isDeadTime
FROM KO_MYKO.dbo.MONSTER_RESPAWN_STABLE_LIST r
LEFT JOIN KO_MYKO.dbo.K_MONSTER m ON m.sSid = r.sSid
WHERE r.sZoneID = 71  -- zone filtresi
ORDER BY r.GroupNumber, r.sIndex
```

---

### D.4 NPC SATIŞ (FAZ-0.2)

#### ITEM_SELLTABLE Şeması:
```
iSellingGroup  int  -- K_NPC.iSellingGroup ile JOIN anahtarı
Item1..Item24  int  -- 24 slot, her biri iNum (ITEM tablosundaki ID)
nIndex         int  -- sıralama/alt-sayfa (aynı iSellingGroup için birden fazla satır)
```

**Örnek veri:**
```
iSellingGroup=101000, Item1=110150000, Item2=110450000, ..., nIndex=1
iSellingGroup=101000, Item1=140250000, ..., nIndex=2
iSellingGroup=101000, Item1=160150000, ..., nIndex=3
```

**K_NPC ilişkisi:** K_NPC.iSellingGroup = ITEM_SELLTABLE.iSellingGroup.
Bir NPC'nin birden fazla sayfa satışı olabilir (aynı iSellingGroup, farklı nIndex).

**RUSTIK için örnek SELECT:**
```sql
SELECT st.iSellingGroup, st.nIndex,
       st.Item1, st.Item2, st.Item3, st.Item4, st.Item5,
       st.Item6, st.Item7, st.Item8, st.Item9, st.Item10
FROM KO_MYKO.dbo.ITEM_SELLTABLE st
WHERE st.iSellingGroup = (SELECT iSellingGroup FROM KO_MYKO.dbo.K_NPC WHERE sSid = 10001)
ORDER BY st.nIndex
```

**Dikkat:** Item değerleri iNum formatında (ör. 110150000) — ITEM.iNum ile eşleşir, basit int karşılaştırması yeterli.

---

### D.5 QUEST DİYALOG (FAZ-0.4)

#### QUEST_MENU_US Şeması:
```
iNum     int         -- quest ID (DAILY_QUESTS.questid ile bağlantılı)
strMenu  char(100)   -- menü metni (ör. "Confirm", "Where is the [Manager]?")
```

**Örnek veri:**
```
iNum=10, strMenu='Confirm'
iNum=11, strMenu='Where is the [Manager]?'
```

#### QUEST_TALK_US Şeması:
```
iNum    int           -- quest ID
strTalk char(1000)    -- diyalog metni (uzun)
```

**DAILY_QUESTS ile ilişki:**
```
DAILY_QUESTS.questid → QUEST_MENU_US.iNum (diyalog göster)
DAILY_QUESTS.questid → QUEST_TALK_US.iNum (konuşma metni)
```

**RUSTIK için örnek SELECT:**
```sql
SELECT dq.id, dq.StrName, dq.questid,
       qm.strMenu,
       qt.strTalk
FROM KO_MYKO.dbo.DAILY_QUESTS dq
LEFT JOIN KO_MYKO.dbo.QUEST_MENU_US qm ON qm.iNum = dq.questid
LEFT JOIN KO_MYKO.dbo.QUEST_TALK_US qt ON qt.iNum = dq.questid
ORDER BY dq.id
```

---

### D.6 YENİ KARAKTER (FAZ-0.5)

#### CREATE_NEW_CHAR_SET Şeması (başlangıç stat seti):
```
ID            int       -- set ID (ClassType referansı)
ClassType     tinyint   -- karakter sınıfı
SlotID        int       -- slot
ItemID        int       -- başlangıç item ID
ItemDuration  smallint  -- dayanıklılık
ItemCount     smallint  -- adet
ItemFlag      tinyint   -- flag
ItemExpireTime int      -- süre
```

**Örnek veri:**
```
ID=1, ClassType=1, SlotID=0, ItemID=310150004, ItemDuration=1, ItemCount=1
ID=2, ClassType=1, SlotID=1, ItemID=201003001, ItemDuration=4000, ItemCount=1
```

**Format:** ClassType başına slot bazlı item listesi. Sınıf 1 için SlotID=0..N başlangıç itemleri.

#### CREATE_NEW_CHAR_VALUE Şeması (başlangıç istatistik değerleri):
```
nIndex         int
ClassType      tinyint  -- sınıf
JobType        tinyint
Level          tinyint  -- başlangıç level
Exp            bigint
Strength       tinyint
Health         tinyint
Dexterity      tinyint
Intelligence   tinyint
MagicPower     tinyint
FreePoints     smallint
SkillPointFree tinyint
SkillPointCat1..3  tinyint
SkillPointMaster   tinyint
Gold           int      -- başlangıç noah
```

**Örnek veri:**
```
nIndex=1, ClassType=1, JobType=0, Level=1, ..., Gold=50000
nIndex=2, ClassType=2, JobType=0, Level=1, ..., Gold=50000
nIndex=3, ClassType=3, JobType=0, Level=1, ..., Gold=50000
```

**myko-panel mevcut kullanım:** `baslangic_itemleri_getir` invoke CREATE_NEW_CHAR_VALUE kullanıyor (nJob=ClassType). CREATE_NEW_CHAR_SET henüz panel'e bağlanmamış — RUSTIK FAZ bunu ekleyebilir.

**RUSTIK için örnek SELECT:**
```sql
-- Sınıf 1 başlangıç itemleri
SELECT s.ID, s.ClassType, s.SlotID, s.ItemID, i.strName, s.ItemDuration, s.ItemCount
FROM KO_MYKO.dbo.CREATE_NEW_CHAR_SET s
LEFT JOIN KO_MYKO.dbo.ITEM i ON i.iNum = s.ItemID
WHERE s.ClassType = 1
ORDER BY s.SlotID

-- Sınıf 1 başlangıç statları
SELECT * FROM KO_MYKO.dbo.CREATE_NEW_CHAR_VALUE WHERE ClassType = 1
```

---

### D.7 DAILY_QUESTS Gerçek Şema Teyidi

**Gerçek DB kolonları (23 kolon, tinyint id — lowercase):**
```
id           tinyint    -- PK (küçük harf 'id', main.rs nID bekliyor)
StrName      varchar(100)
questid      smallint
timetype     tinyint
killtype     tinyint
MobID1..4   int
KillCount    int
Reward1..4  int
Count1..4   int
ZoneID       tinyint
MinLevel     tinyint
MaxLevel     tinyint
replaytime   tinyint
randomid     tinyint
```

**MAT-21 bulgusu teyit edildi:** editors.rs `iQuestID` arıyor, main.rs `nID` arıyor — gerçekte DB'de kolon adı `id` (tinyint, lowercase). Her iki invoke da yanlış isimle arıyor! `id` doğru kolon adı.

**RUSTIK için doğru WHERE:** `WHERE id = {n}` veya `ORDER BY id`

---

## ÖZET TABLOSU

### Bölüm C — Log Bulgular:
| # | Bulgu | Ciddiyet | Aksiyon |
|---|-------|----------|---------|
| 1 | crashfiles.dmp 25 Nis 18:16 — ODBC login fail → crash | 🔴 KRİTİK | CHIP analiz etmeli |
| 2 | System32/System63 IP 88.254.5.104 — fly hack + genie bot | 🔴 KRİTİK | BAN (DOKTOR/Erencan) |
| 3 | SQLBindParameter Invalid precision (23 Nis) | 🟡 YÜKSEK | SP tespiti gerekli |
| 4 | 25 Nisan 5x server restart | 🟡 YÜKSEK | Deploy/test mi izle |

### Bölüm D — DB Hazırlık:
| # | Konu | Durum | RUSTIK Notu |
|---|------|-------|-------------|
| 1 | ADD_BAN imzası | ✅ Tespit edildi | 6 param, charID yok. gm.rs 7 param gönderiyor → DÜZELT |
| 2 | REMOVE_BAN imzası | ✅ Tespit edildi | 3 param: account, ip, removedBy |
| 3 | BANNED_LIST şeması | ✅ 10 kolon | Kolon sırası raporda |
| 4 | USER_HDD_BAN_LIST şeması | ✅ 4 kolon | HDD/MAC ban için |
| 5 | MONSTER_RESPAWN_STABLE_LIST | ✅ 8 kolon | GroupNumber→Zone bazlı |
| 6 | ITEM_SELLTABLE | ✅ 26 kolon | 24 slot + iSellingGroup + nIndex |
| 7 | QUEST_MENU_US / QUEST_TALK_US | ✅ 2+2 kolon | iNum=questid FK |
| 8 | CREATE_NEW_CHAR_SET | ✅ 8 kolon | ClassType bazlı item seti |
| 9 | CREATE_NEW_CHAR_VALUE | ✅ 17 kolon | nIndex/ClassType stat tablosu |
| 10 | DAILY_QUESTS gerçek PK | ✅ id (tinyint) | iQuestID/nID ikisi de yanlış |

---

**Bynoisee © MalaysiaKO 2026 — MATRIX MAT-22 Log+DB Hazırlık Raporu**
