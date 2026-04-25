# MATRIX_KOLON_MISMATCH_RAPOR
# Tarih: 2026-04-25 | Agent: MATRIX | Session: S83 | Görev: MAT-21
# Durum: TAMAMLANDI

---

## ÖZET

myko-panel backend kodu ile RUSTIK SQL Pipeline Haritası (RUSTIK_SQL_PIPELINE_HARITA.md) arasındaki
kolon/tablo mismatch analizi. Kod okuma + mantık analizi yöntemi kullanıldı.
(SSH erişimi bu session'da izin dışıydı; DB sorguları çalıştırılamadı — mismatch tespiti sadece kod analizi.)

---

## KRİTİK MİSMATCH TABLOSU

| Invoke | Backend Fn | Kod'daki Gerçek Tablo | Pipeline Haritasındaki Yanlış | Mismatch Türü | Ciddiyet |
|---|---|---|---|---|---|
| ev2_event_listesi | event2::event_listesi | **EVENT_OPEN** | SERVER_EVENT | Tablo adı yanlış | 🔴 KRİTİK |
| ev2_event_durum_degistir | event2::event_durum_degistir | **EVENT_OPEN** | SERVER_EVENT | Tablo adı yanlış | 🔴 KRİTİK |
| ev2_event_saat_ayarla | event2::event_saat_ayarla | **EVENT_OPEN** | SERVER_EVENT_TIMER | Tablo adı yanlış | 🔴 KRİTİK |
| ev2_cr_guncelle | event2::cr_guncelle | **COLLECTION_RACE_EVENT_SETTINGS** | CHALLENGE_RANKING | Tablo adı yanlış | 🔴 KRİTİK |
| ev2_cr_odul_guncelle | event2::cr_odul_guncelle | **EVENT_REWARDS** | CHALLENGE_RANKING_REWARD | Tablo adı yanlış | 🔴 KRİTİK |
| klan_notice_guncelle | klan::klan_notice_guncelle | **CLAN** | GUILD_HALL | Tablo adı yanlış | 🔴 KRİTİK |
| klan_gold_guncelle | klan::klan_gold_guncelle | **CLAN** | GUILD_HALL | Tablo adı yanlış | 🔴 KRİTİK |
| karakter_ara | gm2::karakter_ara | **strUserId** (küçük d) | — | Kolon adı tutarsız | 🟡 ORTA |
| hesap_ara | gm2::hesap_ara | ACCOUNT_CHAR.nKCash/nBonusCash | — | Tablo doğru, kolon belirsiz | 🟡 ORTA |

---

## DETAYLI ANALİZ

### 1. EventKontrol Sekmesi — Pipeline Haritası TAMAMEN YANLIŞ

**Harita diyor:** `SERVER_EVENT`, `SERVER_EVENT_TIMER`, `CHALLENGE_RANKING`, `CHALLENGE_RANKING_REWARD`

**Gerçek kod (event2.rs):**
```
event_listesi         → EVENT_OPEN (kolon: EventID, EventType, ZoneID, EventName, EventStatus, Hour1-3, Minute1-3, MinLevel, MaxLevel)
event_durum_degistir  → EVENT_OPEN (kolon: EventStatus, EventID)
event_saat_ayarla     → EVENT_OPEN (kolon: Hour{slot}, Minute{slot}, EventID)
cr_listesi            → COLLECTION_RACE_EVENT_SETTINGS (kolon: nIndex, strEventName, nMinLevel, nMaxLevel, nZoneNo, nDuration, nUserLimit, nRepeatStatus, nAutoHour, nAutoMinute)
cr_guncelle           → COLLECTION_RACE_EVENT_SETTINGS (kolon: nRepeatStatus, nAutoHour, nAutoMinute, nIndex)
cr_odul_listesi       → EVENT_REWARDS (kolon: nIndex, nEventID, strDescription, nItemID, nItemCount, nRate, nItemTime, nItemFlag, nItemSession)
cr_odul_guncelle      → EVENT_REWARDS (kolon: nEventID, strDescription, nItemID, nItemCount, nRate, nItemTime, nItemFlag, nItemSession)
timed_notice_listesi  → TIMED_NOTICE (kolon: nIndex, strType, strText, nZoneNo, nDuration) — NOT: bu panel.db değil KO_MYKO!
```

**Tespit:** Pipeline haritası muhtemelen eski/farklı bir schema'dan kopyalanmış. Bu tablolar KO_MYKO'da mevcut değilse EventKontrol sekmesi tamamen çalışmaz.

---

### 2. KlanKralik Sekmesi — Tablo Adı Mismatch

**Harita diyor:** `GUILD_HALL`

**Gerçek kod (klan.rs):**
```
klan_listesi          → CLAN (kolon: IDNum, strName, Nation, Level, Points, MemberCount, ChiefName, strNotice)
klan_notice_guncelle  → CLAN (kolon: strNotice, IDNum)
klan_gold_guncelle    → CLAN (kolon: Gold, IDNum)
kralik_durumu         → KNIGHT_ROYAL_CAPTAIN JOIN CLAN (kolon: sNation, strKingName, sClanID, sTax, dStartDate)
```

**Tespit:** KO_MYKO'da `GUILD_HALL` değil `CLAN` tablosu kullanılıyor. Pipeline haritası yanlış.

---

### 3. KarakterEditor — strUserId Tutarsızlığı (Case-Sensitive Risk)

**gm.rs (oyuncu_ara, oyuncu_detay, karakter_gold_ekle, vb.):**
```sql
SELECT u.strUserID ...  WHERE u.strUserID = ...  -- büyük D
```

**gm2.rs (karakter_ara, karakter_tam_guncelle, karakter_hapis_gonder, vb.):**
```sql
SELECT TOP 50 strUserId ...  WHERE strUserId LIKE ...  -- küçük d
```

**Tespit:** SQL Server kolon adları case-insensitive — bu mismatch normalde runtime'da sorun yaratmaz.
Ancak bazı sürümlerde veya sorted collation'larda sorun çıkabilir. Temizlenmeli.

---

### 4. HesapEditor — Kolon Adı Analizi

**hesap_ara (gm2.rs:232):**
```sql
SELECT strAccountID, nKCash, nBonusCash, strEmail, strIP 
FROM ACCOUNT_CHAR WHERE strAccountID LIKE '%filtre%'
```

**Frontend beklentisi (OyuncuYonetimi.tsx:7):**
```ts
interface HesapBilgi { account_id: string; k_cash: number; bonus_cash: number; email: string; client_ip: string; }
```

**Backend struct (gm2.rs:221-227):**
```rust
pub struct HesapBilgi { pub account_id: String; pub k_cash: i64; pub bonus_cash: i64; pub email: String; pub client_ip: String; }
```

**Kolon sırası (0→4):** strAccountID | nKCash | nBonusCash | strEmail | strIP

**Struct mapping:** account_id=s[0] ✅ | k_cash=s[1] ✅ | bonus_cash=s[2] ✅ | email=s[3] ✅ | client_ip=s[4] ✅

**Tespit:** Kolon sırası struct ile uyuşuyor. **SORUN OLMAMALI.**

**Ama:** `nKCash`, `nBonusCash`, `strEmail`, `strIP` kolonları ACCOUNT_CHAR tablosunda gerçekten var mı?
KO standart şemasında ACCOUNT_CHAR'ın bu kolonları yoktur — genellikle `nKCash` TB_USER'da olur.
Bu, hesap_ara'nın hiç satır döndürmemesinin (0 sonuç) asıl sebebi olabilir.

---

### 5. KarakterEditor (karakter_ara) — Kolon Analizi

**karakter_ara (gm2.rs:199-203):**
```sql
SELECT TOP 50 strUserId, Level, RebLevel, Exp, Gold, Loyalty, Strong, Dex, HP, Intel, MP, Points, AttackPoint, DefensePoint, Authority, Zone
FROM USERDATA WHERE strUserId LIKE '%filtre%'
```

**Struct KarBilgi alanları:**
```
s[0]=char_id, s[1]=level, s[2]=reb_level, s[3]=exp, s[4]=gold
s[5]=loyalty, s[6]=str_, s[7]=dex, s[8]=hp, s[9]=intel
s[10]=mp, s[11]=points, s[12]=sol_np, s[13]=sag_np
s[14]=authority, s[15]=zone
```

**Frontend KarBilgi (OyuncuYonetimi.tsx:10-12):**
```ts
{ char_id, level, reb_level, exp, gold, loyalty, str_, dex, hp, intel, mp, points, sol_np, sag_np, authority, zone }
```

**Kolon eşleşme analizi:**
| İndex | SQL Kolonu | Struct Alanı | Frontend Alanı | Durum |
|-------|-----------|--------------|----------------|-------|
| 0 | strUserId | char_id | char_id | ✅ OK |
| 1 | Level | level | level | ✅ OK |
| 2 | RebLevel | reb_level | reb_level | ⚠️ RebLevel USERDATA'da var mı? |
| 3 | Exp | exp | exp | ✅ OK |
| 4 | Gold | gold | gold | ✅ OK |
| 5 | Loyalty | loyalty | loyalty | ✅ OK |
| 6 | Strong | str_ | str_ | ✅ OK |
| 7 | Dex | dex | dex | ✅ OK |
| 8 | HP | hp | hp | ✅ OK |
| 9 | Intel | intel | intel | ✅ OK |
| 10 | MP | mp | mp | ✅ OK |
| 11 | Points | points | points | ✅ OK |
| 12 | AttackPoint | sol_np | sol_np | ⚠️ AttackPoint ≠ NP |
| 13 | DefensePoint | sag_np | sag_np | ⚠️ DefensePoint ≠ NP |
| 14 | Authority | authority | authority | ✅ OK |
| 15 | Zone | zone | zone | ✅ OK |

**Tespit:** AttackPoint/DefensePoint → sol_np/sag_np eşleşmesi anlambilimsel olarak yanlış.
Bunlar NP değil, savaş puanı/savunma puanı. Frontend "Sol NP / Sag NP" etiketiyle yanlış gösteriyor.

---

### 6. DailyQuestEditor — İKİ AYRI TABLO, İKİ AYRI ŞEMA

**editors.rs::quest_listesi (iQuestID şeması):**
```sql
SELECT iQuestID, strName, iKillCount, iRewardExp, iRewardLoyalty, bEnabled FROM DAILY_QUESTS
```

**main.rs::daily_quest_listesi (nID şeması — FARKLI):**
```sql
SELECT nID, strName, nQuestID, nTimeType, nKillType, nMobID1-4, nKillCount1, nReward1-4, nCount1-4, nZoneID, nMinLevel, nMaxLevel, nReplayTime, nRandomID FROM DAILY_QUESTS
```

**Tespit:** DAILY_QUESTS tablosuna iki farklı invoke, iki farklı şema ile erişiyor.
- `ed_quest_listesi` (editors.rs) → basit şema, iQuestID primary key
- `daily_quest_listesi` (main.rs) → gelişmiş şema, nID primary key, 23 kolon

**Bu iki invoke aynı tablo mu farklı tablo mu kullanıyor?** Kod `[{gdb}].dbo.DAILY_QUESTS` diyor, aynı tablo.
**Şema çelişkisi:** Tablo hangi şemada? iQuestID mi nID mi? Biri çalışır, diğeri hata verir.

---

### 7. MAIL Tablosu — TB_ITEM_MAIL vs MAIL_BOX

**Pipeline haritası diyor:** `TB_ITEM_MAIL`

**Gerçek kod (item2.rs):**
```sql
mail_gonder → EXEC MAIL_BOX_SEND ...  (SP ile)
mail_listesi → MAIL_BOX (kolon: nIndex, strSender, strRecipient, strSubject, dtSentDate, bRead, bType, nItemID, sCount, bDeleted)
```

**Tespit:** Pipeline haritası `TB_ITEM_MAIL` ve `MAIL` diyor — gerçekte `MAIL_BOX_SEND` SP ve `MAIL_BOX` tablosu kullanılıyor.

---

### 8. parse_satirlar() — GENEL SORUN

Tüm modüllerdeki `parse_satirlar()` fonksiyonu:
```rust
.filter(|l| !t.is_empty() && !t.starts_with("---") && !t.starts_with('(') && t != "NULL")
```

**Sorun:** sqlcmd çıktısında `NULL` değeri olan satır atlanıyor (`t != "NULL"`).
Eğer bir satırın tek kolonu NULL ise, o satır tamamen drop edilir.
**Bu, COUNT(*) sorgularında 0 döndürmesi gerekirken satırın silinmesine yol açabilir.**

Ayrıca `---` filtresi separator satırını atar — bu doğru. Ama `(` ile başlayan `(1 rows affected)` satırı da doğru atılıyor.

---

## SORUN ÖZET TABLOSU (RUSTIK için Fix Listesi)

| # | Konum | Sorun | Öneri |
|---|-------|-------|-------|
| 1 | RUSTIK_SQL_PIPELINE_HARITA.md | EventKontrol tablo adları yanlış | SERVER_EVENT → EVENT_OPEN, SERVER_EVENT_TIMER → EVENT_OPEN, CHALLENGE_RANKING → COLLECTION_RACE_EVENT_SETTINGS, CHALLENGE_RANKING_REWARD → EVENT_REWARDS |
| 2 | RUSTIK_SQL_PIPELINE_HARITA.md | KlanKralik tablo adları yanlış | GUILD_HALL → CLAN (listesi/notice/gold için) |
| 3 | RUSTIK_SQL_PIPELINE_HARITA.md | Mail tabloları yanlış | TB_ITEM_MAIL → MAIL_BOX, MAIL → MAIL_BOX_SEND (SP) |
| 4 | gm2.rs::hesap_ara | ACCOUNT_CHAR'da nKCash/nBonusCash/strEmail/strIP olmaması | DB'de kolon varlığını doğrula, gerekirse TB_USER JOIN ekle |
| 5 | gm2.rs::karakter_ara | AttackPoint→sol_np, DefensePoint→sag_np yanlış mapping | Frontend etiketi düzelt veya kolon adları düzelt |
| 6 | DAILY_QUESTS çakışması | editors.rs iQuestID şeması ≠ main.rs nID şeması | Hangi şema geçerliyse diğerini kaldır |
| 7 | gm2.rs (tüm WHERE) | strUserId küçük d / strUserID büyük D tutarsızlığı | Standartlaştır |

---

## ÇALIŞAN (SORUNSUZ) INVOKE LİSTESİ

Kod analizi bazında hata riski düşük olanlar:

| Invoke | Neden OK |
|--------|---------|
| gm_oyuncu_ara | Kolon sırası struct ile uyuşuyor, 10 kolon beklenti 10 kolon SELECT |
| gm_oyuncu_detay | 23 kolon tam eşleşiyor |
| ban_listesi | 10 kolon tam eşleşiyor |
| gm_yetki_listesi | 25 bit kolon tam eşleşiyor |
| oyuncu_envanter | FetchUserItems SP → USER_ITEMS temp, sonra select; struct 6 kolon OK |
| upgrade_listesi | 5 kolon eşleşiyor |
| mining_listesi | 7 kolon eşleşiyor |
| quest_listesi (editors.rs) | 6 kolon eşleşiyor |
| level_odul_listesi | 4 kolon eşleşiyor |
| jackpot_listesi | 6 kolon eşleşiyor |
| npc_listesi / npc_detay | 43 kolon tam eşleşiyor |
| mob_listesi | 2 kolon, OK |
| item_ara | 4 kolon, OK |
| sunucu_istatistik | COUNT(*) sorgular, OK |

---

## KÖK SEBEP ANALİZİ

RUSTIK_SQL_PIPELINE_HARITA.md büyük olasılıkla:
1. Eski bir KO server şemasından referans alınarak yazıldı
2. Gerçek event2.rs, klan.rs kodları okunmadan oluşturuldu
3. "SERVER_EVENT" gibi isimler tahmin/şablondan geldi

**Gerçek KO_MYKO tabloları:** EVENT_OPEN, COLLECTION_RACE_EVENT_SETTINGS, TIMED_NOTICE, EVENT_REWARDS, CLAN, MAIL_BOX

---

## EYLEM ÖNERİLERİ (RUSTIK FAZ-3 için)

**Öncelik 1 — Acil (Panel çalışmıyor):**
- EventKontrol sekmesi muhtemelen hata döndürüyordur. EVENT_OPEN tablosu KO_MYKO'da var mı doğrula.
- hesap_ara: ACCOUNT_CHAR.nKCash kolonunu doğrula, yoksa `SELECT nKCash FROM KO_MYKO.INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME='ACCOUNT_CHAR'`

**Öncelik 2 — Orta:**
- DAILY_QUESTS şema çakışmasını çöz (iQuestID vs nID)
- strUserId/strUserID tutarsızlığını düzelt
- AttackPoint/DefensePoint frontend etiketlerini düzelt

**Öncelik 3 — Temizlik:**
- RUSTIK_SQL_PIPELINE_HARITA.md güncelle (doğru tablo adlarıyla)

---

**Bynoisee © MalaysiaKO 2026 — MATRIX MAT-21 Kolon Mismatch Raporu**
