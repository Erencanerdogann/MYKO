# 💰 CASH SHOP / PUS — Power-Up Store Sistemi

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `ShoppingMallHandler.cpp`, `PusItemSet.h`, `PusCategorty.h`, `PusRefund.cpp`, `GameDefine.h:4335-4345`
**Hedef:** Cash shop akışı, DB tabloları, GM komut, payment akışı.

---

## 1. PUS NEDİR

**P**ower-**U**p **S**tore — KO'nun in-game cash shop sistemi. Oyuncu gerçek para ile **GM Coin / Cash** alır, oyun içinde Premium / Cape / Genie / Cloak / Buff / Item satın alır.

**Akış:**
```
Web ödeme → DB Cash kolonu artır → Oyuncu in-game F10 (Mall) açar
   → PUS UI → Item seç → STORE_BUY → DB stok azalt → Item posta ile gelir
```

---

## 2. DB TABLOLARI

### A) `PUS_ITEMS` — Satılan Itemlar
**Kolonlar** (`PusItemSet.h:9-10`):
| Kolon | Tip | Açıklama |
|-------|-----|----------|
| ID | uint32 | PUS unique ID |
| ItemID | uint32 | DB ITEM tablosu ID |
| Price | uint32 | Fiyat (Cash veya Coin) |
| BuyCount | uint32 | Stok / Satış sayacı |
| Category | byte | Kategori (PUS_CATEGORY.CategoryID) |
| PriceType | byte | Para birimi (1=Cash, 2=Coin, vb.) |

**Yükleme:** `LoadServerData.cpp:867` — `LOAD_TABLE(CPusItemSet, ...)` boot zamanı + `+reloadpus`

### B) `PUS_CATEGORY` — Kategori Adları
**Kolonlar** (`PusCategorty.h:9-10`):
| Kolon | Tip | Açıklama |
|-------|-----|----------|
| ID | uint32 | Unique |
| CategoryName | string | Görünen ad |
| CategoryID | byte | Kategori numarası |
| Status | byte | 1=aktif, 0=gizli |

### C) `_PUS_REFUND` (`GameDefine.h:4343`) — İade
| Kolon | Tip | Açıklama |
|-------|-----|----------|
| itemid | uint32 | İade item ID |
| itemprice | uint32 | İade fiyat |
| itemcount | uint16 | Adet |
| itemduration | uint16 | Süre (gün) |
| expiredtime | uint32 | Geçerlilik (UNIX) |
| accountid | string | Hangi hesap |
| buytype | uint8 | Satın alma tipi |

### D) İlgili
- **TB_USER.cash** — Web hesabın cash bakiyesi (web ödeme buraya yazıyor)
- **USERDATA** veya `LOAD_WEB_ITEMMALL` SP — in-game tetikleyici

---

## 3. AKIŞ DETAYI

### A) Web Ödeme → Cash
1. Oyuncu siteye girer (`koweb2`)
2. Cash purchase sayfası → ödeme yöntemi (PayTR/iyzico/Papara/havale — **MYKO'da kurulum bekleniyor**)
3. Ödeme onayı → backend webhook → `UPDATE TB_USER SET cash = cash + N WHERE strAccountID = ...`
4. Oyuncu in-game F10'a basar

### B) In-Game Açma (`ShoppingMallHandler.cpp:35-76`)
```cpp
HandleStoreOpen():
  - Ölü/trade/merchant değilse aç
  - Private arena (zone 40-45) YASAK
  - Min 1 boş envanter slotu şart
  - m_bStoreOpen = true
  - UserDataSaveToAgent() — DB save
```

### C) Item Satın Alma
```cpp
STORE_BUY → ShoppingMall(pkt)
  → Cash yeterli mi? (TB_USER.cash >= Price)
  → Stok var mı? (PUS_ITEMS.BuyCount > 0)
  → Envanter slot var mı?
  → DB transaction:
      - TB_USER.cash -= Price
      - PUS_ITEMS.BuyCount -- (varsa stok takip)
      - LetterSystem ile posta gönder VEYA direkt envanter
  → Client güncelle (envanter)
```

### D) Posta (Letter System) ile Hediye
- `STORE_LETTER` → `LetterSystem(pkt)` → ItemID + adet + süre belirle
- Alıcıya **in-game posta** olarak gelir (öteki oyuncu)
- Cash yerine **Coin** (oyun içi GB) ile de hediye gönderilebilir
- ⚠️ Anti-dupe için süre + log kontrol

### E) Refund (İade)
- `PusRefund.cpp:176` — `_PUS_ITEM* item = m_PusItemArray.GetData(pusid)`
- Süresi dolan/iade edilen item DB'ye yazılır
- Admin manuel veya otomatik scheduled job

---

## 4. KATEGORİLER (Standart KO 1098)

⚠️ Gerçek `PUS_CATEGORY` DB SELECT şart, aşağıdaki referans:

| ID | Kategori | İçerik |
|----|----------|--------|
| 1 | **Premium** | Silver/Gold/Platinum 1-30 gün |
| 2 | **Genie** | Genie pet süreli |
| 3 | **Cape / Cloak** | Capa stat bonusu |
| 4 | **Pet** | Pet süreli |
| 5 | **Buff Item** | EXP/NP/Drop scroll |
| 6 | **Anvil Scroll** | +X upgrade scroll |
| 7 | **Reset Scroll** | Stat/skill reset |
| 8 | **Costume / Fashion** | Görünüm |
| 9 | **Mount** | Binek (varsa) |
| 10 | **Special** | Event item |

---

## 5. GM KOMUT (PUS yönetim)

| Komut | Açıklama |
|-------|----------|
| `+reloadpus` | PUS_ITEMS tablosu hot reload |
| `+give <Nick> <ItemID> <Adet> <Süre>` | Manuel hediye (cash yemeden) |
| `+online_give_item <ItemID> <Adet> <Süre>` | Tüm online hediye |
| `+zone_give_item <ZoneID> <ItemID> <Adet> <Süre>` | Zone'daki herkese |
| `+givegenie <Nick> <Süre>` | Genie süresi (PUS bypass) |
| `+genie <Nick>` | Genie aç/kapat |

---

## 6. ŞIRKET TARAFI (Web ↔ DB)

### A) Cash Bakiye Sorgu
```sql
SELECT strAccountID, cash FROM TB_USER WHERE strAccountID = 'X';
```

### B) Cash Ekleme (ödeme webhook)
```sql
UPDATE TB_USER SET cash = cash + 100 WHERE strAccountID = 'X';
INSERT INTO CASH_LOG (strAccountID, miktar, kaynak, tarih)
   VALUES ('X', 100, 'PayTR', GETDATE());
```

⚠️ **AUDIT LOG ŞART** — finansal işlem, izlenebilir olmalı.

### C) Manuel İade
```sql
UPDATE TB_USER SET cash = cash + N WHERE strAccountID = 'X';
INSERT INTO REFUND_LOG (...);
```

---

## 7. ANTI-FRAUD / GÜVENLİK

| Risk | Önlem |
|------|-------|
| Cash duplicate | Transaction ID unique constraint |
| Race condition cash → buy | DB row lock (ROWLOCK hint) veya SP |
| Refund abuse | Log + manual onay |
| Item dupe (stack overflow) | itemcount uint16 → max 65535 cap |
| GM `+give` log eksik | GM komut log şart |
| Webhook spoofing | Imza doğrulama (HMAC) |
| Cash negatif | `CHECK (cash >= 0)` constraint |

---

## 8. LANSMAN HAZIR DURUMU (Kontrol Listesi)

- [ ] `PUS_ITEMS` DB dolu mu? (`SELECT COUNT(*)`)
- [ ] `PUS_CATEGORY` doldu mu?
- [ ] PUS_ITEMS.ItemID — hepsi DB ITEM'da var mı?
- [ ] Premium / Genie / Cape başlangıç paketleri seçildi mi?
- [ ] Web ödeme entegrasyonu kuruldu mu? (PayTR/iyzico/Papara)
- [ ] Webhook endpoint güvenli mi? (HMAC)
- [ ] CASH_LOG / REFUND_LOG tabloları var mı?
- [ ] `+reloadpus` test edildi mi?
- [ ] In-game F10 açılıyor mu?
- [ ] Test satın alma → posta → envanter akışı OK mı?
- [ ] Cash bakiyesi web↔in-game senkron mu?
- [ ] Lansman özel **indirim/bonus** ayarlandı mı?

---

## 9. ÖRNEK PUS_ITEMS GİRDİSİ (Tahmini, doğrula)

```sql
INSERT INTO PUS_ITEMS (ID, ItemID, Price, BuyCount, Category, PriceType)
VALUES
  (1, 379154000, 100, 999, 1, 1),   -- EXP Scroll, 100 cash, kategori Premium
  (2, 900142000, 50, 999, 5, 1),    -- Buff item, 50 cash
  (3, 110001000, 200, 999, 6, 1);   -- Anvil scroll +X, 200 cash
```

⚠️ **Item ID ve Price patron tarafından belirlenir** — bu sadece şablon.

---

## 10. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **Web ↔ in-game cash atomic değil** — webhook fail → cash eksik kalabilir, retry mekanizması şart |
| 2 | **PriceType** — 1=Cash, 2=Coin (GB), 3=Loyalty? — DB select ile doğrula |
| 3 | **BuyCount** — stok mu, satış sayacı mı? Schema'ya göre değişir |
| 4 | **Refund expired** — `expiredtime` UNIX timestamp, scheduled job temizler |
| 5 | **Hediye sistemi** (LetterSystem) — anti-dupe kritik |
| 6 | **Çoklu cihazdan aynı anda buy** — race condition test |
| 7 | **GM `+give` log** — denetim için ŞART (audit trail) |
| 8 | **Premium expire** — günlük scheduled job (DB SP) çalışmalı |

---

## 11. KAYNAK REFERANSLAR

- **Server kod:** `ShoppingMallHandler.cpp`, `PusItemSet.h`, `PusCategorty.h`, `PusRefund.cpp`
- **Struct:** `GameDefine.h:4335-4345` (`_PUS_ITEM`, `_PUS_REFUND`)
- **Yükleme:** `LoadServerData.cpp:867`
- **Anti-cheat:** `XGuard.cpp:1216` (`m_PusItemArray` validation)
- **GM komut:** `GM_KOMUT.md § 5`
- **Web ödeme:** `WEB_PHP.md`, `WEB_API.md`
- **DB:** `DB_SEMA.md`, `DB_STORED_PROC.md`
- **Lansman blocker:** `LANSMAN_CHECKLIST.md`

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Production `PUS_ITEMS` SELECT sonucu ile güncellenecek (MATRIX işi). Web ödeme entegrasyonu (WEBRA) eklendiğinde detay yaz.
