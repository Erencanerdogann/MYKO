# 🎁 NPC DROP / LOOT TABLE — Drop Sistemi

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `MonsterItemSet.h`, `Npc.h:146`, `LoadServerData.cpp:135-158`, `Npc.cpp` (drop logic)
**Hedef:** NPC/Monster drop tablosu, oran sistemi, GM test komutları.

---

## 1. DROP SİSTEMİ NASIL ÇALIŞIR

### Akış
```
Monster ölür (HP=0)
   ↓
Npc.h: m_iItem (drop index) → K_MONSTER_ITEM.sIndex
   ↓
12 slot kontrol (iItem01..iItem12, sPersent01..sPersent12)
   ↓
Her slot için: rand(0,10000) < sPercent ? drop : skip
   ↓
WIZ_ITEM_DROP packet → zone'a item düşür
   ↓
Pickup: yakındaki oyuncu (party varsa party share)
```

### Drop Index Mantığı
- **`Npc.h:146`** — `int m_iItem` = "drop index of monster/npc (K_MONSTER_ITEM table sIndex)"
- Aynı drop tablosunu **birden fazla NPC** kullanabilir (örn: aynı kategori mob)
- Boss'lar **özel index** kullanır (yüksek oran + nadir item)

---

## 2. DB TABLO — K_MONSTER_ITEM

`shared/database/MonsterItemSet.h` (gerçek schema):

| Kolon | Tip | Açıklama |
|-------|-----|----------|
| **sIndex** | uint16 | Drop tablosu unique ID (PK) |
| iItem01..iItem12 | uint32 (×12) | Drop edilebilir Item ID |
| sPersent01..sPersent12 | uint16 (×12) | Drop oranı (0-10000, basis points) |

**`LOOT_DROP_ITEMS = 12`** sabit — her tablo max 12 item.

### Oran Hesabı
```
sPercent = 10000  → %100 garanti drop
sPercent = 1000   → %10 drop
sPercent = 100    → %1 drop
sPercent = 10     → %0.1 drop (nadir)
sPercent = 1      → %0.01 drop (rare/boss item)
```

⚠️ `0` → asla drop etmez (slot kapalı).

---

## 3. ÖRNEK DROP TABLOSU

```sql
-- sIndex=1: Goblin Scout drop
INSERT INTO K_MONSTER_ITEM
  (sIndex, iItem01, sPersent01, iItem02, sPersent02, iItem03, sPersent03, ...)
VALUES
  (1,
   379154000, 5000,   -- Noah pouch %50
   110001000, 100,    -- Sword +0 %1
   900142000, 50,     -- Buff scroll %0.5
   ...
  );
```

**Sıralama önemli mi?** — Hayır, her slot bağımsız `rand` testi. Ama düzen için:
- 1-3: garantili / sık (gold, common)
- 4-8: orta (common item)
- 9-12: nadir (rare/boss item)

---

## 4. NPC ↔ DROP EŞLEME

### Tablo: `K_NPC` (NPC ana tablosu)
```
sNpcID, strName, sLevel, sHP, sMP, ...
m_iItem  → K_MONSTER_ITEM.sIndex (drop index)
```

### Yükleme
```cpp
// LoadServerData.cpp
LoadMonsterItemTable() → CMonsterItemSet → m_MonsterItemArray
LoadNpcTable() → m_NpcItemArray (NPC bilgi)
```

### Reload (runtime)
```cpp
ChatHandler.cpp:1774-1777
  m_NpcItemArray.DeleteAllData();
  m_MonsterItemArray.DeleteAllData();
  LoadMonsterItemTable();
```
GM komut: `+reloaddrops`

---

## 5. GM TEST KOMUTLARI (`GM_KOMUT.md § Drop`)

| Komut | Açıklama |
|-------|----------|
| `+drop N` | Z hedefli NPC drop testi (max 9999 simulation) |
| `+fishing` | Balık drop test (Fishing system) |
| `+mining` | Maden drop test (Mining system) |
| `+npcinfo` | Z hedefli NPC bilgi (drop index dahil) |
| `+mon <DBID>` | NPC spawn (drop test için) |
| `+kill` | Z hedef öldür (drop tetikle) |
| `+reloaddrops` | K_MONSTER_ITEM hot reload |

### Test Akışı
```
1. +mon 1234           # Mob spawn
2. Z ile mob target
3. +drop 100           # 100 kez drop simulation
4. Console output: hangi item kaç kez düştü
```

---

## 6. ÖZEL DROP SISTEMLERI

### A) Boss Drop (yüksek oran nadir item)
- Felankor (World Boss): unique drop index, %5-10 boss item
- Custom Event Boss: `+event` ile spawn, özel drop
- Juraid Devabird: event içinde ek ödül (`EventAwards.ini`)
- Forgotten Temple: özel drop pool

### B) Event Drop (level segmenti)
- BDW kazanan/kaybeden → `EventAwards.ini` ödül
- Chaos Expansion → ranking ödülü
- Tournament → klan ödülü

### C) Trap / Special Spawn
- `K_NPC` üzerinde özel flag
- Treasure mob, Goblin (kaçan)

### D) Fishing/Mining
- ResourceTBL benzeri, oran değişkenli
- `+fishing` / `+mining` GM ile test

---

## 7. PARTY SHARE / ZONE INSERT

### Zone Insert (`packets.h:37`)
```cpp
WIZ_ITEM_DROP = 0x23   // Zone Item Insert
```

### Pickup Logic
- Party varsa: party leader veya rotation
- Solo: yakındaki oyuncu
- "Looter only" timer (5-10sn) — sonra herkes alabilir
- ⚠️ Anti-pickup-spoof check: distance + line-of-sight

---

## 8. ANTI-CHEAT (Drop Tarafı)

| Risk | Önlem |
|------|-------|
| Item dupe | Server-side validation (envanter slot lock) |
| Drop hack (uzaktan pickup) | Distance check (server) |
| Çift pickup race | DB transaction lock |
| Drop rate manipulation | Sadece server hesap (client güvenmez) |
| Stack overflow (count=65535+) | uint16 cap |
| GM `+drop` log | Audit trail |

`XGuard.cpp` ek kontrol yapar (drop edilen item DB'de var mı, valid mi).

---

## 9. ÖRNEK İSTATİSTİK SORGULARI

### Tüm drop tablosu
```sql
SELECT sIndex, COUNT(*) AS slot_dolu
FROM (
  SELECT sIndex, iItem01 AS item, sPersent01 AS pct FROM K_MONSTER_ITEM WHERE iItem01 > 0
  UNION ALL SELECT sIndex, iItem02, sPersent02 FROM K_MONSTER_ITEM WHERE iItem02 > 0
  -- ... 12 kez
) t
GROUP BY sIndex
ORDER BY slot_dolu DESC;
```

### En çok düşen item
```sql
SELECT iItem01 AS item, AVG(sPersent01) AS ortalama_oran
FROM K_MONSTER_ITEM WHERE iItem01 > 0
GROUP BY iItem01
ORDER BY ortalama_oran DESC;
```

### Boş drop tablosu (hata)
```sql
SELECT sIndex FROM K_MONSTER_ITEM
WHERE iItem01=0 AND iItem02=0 AND ... AND iItem12=0;
```

---

## 10. LANSMAN KONTROL LİSTESİ

- [ ] `K_MONSTER_ITEM` dolu mu? (`SELECT COUNT(*)`)
- [ ] Tüm `iItem` referansları DB ITEM'da var mı? (orphan item ID YOK)
- [ ] Boss drop tabloları ayrı index mi?
- [ ] Drop oranları balanced mi? (lansman ekonomi)
- [ ] `+drop 100` test sonucu beklenenle uyumlu mu?
- [ ] Party share doğru çalışıyor mu?
- [ ] Pickup distance check aktif mi?
- [ ] `EventAwards.ini` ek drop'lar `K_MONSTER_ITEM` ile çakışmıyor mu?
- [ ] Custom mob/boss drop tabloları yazıldı mı?
- [ ] `+reloaddrops` test edildi mi?

---

## 11. SIK KARŞILAŞILAN BUG / DURUMLAR

| Bug | Sebep | Çözüm |
|-----|-------|-------|
| **Mob hiç düşmüyor** | `m_iItem` değeri DB'de yok | NPC tablosuna doğru sIndex yaz |
| **Sürekli aynı item** | Tek slot dolu, oran yüksek | Diğer slotları doldur |
| **Item ID geçersiz** | Item silindi, tablo eski | `+reloaddrops` veya DB temizlik |
| **Çift drop** | Race condition | Server lock kontrol |
| **Boss item düşmüyor** | Oran çok düşük (sPercent=1) | `+drop 1000` simülasyon doğrula |
| **Party hatalı pickup** | Party share logic bug | `Npc.cpp` party check kod inceleme |

---

## 12. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **sPercent basis 10000** — yüzde değil! 1000 = %10, 10000 = %100 |
| 2 | **12 slot** sabit — daha fazla item için aynı sIndex'e ek satır YOK, kod değişikliği gerekir |
| 3 | **iItem = 0** = slot kapalı (drop yok) |
| 4 | **Drop tablosu boş NPC** = "no drop" mob (event mob, statue, vs.) |
| 5 | **Boss respawn süresi** drop ile alakalı değil, `K_NPC.sRespawnTime` |
| 6 | **Lansman dengesizliği** — yüksek oran rare item ekonomi bozar |
| 7 | **Reload sonrası hash kontrolü** — `+reloaddrops` sonrası TBL_HASH güncel olmalı |
| 8 | **Audit log** — GM `+drop` test çağrıları log'a |

---

## 13. KAYNAK REFERANSLAR

- **DB Schema:** `shared/database/MonsterItemSet.h`
- **NPC tablo:** `Npc.h:146` (`m_iItem`)
- **Yükleme:** `LoadServerData.cpp:135-158`
- **Reload:** `ChatHandler.cpp:1774-1777`
- **Drop logic:** `Npc.cpp:6642` (`WIZ_ITEM_DROP` packet)
- **Anti-cheat:** `XGuard.cpp`
- **Packet:** `shared/packets.h:37` (`WIZ_ITEM_DROP = 0x23`)
- **GM komut:** `GM_KOMUT.md § 3-4`
- **DB:** `DB_SEMA.md § ITEM`
- **TBL:** `TBL_KATALOG.md § MONSTER`

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Production `K_MONSTER_ITEM` SELECT ile gerçek drop tabloları doğrulanacak (MATRIX). Boss drop özel tablolar derlenecek.
