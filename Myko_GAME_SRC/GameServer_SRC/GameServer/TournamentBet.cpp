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

// Bahis penceresi kapandiginda tekrar duyuru engellemek icin (S115 sabah RUSH)
static std::set<uint8> g_betCloseAnnounced;

// Periyodik status duyuru zamani (zoneID -> son duyuru UNIXTIME)
static std::map<uint8, time_t> g_betLastStatusBroadcast;

// Bet limit: bir oyuncu tek tournament icin max 5M Noah
static const uint32 BET_MAX_PER_USER = 5000000;
static const uint32 BET_MIN_AMOUNT   = 10000;     // min 10k Noah
static const time_t BET_LOCK_AFTER_START_SEC = 120; // tournament basla + 2dk
static const time_t BET_STATUS_BROADCAST_SEC = 30;  // her 30sn status duyuru (acik iken)

// Tournament basladiginde bet kayit alani aç + server-wide duyuru
void OpenTournamentBets(uint8 zoneID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	g_activeBets[zoneID].clear();
	g_betCloseTime[zoneID] = UNIXTIME + BET_LOCK_AFTER_START_SEC;
	g_betCloseAnnounced.erase(zoneID);
	g_betLastStatusBroadcast[zoneID] = UNIXTIME;  // ilk status 30sn sonra

	// S115 sabah RUSH — Server-wide duyuru "BET ACILDI"
	_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (info != nullptr) {
		CKnights* pRed  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
		CKnights* pBlue = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);
		if (pRed != nullptr && pBlue != nullptr) {
			char buf[300] = {0};
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET ACILDI] Zone %u — %s vs %s | 2 dakika bahis kabul ediliyor (+bet KLAN MIKTAR, min 10K max 5M)",
				zoneID, pRed->GetName().c_str(), pBlue->GetName().c_str());
			std::string msg = buf;
			g_pMain->SendNotice(msg.c_str());
		}
	}
}

// S115 sabah RUSH — her saniye GameEventMainTimer'dan cagrilir
// Bahis penceresi yeni kapanan zone varsa "BET KAPANDI" duyurusu + status
void CheckBetWindowClose()
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	time_t now = UNIXTIME;

	for (auto& kv : g_betCloseTime) {
		uint8 zid = kv.first;
		time_t closeT = kv.second;

		// Penceresi yeni kapandi mi?
		if (now >= closeT && g_betCloseAnnounced.count(zid) == 0) {
			g_betCloseAnnounced.insert(zid);

			_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zid);
			if (info == nullptr) continue;
			CKnights* pRed  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
			CKnights* pBlue = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);
			if (pRed == nullptr || pBlue == nullptr) continue;

			// RED ve BLUE havuz topla + en yuksek bahis
			uint32 redPool = 0, bluePool = 0;
			std::string topBetter, topClan;
			uint32 topAmount = 0;
			for (auto& b : g_activeBets[zid]) {
				if (b.betClanID == info->aTournamentClanNum[0]) redPool += b.betAmount;
				else if (b.betClanID == info->aTournamentClanNum[1]) bluePool += b.betAmount;
				if (b.betAmount > topAmount) {
					topAmount = b.betAmount;
					topBetter = b.betterCharName;
					CKnights* pBC = g_pMain->GetClanPtr(b.betClanID);
					if (pBC) topClan = pBC->GetName();
				}
			}

			char buf[400] = {0};
			if (topAmount > 0) {
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[BET KAPANDI] Zone %u — Mac basliyor! %s havuz: %u Noah | %s havuz: %u Noah | En yuksek: %s -> %s (%u Noah)",
					zid, pRed->GetName().c_str(), redPool, pBlue->GetName().c_str(), bluePool,
					topBetter.c_str(), topClan.c_str(), topAmount);
			} else {
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[BET KAPANDI] Zone %u — Mac basliyor! Bahis konulmadi.", zid);
			}
			std::string msg = buf;
			g_pMain->SendNotice(msg.c_str());
		}

		// Periyodik status duyuru (acik iken her 30sn)
		if (now < closeT) {
			auto lastIt = g_betLastStatusBroadcast.find(zid);
			time_t lastT = (lastIt != g_betLastStatusBroadcast.end()) ? lastIt->second : 0;
			if (now - lastT >= BET_STATUS_BROADCAST_SEC) {
				g_betLastStatusBroadcast[zid] = now;

				_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zid);
				if (info == nullptr) continue;
				CKnights* pRed  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
				CKnights* pBlue = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);
				if (pRed == nullptr || pBlue == nullptr) continue;

				uint32 redPool = 0, bluePool = 0;
				size_t totalBets = g_activeBets[zid].size();
				for (auto& b : g_activeBets[zid]) {
					if (b.betClanID == info->aTournamentClanNum[0]) redPool += b.betAmount;
					else if (b.betClanID == info->aTournamentClanNum[1]) bluePool += b.betAmount;
				}

				time_t remaining = closeT - now;
				char buf[300] = {0};
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[BET STATUS] Zone %u — %s: %u Noah (%zu bahis) | %s: %u Noah | Kapanma %lld sn sonra",
					zid, pRed->GetName().c_str(), redPool, totalBets,
					pBlue->GetName().c_str(), bluePool, (long long)remaining);
				std::string msg = buf;
				g_pMain->SendNotice(msg.c_str());
			}
		}
	}
}

// Tournament bitince — kazanan klan'a bahis koyan oyunculara 2x oder
void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	auto it = g_activeBets.find(zoneID);
	if (it == g_activeBets.end()) return;

	// S115 sabah RUSH — sonuc stat icin payout track
	struct _PAYOUT_STAT { std::string name; int64_t delta; };
	std::vector<_PAYOUT_STAT> winners;  // kazananlar (delta > 0)
	std::vector<_PAYOUT_STAT> losers;   // kaybedenler (delta < 0)
	uint32 totalWinPayout = 0, totalLossAmount = 0;

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
			uint32 payout = bet.betAmount * 2;
			winners.push_back({bet.betterCharName, (int64_t)payout});
			totalWinPayout += payout;

			CUser* pUser = g_pMain->GetUserPtr(bet.betterUserID);
			if (pUser != nullptr && pUser->isInGame())
			{
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
			losers.push_back({bet.betterCharName, -(int64_t)bet.betAmount});
			totalLossAmount += bet.betAmount;

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

	// S115 sabah RUSH — server-wide RESOLVE stat duyuru (kazanan/kaybeden sayilari + top 3)
	if (winnerClanID > 0 && (winners.size() > 0 || losers.size() > 0)) {
		// Sirala
		std::sort(winners.begin(), winners.end(),
			[](const _PAYOUT_STAT& a, const _PAYOUT_STAT& b){ return a.delta > b.delta; });
		std::sort(losers.begin(), losers.end(),
			[](const _PAYOUT_STAT& a, const _PAYOUT_STAT& b){ return a.delta < b.delta; });

		char buf[400] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BET RESOLVE] Zone %u: %zu oyuncu kazandi (+%u Noah toplam), %zu oyuncu kaybetti (-%u Noah)",
			zoneID, winners.size(), totalWinPayout, losers.size(), totalLossAmount);
		std::string msg1 = buf;
		g_pMain->SendNotice(msg1.c_str());

		// Top 3 kazanan
		if (!winners.empty()) {
			std::string msgTop = "[BET TOP] En cok kazanan: ";
			size_t cnt = (winners.size() < 3) ? winners.size() : 3;
			for (size_t i = 0; i < cnt; i++) {
				char tbuf[120] = {0};
				_snprintf_s(tbuf, sizeof(tbuf), _TRUNCATE,
					"%zu) %s (+%lld) ", i+1, winners[i].name.c_str(), (long long)winners[i].delta);
				msgTop += tbuf;
			}
			g_pMain->SendNotice(msgTop.c_str());
		}
	} else if (winnerClanID == 0 && it->second.size() > 0) {
		char buf[200] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BET REFUND] Zone %u: Tournament beraberlik, %zu bahis iade edildi.",
			zoneID, it->second.size());
		std::string msg = buf;
		g_pMain->SendNotice(msg.c_str());
	}

	// Cleanup
	g_activeBets.erase(it);
	g_betCloseTime.erase(zoneID);
	g_betCloseAnnounced.erase(zoneID);
	g_betLastStatusBroadcast.erase(zoneID);

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

// =====================================================================
// S115 sabah RUSH — +betstatus (oyuncu mevcut aktif bahis durumunu gor)
// =====================================================================
COMMAND_HANDLER(CUser::HandleBetStatusCommand)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);

	if (g_activeBets.empty()) {
		std::string msg = "[BET] Su an aktif bahis penceresi yok.";
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		Send(&pkt);
		return true;
	}

	for (auto& kv : g_activeBets) {
		uint8 zid = kv.first;
		_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zid);
		if (info == nullptr) continue;
		CKnights* pRed  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
		CKnights* pBlue = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);
		if (pRed == nullptr || pBlue == nullptr) continue;

		uint32 redPool = 0, bluePool = 0;
		std::string topName, topClanName;
		uint32 topAmt = 0;
		uint32 myTotal = 0;       // kullanicinin kendi bahis toplami
		std::string myClanName;
		for (auto& b : kv.second) {
			if (b.betClanID == info->aTournamentClanNum[0]) redPool += b.betAmount;
			else if (b.betClanID == info->aTournamentClanNum[1]) bluePool += b.betAmount;
			if (b.betAmount > topAmt) {
				topAmt = b.betAmount;
				topName = b.betterCharName;
				CKnights* pBC = g_pMain->GetClanPtr(b.betClanID);
				if (pBC) topClanName = pBC->GetName();
			}
			if (b.betterUserID == GetID()) {
				myTotal += b.betAmount;
				CKnights* pBC = g_pMain->GetClanPtr(b.betClanID);
				if (pBC && myClanName.empty()) myClanName = pBC->GetName();
			}
		}

		// Bahis kapanma kalan sure
		time_t closeT = 0;
		auto closeIt = g_betCloseTime.find(zid);
		if (closeIt != g_betCloseTime.end()) closeT = closeIt->second;
		time_t remaining = (closeT > UNIXTIME) ? (closeT - UNIXTIME) : 0;

		char buf[400] = {0};

		// Satir 1: Genel
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BET %u] %s: %u Noah (%zu bahis) | %s: %u Noah",
			zid, pRed->GetName().c_str(), redPool, kv.second.size(),
			pBlue->GetName().c_str(), bluePool);
		std::string m1 = buf;
		{ Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m1); Send(&p); }

		// Satir 2: En yuksek bahis
		if (topAmt > 0) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"  En yuksek: %s -> %s (%u Noah)", topName.c_str(), topClanName.c_str(), topAmt);
			std::string m2 = buf;
			Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m2); Send(&p);
		}

		// Satir 3: Kullanicinin kendi bahisi
		if (myTotal > 0) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"  Senin bahisin: %u Noah (%s)", myTotal, myClanName.c_str());
			std::string m3 = buf;
			Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m3); Send(&p);
		}

		// Satir 4: Kapanma sure
		if (remaining > 0) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"  Kapanmasina: %lld sn", (long long)remaining);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "  BAHIS KAPALI (mac devam ediyor)");
		}
		std::string m4 = buf;
		Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m4); Send(&p);
	}

	return true;
}

// =====================================================================
// S115 sabah RUSH — /betstatus ZONE (GM console)
// =====================================================================
COMMAND_HANDLER(CGameServerDlg::HandleBetStatusConsole)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);

	printf("====== TOURNAMENT BET STATUS ======\n");
	if (g_activeBets.empty()) {
		printf("  Aktif bahis penceresi YOK\n");
		printf("===================================\n");
		return true;
	}

	for (auto& kv : g_activeBets) {
		uint8 zid = kv.first;
		_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zid);
		if (info == nullptr) {
			printf("  Zone %u — TOURNAMENT_DATA YOK (orphan bet?)\n", zid);
			continue;
		}
		CKnights* pRed  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
		CKnights* pBlue = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);
		const char* redName  = pRed ? pRed->GetName().c_str()  : "?";
		const char* blueName = pBlue ? pBlue->GetName().c_str() : "?";

		uint32 redPool = 0, bluePool = 0;
		for (auto& b : kv.second) {
			if (b.betClanID == info->aTournamentClanNum[0]) redPool += b.betAmount;
			else if (b.betClanID == info->aTournamentClanNum[1]) bluePool += b.betAmount;
		}

		time_t closeT = 0;
		auto closeIt = g_betCloseTime.find(zid);
		if (closeIt != g_betCloseTime.end()) closeT = closeIt->second;
		time_t remaining = (closeT > UNIXTIME) ? (closeT - UNIXTIME) : 0;
		const char* status = (remaining > 0) ? "ACIK" : "KAPALI";

		printf("  Zone %u [%s, kalan=%llds]\n", zid, status, (long long)remaining);
		printf("    %s: %u Noah (%zu bahis)\n", redName, redPool, kv.second.size());
		printf("    %s: %u Noah\n", blueName, bluePool);

		// Detay: tum bahisleri sirala
		for (auto& b : kv.second) {
			CKnights* pBC = g_pMain->GetClanPtr(b.betClanID);
			printf("      %s -> %s : %u\n",
				b.betterCharName.c_str(),
				pBC ? pBC->GetName().c_str() : "?",
				b.betAmount);
		}
	}
	printf("===================================\n");
	return true;
}
