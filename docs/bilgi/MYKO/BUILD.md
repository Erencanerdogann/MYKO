# Build Pipeline — GameServer + AntiCheat

**Tarih:** 2026-04-29 | **Hazırlayan:** CHIP | **Kaynak:** Gerçek dosya okuma

---

## Gereksinimler

| Bileşen | Versiyon | Not |
|---------|---------|-----|
| Visual Studio | 2022 | v143 toolset zorunlu |
| Platform Toolset | v143 | MSBuild parametresi |
| Platform | x64 | 32-bit derleme YAPILMAZ |
| Config | Release | Deploy için Release; debug için Debug |
| Windows SDK | 10.0+ | VS ile gelir |
| MSSQL ODBC | — | DSN kurulu olmalı |
| libcurl | statik | Kaynak içinde |
| zlib | statik | Kaynak içinde |
| BugTrap | — | Crash handler |
| Lua 5.1 | statik | GameServer\lua\ |

---

## Çözümler

| Çözüm | Yol | Ne Derler |
|-------|-----|-----------|
| ByNoiseGameServer.sln | `GameServer_SRC\` | GameServer + LogInServer |
| CodeGuardGameServer.sln | `GameServer_SRC\` | ByNoise + Pearl Guard entegre |
| CodeGuardAnticheat.sln | `AntiCheat_SRC\` | code.guard DLL ayrı |

---

## Build Komutları

### GameServer (CHIP kullanır)
```bash
MSBuild ByNoiseGameServer.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -m
```

### Paralel build (daha hızlı)
```bash
MSBuild ByNoiseGameServer.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -m:8
```

### Sadece GameServer projesi
```bash
MSBuild ByNoiseGameServer.sln -t:proj-GameServer -p:Configuration=Release -p:Platform=x64
```

---

## Build Çıktıları

| Dosya | Boyut | Konum |
|-------|-------|-------|
| `GameServer.exe` | ~3.4 MB | `GameServer_SRC\x64\Release\` |
| `LogInServer.exe` | ~496 KB | `GameServer_SRC\x64\Release\` |
| `code.guard` | 5.9 MB | `AntiCheat_SRC\Release\` |

---

## Deploy Süreci

```
1. Yeni exe build et
2. Eski exe yedekle:
     cp GameServer.exe GameServer.exe.bak.<tarih>
3. Yeni exe → Desktop\Server\
4. (Gerekirse) code.guard → Desktop\Server\
5. start_servers.bat → başlat
6. Logs\ kontrol et
```

**Deploy:** DOKTOR + RUSTIK yetkisi. CHIP tek başına deploy YAPAMAZ.

---

## Mevcut Yedek Zinciri

```
Desktop\Server\
├── GameServer.exe                      ← aktif
├── GameServer.exe.bak.22mart
├── GameServer.exe.bak.26mart
├── GameServer.exe.pre_s17_bak
├── GameServer.exe.pre_session2_bak
├── GameServer.exe.pre_batch1_bak
├── LogInServer.exe                     ← aktif
└── code.guard                          ← aktif (Pearl Guard runtime)
```

Ek yedek: `C:\temp\MYKO\yedekler\GameServer_SRC_YEDEK_22MART\` (1767 dosya)

---

## ODBC DSN Kurulum (Zorunlu — Build öncesi kontrol)

```
ODBC Veri Kaynakları (64-bit):
  DSN adı:    CodeGuardMYKO_DB     ← ASCII, Türkçe karakter YASAK
  Server:     localhost\MSSQLSERVER01
  Database:   KO_MYKO
  Auth:       Windows Auth veya SQL Auth
```

**Doğrula:**
```cmd
sqlcmd -S localhost\MSSQLSERVER01 -d KO_MYKO -Q "SELECT 1"
```

---

## ⚠️ Kritik Patlama Dersleri

### 1. LOGIN_PORT Uyumsuzluğu (Silent Death)
```ini
# GameServer.ini
[SETTINGS]
LOGIN_PORT=15100    ← LogInServer.ini PORT ile AYNI OLMALI

# LogInServer.ini  
[SETTINGS]
PORT=15100
```
Farklı olursa: Server boot oluyor, hiç bağlantı kabul etmiyor. Log yok.

### 2. ODBC DSN Türkçe Karakter
```
KO_MAİN   ← YANLIŞ (Türkçe İ → ODBC bulamıyor)
KO_MAIN   ← DOĞRU  (ASCII)
CodeGuardMYKO_DB ← Bizim aktif DSN adı
```

### 3. INI Placeholder Değerleri
```ini
ACCOUNT_PWD=password  ← Gerçek şifreyle değiştir
```
Boş/placeholder bırakma — DB bağlantısı sessizce kesilir.

### 4. XSafe_VERSION Eşleşmesi
- `XGuard.cpp: #define XSafe_VERSION 5`
- Client'ın beklediği XSafe versiyonu ile eşleşmeli
- Değiştirince tüm clientler login olamaz

### 5. code.guard Eksikliği
- GameServer `code.guard` olmadan başlamıyor
- Build sonrası deploy'da unutulabilir — kontrol et

---

## Build Bağımlılıkları (Kaynak içi)

```
GameServer_SRC\
├── libcurl\          ← HTTP (statik link)
├── N3BASE\           ← 3D shape (statik link)
├── scripting\        ← Lua (build'e dahil)
└── shared\           ← ORM + yardımcılar (her iki exe paylaşır)
```

**Önemli:** `shared\` değiştiğinde hem GameServer hem LogInServer yeniden derlenmeli.
Ve client ile AYNI ANDA deploy (CHI-15 dersi — bak: `C:\temp\Chip\hatalar\`).

---

## Build Hataları — Bilinen

| Hata | Çözüm | Kaynak |
|------|-------|--------|
| `std::map` → `std::unordered_map` implicit conversion | `auto` kullan | S46 CHIP hafıza notu |
| Npc.cpp:6002 build hatası | `auto` tipi | S46 |

---

## Hızlı Kontrol Listesi (Deploy öncesi)

- [ ] LOGIN_PORT GameServer.ini == LogInServer.ini PORT
- [ ] ODBC DSN `CodeGuardMYKO_DB` mevcut ve bağlanıyor
- [ ] code.guard deploy edildi
- [ ] XSafe_VERSION değiştirilmediyse client uyumlu
- [ ] `shared\` değişti → client de güncellendi mi?
- [ ] `CharacterMovementHandler.cpp:197` wall cheat açık mı?

---

## İlgili Dosyalar

- Kaynak haritası: `SRC_HARITA.md`
- Anti-cheat: `ANTI_CHEAT.md`
- Önemli CPP: `SRC_ONEMLI_CPP.md`
- Patlama tarihi: `PROJE_TARIHCESI_VE_DERSLER.md`
