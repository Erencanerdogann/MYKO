# 🗂️ SUNUCU & DOSYA YOLLARI — Tek Sayfa Referans

**Tarih:** 2026-04-29 (S88)
**Yazan:** DOKTOR
**Yer:** `C:\temp\MYKO\docs\bilgi\MYKO\SUNUCU_DOSYA_YOLLARI.md`
**Hedef:** Bütün yollar tek yerde — agent klasör keşfi yapmasın, buradan oku.

---

## 1. PRODUCTION SUNUCU (104.238.23.99)

### Erişim
| Bilgi | Değer |
|-------|-------|
| **IP** | `104.238.23.99` (Hostabil VPS) |
| **OS** | Windows Server 2019 Standard EVALUATION ⚠️ |
| **Kullanıcı** | `Administrator` |
| **SSH** | `ssh Administrator@104.238.23.99` |
| **RDP** | mstsc /v:104.238.23.99 |
| **Şifreler** | `Desktop\ERENCAN_HASSAS_BILGILER_USB_ALSAN.md` |

### Game Server Yolları
```
C:\Users\Administrator\Desktop\Server\
├── GameServer.exe              ← Ana oyun servisi
├── GameServer.ini              ← Config (port, DSN)
├── LogInServer.exe             ← Login servisi
├── LoginServer.ini             ← Login config
├── Logs\                       ← Server logları
├── Map\                        ← .smd zone dosyaları
├── Quests\                     ← Lua quest (production kopyası)
├── code.guard                  ← Pearl Guard config
└── Notice.txt / Notice_up.txt  ← Duyuru
```

### Veritabanı
| Bilgi | Değer |
|-------|-------|
| **SQL Instance** | `localhost\MSSQLSERVER01` |
| **TCP Port** | 7354 (named instance) |
| **Ana DB** | `KO_MYKO` |
| **Log DB** | `KO_LOG` |
| **Test DB** | `KO_TEST` + `KO_TEST_LOG` |
| **DSN: KO_MAIN** | → KO_MYKO |
| **DSN: KO_GAME** | → KO_MYKO |
| **DSN: KO_LOG** | → KO_LOG |
| **User** | `sa` |
| **DB Backup** | `C:\MYKO_BACKUP\` + `Desktop\DB_BACKUP\` |

### Network Portları (Production)
| Port | Servis |
|------|--------|
| 15001 | GameServer |
| 15100 | LoginServer |
| 15100-15109 | LoginServer aralığı |
| 80 | patch_server.js (Node) |
| 3001 | orkestra-server (Site API) |
| 8091 | Web (PHP nginx — koweb2) |
| 3010 | orkestra-service komuta |
| 3307 | MariaDB (Flarum forum) |
| 7354 | MSSQL named instance |

### Web Servisleri (Production)
```
C:\koweb2\                     ← Aktif PHP web (nginx altında)
├── (site sayfaları)
├── api\                       ← PHP rest endpoint
├── forum\                     ← Flarum kurulumu
└── nginx config: ?

C:\Users\Administrator\nssm-2.24\   ← MYKO_WEB servis (php-cgi)
```

---

## 2. LOKAL DEV (Bu PC — Erencan)

### Test Server (Lokal Çalıştırma)
```
C:\Users\erenc\Desktop\Server\
├── GameServer.exe                    ← Lokal test build
├── GameServer.exe.bak / .bak.22mart / .bak.26mart
├── GameServer.ini                    ← Lokal config (DSN: CodeGuardMYKO_DB)
├── GameServer.ini.bak_20260326
├── LogInServer.exe / LogInServer.exe.bak_26mart
├── LogInServer.exe.broken_28mart    ← ⚠️ KIRIK BUILD (kullanma)
├── LoginServer.ini
├── 2373_extracted\
├── BACKUP_04_MART_2026\
├── BACKUP_10_MART_2026\
├── backup_28mart\
├── DXT & UIE EDİTÖR\                 ← Editör araçları
├── Logs\
├── Map\
├── Quests\
├── EventAwards.ini / EventSettings.ini
├── ClanPremiumNotice.txt / CapeBonus.txt / Notice.txt
├── KO_ServerPanel.bat / SetPriority.bat
└── code.guard
```

**Lokal INI değerleri (GameServer.ini):**
```ini
ACCOUNT_DSN = CodeGuardMYKO_DB
ACCOUNT_UID = sa
ACCOUNT_PWD = password
GAME_DSN = CodeGuardMYKO_GAME
LOG_DSN = CodeGuard_LOG
LOGIN_IP = 127.0.0.1
LOGIN_PORT = 15100
PORT = 15001
```

⚠️ **DİKKAT:** "password" placeholder olarak duruyor. Production'da gerçek şifre var, **doğrula**.

### Source Code
```
C:\temp\MYKO\src\
├── GameServer_SRC\          ← C++ server source (~282 dosya)
├── AntiCheat_SRC\           ← Pearl Guard source (~203 dosya, 1.3 GB)
├── 3Launcher\               ← Client launcher source
└── INDEX.md
```

### Client (User PC kopyası)
```
C:\MalaysiaKO\                ← Oyuncu client kurulumu (5 GB)
├── KnightOnLine.exe          ← Ana exe
├── ChrSelect / Chr           ← Karakter sistemi
├── CodeGuard\                ← Anti-cheat client kısmı
├── ByNo\
├── Apr_Show.dll
├── 4code.guard / 5code.guard
└── *.ksc                     ← Screenshot dosyaları (oyuncu)
```

### Asset / Bilgi Klasörleri
```
C:\temp\MYKO\
├── src\                      ← C++ kaynak (yukarıda)
├── orkestra-rs\              ← Rust orkestra workspace (8 crate)
├── docs\
│   ├── agents\               ← Agent kimlikleri (KIM/, DOSYA_HARITASI.md)
│   └── bilgi\
│       ├── README.md         ← Çalışma rehberi
│       ├── MYKO\             ← 31 KO bilgi MD (BU DOSYA DAHIL)
│       └── ORKESTRA\         ← Boş (S88+ doldurulacak)
├── PLANLAR\                  ← Plan/brief MD'leri
├── kaynak\
│   └── MD_RAPORLAR\          ← MAT29-32 raporları
└── INDEX.md
```

---

## 3. ORKESTRA RUNTIME (Bu PC)

```
C:\orkestra\                  ← Production runtime
├── orkestra.exe              ← Ana CLI (TAM YOL: /c/orkestra/orkestra.exe)
├── orkestra-service.exe      ← Windows servis (port 3010)
├── orkestra-mcp.exe          ← MCP server
├── orkestra-panel.exe        ← Web panel
├── amiral.exe                ← Amiral UI
├── myko-panel.exe            ← MYKO operasyon panel
├── orkestra.db               ← TEK DB (proje='MYKO-AI'/'ORKESTRA-AI' filter, WAL)
├── config.json
├── BASLAT.bat / FULL_START.bat / FULL_CLEAN.bat / FULL_KAPAT.bat
├── ORKESTRA_KOMUTA.bat       ← Ana komuta arayüzü
├── KATEGORI_MD_HOOK\         ← Hook v1.2 (kategori → MD eşleme)
│   ├── KATEGORI_MD_HOOK.bat
│   ├── KATEGORI_MD_PARSE.ps1
│   └── kategori_md_map.json
├── agent_bat\                ← 9 agent.bat (DOKTOR.bat, MATRIX.bat ...)
├── agent_pid\                ← *.acik flag dosyaları
├── agent_log\                ← Per-agent log
├── logs\                     ← Sistem log
├── scripts\                  ← Yardımcı script
└── yedekler\
```

**Eski yol (deprecated):** `C:\temp\MYKO\orkestra-rs\orkestra.db` — kullanma, asıl `C:\orkestra\orkestra.db`.

---

## 4. AGENT KLASÖRLERİ (`C:\temp\<Agent>\`)

Her agent için aynı yapı:
```
C:\temp\<Agent>\               ← örn: C:\temp\Doktor\, C:\temp\Matrix\, C:\temp\Chip\, ...
├── CLAUDE.md                  ← ORKESTRA-AI sistem kuralları
├── MYKO_CLAUDE.md             ← MYKO-AI alan kartı
├── GOREVLER\                  ← Brief'ler
│   └── INDEX.md
├── hatalar\                   ← Geçmiş hatalar
│   └── INDEX.md
├── sessions\                  ← Session kayıtları (sadece DOKTOR'da yoğun)
├── .claude\
│   └── settings.json          ← PreToolUse hook
└── .claude-memory\            ← Claude Code cache
```

**9 agent klasörü:** Doktor, Matrix, Chip, Kodcu, Webra, Ghost, Rustik, Ganet, Rehber, Jerry (10 toplam DOKTOR dahil).

---

## 5. YEDEKLER (F: SÜRÜCÜ)

### F:\MDBACKUP\ (Hafıza/MD Yedekleri)
```
F:\MDBACKUP\
├── ARSIV\
├── C--Projects_memory\        ← 💎 BOMBA: 64 MD eski proje hafızası
├── KIM\                       ← Agent kim kartları yedek
├── Mimari\
├── INDEX.md
├── MDBACKUP.rar
├── orkestra_S87_KAPANIS_*.db  ← S87 DB yedek (17 MB)
├── orkestra_S87_GECE_KAPANIS_*.db
├── orkestra_exe_S87_*.exe
└── CLAUDE_MD_YEDEK_*.md
```

### F:\MYKOBACKUP\ (Kod/Asset Yedekleri)
```
F:\MYKOBACKUP\
├── 11.06.2024 SRC.rar              ← ⚠️ PATLAYAN proje (sadece referans)
├── 24.03.2026 CLIENT SRC.rar       ← 29.7 GB yeni client
├── ALL SRC GİT BACKUP.rar          ← 5.7 GB git history
├── ALPHA-2383 KO PROJE.rar
├── KO Assets.rar
├── KO DEV ARSIV\                   ← 4539 dosya tools/editör
├── MYKO_PK_28.02.2025.rar          ← 7 GB Session 17c yedek
├── MYKO_BACKUP_22MART.tar.gz       ← 10.8 GB
├── MYKO.rar
├── HASH\                            ← TBL hash dump
├── 25xxacs.rar
├── Dev_Mahsussahne.rar
├── koweb\ + koweb2.rar
├── Server\                          ← Build outputs
├── MalasiakoDB\                     ← DB backup
└── pacht\                           ← Patch dosyaları
```

---

## 6. KRİTİK KISA YOLLAR (Hızlı Erişim)

```bash
# Orkestra CLI (tam yol her zaman)
O=/c/orkestra/orkestra.exe
# eski: /c/temp/MYKO/orkestra-rs/orkestra.exe (DEPRECATED)

# DB
DB=/c/orkestra/orkestra.db

# MYKO bilgi havuzu
BILGI=/c/temp/MYKO/docs/bilgi/MYKO/

# Source
SRC=/c/temp/MYKO/src/GameServer_SRC/
AC_SRC=/c/temp/MYKO/src/AntiCheat_SRC/

# Lokal test server
LOKAL_SRV=/c/Users/erenc/Desktop/Server/

# Production sunucu (SSH)
PROD_SRV="Administrator@104.238.23.99:C:/Users/Administrator/Desktop/Server/"

# Web
WEB_PROD="Administrator@104.238.23.99:C:/koweb2/"

# Yedek
MDBACK=/f/MDBACKUP/
MYBACK=/f/MYKOBACKUP/
```

---

## 7. DEPLOY KOMUTLARI (Production)

```bash
# Dosya yükleme
scp DOSYA Administrator@104.238.23.99:"C:/Users/Administrator/Desktop/Server/"

# Patch deploy (DB + restart)
ssh Administrator@104.238.23.99 "sqlcmd -S localhost\\MSSQLSERVER01 -d KO_MYKO -Q \"INSERT INTO VERSION VALUES (X, X-1, 'X.zip')\""
ssh Administrator@104.238.23.99 "taskkill /IM LogInServer.exe /F"
ssh Administrator@104.238.23.99 "wmic process call create \"C:/Users/Administrator/Desktop/Server/LogInServer.exe\",\"C:/Users/Administrator/Desktop/Server\""

# ⚠️ Reboot sonrası task "completed" ≠ sunucu hazır. SSH geç başlar, BEKLE.
```

---

## 8. DİKKAT EDİLECEKLER

| ⚠️ | Konu | Detay |
|----|------|-------|
| 1 | **Production = onaysız erişim YASAK** | DOKTOR'a sor, patron onayı bekle |
| 2 | **GHOST sunucu YASAK** | S42 kalıcı policy |
| 3 | **JERRY SSH/RDP/DB YASAK** | S52 kalıcı policy |
| 4 | **Lokal `LogInServer.exe.broken_28mart`** | Kullanma — bozuk build |
| 5 | **"password" placeholder INI'de** | Lokal'de placeholder, production'da gerçek şifre var |
| 6 | **OS lisans EVALUATION** | Windows Server 2019 — lansman öncesi lisans bak |
| 7 | **Forward slash kuralı** | Bash'te `/c/...` (Unix-style), `C:\...` değil |
| 8 | **DB asıl: `C:\orkestra\orkestra.db`** | `C:\temp\MYKO\orkestra-rs\orkestra.db` deprecated |

---

## 9. KAYNAK MD'LER

- `Mykoproject.map.md` — Detaylı sistem haritası
- `MATERYAL_HARITASI.md` — Element ↔ dosya eşleme
- `BUILD.md` — Compile + deploy detayı
- `PATCH_SURECI.md` — Patch zip + deploy adımları
- `WEB_PHP.md` — Web yapısı detayı
- `WEB_API.md` — Site API endpoint
- `DB_SEMA.md` — DB tablo şeması

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki güncelleme:** Production INI değerleri doğrulandığında (CHIP/MATRIX yapacak)
