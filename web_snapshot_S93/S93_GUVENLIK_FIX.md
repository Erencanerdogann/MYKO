# S93 Güvenlik Fix Raporu
**Tarih:** 2026-05-02 | **Agent:** DOKTOR

---

## Yapılan Fixler

### 1. Hardcode DB Şifresi Kaldırıldı

**Etkilenen dosyalar:**
- `C:\koweb2\api\destek-form.php`
- `C:\koweb2\api\destek-detay.php`
- `C:\koweb2\api\destek-listem.php`
- `C:\koweb2\api\serve-attachment.php`

**Sorun:** 4 API dosyasında `odbc_pconnect('KO_MYKO', 'web_admin', 'Test1234567890!@#')` düz metin hardcode şifre vardı.

**Fix:** Her dosyaya `require_once dirname(__DIR__) . '/system/config.inc.php';` eklendi.
Bağlantı `$db['db_name']`, `$db['db_user']`, `$db['db_pass']` ile config'den okunuyor.

```php
// ÖNCE
$conn = odbc_pconnect('KO_MYKO', 'web_admin', 'Test1234567890!@#');

// SONRA
require_once dirname(__DIR__) . '/system/config.inc.php';
$conn = odbc_pconnect($db['db_name'], $db['db_user'], $db['db_pass']);
```

---

### 2. .bak Dosyaları Silindi

Web'den erişilebilir kaynak kodu içeren backup dosyaları kaldırıldı:

| Dosya | Durum |
|-------|-------|
| `Pages\Destek.php.bak` | SİLİNDİ |
| `Pages\ForgotPassword.php.bak_s70` | SİLİNDİ |
| `Pages\Register-2.php.bak_s70proxy` | SİLİNDİ |
| `Pages\Register-2.php.bak_s70sifre2` | SİLİNDİ |
| `Pages\Register.php.bak_oncesi_geri` | SİLİNDİ |
| `Pages\Register.php.bak_s70sifre2` | SİLİNDİ |

---

## Kalan Riskler (Fixlenmedi)

| # | Bulgu | Risk | Not |
|---|-------|------|-----|
| 1 | `UserCP.php` string concat doquery | ORTA | prepared statement'a geçiş gerekiyor |
| 2 | Destek API CSRF eksik | ORTA | token kontrolü eklenecek |
| 3 | `TB_USER.strPasswd` düz metin | ORTA | oyun sync zorunluluğu, değiştirilemez |
| 4 | `template.lib.php` pages `/` filtresi | DÜŞÜK | file_exists kısmen koruyor |
| 5 | Session cookie flag'leri | DÜŞÜK | httponly/secure/samesite ayarlanmamış |
| 6 | Die + exception mesajı exposure | DÜŞÜK | prod'da hata gizlenmeli |

---

## Tarama Kapsamı

Taranan dosyalar:
- `index.php`, `template.lib.php`, `config.inc.php`
- `Register.php`, `Register-2.php`
- `UserCP.php`, `ChangePW.php`, `Login.php`, `ForgotPassword.php`
- `api/destek-form.php`, `api/destek-detay.php`, `api/destek-mesaj.php`, `api/destek-listem.php`, `api/serve-attachment.php`
- `_library/functions.php`, `_library/security.php`
