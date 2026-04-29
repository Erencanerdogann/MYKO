# 💿 CLIENT SETUP — Oyuncu Kurulum Rehberi

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `C:\MalaysiaKO\` (kurulu client), `Server.ini`, `Path.Ini`, `Launcher.exe`, `KnightOnline.exe`
**Hedef:** Oyuncunun kurulum akışı, dosya envanteri, sorun çözüm.

---

## 1. CLIENT YAPISI

### Ana Dizin: `C:\MalaysiaKO\`
```
KnightOnline.exe          ← Ana oyun motoru (~30-50 MB)
Launcher.exe              ← Patch + start launcher
Option.exe                ← Grafik/ayar exe
KscViewer.exe             ← Screenshot viewer (.ksc)
Apr_Show.dll              ← Anti-cheat / DLL injection
KOFPL.dll                 ← FPS/perf DLL
OpenAL32.dll              ← Ses
dsetup.dll                ← DirectX setup
unins000.exe              ← Uninstall

cg_crash.log              ← Code Guard crash log

Server.ini                ← Sunucu IP/port
Option.ini                ← Oyuncu grafik ayar
Path.Ini                  ← Asset yol
Scheduler.ini             ← Event takvim (war data)

CodeGuard/                ← Anti-cheat klasörü
4code.guard, 5code.guard  ← AC config dosyaları

Chr/                      ← Karakter modelleri (n3chr)
ChrSelect/                ← Karakter seçim ekran
ByNo/                     ← Logo/UI özel
DTex/                     ← Diffuse textures
Data/                     ← .tbl şifreli data
FD/                       ← (effects?)
Intro/                    ← Intro video
Item/                     ← Item modelleri (n3item)
KnightMovie/              ← Cutscene
Misc/                     ← Çeşitli
Object/                   ← Static obje
SHC/                      ← Shader cache
Scene/                    ← Sahne
Snd/                      ← Ses dosyaları
UI/                       ← Arayüz
Zones/                    ← Harita .smd
```

⚠️ **Tam liste:** `CLIENT_HARITA.md`

---

## 2. SERVER.INI (Bağlantı Config)

```ini
[Server]
Count=1
IP0=104.238.23.99       ← Production IP
Port0=15100             ← LoginServer port

[Version]
Files=2377              ⚠️ Lokal client 2377 (üst version)

[IncludeExe]
Last=2368               ⚠️ Patch incremental 2368'e kadar

[XignCode]
Error=0
```

✅ **VERSIYON ZİNCİRİ AÇIKLAMA (Patron notu — S88):**

İki ayrı sayaç var, çakışma DEĞİL:

| Sayaç | Değer | Anlam |
|-------|-------|-------|
| **Client patch katmanı** (`Server.ini Files`) | **2377** | 2369 base üzerine giydirme patch'lerle 2377'ye geldi |
| **Server kod / UI** (`Define.h`) | **1098** | Oyun mekanik + UI giydirme versiyonu (1098 stili) |

**Mantık:**
- 2369 = orijinal client base
- 8 patch katmanı giydirildi → 2377
- Üzerine 1098 UI/feature stili
- Server `1098` ≠ Client `2377` ÇAKIŞMA YOK — farklı boyutlar

⚠️ **`Server.ini Last=2368`** = Last patch versiyonu (incremental zincir başlangıç noktası).

⚠️ **Geçmiş "patlayan proje" mismatch (`PROJE_TARIHCESI § AŞAMA 3`)** = farklı durum: orada **3 farklı kaynaktan 3 versiyon karışık** (server 1098, client 2369, release 1124) → paket protokol uyumsuzluğu. Bizimki **tek zincir** patch giydirmesi, sağlıklı.

---

## 3. INSTALL AKIŞI (Oyuncu Tarafı)

### A) Site'den İndirme
```
1. malasiako.com → "İndir" butonu
2. MalaysiaKO_Setup.exe (~5 GB) indirilir
   VEYA
   MalaysiaKO_Client.rar (split parts)
3. Setup çalıştır → C:\MalaysiaKO\ kuruluma
4. Masaüstü kısayol oluşur
```

### B) Manuel Kurulum
```
1. .rar dosyasını indir (5 GB)
2. C:\MalaysiaKO\ klasörüne extract
3. Server.ini içeriği doğrula (IP=104.238.23.99)
4. Launcher.exe çalıştır
```

### C) İlk Çalıştırma (Patch Akışı)
```
Launcher.exe açılır
   ↓
HTTP request → http://104.238.23.99:80/patch/version.txt
   ↓
Yerel Files=2377 vs sunucu Files=2378 (örnek)
   ↓
2378.zip indir → C:\MalaysiaKO\ extract → Server.ini güncelle
   ↓
Launcher → KnightOnline.exe çağırır
   ↓
Oyun açılır → karakter seçim → giriş
```

⚠️ **Patch indirme `port 80`** üzerinde — `patch_server.js` Node service.

---

## 4. PATCH SİSTEMİ (Client Tarafı)

### Patch İndirme Akışı (Launcher detay)
```
1. Launcher → version.txt indir (sunucu)
2. Local Files vs Sunucu Files karşılaştır
3. Eksik patchler:
   - 2376.zip, 2377.zip, 2378.zip (ardışık)
4. Her zip:
   - İndir
   - Decrypt (RC4 MYKO key — `SIFRELEME.md`)
   - Extract → C:\MalaysiaKO\ overwrite
5. Server.ini Version.Files güncelle
6. KnightOnline.exe çalıştır
```

### Patch İçeriği
- `.tbl` (şifreli data — Item, Magic, Quest)
- `.code` (UI script — RC4 MYKO)
- `.uif` (UI tema — şifreli)
- `.n3*` (3D asset — model, doku, anim)
- `.smd` (zone/harita)

---

## 5. SİSTEM GEREKSİNİMLERİ

### Minimum
| Bileşen | Şart |
|---------|------|
| OS | Windows 10 (64-bit) |
| CPU | Intel Core 2 Duo / AMD Athlon II |
| RAM | 2 GB |
| GPU | DirectX 9 uyumlu, 512 MB |
| Disk | 5 GB free |
| Network | 1 Mbps |

### Önerilen
| Bileşen | Şart |
|---------|------|
| OS | Windows 10/11 64-bit |
| CPU | i5 / Ryzen 5 |
| RAM | 8 GB |
| GPU | GTX 1050 / RX 560 |
| Disk | 10 GB SSD |
| Network | 10 Mbps |

⚠️ **Anti-cheat (Code Guard)** Win 7/8 desteği belirsiz — Win 10+ zorunlu.

---

## 6. OPTION.INI (Oyuncu Grafik Ayar)

```ini
[Display]
Width=1920
Height=1080
ColorDepth=32
Windowed=0       ← 1=pencereli, 0=tam ekran

[Graphics]
TextureQuality=High
ShadowQuality=Medium
AntiAlias=2x

[Sound]
SoundVolume=80
MusicVolume=60

[Network]
LowBandwidth=0
```

⚠️ Windowed mode = 1 → bazı anti-cheat tetikleyebilir. Test et.

---

## 7. SIK KARŞILAŞAN HATALAR

### A) "Cannot connect to server"
**Sebep:** IP/Port yanlış, firewall, server down
**Çözüm:**
1. `Server.ini` IP=104.238.23.99, Port=15100 doğrula
2. Windows Firewall → KnightOnline.exe izinli
3. ISP'de port 15001/15100 açık mı?
4. Sunucu durum: `malasiako.com/status`

### B) "Patch failed / corrupt"
**Sebep:** İndirme bozuk, disk dolu, AV engelleme
**Çözüm:**
1. `C:\MalaysiaKO\Patch_Temp\` temizle
2. Antivirüs istisna ekle (KnightOnline.exe + Launcher.exe)
3. Disk yer kontrol (5 GB+)
4. Tekrar dene

### C) "Code Guard error"
**Sebep:** AC tetiklendi (cheat tool, yetersiz yetki, sanal makine)
**Çözüm:**
1. Cheat Engine / Wireshark / VPN kapat
2. Yönetici olarak çalıştır (Administrator)
3. Sanal makine değil gerçek PC kullan
4. `cg_crash.log` incele → forum support

### D) "Launcher Cant find KnightOnline.exe"
**Sebep:** Eksik kurulum, AV silmiş
**Çözüm:**
1. AV karantina kontrol
2. Yeniden kur

### E) "Version mismatch"
**Sebep:** Local version = sunucu version değil
**Çözüm:**
1. Launcher'ı yönetici olarak çalıştır → patch al
2. Manuel patch zip indir → extract

### F) FPS düşük / lag
**Sebep:** Donanım/grafik
**Çözüm:**
1. `Option.exe` → düşür ayarlar
2. Windowed mode dene
3. KOFPL.dll versiyonu kontrol

### G) Karakter görünmüyor (model bug)
**Sebep:** Asset bozuk / patch eksik
**Çözüm:**
1. `Chr/`, `ChrSelect/` klasör temiz
2. Yeniden patch al

---

## 8. ANTI-CHEAT (Code Guard / Pearl Guard) Detay

### Çalışma
- `CodeGuard/` klasörü + `Apr_Show.dll`
- Memory scan (Cheat Engine, Wireshark tespit)
- Process listesi tarama
- HWID kayıt (`+pcblock` için)
- Crash log: `cg_crash.log`

### Yasaklı Tool'lar
- Cheat Engine
- Wireshark / Fiddler (paket sniff)
- AutoIt / AutoHotkey (bazı script)
- VMware / VirtualBox (genelde)
- Sandbox

### Yetkili Tool'lar
- Discord (overlay genelde sorun çıkmaz, ama dikkat)
- OBS (yayın, AC izinli)
- Steam (overlay kapat)

⚠️ **Detay:** `ANTI_CHEAT.md`

---

## 9. CLIENT GÜNCELLEME (Lansman Sonrası)

### Major Patch
1. Sunucu `VERSION` tablosuna yeni satır INSERT
2. Patch zip → `C:\koweb2\patch\` upload
3. Oyuncu launcher → otomatik indir
4. Launcher → eski exe yedek + yeni exe

### Hot Reload (Server-side)
- `+reloaditems`, `+reloadmagics`, etc. → DB değişiklik
- Client patch gerektirmez (server-side data)

### TBL Hot Reload
- `tbl_compare.py` → diff
- TBL_HASH güncelle (`TBL_HASH.md`)
- Client patch ile gönder

---

## 10. LANSMAN GÜNÜ ÖZEL KONULAR

- [ ] Site indirme linki çalışıyor mu? (CDN'de mi yoksa direkt mi?)
- [ ] Setup.exe vs .rar — hangisi tercihli?
- [ ] Launcher patch alıyor mu? (port 80 + version.txt)
- [ ] İlk indirme: tam paket boyut + bandwidth tahmin
- [ ] Hosting CDN: Cloudflare R2 / OVH / direct?
- [ ] Antivirüs flag listesi (KO genelde false positive)
- [ ] FAQ sayfası "kurulum sorunu" başlık var mı?
- [ ] Discord "kurulum yardım" kanalı?
- [ ] Yardım dökümantasyonu hazır mı (forum sticky)?

---

## 11. PRODUCTION DAĞITIM

### A) İlk Lansman İndirme
- **5 GB** tek dosya (.rar)
- VEYA **Setup.exe** (içinde patch zip'leri)
- Hosting: malasiako.com `/download/`
- Mirror: yedek hosting (lansman gün trafiği)

### B) Patch Sunumu (sürekli)
- `port 80` patch_server.js
- Patch zip'leri: `C:\koweb2\patch\<version>.zip`
- DB `VERSION` tablosu trigger

### C) CDN
- Lansman 24h: bandwidth çok yüksek
- Cloudflare / OVH CDN düşün
- Origin: 104.238.23.99 (Hostabil)

---

## 12. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **Version uyumsuzluğu silent kill** — `PROJE_TARIHCESI § AŞAMA 3` ders |
| 2 | **Antivirüs false positive** — Code Guard injection benzeri davranış |
| 3 | **Yönetici yetkisi** — Launcher.exe çalıştırırken admin lazım |
| 4 | **Path.Ini relative paths** — `C:\MalaysiaKO\` sabit beklenebilir |
| 5 | **Port 80 patch** — server'da diğer servis varsa çakışır |
| 6 | **5 GB indirme** lansman bandwidth peak — CDN şart |
| 7 | **Setup.exe imza** — Microsoft Defender SmartScreen uyarı verir |
| 8 | **Win 7 desteği YOK** — modern AC engeli |
| 9 | **`cg_crash.log`** AC sorunu → support kanal |
| 10 | **DLL'ler imzalı olmalı** (KOFPL, Apr_Show, OpenAL32) |

---

## 13. KAYNAK REFERANSLAR

- **Client haritası:** `CLIENT_HARITA.md`
- **Patch süreç:** `PATCH_SURECI.md`
- **Asset:** `ASSET.md`
- **Anti-cheat:** `ANTI_CHEAT.md`
- **Şifreleme:** `SIFRELEME.md`
- **Sunucu:** `SUNUCU_DOSYA_YOLLARI.md`
- **Web:** `WEB_PHP.md`
- **Bug:** `WEB_BUG.md`, `GUVENLIK_BUG.md`
- **Dosyalar:** `C:\MalaysiaKO\Server.ini`, `Path.Ini`, `Option.ini`

---

**Sürüm:** v1.1 — S88 (patron versiyon açıklamasıyla düzeltildi)
**Not:** `Server.ini Files=2377` = 2369 base + 8 patch katmanı, `Define.h 1098` = server kod/UI giydirme stili. İki ayrı sayaç, çakışma yok (S88 patron notu).
