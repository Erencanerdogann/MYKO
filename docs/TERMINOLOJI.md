# MalaysiaKO Terminoloji — Yazılım Şirketi Yapısı

**Tarih:** 2026-04-27 (S84) | **Sözlükçü:** REHBER | **Versiyon:** v3.0 (Rütbe → Rol)

---

## 🔴 RÜTBE → ROL TERMİNOLOJİ GEÇİŞİ (S84 — ERENCAN Kararı)

**Karar:** S77-S83 arası 16 askeri rütbe sistemi (ER → MASTER_CODE) kullanıldı. S84'ten itibaren şirket rol yapısına geçildi.

### Terminoloji Haritası

| Eski (S77-S83) | Yeni (S84+) | Bağlam | Örnek |
|---|---|---|---|
| **Rütbe** | **Rol** | Agent'ın unvanı | RUSTIK = Engineering Lead + Principal Engineer |
| **Rütbe puanı** | **Performans Skoru** / **Skor** | Agent'ın başarı metriği (0-∞) | RUSTIK: Skor 524 |
| **Yükseldi, rütbe yükseldi** | **Skor güncellendi** | Puan artışı mesajı | "Skor güncellendi: 524 (+15)" |
| **Sonraki rütbeye X kaldı** | *(kaldırıldı)* | Artık kullanılmaz | — |
| **ER, ONBASI, CAVUS, ...** | *(arşivlendi)* | S77-S83 tarihsel | `RUTBE_SISTEMI_TARIHSEL.md` |
| **MASTER_CODE** | **Principal Engineer ★** | RUSTIK'in rozeti | RUSTIK = Engineering Lead + ★ |

### Detaylı Tanımlar

#### ROL (Eski: Rütbe)

**Tanım:** Organizasyondaki unvan ve sorumluluk başlığı.

**Alan:** Kurumsal (şirkette kim) vs. Teknik (kod yazarken kim)

**Örnek:**
- RUSTIK = "Engineering Lead + Principal Engineer" (kurumsal)
- KODCU = "Game Server Lead" (GAME departmanı içinde)
- REHBER = "Operations & Knowledge Lead" (dokümantasyon ve terminoloji)

**Kullanım:**
- Raporlarda: "Rol: Engineering Lead"
- CLI çıktısında: "orkestra rol RUSTIK" → "Engineering Lead | ENG | Skor: 524"
- CLAUDE.md: "Sen ... Rol: [unvan]"

---

#### PERFORMANS SKORU / SKOR (Eski: Rütbe Puanı)

**Tanım:** Görev başarısını ölçen sayısal puan (0-∞). Başarı, hız, kalite, kural uyumu, test yazma gibi faktörlere bağlı.

**Alan:** Sistem ve değerlendirme

**Aralıklar:**
- 100+ = NORMAL (yeşil) — tüm yetkiler aktif
- 75-99 = UYARI (sarı) — gözleme alınmış
- 50-74 = SON_UYARI (turuncu) — kritik uyarı
- 0-49 = SUSPENDED (kırmızı) — yetkiler kilitli

**Kullanım:**
- Raporlarda: "Skor: 524"
- Mesajlarda: "Skor güncellendi: 524 (+15)"
- DB: `agents.skor` kolonu
- CLI: `orkestra skor RUSTIK` → "Skor: 524 (Frank: 0.92)"

**NOT:** Puan silinmez, tarihsel kaydı `puan_gecmisi` tablosunda kalır.

---

#### FRANK SKORU (Ayrı Sistem — Yeni)

**Tanım:** Kod kalitesi metriği (0.0-1.0). RUSTIK'in bakımı. Performans Skorundan bağımsız.

**Alan:** Teknik kalite

**Görünürlük:**
- Sadece DOKTOR/ERENCAN raporlarında
- Junior agent'lara gizli

**Kullanım:**
- "Frank: 0.92" — kod çok temiz
- "Frank: 0.71" — refactor gerekir

---

#### DEPARTMAN (Yeni)

**Tanım:** Rol'ün bağlı olduğu fonksiyonel alan.

**Alan:** Organizasyon

**7 Departman:**
- **ENG** = Engineering (Orkestra Rust)
- **GAME** = Game Server (C++ sunucu)
- **DATA** = Data & Platform (DB, migration)
- **WEB** = Web & Product (Dashboard, panel)
- **SEC** = Security (Audit, vulnerability)
- **OPS** = Operations & Knowledge (Terminoloji, dokümantasyon)
- **EXEC** = Executive (DOKTOR, JERRY — yönetim)

**Kullanım:**
- CLI: "orkestra org" → ascii tree (departmanlarla)
- Rapor: "Dep: ENG | Rol: Engineering Lead"

---

#### ROZET (Yeni Kavram)

**Tanım:** Rolün üstüne konulan teknik/tarihsel nitelendirme. Yetki değişikliği yapmaz, kimlik ve saygı gösterir.

**Alan:** Tarihsel ve sembolik

**Rozetler:**
- **★ Principal Engineer** (RUSTIK) — eski MASTER_CODE'un devamı, ENG Lead rozeti
- 🟢 Diğer rozetler gelecekte eklenebilir

**Kullanım:**
- UI'da: "RUSTIK ★" (yıldız)
- Rapor: "Rol: Engineering Lead (Principal Engineer)"
- Yetki: Rol ile aynı (rozet + yetki = 1)

---

#### SKOR GÜNCELLEME (Eski: Rütbe Yükselişi)

**Tanım:** Performans Skorunun artması veya azalması. Mesaj formatı standarttır.

**Alan:** Sistem hareketi

**Format (ZORUNLU):**
```
"Skor güncellendi: <YENİ_SKOR> (+/-<FARK>) | Neden: <ÖZET>"
```

**Örnekler:**
- ✅ "Skor güncellendi: 524 (+15) | Neden: REH-1 tamamlandı"
- ✅ "Skor güncellendi: 490 (-34) | Neden: Kapsam dışı işlem (-20) + preamble (-5) + test yok (-9)"
- ❌ "Rütbe yükseldi" ← YASAK
- ❌ "Puan: 524" ← Eksik sebep

**Kullanım:**
- DB log: `agents_skor_gecmisi`
- Mesaj: DOKTOR → agent
- Rapor: günlük özet

---

## ESKI TERMİNOLOJİ (ARŞİVLENDİ)

### 16 Askeri Rütbe Sistemi (S77-S83)

1. ER
2. ONBASI
3. CAVUS
4. ASTSUBAY
5. ÇAVUŞBAŞI
6. TEGMEN
7. YÜZBAŞI
8. TOPÇU YÜZBAŞI
9. BİRİNCİ YÜZBAŞI
10. PIYADE YÜZBAŞI
11. PILOT YÜZBAŞI
12. TABUR KOMUTANI
13. BÖLÜKİNCİ
14. PIYADE ÖNCÜ
15. GENERAL (15. Rütbe)
16. GENERAL (MASTER_CODE — ek rütbe)

**Neden arşivlendi?**
- Şirkette "MASTER_CODE" rütbesi kafa karışıklığı yaratıyor
- Askeri yapı yazılım şirketine uymuyor
- S84'ten rol + departman yapısı daha açık ve profesyonel

**Tarihsel kayıt:** `C:\temp\MYKO\docs\arsiv\RUTBE_SISTEMI_TARIHSEL.md`

---

## TERMİNOLOJİ UYGULAMASI

### CLAUDE.md Satırları (Standart Format)

**Eski (S77-S83):**
```markdown
Rol: (rütbe'ye ait) | Rütbe Puanı: 524
```

**Yeni (S84+):**
```markdown
Rol: Engineering Lead + Principal Engineer ★ | Departman: ENG | Skor: 524 | Frank: 0.92
```

### Rapor Başlığı

**Eski:**
```
RUSTIK   ER/CAVUS/...        Rütbe Puanı: 524
```

**Yeni:**
```
RUSTIK   Engineering Lead    ENG    Skor: 524   Frank: 0.92   Aktif
```

### CLI Çıktı

**Eski:**
```bash
$ orkestra rutbe RUSTIK
Rütbe: MASTER_CODE (524)
```

**Yeni:**
```bash
$ orkestra rol RUSTIK
Rol: Engineering Lead + Principal Engineer ★
Departman: ENG
Skor: 524
Frank: 0.92
Durum: NORMAL (yeşil)
```

---

## PROTOKOL NOTLARI

1. **Türkçe uyumu:** "Rol", "Skor", "Departman" (büyük harf başı) TÜM dokümanlarda tutarlı
2. **Sembol kodu:** ★, 🟢, 🟡, 🟠, 🔴 raporlarda standart
3. **İletişim:** Agent mesajında "Skor güncellendi" formatı ZORUNLU
4. **Arşiv:** Eski rütbeler silinmez — tarihsel araştırma için korunur
5. **Geri dönüş:** "Rütbe" kelimesi SİFTLENDİ (Ctrl+H → "Skor", "Rol") — yanlış kalırsa uyarı

---

## KAYNAKLAR

- Org-Chart: `C:\temp\MYKO\docs\ORG_CHART.md`
- Plan: `C:\Users\erenc\.claude\plans\snoopy-knitting-rocket.md`
- Arşiv: `C:\temp\MYKO\docs\arsiv\RUTBE_SISTEMI_TARIHSEL.md`
- Puan politikası: `C:\temp\MYKO\docs\POLITIKALAR\ESIK_ALTI_POLITIKASI.md`

---

**Bynoisee © MalaysiaKO — Orkestra v3.0 / S84 Faz 1**  
Sözlükçü: REHBER | Onaylayan: DOKTOR | Karar: ERENCAN
