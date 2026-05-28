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

// Bet config — GM komutlari ile ayarlanabilir (S115 ekonomi fix)
// Default degerler; /betlimits /betwindow /betcommission ile degisir
static uint32 g_betMaxPerUser = 100000000;     // bir oyuncu tek tournament max TUTAR (100M default)
static uint32 g_betMinAmount  = 10000;         // min bahis (10K)
static uint8  g_betMaxCountPerUser = 10;       // bir oyuncu tek tournament max BAHIS SAYISI (spam onleme)
static time_t g_betWindowSec  = 120;           // tournament basla + bahis penceresi
static uint8  g_betCommissionPct = 10;         // KOMISYON %10 (sink — oyundan eritilir)
static bool   g_betPreventClanBetraysal = true;// ayni klan rakip tarafa bahis koyamaz
// NOT: GM /betlimits ile degistirebilir. Mutlak ust sinir COIN_MAX (2.1 milyar) — 1GB Noah'a kadar cikar.

static const time_t BET_STATUS_BROADCAST_SEC = 30;  // her 30sn status duyuru (sabit)

// GM komutlarindan erisim icin setter'lar (HandleBet*Command'larda kullanilir)
void SetBetLimits(uint32 minAmt, uint32 maxAmt) { g_betMinAmount = minAmt; g_betMaxPerUser = maxAmt; }
void SetBetWindow(time_t sec) { g_betWindowSec = sec; }
void SetBetCommission(uint8 pct) { g_betCommissionPct = pct; }
void GetBetConfig(uint32& minAmt, uint32& maxAmt, time_t& win, uint8& comm) {
	minAmt = g_betMinAmount; maxAmt = g_betMaxPerUser; win = g_betWindowSec; comm = g_betCommissionPct;
}

// Tournament basladiginde bet kayit alani aç + server-wide duyuru
void OpenTournamentBets(uint8 zoneID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	g_activeBets[zoneID].clear();
	g_betCloseTime[zoneID] = UNIXTIME + g_betWindowSec;
	g_betCloseAnnounced.erase(zoneID);
	g_betLastStatusBroadcast[zoneID] = UNIXTIME;  // ilk status 30sn sonra

	// S115 sabah RUSH — Server-wide duyuru "BET ACILDI"
	_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (info != nullptr) {
		CKnights* pRed  = g_pMain->GetClanPtr(info->aTournamentClanNum[0]);
		CKnights* pBlue = g_pMain->GetClanPtr(info->aTournamentClanNum[1]);
		if (pRed != nullptr && pBlue != nullptr) {
			char buf[360] = {0};
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BAHIS ACILDI / BETTING OPEN] Zone %u — %s vs %s | %llds | +bet KLAN MIKTAR (min %u max %u, komisyon/fee %%%u)",
				zoneID, pRed->GetName().c_str(), pBlue->GetName().c_str(),
				(long long)g_betWindowSec, g_betMinAmount, g_betMaxPerUser, g_betCommissionPct);
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
					"[BAHIS KAPANDI / BETTING CLOSED] Zone %u — Mac basliyor! %s: %u | %s: %u | En yuksek/Top: %s -> %s (%u)",
					zid, pRed->GetName().c_str(), redPool, pBlue->GetName().c_str(), bluePool,
					topBetter.c_str(), topClan.c_str(), topAmount);
			} else {
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[BAHIS KAPANDI / BETTING CLOSED] Zone %u — Mac basliyor! Bahis yok / No bets.", zid);
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
					"[BAHIS / BET] Zone %u — %s: %u (%zu) | %s: %u | Kapanma/Closes: %lld sn",
					zid, pRed->GetName().c_str(), redPool, totalBets,
					pBlue->GetName().c_str(), bluePool, (long long)remaining);
				std::string msg = buf;
				g_pMain->SendNotice(msg.c_str());
			}
		}
	}
}

// HELPER — tum bahisleri iade et (rakipsiz / beraberlik / iptal)
static void RefundAllBets(uint8 zoneID, std::vector<_TOURNAMENT_BET>& bets, const char* reason)
{
	for (auto& bet : bets) {
		if (bet.resolved) continue;
		bet.resolved = true;
		// Socket ID yeniden kullanim guard: isim teyit
		CUser* pUser = g_pMain->GetUserPtr(bet.betterUserID);
		if (pUser != nullptr && pUser->GetName() != bet.betterCharName) pUser = nullptr;
		if (pUser != nullptr && pUser->isInGame()) {
			pUser->GoldGain(bet.betAmount);  // iade (komisyon kesilmez)
			char buf[260] = {0};
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET] %s — %u Noah iade edildi. | Refunded %u Noah.",
				reason, bet.betAmount, bet.betAmount);
			std::string msg = buf;
			Packet pkt;
			ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
			pUser->Send(&pkt);
		}
	}
}

// GM/Web — bahis penceresini manuel KAPAT (mac devam eder, yeni bahis alinmaz)
// closeTime'i simdiye ceker, CheckBetWindowClose duyuruyu yapar
void ForceCloseBetWindow(uint8 zoneID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	auto it = g_betCloseTime.find(zoneID);
	if (it == g_betCloseTime.end()) return;
	it->second = UNIXTIME;  // simdi kapat → CheckBetWindowClose "[BET KAPANDI]" duyurusu yapar
}

// GM/Web — bahis penceresini manuel AC (tournament aktifse, pencere yeniden acilir)
void ForceOpenBetWindow(uint8 zoneID)
{
	_TOURNAMENT_DATA* info = g_pMain->m_ClanVsDataList.GetData(zoneID);
	if (info == nullptr || !info->aTournamentisStarted) return;
	OpenTournamentBets(zoneID);  // pencere + duyuru
}

// GM /betcancel — aktif bahisleri iptal + tam iade (komisyon kesilmez)
void CancelTournamentBets(uint8 zoneID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	auto it = g_activeBets.find(zoneID);
	if (it == g_activeBets.end()) return;

	size_t cnt = it->second.size();
	RefundAllBets(zoneID, it->second, "GM bahisleri iptal etti");

	g_activeBets.erase(it);
	g_betCloseTime.erase(zoneID);
	g_betCloseAnnounced.erase(zoneID);
	g_betLastStatusBroadcast.erase(zoneID);

	if (cnt > 0) {
		char buf[200] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BET IPTAL] Zone %u: GM bahisleri iptal etti, %zu bahis iade edildi.", zoneID, cnt);
		std::string msg = buf;
		g_pMain->SendNotice(msg.c_str());
	}

	g_DBAgent.TournamentBetRefund(zoneID);
}

// Tournament bitince — PARIMUTUEL dagitim (havuz orani + komisyon sink)
// MATEMATIK:
//   Toplam = RED havuz + BLUE havuz
//   Komisyon = Toplam * g_betCommissionPct / 100  (SINK — yakilir, oyundan eritir)
//   Dagitilacak = Toplam - Komisyon
//   Kazanan bahisci = (kendi bahsi / kazanan_havuz) * Dagitilacak
// Server PARA BASMAZ — kaybedenlerin parasi kazananlara, komisyon ekonomiden silinir.
void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID)
{
	std::lock_guard<std::recursive_mutex> lock(g_betLock);
	auto it = g_activeBets.find(zoneID);
	if (it == g_activeBets.end()) return;

	auto& bets = it->second;

	// Beraberlik → tum iade (komisyon kesilmez)
	if (winnerClanID == 0) {
		size_t cnt = bets.size();
		RefundAllBets(zoneID, bets, "Tournament beraberlik");
		if (cnt > 0) {
			char buf[200] = {0};
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET REFUND] Zone %u: Beraberlik, %zu bahis iade edildi (komisyon kesilmedi).",
				zoneID, cnt);
			std::string msg = buf;
			g_pMain->SendNotice(msg.c_str());
		}
		goto cleanup;
	}

	// Havuzlari topla (uint64 — overflow guard: 5M * 1000 kisi = 5 milyar > uint32)
	{
		uint64 winnerPool = 0, loserPool = 0;
		for (auto& bet : bets) {
			if (bet.betClanID == winnerClanID) winnerPool += bet.betAmount;
			else loserPool += bet.betAmount;
		}

		// Tek tarafa bahis (rakipsiz) → tum iade (oran hesaplanamaz)
		if (winnerPool == 0 || loserPool == 0) {
			size_t cnt = bets.size();
			RefundAllBets(zoneID, bets, "Tek tarafa bahis (rakipsiz)");
			if (cnt > 0) {
				char buf[200] = {0};
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"[BET REFUND] Zone %u: Rakipsiz bahis, %zu bahis iade edildi.", zoneID, cnt);
				std::string msg = buf;
				g_pMain->SendNotice(msg.c_str());
			}
			goto cleanup;
		}

		// PARIMUTUEL hesap
		uint64 totalPool = winnerPool + loserPool;
		uint64 commission = totalPool * g_betCommissionPct / 100;  // SINK
		uint64 distributable = totalPool - commission;

		// Stat track
		struct _PAYOUT_STAT { std::string name; int64_t delta; };
		std::vector<_PAYOUT_STAT> winners;
		uint64 totalPaidOut = 0;
		int loserCount = 0;

		for (auto& bet : bets) {
			if (bet.resolved) continue;
			bet.resolved = true;

			// BUG FIX (socket ID yeniden kullanim): mac 10dk surer, bu surede oyuncu
			// cikip ayni socket'e baska biri girebilir. Odeme oncesi ISIM TEYIT et.
			CUser* pUser = g_pMain->GetUserPtr(bet.betterUserID);
			if (pUser != nullptr && pUser->GetName() != bet.betterCharName) {
				pUser = nullptr;  // socket baskasinda — bu oturuma odeme yapma (offline say)
			}

			if (bet.betClanID == winnerClanID) {
				// Pay orani: (kendi bahsi / kazanan havuz) * dagitilacak
				uint64 payout = (uint64)bet.betAmount * distributable / winnerPool;
				// COIN_MAX cap — GoldGain uint32 (max 4.2 milyar), oyuncu cuzdani COIN_MAX (2.1 milyar)
				if (payout > COIN_MAX) payout = COIN_MAX;
				totalPaidOut += payout;
				int64_t kar = (int64_t)payout - (int64_t)bet.betAmount;
				winners.push_back({bet.betterCharName, kar});

				if (pUser != nullptr && pUser->isInGame()) {
					pUser->GoldGain((uint32)payout);
					char buf[260] = {0};
					_snprintf_s(buf, sizeof(buf), _TRUNCATE,
						"[BET] KAZANDIN! +%llu Noah (kar %+lld). | YOU WON! +%llu Noah.",
						(unsigned long long)payout, (long long)kar, (unsigned long long)payout);
					std::string msg = buf;
					Packet pkt;
					ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
					pUser->Send(&pkt);
				}
			} else {
				// Kaybetti — para gitti (havuza/kazananlara dagildi)
				loserCount++;
				if (pUser != nullptr && pUser->isInGame()) {
					std::string msg = "[BET] Kaybettin, paran kazananlara dagildi. | You lost, your bet went to winners.";
					Packet pkt;
					ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
					pUser->Send(&pkt);
				}
			}
		}

		// Server-wide RESOLVE duyuru
		double winnerOdds = (double)distributable / (double)winnerPool;
		char buf[400] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BAHIS SONUC / BET RESULT] Zone %u: Havuz/Pool %llu | Komisyon/Fee %llu (%%%u) | %zu kazandi/won (%.2fx), %d kaybetti/lost",
			zoneID, (unsigned long long)totalPool, (unsigned long long)commission,
			g_betCommissionPct, winners.size(), winnerOdds, loserCount);
		std::string msgR = buf;
		g_pMain->SendNotice(msgR.c_str());

		// Top 3 kazanan
		if (!winners.empty()) {
			std::sort(winners.begin(), winners.end(),
				[](const _PAYOUT_STAT& a, const _PAYOUT_STAT& b){ return a.delta > b.delta; });
			std::string msgTop = "[BAHIS / BET TOP] En cok kazanan / Top winners: ";
			size_t cnt = (winners.size() < 3) ? winners.size() : 3;
			for (size_t i = 0; i < cnt; i++) {
				char tbuf[120] = {0};
				_snprintf_s(tbuf, sizeof(tbuf), _TRUNCATE,
					"%zu) %s (kar %+lld) ", i+1, winners[i].name.c_str(), (long long)winners[i].delta);
				msgTop += tbuf;
			}
			g_pMain->SendNotice(msgTop.c_str());
		}

		printf("[BET ECONOMY] Zone %u: totalPool=%llu commission_SINK=%llu distributed=%llu (loserPool=%llu eritildi)\n",
			zoneID, (unsigned long long)totalPool, (unsigned long long)commission,
			(unsigned long long)totalPaidOut, (unsigned long long)loserPool);
	}

cleanup:
	// Cleanup
	g_activeBets.erase(it);
	g_betCloseTime.erase(zoneID);
	g_betCloseAnnounced.erase(zoneID);
	g_betLastStatusBroadcast.erase(zoneID);

	// DB entegrasyon
	if (winnerClanID == 0)
		g_DBAgent.TournamentBetRefund(zoneID);
	else
		g_DBAgent.TournamentBetResolve(zoneID, winnerClanID);
}

// +bet ClanAdı Miktar
COMMAND_HANDLER(CUser::HandleTournamentBetCommand)
{
	if (vargs.empty())
	{
		g_pMain->SendHelpDescription(this,
			"Bahis: +bet <KlanAdi> <Noah> | iptal icin: +bet cancel | durum: +betstatus");
		return true;
	}

	// +bet cancel — oyuncu kendi aktif bahislerini iptal eder (iade), pencere acikken
	std::string firstArg = vargs.front();
	if (firstArg == "cancel" || firstArg == "iptal")
	{
		std::lock_guard<std::recursive_mutex> lock(g_betLock);
		uint32 refunded = 0;
		int refundCount = 0;
		std::string myAccount = GetAccountName();
		for (auto& kv : g_activeBets) {
			uint8 zid = kv.first;
			// Pencere kapali mi? Kapaliysa iptal edilemez (mac basladi)
			auto cit = g_betCloseTime.find(zid);
			bool windowOpen = (cit != g_betCloseTime.end() && UNIXTIME <= cit->second);
			if (!windowOpen) continue;

			bool anyInZone = false;
			for (auto bit = kv.second.begin(); bit != kv.second.end(); ) {
				// Socket ID + isim teyit (yeniden kullanim guard)
				if (bit->betterUserID == GetID() && bit->betterCharName == GetName() && !bit->resolved) {
					refunded += bit->betAmount;
					refundCount++;
					anyInZone = true;
					GoldGain(bit->betAmount);  // iade
					bit = kv.second.erase(bit);
				} else {
					++bit;
				}
			}
			// DB tutarlilik — bu zone'daki bahislerini CANCELLED yap
			if (anyInZone)
				g_DBAgent.TournamentBetCancelUser(zid, myAccount);
		}
		char buf[200] = {0};
		if (refundCount > 0) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET] %d bahisten ciktin, %u Noah iade edildi. | %d bets cancelled, %u Noah refunded.",
				refundCount, refunded, refundCount, refunded);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET] Iptal edilecek aktif bahsin yok (veya pencere kapali). | No active bets to cancel.");
		}
		std::string msg = buf;
		Packet pkt;
		ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
		Send(&pkt);
		return true;
	}

	if (vargs.size() < 2)
	{
		g_pMain->SendHelpDescription(this,
			"Bahis: +bet <KlanAdi> <Noah>. Ornek: +bet ILKCLAN 100000 | iptal: +bet cancel");
		return true;
	}

	std::string targetClanName = vargs.front(); vargs.pop_front();
	uint32 amount = (uint32)SafeAtoi(vargs.front(), 0, 0x7FFFFFFF);

	if (amount < g_betMinAmount)
	{
		char buf[140];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"Minimum bahis %u Noah. | Minimum bet %u Noah.", g_betMinAmount, g_betMinAmount);
		g_pMain->SendHelpDescription(this, buf);
		return true;
	}

	if (amount > g_betMaxPerUser)
	{
		char buf[140];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"Maximum bahis %u Noah. | Maximum bet %u Noah.", g_betMaxPerUser, g_betMaxPerUser);
		g_pMain->SendHelpDescription(this, buf);
		return true;
	}

	if (m_iGold < amount)
	{
		g_pMain->SendHelpDescription(this, "Yeterli Noah'in yok. | Not enough Noah.");
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
			"Bu klan icin aktif tournament yok, klan adini kontrol et. | No active tournament for this clan.");
		return true;
	}

	// Bahis kapanma zamani gecti mi?
	auto closeIt = g_betCloseTime.find(targetZoneID);
	if (closeIt != g_betCloseTime.end() && UNIXTIME > closeIt->second)
	{
		g_pMain->SendHelpDescription(this,
			"Bahisler kapali (pencere suresi doldu). | Betting closed (window expired).");
		return true;
	}

	// Ayni oyuncunun ayni tournament icin onceki bahisleri toplami + SAYISI
	uint32 totalForThisUser = amount;
	uint8  countForThisUser = 1;  // bu bahis dahil
	for (auto& b : g_activeBets[targetZoneID])
	{
		if (b.betterUserID == GetID()) {
			totalForThisUser += b.betAmount;
			countForThisUser++;
		}
	}
	// Toplam TUTAR limiti
	if (totalForThisUser > g_betMaxPerUser)
	{
		char buf[160] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"Toplam bahis limitin doldu (%u Noah). | Total bet limit reached (%u Noah).",
			g_betMaxPerUser, g_betMaxPerUser);
		g_pMain->SendHelpDescription(this, buf);
		return true;
	}
	// Bahis SAYISI limiti (spam onleme)
	if (countForThisUser > g_betMaxCountPerUser)
	{
		char buf[160] = {0};
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"Bu tournament icin max %u bahis koyabilirsin. | Max %u bets allowed per tournament.",
			g_betMaxCountPerUser, g_betMaxCountPerUser);
		g_pMain->SendHelpDescription(this, buf);
		return true;
	}

	// KLAN SUIKASTI ONLEMI — ayni oyuncu rakip tarafa bahis koyamaz (garanti kar engeli)
	if (g_betPreventClanBetraysal)
	{
		uint16 myClanID = GetClanID();
		// 1) Kullanici daha once bu zone'da KARSI klana bahis koymus mu?
		for (auto& b : g_activeBets[targetZoneID]) {
			if (b.betterUserID == GetID() && b.betClanID != targetClanID) {
				g_pMain->SendHelpDescription(this,
					"Iki klana birden bahis koyamazsin. | Cannot bet on both clans.");
				return true;
			}
		}
		// 2) Kullanici kendi klani savasiyorsa, RAKIP klana bahis koyamaz
		//    (kendi klanina bahis serbest — patron karari)
		if (myClanID > 0 && myClanID != targetClanID) {
			if (myClanID == targetInfo->aTournamentClanNum[0] ||
			    myClanID == targetInfo->aTournamentClanNum[1]) {
				g_pMain->SendHelpDescription(this,
					"Kendi klanin savasirken rakibe bahis koyamazsin. | Cannot bet against your own clan.");
				return true;
			}
		}
	}

	// Para cek
	if (!GoldLose(amount, true))
	{
		g_pMain->SendHelpDescription(this, "Para cekilemedi, tekrar dene. | Payment failed, try again.");
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

	// PARIMUTUEL — su anki havuza gore tahmini oran/kazanc
	uint64 myPool = 0, otherPool = 0;  // bu bahis dahil
	for (auto& b : g_activeBets[targetZoneID]) {
		if (b.betClanID == targetClanID) myPool += b.betAmount;
		else otherPool += b.betAmount;
	}

	char buf[360] = { 0 };
	if (otherPool == 0) {
		// Rakip taraf henuz bahis koymadi — oran hesaplanamaz (rakipsizse iade riski)
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BAHIS VERDIN / BET PLACED] %s'a %u Noah! Rakip yok, oran rakip gelince olusur (rakipsiz biterse IADE). "
			"| No opponent yet, refunded if no rival.",
			targetClanName.c_str(), amount);
	} else {
		uint64 totalPool = myPool + otherPool;
		uint64 distributable = totalPool - (totalPool * g_betCommissionPct / 100);
		double estOdds = (double)distributable / (double)myPool;
		uint64 estPayout = (uint64)amount * distributable / myPool;
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[BAHIS VERDIN / BET PLACED] %s'a %u Noah! Oran/Odds: %.2fx (tahmini/est ~%llu Noah). "
			"Oran degisir, kesin kazanc mac sonu / Final payout at match end.",
			targetClanName.c_str(), amount, estOdds, (unsigned long long)estPayout);
	}
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

		// PARIMUTUEL oran hesabi (rakipsizse oran yok)
		uint64 totalPool = (uint64)redPool + bluePool;
		uint64 distributable = totalPool - (totalPool * g_betCommissionPct / 100);
		double redOdds  = (redPool  > 0 && bluePool > 0) ? ((double)distributable / (double)redPool)  : 0.0;
		double blueOdds = (bluePool > 0 && redPool  > 0) ? ((double)distributable / (double)bluePool) : 0.0;

		// Satir 1: Genel + oran
		if (redPool > 0 && bluePool > 0) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET %u] %s: %u Noah (oran %.2fx) | %s: %u Noah (oran %.2fx) | Komisyon %%%u",
				zid, pRed->GetName().c_str(), redPool, redOdds,
				pBlue->GetName().c_str(), bluePool, blueOdds, g_betCommissionPct);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"[BET %u] %s: %u Noah | %s: %u Noah | (oran rakip gelince olusur)",
				zid, pRed->GetName().c_str(), redPool, pBlue->GetName().c_str(), bluePool);
		}
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
				"  Senin bahisin / Your bet: %u Noah (%s) — iptal: +bet cancel", myTotal, myClanName.c_str());
			std::string m3 = buf;
			Packet p; ChatPacket::Construct(&p, (uint8)ChatType::WAR_SYSTEM_CHAT, &m3); Send(&p);
		}

		// Satir 4: Kapanma sure
		if (remaining > 0) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"  Kapanmasina / Closes in: %lld sn", (long long)remaining);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "  BAHIS KAPALI / BETTING CLOSED (mac devam / match ongoing)");
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

// =====================================================================
// S115 ekonomi fix — GM Bet config komutlari (console)
// =====================================================================

// /betlimits MIN MAX
COMMAND_HANDLER(CGameServerDlg::HandleBetLimitsCommand)
{
	if (vargs.size() < 2) {
		uint32 mn, mx; time_t w; uint8 c;
		GetBetConfig(mn, mx, w, c);
		printf("Usage: /betlimits MIN MAX (su an: min=%u max=%u)\n", mn, mx);
		return true;
	}
	uint32 mn = (uint32)SafeAtoi(vargs.front(), 1000, 0x7FFFFFFF); vargs.pop_front();
	uint32 mx = (uint32)SafeAtoi(vargs.front(), mn, 0x7FFFFFFF);
	extern void SetBetLimits(uint32, uint32);
	SetBetLimits(mn, mx);
	printf("[BET CONFIG] Limit ayarlandi: min=%u max=%u Noah\n", mn, mx);
	return true;
}

// /betwindow SANIYE
COMMAND_HANDLER(CGameServerDlg::HandleBetWindowCommand)
{
	if (vargs.empty()) {
		uint32 mn, mx; time_t w; uint8 c;
		GetBetConfig(mn, mx, w, c);
		printf("Usage: /betwindow SANIYE (su an: %llds, onerilen 30-300)\n", (long long)w);
		return true;
	}
	time_t sec = (time_t)SafeAtoi(vargs.front(), 30, 600);
	extern void SetBetWindow(time_t);
	SetBetWindow(sec);
	printf("[BET CONFIG] Bahis penceresi: %llds (sonraki tournament'tan itibaren)\n", (long long)sec);
	return true;
}

// /betcommission YUZDE (sink — oyundan eritilen)
COMMAND_HANDLER(CGameServerDlg::HandleBetCommissionCommand)
{
	if (vargs.empty()) {
		uint32 mn, mx; time_t w; uint8 c;
		GetBetConfig(mn, mx, w, c);
		printf("Usage: /betcommission YUZDE (su an: %%%u, 0-30 arasi). Komisyon SINK — oyundan eritilir.\n", c);
		return true;
	}
	uint8 pct = (uint8)SafeAtoi(vargs.front(), 0, 30);
	extern void SetBetCommission(uint8);
	SetBetCommission(pct);
	printf("[BET CONFIG] Komisyon: %%%u (her bahis havuzundan yakilir/eritilir)\n", pct);
	return true;
}

// /betcancel ZONE — aktif bahisleri iptal + iade
COMMAND_HANDLER(CGameServerDlg::HandleBetCancelCommand)
{
	if (vargs.empty()) {
		printf("Usage: /betcancel ZONE\n");
		return true;
	}
	uint8 zoneID = (uint8)SafeAtoi(vargs.front(), 1, 255);
	extern void CancelTournamentBets(uint8 zoneID);
	CancelTournamentBets(zoneID);
	printf("[BET CONFIG] Zone %u bahisleri iptal + iade edildi (GM komutu)\n", zoneID);
	return true;
}
