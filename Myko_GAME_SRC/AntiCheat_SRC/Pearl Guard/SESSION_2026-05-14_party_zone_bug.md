# Session 2026-05-14/15 — Party Zone Change Bug

## Semptom
Party'de 2 kişi. Biri zone değiştiriyor:
- Party WINDOW kapanıyor ✅ (KO client PARTY_DELETE doğru işliyor)
- Zone değiştiren kişinin ismi geride kalan kişinin ekranında SARI kalıyor ❌
- Yeni party kurulunca veya zone değişince sarı gidiyor

Ek semptomlar (sonraki testte):
- Başkan zone değiştirince → party patlıyor ama başkan sarı kalıyor
- Üye zone değiştirince → party patlıyor ama giden sarı kalıyor
- Başkan party dağıtınca → normal çalışıyor
- Üye partyden çıkınca → party bug'da kalıyor

---

## GameServer Tarafı — Yapılanlar

### ZoneChangeWarpHandler.cpp
Zone değişiminde `PartyNemberRemove(GetSocketID())` eklendi.

Sonradan eklenen ve sonra kaldırılan:
- `m_vPendingPartyDelete` doldurma bloğu
- `SendPartyInfoOnZoneChange()` çağrısı ZoneChangeLoaded'da

**Son durum:** Sadece `PartyNemberRemove(GetSocketID())` kaldı. `m_vPendingPartyDelete` ve `SendPartyInfoOnZoneChange` kaldırıldı.

### PartyHandler.cpp — PartyNemberRemove count==1 bloğu
Geride tek üye kalınca `PARTY_REMOVE(zone_geçen_id) + PARTY_DELETE` gönderiliyor.
`PartyisDelete()` kaldırıldı — direkt paket gönderilmeye geçildi.

### User.h
`std::vector<uint16> m_vPendingPartyDelete` eklendi (sonradan kullanılmadı ama kaldı).

---

## AntiCheat (Pearl Guard) Tarafı — Denenen Yaklaşımlar

### Kök Neden Analizi
`Object_Player_Callback` her frame çalışıyor. `uiPartyBBS->PartyFind(id)` → KO client memory'den `KO_OFF_PTCOUNT` okuyor. `PARTY_DELETE` sonrası KO client party window'u kapatıyor ama memory stale kalabiliyor → sarı renk devam ediyor.

GM char'ında olmayan ama normal char'da olan: `switch(authority)` bloğunda GM için `SetNameString(..., 0, 0)` çağrılıyor → renk sıfırlanıyor. Normal char için case yok → sarı kalıyor.

---

## Deneme 1 — m_bInParty Guard (ÇALIŞMADI)
**Commit:** `5baacde`
```cpp
bool isPartyMember = Engine->m_bInParty && Engine->uiPartyBBS->PartyFind(id);
```
`PARTY_DELETE`'te `m_bInParty=false`. Ama `m_bInParty` sonradan tekrar `true` oluyorsa etkisiz.
**Sonuç:** Hala sarı kalıyor.

---

## Deneme 2 — KO_OFF_PTCOUNT Sıfırla (BOZDU)
**Commit:** `6dad707` → revert: `d52a2ba`
```cpp
DWORD base = *(DWORD*)(*(DWORD*)KO_DLG + KO_OFF_PTBASE);
*(DWORD*)(base + KO_OFF_PTCOUNT) = 0;
```
Party dışı şeyleri bozdu (3 char sokuncaHer şey bozuldu).
**Sonuç:** Revert edildi.

---

## Deneme 3 — Kendi g_partyIds Set (KISMEN ÇALIŞTI)
**Commit:** `bbc375a`
```cpp
std::unordered_set<uint16> g_partyIds;
// PARTY_INSERT'te: g_partyIds.insert(partyid)
// PARTY_REMOVE'da: g_partyIds.erase(removedId)
// PARTY_DELETE'te: g_partyIds.clear()
// Object_Player_Callback'te: g_partyIds.count(id) > 0
```
Sarı renk düzeldi ama başka senaryolarda sorun devam etti.

**Alt denemeler:**
- `ef84b26`: PARTY_REMOVE'da ID sil → başkan senaryosunda çalışmıyor
- `796ac25`: PARTY_REMOVE'da tamamen clear → 3+ üyeli partyde diğerleri de sarıdan çıkıyor
- `53afe72`: m_bInParty guard geri eklendi

**Sorun:** `partyid` (PARTY_INSERT'ten gelen socket ID) ile `id` (obj memory offset) eşleşmesi güvenilir değil veya PARTY_DELETE handler'ına girilmiyordu.

---

## Deneme 4 — Party Sarı Rengi Tamamen Kaldır (KABUL EDİLDİ)
**Commit:** `9f9d69b`

Eski src referansları incelendi:
- `MYKO_PK_28.02.2025_YEDEK` → Pearl Engine'de WIZ_PARTY handler yok
- `ALPHA-2383 KO PROJE ACS Source` → party renklendirme yok

**Sonuç:** Party sarı isim rengi bizim eklediğimiz bir özellikti, orijinal KO'da yok. Özellik kaldırıldı.

`Object_Player_Callback`'te sarı renk branch'leri silindi, sadece:
- Kendi nick → açık mavi (veya level<30 beyaz)
- Düşman ırk → kırmızı
- Kendi ırk → koyu mavi
- Level<30 → beyaz

---

## ZoneChange Party Kodu Kaldırma (SON DURUM)
**Commit:** `00daea8` (GameServer)

Eski src'lerde `ZoneChangeWarpHandler.cpp`'de hiç party kodu yok. Biz eklemiştik → party davranışı bozuluyordu (intermittent, race condition). Kaldırıldı:
- `m_vPendingPartyDelete` doldurma bloğu
- `SendPartyInfoOnZoneChange()` çağrısı

`PartyNemberRemove(GetSocketID())` hala duruyor — bu zone değişiminde server tarafının party'den çıkarması için gerekli.

---

## Mevcut Durum (Deploy Bekliyor)
- AntiCheat: sarı renk yok, party window kapanması KO client hallediyor ✅
- GameServer: ZoneChange'de sadece `PartyNemberRemove` var, extra kod yok
- **Sunucuya yeni GameServer.exe deploy edilmedi** — onay bekleniyor

---

## Önemli Offsetler
```
KO_OFF_PTBASE  = 0x238  (2369)
KO_OFF_PT      = 0x340
KO_OFF_PTCOUNT = 0x344
KO_WH          = 0x758  (authority offset)
```

## İlgili Dosyalar
- `AntiCheat_SRC\Pearl Guard\Pearl Engine.cpp` — Object_Player_Callback, WIZ_PARTY handler
- `GameServer_SRC\GameServer\ZoneChangeWarpHandler.cpp`
- `GameServer_SRC\GameServer\PartyHandler.cpp` — PartyNemberRemove
- `GameServer_SRC\GameServer\User.h` — m_vPendingPartyDelete (kullanılmıyor, kaldırılabilir)
