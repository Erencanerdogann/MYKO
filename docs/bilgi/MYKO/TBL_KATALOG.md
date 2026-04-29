# TBL Katalog — 246 Dosya (MalaysiaKO 1098)

**Tarih:** 2026-04-29 | **Versiyon:** 1.0 | **Agent:** MATRIX | **Session:** S87+ | **Konum:** `C:\MalaysiaKO\Data\`

---

## Özet

| Metrik | Değer |
|--------|-------|
| **Toplam TBL dosyası** | 246 |
| **Şifreleme** | DES Feistel + K2 XOR (tbl_decrypt.py) |
| **Boyut (binary)** | ~300 MB (tüm Data klasörü) |
| **Locale** | tr (Turkish), us (English), tk (Turkish keyboard?) |
| **Version** | 1098 (Bynoisee custom patch) |

---

## Kategori Dağılımı

| Kategori | Dosya Sayısı | Örnekler |
|----------|--------------|---------|
| **ITEM** | 45+ | Item.tbl, Item_Ext_0-23 (×3 locale), ITEM_SELL, ITEM_UPGRADE |
| **MAGIC / SKILL** | 25+ | MagicTable, MagicType1-9 (×2-3 locale) |
| **NPC / MONSTER** | 10+ | NPC.tbl, NPC_Pos, MonsterData, MonsterDropTable |
| **QUEST** | 1+ | Quest.tbl (locale yok) |
| **CLASS / JOB** | 10+ | Class.tbl, JobClass_*.tbl, StartAttribute, StartEquip |
| **ZONE / MAP** | 15+ | Zone info, DailyRank, INDUN_SCHEDULE |
| **ACHIEVEMENT** | 6 | ACHIEVE_main, ACHIEVE_com, ACHIEVE_mon, ACHIEVE_normal, ACHIEVE_title, ACHIEVE_war |
| **CAPE / CLOAK** | 4 | Cloak, Cloak_pvp, Cape, DisguiseRing (×locale) |
| **CHAT / UI** | 10+ | Chat_Seedmsg, FAQ, Help_*, Connect_image, KnightsRanking |
| **MARKET / EXCHANGE** | 5 | Item_Exchange (×3 locale), ItemEx_* |
| **PATCH / UPDATE** | 3 | VersionList, UpdateInfo |
| **DİĞER** | ~150 | Attendance, BossPhase, KnightSkill, PvPReward, ShuttleInfo, ... |

---

## 🔴 ITEM Kategori (45+ dosya)

### Ana Dosyalar

| Dosya | İçerik | Locale | Boyut ~KB |
|-------|--------|--------|-----------|
| **Item.tbl** | Item master (2100+ item: ID, stat, requirement) | tr | 2000 |
| **Item_Ext_0.tbl** | Item effect detay (magic, special) | tr | 500 |
| **Item_Ext_1-23.tbl** | Item extension part (class/level) | tr | 50 each |
| **Item_Ext_*_us.tbl** | English locale | us | 50 each |
| **Item_Ext_*_tk.tbl** | Turkish keyboard layout | tk | 50 each |
| **ITEM_SELL.tbl** | NPC buy/sell price | — | 200 |
| **ITEM_UPGRADE.tbl** | Anvil +1...+9 (upgrade table) | — | 30 |
| **ITEM_GROUP.tbl** | Item set / bundle | — | 50 |

### DB Mapping
```
Item.tbl (client)  ↔  ITEM (DB)
Item_Ext_*.tbl     ↔  ITEM_EXTENSION
ITEM_SELL.tbl      ↔  ITEM_SELLTABLE
USER envanter      ↔  USERDATA.bItem[144]
```

### Tools
- `tbl_decrypt.py` — DES şifre çöz
- `tbl_edit.py` — Item stat edit (GUI)
- `item_search.py` — Item arama (nama göre)

---

## ✨ MAGIC / SKILL Kategori (25+ dosya)

| Dosya | İçerik | Locale |
|-------|--------|--------|
| **MagicTable.tbl** | Skill ana (300+ spell: ID, name, MP, cooldown, level) | tr |
| **MagicType1.tbl** | Direct damage skill | tr |
| **MagicType2.tbl** | Heal / Buff | tr |
| **MagicType3.tbl** | DoT / HoT | tr |
| **MagicType4.tbl** | Summon / Transform | tr |
| **MagicType5-9.tbl** | Diğer effect tip | tr |
| **MagicType_us** | English | us |
| **KnightSkill.tbl** | Klan skill (3v3, 5v5, CSW) | tr |
| **PusReward.tbl** | Cash shop skill / item | — |

### DB Mapping
```
MagicTable.tbl     ↔  MAGIC_TABLE
MagicType1-9.tbl   ↔  MAGIC_TYPE1-9
Skill bar (client) ↔  USER_SKILL_BAR (DB)
Active buff        ↔  USER_DURATION_SKILL
```

---

## 👹 NPC / MONSTER Kategori (10+ dosya)

| Dosya | İçerik | İlişki |
|-------|--------|--------|
| **NPC.tbl** | NPC master (200+ NPC) | K_NPC (DB) |
| **NPC_Pos.tbl** | NPC spawn lokasyon | aievt (zone event) |
| **MonsterData.tbl** | Monster stat (level, HP, damage) | MONSTER (DB) |
| **MonsterDropTable.tbl** | Monster drop item | NPC_ITEM (DB) |
| **BossData.tbl** | Boss spawn / schedule | NPC_ITEM DROP |
| **BossPhase.tbl** | Boss respawn timer | cron schedule |
| **MobName_us.tbl** | Monster name (English) | — |
| **NPCMovePattern.tbl** | AI walk route | aievt binary |

### Tuzak
- **NPC_ITEM.Probability:** 0-100 scale, 100 = 100% drop
- **MonsterDropTable** vs **NPC_ITEM:** Hangisi kullanılıyor? (S87 rapor)
- **Boss spawn:** INI config vs TBL config (conflict?)

---

## 📚 QUEST Kategori (1+ dosya)

| Dosya | İçerik |
|-------|--------|
| **Quest.tbl** | Quest master (600+ quest) |

### Mantık
- Client'te tbl load (quest görünür)
- **Asıl quest logic:** Lua script'te (511 + 1013 dosya)
- **DB:** QUEST_HELP (text), USER_QUEST_LOG (progress)

### Kaynak
- Client: `Quest.tbl`
- Server Lua: `Desktop\Server\Quests\*.lua` (511)
- Server Lua alt: `server\myko_server\Quests\*.lua` (1013)
- DB: QUEST_HELP, QUEST_SKILLS_CLOSED_DATA, USER_QUEST_LOG

---

## 👕 CLASS / JOB Kategori (10+ dosya)

| Dosya | İçerik |
|-------|--------|
| **Class.tbl** | Sınıf master (4 base: Warrior, Rogue, Priest, Mage) |
| **JobClass_*.tbl** | Master class detay (EM/KA per class) |
| **StartEquip.tbl** | Başlangıç eşya set |
| **StartAttribute.tbl** | Başlangıç stat (class per) |
| **StartSkill.tbl** | Başlangıç skill (class per) |

### Mapping
```
Class.tbl (byClass 0-5)  ↔  Job class master
StartEquip              ↔  BEGINNER_ITEM (DB)
```

---

## 🗺️ ZONE / MAP Kategori (15+ dosya)

| Dosya | İçerik |
|-------|--------|
| **Zone_Info.tbl** | Zone master (40+ zone: level range, PvP flag, spawn point) |
| **ZoneWarp.tbl** | Teleport destination |
| **INDUN_SCHEDULE.tbl** | Dungeon open/close schedule |
| **DailyRank_us.tbl** | Daily ranking list (UI) |
| **ServerZone_us.tbl** | Server zone info (custom) |

### 1098 Zone Listesi
- 1098elmo2004 (El Morad — human start)
- 1098karus2004 (Karus — orc start)
- 1098moradon_0826 (Neutral — trade hub)
- 1098war_a (PvP zone)
- 1098freezone_a/b/c (Free zone varyant)
- 1098In_dungeon01-03 (Dungeon derin)
- BattleZone (Arena)
- (+ 20+ eski/custom zone)

---

## 🏆 ACHIEVEMENT Kategori (6 dosya)

| Dosya | Amaç |
|-------|------|
| **ACHIEVE_main.tbl** | Achievement master (100+ achiev) |
| **ACHIEVE_com.tbl** | Community achievement |
| **ACHIEVE_mon.tbl** | Monster kill achievement |
| **ACHIEVE_normal.tbl** | Normal task achievement |
| **ACHIEVE_title.tbl** | Title unlock achievement |
| **ACHIEVE_war.tbl** | War / PvP achievement |

---

## 👜 CAPE / CLOAK / FASHION Kategori (4+ dosya)

| Dosya | İçerik |
|-------|--------|
| **Cloak.tbl** | PvE cape (item drop) |
| **Cloak_pvp.tbl** | PvP cape (rank reward) |
| **Cape.tbl** | Starter cape set |
| **DisguiseRing.tbl** | Character avatar mask |
| **DisguiseRing_us.tbl** | English version |

---

## 💬 CHAT / UI Kategori (10+ dosya)

| Dosya | İçerik |
|-------|--------|
| **Chat_Seedmsg_us.tbl** | Chat flood message template |
| **FAQ_us.tbl** | FAQ list (UI help) |
| **Help_Small_Class_us.tbl** | Help text (UI) |
| **Help_Medium_Class_us.tbl** | Longer help |
| **Help_Large_Class_us.tbl** | Full help page |
| **Connect_image.tbl** | Loading screen image reference |
| **KnightsRanking_us.tbl** | Klan ranking display |

---

## 💰 MARKET / EXCHANGE Kategori (5 dosya)

| Dosya | İçerik |
|-------|--------|
| **Item_Exchange.tbl** | Item convert (old → new) |
| **Item_Exchange_exp.tbl** | Extended exchange (costume, cape) |
| **Item_Exchange_us.tbl** | English |
| **ItemEx_PvP.tbl** | PvP gear exchange |
| **Genie_Gacha.tbl** | Gacha / summon drop table |

---

## 🔧 PATCH / UPDATE Kategori (3 dosya)

| Dosya | İçerik |
|-------|--------|
| **VersionList.tbl** | Patch version history |
| **UpdateInfo.tbl** | Update announcement text |
| **PatchUrl.tbl** | Download URL |

---

## 📋 DİĞER DOSYALAR (~150)

| Dosya | Amaç | Örnek |
|-------|------|-------|
| **Attendance.tbl** | Daily check-in reward | |
| **BossPhase.tbl** | Boss respawn schedule (cron) | |
| **EventReward.tbl** | Event prize | |
| **EventSchedule.tbl** | Event open/close | |
| **GateInfo.tbl** | Portal warp list | |
| **InventoryWeight.tbl** | Weight limit | |
| **KnightsExp.tbl** | Klan exp table | |
| **KnightsLevel.tbl** | Klan level cost | |
| **LevelExp.tbl** | Character level exp (1-72) | |
| **Resurrection_us.tbl** | Resurrect text | |
| **ShuttleInfo.tbl** | Shuttle (vehicle) | |
| **Teleport_us.tbl** | Teleport NPC list | |
| **TradeInfo.tbl** | Trade restriction | |
| **VipItem.tbl** | VIP benefit item | |
| **WarpGate.tbl** | Gate teleport | |
| **WarZone.tbl** | War event zone | |
| **Warehouse.tbl** | Bank slot limit | |
| **... (110+ more)** | — | — |

---

## ⚠️ Bilinen Bug'lar / Sorunlar

### 1. TS 381001000 (Transformation Scroll)
- **Sorun:** NPC sell price = 1.3M (400k olmalı)
- **Durum:** Çalışmıyor (1098 feature off?)
- **Kontrol:** TBL'de dwSalePrice field = 0 mı?

### 2. Item ID Collision
- **Sorun:** Item.tbl + Item_Ext_*.tbl ID overlap?
- **Kontrol:** Unique ID check yapılmış mı?

### 3. MagicTable vs MagicType Mismatch
- **Sorun:** Skill ID'si bir tabloda yok, diğerinde var
- **Kontrol:** Cross-reference validation

### 4. Locale Senkronizasyon
- **Sorun:** _us vs tr vs tk farkı → client crash
- **Kontrol:** Tüm locale'ler aynı item count var mı?

---

## TBL Edit Prosedürü

```
1. Backup: cp Item.tbl Item.tbl.backup
2. Decrypt: tbl_decrypt.py Item.tbl → Item.json
3. Edit: Item.json düzenle (text editor)
4. Encrypt: tbl_edit.py Item.json → Item.tbl
5. Hash: tbl_hash.py Item.tbl → ITEM_ORG_HASH
6. Config: GameServer.ini [TBL_HASH] güncelle
7. Test: GameServer restart → check boot log
```

---

## Şifreleme Detayı

**Algoritma:** DES Feistel + XOR

```
Input:  Binary TBL data
          ↓
DES Feistel decrypt (K1, K2)
          ↓
XOR with K2
          ↓
Output: Plaintext JSON/CSV
```

**Tool:** `tbl_decrypt.py`
- Otomatik detect: TR vs US vs TK
- Hata handling: corrupt TBL → log error
- Output: Readable binary dump

---

## Erişim & Yaptırım

| İşlem | İzin | Kural |
|-------|------|-------|
| **TBL oku (decrypt)** | ✅ | Backup al |
| **TBL düzenle (local)** | ✅ | DOKTOR onay |
| **TBL sunucuya deploy** | ⚠️ | DOKTOR + Erencan çift onay |
| **TBL sil** | ❌ | YASAK — server crash |
| **TBL hash manipulation** | ❌ | YASAK — anti-cheat bypass |

---

## Kaynak Referansları

- `C:\MalaysiaKO\Data\` — Tüm TBL dosyaları
- `tools\tbl\tbl_decrypt.py` — Decrypt tool
- `tools\tbl\tbl_edit.py` — Edit GUI
- `tools\tbl\tbl_compare.py` — TBL diff
- `tools\tbl\tbl_hash.py` — Hash calculator
- `F:\MDBACKUP\C--Projects_memory\audit\myko_tbl_audit.md` — Detaylı audit (246 dosya)
- `GameServer.ini [TBL_HASH]` — Hash validation

---

**Son Güncelleme:** 2026-04-29 | **Versiyon:** 1.0 | **MATRIX**
