# 💾 BACKUP / RESTORE — DB + Server + Web Yedekleme

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `MYKO_BACKUP\`, `F:\MDBACKUP\`, `F:\MYKOBACKUP\`, MSSQL backup komutları
**Hedef:** Ne yedeklenir, nasıl, nerede tutulur, nasıl geri alınır.

---

## 1. NE YEDEKLENİR

| # | Bileşen | Sıklık | Yer |
|---|---------|--------|-----|
| 1 | **KO_MYKO** (ana DB) | **Saatlik** | Production C:\MYKO_BACKUP\ |
| 2 | **KO_LOG** | Günlük | C:\MYKO_BACKUP\ |
| 3 | **GameServer\*** (binary+INI+TBL) | Patch öncesi | F:\MYKOBACKUP\ |
| 4 | **AntiCheat src** | Major build öncesi | F:\MYKOBACKUP\ |
| 5 | **koweb2** (web) | Günlük | tar.gz F:\ |
| 6 | **Flarum DB** (MariaDB:3307) | Günlük | mysqldump |
| 7 | **orkestra.db** (SQLite WAL) | Session sonu | F:\MDBACKUP\ |
| 8 | **Quest LUA klasörü** | Patch öncesi | F:\MYKOBACKUP\ |
| 9 | **Map .smd** | Map güncellemesi öncesi | F:\MYKOBACKUP\ |
| 10 | **Logs/** | Haftalık archive | F:\MDBACKUP\Logs\ |

---

## 2. MSSQL DATABASE BACKUP

### A) Manuel Full Backup
```sql
BACKUP DATABASE KO_MYKO
TO DISK = 'C:\MYKO_BACKUP\KO_MYKO_FULL_20260508_2200.bak'
WITH FORMAT, INIT, COMPRESSION,
     NAME = 'KO_MYKO Full Backup',
     STATS = 10;
```

### B) Differential Backup (incremental, hızlı)
```sql
BACKUP DATABASE KO_MYKO
TO DISK = 'C:\MYKO_BACKUP\KO_MYKO_DIFF_20260508_2300.bak'
WITH DIFFERENTIAL, COMPRESSION;
```

### C) Transaction Log Backup (log shipping için)
```sql
BACKUP LOG KO_MYKO
TO DISK = 'C:\MYKO_BACKUP\KO_MYKO_LOG_20260508_2330.trn';
```

### D) Otomasyon — SQL Agent Job
```sql
-- Saatlik full (lansman + 1 hafta için, sonra azalt)
USE msdb;
EXEC sp_add_job @job_name='KO_MYKO_HOURLY_BACKUP';

-- Step
EXEC sp_add_jobstep
   @job_name='KO_MYKO_HOURLY_BACKUP',
   @step_name='Full Backup',
   @subsystem='TSQL',
   @command='BACKUP DATABASE KO_MYKO TO DISK = ''C:\MYKO_BACKUP\KO_MYKO_'' + CONVERT(varchar(20), GETDATE(), 112) + ''_'' + REPLACE(CONVERT(varchar(20), GETDATE(), 108), '':'', '''') + ''.bak'' WITH COMPRESSION, INIT;';

-- Schedule (her saat başı)
EXEC sp_add_schedule
   @schedule_name='Every Hour',
   @freq_type=4,         -- daily
   @freq_interval=1,
   @freq_subday_type=8,  -- hourly
   @freq_subday_interval=1;

EXEC sp_attach_schedule
   @job_name='KO_MYKO_HOURLY_BACKUP',
   @schedule_name='Every Hour';

EXEC sp_add_jobserver @job_name='KO_MYKO_HOURLY_BACKUP';
```

⚠️ **MSSQL Express Edition** SQL Agent İÇERMİYOR — Standard/Developer şart, yoksa Windows Task Scheduler + sqlcmd kullan.

### E) Windows Task Scheduler Alternatif
```bat
@echo off
:: backup_hourly.bat
set TS=%date:~10,4%%date:~4,2%%date:~7,2%_%time:~0,2%%time:~3,2%
sqlcmd -S localhost\MSSQLSERVER01 -E -Q "BACKUP DATABASE KO_MYKO TO DISK = 'C:\MYKO_BACKUP\KO_MYKO_%TS%.bak' WITH COMPRESSION, INIT"
```

Task Scheduler'a ekle: her saat başı.

---

## 3. RESTORE PROSEDÜRÜ

### A) Full Restore (DB tamamen değiş)
```sql
USE master;

-- Aktif bağlantıları kapat
ALTER DATABASE KO_MYKO SET SINGLE_USER WITH ROLLBACK IMMEDIATE;

-- Restore
RESTORE DATABASE KO_MYKO
FROM DISK = 'C:\MYKO_BACKUP\KO_MYKO_FULL_20260508_2200.bak'
WITH REPLACE, RECOVERY,
     STATS = 10;

-- Multi-user'a geri al
ALTER DATABASE KO_MYKO SET MULTI_USER;
```

### B) Point-in-Time Restore (zaman bazlı)
```sql
-- Önce full
RESTORE DATABASE KO_MYKO FROM DISK = '...FULL...bak' WITH NORECOVERY, REPLACE;

-- Sonra diff (varsa)
RESTORE DATABASE KO_MYKO FROM DISK = '...DIFF...bak' WITH NORECOVERY;

-- Son log (belirli zamana kadar)
RESTORE LOG KO_MYKO FROM DISK = '...LOG...trn'
WITH STOPAT = '2026-05-08 22:30:00', RECOVERY;
```

### C) Tek Tablo Restore (sıkıntılı, alternatif yol)
- Yedek DB'yi `KO_MYKO_TEMP` olarak restore et
- `INSERT INTO KO_MYKO.dbo.X SELECT * FROM KO_MYKO_TEMP.dbo.X`

---

## 4. SERVER BINARY YEDEKLEME

### A) Patch Öncesi
```bat
:: backup_server.bat (manuel veya automated)
set TS=%date:~10,4%%date:~4,2%%date:~7,2%
xcopy /E /I /Y "C:\Users\Administrator\Desktop\Server" "F:\MYKOBACKUP\Server_%TS%"
```

### B) Sıkıştırma
```bat
:: 7zip
"C:\Program Files\7-Zip\7z.exe" a -t7z "F:\MYKOBACKUP\Server_%TS%.7z" "C:\...\Server\"
```

### C) Yedek Zinciri (mevcut)
- `GameServer.exe.bak`, `.bak.22mart`, `.bak.26mart`
- `LogInServer.exe.bak_26mart`
- `GameServer.ini.bak_20260326`
- `BACKUP_04_MART_2026\`, `BACKUP_10_MART_2026\`, `backup_28mart\`
- `2373_extracted\`

⚠️ **`LogInServer.exe.broken_28mart`** — KIRIK build, kullanma.

---

## 5. WEB YEDEKLEME

### A) koweb2 (PHP)
```bash
# Production'da
cd /c/koweb2
tar -czf /f/MYKOBACKUP/koweb2_$(date +%Y%m%d).tar.gz .
```

### B) Flarum forum (MariaDB:3307)
```bash
# Forum dosyaları
tar -czf /f/MYKOBACKUP/forum_files_$(date +%Y%m%d).tar.gz /c/koweb2/forum

# Forum DB
mysqldump -u root -p -P 3307 flarum_db > /f/MYKOBACKUP/flarum_db_$(date +%Y%m%d).sql
```

---

## 6. ORKESTRA DB YEDEKLEME

```bash
# SQLite WAL safe backup
sqlite3 /c/orkestra/orkestra.db ".backup '/f/MDBACKUP/orkestra_$(date +%Y%m%d_%H%M).db'"

# Veya komuta
$O kapanis ile otomatik yedekleniyor (S87'de S87_KAPANIS_*.db örneği)
```

**Mevcut yedek zinciri:**
```
F:\MDBACKUP\
├── orkestra_S87_KAPANIS_*.db (17 MB)
├── orkestra_S87_GECE_KAPANIS_*.db
├── orkestra_exe_S87_v2_oncesi_20260429.exe
└── (haftalık + session sonu)
```

---

## 7. RETENTION POLİTİKASI

| Yedek tipi | Tut | Sil |
|------------|-----|-----|
| Saatlik DB | Son 24 saat | 24h sonra silinir |
| Günlük DB | Son 30 gün | 30d sonra silinir |
| Haftalık DB | Son 12 hafta | 12w sonra silinir |
| Aylık DB | Son 12 ay | 12m sonra silinir |
| **Yıllık DB** | **KALICI** | Hiç |
| Server binary | Major patch'ler | Eski 6 ay sonra silinir |
| Web | Son 30 gün | 30d sonra |
| GM/HACK log | KALICI (audit) | Hiç |
| Crash dump | Son 6 ay | 6m sonra |

---

## 8. DR (Disaster Recovery) Planı

### Senaryo: Production Sunucu Tamamen Down
**Bileşenler:**
- ✅ DB yedek (F:\MYKOBACKUP\MalasiakoDB\) — son 30 gün
- ✅ Server binary (F:\MYKOBACKUP\Server\)
- ✅ Web (F:\MYKOBACKUP\koweb2.rar)
- ✅ Source (F:\MYKOBACKUP\ALL SRC GİT BACKUP.rar)

**RTO** (Recovery Time Objective) = **4 saat**
**RPO** (Recovery Point Objective) = **1 saat** (saatlik DB backup)

### Adımlar
1. **Yeni VPS provision** (Hostabil yedek)
2. **Windows Server kurulum** (lisanslı, EVALUATION değil)
3. **MSSQL 2019 install** + named instance `MSSQLSERVER01`
4. **DB restore:** En son `.bak` dosyasından
5. **ODBC DSN** kur: KO_MAIN, KO_GAME, KO_LOG (Türkçe karaktersiz)
6. **GameServer.ini + LogInServer.ini** restore
7. **Server binary** restore (Desktop\Server\)
8. **TBL_HASH** doğrula (`+reloadtables` boot sonrası)
9. **Pearl Guard `code.guard`** doğrula
10. **DNS güncelle** (malasiako.com → yeni IP)
11. **Web restore** (koweb2)
12. **Forum DB restore** (mysqldump'tan)
13. **Patch port 80** node servis
14. **Test:** GM hesap login → +count
15. **Duyuru:** `+noticeall "Sistem geri geldi"`

---

## 9. LANSMAN ÖNCESİ HAZIRLIK

### T-7 GÜN (01 Mayıs)
- [ ] Saatlik DB backup script test
- [ ] F:\ alan kontrol (en az 100 GB free)
- [ ] Restore prosedürü dry-run (test DB üzerinde)
- [ ] Yedekler şifreli mi? (AES BitLocker)
- [ ] Off-site copy var mı? (cloud storage düşün)

### T-1 GÜN (07 Mayıs)
- [ ] Full backup al → tag `LAUNCH_T-1`
- [ ] Server binary tar.gz → `LAUNCH_T-1`
- [ ] Web tar.gz → `LAUNCH_T-1`
- [ ] Git tag (orkestra-rs + MYKO repos)
- [ ] DR procedure son okunma

### Lansman Sonrası
- [ ] T+1h: ilk saatlik backup OK
- [ ] T+24h: full backup → `LAUNCH_T+24`
- [ ] T+1w: haftalık özet rapor

---

## 10. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **`KO_MYKO` SET SINGLE_USER → restore → MULTI_USER** sırası kritik |
| 2 | **Restore sırasında oyuncular DC olur** — bakım modu duyurusu şart |
| 3 | **`.bak` dosyası lokalde kalsın** — uzak depolamaya kopya |
| 4 | **Yedek doğrulama:** `RESTORE VERIFYONLY FROM DISK = '...'` |
| 5 | **MSSQL Express:** SQL Agent yok → Task Scheduler kullan |
| 6 | **DB compression** — yedek %70 küçültür ama CPU kullanır |
| 7 | **F:\ disk dolarsa** = backup duraklatılır = OPS riski |
| 8 | **ODBC DSN Türkçe karakter** = silent fail (KO_MAİN bug) |
| 9 | **Backup sırasında SQL kilit** uzun sürebilir, peak'ten kaçın |
| 10 | **Restore test edilmemiş yedek = yedek değildir** |

---

## 11. KAYNAK REFERANSLAR

- **DB connection:** `DB_SEMA.md § Bağlantı`
- **DSN:** `BUILD.md § ODBC DSN`
- **Server yolları:** `SUNUCU_DOSYA_YOLLARI.md`
- **Web:** `WEB_PHP.md`, `WEB_FORUM.md`
- **Patch:** `PATCH_SURECI.md`
- **MSSQL Resmi:** [docs.microsoft.com/sql/relational-databases/backup-restore](https://docs.microsoft.com/sql)
- **Mevcut yedekler:** `F:\MYKOBACKUP\`, `F:\MDBACKUP\`

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Production'da gerçek SQL Agent kuruldu mu doğrulanacak (MATRIX). DR dry-run yapılınca güncelle.
