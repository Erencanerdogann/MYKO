# Web Forum — Flarum

## Yer + Kurulum

- **Path (Production):** C:\koweb2\forum\ (104.238.23.99)
- **Nginx:** http://104.238.23.99:8091/forum/ → `/forum` alias
- **Domain:** https://malaysiako.com/forum/
- **DB:** MariaDB localhost:3307 (flarum_db database)
- **Binary:** Flarum v1 (PHP Framework 6+)

---

## Durum Özet

| Metrik | Değer |
|--------|-------|
| **Eklenti sayısı** | 0 (sıfır) — vanilla Flarum |
| **SSO entegrasyonu** | YOK — KO site ile entegre değil |
| **Tema** | Flarum default |
| **Forum başlığı** | "MalaysiaKO Community" (varsayılan) |
| **Erişim** | HTTP 200, JSON API çalışıyor |
| **Üye sayısı** | TBD (test ortamı olabilir) |

---

## Flarum Mimarı

**Teknoloji:**
- **Framework:** Flarum (Laravel-based PHP)
- **Frontend:** Vue.js (SPA)
- **API:** REST + JSON
- **DB:** MariaDB (MySQL-compatible)

**Klasör yapısı:**
```
forum/
├── public/                # Web root
├── storage/              # Logs, cache
├── extensions/           # Add-on yer
├── vendor/               # Composer packages
└── config.php            # Ayarlar
```

---

## DB Schema (Kapsam)

**Flarum default tablolar:**
- `users` (kayıtlı üyeler)
- `discussions` (başlıklar)
- `posts` (yanıtlar)
- `tags` (kategori/etiket)
- `groups` (member, moderator, admin)
- `notifications` (alert)

**KO TB_USER ile bağ:**
- **VAR DEĞIL** — SSO entegrasyonu yapılmadı
- Flarum users ↔ KO TB_USER bağlantısı manual

---

## Erişim Yetkileri

| Rol | Flarum Group | İzin |
|-----|--------------|------|
| Misafir (Guest) | guests | View discussions + posts |
| Üye (Member) | members | Create discussion, post reply |
| Moderatör | moderators | Edit, delete posts + discussions |
| Admin | admins | Full control |

---

## Söz Konusu Diskusyon Kategorileri

**Türkçe forum bölümleri** (expected, TBD):
- Harita / Quest
- PvP / Clan
- Haber / Açıdünyalar
- Teknik Destek
- Off-Topic

**Durum:** Test ortamında boş olabilir.

---

## Lansmadan Önce Alınması Gereken Kararlar

### 1. SSO Entegrasyonu
- **Karar 1A:** Flarum → KO web login-ile bağla
  - Forum register atla, KO login'i kullan
  - JWT token shared
  - Cost: 1-2 Flarum eklenti yazma / test

- **Karar 1B:** Ayrı login sistemi
  - KO login ≠ Forum login
  - SSO yapma
  - Cost: Düşük, ama kullanıcı confusion

**Recommendation:** 1A (SSO), lansmada lazım

---

### 2. Eklenti İhtiyacı
- **Rich text editor:** Flarum default Markdown, BBCode mi lazım? → Check oyuncu talebesi
- **Captcha:** Spam botları için reCAPTCHA eklentisi?
- **Emoji reactions:** Post'a emoji tepki?
- **Email notifications:** Flarum → SMTP integration
- **Moderation tools:** Post silme, user suspend

**Recommendation:** Vanilla Flarum başlasın, pain points bulundukça eklenti ekle

---

### 3. Moderasyon
- **Kim mod olacak?** (şu an REDACTED)
- **Kurallar:** KO sunucu rules'a ek forum-specific kurallar?
- **Strike system:** Flarum'a KO strike sistemi entegre mi? (separate tracking?)

**Recommendation:** İlk haftada mod team belirle, kurallar yazıl

---

## URL Yapısı

```
https://malaysiako.com/forum/

Home: /forum/
Discussion list: /forum/d/<slug>-<id>
Single post: /forum/p/<post_id>
User profile: /forum/u/<username>
Search: /forum/search
API: /forum/api/discussions, /forum/api/posts
```

---

## Admin Panel

- **Giriş:** /forum/admin (Flarum dashboard)
- **Settings:** Forum title, description, logo, color
- **Extensions:** Eklenti install/enable
- **Users:** Manage members, roles, suspend
- **Posts:** Moderate discussions + replies
- **Permissions:** Define grup izinleri

---

## SMTP Yapılandırması

Email gönderme (notification + verification):
- **Provider:** TBD (Gmail SMTP, SendGrid vb.)
- **Config file:** `config.php` MAIL_ constants
- **Test:** Forum ayarlarından test mail gönder

**Durum:** TBD, setup bekliyor

---

## Entegrasyon Noktaları (WEB ↔ FORUM)

### Current (Separated)
```
Web (104.238.23.99:8091)          Forum (104.238.23.99:8091/forum/)
├─ KO login (Rust API)             ├─ Flarum login (internal)
├─ User profile                    └─ Flarum profile
└─ (no link)
```

### Future (SSO)
```
Web (KO login)
  │ JWT token
  ├─→ Forum (auto-login via token)
  └─→ Game Server (same token)
```

---

## Veri Taşıma (Migration)

Eğer başka bir forum'dan migrate gerekirse:
- **Flarum migration tools:** Community tarafından yazılmış (Discourse, phpBB importers)
- **Manual SQL:** discussions, posts, users CSV import
- **Current state:** Yeni forum başlatıyor (migration yok)

---

## Performans + Skalabilite

- **Cached:** Flarum'da cache-busting (JS/CSS versioning)
- **DB optimization:** forum DB'yi ayrı MariaDB port (3307) tutmak iyi
- **Load:** Vanilla Flarum 1000 concurrent users rahat handle eder
- **Backup:** MariaDB backup = forum veri backup

---

## Sorunlar Tespit Edilen (S84 Audit)

### 1. Flarum Status
- **HTTP 200** ✅ Flarum yükleniyor
- **JSON API** ✅ Erişilebilir
- **Frontend render** ✅ Sayfa normal

### 2. No Custom Extensions
- Flarum vanilla (eklentisiz) çalışıyor → minimal risk

### 3. No Data
- Forum discussions boş (test ortamı olabilir)
- Predefined discussions TBD

---

## Devir Notları (WEB+FORUM S87)

1. **SSO kararı:** S87'de DOKTOR'la karar al
   - Forum standalone kalır (1B) → SSO yapma
   - KO'ya entegre (1A) → Flarum eklentisi yaz

2. **Admin account:** Forum'da superadmin setup check et
   - Default email: ?
   - Default password: REDACTED'de

3. **Customization:** Logo, renk, başlık KO branding'e göre
   - Logo upload → /forum/admin/appearance
   - Color theme CSS override

4. **Backup:** MariaDB 3307 günlük backup pipeline kurulu mu?

5. **SMTP:** Email sending test et
   - Test mail → DOKTOR inbox
   - Spam filter check

---

## Test Checklist

- [ ] /forum/ erişilebilir
- [ ] Admin panel /forum/admin açılıyor
- [ ] New discussion + post yaratılabiliyor
- [ ] Search çalışıyor
- [ ] User profile düzenleme OK
- [ ] Email send test (SMTP)
- [ ] Mobile responsive OK
- [ ] API endpoint /forum/api/discussions JSON döndürüyor

---

## Dosya Kaynağı

- **Flarum kurulum:** C:\koweb2\forum\ (production)
- **DB:** MariaDB localhost:3307 flarum_db
- **Nginx config:** /etc/nginx/sites-available/malaysiako (TBD, production sunucuda)

---

## Referanslar

- Flarum official: https://flarum.org/
- Flarum API: https://flarum.org/docs/extend/api/
- Extensions marketplace: https://extiverse.com/
- Community: https://discuss.flarum.org/

---

**Yazı:** WEBRA | **Tarih:** 2026-04-29 | **Kategori:** FORUM
