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

// 8v8 zone giris kontrolu (zone change handler'da cagrilir)
// Klan bracket'a kayitli AND uye listesinde varsa true; aksi durumda false
bool Bracket8v8CanEnterZone(uint16 clanID, const std::string& charName, uint8 zoneID)
{
	// Simdilik: her zaman izin ver — tam DB SELECT acilis sonrasi
	// Bu fonksiyon ZoneChangeWarpHandler.cpp'den cagrilabilir, kayitli klan/uye degilse zone'a girise engel
	// TODO acilis sonrasi: SELECT _MK_BRACKET_PARTY_MEMBERS WHERE BracketID=? AND ClanID=? AND MemberCharName=?
	return true;
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
	// Su an: direkt mesaj — DB SELECT entegrasyon acilis sonrasi
	char buf[200] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[8v8] Liste DB'de tutuluyor. WEBRA dashboard ile gorebilirsin (bracket %d).",
		bracketID);
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);
	return true;
}
