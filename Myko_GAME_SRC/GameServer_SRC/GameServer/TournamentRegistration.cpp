// =========================================================================
// S115 TUR 11 — Tournament Klan Kayit Sistemi
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// PATRON karari: "hem web sitesinde kayıt hemde NPC kayıt iki sistemde
//                 birbirini takip edebilecek"
// Yontem 1 (CHIP): Chat komutu +tournamentreg (klan lideri kullanir)
// Yontem 2 (Acilis sonrasi): NPC dialog (standart KO NPC talk)
// Yontem 3 (WEBRA): Site formu
// HEPSI ayni DB tablosuna yazıyor: _MK_TOURNAMENT_REGISTRATION (MATRIX brief)
//
// Su an RAM-only, MATRIX SP'leri gelince DB persistence eklenecek.
// PG temiz: chat komut, opcode YOK.
// =========================================================================

#include "stdafx.h"

struct _TOURNAMENT_REGISTRATION {
	uint16 clanID;
	std::string clanName;
	std::string leaderName;
	time_t registeredAt;
	std::string registeredVia; // "NPC", "WEB", "CHAT"
};

static std::vector<_TOURNAMENT_REGISTRATION> g_registrations;
static std::recursive_mutex g_regLock;

// Yeni kayit ekle (RAM + DB)
bool RegisterClanForTournament(uint16 clanID, const std::string& clanName,
                                const std::string& leaderName, const std::string& via)
{
	std::lock_guard<std::recursive_mutex> lock(g_regLock);

	// Zaten kayit mi?
	for (auto& r : g_registrations)
	{
		if (r.clanID == clanID) return false; // duplicate
	}

	_TOURNAMENT_REGISTRATION reg;
	reg.clanID        = clanID;
	reg.clanName      = clanName;
	reg.leaderName    = leaderName;
	reg.registeredAt  = UNIXTIME;
	reg.registeredVia = via;
	g_registrations.push_back(reg);

	// TODO: MATRIX SP_TOURNAMENT_REG_INSERT cagri (acilis sonrasi)
	// CDBAgent::TournamentRegInsert(clanID, clanName, leaderName, "", via, nullptr);

	printf("[TOURNAMENT_REG] Clan registered: %s (via %s)\n", clanName.c_str(), via.c_str());
	return true;
}

// Kayit iptal
bool UnregisterClanForTournament(uint16 clanID)
{
	std::lock_guard<std::recursive_mutex> lock(g_regLock);
	for (auto it = g_registrations.begin(); it != g_registrations.end(); ++it)
	{
		if (it->clanID == clanID)
		{
			g_registrations.erase(it);

			// TODO: MATRIX SP_TOURNAMENT_REG_CANCEL cagri
			return true;
		}
	}
	return false;
}

// Kayitli klanlar listesini al (GM panel + WEB ortak)
std::vector<_TOURNAMENT_REGISTRATION> GetTournamentRegistrations()
{
	std::lock_guard<std::recursive_mutex> lock(g_regLock);
	return g_registrations; // copy
}

// +tournamentreg komutu (klan lideri kullanir)
COMMAND_HANDLER(CUser::HandleTournamentRegCommand)
{
	if (!isInClan())
	{
		g_pMain->SendHelpDescription(this, "Klan uyesi olmalisin.");
		return true;
	}

	if (!isClanLeader())
	{
		g_pMain->SendHelpDescription(this, "Sadece klan lideri kayit yapabilir.");
		return true;
	}

	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Klan bulunamadi.");
		return true;
	}

	// Iptal mi? `+tournamentreg cancel`
	if (!vargs.empty())
	{
		std::string arg = vargs.front();
		if (arg == "cancel" || arg == "iptal")
		{
			if (UnregisterClanForTournament(pClan->GetID()))
				g_pMain->SendHelpDescription(this, "Klan tournament kaydi iptal edildi.");
			else
				g_pMain->SendHelpDescription(this, "Aktif kaydin yok.");
			return true;
		}
	}

	// Yeni kayit
	if (RegisterClanForTournament(pClan->GetID(), pClan->GetName(),
	                              GetName(), "CHAT"))
	{
		char buf[200] = { 0 };
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[TOURNAMENT REG] %s klani turnuvaya kayit oldu! (Toplam: %zu klan)",
			pClan->GetName().c_str(), g_registrations.size());
		std::string msg = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		g_pMain->Send_All(&pkt);
	}
	else
	{
		g_pMain->SendHelpDescription(this, "Klanin zaten kayitli.");
	}

	return true;
}

// GM komutu: /tournamentreglist — kayitli klanlari listele
COMMAND_HANDLER(CGameServerDlg::HandleTournamentRegListCommand)
{
	auto regs = GetTournamentRegistrations();
	printf("====== TOURNAMENT REGISTRATIONS (%zu) ======\n", regs.size());
	for (auto& r : regs)
	{
		printf("  Clan(%u) %-21s | Leader: %-21s | Via: %s\n",
			r.clanID, r.clanName.c_str(), r.leaderName.c_str(), r.registeredVia.c_str());
	}
	printf("===========================================\n");
	return true;
}
