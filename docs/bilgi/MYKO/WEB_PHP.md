# Web PHP — koweb2

## Yer

- **Production:** 104.238.23.99:8091 (PHP nginx)
- **Lokal yedek:** F:\MYKOBACKUP\koweb\ (en yeni sürüm, Nisan 2026)

---

## Mimarı

Web sitesi **MVC mimarisi** kullanıyor:
- **Controller:** `_application/*.php` (register.php, login.php vs.)
- **View:** `theme/fusion/pages/*.php` (HTML render)
- **Library:** `_library/` (business logic, DB işlemleri)
- **Config:** `system/config.php` (DB bağlantı, constants)

Erişim kontrolü: `if( !defined('FEAR') ) die()` pattern tüm sayfalarda.

---

## Sayfa Listesi

| Sayfa | Amaç | Koşul | HTTP |
|-------|------|-------|------|
| `/` | Anasayfa | Herkese açık | 200 |
| `/register` | Hesap kaydı | Form + OTP | 200 |
| `/login` | Giriş | Form POST | 200 |
| `/forgotpassword` | Şifre sıfırla | Email talep | 200 |
| `/online` | Online oyuncular | DB query CURRENTUSER | 200 |
| `/rankings` | Sıralama (çok tip) | USERDATA | 200 |
| `/server-status` | Sunucu durumu | TCP ping | 200 |
| `/news` | Haber arşivi | Statik içerik | 200 |
| `/rules` | Sunucu kuralları | Statik | 200 |
| `/downloads` | İndirme | Patch Server (OFFLINE) | 200 |
| `/usercp` | Hesap paneli | Auth gerekli, login redirect | 200 |
| `/quest` | Quest bilgisi | Statik | 200 |
| `/forum/` | Flarum | Nginx proxy | 200 |

**Tarama:** 23 sayfa test (S84 WEBRA_S84_SITE_AUDIT.md) — hepsi 200 döndü, hiç 404/500 yok.

---

## Register (Kayıt) Akışı

### HTML Form (`register.php`)
```
POST /register
├─ kullanici_adi (4-16 char)
├─ sifre (6-20 char)
├─ email (valid email)
├─ gizli_soru (security Q)
├─ telefon (optional)
├─ seal (CAPTCHA?)
└─ otp (OTP token?)
```

### Backend İşlemi
1. PHP `register.php` form validation (regex, CAPTCHA check)
2. POST → Rust API `/api/site/register` (localhost:3001)
3. Rust API:
   - Duplicate check (SELECT COUNT strAccountID, Email)
   - Şifre SHA256 hash (strPasswdHash)
   - Şifre plain text 28 char (strPasswd) — **GÜVENLİK SORUNU**
   - Email token üretim (verify_token)
   - INSERT TB_USER (strAccountID, strPasswd, strPasswdHash, Email, koweb_email, koweb_email_auth='0', koweb_email_random_code, strAuthority=1)
4. SMTP async: Doğrulama maili → Email adresine link `/api/site/verify?user=X&code=Y`
5. Response: `{ok, kullanici, msg: "Kayıt başarılı, email doğrulayın"}`

### Rate Limit
- 3 kayıt / dakika IP başına

**⚠️ BUG HATIRLATMA:** strWebHash kolon NULL bug (S84 memory) — auto-register hesapları site login bozuyor.

---

## Login (Giriş) Akışı

### HTML Form (`login.php`)
```
POST /login
├─ kullanici_adi
└─ sifre
```

### Backend İşlemi
1. PHP `login.php` form validation
2. POST → Rust API `/api/site/login` (localhost:3001)
3. Rust API:
   - SHA256 hash(sifre) hesapla
   - SELECT strAccountID, strAuthority, koweb_email_auth FROM TB_USER WHERE strAccountID = @user AND strPasswdHash = @hash
   - Email doğrulama kontrol: koweb_email_auth == '1' ise devam, değilse **403 FORBIDDEN**
   - Token üretim
   - Response: `{ok, kullanici, yetki (1/2/9), token}`
4. Token **cookie** olarak set (name: session_token?)
5. Redirect → /UserCP veya referrer

### Rate Limit
- 10 giriş denemesi / dakika IP başına

**Yetki Seviyeleri:**
- 1 = Normal kullanıcı
- 2 = GM (Game Master)
- 9 = Admin

---

## API Mimarisi (Rust — localhost:3001)

Web site'si Rust backend'i (orkestra-server.exe) ile konuşur. Endpoint'ler:

| Endpoint | Method | Rate | Param | Response | Not |
|----------|--------|------|-------|----------|-----|
| /api/site/register | POST | 3/dk | {kullanici, sifre, email} | {ok, kullanici, msg} | Email verify gerekli |
| /api/site/login | POST | 10/dk | {kullanici, sifre} | {ok, kullanici, yetki, token} | Email doğrulama şart |
| /api/site/verify | GET | - | user, code | {status, msg} | Email link |
| /api/site/online | GET | 100/dk | - | {count} | CURRENTUSER table |
| /api/site/rankings | GET | 100/dk | type, limit | {rankings: []} | USERDATA order |
| /api/site/server-status | GET | 100/dk | - | {gameserver, loginserver, durum} | TCP ping |
| /api/site/health | GET | 100/dk | - | {ok, mssql, versiyon} | DB health check |
| /api/site/forgot-password | POST | - | email | {ok, msg} | Email reset link |

**DB Bağlantısı:** Rust → Tiberius (MSSQL driver) → MSSQL Server localhost\MSSQLSERVER01:1433 KO_MYKO DB

---

## CORS + Güvenlik

### PHP Proxy Pattern
Web (port 8091) → API (port 3001) arasında cors-headers karşılaştırması:
```
Origin: http://104.238.23.99:8091
→ Rust API CORS allow
```

### Token Yönetimi
- Token **JWT mi, Session ID mi?** → Rust kodu JWT gibi davranıyor (base64 encoded random?)
- Cookie name: Muhtemelen `session_token` veya `auth_token`
- Expiry: Tanımlı değil, check etme

### Şifreleme
- **SHA256** hash: Standard, güvenli
- **Plain text backup:** strPasswd (28 char) — **REDACTED** burada, gerçek değer DB'de

---

## Sorunlar (S84 Audit'den)

### 1. strWebHash NULL — 🔴 KRİTİK
Rusty API insert sırasında strWebHash sütununu ignore ediyor → otomatik kayıt hesapları login bozuluyor.
- **Çözüm:** Rust API INSERT'e strWebHash ekle (hash şifre yaş günü vs)
- **Status:** BEKLIYOR (WEB_BUG.md'de)

### 2. TLS Yok — 🟡 MITM Açığı
HTTP-only, SSL/TLS sarması yok. Payload replay riski.
- **Çözüm:** LetsEncrypt SSL + nginx HTTPS 443 yapısı
- **Timeline:** Lansmana yetişir mi? (Mayıs 8)

### 3. Plain Text Şifre Backup — 🟡 Tasarım
strPasswd column (plain 28 char) → DB dump riski
- **Çözüm:** Gerçek environment'de strPasswd NULL set, sadece strPasswdHash kullan
- **Timeline:** Uzun vade

### 4. Email Doğrulama Token Bekleme
API `/api/site/verify?user=X&code=Y` bekleme süresi tanımlı değil (24s, 7g vs?)
- **Not:** Audit'te detaylı test yok

---

## İstatistikler (S84)

- **Tarama:** 23 sayfa, hepsi 200 OK
- **Login Server:** Online (yeşil)
- **Game Server:** Online (yeşil)
- **Patch Server:** OFFLINE (kırmızı) — `/downloads` bozuk
- **Forum:** Flarum çalışıyor (malaysiako.com/forum)

---

## 🗂️ KLASÖR YAPISI DETAYI

`F:\MYKOBACKUP\koweb2/` extract sonrası klasör örgütlenmesi:

```
koweb2/
├── _application/                    ← Controller katmanı
│   ├── register.php                 ← Kayıt formu + işlemi
│   ├── login.php                    ← Giriş formu + işlemi
│   ├── forgotpassword.php           ← Şifre sıfırlama
│   ├── usercp.php                   ← Hesap paneli (auth)
│   ├── logout.php                   ← Çıkış
│   └── api_gateway.php              ← Rust API proxy (POST yönlendir)
├── _library/                        ← Business logic + DB layer
│   ├── database.php                 ← MSSQL bağlantı
│   ├── user.class.php               ← Kullanıcı işlemleri
│   ├── validation.class.php         ← Form doğrulama
│   ├── email.class.php              ← SMTP wrapper
│   └── security.class.php           ← Hash, token, salt
├── system/
│   ├── config.php                   ← DB credentials, API endpoint
│   ├── routes.php                   ← URL → Controller mapping
│   ├── autoload.php                 ← Sınıf auto-load
│   └── error.php                    ← Error handler
├── theme/fusion/
│   ├── layout.php                   ← Master template
│   ├── pages/                       ← HTML render dosyaları
│   │   ├── index.php
│   │   ├── register.php
│   │   ├── login.php
│   │   ├── online.php
│   │   ├── rankings.php
│   │   ├── server-status.php
│   │   ├── news.php
│   │   ├── downloads.php
│   │   └── usercp/
│   │       ├── profile.php
│   │       ├── password.php
│   │       └── logout.php
│   ├── assets/
│   │   ├── css/
│   │   │   ├── style.css
│   │   │   └── responsive.css
│   │   ├── js/
│   │   │   ├── app.js
│   │   │   └── validate.js
│   │   └── images/
├── vendor/                          ← Composer packages
├── public/
│   ├── index.php                    ← Entry point
│   └── .htaccess                    ← Apache rewrite
├── uploads/                         ← User uploads
│   ├── avatars/
│   └── files/
├── logs/                            ← Sistem logları
├── cache/                           ← Geçici dosyalar
└── composer.json
```

---

## 🔄 REGISTER AKIŞI — DETAYLI ADİM ADIM

### Adım 1: Form Yükleme (GET /register)
Browser → Nginx → PHP render → HTML form (CSRF token embed)

### Adım 2: Form Submit (POST /register)
```
Browser POST /register
↓
_application/register.php handler
↓
PHP validation: regex (username), email format, password length
↓
Call Rust API: POST http://localhost:3001/api/site/register
  Body: {username, email, password, secret_answer}
↓
Rust API:
  - Rate limit: 3/dk per IP → 429 if exceed
  - Duplicate check: SELECT COUNT(*) FROM TB_USER
  - SHA256(password) → strPasswdHash
  - Token üret: 32B random hex
  - INSERT TB_USER
  - SMTP async: email token gönder
  - Response 201: {ok, username, message}
↓
PHP: Set cookie + redirect /login
```

---

## 🔐 LOGIN AKIŞI — DETAYLI

### Adım 1: Login Form (GET /login)
Browser → Nginx → PHP → HTML form (username + password)

### Adım 2: Login Submit (POST /login)
```
Browser POST /login
↓
_application/login.php handler
↓
Call Rust API: POST http://localhost:3001/api/site/login
↓
Rust API:
  - Rate limit: 10/dk per IP
  - SHA256(password) hesapla
  - SELECT strAccountID, strAuthority, koweb_email_auth
    FROM TB_USER
    WHERE strAccountID = @user AND strPasswdHash = @hash
  - NOT FOUND: 401 UNAUTHORIZED
  - koweb_email_auth != '1': 403 FORBIDDEN (unverified)
  - Token üret
  - Response 200: {ok, username, authority, token}
↓
PHP: Set cookie (session_token, 30 gün)
↓
PHP: Redirect /usercp
```

---

## 🔗 CORS + PROXY MIMARISI

### Problem
- Frontend (Browser 8091) → Backend (Rust 3001) cross-origin
- Tarayıcı CORS policy nedeniyle **preflight OPTIONS** isteği gönderiyor

### Çözüm: Server-Side Proxy (PHP)
```
Browser → POST /api/register (8091)
↓
PHP: api_gateway.php middleware
↓
curl POST to http://localhost:3001/api/site/register
  (same-origin curl, CORS yok)
↓
Rust API response → PHP cevabı
↓
Browser: CORS bypass (same-origin response)
```

### Nginx Alternatifi
```
/api/* → proxy_pass http://localhost:3001
  (reverse proxy, tarayıcı fark etmez)
```

---

## 📄 SAYFA SAYFA HÜCREL

### index.php (Anasayfa — Public)
- **URL:** `/`
- **İçerik:** Sunucu durumu, online sayısı, son haberler, download butonları
- **Backend:** API calls: `/api/site/online`, `/api/site/server-status`
- **Cache:** Nginx 5 dakika

### register.php (Kayıt Formu)
- **URL:** `/register`
- **Form Fields:** username, email, password, secret_question, CSRF token
- **Backend:** Rust API POST /api/site/register
- **Errors:** 409 (duplicate), 400 (validation), 429 (rate limit)
- **Success:** Redirect /login

### login.php (Giriş Formu)
- **URL:** `/login`
- **Form Fields:** username, password
- **Backend:** Rust API POST /api/site/login
- **Cookie:** session_token set (30 gün)
- **Redirect:** /usercp

### online.php (Çevrim İçi Oyuncular)
- **URL:** `/online`
- **Query String:** ?zone=city&level=50 (filtre, optional)
- **İçerik:** Tablo (No, Username, Level, Zone, Klan)
- **Backend:** API GET /api/site/online
- **Refresh:** 30 saniye auto-refresh (JavaScript)

### rankings.php (Sıralamalar)
- **URL:** `/rankings`
- **Sekmeleri:** Top 20 Klanlar, Oyuncular, Zenginler, PvP
- **Backend:** API GET /api/site/rankings?type=X&limit=20
- **Cache:** 1 saat

### server-status.php (Sunucu Durumu)
- **URL:** `/server-status`
- **İçerik:** Game Server + Login Server durumu (Online/Offline), MSSQL health
- **Backend:** API GET /api/site/server-status (TCP ping)
- **Refresh:** 10 saniye

### downloads.php (İndirme)
- **URL:** `/downloads`
- **İçerik:** Client installer, manual patch, support email
- **Note:** Patch Server OFFLINE (S84 audit)

### usercp.php (Hesap Paneli — Auth Gerekli)
- **URL:** `/usercp`
- **Redirect:** Girişsiz ise /login
- **Alt Sayfalar:** /usercp/profile, /usercp/password, /usercp/logout
- **Backend:** Session token doğrulama

### forum/ (Flarum Forum)
- **URL:** `/forum/`
- **Teknoloji:** Flarum (PHP forum engine)
- **Proxy:** Nginx reverse proxy

---

## ⚙️ KONFİGÜRASYON DOSYALARI

### system/config.php
```php
define('DB_HOST', 'localhost\MSSQLSERVER01');
define('DB_NAME', 'KO_MYKO');
define('API_BASE', 'http://localhost:3001');
define('SMTP_HOST', 'mail.example.com');
define('SESSION_TIMEOUT', 30 * 24 * 60 * 60); // 30 gün
define('SITE_URL', 'http://104.238.23.99:8091');
define('DEBUG_MODE', false);
```

### nginx config
```nginx
server {
    listen 8091;
    root /var/www/koweb2/public;
    
    # /api/* → localhost:3001
    location /api/ {
        proxy_pass http://localhost:3001;
        proxy_set_header X-Forwarded-For $remote_addr;
    }
    
    # /forum/* → Flarum
    location /forum/ {
        proxy_pass http://localhost:8092;
    }
    
    # PHP FPM
    location ~ \.php$ {
        fastcgi_pass unix:/run/php/php-fpm.sock;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
    }
    
    # Statik: 7 gün cache
    location ~* \.(jpg|css|js|png|gif)$ {
        expires 7d;
    }
    
    # Pretty URLs
    location / {
        try_files $uri $uri/ /index.php?$query_string;
    }
}
```

---

## 🎨 TEMA FRAMEWORK (Fusion)

### CSS + JavaScript
- **Framework:** Bootstrap 4/5 (varsa) veya custom CSS
- **Responsive:** 320px (mobile) → 1200px+ (desktop)
- **jQuery:** (varsa) form handling, AJAX
- **Vanilla JS:** app.js (validation), event listeners

### Components
- **Navbar:** Logo, menu, login/logout button
- **Forms:** Validation feedback, error messages
- **Tables:** Online oyuncular, sıralamalar
- **Modals:** Confirm dialogs, alerts
- **Cards:** News, server status, user profile

---

## 📊 PERFORMANS

### Nginx Caching
- Static assets: 7 gün cache
- API responses: 30 saniye cache (online, rankings)
- HTML pages: No cache (dynamic)

### PHP OPcache
```
opcache.enable=1
opcache.memory_consumption=256
opcache.validate_timestamps=0 (production)
```

### DB Optimization
- Index: TB_USER (strAccountID, Email)
- Index: USERDATA (strAccountID, EXP, Nation_Contributions)

---

## Devir Notları (WEB+FORUM S87)

1. **Koweb kaynak:** F:\MYKOBACKUP\koweb/ kullan (en yeni, 27 Nisan)
2. **Şifre MD'ye yazma:** REDACTED format
3. **SSH gerekli değil:** Lokal rar yeterli
4. **DOKTOR onayla:** Production değişiklik öncesi

---

**Yazı:** WEBRA | **Tarih:** 2026-04-29 | **Kategori:** WEB | **Versiyon:** v2 (Zenginleştirilmiş)
