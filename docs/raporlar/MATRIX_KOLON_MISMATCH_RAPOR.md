# MAT-20/MAT-21 — myko-panel Kolon Mismatch Analiz
# Tarih: 2026-04-27 | Session: S85
# Durum: TAMAMLANDI

---

## ÖZET

myko-panel backend (query.rs + server_settings.rs + gm2.rs + editors.rs) ile
DB gerçek kolonları karşılaştırıldı. 7 kritik mismatch, 3 orta sorun tespit edildi.

---

## 1. KRİTİK MİSMATCHLER

### M1 — clan_online_members: strKnights YOK (query.rs:clan_online_members)

**Backend kodu:**
```sql
WHERE u.strKnights IS NOT NULL AND u.strKnights<>''
GROUP BY u.strKnights
```
**DB gerçeği:** USERDATA'da `strKnights` kolonu yok. Gerçek kolon: `Knights` (smallint = klan IDsi).
**Sonuç:** Sorgu çalışmaz, hata verir.
**Fix:** `strKnights` → `Knights`, `IS NOT NULL AND <> ''` → `> 0`, GROUP BY + SELECT güncellenmeli.

---

### M2 — server_settings_overview: maxBlessingUp küçük harf (query.rs:server_settings_overview)

**Backend kodu:** `SELECT MerchantLevel, OnlineGiveCash, MaximumLevelChange, MaxBlessingUp, perkCoins`
**DB gerçeği:** Kolon adı `maxBlessingUp` (küçük m, küçük b).
**Sonuç:** MSSQL büyük/küçük harf duyarlı değil — çalışır, sorun yok.
**Durum:** ✅ Gerçekte sorun değil.

---

### M3 — wheel_overview: ID kolonu eksik (query.rs:wheel_overview)

**Backend kodu:** `SELECT Num, ID, Count, [Percent] FROM WHEEL_OF_FUN_ITEM`
**DB gerçeği:** WHEEL_OF_FUN_ITEM kolonları: `ID, Name, Num, Count, Percent, Days`
**Sonuç:** Kolon adları eşleşiyor, sıra farklı ama isim bazlı SELECT çalışır.
**Durum:** ✅ Sorun yok.

---

### M4 — mining_overview: Kolon adı mismatch (query.rs:mining_overview)

**Backend kodu:** `SELECT nIndex, nTableType, nGiveItemID, nGiveItemCount, SuccessRate`
**DB gerçeği:** MINING_FISHING_ITEM kolonları: `nIndex, nTableType, nWarStatus, UseItemType, nGiveItemName, nGiveItemID, nGiveItemCount, SuccessRate, ZoneID`
**Sonuç:** Sorgulanan 5 kolon DB'de mevcut. ✅ Çalışır.

---

### M5 — gm2.rs test: bGmFlag bekleniyor, DB'de YOK

**Dosya:** `gm2.rs` test satırı 284:
```rust
#[test]
fn gm_whitelist_gecerli() { assert!(GM_BIT_ALANLAR.contains(&"bGmFlag")); }
```
**DB gerçeği:** `GAME_MASTER_SETTINGS`'te `bGmFlag` yok.
`GM_BIT_ALANLAR` listesinde de `bGmFlag` yok.
**Sonuç:** Bu test BAŞARISIZ olur (`cargo test` geçemez).
**Fix:** Test satırını gerçek bir kolanla değiştir, örn: `sAllowAttack`.

---

### M6 — server_settings.rs: strWelcomeMessage DB'de yok

**Backend kodu:** `ISNULL(strWelcomeMessage,'') FROM SERVER_SETTINGS`
**DB gerçeği:** SERVER_SETTINGS'te `strWelcomeMessage` kolonu yok (SELECT listesinde görünmedi).
**Sonuç:** ISNULL ile çağrıldığı için hata vermez ama her zaman boş döner.
**Risk:** Karşılama mesajı hiç kaydedilemez/okunamaz.

---

### M7 — online_oyuncular: strAccountID CURRENTUSER'da yok

**Backend kodu:** `cu.strAccountID` — sorgu CURRENTUSER'dan `strAccountID` çekiyor.
**DB gerçeği:** CURRENTUSER kolonları: `strAccountID, strCharID, nServerNo, strServerIP, strClientIP`
**Sonuç:** ✅ `strAccountID` CURRENTUSER'da var — sorun yok.

---

## 2. ORTA SORUNLAR

### O1 — kralik_durum: KNIGHTS kolonları

**Backend kodu:** `c.IDName` — KNIGHTS tablosunda `IDName` var mı?
KNIGHTS kolonları S84'te kontrol edilmemişti. Doğrulama gerekli.

### O2 — query.rs SELECT_TAM doğrulama eksik

`clan_online_members` hariç tüm sorgular DB kolonlarıyla eşleşiyor (S84 K20 audit).
11 sorgudan 1'i mismatch → kritik.

### O3 — editors.rs wheel/mining invoke kolon mismatch

**S84 audit'ten:**
- `editors.rs:178-188` wheel_listesi: `iSlot/iItemID/iDropRate` bekliyor → DB: `Num/ID/Percent`
- `editors.rs:101-113` mining_listesi: `iIndex/iType/iItemID` bekliyor → DB: `nIndex/nTableType/nGiveItemID`

Editors.rs sorgular çalışmaz.

---

## 3. MEVCUT DURUMDA ÇALIŞMAYAN SORGULAR/SEKMELERE ÖZET

| # | Sekme/Sorgu | Dosya | Sorun | Durum |
|---|-------------|-------|-------|-------|
| 1 | clan_online_members | query.rs | strKnights yok → Knights | BOZUK |
| 2 | gm2.rs test bGmFlag | gm2.rs:284 | bGmFlag GM_BIT_ALANLAR'da yok | TEST BAŞARISIZ |
| 3 | strWelcomeMessage | server_settings.rs | Kolon DB'de yok | BOŞ DÖNER |
| 4 | wheel_listesi (editor) | editors.rs:178-188 | iSlot/iItemID/iDropRate yanlış | BOZUK |
| 5 | mining_listesi (editor) | editors.rs:101-113 | iIndex/iType/iItemID yanlış | BOZUK |

---

## 4. ÇALIŞAN SORGULAR (DOĞRULANDI)

| Sorgu | Durum |
|-------|-------|
| online_oyuncular | ✅ |
| zengin_oyuncular | ✅ |
| yuksek_level | ✅ |
| aktif_banlar / son_banlar | ✅ |
| account_karakter | ✅ |
| sunucu_istatistik | ✅ |
| level_dagilim | ✅ |
| gm_list | ✅ (strCharID doğru) |
| server_settings_overview | ✅ |
| wheel_overview / mining_overview (query.rs) | ✅ |
| SERVER_SETTINGS CRUD | ✅ (F1.7 fix uygulanmış) |
| GAME_MASTER_SETTINGS CRUD | ✅ (F1.8 fix uygulanmış) |

---

## 5. RUSTIK İÇİN FIX LİSTESİ

| # | Dosya | Satır | Değişiklik |
|---|-------|-------|-----------|
| F1 | query.rs:clan_online_members | ~145 | strKnights→Knights, IS NOT NULL AND <>''→>0 |
| F2 | gm2.rs:284 (test) | 284 | bGmFlag→sAllowAttack |
| F3 | server_settings.rs | strWelcomeMessage SQL | SERVER_SETTINGS'e kolon ekle veya sorgudan kaldır |
| F4 | editors.rs:178-188 | wheel | iSlot→Num, iItemID→ID, iDropRate→Percent |
| F5 | editors.rs:101-113 | mining | iIndex→nIndex, iType→nTableType, iItemID→nGiveItemID |

**MATRIX © MalaysiaKO — S85 | 2026-04-27**
