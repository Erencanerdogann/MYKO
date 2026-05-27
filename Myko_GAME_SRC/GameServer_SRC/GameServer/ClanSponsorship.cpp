// =========================================================================
// S115 FAZ 15 — Klan Sponsorlugu
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// Zengin oyuncular klan icin Noah/NP destek (sponsorluk).
// MATRIX SP'leri (migration 111): SP_CLAN_SPONSOR_DEPOSIT (OUT NewSponsorID)
//
// Komutlar (chat, klan uyesi):
//   +klansponsor       <miktar>   — Noah ile klan kasasina katki
//   +klansponsornp     <miktar>   — NP ile klan kasasina katki
//
// WEBRA "Lider Tablosu" sayfasi DB'den okur (acilis sonrasi).
//
// PG temiz: WIZ_CHAT, yeni opcode YOK.
// =========================================================================

#include "stdafx.h"

const int64_t SPONSOR_MIN_NOAH = 100000;      // 100K Noah min
const int64_t SPONSOR_MAX_NOAH = 500000000;   // 500M Noah max (tek seferde)
const int32_t SPONSOR_MIN_NP = 100;
const int32_t SPONSOR_MAX_NP = 50000;

// +klansponsor MIKTAR (Noah)
COMMAND_HANDLER(CUser::HandleKlanSponsorCommand)
{
	if (!isInClan()) {
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin.");
		return true;
	}
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this,
			"+klansponsor MIKTAR (Noah). Min 100K, max 500M. Ornek: +klansponsor 5000000");
		return true;
	}

	int64_t amount = (int64_t)atoll(vargs.front().c_str());
	if (amount < 0) amount = 0;
	if (amount < SPONSOR_MIN_NOAH) {
		g_pMain->SendHelpDescription(this, "Min 100K Noah sponsor olabilirsin.");
		return true;
	}
	if (amount > SPONSOR_MAX_NOAH) {
		g_pMain->SendHelpDescription(this, "Tek seferde max 500M Noah sponsor olabilirsin.");
		return true;
	}

	if ((int64_t)m_iGold < amount) {
		g_pMain->SendHelpDescription(this, "Yeterli Noah'in yok.");
		return true;
	}

	// Para cek
	if (!GoldLose((uint32)amount, true)) {
		g_pMain->SendHelpDescription(this, "Para cekilemedi, tekrar dene.");
		return true;
	}

	// DB INSERT (MATRIX migration 111)
	int32_t sponsorID = g_DBAgent.ClanSponsorDeposit(GetClanID(), GetID(), GetName(),
		amount, 0);
	if (sponsorID == 0) {
		// DB hata — para iade
		GoldGain((uint32)amount);
		g_pMain->SendHelpDescription(this, "DB hatasi, para iade edildi.");
		return true;
	}

	char buf[200] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[KLAN SPONSOR] Klanina %lld Noah katki yaptin (ID=%d). Tebrikler!",
		amount, sponsorID);
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);

	return true;
}

// +klansponsornp MIKTAR (National Points)
COMMAND_HANDLER(CUser::HandleKlanSponsorNPCommand)
{
	if (!isInClan()) {
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin.");
		return true;
	}
	if (vargs.empty()) {
		g_pMain->SendHelpDescription(this,
			"+klansponsornp MIKTAR (NP). Min 100, max 50K. Ornek: +klansponsornp 1000");
		return true;
	}

	int32_t amount = (int32_t)SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);
	if (amount < SPONSOR_MIN_NP || amount > SPONSOR_MAX_NP) {
		g_pMain->SendHelpDescription(this, "NP miktar: min 100, max 50000.");
		return true;
	}

	if (m_iLoyalty < (uint32)amount) {
		g_pMain->SendHelpDescription(this, "Yeterli NP'in yok.");
		return true;
	}

	// NP cek (- yon)
	SendLoyaltyChange("sponsor", -amount, false, false, false);

	int32_t sponsorID = g_DBAgent.ClanSponsorDeposit(GetClanID(), GetID(), GetName(),
		0, amount);
	if (sponsorID == 0) {
		SendLoyaltyChange("sponsor_refund", amount, false, false, false);
		g_pMain->SendHelpDescription(this, "DB hatasi, NP iade edildi.");
		return true;
	}

	char buf[200] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[KLAN SPONSOR] Klanina %d NP katki yaptin (ID=%d). Tebrikler!",
		amount, sponsorID);
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);

	return true;
}
