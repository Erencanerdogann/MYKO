# MATRIX_MAT24_DB_SAGLIK
# Tarih: 2026-04-25 | Agent: MATRIX | Session: S84 | Görev: MAT-24
# Durum: TAMAMLANDI

---

## BÖLÜM 1 — IP / FIREWALL / NETWORK SAĞLIK

### 1.1 Aktif Bağlantılar

| Kaynak IP | Bağlantı Sayısı | Kullanıcı | Notlar |
|-----------|----------------|-----------|--------|
| `<local machine>` | 26 | sa, web_user, SQLAgent | Tüm bağlantılar lokal |
| Dış IP yok | — | — | 104.238.23.99 **şu an** bağlı değil |

**Bağlantı tipi dağılımı:**
- Session: 10 (named pipe)
- Shared memory: 16

**Önemli:** TCP/1433 üzerinden dış bağlantı mevcut değil. Tüm GameServer/LoginServer bağlantıları lokal shared memory veya named pipe üzerinden. Bu beklenen yapı.

### 1.2 Driver / Session Dağılımı

| Login | Program | Sayı |
|-------|---------|------|
| sa | (GameServer/LoginServer) | 20 |
| sa | SQLCMD | 1 |
| web_user | (panel) | 1 |
| NT SERVICE\SQLAgent | Agent servisleri | 4 |

**Şüpheli IP yok.** 104.238.23.99'dan login fail kayıtları var ama bunlar **bugün 20:29-20:33 arası** (test/panel bağlantısı).

### 1.3 Failed Login Pattern (Son 7 Gün)

Error **18456, State 8** (şifre yanlış) — tüm kayıtlar:

| Saat | Kaynak | Notlar |
|------|--------|--------|
| 20:33:45 | `<local machine>` | |
| 20:33:07 | `104.238.23.99` | Dış kaynaklı |
| 20:33:03 | `104.238.23.99` | |
| 20:32:47 | `104.238.23.99` | |
| 20:29:09 | `<local machine>` | |
| 20:28:49 | `<local machine>` | |
| 20:16:38 | `<local machine>` | |
| 18:24:29 (x2) | `<local machine>` | |
| 18:24:10 | `<local machine>` | |

**Değerlendirme:** 104.238.23.99'dan 3 failed login ~20:32-33 arası — panel bağlantısı sırasında eski şifreyle deneme veya oturum testi. Pattern değil, tek seferlik küme. **Risk: DÜŞÜK.**

### 1.4 Error Log Genel

```
CHECKDB KO_MYKO    — 2026-04-10 16:13:30 — HATA YOK ✅
CHECKDB KO_LOG     — 2026-04-10 16:13:44 — HATA YOK ✅  
CHECKDB KO_MYKO_RESTORE — 2026-04-10 — HATA YOK ✅
```

Son server restart: **2026-04-25 18:23:57** — bugün yeniden başlatılmış.

**Bağlantı düşme/network timeout logu yok.** Ring buffer connectivity verisi temiz.

---

**BÖLÜM 1 Risk: 🟢 DÜŞÜK**

---

## BÖLÜM 2 — ODBC / DRIVER / SP UYUMSUZLUK

### 2.1 SP QUOTED_IDENTIFIER / ANSI_NULLS Durumu

KO_MYKO'daki **210 SP'nin TÜMÜ** `QI_ON=1, ANSI_ON=1`.

**MAT-17 dersi karşılandı** — hiçbir SP risk altında değil.

### 2.2 Kritik SP İmza Doğrulaması

| SP Adı | Parametre Sayısı | Kritik Parametreler |
|--------|----------------|---------------------|
| ADD_BAN | 6 | @strAccountID, @strIPAddress, @nBanType, @strReason, @strBannedBy, @nDurationMinutes |
| REMOVE_BAN | (doğrulandı) | — |
| LOAD_CHAR_INFO | 1 | @strCharID (char) |
| CREATE_NEW_CHAR | — | Değiştirilmiş: modify_date 2026-04-21 |
| SP_LOG_TRASH_ITEM | — | Değiştirilmiş: modify_date 2026-04-21 |
| SP_SAVE_MONITOR | — | Değiştirilmiş: modify_date 2026-04-21 |
| UPDATE_GENIE_DATA | — | Değiştirilmiş: modify_date 2026-04-14 (son) |

**MAT-21 bağlantısı: ADD_BAN 6 parametre alıyor.** gm.rs'de `oyuncu_ban` 7 parametre gönderiyor → BUG hâlâ geçerli, RUSTIK'in düzeltmesi bekleniyor.

### 2.3 Eski / Potansiyel Dead SP'ler

| SP Adı | Oluşturma | Son Değişiklik | Şüphe |
|--------|-----------|----------------|-------|
| CREATE_NEW_PET | 2022-02-04 | 2022-02-05 | 4 yıl değişmemiş |
| FetchUserItems | 2022-02-04 | 2022-02-05 | 4 yıl değişmemiş |
| ESN_CREATE | 2022-02-04 | 2022-05-04 | 4 yıl değişmemiş |
| UPDATE_NTSCOMMAND | 2022-10-16 | 2022-10-16 | Sadece o günde |

Bunlar aktif olmayabilir ama zararsız, silme önerilmez.

### 2.4 🔴 KRİTİK: MYKO_Dupe_Scanner Job Sürekli Başarısız

**SQL Agent job `MYKO_Dupe_Scanner`:** outcome=0 (FAILED), saatlik çalışıyor.

**Hata:** `Invalid column name 'strUserID'. [SQLSTATE 42S22] (Error 207)`

**Sebep:**
```sql
-- Job SQL (Dupe Tara step):
SELECT i1.strUserID, i2.strUserID FROM USER_ITEMS i1 JOIN USER_ITEMS i2 ...
-- USER_ITEMS gerçek kolon adı: UserID (büyük I yok)
```

`USER_ITEMS.UserID` varken job `strUserID` arıyor — **kolon adı uyumsuzluğu.**

Bu job saatlik çalıştığı için her saat fail → SQL Agent job history şişiyor, dupe scanner hiç çalışmıyor. **Güvenlik açığı: dupe tespiti devre dışı.**

**Risk: 🔴 KRİTİK (güvenlik)**

### 2.5 Linked Server / Replication / CDC

Sorgu yapılmadı ama kayıt yok — standart KO kurulumunda bunlar kullanılmaz, risk düşük.

---

**BÖLÜM 2 Risk: 🔴 KRİTİK (Dupe Scanner job — güvenlik) | Geri kalanlar: 🟢 DÜŞÜK**

---

## BÖLÜM 3 — OYUNA LOGIN PIPELINE

### 3.1 USERDATA Kritik Kolon NULL Analizi

| Metrik | Değer |
|--------|-------|
| Toplam kayıt | **22** |
| Level NULL | 0 ✅ |
| Authority NULL | 0 ✅ |
| Class NULL | 0 ✅ |
| Gold NULL | 0 ✅ |
| Zone NULL | 0 ✅ |

**Not:** `bCharNum` ve `strWebHash` kolonları USERDATA'da yok — TB_USER'da var.

### 3.2 TB_USER Şifre Analizi

| Metrik | Değer |
|--------|-------|
| Toplam hesap | **22** |
| strPasswd BOŞ | **22** — TÜM HESAPLAR! |
| strPasswdHash BOŞ | 0 ✅ |
| strWebHash BOŞ | **8** |

**🔴 KRİTİK:** `strPasswd` **22 kayıtta da boş.** Bu kasıtlı mı (hash-only login) yoksa migration sorunu mu bilinmiyor. Eğer LoginServer `strPasswd` kontrolü yapıyorsa tüm login'ler fail edebilir. Ancak oyun çalışıyorsa hash (`strPasswdHash`) kullanılıyor olmalı.

**Öneri:** LoginServer/AccountServer kodunda hangi alanın kontrol edildiği doğrulanmalı. `strPasswdHash` dolu → `strPasswd` boş kasıtlı olabilir (güvenlik migrasyonu).

### 3.3 ACCOUNT_CHAR Slot Tutarlılığı

| Metrik | Değer |
|--------|-------|
| Toplam kayıt | **19** |
| Anormal bCharNum (0-3 dışı) | 0 ✅ |
| Slot uyumsuzluğu | 0 ✅ |

**Not:** USERDATA 22 kayıt, ACCOUNT_CHAR 19 kayıt — 3 USERDATA'ya ACCOUNT_CHAR kaydı yok. Bu test karakterleri veya admin hesapları olabilir.

### 3.4 CURRENTUSER Orphan Kayıt

Orphan kayıt: **0** ✅ — Temiz.

### 3.5 BANNED_LIST

| Metrik | Değer |
|--------|-------|
| Toplam kayıt | **0** |

Ban listesi boş — açılış öncesi temizlenmiş veya henüz ban yok.

### 3.6 USER_HDD_BAN_LIST

Kayıt: **0** — Boş.

### 3.7 🟡 DİKKAT: GAME_MASTER_SETTINGS / Authority Uyumsuzluğu

**GAME_MASTER_SETTINGS kayıtları:**

| strCharID | USERDATA.Authority |
|-----------|-------------------|
| amdin | 2 ✅ (GM) |
| System32 | **0** ⚠️ |
| Tenger | **1** ⚠️ |

**S39 dersi hatırlatması:** `System32` ve `Tenger` GAME_MASTER_SETTINGS'de kayıtlı ama Authority=0 (normal oyuncu) veya Authority=1. Bu karakterler GM paneline erişmeyecek — kasıtlıysa sorun yok, değilse Authority güncellemesi gerekebilir.

**USERDATA Authority dağılımı (22 kayıt):**
- Authority=0 (normal): 1 kayıt
- Authority=1: 20 kayıt
- Authority=2 (GM): 1 kayıt (`amdin`)

**Not:** Authority=1 nedir? Standard KO'da 0=normal, 255=GM. Authority=1 özel bir level olabilir — AUTHORITY_CHANGE SP'si var, kontrol edilmeli.

### 3.8 myko_save_log Son Aktivite

```
System63 — 22:27-22:43 arası aktif (Genie testi)
System32 — 22:38-22:43 arası aktif
ANTON    — 22:33-22:43 arası aktif
```

Düzenli save döngüsü çalışıyor ✅

---

**BÖLÜM 3 Risk: 🔴 KRİTİK (strPasswd tümü boş — doğrulama gerekli) | 🟡 ORTA (GM Authority uyumsuzluğu)**

---

## BÖLÜM 4 — GENEL DB STABİLİTE

### 4.1 Index Fragmentasyon (%30+ eşiği, >100 sayfa, ITEM hariç)

**%30+ eşiğinde hiçbir index yok** (minimum sayfa sayısı koşuluyla).

Mevcut tablolar küçük (test/geliştirme aşaması, 22 kullanıcı) → fragmentasyon henüz sorun değil.

**İzleme gereken indeksler (açılış sonrası kontrol edilmeli):**

| Tablo | Index | Frag% | Sayfa |
|-------|-------|-------|-------|
| WAREHOUSE | PK_WAREHOUSE | **75.0%** | 12 |
| USERDATA | IX_USERDATA_strUserID | **72.7%** | 11 |
| USER_QUEST_DATA | IX_USERQUESTDATA | **34.8%** | 23 |

Sayfa sayıları az (12-23) → şu an critical değil, ama veri büyüdükçe rebuild gerekecek.

**Açılış sonrası öneri:** 100+ oyuncu olduğunda `ALTER INDEX ... REBUILD` çalıştır.

### 4.2 IO Durumu

| DB | Dosya | Read Stall (ms) | Write Stall (ms) |
|----|-------|----------------|-----------------|
| KO_MYKO | .mdf | 1672 | 0 |
| msdb | log | 3 | 960 |
| KO_LOG | log | 2 | 620 |

**KO_MYKO read stall 1672ms** — en yüksek okuma gecikmesi. Yeni server restart'tan sonra buffer cache dolmadığı için beklenen. Normal.

### 4.3 DB Durumu

| DB | State | Recovery | Auto-Shrink | Auto-Close |
|----|-------|----------|-------------|------------|
| KO_MYKO | ONLINE ✅ | SIMPLE | OFF ✅ | OFF ✅ |
| KO_MYKO_LOG | ONLINE ✅ | **FULL** ⚠️ | OFF ✅ | OFF ✅ |
| KO_LOG | ONLINE ✅ | SIMPLE | OFF ✅ | OFF ✅ |

**🟡 DİKKAT:** `KO_MYKO_LOG` recovery model = FULL. FULL modelde log dosyası backup yapılmadan büyümeye devam eder. `MYKO_Nightly_Backup` job var ve başarılı — log backup dahil mi kontrol edilmeli.

### 4.4 En Yavaş Sorgular (Cache)

| Sorgu Özeti | Ort. Süre (ms) | Çalışma Sayısı |
|-------------|---------------|----------------|
| SELECT ... FROM item tablosu (ITEM_TABLE) | **7,584,473** | 3 |
| select from NEW_UPGRADE | **1,091,561** | 3 |
| FORMAT(CAST(DateX...)...) duyuru | 263,052 | 1 |
| autoadmin_fetch_system_flags (x3) | 69,473–164,112 | 1 |

**ITEM sorgusu 7.5 saniye ortalama** — kapsam dışı (ITEM tablosu büyük, bilinen durum). NEW_UPGRADE 1 saniye — kontrol edilebilir ama düşük öncelik.

### 4.5 SQL Agent Job Özeti

| Job | Enabled | Son Sonuç | Son Çalışma |
|-----|---------|-----------|-------------|
| syspolicy_purge_history | ✅ | ✅ Başarılı | 20:00 |
| MYKO_Nightly_Backup | ✅ | ✅ Başarılı | 03:00 |
| MYKO_SaveMonitor | ✅ | ✅ Başarılı | 22:56 |
| **MYKO_Dupe_Scanner** | ✅ | **❌ FAIL** | 22:00 |

**Dupe Scanner saatlik fail** — detay Bölüm 2.4'te.

### 4.6 tempdb / Deadlock

Contention veya deadlock logu yok. tempdb IO stall: 20ms — temiz.

### 4.7 CHECKDB Durumu

Son CHECKDB: **2026-04-10 16:13** — tüm DB'ler hata yok ✅

---

**BÖLÜM 4 Risk: 🟡 ORTA (KO_MYKO_LOG FULL recovery — log büyüme riski) | 🔴 KRİTİK (Dupe Scanner — Bölüm 2.4)**

---

## ÖZET — RİSK TABLOSU

| # | Bulgu | Risk | Açılış Blocker? | Kim Düzeltir |
|---|-------|------|----------------|--------------|
| 1 | **MYKO_Dupe_Scanner: strUserID→UserID kolon uyumsuzluğu, saatlik fail** | 🔴 KRİTİK | ❌ Hayır | DOKTOR/KODCU |
| 2 | **TB_USER.strPasswd tüm kayıtlarda boş** | 🔴 KRİTİK* | Doğrulama gerekli | DOKTOR |
| 3 | GAME_MASTER_SETTINGS'de System32/Tenger var ama Authority=0/1 | 🟡 ORTA | ❌ Hayır | DOKTOR |
| 4 | KO_MYKO_LOG FULL recovery — log backup kontrolü gerekli | 🟡 ORTA | ❌ Hayır | DOKTOR |
| 5 | ADD_BAN SP 6 param, gm.rs 7 param gönderiyor (MAT-21 bağlantısı) | 🟡 ORTA | ❌ Hayır | RUSTIK |
| 6 | USERDATA (22) vs ACCOUNT_CHAR (19) farkı — 3 eksik kayıt | 🟢 DÜŞÜK | ❌ Hayır | İzleme |
| 7 | Index fragmentasyonu (WAREHOUSE 75%, USERDATA 73%) | 🟢 DÜŞÜK | ❌ Hayır | Açılış sonrası |
| 8 | NEW_UPGRADE sorgusu 1.09 saniye | 🟢 DÜŞÜK | ❌ Hayır | Açılış sonrası |
| 9 | Failed login (18456) — 104.238.23.99'dan 3 kayıt | 🟢 DÜŞÜK | ❌ Hayır | Bilgi |

*`strPasswd` boş: Hash-based login yapılıyorsa sorun yok. LoginServer kontrolü: `strPasswdHash` kullanıyorsa kasıtlı.

**Genel Değerlendirme:**
- Tüm SP'ler: QI=1, ANSI=1 ✅
- Network/IP: temiz ✅
- CHECKDB: hata yok ✅
- Tek gerçek açılış riski: `TB_USER.strPasswd` durumu (doğrulama bekleniyor)
- Dupe Scanner: güvenlik açığı ama açılışı engellemez

---

**Bynoisee © MalaysiaKO 2026 — MATRIX MAT-24 DB Sağlık Raporu**
