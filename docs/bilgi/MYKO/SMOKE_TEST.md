# 🚬 SMOKE TEST — Lansman Öncesi Sistem Doğrulama

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Hedef:** Lansman öncesi 1 hesapla baştan sona oyun döngüsü test, kritik akışlar.

---

## 1. SMOKE TEST NEDIR

**Tanım:** "Sistem ayağa kalkıyor mu?" hızlı doğrulama. 10-15 dakikada kritik akış test edilir, BLOCKER varsa lansman ertelenir.

**Felsefe:** "Her detayı test etme — sadece **çıt çıkmadan çalışıyor mu** doğrula."

---

## 2. PREP (Test Öncesi)

### Test Hesabı Hazırlığı
```sql
-- Test web hesabı
INSERT INTO TB_USER (strAccountID, strPasswd, strWebHash, ...)
VALUES ('SMOKE01', 'SmokeTest123', 'hash...', ...);

-- Test in-game karakter (boş, char create test edecek)
-- USERDATA boş, char select ekranında 0 karakter
```

### GM Hesabı Hazır
- Authority=0 + GAME_MASTER_SETTINGS tablosunda kayıt
- Test sırasında müdahale lazım olursa

### Client Kurulumu
- Temiz `C:\MalaysiaKO\` (cache yok)
- `Server.ini` → IP=104.238.23.99 (production) veya lokal

---

## 3. SMOKE TEST AKIŞI (15dk)

### ✅ AŞAMA 1: WEB (3dk)

#### A) Site açılıyor mu?
- [ ] http://malasiako.com (200 OK)
- [ ] HTTPS yönlendirmesi (TLS aktif olduğunda)
- [ ] Ana sayfa içerik yükleniyor
- [ ] Online sayım gösterge çalışıyor (sıfır da olabilir)
- [ ] Server status "ONLINE"

#### B) Register
- [ ] Register formu görünür
- [ ] Test hesap oluştur: `SMOKE01` / `SmokeTest123` / e-posta
- [ ] CAPTCHA çalışıyor (varsa)
- [ ] Rate limit (3/dk) aktif
- [ ] DB'de TB_USER satırı oluştu mu?
- [ ] **strWebHash NOT NULL** mi? (lansman blocker bug)

#### C) Login (Site)
- [ ] Test hesabı ile login
- [ ] Token döner
- [ ] Profil sayfası açılır
- [ ] Cash bakiyesi görünür (0 olmalı yeni hesap)

#### D) Forum
- [ ] Forum URL açılıyor (`/forum`)
- [ ] Register yapılabiliyor
- [ ] Sticky/duyuru görünür

---

### ✅ AŞAMA 2: PATCH (2dk)

- [ ] http://104.238.23.99:80/patch/version.txt yanıt veriyor
- [ ] DB `VERSION` tablosu güncel
- [ ] Test client → Launcher → patch indir → tamamlanır
- [ ] `Server.ini Files` güncellendi mi?

---

### ✅ AŞAMA 3: GAME LOGIN (3dk)

- [ ] `KnightOnline.exe` çalıştır (admin)
- [ ] Login ekranı açılıyor
- [ ] Code Guard çalışıyor (`cg_crash.log` oluşmuyor)
- [ ] Hesap: `SMOKE01` / `SmokeTest123` → giriş başarılı
- [ ] Karakter seçim ekranı (boş, 0 karakter)

---

### ✅ AŞAMA 4: CHARACTER CREATION (2dk)

- [ ] "Yeni karakter" butonu çalışıyor
- [ ] Sınıf seçimi: Warrior / Rogue / Mage / Priest
- [ ] İrk seçimi: Karus / El Morad
- [ ] İsim girilebiliyor (Türkçe/karakter limit)
- [ ] Karakter yaratılır → DB USERDATA + USER_ITEM (varsayılan item)
- [ ] Karakter listesi 1'e çıktı

---

### ✅ AŞAMA 5: GAME ENTRY (1dk)

- [ ] Karakter seç → "Giriş"
- [ ] Loading ekranı
- [ ] Spawn zone (Karus/Elmorad başlangıç bölgesi)
- [ ] Karakter görünüyor (model)
- [ ] Çevre yükleniyor (NPC, harita)
- [ ] FPS makul (30+)

---

### ✅ AŞAMA 6: BASIC GAMEPLAY (3dk)

#### A) Hareket
- [ ] WASD veya tıklama → karakter hareket eder
- [ ] Koşma (Shift)
- [ ] Zıplama (Space)
- [ ] Wall cheat **AKTIF** (engele rağmen geçilemez — `CMH.cpp:264` doğrula)

#### B) Skill
- [ ] Default skill 1-2 kullanılabilir
- [ ] Skill animasyonu çalışıyor
- [ ] MP düşüyor
- [ ] Cooldown aktif

#### C) Mob Saldırı
- [ ] Yakındaki mob target alınabilir
- [ ] Saldırı (sol klik) → hasar verilir
- [ ] Mob ölünce drop düşer (item / noah)
- [ ] EXP artışı

#### D) Inventory
- [ ] "I" tuşu envanteri açar
- [ ] Drop alınan item görünür
- [ ] Item bilgi (sağ tık) görünür
- [ ] Item kullanılabilir (örn potion)

#### E) Trade Test
- [ ] Başka karaktere yakın yaklaş
- [ ] Trade istek
- [ ] Item swap → DB transaction → senkron
- [ ] LOG_TRADE'de kayıt

#### F) Chat
- [ ] Genel chat mesaj gönder
- [ ] Görünüyor mu?
- [ ] Küfür filtresi tetikleniyor mu? (test küfür kelime)
- [ ] PM (özel mesaj) çalışıyor mu?

---

### ✅ AŞAMA 7: PUS / CASH SHOP (1dk)

- [ ] F10 (Mall) açılır mı?
- [ ] Kategoriler görünüyor (PUS_CATEGORY)
- [ ] Item listesi yükleniyor (PUS_ITEMS)
- [ ] Test alım yapılabiliyor (cash bakiye yetersizse "yetersiz" mesajı)
- [ ] Kapatma çalışıyor

---

### ✅ AŞAMA 8: GM KOMUT (1dk)

GM hesabıyla:
- [ ] `+count` → online sayım gösterir
- [ ] `+permanent "Test"` → üst bar değişir
- [ ] `+offpermanent` → reset
- [ ] `+notice "Smoke test"` → tüm chat görür
- [ ] `+give SMOKE01 379154000 1 1` → test hesabına item gider

---

### ✅ AŞAMA 9: LOGOUT (1dk)

- [ ] ESC → menü → "Çıkış"
- [ ] Karakter kaydedilir (DB USERDATA SET)
- [ ] DC olur
- [ ] LOGIN_*.log'da çıkış kaydı
- [ ] Tekrar login → karakter aynı yerde, item korundu

---

## 4. SONUÇ DEĞERLENDİRME

### TÜM YEŞİL (✅)
→ **Lansman GO** — sistem genel çalışıyor.

### KIRMIZI (❌) Sayısı
| Hata sayısı | Karar |
|-------------|-------|
| 0 | LANSMAN GO |
| 1-2 (kritik değil) | Hot fix + lansman GO |
| 1+ kritik (BLOCKER) | LANSMAN ERTELE / fix bekle |

### Kritik Sayılan
- ❌ Web register/login çalışmıyor
- ❌ Game login çalışmıyor
- ❌ Karakter create yapılamıyor
- ❌ Item drop/pickup çalışmıyor
- ❌ DB lock / corruption
- ❌ Crash (in-game)
- ❌ Wall cheat **KAPALI** (lansman blocker)
- ❌ strWebHash NULL bug (auto-register giremiyor)

### Kritik Olmayan
- 🟡 FPS düşük (oyuncu donanım sorunu olabilir)
- 🟡 Forum eklenti eksik
- 🟡 TLS yok (uzun vade)
- 🟡 Captcha yok
- 🟡 Bazı NPC dialog Türkçe değil

---

## 5. STRESS TEST (Opsiyonel)

### Load Test
- 200 bot ile login (+user_bots)
- Online tutuluyor mu?
- DB query süre artıyor mu?
- RAM/CPU sürdürülebilir mi?

### Event Test
- `+csw` → Delos kale açılıyor mu?
- 50+ oyuncu ile event başlat → server crash yok mu?

### DB Test
- 1000 trade simülasyonu
- 100 character create paralel
- Item dupe deneme (anti-cheat tetikleniyor mu?)

---

## 6. AUTOMATION (Uzun Vade)

### Kontrol
- Selenium / Playwright (web)
- Custom KO bot (in-game smoke bot)
- DB query checks (script)
- Discord report (yeşil/kırmızı)

⚠️ S88'de manuel — Otomasyon S89+ planında.

---

## 7. SMOKE TEST RAPORU TEMPLATE

```markdown
# SMOKE TEST RAPORU — <TARIH SAAT>

**Tester:** DOKTOR (veya CHIP+KODCU)
**Süre:** 14 dakika
**Sonuç:** ✅ GO / ❌ BLOCKER

## Aşama Sonuçları
| Aşama | Sonuç | Not |
|-------|-------|-----|
| 1 Web | ✅ | Register OK |
| 2 Patch | ✅ | 2378 indi |
| 3 Login | ✅ | |
| 4 Char create | ✅ | |
| 5 Entry | ✅ | Spawn Karus |
| 6 Gameplay | 🟡 | Wall cheat passive — fix bekleniyor |
| 7 PUS | ✅ | Mall açılıyor |
| 8 GM | ✅ | Komutlar çalışıyor |
| 9 Logout | ✅ | Senkron OK |

## Bulgular
- Wall cheat KAPALI doğrulandı (`CMH.cpp:264`) — CHIP fix bekleniyor
- TLS yok — post-launch

## Karar
- Wall cheat fix sonrası LANSMAN GO
```

---

## 8. LANSMAN ÖNCESI ÇALIŞTIR (T-24 ve T-2)

### T-24 saat (07 Mayıs Perşembe)
- Tam smoke test (15dk)
- Bulguları DOKTOR'a rapor
- Hot fix listesi → CHIP/MATRIX/WEBRA

### T-2 saat (08 Mayıs 16:00)
- Hızlı smoke (Web register + game login + char create)
- 5 dakika quick check
- ✅ ✅ ✅ ise GO

---

## 9. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **Test hesabını lansmandan SİL** — gerçek oyuncu kullansın diye |
| 2 | **GM `+give` sonrası item geri alınmaz** — test hesabıyla yap |
| 3 | **Stress test = production sunucuda yapma** — lokal test server şart |
| 4 | **Smoke test sırası önemli** — web → patch → login → game (sıraya göre) |
| 5 | **Bot test (`+user_bots`)** lansmanda KAPATILSIN (gerçek online görünsün) |
| 6 | **Manuel smoke 15dk yeterli** — derin test post-launch |
| 7 | **Discord/forum log akıyor mu** smoke esnasında — alert test |
| 8 | **Wall cheat gerçekten aktif mi** doğrula — `+gm` ile invisible test |

---

## 10. KAYNAK REFERANSLAR

- **Web:** `WEB_PHP.md`, `WEB_API.md`
- **Game login:** `SRC_HARITA.md`, `BUILD.md § Patlama dersleri`
- **GM:** `GM_KOMUT.md`
- **Cash:** `CASH_SHOP_PUS.md`
- **Drop:** `NPC_DROP_LOOT.md`
- **Anti-cheat:** `ANTI_CHEAT.md`
- **Lansman:** `LANSMAN_CHECKLIST.md`
- **Crash:** `CRASH_RECOVERY.md`
- **Log:** `LOG_MONITORING.md`

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Lansman 1 hafta sonrası gerçek deneyime göre aşama eklenecek/çıkarılacak. Otomasyon (Selenium/script) S89+ plan.
