# 🎫 PLAYER SUPPORT — Oyuncu Destek Akışı

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** Web (`koweb2`), Forum (Flarum), Discord (planlı), GM komutları
**Hedef:** Oyuncu sorununu nasıl alırız, nasıl çözeriz, kim bakar.

---

## 1. SUPPORT KANALLARI (Lansman için)

### Aktif (lansman gününde hazır olmalı)
| # | Kanal | Sorumlu | Yanıt süresi |
|---|-------|---------|--------------|
| 1 | **Forum** (Flarum) | Moderatör + DOKTOR | 24h |
| 2 | **Discord** | Moderatör + GM | 1-4h |
| 3 | **In-game `/gm`** komut | Aktif GM | Anlık |
| 4 | **Site iletişim formu** | DOKTOR | 48h |

### Planlanan (post-launch)
- E-posta destek (`destek@malasiako.com`)
- Whatsapp/Telegram grup (kapalı)
- Ticket system (entegre forum)

---

## 2. OYUNCU SORUN TİPLERİ

### A) Hesap Sorunları
| Sorun | Çözüm | Sorumlu |
|-------|-------|---------|
| Şifre unuttum | Web → Forgot password (e-posta) | WEBRA |
| Hesabım çalındı | Manuel verify (kayıt e-posta + ödeme delili) | DOKTOR + WEBRA |
| Web'e giremiyorum (auto-register) | strWebHash NULL bug → manuel UPDATE | WEBRA |
| Karakter silindi | Backup restore (saatlik) | MATRIX |
| 2 hesap birleştir | YASAK (policy) | — |
| Hesap ban | Forum unban appeal | DOKTOR + GHOST |

### B) Item/Para Sorunları
| Sorun | Çözüm | Sorumlu |
|-------|-------|---------|
| Item kayboldu | LOG_USER_ACTION incele → manuel `+give` | MATRIX |
| Ticari sorun (trade fail) | LOG_TRADE incele → DB rollback | MATRIX |
| Anvil scroll uçtu | LOG_USER_ACTION → kanıt → `+give` | MATRIX |
| Cash bakiye eksik | LOG_PUS + ödeme webhook log → düzelt | WEBRA + MATRIX |
| Premium aktif olmadı | TB_USER + DB SP kontrol | WEBRA |
| Item dupe şüphesi | LOG_DUPE + ban | GHOST |

### C) Game Sorunları
| Sorun | Çözüm | Sorumlu |
|-------|-------|---------|
| Karakter askıda kaldı | `+bug <Nick>` (askı kurtar) | GM in-game |
| Quest bozuk | Lua quest debug | KODCU |
| Skill çalışmıyor | DB MAGIC_TABLE + reload | MATRIX |
| Map crash | Logs\GENERAL incele | CHIP |
| Connection timeout | Network / firewall | DOKTOR |
| FPS / lag | Client OPTION.INI → graphic settings | KODCU |

### D) Hile Şikayeti
| Sorun | Çözüm | Sorumlu |
|-------|-------|---------|
| "X kullanıcı hile yapıyor" | HACK_*.log incele → kanıt → ban | GHOST + GM |
| Speed hack | XGuard log + ban | GHOST |
| Wall cheat | (lansman önce fix) + ban | CHIP + GHOST |
| Bot tespit | Forum kanıt + GM in-game gözlem | GM |
| Açık account ban | Müracaat → kanıt incele → karar | DOKTOR |

---

## 3. SOP (Standard Operating Procedure)

### Tek Sorun Akışı
```
1. Oyuncu kanal üzerinden ulaşır (forum/discord/in-game)
   ↓
2. Moderatör triage:
   - Hangi tip? (yukarıdaki kategori)
   - Aciliyet? (Acil / Normal / Düşük)
   - Sorumlu agent?
   ↓
3. Moderatör → ilgili agent'a iletir (DOKTOR üzerinden)
   ↓
4. Agent inceleme:
   - Log incele (LOG_USER_ACTION, GM_*.log, HACK_*.log)
   - DB sorgu (USERDATA, USER_ITEM)
   - Kanıt yeterli mi?
   ↓
5. Karar:
   - Çözüm uygula (DB UPDATE, +give, +unblock, +bug)
   - Reddet (kanıt yok)
   - Eskaleyt (DOKTOR → Patron)
   ↓
6. Oyuncuya geri bildirim (forum reply / discord DM)
   ↓
7. Audit kayıt (DB LOG_SUPPORT veya md)
```

### Acil Durum (5dk içinde müdahale)
- Mass disconnect
- Server crash
- DDoS
- Toplu hile (event)
- Cash dupe
- DB corruption şüphe

→ **DOKTOR'a duyuru** (Discord ping) → ilgili agent acil katılır.

---

## 4. GM IN-GAME DAVRANIŞ KURALLARI

### YAPMA
- ❌ Oyuncuya ücretsiz item verme (sosyal — hediye ekonomi bozar)
- ❌ Favori karakteri kayır
- ❌ Yetkin dışında karar (DOKTOR onaysız ban kaldırma)
- ❌ Premium hesaba görünmez gözcülük (oyuncu güveni)
- ❌ Açıkça GM olarak chat (özel mesaj tercih)

### YAP
- ✅ `+gm` ile görünmez gözlem (hile takibi)
- ✅ `+bug <Nick>` askı kurtarma (sorun çözme)
- ✅ Spam ban (`+block` kısa süre)
- ✅ Resmi event açma (`+csw`, `+chaosopen`)
- ✅ Audit log'a düşeceğini bil — her komut izlenebilir
- ✅ `+changegm` öncesi DOKTOR onayı al

### Yetki Hiyerarşisi
| Rol | Yetki |
|-----|-------|
| **Patron** (Erencan) | Hepsi, kalıcı |
| **DOKTOR** (Opus PM) | Görev koordinasyon, plan, onay |
| **GM Senior** (kalıcı, Authority=0) | Tüm GM komutları, ban karar |
| **GM Junior** (anlık, `+gm` toggle) | Bakım, event açma |
| **Moderatör** (forum/discord) | Forum ban, çağrı yönlendirme |

---

## 5. AUDIT KAYIT (her destek)

### DB Tablo (önerilen)
```sql
CREATE TABLE LOG_SUPPORT (
   id INT IDENTITY PRIMARY KEY,
   ticket_no VARCHAR(20),
   oyuncu VARCHAR(50),
   sorun_tipi VARCHAR(50),
   detay TEXT,
   sorumlu_gm VARCHAR(50),
   karar VARCHAR(20),  -- 'COZULDU', 'RED', 'ESKALE'
   aksiyon TEXT,        -- '+give X 1 30' vs.
   log_time DATETIME DEFAULT GETDATE()
);
```

### Forum Threading
- Her ticket → 1 forum thread (kapalı kategori)
- Çözüm sonrası "RESOLVED" tag
- 30 gün sonra arşiv

---

## 6. ŞİKAYET REDDETME (Politika)

### Reddedilen Talepler
- "Item kaybım" → log yok, kanıt yok
- "Hesap geri ver" → orijinal kayıt e-postası eşleşmiyor
- "Banı kaldır" → tekrarlanan hile, son kararsız
- "Cash iade" → ödeme onayı yok / 30 gün geçti
- "GM ol" → kapalı kayıt, davet usulü

### Onay Şartları (örnek "item geri")
- ✅ LOG_USER_ACTION'da kanıt var
- ✅ Trade/dupe değil
- ✅ Son 7 gün içinde
- ✅ İlk kez başvuruyor (tekrarcı değil)

---

## 7. KÖTÜ NIYET TESPİTİ

### Davranış Kalıpları
| Davranış | Şüphe |
|----------|-------|
| Aynı IP'den 10+ hesap | Multi-account / botnet |
| Saat fark sürekli aynı | Bot |
| Yeni hesap → hemen yüksek level | Powerlevel hile |
| Toplu fısıltı (whisper) | Spam bot |
| Aynı hata mesajı tekrar | Exploit deneme |
| Cash → kısa zamanda iade | Fraud (kart geri çekme) |

### Önlem
- IP rate limit (web + game)
- HWID ban (`+pcblock`)
- Discord/forum ban listesi
- Manuel review (yüksek değer item satışı)

---

## 8. KRİZ SENARYOLARI

### A) Server 1 saat down
- Forum + Discord + sosyal medya duyuru
- "Bakım modu" başlığı
- Çözüm sonrası: 1 saat free EXP/Drop bonus duyuru

### B) Toplu Item Dupe
- `+down 5` → restart (kısa)
- DB rollback son saatlik backup'a
- HACK log analiz → ban
- Duyuru: "Hile tespit, log incelendi, dupe item silindi"

### C) Wave hile (event'te)
- Acil GM müdahale
- HACK log toplu analiz
- 24h bekleme + toplu ban
- Duyuru: "Hile temizliği yapıldı"

### D) DDoS
- CDN/firewall escalate
- Hostabil destek ara
- Sunucu IP değişikliği (gerekirse)
- Discord/forum üzerinden duyuru

### E) Cash Fraud
- Ödeme provider ile iletişim
- Hesap dondurma
- Ban + iade işlem

---

## 9. KOMUNIKASYON TEMPLATE'LERI

### A) "İtemim kayboldu" → Kanıt yok
> Selam <Nick>, başvurunu inceledik. LOG sisteminde belirttiğin saatte item kaybı tespit edemedik. Spesifik trade ID veya kanıt (ekran görüntüsü) varsa paylaşabilir misin? Aksi halde işlem kapatılacak.

### B) Item iade onaylandı
> <Nick>, başvurunu inceledik. Log'da X (Trade ID) işlemde sorun tespit ettik. Item'ı in-game posta ile gönderdik. Kontrol et lütfen.

### C) Ban uyarısı
> <Nick>, hesabın geçici olarak banlandı (sebep: şüpheli aktivite). 24 saat içinde forum üzerinden itiraz başvurusu yapabilirsin: [link]

### D) Sunucu bakım duyuru
> 🔧 **Sunucu Bakımı** | <gün> <saat> başlayıp ~30 dk sürecek. Bakım sonrası bonus EXP %50 (1 saat). Anlayışınız için teşekkürler.

---

## 10. LANSMAN GÜNÜ HAZIRLIK

- [ ] Forum kategoriler oluşturuldu mu? (Genel / Hata / Hesap / Hile)
- [ ] Forum moderatör seçildi mi?
- [ ] Discord sunucu kuruldu mu? Oda yapısı?
- [ ] Site iletişim formu çalışıyor mu?
- [ ] GM hesapları belirlendi mi (kim, hangi vardiyada)?
- [ ] FAQ sayfası hazır mı?
- [ ] Ticket template kayıt formu hazır mı?
- [ ] Ban/unban prosedürü dokumante mi?
- [ ] LOG_SUPPORT DB tablosu var mı?
- [ ] Cash iade prosedürü onaylandı mı?

---

## 11. DİKKAT NOKTALARI

| ⚠️ | Konu |
|----|------|
| 1 | **GM keyfi karar yok** — DB log + DOKTOR onayı |
| 2 | **Cash iadesi finansal — KVKK** dikkat |
| 3 | **Forum ban + discord ban + game ban** ayrı ayrı yapılır |
| 4 | **Tehdit/küfür içerikli ticket** doğrudan reddet, ban |
| 5 | **Çocuk hesabı şüphesi** → KVKK uyarınca veli onayı |
| 6 | **Toplu duyuru** → site + forum + discord + in-game birlikte |
| 7 | **Türkçe iletişim** zorunlu (proje hedefi Türkiye) |
| 8 | **Yedek vardiya** lansman gecesi şart (3 vardiya) |

---

## 12. KAYNAK REFERANSLAR

- **GM Komut:** `GM_KOMUT.md`
- **Web:** `WEB_PHP.md`, `WEB_API.md`
- **Forum:** `WEB_FORUM.md`
- **Cash:** `CASH_SHOP_PUS.md`
- **Log:** `LOG_MONITORING.md`
- **Backup:** `BACKUP_RESTORE.md`
- **Bug:** `WEB_BUG.md`, `GUVENLIK_BUG.md`

---

**Sürüm:** v1.0 — S88 ilk yazım
**Sonraki:** Lansman 1 hafta deneyimi sonrası gerçek ticket akışlarına göre güncellenecek. Discord sunucu kurulumu sonrası rol/yetki MD'si ek.
