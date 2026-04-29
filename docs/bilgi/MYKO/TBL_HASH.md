# TBL_HASH Validation — MalaysiaKO 1098

**Tarih:** 2026-04-29 | **Versiyon:** 1.0 | **Agent:** MATRIX | **Session:** S87+

---

## Özet

GameServer boot'ta, client'ten gelen `.tbl` dosyalarının **integrity** kontrol edilir. Hash mismatch → **server crash** (anti-cheat mekanizması).

| Dosya | Hash Algoritma | Kontrol Yeri |
|-------|----------------|--------------|
| `Item.tbl` | MD5 | `GameServer.ini [TBL_HASH] ITEM_ORG` |
| `MagicTable.tbl` | MD5 | `GameServer.ini [TBL_HASH] MAGIC_MAIN` |
| `MagicTable_tk.tbl` | MD5 | `GameServer.ini [TBL_HASH] MAGIC_MAIN_TK` |
| `Zone*.tbl` | MD5 | `GameServer.ini [TBL_HASH] ZONES` |

---

## Mekanizma

### Boot Flow
```
GameServer start
    ↓
[TBL_HASH] section read from GameServer.ini
    ↓
For each critical TBL (Item, Magic, Zone):
    1. File load: C:\MalaysiaKO\Data\Item.tbl
    2. MD5 hash calc
    3. Compare with INI hash
    4. Hash match? → OK : CRASH (exit -1)
    ↓
Server ready
```

### Hash Mismatch Error
```
[ERROR] TBL integrity failed: Item.tbl
Expected: 47c1b0b24d99620b2ca5fdd7d59f1b5c
Got:      a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
Exit code: -1
```

---

## GameServer.ini [TBL_HASH] Bölümü

```ini
[TBL_HASH]
ITEM_ORG=47c1b0b24d99620b2ca5fdd7d59f1b5c
MAGIC_MAIN=a69367c11fff9b2e8c931e13067cbdb2
MAGIC_MAIN_TK=0b7ad4a0c9cba0d251fd9cf6065ac05f
ZONES=7df418ae59a9379beeb5bada629f4fef
```

### Şu Anki Değerler (S87)

| Dosya | MD5 Hash | Tarih | Agent |
|-------|----------|-------|-------|
| **ITEM_ORG** | `47c1b0b24d99620b2ca5fdd7d59f1b5c` | 2026-03-19 | — |
| **MAGIC_MAIN** | `a69367c11fff9b2e8c931e13067cbdb2` | 2026-03-19 | — |
| **MAGIC_MAIN_TK** | `0b7ad4a0c9cba0d251fd9cf6065ac05f` | 2026-03-19 | — |
| **ZONES** | `7df418ae59a9379beeb5bada629f4fef` | 2026-03-19 | — |

---

## Hash Hesaplama

### Algoritma: MD5

```python
import hashlib

def tbl_hash(filepath):
    with open(filepath, 'rb') as f:
        return hashlib.md5(f.read()).hexdigest()

hash_value = tbl_hash('C:\\MalaysiaKO\\Data\\Item.tbl')
print(hash_value)  # → 47c1b0b24d99620b2ca5fdd7d59f1b5c
```

### Command Line (Windows)

```cmd
# PowerShell
(Get-FileHash -Path "C:\MalaysiaKO\Data\Item.tbl" -Algorithm MD5).Hash

# Output: 47C1B0B24D99620B2CA5FDD7D59F1B5C
```

### Tool: tbl_hash.py

```bash
python tools/tbl/tbl_hash.py --file C:/MalaysiaKO/Data/Item.tbl
# Output: 47c1b0b24d99620b2ca5fdd7d59f1b5c
```

---

## TBL Değiştirilince Prosedürü

### Senaryo: Item.tbl stat güncellemesi

```
1. BACKUP
   cp C:\MalaysiaKO\Data\Item.tbl C:\MalaysiaKO\Data\Item.tbl.backup_S87

2. DECRYPT
   python tools/tbl/tbl_decrypt.py Item.tbl → Item.json
   (veya tbl_edit.py ile GUI edit)

3. EDIT
   Item.json → Damage değeri +10% (örnek)
   
4. ENCRYPT
   python tools/tbl/tbl_edit.py Item.json → Item.tbl.new
   cp Item.tbl.new Item.tbl

5. HASH HESAPLA
   python tools/tbl/tbl_hash.py Item.tbl
   Output: NEW_HASH = a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6

6. INI GÜNCELLE
   GameServer.ini [TBL_HASH] ITEM_ORG = a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
   
7. RESTART
   GameServer restart → boot log kontrol

8. VERIFY
   Boot log: "[TBL] Item.tbl hash OK"
   → Server ready
```

---

## Boot Log Kontrol

### Başarılı Boot
```
[INFO] Loading TBL files...
[INFO] Item.tbl loaded, hash verified
[INFO] MagicTable.tbl loaded, hash verified
[INFO] Zone data loaded, hash verified
[INFO] Server ready on port 15001
```

### Başarısız Boot
```
[ERROR] TBL integrity check FAILED
[ERROR] Item.tbl hash mismatch
  Expected: 47c1b0b24d99620b2ca5fdd7d59f1b5c
  Got:      a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
[ERROR] Server exit code: -1

→ Server crashes, no further log
```

---

## Mismatch Nedenleri

| Sebep | Çözüm |
|-------|-------|
| **TBL file değiştirildi** | İNİ hash'ı güncelle (prosedür 5-6) |
| **Kısmen yazılmış/corrupt TBL** | Backup'dan restore (Step 1) |
| **Wrong encryption/decryption** | tbl_decrypt.py / tbl_edit.py versyon check |
| **Locale karışması (tr vs us)** | Doğru locale file kontrol (Item.tbl vs Item_us.tbl) |
| **File lock (başka process açık)** | Tüm TBL reader kapat (client, tbl_tool, vb) |
| **Permission denied (INI read-only)** | GameServer.ini chmod 666 (Windows: remove read-only flag) |

---

## ⚠️ Tuzaklar & Uyarılar

### 1. MD5 Case Sensitivity
- `47c1b0b24...` (lowercase) vs `47C1B0B24...` (uppercase)
- INI parser → case-insensitive (ikiside çalışır)
- **Best practice:** lowercase (consistency)

### 2. Multiple Locale Hash
- MAGIC_MAIN (TR)
- MAGIC_MAIN_TK (Turkish keyboard)
- MAGIC_MAIN_US (English) — varsa kontrol et
- **Hepsi aynı dosya mı, farklı mı?** (audit gerekli)

### 3. TBL File vs TBL.BAK
- `Item.tbl` → hash checked
- `Item.tbl.backup` → ignored (dosya adı farklı)
- **Backup güvenli ama restore için manuel adım gerekli**

### 4. Hash Cache
- GameServer boot'ta **memory**'ye load
- Runtime'da TBL değiştirilirse → hash check yapılmaz
- **Çözüm:** Server restart (reload TBL + verify)

### 5. Atomic File Update
- TBL yazılırken partial write → corrupt
- **Güvenli:** Write `.tmp` → Rename `.tbl` (atomic)
- Tool: tbl_edit.py → handles this

---

## Deploy Kontrol Listesi

[ ] 1. Backup al: `*.tbl.backup_S<N>`
[ ] 2. TBL edit et (DOKTOR onay)
[ ] 3. Hash hesapla (tbl_hash.py)
[ ] 4. INI güncellemesi (GameServer.ini)
[ ] 5. Test sunucusu boot (local)
[ ] 6. Boot log kontrol (hash OK?)
[ ] 7. Player test (item visible?)
[ ] 8. Prod sunucuya deploy (104.238.23.99 — çift onay)
[ ] 9. Prod boot log (hash OK?)
[ ] 10. Player notice (restart warning)

---

## Güvenlik Implikasyonları

### Hash = Anti-Cheat
- Client'te modded TBL (item damage +999%) → hash mismatch → server reject
- **Server-side validation → cheating prevent**

### Vulnerable Scenario
- Admin ha hash bypass → TBL tamper possible (ama boot fail)
- **Mitigation:** Admin access control, audit log, backup policy

### Best Practice
1. Tüm TBL değişiklikleri log et (git + orkestra DB)
2. Backup mandatory (restore für rollback)
3. Deploy approval (DOKTOR + Erencan)
4. Boot test before prod

---

## Kaynak Referansları

- `GameServer.ini` — [TBL_HASH] sektion
- `tools\tbl\tbl_hash.py` — MD5 calculator
- `tools\tbl\tbl_decrypt.py` — Decrypt + validate
- `tools\tbl\tbl_edit.py` — Edit + re-encrypt
- Boot log: `Server\Logs\*.log`
- Memory: TBL edit workflow (S84-S87)

---

## Örnek: Item.tbl Update (tam flow)

**Hedef:** Demon Sword damage +100 → +150

```bash
# Step 1: Backup
cp C:\MalaysiaKO\Data\Item.tbl C:\MalaysiaKO\Data\Item.tbl.backup_S87_29april

# Step 2: Decrypt
python tools/tbl/tbl_decrypt.py Item.tbl Item.json

# Step 3: Edit (manual JSON editor veya Python)
# Item ID = 123 (Demon Sword), iDamage = 100 → 150

# Step 4: Encrypt
python tools/tbl/tbl_edit.py Item.json Item.tbl

# Step 5: Hash
python tools/tbl/tbl_hash.py Item.tbl
# NEW: a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6

# Step 6: INI Update (Notepad)
# GameServer.ini [TBL_HASH]
# ITEM_ORG=a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6

# Step 7: Test Boot
# GameServer.exe → observe log
# [INFO] Item.tbl loaded, hash verified ✓

# Step 8: In-game Test
# /item 123 → check damage 150 ✓

# Step 9: Deploy (if prod)
# scp Item.tbl 104.238.23.99:C:/MalaysiaKO/Data/
# scp GameServer.ini 104.238.23.99:C:/Server/
# Restart server on prod

# Step 10: Commit
# git add Item.tbl GameServer.ini
# git commit -m "fix(item): Demon Sword damage 100->150 (hash a1b2c3d4)"
```

---

**Son Güncelleme:** 2026-04-29 | **Versiyon:** 1.0 | **MATRIX**
