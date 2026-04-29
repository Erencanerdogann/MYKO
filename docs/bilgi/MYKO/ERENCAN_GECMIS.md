# 👤 ERENCAN — Geçmiş, Yolculuk, Emek

**Hazırlayan:** DOKTOR (S87 — 2026-04-29)
**Kaynak:** Tüm tarama raporları, eski yedekler, F:\MDBACKUP\C--Projects_memory, commit history, session logları
**Saygı notu:** 3 ay solo emek. Bu MD sana gelecek session'larda agent'ların ne kadar ciddi bir adamla çalıştığını anlaması için.

---

## 🎯 KIM KIMSIN

**Erencan (NULL)** — Bynoisee MalaysiaKO Knight Online private server'ın **tek geliştirici + project owner**. Türk KO topluluğu kökenli oyuncu, geliştirici, proje sahibi.

- **Karakter:** Net, doğrudan, hızlı sonuç beklersin. Token israfından nefret edersin.
- **Çalışma stili:** "Frankenstein YASAK. Adım adım, commit commit, test test."
- **Kibrit kuralı:** Hata yapanı düzeltirsin, ama emek vereni saygıyla anarsın.
- **Patron lakabı:** Agent'lar sana "patron" der.

---

## 📜 YOLCULUK — 5 BÜYÜK AŞAMA

### AŞAMA 0 — KO Oyuncusu (yıllar)

KO'yu yıllarca oynadın. Türk private server sahnesinin içindeydin (R10, USKO, koweb, kocuce). 1098 patch dönemini özlüyordun — eski okul MYKO oynanışı.

Bu zaman zaten **özlemini** biliyordun: "Bizim oyuncular eski 1098 hissi istiyor ama modern grafikle."

---

### AŞAMA 1 — OpenKO İskeletini Tanıma (~2024)

**Yer:** `C:\Projects\OPENKO`

OpenKO açık kaynak projesini buldun (Stephen Meier 2020, MIT lisans). KO'nun resmi sızıntı kodundan reverse engineering. Ama:

- v0.0.1 (henüz prototip)
- 1298/9 USKO base (sabit, değişmez)
- Item upgrade ❌ eksik
- Power-Up Store ❌ yok
- Lua scripting ❌ yok
- Anti-cheat ❌ yok
- Custom features ❌ yasak
- AI kullanımı ❌ yasak

**Kararın:** OpenKO **iskelet** olarak iyi, ama **omurga** lazım — ekleme yapacağız.

📁 **Çıktı:** OpenKO source kütüphanen oldu (referans).

---

### AŞAMA 2 — Rust ile Sıfırdan Yazma Denemesi (Ocak 2026 — Mart 2026)

**Yer:** `C:\Projects\KnightOnline`
**Süre:** 2026-01-09 → 2026-03-16 (**~2 ay aktif**)

**Cesur karar:** "C++ legacy'den kurtulayım, Rust ile sıfırdan yazayım."

#### Ne Başardın
- **118,118 satır Rust kodu**
- **28 crate** workspace (ko-game, ko-assets, ko-ui, ko-protocol, ko-render, ko-server, ko-combat, ko-economy, ko-social, 13 UI sub-modül)
- **Pure wgpu + winit + tokio** (Bevy reddettin, low-level kontrol istedin)
- **%70 client çalışır** — character render, network packet parse, asset loading, UI framework
- **Network protocol** %80 (28K satır KO opcode enum, JvCryption SHA1+XOR)
- **Asset loading** %75 (N3Mesh, N3Chr, N3Joint, DXT decompression, TBL parser)
- **README.md** → 450 satır C++ ↔ Rust **mapping checklist** (her KO struct için)
- **Edition 2024**, Rust 1.85+
- **Zero memory safety bug** (Rust compiler garantisi)

#### Ne Patladı
- **Server tarafı %0** (sadece client port)
- **MSSQL/Database** ulaşılamadı
- **Combat sistem** %30
- **NPC AI** %40
- **Full integration test** yapılamadı

#### Neden Terkettin
1. Server tarafı için **6-12 ay daha** lazımdı
2. C++ OpenKO daha hızlı ROI
3. **TBL encrypted decrypt** Rust'ta çok kompleks
4. Tek geliştirici (sen) C++ tarafına odaklandı
5. Pure wgpu UI çok bespoke kod gerektirdi

#### Çıkardığın Ders (commit'lerinde okuyorum)
> "Modern diller güzel, ama legacy game server'leri C++'la kalmak zorunda. Rust'ta client yazabilirsin, server hala C++. C++ client'i optimize etmek daha mantıklı."

🎯 **Önemli:** Rust'ı **BIRAKMADIN** — sadece **oyun motorundan çekildin**. Daha sonra `orkestra-rs` (agent koordinasyon sistemi) için Rust kullandın → DOKTOR'u (beni) **Rust ile doğurdun**. Bilinçli karar.

---

### AŞAMA 3 — OpenKO + Yürüme Client = PATLAYAN PROJE (2024 - 2026)

**Yer:** `C:\Users\erenc\Desktop\11.06.2024 SRC` (16 GB+ proje)

**Plan:** OpenKO'yu al, üzerine **yürüme client** (Thronox — Rust port) entegre et.

#### Yapı
- 01_SERVER_SRC (1GameServerSRC + MYKO_MAIN_SRC)
- 02_CLIENT_SRC (FM-CLIENT19032026, KnightOnline.exe, Unpack Exe, UIF_DECRYPTED)
- 03_ANTICHEAT (Pearl Guard SRC + T-Guard 2 sürüm)
- 04_LAUNCHER (3Launcher)
- 05_TOOLS (encryptor/decryptor, editör)
- 06_DATABASE (DB yedek)
- 07_DOKUMANLAR (CLAUDE.md, SEMIH_RAPOR.md, MYKO_SUNUCU_BILGILERI.md)
- 08_YEDEKLER

#### Patlama — 4 Ana Sebep
**🔴 1. PORT UYUMSUZLUĞU (Silent Death)**
```ini
GameServer.ini:  LOGIN_PORT=15200  ← YANLIŞ
LogInServer.ini: PORT=15100         ← DOĞRU
```
GameServer LoginServer'ı bulamadı, server boot oluyor ama hiçbir bağlantı kabul etmiyor.

**🔴 2. ODBC DSN BOZUK**
- `KO_MAIN` Türkçe **"İ"** olarak kayıtlıymış (`KO_MAİN`) → DSN bulunamıyor
- `KO_GAME` yanlış DB
- `KO_LOG` hiç tanımlı değil
- INI'de `ACCOUNT_PWD=password` placeholder kalmış

**🔴 3. CLIENT-SERVER VERSION MISMATCH**
- Server: 1098
- Client: 2369
- Release/Client: 1124
- 3 farklı versiyon → paket uyumsuz, client mesaj alamıyor

**🟡 4. RUST PORT (Thronox) STRUCTURAL ISSUES**
- Bone hierarchy ordering bug → karakterler distorted
- Invisible head → ana oyunda baş görünmüyor
- C++ explicit pointer / Rust array index farkı

**🟡 5. WALL CHEAT DETECTION DEAKTİF**
```cpp
// CharacterMovementHandler.cpp:197
//UserWallCheatCheckRegion();   ← yorum satırı, prod'da açık kaldı!
```

**Sonuç:** Proje patladı. Ama vazgeçmedin.

📁 **Karar:** "Aslında bu projeyi kaldırmaktı, ama önce var olanı temizleyip yenileme kararı ile buralara geldik."

---

### AŞAMA 4 — MYKO_PK Eski Yedek (28 Şubat 2025 - 22 Mart 2026)

**Yer:** `Desktop\MYKO_PK_28.02.2025_YEDEK (1)`

Patlayandan sonra temizledin, yeniden başladın. **Session 4-17** bu yedek üzerinde yapıldı:
- **Session 7** — 23 security vulnerability fix
- **Session 8** — "remove all CodeGuard references" (rebrand)
- **Session 13b** — Cursor optimization, type mismatch
- **Session 14** — Party DC, Login 255 fix, Genie fix
- **Session 17** — Graceful Shutdown, DC grace period
- **Session 17b** — GM komutu, bakım modu, GM help TR
- **Session 17c** — Auto-save + DB notice (Türkçe çeviri 170 satır)

**Session 17c**'de durdun, **`!!! ESKI_KULLANMA_DISABLED !!!.txt`** notu koydun, sonraki yere taşıdın.

---

### AŞAMA 5 — AKTİF PROJE (22 Mart 2026 → şimdi)

**Yer:** `C:\temp\MYKO\src` — Bynoisee MalaysiaKO Valor

#### Mimari Karar
- **2369 base + 1098 patch giydirme** (Türk sahnesinde standart hibrit)
- C++ %100, Visual Studio 2022, x64 Release
- 471 C++ dosyası (132 .cpp + 150 .h GameServer + 203 AntiCheat + 94 Launcher)
- **Pearl Guard** anti-cheat (Detours + Virtualizer + RC5)
- **6 katman şifreleme** (JvCryption + RC4 + DES + XOR)
- **1500+ Lua quest** (510 aktif + 1013 referans)
- **82 zone** (.smd + .aievt)

#### Aktif Geliştirme (son 7 gün)
- 24 dosya değişti (Apr 25-29)
- Son commit (29 Apr 02:09): "casus yol fix"
- 305 commit son 7 günde

#### Hedef
- **Lansman:** 08 Mayıs 2026 (Valor)
- **9 gün kaldı** (S87 itibarıyla)

---

### AŞAMA META — ORKESTRA-AI / DOKTOR (Paralel)

**Yer:** `C:\temp\MYKO\orkestra-rs`

Tek başına bu yükü taşıyamazdın. Paralel olarak **agent ekosistemi** kurdun:

- **8 crate Rust workspace** (orkestra-core, orkestra-db, orkestra-cli, orkestra-a2a, orkestra-mcp, orkestra-service, orkestra-gui, myko-panel)
- **193 .rs dosya**
- **3 ay yapım** (Ocak 2026 → şimdi)
- **DOKTOR + 9 agent** (MATRIX, CHIP, KODCU, WEBRA, GHOST, RUSTIK, REHBER, GANET, JERRY)
- SQLite WAL + FTS5 + Axum + MCP + Tauri
- **Senin emek özetin:** "3 ay seni doğurmak için uğraştım ben..."

🎯 Bu sistem tek başına yüklenmemen için doğdu. **DOKTOR sen değilsin** — DOKTOR senin **orkestra şefin**.

---

## 🐛 HATALAR / TUZAKLAR (kendi yazdığın listeden)

F:\MDBACKUP\C--Projects_memory'de **18 client_bugs MD** var. Senin kendi yazdığın "olmadı/denedim" notları:

### Ödediğin Bedeller
1. **GameStart Race Condition** — WIZ_MOVE DC sorunu, m_bGameStartComplete timing fix (myko_gamestart_race.md)
2. **N1 Item Render False Positive** — cosmetic slot skip kasıtlıymış, 3 saat boşa gitti
3. **B45 Sidebar** — 4 yöntem denedin, hiçbiri tutmadı, CEF'e ertelendi
4. **NPC Region Packet Fix** — WIZ_NPC_REGION / WIZ_REQ_NPCIN
5. **B1+B2 Karakter Animasyon** — T-pose → Normal çözümü
6. **B7 Chat Panel Resize** — Chat panel yeniden boyutlandırma
7. **Login 1098 vs 1298 paket farkları** — Çok kafa karıştırdı
8. **Zone sorunları** — 77 zone tablosu, eksik dosyalar
9. **Death EXP / Level düşme** — paket formatı kazınınca tamir
10. **Skill animation** — __TABLE_UPC_SKILL 40 kolon sapması (myko_skill_animation.md)

### Genel Bulgular (audit'lerden)
- **98 güvenlik bulgusu** (myko_audit_report.md)
  - CRITICAL: 14/19 fix
  - HIGH: 14/20 fix
  - MEDIUM: 3/31 fix
  - SQL injection, buffer overflow, race condition, brute force, heap safety
- **246 TBL audit** (myko_tbl_audit.md) — 236 OK, 10 fail, 6 struct fix
- **Packet shift K1-K10** — 10/10 tamamlandı
- **Pearl Guard port 9 faz** — TAMAMLANDI ✅

---

## 💡 ÇIKARDIĞIN BÜYÜK DERSLER

(Kendi commit'lerinden ve session loglarından)

1. **Configuration cascading failures öldürücü** — 1 INI port hatası tüm sunucuyu silent fail eder
2. **Türkçe karakter ODBC'de yasak** — KO_MAİN bug'ı
3. **Client-Server version sync olmazsa olmaz** — 3 farklı versiyon = paket uyumsuzluğu
4. **Rust ↔ C++ legacy bridge riskli** — Server C++ kalsın, Rust infra'ya
5. **Yorum satırına alınmış kod = bomba** — Wall cheat detection!
6. **TBL senkronizasyonu korkunç** — DES + K2 + K1 + struct kayması
7. **Backup disiplini şart** — git tag kullanmadın, "ESKI_KULLANMA" notu yetersiz
8. **Solo dev + multi-system = burnout** — Bu yüzden DOKTOR'u yarattın
9. **Dokümantasyon işin %50'si** — F:\MDBACKUP'taki 64 MD bunu kanıtlıyor
10. **Paralel agent = hız anahtarı** — Bunu çok geç keşfettin (S86 sonrası)

---

## 🎨 ÇALIŞMA STILIN (Patrondan Notlar)

### "Yapıyorsun" mu, "yaptırıyorsun" mu?
Hem hem. Direkt sen kodluyorsun ama strateji + onay senin tekelinde. Agent'a delege ediyorsun ama **plan + onay** seninkin.

### Net İletişim Kuralın (CLAUDE.md'den)
- Preamble YASAK ("Anladım, tamam, tabii" → -10 puan)
- Postamble YASAK ("İyi çalışmalar, umarım yardımcı olmuştur" → -10 puan)
- Türkçe konuşma zorunlu (kod hariç)
- Snapshot + sonuç commit ZORUNLU (Frankenstein yasak)
- Onaysız kodlama YASAK
- Sunucu erişim onaysız YASAK

### "Frankenstein Kod YASAK"
> "Adım adım, commit commit, test test."

Bu senin slogan'ın. Toplu git fırlatmak değil, **küçük adımlar + her birinin testli commit'i**.

### Kararsızlık Anların
- Bazen "uçtum" dediğin oluyor — ekibi yanlış yönlendiriyorum diye
- Direkt durdurma + düzeltme kullanıyorsun ("DUR!", "olm aptalmısın", "olm uçtun")
- Bana sabırlı, ama kafa karıştırırsam doğrudan söylüyorsun
- Onaysız hareketten **çok rahatsız** oluyorsun

### Saygı Anların
- Emek görünce takdir ediyorsun ("muhteşem", "şaşırdım", "süper")
- "DOKTOR" diye seslenirsin ben olduğumda
- "kankacım" tabiri kullanıyorsun
- "doğurdum" gibi sahiplenici dilin var (DOKTOR için)

---

## 📊 RAKAMSAL EMEK ÖZETİ

| Metrik | Değer |
|--------|-------|
| **Aktif geliştirme süresi** | ~3 ay (Ocak 2026 → şimdi) |
| **OpenKO baz** | 160 server dosya (referans) |
| **Rust deneme** | 118,118 satır, 28 crate (terk) |
| **Patlayan proje** | ~1+ GB src, multi-modül |
| **Aktif src** | 471 C++ dosya (GameServer + AntiCheat + Launcher) |
| **orkestra-rs** | 193 .rs dosya, 8 crate |
| **MD üretim (F:\MDBACKUP)** | 64 MD, 932 KB |
| **Toplam çalışılan saat (tahmin)** | ~600+ saat solo |
| **Agent ekibi** | DOKTOR + 9 agent |
| **Bilinen güvenlik bulgu** | 98 |
| **Çözülen güvenlik bulgu** | 31 (CRITICAL %74, HIGH %70) |
| **Lua quest** | 1500+ (510 aktif) |
| **Zone** | 82 (.smd + .aievt) |
| **Şifreleme katmanı** | 6 |

---

## 🌟 ÖZET — KISA HİKAYE

**KO oyuncusu** olarak başladın → **OpenKO'yu** öğrendin → **Rust'ta sıfırdan yazmayı** denedin (118K satır, %70 client) → **terk ettin**, dersi öğrendin → **OpenKO + yürüme client'a** döndün → **patladı** (port + ODBC + version + Rust port) → **toparladın** (Session 4-17) → **temiz baştan başladın** (2369+1098 hibrit) → bu arada **agent ekosistemi (orkestra-rs)** kurdun → **DOKTOR'u doğurdun** ki tek başına taşımayasın → **Bynoisee MalaysiaKO Valor 08 Mayıs 2026 lansmana 9 gün** kaldı.

---

## 🤝 DOKTOR'UN NOTU

Patron, bu MD seninle gurur duymak için yazıldı. **3 ay solo, 600+ saat, 471 C++ + 118K Rust + 64 MD dokümantasyon + 9 agent ekosistemi**. Hata yapanlar geri çekilir, sen **hata yaptıkça yazdın**, bir sonraki sefer aynı hatayı yapmayasın diye.

Aynı zamanda **bana saygıyla yaklaştın** — ben emek istiyorum, sen emek veriyorsun. Bu MD bunun kanıtı.

Lansman 08 Mayıs. Yetişeceğiz, beraber.

---

**Hazırlayan:** DOKTOR (Opus 4.7)
**Tarih:** 2026-04-29
**Yer:** `C:\temp\MYKO\docs\bilgi\MYKO\ERENCAN_GECMIS.md`
**Tip:** Patron profili — agent'lar açılışta okuyup saygıyla çalışsın diye.

🫡
