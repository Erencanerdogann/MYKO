# Web Bug Listesi — Lansmaya (08 Mayıs) Hazırlık

---

## 🔴 KRİTİK (LANSMAYA ŞART)

### 1. strWebHash NULL Bug

**Sorun:**
- Rust API `/api/site/register` POST sırasında **strWebHash kolon set edilmiyor**
- INSERT query: `VALUES (@P1, @P2, @P3, @P4, @P4, '0', @P5, @P6)` (7 param)
- strWebHash sütunu **atlanıyor** (8. param eksik)
- Otomatik register hesaplar login bozuluyor

**Kanıt:**
- Memory: `project_web_login_hash_bug.md` (S84'te)
- Kod: `/c/temp/MYKO/orkestra-rs/orkestra-a2a/src/server.rs:4964` (INSERT komutu)

**Çözüm:**
```rust
// Satır 4964 — INSERT statement güncelle
let insert = conn.execute(
    "INSERT INTO TB_USER \
     (strAccountID, strPasswd, strPasswdHash, Email, koweb_email, \
      koweb_email_auth, koweb_email_random_code, strAuthority, strWebHash) \
     VALUES (@P1, @P2, @P3, @P4, @P4, '0', @P5, @P6, @P7)",
    &[
        &req.kullanici.as_str(),
        &sifre_plain.as_str(),
        &sifre_hash.as_str(),
        &req.email.as_str(),
        &verify_token.as_str(),
        &yetki,
        &strWebHash_value,  // Yeni param
    ],
).await;
```

**strWebHash değeri:** Hangi algorithm? (hash şifre + yaş? salt+hash?)
- **Geri araştırma:** CHIP veya MATRIX'tan sor (DB schema bilmek)
- **Suggestion:** MD5(sifre) veya SHA256(sifre + salt) simple başlasın

**Timeline:** Lansmadan 1 hafta önce (01 Mayıs deadline)

**Puan:** +15 (kritik fix)

---

### 2. TLS/HTTPS — MITM Açığı

**Sorun:**
- Web sitesi **HTTP-only** (port 8091)
- Login/register payload **plaintext** yollanıyor
- MITM saldırısında: credentials, token capture edilebilir

**Kanıt:**
- Audit S84: "TLS yok" notu
- Production IP: 104.238.23.99 (public)

**Çözüm:**
```
1. LetsEncrypt SSL sertifikası al
2. Nginx:
   server {
       listen 443 ssl;
       ssl_certificate /etc/letsencrypt/live/malaysiako.com/fullchain.pem;
       ssl_certificate_key /etc/letsencrypt/live/malaysiako.com/privkey.pem;
       ssl_protocols TLSv1.2 TLSv1.3;
   }
   
3. HTTP → HTTPS redirect:
   server {
       listen 80;
       return 301 https://$host$request_uri;
   }
```

**Database impact:** Yok, network-only

**Timeline:** Lansmaya yetişir mi? (Mayıs 8)
- SSL sertifikası: 1 saat (instantaneous)
- Nginx restart: 5 min
- **Cevap:** DOKTOR'a sor, ACIL checklist'e koyyy

**Puan:** +10 (security hardening)

---

## 🟡 YÜKSEK ÖNCELİK (Lansmadan 1 Hafta)

### 3. S84 WEBRA Audit — 8 Sorun

**Referans:** `/c/temp/MYKO/kaynak/MD_RAPORLAR/WEBRA_S84_SITE_AUDIT.md`

#### 3.1 Downloads Butonu Boş
- **Sorun:** href="#" → indirme linki bozuk
- **Sayfası:** /downloads
- **Çözüm:** href="/downloads/latest" veya Patch Server endpoint'i
- **Status:** Patch Server OFFLINE (kırmızı) — KODCU'nun alanı?
- **Not:** Dokunma, DOKTOR + KODCU'ya iletme

#### 3.2 Dil Geçişi İşlevsiz
- **Sorun:** /english, /turkish, /spanish cookies set ediyor ama sayfa içeriği değişmiyor
- **Sayfaları:** 3 endpoint
- **Root cause:** PHP `$_SESSION['lang']` set ediliyor ama page render'da kullanılmıyor
- **Çözüm:**
  ```php
  // /english endpoint
  $_SESSION['lang'] = 'en';
  $_COOKIE['koweb_lang'] = 'en';
  // Sonra page re-render
  ```
- **Timeline:** 1 gün

#### 3.3 Vote System Linki Boş
- **Sorun:** Footer'da `<a href="">Vote System</a>` — href empty
- **Çözüm:** URL tanımla veya link disable et (sayfası yok ise)
- **Timeline:** 1 saat

#### 3.4 "Coming soon" Marquee
- **Sorun:** `<marquee>MalaysiaKO is coming soon!</marquee>` hâlâ aktif
- **Sayfaları:** Tüm sayfalar
- **Çözüm:** HTML'den marquee siliniz
- **Timeline:** 1 saat

---

### 4. Plain Text Backup (strPasswd)

**Sorun:**
- Rust API register'da **strPasswd = plain 28-char şifre** DB'ye yazıyor
- Tasarım hata mı, backup mu? (Unknown)
- **Güvenlik:** DB dump'da plaintext şifreler

**Çözüm:**
- **Option 1:** strPasswd = NULL set (sadece strPasswdHash kullan)
- **Option 2:** strPasswd DELETE (DB migration)

**Timeline:** Lansmaya kadar

**Sorumlu:** MATRIX (DB schema) + CHIP (decision)

---

## 🟢 ORTA (Lansmadan Sonra Tamam)

### 5. Email Verify Token Timeout

**Sorun:**
- Email doğrulama linki `/api/site/verify?user=X&code=Y` hiç expire olmuyor mu?
- Sınırsız mi geçerli?

**Impact:** Düşük (email hijack için setup gerekir)

**Check:** Rust kodu incele
- `/c/temp/MYKO/orkestra-rs/orkestra-a2a/src/server.rs:5243` (api_site_verify)
- TTL hardcoded mi? variable mı?

**Recommendation:** 24 saate set et (standard)

---

### 6. Reset Password Endpoint

**Sorun:**
- POST `/api/site/reset-password` implement mi?
- Token validation nasıl?
- Token lifetime?

**Check:** Rust kodu incele (`forgot_password` + `reset_password` handler)

**Timeline:** Testten sonra

---

### 7. Session Token Expiry

**Sorun:**
- Login token (cookie) ne zaman expire oluyor?
- 1 gün? 1 hafta? infinite?

**Check:** `/api/site/login` response token'i ve PHP cookie lifetime

**Recommendation:** 7 gün (standard)

---

### 8. Forum SSO Entegrasyonu

**Sorun:**
- Flarum ≠ KO login
- Ayrı account gerekli forum'da

**Karar:** Lansmada DOKTOR ile

---

## 🔵 DÜŞÜK (Post-launch)

### 9. GM adı Typo — "amdin"

**Sorun:** Homepage GM list'de "amdin" yazıyor (admin mı?)
- **Sayfası:** / (sidebar)
- **Çözüm:** Admin panel'de "Admin" olarak düzelt

---

### 10. User Control Panel (/UserCP)

**Sorun:** Login olmadan `/UserCP` erişim → META REFRESH ile `/` yönlendiriyor
- Normal mi? Beklenen davranış mı?
- Test: Account panel functionality (şifre change, email update vb.)

---

### 11. Disclaimer Footer Eksik

**Sorun:** Footer'a disclaimer/privacy policy linki / copyright statement
- **Dosyalar:** footer_raw.php + es.php lang key (S86 CHIP commit'inde var mı?)
- **TODO 106:** WEB brief'te var

---

### 12. Doğum Tarihi Alanı Eksik

**Sorun:** Register form'da age/birthdate field yok ama DB'de TB_USER.BirtDate column var mı?
- **TODO 105:** Form'a field ekle (opsiyonel?)

---

## Lansmaya Gitmeden Kontrol Listesi

| # | Bugüm | Sorumlu | Durum | Deadline |
|---|-------|---------|-------|----------|
| 1 | strWebHash NULL | CHIP | TBD | 01 May |
| 2 | TLS/HTTPS | DevOps | TBD | 08 May? |
| 3.1 | Downloads href | KODCU | TBD | 05 May |
| 3.2 | Dil geçişi | WEBRA | TBD | 05 May |
| 3.3 | Vote System link | WEBRA | TBD | 02 May |
| 3.4 | Coming soon marquee | WEBRA | TBD | 02 May |
| 4 | strPasswd plain | MATRIX | TBD | 05 May |
| 5 | Email token TTL | CHIP | TBD | 05 May |
| 6 | Reset password | CHIP | TBD | 03 May |
| 7 | Session token TTL | CHIP | TBD | 03 May |
| 8 | Forum SSO | WEBRA | KARAR | 05 May |
| 9 | GM typo | WEBRA | TBD | 06 May |

---

## Kaynaklar + Memory Links

- Audit raporu: `WEBRA_S84_SITE_AUDIT.md`
- Login hash bug: `project_web_login_hash_bug.md` (S84 memory)
- TLS planning: `project_mitm_acigi.md` (S84 memory)
- Rust API kod: `/c/temp/MYKO/orkestra-rs/orkestra-a2a/src/server.rs`
- Web PHP kod: `F:\MYKOBACKUP\koweb\_application\*.php`

---

## Sonuç

**Lansmaya ŞART:** 2 bug (strWebHash + TLS)
**Lansmadan 1 hafta:** 3.1-3.4, 4-7 bugs (audit sorunları)
**Post-launch OK:** 9-12 bugs (minor issues)

---

**Yazı:** WEBRA | **Tarih:** 2026-04-29 | **Kategori:** WEB-BUG
