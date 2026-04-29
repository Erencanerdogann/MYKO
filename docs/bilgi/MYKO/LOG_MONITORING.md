# 📊 LOG / MONITORING — Sunucu İzleme Rehberi

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `C:\Users\erenc\Desktop\Server\Logs\`, `GameServerDlg.cpp`, `User.cpp`, server log dosyaları
**Hedef:** Hangi log neyi söyler, lansmanda ne izlenir, crash recovery.

---

## 1. LOG KLASÖR YAPISI

### Lokal Test
```
C:\Users\erenc\Desktop\Server\Logs\
├── GENERAL_YYYY-MM-DD.log        ← Genel server log
├── DISCONNECT_YYYY-MM-DD.log     ← DC eventleri
├── GM_YYYY-MM-DD.log              ← GM komut audit (KRİTİK)
├── HACK_YYYY-MM-DD.log            ← Anti-cheat tetiklemeleri
├── LOGIN_YYYY-MM-DD.log           ← Giriş/çıkış
├── MERCHANT_YYYY-MM-DD.log        ← Pazar (offline merchant) işlemler
├── LOGIN_STARTUP_DEBUG.txt        ← Boot sırası debug
├── GameServer.log                 ← Ana server log (rotating)
├── LoginServer.log                ← Login server ana log
└── Login_<DD>_<MM>.log            ← Günlük login detay
```

### Production (`104.238.23.99`)
- Aynı yapı: `C:\Users\Administrator\Desktop\Server\Logs\`
- DB log: `KO_LOG` veritabanı (ek tablolar)
- Web log: nginx access/error (`C:\koweb2\logs\` veya nssm)

---

## 2. LOG TİPLERİ — NE NE GÖSTERİR

### A) GENERAL_*.log
- Server start/stop
- Zone load
- Tablo reload (`+reload*` sonrası)
- Genel info/warning
- **Format:** `[YYYY-MM-DD HH:MM:SS] [LEVEL] mesaj`

**İzlenecek:**
- `[ERROR]` veya `[FATAL]`
- `Failed to load`
- `Cannot connect to DB`
- `Memory allocation failed`

### B) DISCONNECT_*.log
- Oyuncu DC olduğu zaman
- DC sebebi: timeout, crash, manual quit, kick
- **Format:** `[saat] Nick=X AccID=Y Reason=Z IP=...`

**İzlenecek:**
- Kümeli DC (network sorun)
- Spesifik bir oyuncu sürekli DC (hile şüphe)
- "Crash" reason → CHIP'a ilet

### C) GM_*.log (KRİTİK — AUDIT)
- Tüm `+komut` çağrıları
- Hangi GM, ne yaptı, kime, ne zaman
- **Format:** `[saat] GM=Nick command=+give target=Test args=...`

**İzlenecek:**
- Yetkisiz GM komut (Authority hatası)
- Aşırı `+give` (hediye spam → ekonomi)
- `+block` / `+ipban` audit
- `+changegm` (kalıcı yetki değişimi)

### D) HACK_*.log
- Pearl Guard / XGuard tetiklemesi
- Speed hack, wall hack, packet replay
- **Format:** `[saat] Nick=X HackType=Y Detail=Z`

**İzlenecek (LANSMAN KRİTİK):**
- Wall cheat (`UserWallCheatCheckRegion` aktif olduktan sonra)
- Speed hack
- Memory edit (Cheat Engine vs.)
- Packet flood

### E) LOGIN_*.log
- Login/logout zaman damgası
- Hesap, IP, sonuç (success/fail)
- **Format:** `[saat] Login Account=X IP=Y Result=Z`

**İzlenecek:**
- Brute force (aynı IP'den 100 fail)
- Account stuffing (sözlük saldırısı)
- IP coğrafi anomali

### F) MERCHANT_*.log
- Offline merchant satış/alış
- Item dupe / fiyat manipülasyon tespit

### G) LOGIN_STARTUP_DEBUG.txt
- Server boot sırası
- DSN bağlantı doğrulama
- Tablo yükleme sıra
- ⚠️ **Boot sorunu** → ilk bakılacak dosya

---

## 3. KO_LOG VERİTABANI (DB Logları)

### Tablolar (referans, doğrula)
| Tablo | İçerik |
|-------|--------|
| `LOG_GMCOMMAND` | GM komut audit (DB kopyası) |
| `LOG_USER_ACTION` | Oyuncu eylem (item drop, trade, kill) |
| `LOG_LOGIN` | Login/logout |
| `LOG_BAN` | Ban/unban geçmiş |
| `LOG_PUS` | Cash shop satın alma |
| `LOG_TRADE` | Oyuncu-oyuncu trade |
| `LOG_CHAT` | Chat (küfür filtresi audit) |
| `LOG_DUPE` | Item dupe tespit |
| `LOG_KILL` | PvP kill |

### Sorgu Örnekleri
```sql
-- Son 1 saat GM komutları
SELECT * FROM LOG_GMCOMMAND WHERE log_time > DATEADD(HH,-1,GETDATE());

-- Son 24h ban
SELECT * FROM LOG_BAN WHERE log_time > DATEADD(DD,-1,GETDATE());

-- En çok satılan PUS item
SELECT itemid, COUNT(*) FROM LOG_PUS
WHERE log_time > '2026-05-08'
GROUP BY itemid ORDER BY 2 DESC;
```

---

## 4. WEB LOG (nginx + PHP + Rust API)

### nginx access log
```
C:\koweb2\logs\access.log
Format: IP - - [tarih] "GET /api/site/... HTTP/1.1" status size
```

**İzlenecek:**
- 4xx (404, 401, 403) → bozuk istekler
- 5xx → backend hata
- Aşırı istek (DDoS şüphe)

### nginx error log
```
C:\koweb2\logs\error.log
```
- PHP fatal error
- Backend bağlantı timeout
- Permission hata

### Rust API (orkestra-server.exe :3001)
- stdout/stderr → komuta paneli veya `C:\orkestra\logs\`
- JSON yapılı log (tracing crate)

**İzlenecek:**
- Rate limit hit (3/dk register, 10/dk login)
- Token validation fail
- DB query timeout

---

## 5. LANSMAN GÜNÜ MONITORING

### Real-time İzleme (her 5dk)
```bash
# Online sayım
+count   (in-game)

# Crash kontrol
ls Logs\ | grep -i crash | tail -5

# Disk yer
df -h C:\

# DB connection
sqlcmd -Q "SELECT @@CONNECTIONS"

# Aktif oturum
sqlcmd -Q "SELECT COUNT(*) FROM USERDATA WHERE OnLine=1"

# CPU/RAM
tasklist | grep -i -E "GameServer|LoginServer|sqlservr"
```

### Discord Webhook (önerilen, henüz kurulmadı)
- Online sayım → 10dk
- Crash → anında
- Hile uyarı → anında
- DB error → anında

### Grafana / Prometheus (uzun vade)
- Metric export: orkestra-server `/metrics` endpoint?
- DB query süre histogram
- Online sayı time-series
- Crash sayım

---

## 6. CRASH ANALİZİ

### Belirti
- `GameServer.exe` process aniden gitti
- Oyuncuların hepsi DC oldu
- `Logs\GENERAL_*.log` son satır crash öncesi

### İlk Adımlar
1. **Process check:** `tasklist | grep GameServer`
2. **Last log:** `tail -100 Logs\GENERAL_<bugün>.log`
3. **BugTrap dump:** `C:\Users\Administrator\Desktop\Server\BugTrap-x64.dll` ürettiği dump dosyası
4. **DB lock:** `sp_who2` blocking SPID
5. **Memory:** RAM tükendi mi (`tasklist /v`)

### Crash Dump Analiz
```
BugTrap → .dmp dosyası → WinDbg / VS 2022 ile aç
   → call stack incele
   → CHIP'e ilet (C++ debug)
```

### Restart Prosedürü
```bash
# 1. Process kalıntı temizle
taskkill /IM GameServer.exe /F

# 2. Crash log yedekle
move Logs\GENERAL_*.log Logs\crashed\

# 3. Restart
wmic process call create "C:\Users\Administrator\Desktop\Server\GameServer.exe","C:\Users\Administrator\Desktop\Server"

# 4. Boot kontrol
tail -f Logs\LOGIN_STARTUP_DEBUG.txt

# 5. Online doğrula
+count
```

⚠️ **3 crash arka arkaya** → DB veya disk problemi, derin inceleme şart.

---

## 7. GÜNLÜK MONITORING RUTINI

### Sabah (08:00)
- [ ] Server uptime kontrol (`+count` SSH)
- [ ] Disk yer (>10 GB free)
- [ ] DB backup başarılı mı (`F:\MYKOBACKUP\`)
- [ ] Crash log var mı? (`Logs\crashed\`)
- [ ] Online peak gece? (LOG_LOGIN sorgusu)

### Öğle (13:00)
- [ ] Web register sayısı
- [ ] PUS satış sayısı
- [ ] HACK_*.log incele

### Akşam (19:00) — Pre-event
- [ ] Online hazır mı?
- [ ] Event saati doğru mu? (`SELECT * FROM EVENT_SCHEDULE`)
- [ ] GM hazır mı?
- [ ] Discord duyuru atıldı mı?

### Gece (24:00) — Pre-rotate
- [ ] Log rotation hazır mı (eski log F:\)
- [ ] DB backup planlı (saat 03:00 vb.)
- [ ] Yarın için event takvimi onaylandı mı?

---

## 8. ALERT EŞİKLERİ

| Metrik | Sarı | Kırmızı |
|--------|------|---------|
| Online sayı | <10 | <5 (fakat lansman üstü) |
| RAM kullanım | %75 | %90 |
| CPU | %70 | %85 |
| Disk free | <30 GB | <10 GB |
| DB connection | <50 | <10 |
| Crash/saat | 1 | 3+ |
| HACK log/saat | 5 | 20+ |
| 5xx web error | 10/dk | 50/dk |
| Login fail rate | %20 | %50 |

---

## 9. LOG RETENTION (Saklama)

| Log tipi | Süre | Yer |
|----------|------|-----|
| GENERAL | 30 gün lokal, 1 yıl F: | `F:\MDBACKUP\Logs\` |
| GM | KALICI (audit) | `F:\MDBACKUP\Logs\GM\` |
| HACK | KALICI (delil) | `F:\MDBACKUP\Logs\HACK\` |
| LOGIN | 90 gün | sonra siler |
| DISCONNECT | 30 gün | rotation |
| MERCHANT | 90 gün | dupe takip |
| Web access | 30 gün | nginx default |

⚠️ **GDPR/KVKK** — IP + hesap log şifre olmamalı, kişisel veri minimum.

---

## 10. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **Log dosyası büyür** — disk dolması = server crash |
| 2 | **GM_*.log audit silinmesin** — yasal/güvenlik kanıt |
| 3 | **HACK log → ban delili** — kanıtsız ban yasak |
| 4 | **Crash dump GitHub'a koyma** — hassas bilgi içerebilir |
| 5 | **DB log table büyük** → indexli tut, ay sonu archive |
| 6 | **Lansman 24h log boyutu** — ~5 GB beklenebilir, F: hazır |
| 7 | **Log timezone** — UTC mu, UTC+3 mü? Tutarlı olsun |
| 8 | **Sensitive data** — şifre/cash log'a yazılmasın |

---

## 11. KAYNAK REFERANSLAR

- **Lokal Logs:** `C:\Users\erenc\Desktop\Server\Logs\`
- **Production Logs:** `Administrator@104.238.23.99:Desktop\Server\Logs\`
- **DB log:** `KO_LOG` veritabanı (DSN: KO_LOG)
- **Server kod log:** `GameServerDlg.cpp`, `User.cpp`, `MagicInstance.cpp`
- **BugTrap:** `Server\BugTrap-x64.dll` (crash dump)
- **Web log:** `C:\koweb2\logs\`
- **GM komut:** `GM_KOMUT.md` (audit referansı)
- **Anti-cheat:** `ANTI_CHEAT.md`, `GUVENLIK_BUG.md`

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Discord webhook + Grafana entegrasyonu eklendiğinde güncelle. Lansman sonrası gerçek log yoğunluğu ile alert eşikleri kalibre edilecek.
