# Party Zone Değişimi — İsim Sarı Kalma Bug Notları

## Semptom
Party'de 2 kişi. Biri zone değiştiriyor. Geride kalan kişinin ekranında:
- Party WINDOW kapanıyor ✅ (KO client PARTY_DELETE doğru işliyor)
- Zone değiştiren kişinin ismi SARI kalmaya devam ediyor ❌
- Yeni partiye girilince veya zone değişince sarı gidiyor

## GameServer Tarafı (TAMAMLANDI)

Yapılanlar (tüm bunlar çalışıyor, loglar doğruladı):
- `ZoneChangeWarpHandler.cpp`: `m_vPendingPartyDelete` dolduruluyor, `PartyNemberRemove` çağrılıyor
- `PartyHandler.cpp` → `PartyNemberRemove` count==1: geride kalan üyelere `PARTY_REMOVE + PARTY_DELETE` gönderiliyor
- `PartyHandler.cpp` → `SendPartyInfoOnZoneChange`: zone geçen kişi zoneloaded olunca kendi PARTY_DELETE'ini alıyor
- `User.h`: `std::vector<uint16> m_vPendingPartyDelete` eklendi

**Denenen ama ÇALIŞMAYAN yaklaşımlar:**
- `PartyLeaderPromote` zone öncesi çağırmak → PARTY_INSERT confusion yarattı, kaldırıldı
- Sadece PARTY_DELETE göndermek → window kapandı ama isim kaldı
- PARTY_REMOVE + PARTY_DELETE → window kapandı ama isim kaldı

## Pearl Guard (AntiCheat) Kök Neden

### Sorun: SetNameString color cache
`Object_Player_Callback` her frame çalışıyor. `isPartyMember=false` olduğunda SetNameString **hiç çağrılmıyor** — eski sarı renk (0xFFFFFF00) object memory'de (offset 0x738) kalıyor.

```
// Mevcut kod — YANLIŞ:
bool isPartyMember = Engine->m_bInParty && uiPartyBBS->PartyFind(id);
if (isPartyMember)
    SetNameString(... sarı ...);
else if (...)
    SetNameString(... başka renk ...);
// else → HİÇ ÇAĞIRILMIYOR → eski renk kalıyor
```

### Neden `else` çağrılmıyor?
`Object_Player_Callback`'te `GetName(obj) == GetName(kendi_karakter)` kontrolü var.
Zone değiştiren kişi başka zone'da — o kişinin obj pointer'ı hala render pool'da olabilir.
`isPartyMember=false` → sarı renk branch'i skip → başka branch da çalışmıyor çünkü condition chain'i false.

### Olası fix (henüz denenmedi):
```cpp
// PARTY_DELETE handler'da (satır 6209):
Engine->m_bInParty = false;
// Burada tüm visible player obj'lerini iterate edip renk force-reset etmek gerekebilir
// VEYA Object_Player_Callback'in false path'i her zaman doğru rengi SET etmeli
```

### Denenen (ÇALIŞMADI):
- `m_bInParty` guard → `isPartyMember = Engine->m_bInParty && PartyFind(id)`
  - m_bInParty=false yapıldıktan sonra SetNameString çağrılmıyor, renk güncellenmedi

## Önemli Offsetler
```
KO_OFF_PTBASE = 0x238   // 2369
KO_OFF_PT     = 0x340
KO_OFF_PTCOUNT = 0x344
```

## Sıradaki Deneme
`Object_Player_Callback`'te `isPartyMember=false` durumunda da açıkça doğru rengi SET et.
Yani her condition branch'i (`if/else if/else`) her zaman SetNameString çağırmalı.
Şu anda "else" durumunda sarı yerine başka renk set edilmiyor.

## İlgili Dosyalar
- `Pearl Guard\Pearl Engine.cpp` satır 1654–1707 (Object_Player_Callback)
- `Pearl Guard\Pearl Engine.cpp` satır 6207–6226 (PARTY_DELETE handler)
- `Pearl Guard\UIPartyBBS.cpp` satır 102–137 (PartyFind)
- `GameServer\PartyHandler.cpp` (tüm party logic)
- `GameServer\ZoneChangeWarpHandler.cpp` (zone + party cleanup)
