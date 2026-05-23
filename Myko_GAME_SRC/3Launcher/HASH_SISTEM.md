# Launcher Hash Check Sistemi (S113 — 2026-05-23)

## Amac
Patch indirme sirasinda **MITM saldirilarina karsi koruma**.
Indirilen zip dosyasinin MD5 hash'i sunucudan gelen hash ile karsilastirilir, eslesmezse reddedilir.

---

## Mimari

### Akis
```
1. Launcher LoginServer'a `LS_DOWNLOADINFO_REQ` (opcode 0x02) gonderir
2. LoginServer DB'den VERSION tablosunu okur (sVersion, strFilename, strFileHash)
3. LoginServer paketi gonderir: [ftpURL][ftpPATH][fileCount][filename1][hash1][filename2][hash2]...
4. Launcher her dosya icin:
   a) HTTP/80 ile indir
   b) MD5 hesapla
   c) Sunucudan gelen hash ile karsilastir
   d) Eslesirse: extract et, Server.ini guncelle
   e) Eslesmezse: dosyayi sil, return false (patch zinciri kirilir)
```

### Geri donus uyumlu mod
DB'de `strFileHash` NULL/bos ise:
- LoginServer `ISNULL(..., '')` ile bos string gonderir
- Launcher `if (!expectedHash.empty())` ile **atlar** → hash check devre disi
- Patch yine indirilir (koruma yok, ama calisir)

Bu sayede hash check **opsiyonel** — eski DB'lerle uyumlu.

---

## Kod Lokasyonu

### `LauncherEngine.h:22` — Fonksiyon declaration
```cpp
bool DownloadPatch(std::string server, std::string path, std::string file, std::string expectedHash);
```

### `LauncherEngine.cpp:1-2` — Include
```cpp
#include "LauncherEngine.h"
#include "MD5.h"
```

### `LauncherEngine.cpp:259+` — DownloadPatch fonksiyonu
```cpp
bool Launcher::DownloadPatch(std::string server, std::string path, std::string file, std::string expectedHash)
{
    // ... mevcut download mantigi ...
    FTPClient.DownloadFile(file, path + "/" + file);
    FTPClient.CleanupSession();

    // MD5 hash dogrulama — DB hash bos olabilir, atla (geri donus uyumlu)
    if (!expectedHash.empty())
    {
        MD5 md5check;
        std::string fileHash = md5check.FileMD5Check(file.c_str());
        if (fileHash != expectedHash)
        {
            SetState(xorstr("Patch hash mismatch: ") + file);
            std::remove(file.c_str());
            return false;
        }
    }
    // ... extract, version update ...
}
```

### `LauncherEngine.cpp:367` (HandlePacket case 0x2) — Paket parse
```cpp
for (int i = 0; i < fileCount; i++) {
    std::string file, fileHash;
    pkt >> file >> fileHash;
    DownloadPatch(ftpURL, ftpPATH, file, fileHash);
}
```

---

## DB Tablo (KO_MYKO.VERSION)

| Sutun | Tip | Aciklama |
|---|---|---|
| sVersion | smallint | Patch numarasi (2370, 2371, ...) |
| sHistoryVersion | smallint | Geriye uyumlu version |
| strFilename | char | "2370.zip" gibi |
| **strFileHash** | nvarchar | **MD5 hex string (32 karakter), NULL ise atla** |

### Hash doldurma (SQL)
```sql
UPDATE VERSION SET strFileHash = '<32-char-hex>' WHERE sVersion = <n>;
```

Hash hesaplama (sunucu PowerShell):
```powershell
Get-FileHash 'C:\Users\Administrator\Desktop\Server\patch\2373.zip' -Algorithm MD5
```

---

## LoginServer Tarafi

### Protokol
`LoginSession.cpp:189-225` (HandlePatches)
```cpp
result << uint16(downloadset.size());
for (const auto& entry : downloadset) {
    result << entry.filename << entry.hash;  // hash dolu veya bos string
}
```

### KORU
- Hash kismi her zaman gonderilir (eski/yeni Launcher fark etmez)
- Eski Launcher uyumsuz olur (parse fail) — KASIT, eski Launcher'lar guncellenmeli

---

## Build

### Toolset
- **Visual Studio 2022 v143**
- Configuration: **Debug | x86** (musteridekiyle uyumlu)
- DirectX SDK: June 2010 — `$(DXSDK_DIR)Lib\x86`

### Komut
```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe" \
  "C:\temp\MYKO\Myko_GAME_SRC\3Launcher\Launcher.sln" \
  -p:Configuration=Debug -p:Platform=x86 -p:PlatformToolset=v143 -t:Rebuild -m
```

### Cikti
`C:\temp\MYKO\Myko_GAME_SRC\3Launcher\Win32\Debug\Launcher.exe` (~3.9 MB)

### Warning suppression (vcxproj icinde)
```xml
<ClCompile>
    <DisableSpecificWarnings>4244</DisableSpecificWarnings>
</ClCompile>
<Link>
    <IgnoreSpecificDefaultLibraries>LIBCMT</IgnoreSpecificDefaultLibraries>
</Link>
```

---

## Test

### Mevcut DB durumu (NULL hash)
- Yeni Launcher acilir, patch'leri indirir, hash atlar, ext eder, START gelir ✅
- Test edilmis: 2026-05-23 patron tarafindan dogrulandi

### Hash dolu testi (henuz yapilmadi)
- DB'ye gercek MD5'ler yazilacak
- Yeni Launcher patch'i indirip hash karsilastiracak
- Bozuk patch (MITM simule) → hash mismatch → `Patch hash mismatch:` mesaji + extract iptal

---

## .code Dosyalari (Launcher Gorselleri)

Launcher acilirken `C:\MalaysiaKO\CodeGuard\Launcher\` icinden 12 `.code` dosyasi okur:
- `Background.code`
- `StartMouseOut/Over/Click.code`
- `OptionsMouseOut/Over/Click.code`
- `CloseMouseOut/Over/Click.code`
- `ProgressEmpty.code`
- `ProgressValue.code`

`.code` = `D3DXCreateTextureFromFileEx` ile yuklenen sifrelenmis PNG (decrypt routine `Launcher.cpp` icinde).

### UIXSettings.ini
Pozisyon/boyut ayarlari: `C:\MalaysiaKO\CodeGuard\Launcher\UIXSettings.ini`

---

## Onemli Notlar (S113 dersleri)

1. **Bizim eski 3Launcher source** (ByNoisee PNG versiyon) **musteri Launcher DEGIL** — Cyber Hook source uzerine yazildi (commit `b4854cf`)
2. **Eski musteri Launcher** (4.8 MB) hash check **YOK** — yeni LoginServer paketini parse edemez (uyumsuz) — guncellenmeli
3. **Patch ile Launcher.exe degistirmek** mumkun degil (file lock) — Setup EXE veya helper script gerek
4. **Hash NULL durumu** koruyucu fallback — production'a alirken sirayla doldurulabilir
5. **MITM korumasi** sadece patch dosyasi icin — `code.guard` runtime tamper icin AYRI koruma

---

## Tarih ve Imza
- **Eklenme:** 2026-05-23 (CHIP, S113)
- **Brief:** MSG:5692 (DOKTOR)
- **Commit:** `b4854cf`
- **Source bulunmasi:** patron Erencan, `C:\Users\erenc\Desktop\11.06.2024 SRC\3Launcher\` yedeginden
- **Test:** 2026-05-23 patron dogrulamasi (calisir)
