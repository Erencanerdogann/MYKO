// =========================================================================
// S115 TUR 8 — Tournament Spectator Bet
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// PATRON karari: "kendı clanına da bahıs yapar adam belkı karsıya oynıcak"
// Mantik:
//   - Tournament aktif iken oyuncu +bet ClanAdı Miktar komutuyla bahis koyar
//   - Bahis suresi: tournament basladiktan 2 dakika sonra kapanir (manipulasyon)
//   - Tournament bitince kazanan klan'a bahis koyan oyuncular 2x kazanir
// PG temiz: sade chat command, yeni opcode YOK
// =========================================================================

#include "stdafx.h"

// RAM-level bet kaydı (DB'ye ek olarak — quick lookup icin)
// MATRIX brief'i: _MK_TOURNAMENT_BETS tablo + 4 SP
struct _TOURNAMENT_BET {
	uint16 betterUserID;          // bahis koyan user
	std::string betterCharName;   // cache (kazanan duyuru icin)
	uint8 zoneID;                 // tournament zone
	uint16 betClanID;             // bahis koyulan klan
	uint32 betAmount;             // Noah miktari
	bool resolved;                // tournament biti, odeme yapıldı mi
};

// Aktif bahisler (key = zoneID, value = bet listesi)
static std::map<uint8, std::vector<_TOURNAMENT_BET>> g_activeBets;
static std::recursive_mutex g_betLock;

// Bahis kapanma zamani (zoneID -> UNIXTIME)
// Tournament basladiktan 2dk (120sn) sonra bahis kapanir
static std::map<uint8, time_t> g_betCloseTime;

// Bet limit: bir oyuncu tek tournament icin max 5M Noah
static const uint32 BET_MAX_PER_USER = 5000000;
static const uint32 BET_MIN_AMOUNT   = 10000;     // min 10k Noah
static const time_t BET_LOCK_AFTER_START_SEC = 120; // tournament basla + 2dk

// Tournament basladiginde bet kayit alani aç
void OpenTournamentBets(uint8 zoneID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	g_activeBets[zoneID].clear();
	g_betCloseTime[zoneID] = UNIXTIME + BET_LOCK_AFTER_START_SEC;
}

// Tournament bitince — kazanan klan'a bahis koyan oyunculara 2x oder
void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	auto it = g_activeBets.find(zoneID);
	if (it == g_activeBets.end()) return;

	for (auto& bet : it->second)
	{
		if (bet.resolved) continue;
		bet.resolved = true;

		if (winnerClanID == 0) // berabere — havuz iade
		{
			CUser* pUser = g_pMain->GetUserPtr(bet.betterUserID);
			if (pUser != nullptr && pUser->isInGame())
			{
				pUser->GoldGain(bet.betAmount); // iade
				std::string msg = "[BET] Tournament berabere bitti, bahisin iade edildi.";
				Packet pkt;
				ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
				pUser->Send(&pkt);
			}
		}
		else if (bet.betClanID == winnerClanID) // kazandi
		{
			CUser* pUser = g_pMain->GetUserPtr(bet.betterUserID);
			if (pUser != nullptr && pUser->isInGame())
			{
				uint32 payout = bet.betAmount * 2;
				pUser->GoldGain(payout);

				char buf[200] = { 0 };
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[BET] Bahis kazandin! +%u Noah aldin.", payout);
				std::string msg = buf;
				Packet pkt;
				ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
				pUser->Send(&pkt);
			}
		}
		else // kaybetti
		{
			CUser* pUser = g_pMain->GetUserPtr(bet.betterUserID);
			if (pUser != nullptr && pUser->isInGame())
			{
				std::string msg = "[BET] Bahis kaybettin. Bir dahaki tournament'a sans bol :)";
				Packet pkt;
				ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
				pUser->Send(&pkt);
			}
		}
	}

	// Cleanup
	g_activeBets.erase(it);
	g_betCloseTime.erase(zoneID);

	// S115 TUR 8 DB entegrasyon — MATRIX MSG:5907 (SP_TOURNAMENT_BET_RESOLVE veya REFUND)
	if (winnerClanID == 0)
		g_DBAgent.TournamentBetRefund(zoneID);   // berabere -> iade
	else
		g_DBAgent.TournamentBetResolve(zoneID, winnerClanID);  // kazanan belli -> dagit
}

// +bet ClanAdı Miktar
COMMAND_HANDLER(CUser::HandleTournamentBetCommand)
{
	if (vargs.size() < 2)
	{
		g_pMain->SendHelpDescription(this,
			"Tournament bahis: +bet <KlanAdi> <Noah>. Ornek: +bet ILKCLAN 100000");
		return true;
	}

	std::string targetClanName = vargs.front(); vargs.pop_front();
	uint32 amount = (uint32)SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	if (amount < BET_MIN_AMOUNT)
	{
		char buf[100];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"Minimum bahis %u Noah", BET_MIN_AMOUNT);
		g_pMain->SendHelpDescription(this, buf);
		return true;
	}

	if (amount > BET_MAX_PER_USER)
	{
		char buf[100];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"Maximum bahis %u Noah (tek tournament)", BET_MAX_PER_USER);
		g_pMain->SendHelpDescription(this, buf);
		return true;
	}

	if (m_iGold < amount)
	{
		g_pMain->SendHelpDescription(this, "Yeterli Noah'in yok.");
		return true;
	}

	// Aktif tournament var mi? Hangi zone'da?
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	uint8 targetZoneID = 0;
	uint16 targetClanID = 0;
	_TOURNAMENT_DATA* targetInfo = nullptr;

	uint8 candidateZones[] = { 77, 78, 96, 97, 98, 99 };
	for (uint8 zid : candidateZones)
	{
		_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zid);
		if (info == nullptr || !info->aTournamentisStarted) continue;

		// Klan adi karsi var mi bu tournament'ta?
		CKnights* pRedClan  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
		CKnights* pBlueClan = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);

		if (pRedClan && pRedClan->GetName() == targetClanName) {
			targetZoneID = zid; targetInfo = info; targetClanID = pRedClan->GetID(); break;
		}
		if (pBlueClan && pBlueClan->GetName() == targetClanName) {
			targetZoneID = zid; targetInfo = info; targetClanID = pBlueClan->GetID(); break;
		}
	}

	if (targetInfo == nullptr || targetClanID == 0)
	{
		g_pMain->SendHelpDescription(this,
			"Bu klan icin aktif tournament bulunamadi. Klan adini kontrol et.");
		return true;
	}

	// Bahis kapanma zamani gecti mi?
	auto closeIt = g_betCloseTime.find(targetZoneID);
	if (closeIt != g_betCloseTime.end() && UNIXTIME > closeIt->second)
	{
		g_pMain->SendHelpDescription(this,
			"Bu tournament icin bahisler kapali (2dk gecti).");
		return true;
	}

	// Ayni oyuncunun ayni tournament icin onceki bahisleri toplami + yeni
	uint32 totalForThisUser = amount;
	for (auto& b : g_activeBets[targetZoneID])
	{
		if (b.betterUserID == GetID())
			totalForThisUser += b.betAmount;
	}
	if (totalForThisUser > BET_MAX_PER_USER)
	{
		g_pMain->SendHelpDescription(this,
			"Bu tournament icin toplam bahis limitin doldu (5M Noah).");
		return true;
	}

	// Para cek
	if (!GoldLose(amount, true))
	{
		g_pMain->SendHelpDescription(this, "Para cekilemedi, tekrar dene.");
		return true;
	}

	// Bahis kaydet (RAM)
	_TOURNAMENT_BET bet;
	bet.betterUserID  = GetID();
	bet.betterCharName = GetName();
	bet.zoneID        = targetZoneID;
	bet.betClanID     = targetClanID;
	bet.betAmount     = amount;
	bet.resolved      = false;
	g_activeBets[targetZoneID].push_back(bet);

	// S115 TUR 8 DB entegrasyon — MATRIX MSG:5907 (SP_TOURNAMENT_BET_PLACE)
	g_DBAgent.TournamentBetPlace(targetZoneID, GetAccountName(), GetName(),
	                              targetClanID, targetClanName, (int32_t)amount);

	char buf[200] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[BET] %s'a %u Noah bahis koyuldu. Kazanirsa 2x odeme!",
		targetClanName.c_str(), amount);
	std::string msg = buf;
	Packet pkt;
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send(&pkt);

	return true;
}
