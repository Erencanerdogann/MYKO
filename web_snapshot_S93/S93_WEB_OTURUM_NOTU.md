# S93 Web Oturum Notu — MalaysiaKO Site Çalışmaları
**Tarih:** 2026-05-02 | **Agent:** DOKTOR | **Patron:** Erencan

---

## Yapılan İşler

### 1. MATRIX — DB İngilizceye Çeviri (MAT-33)
- `_MALAYSIAKO_Notice`, `_MALAYSIAKO_Duyuru`, `SERVER_RESOURCE` tablolarındaki 109 satır TR→EN çevrildi
- MATRIX yedek aldı, güncelledi, doğruladı
- **Durum:** TAMAMLANDI

### 2. censor_words.txt Oluşturuldu ve Deploy Edildi
- `C:\temp\MYKO\src\GameServer_SRC\GameServer\censor_words.txt` oluşturuldu
- İçerik: TR küfür listesi + leet speak TR + İngilizce + leet speak EN (~200 kelime)
- Sunucuya deploy: `C:\Users\Administrator\Desktop\Server\censor_words.txt`
- Boyut: 196B → 1512B
- GM komutu: `+censorreload` (GameServerDlg.cpp'de var)
- **Durum:** TAMAMLANDI

### 3. Dijital Oyun Yasası Uyum Listesi
- `C:\temp\MYKO\dijital_oyun_yasasi_uyum_brief\dijital_oyun_yasasi_uyum_brief.md` okundu
- Web sitesi ile ilgili yapılacaklar belirlendi:
  - [ ] Kayıt yaş kontrolü (15+) — doğum tarihi alanı + kontrol
  - [ ] Ebeveyn bilgilendirme sayfası (zaten `EbeveynKontrol.php` var, detaylandırılacak)
  - [ ] WhatsApp footer butonu
  - [ ] Flarum küfür filtresi plugin (detaylandırılacak)
- **Durum:** Liste hazır, implementasyon bekliyor

### 4. header.php — addEventListener Null Fix
- `header.php:60-65` — `koLoginForm` elementi sadece login sayfasında var
- Diğer sayfalarda `null.addEventListener` → crash veriyordu
- Fix: `if (_kof)` null kontrolü eklendi
- Deploy edildi: `C:\koweb2\theme\fusion\header.php`
- **Durum:** TAMAMLANDI

### 5. Register.php — Eski Güzel Versiyon Geri Getirildi
- `Register.php.bak_s70sifre2` (530 satır) aktif Register.php olarak geri yüklendi
- Bu versiyon içerir:
  - MalaysiaKO Kullanıcı Sözleşmesi & Gizlilik Politikası (scroll'lu kutu)
  - Sözleşme onay checkbox'ı
  - AJAX kullanıcı adı kontrol (checkuser.php)
  - Şifre güç göstergesi (6 kural: uzunluk, büyük/küçük harf, rakam, ardışık, tekrar)
  - E-posta domain validasyonu (geçici mail engelleme)
  - Telefon doğrulama (20+ ülke, TR operatör prefix kontrolü)
  - `_MALAYSIAKO_Notice` doğru tablo adı
  - venagame metni YOK
- **Durum:** TAMAMLANDI, canlıda

### 6. MalaysiaKO.css — Eksik CSS Sınıfları Eklendi
- `.popup-block`, `.reg-form`, `.input-re`, `.email1`, `.secret1`, `.ssn1` eklendi
- Deploy: `C:\koweb2\theme\fusion\Css\MalaysiaKO.css`
- **Durum:** TAMAMLANDI

---

## Hatalar ve Öğrenilen Dersler

### HATA-1: WEBRA yanlış klasöre deploy etti
- **Sebep:** WEBRA `C:\koweb\` yerine `C:\koweb2\`'yi hedef alması gerekirken karıştırdı
- **Sonuç:** Canlı site etkilenmedi, ama vakit kaybı
- **Çözüm:** SSH ile doğru yol teyit edildi, WEBRA kapatıldı

### HATA-2: WEBRA Register.php CSS'i kırdı
- **Sebep:** WEBRA görev kapsamı dışında Register sayfasını modifiye etti
- **Sonuç:** CSS bozuk, sayfa çalışmıyordu
- **Ceza:** WEBRA -50 puan, kapatıldı. Web işleri DOKTOR üstlendi.

### HATA-3: top.php birden fazla kez yanlış geri döndürüldü
- **Sebep:** Server'da birden fazla .bak dosyası vardı, hangisinin doğru olduğu karışıktı
- **Çözüm:** `git show 683c36c` ile temiz versiyon alındı

### HATA-4: Register.php SQL hatası "Sorguda hata olustu"
- **Sebep:** Git'teki Register.php `_GNYSOFT_mykol` tablo adını kullanıyordu, DB'de tablo `_MALAYSIAKO_mykol`
- **Gerçek çalışan versiyon (bak_s70sifre2) hiç git'e commit edilmemiş** — KURAL 0 ihlali (WEBRA)
- **Çözüm:** Server'daki `Register.php.bak_s70sifre2` geri yüklendi

### HATA-5: CSS'e `%%` escape problemi
- **Sebep:** Windows cmd `echo` ile CSS append edildi, `%` karakteri `%%` olarak escape edilmeli
- **Sonuç:** `.popup-block { display:block; width:100%%; }` — çift %% yazıldı
- **Çözüm:** CSS tüm dosya scp ile çekildi, bash heredoc ile düzgün eklendi, tekrar deploy edildi

---

## Mevcut DB Tablo Adları (Doğru)
```sql
_MALAYSIAKO_Notice       -- site duyuruları
_MALAYSIAKO_Duyuru       -- oyun içi duyurular
_MALAYSIAKO_mykol        -- kayıt bilgileri (mail, telefon, seal, otp...)
SERVER_RESOURCE          -- sunucu kaynakları (İngilizce yapıldı)
```

---

## Dosya Yolları (Sunucu)
```
C:\koweb2\                               -- site root
C:\koweb2\theme\fusion\Pages\            -- sayfa PHP'leri
C:\koweb2\theme\fusion\Css\MalaysiaKO.css -- ana CSS
C:\koweb2\theme\fusion\header.php        -- header (login formu)
C:\koweb2\theme\fusion\top.php           -- üst kısım
C:\Users\Administrator\Desktop\Server\censor_words.txt  -- küfür listesi
```

---

## Bekleyen İşler (S94+)
1. Register yaş kontrolü 15+ (doğum tarihi alanı eklenmeli)
2. EbeveynKontrol.php sayfası detaylandırılacak + WhatsApp butonu
3. Footer'a WhatsApp hattı butonu
4. Flarum küfür filtresi
5. Login hata mesajı popup'ının konumu/stili (kullanıcı AJAX yazdı, ufak tweaklar kalabilir)
