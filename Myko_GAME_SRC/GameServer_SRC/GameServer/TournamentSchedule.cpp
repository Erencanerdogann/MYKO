// =========================================================================
// S115 TUR 10 — Tournament Auto-Schedule
// Yazan: CHIP | Tarih: 2026-05-27
// =========================================================================
// PATRON karari: "tablo bos kalsin, zaman girilmesin"
//   - DB tablo _MK_TOURNAMENT_SCHEDULE bos
//   - GM elden ekler (web panel veya direkt SQL)
//   - GameServer her dakika kontrol eder
//   - Uygun zaman gelmis kayit varsa otomatik HandleTournamentStart
// PG temiz: server-side scheduler, opcode YOK
// =========================================================================

#include "stdafx.h"

// Schedule kayit RAM cache (DB'den yuklenmis)
struct _TOURNAMENT_SCHEDULE_ENTRY {
	uint8  zoneID;
	uint8  dayOfWeek;        // 1=Pazartesi, 7=Pazar (0 = her gun)
	uint8  hourUTC;          // 0-23
	uint16 durationMinutes;
	bool   enabled;
	std::string redClanName;  // Eger bos ise patron elle eklemis demek; otomatik sec
	std::string blueClanName;
	time_t lastTriggered;     // En son ne zaman tetiklendi (ayni anda tekrar yapmamak icin)
};

static std::vector<_TOURNAMENT_SCHEDULE_ENTRY> g_schedules;
static std::recursive_mutex g_scheduleLock;
static time_t g_lastScheduleCheck = 0;

// MATRIX MSG:5897 — _MK_TOURNAMENT_SCHEDULE tablosu canli (bos, GM doldurur)
// GameServer init'inde + reload komutunda cagrilir
void LoadTournamentScheduleFromDB()
{
	std::lock_guard<std::recursive_mutex> lock(g_scheduleLock);
	g_schedules.clear();

	std::unique_ptr<OdbcCommand> dbCommand(g_DBAgent.GetGameDB()->CreateCommand());
	if (dbCommand.get() == nullptr) {
		printf("[TOURNAMENT_SCHEDULE] DB command olusturulamadi\n");
		return;
	}

	// Tum aktif schedule kayitlarini cek
	if (!dbCommand->Execute(_T("SELECT ZoneID, DayOfWeek, HourUTC, DurationMinutes, Enabled FROM KO_LOG.dbo._MK_TOURNAMENT_SCHEDULE WHERE Enabled = 1"))) {
		// DB hata - bos calisir
		printf("[TOURNAMENT_SCHEDULE] DB SELECT hata (tablo bos olabilir)\n");
		return;
	}

	while (dbCommand->MoveNext()) {
		_TOURNAMENT_SCHEDULE_ENTRY entry;
		uint8 zid = 0, dow = 0, hour = 0;
		int16 dur = 30;
		uint8 enabled = 1;

		dbCommand->FetchByte(1, zid);
		dbCommand->FetchByte(2, dow);
		dbCommand->FetchByte(3, hour);
		dbCommand->FetchInt16(4, dur);
		dbCommand->FetchByte(5, enabled);

		entry.zoneID = zid;
		entry.dayOfWeek = dow;
		entry.hourUTC = hour;
		entry.durationMinutes = (uint16)dur;
		entry.enabled = (enabled != 0);
		entry.lastTriggered = 0;
		// Klan adlari su an scheduled tablosunda yok — GM run-time karar verir
		// Veya tablo'ya kolon eklemek lazim (acilis sonrasi)
		entry.redClanName.clear();
		entry.blueClanName.clear();
		g_schedules.push_back(entry);
	}

	printf("[TOURNAMENT_SCHEDULE] Loaded %zu schedules from DB\n", g_schedules.size());
}

// Her dakika cagrilir (GameEventMainTimer veya benzer)
// PATRON karari: tablo bos kalir, bu fonksiyon GM tablo doldurana kadar bos donus
void TournamentScheduleTimer()
{
	// Her dakika kontrol (60sn cooldown)
	if (UNIXTIME - g_lastScheduleCheck < 60) return;
	g_lastScheduleCheck = UNIXTIME;

	std::lock_guard<std::recursive_mutex> lock(g_scheduleLock);
	if (g_schedules.empty()) return; // GM henuz kayit eklemedi

	// Simdiki zaman bilesi (UTC)
	time_t now = UNIXTIME;
	struct tm tmNow;
	gmtime_s(&tmNow, &now);

	int currentDayOfWeek = (tmNow.tm_wday == 0) ? 7 : tmNow.tm_wday; // 1=Pzt, 7=Pzr
	int currentHour = tmNow.tm_hour;
	int currentMinute = tmNow.tm_min;

	// Sadece **tamamlanmis saat** (HH:00) iken tetikle
	if (currentMinute != 0) return;

	for (auto& entry : g_schedules)
	{
		if (!entry.enabled) continue;
		if (entry.dayOfWeek != 0 && entry.dayOfWeek != currentDayOfWeek) continue;
		if (entry.hourUTC != currentHour) continue;

		// Bugun zaten tetiklenmis mi? (gun degisirken tekrar tetiklenmesin)
		if (entry.lastTriggered > 0 && (now - entry.lastTriggered) < 23 * 3600) continue;

		// Zone'da aktif tournament var mi?
		if (g_pMain->m_ClanVsDataList.GetData(entry.zoneID) != nullptr)
		{
			printf("[TOURNAMENT_SCHEDULE] Skip: Zone %d already has active tournament\n", entry.zoneID);
			continue;
		}

		// PATRON karari: bos kalirsa otomatik baslamasin
		// Klan adlari eksikse manuel acilim gerekli
		if (entry.redClanName.empty() || entry.blueClanName.empty())
		{
			// Otomatik klan secimi: aktif klanlar arasinda rastgele?
			// Su an YOK — patron istek olarak ekleyebilir
			continue;
		}

		// TODO: Otomatik HandleTournamentStart cagri
		// Su an iskelet — ConsoleCommand::Process("tournamentstart Red Blue Zone Duration") yontemi var
		// veya direkt _TOURNAMENT_DATA olustur + PutData
		printf("[TOURNAMENT_SCHEDULE] WOULD TRIGGER: Zone=%d Red=%s Blue=%s Duration=%d\n",
			entry.zoneID, entry.redClanName.c_str(),
			entry.blueClanName.c_str(), entry.durationMinutes);

		entry.lastTriggered = now;
	}
}
