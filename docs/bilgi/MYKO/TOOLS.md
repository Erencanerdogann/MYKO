# Tools Envanter — MYKO Şifreleme + Araçları

Versiyon: 2.1 | Tarih: 29 Nisan 2026 | Detay Seviyesi: KAPSAMLI

---

## TBL Araçları (Referans — MATRIX Kullanır)

| Tool | Amaç | Bağımlılık | Konum |
|------|------|-----------|-------|
| tbl_decrypt.py | DES + K2 XOR + K1 çöz | Python 3.8+ | tools/tbl/ |
| tbl_edit.py | Satır düzenle (yedek alır) | Python | tools/tbl/ |
| tbl_edit_v2.py | V2 edit (geliştirilmiş) | Python | tools/tbl/ |
| tbl_compare.py | İki .tbl karşılaştır | Python | tools/tbl/ |
| tbl_fix_encoding.py | Encoding düzelt | Python | tools/tbl/ |
| tbl_fix_acs.py | ACS field fix | Python | tools/tbl/ |
| tbl_scan_all.py | Toplu tarama | Python | tools/tbl/ |

### Kullanım
```bash
python tbl_decrypt.py "Data/Item.tbl"
python tbl_edit.py "Data/Item.tbl" --row=100 --col=2 --value="new_value"
python tbl_compare.py "Data/Item.tbl" "Data/Item_backup.tbl"
```

**Detay:** TBL tools Python 3.8+ ve pycryptodome ile çalışır. DES+K2 XOR decrypt. CSV-like çıktı. Türkçe encoding sorunları olabilir (UTF-8 vs CP1254).

---

## UIF Araçları (UI Dosyaları)

| Tool | Amaç | Dil | Konum |
|------|------|-----|-------|
| Uif-Decryptor | .xcurse → .uif | C++ (VS 2022) | tools/Uif-Decryptor/ |
| Uif-Encryptor | .uif → .xcurse | C++ (VS 2022) | tools/Uif-Encryptor/ |

### Spesifikasyon
- **Input:** Encrypted .xcurse (RC4 MYKO + xorstr obfuscation)
- **Output:** Plain .uif (XML-like UI definition)
- **Build:** Visual Studio 2022, Windows CryptoAPI
- **Dependencies:** windows.h, crypto.h

### Kullanım
```bash
Uif-Decryptor.exe "CodeGuard/Code/UIBasicGroup_us.xcurse" "output.uif"
Uif-Encryptor.exe "plain.uif" "CodeGuard/Code/UIBasicGroup_us.xcurse"
```

**Detay:** C++ VS 2022 ile build. RC4 key (SHA-1 derivation). xorstr runtime decrypt. Windows CryptoAPI.

---

## Key Rotation Araçları

| Tool | Amaç | Çıktı | Sıra |
|------|------|-------|------|
| key_generator.py | 6 sistem key üret | MYKO_NEW_KEYS.md | 1 |
| rc4_re_encrypt.py | .code re-encrypt | Patched .code files | 2 |
| tbl_re_encrypt.py | .tbl re-encrypt | Patched .tbl files | 3 |
| exe_key_patcher.py | Client EXE patch | Patched KnightOnline.exe | 4 |
| src_encrypt.py | Source string obfuscate | xorstr encrypted | 5 |

### Konum
```
tools/key_rotation/
├── key_generator.py
├── rc4_re_encrypt.py
├── tbl_re_encrypt.py
├── exe_key_patcher.py
├── src_encrypt.py
└── MYKO_NEW_KEYS.md (OUTPUT, GİZLİ)
```

**Detay:** KEY_ROTATION.md bkz. § 11 tool detay kartları.

---

## Deployment Araçları

| Tool | Amaç | Kapsam | Konum |
|------|------|--------|-------|
| patch_tool.py | SSH + MSSQL deploy | GameServer updates | tools/ |
| build_xlsx.py | Balance sheet export | Item/Skill stats | tools/ |
| item_search.py | Item arama | Itemid/Name lookup | tools/ |

### patch_tool.py (KODCU Kullanır)
```bash
python patch_tool.py --host=104.238.23.99 --user=sa --password=*** --file=Data/Item.tbl
```

⚠️ SSH credentials içerir, secure storage'da tutulmalı

**Detay:** SSH (paramiko), MSSQL (pyodbc). Production update. Credentials config file'da.

---

## UI Editor'ler (Desktop)

| Tool | Amaç | Dosya |
|------|------|-------|
| UIE.exe | UIF visual editor | .uif → drag&drop |
| N3TexViewerPNG.exe | Texture preview | .ntf → PNG viewer |
| DXT Converter | DXT2-5 → PNG | Texture utility |

**Konum:** `Desktop\Server\` (lokal dev environment)

**Detay:** UIE.exe Windows GUI. Drag&drop UIF editing. N3TexViewerPNG.exe texture preview (DXT2-5 decode).

---

## Setup & Distribution

**Dosya:** `setup/myko_setup_v3.iss`  
**Tool:** Inno Setup 6.2+  
**Çıktı:** Installer EXE (Türkçe UI)  
**Target:** `C:\MalaysiaKO\`

### Aşamalar
1. Client files (EXE, DLL, .tbl, .code)
2. Mod handler (Moonlight)
3. Registry keys
4. Shortcut

**Detay:** Inno Setup ISS script. Türkçe localization. Moonlight mod support. Reg key kurulum.

---

## Diğer Araçlar

| Tool | Amaç | Bağımlılık |
|------|------|-----------|
| build_xlsx.py | Item/Skill balance sheet | Python, openpyxl |
| item_search.py | ItemID ↔ Name lookup | Python |
| make_shortcut.ps1 | Desktop shortcut creator | PowerShell |

**Detay:** build_xlsx.py tbl_decrypt çıktısı → Excel. item_search.py lokal index. make_shortcut.ps1 .lnk oluştur.

---

## Tools Arası Akış (MASTER DIAGRAM)

```
[Source Code]
    ↓ src_encrypt.py (xorstr obfuscate)
[Encrypted Source]
    ↓ VS Build (msbuild)
[GameServer.exe + DLL]
    ↓
[.code Dosyaları] ← rc4_re_encrypt.py (key rotation)
[.tbl Dosyaları] ← tbl_re_encrypt.py (DES+K2+K1)
    ↓
[Data/ + CodeGuard/ Assets]
    ↓
[key_generator.py → MYKO_NEW_KEYS.md]
    ↓ exe_key_patcher.py (binary patch)
[KnightOnline.exe (client)]
    ↓
[patch_tool.py] ← SSH/MSSQL deploy
    ↓
[Production Server]
    ↓
[Client auto-update]
    ↓
[myko_setup_v3.iss] ← installer
    ↓
[C:\MalaysiaKO\]
```

---

## USE CASE'LER

### Case 1: Item Ekle (Item.tbl Düzenle)

**Amaç:** Yeni item ID 99999 ekle, Level=50, Damage=100

**Adımlar:**
```
1. tbl_decrypt.py Data/Item.tbl
   → plaintext Item.tbl.dec (CSV-like)
2. Excel'de aç, satır ekle: 99999,Legendary Sword,50,100,...
3. tbl_edit.py Data/Item.tbl --row=9999 --col=2 --value="Legendary Sword"
4. Yedek: Item.tbl → Item_backup.tbl
5. tbl_re_encrypt.py (yeni key varsa)
6. TBL_HASH.md güncelle
7. Server restart
8. Test: /item 99999
```

**Timing:** 10 dakika
**Risk:** Encoding, satır format, duplicate ID

---

### Case 2: UI Değiştir (UIBasicGroup_us.xcurse)

**Amaç:** Login window rengini değiştir

**Adımlar:**
```
1. Uif-Decryptor.exe UIBasicGroup_us.xcurse output.uif
   → Plaintext XML-like
2. UIE.exe output.uif → drag&drop color change
3. Save
4. Uif-Encryptor.exe output.uif UIBasicGroup_us.xcurse
   → Encrypted .xcurse
5. Client update
6. Test: login screen color OK?
```

**Timing:** 15 dakika
**Risk:** Element ID consistency, xorstr obfuscation

---

### Case 3: Patch Yayınla (Oyuncu Update)

**Amaç:** Yeni GameServer.exe + Data/*.tbl + .code push

**Adımlar:**
```
1. Source code değiştir (bug fix veya feature)
2. src_encrypt.py (secret string'ler)
3. VS Build → GameServer.exe
4. rc4_re_encrypt.py (key rotation gerekirse)
5. tbl_re_encrypt.py (key rotation gerekirse)
6. patch_tool.py deploy (SSH + MSSQL)
7. Client side: myko_setup_v3.iss → patch yayınla
8. Test: 10 client bağlan, smooth?
```

**Timing:** 1-2 saat
**Risk:** Client sync (old client + new server = incompatible)

---

### Case 4: Key Rotation (6 Sistem Yenile)

**Amaç:** Periyodik key change (güvenlik)

**Adımlar:** KEY_ROTATION.md § 12 scenarios bkz.

**Timing:** 60-90 dk downtime
**Risk:** Client/Server key mismatch

---

## HATA / TUZAK LİSTESİ

| Hata | Sebep | Çözüm |
|------|-------|-------|
| tbl_decrypt: Encoding error | Türkçe char | iconv veya Python encoding set |
| tbl_edit: Satır offset | TBL format | TBL_HASH.md kontrol, tbl_scan_all.py |
| Uif-Decryptor: x86 crash | x64 only | VS 2022 64-bit build |
| patch_tool: SSH timeout | Network | Host/port/key kontrol |
| exe_patcher: offset mismatch | Version | --version parameterini doğrula |

---

## GELECEK İYİLEŞTİRMELER

- TBL editor GUI (myko-panel web entegrasyon)
- Tool unit testleri (pytest)
- CI/CD pipeline (GitHub Actions SSH deploy)
- Key rotation automation (cron job)
- Telemetry logging (tool.log)

---

**Bynoisee © MalaysiaKO 2026 — Tools v2.1**
