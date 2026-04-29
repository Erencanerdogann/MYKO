# GAME LOGIC — Event Sistemi, PvP, Klan, Ekonomi

**Tarih:** 2026-04-29 | **Kategori:** GAME MECHANICS | **1098 Content:** 6 event tipi, Cape/Premium, Clan system

---

## 1. EVENT SİSTEMİ GENEL

Event = **zaman-belirli PvP/PvE faaliyeti** + **ödül mekanizması**.

**Bileşenler:**
- **EventMainSystem.cpp** — Logic
- **EventMainTimer.cpp** — Scheduler (Scheduler.ini binary)
- **EventAwards.ini** — Ödül config
- **EventSettings.ini** — ON/OFF switch

---

## 2. SCHEDULER.INI (EVENT TIMING)

### Dosya

```
C:\MalaysiaKO\Scheduler.ini (141 KB, 4743 satır)
```

**Format:** Binary hex (human-readable değil)
- **War schedule** (CSW, BDW, Lunar War timing)
- **Server maintenance** window
- **Patch check** interval

**Okuyamaz:** Text editor → `EventMainTimer.cpp` enum mapping gerek

---

## 3. 1098 EVENT TİPLERİ (6 ANA)

### CSW — Castle Siege War (Delos)

| Parametresidir | Değer |
|---------|-------|
| **İsim** | Castle Siege Warfare |
| **Harita** | Delos zone |
| **Zaman** | Cuma + Pazar (Scheduler binary) |
| **Süre** | 60 dakika |
| **Oyuncu** | 2+ klan, max 50/side |
| **Ödül** | Loser=100NP/500Noah, Winner=300NP/3000Noah |
| **Requirement** | Klan grade 5+ |

**Logic:** `CastleSiegeWar.cpp`

**DB:** `KNIGHTS.sNP` → Klan NP artış

### BDW — Border Defense War

| Parametresidir | Değer |
|---------|-------|
| **İsim** | Border Defense War |
| **Zaman** | Haftanın 3 gün (schedule) |
| **Harita** | War Zone |
| **Oyuncu** | Open participation |
| **Ödül** | Level tier (Lvl 50-60: 100NP, Lvl 61-72: 200NP) |
| **Duration** | 30 dakika |

**Logic:** `EventMainSystem.cpp` BDW handler

**DB:** `USER_LOYALTY` → NP increment

### Chaos Expansion / Chaos Stone

| Parametresidir | Değer |
|---------|-------|
| **İsim** | Chaos Expansion |
| **Event** | Stone droplarını toplama, ranking |
| **Ödül** | Top 3: 1st=+500Noah, 2nd=+300Noah, 3rd=+100Noah |
| **Duration** | 45 dakika |
| **Harita** | War Zone |

**Logic:** `EventMainSystem.cpp` chaos handler

### Juraid Mountain Defense

| Parametresidir | Değer |
|---------|-------|
| **İsim** | Juraid Mountain (8v8 PvPvE) |
| **Boss** | Devabird (solo = 8 oyuncu öldürt) |
| **Ödül** | Loser=389205000 item, Winner=389196000 item |
| **Team size** | 8 vs 8 |
| **Duration** | 40 dakika |
| **Harita** | Juraid Mountain zone |

### Lunar War

| Parametresidir | Değer |
|---------|-------|
| **İsim** | Lunar War |
| **Zaman** | Pazartesi + Cumartesi 5/13/19:00 |
| **Level req** | 30+ |
| **Ödül** | Participation reward (NP + item) |
| **Duration** | 120 dakika |
| **PvP** | Open nation |

### Felankor (Custom, 1098 Focus)

| Parametresidir | Değer |
|---------|-------|
| **İsim** | Felankor Event |
| **Zaman** | Servis restart sonrası (custom timing) |
| **Content** | Mini-boss dungeon + treasure |
| **Ödül** | High-tier item spawn |
| **1098 status** | ÖZEL (Bifrost alternative) |

---

## 4. EVENTAWARDS.INI (ÖDÜl MASTERI)

```ini
[BORDER_DEFENSE_WAR]
LVL_50_60_WINNER = 200NP
LVL_50_60_LOSER = 100NP
LVL_61_72_WINNER = 300NP
LVL_61_72_LOSER = 200NP

[CASTLE_SIEGE_WARFARE]
WINNER_ITEM_ID = 389200000  (high-tier drop)
WINNER_NP = 300
LOSER_NP = 100
LOSER_ITEM = 389199000

[CHAOS_EXPANSION]
RANK_1 = +500NP
RANK_2 = +300NP
RANK_3 = +100NP
RANK_4_10 = +50NP

[JURAID_MOUNTAIN_DEFENSE]
WINNER_ITEM = 389196000 (boss item)
LOSER_ITEM = 389205000
PARTICIPATION = 100NP

[LUNAR_WAR]
KILL_REWARD = 50NP per kill
LEVEL_THRESHOLD = 30

[FELANKOR]
BOSS_DROP = 389250000 (rare)
TREASURE_SPAWN = 3x item
```

**Server load:** `EventAwards.ini` → EventMainSystem.cpp event başında

---

## 5. EVENTSETTINGS.INI (SWITCH)

```ini
[EVENT_STATUS]
BORDER_DEFENSE_WAR = 1  (ON)
CASTLE_SIEGE = 1        (ON)
CHAOS_EXPANSION = 1     (ON)
JURAID_MOUNTAIN = 1     (ON)
LUNAR_WAR = 1           (ON)
FELANKOR = 1            (ON)

[CLAN_BUFF_SYSTEM]
STATUS = 1              (Clan premium ON)
```

**Değiştirme:** Server restart needed (`taskkill GameServer`)

---

## 6. KLAN SİSTEMİ (Knights)

### Klan Oluşturma

| Requirement | Değer |
|-------------|-------|
| **Level** | 30+ |
| **Character rank** | Knight (`USERDATA.Authority` 1=user, 2=knight) |
| **Cost** | 100M Noah |
| **Leadership skill** | 1 seviyelik liderlik |

### Klan Rütbeleri

| Rank | Yetki | Upkeep |
|------|-------|--------|
| **GRADE_4** | Temel | 72k loyalty/ay |
| **GRADE_3** | Orta (CSW eligible) | 144k |
| **GRADE_2** | İleri | 360k |
| **GRADE_1** | Master (best ödül) | 720k |

**Loyalty = Klan ödü parasıydı (1098'de fixed)**

### Klan Member Rütbeleri

| Rütbe | Yetki |
|-------|-------|
| **Knight Leader** | Full control |
| **Chief Knight** | Manage members |
| **Senior Knight** | Moderator |
| **Knight** | Member |

**DB:**
```sql
KNIGHTS           — Klan master
KNIGHTS_USER      — Membership
KNIGHTS_RANK      — Rütbe config
CLAN_BANK         — Klan vault
```

---

## 7. CLAN PREMIUM (BUFF SISTEM)

### Buff Özellikleri

```ini
[CLAN_PREMIUM]
EXP_BONUS = +30%        — Leveling hız
NP_BONUS = +8           — NP kazanç flat
DROP_RATE = +1%         — Loot rate
NOAH_BONUS = +30%       — Gold earning
SELL_PERCENT = +50%     — NPC alış fiyat
```

### Nasıl Çalışır

1. Oyuncu **klan üyesi** → `KNIGHTS_USER.clan_id = X`
2. **Clan grade** check → `KNIGHTS.grade`
3. **Login** sırasında buff apply → `USER_DURATION_SKILL`
4. **Zone change/logout** → buff remove

**DB:** `USER_DURATION_SKILL` table (timer track)

---

## 8. CAPE BONUS (Capa)

### Cape İtem Bonusu

```
CapeBonus.txt:
BONUS_HP = 300
BONUS_MP = 150
BONUS_AP = 3        (Attack Power)
BONUS_NP = +5       (National Point flat)
```

### Mekanizm

- **Item equipped** → "Cape Slot"
- **Login sırasında** HP/MP/AP stat addition
- **Logout** → removal

**Dosya:** `C:\MalaysiaKO\CapeBonus.txt` (oyuncu config)

---

## 9. PVP / SAVAŞ SİSTEMİ

### Attack Handler

`AttackHandler.cpp`:
1. **Player A attacks Player B**
2. **PvP check:** Same zone + opposite nation?
3. **Hit calc:** Accuracy, dodge, critical
4. **Damage calc:** `BattleSystem.cpp` formula

### Damage Formülü (Basit)

```
Damage = (Attack Power) - (Defense) * crit_multiplier
Critical = (Dexterity × 0.5) % chance
```

### Kill Reward

- **Solo kill** → 50 NP
- **Party kill** → 50 NP / party_size
- **PvP zone** → only valid (Ardream / War Zone)
- **1098'de** → War Zone (Ardream YOK)

---

## 10. RIVAL / RANKING SİSTEMİ

### Rival List

**Oyuncu rakip ekleme:**

```sql
USER_RIVAL — Rakip listesi
├── user_id
├── rival_id
├── kill_count (hedefedilen öldürme sayısı)
└── last_kill_date
```

### Daily Ranking

```sql
RANK_DAY  — Günlük sıralama (24-hour reset)
RANK_MONTH — Aylık sıralama (her ayın sonunda reset)
```

**Kriterler:**
- Total NP
- Total kill count
- Level

---

## 11. NP SİSTEMİ (National Point)

### NP Kazanç

| Aktivite | Kazanç |
|----------|--------|
| **PvP kill** | 50 NP (nation match) |
| **Event win** | 100-500 NP (event type) |
| **Clan premium** | +8 NP (buff) |
| **Cape** | +5 NP (flat) |

### NP Kaybı

- **Level 10'dan düşük:** NP yok
- **Death:** NP kaybı yok (1098'de)

### NP Sıralama

```sql
SELECT TOP 100 * FROM USERDATA ORDER BY sLoyalty DESC;
```

**Küçülebilir:** `/ranking` ingame command

---

## 12. EKONOMI BALANCE (1098)

### Item Price Tablo (ITEM_SELL.tbl)

**NPC alış-satış fiyat (fixed):**

```
Iron Sword (ID=1001): BUY=50k, SELL=25k
Leather Armor (ID=1002): BUY=80k, SELL=40k
...
```

### Gold Earning Rate

| Aktivite | Noah kazanç |
|----------|-------------|
| **Mob kill** | Lvl × 1000 |
| **Quest reward** | 10k-100k |
| **Trade** | Player-to-player (NPC intermediary) |
| **Clan premium** | +30% (multiplier) |
| **Offline merchant** | Passive income |

### GB (Gold Bar)

- **1 GB = 100M Noah**
- **Real-money cash shop** (PUS)

---

## 13. BONUS SISTEM ÖZET

| Bonus Türü | Source | Amount | Stacking |
|------------|--------|--------|----------|
| **Clan EXP** | Klan premium | +30% | Yes |
| **Cape HP** | Item | +300 HP | Yes |
| **Cape AP** | Item | +3 attack | Yes |
| **Cape NP** | Item | +5 flat | Yes |
| **Event NP** | Participation | 50-300 | Event-only |
| **Level scale** | Base | Lvl × coefficient | Auto |

---

## 14. 1098 ÖZET — EVENT + EKONOMI

### Aktif Event

1. **CSW** — Delos (Cuma/Pazar)
2. **BDW** — War Zone (3 gün/hafta)
3. **Chaos Expansion** — War Zone (40 min)
4. **Juraid Mountain** — (8v8, 40 min)
5. **Lunar War** — (2 gün/hafta)
6. **Felankor** — (restart sonrası, **custom**)

### Ekonomi Engine

- **Fixed NPC price** (ITEM_SELL.tbl)
- **Level-based mob loot** (scalable)
- **Clan premium multiplier** (30% exp, 30% gold)
- **Cape bonus** (flat stat)
- **Premium cash shop** (PUS)

### 1098'de YOK

- Bifrost event
- Ardream zone (PvP)
- Dragon Cave dungeon
- Advanced crafting

---

## 15. PERF IMPACT NOTES

| Event | Load | Lag Risk | Mitigation |
|-------|------|----------|------------|
| **CSW** | High | Network bottleneck | Max 50/side |
| **Lunar War** | Medium | Particle spam | Cap max players |
| **Chaos** | Low | Item spam | Cleanup timer |
| **Juraid** | Medium | Boss AI | Single instance |

---

## 16. DİKKAT NOKTALARI

⚠️ **1098 = 6 event** (Bifrost yok = alternative Felankor)
⚠️ **Scheduler.ini binary** — timing human-readable değil
⚠️ **Klan premium +30%** → exploit risk (rate control)
⚠️ **Cape +5NP** → farming mechan-abuse check
⚠️ **Offline merchant** — passive gold (balance watch)
⚠️ **NP reset yok** → rank permanent (account age advantage)

---

## 17. KAYNAKLARA BAĞLA

- **EventAwards.ini** — Ödül config
- **EventSettings.ini** → Event ON/OFF
- **CastleSiegeWar.cpp** → CSW logic
- **EventMainSystem.cpp** → Event manager
- **BattleSystem.cpp** → Damage formula

---

**Dosya sürümü:** v1.0
**Yazanı:** KODCU | **İnceleme:** —
