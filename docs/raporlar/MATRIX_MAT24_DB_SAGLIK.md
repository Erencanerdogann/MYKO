# MAT-24 — Açılış Öncesi DB Sağlık Check
# Tarih: 2026-04-27 | Session: S85
# Durum: TAMAMLANDI

---

## ÖZET

KO_MYKO + KO_MYKO_LOG kapsamlı tarama yapıldı. 1 kritik, 2 orta, 3 bilgi bulgusuna ulaşıldı.
Açılışı engelleyecek kritik bir yapısal sorun yok. Orphan karakter sorunu dikkat gerektiriyor.

---

## 1. VERİTABANI GENEL DURUM

| DB | Tablo Sayısı | Durum |
|----|-------------|-------|
| KO_MYKO | 226 | OK |
| KO_MYKO_LOG | 18 | OK (tüm tablolar boş — sunucu henüz açılmadı) |

---

## 2. KULLANICI / KARAKTER SAĞLIĞI

| Kontrol | Değer | Durum |
|---------|-------|-------|
| TB_USER (hesap) | 25 | OK |
| USERDATA (karakter) | 23 | OK |
| ACCOUNT_CHAR | 20 | OK |
| NULL strUserID | 0 | OK |
| Level 0 karakter | 0 | OK |
| Geçersiz Level (>200) | 0 | OK |
| Geçersiz Nation | 0 | OK |
| Negatif Gold | 0 | OK |
| Negatif Exp | 0 | OK |
| Duplicate strUserID | 0 | OK |

---

## 3. KRİTİK — ORPHAN KARAKTER (15 adet)

TB_USER'da hesabı olmayan USERDATA kayıtları:

| strUserID | Level | Nation |
|-----------|-------|--------|
| Tenger1 | 1 | El Morad |
| BYnoiseeII | 21 | El Morad |
| DOKTOR | 59 | El Morad |
| Lyrica | 60 | El Morad |
| ANTON | 66 | El Morad |
| MERGENN | 72 | El Morad |
| System63 | 72 | El Morad |
| System62 | 72 | El Morad |
| DEMIROV | 1 | Karus |
| MERGEN | 1 | Karus |
| TUFEK | 1 | Karus |
| XMET | 6 | Karus |
| Semih | 20 | Karus |
| IMAM | 72 | Karus |
| amdin | 72 | Karus |

Risk: GameServer bu karakterleri yükleyebilir ama login doğrulaması başarısız olur.
ANTON ve System63 şu an online (myko_save_log aktif).
Öneri: TB_USER hesapları oluşturulmalı VEYA test karakteri olduğu belgelenmeli.

---

## 4. ORTA — GAME_MASTER_SETTINGS KOLON FARKI

Tabloda strAccountID ve bAuthority yok. Mevcut: strCharID (karakter adına göre GM tanımı).
Risk: Backend strAccountID bekliyorsa çalışmaz.
Öneri: Backend kodunu strCharID ile eşleştir.

---

## 5. ORTA — BANNED_LIST KOLON FARKI

strUserID yok, asıl kolon: strAccountID. Tarih: dtBanStart/dtBanEnd (dtDate yok).
Aktif ban: 0 kayıt.
Risk: Backend/panel dtDate veya strUserID kullanıyorsa hata verir.

---

## 6. SQL AGENT JOB DURUMU

| Job | Enabled | Son Çalışma | Sonuç |
|-----|---------|-------------|-------|
| MYKO_Dupe_Scanner | EVET | 27.04.2026 21:00 | Başarılı |
| MYKO_Nightly_Backup | EVET | 27.04.2026 03:00 | Başarılı |
| MYKO_SaveMonitor | EVET | 27.04.2026 21:56 | Başarılı |
| syspolicy_purge_history | EVET | 27.04.2026 02:00 | Başarılı |

---

## 7. YEDEK DURUMU

13 USERDATA yedek tablosu: 14 Nisan - 27 Nisan 2026. En son: USERDATA_BK_20260427030000.

---

## 8. GAME_SERVER_LIST

Sunucu: ADONIS | IP: 104.238.23.99 | Port: 5000 (oyun) / 4000 (login) — Doğru.

---

## SONUÇ

| Önem | Bulgu | Aksiyon |
|------|-------|---------|
| KRİTİK | 15 orphan karakter (TB_USER hesabı yok) | Hesap oluştur veya belgele |
| ORTA | GAME_MASTER_SETTINGS strCharID kullanıyor | Backend kolon adını doğrula |
| ORTA | BANNED_LIST kolon adları farklı | Backend/panel kodu kontrol |
| BİLGİ | Tüm joblar çalışıyor | — |
| BİLGİ | 13 günlük yedek mevcut | — |
| BİLGİ | KO_LOG boş (açılış öncesi normal) | — |

Açılışı engelleyecek yapısal sorun YOK. Orphan karakterler risk oluşturabilir.

**MATRIX © MalaysiaKO — S85 | 2026-04-27**
