# CLIENT HARITA — KnightOnline Oyuncu Istemcisi Yapısı

**Tarih:** 2026-04-29 | **Kategori:** GAME CLIENT | **Boyut:** 5.0 GB | **Versiyon:** 2369 base + 1098 patch

---

## 1. GENEL BAKIŞ

Oyuncunun bilgisayarında çalışan istemci (`C:\MalaysiaKO\`). **15 MB çekirdeği + 5 GB asset**. İçinde:
- **KnightOnline.exe** — ana oyun yürütülebilir
- **DLL'ler** — grafik/ses/şifreleme
- **Data** — tablo veri (246 .tbl dosyası)
- **Asset** — 3D model, doku, ses (Item/Object/UI/Chr)
- **Config** — sunucu IP, grafik ayarı
- **CodeGuard** — anti-cheat koruma

---

## 2. ANA DOSYALAR

### Yürütülebilir

| Dosya | Boyut | İş |
|-------|-------|-----|
| **KnightOnline.exe** | 15 MB | Oyun ana logic |
| **Launcher.exe** | 4.8 MB | Başlatıcı, patch check |
| **Option.exe** | 332 KB | Grafik/ses ayarı |
| **KscViewer.exe** | 492 KB | Item/karakter viewer |

### DLL'ler

| Dosya | İş |
|-------|-----|
| **KOFPL.dll** | Network/packet (KO For Play Library) |
| **code.guard** | Anti-cheat hook (6.0 MB) |
| **OpenAL32.dll** | Ses kütüphanesi |
| **libvorbisfile.dll** | OGG ses codec |
| **Apr_Show.dll** | DirectX renderer |

### Konfigürasyon

| Dosya | İçerik |
|-------|--------|
| **Server.ini** | IP, port, XignCode, version |
| **Option.ini** | Grafik quality, sound, language |
| **Path.Ini** | Asset yol ayarı |
| **Scheduler.ini** | War event schedule (binary hex, 141 KB) |

### Log Dosyaları

| Dosya | İçerik |
|-------|--------|
| **cg_crash.log** | CodeGuard crash raporu |
| **log.klg** | Oyun aktivite logu (şifreli) |
| **.dmp** | Crash dump (varsa) |

---

## 3. KLASÖR HAKİMİYETİ

```
C:\MalaysiaKO\
├── Data\                   (36 MB — 246 .tbl dosya)
│   ├── Item.tbl           — Item master
│   ├── Item_Ext*.tbl      — Item effect
│   ├── MagicTable.tbl     — Skill tablo
│   ├── MagicType1-9.tbl   — Skill tipleri
│   ├── NPC.tbl            — NPC master
│   ├── Quest.tbl          — Quest tanım
│   ├── Class.tbl          — Sınıf (Warrior/Rogue/Mage)
│   ├── JobClass.tbl       — Master sınıf
│   ├── ZONE_INFO.tbl      — Harita bilgisi
│   ├── Achieve*.tbl       — Achievement
│   ├── ITEM_SELL.tbl      — NPC alış-satış
│   ├── ITEM_UPGRADE.tbl   — Anvil oran (+1...+9)
│   ├── ITEM_PRODUCE.tbl   — Craft malzeme
│   ├── ITEM_COMPOSE.tbl   — Item birleştirme
│   ├── ITEM_ELEMENT.tbl   — Stat formula
│   ├── SPELL_EFFECT.tbl   — Skill efekt
│   └── [240 daha...]
│
├── Item\                   (540 MB — 3D model)
│   ├── item.src           — Binary N3 asset
│   ├── item.hdr           — Header
│   └── [item varyantları]
│
├── Object\                 (302 MB — harita objesi)
│   ├── object.src
│   ├── object.hdr
│   └── [NPC item drop]
│
├── UI\                     (2.9 GB — EN BÜYÜK ASSET)
│   ├── ui.src             — UI layout/resource
│   ├── ui.hdr
│   └── [login, inventory, skill window...]
│
├── Chr\                    (124 MB — Karakter model)
│   ├── *_EM.n3chr         — El Morad karakter
│   ├── *_KA.n3chr         — Karus karakter
│   ├── *.n3anim           — Animasyon
│   └── [class+race combo]
│
├── Snd\                    (49 MB — Ses)
│   ├── *.ogg              — Ogg Vorbis (login, bgm, sfx)
│   └── [zone, npc sesler]
│
├── Zones\                  (708 MB — Harita dosya)
│   ├── 1098elmo2004.smd   — El Morad zone
│   ├── 1098karus2004.smd  — Karus zone
│   ├── *.smd              — [59 daha zone]
│   └── *.aievt            — NPC spawn event
│
├── CodeGuard\              (100+ .code dosya — Şifreli script)
│   ├── Code\re_*.code     — Login akışı
│   ├── Code\macho_*.code  — NPC interaction
│   ├── Code\co_*.code     — Karakter
│   ├── Code\El_*.code     — Elmorad UI
│   └── Code\Ka_*.code     — Karus UI
│
├── ChrSelect\              — Karakter seçim ekranı
├── ByNo\                   — Bynoisee branding UI
├── SHC\                    — Karakter stat ekranı
├── FD\                     — Friend list/UI
├── DTex\                   — Doku cache
├── fx\                     — Efekt (particle)
├── Misc\                   — Çeşitli (crafting, etc)
├── KnightMovie\            — Intro video
├── Intro\                  — İntro
├── npcimg\                 — NPC yüz (açık format)
├── fonts\                  — FontFile
├── icon\                   — Item icon
├── info\                   — Bilgi ekranı
│
├── Server.ini              — [NETWORK] IP:PORT
├── Option.ini              — [DISPLAY] resolution, quality
├── Path.Ini                — [PATH] asset yolları
├── Scheduler.ini           — War timer (binary 141 KB)
├── EventAwards.ini         — Event ödül (sunucuda reflex)
├── EventSettings.ini       — Event ON/OFF (sunucuda reflex)
├── CapeBonus.txt           — Cape stat (300HP/150MP/3AP/+5NP)
├── ClanPremiumNotice.txt   — Premium buff
├── censor_words.txt        — Chat filter (30 spam kelime)
│
├── KnightOnline.exe        — Ana oyun
├── Launcher.exe            — Başlatıcı
├── Option.exe              — Grafik ayarı
├── KscViewer.exe           — Item viewer
├── KOFPL.dll               — Network lib
├── code.guard              — Anti-cheat (6 MB)
├── OpenAL32.dll
├── libvorbisfile.dll
├── Apr_Show.dll
│
└── [log dosyaları: cg_crash.log, log.klg, crash dumps]
```

---

## 4. SERVER.INI YAPISI

```ini
[NETWORK]
IP = 104.238.23.99           # Production server
PORT = 15001                 # Game port
LOGIN_PORT = 15100           # Auth port

[VERSION]
CLIENT_VERSION = 1098        # 2369 base + 1098 patch
BUILD_NUMBER = 230429        # Oluşturma tarihi

[XIGNCODE]
STATUS = 1                   # XignCode anti-cheat ON
DLL_PATH = code.guard        # DLL ismi
MONITOR_PORT = 15002         # XignCode monitor

[LANGUAGE]
DEFAULT = TR                 # Türkçe

[GRAPHICS]
RENDERER = DirectX9          # Direct3D 9.0
RESOLUTION_X = 1024
RESOLUTION_Y = 768

[ASSET_PATHS]
DATA_FOLDER = Data/
ITEM_FOLDER = Item/
OBJECT_FOLDER = Object/
UI_FOLDER = UI/
CHR_FOLDER = Chr/
```

---

## 5. OPTION.INI YAPISI (Oyuncu Ayarı)

```ini
[DISPLAY]
Resolution = 1024x768 (users custom)
FullScreen = 1 or 0
Quality = HIGH / NORMAL / LOW
FontSize = 12

[SOUND]
EnableSound = 1
Volume = 100
BGMVolume = 80
SFXVolume = 80
VoiceVolume = 80
EnableVoice = 1

[GAME]
ShowChatBubble = 1
ShowItemNames = 1
ParticleQuality = HIGH / NORMAL / LOW
MaxPlayers = 200 (render count)

[LANGUAGE]
Lang = Turkish (tr)
```

**Not:** Oyuncu değişiklikleri lokal; sunucu ayarları (Server.ini) global sync'lenir.

---

## 6. N3 ASSET FORMATI

Tüm 3D model (.src + .hdr) N3 format:
- **N3Mesh** — Harita, statik obje
- **N3PMesh** — Physics mesh (çarpışma)
- **N3Chr** — Karakter/NPC model
- **N3Joint** — Kemik yapısı
- **N3Anim** — Animasyon veri
- **N3Dxt** — Doku binary (DXT compressed)

**Dekriptleme:** Tools → `Uif-Decryptor` (UI), `tbl_decrypt.py` (tablo)

---

## 7. .TBL (Veri Tabloları)

**246 dosya, 36 MB toplam. Şifreli format (DES Feistel + K2 XOR).**

Önemli tbl'ler (MATERYAL_HARITASI.md bak):
- **Item.tbl** — Tüm item (ID, isim, tier, statlar)
- **MagicTable.tbl** — Skill listesi
- **NPC.tbl** — NPC/Monster master
- **Quest.tbl** — Quest başlıklar (detay Lua'da)
- **Class.tbl** — Sınıf adı, yetenekler
- **ZONE_INFO.tbl** — Harita bilgisi
- **Achieve*.tbl** — Achievement (5 tablo)

**Şifre çöz:** `tools/tbl_decrypt.py` (GHOST alanı, referans)

---

## 8. CODEGUARD — Anti-Cheat Koruma

### Client side

| Dosya | İş |
|-------|-----|
| **code.guard** (6 MB) | Ana anti-cheat DLL |
| **CodeGuard\Code\*.code** (3.7 MB) | RC4 şifreli UI script |

### .code Dosya Grubu

| Prefix | Amaç |
|--------|------|
| **re_*** | Login akışı (re_login_intro, re_reconnect) |
| **macho_*** | NPC dialog |
| **co_*** | Karakter (co_character_seal) |
| **El_*** | Elmorad nation spesifik |
| **Ka_*** | Karus nation spesifik |

**Şifreleme:** RC4 MYKO (SHA-1 key derivation)

---

## 9. LAUNCHER (Başlatıcı) AKIŞI

1. **Patch check** (Launcher.exe)
   - Patch server: `104.238.23.99:80`
   - `VERSION` tablo → (X, X-1, 'X.zip')
   - Fark bulunursa indir

2. **Patch uygula**
   - `patch\X.zip` → extract
   - `KnightOnline.exe` + `code.guard` + `CodeGuard\Code\*.code` güncelle

3. **XignCode init**
   - `code.guard` DLL load
   - Anti-cheat monitor başlat

4. **KnightOnline.exe** başlat
   - Server IP'sine bağlan (Server.ini)
   - Login handler

---

## 10. DEV_CLIENT vs PRODUCTION

### C:\temp\MYKO\DEV_CLIENT\

- Geliştirici/test client
- **Eski version** (Launcher'sız)
- Direct KnightOnline.exe run
- Asset yol hardcoded (`Path.Ini`)
- NEW_CLIENT/ — test build

### C:\MalaysiaKO\ (Production)

- Oyuncu client
- Launcher ile patch kontrol
- Server.ini live IP
- CodeGuard aktif
- Şifreli asset + obfuscated code

**Fark:** Test sürümü debugging amaçlı, prod sürümü protected

---

## 11. LOG SİSTEMİ

### Client Logs

| Dosya | Kaynağı |
|-------|---------|
| **cg_crash.log** | CodeGuard exception |
| **log.klg** | KnightOnline.exe output (şifreli) |
| **.dmp** | Crash dump (Windows) |

**Okunması:** Log viewer araç yok (şifreli). Exception mesaj `cg_crash.log`'da.

---

## 12. 1098 PATCH — ÖZELLİKLER

1098 patchinde eklenen:
- **Quest tablo** genişleme (Minerva vb.)
- **Skill tip** düzenlemeleri
- **Event sistem** (CSW, Lunar War, Chaos)
- **Cape bonus** (+300HP/150MP/3AP/+5NP)
- **Clan premium** (Exp+30%, Drop+1%, Noah+30%)

1098'de **YOK:**
- Bifrost expansion
- Ardream zone
- Dragon Cave
- Celestial Tower

---

## 13. PATCH SÜRECI (İLİŞKİLİ)

Oyuncu tarafında:
1. Launcher patch check
2. **patch\2370.zip** ... **patch\2373.zip** sırayla indir/apply
3. Yeni exe + dll + .code re-encrypt yapılı
4. GameServer restart (server tarafında)
5. Yeniden bağlan

**Detay:** PATCH_SURECI.md

---

## 14. DİKKAT NOKTALARI

⚠️ **Asset şifreleme 6 katman** — .tbl/.code/.uif encrypt/decrypt araçlı
⚠️ **CodeGuard aktif** — Runtime modification algılar
⚠️ **2.9 GB UI asset** — En büyük bileşen
⚠️ **Moradon M-key bug** (memory: project_moradon_map_bug.md) — map gösterim
⚠️ **Asset binary** — metin editör ile düzenlenmez

---

## SONRAKI ADIMLAR

1. **ASSET.md** — Item/Object/UI/Chr detay
2. **MAP_ZONE.md** — 11 zone (1098 prefix)
3. **LUA_QUEST.md** — 510 quest katalog
4. **GAME_LOGIC.md** — Event sistemi
5. **PATCH_SURECI.md** — Deploy akışı

---

**Dosya sürümü:** v1.0
**Yazanı:** KODCU | **İnceleme:** —
