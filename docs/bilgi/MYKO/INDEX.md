# 📚 MYKO BİLGİ İNDEKSİ — Mağara Modu

**Bu dosya AGENT'IN İLK BAKACAĞI YERDİR.** Arama YAPMA. Soru → MD eşlemesi burada.

**Tarih:** 2026-04-29 (S87)
**Yer:** `C:\temp\MYKO\docs\bilgi\MYKO\INDEX.md`
**Mantık:** Token tasarrufu — aramaya/keşfe gerek yok, soru → tek satır cevap.

---

## 🎯 SORU → MD/KOMUT (Hızlı Bakış)

### Oyun Temel
| Soru | Cevap |
|------|-------|
| KO nedir, sınıf, ırk, harita? | `_KO_TEMEL_HERKES_OKU.md` |
| Bizim sürüm? (2369+1098) | `_KO_TEMEL_HERKES_OKU.md` § 1098 |
| Patron geçmişi, emek? | `ERENCAN_GECMIS.md` |
| Eski projelerden ders? | `PROJE_TARIHCESI_VE_DERSLER.md` |
| KO öğrenme yolu? | `_OGRENME_PLANI.md` |
| İnternet kaynak (Wiki/Reddit/RogACS)? | `_KAYNAK_HAVUZU.md` |
| Sistem haritası (GAME/DB/SRC/WEB)? | `Mykoproject.map.md` |
| Element ↔ dosya eşlemesi? | `MATERYAL_HARITASI.md` |

### DB / Tablo / SP
| Soru | Cevap |
|------|-------|
| Tablo listesi, kolon, ilişki? | `DB_SEMA.md` |
| USERDATA, USER_ITEM, ITEM_SELLTABLE detay? | `DB_SEMA.md` |
| Stored procedure (LOAD_USER_DATA vb)? | `DB_STORED_PROC.md` |
| 246 .tbl dosyası ne? | `TBL_KATALOG.md` |
| TBL_HASH validation? | `TBL_HASH.md` |
| Item ID 381001000? | `$O myko-item-info 381001000` (yeni komut) |
| DB schema sorgusu? | `$O myko-db "SELECT ..."` |
| Auto-register strWebHash bug? | `WEB_BUG.md`, `DB_SEMA.md § TB_USER` |

### Source Code (C++)
| Soru | Cevap |
|------|-------|
| GameServer modüller? | `SRC_HARITA.md` |
| User.cpp, MagicInstance ne yapar? | `SRC_ONEMLI_CPP.md` |
| Anti-cheat (Pearl Guard)? | `ANTI_CHEAT.md` |
| Build / VS 2022? | `BUILD.md` |
| Wall cheat detection? | `ANTI_CHEAT.md`, `SRC_ONEMLI_CPP.md`, ⚠️ `CharacterMovementHandler.cpp:264` KAPALI |
| Login port (15100/15200)? | `BUILD.md § Patlama dersleri` |
| Skill formül kodu? | `MagicProcess.cpp` (src) |

### Client / Asset / Map / Lua
| Soru | Cevap |
|------|-------|
| KnightOnline.exe yapısı? | `CLIENT_HARITA.md` |
| N3 asset (mesh, anim, texture)? | `ASSET.md` |
| Zone listesi (1098 prefix 11 zone)? | `MAP_ZONE.md` |
| Lua quest (510 dosya, RogACS)? | `LUA_QUEST.md` |
| Lua API (CheckNation, SelectMsg)? | `LUA_QUEST.md § API` |
| 01_main.lua, *_Move.lua, *_SPELLI.lua? | `LUA_QUEST.md § Özel dosyalar` |
| Event sistemi (CSW/BDW/Chaos/Juraid)? | `GAME_LOGIC.md` |
| Patch zip + deploy? | `PATCH_SURECI.md` |
| Moradon M-key bug? | `MAP_ZONE.md` |

### Web / Site
| Soru | Cevap |
|------|-------|
| koweb2 PHP yapısı? | `WEB_PHP.md` |
| Site API endpoint? | `WEB_API.md` |
| Flarum forum? | `WEB_FORUM.md` |
| strWebHash NULL bug? | `WEB_BUG.md` |
| TLS yok / MITM? | `WEB_BUG.md` |
| Register/login akışı? | `WEB_PHP.md § Akış` |

### Güvenlik / Şifreleme / Tools
| Soru | Cevap |
|------|-------|
| 6 katman şifreleme (JvCryption/RC4/DES/XOR)? | `SIFRELEME.md` |
| Key rotation pipeline? | `KEY_ROTATION.md` ⚠️ kısa, v2'de zenginleşecek |
| TBL/UIF/Patch toolları? | `TOOLS.md` ⚠️ kısa, v2'de zenginleşecek |
| Bilinen exploit (98 bulgu)? | `GUVENLIK_BUG.md` |
| Pearl Guard port (9 faz)? | `ANTI_CHEAT.md`, `GUVENLIK_BUG.md` |
| Packet shift K1-K10? | `GUVENLIK_BUG.md` |

### Komut / Operasyon
| Soru | Cevap |
|------|-------|
| MYKO komutları listesi? | `MYKO_KOMUTLARI.md` |
| Hook sistemi nasıl çalışır? | `C:\orkestra\KATEGORI_MD_HOOK\README.md` |
| Görev kategorisi → MD eşleme? | `C:\orkestra\kategori_md_map.json` |

### Sunucu / Yol / Deploy
| Soru | Cevap |
|------|-------|
| Production sunucu IP/yol/port? | `SUNUCU_DOSYA_YOLLARI.md` |
| Lokal dev server yolu? | `SUNUCU_DOSYA_YOLLARI.md § 2` |
| Client (C:\MalaysiaKO) yapısı? | `SUNUCU_DOSYA_YOLLARI.md § 2`, `CLIENT_HARITA.md`, `CLIENT_SETUP.md` |
| Yedekler nerede (F:\)? | `SUNUCU_DOSYA_YOLLARI.md § 5` |
| Deploy komutları (scp/ssh)? | `SUNUCU_DOSYA_YOLLARI.md § 7` |
| INI port/DSN değerleri? | `SUNUCU_DOSYA_YOLLARI.md § 1`, § 2` |
| Agent klasör yapısı? | `SUNUCU_DOSYA_YOLLARI.md § 4` |

### Lansman / GM / Operasyon (S88 yeni)
| Soru | Cevap |
|------|-------|
| GM komut listesi (200 komut)? | `GM_KOMUT.md` |
| Yetki sistemi (Authority)? | `GM_KOMUT.md § 1` |
| Event takvimi / saatleri? | `EVENT_TAKVIMI.md` |
| EVENT_SCHEDULE DB tablosu? | `EVENT_TAKVIMI.md § 1` |
| Lansman checklist (T-24, T-2)? | `LANSMAN_CHECKLIST.md` |
| 4 BLOCKER (wall cheat, hash, scroll, smoke)? | `LANSMAN_CHECKLIST.md § BLOCKER` |
| Cash shop (PUS) sistemi? | `CASH_SHOP_PUS.md` |
| PUS_ITEMS / PUS_CATEGORY? | `CASH_SHOP_PUS.md § 2` |
| NPC drop / loot tablosu? | `NPC_DROP_LOOT.md` |
| K_MONSTER_ITEM (12 slot)? | `NPC_DROP_LOOT.md § 2` |
| Log dosyaları nerede, ne anlam? | `LOG_MONITORING.md` |
| Crash recovery prosedürü? | `CRASH_RECOVERY.md` |
| Backup / restore (saatlik DB)? | `BACKUP_RESTORE.md` |
| Player support / ticket akışı? | `PLAYER_SUPPORT.md` |
| Client setup (oyuncu kurulum)? | `CLIENT_SETUP.md` |
| Smoke test (lansman öncesi)? | `SMOKE_TEST.md` |

---

## 📂 TÜM MD'LER (Alfabetik, 29 dosya)

| MD | Boyut | Yazan | Kategori |
|----|-------|-------|----------|
| `_KAYNAK_HAVUZU.md` | 19 KB | DOKTOR | Referans |
| `_KO_TEMEL_HERKES_OKU.md` | 5.7 KB | DOKTOR | **HERKES OKUR** |
| `_OGRENME_PLANI.md` | 3.7 KB | DOKTOR | Plan |
| `ANTI_CHEAT.md` | 9.8 KB | CHIP | KOD |
| `ASSET.md` | 9.7 KB | KODCU | GAME |
| `BUILD.md` | 7.8 KB | CHIP | KOD |
| `CLIENT_HARITA.md` | 11.2 KB | KODCU | GAME |
| `DB_SEMA.md` | 14.0 KB | MATRIX | DB |
| `DB_STORED_PROC.md` | 8.9 KB | MATRIX | DB |
| `ERENCAN_GECMIS.md` | 13.2 KB | DOKTOR | **HERKES OKUR** |
| `GAME_LOGIC.md` | 9.9 KB | KODCU | GAME |
| `GUVENLIK_BUG.md` | 6.3 KB | GHOST | GUVENLIK |
| `INDEX.md` | bu dosya | DOKTOR | **HERKES İLK BAKAR** |
| `KEY_ROTATION.md` | 0.8 KB ⚠️ | GHOST (v2 bekl.) | GUVENLIK |
| `LUA_QUEST.md` | 34 KB | KODCU | GAME |
| `MAP_ZONE.md` | 8.9 KB | KODCU | GAME |
| `MATERYAL_HARITASI.md` | 19 KB | DOKTOR | Referans |
| `MYKO_KOMUTLARI.md` | yeni | DOKTOR | Komut |
| `Mykoproject.map.md` | 17.9 KB | DOKTOR | Sistem |
| `PATCH_SURECI.md` | 9.7 KB | KODCU | GAME |
| `PROJE_TARIHCESI_VE_DERSLER.md` | 18.3 KB | DOKTOR | **HERKES OKUR** |
| `SIFRELEME.md` | 5.6 KB | GHOST | GUVENLIK |
| `SRC_HARITA.md` | 17.8 KB | CHIP | KOD |
| `SRC_ONEMLI_CPP.md` | 8.4 KB | CHIP | KOD |
| `TBL_HASH.md` | 7.5 KB | MATRIX | DB |
| `TBL_KATALOG.md` | 10.5 KB | MATRIX | DB |
| `TOOLS.md` | 4.2 KB ⚠️ | GHOST (v2 bekl.) | GUVENLIK |
| `WEB_API.md` | 8.5 KB | WEBRA | WEB |
| `WEB_BUG.md` | 7.2 KB | WEBRA | WEB |
| `WEB_FORUM.md` | 6.6 KB | WEBRA | WEB |
| `WEB_PHP.md` | 15 KB | WEBRA | WEB |

**Toplam:** 29 MD, ~330 KB, ~10000+ satır.

---

## 🎮 KATEGORİ → MD LİSTESİ (Hook Eşlemesi)

Hook bunu okur, agent açılışında verir. **Agent başka MD aramaz.**

| Kategori | Zorunlu MD'ler |
|----------|----------------|
| **DB** | `_KO_TEMEL` + `INDEX` + `DB_SEMA` + `DB_STORED_PROC` + `TBL_KATALOG` + `TBL_HASH` |
| **GAME** | `_KO_TEMEL` + `INDEX` + `GAME_LOGIC` + `MAP_ZONE` + `LUA_QUEST` + `ASSET` + `CLIENT_HARITA` + `PATCH_SURECI` |
| **KOD** | `_KO_TEMEL` + `INDEX` + `SRC_HARITA` + `SRC_ONEMLI_CPP` + `ANTI_CHEAT` + `BUILD` |
| **WEB** | `_KO_TEMEL` + `INDEX` + `WEB_PHP` + `WEB_API` + `WEB_FORUM` + `WEB_BUG` |
| **GUVENLIK** | `_KO_TEMEL` + `INDEX` + `SIFRELEME` + `KEY_ROTATION` + `TOOLS` + `GUVENLIK_BUG` |
| **DEPLOY** | `_KO_TEMEL` + `INDEX` + `PATCH_SURECI` + `BUILD` |
| **BUG** | `_KO_TEMEL` + `INDEX` + `GUVENLIK_BUG` + `WEB_BUG` + `PROJE_TARIHCESI_VE_DERSLER` |
| **PANEL** | `_KO_TEMEL` + `INDEX` + `WEB_PHP` |
| **DEFAULT** | `_KO_TEMEL` + `INDEX` |

**Önemli:** Her kategori başında **`_KO_TEMEL_HERKES_OKU.md` + `INDEX.md`** ZORUNLU. Sonra kategori spesifik MD'ler.

---

## 🚫 ARAMA YASAK — Yapılacaklar

### Bunu YAPMA
```
❌ find . -name "*.md"          → ZAMAN İSRAFI
❌ ls -R                         → token israfı
❌ grep -r "item"                → arama, INDEX'ten bul
❌ "MD'leri tara"                → her şey indekslendi
❌ Klasör keşfi                  → INDEX'e bak
```

### Bunu YAP
```
✅ Read INDEX.md                  → soru-cevap bul
✅ Read <kategori MD>             → konu detay
✅ $O myko-item-info <ID>         → DB sorgu (yeni)
✅ $O myko-bilgi DB               → kategori MD listesi (yeni)
✅ DOKTOR'a sor                   → bilmiyorsan mesaj at
```

---

## 🛠️ KOMUT ÖZET (Detay → `MYKO_KOMUTLARI.md`)

```bash
O=/c/orkestra/orkestra.exe

# Görev/Mesaj/Hafıza
$O gorevler <AGENT> --proje MYKO-AI
$O gel <AGENT> --proje MYKO-AI
$O mesaj <FROM> <TO> "..."
$O hafiza-ekle <AGENT> "baslik" --icerik "..."

# Bilgi sorgu (yeni — RUSTIK eklicek)
$O myko-bilgi <KATEGORI>          # kategori MD listesi
$O myko-ara "anahtar kelime"      # FTS5 arama
$O myko-item-info <ID>
$O myko-skill-info <ID>
$O myko-zone-list

# Kapanış
$O kapanis <AGENT> "KAPANIS: ... | KALAN: ... | BLOCKER: ..."
```

---

## ⚠️ KRİTİK NOTLAR

1. **`_KO_TEMEL_HERKES_OKU.md` + bu INDEX.md** her agent açılışta ZORUNLU
2. **Halüsinasyon yasak** — bilmediğin yere `[KAYNAK YOK — TEYIT]`
3. **Production sunucuya dokunma** — onay olmadan
4. **GHOST sunucu YASAK** (S42 kalıcı policy)
5. **JERRY SSH/RDP/DB YASAK** (S52)
6. **Wall cheat detection KAPALI** (`CMH.cpp:264`) — lansmandan önce ŞART
7. **PreToolUse hook bug** — sürdürülüyor, RUSTIK fix kuyrukta

---

## 📅 SON GÜNCELLEME

- 2026-04-29 — DOKTOR ilk yazım (S87 sonu, 29 MD katalogla)
- Sonraki: hook v2 + `kategori_md_map.json` güncellemesi (RUSTIK işi)
