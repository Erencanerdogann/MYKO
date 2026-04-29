# 🚀 LANSMAN CHECKLIST — Bynoisee MalaysiaKO Valor

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Lansman:** **08 Mayıs 2026 (Cuma)** — 8 GÜN KALDI
**Hedef:** Tek dosya tam çalıştırılır kontrol listesi.

---

## 🔴 BLOCKER (Lansman İPTAL nedenleri)

| # | Konu | Sorumlu | Durum |
|---|------|---------|-------|
| 1 | **Wall cheat detection KAPALI** (`CharacterMovementHandler.cpp:264`) — yorum satırı kalkmalı | CHIP | ❌ |
| 2 | **strWebHash NULL** — auto-register hesap siteye giremez | WEBRA | ❌ |
| 3 | **TS Item 381001000** — Duration=1, SellPrice=0 hatalı (Transformation Scroll) | MATRIX | ❌ |
| 4 | **Smoke test** (login → char create → walk → skill → trade → logout) | CHIP+KODCU | ❌ |

---

## 🟡 YÜKSEK ÖNCELİK (Lansmadan 1 hafta önce)

| # | Konu | Sorumlu |
|---|------|---------|
| 5 | TLS / HTTPS web tarafı (Let's Encrypt) | WEBRA |
| 6 | Forum eklenti (kayıt + SSO) | WEBRA |
| 7 | Production `EVENT_SCHEDULE` doğrulama (`SELECT *`) | MATRIX |
| 8 | EventAwards.ini item ID'leri DB'de var mı? | MATRIX |
| 9 | `Notice.txt` lansman duyurusu hazır | DOKTOR |
| 10 | `code.guard` deploy edildi mi? (Pearl Guard) | CHIP+GHOST |
| 11 | Patch system çalışıyor mu? (port 80 + DB VERSION) | KODCU |
| 12 | TBL_HASH validation OK (`Logs\boot.log`) | MATRIX |
| 13 | DB backup otomasyon (saatlik) | MATRIX |
| 14 | Production sunucu lisans (Windows Server EVALUATION) | DOKTOR/Patron |

---

## 🟢 ORTA (Lansman gününe kadar)

| # | Konu | Sorumlu |
|---|------|---------|
| 15 | Site banner/SEO/analytics | WEBRA |
| 16 | Captcha register formu | WEBRA |
| 17 | Password recovery | WEBRA |
| 18 | Anti-bot rate limit | WEBRA+GHOST |
| 19 | Discord webhook (online sayım) | DOKTOR |
| 20 | Player support kanalı | DOKTOR |

---

## 🔵 DÜŞÜK (Post-launch)

| # | Konu |
|---|------|
| 21 | Moradon M-key harita bug |
| 22 | Player ticket sistemi |
| 23 | Ranking sayfa cache |
| 24 | Admin dashboard |

---

## 📋 LANSMAN GÜNÜ ADIM ADIM (08 Mayıs 2026 — Cuma)

### T-24 SAAT (07 Mayıs Perşembe)
- [ ] **DB Full Backup:** `BACKUP DATABASE KO_MYKO TO DISK = ...`
- [ ] **Server snapshot:** GameServer.exe + LogInServer.exe + INI yedek
- [ ] **Web snapshot:** koweb2 tar.gz
- [ ] **Git tag:** `LAUNCH_T-24` repo'ya
- [ ] **F: yedek:** `F:\MYKOBACKUP\LAUNCH_T-24_*.rar`
- [ ] **Patch testi:** Test client patch alabiliyor mu?
- [ ] **TLS sertifika:** Aktif mi?
- [ ] **Forum:** Erişilebilir mi?
- [ ] **DNS:** malasiako.com → 104.238.23.99 doğru mu?
- [ ] **Notice.txt:** Lansman mesajı yazılı mı?
- [ ] **EVENT_SCHEDULE:** Cuma+Cumartesi+Pazar saatleri dolu mu?

### T-12 SAAT (08 Mayıs sabah 06:00)
- [ ] **Sunucu uptime:** `+count` (test bağlantı)
- [ ] **Log temiz:** Logs\ boş veya rotated
- [ ] **Disk yer:** Production C:\ minimum 20 GB free
- [ ] **MSSQL:** `sp_who2` — kilit yok mu?
- [ ] **Pearl Guard:** `code.guard` aktif (`grep XSafe_ACTIVE`)
- [ ] **Smoke test:** Test hesabıyla login → char create → walk

### T-2 SAAT (08 Mayıs 16:00)
- [ ] **GM hesapları hazır:** Authority=0 + GAME_MASTER_SETTINGS
- [ ] **+permanent ayarlandı**
- [ ] **Discord/sosyal medya** lansman tweet hazır
- [ ] **Patch versiyon:** Final patch zip yüklü, DB VERSION INSERT
- [ ] **Server.ini Version:** Client ile uyumlu (2369 + 1098)

### T-0 (08 Mayıs 18:00 — AÇILIŞ)
- [ ] **`+noticeall "Sunucu acildi! Iyi oyunlar."`**
- [ ] **`+permanent "Bynoisee MalaysiaKO Valor — HOSGELDINIZ"`**
- [ ] **Online sayım takibi:** `+count` her 10dk
- [ ] **CSW 19:00** (`+csw`)
- [ ] **Chat moderasyonu:** Spam ban (`+block`)
- [ ] **Crash log:** `Logs\crash_*.log` izle
- [ ] **DB transaction:** Yavaş query var mı (`sp_who2 active`)

### T+12 SAAT (Lansman ertesi sabah)
- [ ] **DB backup** (saatlik otomasyon doğrula)
- [ ] **Bug raporu:** Forum + chat'ten gelen şikayet derle
- [ ] **Online peak:** Maksimum eşzamanlı sayı
- [ ] **Crash sayım:** Logs\ analiz
- [ ] **Web register adedi:** SELECT COUNT(*) FROM TB_USER WHERE created > '2026-05-08'
- [ ] **Hot fix listesi:** Acil düzeltilecekler

---

## 🛡️ GÜVENLİK KONTROL (Lansman Öncesi)

| # | Konu | Komut/Yöntem |
|---|------|--------------|
| 1 | Wall cheat aktif | `grep -n UserWallCheatCheckRegion CharacterMovementHandler.cpp` (yorum satırı YOK olmalı) |
| 2 | Pearl Guard | `code.guard` dosyası mevcut, `XSafe_ACTIVE 1` |
| 3 | SQL injection | Banka/web/login formları sql parametre kullanıyor mu? |
| 4 | Buffer overflow | Memo defter audit (98 bulgu / 31 fix yapılmış, kalan 67) |
| 5 | Brute force | Login deneme rate limit |
| 6 | DDoS | CDN/firewall (production sunucu) |
| 7 | Packet shift | K1-K10 fix tamam (memory) |
| 8 | TLS | HTTPS aktif mi? |
| 9 | Audit log | GM komut kullanım log'u |
| 10 | Backup encrypted | `F:\MYKOBACKUP\` AES |

---

## 📊 OYUN İÇERİK KONTROL

| # | Alan | Doğrulama |
|---|------|-----------|
| 1 | **Item** | `SELECT COUNT(*) FROM ITEM` (beklenen: 5000+) |
| 2 | **Skill** | `SELECT COUNT(*) FROM MAGIC_TABLE` |
| 3 | **NPC spawn** | Her zone NPC sayısı `+countzone <ID>` |
| 4 | **Quest** | 510 lua dosyası syntax check |
| 5 | **Drop table** | Boss drop testi `+drop 100` |
| 6 | **Cash shop** | `SELECT COUNT(*) FROM PUS_ITEMS` |
| 7 | **Anvil** | +1...+8 test (Charon NPC) |
| 8 | **Class change** | Lvl 60 master quest test |
| 9 | **CSW** | Delos kale grade 5+ klan var mı? |
| 10 | **Premium** | PUS purchase → in-game item akışı |

---

## 💰 EKONOMI KONTROL

- [ ] Item BuyPrice > SellPrice (her item)
- [ ] Anvil scroll fiyat balanced
- [ ] Premium fiyat tutarlı (PUS_ITEMS)
- [ ] Trade limit (oyuncu-oyuncu)
- [ ] Offline merchant düzgün çalışıyor
- [ ] Noah cap (max para tutma) ayarlandı
- [ ] GB (Gold Bar) drop oranı kontrol

---

## 🌐 WEB / FORUM KONTROL

- [ ] Register form çalışıyor (rate 3/dk)
- [ ] Login form çalışıyor (rate 10/dk)
- [ ] Auto-register hesap **siteye girebiliyor** (strWebHash bug fix)
- [ ] Online sayım sayfası
- [ ] Ranking sayfa
- [ ] Server status (up/down)
- [ ] Forum erişilebilir
- [ ] Forum register
- [ ] Patch indirme (port 80)
- [ ] CORS doğru (PHP ↔ Rust API)

---

## 🚨 ACİL DURUM PROTOKOLÜ

### Sunucu çöker
1. `taskkill /IM GameServer.exe /F` + restart
2. `wmic process call create "C:\Users\Administrator\Desktop\Server\GameServer.exe", ...`
3. **Reboot sonrası SSH BEKLE** — task "completed" ≠ sunucu hazır
4. `+count` kontrol bağlantı

### DB lock
1. `sp_who2 active` → blocking SPID bul
2. `KILL <SPID>` (dikkat — uncommit veri)
3. QUOTED_IDENTIFIER bug için `DB_STORED_PROC.md § QI Bug` referans

### Toplu hile/exploit
1. `+aireset` (AI reset)
2. `+block <Nick>` (ban)
3. `+ipban <Nick> 0 hile` (IP ban)
4. Logs\ analiz, sonra hot fix

### Patch bozuk
1. `+down 5` (5dk sonra kapat)
2. DB `VERSION` tablosu eski versiyona REVERT
3. Patch zip eski sürüm yükle
4. `+noticeall "Hatali patch geri alindi"`

---

## 📞 İLETİŞİM ZİNCİRİ

| Sorun | İlk durak | İkinci |
|-------|-----------|--------|
| Game server crash | CHIP | DOKTOR → Patron |
| DB sorunu | MATRIX | DOKTOR |
| Web/Forum | WEBRA | DOKTOR |
| Patch | KODCU | DOKTOR |
| Hile/exploit | GHOST | CHIP+DOKTOR |
| Genel | DOKTOR | Patron (Erencan) |
| **Sunucu erişim YASAK** | GHOST, JERRY | DOKTOR-Patron onay |

---

## 🎯 BAŞARI METRİKLERİ (Lansman 24h)

- [ ] **Online peak ≥ 200** (hedef başarı)
- [ ] **Crash sayısı = 0** (kritik hata yok)
- [ ] **Web register ≥ 100** (siteye trafik)
- [ ] **Discord/forum aktivitesi**
- [ ] **Hile şikayeti < 5** (anti-cheat çalışıyor)
- [ ] **DB boyutu büyüme:** sağlıklı log (anormal değil)

---

## 11. KAYNAK REFERANSLAR

- `GM_KOMUT.md` — GM müdahale komutları
- `EVENT_TAKVIMI.md` — Event saatleri
- `WEB_BUG.md` — Web kritik buglar
- `GUVENLIK_BUG.md` — 98+31 fix listesi
- `SUNUCU_DOSYA_YOLLARI.md` — Tüm yollar
- `PATCH_SURECI.md` — Patch deploy
- `BUILD.md` — Build + deploy

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Her T-X check sonrası güncelle, gerçek tamamlanan/kalan işle.
**KRİTİK:** Lansman 8 gün — 4 BLOCKER + 10 yüksek öncelik **bugün başlamalı**.
