// =========================================================================
// S115 FAZ 14 — Spectator (Izleyici) Modu
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// Oyuncu tournament/bracket/CTF zone'una "izleyici" olarak girer:
//   - Olmez (GM invisible benzeri ABNORMAL_INVISIBLE)
//   - PVP target olmaz
//   - Bahis sistemi entegre (+bet ile beraber)
//   - Mac bitince otomatik cikis
//
// Tournament zone listesi: 77, 78, 96-99 (Party VS)
// CTF zone'lari da dahil olabilir (ileride genisletilir)
//
// Komut: +spectate ZONE_ID (oyuncu kullanir)
//        +spectate exit    (cikis)
//
// PG temiz: standart ABNORMAL_INVISIBLE state (GM modunda kullaniliyor),
//           yeni opcode YOK.
// =========================================================================

#include "stdafx.h"

// Aktif spectator listesi (UserID set)
static std::set<int32_t> g_spectatorUsers;
static std::recursive_mutex g_spectatorLock;

// Kontrol — kullanici spectator mi?
bool IsUserSpectator(int32_t userID)
{
	std::lock_guard<std::recursive_mutex> lock(g_spectatorLock);
	return g_spectatorUsers.find(userID) != g_spectatorUsers.end();
}

// Spectator listesinden cikar (logout / mac bitti)
void RemoveSpectator(int32_t userID)
{
	std::lock_guard<std::recursive_mutex> lock(g_spectatorLock);
	g_spectatorUsers.erase(userID);
}

// +spectate ZONE_ID  veya  +spectate exit
COMMAND_HANDLER(CUser::HandleSpectateCommand)
{
	if (isDead() || isTrading() || isMerchanting()) {
		g_pMain->SendHelpDescription(this, "Spectator olamazsin: olu/ticaret durumu.");
		return true;
	}

	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this,
			"+spectate ZONE  veya  +spectate exit. Tournament zone'lari: 77/78/96/97/98/99");
		return true;
	}

	std::string arg = vargs.front();

	// Cikis
	if (arg == "exit" || arg == "cikis" || arg == "out") {
		if (!IsUserSpectator(GetID())) {
			g_pMain->SendHelpDescription(this, "Zaten spectator degilsin.");
			return true;
		}
		RemoveSpectator(GetID());

		// BUG #17 FIX — Invisible state temizle, sonra zone change (GM mode pattern)
		m_bAbnormalType = ABNORMAL_NORMAL;
		ZoneChange(ZONE_MORADON, 905.0f, 870.0f);
		g_pMain->SendHelpDescription(this, "Spectator modundan ciktin.");
		return true;
	}

	// Spectator girisi
	uint8 zoneID = (uint8)SafeAtoi(arg, 1, 255);

	// Sadece tournament/CTF zone'lari kabul (basit kontrol)
	bool validZone =
		(zoneID == 77 || zoneID == 78 ||
		 zoneID == 96 || zoneID == 97 || zoneID == 98 || zoneID == 99);
	if (!validZone) {
		g_pMain->SendHelpDescription(this,
			"Sadece tournament zone (77/78/96-99) icin spectator olabilirsin.");
		return true;
	}

	// Zone'da aktif tournament/CTF var mi?
	_TOURNAMENT_DATA* tInfo = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (tInfo == nullptr || !tInfo->aTournamentisStarted) {
		g_pMain->SendHelpDescription(this,
			"Bu zone'da aktif tournament yok. Bekle veya farkli zone dene.");
		return true;
	}

	// Tournament'a kayitli klan uyesi mi? Eger oyleyse spectator olamaz (savasacak)
	if (isInClan()) {
		uint16 cid = GetClanID();
		if (cid == tInfo->aTournamentClanNum[0] || cid == tInfo->aTournamentClanNum[1]) {
			g_pMain->SendHelpDescription(this,
				"Kendi klanin savasiyor — spectator olamazsin, savas!");
			return true;
		}
	}

	// Spectator listesine ekle
	{
		std::lock_guard<std::recursive_mutex> lock(g_spectatorLock);
		g_spectatorUsers.insert(GetID());
	}

	// BUG #17/#18 FIX — GM pattern: UserInOut(INOUT_OUT) ONCE, sonra ABNORMAL_INVISIBLE
	// PVP target olmaz, oldurulmez (RegionHandler invisible kullanicilari atlar)
	UserInOut(INOUT_OUT);
	m_bAbnormalType = ABNORMAL_INVISIBLE;

	// Zone'a teleport (orta nokta) — check=false ile CanChangeZone bypass (spectator yetki)
	ZoneChange(zoneID, 700.0f, 700.0f, -1, false);

	char buf[200] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[SPECTATOR] Zone %u tournament izlemeye basladin. Cikmak icin: +spectate exit. "
		"Bahis koymak icin: +bet KLAN_ADI MIKTAR",
		zoneID);
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);

	return true;
}
