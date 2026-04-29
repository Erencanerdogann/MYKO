# Güvenlik Bug Listesi — MYKO Knight Online

**Tarih:** 29 Nisan 2026  
**Kaynak:** F:\MDBACKUP\C--Projects_memory\audit\myko_audit_report.md  
**Toplam:** 98 bulgu, 31 fix uygulandı, 61 kalan

---

## Özet

| Seviye | Bulunan | Fix Yapılan | Kalan |
|--------|---------|------------|-------|
| CRITICAL | 19 | 14 | 5 |
| HIGH | 20 | 14 | 6 |
| MEDIUM | 31 | 3 | 28 |
| LOW | 28 | 0 | 28 |
| **TOPLAM** | **98** | **31** | **67** |

### Düzeltilmiş 31 Bug (Uygulanmış)

**Server:** 8 fix (SQL injection, buffer overflow, heap safety, gold overflow)  
**Login:** 3 fix (brute force, buffer overflow, validation)  
**Client:** 11 fix (socket buffer, paket boyut, magic check, null deref, file overflow)  
**Shared:** 3 fix (ByteBuffer overflow, format argümanı)  
**N3Base:** 6 fix (division by zero, double-add, delete[], null deref, limits)  

---

## Atlanan Bulgular (5)

| Bulgu | Seviye | Neden Atlanmadı |
|-------|--------|-------------------|
| JvCryption hardcoded key | CRITICAL | Private server için beklenen |
| JvCryption XOR cipher | HIGH | Oyun protokolü, standart |
| Login plaintext (no TLS) | CRITICAL | Oyun protokolü, TLS eklenmez |
| DB plaintext password | HIGH | Schema degişikliği gerekli, prod risk |
| Merchant close exploit | HIGH | B44 özel fix gerekli |

---

## Paket Shift Fixleri (K1-K10)

**Durumu:** ✅ TAMAMLANDI (10/10)  
**Dosya:** F:\MDBACKUP\C--Projects_memory\pearl_guard\myko_packet_shift_fixes.md

Tüm K1-K10 packet shift exploitleri kapatıldı:
- K1 duplicate packet key validation
- K2-K10 spoofing detection
- Sequence number verification

---

## Pearl Guard Port (Anti-Cheat C++)

**Durumu:** ✅ TAMAMLANDI (9 faz)  
**Detay:** F:\MDBACKUP\C--Projects_memory\pearl_guard\pearl_guard_port.md

**Opcode Durumu:**
- ✅ 21 opcode fully implemented
- ⏳ 9 opcode portable (refactor ufak)
- 🔴 3 opcode hook gerekli (engine integration)

---

## Bilinen Exploit'ler (Açık/Kapatılmış)

| Exploit | Durum | Açıklama |
|---------|-------|----------|
| Wall cheat (detection) | ⏳ DOĞRULA | CharacterMovementHandler.cpp:197 yorum satırı |
| TS 381001000 (Transformation) | 🔴 AÇIK | Duration=1, SellPrice=0 bug |
| Web TLS yok | 🔴 AÇIK | MITM riski (WEBRA'nın alanı) |
| strWebHash NULL | 🔴 AÇIK | Auto-register sorun (WEBRA) |
| Item upgrade (Anvil) | ⏳ DOĞRULA | Rate manipulation potansiyeli |
| QUOTED_IDENTIFIER bug | ✅ KAPATILDI | 199 SP, ALTER PROC |
| K1-K10 packet shift | ✅ KAPATILDI | 10/10 fix |
| Brute force (login) | ✅ KAPATILDI | 5 basarısız → 10dk kilit |
| Buffer overflow (news) | ✅ KAPATILDI | Boyut kontrolü |
| SQL injection | ✅ KAPATILDI | Parametreli query |

---

## 1098 Dönemi Bilinen Exploit'ler

| Exploit | Detay | Kurus |
|---------|-------|-------|
| Anvil rate hack | +9 success rate = %100 | Money dupe |
| Drop level abuse | Drop table scaling | XP/item dupe |
| Trade race dupe | Multi-trade concurrent | Item dup |
| Wall hack | No collision detection | PvE abuse |
| Invisibility | State bit flip | PvP abuse |
| Item duplication | Copy inventory slot | Economy break |

---

## Kategori: SQL Injection

| Dosya | Satır | Durum | Açıklama |
|-------|-------|-------|----------|
| DBAgent.cpp | 4997 | ✅ KAPATILDI | Parametreli query |
| globals.h | 667 | ✅ KAPATILDI | Sanitizasyon güçlendirildi |

---

## Kategori: Buffer Overflow / Heap Safety

| Dosya | Satır | Durum | Detay |
|-------|-------|-------|-------|
| Bird.cpp | 131 | ✅ KAPATILDI | fscanf %259s (bounded) |
| APISocket.cpp | 226 | ✅ KAPATILDI | Paket boyut guard |
| N3Chr.cpp | 1710 | ✅ KAPATILDI | Division by zero |
| N3Texture.cpp | 617 | ✅ KAPATILDI | Double-add size |
| LoginSession.cpp | - | ✅ KAPATILDI | Buffer check |

---

## Kategori: Brute Force Koruma

| Dosya | Durum | Çözüm |
|-------|-------|-------|
| LoginSession.cpp | ✅ KAPATILDI | 5 fail → 10dk lock |

---

## Kategori: Race Condition

| Dosya | Durum | Not |
|-------|-------|-----|
| TradeHandler.cpp | ⏳ MEDIUM | Single DB connection (parallelism limited) |
| MerchantHandler.cpp | ✅ KAPATILDI | SLOT_MAX check |

---

## Kategori: Paket Shift (K1-K10)

**Durumu:** ✅ TAMAMLANDI (10/10)

---

## KALAN BULGULAR (61) — Kategoriler

### MEDIUM (31)

**Performans:**
- Per-packet heap allocation (new/delete loop) — Client
- Sleep(2000) main thread freeze — Client
- Debug file open/close every frame — Client
- FindChildByIDRecursive risk — N3Base
- GlobalAlloc legacy — N3Base
- O(n) lookup (index) — N3Base
- 256KB packet allocation — Login

**Kalite:**
- strcpy → strncpy — Server
- Dead HOOKACTIVE code — Server
- Duplicate merchant path — Server
- Magic number'lar — Server
- Hardcoded sub-opcode — Shared

**Stabilite:**
- Static buffer thread-safety — Client
- TBL duplicate key — N3Base
- GrassNum hardcoded — N3Base

### LOW (28)

- Dead opcode'lar
- Version duplication
- Enum gaps
- Reference count issue
- Static thread-safety risk
- Render redundancy
- ODBC plaintext (not used, config override)

---

## Şu Ana Kadar Fix Edilmemiş Açık Exploit'ler

| Exploit | Risk | Açıklama | Kimin İşi |
|---------|------|----------|----------|
| Anvil rate %100 | 🔴 YÜKSEK | +9 başarısı garantili | KODCU/RUSTIK |
| TS 381001000 | ⚠️ ORTA | Transform scroll duration=1 | KODCU |
| TLS yok | 🔴 YÜKSEK | MITM packet sniff | WEBRA |
| strWebHash NULL | ⚠️ ORTA | Auto-register bypass | WEBRA |

---

## Detay Verme Prensibi

✅ **VER:** "Var/yok" + "kapatıldı/açık" durumu  
✅ **VER:** Risk seviyesi (CRITICAL/HIGH/MEDIUM/LOW)  
✅ **VER:** Dosya:satır referansı  
🔴 **YAZMA:** "Nasıl exploit edilir" kod/yöntemi  
🔴 **YAZMA:** Vulnerable kod snippet  
🔴 **YAZMA:** PoC exploit  

---

## Sonraki Adımlar

1. **MEDIUM fix'ler:** strcpy, Sleep, memory leak (28 is, KODCU)
2. **LOW cleanup:** Dead code removal, enum consistency (28 is, RUSTIK)
3. **Açık exploit'ler:** Anvil rate, TLS (WEBRA + KODCU koordinasyon)
4. **Audit tekrar:** 6 ay sonra CRITICAL + HIGH re-assessment

---

## Referanslar

- F:\MDBACKUP\C--Projects_memory\audit\myko_audit_report.md — Tam audit
- F:\MDBACKUP\C--Projects_memory\pearl_guard\myko_packet_shift_fixes.md — K1-K10
- F:\MDBACKUP\C--Projects_memory\pearl_guard\pearl_guard_port.md — Anti-cheat

