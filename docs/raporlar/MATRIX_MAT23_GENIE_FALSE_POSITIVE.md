# MAT-22/MAT-23 — Genie False Positive Analiz
# Tarih: 2026-04-27 | Session: S85
# Durum: TAMAMLANDI

---

## ÖZET

System32 / System321 (88.254.5.104) Erencan test karakterleri genie kullanırken
FLY_HACK 1074 + GENIE_DISPATCH 361 kayıt üretmiş.
Kök sebep bulundu — kod düzeltmesi gerekiyor ama bu rapor analiz-only.

---

## KÖK SEBEP

### 1. FLY_HACK False Positive — Genie Y=0xFFFF Sorunu

**Dosya:** `CharacterMovementHandler.cpp:19`
**Satır:** `if (will_y == 0xFFFF) will_y = curY1;`

Genie bot hareket paketi gönderirken Y koordinatını bilmediği durumlarda `0xFFFF` (65535) değeri gönderir.
Bu değer satır 19'da `curY1` ile değiştiriliyor — **düzeltme doğru.**

**Problem:** Düzeltme satır 103'ten ÖNCE yapılıyor (`real_y = will_y / 10.0f`).
Ancak bazı durumlarda `curY1` değeri de yüksek bir değer olabilir (terrain yüksek bölge, örn. dağ etekleri).
Genie hızlı hareket ederken `curY1` henüz güncellenmemişse eski yüksek Y değeri kalıyor.

**FLY_HACK tetiklenme senaryosu:**
1. Genie `will_y = 0xFFFF` gönderir
2. Kod `will_y = curY1` yapar
3. Ama `curY1` = bir önceki yüksek terrain pozisyonu (örn. 1800 = 180.0f)
4. `real_y = 1800 / 10.0f = 180.0f` → Zone 72 (Moradon/El Morad) maxY = 200.0f
5. Sınırı geçmez → log yok

**Ama 1074 kayıt nasıl oluştu?**
Terrain geçişlerde (düzlükten yüksek bölgeye) genie hareketi sırasında:
- `curY1` hâlâ düşük (eski pozisyon)
- `will_y` = gerçek yüksek terrain hedefi (0xFFFF değil, normal yüksek değer)
- `real_y > maxY` → FLY_HACK tetikleniyor
- **Bu false positive:** karakter gerçekten o noktaya gidecek ama sistem hack diyor.

### 2. GENIE_DISPATCH Kaydı — Kaynak Bulunamadı

`GENIE_DISPATCH` string kaynak kodda (`GenieHandler.cpp`, `CharacterMovementHandler.cpp`,
tüm `.cpp` dosyaları) **bulunamadı.** 

Muhtemel açıklamalar:
- Eski bir anti-cheat sisteminin DB'ye yazdığı kayıt (şu an kod'da yok)
- Harici bir monitoring aracının log formatı
- Sunucudaki derlenmiş exe'de var ama kaynak kodda silinmiş

Log dosyaları `./Logs/HACK_<tarih>.log` formatına yazılıyor (`LogSystem.h:filePath`).
Test tarihi Mart 2026 — `HACK_2026-03-XX.log` dosyaları sunucuda aranmalı (SSH erişimi gerekli).

---

## ETKİLENEN ZONELER

| Zone | maxY Limiti | Risk |
|------|-------------|------|
| Delos, Ronark Land, Ronark Base | 350.0f | Düşük |
| Ardream, Bifrost, CSW, Juraid, Draki | 300.0f | Orta |
| Karus/ElMorad (tüm versiyonlar) | 250.0f | **YÜKSEK** — Ana bölgeler |
| Diğer tüm zoneler | 200.0f | **YÜKSEK** |

Ana bölgeler (Karus/ElMorad) 250.0f ile sınırlı. Dağ/tepe bölgelerinde terrain 200-240 seviyesine çıkabiliyor.
Genie hızlı hareket ederken bir sonraki terrain noktasının Y değeri = false positive.

---

## AÇILIŞ RİSKİ DEĞERLENDİRMESİ

**Her oyuncuda olur mu?**
Evet — genie kullanan HER oyuncu benzer durumla karşılaşır.
Log'a düşmesi yeterli, genie çalışmaya devam eder (`real_y = GetY()` ile geri döndürülüyor).
Ama log'da yığılma olursa sunucu disk dolabilir.

**Forum problemi yaratır mı?**
Doğrudan oyuncu deneyimini bozmaz (genie durmaz, karakter yerinde kalır).
Ama 1000+ hack logu admin panelde görünürse yanlış alarm.

---

## ÇÖZÜM ÖNERİSİ (KOD DEĞİŞİKLİĞİ — RUSTIK/CHIP görevi)

`CharacterMovementHandler.cpp:136-139` — FLY_HACK bloğuna Genie istisnası ekle:

```cpp
if (real_y > maxY)
{
    // Genie hareketi ise false positive ihtimali yüksek — sadece log, disconnect etme
    if (m_bGenieStatus) {
        LOG_HACK("[FLY_HACK_GENIE_SKIP] User=%s Zone=%u Y=%.1f MaxY=%.1f", ...);
        real_y = GetY();
    } else {
        LOG_HACK("[FLY_HACK] User=%s Zone=%u Y=%.1f MaxY=%.1f IP=%s", ...);
        real_y = GetY();
    }
}
```

Alternatif: Genie aktifken maxY limitini %20 artır (terrain geçiş toleransı).

**GENIE_DISPATCH için:** SSH ile sunucuda `HACK_2026-03-XX.log` incelenmeli.

---

## SONUÇ

| Bulgu | Durum |
|-------|-------|
| FLY_HACK kök sebep | Terrain geçişte genie Y false positive — KOD'DA VAR |
| GENIE_DISPATCH kök sebep | Kaynak kodda bulunamadı — eski sistem veya derlenmiş exe'de |
| Açılış engelleyici mi? | HAYIR — genie çalışmaya devam eder |
| Oyuncu deneyimi etkisi | Minimal — karakter yerinde kalır, genie devam eder |
| Disk riski | VAR — yoğun kullanımda HACK log şişer |
| Fix gereken yer | CharacterMovementHandler.cpp:136-139 |

**MATRIX © MalaysiaKO — S85 | 2026-04-27**
