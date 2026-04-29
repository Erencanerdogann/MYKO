# 🎮 MYKO'ya Özel Komut Listesi (Agent + Operasyon)

**Tarih:** 2026-04-29 (S87)
**Hedef:** MYKO oyun operasyonu için agent komutları + DOKTOR komutları.
**Yer:** Bu liste DOKTOR.md ve agent CLAUDE.md'lerine gömülecek.

---

## A. MEVCUT ORKESTRA KOMUTLARI — MYKO'da Nasıl Kullanılır

### Görev Yönetimi

```bash
O=/c/orkestra/orkestra.exe

# Görev ata — proje + kategori KRITIK
$O gorev <AGENT> "..." --oncelik <ACIL/YUKSEK/NORMAL/DUSUK> --kategori <DB/GAME/WEB/KOD/GUVENLIK/...> --proje MYKO-AI

# Aktif görevleri listele (sadece MYKO)
$O gorevler MATRIX --proje MYKO-AI

# İlerleme bildir
$O ilerleme MATRIX MAT-26 50

# Bitir
$O bitti MATRIX MAT-26
```

### Mesaj
```bash
# MYKO konuşması
$O mesaj DOKTOR MATRIX "CTX: ... | TASK: ... | OUT: ... | LIMIT: ... | [PUAN:N]" --proje MYKO-AI

# Acil mesaj (öncelik=ACIL)
$O acil-mesaj DOKTOR CHIP "Wall cheat blocker — hemen bak"

# Kritik mesaj (öncelik=KRITIK, lansman/güvenlik)
$O kritik-mesaj DOKTOR GHOST "strWebHash NULL bug — production etkili"

# Mesaj oku
$O gel DOKTOR --proje MYKO-AI
```

### Hafıza
```bash
# MYKO öğrenme/keşif notu
$O hafiza-ekle DOKTOR "S87 KESIF - RogACS kaynak" --icerik "..." --proje MYKO-AI

# Notları listele
$O hafiza-notlar DOKTOR --proje MYKO-AI
```

### Rapor
```bash
$O rapor-yaz DOKTOR "S87 ozet"
$O raporlar DOKTOR
$O gunluk-ozet
```

### Kapanış
```bash
$O kapanis MATRIX "KAPANIS: ... | KALAN: ... | BLOCKER: ..."
$O durum-guncelle MATRIX IDLE
$O durum-mesaj MATRIX "Session tamamlandi"
```

---

## B. MYKO'YA ÖZEL — YENİ İHTİYAÇ KOMUTLAR

⚠️ Bunlar **henüz yok**. Lansman öncesi RUSTIK eklesin (CLI işi).

### Oyun Operasyonu

```bash
# DB sorgu (KO_MYKO MSSQL — read-only)
$O myko-db "SELECT TOP 10 * FROM USERDATA WHERE Level = 72"

# TBL extract (.tbl decrypt)
$O myko-tbl-decrypt "Item.tbl"

# TBL düzenle (single satır, otomatik backup + hash güncelle)
$O myko-tbl-edit "Item.tbl" --row 100 --col 5 --value 1500 --reason "TS scroll fix"

# TBL_HASH yeniden hesapla + INI güncelle
$O myko-tbl-hash-update

# Lua quest kontrol (syntax check)
$O myko-lua-check "Quests/14204_Minerva.lua"

# Item ara (ID veya isim)
$O myko-item-search "transformation scroll"
$O myko-item-info 381001000

# Skill ara
$O myko-skill-info <skillID>

# NPC ara
$O myko-npc-info <NPCID>

# Zone listesi (1098 prefix)
$O myko-zone-list
```

### Patch / Deploy

```bash
# Patch zip oluştur (file diff'ten)
$O myko-patch-build --version 2374 --files "Data/Item.tbl,UI/login.uif"

# Patch yükle
$O patch-yukle 2374.zip   (mevcut)

# Deploy hazırla
$O deploy-hazirla 2374    (mevcut)

# Deploy uygula (SSH + DB)
$O deploy-uygula 2374     (mevcut)

# Deploy durum
$O deploy-durum           (mevcut)
```

### Sunucu Kontrol

```bash
# Production GameServer status
$O myko-status

# Online oyuncular (production DB query)
$O myko-online

# Server restart (acil)
$O myko-restart-game

# Login durdur (bakım modu)
$O myko-maintenance on
$O myko-maintenance off
```

### Log / Monitor

```bash
# GameServer son loglar
$O myko-log GameServer 100

# Login son loglar
$O myko-log LoginServer 50

# Crash dump var mı
$O myko-crash-check

# Pearl Monitor başlat
$O myko-monitor-start
$O myko-monitor-stop
```

### Bilgi Sorgu (yeni 22 MD'den)

```bash
# Kategori MD'leri göster
$O myko-bilgi DB              # → DB_SEMA, DB_STORED_PROC, TBL_KATALOG, TBL_HASH
$O myko-bilgi GAME            # → GAME_LOGIC, MAP_ZONE, LUA_QUEST, ASSET, CLIENT_HARITA, PATCH_SURECI
$O myko-bilgi WEB             # → WEB_PHP, WEB_API, WEB_FORUM, WEB_BUG
$O myko-bilgi KOD             # → SRC_HARITA, SRC_ONEMLI_CPP, ANTI_CHEAT, BUILD
$O myko-bilgi GUVENLIK        # → SIFRELEME, KEY_ROTATION, TOOLS, GUVENLIK_BUG

# Tam metin arama (FTS5)
$O myko-ara "1098 patch"
$O myko-ara "wall cheat"
$O myko-ara "RogACS"

# Sözlük (REHBER üretecek)
$O myko-sozluk anvil
$O myko-sozluk NP
```

### GM / Operasyon

```bash
# GM yetki ver (USERDATA.Authority=2 + GAME_MASTER_SETTINGS)
$O myko-gm-ekle <username>
$O myko-gm-cikar <username>
$O myko-gm-list

# Karakter ara
$O myko-char-find <username>

# Ban/Unban
$O myko-ban <username> <gun> "<sebep>"
$O myko-unban <username>

# Item ver (GM)
$O myko-give-item <username> <itemID> <count>
```

---

## C. AGENT BAZINDA YETKILER

### Komut → Yetki Matrisi

| Komut | DOKTOR | MATRIX | CHIP | KODCU | WEBRA | GHOST | RUSTIK |
|-------|--------|--------|------|-------|-------|-------|--------|
| `gorev`, `bitti` | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| `mesaj`, `gel` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `myko-db SELECT` | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| `myko-db INSERT/UPDATE/DELETE` | ✅ onay | ⚠️ onay | ❌ | ❌ | ❌ | ❌ | ❌ |
| `myko-tbl-decrypt` | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| `myko-tbl-edit` | ✅ onay | ⚠️ onay | ❌ | ❌ | ❌ | ❌ | ❌ |
| `myko-lua-check` | ✅ | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ |
| `myko-item-search`, `info` | ✅ | ✅ | ❌ | ✅ | ❌ | ❌ | ❌ |
| `myko-zone-list` | ✅ | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ |
| `myko-patch-build` | ✅ onay | ❌ | ❌ | ✅ onay | ❌ | ❌ | ❌ |
| `deploy-uygula` (production) | ✅ onay | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ onay |
| `myko-status`, `online`, `log` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `myko-restart-game` | ✅ onay | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ onay |
| `myko-gm-ekle`, `ban`, `give-item` | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| `myko-bilgi`, `ara`, `sozluk` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `myko-monitor-*` | ✅ | ❌ | ✅ | ✅ | ❌ | ✅ | ❌ |

⚠️ = onay gerekli (DOKTOR onayı şart)
❌ = yetkisiz (deneme yasak)

---

## D. PROJE FILTER — ZORUNLU

Tüm DB komutlarında `--proje MYKO-AI` veya `--proje ORKESTRA-AI` ZORUNLU olacak (RUSTIK fix sonrası).

**Şimdilik (geçici):**
- `gorevler <AGENT>` → tüm projeler döner, agent eline ne gelirse alır
- `mesaj` → proje yok, herkese gider
- `hafiza-ekle` → proje yok

**Yeni (RUSTIK fix sonrası):**
```bash
$O gorevler MATRIX --proje MYKO-AI    # sadece oyun
$O gorevler RUSTIK --proje ORKESTRA-AI # sadece sistem
$O mesaj X Y "..." --proje MYKO-AI
$O hafiza-ekle X "..." --proje MYKO-AI
```

**Default davranış:** `--proje` yoksa **MYKO-AI** (oyun ana iş).

---

## E. KISALT MA / ALIAS

DOKTOR + agent'ların sık kullanacağı kısaltmalar:

```bash
alias O='/c/orkestra/orkestra.exe'
alias myko-todo='$O gorevler --proje MYKO-AI'
alias myko-gel='$O gel --proje MYKO-AI'
alias myko-status='$O myko-status'  # yeni
```

---

## F. AGENT BRIEF'LERİNDE OTOMATİK GÖMÜLECEK

Her agent açıldığında, **kendi alanına ait komutları** açılış prompt'unda görsün:

### MATRIX brief'i — alt bölüm
```markdown
## SENİN KOMUTLARIN
- $O myko-db "SELECT ..."          ✅
- $O myko-tbl-decrypt              ✅
- $O myko-tbl-edit ...             ⚠️ DOKTOR onayı
- $O myko-tbl-hash-update          ⚠️ onay
- $O myko-item-info <ID>           ✅
```

### CHIP brief'i
```markdown
## SENİN KOMUTLARIN
- $O myko-status                   ✅
- $O myko-log GameServer           ✅
- $O myko-monitor-start            ✅
- $O myko-crash-check              ✅
```

### KODCU brief'i
```markdown
## SENİN KOMUTLARIN
- $O myko-lua-check                ✅
- $O myko-item-search/info         ✅
- $O myko-zone-list                ✅
- $O myko-patch-build              ⚠️ onay
```

### WEBRA brief'i
```markdown
## SENİN KOMUTLARIN
- $O myko-online                   ✅
- WebFetch + curl                  ✅ (test endpoint)
```

### GHOST brief'i
```markdown
## SENİN KOMUTLARIN
- $O myko-monitor-start            ✅
- $O myko-crash-check              ✅
- $O myko-log                      ✅
- ⚠️ Sunucu erişim YASAK (S42)
```

### RUSTIK brief'i
```markdown
## SENİN KOMUTLARIN
- cargo build --release            ✅ (lokal)
- $O deploy-uygula                 ⚠️ DOKTOR onayı
- $O myko-restart-game             ⚠️ acil onay
- Source code tüm                  ✅ (oku/yaz)
```

---

## G. SONRAKİ ADIMLAR (Şu Sıra)

### Hemen (S87 sonu)
1. ✅ Bu listeyi DOKTOR.md'ye gömeyim mi (sen onayla)
2. ✅ Agent CLAUDE.md'lerine **kendi komut alt bölümünü** ekleyim mi
3. ⏳ RUSTIK'e **`--proje` flag** brief
4. ⏳ Wall cheat fix (kritik)

### Sonra (yeni session)
5. RUSTIK MYKO-özel komutları implement (Rust koduna)
   - `myko-db`, `myko-tbl-*`, `myko-lua-check`, `myko-item-*`, `myko-zone-list`
   - `myko-patch-build`, `myko-status`, `myko-online`
   - `myko-gm-*`, `myko-ban`, `myko-give-item`
   - `myko-monitor-*`, `myko-bilgi`, `myko-ara`, `myko-sozluk`
6. Test fixture (her komut + yetki)
7. DOKTOR.md + 9 agent CLAUDE.md güncellenir

---

## H. SOR — KARAR SENİN

1. **Bu listeyi onaylar mısın?** Eklenecek/çıkarılacak komut var mı?
2. **DOKTOR.md'ye şimdi gömeyim mi**, yoksa RUSTIK implement edince mi?
3. **Hangi yetki matrisi onaylanır** (kim ne yapabilir)?

Söyle, ona göre.
