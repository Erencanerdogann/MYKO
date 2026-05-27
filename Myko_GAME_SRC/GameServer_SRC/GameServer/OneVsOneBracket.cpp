// =========================================================================
// S115 FAZ 16 — 1v1 Bracket Turnuva (Solo)
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// Clan Bracket'a benzer, ama klan yerine solo oyuncu.
// 16/32/64 kisi destekler.
//
// MATRIX SP'leri (migration 112_1v1_bracket): SP_1V1_CREATE/REGISTER/...
//
// Komutlar:
//   /1v1create "Ad" MaxPlayer  (console)
//   /1v1start BID
//   /1v1cancel BID
//   +1v1reg BID                (oyun ici)
//
// PG temiz: WIZ_CHAT, yeni opcode YOK.
// =========================================================================

#include "stdafx.h"

// GM Komutlari (console)
COMMAND_HANDLER(CGameServerDlg::HandleOneVsOneCreateCommand)
{
	if (vargs.size() < 2) {
		printf("Usage: /1v1create \"Ad\" MaxPlayers (16/32/64)\n");
		return true;
	}
	std::string name = vargs.front(); vargs.pop_front();
	uint8 maxPlayers = (uint8)SafeAtoi(vargs.front(), 16, 64);

	if (maxPlayers != 16 && maxPlayers != 32 && maxPlayers != 64) {
		printf("[1V1] MaxPlayers must be 16/32/64\n");
		return true;
	}

	int32_t bid = g_DBAgent.OneVsOneCreate(name, maxPlayers, "console");
	printf("[1V1] Created: BID=%d Name='%s' MaxPlayers=%u\n",
		bid, name.c_str(), maxPlayers);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleOneVsOneStartCommand)
{
	if (vargs.empty()) {
		printf("Usage: /1v1start BID\n");
		return true;
	}
	int32_t bid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	if (g_DBAgent.OneVsOneGenerateMatches(bid)) {
		printf("[1V1] Started: BID=%d Round 1 maclari olustu\n", bid);
	} else {
		printf("[1V1] Start hata: BID=%d\n", bid);
	}
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleOneVsOneCancelCommand)
{
	if (vargs.empty()) {
		printf("Usage: /1v1cancel BID\n");
		return true;
	}
	int32_t bid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	if (g_DBAgent.OneVsOneCancel(bid)) {
		printf("[1V1] Cancelled: BID=%d\n", bid);
	} else {
		printf("[1V1] Cancel hata: BID=%d\n", bid);
	}
	return true;
}

// +1v1reg BID
COMMAND_HANDLER(CUser::HandleOneVsOneRegCommand)
{
	if (isDead()) {
		g_pMain->SendHelpDescription(this, "Olu iken 1v1 bracket kayit yapamazsin.");
		return true;
	}
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this, "+1v1reg BID. Ornek: +1v1reg 1");
		return true;
	}
	int32_t bid = SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	std::string result;
	bool ok = g_DBAgent.OneVsOneRegister(bid, GetID(), GetName(),
		(uint8)GetClass(), GetLevel(), result);

	char buf[200] = { 0 };
	if (ok) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[1V1] Bracket %d kaydin onaylandi. Bekleyenler arasindasin!", bid);
	} else {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[1V1] Hata: %s", result.c_str());
	}
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);
	return true;
}
