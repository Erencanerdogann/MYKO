# 🎮 KO TEMEL — HERKESE 1 SAYFA ÖZET

**Bu MD'yi her agent açılışta okumak zorunda.** Uzmanı olduğun alana girmeden önce **KO oyununun ne olduğunu** bil ki kendi alanını bağlama oturt.

**Bizim oyun:** Bynoisee MalaysiaKO Valor — **2369 base + 1098 patch giydirme** — Lansman 08 Mayıs 2026

---

## 1. KO NEDİR (5 satır)

Knight Online: 2002 Kore yapımı MMORPG. PvP odaklı. 2 ırk savaşır:
- **Karus** (orc benzeri, Cypher laneti) — kuzey, Luferson Castle
- **El Morad / Elmorad** (insan, King Manes) — güney

Oyuncular klan kurar, level 1-72 (1098'de cap 72), Karus vs Elmorad savaşır. Para birimi **Noah** + **GB** (Gold Bar = 100M Noah). National Point (NP) PvP kıllıyla kazanılır, sıralama belirler.

---

## 2. SINIFLAR (4 ana)

| Sınıf | Master (lvl 60+) | Rol |
|-------|------------------|-----|
| **Warrior** | Blade Master (EM) / Berserker Hero (KA) | Tank/Melee |
| **Rogue** | Kasar Hood (EM) / Shadow Vain (KA) | Archer/Assassin burst |
| **Priest** | Paladin (EM) / Shadow Knight (KA) | Heal/Buff/Tank |
| **Mage** | Arch Mage (EM) / Elemental Lord (KA) | Magic burst |
| **Kurian/Porutu** | Transform sınıf | Special |

Job change: lvl 10 (1. job) + **lvl 60 master quest**.

---

## 3. HARITALAR (1098 dönemi)

| Harita | Ne |
|--------|-----|
| **Moradon** | Neutral hub, alış-veriş, anvil |
| **Ronark Land (RL/CZ)** | Ana PvP zone (eski adı Colony Zone) |
| **Eslant** | lvl 60+ PvE, ırk başına ayrı |
| **Delos** | Castle siege (CSW) |
| **War zones** | BattleZone, freezone, war_a |
| **Karus / El Morad** | Başlangıç bölgeleri |
| **Dungeon serisi** | 1098In_dungeon01-03, dungeon_a/b/c |

⚠️ **1098'de YOK:** Bifrost, Ardream, Dragon Cave, Lunar Valley.

---

## 4. ITEM + ANVIL (Upgrade)

- Item slot: 28 envanter
- **Upgrade +1...+9** (1098'de **+8 cap weapon/armor**, +1 accessory)
- **Anvil** Moradon'da, NPC Charon scroll satar
- +7 sonrası başarı %50'nin altı, kırılma riski yüksek
- **Rebirth Scroll** ile +7 → +1 rebirth dönüşümü
- **Item.tbl** + **MagicTable.tbl** — clientte
- **USERDATA + USER_ITEM** — DB'de (USER_ITEMS tekil değil!)

---

## 5. PVP / NP / KLAN

- **NP** = National Point. Solo kill = 50 NP.
- PvP zone: Ardream (1098'de YOK) → bizde **CZ/Ronark**
- **Klan** kurma: Knight rütbesi + 100M Noah + Leadership skill
- **Klan rütbeleri:** Knight, Senior Knight, ChiefKnight, Knight Leader
- **CSW (Castle Siege War):** Delos kalesi savaşı, klan grade 5+ gerekli

---

## 6. SAVAŞ / EVENT (1098)

| Event | Açıklama |
|-------|----------|
| **CSW** | Delos castle siege, haftada 1 |
| **BDW** (Border Defense War) | Sınır savunma, lvl tier ödül |
| **Chaos Expansion** | Chaos Stone, ranking ödül |
| **Juraid Mountain** | 8v8 PvPvE Devabird boss |
| **Forgotten Temple** | Juraid bodrum, 32 oyuncu |
| **Lunar War** | Pzt+Cmt 5/13/19 saatleri |

⚠️ **1098'de Bifrost expansion YOK** → Felankor + custom event boss ana atraksiyon.

---

## 7. EKONOMİ

- **Noah** (gold) — temel para
- **GB** (Gold Bar) — 1 GB = 100M Noah
- **PUS** (Power-Up Store) — cash shop, Premium/Genie/Cape/Cloak satılır
- **NPC alış-satış:** ITEM_SELL.tbl, fiyat sabit
- **Anvil scroll:** Charon (Moradon)
- **Trade:** Oyuncu-oyuncu, Trade window
- **Offline merchant:** AFK trade botu (1098 normal feature)

---

## 8. BİZİM SUNUCU (MalaysiaKO)

| Bilgi | Değer |
|-------|-------|
| **Versiyon** | 2369 base + 1098 patch |
| **Level cap** | 65-72 |
| **Max upgrade** | +8 weapon/armor, +1 accessory |
| **Production IP** | 104.238.23.99 |
| **Game port** | 15001 |
| **Login port** | 15100 |
| **DB** | localhost\MSSQLSERVER01 / KO_MYKO |
| **Web** | malaysiako.com (port 8091) |
| **Anti-cheat** | Pearl Guard (Code Guard) |
| **Lansman** | 08 Mayıs 2026 (Valor) |

---

## 9. BİZİM 5 AGENT — ALANLAR

| Agent | Sorumluluk | Kısa |
|-------|------------|------|
| **MATRIX** | DB + TBL | Server backend, .tbl, schema |
| **CHIP** | SRC + ANTI-CHEAT | C++ server source, Pearl Guard |
| **KODCU** | CLIENT + ASSET + MAP + LUA + LOGIC + PATCH | Oyuncu tarafı bütünü |
| **WEBRA** | WEB + FORUM | Site, API, Flarum forum |
| **GHOST** | ENCRYPTION + TOOLS | 6 katman şifre, key rotation |

---

## 10. BİZE ÖZEL DURUMLAR (memory + tarihçe)

- ⚠️ **TS 381001000 transformation scroll** — bug var (Duration=1, SellPrice=0)
- ⚠️ **strWebHash NULL** — auto-register hesap siteye giremez
- ⚠️ **TLS yok** — MITM açığı
- ⚠️ **Moradon M-key harita bug** — yanlış konum gösteriyor
- ⚠️ **Wall cheat detection** — geçmişte yorum satırına alınmıştı (patlayan projede). Bizimde DOĞRULA.
- ✅ **Pearl Guard port** — 9 faz tamamlandı (F:\MDBACKUP\C--Projects_memory)
- ✅ **98 güvenlik bulgusu / 31 fix** — myko_audit_report.md'de
- ✅ **Packet shift (K1-K10)** — 10/10 tamamlandı

---

## 11. SAYGINLI HATIRLATMA

Patron 3 ay solo emek verdi:
- OpenKO'dan başladı → Rust port denedi (118K LOC, %70) → terk → C++ → patladı → bugünkü proje
- Aynı zamanda **agent ekosistemi (orkestra-rs)** kurdu → tek başına taşımasın diye

**Aynı hatayı yapmamak için:** PROJE_TARIHCESI_VE_DERSLER.md'yi (10 ders) bil.

---

## 12. KAYNAKLAR (DAHA FAZLA İSTERSEN)

- `_KAYNAK_HAVUZU.md` — 80+ URL (Wiki, Reddit, R10, kopazar.com, GitHub source)
- `_OGRENME_PLANI.md` — 17 KO bilgi MD'si yol haritası
- `Mykoproject.map.md` — bizim sistem haritası
- `MATERYAL_HARITASI.md` — element ↔ dosya
- `PROJE_TARIHCESI_VE_DERSLER.md` — emek tarihçesi + 10 ders

---

**SON:** Bu 12 madde temel. Detay için ilgili MD'yi aç. **Halüsinasyon yapma** — emin değilsen `_KAYNAK_HAVUZU` aç.
