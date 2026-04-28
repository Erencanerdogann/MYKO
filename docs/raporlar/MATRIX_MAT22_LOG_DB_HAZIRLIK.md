# MAT-21/MAT-22 — GameServer Log Analiz + Trilogy DB Hazırlık
# Tarih: 2026-04-27 | Session: S85
# Durum: TAMAMLANDI

---

## BÖLÜM C — GAMESERVER LOG ANALİZİ

### Log Dosyaları (C:\temp\MYKO\server\myko_server\Logs\)

| Dosya | Satır | İçerik |
|-------|-------|--------|
| GENERAL_2026-03-18.log | 16 | Sadece Server starting + ServerConfig |
| GENERAL_2026-03-19.log | 44 | Sadece Server starting + ServerConfig |
| GENERAL_2026-03-20.log | 36 | Sadece Server starting + ServerConfig |
| GENERAL_2026-03-21.log | 76 | Sadece Server starting + ServerConfig |
| GameServer.log | 6 | ODBC hatalar + CREATE_NEW_CHAR hatası |
| LoginServer.log | 40 | ODBC hatalar + CHECK_ACCOUNT PK ihlali |
| Login_18/19/20/21.log | 0 | Boş |

### Log Sistemi Hakkında

LogSystem.h'a göre ODBC/SP/QI/SQLBindParameter/timeout/deadlock hataları
`./Logs/ODBC_<tarih>.log` ve `HACK_<tarih>.log` dosyalarına yazılıyor.
Bu dosyalar yerel `C:\temp\MYKO\server\myko_server\Logs\` klasöründe **yok** —
sunucuda çalışan GameServer kendi `./Logs/` klasörüne yazıyor.
SSH erişimi olmadan görülemiyor.

### Tespit Edilen Hatalar (GameServer.log + LoginServer.log)

**HATA 1 — ODBC DSN Bulunamadı (KRİTİK)**
```
Source: SQLDriverConnect
Error: Veri kaynağı adı bulunamadı ve varsayılan sürücü belirtilmemiş
```
Açıklama: DSN konfigürasyonu eksik/yanlış. ODBC bağlantı kurulamıyor.
Eski test tarihi (Mart 2026) — mevcut sistemde düzeltilmiş olabilir.

**HATA 2 — CHECK_ACCOUNT PK İhlali (YÜKSEK)**
```
Source: CALL ACCOUNT_LOGIN(?, ?)
Error: Violation of PRIMARY KEY constraint 'PK_CHECK_ACCOUNT'
       Cannot insert duplicate key in object 'dbo.CHECK_ACCOUNT' (tenger)
```
5 kez tekrar etmiş. LOGIN SP aynı hesabı iki kez insert etmeye çalışıyor.
Kök sebep: SP içinde mevcut kayıt kontrolü yok veya CLEAR_REMAIN_USERS çalışmamış.

**HATA 3 — USER_ACHIEVE_LOAD_DATA Tablo Yok (ORTA)**
```
Source: CALL CREATE_NEW_CHAR(...)
Error: Invalid object name 'USER_ACHIEVE_LOAD_DATA'
```
5 kez tekrar etmiş. Tablo mevcut DB'de var (INFORMATION_SCHEMA'da görüldü).
Eski test döneminde (Mart 2026) tablo henüz oluşturulmamıştı.

**HATA 4 — QUEST_SKILLS_CLOSED_DATA Tablo Yok (ORTA)**
```
Source: CALL CREATE_NEW_CHAR(...)
Error: Invalid object name 'QUEST_SKILLS_CLOSED_DATA'
```
Tablo mevcut DB'de var. Aynı şekilde Mart 2026 öncesine ait.

**HATA 5 — Mimari Uyumsuzluk (BİLGİ)**
```
Error: DSN'de Sürücü ile Uygulama arasında mimari uyuşmazlık
```
32-bit uygulama 64-bit DSN'e veya tersi bağlanmaya çalışıyor.

**HATA 6 — CONCURRENT SELECT Çakışması (ORTA)**
```
Source: SELECT serverid, zone1_count... FROM CONCURRENT
Error: Connection is busy with results for another command
```
Aynı connection üzerinde paralel sorgu çakışması.

### Güncel Durum

Hata 3, 4 ve 5: Tablolar artık mevcut → Mart 2026 testlerinden kalma log.
Hata 1, 2, 6: Sunucuda hâlâ gerçekleşebilir — doğrulama için canlı log gerekli.

---

## BÖLÜM D — TRILOGY DB HAZIRLIK (RUS-66 FAZ-0/4)

### ADD_BAN / REMOVE_BAN SP İmzaları

**ADD_BAN:**
| Parametre | Tip | Yön |
|-----------|-----|-----|
| @strAccountID | varchar | IN |
| @strIPAddress | varchar | IN |
| @nBanType | tinyint | IN |
| @strReason | nvarchar | IN |
| @strBannedBy | varchar | IN |
| @nDurationMinutes | int | IN |

**REMOVE_BAN:**
| Parametre | Tip | Yön |
|-----------|-----|-----|
| @strAccountID | varchar | IN |
| @strIPAddress | varchar | IN |
| @strRemovedBy | varchar | IN |

---

### MONSTER_RESPAWN_STABLE_LIST Şeması

| Kolon | Tip | NULL |
|-------|-----|------|
| sIndex | smallint | NO |
| GroupNumber | smallint | NO |
| sSid | smallint | NO |
| isNpc | tinyint | NO |
| sZoneID | tinyint | NO |
| sCount | smallint | NO |
| sRadius | tinyint | NO |
| isDeadTime | int | NO |

Kayıt sayısı: 6

---

### ITEM_SELLTABLE Şeması

| Kolon | Tip |
|-------|-----|
| iSellingGroup | int |
| Item1-Item24 | int (24 kolon) |
| nIndex | int |

Toplam: 26 kolon. Kayıt sayısı: 3025

---

### QUEST_MENU_US Şeması

| Kolon | Tip | Boyut |
|-------|-----|-------|
| iNum | int | — |
| strMenu | char | 100 |

Kayıt sayısı: 12068

---

### QUEST_TALK_US Şeması

| Kolon | Tip | Boyut |
|-------|-----|-------|
| iNum | int | — |
| strTalk | char | 1000 |

Kayıt sayısı: 375

---

### CREATE_NEW_CHAR_SET Şeması

| Kolon | Tip | NULL |
|-------|-----|------|
| ID | int | NO |
| ClassType | tinyint | YES |
| SlotID | int | NO |
| ItemID | int | NO |
| ItemDuration | smallint | NO |
| ItemCount | smallint | NO |
| ItemFlag | tinyint | NO |
| ItemExpireTime | int | NO |

Kayıt sayısı: 375

---

## ÖZET

### C) Log Analizi

| Hata | Seviye | Güncel Risk |
|------|--------|-------------|
| ODBC DSN bulunamadı | KRİTİK | Doğrulanmalı |
| CHECK_ACCOUNT PK ihlali | YÜKSEK | Aktif risk |
| USER_ACHIEVE_LOAD_DATA | ORTA | Tablo artık var |
| QUEST_SKILLS_CLOSED_DATA | ORTA | Tablo artık var |
| Mimari uyumsuzluk | BİLGİ | Doğrulanmalı |
| CONCURRENT çakışması | ORTA | Aktif risk |

### D) Trilogy DB Hazırlık

| Tablo/SP | Durum |
|----------|-------|
| ADD_BAN | VAR — 6 parametre |
| REMOVE_BAN | VAR — 3 parametre |
| MONSTER_RESPAWN_STABLE_LIST | VAR — 8 kolon, 6 kayıt |
| ITEM_SELLTABLE | VAR — 26 kolon (24 item slot), 3025 kayıt |
| QUEST_MENU_US | VAR — 2 kolon, 12068 kayıt |
| QUEST_TALK_US | VAR — 2 kolon, 375 kayıt |
| CREATE_NEW_CHAR_SET | VAR — 8 kolon, 375 kayıt |

RUS-66 için tüm tablolar ve SP'ler mevcut. RUSTIK kullanabilir.

**MATRIX © MalaysiaKO — S85 | 2026-04-27**
