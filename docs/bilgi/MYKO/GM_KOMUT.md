# 🛡️ GM KOMUT REHBERİ — MYKO 1098

**Tarih:** 2026-04-29 (S88) | **Yazan:** DOKTOR
**Kaynak:** `C:\temp\MYKO\src\GameServer_SRC\GameServer\ChatHandler.cpp` (200 komut), `GMCommandsHandler.cpp`, `shared\globals.h`
**Hedef:** Lansman gününde GM müdahale için tek sayfa referans.

---

## 1. YETKI SİSTEMİ (Authority)

`shared/globals.h:562-564` — `enum AuthorityTypes`:

| Değer | Sabit | Anlam |
|-------|-------|-------|
| **0** | `AUTHORITY_GAME_MASTER` | Kalıcı GM (tüm yetki, görünmez yapabilir) |
| **1** | `AUTHORITY_PLAYER` | Normal oyuncu |
| **2** | `AUTHORITY_GM_USER` | Anlık GM (`+gm` toggle ile aktif) |

**DB:** `USERDATA.Authority` (kalıcı için 0). Anlık GM (`+gm`) çalışma süresi karakter session'ı.

**Aktivasyon:**
1. `USERDATA.Authority = 0` SET (DB)
2. `GAME_MASTER_SETTINGS` tablosuna kayıt INSERT (server doğrulama yapıyor)
3. Client reconnect

⚠️ **`isGM()` check:** Her komut başında `if (!isGM()) return false`. Komut listesindeki tüm `&CUser::Handle*` fonksiyonları bu check'i yapar.

---

## 2. KOMUT KULLANIMI (Genel)

- **Prefix:** `+` ile başla (örn: `+notice merhaba`)
- **Hedef seçimi:** "Z" tuşu ile target select (mob/npc/oyuncu) — bazı komutlarda gerek
- **2 menü tipi:**
  - **Server-wide** komut (önbellek `CGameServerDlg` üzerinde): `+notice`, `+csw`, `+down`, `+reload*`
  - **User-bound** komut (`CUser` üzerinde): `+give`, `+zone`, `+mon`, `+npc`, `+gm`, vb.

---

## 3. KRİTİK LANSMAN KOMUTLARI (mutlaka bilinmeli)

### Bakım & Kapatma
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `+down N` | N dk sonra sunucuyu kapatır | `+down 5` |
| `+care N` | N dk sonra bakım modu | `+care 10` |
| `+careoff` | Bakım modunu kapatır | `+careoff` |
| `+notice <metin>` | Server-wide chat duyuru | `+notice Sunucu 5dk sonra restart` |
| `+noticeall <metin>` | Tüm dünyaya duyuru | `+noticeall Lansman acildi!` |
| `+permanent <metin>` | Üst-bar kalıcı yazı | `+permanent Bynoisee MalaysiaKO` |
| `+offpermanent` | Kalıcı yazıyı sıfırla | |
| `+reloadnotice` | `Notice.txt` yeniden yükle | |

### Oyuncu Yönetimi
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `+kill <Nick>` | Oyuncuyu DC eder | `+kill Karakter1` |
| `+block <Nick> [N]` | Ban (N dakika; 0/yok = permanent) | `+block Spammer 1440` |
| `+unblock <Nick>` | Ban kaldır | `+unblock Karakter1` |
| `+pcblock <Nick>` | PC (HWID) ban | |
| `+ipban <Nick> <dk> <sebep>` | IP ban | `+ipban Spam 0 hile` |
| `+ipunban <Nick>` | IP ban kaldır | |
| `+banlist` | Aktif banları göster | |
| `+changegm <Nick>` | Hedefi GM yap (kalıcı) | |
| `+gm` | Kendi GM modunu toggle (anlık görünmez) | |
| `+job N <Nick>` | Sınıf değiştir (1-Warrior, 2-Rogue, 3-Mage, 4-Priest) | `+job 3 Test` |
| `+gender <Nick>` | Cinsiyet toggle | |
| `+level <Nick> N` | Level set | `+level Test 83` |
| `+open_master <Nick>` | Master aç | |
| `+open_skill <Nick>` | Tüm skill aç | |
| `+open_questskill <Nick>` | Tüm görev aç | |
| `+clear <Nick>` | Envanter temizle | |
| `+bug` | Askıda kalan karakter kurtar | |
| `+partytp <Nick>` | Hedefin partisini yanına çek | |

### Item Verme
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `+give <Nick> <ItemID> <Adet> <Süre>` | Süreli item gönder | `+give Test 110001000 1 30` (30 gün) |
| `+online_give_item <ItemID> <Adet> <Süre>` | Online HERKESE | `+online_give_item 379154000 1 0` |
| `+zone_give_item <ZoneID> <ItemID> <Adet> <Süre>` | Zone'daki herkese | `+zone_give_item 21 110001000 1 7` |
| `+givegenie <Nick> <Süre>` | Genie süresi | |
| `+genie <Nick>` | Genie aç/kapat | |

### Hareket / Spawn
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `+zone <ZoneID>` | Zone'a ışınlan | `+zone 21` (Moradon) |
| `+mon <DBID>` | Mob spawn (respawn yok) | `+mon 1234` |
| `+npc <DBID>` | NPC spawn (respawn yok) | |
| `+kill` (Z hedefli) | Hedef mob/npc öldür | |
| `+aireset` | Tüm AI reset | |
| `+npcinfo` | Hedef NPC bilgi (Z ile) | |

### Drop / Test
| Komut | Açıklama |
|-------|----------|
| `+drop N` | NPC drop testi (Z hedef, max 9999) |
| `+fishing` | Balık drop testi |
| `+mining` | Maden drop testi |
| `+testing` | Sunucu test komutu |
| `+tbl` | TBL verilerini DB'ye yaz |

### Online Sayım
| Komut | Açıklama |
|-------|----------|
| `+count` | Toplam online sayısı |
| `+countzone <ZoneID>` | Zone'daki online sayısı |
| `+countlevel <N>` | Belirli level'deki online sayısı |
| `+info <Nick>` | Açık programları göster (anti-cheat) |

---

## 4. EVENT KOMUTLARI

| Event | Aç | Kapat |
|-------|-----|------|
| **CSW** (Castle Siege War) | `+csw` | `+cswclose` |
| **BDW** (Border Defense War) | `+borderopen` | `+borderclose` |
| **Chaos Expansion** | `+chaosopen` | `+chaosclose` |
| **Juraid Mountain** | `+juraidopen` | `+juraidclose` |
| **Forgotten Temple** | `+ftopen` | `+ftclose` |
| **MadClas** (Cindirella) | `+madclas <Tip>` (1=47lvl, 2=59lvl, 3=83lvl) | `+madclasclose` |
| **Snow War** | `+snow` | `+close` |
| **UTC** (Under the Castle) | `+utc` | `+utc` (toggle) |
| **Tournament** (Klan) | `+tournamentstart` | `+tournamentclose` |
| **Beef Event** | `+beefopen` | `+beefclose` |
| **Lottery** | `+lottery` | `+lotteryclose` |
| **Collection Race** | `+cropen <ID>` | `+crclose` |
| **Bowl Event** | `+bowlevent <Zone> <Süre> <Saniye>` | otomatik |
| **Special Event** | `+event` | — |
| **Santa** | `+santa` | `+santaclose` |
| **Angel** | `+angel` | `+angelclose` |
| **War result** | `+warresult` | — |
| **Captain (kaptanlık)** | `+captain <Nick>` | — |
| **Discount (kazanan ulus)** | `+discount` | `+offdiscount` |
| **Discount (herkes)** | `+alldiscount` | `+offdiscount` |
| **War Close (genel)** | `+close` | — |
| **TPAll** | `+tpall <ZoneID>` (zone'daki herkesi home'a çek) | — |

---

## 5. RELOAD KOMUTLARI (Hot Reload — DB değişiklik sonrası)

| Komut | Hangi tablo |
|-------|-------------|
| `+reloadalltables` | **HEPSI** (genel) |
| `+reloadtables` | Genel tablolar |
| `+reload_item` / `+reloaditems` | ITEM |
| `+reloadupgrade` | UPGRADE |
| `+reloadmagics` | MAGIC_TABLE (skill) |
| `+reloadquests` | Quest |
| `+reloadranks` | Sıralama |
| `+reloaddrops` | Drop |
| `+reloadkings` | Kral sistem |
| `+reloadtitle` | Sağ-üst başlık |
| `+reloadpus` | PUS_ITEMS (cash shop) |
| `+reloaddungeon` | Zindan savunma |
| `+reloaddraki` | Draki Tower |
| `+reloadevent` | EVENT_SCHEDULE |
| `+reloadpremium` | Klan premium |
| `+reloadsocial` | Sosyal grup ikon |
| `+reloadclanpnotice` | Klan premium duyuru |
| `+reloadbug` | Bug tablosu |
| `+reloadbot` | Bot bilgi |
| `+reloadlreward` | Level ödül |
| `+reloadmreward` | Merchant level ödül |
| `+reloadzoneon` | Zone online ödül |
| `+reload_cind` | Cindirella (MadClas) |
| `+reload_table` | Generic table |
| `+reloadnotice` | Notice.txt |

---

## 6. CONFIG / RUNTIME

| Komut | Açıklama |
|-------|----------|
| `+config <key> <value>` | Runtime config değiştir |
| `+resetloyalty` | Loyalty (sağ NP) sıfırla |
| `+censor` / `+uncensor` | Küfür filtresi aç/kapat |
| `+censoradd <kelime>` | Kelime ekle |
| `+censordel <kelime>` | Kelime sil |
| `+censorreload` | `censor_words.txt` yeniden yükle |

---

## 7. BOT (Test) KOMUTLARI

| Komut | Açıklama |
|-------|----------|
| `+user_bots <adet> <süre> <tip> <minLevel>` | Online görünüm botu (1=Mining, 2=Fishing, 3=Standing, 4=Sitting, 5=Random) |
| `+mbot` | Merchant bot ekle |
| `+mbotsave` | Merchant bot tabloya kaydet |
| `+sbot` | Merchant botları temizle |
| `+savebotmerchant` | Merchant bot kaydet |
| `+loadbotmerchant` | Merchant bot yükle |
| `+remove_bots` | Tüm botları DC |
| `+botfarmer` | Çiftçi bot |
| `+bot_login` | Bot giriş tipleri |

---

## 8. DİKKAT & UYARI

| ⚠️ | Konu |
|----|------|
| 1 | **`+down` PROD'DA 0 yazma** — anında kapanır, oyuncuları DC eder |
| 2 | **`+block` permanent (0)** — geri dönüşü manuel `+unblock` |
| 3 | **`+online_give_item`** — toplu, log'a düşer, gözle gör |
| 4 | **`+open_skill` / `+open_master`** — debug için, prod oyuncuya kullanma |
| 5 | **`+gm` toggle** — sadece GM_USER için, kalıcı GM'de fark yok |
| 6 | **`+aireset`** — tüm zone'da AI restart, lag yapabilir |
| 7 | **`+changegm`** — kalıcı, DB'ye yazar — YANLIŞ kullanırsa geri için DB UPDATE şart |
| 8 | **Komut TR/EN karışık** — hedef oyuncu ismi case-sensitive olabilir, doğrula |

---

## 9. SIK KULLANILAN ZONE ID'LER (referans, MAP_ZONE.md'den)

| ID | Zone |
|----|------|
| 21 | Moradon |
| 11 | Karus Eslant |
| 12 | El Morad Eslant |
| 30 | Ronark Land (CZ) |
| 31 | Delos (CSW) |
| (diğer için) | `MAP_ZONE.md` |

---

## 10. LANSMAN GÜNÜ KISA YOL

```
1. AÇILIŞ
   +permanent "Bynoisee MalaysiaKO Valor — HOSGELDINIZ"
   +noticeall "Sunucu acildi! Iyi oyunlar."

2. ETKİNLİK BAŞLATMA
   +csw                    # 19:00 castle siege
   +chaosopen              # event saatlerinde
   +borderopen             # BDW

3. ACIL DURUM
   +count                  # online kontrol
   +banlist                # aktif banlar
   +block <spam_nick>      # spam ban
   +reloaditems            # item tablosu güncelleme sonrası

4. KAPATMA
   +noticeall "5 dakika sonra restart"
   +down 5
```

---

## 11. KAYNAK REFERANSLAR

- **Komut tanımı:** `ChatHandler.cpp:233-432` (CGameServerDlg) + `:165-432` (CUser)
- **Komut implementasyon:** `GMCommandsHandler.cpp` (3000+ satır)
- **Yetki kontrol:** `User.h:872-873` (`isGM()`, `isGMUser()`)
- **Authority enum:** `shared/globals.h:560-565`
- **DB tablo:** `USERDATA.Authority`, `GAME_MASTER_SETTINGS`
- **İlgili MD:** `DB_SEMA.md § USERDATA`, `MAP_ZONE.md`, `GAME_LOGIC.md`

---

**Sürüm:** v1.0 — S88 ilk yazım (200 komut katalogu)
**Sonraki:** Lansman sonrası kullanılan komutların log'una göre güncelle.
