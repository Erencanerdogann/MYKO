# Sözleşme Scroll Zorunluluğu + İmza Kaydı
**Tarih:** 2026-05-02 | **Agent:** DOKTOR

---

## Yapılan İşlemler

### 1. DB — `_MK_sozlesme_log` Tablosu Oluşturuldu

```sql
CREATE TABLE _MK_sozlesme_log (
    id       INT IDENTITY(1,1) PRIMARY KEY,
    username NVARCHAR(21) NOT NULL,
    ip       NVARCHAR(45) NOT NULL,
    tarih    DATETIME DEFAULT GETDATE(),
    versiyon NVARCHAR(10) NOT NULL DEFAULT '1.0'
);
```

**Amaç:** Her kayıt olan kullanıcının sözleşmeyi hangi tarihte, hangi IP'den, hangi versiyon için kabul ettiğini saklar. İleride sözleşme güncellenirse versiyon '2.0' yapılır, eski kayıtlar korunur.

---

### 2. Register.php — Scroll Zorunluluğu

**Dosya:** `C:\koweb2\theme\fusion\Pages\Register.php`

**Değişiklikler:**

- Sözleşme kutusuna `id="sozlesmeKutu"` eklendi
- Checkbox başlangıçta `disabled` — kullanıcı sözleşmeyi okumadan tik atamaz
- Kırmızı uyarı mesajı: *"Sözleşmeyi okumak için lütfen aşağıya kaydırın."*
- JS scroll event: kutu sonuna (<20px kala) scroll edilince checkbox aktif olur, uyarı kaybolur

```javascript
var skutu = document.getElementById('sozlesmeKutu');
var sOnay = document.getElementById('sozlesmeOnay');
skutu.addEventListener('scroll', function(){
    var kalan = skutu.scrollHeight - skutu.scrollTop - skutu.clientHeight;
    if(kalan < 20) {
        sOnay.disabled = false;
        // uyarı mesajı gizlenir
    }
});
```

---

### 3. Register-2.php — İmza Kaydı

**Dosya:** `C:\koweb2\theme\fusion\Pages\Register-2.php`

Kayıt başarılı olduktan sonra (ACCOUNT_CHAR INSERT'in hemen arkası):

```php
$ins5 = odbc_prepare($conn, "INSERT INTO _MK_sozlesme_log (username, ip, tarih, versiyon) VALUES (?, ?, ?, ?)");
if ($ins5) @odbc_execute($ins5, array($username, $ip, $regdate, '1.0'));
```

**Kaydedilen bilgiler:**
| Kolon | Değer |
|-------|-------|
| username | Kayıt olan kullanıcı adı |
| ip | Kayıt anındaki IP adresi |
| tarih | Kayıt tarihi (Y-m-d H:i:s) |
| versiyon | Sözleşme versiyonu ('1.0') |

---

## Sözleşme Versiyonu Güncellemesi

İleride sözleşme değişirse:
1. Register-2.php'deki `'1.0'` → `'2.0'` yap
2. DB'de sorgu: `SELECT * FROM _MK_sozlesme_log WHERE versiyon='1.0'` — eski imzalar görünür

---

## Test

1. Register sayfasını aç
2. Checkbox'ın disabled (gri/tıklanamaz) olduğunu kontrol et
3. Sözleşme kutusunu sonuna kadar kaydır → checkbox aktif olur
4. Kayıt tamamla → `SELECT * FROM _MK_sozlesme_log` ile imzayı doğrula
