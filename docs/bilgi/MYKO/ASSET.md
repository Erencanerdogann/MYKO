# ASSET — 3D Model, Doku, Ses, Şifreli Script

**Tarih:** 2026-04-29 | **Kategori:** GAME CLIENT | **Boyut:** 3.8 GB asset + 3.7 MB code | **Format:** N3 + RC4

---

## 1. ASSET GENEL

Oyuncunun gördüğü tüm görsel/ses içeriği:
- **Item asset** (540 MB) — İtem 3D model
- **Object asset** (302 MB) — Harita obje, mağaza
- **UI asset** (2.9 GB) — En büyük, layout + resource
- **Character asset** (124 MB) — Oyuncu/NPC model
- **Sound asset** (49 MB) — Ogg Vorbis ses

**Toplam:** ~5 GB client klasöründe

---

## 2. N3 FORMAT (Native 3D)

Bynoisee'nin özel N3 binary format. **Açık:** Polygon, vertex, bone, animasyon.

### N3 Alt Formatlar

| Format | Amaç |
|--------|------|
| **N3Mesh** | Statik harita/obje |
| **N3PMesh** | Physics mesh (çarpışma) |
| **N3Chr** | Karakter/NPC rigged model |
| **N3Joint** | Bone/skeleton yapı |
| **N3Anim** | Animasyon veri (keyframe) |
| **N3Dxt** | Doku (DXT compressed) |

**Dekriptleme:** 
- `.src + .hdr` → Binary (şifreli)
- Tools: `Uif-Decryptor` (UI), `Hyper3D converter` (model)
- **GHOST'un alanı** (referans okuyabiliriz)

---

## 3. ITEM ASSET

### Konum
```
C:\MalaysiaKO\Item\
├── item.src              (540 MB)
├── item.hdr              (header)
└── [varyantlar]
```

### İçerik

- **540 MB item.src** — Bütün item 3D modeli
- **Başına tüm item ID** → Envantere eklenince render
- **Doku:** Item ikon (icon/) + 3D mesh doku (Item/)
- **Animation:** Equip animasyon (silah sallamak, zırh giyinti)

### Item İşlemi

1. Oyuncu **item ID** drop/pickup
2. **Item.tbl** → item model ID bak
3. **item.src** → model render
4. **Item_Ext.tbl** → efekt varsa display (glow, particle)

**Ör:** ID=1001 (Iron Sword) → item.src'de sıra 1001 → 3D model render + orange glow

---

## 4. OBJECT ASSET

### Konum
```
C:\MalaysiaKO\Object\
├── object.src            (302 MB)
├── object.hdr
└── [harita obje]
```

### İçerik

- **Harita dekorasyonu** (ağaç, kaya, bina)
- **NPC spawn nokta** (fiziksel obje yerine)
- **Shopkeeper stand** (alış-satış NPC yeri)
- **Anvil** (upgrade NPC yeri)
- **Teleport gate** (portal)

### Kullanım

Map loader → Zone açılırken:
1. **.smd zone dosyası** (harita verileri)
2. **NPC_Pos.tbl** (NPC spawn)
3. **object.src** → obje render (dekor)

---

## 5. UI ASSET (EN BÜYÜK)

### Konum
```
C:\MalaysiaKO\UI\
├── ui.src                (2.9 GB!) ← En büyük asset
├── ui.hdr
└── [login, inventory, menu...]
```

### İçerik

**2.9 GB — Oyunun tüm UI elementleri:**
- **Login ekranı** (karakter seçim, password)
- **Inventory** (envanter grid 28 slot)
- **Skill window** (spell bar)
- **Character sheet** (stat görüntü)
- **Chat window**
- **NPC dialog** (quest, trade)
- **Mini-map** (harita fragment)
- **Status bar** (HP/MP/EXP)
- **Nation symbol** (Karus kızıl, Elmorad mavi)

### UI Yapı

- **Layout:** Psd/Adobe XD → compile binary
- **Resource:** Font, icon, button texture
- **Script:** CodeGuard\Code\*.code (ayrı)

**Not:** En büyük boyut asset → UI resource paketi

---

## 6. CHARACTER ASSET

### Konum
```
C:\MalaysiaKO\Chr\
├── *_EM.n3chr           — El Morad karakter
├── *_KA.n3chr           — Karus karakter
├── *.n3anim             — Animasyon
├── *.n3joint            — Bone
└── [10+ class+race combo]
```

### İçerik

**124 MB karakter 3D model + animasyon:**

| Dosya Türü | Amaç |
|------------|------|
| `*_EM.n3chr` | El Morad sınıf modeli (erkek/kadın) |
| `*_KA.n3chr` | Karus sınıf modeli (erkek/kadın) |
| `*.n3anim` | Animasyon (idle, walk, run, attack, death) |
| `*.n3joint` | Skeleton (kol/bacak/gövde bone) |

### Sınıf Varyantları

- **Warrior_EM, Warrior_KA** — Savaşçı
- **Rogue_EM, Rogue_KA** — Hırsız
- **Mage_EM, Mage_KA** — Büyücü
- **Priest_EM, Priest_KA** — Rahip
- **Kurian, Porutu** — Transform sınıf

### Animasyon Setleri

- **Idle** — Bekleme
- **Walk** — Yürüme
- **Run** — Koşma
- **Attack1/Attack2/Attack3** — Saldırı combo
- **Skill** — Büyü yapma
- **Hit** — Hasar aldı
- **Death** — Ölüm
- **Sit, Sleep, Dance** — Emote

**Not:** Equipment (silah, zırh) — **ayrı model blend** (tidak bawaan, item render overlay)

---

## 7. SOUND ASSET

### Konum
```
C:\MalaysiaKO\Snd\
├── *.ogg                — Ogg Vorbis (128 kbps)
└── [49 MB toplam]
```

### Ses Kategorileri

| Kategori | Örnek |
|----------|-------|
| **Login** | login_ok.ogg, char_select.ogg |
| **BGM** | zone_bgm.ogg (Moradon, Ronark, Dungeon) |
| **Skill** | fireball.ogg, heal.ogg |
| **Hit** | punch.ogg, sword_hit.ogg |
| **UI** | button_click.ogg, menu_open.ogg |
| **NPC** | merchant_voice.ogg (sesli NPC) |
| **Event** | war_horn.ogg, castle_siege.ogg |

### Özellikler

- **Codec:** Ogg Vorbis (OpenAL32.dll + libvorbisfile.dll)
- **Bitrate:** 128 kbps (49 MB = ~1 saat)
- **3D Audio:** Oyuncu konumu → ses direction

---

## 8. CODEGUARD CODE DOSYALARI

### Konum
```
C:\MalaysiaKO\CodeGuard\
├── Code\*.code           (3.7 MB, şifreli script)
└── [100+ dosya]
```

### .code Dosya Grupları

#### Login Flow (.code)

| Dosya | İş |
|-------|-----|
| **re_login_intro.code** | Giriş ekranı |
| **re_login_select.code** | Karakter seçim |
| **re_reconnect.code** | Yeniden bağlan |
| **re_create_char.code** | Karakter oluştur |

#### NPC Interaction (.code)

| Dosya | İş |
|-------|-----|
| **macho_npc_dialog.code** | NPC konuşma |
| **macho_merchant.code** | Alış-satış window |
| **macho_quest.code** | Quest dialog |
| **macho_skill.code** | Skill öğren |

#### Character UI (.code)

| Dosya | İş |
|-------|-----|
| **co_character_info.code** | Stat ekranı |
| **co_inventory.code** | Envanter |
| **co_skill_window.code** | Skill bar |
| **co_character_seal.code** | Karakter lock/unlock |

#### Nation-Specific (.code)

| Dosya | Amaç |
|-------|------|
| **El_morad_*.code** | El Morad başlangıç UI |
| **El_*.code** | Elmorad nation spesifik |
| **Ka_karus_*.code** | Karus başlangıç UI |
| **Ka_*.code** | Karus nation spesifik |

### .code Şifreleme

- **Şifresi:** RC4 MYKO
- **Key:** SHA-1 derivation (Windows CryptoAPI)
- **Yeniden şifre:** `tools\key_rotation\rc4_re_encrypt.py` (GHOST)

---

## 9. DEKRIPTLEME TOOLS

Tüm asset şifreli. **Referans amaçlı açma:**

### TBL Dekriptleme
```bash
python tools/tbl_decrypt.py Data/Item.tbl
```
**Output:** Plain text (Item ID, name, stat)

### UI Asset Dekriptleme
```
tools\Uif-Decryptor\ — GUI tool
→ ui.src input
→ decompiled layout export
```

### N3 Viewer
```
N3TexViewerPNG.exe — Doku preview
→ item.src drop
→ model görüntüle
```

### .code Decompile
- **Açık:** RC4 key varsa decrypt → Lua/script decompile
- **Kapalı:** GHOST domain
- **Reference:** `tools\CODE_analysis\` (eski .code örnekleri)

---

## 10. ASSET PIPELINE (YÖNETİMİ)

### Dev → Test → Prod

1. **Dev Client** (`C:\temp\MYKO\DEV_CLIENT\`)
   - Yeni asset test
   - Unencrypted version (debugging)

2. **Test Client** (`C:\MalaysiaKO\`)
   - Asset encrypt
   - Patch check

3. **Deploy**
   - Patch zip oluştur (diff)
   - SSH upload
   - Oyuncu patch download

### Encrypt Adımları (GHOST domain)

```bash
# 1. .tbl şifrele
python tools/key_rotation/tbl_re_encrypt.py Item.tbl

# 2. .code şifrele  
python tools/key_rotation/rc4_re_encrypt.py *.code

# 3. .uif şifrele
tools/Uif-Encryptor/ (GUI)

# 4. Patch zip yap
patch_tool.py → 2373.zip oluştur

# 5. SSH upload
ssh upload patch/2373.zip → 104.238.23.99
```

---

## 11. ASSET HATALARı VE FIX

### Bilinen Sorunlar

| Sorun | Çözüm |
|-------|-------|
| **Item model yok** | Item.tbl model ID → item.src sıra check |
| **UI glitch** | ui.src corrupt → backup restore |
| **NPC animasyon stuck** | n3anim keyframe → recompile |
| **Doku flickering** | DXT compression → quality up |
| **Ses crackling** | OGG bitrate arttır (128→192) |

### Moradon M-key Bug (memory)

- **Sorun:** Map Moradon'da M-key (mini-map) yanlış konum gösteriyor
- **Sebebi:** ui.src ile map.smd farkı
- **Çözüm:** (bekliyor — CHIP detaylı check)

---

## 12. 1098 PATCH — ASSET DEĞİŞİKLİKLERİ

1098 patchinde:
- **Skill efekt** Update (CodeGuard\Code\)
- **NPC model** Düzenleme (Chr\ varyant ekle)
- **UI layout** Mod (ui.src genişlemesi)
- **Event UI** Ekleme (CSW, Lunar War button)

**Eklenmiş:** Bifrost UI element → 1098'de removed (Bifrost yok)

---

## 13. DİKKAT NOKTALARI

⚠️ **5 GB asset** — patching yavaş, diff small tutmak önemli
⚠️ **2.9 GB UI asset** — en risky, kırılırsa client login yapamaz
⚠️ **N3 format proprietary** — açık doküman yok (reverse engineer)
⚠️ **6 katman şifreleme** — dekrypt tools GHOST'a sor
⚠️ **RC4 key rotation** — patch versiyon → yeni key → sorunlar mümkün
⚠️ **Character asset race+class combo** — unbalanced düzenlemeler gameplay etkileyebilir

---

## 14. DOSYA ÖZETİ

| Bileşen | Boyut | Durumu | Önemli |
|---------|-------|--------|--------|
| Item asset | 540 MB | Stable | Yüksek |
| Object asset | 302 MB | Stable | Orta |
| UI asset | 2.9 GB | Kritik | Çok yüksek |
| Character asset | 124 MB | Stable | Orta |
| Sound asset | 49 MB | Stable | Düşük |
| CodeGuard code | 3.7 MB | Protected | Yüksek |

---

## 15. KAYNAKLARA BAĞLA

- **MATERYAL_HARITASI.md** → Asset tablosu
- **CLIENT_HARITA.md** → Client yapı
- **PATCH_SURECI.md** → Asset deploy
- **GHOST domain** → Encryption/tools detay
- **CHIP domain** → Server asset sync

---

**Dosya sürümü:** v1.0
**Yazanı:** KODCU | **İnceleme:** —
