// =========================================================================
// S115 FAZ 12 — 8v8 Clan vs Clan Bracket Entegrasyon
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// Klan lideri 8'erli party uye listesini belirler.
// Tournament zone girisinde sadece liste uyeleri kabul edilir.
// MATRIX SP'leri: SP_BRACKET_PARTY_MEMBER_ADD (hazir, MSG:5915)
//
// Komutlar (chat, klan lideri):
//   +bracket8v8add  <BracketID> <Nick> [PartyNum]   — uye ekle (default party=1)
//   +bracket8v8list <BracketID>                     — atanmis uyeler
//   +bracket8v8del  <BracketID> <Nick>              — uye sil (su an pasif, DB SP yok)
//
// PG temiz: WIZ_CHAT, yeni opcode YOK.
// =========================================================================

#include "stdafx.h"

// =========================================================================
// 8v8 zone giris kontrolu — ZoneChangeWarpHandler.cpp'den cagrilir
//
// Mantik:
//   1) Zone'da aktif tournament (TOURNAMENT_DATA) var mi? Yoksa = bracket-disi giris,
//      izin ver (normal /tournamentstart maci, herkese acik).
//   2) Tournament'in bracketMatchID > 0 mi? Yoksa = bracket disi, izin ver.
//   3) Klan, bu mac'in red/blue klani mi? Degilse = baska klan, izin VERME
//      (bracket kapali maclar — sadece taraf klanlari girer).
//   4) Klan dogru ve MATRIX SP'si APPLY edildiyse: party member listesinde mi?
//      Listede ise izin ver, degilse = lider eklemedi, izin VERME.
//
// fail-open tasarim: BracketPartyMemberCheck false dondugunde 2 sebep olabilir:
//   (a) MATRIX SP henuz APPLY edilmedi (acilis oncesi gecici)
//   (b) Lider gercekten eklemedi (asil senaryo)
//   Ikisini ayirt etmek icin g_bracket_party_check_failed_once flag'i ile bir
//   kerelik uyari log'la, sonra defansif olarak TRUE don (acilis maclarini
//   bozmamak icin). MATRIX 108b APPLY edildiginde SP basarili calisir, filtre
//   aktif olur. Defansif tasarim — patron lokal test edip MATRIX'i bekliyor.
//
// Returns: true=giris izinli, false=giris engelli
// =========================================================================
extern bool BracketGetMatchInfo(int32_t matchID, int32_t& bracketIDOut,
                                uint16& redClanIDOut, uint16& blueClanIDOut);

static bool g_bracketPartyCheckSPWarned = false;

bool Bracket8v8CanEnterZone(uint16 clanID, const std::string& charName, uint8 zoneID)
{
	// 1) Tournament aktif mi?
	_TOURNAMENT_DATA* tInfo = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (tInfo == nullptr) return true;          // normal zone, bracket disi
	if (!tInfo->aTournamentisStarted) return true;

	// 2) Bracket'a bagli mac mi?
	int32_t matchID = tInfo->bracketMatchID;
	if (matchID <= 0) return true;              // standalone tournament, herkes girer

	// 3) Klan match'in taraflarindan biri mi?
	int32_t bracketID = 0;
	uint16 redClanID = 0, blueClanID = 0;
	if (!BracketGetMatchInfo(matchID, bracketID, redClanID, blueClanID)) {
		// Match RAM'de yok — RAM cache bayat olabilir, defansif olarak izin ver
		// (BracketAutoStartTimer mac olusturmus olabilir, refresh gecikmesi)
		printf("[8v8] CanEnter: match=%d RAM'de yok (zone=%u clan=%u user=%s) → ALLOW (defansif)\n",
			matchID, zoneID, clanID, charName.c_str());
		return true;
	}
	if (clanID != redClanID && clanID != blueClanID) {
		// Baska klan kullanicisi bracket mac zone'una giremez
		return false;
	}

	// 4) Party member listesinde mi? (MATRIX SP gerekiyor)
	std::vector<CDBAgent::_BPM_LIST_ROW> rows;
	if (!g_DBAgent.BracketPartyMemberList(bracketID, clanID, rows)) {
		// SP yok veya hata — fail-open, bir kerelik uyari
		if (!g_bracketPartyCheckSPWarned) {
			printf("[8v8] UYARI: SP_BRACKET_PARTY_MEMBER_LIST/CHECK yok veya hata.\n"
				"        MATRIX 108b APPLY edilmemis olabilir. Filtre PASIF — herkes giriyor.\n"
				"        Bu uyari bir kez basilir. APPLY sonrasi otomatik aktif olur.\n");
			g_bracketPartyCheckSPWarned = true;
		}
		return true;  // fail-open: acilis maclarini bozmamak icin izin ver
	}

	// SP basarili calisti — liste bos veya icinde charName var mi?
	if (rows.empty()) {
		// Lider hic kimse eklemedi → tum klan uyelerinin girisine izin (fallback)
		// Bu pre-mac asama (lider listeleri henuz hazirlamadi); klan tarafindan
		// resmi 8v8 olmadigi takdirde herkes girer (normal tournament gibi).
		return true;
	}
	for (auto& r : rows) {
		if (r.memberCharName == charName) return true;
	}

	// Lider listeyi hazirladi ama bu kullanici listede degil
	printf("[8v8] CanEnter: bracket=%d clan=%u user=%s lider listesinde yok → DENY\n",
		bracketID, clanID, charName.c_str());
	return false;
}

// +bracket8v8add <BracketID> <Nick> [PartyNum]
COMMAND_HANDLER(CUser::HandleBracket8v8AddCommand)
{
	if (!isInClan()) {
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin.");
		return true;
	}
	if (!isClanLeader()) {
		g_pMain->SendHelpDescription(this, "Sadece klan lideri uye ekleyebilir.");
		return true;
	}
	if (vargs.size() < 2) {
		g_pMain->SendHelpDescription(this,
			"+bracket8v8add <BracketID> <Nick> [Party#]. Ornek: +bracket8v8add 1 Erencan 1");
		return true;
	}

	int32_t bracketID = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);
	vargs.pop_front();
	std::string nick = vargs.front();
	vargs.pop_front();
	uint8 partyNum = 1;
	if (!vargs.empty()) {
		partyNum = (uint8)SafeAtoi(vargs.front(), 1, 4);
	}

	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr) {
		g_pMain->SendHelpDescription(this, "Klan bulunamadi.");
		return true;
	}

	// BUG #19 FIX: Offline uyesi de eklenebilmeli (mac oncesi pre-register).
	// Klan uyeligi kontrol MATRIX SP_BRACKET_PARTY_MEMBER_ADD icinde yapilir
	// (DB sorgusu USERDATA.Knights = ?). SP ALREADY_IN_PARTY/NOT_CLAN_MEMBER doner.

	// DB SP_BRACKET_PARTY_MEMBER_ADD (MATRIX MSG:5915)
	std::string result;
	bool ok = g_DBAgent.BracketPartyMemberAdd(bracketID, pClan->GetID(),
		nick, partyNum, "LEADER", result);

	char buf[200] = { 0 };
	if (ok) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[8v8] %s party %u'e eklendi (bracket %d).",
			nick.c_str(), partyNum, bracketID);
	} else {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[8v8] Hata: %s (bracket %d).", result.c_str(), bracketID);
	}

	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);
	return true;
}

// +bracket8v8list <BracketID> — kendi klanindaki uyeleri listele
COMMAND_HANDLER(CUser::HandleBracket8v8ListCommand)
{
	if (!isInClan()) {
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin.");
		return true;
	}
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this,
			"+bracket8v8list <BracketID>. Ornek: +bracket8v8list 1");
		return true;
	}

	int32_t bracketID = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	// DB SELECT — klan'in atadigi tum uyeleri al
	std::vector<CDBAgent::_BPM_LIST_ROW> rows;
	if (!g_DBAgent.BracketPartyMemberList(bracketID, GetClanID(), rows)) {
		g_pMain->SendHelpDescription(this,
			"[8v8] DB hata — liste cekilemedi. MATRIX SP yetisemediyse acilis oncesi SP_BRACKET_PARTY_MEMBER_LIST gerek.");
		return true;
	}

	// Bos liste
	if (rows.empty()) {
		char buf[200] = { 0 };
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[8v8] Bracket %d, klanin %u — henuz atanmis uye yok. Lider: +bracket8v8add <BID> <Nick> [Party#]",
			bracketID, GetClanID());
		std::string msg = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		Send(&pkt);
		return true;
	}

	// Header
	char buf[256] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[8v8] Bracket %d — klan %u atanmis uyeler (toplam %zu):",
		bracketID, GetClanID(), rows.size());
	std::string header = buf;
	{
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &header);
		Send(&pkt);
	}

	// Satir satir (max 32: bracket party limit 4x8)
	const size_t MAX_LIST_ROWS = 32;
	size_t count = (rows.size() < MAX_LIST_ROWS) ? rows.size() : MAX_LIST_ROWS;
	for (size_t i = 0; i < count; i++) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"  Party%u: %s (%s, %s)",
			rows[i].partyNumber,
			rows[i].memberCharName.c_str(),
			rows[i].assignedBy.c_str(),
			rows[i].addedAt.c_str());
		std::string line = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &line);
		Send(&pkt);
	}
	if (rows.size() > MAX_LIST_ROWS) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"  ... +%zu daha (max %zu gosterildi)",
			rows.size() - MAX_LIST_ROWS, MAX_LIST_ROWS);
		std::string more = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &more);
		Send(&pkt);
	}

	return true;
}
