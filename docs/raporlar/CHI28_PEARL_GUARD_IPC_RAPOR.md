# CHI-28 — Pearl Guard DOKUNMA Listesi + GameServer IPC Hazırlığı
**Agent:** CHIP | **Session:** S83 | **Tarih:** 2026-04-26
**Görev:** C++ kod DEĞİŞTİRME YOK — sadece analiz + rehber

---

## M1 — Pearl Guard "DOKUNMA" Listesi

### TEMEL BULGU: Pearl Guard CLIENT taraflı DLL'dir

Pearl Guard (`Pearl Guard.dll`) KnightOnLine.exe'ye inject olan bir istemci taraflı anti-cheat'tir.
**GameServer ile doğrudan bağlantısı YOK.** Sunucu tarafında ayrı bir anti-cheat sistemi var: **XGuard** (`XGuard.cpp`).

Bu ayrım KODCU için kritik:
- `taskkill /F GameServer.exe` → Pearl Guard'ı TETİKLEMEZ (Pearl Guard client'ta)
- Sunucu restart/stop işlemleri Pearl Guard'dan bağımsız

---

## 🔴 KESİNLİKLE DOKUNULMAYACAK

### AntiCheat_SRC/ — Komple YASAK

```
C:\temp\MYKO\src\AntiCheat_SRC\Pearl Guard\Pearl.cpp          — DLL entry + inject mekanizması
C:\temp\MYKO\src\AntiCheat_SRC\Pearl Guard\Pearl Engine.cpp   — UI hook + oyun motoru bağlantısı
C:\temp\MYKO\src\AntiCheat_SRC\Pearl Guard\FunctionGuard.cpp  — CRC32 integrity check sistemi
C:\temp\MYKO\src\AntiCheat_SRC\Pearl Guard\BanSystem.cpp      — Registry + dosya bazlı ban sistemi
```

**Neden:** Bu dosyalar birbirini hash'leyerek bütünlük doğrular. Herhangi bir byte değişikliği CRC32 uyuşmazlığına yol açar → Shutdown() tetiklenir → istemci kapanır.

### FunctionGuard mekanizması (Pearl.cpp → FunctionGuard.h)

```cpp
// FunctionGuard.cpp:
DWORD hash = crc32((uint8*)startAddress, size, -1);
// Check sırasında:
DWORD hash = crc32((uint8*)(inf.inAddress - 32), inf.size, -1);
if (hash != inf.hash) return false;  // → Shutdown()
```

**KURAL:** AntiCheat_SRC altındaki HİÇBİR dosyaya dokunma.

### GameServer tarafı — XGuard.cpp kritik alanlar

```
C:\temp\MYKO\src\GameServer_SRC\GameServer\XGuard.cpp
```

Sunucu anti-cheat protokolü. Bu fonksiyonlara dokunmak oyuncuların disconnect olmasına yol açar:

| Fonksiyon | Satır | Risk |
|-----------|-------|------|
| `XSafe_StayAlive()` | ~496 | Heartbeat doğrulama — MD5 imza + tick zaman kontrolü |
| `XSafe_AuthInfo()` | ~571 | Kimlik doğrulama |
| `XSafe_ProcInfo()` | ~591 | İstemci process listesi analizi |

```cpp
// XSafe_StayAlive — dokunma:
public_key = md5("1X" + XSafe_VERSION + "10001" + clock1 + ischeckdecated2 + accountid);
if (public_key != uPublic_key) goDisconnect("heart beat md5 encrypt2 error");
```

**`XSafe_VERSION = 5`** — bu sayı değişirse TÜM istemciler disconnect olur.

---

## 🟡 DİKKATLİ (test gerekli)

### shared/ dizini

```
C:\temp\MYKO\src\GameServer_SRC\shared\
```

LoginServer + GameServer ortak kodu. Değiştirince **iki exe birden rebuild** gerekir (CHI-15 dersi).
`signal_handler.cpp` burada — SIGTERM/SIGINT handler'ı. Değiştirmek shutdown akışını bozabilir.

### ConsoleInputThread.cpp

```
C:\temp\MYKO\src\GameServer_SRC\GameServer\ConsoleInputThread.cpp
```

Console komut döngüsü. `g_bRunning` flag'ini izler. Değişiklik shutdown sıralamasını etkileyebilir.

### CharacterMovementHandler.cpp — FLY_HACK bloğu (satır ~117)

```cpp
if (!isGM() && real_y > 0.0f)  // ← m_bGenieStatus kontrolü yok
```

Genie kullanıcıları için false positive üretiyor (MATRIX MAT-23 + ACIL FIX-A konusu).

---

## 🟢 GÜVENLİ (serbestçe dokunulabilir)

| Dosya | Neden güvenli |
|-------|---------------|
| `DBAgent.cpp` | DB iletişimi — Pearl Guard / XGuard ile ilgisiz |
| `ChatHandler.cpp` — komut tablosu | Yeni console komutu ekleme güvenli |
| `lua_bindings.cpp` | Script motoru — anti-cheat dışı |
| `GameServerDlg.cpp` — timer bölümleri | Event/timer mantığı — anti-cheat dışı |
| `ServerStartStopHandler.cpp` — `ShutdownTimer()` | Graceful shutdown akışı — değiştirilebilir |
| `main.cpp` — Mutex bloğu | Singleton guard — güvenli |

---

## M2 — Server Kapatma Yöntemleri

### Yöntem 1: `+down` console komutu (GRACEFUL — ÖNERİLEN)

**Nasıl çalışır:**
```
GameServer console → +down
→ HandleShutdownCommand() tetiklenir (ChatHandler.cpp:1315)
→ m_Shutdownstart = true, m_Shutdownfinishtime = UNIXTIME + 1
→ ShutdownTimer() (her saniye çalışır, ServerStartStopHandler.cpp:1293)
→ Tüm oyuncuları uyarır (in-game chat: "Sunucu X dakika/saniye sonra kapanıyor")
→ KickOutAllUsers() → DB queue boşalır
→ s_hEvent.Signal() → main() döngüsü çıkar
→ g_bRunning = false → destructor → tüm thread'ler temiz kapanır
```

**Süre:** ~1 dakika (kod `UNIXTIME + 1` diyor ama ShutdownTimer süreci bekler)
**Veri kaybı:** YOK — tüm oyuncular LogOut() + OnDisconnect() ile kaydedilir
**Pearl Guard:** ETKİLEMEZ (client taraflı)
**Önerim:** NORMAL KAPATMA için bu yöntem

### Yöntem 2: SIGTERM / CTRL+C (GRACEFUL — ikinci seçenek)

**Nasıl çalışır:**
```
signal(SIGTERM, OnSignal) — signal_handler.cpp
→ OnSignal() → s_hEvent.Signal()
→ main() döngüsü çıkar
→ g_bRunning = false → aynı destructor akışı
```

**myko-panel'den kullanım:**
```rust
// SSH üzerinden:
ssh administrator@104.238.23.99 "taskkill /IM GameServer.exe /T"
// VEYA Windows'ta SIGTERM yok, CTRL+C console event göndermek gerekir
// → GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid)  — sadece aynı console group'tan çalışır
// PID ile farklı process'e CTRL+C göndermek Windows'ta kısıtlı
```

**Sınır:** SSH ile dışarıdan SIGTERM göndermek Windows'ta güvenilmez.

### Yöntem 3: `taskkill /F` (HARD KILL — ACİL DURUM)

```
taskkill /F /IM GameServer.exe
→ Process anında sonlandırılır
→ Destructor ÇALIŞMAZ → oyuncu verisi KAYBEDİLEBİLİR
→ DB queue boşalmaz → tutarsız veri
```

**Risk:** Açık karakter envanteri, para, XP kaybı
**Pearl Guard:** ETKİLEMEZ (client taraflı)
**Önerim:** SADECE ACİL DURUM

### Yöntem 4: Console komutu — SSH üzerinden inject (DOLAYLI — ÖNERİLEN IPC YÖNTEMİ)

Aşağıdaki IPC bölümünde detay.

---

## M3 — Panel-GameServer IPC Yöntemleri

### Seçenek A: Yok — Sadece DB üzerinden (MEVCUT DURUM)

myko-panel → SQL UPDATE → GameServer DB'yi okur (tabloyu reload edince)

```sql
-- Örnek: Duyuru gönder
INSERT INTO tbl_notice VALUES (...)
-- Sonra: +reloadnotice console komutu ile aktif
```

**Gecikme:** 0-60 saniye (reload aralığına göre)
**Kurulum:** Sıfır — zaten çalışıyor
**Önerim:** Basit veri güncellemeleri için yeterli

### Seçenek B: SSH + `+down` console komutu (ÖNERİLEN BAŞLANGIÇ)

```rust
// myko-panel Rust kodu (örnek):
use std::process::Command;
Command::new("ssh")
    .args([
        "-i", "~/.ssh/id_rsa",
        "Administrator@104.238.23.99",
        // stdin'e komut inject etmek için özel yaklaşım gerekli
    ])
    .output();
```

**Sorun:** GameServer console'una stdin inject etmek için process'in TTY'sine bağlanmak gerekir. Standart SSH komutu ile doğrudan stdin yazma kısıtlı.

**Çalışan yol:** `SendKeys` veya Windows'ta `WriteConsoleInput` API — ama bu sunucuda ek araç gerektirir.

**Daha temiz alternatif:** `HandleConsoleCommand()` fonksiyonu zaten `ProcessServerCommand()` çağırıyor. Eğer GameServer'a bir **named pipe** veya **admin TCP port** eklersek panel doğrudan komut gönderebilir. Bu M3'ün önerisi.

### Seçenek C: Named Pipe — MEVCUT DEĞİL, EKLENEBİLİR

GameServer kaynak kodunda named pipe YOK. Eklemek güvenli:

```cpp
// Eklenebilecek yer: main.cpp, StartConsoleInputThread() yanına
// StartAdminPipeThread() — \\.\pipe\GameServerAdmin
// HandleConsoleCommand(cmd) ile mevcut komut tablosunu kullanır
// Tüm +down, +reloadtables vb. komutlar otomatik çalışır
```

**Güvenlik:** Pipe'ı sadece localhost'a aç → SSH tüneli üzerinden myko-panel erişir
**Ekstra:** Pearl Guard'dan tamamen bağımsız (server tarafı)
**KODCU için:** Bu yaklaşım KOD-7'nin "hot-reload" hedefine de hizmet eder

### Seçenek D: Mevcut komut tablosu genişletme (GÜVENLİ HOT-RELOAD)

`s_commandTable`'a yeni komut eklemek güvenli:

```cpp
{ "reloadquests",  &CGameServerDlg::HandleReloadQuestCommand, "..." },
// Zaten var — +reloadtables, +reloadmagics, +reloaddrops, +reloadnotice
// Tüm bunlar panel üzerinden tetiklenebilir (Seçenek B veya C ile)
```

---

## KODCU (KOD-7) İçin Öneri Özeti

### Server Durdur butonu:
1. **İlk tercih:** SSH → `+down` console komutu inject (graceful, veri güvenli)
2. **Fail ise:** `taskkill /F` (veri kaybı riski, kullanıcıya uyarı göster)
3. **Çift onay diyaloğu:** Evet (ÖNERİLEN, +down) / Zorla Kapat (taskkill)

### Hot-reload (quest/item/drop tabloları):
- `+reloadtables`, `+reloadtables2`, `+reloadtables3` komutları mevcut
- `+reloadmagics`, `+reloadquests`, `+reloaddrops`, `+reloadnotice` ayrı komutlar var
- Panel'den tetiklemek için SSH stdin inject VEYA named pipe (Seçenek C) şart

### Pearl Guard ile ilgili risk:
- **SIFIR** — Pearl Guard client taraflı, sunucu operasyonlarından bağımsız
- GameServer restart/stop Pearl Guard'ı tetiklemez
- Sunucu binary değişirse istemcilerin restart atması gerekir (beklenen davranış)

### C++ değişiklik gerekiyorsa güvenli alanlar:
- `ChatHandler.cpp` — yeni console komutu ekle (komut tablosuna satır)
- `ServerStartStopHandler.cpp` — shutdown timing ayarla
- `main.cpp` — named pipe thread başlat (Seçenek C için)
- `DBAgent.cpp` — yeni DB sorgusu ekle

### C++ değişiklik KESİNLİKLE yasak:
- `AntiCheat_SRC/` altı — hepsi
- `XGuard.cpp` — XSafe_VERSION, StayAlive, AuthInfo
- `shared/` — dikkatli ol, LoginServer etkiler

---

**Hazırlayan:** CHIP | **Referans görev:** CHI-28 | **Durum:** M1+M2+M3 TAMAM
