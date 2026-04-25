# MATRIX_MAT23_GENIE_FALSE_POSITIVE
# Tarih: 2026-04-25 | Agent: MATRIX | Session: S83 | Görev: MAT-23
# Durum: TAMAMLANDI

---

## BÖLÜM 1 — FLY_HACK KÖK SEBEP

### Tespit

**Kod konumu:** `GameServer_SRC/GameServer/CharacterMovementHandler.cpp:117-141`

FLY_HACK kontrolü `MoveProcess()` içinde:

```cpp
// Satır 117-141
if (!isGM() && real_y > 0.0f)
{
    float maxY = 200.0f;
    uint8 zoneID = GetZoneID();
    // Zone bazlı maxY:
    // KARUS/ELMORAD/ESLANT grupları → maxY = 250.0f
    ...
    if (real_y > maxY)
    {
        LOG_HACK("[FLY_HACK] User=%s Zone=%u Y=%.1f MaxY=%.1f IP=%s", ...);
        real_y = GetY(); // eski Y'ye geri dönder
    }
}
```

### 0xFFFF Problemi

**Kod konumu:** `CharacterMovementHandler.cpp:19`

```cpp
pkt >> will_x >> will_z >> will_y >> speed >> echo >> curX1 >> curZ1 >> curY1;
if (will_y == 0xFFFF) will_y = curY1; // Genie: KO exe Y bilinmiyorsa 0xFFFF gönderir
```

**Matematiksel kanıt:**
- Genie aktifken KO client Y koordinatını bilmediğinde `will_y = 0xFFFF (65535)` gönderir
- `real_y = will_y / 10.0f = 65535 / 10 = 6553.5`
- ZONE_KARUS (Zone=1) ve ZONE_KARUS_ESLANT (Zone=11) için `maxY = 250.0f`
- `6553.5 > 250.0` → FLY_HACK tetikleniyor

**Logdaki kanıt:**
```
[FLY_HACK] User=System32 Zone=1  Y=6525.1 MaxY=250.0
[FLY_HACK] User=System63 Zone=11 Y=6553.5 MaxY=250.0
```

- System32 Zone=1: `Y=6525.1` — `0xFFFF` yerine paket gürültüsü veya farklı Genie hareketi (tam 0xFFFF değil ama çok yüksek)
- System63 Zone=11: `Y=6553.5` — tam `65535/10 = 6553.5` → **kesin 0xFFFF senaryosu**

### Neden 0xFFFF düzeltmesi işe yaramıyor?

Satır 19'daki `if (will_y == 0xFFFF) will_y = curY1;` kontrolü doğru mantıkta ama **FLY_HACK öncesinde çalışıyor** gibi görünüyor. Ancak System63'ün `Y=6553.5` loglanması bu düzeltmenin o an çalışmadığını gösteriyor.

Olası sebepler:
1. `curY1` da 0 veya geçersiz → `will_y = 0` → `real_y = 0` → FLY_HACK tetiklenmez ama pozisyon bozulur; **ya da**
2. Farklı Genie hareket paketi yolu `MoveProcess`'i bypass ediyor, `0xFFFF` kontrolü geçilmiyor; **ya da**
3. `GenieAttackProgress → GenieMove → MoveProcess(pkt)` zincirinde pkt okuma sırası farklı — `curY1` okuma sırası kayıyor

**En güçlü hipotez:** `GenieAttackProgress` çağrısında pkt format'ı normal `MoveProcess` pkt format'ından farklı. Genie paketi ek bir `uint8 command` okuyor (`pkt.read<uint8>()`) → pkt okuma offset kayıyor → `will_y` alanına istenmeyen değer (0xFFFF veya başka büyük sayı) geliyor.

---

## BÖLÜM 2 — GENIE_DISPATCH KÖK SEBEP

### Tespit

**Kod konumu:** `GameServer_SRC/GameServer/GenieHandler.cpp:96`

```cpp
void CUser::GenieAttackProgress(Packet & pkt)
{
    uint8 command = pkt.read<uint8>();
    if (UNIXTIME > m_1098GenieTime) return SendGenieStop(true);

    LOG_HACK("[GENIE_DISPATCH] User=%s cmd=%d", GetName().c_str(), command);
    ...
}
```

**Köl sebep:** `LOG_HACK("[GENIE_DISPATCH]")` satırı **her** Genie saldırı komutunda tetikleniyor. Bu CHIP'in eklediği tanısal log. `LOG_HACK` makrosu HACK loguna yazıyor — bilerek eklendi, hack tespiti için değil davranış izleme için.

**cmd değerleri:**
- `cmd=1` → GenieMove
- `cmd=2` → GenieRotate
- `cmd=4` → GenieMainAttack

Genie aktif farm: her hareket + saldır + dön döngüsü → saniyede 3-5 log satırı → 2 dakikada 361 satır.

**Bu bir hata değil:** CHIP'in LOG_HACK kullanımı kasıtlı teşhis logu. Sorun bu logların HACK dosyasına gitmesi ve alarm yaratması.

---

## BÖLÜM 3 — TETİKLEYİCİ SENARYOLAR

| Senaryo | FLY_HACK tetikler mi | GENIE_DISPATCH tetikler mi |
|---------|---------------------|---------------------------|
| Genie aktif + hareket komutu | ✅ Evet (Y=0xFFFF paketi) | ✅ Evet (her cmd) |
| Genie aktif + saldırı | ✅ Mümkün | ✅ Evet |
| Normal hareket (Genie kapalı) | Hayır | Hayır |
| GM karakteri + Genie | Hayır (isGM() bypass) | ✅ Evet |
| Zone geçişi sonrası Genie | ✅ Yüksek risk (curY1 sıfırlanıyor) | ✅ Evet |

**En kritik senaryo:** Genie açıkken herhangi bir hareket paketi → `0xFFFF` Y değeri → `real_y = 6553.5` → FLY_HACK → `real_y = GetY()` ile düzeltiliyor (hareket engellenmiyor, sadece log). Oyuncu hareketi devam ediyor, sadece log basılıyor.

---

## BÖLÜM 4 — TÜM OYUNCULARI ETKİLER Mİ?

**Evet, Genie kullanan her oyuncu bu logları üretir.**

Koşullar:
- Genie aktif (premium gerektiriyor: `LootandGeniePremium && GetPremium()`)
- Herhangi bir hareket veya saldırı komutu

**Etki analizi:**
- FLY_HACK logu: Hareket engellenmiyor (`real_y = GetY()` ile düzeltme yapılıyor), oyuncu kick/ban almıyor. Sadece log.
- GENIE_DISPATCH logu: Hiçbir aksiyon yok, sadece log.

**Açılış riski:** Çok sayıda premium oyuncu Genie kullanırsa:
- HACK log dosyası çok hızlı büyür (saniyede 5-10 satır/oyuncu)
- Log analiz/alarm sistemleri gürültüye boğulur
- Gerçek hackçı tespiti zorlaşır
- Disk I/O yükü (düşük risk ama 100+ oyuncuda ciddi)

**Hareket veya ban açısından:** Açılış engeli yok. Oyuncu deneyimi etkilenmiyor.

---

## BÖLÜM 5 — ÖNERİLEN FIX

### FIX A — FLY_HACK: Genie aktifken bypass (KODCU/CHIP)

**Dosya:** `CharacterMovementHandler.cpp:117`

```cpp
// MEVCUT (sorunlu):
if (!isGM() && real_y > 0.0f)
{
    ...
    if (real_y > maxY)
        LOG_HACK("[FLY_HACK] ...");
}

// ÖNERİLEN FIX:
if (!isGM() && !m_bGenieStatus && real_y > 0.0f)
{
    ...
    if (real_y > maxY)
        LOG_HACK("[FLY_HACK] ...");
}
```

`m_bGenieStatus` bool değişkeni `GenieStart()`'ta `true`, `GenieStop()`'ta `false` yapılıyor. Genie aktifken Y kontrolü skip edilir.

**Alternatif (daha güvenli):** `real_y > maxY` kontrolüne ek olarak `real_y != 6553.5f` (0xFFFF/10) özel case kontrolü:

```cpp
float genie_sentinel = 0xFFFF / 10.0f; // 6553.5
if (real_y > maxY && fabsf(real_y - genie_sentinel) > 1.0f)
    LOG_HACK("[FLY_HACK] ...");
```

Bu yaklaşım Genie dışı gerçek fly hack'i kaçırmaz.

### FIX B — 0xFFFF düzeltme doğrulaması (KODCU)

**Dosya:** `CharacterMovementHandler.cpp:19`

```cpp
// MEVCUT:
if (will_y == 0xFFFF) will_y = curY1;

// Eğer curY1 da 0 ise, GetY()'den al:
if (will_y == 0xFFFF)
    will_y = (curY1 > 0) ? curY1 : (uint16)(GetY() * 10.0f);
```

### FIX C — GENIE_DISPATCH log seviyesi (CHIP)

**Dosya:** `GenieHandler.cpp:96`

```cpp
// MEVCUT (HACK loguna yazıyor):
LOG_HACK("[GENIE_DISPATCH] User=%s cmd=%d", ...);

// ÖNERİLEN (DEBUG loguna taşı veya kaldır):
// LOG_DEBUG("[GENIE_DISPATCH] User=%s cmd=%d", ...);
// veya tamamen kaldır — teşhis tamamlandıysa gerek yok
```

`LOG_HACK` yerine `LOG_DEBUG` veya `TRACE` kullanılırsa HACK log dosyasına yazılmaz.

---

## BÖLÜM 6 — RİSK SEVİYESİ

| Konu | Risk | Açılış Engelleyici mi |
|------|------|----------------------|
| FLY_HACK false positive logu | 🟡 ORTA | ❌ Hayır |
| GENIE_DISPATCH log gürültüsü | 🟢 DÜŞÜK | ❌ Hayır |
| Gerçek FLY_HACK tespitinin gürültüye boğulması | 🟡 ORTA | ❌ Hayır |
| Disk I/O (100+ Genie oyuncu) | 🟢 DÜŞÜK | ❌ Hayır |
| Oyuncu kick/ban (false positive) | ❌ Yok | ❌ Hayır |

**Genel değerlendirme:** **Açılış engelleyici değil.** Oyuncu deneyimini etkilemiyor. Ancak açılış sonrası log dosyaları hızlı büyüyecek ve gerçek hack tespiti zorlaşacak.

**Önerilen sıra:**
1. FIX C (CHIP) — en hızlı: 1 satır değişiklik, GENIE_DISPATCH gürültüsü biter
2. FIX A (KODCU) — Genie FLY_HACK bypass, 1-2 satır değişiklik
3. FIX B (KODCU) — 0xFFFF güvenli fallback, isteğe bağlı

---

## ÖZET

| # | Bulgu | Kök Sebep | Kim Düzeltir |
|---|-------|-----------|--------------|
| 1 | FLY_HACK 1074/7 kayıt | Genie `will_y=0xFFFF` → `6553.5f` FLY_HACK eşiğini (250.0f) aşıyor. `MoveProcess` Genie state'i kontrol etmiyor. | KODCU |
| 2 | GENIE_DISPATCH 361 kayıt | CHIP'in `LOG_HACK()` teşhis logu her Genie saldırı komutunda tetikleniyor. Kasıtlı ama yanlış log seviyesi. | CHIP |

**Açılış riski: DÜŞÜK.** Oyuncu hareketi/ban etkilenmiyor. Sadece log kirliliği sorunu.

---

**Bynoisee © MalaysiaKO 2026 — MATRIX MAT-23 Genie False Positive Raporu**
