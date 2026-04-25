# MATRIX_MAT25_DUPE_SCANNER_FIX
# Tarih: 2026-04-25 | Agent: MATRIX | Session: S84 | Görev: MAT-25
# Durum: TAMAMLANDI

---

## Özet

`MYKO_Dupe_Scanner` SQL Agent job'ının "Dupe Tara" adımı `USER_ITEMS.strUserID` kolonunu arıyordu.
Gerçek kolon adı `UserID`. Kolon adı düzeltildi, job başarılı çalıştı.

---

## Kök Sebep

| | Değer |
|--|--|
| Job | MYKO_Dupe_Scanner |
| Step | Dupe Tara (step_id=1) |
| Hata | `Invalid column name 'strUserID'` (SQLSTATE 42S22) |
| Neden | `USER_ITEMS` tablosunda `strUserID` yok, kolon adı `UserID` |
| Süre | Her saat başı fail — ne kadar süredir bilinmiyor |

---

## Yapılan Değişiklik

**Yöntem:** `msdb.dbo.sp_update_jobstep`

**Değişen satırlar (3 yerde):**
```sql
-- ÖNCE (hatalı):
i1.strUserID,
i2.strUserID,
AND i1.strUserID != i2.strUserID

-- SONRA (düzeltilmiş):
i1.UserID,
i2.UserID,
AND i1.UserID != i2.UserID
```

**Tam düzeltilmiş SQL:**
```sql
INSERT INTO DUPE_LOG (SerialHex, UserID1, UserID2, ItemID, DetectedAt)
SELECT
    CONVERT(NVARCHAR(100), i1.ItemSerial, 2),
    i1.UserID,
    i2.UserID,
    i1.nItemID,
    GETDATE()
FROM USER_ITEMS i1
JOIN USER_ITEMS i2
    ON i1.ItemSerial = i2.ItemSerial
    AND i1.UserID != i2.UserID
    AND i1.ItemSerial IS NOT NULL
WHERE NOT EXISTS (
    SELECT 1 FROM DUPE_LOG d
    WHERE d.SerialHex = CONVERT(NVARCHAR(100), i1.ItemSerial, 2)
    AND d.Resolved = 0
)
```

**Yedek:** `C:\temp\MYKO\db\MAT25_dupe_scanner_oncesi.sql`

---

## Test Sonucu

```
Job manuel çalıştırıldı: 2026-04-25 23:16:39
run_status = 1 (BAŞARILI)
The Job was invoked by User sa. The last step to run was step 1 (Dupe Tara).
```

**Önceki durum:** run_status=0, Her saat fail (schedule: Her1Saat)
**Sonraki durum:** run_status=1, Başarılı ✅

---

## Risk

**Düşük.** Job zaten fail durumdaydı, düzeltme dışında bir şey değişmedi.
DUPE_LOG tablosuna yeni kayıt eklemedi (USER_ITEMS şu an boş veya tekrarlayan serial yok).

---

**Bynoisee © MalaysiaKO 2026 — MATRIX MAT-25 Dupe Scanner Fix**
