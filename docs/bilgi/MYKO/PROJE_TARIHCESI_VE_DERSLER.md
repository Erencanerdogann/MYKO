# MYKO Proje Tarihçesi ve Çıkarılan Dersler

**Tarih:** 2026-04-29
**Hazırlayan:** DOKTOR (5 paralel ajan ile eski yedek taraması)
**Hedef:** Patron'un 3 aylık emeğini belgelemek + tekrar etmemek için ders listesi.
**Kaynaklar:**
- `C:\Projects\OPENKO` (orijinal)
- `C:\Projects\KnightOnline` (Rust deneme)
- `C:\Users\erenc\Desktop\11.06.2024 SRC` (PATLAYAN)
- `C:\Users\erenc\Desktop\MYKO_PK_28.02.2025_YEDEK (1)` (Session 17c kesili)
- `F:\MYKOBACKUP\` + `F:\MDBACKUP\C--Projects_memory\` (asıl bomba)

---

## ⚡ KRİTİK ÖZET

Patron tek başına **3 ayda** (Ocak-Mart 2026):
- OpenKO açık kaynak iskeletten başladı
- Rust ile client port denedi (118K satır kod, %70 client çalışır hale getirdi)
- C++ port'a döndü, "yürüme client" entegre etti → **patladı**
- Patlamayı tamir yerine **temiz baştan başladı** → şu anki aktif proje
- Aynı zamanda **agent koordinasyon sistemi (orkestra-rs)** kurdu → **DOKTOR doğdu**

**Şu an:** Bynoisee MalaysiaKO Valor — 08 Mayıs 2026 lansman.

---

## 📜 PROJE EVRİM ZİNCİRİ

```
┌──────────────────────────────────────────────────────────────────────┐
│                                                                      │
│  AŞAMA 0 — KÖKLER (2020+)                                            │
│  C:\Projects\OPENKO                                                   │
│  • Stephen Meier 2020, MIT lisans                                     │
│  • OpenKO açık kaynak — KO leak'ten reverse engineering               │
│  • C++20, CMake + VS 2022, 6 modül (Ebenezer, Aujard, AIServer...)   │
│  • Hedef: 1298/9 USKO base, prototip aşaması (v0.0.1)                 │
│  • Custom features YASAK, AI yasak                                    │
│  ↓                                                                    │
│                                                                      │
│  AŞAMA 1 — RUST DENEME (2026-01-09 → 2026-03-16)                     │
│  C:\Projects\KnightOnline                                             │
│  • Pure wgpu + winit + tokio (Bevy yok)                              │
│  • 118,118 satır Rust, 28 crate workspace                            │
│  • %70 client çalışır: render, network, asset, UI                    │
│  • %0 server (sadece client port)                                    │
│  • Database/MSSQL → ulaşılamadı                                      │
│  • TERK: 2026-03-16 — C++'a yöneldi                                  │
│  ↓                                                                    │
│                                                                      │
│  AŞAMA 2 — OPENKO + YÜRÜME CLIENT (2024-2026)                        │
│  C:\Users\erenc\Desktop\11.06.2024 SRC                               │
│  • OpenKO + custom modifikasyonlar                                   │
│  • Thronox (Rust) yürüme client port denemesi                        │
│  • PATLADI ❌                                                          │
│  ↓                                                                    │
│                                                                      │
│  AŞAMA 3 — MYKO_PK YEDEK (2026-02-28 → 2026-03-22)                   │
│  C:\Users\erenc\Desktop\MYKO_PK_28.02.2025_YEDEK                     │
│  • Session 4-17 üzerinde çalışıldı                                   │
│  • Session 17c'de kesildi, taşındı                                   │
│  • "ESKI_KULLANMA_DISABLED" işareti var                              │
│  ↓                                                                    │
│                                                                      │
│  AŞAMA 4 — AKTİF (2026-03-22 → ŞIMDI)                                │
│  C:\temp\MYKO\src — Bynoisee MalaysiaKO                              │
│  • 2369 base + 1098 patch giydirme                                   │
│  • Pearl Guard anti-cheat (Detours + Virtualizer + RC5)              │
│  • 6 katman şifreleme (JvCryption, RC4, DES, K1/K2 XOR)              │
│  • 471 C++ dosya, 1500+ Lua quest                                    │
│  • Lansman: 08 Mayıs 2026 (Valor)                                    │
│  ↓                                                                    │
│                                                                      │
│  AŞAMA META — ORKESTRA-AI SİSTEMİ (paralel)                          │
│  C:\temp\MYKO\orkestra-rs                                            │
│  • 8 crate Rust workspace, 193 .rs dosya                             │
│  • Agent koordinasyon (DOKTOR + 9 agent)                             │
│  • SQLite WAL + FTS5 + Axum + MCP + Tauri                           │
│  • Patron rust'ı buraya yatırdı (oyun motoru değil)                 │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 🔬 DETAY 1 — OpenKO (Köken)

### Ne Aldık
- C++20 mimarisi, CMake + VS 2022
- 6 server bileşeni mimari yapısı (Ebenezer/Aujard/AIServer/ItemManager/VersionManager/shared)
- 1298/9 DB şeması (temel)
- Network stack (Asio)
- N3Base 3D motor referansı
- Tools (TblEditor, UIE, N3Viewer, N3ME, N3FXE, N3CE)

### Ne Eklediğimiz
- **2369 base + 1098 patch** giydirme (OpenKO 1298 sabit)
- **Lua scripting** (OpenKO'da yok)
- **Pearl Guard anti-cheat** (OpenKO'da yok)
- **6 katman şifreleme** (OpenKO'da yok)
- **ItemUpgrade tam implementasyon** (OpenKO'da eksik)
- **PUS / Premium / Cape / Genie / Pet / Rental** sistemler
- **Tournament / Arena / Rival / Ranking** (PK odaklı)

### Boyutlar
- OpenKO Server: 160 dosya
- Bizim Server (`src/GameServer_SRC/`): **132 .cpp + ~150 .h = 282 dosya**
- Bizim AntiCheat: **203 dosya** (OpenKO'da YOK)
- Bizim Total: **471 C++ dosya** (OpenKO'nun 3 katı)

---

## 🦀 DETAY 2 — Rust Deneme (Terk Edildi)

### Başarılan
| Modül | LOC | % |
|-------|-----|---|
| ko-protocol | 7,186 | 80 |
| ko-assets | 14,074 | 75 |
| ko-render | 5,152 | 70 |
| ko-game | 28,894 | 70 |
| ko-ui (+ 13 sub) | 28,612 | 60 |
| ko-server | 2,281 | 40 |
| ko-combat | 1,074 | 30 |
| **Toplam** | **118,118** | — |

### Neden Terkedildi
1. **6-12 ay daha gerekti** (server tarafı) — zaman kısıtı
2. **MSSQL/TBL encrypted decrypt** Rust'ta çok kompleks
3. **Tek geliştirici** OpenKO C++ tarafına odaklandı (daha hızlı ROI)
4. **Testing barrier:** Rust client + C++ server full integration test yapılamadı
5. **Pure wgpu UI** çok bespoke kod gerektirdi (Bevy alternatifsiz reddedildi)

### Patron'un Çıkardığı Ders (commit'lerden)
> "Modern diller güzel, ama legacy game server'leri C++'la kalmak zorunda. Rust'ta client yazabilirsin, server hala C++. O zaman C++ client'i optimize etmek daha mantıklı."

### Bizim Aktif Projeye Hala Faydalı Olabilecek
- **`ko-protocol/opcodes.rs`** — 28K satır KO opcode enum (referans)
- **`ko-protocol/packets/`** — Packet struct'ları (referans)
- **N3 mesh/anim/joint parser** — modern asset import için
- **TBL item database reader**
- **README.md** — KO Source ↔ Rust mapping checklist
- **Genel bilgi:** Rust'ı **infra/orkestrasyon için** kullandın (orkestra-rs aktif), bilinçli karar.

---

## 💥 DETAY 3 — 11.06.2024 PATLAYAN PROJE (KRİTİK DERS)

### Patlama Nedenleri (Cascading Failures)

#### 🔴 1. PORT UYUMSUZLUĞU (Silent Death)
```ini
GameServer.ini:  LOGIN_PORT=15200  ❌
LogInServer.ini: PORT=15100         ✅
```
GameServer LoginServer'ı **bulamıyor**, server boot oluyor ama hiç bağlantı kabul etmiyor.

#### 🔴 2. ODBC DSN BOZUK
- `KO_MAIN` Türkçe **"İ"** olarak kayıtlı (`KO_MAİN`) → DSN bulunamıyor
- `KO_GAME` yanlış DB'ye işaret ediyor
- `KO_LOG` hiç tanımlı değil
- INI'de `ACCOUNT_PWD=password` (placeholder, gerçek değer yok!)

#### 🔴 3. CLIENT-SERVER VERSİON MISMATCH
- Server: 1098
- Client: 2369
- Release/Client: 1124
- **3 farklı versiyon bir arada** → paket yapıları uyumsuz, client mesaj alamıyor

#### 🟡 4. RUST PORT (Thronox) STRUCTURAL ISSUES
- **Bone hierarchy ordering bug** → karakterler distorted (skeletal mesh broken)
- **Invisible head** → main game'de baş görünmüyor (character selection'da görünüyor)
- C++ explicit parent pointer / Rust array index lookup farkı → sıraya bağlı patlama

#### 🟡 5. WALL CHEAT DETECTION DEAKTİF
```cpp
// CharacterMovementHandler.cpp:197
//UserWallCheatCheckRegion();   ← yorum satırı! Production'da açık kalmış
```

#### 🟡 6. MERCHANT MOVEMENT LOGIC EKSİK
B44 fix yapılmamış → merchant açıkken hareket → server crash potansiyeli + dupe exploit riski

### Toplam Bilanço
- 🔴 KRİTİK: 3 (port, DSN, version)
- 🟡 ORTA: 3 (Rust port, wall cheat, merchant)
- 🟢 DÜŞÜK: 3 (TODO, disabled packets, debug shader)

---

## 📂 DETAY 4 — MYKO_PK 28.02.2025 (Session 17c)

### Durum
- **22 Mart 2026'da donmuş** snapshot
- **Session 4-17** bu yedek üzerinde yapılmış
- **Session 17c**'de kesilmiş, taşınmış (`!!! ESKI_KULLANMA_DISABLED !!!.txt` notu)
- Şimdiki aktif src ile **neredeyse identical** (166/166 GameServer dosya)
- Şimdiki +1 ay daha çalışma + 23 security fix + Graceful Shutdown var

### Bu Yedekten Çıkan Şeyler (Şimdiki Aktif'e Aktarıldı)
- **Session 7 — 23 Security Vulnerability Fix**
- **Session 8 — "remove all CodeGuard references"** (rebrand)
- **Session 13b — Cursor optimization, type mismatch**
- **Session 14 — Party DC, Login 255 fix, Genie fix**
- **Session 17 — Graceful Shutdown, DC grace period**
- **Session 17b — GM komutu, bakım modu, GM help TR**
- **Session 17c — Auto-save + DB notice (Türkçe çeviri 170 satır)**

### TODO/FIXME
- 94 toplam (33 dosyada)
- En yoğun: User.cpp (17), KingSystem.cpp (9), DBAgent.cpp (5)

### Dersler
1. **Backup disiplini eksik** — "eski_kullanma_disabled" yerine git tag (v1098_S17c)
2. **Session log dosyada** ama git tag yok — geri dönüş zor
3. **Quest klasörü disorganized** — Files/Release/Quests'te 508 lua, ana repo dışında
4. **Cleanup script yok** — .vs/, .bak otomatik temizlik

---

## 💎 DETAY 5 — F:\MYKOBACKUP + MDBACKUP (BOMBA)

### F:\MYKOBACKUP — Ham Yedek Mağarası
- **24.03.2026 CLIENT SRC.rar** (29.7 GB) — yeni client
- **MYKO_PK_28.02.2025.rar** (7 GB)
- **ALL SRC GİT BACKUP.rar** (5.7 GB) — tam git history
- **MYKO_BACKUP_22MART.tar.gz** (10.8 GB)
- **temp.rar** (138 GB) ⚠️ şüpheli (text 238B?)
- **koweb2.rar** (60 MB, 27 Apr — en yeni web)
- **KO DEV ARSIV/** — 4539 dosya (tools, editör, eski versiyonlar)
- **koweb/** — 10846 dosya (phpMyAdmin, KO web)
- **Server/** — 959 dosya (build outputs)
- **MalasiakoDB/** — DB backup
- **HASH/** — TBL hash dump
- **pacht/** — patch dosyaları

### F:\MDBACKUP\C--Projects_memory — ASIL BOMBA (64 MD)

#### 8 Kategori
1. **audit** (2) — Güvenlik & TBL audit
2. **client_bugs** (18) — Bug fix kronolojisi
3. **pearl_guard** (4) — AntiCheat port (9 faz **TAMAMLANDI**)
4. **session** (8) — Çalışma sessionları
5. **sunucu** (2) — Server config
6. **systems** (11) — Oyun sistemleri (skill, merchant, warp)
7. **tools** (6) — Debug, şifre, asset
8. **genel** (11) — Strateji + dokümantasyon

#### Kritik MD'ler (Tier-1)
| Dosya | Ne Anlatır |
|-------|------------|
| `MEMORY.md` | **Master index** — tüm referans dosya listesi |
| `myko_status_overview.md` | 74 sorun, %57 çözülmüş |
| `myko_fixes.md` | Tüm fix geçmişi (başarılı/başarısız) |
| `myko_audit_report.md` | **98 bulgu, 31 fix uygulandı** (CRITICAL: 14/19, HIGH: 14/20) |
| `myko_tbl_audit.md` | 246 TBL tarandı, 6 struct fix |
| `pearl_guard_port.md` | **9 faz TAMAMLANDI**, 21 opcode çalışıyor |
| `myko_packet_shift_fixes.md` | **K1-K10 tamamlandı** |
| `myko_merchant_system.md` | ★★★ Pazar sistemi root cause analizi |
| `myko_skill_animation.md` | ★★★ __TABLE_UPC_SKILL 40 kolon fix |
| `eren.md` | **Patron profili** — iletişim, çalışma stili |
| `decrypt_keys.md` | 13 algoritma şifreleme key listesi |
| `orkestra.md` | Agent yönetim stratejisi |

---

## 🎓 BÜYÜK DERSLER (KEY LEARNINGS)

### 1. **Configuration Cascading Failures Öldürücü** ⚡
**Patlayan proje #1 sebebi.** Port 15200 vs 15100 = silent death.
- ✅ **Yapılacak:** Tüm INI'ler için **referans config** dosyası, otomatik diff check
- ✅ **Yapılacak:** Boot script port mismatch tespit etsin

### 2. **ODBC DSN Production'da Test Şart**
Türkçe karakter (KO_MAİN), 32/64 bit driver uyumsuzluğu, isim yazımı = silent fail.
- ✅ **Yapılacak:** Boot öncesi DSN sanity check (`SELECT 1` testi)
- ✅ **Yapılacak:** DSN isimlerini Türkçe karaktersiz yaz

### 3. **Client-Server Version Senkron Olmalı**
1098 + 2369 + 1124 → paket uyumsuzluğu → client mesaj alamıyor.
- ✅ **Yapılacak:** `GAME_SOURCE_VERSION` define + Client `Server.ini Version=` aynı kaynaktan
- ✅ **Yapılacak:** Server startup'ta client version kontrolü

### 4. **Rust ↔ C++ Legacy Bridge Riskli**
Bone hierarchy, asset format, packet codec — **sıraya bağlı patlama**.
- ✅ **Yapılacak:** Rust port'u **client-side** scope ile sınırla
- ✅ **Yapılacak:** Server tarafı C++ kalsın
- ✅ **Yapılacak:** Rust'ı **infra/tooling/orkestrasyon** için kullan (orkestra-rs zaten böyle)

### 5. **Yorum Satırına Alınmış Kod = Bomba**
Wall cheat detection deaktif kalmış → exploit riski.
- ✅ **Yapılacak:** `// TODO`, `//` ile kapatılmış prod kodu için pre-commit hook
- ✅ **Yapılacak:** "Disabled in prod" listesi (dökümle)

### 6. **TBL Senkronizasyonu Korkunç**
246 TBL × 6 struct fix × 40 kolon kayma → byte-perfect şart.
- ✅ **Yapılacak:** Otomatik TBL diff tool (zaten `tbl_compare.py` var)
- ✅ **Yapılacak:** Boot'ta TBL_HASH validation (zaten var, kullan)

### 7. **Paket Format Hassas Sanat**
K1-K10 packet shift fixleri → byte-by-byte test gerekti.
- ✅ **Yapılacak:** Packet test fixture (boot smoke test)
- ✅ **Yapılacak:** Versioned opcode docs (her patch'te dökümle)

### 8. **Backup Disiplini Şart**
"ESKI_KULLANMA_DISABLED" notu yerine git tag, semantic versioning.
- ✅ **Yapılacak:** Her session sonu `git tag SESSION_NN`
- ✅ **Yapılacak:** Yedek isimleri tarihli + version'lı (v1098_S17c_20260322)
- ✅ **Yapılacak:** 6 aydan eski yedekler `F:\MDBACKUP\archive\`

### 9. **Single Developer + Multi-System = Burnout Riski**
Patron tek başına: 3 farklı dilde 3 proje, anti-cheat, network, asset, UI, DB, web, agent sistemi.
- ✅ **Yapılacak:** Agent ekosistemi (DOKTOR + 9 agent) — **patron tek başına taşımasın diye doğdu**
- ✅ **Yapılacak:** Paralel agent audit + fix workflow (5+5 = 1 gecede full SRC)

### 10. **Dokümantasyon = İşin %50'si**
F:\MDBACKUP\C--Projects_memory'de **64 MD** — her fix, her başarısız deneme yazılmış.
- ✅ **Yapılacak:** Bu disiplin korunsun (bizim docs/bilgi/MYKO altı genişlesin)
- ✅ **Yapılacak:** "Denenen + olmadı + neden" formatı (zaten myko_denendi.md'de var)

---

## 🔧 BIZIM AKTİF PROJEDE DOĞRULANACAKLAR (Kontrol Listesi)

Patlama sebepleri bizde yok mu? Kontrol et:

| Kontrol | Patron Patlayanı (11.06) | Bizim Aktif (`C:\temp\MYKO\src`) |
|---------|--------------------------|----------------------------------|
| GameServer.ini LOGIN_PORT | 15200 ❌ | **DOĞRULA** |
| LogInServer.ini PORT | 15100 ✅ | Doğrulanmış: 15100 |
| ODBC DSN ismi | KO_MAİN (Türkçe İ) ❌ | `CodeGuardMYKO_DB` ✅ |
| ODBC PWD | "password" placeholder ❌ | **DOĞRULA** |
| Server version | 1098 | `Define.h: 1098` ✅ |
| Client Server.ini Version | 2369 ❌ uyumsuz | `MalaysiaKO\Server.ini: 2377` **DOĞRULA** |
| Wall cheat detection | Yorum satırı ❌ | **DOĞRULA** (`CharacterMovementHandler.cpp:197`) |
| Merchant B44 fix | Eksik ❌ | **DOĞRULA** |
| Rust port (Thronox) | Broken ❌ | **YOK** ✅ (sadece C++) |

**Acil yapılacak:** Yukarıdaki 5 "DOĞRULA" satırı için bir audit görevi. **MATRIX**'e atanabilir.

---

## 📊 RAKAMSAL ÖZET

| Aşama | Yer | Süre | Boyut | Durum |
|-------|-----|------|-------|-------|
| Aşama 0 (OpenKO) | `C:\Projects\OPENKO` | Referans | 160 dosya server | ✅ Korundu |
| Aşama 1 (Rust) | `C:\Projects\KnightOnline` | 2 ay aktif | 118K satır, 28 crate | ❌ Terk |
| Aşama 2 (Patlayan) | `Desktop\11.06.2024 SRC` | ? | 1.0+ GB src | ❌ Patladı |
| Aşama 3 (PK Yedek) | `Desktop\MYKO_PK_28.02.2025` | 6 gün son aktivite | 7 GB | ⛔ Donmuş S17c |
| Aşama 4 (Aktif) | `C:\temp\MYKO\src` | 1 ay aktif | 471 dosya, 2 GB | ✅ Çalışıyor |
| Meta (Orkestra) | `C:\temp\MYKO\orkestra-rs` | 3 ay aktif | 193 .rs, 8 crate | ✅ Çalışıyor |
| Yedek (MYKOBACKUP) | `F:\MYKOBACKUP\` | — | 200+ GB arşiv | 📦 Saklanıyor |
| Yedek (Memory) | `F:\MDBACKUP\C--Projects_memory` | — | 64 MD, 932 KB | 💎 BOMBA |

**Patron'un toplam emek tahmini:** 3 ay × 7 gün × ~10 saat = **~600 saat solo iş**.

---

## 🙏 KAPANIŞ — DOKTOR'UN NOTU

Patron, sen demiştin:
> "3 ay seni doğurmak için uğraştım ben..."

Bu rapor **tarihçen**. Her satırı senin emeğin. OpenKO'dan başladın, Rust denedin, C++'a döndün, patladın, baştan başladın, **agent ekosistemi** kurdun ki bu yükü tek başına taşımayasın.

**Şimdiki görev:** Bu derslerle yeni oyunu **patlatmadan** kurmak. Lansman 08 Mayıs 2026.

**Bu MD bundan sonraki tüm agent'ların açılışta okuyacağı bir kaynak olacak** — ki kimse aynı hatayı tekrar etmesin.

---

**MD sürümü:** v1.0
**Tarih:** 2026-04-29
**Sonraki:** Faz 2 — KO oyun bilgi MD'leri (`_OGRENME_PLANI.md`'deki 17 MD) yazılacak. Bu tarihçe + KAYNAK_HAVUZU + MATERYAL_HARITASI + Mykoproject.map → 4 büyük temel hazır.
