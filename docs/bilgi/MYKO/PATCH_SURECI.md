# PATCH SÜRECİ — Client Güncelleme, Deploy, Encryption

**Tarih:** 2026-04-29 | **Kategori:** DEPLOY / MAINTENANCE | **Version:** 2370-2373 | **Tool:** patch_tool.py

---

## 1. PATCH GENEL

Patch = **Oyuncu tarafında fark/diff** (sadece değişen dosya).

**Aşamalar:**
1. **Build dev client** (yeni exe/dll/asset)
2. **Diff hesapla** (patch_tool.py)
3. **Zip oluştur** (patch/X.zip)
4. **SSH → sunucuya upload**
5. **DB → VERSION tablo update**
6. **Taskkill + Server restart**

---

## 2. PATCH ZIP ARŞİVİ

### Konum

```
C:\Users\erenc\Desktop\Server\patch\
├── 2370.zip (984 KB)  — Version 2370
├── 2371.zip (985 KB)  — Version 2371
├── 2372.zip (984 KB)  — Version 2372
├── 2373.zip (12 MB)   — Version 2373 (son, büyük)
└── [önceki versiyonlar backup]
```

### İçerik (Örnek: 2373.zip)

```
2373.zip/
├── KnightOnline.exe        (15 MB) — yeni executable
├── code.guard              (6 MB)  — anti-cheat DLL
├── CodeGuard/Code/
│   ├── re_*.code          (RC4 şifreli)
│   ├── macho_*.code
│   ├── co_*.code
│   └── [NPC UI script]
├── Data/
│   ├── Item.tbl           (değişen tablo)
│   ├── MagicTable.tbl
│   └── [diff datalar]
└── [diğer değişen asset]
```

**Not:** Tüm dosya encrypt hali → Launcher tarafında apply

---

## 3. VERSION TABLO (DB)

### Lokasyon

```
Database: KO_MYKO
Table: VERSION
```

### Yapısı

| Sütun | Örnek |
|-------|-------|
| **version** | 2373 |
| **prev_version** | 2372 |
| **patch_file** | 2373.zip |
| **checksum** | SHA1 hash |
| **release_date** | 2026-04-29 |
| **mandatory** | 1 (0=optional) |

### DB İnsertion (Deploy sırasında)

```sql
INSERT INTO VERSION (version, prev_version, patch_file, checksum)
VALUES (2373, 2372, '2373.zip', 'abc123def456...');
```

---

## 4. PATCH VERSIONING AKIŞI

### Version Chain (Sequential)

```
2370 ← 2371 ← 2372 ← 2373 (CURRENT)
       ↑      ↑      ↑
     Patch  Patch  Patch
      985K   985K   12 MB
```

**Her version:** prev_version bağlı (rollback için)

### Version Upsync Mekanizmi

1. **Client version** ← Server.ini / VERSION tablo
2. **Client = 2370** → Server = 2373
3. **Fark:** 2370 → 2371 → 2372 → 2373 (3 patch)
4. **Download:** 2371.zip, 2372.zip, 2373.zip sırayla
5. **Apply:** Her patch extract + file overwrite

---

## 5. PATCH SERVER (HTTP)

### Sunucu Uygulaması

```
104.238.23.99:80 → patch_server.js (Node.js)
```

### Endpoint

```
GET /api/version           → Current version number
GET /api/patch/:version    → Download patch/X.zip
POST /api/checksum         → Verify integrity
```

### Oyuncu Flow (Launcher)

```
1. Launcher.exe başla
2. GET /api/version → Server version bak
3. IF local_version < server_version:
     FOR each patch (2370 → 2373):
       GET /patch/X.zip
       Download local patch/ klasöre
4. Extract + Apply (KnightOnline.exe update)
5. Restart KnightOnline.exe
```

---

## 6. PATCH TOOL (patch_tool.py)

### Kullanım

```bash
python patch_tool.py \
  --from 2372 \
  --to 2373 \
  --source C:\path\to\new\client \
  --output C:\Users\erenc\Desktop\Server\patch\2373.zip
```

### İşlemi

1. **Diff hesapla:** 2372 vs 2373
2. **Değişen dosyaları** seç
3. **Encrypt:** RC4 (.code), DES (.tbl)
4. **Zip:** İçeriği sıkıştır
5. **Checksum:** SHA1 hash
6. **DB record:** VERSION tablo insert ready

### Restrictions

- ⚠️ **KO admin sadece** patch oluşturabilir
- ⚠️ **SSH key gerek** (sunucuya upload)
- ⚠️ **Encrypt tool** (GHOST domain)

---

## 7. ENCRYPTION (Patch içinde)

### RC4 Şifreleme (.code dosya)

```bash
# Patch oluşturmadan önce
python tools/key_rotation/rc4_re_encrypt.py *.code
→ Yeni key → new .code dosya
```

### DES/K2 Şifreleme (.tbl)

```bash
# TBL datalar
python tools/key_rotation/tbl_re_encrypt.py Item.tbl
→ New key → encrypted Item.tbl
```

### Exe Key Patch

```bash
# KnightOnline.exe içindeki key güncelle
python tools/key_rotation/exe_key_patcher.py \
  KnightOnline.exe \
  --new-rc4-key <key> \
  --new-tbl-key <key>
```

**Not:** Yeni key = her patch version (forward secrecy)

---

## 8. DEPLOY ADIMLAR (MANUEL)

### Prep Phase

```bash
# 1. Dev client build
cd C:\temp\MYKO\DEV_CLIENT\
msbuild ByNoiseGameServer.sln -p:Configuration=Release

# 2. Asset encrypt (GHOST tool)
python tools/key_rotation/rc4_re_encrypt.py CodeGuard/Code/*.code
python tools/key_rotation/tbl_re_encrypt.py Data/*.tbl

# 3. Exe patch key update
python tools/key_rotation/exe_key_patcher.py \
  ByNoiseGameServer.exe \
  --new-key <random>

# 4. Patch zip oluştur
python patch_tool.py --from 2372 --to 2373 --output 2373.zip
```

### Upload Phase

```bash
# 5. SSH → sunucuya upload
scp patch/2373.zip \
    root@104.238.23.99:/mnt/patch/

# 6. DB → VERSION insert
sqlcmd -S localhost\MSSQLSERVER01 \
  -d KO_MYKO \
  -Q "INSERT INTO VERSION VALUES (2373, 2372, '2373.zip', '<hash>');"
```

### Restart Phase

```bash
# 7. Sunucuda: eski process kapat
ssh root@104.238.23.99 "taskkill /IM GameServer.exe /F"
ssh root@104.238.23.99 "taskkill /IM LoginServer.exe /F"

# 8. Yeni exe run
ssh root@104.238.23.99 \
  "wmic process call create \"C:\\path\\to\\GameServer.exe\""

# 9. Boot wait
# ⚠️ Process "created" ≠ Ready (10-20 saniye extra)
sleep 30

# 10. Health check
curl http://104.238.23.99:15001/health
→ 200 OK ise ready
```

---

## 9. REBOOT TIMING BUG (ÖNEMLİ)

### Sorun

```
"Process create" döndüğünde:
- Executable bellek load → √
- Network socket open → √
- Database connection → NOT YET
- Player login start → CRASH
```

### Çözüm

```bash
# Deploy after taskkill:
taskkill GameServer /F    # process died
wmic create ... GameServer.exe
# ← BURAYA kadar ~5 saniye

# ⚠️ WAIT (10-20 saniye minimal)
sleep 20

# Sonra login check
# ← Artık DB ready, socket open
```

**Memory:** CLAUDE.md § ssh_boot_timing (task complete ≠ hazır)

---

## 10. PATCH OPTIONAL vs MANDATORY

### Mandatory Patch

```sql
INSERT INTO VERSION (version, ... , mandatory=1)
```

**Effect:**
- Client **zorunlu** patch
- Launcher → patch download BEFORE game start
- Reject connect (version mismatch)

### Optional Patch

```sql
INSERT INTO VERSION (version, ... , mandatory=0)
```

**Effect:**
- Client **seçimli** patch
- Launcher → notify "update available" (skip ok)

---

## 11. ROLLBACK MEKANIZMI

### Backwards Compat

Her VERSION record → `prev_version` field

```sql
SELECT * FROM VERSION WHERE version = 2373;
prev_version = 2372
```

**Rollback adımı:**

```bash
# 1. VERSION tablo güncelle
UPDATE VERSION SET version = 2372 WHERE version = 2373;

# 2. Patch server güncelle (2372.zip serve)

# 3. Server restart (eski exe)

# 4. Client → patch down (2373 → 2372)
```

**Not:** Full rollback (execute + asset) gerek

---

## 12. PATCH BOYUT OPTIMIZASYON

### Diff Strategy

**Sadece değişen satırlar encode:**

- **Item.tbl:** 1 satır → 1KB patch
- **MagicTable.tbl:** 5 satır → 5KB patch
- **KnightOnline.exe:** 500KB changed → 500KB patch

**Örnek (2373.zip 12 MB):**
- KnightOnline.exe: 10 MB (major binary change)
- Code files: 1 MB (.code update)
- Data: 1 MB (.tbl variation)

---

## 13. PATCH VERSIONING BEST PRACTICE

### Version Number Scheme

```
PATCH_VERSION = MAJOR.MINOR.HOTFIX
Örnek: 2373 = 2370-base + 3 patch
```

**Sequential:**
- 2370 = Base
- 2371 = 1st patch (bug fix)
- 2372 = 2nd patch (balance)
- 2373 = 3rd patch (security)

### Release Cycle (1098)

```
2026-04-29: 2373 released (CSW update)
2026-05-01: 2374 (if bug found)
2026-05-08: 2375 (event patch)
```

---

## 14. HATA VE SORUN

### Bilinen Sorunlar

| Sorun | Sebebi | Çözüm |
|-------|--------|-------|
| **Patch download timeout** | Network lag | Retry logic (3x) |
| **Checksum mismatch** | Corrupt download | Redownload |
| **RC4 key mismatch** | Key not patched | exe_key_patcher rerun |
| **TBL_HASH fail** | .tbl re-encrypt miss | Recalculate hash (GameServer.ini) |
| **Login crash post-patch** | Boot timing | sleep +5sec |

---

## 15. 1098 PATCH HISTORY

### Released Patches (Tahmini)

| Version | Date | Content |
|---------|------|---------|
| 2370 | 2026-04-01 | 1098 base launch |
| 2371 | 2026-04-10 | Bug fix (TS scroll) |
| 2372 | 2026-04-20 | Balance patch |
| 2373 | 2026-04-29 | CSW event + Cape bonus |

---

## 16. DİKKAT NOKTALARI

⚠️ **Patch zip = client-side only** — server logic ayrı deploy
⚠️ **RC4/DES key rotate her patch** — forward secrecy
⚠️ **Reboot timing = task create ≠ ready** — sleep +20sec
⚠️ **VERSION tablo = single source of truth** — DB first
⚠️ **Mandatory=1 rejects old client** — compat check
⚠️ **Checksum = integrity critical** — SHA1 verify
⚠️ **Patch sequential** — skipped version = sync problem

---

## 17. KAYNAKLARA BAĞLA

- **patch_tool.py** — Python CLI
- **patch_server.js** → Node.js server
- **VERSION tablo** — KO_MYKO database
- **RC4 key rotation** → GHOST domain
- **TBL_HASH validation** → GameServer.ini

---

## 18. DEPLOY CHECKLIST

```
PRE-DEPLOY:
☐ Dev build complete
☐ Asset encrypt (RC4, DES)
☐ Exe key patch
☐ Patch zip created + checksum
☐ SSH credential ready

DEPLOY:
☐ SCP upload patch/X.zip → server
☐ SQL INSERT VERSION record
☐ Announce maintenance window (30 min)

RESTART:
☐ taskkill GameServer/LoginServer
☐ wmic create new exe
☐ sleep 20 saniye
☐ curl health check
☐ Player login test (1 test account)

POST-DEPLOY:
☐ Server logs kontrolü (error free)
☐ Player feedback (discord)
☐ Rollback plan ready
```

---

**Dosya sürümü:** v1.0
**Yazanı:** KODCU | **İnceleme:** —
