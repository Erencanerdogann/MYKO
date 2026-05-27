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

	// S115 TUR 11 DB entegrasyon — MATRIX MSG:5907 (SP_TOURNAMENT_REG_INSERT)
	// leaderAccountID bos string (chat komutdan veri yok, NPC ise dolu olabilir)
	g_DBAgent.TournamentRegInsert(clanID, clanName, leaderName, "", via);

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

			// S115 TUR 11 DB entegrasyon — MATRIX MSG:5907 (SP_TOURNAMENT_REG_CANCEL)
			g_DBAgent.TournamentRegCancel(clanID);

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

// =====================================================================
// S115 — Tournament Manager NPC Lua dialog (32756) icin 5 method
// =====================================================================
// Donus kodlari (1xx = basari, 2xx = soft hata, 4xx = klan/lider sorun, 5xx = sistem hata)

int CUser::LuaTournamentRegister()
{
	if (!isInClan()) return 4;        // 4 = klan yok
	if (!isClanLeader()) return 3;    // 3 = lider degil
	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr) return 4;

	if (RegisterClanForTournament(pClan->GetID(), pClan->GetName(),
	                              GetName(), "NPC_DIALOG")) {
		// Server-wide duyuru
		char buf[200] = { 0 };
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[TOURNAMENT REG] %s klani turnuvaya kayit oldu (NPC). Toplam: %zu klan.",
			pClan->GetName().c_str(), GetTournamentRegistrations().size());
		std::string msg = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		g_pMain->Send_All(&pkt);
		return 1;  // basari
	}
	return 2;  // zaten kayitli
}

int CUser::LuaTournamentCancelReg()
{
	if (!isInClan() || !isClanLeader()) return 0;
	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr) return 0;
	return UnregisterClanForTournament(pClan->GetID()) ? 1 : 0;
}

int CUser::LuaBracketActiveCount()
{
	// Bos slot saymak icin DBAgent.BracketLoadActive cagri
	std::vector<CDBAgent::_BRACKET_INFO_ROW> rows;
	if (!g_DBAgent.BracketLoadActive(rows)) return 0;
	// Sadece REGISTRATION durumda olanlar (kayit acik) — ACTIVE basladi, kayit kapali
	int count = 0;
	for (auto& r : rows) {
		if (r.status == "REGISTRATION") count++;
		if (count >= 4) break;  // Lua dialog limiti
	}
	return count;
}

int CUser::LuaBracketRegisterByIndex(int idx)
{
	if (!isInClan() || !isClanLeader()) return 0;
	if (idx < 1 || idx > 4) return 0;

	CKnights* pClan = g_pMain->GetClanPtr(GetClanID());
	if (pClan == nullptr) return 0;

	// Aktif REGISTRATION bracket'lardan idx-1'inci olani al
	std::vector<CDBAgent::_BRACKET_INFO_ROW> rows;
	if (!g_DBAgent.BracketLoadActive(rows)) return 0;

	int32_t targetBracketID = 0;
	int count = 0;
	for (auto& r : rows) {
		if (r.status != "REGISTRATION") continue;
		count++;
		if (count == idx) {
			targetBracketID = r.bracketID;
			break;
		}
	}
	if (targetBracketID == 0) return 0;

	// DB SP_BRACKET_REGISTER cagri (BracketTournament.cpp:197 mantigi)
	std::string result;
	bool ok = g_DBAgent.BracketRegister(targetBracketID, pClan->GetID(),
		pClan->GetName(), GetName(), result);

	char buf[200] = { 0 };
	if (ok) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BRACKET REG] %s klani Bracket %d'e kayit oldu (NPC).",
			pClan->GetName().c_str(), targetBracketID);
	} else {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BRACKET REG] Hata: %s", result.c_str());
	}
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);

	return ok ? 1 : 0;
}

int CUser::LuaBracket8v8ListLatest()
{
	if (!isInClan()) return 0;

	// En son REGISTRATION durumdaki bracket
	std::vector<CDBAgent::_BRACKET_INFO_ROW> rows;
	if (!g_DBAgent.BracketLoadActive(rows)) return 0;

	int32_t targetBracketID = 0;
	for (auto& r : rows) {
		if (r.status == "REGISTRATION") {
			targetBracketID = r.bracketID;
			// son aktif olani al (siralama BracketID DESC olmuyorsa son row)
		}
	}
	if (targetBracketID == 0) {
		// Bracket yoksa kullaniciya bilgi
		std::string msg = "[8v8] Su an aktif bracket turnuvasi yok.";
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		Send(&pkt);
		return 0;
	}

	// Liste cek + chat'e yaz (Bracket8v8.cpp HandleBracket8v8ListCommand mantigi)
	std::vector<CDBAgent::_BPM_LIST_ROW> bpmRows;
	if (!g_DBAgent.BracketPartyMemberList(targetBracketID, GetClanID(), bpmRows)) {
		std::string msg = "[8v8] DB hata — liste cekilemedi.";
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		Send(&pkt);
		return 0;
	}

	char buf[256] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[8v8] Bracket %d — atanmis uyeler (klan %u, toplam %zu):",
		targetBracketID, GetClanID(), bpmRows.size());
	std::string header = buf;
	{
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &header);
		Send(&pkt);
	}

	if (bpmRows.empty()) {
		std::string msg = "  (henuz kimse eklenmemis. Lider +bracket8v8add ile ekler.)";
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		Send(&pkt);
		return 1;
	}

	const size_t MAX = 32;
	size_t count = (bpmRows.size() < MAX) ? bpmRows.size() : MAX;
	for (size_t i = 0; i < count; i++) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"  Party%u: %s (%s)",
			bpmRows[i].partyNumber, bpmRows[i].memberCharName.c_str(),
			bpmRows[i].assignedBy.c_str());
		std::string line = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &line);
		Send(&pkt);
	}
	return 1;
}
