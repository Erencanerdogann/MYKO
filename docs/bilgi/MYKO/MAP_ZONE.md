# MAP / ZONE — 82 Harita Dosyası, 11 Ana Zone (1098)

**Tarih:** 2026-04-29 | **Kategori:** GAME MAP | **Sayı:** 61 .smd + 14 .aievt | **1098 İçeriği:** 11 aktif zone

---

## 1. HARITA DOSYA YAPISI

### Konum
```
C:\Users\erenc\Desktop\Server\Map\
├── [61 .smd dosyası]   — Zone harita binary
├── [14 .aievt dosyası] — NPC AI spawn event
└── WarpGateEditor.exe  — Warp nokta edit tool
```

### .smd Format

- **Binary harita dosyası** — terrain, collision, spawn point
- **N3Mesh** → statik geometry (ağaç, kaya, bina)
- **Size:** 1 MB ~ 3.8 MB (zone büyüklüğüne göre)
- **Şifreleme:** Yok (binary proprietary)

### .aievt Format

- **NPC AI event file** — spawn noktası, behavior, patrol
- **Binary** — 14 dosya
- **Yönetim:** AIServer yok → NpcThread.cpp + Lua event

---

## 2. 1098 PATCH — 11 AKTIF ZONE (BİZİMKİLER)

### Ana Haritalar (1098 Prefix — Kritik)

| .smd Dosya | Zone Adı | Amaç | Boyut | Oyuncu Tipik |
|------------|----------|------|-------|--------------|
| **1098elmo2004.smd** | El Morad | Elmorad başlangıç bölgesi | 3.8 MB | Lvl 1-10 |
| **1098karus2004.smd** | Karus | Karus başlangıç bölgesi | 3.3 MB | Lvl 1-10 |
| **1098moradon_0826.smd** | Moradon | Neutral hub, alış-satış | 2.9 MB | Tüm seviye |
| **1098war_a.smd** | War Zone | Ana PvP savaş alanı | 1.0 MB | Lvl 50+ |
| **1098freezone_a.smd** | Free Zone A | Kaynak toplama (NO PvP) | 1.1 MB | Lvl 20-40 |
| **1098freezone_b.smd** | Free Zone B | Kaynak toplama | 1.1 MB | Lvl 20-40 |
| **1098freezone_c.smd** | Free Zone C | Kaynak toplama | 1.1 MB | Lvl 20-40 |
| **1098In_dungeon01.smd** | Dungeon (İç 1) | PvE boss dungeon | 2.2 MB | Lvl 40+ |
| **1098In_dungeon02.smd** | Dungeon (İç 2) | PvE boss dungeon | 2.2 MB | Lvl 40+ |
| **1098In_dungeon03.smd** | Dungeon (İç 3) | PvE boss dungeon | 2.2 MB | Lvl 40+ |
| **1098Eslant_*.smd** | Eslant | Lvl 60+ PvE zone | Var. | Lvl 60+ |

**Toplam 1098:** 11 aktif zone (+2 varyant Eslant)

---

## 3. İKİNCİ KATMAN HARITALAR (Eski, Referans)

**1098'de İNAKTİF (sunucuda açılmadı):**

| .smd Dosya | Zone | Durum |
|------------|------|-------|
| **BattleZone.smd** | Battle Arena | Kapalı |
| **Code_Moradon_war.smd** | Moradon War Event | Kapalı |
| **arena.smd** | Tournament Arena | Kapalı |
| **bossmode.smd** | Boss Mode | Kapalı |
| **14th_oldmoradon.smd** | Eski Moradon (v14) | Kapalı |
| **2017_flagwar.smd** | Flag War | Kapalı |
| **LK_war_a01.smd** | LK War | Kapalı |

**Neden kapalı?** 2369 base eski sürüm, 1098 patch sadece güncel 11 zone + dungeon series

---

## 4. DUNGEON SERİSİ HARITASI

### Dungeon Hiyerarşi

| Dosya | Zone | Seviye | Type |
|-------|------|--------|------|
| **1098In_dungeon01.smd** | Dungeon I | Lvl 40-50 | Linear |
| **1098In_dungeon02.smd** | Dungeon II | Lvl 50-60 | Loop |
| **1098In_dungeon03.smd** | Dungeon III | Lvl 60-70 | Boss lair |
| **dungeon_a.smd** | Dungeon A (eski) | — | Referans |
| **dungeon_b1th.smd** | Dungeon B1 | — | Referans |
| **dungeon_b2th.smd** | Dungeon B2 | — | Referans |
| **dungeon_b3th2015.smd** | Dungeon B3 (2015) | — | Referans |
| **dungeon_d.smd** | Dungeon D | — | Referans |

**Not:** 1098In_dungeon01-03 **aktif**, dungeon_a/b/d **kapalı** (eski Bifrost expansion)

---

## 5. ZONE DETAY — 1098 ÖNEMLI

### Moradon (1098moradon_0826.smd)

**Hub zone — Tüm oyuncular merkez.**

- **NPC:** Alış-satış (Charon), Anvil (smith), Teleport (warp master)
- **Safety:** PvP yok, gank güvenli
- **Spawn:** Lvl 1 yeni karakter başlangıç
- **Bug:** M-key (mini-map) yanlış konum gösteriyor (memory: moradon_map_bug)

### War Zone (1098war_a.smd)

**Ana PvP savaş alanı.**

- **Karus vs Elmorad** — açık savaş
- **Level:** 50+ önerilir
- **Loot:** Drop rate yüksek
- **Event:** CSW (Castle Siege War) de savaş
- **Safezone:** YOK (gank riski yüksek)

### Free Zone (1098freezone_a/b/c.smd)

**PvP olmayan kaynak bölgesi.**

- **Mobbing:** Güvenli
- **Drop:** Normal
- **Quest mobları:** En yaygın
- **Kullanım:** Lvl 20-40 grinding

### El Morad & Karus

**Başlangıç bölgeleri (Lvl 1-10).**

- **El Morad:** İnsan başlangıç
- **Karus:** Orc başlangıç
- **NPC:** Tutorial quest (Sphie, Charel vb.)
- **Moblar:** Zayıf (starter-tier)

### Dungeon 01-03 Serisi

**PvE dungeonlar, boss loot.**

- **Dungeon01:** Zayıf boss (Lvl 40)
- **Dungeon02:** Orta boss (Lvl 50)
- **Dungeon03:** Güçlü boss (Lvl 60)
- **Loot:** High-tier item drop

---

## 6. WARP GATE (TELEPORT)

### WarpGateEditor.exe

Tool: `C:\Users\erenc\Desktop\Server\Map\WarpGateEditor.exe`

**İş:** Zone geçişi warp point tanımla/edit

```
Moradon → War Zone
Moradon → Dungeon 01
Dungeon 01 → Dungeon 02
El Morad → Moradon (tutorial complete sonrası)
```

**DB:** `WARP_LIST` tablo (koord, destination zone ID)

---

## 7. NPC SPAWN (.aievt)

### .aievt Dosyaları (14 toplam)

| Dosya | Amaç |
|-------|------|
| **moradon.aievt** | Moradon NPC spawn |
| **war_zone.aievt** | War zone NPC |
| **freezone.aievt** | Free zone mobs |
| **dungeon01.aievt** | Dungeon I boss + mobs |
| **dungeon02.aievt** | Dungeon II |
| **dungeon03.aievt** | Dungeon III |
| **elmorad.aievt** | El Morad NPC |
| **karus.aievt** | Karus NPC |
| **eslant.aievt** | Eslant PvE |
| **[5+ event aievt]** | Event spawn (CSW, war) |

### Spawn Yapısı (aievt)

Binary format:
```
[NPC_ID] [X] [Y] [Z]           — Koord
[COUNT] [RESPAWN_TIME] [BEHAVIOR] — Sayı, respawn, AI type
```

**NPC.tbl + NPC_Pos.tbl** → aievt verisi tekrar sorgulanmaz (cache)

---

## 8. ZONE INFO TABLO

### ZONE_INFO.tbl (Client)

| Sütun | Örnek |
|-------|-------|
| ZONE_ID | 1 (Moradon) |
| NAME | Moradon |
| MAX_USER | 500 |
| MAX_MONSTER | 200 |
| WEATHER | 1 (sunny) |

### DB: ZONE_INFO

```sql
SELECT * FROM ZONE_INFO WHERE ZONE_ID = 1;
ZONE_ID | NAME | DESC | TYPE
--------+------+------+-------
1       | Moradon | Neutral hub | 0 (hub)
2       | War Zone | PvP zone | 1 (pvp)
3       | Free Zone | Safe zone | 2 (safe)
```

---

## 9. 1098 ÖZELLİKLERİ — YOK OLANLAR

1098'de **AÇILMAYAN** zone'lar:

| Zone | Neden | Alternative |
|------|-------|-------------|
| **Bifrost** | Expansion kapalı | War Zone ana |
| **Ardream** | Expansion kapalı | War Zone |
| **Dragon Cave** | Expansion kapalı | Dungeon serisi |
| **Celestial Tower** | Yok | — |
| **Lunar Valley** | Yok (event-only) | Lunar War event |

**1098 focus:** 11 core zone + dungeon = yeterli content

---

## 10. HARITA HATA VE BUG

### Moradon M-key Bug (AKTIF)

- **Sorun:** Minimap (M-key) Moradon'da yanlış position gösteriyor
- **Sebebi:** map.smd coords ≠ UI render coords
- **Impact:** Oyuncu şaşırır, navigation zor
- **Çözüm:** (memory: project_moradon_map_bug.md — detay bekleniyor)

### Collision Issues

- **Ağaç/kaya geçme** — NPC pathfinding hata
- **Çözüm:** NpcThread.cpp → Region.cpp pathfinding check

### Zone Lag Spots

- War Zone → bot farm yoğun → lag
- Dungeon03 → multiple client particle → lag
- **Çözüm:** MAX_PLAYER limit (ZONE_INFO) arttır

---

## 11. HARITA YÖNETİMİ

### Edit Akışı

1. **WarpGateEditor.exe** aç
2. Zone .smd load
3. Warp point drag/drop
4. `.aievt` export (binary)
5. Sunucuya upload

### Compile

- .smd binary (read-only) — edit tool şart
- .aievt binary — regex/hex edit (not recommended)

### Backup

- `F:\MDBACKUP\Map\` — tüm zone backup
- Version: 1098_backup_20260429

---

## 12. PERFORMANCE NOTES

| Zone | Max Player | FPS Target | Note |
|------|------------|-----------|------|
| Moradon | 500 | 60 | Hub — network bottleneck |
| War Zone | 200 | 60 | PvP — packet storm |
| Free Zone | 300 | 60 | Safe — stable |
| Dungeon | 50 | 60 | Enclosed — FX heavy |

**Region system:** Map divide by 64x64 chunk → only render near region

---

## 13. 1098 MAP SNAPSHOT

```
AKTIF ZONE HARITASI (1098 Patch)

         [Karus]
            |
    [War Zone] ← CSW Event
     /  |  \
  El M / FZ \ Eslant
    \     /
    Moradon ← Hub
     / | \
   FZ FZ FZ
     \ | /
    [Dungeon01-03] ← Boss
```

---

## 14. İLİŞKİLİ DOSYALAR

- **Scheduler.ini** — War event timing
- **NPC.tbl** — Zone NPC master
- **NPC_Pos.tbl** — Spawn koord
- **ZONE_INFO.tbl** — Zone param
- **EventAwards.ini** — War ödül
- **Mykoproject.map.md** — Sistem diagram

---

## 15. DİKKAT NOKTALARI

⚠️ **1098 = 11 core zone** — eski zone'lar kapalı
⚠️ **.smd binary proprietary** — tool zorunlu (hex edit risky)
⚠️ **Moradon M-key bug** — oyuncu frustration
⚠️ **Bifrost YOK** → War Zone + Custom event = ana content
⚠️ **Zone region system** — 64x64 chunk optimize
⚠️ **NPC spawn respawn timer** → aievt değişince reset gerek

---

**Dosya sürümü:** v1.0
**Yazanı:** KODCU | **İnceleme:** —
