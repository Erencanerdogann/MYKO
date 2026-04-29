# 📅 EVENT TAKVİMİ — MYKO 1098 Lansman Hazırlık

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `Define.h:80-95`, `EventScheduleStatusSet.h`, `Scheduler` (DB), `EventAwards.ini`, `EventSettings.ini`, `GAME_LOGIC.md`
**Hedef:** Lansman gününde event saatlerinin tek tablo görünümü.

---

## 1. EVENT YÖNETİMİ NASIL ÇALIŞIR

### Üç Katman
```
┌─────────────────────────────────────┐
│ 1) DB: EVENT_SCHEDULE tablo         │  ← Otomatik tetiklenenler
│    EventName, StartDays, StartHour  │     (cron benzeri)
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│ 2) Define.h: SÜRE / SAYIM           │  ← Event uzunluğu
│    JURAD_TIME=3000s, BDW=1800s      │
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│ 3) GM Komutu: Manuel start/stop     │  ← `+csw`, `+chaosopen`, vb.
└─────────────────────────────────────┘
```

### Sürelerden Hatırlatma (`Define.h:89-95`)
| Event | Süre (saniye) | Süre (dakika) |
|-------|---------------|---------------|
| Juraid Mountain | 3000 | 50 |
| Border Defense War | 1800 | 30 |
| Chaos Expansion | 1200 | 20 |
| Monster Stone (event) | 1800 | 30 |
| Draki Tower (kill timer) | 315 | 5.25 |
| Temple Termination (finish) | 20 | — |

### EVENT_SCHEDULE DB Kolonları (`EventScheduleStatusSet.h:24-34`)
| Kolon | Tip | Açıklama |
|-------|-----|----------|
| EventLocalID | byte | Yerel ID |
| EventType | byte | Event tipi |
| EventZoneID | uint16 | Hangi zone |
| EventName | string | Görünen ad |
| StartDays | string | Hangi günler (1=Pzt..7=Pzr, virgüllü) |
| EventStatus | byte | 0=kapalı, 1=aktif |
| EventStartHour[] | uint32 (array) | Başlangıç saatleri (UTC) |

`+reloadevent` ile DB'den hot reload.

---

## 2. STANDART HAFTALIK TAKVİM (1098 referans)

⚠️ **Aşağıdaki saatler GENEL KO 1098 standart referansıdır.** MalaysiaKO'nun kendi DB'sindeki `EVENT_SCHEDULE.StartDays` + `EventStartHour[]` ile karşılaştırılmalı. Patron/MATRIX `SELECT * FROM EVENT_SCHEDULE` ile doğrulayacak.

### Pazartesi
| Saat (UTC+3 TR) | Event | Komut |
|------|-------|-------|
| 13:00 | Lunar War | otomatik |
| 19:00 | Lunar War | otomatik |
| 21:00 | CSW (Castle Siege War) | `+csw` |
| 23:00 | Forgotten Temple | `+ftopen` |

### Salı
| Saat | Event | Komut |
|------|-------|-------|
| 18:00 | Border Defense War | `+borderopen` |
| 20:00 | Chaos Expansion | `+chaosopen` |
| 22:00 | Juraid Mountain | `+juraidopen` |

### Çarşamba
| Saat | Event | Komut |
|------|-------|-------|
| 19:00 | Snow War (kış) | `+snow` |
| 21:00 | UTC (Under the Castle) | `+utc` |
| 22:00 | Beef Event | `+beefopen` |

### Perşembe
| Saat | Event | Komut |
|------|-------|-------|
| 18:00 | Border Defense War | `+borderopen` |
| 20:00 | Chaos Expansion | `+chaosopen` |
| 22:00 | Juraid Mountain | `+juraidopen` |

### Cuma
| Saat | Event | Komut |
|------|-------|-------|
| 19:00 | Lottery | `+lottery` |
| 21:00 | Tournament (Klan) | `+tournamentstart` |
| 22:00 | MadClas | `+madclas <Tip>` |

### Cumartesi (en yoğun)
| Saat | Event | Komut |
|------|-------|-------|
| 13:00 | Lunar War | otomatik |
| 19:00 | Lunar War | otomatik |
| 20:00 | BDW + Chaos | `+borderopen`, `+chaosopen` |
| 21:00 | CSW | `+csw` |
| 22:00 | Juraid + Forgotten Temple | `+juraidopen`, `+ftopen` |
| 23:00 | Bowl Event | `+bowlevent <zone> <süre> <saniye>` |

### Pazar
| Saat | Event | Komut |
|------|-------|-------|
| 18:00 | Collection Race | `+cropen 1` |
| 20:00 | Border Defense War | `+borderopen` |
| 22:00 | Chaos Expansion | `+chaosopen` |

⚠️ **MalaysiaKO Spesifik:** Patron'un belirleyeceği saatler farklı olabilir. Yukarıdaki "iskelet" — DB doğrulaması şart.

---

## 3. EVENT TIPLERI (1098 — Aktif Olanlar)

### A) PvP Savaş Eventleri
| Event | Açıklama | Min Lvl | Süre | Zone |
|-------|----------|---------|------|------|
| **CSW** (Castle Siege War) | Delos kalesi savaşı (klan grade 5+) | 60+ | 90 dk | Delos |
| **BDW** (Border Defense War) | Sınır savunma (lvl tier ödül) | 20+ | 30 dk | War zone |
| **Lunar War** | Ulus savaşı | 60+ | 30 dk | RL/CZ |
| **Juraid Mountain** | 8v8 PvPvE (Devabird boss) | 60+ | 50 dk | Juraid |
| **Forgotten Temple** | Juraid bodrum (32 oyuncu) | 60+ | 30 dk | FT |
| **Snow War** (kış) | Snowball PvP | 30+ | 30 dk | SnowZone |

### B) Tournament/Yarışma
| Event | Açıklama |
|-------|----------|
| **Tournament** | Klan turnuvası (`+tournamentstart`) |
| **Chaos Expansion** | Chaos Stone ranking |
| **MadClas** (Cindirella) | Level segmenti yarışma (`+madclas 1/2/3`) |
| **UTC** (Under the Castle) | Zindan PvP |
| **Lottery** | Piyango |
| **Collection Race** | Toplama yarışı (`+cropen <ID>`) |

### C) Boss / Spawn
| Event | Açıklama |
|-------|----------|
| **Felankor** | World Boss (1098 ana atraksiyon) |
| **Custom Event Boss** | Özel boss spawn (`+event`) |
| **Beef Event** | Özel ödül event |
| **Bowl Event** | Top toplama |

### D) Eğlence/Sezon
| Event | Açıklama |
|-------|----------|
| **Santa** (kış) | Uçan Noel Baba (`+santa`) |
| **Angel** | Uçan melek (`+angel`) |
| **Snow** (kış sezonu) | Kar event |

⚠️ **1098'de YOK** (referans):
- Bifrost expansion ❌
- Ardream PK ❌ (yerine CZ/Ronark)
- Dragon Cave ❌
- Lunar Valley ❌

---

## 4. EVENTAWARDS.INI — ÖDÜL TABLOSU

`C:\Users\erenc\Desktop\Server\EventAwards.ini` (gerçek dosya):

### BORDER_DEFENSE_WAR Örneği
```ini
[BORDER_DEFENSE_WAR]
WINNING_LEVEL_20_TO_29_1 = 900142000  ← Item ID
WINNING_LEVEL_20_TO_29_2 = 900142000
WINNING_LEVEL_20_TO_29_3 = 379154000
WINNING_LEVEL_20_TO_29_4 = 900017000
LOSER_LEVEL_20_TO_29_1   = 900017000
LOSER_LEVEL_20_TO_29_2   = 900142000
LOSER_LOYALTY            = 100
...
```

**Yapı:** Level segmenti × kazanan/kaybeden × ödül slotu (1-4) → Item ID.

**Level segmentleri (BDW):**
- 20-29, 30-39, 40-49, 50-57, 58-64, 65-72, 73-80, 81-MAX

⚠️ **Item ID kontrolü ŞART** — DB ITEM tablosu ile cross-check (var mı, geçerli mi).

---

## 5. EVENTSETTINGS.INI — ON/OFF SWITCH

`C:\Users\erenc\Desktop\Server\EventSettings.ini`:

```ini
[CLAN_BUFF_SYSTEM]
5_ONLINE_EXP=1   ← Klan 5+ online → exp bonus
5_ONLINE_NP=1
10_ONLINE_EXP=1
10_ONLINE_NP=1
...
30_ONLINE_EXP=1
30_ONLINE_NP=1
STATUS=1         ← Sistem aktif mi?
```

**Diğer beklenen bölümler** (memory/dosyaya göre kontrol et):
- `[BONUS]` (DROP/EXP/MONEY/NP global oranlar)
- `[CASTLE]` (kale ulus)
- `[CLAN_GRADE]` (grade NP eşik)
- `[CLAN_PREMIUM]` (premium bonusları)
- `[NATIONAL_POINTS]` (NP source)
- `[MERCHANT]` / `[MILITARY_CAMP]` (ulus zone bayrak)

---

## 6. SCHEDULER.INI (BEKLENEN, DOĞRULA)

⚠️ Lokal `Server\` klasöründe `Scheduler.ini` BULUNAMADI. Production'da `EVENT_SCHEDULE` DB tablosu kullanılıyor olabilir, INI ikincil.

**Doğrulama:**
```sql
SELECT * FROM EVENT_SCHEDULE ORDER BY EventStartHour, StartDays;
```

---

## 7. LANSMAN GÜNÜ (08 MAYIS 2026 — CUMA) ÖNERİSİ

| Saat (TR) | Event | Komut | Not |
|-----------|-------|-------|-----|
| 18:00 | **AÇILIŞ** | `+permanent "Bynoisee MalaysiaKO Valor"`, `+noticeall "HOSGELDINIZ"` | Sunucu açık |
| 18:30 | İlk duyuru | `+noticeall "30dk sonra ilk event"` | |
| 19:00 | **CSW** (lansman özel) | `+csw` | İlk büyük event |
| 20:00 | **Chaos** | `+chaosopen` | Aktif tutmak için |
| 21:00 | **BDW** | `+borderopen` | Level segmenti ödülü |
| 22:00 | **Juraid + FT** | `+juraidopen`, `+ftopen` | Akşam pik |
| 23:00 | **MadClas** + **Lottery** | `+madclas 3`, `+lottery` | Gece eğlence |
| 00:00 | **Tournament** (klan) | `+tournamentstart` | Hafta sonu başlangıç |

⚠️ **Patron onayı şart** — bu sadece taslak.

---

## 8. KONTROL LİSTESİ (Lansman Öncesi)

- [ ] DB `EVENT_SCHEDULE` dolu mu? (`SELECT COUNT(*)`)
- [ ] EventStatus=1 olanlar lansman saatleriyle uyumlu mu?
- [ ] `EventAwards.ini` ödül itemleri DB'de var mı?
- [ ] `EventSettings.ini` STATUS=1 (aktif sistemler)
- [ ] `+reloadevent` çalışıyor mu?
- [ ] CSW için Delos kale grade hazır mı?
- [ ] Captain (kaptan) atama mekanizması çalışıyor mu? (`+captain`)
- [ ] Discount sistem (kazanan ulus indirim) test edildi mi?
- [ ] Event log akıyor mu? (`Logs\` klasörü)
- [ ] Saat dilimi doğru mu? (Sunucu UTC+3 TR)

---

## 9. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **Saat dilimi:** Sunucu UTC+3 (TR). Oyuncu farklı bölgede ise UI'da düzeltme yok. |
| 2 | **2 event aynı anda:** Conflict olabilir, test etmeden kullanma. |
| 3 | **CSW 90 dk:** Define.h'da yok ama tarihen biliniyor — kod kontrol et. |
| 4 | **Bowl Event:** `+bowlevent` parametre 3 (zone, süre, saniye_aralik) — kullanım dikkat. |
| 5 | **Lottery:** Tek seferlik, `+lotteryclose` ile kapat. |
| 6 | **Tournament:** Hafta boyunca açık kalabilir, kontrol et. |
| 7 | **`+reloadevent`** çağrısı — runtime DB değişiklik gerekirse. |

---

## 10. KAYNAK REFERANSLAR

- **Süre:** `GameServer_SRC\GameServer\Define.h:80-100`
- **DB schema:** `shared\database\EventScheduleStatusSet.h`
- **GM komut:** `GM_KOMUT.md § 4`
- **Gameplay detay:** `GAME_LOGIC.md § 3-9`
- **INI dosyaları:** `C:\Users\erenc\Desktop\Server\Event*.ini`
- **DB tablo:** `EVENT_SCHEDULE`, `BAN_LIST`, `KING_SYSTEM`, `KNIGHTS`

---

**Sürüm:** v1.0 — S88 ilk yazım (iskelet)
**Sonraki:** Production `EVENT_SCHEDULE` SELECT sonucu ile doğrulanacak (MATRIX işi).
