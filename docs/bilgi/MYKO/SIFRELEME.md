# 6 Katman Şifreleme Mimarisi — MYKO Knight Online

**Versiyon:** 2.1  
**Tarih:** 29 Nisan 2026  
**Kapsam:** Tüm şifreleme katmanları — paket, kod, dokular, veritabanı dosyaları

---

## 🏗️ Genel Mimari

MYKO sunucusu 6 katmanlı şifreleme kullanır:

```
         Client (KnightOnline.exe)
              ↓ / ↑
    [1] JvCryption (network paket)
              ↓ / ↑
         Login/Game Server
              ↓
    [2] RC4 MYKO (.code dosyaları)
    [3] RC4 NTF (texture dosyaları)
    [4] K2 XOR (TBL katman 2)
    [5] K1 XOR (TBL katman 1)
    [6] DES Feistel (TBL katman 0)
              ↓
      Disk Dosyaları
      (Data/*.tbl)
```

---

## 📡 Katman 1: JvCryption (Network Paketleri)

**Amaç:** Client ↔ Login Server haberleşmesini şifrele  
**Dosya:** `shared/JvCryption.cpp:6`  
**Algoritma:** Custom XOR stream cipher (simetrik)

### Parametreler
```
Private Key:  0x1207500120128966 (uint64, hardcoded)
Seed:         2157 (uint16)
Multiplier:   2171 (uint16)
Length-Key:   (packet_length × 157) & 0xFF
Magic:        0xAA55 (header), 0x55AA (tail)
Signature:    0x1EFC (decrypted packet header)
```

### Şifreleme Süreci

1. **Handshake (Login Server):**
   - Paket tipi: `LS_CRYPTION (0xF2)`
   - Server → Client: 8-byte public key
   - Session key = publicKey XOR 0x1207500120128966

2. **Paket Şifreleme:**
   ```
   out[i] = ((in[i] XOR rsk) XOR pkey[i%8]) XOR lkey
   ```

3. **Framing:**
   - Header: 0xAA55
   - Tail: 0x55AA
   - CRC-32 doğrulaması (paket bütünlüğü)

### Güvenlik Değerlendirmesi

| Zayıflık | Ciddiye | Açıklama |
|----------|---------|----------|
| Hardcoded key | ⚠️ ORTA | Private server için beklenen, KO topluluğu bilinyor |
| Custom XOR | ⚠️ ORTA | Stream cipher, hızlı ama kriptografik olarak zayıf |
| Replay attack riski | 🔴 YÜKSEK | Session key statik, tekerrür engellemesi yok |

---

## 🔐 Katman 2: RC4 MYKO (.code Dosyaları)

**Amaç:** Oyun kodunu şifrele (UI script, logic)  
**Dosya:** `N3Base/N3BaseFileAccess.cpp:24-25`  
**Kullanım:** CodeGuard\Code\*.code, .uif dosyaları, zone, shape  
**Algoritma:** RC4 (Windows CryptoAPI, 128-bit)

### Parametreler
```
Key:           [GİZLİ]
Hash Length:   29 byte
Block Size:    4096 byte
WinAPI:        CryptoAPI + SHA-1
```

### Şifreleme Süreci

1. **Anahtar Türetme:** Windows CryptDeriveKey (SHA-1)
2. **Dosya Yapısı:**
   ```
   [0-3]    Şifrelenmemiş (4 byte)
   [4-end]  4096-byte bloklar halinde RC4 decrypt
   ```

---

## 🎨 Katman 3: RC4 NTF (Texture Dosyaları)

**Amaç:** Texture dosyalarını şifrele  
**Dosya:** `N3Base/WinCrypt.h:10`  
**Kullanım:** NTF dosyaları (version = 7)  
**Algoritma:** RC4 (Windows CryptoAPI, 128-bit)

### Parametreler
```
Key:            [GİZLİ]
Magic:          "NTF" + version (7 = RC4)
```

---

## 🔢 Katman 4: K2 XOR Stream Cipher (TBL Layer 2)

**Amaç:** .tbl dosyalarının ikinci şifreleme katmanı  
**Dosya:** `N3Base/N3TableBaseImpl.cpp:229-231`  
**Uygulama Sırası:** DES decrypt SONRASI

### Parametreler
```
Seed:       0x0418
Multiplier: 0x8041
Addend:     0x1804
Skip:       5 byte (DES sonrası)
```

---

## 🔑 Katman 5: K1 XOR Stream Cipher (OpenKO Orijinal)

**Amaç:** OpenKO TBL dosyalarının şifrelenmesi  
**Dosya:** `N3Base/N3TableBaseImpl.cpp:136-138`  
**Uygulama Sırası:** Eğer DES magic yoksa K1 dene

### Parametreler
```
Seed:       0x0816
Multiplier: 0x6081
Addend:     0x1608
```

---

## 🔐 Katman 6: DES Feistel (TBL Layer 0 — En Dış)

**Amaç:** .tbl dosyalarının birincil şifrelenmesi (MYKO custom)  
**Dosya:** `N3Base/N3TableBaseImpl.cpp:18-119`  
**Algoritma:** 16-round custom Feistel cipher

### Magic Header (Dosya Tanıma)
```
4C 26 43 7F 80 F1 57 98 79 FC AF 26 86 D6 20 8E
```

### Dosya Yapısı
```
[0-15]           Magic header (16 byte)
[16-19]          Orijinal dosya boyutu (big-endian)
[20-end]         Encrypted payload (8-byte bloklar)
```

### Key Schedule
- `s_secret1[]` — 48 × uint16 (Round key'leri, hardcoded)
- `s_secret2[]` — 48 × int32 (Bit expansion)
- `s_secret3[]` — 32 × int32 (Permutation)
- `s_secretArrays[8][64]` — S-box tabloları (uint32)

### Güvenlik Değerlendirmesi

| Özellik | Durum | Açıklama |
|---------|-------|----------|
| Round sayısı | 16 | DES ile aynı |
| Custom S-box | ✅ | MYKO kendi tabloları |
| Key reuse | ⚠️ | Sabit hardcoded |
| Brute force | 🔴 YÜKSEK | Reverse mühendislik mümkün |

---

## 📊 Dosya → Katmanlar Haritası

| Dosya Tipi | Katmanlar | Sıra | Açıklama |
|------------|-----------|------|----------|
| .tbl | DES + K2 + K1 | DES → K2 XOR → K1 XOR | Veritabanı tabloları |
| .code | RC4 MYKO | RC4 | Oyun mantığı |
| .ntf | RC4 NTF | RC4 | Doku dosyaları |
| .n3anim | DES-only | DES | Animasyon |
| Network paket | JvCryption | XOR | Client ↔ Server |

---

## 🛠️ Şifre Çöztme Araçları

```bash
# TBL dosyası şifre çöz
python tools/tbl/tbl_decrypt.py "Data/Item.tbl"

# RC4 MYKO yeniden şifrele
python tools/key_rotation/rc4_re_encrypt.py

# UIF decrypt/encrypt
./Uif-Decryptor/Uif-Decryptor.exe input.xcurse output.uif
```

---

## ⚠️ Gerçek Anahtar Değerleri

**NOT:** Bu dokümanda anahtar değerleri **[GİZLİ]** belirtilmiştir.

Kaynak:
- `C:\temp\MYKO\tools\key_rotation\MYKO_NEW_KEYS.md` (sınırlı erişim)

**Güvenlik:** Anahtar değerleri şifreli repo'da saklanmalı, koddan ayrılmalı.

---

## 📚 Referanslar

- F:\MDBACKUP\C--Projects_memory\tools\decrypt_keys.md — Detaylı analiz
- C:\temp\MYKO\tools\key_rotation\ — Python decrypt toolları
