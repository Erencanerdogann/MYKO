# 🔥 CRASH RECOVERY — Server Çökmesi Sonrası Geri Dönüş

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `BugTrap-x64.dll`, `Logs\GENERAL_*`, `LOGIN_STARTUP_DEBUG.txt`, server kod
**Hedef:** Server çöktüğünde ne yapılır, sebep nasıl bulunur, prevention.

---

## 1. CRASH TIPLERI

### A) Hard Crash (Process kaybı)
- `GameServer.exe` veya `LogInServer.exe` aniden gitti
- BugTrap dump üretebilir (`.dmp` dosyası)
- Tüm oyuncular DC olur
- **En yaygın:** Memory corruption, null pointer, stack overflow

### B) Soft Crash (Hang / Freeze)
- Process çalışıyor ama yanıt vermiyor
- Oyuncular DC olur ama process tasklist'te
- **Sebep:** Deadlock, infinite loop, DB query timeout

### C) DB Crash
- MSSQL servis durdu (`MSSQLSERVER01`)
- GameServer hata mesajı verir, oyuncular kayıt yapamaz
- Genelde ana process çalışıyor ama "DB connection lost"

### D) Cascade Crash
- DB kapandı → GameServer hata → LoginServer dolaylı etki
- Veya GameServer crash → DB veri kayıp transaction

---

## 2. CRASH TESPİT (5dk içinde)

### Process Check
```bash
# Production SSH
tasklist | grep -i -E "GameServer|LogInServer|sqlservr"

# Beklenen:
# GameServer.exe       PID  ...
# LogInServer.exe      PID  ...
# sqlservr.exe         PID  ...   (MSSQL ana servis)
```

### Online Sayım
```
+count       # 0 ise sorun var (önceden 100 idiyse)
```

### Log Tail
```bash
# Son 50 satır GENERAL log
tail -50 "C:\Users\Administrator\Desktop\Server\Logs\GENERAL_$(date +%Y-%m-%d).log"

# Crash satırı bul
grep -i -E "crash|fatal|exception|error" Logs\GENERAL_*.log | tail -10
```

### Discord Webhook (planlanan)
- Online sayı 0 düşerse alert
- Process kaybolursa alert
- Disk doldursa alert

---

## 3. CRASH ANALİZ ADIMLARI

### Adım 1: Son Logları Topla
```
1. C:\Users\Administrator\Desktop\Server\Logs\GENERAL_<bugün>.log son 200 satır
2. C:\Users\Administrator\Desktop\Server\Logs\LOGIN_STARTUP_DEBUG.txt
3. BugTrap-x64.dll → .dmp dosyası (varsa, son üretilen)
4. cg_crash.log (anti-cheat tarafı)
5. Windows Event Viewer → Application log (Error/Critical)
```

### Adım 2: Crash Pattern
| İz | Olası sebep |
|----|-------------|
| `Access Violation 0xC0000005` | Null pointer / freed memory |
| `Stack Overflow 0xC00000FD` | Sonsuz recursion |
| `DB connection lost` | MSSQL down / network |
| `ODBC: KO_MAİN not found` | DSN Türkçe karakter (klasik bug) |
| `Cannot allocate memory` | RAM tükendi |
| `Unhandled exception in MagicInstance` | Skill logic |
| `Lua error in quest_xx.lua` | Quest script hata |

### Adım 3: BugTrap Dump Analiz
```
1. .dmp dosyasını WinDbg veya VS 2022 ile aç
2. !analyze -v
3. Call stack incele
4. CHIP'a ilet (C++ debug tarafı)
```

### Adım 4: DB Durum
```sql
-- MSSQL aktif mi?
SELECT @@SERVERNAME, @@VERSION;

-- Bekleyen transaction
DBCC OPENTRAN('KO_MYKO');

-- Lock kontrol
EXEC sp_who2 active;

-- Disk yer
EXEC xp_fixeddrives;
```

---

## 4. RESTART PROSEDÜRÜ

### A) Hızlı Restart (5 dakika)
```bash
# 1. Process kalıntı temizle
taskkill /IM GameServer.exe /F
taskkill /IM LogInServer.exe /F

# 2. Crash log yedekle
mkdir Logs\crashed 2>nul
move Logs\GENERAL_*.log Logs\crashed\
move Logs\crash*.log Logs\crashed\

# 3. LoginServer önce başlat
cd "C:\Users\Administrator\Desktop\Server"
start "" LogInServer.exe

# 4. 30 saniye bekle, sonra GameServer
timeout /t 30
start "" GameServer.exe

# 5. Boot kontrol
type Logs\LOGIN_STARTUP_DEBUG.txt | tail -30
```

⚠️ **Sıra önemli:** LoginServer ÖNCE — GameServer bağlantı kurmaya çalışır.

### B) Tam Restart (DB dahil — 15 dakika)
```bash
# 1. Tüm process kapat
taskkill /IM GameServer.exe /F
taskkill /IM LogInServer.exe /F

# 2. MSSQL restart
net stop MSSQLSERVER01 /Y
net start MSSQLSERVER01

# 3. DB sağlık kontrol
sqlcmd -S localhost\MSSQLSERVER01 -E -Q "SELECT @@VERSION"
sqlcmd -S localhost\MSSQLSERVER01 -E -d KO_MYKO -Q "SELECT COUNT(*) FROM USERDATA"

# 4. LoginServer + GameServer
start "" LogInServer.exe
timeout /t 30
start "" GameServer.exe

# 5. Boot doğrula
type Logs\LOGIN_STARTUP_DEBUG.txt
+count   # SSH üzerinden test bağlantı
```

### C) DB Restore Restart (DB corrupt — 1+ saat)
1. Tüm process kapat
2. DB single user mode → restore (`BACKUP_RESTORE.md § 3`)
3. Multi user mode'a al
4. Server restart
5. Online'lara duyuru: "Sistem geri geldi, oluşan kayıp ~1 saat"

---

## 5. KÖK SEBEP ANALIZI (Post-Mortem)

### Şablon
```markdown
# CRASH POST-MORTEM — <TARIH SAAT>

## Özet (1 paragraf)
- Saat: 19:35
- Tip: Hard crash GameServer.exe
- Süre: 8 dakika downtime
- Etki: ~150 oyuncu DC

## Belirti
- Online sayım 145 → 0 ani düşüş
- Discord alert tetiklendi
- Forum şikayet: 12 mesaj

## Kök Sebep
- Logs\GENERAL_*.log son satır: "Access Violation in MagicInstance::ExecuteSkill, skill_id=490077"
- skill_id=490077 → SnowWar event skill
- BugTrap dump: null pointer, m_pTarget freed
- Sebep: Snow event kapanırken target hala kullanımda

## Yapılan
- Restart 5dk içinde tamamlandı
- Hot fix: skill 490077 null check eklendi (CHIP)
- Test edildi
- Patch deploy: 2378 versiyon

## Önlem
- Event skill cleanup'a null check (kod review)
- BugTrap auto-upload setup
- Discord alert eşik 50→25 düşürüldü

## Sorumlu
- Tespit: DOKTOR
- Fix: CHIP
- Deploy: RUSTIK
```

---

## 6. PREVENTİV ÖNLEMLER

### A) Kod Tarafı
- Null check zorunlu (özellikle skill, item, target)
- Try-catch (C++ exception handling)
- Memory pool kullan (frequent alloc/free)
- Static analyzer (PVS-Studio, Coverity) — `BUILD.md`
- Code review (pre-merge)

### B) Operasyon Tarafı
- BugTrap auto-restart (process down → otomatik aç)
- Saatlik DB backup
- Disk monitor (>80% dolu → alert)
- Log rotation (7 gün lokal)
- DB compress + index rebuild (haftalık)

### C) Test
- Pre-deploy smoke test (`SMOKE_TEST.md`)
- Load test (>200 oyuncu simülasyon)
- Stress test event (CSW yoğunluk)
- Chaos engineering (random crash injection — uzun vade)

---

## 7. AUTO-RECOVERY (Watchdog)

### Process Watchdog
```bat
:: watchdog.bat — Task Scheduler 1dk loop
@echo off
tasklist | findstr "GameServer.exe" >nul
if errorlevel 1 (
   echo %date% %time% GameServer kayip, restart >> watchdog.log
   start "" "C:\Users\Administrator\Desktop\Server\GameServer.exe"
)

tasklist | findstr "LogInServer.exe" >nul
if errorlevel 1 (
   echo %date% %time% LogInServer kayip, restart >> watchdog.log
   start "" "C:\Users\Administrator\Desktop\Server\LogInServer.exe"
)
```

⚠️ **Otomatik restart 3 kez başarısız** → manuel müdahale (sürekli crash döngüsü engelle).

### NSSM Service Wrapper (önerilen)
```bash
nssm install GameServerSvc "C:\Users\Administrator\Desktop\Server\GameServer.exe"
nssm set GameServerSvc AppDirectory "C:\Users\Administrator\Desktop\Server"
nssm set GameServerSvc Start SERVICE_AUTO_START
nssm set GameServerSvc AppRestartDelay 30000   # 30sn bekle
nssm start GameServerSvc
```

→ Process kaybolursa NSSM otomatik restart, log Windows Event'a düşer.

---

## 8. LANSMAN GÜNÜ HAZIR DURUMU

- [ ] BugTrap aktif mi? (`BugTrap-x64.dll` Server\ klasöründe)
- [ ] Watchdog kurulu mu? (NSSM veya .bat scheduler)
- [ ] Discord webhook crash alert?
- [ ] Saatlik DB backup test edildi mi?
- [ ] Restart prosedürü Patron + DOKTOR + CHIP biliyor mu?
- [ ] Log archive dolduğunda F: kopyalama?
- [ ] Disk yer >50 GB free?
- [ ] RAM swap ayarlandı mı?
- [ ] Windows Update otomatik kapalı mı? (peak'te update reboot olmaz)
- [ ] DC oyunculara duyuru template hazır mı?

---

## 9. SIK CRASH SEBEPLERİ (KO 1098 deneyim)

| Sebep | Önlem |
|-------|-------|
| Skill null target | CHIP code review |
| Item dupe race | DB transaction |
| Quest lua infinite loop | Lua timeout |
| Magic instance freed | Memory pool |
| TBL hash mismatch | TBL_HASH validation |
| ODBC reconnect fail | Retry logic |
| Pearl Guard memory scan crash | AC team incele |
| Patch eski exe ile uyumsuz | Patch süreç |

---

## 10. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **Restart'tan önce log al** — yoksa sebep kaybolur |
| 2 | **DB corrupt → restore** sıra önemli (single→multi user) |
| 3 | **Auto-restart 3 kez fail** → manuel müdahale, sonsuz döngü engelle |
| 4 | **BugTrap dump > 100 MB olabilir** — F: yedek alanı şart |
| 5 | **Crash sebebi DB ise** restart → yine crash → kök sebep DB |
| 6 | **Lansman gece** → vardiya şart (DOKTOR + CHIP + MATRIX min) |
| 7 | **Reboot sonrası SSH geç başlar** — BEKLE (memory) |
| 8 | **GameServer önce kapat, LoginServer sonra** — ters sıra connection drop |
| 9 | **DB transaction ortasında crash** → uncommitted veri kayıp |
| 10 | **Log dosyası 5+ GB ise** → archive + sil, performans için |

---

## 11. KAYNAK REFERANSLAR

- **Log:** `LOG_MONITORING.md`
- **Backup:** `BACKUP_RESTORE.md`
- **Build:** `BUILD.md`
- **Anti-cheat:** `ANTI_CHEAT.md`
- **Source:** `SRC_HARITA.md`, `SRC_ONEMLI_CPP.md`
- **DB:** `DB_SEMA.md`, `DB_STORED_PROC.md`
- **GM:** `GM_KOMUT.md` (`+down`, `+care`)
- **Sunucu yol:** `SUNUCU_DOSYA_YOLLARI.md`
- **BugTrap:** [bugtrap.io](https://bugtrap.io)
- **NSSM:** [nssm.cc](https://nssm.cc)

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Lansman ilk hafta gerçek crash (varsa) post-mortem ile güncelle. Watchdog kurulduğunda detay ekle.
