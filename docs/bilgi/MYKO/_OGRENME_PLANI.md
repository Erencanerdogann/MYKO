# MYKO Öğrenme Planı (Knight Online — 1098 patch / 2369 base)

**Hedef:** Internet kaynaklarından (sadece) Knight Online oyununu temel-orta-ileri seviyede öğrenmek.
**Kapsam:** Oyun mekaniği, klan/savaş sistemi, item, skill, sınıf, harita, görev, ekonomi.
**Hariç (şimdilik):** DB schema, src kod yapısı — bunlar dosya okutulduğunda öğrenilecek.

---

## SIRALAMA — Hangisinden Başlamalı

### FAZ 1 — Temel (oyun nedir, nasıl başlar)
1. **README.md** — oyun özeti (MMORPG, PvP odaklı, 2 ırk, 8 sınıf)
2. **00_GENEL_BAKIS.md** — tarih, yapımcı, önemli versiyonlar (1098 patch nedir, 2369 client nedir)
3. **01_IRK_VE_SINIF.md** — Karus / El Morad ırkları, sınıflar (Warrior, Rogue, Mage, Priest)
4. **02_OYUN_BASLAMA.md** — yeni karakter oluşturma, level 1-30 yolu, başlangıç haritaları

### FAZ 2 — Sistemler (mekanik nasıl çalışır)
5. **03_SKILL_SISTEMI.md** — skill ağaçları, master skill, transform, parti skilleri
6. **04_ITEM_SISTEMI.md** — item slot, anvil/upgrade (+1...+9), unique, accessory, rebirth
7. **05_LEVEL_VE_XP.md** — leveling spotları, premium, double exp, NP (national point)
8. **06_PARTI_VE_KLAN.md** — parti kuralları, klan sistemi, klan rütbeleri, klan savaşı

### FAZ 3 — PvP ve Savaş (oyunun kalbi)
9. **07_HARITALAR.md** — Moradon (lonca), Ronark Land (PvP), Delos (kale), Eslant, Bifrost, Felankor
10. **08_PVP_VE_NP.md** — NP kazanma/kaybetme, kill streak, ks, war zone kuralları
11. **09_KALE_SAVASI.md** — Delos castle siege, Crusade Wars (CSW), Lunar War
12. **10_BOSS_VE_DROP.md** — Felankor, Krowaz, Bifrost, world boss listesi, drop tabloları

### FAZ 4 — İleri Seviye (1098 spesifik)
13. **11_QUEST_SISTEMI.md** — quest tipleri, daily, repeat, class change quest, level-up quest
14. **12_EKONOMI.md** — power-up store, NPC alış-satış, exchange (anvil), gold farming
15. **13_PATCH_FARKLAR.md** — 1098 vs diğer patch'ler — bizim oyuna özel ne var (bizim oyun: 2369 base + 1098 giydirme)
16. **14_KISALTMALAR_TERIMLER.md** — KO sözlüğü (oc, ks, np, gb, mb, lr, st, vb.)

### FAZ 5 — Topluluk ve Meta
17. **15_PRIVATE_SERVER_SAHNESI.md** — private server kültürü, balance farklılıkları
18. **16_SUNUCU_KULTURU.md** — popüler private serverlar (USKO, Apex, Steam, CN), oyuncu davranışı

---

## KAYNAK STRATEJİSİ

**Birinci kaynak (resmi/güvenilir):**
- `knightonline.com` veya yapımcı sitesi (1098 dökümanları varsa)
- KO Wiki (knightonline.fandom.com)
- Steam KO wiki

**İkinci kaynak (topluluk):**
- KO subreddit (r/knightonline)
- Eski forum arşivleri (mpgh, koforums)
- YouTube — özellikle 1098 patch oynanış videoları
- Türk topluluk forumları (uskoforumu vb.)

**Üçüncü kaynak (private server):**
- Apex KO, USKO eski wikileri
- Bizim oyunla aynı patch'i kullanan örnek sunucular

---

## ÇALIŞMA YÖNTEMİ

1. **Her MD için:** önce 2-3 kaynaktan oku, çelişen bilgi varsa not düş
2. **1098 spesifik bölüm:** her MD'nin sonunda "1098 patch'te bu nasıl?" diye ek notu olacak
3. **Türkçe yaz**, teknik terimleri orijinal bırak (skill adları, item adları)
4. **Halüsinasyon yok:** emin olmadığın yere `[KAYNAK YOK — TEYIT GEREKLI]` yaz
5. **Resim/diagram yerine** metin tablo kullan (MD'de iyi okunur)

---

## ÇIKTI

`C:\temp\MYKO\docs\bilgi\MYKO\` altına 17+ MD:
- README.md (giriş)
- 00 → 16 ana konu MD'leri
- _OGRENME_PLANI.md (bu dosya)

Her MD: 200-500 satır, başlıklı, sade.

---

## HIZ

**Acele yok.** Bir MD bitince patron'a göstereceğim, onay alınca diğerine geçeceğim. Yanlış bilgi tespit ederse düzelteceğim.
