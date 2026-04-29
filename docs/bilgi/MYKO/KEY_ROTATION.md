# Key Rotation Pipeline

Versiyon: 3.1 | Tarih: 29 Nisan 2026 | Detay Seviyesi: KAPSAMLI

---

## 1. AMAÇ VE NE ZAMAN YAPILIR

Cryptographic key rotation, MYKO'daki 6 şifreleme sisteminin anahtarlarını periyodik veya acil durumlarda güvenli şekilde değiştirmedir.

### Tetikleyiciler

- **Periyodik:** Yıl başında veya majör release'de
- **Acil:** Anahtarın sızıntı şüphesi, developer çıkışı
- **Son Rotation:** 2026-03-23

### Neden Önemli

- JvCryption sızarsa → paket trafiği decrypt
- .code sızarsa → AI logic reverse-engineer
- TBL sızarsa → item/skill müdahale

---

## 2. 6 SISTEM ÖZET

| Sistem | Konum | Dosya | Sıra |
|--------|-------|-------|------|
| JvCryption | Client packet | Network | 1 |
| RC4 MYKO | CodeGuard\Code\*.code | 100+ | 2 |
| RC4 NTF | Data\*.ntf | 50-80 | 3 |
| K2 XOR | Data\*.tbl Layer2 | 246 | 4 |
| K1 XOR | Legacy OpenKO | ~50 | 5 |
| DES s_secret1 | TBL wrapper | 246 | 6 |

---

## 3. ROTATION ADIMLARI

### Adım 1-6: Tool Pipeline

1. key_generator.py → MYKO_NEW_KEYS.md
2. rc4_re_encrypt.py → .code yenile
3. tbl_re_encrypt.py → .tbl yenile
4. GameServer compile
5. exe_key_patcher.py → KnightOnline.exe patch
6. Deploy + test

---

## 11. TOOL DETAY KARTLARI

### key_generator.py

- Dosya: C:	emp\MYKO	ools\key_rotation\key_generator.py
- Bağımlılık: Python 3.8+, secrets
- Parametreler: --type (jv|rc4|k2|des), --output
- Çalıştırma: python key_generator.py
- Çıktı: MYKO_NEW_KEYS.md
- ⚠️ GİZLİ, GitHub YASAK
- OLD_KEYS dict (lines 23-35) eski key'leri bilir

---

### rc4_re_encrypt.py

- Dosya: C:	emp\MYKO	ools\key_rotationc4_re_encrypt.py
- Bağımlılık: Python, hashlib
- Dosyalar: CodeGuard\Code\*.code (~100-120)
- Parametreler: --test, --dir, --old-key, --new-key, --dry-run
- Timing: 100 dosya = 5-10 dk
- Backup: Otomatik *.code.bak
- Algoritma: SHA-1 → RC4 128-bit

Workflow: Decrypt → CRC → Encrypt(new key) → Save

---

### tbl_re_encrypt.py

- Dosya: C:	emp\MYKO	ools\key_rotation	bl_re_encrypt.py
- Dosyalar: Data\*.tbl (246 dosya)
- Parametreler: --test, --dir, --dry-run, --new-keys JSON
- Timing: 246 × 3 = 738 işlem ≈ 15-30 dk
- Sıra: DES → K2 XOR → K1 XOR

Encryption:
- K1 XOR (seed=0x0816, mult=0x6081, add=0x1608)
- K2 XOR (seed=0x0418, mult=0x8041, add=0x1804)
- DES Feistel (s_secret1 = 48×uint16, 16 round)

JSON: s_secret1 array + k2/k1 params

---

### exe_key_patcher.py

- Dosya: C:	emp\MYKO	ools\key_rotation\exe_key_patcher.py
- Target: KnightOnline.exe, code.guard DLL
- Parametreler: --exe, --scan, --patch-jv, --patch-rc4, --patch-k2, --patch-des
- Backup: Otomatik *.exe.bak
- Version-Specific: v2369/v1098 offset farklı

Workflow:
1. --scan (pattern ara, değişiklik YOK)
2. Match=1 ise --patch (yeni key yaz)
3. PE checksum update
4. Rollback: .bak dosyasından

⚠️ Themida'lı exe'de key bulunamaz. Offset yanlışsa crash.

---

### src_encrypt.py

- Dosya: C:	emp\MYKO	ools\key_rotation\src_encrypt.py
- Amaç: C++ string xorstr ile encrypt
- Bağımlılık: xorstr C++ header

---

## 12. SENARYO ÖRNEKLERI

### JvCryption (Client+Server)

1. key_generator.py → yeni key
2. exe_key_patcher.py --scan
3. exe_key_patcher.py --patch-jv
4. GameServer source update
5. Compile
6. DEV test
7. Production: client+server aynı anda

Yanlışsa: Client yeni, Server eski → decrypt fail → drop

---

### Toplu (6 Sistem)

1. key_generator.py → MYKO_NEW_KEYS.md
2. rc4_re_encrypt.py → .code yenile
3. tbl_re_encrypt.py → .tbl yenile
4. GameServer compile
5. exe_key_patcher.py → KnightOnline.exe
6. Production: Server + Client + Test 24h

Timing: ~60-90 dk downtime

---

## 13. ERROR HANDLING

| Hata | Sebep | Çözüm |
|------|-------|-------|
| tbl: Invalid magic | DES header | Old keys kontrol |
| rc4: File lock | Process açık | Close → tekrar |
| exe: Multiple matches | Fuzzy binary | Offset elle veya --version |
| exe: Offset mismatch | Version yanlış | Version parameterini doğrula |

---

**Bynoisee © MalaysiaKO 2026 — Key Rotation v3.1**
