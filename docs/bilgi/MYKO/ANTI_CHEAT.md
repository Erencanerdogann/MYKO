# Pearl Guard / Code Guard Anti-Cheat

**Tarih:** 2026-04-29 | **Hazırlayan:** CHIP | **Kaynak:** Gerçek dosya okuma
**GİZLİLİK:** Hassas key/algoritma detayları bu dosyada YOK. Sadece mimari.

---

## Genel Mimari

```
┌──────────────────────────────────────────────────────┐
│  CLIENT TARAF                                        │
│  code.guard (5.9 MB DLL)                            │
│    ├── Pearl Guard (UI/hook katmanı)                │
│    ├── CODE Cli.710F3AD0 (kod obfuskasyon)          │
│    ├── DetourAPI 3.0 (kernel hook)                  │
│    ├── Virtualizer (kod virtualizasyon)             │
│    └── RC5 cipher                                   │
│                          ↕  XSafe protokol          │
│  SERVER TARAF                                        │
│  XGuard.cpp (GameServer içinde)                     │
│    ├── XSafe_StayAlive() → heartbeat + challenge    │
│    ├── XSafe_ProcInfo() → process sorgulama         │
│    └── XSafe_Log() → cheat log                     │
└──────────────────────────────────────────────────────┘
```

---

## Bileşenler

| Bileşen | Konum | Amaç |
|---------|-------|------|
| Pearl Guard | `AntiCheat_SRC\Pearl Guard\` | Ana DLL — client koruma |
| CODE Cli.710F3AD0 | `Pearl Guard\CODE Cli.710F3AD0\` | Kod obfuskasyon + dosya şifreleme |
| DetourAPI 3.0 | `Pearl Guard\DetourAPI\` | Microsoft Detours (API hook) |
| Virtualizer | `Pearl Guard\Virtualizer\` | Kod virtualizasyon (tersine mühendisliğe karşı) |
| discord-rpc-master | `Pearl Guard\discord-rpc-master\` | Discord status entegrasyonu |
| RC5 | `Pearl Guard\RC5\` | Simetrik şifreleme |
| N3BASE | `Pearl Guard\N3BASE\` | GameServer 3D yapı kopyası (client taraf) |
| XGuard.cpp | `GameServer_SRC\GameServer\XGuard.cpp` | Server taraf hook (2,334 satır) |

---

## Source İstatistikleri

| Alan | Değer |
|------|-------|
| Klasör | `C:\temp\MYKO\src\AntiCheat_SRC\` |
| Boyut | ~1.3 GB |
| .cpp + .h | 203 dosya |
| Çözüm | `CodeGuardAnticheat.sln` |
| Runtime DLL | `code.guard` (5.9 MB, Desktop\Server\) |

---

## XGuard.cpp — Server Taraf (Detay)

**Dosya:** `GameServer_SRC\GameServer\XGuard.cpp` (2,334 satır)

### Sabitler
```cpp
#define XSafe_ACTIVE 1         // 0 yapılırsa AC devre dışı
#define XSafe_VERSION 5        // Client XSafe versiyonu ile eşleşmeli
#define XSafe_ALIVE_TIMEOUT 60 // 60 sn heartbeat gelmezse kick
#define XSafe_SUPPORT_CHEK 30  // Support check aralığı (sn)
```

### Fonksiyonlar

| Fonksiyon | Satır | Amaç |
|-----------|-------|------|
| `XSafe_StayAlive()` | ~496 | Heartbeat + MD5 challenge-response |
| `XSafe_ProcInfo()` | ~591 | Çalışan process/window listesi sorgula |
| `XSafe_SendProcessInfoRequest()` | ~645 | Process sorgusunu başlat |
| `XSafe_Log()` | ~658 | Cheat tespiti log kaydı |
| `XSafe_Support()` | ~672 | Destek/rapor mekanizması |
| `XSafe_UIRequest()` | ~694 | UI bilgisi + PUS senkron |
| `XSafe_ReqMerchantList()` | ~99 | Güvenli merchant list |
| `XSafe_ItemNotice()` | ~85 | Item bildirim |
| `XSafe_SendMessageBox()` | ~92 | Client'a uyarı kutusu |
| `XSafe_Reset()` | ~481 | AC state sıfırla |
| `XSafe_AuthInfo()` | ~571 | Auth bilgisi doğrula |

### Challenge Mekanizması
MD5 tabanlı, `XSafe_VERSION` bağımlı. [GİZLİ — public MD'de yok]

### Kick Mekanizması
`XSafe_ALIVE_TIMEOUT` (60 sn) içinde `XSafe_StayAlive` paket gelmezse oyuncu kick edilir.

---

## Pearl Guard — Client DLL Yapısı

### Pearl.cpp (Ana DLL)
**Giriş noktası:** `Pearl.cpp`
**PearlEngine:** Merkezi engine nesnesi (`Engine->power = false` → DLL kapanır)
**Shutdown:** `Shutdown(message)` → MessageBox göster → `exit(0)`
**Şifreleme yardımcısı:** `LoadCrypto()`

### CODE Cli.710F3AD0 — Dosya Şifreleme
- `CreateFileA` hook → Oyun dosyaları okunurken şifreli sürümü şeffaf çöz
- Dosyaları geçici yola (`xCodeTMP`) kopyala, şifreyi çöz, oradan sun
- Hedef: `.code` uzantılı UI/script dosyaları
- [GİZLİ — tam algoritma public MD'de yok]

### DetourAPI 3.0
- Microsoft Detours kütüphanesi
- Kernel seviyesi API hook (CreateFileA, [diğerleri GİZLİ])
- Tersine mühendislik / cheat araçlarına karşı

### Virtualizer
- Kritik kodu CPU-independent bytecode'a çevirir
- Tersine mühendislik maliyetini arttırır
- `VirtualizerSDK32.lib` → build'e link edilir

---

## .code Dosyaları (UI/Script)

| Özellik | Değer |
|---------|-------|
| Şifreleme | RC4 (MYKO key) |
| Konum | `Desktop\Server\` yanı + game klasörü |
| Sayı | 100+ dosya |
| Prefix örnekleri | `re_*`, `macho_*`, `co_*`, `El_*`, `Ka_*` |
| İçerik | UI layout, script, konfigürasyon |
| Erişim | Yalnızca Pearl Guard DLL çözebilir (runtime) |

⚠️ Bu dosyaların içeriğini MD'ye yapıştırma — sızdırma riski.

---

## Güvenlik Sınıflandırması

### 🔴 KESİNLİKLE DOKUNULMAYACAK

- `AntiCheat_SRC\*` — tüm AC kaynak kodu
- `Pearl Guard\*` — tüm alt klasörler
- `XGuard.cpp` — server hook (değiştirirsen AC protokol bozulur)
- `code.guard` — runtime DLL (replace etme)
- `XSafe_VERSION` sabiti — client ile eşleşmeli, değişince tüm clientleri kick eder

### 🟡 DİKKATLİ (değiştirmeden önce test et)

- `XSafe_ALIVE_TIMEOUT` — değiştirirsen kick agresifliği değişir
- `XSafe_SUPPORT_CHEK` — rate limiter, çok düşük yapma
- Herhangi bir `XSafe_*` fonksiyon imzası — paket formatı bağımlı

### 🟢 GÜVENLİ (XGuard ile doğrudan bağlantısı yok)

- `DBAgent.cpp` — DB iletişimi (AC ile alakasız)
- `LuaEngine.cpp` — Lua quest köprüsü
- `QuestHandler.cpp` — quest logic
- `ChatHandler.cpp` — chat (XSafe komutları hariç)
- `EventMainSystem.cpp` — event zamanlama
- `Knights.cpp`, `KnightsManager.cpp` — klan
- `Npc.cpp`, `NpcThread.cpp` — NPC AI

---

## Server Kapatma Analizi (Pearl Guard Açısından)

### taskkill /F GameServer.exe
- Pearl Guard **client DLL** — GameServer'ı izlemiyor
- GameServer kapanırsa client bağlantısı kopuyor (socket closed)
- AC tetiklenmiyor — sadece bağlantı timeout

### Graceful Shutdown (/down komutu)
- Tüm kullanıcı oturumları DB'ye kaydedilir
- XGuard bekleyen paketler flush edilir
- Server kapanır, client'lar disconnect alır
- AC açısından temiz — önerilen yöntem

---

## Panel-GameServer IPC Analizi

### Mevcut Durum
GameServer'da named pipe, memory-mapped file veya TCP admin port **yok**.
Tek IPC yolu: **DB üzerinden komut tablosu (asenkron)**.

### Seçenek A: DB üzerinden (mevcut, kullanılabilir)
```
myko-panel → DB UPDATE (komut_queue tablosu)
GameServer timer → DB polling → komutu işle
```
- Gecikme: 1-30 saniye (timer aralığına bağlı)
- Risk: Cache farkı, geç tepki
- Avantaj: Sıfır kod değişikliği

### Seçenek B: Console komut (SSH üzerinden)
```
myko-panel → SSH → tasklist/taskkill veya stdin inject
```
- GameServer console'a `/down`, `/notice` gönderilebilir
- Risk: SSH overhead, güvenlik
- Detay: DOKTOR SSH yetkisi gerekli

### Seçenek C: Named Pipe ekle (geliştirme gerekli)
- `\\.\pipe\GameServerAdmin` oluşturulabilir
- CHIP'in onayı + kod değişikliği + DOKTOR plan onayı şart
- Açılış öncesi: **Gerek yok, Seçenek A yeterli**

### RUSTIK için Öneri
- **Normal kapatma:** `/down` komutu (console veya SSH inject)
- **Acil kapatma:** `taskkill /F` (veri kaybı riski var, son çare)
- **IPC:** Seçenek A (DB) — başlangıç için yeterli

---

## Patlama Dersleri (Pearl Guard ilgili)

1. `code.guard` yoksa GameServer başlamıyor (checksum kontrolü)
2. `XSafe_VERSION` yanlışsa tüm clientler login olamıyor
3. `shared\` değişince client + server AYNI ANDA deploy (CHI-15 dersi)
4. Wall cheat detection kapalı → `CharacterMovementHandler.cpp:197` doğrula

---

## 1098 PATCH NOTU

Pearl Guard bu kaynakta **v2369 client + 1098 patch dosyaları** kombinasyonu için optimize edilmiştir.

| Konu | Değer |
|------|-------|
| Base client | v2369 |
| Patch seviyesi | 1098 |
| `code.guard` boyutu | ~6.0 MB |
| Anti-cheat katmanı | Pearl Guard (custom) + code obfuscation |
| Hedef cheat seviyesi | 1098 dönemi: wall/speed/item dupe odak |

**1098 dönemi AC seviyesi: ORTA**
- Wall hack: `CharacterMovementHandler.cpp:264` — `UserWallCheatCheckRegion()` YORUM SATIRINDA (devre dışı)
  - ⚠️ Bu fonksiyon aktif edilmeli. Şu an wall cheat tespiti KAPALI.
  - Etkinleştirme: yorum satırını kaldır, test et
- Speed hack: XSafe heartbeat + server taraf movement validation
- Item dupe: XSafe_StayAlive MD5 challenge ile kısmen korunuyor

**Pearl Guard + 1098 uyumu:**
- `code.guard` DLL `Desktop\Server\` altında olmalı (GameServer başlamazsa ilk bak)
- `.code` uzantılı UI dosyaları 1098 patch ile gelen dosyalar — 2369 orijinallerinden farklı
- RC4 key MYKO'ya özel — key değişince tüm .code dosyaları çalışmaz
- `XSafe_VERSION 5` — 1098 patch client bu versiyonla eşleşiyor

**Kontrol — deploy öncesi:**
```bash
# code.guard deploy edildi mi?
ls Desktop/Server/code.guard

# XSafe aktif mi?
grep "XSafe_ACTIVE" GameServer_SRC/GameServer/XGuard.cpp
# → #define XSafe_ACTIVE 1  (1 olmalı)

# Wall cheat detection durumu:
grep "UserWallCheatCheckRegion" GameServer_SRC/GameServer/CharacterMovementHandler.cpp
# → Satır 264: yorum satırında = KAPALI (dikkat!)
```

---

## İlgili Dosyalar

- Server kaynağı: `SRC_HARITA.md`
- XGuard detayı: `SRC_ONEMLI_CPP.md`
- Build: `BUILD.md`
