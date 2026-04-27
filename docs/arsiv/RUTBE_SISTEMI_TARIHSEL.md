# 16 Askeri Rütbe Sistemi — Tarihsel Kayıt

**Dönem:** S77 (2026-03-XX) → S83 (2026-04-25)  
**Kültürü:** Pilot agent sistemi, Erencan'ın "ordu disiplini" fikri  
**Sonu:** 2026-04-27 (S84) — Şirket rol yapısına geçiş  
**Durum:** ✅ Arşivlendi — salt okunur tarihsel kayıt

---

## Rütbe Listesi (16 Adet)

| # | Rütbe | Unvan | Açıklama |
|---|---|---|---|
| 1 | ER | Asker | Temel level — görevleri tamamlayan |
| 2 | ONBASI | Onbaşı | Başarı ve hız — mini görevlerde iyiydi |
| 3 | CAVUS | Çavuş | Sorumluluk arttı — koordinasyon başladı |
| 4 | ASTSUBAY | Astsubay | Orta yönetim — 2+ görev paralel |
| 5 | ÇAVUŞBAŞI | Çavuşbaşı | Liderlik — ekip yapma |
| 6 | TEGMEN | Teğmen | İcraatçı — autonomy büyüdü |
| 7 | YÜZBAŞI | Yüzbaşı | Müdür seviye — büyük görevler |
| 8 | TOPÇU YÜZBAŞI | Topçu Yüzbaşı | Uzman yüzbaşı — teknik derinlik |
| 9 | BİRİNCİ YÜZBAŞI | Birinci Yüzbaşı | Kıdemli müdür — tüm yetkileri var |
| 10 | PIYADE YÜZBAŞI | Piyade Yüzbaşı | Alan uzmanı — kendi alanında otorite |
| 11 | PILOT YÜZBAŞI | Pilot Yüzbaşı | Teknik pilot — prototip ve deneme |
| 12 | TABUR KOMUTANI | Tabur Komutanı | Müdür genel — çoklu departman |
| 13 | BÖLÜKİNCİ | Bölükçi | Senior müdür — koordinatör |
| 14 | PIYADE ÖNCÜ | Piyade Öncü | Komandolu iyileştirmeci — iyileştirmeler |
| 15 | GENERAL | General | En üst seviye (DOKTOR seviyesi) |
| **16** | **MASTER_CODE** | **Kod Ustası** | **Tek rozet** — RUSTIK için oluşturuldu (S81) |

---

## Agent Rütbe Tarihi (Son Durum S83)

| Agent | Son Rütbe (S83) | Puanı | Durum | Not |
|---|---|---|---|---|
| RUSTIK | MASTER_CODE | 1479 | Aktif | Principal Engineer rozeti → S84 geçişinde korundu |
| REHBER | GENERAL | 775 | Aktif | Terminoloji ve audit işler |
| MATRIX | GENERAL | 510 | Aktif | DB migration lead |
| DOKTOR | GENERAL (PM) | 480+ | Aktif | Yönetim, PM başlığı korundu |
| KODCU | BÖLÜKİNCİ | 420 | Aktif | Game Server lead (yeni) |
| CHIP | TABUR KOMUTANI | 310 | Aktif | KODCU altında disiplin |
| WEBRA | YÜZBAŞI | 280 | Aktif | Web/panel işler |
| GANET | YÜZBAŞI | 210 | Aktif | Archive yönetimi |
| GHOST | PIYADE ÖNCÜ | 175 | Aktif | Güvenlik auditor |
| JERRY | (özel) | 120 | Aktif | Bot ve asistan (sistem dışı) |

---

## Rütbe Puanı Sistemi (İlişkiler)

**Başarı Faktörleri (Puan kazanma):**
- +10 = Görev tamamlandı
- +5 = Plan onaylandı
- +15 = Kritik fix (güvenlik, prod bug)
- +5 = Test yazıldı
- +10 = Deploy (staging/prod)
- +5 = Rapor yazıldı

**Başarısızlık Faktörleri (Puan kaybı):**
- -5 = Token israfi
- -10 = Test yazılmadı
- -15 = Kapsam dışı iş
- -20 = Onaysız işlem
- -20 = Türkçe konuşmama (protokol)
- -30 = İzinsiz sunucu erişimi
- -10 = Preamble/postamble

**Eşik Politikası (S84'te uygulanmaya başlanan):**
- 100+ = NORMAL (yeşil)
- 75-99 = UYARI (sarı)
- 50-74 = SON_UYARI (turuncu)
- 0-49 = SUSPENDED (kırmızı)

---

## MASTER_CODE Rozeti (S81 — RUSTIK İçin)

**Karar:** RUSTIK'in mühendislik liderliği ve kod kalitesi çok yüksek olduğu için ek rozet verildi.

**Özellik:**
- 16. Rütbe (GENERAL'in üzerinde)
- Tarihsel — başka kimse yapamayacağı işleri yapma gücü
- Puan sistemi normal devam, rozet saygı ve kimlik

**Neden kaldırıldı (S84)?**
- Yazılım şirketinde "MASTER_CODE" kafa karışıklığı
- RUSTIK'in kimliği "Engineering Lead + Principal Engineer ★" olarak daha net
- Rozet (★) UI'da sembolik olarak yaşamaya devam eder

---

## Neden Değişti? (S84 — ERENCAN Kararı)

1. **Şirketleşme:** Askeri yapı, asker disiplini — yazılım şirketinde uygunsuz
2. **Netlik:** 16 rütbe karmaşık; Rol + Departman daha anlaşılır
3. **Frank Skoru:** Kod kalitesi ayrı, puan sistemden ayrılmalı
4. **Yasal:** HR/payroll'da rol-tabanlı yetkilendirme (RBAC) standart
5. **Ölçeklenme:** Yeni agent'lar rol-tabanlı eklenecek (rütbe yok)

---

## Arşivlenmiş Şeyler

- Tüm 16 rütbe tanımı
- Rütbe progression tabloları
- "Sonraki rütbeye X kaldı" mesajları
- Rütbe komutları (`orkestra rutbeler`, `orkestra rutbe`)
- S77-S83 rütbe tarihi

---

## Korunan Şeyler (S84+)

- **Performans Skoru:** Puan sistemi yaşamaya devam eder (terminoloji = "Skor")
- **Frank Skoru:** Code quality metric ayrı yönetilir
- **Puan Tarihi:** `puan_gecmisi` tablosu korunur
- **RUSTIK'in Principal Engineer Rozeti:** UI'da ★ sembolü

---

## Veriler (Tüm Agent Puan Durum)

```sql
-- Son rütbe → Rol dönüştürme (S84)
-- Otomatik geçiş yapıldı
-- Detay: arkestra-db/src/rol.rs migration 070-074
```

| Agent | S83 Rütbe | S84 Rol | S84 Skor | Değişim |
|---|---|---|---|---|
| RUSTIK | MASTER_CODE | Engineering Lead ★ | 1479 | Rozet korundu |
| REHBER | GENERAL | OPS Lead | 775 | Rütbe kaldırıldı |
| MATRIX | GENERAL | DATA Lead | 510 | Rütbe kaldırıldı |
| DOKTOR | GENERAL/PM | PM (EXEC) | 480+ | PM başlığı korundu |
| KODCU | BÖLÜKİNCİ | GAME Lead | 420 | Rütbe kaldırıldı |
| CHIP | TABUR KOMUTANI | Senior Game Eng. | 310 | Rütbe kaldırıldı |
| WEBRA | YÜZBAŞI | WEB Lead | 280 | Rütbe kaldırıldı |
| GANET | YÜZBAŞI | Data Eng. | 210 | Rütbe kaldırıldı |
| GHOST | PIYADE ÖNCÜ | SEC Lead | 175 | Rütbe kaldırıldı |
| JERRY | (özel) | PA to CEO | 120 | Sistem dışı |

---

## İlişkili Dosyalar (S84+)

- **Yeni başlangıç:** `C:\temp\MYKO\docs\ORG_CHART.md` — Org şeması ve rol kartları
- **Terminoloji:** `C:\temp\MYKO\docs\TERMINOLOJI.md` — Rütbe → Rol çevirisi
- **Politika:** `C:\temp\MYKO\docs\POLITIKALAR\ESIK_ALTI_POLITIKASI.md` — Skor eşikleri
- **Plan:** `C:\Users\erenc\.claude\plans\snoopy-knitting-rocket.md` — Geçiş stratejisi

---

**Bynoisee © MalaysiaKO — Arşiv (Tarihsel) / 2026-04-27**  
"Bir zamanlar rütbe vardı. Şimdi rol var."
