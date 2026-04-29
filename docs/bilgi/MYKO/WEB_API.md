# Site API — Rust orkestra-server.exe (:3001)

## Yer + Başlatma

- **Port:** Localhost 3001 (production de 104.238.23.99:3001)
- **Binary:** `C:\temp\MYKO\orkestra-rs\target\release\orkestra-server.exe`
- **Feature:** `--features=site-api` ile derlenmiş
- **DB:** orkestra.db (Rust SQLite) + MSSQL pool (site-api feature)

**Başlatma:**
```bash
orkestra-server.exe --port 3001 --db /path/to/orkestra.db
```

---

## Mimarı

**Teknoloji Stack:**
- **Framework:** Axum (Rust async HTTP)
- **DB Driver:** Tiberius (MSSQL, async)
- **Auth:** Token-based (random 32B hex)
- **Rate Limit:** IP + endpoint bazı per-minute
- **CORS:** AllowOrigin: localhost:8091 (nginx proxy)

**Endpoint Katmanları:**
1. **Middleware:** Rate limit, CORS, IP log
2. **Handler:** Validation, DB query, response JSON
3. **DB Layer:** Tiberius → localhost\MSSQLSERVER01 KO_MYKO

---

## Endpoint Tablosu

| Method | Path | Rate Limit | Gövde | Response | Auth | Not |
|--------|------|-----------|--------|----------|------|-----|
| **POST** | /api/site/register | 3/dk | {kullanici, sifre, email} | {ok, kullanici, msg} | None | Email verify token üret |
| **POST** | /api/site/login | 10/dk | {kullanici, sifre} | {ok, kullanici, yetki, token} | None | koweb_email_auth='1' şart |
| **GET** | /api/site/verify | - | ?user=X&code=Y | {status, msg} | None | Email doğrulama link |
| **POST** | /api/site/forgot-password | - | {email} | {ok, msg} | None | Reset token email'le |
| **POST** | /api/site/reset-password | - | {token, yeni_sifre} | {ok, msg} | None | Şifre sıfırla |
| **GET** | /api/site/online | 100/dk | - | {count} | None | SELECT COUNT(*) CURRENTUSER |
| **GET** | /api/site/rankings | 100/dk | ?type=?&limit=? | {rankings: []} | None | TOP 20 USERDATA |
| **GET** | /api/site/server-status | 100/dk | - | {gameserver, loginserver, durum} | None | TCP ping 15100+15001 |
| **GET** | /api/site/health | 100/dk | - | {ok, mssql, versiyon} | None | DB health check |

---

## Detaylı Endpoint Açıklaması

### 1. POST /api/site/register

**Request:**
```json
{
  "kullanici": "TURK",
  "sifre": "Sifre123",
  "email": "turk@example.com"
}
```

**Validation:**
- kullanici: 4-16 char, alphanumeric + underscore
- sifre: 6-20 char
- email: RFC 5322 format

**Rate Limit:** 3/dk per IP (429 TOO_MANY_REQUESTS)

**DB Query:**
1. Duplicate check: `SELECT COUNT(*) FROM TB_USER WHERE strAccountID = @P1 OR Email = @P2`
2. INSERT:
   ```sql
   INSERT INTO TB_USER 
     (strAccountID, strPasswd, strPasswdHash, Email, koweb_email, 
      koweb_email_auth, koweb_email_random_code, strAuthority)
   VALUES 
     (@user, @plain_28ch, @sha256_hash, @email, @email, 
      '0', @verify_token, 1)
   ```

**Response (201 CREATED):**
```json
{
  "ok": true,
  "kullanici": "TURK",
  "msg": "Kayıt başarılı! Lütfen e-posta adresinizi doğrulayın."
}
```

**Errors:**
- 400 BAD_REQUEST: Validation hatası
- 409 CONFLICT: Duplicate kullanıcı/email
- 429 TOO_MANY_REQUESTS: Rate limit
- 500 INTERNAL_SERVER_ERROR: DB hatası

**Async Event:** SMTP maili fire-forget (backgroundta gönder)

**⚠️ BUG:** strWebHash kolon NULL bırakılıyor → login bozulabiliyor

---

### 2. POST /api/site/login

**Request:**
```json
{
  "kullanici": "TURK",
  "sifre": "Sifre123"
}
```

**Rate Limit:** 10/dk per IP

**DB Query:**
```sql
SELECT strAccountID, strAuthority, koweb_email_auth
FROM TB_USER 
WHERE strAccountID = @user 
  AND strPasswdHash = @sha256_hash
```

**Auth Logic:**
1. SHA256(sifre) hesapla
2. DB'de strPasswdHash ile karşılaştır
3. koweb_email_auth == '1' mi? → HAYIR ise 403 FORBIDDEN dön
4. strAuthority okunur (tinyint → u8)

**Response (200 OK):**
```json
{
  "ok": true,
  "kullanici": "TURK",
  "yetki": 1,
  "token": "a1b2c3d4..."
}
```

**Yetki Seviyeleri:**
- 1 = Normal oyuncu
- 2 = GM (Game Master)
- 9 = Admin

**Token:** 32B random hex, cookie olarak PHP tarafında set

**Errors:**
- 400 BAD_REQUEST: Empty field
- 401 UNAUTHORIZED: Username/password mismatch
- 403 FORBIDDEN: Email not verified
- 429 TOO_MANY_REQUESTS: Rate limit
- 500 INTERNAL_SERVER_ERROR: DB hatası

---

### 3. GET /api/site/verify

**Query String:**
```
?user=TURK&code=a1b2c3d4e5f6...
```

**DB Query:**
```sql
SELECT koweb_email_random_code, koweb_email
FROM TB_USER
WHERE strAccountID = @user
```

**Logic:**
1. Token karşılaştır (DB token == query code)
2. Match ise:
   ```sql
   UPDATE TB_USER SET koweb_email_auth = '1' WHERE strAccountID = @user
   ```

**Response (200 OK):**
```json
{
  "status": "ok",
  "msg": "E-posta adresiniz doğrulanmıştır. Şimdi giriş yapabilirsiniz."
}
```

**Errors:**
- 400 BAD_REQUEST: Missing parameter
- 400: Kullanıcı/code mismatch
- 500: DB hatası

---

### 4. GET /api/site/online

**Query:** (hiç)

**DB Query:**
```sql
SELECT COUNT(*) FROM CURRENTUSER
```

**Response (200):**
```json
{
  "count": 234
}
```

DB hazır değilse: `{count: 0}`

---

### 5. GET /api/site/rankings

**Query String:**
```
?type=national&limit=20  (example)
```

**DB Query (örnek):**
```sql
SELECT TOP @limit strAccountID, Nation_Contributions, ...
FROM USERDATA
ORDER BY Nation_Contributions DESC
```

**Response (200):**
```json
{
  "rankings": [
    {"no": 1, "kullanici": "IMAM", "puan": 1069, "klan": "?"},
    {"no": 2, "kullanici": "TURK", "puan": 950, "klan": "?"}
  ]
}
```

---

### 6. GET /api/site/server-status

**Query:** (hiç)

**Logic:**
```rust
TcpStream::connect_timeout("104.238.23.99:15100", 3s)  // LS
TcpStream::connect_timeout("104.238.23.99:15001", 3s)  // GS
```

**Response (200):**
```json
{
  "gameserver": {"ip": "104.238.23.99", "port": 15100, "acik": true},
  "loginserver": {"ip": "104.238.23.99", "port": 15001, "acik": false},
  "durum": "KISMI"
}
```

Status: "ACIK" (both), "KISMI" (one), "KAPALI" (none)

---

### 7. GET /api/site/health

**Query:** (hiç)

**Logic:**
```rust
pool.get() → test query ("SELECT @@VERSION")
```

**Response (200 CONNECTED):**
```json
{
  "ok": true,
  "mssql": "BAĞLI",
  "versiyon": "Microsoft SQL Server 2019 (RTM) - 15.0.2000.5"
}
```

**Response (500 OFFLINE):**
```json
{
  "ok": false,
  "mssql": "HATA",
  "hata": "[timeout] connection failed"
}
```

---

## Auth Flow Özet

```
1. /api/site/register → Email token
   ↓
2. Email link click: /api/site/verify?user=X&code=Y
   ↓
3. koweb_email_auth = '1'
   ↓
4. /api/site/login OK → token returned
   ↓
5. Token cookie set (PHP)
   ↓
6. Sonraki istekler token ile (TBD — header/cookie format tanımlı değil)
```

---

## Rate Limiting Detay

**Per-IP tracking:**
- register: 3/dk
- login: 10/dk
- others: 100/dk (online, rankings, server-status, health)

**Implementation:** In-memory HashMap + Instant (Linux epoch secs)

**Reset:** 60 saniyelik sliding window

---

## Error Kodları + HTTP Status

| Status | JSON hata | Neden |
|--------|-----------|-------|
| 200 | - | Başarı |
| 201 | - | Kayıt başarılı |
| 400 | "hata": "..." | Validation hatası |
| 401 | "hata": "Kullanıcı adı veya şifre hatalı" | Login fail |
| 403 | "hata": "E-posta doğrulanmamış" | Email auth pending |
| 409 | "hata": "Bu kullanıcı adı zaten kayıtlı" | Duplicate |
| 429 | "hata": "Çok fazla giriş denemesi" | Rate limit |
| 500 | "hata": "DB bağlantı hatası" | Server error |
| 503 | "hata": "Veritabanı hazır değil" | DB pool down |

---

## DB Şema (Site API Kapsam)

**Yazılan Tablolar:**
- TB_USER (INSERT register, UPDATE verify)
  - strAccountID (PK)
  - strPasswd (plain, 28ch)
  - strPasswdHash (SHA256)
  - Email
  - koweb_email
  - koweb_email_auth ('0' → '1')
  - koweb_email_random_code
  - strAuthority (1/2/9)

**Okunan Tablolar:**
- CURRENTUSER (online count)
- USERDATA (rankings)

---

## Güvenlik Notları

### ✅ İyi Yapılan
- SHA256 şifre hash
- Rate limiting (3/dk register, 10/dk login)
- Email doğrulama şart
- Input validation (regex)

### ⚠️ Sorunlu
- Plain text backup (strPasswd) — DB dump riski
- Token expiry tanımlı değil — session hijack riski
- HTTPS yok — MITM açığı
- strWebHash NULL — login bug

### 🔴 Kritik
- Email verify token timeout tanımlı değil
- Reset password endpoint → authentication detay eksik

---

## Devir Notları (WEB+FORUM S87)

1. **Rust derlemesi:** `cargo build --release --features=site-api`
2. **MSSQL pool:** Otomatik init, timeout 30s default
3. **Logging:** stderr'e event log, DB insert yok
4. **Feature:** site-api kapalı → 501 NOT_IMPLEMENTED

---

**Yazı:** WEBRA | **Tarih:** 2026-04-29 | **Kategori:** WEB-API
