#include "stdafx.h"
#include "DBAgent.h"
#include "../shared/DateTime.h"
#include "../shared/Ini.h"
// S114 K3 FAZ 6: Discord webhook KAPALI (crash riski) — include kaldirildi

using std::string;

ServerCommandTable CGameServerDlg::s_commandTable;
ChatCommandTable CUser::s_commandTable;

void CGameServerDlg::InitServerCommands()
{
	static Command<CGameServerDlg> commandTable[] = 
	{
		{ "help",				&CGameServerDlg::HandleHelpCommand,					"Show all commands." },
		{ "resetloyalty",		&CGameServerDlg::HandleResetRLoyaltyCommand,		"Reset Loyalty" },
		{ "notice",				&CGameServerDlg::HandleNoticeCommand,				"Sends a server-wide chat notice." },
		{ "noticeall",			&CGameServerDlg::HandleNoticeallCommand,			"Sends a server-wide chat notice." },
		{ "kill",				&CGameServerDlg::HandleKillUserCommand,				"Disconnects the specified player" },
		{ "open1",				&CGameServerDlg::HandleWar1OpenCommand,				"Opens war zone 1" },
		{ "open2",				&CGameServerDlg::HandleWar2OpenCommand,				"Opens war zone 2" },
		{ "open3",				&CGameServerDlg::HandleWar3OpenCommand,				"Opens war zone 3" },
		{ "open4",				&CGameServerDlg::HandleWar4OpenCommand,				"Opens war zone 4" },
		{ "open5",				&CGameServerDlg::HandleWar5OpenCommand,				"Opens war zone 5" },
		{ "open6",				&CGameServerDlg::HandleWar6OpenCommand,				"Opens war zone 6" },
		{ "snow",				&CGameServerDlg::HandleSnowWarOpenCommand,			"Opens the snow war zone" },
		{ "csw",				&CGameServerDlg::HandleSiegeWarOpenCommand,			"Opens the Castle Siege War zone" },
		{ "close",				&CGameServerDlg::HandleWarCloseCommand,				"Closes the active war zone" },
		{ "cswclose",			&CGameServerDlg::HandleCastleSiegeWarClose,			"Closes the active csw zone" },
		{ "cswskip",			&CGameServerDlg::HandleCastleSiegeWarSkipTimer,		"CSW timer'i 5sn'ye dusurur (test)" },
		{ "down",				&CGameServerDlg::HandleShutdownCommand,				"Shuts down the server" },
		{ "shutdown",			&CGameServerDlg::HandleConsoleShutdownCommand,		"Sunucu N dk sonra kapat (orn: /shutdown 5)" },
		{ "caremode",			&CGameServerDlg::HandleConsoleMaintenanceCommand,	"Bakim modu N dk sonra (orn: /caremode 10)" },
		{ "caremodeoff",		&CGameServerDlg::HandleConsoleMaintenanceOffCommand,"Bakim modu iptal" },
		{ "discount",			&CGameServerDlg::HandleDiscountCommand,				"Enables server discounts for the winning nation of the last war" },
		{ "alldiscount",		&CGameServerDlg::HandleGlobalDiscountCommand,		"Enables server discounts for everyone" },
		{ "offdiscount",		&CGameServerDlg::HandleDiscountOffCommand,			"Disables server discounts" },
		{ "captain",			&CGameServerDlg::HandleCaptainCommand,				"Sets the captains/commanders for the war" },
		{ "santa",				&CGameServerDlg::HandleSantaCommand,				"Enables a flying Santa Claus." },
		{ "santaclose",			&CGameServerDlg::HandleSantaOffCommand,				"Disables a flying Santa Claus/angel." },
		{ "angel",				&CGameServerDlg::HandleAngelCommand,				"Enables a flying angel." },
		{ "angelclose",			&CGameServerDlg::HandleSantaOffCommand,				"Disables a flying Santa Claus/angel." },
		{ "permanent",			&CGameServerDlg::HandlePermanentChatCommand,		"Sets the permanent chat bar to the specified text." },
		{ "offpermanent",		&CGameServerDlg::HandlePermanentChatOffCommand,		"Resets the permanent chat bar text." },
		{ "beefclose",			&CGameServerDlg::HandleBeefEventClose,				"Beef Event kapatir. Ornek: +beefclose" },
		{ "reloadnotice",		&CGameServerDlg::HandleReloadNoticeCommand,			"Duyuru listesini yeniler" },
		{ "reloadtables",		&CGameServerDlg::HandleReloadTablesCommand,			"Tablolari yeniler" },
		{ "reloadtables2",		&CGameServerDlg::HandleReloadTables2Command,		"Tablolari yeniler" },
		{ "reloadtables3",		&CGameServerDlg::HandleReloadTables3Command,		"Tablolari yeniler" },
		{ "reloadmagics",		&CGameServerDlg::HandleReloadMagicsCommand,			"Skill tablolarini yeniler" },
		{ "reloadquests",		&CGameServerDlg::HandleReloadQuestCommand,			"Gorev tablolarini yeniler" },
		{ "reloadranks",		&CGameServerDlg::HandleReloadRanksCommand,			"Siralama tablolarini yeniler" },
		{ "reloaddrops",		&CGameServerDlg::HandleReloadDropsCommand,			"Drop tablolarini yeniler" },
		{ "reloaddrops2",		&CGameServerDlg::HandleReloadDropsRandomCommand,	"Drop tablolarini yeniler" },
		{ "reloadkings",		&CGameServerDlg::HandleReloadKingsCommand,			"Kral tablolarini yeniler" },
		{ "reloadtitle",		&CGameServerDlg::HandleReloadRightTopTitleCommand,	"Baslik tablolarini yeniler" }, 
		{ "reloadpus",			&CGameServerDlg::HandleReloadPusItemCommand,		"Tablolari yeniler" }, 
		{ "reloaditems",		&CGameServerDlg::HandleReloadItemsCommand,			"Item tablolarini yeniler" },
		{ "reloaddungeon",		&CGameServerDlg::HandleReloadDungeonDefenceTables,	"Zindan savunma tablolarini yeniler" },
		{ "reloaddraki",		&CGameServerDlg::HandleReloadDrakiTowerTables,		"Draki Kule tablolarini yeniler" },
		{ "reloadevent",		&CGameServerDlg::HandleEventScheduleResetTable,		"Event zamanlama tablolarini yeniler" },
		{ "reloadpremium",		&CGameServerDlg::HandleReloadClanPremiumTable,		"Klan premium tablosunu yeniler" },
		{ "reloadsocial",		&CGameServerDlg::HandleTopLeftCommand,				"Sosyal grup ikonunu yeniler" },
		{ "reloadclanpnotice",	&CGameServerDlg::HandleReloadBonusNotice,			"Klan premium duyuru listesini yeniler" },
		{ "reload_item",		&CGameServerDlg::HandleReloadItems,					"Item tablosunu yeniler" },
		{ "reloadupgrade",		&CGameServerDlg::HandleReloadUpgradeCommand,		"Upgrade tablosunu yeniler" },
		{ "reloadbug",			&CGameServerDlg::HandleReloadRankBugCommand,		"Bug tablosunu yeniler" },
		{ "reloadbot",			&CGameServerDlg::HandleReloadBotInfoCommand,		"Bot bilgi tablosunu yeniler" },
		
		{ "reloadlreward",		&CGameServerDlg::HandleReloadLevelRewardCommand,		"Level odul tablosunu yeniler" },
		{ "reloadmreward",		&CGameServerDlg::HandleReloadMerchantLevelRewardCommand,"Merchant level odul tablosunu yeniler" },

		{ "reloadzoneon",		&CGameServerDlg::HandleReloadZoneOnlineRewardCommand,"Zone online odul tablosunu yeniler" },

		{ "madclas",			&CGameServerDlg::HandleCindirellaWarOpen,			"MadClas Event Başlatır - Open MadClas Event Type  1(47Lwl) 2(59Lwl) 3(83Lwl)" },
		{ "madclasclose",		&CGameServerDlg::HandleCindirellaWarClose,			"MadClas Event Kapatır - Close Mad Class Event" },
		{ "ftopen",				&CGameServerDlg::HandleForgettenTempleEvent,		"BF Event Başlatır - Open Forgetten Temple" },
		{ "ftclose",			&CGameServerDlg::HandleForgettenTempleEventClose,	"BF Event Kapatır - Close Forgetten Temple" },
		{ "count",				&CGameServerDlg::HandleCountCommand,				"Online Oyuncu Sayısını Gösterir - Get online user count." },
		{ "tpall",				&CGameServerDlg::HandleTeleportAllCommand,			"Belirtilen zone deki oyuncuları belirtilen zone ışınlar - Players send to home zone." },
		{ "warresult",			&CGameServerDlg::HandleWarResultCommand,			"Set result for War" },
		{ "utc",				&CGameServerDlg::HandleEventUnderTheCastleCommand,	"UTC event başlatır - Open & close event Under the Castle zone" },
		{ "tournamentstart",	&CGameServerDlg::HandleTournamentStart,				"Start is Clan Tournament" },
		{ "tournamentclose",	&CGameServerDlg::HandleTournamentClose,				"Close is Clan Tournament" },
		{ "tournamentreglist",	&CGameServerDlg::HandleTournamentRegListCommand,	"Tournament kayitli klan listesi (console)" },
		{ "betstatus",			&CGameServerDlg::HandleBetStatusConsole,			"Tum aktif bahis durumunu console'a yaz (GM)" },
		{ "betlimits",			&CGameServerDlg::HandleBetLimitsCommand,			"Bahis limit ayarla. Ornek: /betlimits 10000 5000000" },
		{ "betwindow",			&CGameServerDlg::HandleBetWindowCommand,			"Bahis penceresi suresi. Ornek: /betwindow 120 (saniye)" },
		{ "betcommission",		&CGameServerDlg::HandleBetCommissionCommand,		"Bahis komisyonu %% (sink, oyundan eritilir). Ornek: /betcommission 10" },
		{ "betcancel",			&CGameServerDlg::HandleBetCancelCommand,			"Aktif bahisleri iptal + iade. Ornek: /betcancel 96" },
		{ "leaguecreate",		&CGameServerDlg::HandleLeagueCreateCommand,			"Lig olustur (round-robin). Ornek: /leaguecreate \"Acilis Ligi\" 5" },
		{ "leaguestart",		&CGameServerDlg::HandleLeagueStartCommand,			"Lig basla (fikstur olusur). Ornek: /leaguestart 1" },
		{ "leaguestatus",		&CGameServerDlg::HandleLeagueStatusCommand,			"Lig puan tablosu (console). Ornek: /leaguestatus 1" },
		{ "leaguecancel",		&CGameServerDlg::HandleLeagueCancelCommand,			"Lig iptal. Ornek: /leaguecancel 1" },
		{ "tournamentreward",	&CGameServerDlg::HandleTournamentRewardCommand,		"GM manuel odul. Ornek: /tournamentreward noah klan RedClan 5000000 | item oyuncu Ahmet 379010 1" },
		{ "partyvs",			&CGameServerDlg::HandlePartyVsCommand,				"Party vs Party duello (anlik). Ornek: /partyvs Ahmet Mehmet 96 10" },
		{ "eventcreate",		&CGameServerDlg::HandleEventCreateCommand,			"Zamanlanmis event (KAYIT->BAHIS->MAC). Ornek: /eventcreate Ahmet Mehmet 96 10 10 30" },
		{ "eventconfig",		&CGameServerDlg::HandleEventConfigCommand,			"Event varsayilan sure ayar. Ornek: /eventconfig 10 10 30 30" },
		{ "eventlist",			&CGameServerDlg::HandleEventListCommand,			"Aktif event'leri listele" },
		{ "eventcancel",		&CGameServerDlg::HandleEventCancelCommand,			"Event iptal. Ornek: /eventcancel 1" },
		{ "partybracketcreate",	&CGameServerDlg::HandlePartyBracketCreateCommand,	"Party bracket olustur. Ornek: /partybracketcreate \"Party Cup\" 8" },
		{ "partyleaguecreate",	&CGameServerDlg::HandlePartyLeagueCreateCommand,	"Party lig olustur. Ornek: /partyleaguecreate \"Party Lig\" 5" },
		{ "partybracketstart",	&CGameServerDlg::HandlePartyBracketStartCommand,	"Party bracket basla. Ornek: /partybracketstart 1" },
		{ "partyleaguestart",	&CGameServerDlg::HandlePartyLeagueStartCommand,		"Party lig basla. Ornek: /partyleaguestart 1" },
		{ "bracketcreate",		&CGameServerDlg::HandleBracketCreateCommand,		"Bracket olustur. Ornek: /bracketcreate \"Acilis Cup\" 16" },
		{ "bracketstart",		&CGameServerDlg::HandleBracketStartCommand,			"Bracket baslat. Ornek: /bracketstart 1" },
		{ "bracketstatus",		&CGameServerDlg::HandleBracketStatusCommand,		"Bracket durumu. Ornek: /bracketstatus 1" },
		{ "bracketcancel",		&CGameServerDlg::HandleBracketCancelCommand,		"Bracket iptal. Ornek: /bracketcancel 1" },
		{ "ctfstart",			&CGameServerDlg::HandleCTFStartCommand,				"Crystal CTF basla. Ornek: /ctfstart RED BLUE 96" },
		{ "ctfclose",			&CGameServerDlg::HandleCTFCloseCommand,				"Crystal CTF kapat. Ornek: /ctfclose 96" },
		{ "1v1create",			&CGameServerDlg::HandleOneVsOneCreateCommand,		"1v1 Bracket olustur. Ornek: /1v1create \"Solo Cup\" 16" },
		{ "1v1start",			&CGameServerDlg::HandleOneVsOneStartCommand,		"1v1 Bracket basla. Ornek: /1v1start 1" },
		{ "1v1cancel",			&CGameServerDlg::HandleOneVsOneCancelCommand,		"1v1 Bracket iptal. Ornek: /1v1cancel 1" },
		{ "1v1status",			&CGameServerDlg::HandleOneVsOneStatusCommand,		"1v1 Bracket durumu. Ornek: /1v1status 1" },
		{ "chaosopen",			&CGameServerDlg::HandleChaosExpansionOpen,			"Chaos Event başlatır - Open is Chaos Expansion" },
		{ "borderopen",			&CGameServerDlg::HandleBorderDefenceWar,			"BDW Event Başlatır - Open is Border Defence War" },
		{ "juraidopen",			&CGameServerDlg::HandleJuraidMountain,				"JR Event Başlatır - Open is Juraid Mountain" },
		{ "beefopen",			&CGameServerDlg::HandleBeefEvent,					"Beef Event Başlatır - Open is Beef Event" },		
		{ "chaosclose",			&CGameServerDlg::HandleChaosExpansionClose,			"Chaos Event Kapatır - Chaos Expansion Close" },
		{ "borderclose",		&CGameServerDlg::HandleBorderDefenceWarClose,		"BDW Evenet Kapatır - Border Defence War Close" },
		{ "juraidclose",		&CGameServerDlg::HandleJuraidMountainClose,			"JR Event Kapatır -Juraid Mountain Close" },		
		{ "lottery",			&CGameServerDlg::HandleLotteryStart,				"Lottery Event Başlatır -Lottery Start" },				
		{ "lotteryclose",		&CGameServerDlg::HandleLotteryClose,				"Lottery Event Kapatır - Lottery Close" },
		{ "testing",			&CGameServerDlg::HandleServerGameTestCommand,		"Sunucu test komutu" },
		{ "user_bots",			&CGameServerDlg::HandleServerBotCommand,			"Online Oyuncu gibi bot atar - Server User Bot Command Count Time(AS MINUTE) ResType(1 Mining 2 Fishing 3 Standing 4 Sitting) minlevel" },
		{ "aireset",			&CGameServerDlg::HandleAIResetCommand,				"Tüm Mob ve Npc leri resetler - AI Reset Komutu(Tüm Mob ve npc leri yeniler)"	},
		{ "block",				&CGameServerDlg::Handlebannedcommand,				"Oyuncu Banlama - Player permanent ban" },
		{ "bug",				&CGameServerDlg::HandleBugdanKurtarCommand,			"askida kalan karakteri kurtar" },
		{ "reload_cind",		&CGameServerDlg::HandleReloadCindirellaCommand,		"Reloads the in-game Cindirella Table list" },
		{ "setweather",			&CGameServerDlg::HandleSetWeatherCommand,			"Hava tipini degistir (1=fine 2=rain 3=snow) [miktar 0-100]" },
		{ "reloadcsw",			&CGameServerDlg::HandleReloadCswCommand,			"CSW tablolarini yeniler" },

		// S120 GM_MOD — B1 para/item/stat server-form (HTTP'den, hedef online olmali, audit'li)
		{ "noah",				&CGameServerDlg::HandleGoldChangeServerCommand,		"Oyuncuya gold ver/al. /noah <Char> <Gold(+/-)>" },
		{ "np",					&CGameServerDlg::HandleLoyaltyChangeServerCommand,	"Oyuncuya NP ver. /np <Char> <Loyalty>" },
		{ "kc",					&CGameServerDlg::HandleKcChangeServerCommand,		"Oyuncuya KC ver. /kc <Char> <KC>" },
		{ "exp",				&CGameServerDlg::HandleExpChangeServerCommand,		"Oyuncuya EXP ver. /exp <Char> <Exp(+/-)>" },
		{ "give",				&CGameServerDlg::HandleGiveItemServerCommand,		"Oyuncuya item ver. /give <Char> <ItemID> [Adet] [Sure]" },
		{ "level",				&CGameServerDlg::HandleLevelChangeServerCommand,	"Oyuncuya level ver. /level <Char> <10-83>" },
		// S120 GM_MOD — B2 ceza server-form (block/unblock zaten yukarida)
		{ "mute",				&CGameServerDlg::HandleMuteServerCommand,			"Oyuncuyu sustur. /mute <Char> [Gun]" },
		{ "unmute",				&CGameServerDlg::HandleUnMuteServerCommand,			"Susturmayi kaldir. /unmute <Char>" },
		{ "namechange",			&CGameServerDlg::HandleNameChangeServerCommand,		"Karakter adi degistir (online). /namechange <EskiNick> <YeniNick>" },

	};

	init_command_table(CGameServerDlg, commandTable, s_commandTable);
}

void CGameServerDlg::CleanupServerCommands() { free_command_table(s_commandTable); }

void CUser::InitChatCommands()
{
	static Command<CUser> commandTable[] = 
	{
		// Command				Handler											Help message
		{ "help",				&CUser::HandleHelpCommand,						"Tum komutlari goster." },
		{ "resetloyalty",		&CUser::HandleResetRLoyaltyCommand,				"Sağ Np Sıfırlar - Reset Loyalty" },
		{ "give",				&CUser::HandleGiveItemCommand,					"Belirlenen kişiye süreli item gönderir - Gives a player an item. Arguments: character name | item ID | Count | Time" },
		{ "zone_give_item",		&CUser::HandleOnlineZoneGiveItemCommand,		"Belirtilen zonedeki tum oyunculara item verir. Ornek: +zone_give_item ZoneID ItemID Adet Sure" },
		{ "online_give_item",	&CUser::HandleOnlineGiveItemCommand,			"Online olan tüm oyunculara item gönderir - Gives an item to the whole player. Arguments: Item ID | Count | Time" },
		{ "zone",				&CUser::HandleZoneChangeCommand,				"Istediğin zone gider - Teleports you to the specified zone. Arguments: +zone 'Zone ID' " },
		{ "mon",				&CUser::HandleMonsterSummonCommand,				"Istenilen Mob'u yanina işinlar - Spawns the specified monster (does not respawn). Arguments: monster's database ID" },
		{ "npc",				&CUser::HandleNPCSummonCommand,					"Istenilen Npc Yanina işinlar - Spawns the specified NPC (does not respawn). Arguments: NPC's database ID" },
		{ "kill",				&CUser::HandleMonKillCommand,					"Z de olan Mob ve npc öldürür - Kill a NPC or Monster, Arguments: select an Npc and monster than use this command" },
		{ "open1",				&CUser::HandleWar1OpenCommand,					"Savas bolgesi 1 acar" },
		{ "open2",				&CUser::HandleWar2OpenCommand,					"Savas bolgesi 2 acar" },
		{ "open3",				&CUser::HandleWar3OpenCommand,					"Savas bolgesi 3 acar" },
		{ "open4",				&CUser::HandleWar4OpenCommand,					"Savas bolgesi 4 acar" },
		{ "open5",				&CUser::HandleWar5OpenCommand,					"Savas bolgesi 5 acar" },
		{ "open6",				&CUser::HandleWar6OpenCommand,					"Savas bolgesi 6 acar" },
		{ "captain",			&CUser::HandleCaptainCommand,					"Savaştaki kaptani belirler - Sets the captains/commanders for the war" },
		{ "snow",				&CUser::HandleSnowWarOpenCommand,				"Kar savasi bolgesi acar" },
		{ "csw",				&CUser::HandleSiegeWarOpenCommand,				"CSW Savaşı baslatir - Opens the Castle Siege War zone. How does it work? Expamle : +csw" },
		{ "close",				&CUser::HandleWarCloseCommand,					"Aktif Olan Savası Kapatir - Closes the active war zone. How does it work? Expamle : +close" },
		{ "cswclose",			&CUser::HandleCastleSiegeWarClose,				"Csw Savaşı Kapatır - Closes the active CSW Zone. How does it work? Expamle : +cswclose" },
		{ "cswskip",			&CUser::HandleCastleSiegeWarSkipTimer,			"CSW Timer'i 5sn'ye dusurur (test) - Example : +cswskip" },
		{ "np",					&CUser::HandleLoyaltyChangeCommand,				"Belirlenen Kullanıcıya NP verir - Change a player an loyalty. How does it work? Expamle : +np CharacterNick 100" },
		{ "exp",				&CUser::HandleExpChangeCommand,					"Belirtilen KUllanıcıya Exp Verir - Change a player an exp. How does it work? Expamle : +exp CharacterNick 100" },
		{ "noah",				&CUser::HandleGoldChangeCommand,				"Belirtilen Kullanıcıya Para verir - Change a player an gold. How does it work? Expamle : +noah CharacterNick 100" },
		{ "kc",					&CUser::HandleKcChangeCommand,					"Belirtilen Kullanıcıya KC verir - Change a player an KC. How does it work? Expamle : +kc CharacterNick 100 " },
		{ "tl",					&CUser::HandleTLBalanceCommand,					"Belirtilen Kullancıya TL verir - Change a player an KC. How does it work? Expamle : +kc CharacterNick 100 " },
		{ "exp_add",			&CUser::HandleExpAddCommand,					"Exp Event Aktif eder - Sets the server-wide XP event. If bonusPercent is set to 0, the event is ended. Arguments: bonusPercent" },
		{ "np_add",				&CUser::HandleNPAddCommand,						"Np Event Aktif eder - Sets the server-wide NP event. If bonusPercent is set to 0, the event is ended. Arguments: bonusPercent" },
		{ "money_add",			&CUser::HandleMoneyAddCommand,					"Coin Event Aktif eder - Sets the server-wide coin event. If bonusPercent is set to 0, the event is ended. Arguments: bonusPercent" },
		{ "drop_add",			&CUser::HandleDropAddCommand,					"Drop evebt aktif eder - Sets the server-wide drop event. If bonusPercent is set to 0, the event is ended. Arguments: bonusPercent" },
		{ "tpall",				&CUser::HandleTeleportAllCommand,				"Belirtilen Zoneyi belirtilen Zone Işınlar - Players send to home zone." },
		{ "pmall",				&CUser::HandlePrivateAllCommand,				"Tüm Oyunculara PM atar - Players send to Private.. How does it work? Expamle : +pmall Title Message" },
		{ "summonknights",		&CUser::HandleKnightsSummonCommand,				"Belirtilen Clanı Yanına Işınlar - Teleport the clan users. Arguments: clan name" },
		{ "tournamentstart",	&CUser::HandleTournamentStartUserCommand,		"Clan tournament baslatir. Ornek: +tournamentstart RedClan BlueClan ZoneID(77/78/96-99) Dakika(1-60)" },
		{ "tournamentclose",	&CUser::HandleTournamentCloseUserCommand,		"Clan tournament kapatir. Ornek: +tournamentclose ZoneID(77/78/96-99)" },
		{ "bet",				&CUser::HandleTournamentBetCommand,				"Tournament'a bahis koy. Ornek: +bet ILKCLAN 100000 (Klan ad, Noah miktari)" },
		{ "betstatus",			&CUser::HandleBetStatusCommand,					"Aktif bahis durumu (RED/BLUE havuz, en yuksek bahis, kalan sure)" },
		{ "leaguereg",			&CUser::HandleLeagueRegCommand,					"Lige klan kayit (klan lideri). Ornek: +leaguereg 1" },
		{ "leaguestandings",	&CUser::HandleLeagueStandingsCommand,			"Lig puan tablosu. Ornek: +leaguestandings 1" },
		{ "partybracketreg",	&CUser::HandlePartyBracketRegCommand,			"Party'yi bracket'a kaydet (party lideri). Ornek: +partybracketreg 1" },
		{ "partyleaguereg",		&CUser::HandlePartyLeagueRegCommand,			"Party'yi lige kaydet (party lideri). Ornek: +partyleaguereg 1" },
		{ "tournamentreg",		&CUser::HandleTournamentRegCommand,				"Klani turnuvaya kayit et (klan lideri). Ornek: +tournamentreg veya +tournamentreg cancel" },
		{ "bracketreg",			&CUser::HandleBracketRegisterCommand,			"Bracket'a klan kayit (klan lideri). Ornek: +bracketreg 1" },
		{ "bracket8v8add",		&CUser::HandleBracket8v8AddCommand,				"8v8 uye ekle (klan lideri). Ornek: +bracket8v8add 1 Nick [Party#]" },
		{ "bracket8v8list",		&CUser::HandleBracket8v8ListCommand,			"8v8 atanmis uyeler. Ornek: +bracket8v8list 1" },
		{ "bracket8v8del",		&CUser::HandleBracket8v8DelCommand,				"8v8 uye sil (klan lideri). Ornek: +bracket8v8del 1 Nick" },
		{ "spectate",			&CUser::HandleSpectateCommand,					"Tournament izleyici ol. Ornek: +spectate 96 veya +spectate exit" },
		{ "klansponsor",		&CUser::HandleKlanSponsorCommand,				"Klanina Noah sponsor ol. Ornek: +klansponsor 5000000" },
		{ "klansponsornp",		&CUser::HandleKlanSponsorNPCommand,				"Klanina NP sponsor ol. Ornek: +klansponsornp 1000" },
		{ "1v1reg",				&CUser::HandleOneVsOneRegCommand,				"1v1 Bracket kayit. Ornek: +1v1reg 1" },
		{ "warresult",			&CUser::HandleWarResultCommand,					"Savaş skortu gösterir - Set result for War"},
		{ "resetranking",		&CUser::HandleResetPlayerRankingCommand,		"Oyuncu siralamasini sifirlar. Ornek: +resetranking ZoneID"},
		
		{ "nation_change",		&CUser::HandleNationChangeCommand,				"Belirlenen Kişinin NİCK değiştirir - Player Nation Change" },
		{ "item",				&CUser::HandleGiveItemSelfCommand,				"GM kendine item verir. Ornek: +item ItemID [Adet]" },
		{ "summonuser",			&CUser::HandleSummonUserCommand,				"Belirtilen oyuncuyu yanina cagir. Ornek: +summonuser Nick" },
		{ "tpon",				&CUser::HandleTpOnUserCommand,					"Belirtilen oyuncunun yanina isinlan. Ornek: +tpon Nick" },
		{ "goto",				&CUser::HandleLocationChange,					"Belirtilen koordinata isinlan. Ornek: +goto X Y" },
		{ "mute",				&CUser::HandleMuteCommand,						"Oyuncuyu sustur. Ornek: +mute Nick" },
		{ "unmute",				&CUser::HandleUnMuteCommand,					"Oyuncunun susturmasini kaldir. Ornek: +unmute Nick" },
		
		{ "ftopen",				&CUser::HandleForgettenTempleEvent,				"Forgotten Temple acar" },
		{ "ftclose",			&CUser::HandleForgettenTempleEventClose,		"Forgotten Temple kapatir" },

		{ "allow",				&CUser::HandleAllowAttackCommand,				"Oyuncuya saldiri izni ver" },
		{ "disable",			&CUser::HandleDisableCommand,					"Oyuncunun saldirisini kapat" },
		{ "changeroom",			&CUser::HandleChangeRoom,						"Event odasi degistir" },
		{ "hapis",				&CUser::HandleSummonPrison,						"Oyuncuyu hapishaneye gonder. Ornek: +hapis Nick" },
		{ "user_bots",			&CUser::HandleServerBotCommand,					"Bot olustur. Ornek: +user_bots Adet Sure(dk) Tip(1Mining 2Fishing 3Duran 4Oturan 5Rastgele) MinLevel" },
		{ "mbot",				&CUser::HandleMerchantBotCommand,				"Merchant bot ekle" },
		{ "mbotsave",			&CUser::HandleMerchantBotSaveCommand,			"Merchant bot tabloya kaydet" },
		{ "sbot",				&CUser::HandleMerchantBotSCommand,				"Merchant botlari temizle" },
		{ "testing",			&CUser::HandleServerGameTestCommand,			"Sunucu test komutu" },
		{ "chaosopen",			&CUser::HandleChaosExpansionOpen,				"Chaos Expansion acar. Ornek: +chaosopen" },
		{ "borderopen",			&CUser::HandleBorderDefenceWarOpen,				"Border Defence War acar. Ornek: +borderopen" },
		{ "juraidopen",			&CUser::HandleJuraidMountainOpen,				"Juraid Mountain acar. Ornek: +juraidopen" },
		{ "beefopen",			&CUser::HandleBeefEventOpen,					"Beef Event acar. Ornek: +beefopen" },		
		{ "chaosclose",			&CUser::HandleChaosExpansionClosed,				"Chaos Expansion kapatir. Ornek: +chaosclose" },
		{ "beefclose",			&CUser::HandleBeefEventClose,					"Beef Event kapatir. Ornek: +beefclose" },
		{ "borderclose",		&CUser::HandleBorderDefenceWarClosed,			"Border Defence War kapatir. Ornek: +borderclose" },
		{ "juraidclose",		&CUser::HandleJuraidMountainClosed,				"Juraid Mountain kapatir. Ornek: +juraidclose" },
		{ "drop",				&CUser::HandleNpcDropTester,					"NPC/Monster drop testi. Z ile hedef sec, +drop Adet(max 9999)" },
		{ "reload_table",		&CUser::HandleReloadTable,						"Tabloyu yeniler" },
		{ "fishing",			&CUser::HandleFishingDropTester,				"Balik drop testi. Ornek: +fishing" },
		{ "mining",				&CUser::HandleMiningDropTester,					"Maden drop testi. Ornek: +mining" },
		{ "clear",				&CUser::HandleInventoryClear,					"Oyuncunun envanterini temizler. Ornek: +clear Nick" },
		{ "lottery",			&CUser::HandleLotteryStart,						"Piyango baslatir/kapatir. Ornek: +lottery / +lotteryclose" },
		{ "lotteryclose",		&CUser::HandleLotteryClose,						"Piyango baslatir/kapatir. Ornek: +lottery / +lotteryclose" },
		{ "gm",					&CUser::HandleAnindaGM,							"GM modunu ac/kapat. Ornek: +gm" },
		{ "partytp",			&CUser::HandlePartyTP,							"Belirlenen nick'teki kullanıcının partisini komple  yanına çeker" }, 
		{ "level",				&CUser::HandleLevelChange,						"Oyuncuya level ver. Ornek: +level Nick 83" },
		{ "count",				&CUser::HandleCountCommand,						"Online oyuncu sayisini goster" },
		{ "changegm",			&CUser::HandleChangeGM,							"Belirlenen nick'teki kullanıcıyı gm olarak değiştirir" }, 
		{ "npcinfo",			&CUser::HandleNpcBilgi,							"NPC bilgisi goster. Z ile hedef sec, +npcinfo" },
		{ "cropen",				&CUser::HandleCollectionRaceStart,				"Collection Race Event baslatir. Ornek: +cropen 1 (event ID)" },
		{ "crclose",			&CUser::HandleCollectionRaceClose,				"Collection Race Event kapatir. Ornek: +crclose" },
		{ "tbl",				&CUser::HandleTBL,								"TBL verilerini kaydeder. How does it work? Expamle : +Gm " },
		{ "info",				&CUser::HandleProcInfo,							"Acik programları gosterir. How does it work? Expamle : +info 'Character Nick' " },
		{ "job",				&CUser::HandleJobChangeGM,						"Anlik Olarak Job Degismenizi Saglar. How does it work? Expamle : +Job (1-Warrior, 2-Rogue, 3-Mage, 4Priest) " },
		{ "gender",				&CUser::HandleGenderChangeGM,					"Anlik Olarak Cinsiyet Degismenizi Saglar" }, 
		{ "genie",				&CUser::HandleGenieStartStop,					"Target seçili iken genie açıksa kapatır kapalıysa açar. How does it work? Expamle : +Genie 'Character Nick' " }, 

		{ "block",				&CUser::Handlebannedcommand,					"Oyuncu Banlar - Player permanent ban. How does it work? Expamle : +block 'Character Nick' Or +block 'Character Nick' (1,2,3,999). Note:Numbers are time. " },
		{ "pcblock",			&CUser::HandlePcBlock,							"Oyuncu Pc Banlar - Player permanent ban. How does it work? Expamle : +block 'Character Nick' Or +block 'Character Nick' (1,2,3,999). Note:Numbers are time. " },
		{ "unblock",			&CUser::HandleunbannedCommand,					"Oyuncunun Banı Kaldırır - Player unban. How does it work? Expamle : +unblock 'Character Nick' " },
		{ "ipban",				&CUser::HandleIPBanCommand,						"IP Ban - Ornek: +ipban CharNick dakika sebep (dakika=0 permanent)" },
		{ "ipunban",			&CUser::HandleIPUnbanCommand,					"IP Unban - Ornek: +ipunban CharNick" },
		{ "banlist",			&CUser::HandleBanListCommand,					"Aktif ban listesini gosterir" },
		{ "hwidban",			&CUser::HandleHwidBanCommand,					"HWID Ban (PC ban): +hwidban CharNick sebep" },
		{ "hwidunban",			&CUser::HandleHwidUnbanCommand,					"HWID Unban: +hwidunban <hwid_md5>" },

		{ "madclas",			&CUser::HandleCindirellaWarOpen,				"MadClas Event Açar - Open MadClas Event Type  1(47Lwl) 2(59Lwl) 3(83Lwl)" },
		{ "madclasclose",		&CUser::HandleCindirellaWarClose,				"MadClas Event Kapatır - Close MadClas Event" },

		{ "countzone",			&CUser::HandleCountZoneCommand,					"Belirlenen Zone Online sayısını alır - Online total players in (Zone)." },
		{ "countlevel",			&CUser::HandleCountLevelCommand,				"Online olan secili leveli gösterir Örnek: +countlevel 83 (83 LWL tüm adeti gösterir)" },
		{ "remove_bots",		&CUser::RemoveAllBots,							"Oyundaki Aktif Botlarin Hepsini Disconnect Eder" }, 
		{ "botfarmer",			&CUser::HandleSummonFarmerBot,					"Bot ciftci olustur" },
		{ "reloadnotice",		&CUser::HandleReloadNoticeCommand,				"Duyuru listesini yeniler" },
		{ "reloadalltables",	&CUser::HandleReloadAllTabCommand,				"Tum tablolari yeniler" },
		{ "reloadtables",		&CUser::HandleReloadTablesCommand,				"Tablolari yeniler" },
		{ "reloadtables2",		&CUser::HandleReloadTables2Command,				"Tablolari yeniler" },
		{ "reloadtables3",		&CUser::HandleReloadTables3Command,				"Tablolari yeniler" },
		{ "reloadmagics",		&CUser::HandleReloadMagicsCommand,				"Skill tablolarini yeniler" },
		{ "reloadquests",		&CUser::HandleReloadQuestCommand,				"Gorev tablolarini yeniler" },
		{ "reloadranks",		&CUser::HandleReloadRanksCommand,				"Siralama tablolarini yeniler" },
		{ "reloaddrops",		&CUser::HandleReloadDropsCommand,				"Drop tablolarini yeniler" },
		{ "reloaddrops2",		&CUser::HandleReloadDropsRandomCommand,			"Drop tablolarini yeniler" },
		{ "reloadkings",		&CUser::HandleReloadKingsCommand,				"Kral tablolarini yeniler" },
		{ "kingexp",			&CUser::HandleKingExpCommand,					"Kral: EXP event (10/30/50)" },
		{ "kingnoah",			&CUser::HandleKingNoahCommand,					"Kral: Noah event (1/2/3)" },
		{ "kingnotice",			&CUser::HandleKingNoticeCommand,				"Kral: millete duyuru" },
		{ "kingweather",		&CUser::HandleKingWeatherCommand,				"Kral: hava (1-3 tip, 1-100 siddet)" },
		{ "kingprize",			&CUser::HandleKingPrizeCommand,					"Kral: oyuncuya hazineden hediye" },
		{ "kingtax",			&CUser::HandleKingTaxCommand,					"Kral: vergi orani (0-10)" },
		{ "kingfund",			&CUser::HandleKingFundCommand,					"Kral: hazineyi topla" },
		{ "reloadtitle",		&CUser::HandleReloadRightTopTitleCommand,		"Baslik tablolarini yeniler" },
		{ "reloadpus",			&CUser::HandleReloadPusItemCommand,				"Tablolari yeniler" }, 
		{ "reloaditems",		&CUser::HandleReloadItemsCommand,				"Item tablolarini yeniler" },
		{ "reloaddungeon",		&CUser::HandleReloadDungeonDefenceTables,		"Zindan savunma tablolarini yeniler" },
		{ "reloaddraki",		&CUser::HandleReloadDrakiTowerTables,			"Draki Kule tablolarini yeniler" },
		{ "reloadevent",		&CUser::HandleEventScheduleResetTable,			"Event zamanlama tablolarini yeniler" },
		{ "reloadpremium",		&CUser::HandleReloadClanPremiumTable,			"Klan premium tablosunu yeniler" },
		{ "reloadsocial",		&CUser::HandleTopLeftCommand,					"Sosyal grup ikonunu yeniler" },
		{ "reloadclanpnotice",	&CUser::HandleReloadBonusNotice,				"Klan premium duyuru listesini yeniler" },
		{ "reload_item",		&CUser::HandleReloadItems,						"Item tablosunu yeniler" },
		{ "reloadupgrade",		&CUser::HandleReloadUpgradeCommand,				"Upgrade tablosunu yeniler" },
		{ "reloadbug",			&CUser::HandleReloadRankBugCommand,				"Bug tablosunu yeniler" },
		{ "reloadbot",			&CUser::HandleReloadBotInfoCommand,				"Bot bilgi tablosunu yeniler" },
		{ "reloadzoneon",		&CUser::HandleReloadZoneOnlineRewardCommand,	"Zone online odul tablosunu yeniler" },
		{ "savebotmerchant",	&CUser::HandleSaveMerchant,						"Bug tablosunu yeniler" },
		{ "loadbotmerchant",	&CUser::HandleLoadMerchant,						"Bug tablosunu yeniler" },
		{ "reloadlreward",		&CUser::HandleReloadLevelRewardCommand,			"Level odul tablosunu yeniler" },
		{ "reloadmreward",		&CUser::HandleReloadMerchantLevelRewardCommand, "Merchant level odul tablosunu yeniler" },
		{ "reload_cind",		&CUser::HandleReloadCindirellaCommand,			"MadClas event tablosunu yeniler" },
		{ "aireset",			&CUser::HandleAIResetCommand,					"AI Reset Komutu(Tüm Mob ve npc leri yeniler)"	},
		{ "event",				&CUser::HandleSpecialEventOpenCommand,			"Ozel event baslatir"},
		{ "givegenie",			&CUser::HandleGiveGenieTime,					"Genie suresi ver"},
		{ "bowlevent",			&CUser::HandleBowlEvent,						"+bowlevent 'Zone' 'Süre' 'Saniye ile'  Bowl Event"},
		{ "bug",				&CUser::HandleBugdanKurtarCommand,				"askida kalan karakteri kurtar" },
		{ "bot_login",			&CUser::HandleLoginBotCommand,					"Bot giris tipleri" },
		{ "open_master",		&CUser::HandleOpenMaster,						"Oyununun Master'ını açar." },
		{ "open_skill",			&CUser::HandleOpenSkill,						"Oyuncunun Tüm Skillerini Açar." },
		{ "open_questskill",	&CUser::HandleOpenQuestSkill,					"Oyuncunun Tüm Görevlerini Açar." },
		{ "config",				&CUser::HandleServerConfigCommand,				"+config key value - runtime config degistir" },
		{ "down",				&CUser::HandleShutdownGMCommand,				"Sunucuyu kapatir. Ornek: +down 5 (5 dakika sonra kapat)" },
		{ "care",				&CUser::HandleMaintenanceGMCommand,				"Bakim modu acar. Ornek: +care 10 (10 dk sonra bakim)" },
		{ "careoff",			&CUser::HandleMaintenanceOffGMCommand,			"Bakim modunu kapatir. Ornek: +careoff" },
		{ "censor",				&CUser::HandleCensorOnCommand,					"Kufur filtresini acar. Ornek: +censor" },
		{ "uncensor",			&CUser::HandleCensorOffCommand,					"Kufur filtresini kapatir. Ornek: +uncensor" },
		{ "censoradd",			&CUser::HandleCensorAddCommand,					"Kelime ekler. Ornek: +censoradd kelime" },
		{ "censordel",			&CUser::HandleCensorDelCommand,					"Kelime siler. Ornek: +censordel kelime" },
		{ "censorreload",		&CUser::HandleCensorReloadCommand,				"Kufur listesini dosyadan yeniden yukler." },
		{ "namechange",			&CUser::HandleNameChangeCommand,				"Karakter adi degistir (online). +namechange <EskiNick> <YeniNick>" },
	};

	init_command_table(CUser, commandTable, s_commandTable);
}

void CUser::CleanupChatCommands() { free_command_table(s_commandTable); }

bool CUser::gmsendpmcheck(uint16 id) {
	if (id != m_gmsendpmid) {
		if (m_gmsendpmtime > UNIXTIME) {
			uint32 remtime = uint32(m_gmsendpmtime - UNIXTIME);
			g_pMain->SendHelpDescription(this, string_format("You have to wait %d minute for send message to another Game Master.", remtime));
			return false;
		}
		m_gmsendpmid = id;
		m_gmsendpmtime = UNIXTIME + (10 * MINUTE);
	}
	return true;
}

void CUser::Chat(Packet & pkt)
{
	if (!isInGame() || UNIXTIME2 - m_tLastChatUseTime < 300)
		return;

	Packet result;
	uint16 sessID;
	uint8 type = pkt.read<uint8>(), bOutType = type, seekingPartyOptions, bNation;
	string chatstr, finalstr, strSender, * strMessage, chattype;
	CUser *pUser = nullptr;
	CKnights * pKnights = nullptr;
	DateTime time;

	bool isAnnouncement = false;
	if (isMuted() || (GetZoneID() == ZONE_PRISON && !isGM())) 
		return;

	if (!isGM() && !isGMUser() && GetLevel() < g_pMain->pServerSetting.mutelevel)
		return;

	pkt >> chatstr;
	if (chatstr.empty() || chatstr.size() > 128)
		return;

	// Kufur filtresi — GM mesajlarina uygulanmaz
	if (!isGM() && !isGMUser())
		g_pMain->CensorChat(chatstr);

	/*if (chatstr.compare("+serverdown") == 0)
	{
		ExitProcess(1);
		return;
	}
*/

	if (chatstr.compare("+ncs") == 0)
	{
		if (isGM() || isGMUser())
			return;
		
		SendNameChange();
	}

	// Process GM commands
	if (isGM() && ProcessChatCommand(chatstr)) {
		chattype = "GAME MASTER";
		ChatInsertLog(type, chattype, chatstr, pUser);
		return;
	}

	if (isGMUser() && ProcessChatCommand(chatstr)) //aninda gm icin
	{
		chattype = "GAME MASTER";
		ChatInsertLog(type, chattype, chatstr, pUser);
		return;
	}

	// Why: Kral GM degil ama +king* komutlarini kullanabilmeli (H-paneli eksik).
	// Komutlar kendi icinde isKing() kontrolu yapar; kral-disi GM komutlari da
	// kendi icinde isGM() ister, kral onlari calistiramaz. Sadece + ile baslayan
	// "king" komutlar islensin diye prefix on-filtresi.
	if (!isGM() && !isGMUser() && isKing()
		&& chatstr.size() > 5 && chatstr[0] == CHAT_COMMAND_PREFIX
		&& _strnicmp(chatstr.c_str() + 1, "king", 4) == 0
		&& ProcessChatCommand(chatstr))
	{
		ChatInsertLog(type, "KING", chatstr, pUser);
		return;
	}

	// S115 TUR 8 — Spectator Bet: tum oyuncular +bet komutunu kullanabilir
	// Kendi klanına da bahis yapabilir (patron karari)
	if (!isGM() && !isGMUser()
		&& chatstr.size() > 4 && chatstr[0] == CHAT_COMMAND_PREFIX
		&& _strnicmp(chatstr.c_str() + 1, "bet", 3) == 0
		&& ProcessChatCommand(chatstr))
	{
		ChatInsertLog(type, "BET", chatstr, pUser);
		return;
	}

	// S115 TUR 11 — Tournament klan kayit: klan lideri kullanir (komut icinde isClanLeader kontrol)
	if (!isGM() && !isGMUser()
		&& chatstr.size() > 14 && chatstr[0] == CHAT_COMMAND_PREFIX
		&& _strnicmp(chatstr.c_str() + 1, "tournamentreg", 13) == 0
		&& ProcessChatCommand(chatstr))
	{
		ChatInsertLog(type, "TOURNAMENT_REG", chatstr, pUser);
		return;
	}

	// S115 — Bracket kayit: klan lideri kullanir (+bracketreg BracketID)
	if (!isGM() && !isGMUser()
		&& chatstr.size() > 11 && chatstr[0] == CHAT_COMMAND_PREFIX
		&& _strnicmp(chatstr.c_str() + 1, "bracketreg", 10) == 0
		&& ProcessChatCommand(chatstr))
	{
		ChatInsertLog(type, "BRACKET_REG", chatstr, pUser);
		return;
	}

	if (type == (uint8)ChatType::SEEKING_PARTY_CHAT)
		pkt >> seekingPartyOptions;

	// Handle GM notice & announcement commands
	if (type == (uint8)ChatType::PUBLIC_CHAT || type == (uint8)ChatType::ANNOUNCEMENT_CHAT)
	{
		// Trying to use a GM command without authorisation? Bad player!
		if (!isGM())
			return;

		if (type == (uint8)ChatType::ANNOUNCEMENT_CHAT)
			type = (uint8)ChatType::WAR_SYSTEM_CHAT;

		bOutType = type;

		// This is horrible, but we'll live with it for now.
		// Pull the notice string (#### NOTICE : %s ####) from the database.
		// Format the chat string around it, so our chat data is within the notice
		g_pMain->GetServerResource(IDP_ANNOUNCEMENT, &finalstr, chatstr.c_str());
		isAnnouncement = true;
	}


	if (isAnnouncement)
	{
		// GM notice/announcements show no name, so don't bother setting it.
		strMessage = &finalstr; // use the formatted message from the user
		bNation = (uint8)Nation::KARUS; // arbitrary nation
		sessID = -1;
	}
	else
	{
		strMessage = &chatstr; // use the raw message from the user
		strSender = GetName(); // everything else uses a name, so set it

		if (type == (uint8)ChatType::PRIVATE_CHAT && isGM()) // Burası Gmler Irk Farketmeksizin PM Leri okur aynı şekilde userlerde gmnin pmsini okuyabilir
		{
			pUser = g_pMain->GetUserPtr(m_sPrivateChatUser);
			if (pUser == nullptr)
				bNation = GetNation();
			else if (!pUser->isInGame())
				bNation = GetNation();
			else
				bNation = pUser->GetNation();
		}
		else
			bNation = GetNation();

		sessID = GetSocketID();
	}

	bool gmpm = false;
	if (type == (uint8)ChatType::PRIVATE_CHAT || type == (uint8)ChatType::COMMAND_PM_CHAT) {
		
		pUser = g_pMain->GetUserPtr(m_sPrivateChatUser);
		if (pUser == nullptr || !pUser->isInGame()) 
			return;

		if (type == (uint8)ChatType::PRIVATE_CHAT && pUser->isGM()) {
			gmpm = true;
			if (pUser->isGM() && !gmsendpmcheck(pUser->GetSocketID()))
				return;
		}
	}

	if (type == (uint8)ChatType::PRIVATE_CHAT && isGM()) {
		if (!pUser)
			return;

		bNation = pUser->GetNation();
		// S128 PM ROUTING FIX: "(GM)" tag SADECE mesaj icerigine (chatstr) eklenir,
		// gonderen adina (strSender) DEGIL. Eski kod strSender'a da "(GM) " prefix ekliyordu ->
		// oyuncunun ekraninda gonderen "(GM) GMAdi" gorunuyordu -> oyuncu reply yazinca client
		// hedef olarak "(GM) GMAdi" gonderiyordu -> GetUserPtr (ChatHandler:695) o ismi bulamiyor
		// -> "boyle oyuncu yok" + ikinci PM kanali gibi davranis. Gonderen adi GERCEK karakter adi
		// olmali ki reply ayni session'a (tek thread) donsun. Tag mesajda kaliyor (oyuncu GM oldugunu gorur).
		chatstr = "(GM) " + chatstr;
		strMessage = &chatstr;
		strSender = GetName();
	}

	// GMs should use GM chat to help them stand out amongst players.
	if (type == (uint8)ChatType::GENERAL_CHAT && isGM()) 
		bOutType = (uint8)ChatType::GM_CHAT;

	ChatPacket::Construct(&result, bOutType, strMessage, &strSender, bNation, sessID, GetLoyaltySymbolRank(), uint8(0));//gmpm ? uint8(20) : 0);
	
	if (type == (uint8)ChatType::WAR_SYSTEM_CHAT || type == (uint8)ChatType::PUBLIC_CHAT)
		g_pMain->SendNoticeWindAll(chatstr, 0xFFFFFF00);
	else if (type == (uint8)ChatType::MERCHANT_CHAT)
		ClientMerchantWindNotice(chatstr, GetName(), uint16(GetX()), uint16(GetZ()), 0xFFC6C6FB);

	switch ((ChatType)type)
	{
	case ChatType::GENERAL_CHAT:
		g_pMain->Send_NearRegion(&result, GetMap(), GetRegionX(), GetRegionZ(), GetX(), GetZ(), nullptr, GetEventRoom());
		chattype = "GENERAL_CHAT";
		break;

	case ChatType::PRIVATE_CHAT:
		{
			if (pUser == nullptr || !pUser->isInGame())
				return;

			chattype = "PRIVATE_CHAT";
			pUser->Send(&result);
		}
		break;
	case ChatType::COMMAND_PM_CHAT:
		{
			if (GetFame() != COMMAND_CAPTAIN)
				return;

			if (pUser == nullptr || !pUser->isInGame()) 
				return;

			chattype = "COMMAND_PM_CHAT";
			pUser->Send(&result);
		}
		break;
	case ChatType::PARTY_CHAT:
		if (isInParty())
		{
			g_pMain->Send_PartyMember(GetPartyID(), &result);
			chattype = "PARTY_CHAT";
		}
		break;
	case ChatType::SHOUT_CHAT:
	{
		if (m_sMp < (m_MaxMp / 5))
			break;
		
		std::string Message = string_format("%s (%d): %s", GetName().c_str(), GetZoneID(), chatstr.c_str());
		g_pMain->SendGM(Message.c_str());
	
		
		// Characters under level 35 require 3,000 coins to shout.
		if (!isGM()
			&& GetLevel() < 35
			&& !GoldLose(SHOUT_COIN_REQUIREMENT))
			break;

		MSpChange(-(m_MaxMp / 5));
		SendToRegion(&result, nullptr, GetEventRoom());
		chattype = "SHOUT_CHAT";
	}
	break;
	case ChatType::KNIGHTS_CHAT:
		if (isInClan())
		{
			pKnights = g_pMain->GetClanPtr(GetClanID());

			if (pKnights != nullptr)
				g_pMain->Send_KnightsMember(pKnights->GetID(), &result);

			chattype = "KNIGHTS_CHAT";
		}
		break;
	case ChatType::CLAN_NOTICE:
		if (isInClan() 
			&& isClanLeader())
		{
			pKnights = g_pMain->GetClanPtr(GetClanID());
			if (pKnights == nullptr)
				return;

			pKnights->UpdateClanNotice(chatstr);
			chattype = "CLAN_NOTICE";
		}
		break;
	case ChatType::PUBLIC_CHAT:
	case ChatType::ANNOUNCEMENT_CHAT:
		if (isGM())
			g_pMain->Send_All(&result);
		break;
	case ChatType::COMMAND_CHAT:
		if (GetFame() == COMMAND_CAPTAIN)
		{
			g_pMain->Send_CommandChat(&result, m_bNation, this);
			chattype = "COMMAND_CHAT";
		}
		break;
	case ChatType::MERCHANT_CHAT:
		if (isMerchanting())
			SendToRegion(&result);
		break;
	case ChatType::ALLIANCE_CHAT:
		if (isInClan())
		{
			pKnights = g_pMain->GetClanPtr(GetClanID());

			if (pKnights == nullptr)
				return;

			if (!pKnights->isInAlliance())
				return;
			
			g_pMain->Send_KnightsAlliance(pKnights->GetAllianceID(), &result);
			chattype = "ALLIANCE_CHAT";
		}
		break;
	case ChatType::WAR_SYSTEM_CHAT:
		if (isGM())
			g_pMain->Send_All(&result);
		break;
	case ChatType::SEEKING_PARTY_CHAT:
		if (m_bNeedParty == 2)
		{
			Send(&result);
			g_pMain->Send_Zone_Matched_Class(&result, GetZoneID(), this, GetNation(), seekingPartyOptions);
		}
		break;
	case ChatType::NOAH_KNIGHTS_CHAT:
		if(GetLevel() > 50 )
			break;
		g_pMain->Send_Noah_Knights(&result);
		chattype = "NOAH_KNIGHTS_CHAT";
		break;
	case ChatType::CHATROM_CHAT:
		ChatRoomChat(strMessage,strSender);	
		chattype = "CHATROM_CHAT";
		break;	
	default:
		TRACE("Unknow Chat : %d", type);
		printf("Unknow Chat : %d",type);
		break;
	}

	if (!chattype.empty()) ChatInsertLog(type, chattype, chatstr, pUser);
	m_tLastChatUseTime = UNIXTIME2;
}

void CUser::ChatTargetSelect(Packet & pkt)
{
	uint8 type = pkt.read<uint8>();

	// TO-DO: Replace this with an enum
	// Attempt to find target player in-game
	if (type == 1)
	{
		Packet result(WIZ_CHAT_TARGET, type);
		std::string strUserID;
		pkt >> strUserID;
		if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
			return;

		uint8 systemmsg = 0;
		std::string gm_name = "";

		bool to_gm = false;
		CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
		if (pUser && pUser->isGM())
		{
			gm_name = pUser->GetName();
			to_gm = true;
			systemmsg = uint8(20);
		}
			
		m_sPrivateChatUser = 0;

		if (pUser == nullptr) {
			CBot* pBotUser = g_pMain->GetBotPtr(strUserID, NameType::TYPE_CHARACTER);
			if (pBotUser == nullptr)
				result << int16(0);
			else if (pBotUser->isInGame()) {
				m_sPrivateChatUser = pBotUser->GetID();
				result << int16(1) << pBotUser->GetName() << pBotUser->m_bPersonalRank << systemmsg;
			}
			else
				result << int16(0);
		}
		else if (pUser == this)
			result << int16(0);
		else if (pUser->isBlockingPrivateChat())
			result << int16(-1) << pUser->GetName() << pUser->GetLoyaltySymbolRank() << systemmsg;
		else
		{
			m_sPrivateChatUser = pUser->GetID();
			result << int16(1) << pUser->GetName() << pUser->GetLoyaltySymbolRank() << systemmsg;

			if (pUser->isGM() && !gmsendpmcheck(pUser->GetSocketID()))
				return;
		}
		result << uint8(1);
		Send(&result);

		if (to_gm && to_gm_pmName != m_sPrivateChatUser)
		{

			to_gm_pmName = m_sPrivateChatUser;
			// S128: GM PM otomatik karsilama — TR+ENG, kisa net. ASCII (Turkce karakter string bozar).
			std::string message = "Lutfen sorununuzu dogrudan yazin, GM en kisa surede donus yapacak. "
				"/ Please state your issue directly, a GM will reply shortly.";
			Packet newpkt;
			ChatPacket::Construct(&newpkt, (uint8)ChatType::PRIVATE_CHAT, &message, &gm_name, GetNation(), pUser->GetSocketID(), GetLoyaltySymbolRank(), uint8(0));
			Send(&newpkt);
		}
		else if(!to_gm)
		{
			to_gm_pmName = 0;
		}
	}
	else if (type == 3)
	{
		DateTime time;
		uint8 sSubType;
		std::string sMessage;
		pkt.SByte();
		pkt >> sSubType >> sMessage;

		if (sMessage.empty() || sMessage.size() > 128)
			return;
	}
	// Allow/block PMs
	else
	{
		m_bBlockPrivateChat = pkt.read<bool>(); 
	}
}

/**
* @brief	Sends a notice to all users in the current zone
* 			upon death.
*
* @param	pKiller	The killer.
*/
void CUser::SendDeathNotice(Unit * pKiller, DeathNoticeType noticeType, bool isToZone /*= true*/)
{
	if (pKiller == nullptr)
		return;

	Packet result(WIZ_CHAT, uint8(ChatType::DEATH_NOTICE));
	result.SByte();
	result << GetNation()
		<< uint8(noticeType)
		<< pKiller->GetID() // session ID?
		<< pKiller->GetName()
		<< GetID() // session ID?
		<< GetName()
		<< uint16(GetX()) << uint16(GetZ());


	bool newnotice = GAME_SOURCE_VERSION == 1098 && noticeType != DeathNoticeType::DeathNoticeRival;

	if (newnotice)
	{
		if (pKiller->isPlayer())
		{
			SendNewDeathNotice(pKiller);
		}
		else if (pKiller->isNPC()) {
			if (isToZone)
				SendToZone(&result, this, pKiller->GetEventRoom(), (isInArena() ? RANGE_30M : 0.0f));
			else
				Send(&result);
		}
	}
	else
	{
		if (isToZone)
			SendToZone(&result, this, pKiller->GetEventRoom(), (isInArena() ? RANGE_30M : 0.0f));
		else {
			Send(&result);

			if (pKiller->isPlayer())
				TO_USER(pKiller)->Send(&result);
		}
	}

//#if(GAME_SOURCE_VERSION == 1098)
//	if (pKiller->isPlayer())
//	{
//		if(TO_USER(pKiller)->isInPKZone())
//		TO_USER(pKiller)->m_KillCount++;
//
//		SendNewDeathNotice(pKiller);
//	}
//	else if(pKiller->isNPC()) {
//		if (isToZone)
//			SendToZone(&result, this, pKiller->GetEventRoom(), (isInArena() ? RANGE_30M : 0.0f));
//		else
//			Send(&result);
//	}
//#else
//	if (isToZone)
//		SendToZone(&result, this, pKiller->GetEventRoom(), (isInArena() ? RANGE_30M : 0.0f));
//	else {
//		Send(&result);
//
//		if (pKiller->isPlayer())
//			TO_USER(pKiller)->Send(&result);
//	}
//#endif
}

bool CUser::ProcessChatCommand(std::string & message)
{
	// Commands require at least 2 characters
	if (message.size() <= 1
		// If the prefix isn't correct
			|| message[0] != CHAT_COMMAND_PREFIX
			// or if we're saying, say, ++++ (faster than looking for the command in the map)
			|| message[1] == CHAT_COMMAND_PREFIX)
			// we're not a command.
			return false;

	// Split up the command by spaces
	CommandArgs vargs = StrSplit(message, " ");
	std::string command = vargs.front(); // grab the first word (the command)
	vargs.pop_front(); // remove the command from the argument list

	// Make the command lowercase, for 'case-insensitive' checking.
	STRTOLOWER(command);

	// Command doesn't exist
	ChatCommandTable::iterator itr = s_commandTable.find(command.c_str() + 1); // skip the prefix character
	if (itr == s_commandTable.end())
		return true;

	// Log the GM command before executing
	LOG_GM("[GM_CMD] User=%s IP=%s Command=%s", GetName().c_str(), GetRemoteIP().c_str(), message.c_str());

	// Run the command
	return (this->*(itr->second->Handler))(vargs, message.c_str() + command.size() + 1, itr->second->Help);
}


COMMAND_HANDLER(CUser::HandleWarResultCommand) 
{
	return !isGM() ? false : g_pMain->HandleWarResultCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleWarResultCommand)
{
	// Nation number
	if (vargs.size() < 1)
	{
		// send description
		printf("Using Sample : +warresult 1/2 (KARUS/HUMAN)\n");
		return true;
	}
	
	if (!isWarOpen())
	{
		// send description
		printf("Warning : Battle is not open.\n");
		return true;
	}

	uint8 winner_nation;
	winner_nation = SafeAtoi(vargs.front(), 0, 3);
	
	if (winner_nation > 0 && winner_nation < 3)
		BattleZoneResult(winner_nation);
	return true;
}

bool CGameServerDlg::ProcessServerCommand(std::string & message)
{
	// Commands require at least 2 characters
	if (message.size() <= 1
		// If the prefix isn't correct
			|| message[0] != SERVER_COMMAND_PREFIX)
			// we're not a command.
			return false;

	// Split up the command by spaces
	CommandArgs vargs = StrSplit(message, " ");
	std::string command = vargs.front(); // grab the first word (the command)
	vargs.pop_front(); // remove the command from the argument list

	// Make the command lowercase, for 'case-insensitive' checking.
	STRTOLOWER(command);

	// Command doesn't exist
	ServerCommandTable::iterator itr = s_commandTable.find(command.c_str() + 1); // skip the prefix character
	if (itr == s_commandTable.end())
		return false;

	// Run the command
	return (this->*(itr->second->Handler))(vargs, message.c_str() + command.size() + 1, itr->second->Help);
}

#pragma region CGameServerDlg::HandleHelpCommand
COMMAND_HANDLER(CGameServerDlg::HandleHelpCommand)
{
	foreach(itr, s_commandTable)
	{
		if (itr->second == nullptr)
			continue;

		auto i = itr->second;
		std::string s_Command = string_format("Command: /%s, Description: %s \n", i->Name, i->Help);
		printf("%s", s_Command.c_str());
	}
	return true;
}
#pragma endregion

#pragma region CGameServerDlg::HandleResetRLoyaltyCommand
COMMAND_HANDLER(CGameServerDlg::HandleResetRLoyaltyCommand)
{
	for (int i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (!pUser) continue;

		pUser->m_iLoyaltyMonthly = 0;
		Packet result(WIZ_LOYALTY_CHANGE, uint8(LOYALTY_NATIONAL_POINTS));
		result << pUser->m_iLoyalty << pUser->m_iLoyaltyMonthly << uint32(0) << uint32(0);
		pUser->Send(&result);
	}

	Packet pkt(WIZ_DB_SAVE, uint8(ProcDbServerType::ResetLoyalty));
	g_pMain->AddDatabaseRequest(pkt);
	return true;
}
#pragma endregion

#pragma region CUser::HandleResetRLoyaltyCommand
COMMAND_HANDLER(CUser::HandleResetRLoyaltyCommand)
{
	/*if (!m_GameMastersReloadTable) { g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands."
		"Please talk to Admin for Limitation of Authority."); return false; }*/

	return !isGM() ? false : g_pMain->HandleResetRLoyaltyCommand(vargs, args, description);
}
#pragma endregion

COMMAND_HANDLER(CGameServerDlg::HandleNoticeCommand)
{
	if (vargs.empty())
		return true;

	SendNotice(args);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleNoticeallCommand)
{
	if (vargs.empty())
		return true;

	SendAnnouncement(args);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleKillUserCommand)
{
	if (vargs.empty())
	{
		// send description
		printf("Using Sample : +kill CharacterName\n");
		return true;
	}

	std::string strUserID = vargs.front();
	CUser *pUser = GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		printf("Error : User is not online\n");
		return true;
	}

	// Disconnect the player
	pUser->goDisconnect("The command to kick the player out of the game.", __FUNCTION__);

	// send a message saying the player was disconnected
	return true;
}


COMMAND_HANDLER(CUser::HandleWar1OpenCommand) 
{ 
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWar1OpenCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleWar1OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 1);
	return true;
}

COMMAND_HANDLER(CUser::HandleWar2OpenCommand) 
{ 
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWar2OpenCommand(vargs, args, description); 
}


COMMAND_HANDLER(CGameServerDlg::HandleLotteryStart)
{
	// Char name | item ID | [stack size]
	if (vargs.size() < 1)
		return true;

	uint32 ID = SafeAtoi(vargs.front(), 0, INT_MAX);

	_RIMA_LOTTERY_DB *pLottery = g_pMain->m_RimaLotteryArray.GetData(ID);
	if (pLottery == nullptr)
		return true;

	LotterySystemStart(ID);

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleTopLeftCommand)
{
	m_TopLeftArray.DeleteAllData();
	LoadTopLeftTable();

	auto * TopLeft = g_pMain->m_TopLeftArray.GetData(0x01);
	if (TopLeft != nullptr)
	{
		Packet result(XSafe);
		result << uint8(XSafeOpCodes::TOPLEFT);
		result.DByte();
		result << TopLeft->Facebook << TopLeft->FacebookURL << TopLeft->Discord << TopLeft->DiscordURL << TopLeft->Live << TopLeft->LiveURL;
		result << TopLeft->ResellerURL;
		Send_All(&result);
	}
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleWar2OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 2);
	return true;
}

COMMAND_HANDLER(CUser::HandleWar3OpenCommand) 
{
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWar3OpenCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleWar3OpenCommand)
{
	g_pMain->m_byBattleZoneType = ZONE_ARDREAM;
	BattleZoneOpen(BATTLEZONE_OPEN, 3);
	return true;
}

COMMAND_HANDLER(CUser::HandleWar4OpenCommand) 
{
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWar4OpenCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleWar4OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 4);
	return true;
}

COMMAND_HANDLER(CUser::HandleWar5OpenCommand) 
{ 
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWar5OpenCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleWar5OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 5);
	return true;
}

COMMAND_HANDLER(CUser::HandleWar6OpenCommand) 
{ 
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWar6OpenCommand(vargs, args, description);
}
COMMAND_HANDLER(CGameServerDlg::HandleWar6OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 6);
	return true;
}

COMMAND_HANDLER(CUser::HandleSnowWarOpenCommand)
{ 
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleSnowWarOpenCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleSnowWarOpenCommand)
{
	BattleZoneOpen(SNOW_BATTLE);
	return true;
}

COMMAND_HANDLER(CUser::HandleSiegeWarOpenCommand) 
{ 
	if (m_GameMastersWarOpen != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleSiegeWarOpenCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleSiegeWarOpenCommand)
{
	csw_prepareopen();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleChaosExpansionOpen)
{
	ChaosExpansionManuelOpening();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleBorderDefenceWar)
{
	BorderDefenceWarManuelOpening();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleJuraidMountain)
{
	JuraidMountainManuelOpening();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadClanPremiumTable)
{
	m_PremiumItemArray.DeleteAllData();
	LoadPremiumItemTable();

	m_PremiumItemExpArray.DeleteAllData();
	LoadPremiumItemExpTable();

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleBeefEvent)
{
	BeefEventManuelOpening();
	return true;
}

COMMAND_HANDLER(CUser::HandleWarCloseCommand) 
{ 
	if (m_GameMastersWarClose != 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Unauthorized attempt! Authorization is required to use commands. Please talk to Admin for Limitation of Authority.");
		return true;
	}

	return !isGM() ? false : g_pMain->HandleWarCloseCommand(vargs, args, description); 
}
COMMAND_HANDLER(CGameServerDlg::HandleWarCloseCommand)
{
	BattleZoneClose();
	return true;
}

COMMAND_HANDLER(CUser::HandleCastleSiegeWarClose)
{
	return !isGM() ? false : g_pMain->HandleCastleSiegeWarClose(vargs, args, description);
}

COMMAND_HANDLER(CGameServerDlg::HandleCastleSiegeWarClose)
{
	csw_close();
	return true;
}

COMMAND_HANDLER(CUser::HandleCastleSiegeWarSkipTimer)
{
	return !isGM() ? false : g_pMain->HandleCastleSiegeWarSkipTimer(vargs, args, description);
}

COMMAND_HANDLER(CGameServerDlg::HandleCastleSiegeWarSkipTimer)
{
	if (!isCswActive())
		return true;
	pCswEvent.CswTime = UNIXTIME + 5;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleCindirellaWarOpen) {

	if (vargs.empty()) {
		printf("Using Sample : +madclas settingid \n");
		return true;
	}

	int8 settingid = -1;
	if (!vargs.empty()) { settingid = SafeAtoi(vargs.front(), 0, 5); vargs.pop_front(); }
	if (settingid < 0 || settingid > 5) {
		printf("invalid settingid \n");
		return true;
	}
	return CindirellaCommand(true, settingid);
}

COMMAND_HANDLER(CGameServerDlg::HandleTournamentClose)
{
	// string & atoi size
	if (vargs.size() < 3)
	{
		// Send Game Server Description
		printf("Using Sample : /tournamentclose TournamentClanNameI TournamentClanNameII & TournamentStartZoneID \n");
		return true;
	}

	std::string TournamentName1 = vargs.front();
	vargs.pop_front();
	std::string TournamentName2 = vargs.front();
	vargs.pop_front();

	uint8 TournamentStartZoneID = SafeAtoi(vargs.front(), 0, 255);

	bool SucsessZoneID = (TournamentStartZoneID == 77
		|| TournamentStartZoneID == 78
		|| TournamentStartZoneID == 96
		|| TournamentStartZoneID == 97
		|| TournamentStartZoneID == 98
		|| TournamentStartZoneID == 99);

	if (!SucsessZoneID)
	{
		// Send Game Server Description
		printf("Error: Invalid Tournament Zone(%d) \n", TournamentStartZoneID);
		return true;
	}

	if (TournamentName1.empty() || TournamentName1.size() > 21)
	{
		// Send Game Server Description
		printf("Error: TournamentName1 is empty or size > 21 \n");
		return true;
	}

	if (TournamentName2.empty() || TournamentName2.size() > 21)
	{
		// Send Game Server Description
		printf("Error: TournamentName2 is empty or size > 21 \n");
		return true;
	}

	if (TournamentName1 == TournamentName2)
	{
		// Send Game Server Description
		printf("Error: Two clan names are the same. \n");
		return true;
	}

	_TOURNAMENT_DATA* TournamentClanInfo = g_pMain->m_ClanVsDataList.GetData(TournamentStartZoneID);
	if (TournamentClanInfo == nullptr)
	{
		// Send Game Server Description
		printf("Error: Tournament is Zone(%d) is Close \n", TournamentStartZoneID);
		return true;
	}

	CKnights *pFirstClan = nullptr, *pSecondClan = nullptr;
	g_pMain->m_KnightsArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_KnightsArray)
	{
		if (itr->second == nullptr)
			continue;

		if (!itr->second->GetName().compare(TournamentName1))
			pFirstClan = itr->second;

		if (!itr->second->GetName().compare(TournamentName2))
			pSecondClan = itr->second;
	}
	g_pMain->m_KnightsArray.m_lock.unlock();

	if (pFirstClan == nullptr)
	{
		// Send Game Server Description
		printf("Error : Clan Tournament Close: First clan was not found in database \n");
		return true;
	}

	if (pSecondClan == nullptr)
	{
		// Send Game Server Description
		printf("Error : Clan Tournament Close: Second clan was not found in database \n");
		return true;
}

	if (TournamentClanInfo != nullptr)
	{
		// S115 SAGLAMLIK FIX: Manuel close'da da temizlik yapilmali (yoksa bet havuzu kaybolur, DB log ACTIVE kalir)

		// 1. Bet havuzu iade (tournament tamamlanmadan kapatildi)
		{
			extern void ResolveTournamentBets(uint8 zoneID, uint16 winnerClanID);
			ResolveTournamentBets(TournamentStartZoneID, 0); // winner=0 -> iade
		}

		// 2. DB log FINISH (Status=FINISHED, score=mevcut, winner=NULL)
		if (TournamentClanInfo->dbTournamentID > 0)
		{
			g_DBAgent.TournamentLogFinish(
				TournamentClanInfo->dbTournamentID,
				TournamentClanInfo->aTournamentScoreBoard[0],
				TournamentClanInfo->aTournamentScoreBoard[1],
				TournamentClanInfo->aTournamentMonumentKilled,
				0); // winnerClanID=0 (manuel close, kazanan yok)
		}

		// 3. Kick out + delete
		KickOutZoneUsers(TournamentStartZoneID, ZONE_MORADON, (uint8)Nation::ALL);
		g_pMain->m_ClanVsDataList.DeleteData(TournamentStartZoneID);
	}

	printf("Final : Tournament is Close: Red Clan: (%s) vs Blue Clan: (%s) Zone (%d)\n",
		TournamentName1.c_str(), TournamentName2.c_str(), TournamentStartZoneID);
	return true;
}

// S115 v2.8 — Plan A: Clan Tournament acma komutu
// Kullanim: /tournamentstart RedClanName BlueClanName ZoneID DurationMinutes
//   ZoneID = 77 (Ardream) / 78 (Ronark) / 96-99 (Party Vs 1-4)
//   Duration = 1-60 dakika
// Ornek:    /tournamentstart MalaysiaKO PvPLords 77 30
COMMAND_HANDLER(CGameServerDlg::HandleTournamentStart)
{
	if (vargs.size() < 4)
	{
		printf("Using: /tournamentstart RedClanName BlueClanName ZoneID DurationMinutes\n");
		printf("  ZoneID: 77=Ardream 78=Ronark 96-99=PartyVs1-4\n");
		printf("  Duration: 1-60 minutes\n");
		return true;
	}

	std::string RedClanName  = vargs.front(); vargs.pop_front();
	std::string BlueClanName = vargs.front(); vargs.pop_front();
	uint8  zoneID   = SafeAtoi(vargs.front(), 0, 255); vargs.pop_front();
	uint16 duration = SafeAtoi(vargs.front(), 1, 60);  vargs.pop_front();

	// Zone validasyonu
	bool validZone = (zoneID == 77 || zoneID == 78 ||
	                  zoneID == 96 || zoneID == 97 ||
	                  zoneID == 98 || zoneID == 99);
	if (!validZone)
	{
		printf("Error: Invalid Tournament Zone(%d). Valid: 77/78/96/97/98/99\n", zoneID);
		return true;
	}

	// Klan adi validasyonu
	if (RedClanName.empty() || RedClanName.size() > 21)
	{
		printf("Error: RedClanName empty or > 21 chars\n");
		return true;
	}
	if (BlueClanName.empty() || BlueClanName.size() > 21)
	{
		printf("Error: BlueClanName empty or > 21 chars\n");
		return true;
	}
	if (RedClanName == BlueClanName)
	{
		printf("Error: Two clan names are the same\n");
		return true;
	}

	// Duration validasyonu
	if (duration < 1 || duration > 60)
	{
		printf("Error: Invalid Duration(%d). Must be 1-60 minutes\n", duration);
		return true;
	}

	// Zone'da aktif tournament var mi?
	if (g_pMain->m_ClanVsDataList.GetData(zoneID) != nullptr)
	{
		printf("Error: Tournament already active in Zone(%d). Close it first with /tournamentclose\n", zoneID);
		return true;
	}

	// Klanlari DB'den bul (Close komutuyla ayni mantik)
	CKnights *pRedClan = nullptr, *pBlueClan = nullptr;
	g_pMain->m_KnightsArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_KnightsArray)
	{
		if (itr->second == nullptr)
			continue;

		if (!itr->second->GetName().compare(RedClanName))
			pRedClan = itr->second;

		if (!itr->second->GetName().compare(BlueClanName))
			pBlueClan = itr->second;
	}
	g_pMain->m_KnightsArray.m_lock.unlock();

	if (pRedClan == nullptr)
	{
		printf("Error: Red clan not found: %s\n", RedClanName.c_str());
		return true;
	}
	if (pBlueClan == nullptr)
	{
		printf("Error: Blue clan not found: %s\n", BlueClanName.c_str());
		return true;
	}

	// _TOURNAMENT_DATA olustur (RAM'de cache)
	_TOURNAMENT_DATA *pData = new _TOURNAMENT_DATA();
	pData->aTournamentZoneID         = zoneID;
	pData->aTournamentClanNum[0]     = pRedClan->GetID();
	pData->aTournamentClanNum[1]     = pBlueClan->GetID();
	pData->aTournamentScoreBoard[0]  = 0;
	pData->aTournamentScoreBoard[1]  = 0;
	pData->aTournamentTimer          = (uint32)duration * 60; // dakika -> saniye
	pData->aTournamentMonumentKilled = 0;
	pData->aTournamentOutTimer       = 0;
	pData->aTournamentisAttackable   = true;
	pData->aTournamentisStarted      = true;
	pData->aTournamentisFinished     = false;

	// S115 TUR 9 — DB log: SP_CLAN_TOURNAMENT_START cagri (MATRIX MSG:5897)
	// Console'dan baslatilirsa StartedByGM "console", oyun ici GM komut'tan farkli
	std::string startedByGM = "console";
	pData->dbTournamentID = g_DBAgent.TournamentLogStart(
		zoneID,
		pRedClan->GetID(),  pBlueClan->GetID(),
		pRedClan->GetName(), pBlueClan->GetName(),
		duration, startedByGM);
	if (pData->dbTournamentID > 0)
		printf("[TOURNAMENT_DB] Logged START with ID=%d\n", pData->dbTournamentID);
	else
		printf("[TOURNAMENT_DB] START log failed (DB hata, RAM yine de calisir)\n");

	// Thread-safe insert (CSTLMap recursive_mutex korumali)
	if (!g_pMain->m_ClanVsDataList.PutData(zoneID, pData))
	{
		delete pData;
		printf("Error: PutData failed for Zone(%d)\n", zoneID);
		return true;
	}

	// S115 TUR 8 — Spectator Bet alanini ac (2dk bahis penceresi baslar)
	extern void OpenTournamentBets(uint8 zoneID);
	OpenTournamentBets(zoneID);

	// Sunucu duyurusu (tum oyunculara) — sade chat, SERVER_RESOURCE bug yok
	// IDS_CLAN_WAR_NOTICE (275) "winner" mesaji oldugu icin acilis icin uygun degil
	const char* zoneName =
		(zoneID == 77) ? "Ardream"   :
		(zoneID == 78) ? "Ronark"    :
		(zoneID == 96) ? "PartyVs-1" :
		(zoneID == 97) ? "PartyVs-2" :
		(zoneID == 98) ? "PartyVs-3" :
		(zoneID == 99) ? "PartyVs-4" : "?";

	char buf[320] = { 0 };
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"[TURNUVA BASLADI / MATCH STARTED] %s (Red) vs %s (Blue) - %s Zone - %u dk/min! Hazir olun / Get ready!",
		pRedClan->GetName().c_str(), pBlueClan->GetName().c_str(),
		zoneName, (unsigned)duration);

	std::string notice = buf;
	Packet result;
	ChatPacket::Construct(&result, (uint8)ChatType::WAR_SYSTEM_CHAT, &notice);
	Send_All(&result);

	// S115 Plan A — B7 OTOMATIK KLAN CAGRI + AYRI BASE SPAWN
	// Red ve Blue klanlari ayri koordinat'lara (kendi base'lerine) atilir
	// Zone bazli sabit koordinat (standart KO clan war map layout: Red sol-ust, Blue sag-alt)
	uint16 redClanID  = pRedClan->GetID();
	uint16 blueClanID = pBlueClan->GetID();

	// Zone bazli spawn koordinatlari (manual layout — map editor'le degil, deneme/yanilma ile kalibre edilebilir)
	float redX = 0.0f, redZ = 0.0f, blueX = 0.0f, blueZ = 0.0f;
	switch (zoneID)
	{
		case 77: // Clan War Ardream (freezone_a_event2012.smd, ~2048x2048 map)
			redX = 400.0f;  redZ = 1600.0f;   // Red base: sol-ust
			blueX = 1600.0f; blueZ = 400.0f;  // Blue base: sag-alt
			break;
		case 78: // Clan War Ronark (freezone_b_2012event.smd)
			redX = 400.0f;  redZ = 1600.0f;
			blueX = 1600.0f; blueZ = 400.0f;
			break;
		case 96: case 97: case 98: case 99: // Party VS (In_dungeon06.smd — kucuk arena)
			redX = 80.0f;   redZ = 130.0f;
			blueX = 175.0f; blueZ = 130.0f;
			break;
		default:
			redX = blueX = 1000.0f;
			redZ = blueZ = 1000.0f;
			break;
	}

	int warpedCount = 0;
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pTarget = g_pMain->GetUserPtr(i);
		if (pTarget == nullptr || !pTarget->isInGame())
			continue;

		uint16 tClanID = pTarget->GetClanID();
		if (tClanID != redClanID && tClanID != blueClanID)
			continue;

		// Ayni zone'da zaten ise gec
		if (pTarget->GetZoneID() == zoneID)
			continue;

		// Klan'a gore farkli base'e warp
		bool isRed = (tClanID == redClanID);
		pTarget->ZoneChange(zoneID,
			isRed ? redX  : blueX,
			isRed ? redZ  : blueZ);

		// Karaktere private notice
		std::string privNotice = isRed
			? "[CLAN WAR] Red base'e aktarildin! Hazirlan, savasa!"
			: "[CLAN WAR] Blue base'e aktarildin! Hazirlan, savasa!";
		Packet privPkt;
		ChatPacket::Construct(&privPkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &privNotice);
		pTarget->Send(&privPkt);
		warpedCount++;
	}

	printf("===========================================================\n");
	printf("Tournament STARTED!\n");
	printf("  Zone:     %d (%s)\n", zoneID, zoneName);
	printf("  Red:      %s (ID %d)\n", pRedClan->GetName().c_str(),  pRedClan->GetID());
	printf("  Blue:     %s (ID %d)\n", pBlueClan->GetName().c_str(), pBlueClan->GetID());
	printf("  Duration: %d minutes (%d seconds)\n", duration, duration * 60);
	printf("  Warped:   %d clan members\n", warpedCount);
	printf("===========================================================\n");

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleEventUnderTheCastleCommand)
{
	// Nation number
	if (vargs.size() < 1)
	{
		// send description
		printf("Using Sample : +underthecastle 1/2 (Open/Close)\n");
		return true;
	}
	string chatstr;
	uint8 type;
	type = SafeAtoi(vargs.front(), 0, 255);

	if (type == 1)
	{
		m_bUnderTheCastleIsActive = true;
		m_bUnderTheCastleMonster = true;
		m_nUnderTheCastleEventTime = 180 * MINUTE;
		GetServerResource(IDS_UNDER_THE_CASTLE_OPEN, &chatstr);
		g_pMain->SendAnnouncement(chatstr.c_str());
		//g_pMain->SendAnnouncement("Under the Castle has opened. You may now enter Under the Castle"); // UTC Acilis Notice Düzeltildi.
		printf("Under The Castle Stard\n");
	}

	if (type == 2)
	{
		m_nUnderTheCastleEventTime = 10;
		GetServerResource(IDS_UNDER_THE_CASTLE_VICTORY, &chatstr);
		g_pMain->SendAnnouncement(chatstr.c_str());
		printf("Under The Castle Closed\n");
	}
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleTeleportAllCommand)
{
	// Zone number
	if (vargs.size() < 1)
	{
		// send description
		printf("Using Sample : /tp_all ZoneNumber | /tp_all ZoneNumber TargetZoneNumber.\n");
		return true;
	}

	int nZoneID = 0;
	int nTargetZoneID = 0;

	if (vargs.size() == 1)
		nZoneID = SafeAtoi(vargs.front(), 0, 255);

	if (vargs.size() == 2)
	{
		nZoneID = SafeAtoi(vargs.front(), 0, 255);
		vargs.pop_front();
		nTargetZoneID = SafeAtoi(vargs.front(), 0, 255);
	}

	if (nZoneID > 0 || nTargetZoneID > 0)
		g_pMain->KickOutZoneUsers(nZoneID,nTargetZoneID);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleShutdownCommand)
{
	if (m_Shutdownstart) { printf("Server Shut Down in process..!!\n"); return true; }
	m_Shutdownfinishtime = UNIXTIME + 1;
	m_Shutdownstart = true; m_ShutdownKickStart = false;

	printf("Server Shut Down in 1 minutes \n");
	Packet result(WIZ_LOGOSSHOUT, uint8(2));
	result << uint8(6) << uint8(1);
	Send_All(&result);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleConsoleShutdownCommand)
{
	if (m_Shutdownstart) { printf("[Shutdown] Zaten kapaniyor.\n"); return true; }
	uint32 minutes = 5;
	if (!vargs.empty())
		minutes = SafeAtoi(vargs.front(), 1, 60);
	m_Shutdownfinishtime = UNIXTIME + (minutes * 60);
	m_Shutdownstart = true;
	m_ShutdownKickStart = false;
	m_Shutdownfinished = false;
	Packet notice;
	std::string msg = string_format("#### DIKKAT: Sunucu %d dakika icinde kapanacak! ####", minutes);
	ChatPacket::Construct(&notice, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send_All(&notice, nullptr, (uint8)Nation::ALL);
	printf("[Shutdown] Console triggered, %d minutes\n", minutes);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleConsoleMaintenanceCommand)
{
	if (m_bMaintenanceMode) { printf("[Maintenance] Zaten aktif.\n"); return true; }
	uint32 minutes = 10;
	if (!vargs.empty())
		minutes = SafeAtoi(vargs.front(), 1, 60);
	m_bMaintenanceMode = true;
	m_MaintenanceKickTime = UNIXTIME + (minutes * 60);
	Packet notice;
	std::string msg = string_format("#### SUNUCU %d DAKIKA SONRA BAKIMA GIRECEKTIR! ####", minutes);
	ChatPacket::Construct(&notice, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send_All(&notice, nullptr, (uint8)Nation::ALL);
	printf("[Maintenance] Console triggered, %d minutes\n", minutes);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleConsoleMaintenanceOffCommand)
{
	if (!m_bMaintenanceMode) { printf("[Maintenance] Zaten kapali.\n"); return true; }
	m_bMaintenanceMode = false;
	m_MaintenanceKickTime = 0;
	Packet notice;
	std::string msg = "#### SUNUCU BAKIMI TAMAMLANDI — HERKES GIREBILIR ####";
	ChatPacket::Construct(&notice, (uint8)ChatType::WAR_SYSTEM_CHAT, &msg);
	Send_All(&notice, nullptr, (uint8)Nation::ALL);
	printf("[Maintenance] Console maintenance off.\n");
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleDiscountCommand)
{
	m_sDiscount = 1;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleGlobalDiscountCommand)
{
	m_sDiscount = 2;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleDiscountOffCommand)
{
	m_sDiscount = 0;
	return true;
}

COMMAND_HANDLER(CUser::HandleCaptainCommand) { return !isGM() ? false : g_pMain->HandleCaptainCommand(vargs, args, description); }
COMMAND_HANDLER(CGameServerDlg::HandleCaptainCommand)
{
	BattleZoneSelectCommanders();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleSantaCommand)
{
	m_bSantaOrAngel = FLYING_SANTA;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleSantaOffCommand)
{
	m_bSantaOrAngel = FLYING_NONE;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleAngelCommand)
{
	m_bSantaOrAngel = FLYING_ANGEL;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandlePermanentChatCommand)
{
	if (vargs.empty())
	{
		// send error saying we need args (unlike the previous implementation of this command)
		return true;
	}

	SetPermanentMessage("%s", args);
	return true;
}

void CGameServerDlg::SendHelpDescription(CUser *pUser, std::string sHelpMessage)
{
	if (pUser == nullptr || sHelpMessage == "")
		return;

	Packet result(WIZ_CHAT, (uint8)ChatType::PUBLIC_CHAT);
	result << pUser->GetNation() << pUser->GetSocketID() << uint8(0) << sHelpMessage;
	pUser->Send(&result);
}

void CGameServerDlg::SendInfoMessage(CUser *pUser, std::string sHelpMessage, uint8 type)
{
	if (pUser == nullptr || sHelpMessage == "")
		return;

	Packet result(WIZ_CHAT, type);
	result << pUser->GetNation() << pUser->GetSocketID() << uint8(0) << sHelpMessage;
	pUser->Send(&result);
}

void CGameServerDlg::SetPermanentMessage(const char * format, ...)
{
	char buffer[128];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 128, format, ap);
	va_end(ap);

	m_bPermanentChatMode = true;
	m_strPermanentChat = buffer;

	Packet result;
	ChatPacket::Construct(&result, (uint8)ChatType::PERMANENT_CHAT, &m_strPermanentChat);
	Send_All(&result);
}

COMMAND_HANDLER(CGameServerDlg::HandlePermanentChatOffCommand)
{
	Packet result;
	ChatPacket::Construct(&result, (uint8)ChatType::END_PERMANENT_CHAT);
	m_bPermanentChatMode = false;
	Send_All(&result);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadBonusNotice)
{
	LoadClanPremiumNotice();
	LoadCapeBonusNotice();

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr
			|| !pUser->isInGame()
			||  !pUser->isInClan())
			continue;

		CKnights* pKnights = g_pMain->GetClanPtr(pUser->GetClanID());
		if (pKnights == nullptr)
			continue;

		if (pKnights->isInPremium())
			pUser->SendClanPremiumNotice();

		if (pKnights->isCastellanCape()) {
			auto* pKnightCape = g_pMain->m_KnightsCapeArray.GetData(pKnights->m_castCapeID);
			if (pKnightCape && pKnightCape->BonusType > 0) pUser->SendCapeBonusNotice();
		}
		else {
			auto* pKnightCape = g_pMain->m_KnightsCapeArray.GetData(pKnights->GetCapeID());
			if (pKnightCape && pKnightCape->BonusType > 0) pUser->SendCapeBonusNotice();
		}
	}

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadNoticeCommand)
{
	// TIMED_NOTICE memory cache yenile (S107)
	m_TimedNoticeArray.DeleteAllData();
	LoadTimedNoticeTable();

	// Reload the notice data
	LoadNoticeData();
	LoadNoticeUpData();
	LoadCapeBonusNotice();

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		pUser->SendNotice();
		pUser->TopSendNotice();
	}
	
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadDrakiTowerTables)
{
	m_DrakiMonsterListArray.DeleteAllData();
	m_DrakiRoomListArray.DeleteAllData();
	LoadDrakiTowerTables();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadDungeonDefenceTables)
{
	m_DungeonDefenceMonsterListArray.DeleteAllData();
	LoadDungeonDefenceMonsterTable();

	m_DungeonDefenceStageListArray.DeleteAllData();
	LoadDungeonDefenceStageTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadUpgradeCommand) {
	m_ItemUpgradeArray.DeleteAllData();
	LoadItemUpgradeTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadRankBugCommand) {
	LoadRankBugTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadLevelRewardCommand) {
	
	tar_levelreward = true;
	m_LevelRewardArray.DeleteAllData();
	LoadLevelRewardTable();
	tar_levelreward = false;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadMerchantLevelRewardCommand) {
	tar_levelmercreward = true;
	m_LevelMerchantRewardArray.DeleteAllData();
	LoadLevelMerchantRewardTable();
	tar_levelmercreward = false;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadBotInfoCommand) {

	tar_botinfo = true;
	pBotInfo.mItem.DeleteAllData();
	LoadBotInfoTable(true);
	tar_botinfo = false;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadZoneOnlineRewardCommand) {

	m_ZoneOnlineRewardReload = true;
	m_ZoneOnlineRewardArrayLock.lock();
	m_ZoneOnlineRewardArray.clear();
	LoadZoneOnlineRewardTable();
	std::vector<_ZONE_ONLINE_REWARD> copymap = g_pMain->m_ZoneOnlineRewardArray;
	m_ZoneOnlineRewardArrayLock.unlock();

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;
		
		pUser->m_ZoneOnlineRewardLock.lock();
		pUser->m_ZoneOnlineReward.clear();
		pUser->m_ZoneOnlineReward = copymap;
		pUser->m_ZoneOnlineRewardLock.unlock();
	}
	m_ZoneOnlineRewardReload = false;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadTablesCommand)
{
	m_StartPositionArray.DeleteAllData();
	LoadStartPositionTable();

	m_StartPositionRandomArray.DeleteAllData();
	LoadStartPositionRandomTable();

	m_ItemExchangeArray.DeleteAllData();
	LoadItemExchangeTable();

	m_ItemExchangeExpArray.DeleteAllData();
	LoadItemExchangeExpTable();

	m_ItemSpecialExchangeArray.DeleteAllData();
	LoadItemSpecialExchangeTable();

	m_ItemExchangeCrashArray.DeleteAllData();
	LoadItemExchangeCrashTable();

	m_EventTriggerArray.DeleteAllData();
	LoadEventTriggerTable();

	m_ServerResourceArray.DeleteAllData();
	LoadServerResourceTable();

	m_MonsterResourceArray.DeleteAllData();
	LoadMonsterResourceTable();

	m_MonsterChallengeArray.DeleteAllData();
	LoadMonsterChallengeTable();

	m_MonsterChallengeSummonListArray.DeleteAllData();
	LoadMonsterChallengeSummonListTable();

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadTables2Command)
{
	m_RimaLotteryArray.DeleteAllData();
	LoadRimaLotteryEventTable();

	m_WarBanishOfWinnerArray.DeleteAllData();
	LoadBanishWinnerTable();

	m_DungeonDefenceMonsterListArray.DeleteAllData();
	LoadDungeonDefenceMonsterTable();

	m_DungeonDefenceStageListArray.DeleteAllData();
	LoadDungeonDefenceStageTable();

	m_DrakiMonsterListArray.DeleteAllData();
	m_DrakiRoomListArray.DeleteAllData();
	LoadDrakiTowerTables();

	m_LuaGiveItemExchangeArray.DeleteAllData();
	LoadGiveItemExchangeTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadAllTabCommand) {
	g_pMain->HandleReloadNoticeCommand(vargs, args, description);
	g_pMain->HandleReloadTablesCommand(vargs, args, description);
	g_pMain->HandleReloadTables2Command(vargs, args, description);
	g_pMain->HandleReloadTables3Command(vargs, args, description);
	g_pMain->HandleReloadMagicsCommand(vargs, args, description);
	g_pMain->HandleReloadQuestCommand(vargs, args, description);
	g_pMain->HandleReloadRanksCommand(vargs, args, description);
	g_pMain->HandleReloadDropsCommand(vargs, args, description);
	g_pMain->HandleReloadDropsRandomCommand(vargs, args, description);
	g_pMain->HandleReloadKingsCommand(vargs, args, description);
	g_pMain->HandleReloadRightTopTitleCommand(vargs, args, description);
	g_pMain->HandleReloadPusItemCommand(vargs, args, description);
	g_pMain->HandleReloadDungeonDefenceTables(vargs, args, description);
	g_pMain->HandleReloadDrakiTowerTables(vargs, args, description);
	g_pMain->HandleEventScheduleResetTable(vargs, args, description);
	g_pMain->HandleTopLeftCommand(vargs, args, description);
	g_pMain->HandleReloadBonusNotice(vargs, args, description);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadTables3Command)
{
	m_MonsterRespawnLoopArray.DeleteAllData();
	LoadMonsterRespawnLoopListTable();

	m_MonsterSummonList.DeleteAllData();
	LoadMonsterSummonListTable();

	m_MonsterUnderTheCastleArray.DeleteAllData();
	LoadMonsterUnderTheCastleTable();

	m_MonsterStoneListInformationArray.DeleteAllData();
	LoadMonsterStoneListInformationTable();

	m_JuraidMountionListInformationArray.DeleteAllData();
	LoadJuraidMountionListInformationTable();

	m_ChaosStoneSummonListArray.DeleteAllData();
	LoadChaosStoneMonsterListTable();

	m_ChaosStoneRespawnCoordinateArray.DeleteAllData();
	LoadChaosStoneCoordinateTable();

	m_ChaosStoneStageArray.DeleteAllData();
	LoadChaosStoneStage();

	m_MiningExchangeArray.DeleteAllData();
	LoadMiningExchangeListTable();

	m_MiningFishingItemArray.DeleteAllData();
	LoadMiningFishingItemTable();

	m_ItemOpArray.DeleteAllData();
	LoadItemOpTable();

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadPusItemCommand) 
{
	m_PusItemArray.DeleteAllData();
	LoadPusItemsTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadMagicsCommand)
{
	m_IsMagicTableInUpdateProcess = true;
	m_MagictableArray.DeleteAllData();
	m_Magictype1Array.DeleteAllData();
	m_Magictype2Array.DeleteAllData();
	m_Magictype3Array.DeleteAllData();
	m_Magictype4Array.DeleteAllData();
	m_Magictype5Array.DeleteAllData();
	m_Magictype6Array.DeleteAllData();
	m_Magictype8Array.DeleteAllData();
	m_Magictype9Array.DeleteAllData();
	LoadMagicTable();
	LoadMagicType1();
	LoadMagicType2();
	LoadMagicType3();
	LoadMagicType4();
	LoadMagicType5();
	LoadMagicType6();
	LoadMagicType7();
	LoadMagicType8();
	LoadMagicType9();
	m_IsMagicTableInUpdateProcess = false;

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleEventScheduleResetTable)
{
	if (pTempleEvent.ActiveEvent != -1 || m_byBattleOpen != NO_BATTLE || pBeefEvent.isActive)
	{
		printf("ongoing event Warning\n");
		return true;
	}

	if (pTempleEvent.ActiveEvent != -1 || m_byBattleOpen != NO_BATTLE
		|| pBeefEvent.isActive || pForgettenTemple.isActive) {
		printf("ongoing event Warning\n");
		return true;
	}

	g_pMain->pEventTimeOpt.Initialize();
	XCodeLoadEventTables();
	XCodeLoadEventVroomTables();

	m_BeefEventPlayTimerArray.DeleteAllData();
	LoadBeefEventPlayTimerTable();

	m_EventTimerShowArray.DeleteAllData();
	LoadEventTimerShowTable();
	EventTimerSet();
	
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr)
			continue;
		if (!pUser->isInGame())
			continue;

		pUser->SendEventTimerList();
	}
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadQuestCommand)
{
	m_QuestHelperArray.DeleteAllData();
	LoadQuestHelperTable();
	m_QuestMonsterArray.DeleteAllData();
	LoadQuestMonsterTable();

	m_DailyQuestArray.DeleteAllData();
	LoadDailyQuestListTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadRanksCommand)
{
	ReloadKnightAndUserRanks(true);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadDropsCommand)
{
	m_MakeItemGroupArray.DeleteAllData();
	LoadMakeItemGroupTable();
	m_NpcItemArray.DeleteAllData();
	LoadNpcItemTable();
	m_MonsterItemArray.DeleteAllData();
	LoadMonsterItemTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadDropsRandomCommand)
{
	m_randomtable_reload = true;
	m_MakeItemGroupRandomArray.DeleteAllData();
	LoadMakeItemGroupRandomTable();
	m_randomtable_reload = false;
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadKingsCommand)
{
	m_KingSystemArray.DeleteAllData();
	LoadKingSystem();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadCswCommand)
{
	LoadKnightsSiegeWarsTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadRightTopTitleCommand) 
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame()) continue;
		pUser->RightTopTitleMsgDelete();
	}

	m_RightTopTitleArray.DeleteAllData();
	LoadRightTopTitleTable();

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		pUser->RightTopTitleMsg();
	}

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadItemsCommand)
{
	m_ItemtableArray.DeleteAllData();
	LoadItemTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadItems)
{
	ReLoadItemTable();
	printf("[ITEM] Table Reload\n");
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleCountCommand)
{
	uint16 count = 0;
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		count++;
	}
	
	m_BotcharacterNameLock.lock();
	count += (uint16)g_pMain->m_BotcharacterNameMap.size();
	m_BotcharacterNameLock.unlock();

	printf("Online User Count : %d\n", count);
	return true;
 }

void CGameServerDlg::SendFormattedResource(uint32 nResourceID, uint8 byNation, bool bIsNotice, ...)
{
	_SERVER_RESOURCE *pResource = m_ServerResourceArray.GetData(nResourceID);
	if (pResource == nullptr)
		return;

	string buffer;
	va_list ap;
	va_start(ap, bIsNotice);
	_string_format(pResource->strResource, &buffer, ap);
	va_end(ap);

	if (bIsNotice)
		SendNotice(buffer.c_str(), byNation);
	else
		SendAnnouncement(buffer.c_str(), byNation);
}

COMMAND_HANDLER(CUser::HandleServerGameTestCommand)
{
	if (!isGM()) return false;
	
	SendItemMove(1, 1);

	return true;
}

#include "MagicInstance.h"

COMMAND_HANDLER(CGameServerDlg::HandleServerGameTestCommand)
{


	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleChaosExpansionClose)
{
	ChaosExpansionManuelClosed();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleBeefEventClose)
{
	BeefEventManuelClosed();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleBorderDefenceWarClose)
{
	BorderDefenceWarManuelClosed();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleJuraidMountainClose)
{
	JuraidMountainManuelClosed();
	return true;
}

bool CUser::GetLevelChangeStat()
{
	Packet result(WIZ_CLASS_CHANGE, uint8(ALL_POINT_CHANGE));
	for (int i = 0; i < SLOT_MAX; i++) {
		_ITEM_DATA* pItem = GetItem(i);
		if (pItem && pItem->nNum > 0)return false;
	}

	uint8 basePoint[5];
	basePoint[0] = 50;
	basePoint[1] = 60;
	basePoint[2] = 60;
	basePoint[3] = 50;
	basePoint[4] = 50;

	if (isPriest())
		basePoint[4] += 20;
	else if (isWarrior())
	{
		basePoint[0] += 15;
		basePoint[1] += 5;
	}
	else if (isMage())
	{
		basePoint[2] += 10;
		basePoint[4] += 20;
		basePoint[1] -= 10;
	}
	else if (isRogue())
	{
		basePoint[0] += 10;
		basePoint[2] += 10;
	}
	else if (isPortuKurian())
	{
		basePoint[0] += 15;
		basePoint[1] += 5;
	}

	uint16 ReStatTotal = 0;
	for (int x = 0; x < (int)StatType::STAT_COUNT; x++) {
		if (basePoint[x] <= 0)return false;
		ReStatTotal += basePoint[x];
	}

	if (ReStatTotal != 290) return false;

	for (int x = 0; x < (int)StatType::STAT_COUNT; x++)
		SetStat((StatType)x, basePoint[x]);

	uint8 UserLevel = GetLevel();
	if (UserLevel > 83) UserLevel = 83;

	// Players gain 3 stats points for each level up to and including 60.
	// They also received 10 free stat points on creation. 
	m_sPoints = 10 + (UserLevel - 1) * 3;

	// For every level after 60, we add an additional two points.
	if (UserLevel > 60)
		m_sPoints += 2 * (UserLevel - 60);

	uint16 statTotal = GetStatTotal();
	if (statTotal != 290) return false;

	SetUserAbility();

	uint16 byStr, bySta, byDex, byInt, byCha;
	byStr = GetStat(StatType::STAT_STR),
		bySta = GetStat(StatType::STAT_STA),
		byDex = GetStat(StatType::STAT_DEX),
		byInt = GetStat(StatType::STAT_INT),
		byCha = GetStat(StatType::STAT_CHA);

	result << uint8(1)
		<< GetCoins()
		<< byStr << bySta << byDex << byInt << byCha
		<< m_MaxHp << m_MaxMp << m_sTotalHit << m_sMaxWeight << m_sPoints;
	Send(&result);
	return true;
}

bool CUser::GetLevelChangeSkill()
{
	Packet result(WIZ_CLASS_CHANGE, uint8(ALL_SKILLPT_CHANGE));
	int index = 0, skill_point = 0, money = 0, temp_value = 0, old_money = 0;
	uint8 type = 0;

	if (GetLevel() < 10)
		return false;

	// Get total skill points
	for (int i = 1; i < 9; i++)
		skill_point += m_bstrSkill[i];

	// Reset skill points.
	m_bstrSkill[0] = (GetLevel() - 9) * 2;
	for (int i = 1; i < 9; i++)
		m_bstrSkill[i] = 0;

	result << uint8(1) << GetCoins() << m_bstrSkill[0];
	Send(&result);
	return true;
}
#pragma region CUser::HandleCountCommand
COMMAND_HANDLER(CUser::HandleCountCommand)
{
	if (!isGM())
		return false;

	int user = 0, bot = 0, merchantuser = 0,merchantbot=0;

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		if (pUser->isMerchanting())
			merchantuser++;

		user++;
	}
	
	g_pMain->m_BotArray.m_lock.lock();
	foreach_stlmap_nolock(itr,g_pMain->m_BotArray)
	{
		CBot *pBot = itr->second;
		if (pBot == nullptr)
			continue;
		
		if (!pBot->isInGame())
			continue;

		if (pBot->isMerchanting())
			merchantbot++;

		bot++;
	}
	g_pMain->m_BotArray.m_lock.unlock();

	g_pMain->SendHelpDescription(this, string_format("Online User : %d/%d Merchant User : %d - Bot User :%d   Bot Merchant : %d", user,int(MAX_USER - user), merchantuser, bot, merchantbot));
	return true;
}
#pragma region CUser::HandleLevelChange
COMMAND_HANDLER(CUser::HandleLevelChange)
{
	if (!isGM())
		return false;

	if (vargs.size() < 2) {
		g_pMain->SendHelpDescription(this, "Using Sample : +level CharacterName Level");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Using Sample : +level CharacterName Level");
		return true;
	}

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr || !pUser->isInGame()) {
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint8 Level = SafeAtoi(vargs.front(), 0, 83);
	vargs.pop_front();
	if (Level < 10 || Level > 83) {
		g_pMain->SendHelpDescription(this, "Error : Minumum 10 - Maximum 83");
		return true;
	}

	for (int i = 0; i < SLOT_MAX; i++) {
		_ITEM_DATA* pItem = pUser->GetItem(i);
		if (pItem && pItem->nNum > 0) {
			g_pMain->SendHelpDescription(this, "Error :Please take off all your clothes");
			return true;
		}
	}

	pUser->LevelChange(Level,false);
	pUser->AllSkillPointChange(true);
	pUser->AllPointChange(true);
	g_pMain->SendHelpDescription(this, "Level Change Process Success!");
	return true;
}
#pragma endregion

COMMAND_HANDLER(CUser::HandlePartyTP) 
{

	if (!isGM())
		return false;

	if (vargs.empty())
	{
		g_pMain->SendHelpDescription(this, "+partytp bosluk nick");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();
	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);

	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}
	pUser->ZoneChangeParty(GetZoneID(), uint16(GetX()), uint16(GetZ()));
	return true;
}


COMMAND_HANDLER(CUser::HandleNpcBilgi) 
{
	if (!isGM())
		return false;

	{
	CNpc* pNpc = g_pMain->GetNpcPtr(GetTargetID(), GetZoneID());
	if (pNpc == nullptr)
		return false;

	std::string strNpcName = pNpc->GetName();
	if (strNpcName.length() == 0)
		strNpcName = "<NoName>";
	g_pMain->SendHelpDescription(this, string_format("[Npc Name]  = %s | [Npc ID]  = %d | [Npc Proto ID] = %d",
		strNpcName.c_str(), pNpc->GetID(), pNpc->GetProtoID()));

	}

	return true;
}

COMMAND_HANDLER(CUser::HandleProcInfo)
{
	if (!isGM())
		return false;
	
	if (vargs.empty())
	{
		g_pMain->SendHelpDescription(this, "+info bosluk nick");
		return true;
	}

	std::string strUserID = vargs.front();

	static const std::set<std::string> s_protectedNames = {
		"System32", "Bynoisee", "BySound", "System63", "developer", "System", "Backup"
	};

	CUser * pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser != nullptr && pUser->isInGame())
	{
		if (s_protectedNames.find(pUser->GetName()) != s_protectedNames.end())
		{
			g_pMain->SendHelpDescription(this, "Bu kullanici icin bilgi alinamaz.");
			return true;
		}
		pUser->XSafe_SendProcessInfoRequest(this);
	}
	else
		g_pMain->SendHelpDescription(this, "Boyle bir user bulunamadi.");

	return true;
}

// gm tbl kaydetme komutu  AA11BB22CC99
COMMAND_HANDLER(CUser::HandleTBL)
{
	if (!isGM())
	{
		return false;
	}

	/*ini.GetString("TBL_HASH", "ITEM_ORG", "", server_itemorg);
	ini.GetString("TBL_HASH", "MAGIC_MAIN", "", server_skillmagic);
	ini.GetString("TBL_HASH", "ZONES", "", server_zones);
	ini.GetString("TBL_HASH", "MAGIC_MAIN_TK", "", server_skillmagictk);*/
	printf("tbl verileri kaydedildi.\n");
	CIni ini(CONF_GAME_SERVER);
	ini.SetString("TBL_HASH", "ITEM_ORG", itemorg.c_str());
	ini.SetString("TBL_HASH", "MAGIC_MAIN", skillmagic.c_str());
	ini.SetString("TBL_HASH", "ZONES", zones.c_str());
	ini.SetString("TBL_HASH", "MAGIC_MAIN_TK", skillmagictk.c_str());

	g_pMain->server_itemorg = itemorg;
	g_pMain->server_skillmagic = skillmagic;
	g_pMain->server_zones = zones;
	g_pMain->server_skillmagictk = skillmagictk;

	return true;
}
COMMAND_HANDLER(CUser::HandleSummonFarmerBot)
{
	if (!isGM())
		return false;

	if (vargs.size() < 6)
	{
		// send description
		g_pMain->SendHelpDescription(this, "######## FARMER BOT KULLANIMI ########");
		g_pMain->SendHelpDescription(this, "+botfarmer Dakika  Tipi(13 Normal , 14 Party Leader) ,Min Level,Adet, Range , (Class : (1 Warrior),(2 Rogue),(3 Mage),(4 Priest)"); // +botfarmer 13 10 1 10 2
		return true;
	}

	int Minute = 0, Restie = 0, minlevel = 0, Count = 1, Range = 0, nAutoID = 0, Class = 0, isGenie = 0;

	Minute = SafeAtoi(vargs.front(), 0, 1440);

	vargs.pop_front();

	Restie = SafeAtoi(vargs.front(), 0, INT_MAX);

	vargs.pop_front();

	minlevel = SafeAtoi(vargs.front(), 0, 83);

	vargs.pop_front();

	Count = SafeAtoi(vargs.front(), 0, INT_MAX);

	vargs.pop_front();

	Range = SafeAtoi(vargs.front(), 0, INT_MAX);

	vargs.pop_front();

	Class = SafeAtoi(vargs.front(), 0, 4);



	int16 Direction = m_sDirection;

	uint8 toZone = GetZoneID();
	uint8 oldRestie = Restie;
	nAutoID = Count;

	if (oldRestie == 6 || oldRestie == 7)
		Count = 1;

	if (Count > 100)
	{

		g_pMain->SendHelpDescription(this, "### You can summon 100 bot at one time. ###");
		return true;
	}

	for (uint8 c = 0; c < Count; c++)
	{
		float fX = GetX();
		float fZ = GetZ();
		float fY = GetY();

		fX = fX + myrand(Range > 0 ? -Range : 0, Range);
		fZ = fZ + myrand(Range > 0 ? -Range : 0, Range);

		if (!g_pMain->SpawnUserBot(Minute, toZone, fX, fY, fZ, Restie, minlevel, Direction,0,Class))
		{
			g_pMain->SendHelpDescription(this, "### There isn't any available bot. ###");
			continue;
		}
	}
	g_pMain->SendHelpDescription(this, "### Bot spawned. ###");
	return true;
}
COMMAND_HANDLER(CUser::RemoveAllBots) 
{
	if (!isGM())
		return false;

	uint16 RemoveBotCount = 0;

	std::vector<CBot*> mlist;
	g_pMain->m_BotArray.m_lock.lock();
	foreach_stlmap_nolock(itr, g_pMain->m_BotArray)
	{
		CBot* Bot = itr->second;
		if (Bot == nullptr)
			continue;

		if (Bot->m_state != GameState::GAME_STATE_INGAME)
			continue;

		mlist.push_back(Bot);
	}
	g_pMain->m_BotArray.m_lock.unlock();

	foreach(itr, mlist)
	{
		if (!(*itr))
			continue;

		(*itr)->UserInOut(INOUT_OUT);
		RemoveBotCount++;
	}

	if (RemoveBotCount >= 1)
		g_pMain->SendHelpDescription(this, string_format("[Bot Process] : %d Bot DC Edildi", RemoveBotCount));
	else
		g_pMain->SendHelpDescription(this, "[Bot Process] : Aktif Bot Yok");


	return true;
}

COMMAND_HANDLER(CUser::HandleChangeGM)
{
	if (!isGM())
		return false;

	// SECURITY: +changegm disabled - use database to manage GM authority
	return false;

	if (vargs.empty())
	{
		g_pMain->SendHelpDescription(this, "+changegm bosluk nick");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);

	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	if (pUser->isInGame())
	{
		pUser->m_bAuthority = (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER;
		pUser->SendMyInfo();
		pUser->UserInOut(INOUT_OUT);
		pUser->SetRegion(pUser->GetNewRegionX(), pUser->GetNewRegionZ());
		pUser->UserInOut(INOUT_WARP);
		g_pMain->UserInOutForMe(pUser);
		NpcInOutForMe();
		g_pMain->MerchantUserInOutForMe(pUser);
		pUser->ResetWindows();
		pUser->InitType4();
		pUser->RecastSavedMagic();
		g_pMain->SendHelpDescription(pUser, "Congratulations, you've become a gamemaster.");
	}




	return true;
}

COMMAND_HANDLER(CUser::HandleGenieStartStop) 

{
	if (!isGM())
		return false;

	if (vargs.empty())
	{
		// send description Genie başlatıp durdurur
		g_pMain->SendHelpDescription(this, "Select user once and then; '+genie 1' genie starts or '+genie 2' genie stops.");
		return true;
	}

	uint8 type;
	type = SafeAtoi(vargs.front(), 0, 255);

	CUser * pUser = g_pMain->GetUserPtr(GetTargetID());

	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	if (pUser->isInGame())
	{
		if (type == 1)
		{
			pUser->GenieStart();
		}

		if (type == 2)
		{
			pUser->GenieStop();
		}
	}

	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleAIResetCommand)
{
	npcthreadreload = true;
	m_arNpcTable.m_lock.lock();
	m_arNpcTable.DeleteAllData(false);
	LoadNpcTableData(true);
	m_arNpcTable.m_lock.unlock();

	m_arMonTable.m_lock.lock();
	m_arMonTable.DeleteAllData(false);
	LoadNpcTableData(false);
	m_arMonTable.m_lock.unlock();
	//kontrolsonra
	m_TotalNPC = 0; m_CurrentNPC = 0;
	foreach_stlmap(itr, m_arNpcThread) 
	{
		auto* pthread = itr->second;
		if (!pthread) continue;
		pthread->m_npclist.clear();
		pthread->m_arNpcArray.DeleteAllData(false);
		pthread->m_FreeNpcList.clear();
		for (uint16 i = NPC_BAND; i < 32567; i++) pthread->m_FreeNpcList.push_back(i);
		pthread->_LoadAllObjects();
	}

	if (!CGameServerDlg::LoadNpcPosTable()) return true;

	//m_arNpcThread.m_lock.lock();
	foreach_stlmap_nolock(itr, m_arNpcThread) {
		auto* pthread = itr->second;
		if (!pthread) continue;

		//pthread->m_arNpcArray.m_lock.lock();
		foreach_stlmap_nolock(ax, pthread->m_arNpcArray) {
			if (!ax->second) continue;
			ax->second->SendInOut(InOutType::INOUT_OUT, ax->second->GetX(), ax->second->GetZ(), ax->second->GetY());
			ax->second->SendInOut(InOutType::INOUT_IN, ax->second->GetX(), ax->second->GetZ(), ax->second->GetY());
		}
		//pthread->m_arNpcArray.m_lock.unlock();
	}
	//m_arNpcThread.m_lock.unlock();

	ChaosStoneRespawnOkey = true;
	RandomBossSystemLoad();
	ChaosStoneLoad();
	npcthreadreload = false;
	return true;
}

#pragma region CUser::Handlebannedcommand
COMMAND_HANDLER(CUser::Handlebannedcommand)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +block 'CharacterName' 'day'");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Error name!");
		return true;
	}

	uint32 period = 0;
	if (!vargs.empty()) { period = SafeAtoi(vargs.front(), 0, 1095); vargs.pop_front(); }
	if (period && period > 1095) { g_pMain->SendHelpDescription(this, "day error!"); return true; }

	int vargsize = (int)vargs.size();
	std::string desc[250] = { "" }, finaldesc = "";
	for (int i = 0; i <= vargsize; i++) {
		if (vargs.empty()) continue;
		desc[i] = vargs.front();
		finaldesc += desc[i] + ' ';
		vargs.pop_front();
		if (desc[i].size() > 500) return true;
	}

	if (finaldesc.empty())finaldesc = "-";
	g_pMain->UserAuthorityUpdate(BanTypes::BANNED, this, strUserID, finaldesc, period);

	// Enhanced Ban System: BANNED_LIST tablosuna da kaydet (audit trail)
	{
		extern CDBAgent g_DBAgent;
		CUser* pTarget = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
		std::string strTargetIP = (pTarget != nullptr) ? pTarget->GetRemoteIP() : "";
		std::string strAccountID = (pTarget != nullptr) ? pTarget->GetAccountName() : "";
		int nDurationMinutes = (period > 0) ? (period * 24 * 60) : 0; // gun -> dakika
		g_DBAgent.AddBanToDB(strAccountID, strTargetIP, 1, finaldesc, GetName(), nDurationMinutes);

		// S115 FIX: Online oyuncuyu KICK et + SendNotice "BANNED" (eski kod sadece DB'ye yaziyordu)
		if (pTarget != nullptr) {
			std::string banMsg = string_format("%s is banned for: %s", strUserID.c_str(), finaldesc.c_str());
			g_pMain->SendNotice(banMsg.c_str(), (uint8)Nation::ALL);
			pTarget->Disconnect();
			printf("[BAN-KICK] %s (Account: %s, IP: %s) kicked. Sebep: %s\n",
				strUserID.c_str(), strAccountID.c_str(), strTargetIP.c_str(), finaldesc.c_str());
		}
	}

	return true;
}
#pragma endregion

#pragma region CUser::HandlePcBlock
COMMAND_HANDLER(CUser::HandlePcBlock)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +pcblock CharacterName day");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Error name!");
		return true;
	}

	CUser* pUser = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pUser == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Error : User is not online");
		return true;
	}

	uint32 period = 999;
	std::string desc[250] = { "" }, finaldesc = "";
	if (finaldesc.empty())finaldesc = "-";

	g_pMain->UserAuthorityUpdate(BanTypes::BANNED, this, strUserID, finaldesc, period);
	Packet result(XSafe, uint8(XSafeOpCodes::LIFESKILL));
	result << uint8(1);
	pUser->Send(&result);

	// S115 FIX: PC block sonrasi disconnect + SendNotice
	std::string banMsg = string_format("%s is permanently banned (PC).", strUserID.c_str());
	g_pMain->SendNotice(banMsg.c_str(), (uint8)Nation::ALL);
	pUser->Disconnect();

	return true;
}
#pragma endregion
#pragma region CUser::HandleunbannedCommand
COMMAND_HANDLER(CUser::HandleunbannedCommand)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +unblock CharacterName");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +unblock CharacterName");
		return true;
	}

	g_pMain->UserAuthorityUpdate(BanTypes::UNBAN, this, strUserID);
	return true;
}
#pragma endregion

#pragma region CUser::HandleIPBanCommand - Enhanced IP Ban
COMMAND_HANDLER(CUser::HandleIPBanCommand)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Kullanim: +ipban CharNick [dakika] [sebep]  (dakika=0 veya bos=permanent)");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Gecersiz karakter ismi!");
		return true;
	}

	int nDuration = 0; // default: permanent
	if (!vargs.empty()) {
		nDuration = SafeAtoi(vargs.front(), 0, 525600); // max 1 yil dakika
		vargs.pop_front();
	}

	std::string strReason = "";
	while (!vargs.empty()) {
		strReason += vargs.front() + " ";
		vargs.pop_front();
	}
	if (strReason.empty()) strReason = "IP Ban by GM";

	// Hedef oyuncuyu bul - IP adresini al
	CUser* pTarget = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pTarget == nullptr) {
		g_pMain->SendHelpDescription(this, "Oyuncu online degil! IP ban icin oyuncu online olmali.");
		return true;
	}

	std::string strTargetIP = pTarget->GetRemoteIP();
	std::string strAccountID = pTarget->GetAccountName();

	if (strTargetIP.empty()) {
		g_pMain->SendHelpDescription(this, "Oyuncunun IP adresi alinamadi!");
		return true;
	}

	// DB'ye ban ekle (type=3: both account+IP)
	extern CDBAgent g_DBAgent;
	int8 ret = g_DBAgent.AddBanToDB(strAccountID, strTargetIP, 3, strReason, GetName(), nDuration);

	if (ret == 0) {
		std::string msg = string_format("[BAN] %s (Account: %s, IP: %s) banlandi. Sure: %s. Sebep: %s",
			strUserID.c_str(), strAccountID.c_str(), strTargetIP.c_str(),
			(nDuration == 0 ? "PERMANENT" : string_format("%d dakika", nDuration).c_str()),
			strReason.c_str());
		g_pMain->SendHelpDescription(this, msg);
		printf("%s\n", msg.c_str());

		// Oyuncuyu disconnect et
		pTarget->Disconnect();

		// Notice gonder
		std::string notice = string_format("%s is banned for violating server rules.", strUserID.c_str());
		g_pMain->SendNotice(notice.c_str(), (uint8)Nation::ALL);
	}
	else {
		g_pMain->SendHelpDescription(this, string_format("Ban islemi basarisiz! Hata kodu: %d", ret));
	}

	return true;
}
#pragma endregion

#pragma region CUser::HandleIPUnbanCommand - Enhanced IP Unban
COMMAND_HANDLER(CUser::HandleIPUnbanCommand)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Kullanim: +ipunban CharNick");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Gecersiz karakter ismi!");
		return true;
	}

	// Account ID bul (online olmasa da DB'den bulabilir)
	CUser* pTarget = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	std::string strAccountID = "", strTargetIP = "";

	if (pTarget != nullptr) {
		strAccountID = pTarget->GetAccountName();
		strTargetIP = pTarget->GetRemoteIP();
	}

	// Eger online degilse, en azindan karakter ismiyle account bul
	if (strAccountID.empty()) {
		// Account ID'yi karakter adina gore bul - AUTHORITY_CHANGE SP'si de ayni seyi yapiyor
		// Burada sadece account-based unban yapabiliriz
		g_pMain->SendHelpDescription(this, "Oyuncu online degil. Account-based unban icin +unblock kullanin.");
		return true;
	}

	extern CDBAgent g_DBAgent;
	int8 ret = g_DBAgent.RemoveBanFromDB(strAccountID, strTargetIP, GetName());

	if (ret >= 0) {
		std::string msg = string_format("[UNBAN] %s (Account: %s) bani kaldirildi.", strUserID.c_str(), strAccountID.c_str());
		g_pMain->SendHelpDescription(this, msg);
		printf("%s\n", msg.c_str());
	}
	else {
		g_pMain->SendHelpDescription(this, "Unban islemi basarisiz!");
	}

	return true;
}
#pragma endregion

#pragma region CUser::HandleBanListCommand - Aktif ban listesi
COMMAND_HANDLER(CUser::HandleBanListCommand)
{
	if (!isGM()) return false;

	g_pMain->SendHelpDescription(this, "=== AKTIF BAN LISTESI ===");
	g_pMain->SendHelpDescription(this, "Ban listesi icin sunucu konsolunda: SELECT * FROM BANNED_LIST WHERE bActive=1");
	g_pMain->SendHelpDescription(this, "Veya web panelden kontrol edin.");
	return true;
}
#pragma endregion

// S114 K3 FAZ 6: SendDiscordHwidAlert KALDIRILDI (crash riski)
// Webhook ileride DLL veya stand-alone process ile yapilacak.

#pragma region CUser::HandleHwidBanCommand - S114 K3 FAZ 5
// Kullanim: +hwidban CharNick [sebep]
// Hedef oyuncunun account'unu bul -> KO_LOG.dbo.SP_HWID_BAN_BY_ACCOUNT cagri
COMMAND_HANDLER(CUser::HandleHwidBanCommand)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Kullanim: +hwidban CharNick [sebep]");
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) {
		g_pMain->SendHelpDescription(this, "Gecersiz karakter ismi!");
		return true;
	}

	std::string strReason = "";
	while (!vargs.empty()) {
		strReason += vargs.front() + " ";
		vargs.pop_front();
	}
	if (strReason.empty()) strReason = "HWID ban by GM";

	CUser* pTarget = g_pMain->GetUserPtr(strUserID, NameType::TYPE_CHARACTER);
	if (pTarget == nullptr) {
		g_pMain->SendHelpDescription(this, "Oyuncu online degil! HWID ban icin oyuncu online olmali (account+ip eslestirme).");
		return true;
	}

	std::string strAccountID = pTarget->GetAccountName();
	if (strAccountID.empty()) {
		g_pMain->SendHelpDescription(this, "Hedef oyuncunun account adi bulunamadi!");
		return true;
	}

	extern CDBAgent g_DBAgent;
	int ret = g_DBAgent.HwidBanByAccount(strAccountID, strReason, GetName());

	switch (ret) {
	case 0: {
		std::string msg = string_format("[HWID BAN] %s (Account: %s) PC banlandi. Sebep: %s",
			strUserID.c_str(), strAccountID.c_str(), strReason.c_str());
		g_pMain->SendHelpDescription(this, msg);
		printf("%s\n", msg.c_str());
		pTarget->Disconnect();
		std::string notice = string_format("%s'in PC'si HWID ban yedi (cheat tespiti).", strUserID.c_str());
		g_pMain->SendNotice(notice.c_str(), (uint8)Nation::ALL);
		// S114 K3 FAZ 6: Discord webhook — KAPALI (crash riski, sonra cozulecek)
		// SendDiscordHwidAlert("HWID BAN VERILDI", ...);
		break;
	}
	case 1:
		g_pMain->SendHelpDescription(this, "HWID log kaydi yok! Oyuncu Launcher yeni surumle login olmadi (HWID rapor edilmedi).");
		break;
	case 2:
		g_pMain->SendHelpDescription(this, "Bu HWID zaten banli!");
		break;
	default:
		g_pMain->SendHelpDescription(this, string_format("HWID ban basarisiz! Hata: %d", ret));
		break;
	}
	return true;
}
#pragma endregion

#pragma region CUser::HandleHwidUnbanCommand - S114 K3 FAZ 5
// Kullanim: +hwidunban <hwid_md5>
COMMAND_HANDLER(CUser::HandleHwidUnbanCommand)
{
	if (!isGM()) return false;

	if (vargs.size() < 1) {
		g_pMain->SendHelpDescription(this, "Kullanim: +hwidunban <hwid_md5_32char>");
		return true;
	}

	std::string strHwid = vargs.front();
	vargs.pop_front();

	if (strHwid.length() != 32) {
		g_pMain->SendHelpDescription(this, "Gecersiz HWID! 32 karakter MD5 hex olmali.");
		return true;
	}

	extern CDBAgent g_DBAgent;
	int rows = g_DBAgent.HwidUnban(strHwid);

	if (rows > 0) {
		g_pMain->SendHelpDescription(this, string_format("[HWID UNBAN] %s -> %d ban kaldirildi.", strHwid.c_str(), rows));
		printf("[HWID UNBAN] %s -> %d rows\n", strHwid.c_str(), rows);
	} else if (rows == 0) {
		g_pMain->SendHelpDescription(this, "Bu HWID ban listesinde degil.");
	} else {
		g_pMain->SendHelpDescription(this, "HWID unban basarisiz (DB hata).");
	}
	return true;
}
#pragma endregion

COMMAND_HANDLER(CGameServerDlg::Handlebannedcommand)
{
	if (vargs.size() < 1) { printf("Using Sample : +block CharacterName\n"); return true; }
	std::string strUserID = vargs.front();
	vargs.pop_front();
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) { printf("character name error!\n"); return true; }

	UserAuthorityUpdate(BanTypes::BANNED, nullptr, strUserID);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::Handleunbannedcommand)
{
	if (vargs.size() < 1) { printf("Using Sample : +unblock CharacterName\n"); return true; }
	std::string strUserID = vargs.front();
	vargs.pop_front();
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE) { printf("character name error!\n"); return true; }
	UserAuthorityUpdate(BanTypes::UNBAN, nullptr, strUserID);
	return true;
}


COMMAND_HANDLER(CGameServerDlg::HandleCindirellaWarClose) { return CindirellaCommand(false, -1); }

COMMAND_HANDLER(CUser::HandleCindirellaWarClose)
{
	if (!isGM()) return false;
	return g_pMain->CindirellaCommand(false, -1, this);
}

COMMAND_HANDLER(CGameServerDlg::HandleReloadCindirellaCommand) {
	//ReqSendReloadTable(e_reloadpingtype::cindtables);

	if (pCindWar.isStarted() && pCindWar.isPrepara()) 
		return true;

	for (int i = 0; i < 5; i++)
		m_CindirellaItemsArray[i].DeleteAllData();

	m_CindirellaStatArray.DeleteAllData();

	memset(&pCindWar.m_warrior, 0, sizeof(pCindWar.m_warrior));
	memset(&pCindWar.m_rogue, 0, sizeof(pCindWar.m_rogue));
	memset(&pCindWar.m_mage, 0, sizeof(pCindWar.m_mage));
	memset(&pCindWar.m_priest, 0, sizeof(pCindWar.m_priest));
	memset(&pCindWar.m_kurian, 0, sizeof(pCindWar.m_kurian));

	LoadCindirellaItemsTable();
	LoadCindirellaStatSetTable();
	LoadCindirellaSettingTable();
	LoadCindirellaRewardsTable();
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleForgettenTempleEvent)
{
	if (vargs.size() < 1) { printf("Using Sample : /ftopen Type\n"); return true; }

	uint8 Type = 0;
	if (!vargs.empty()) { Type = SafeAtoi(vargs.front(), 0, 255); vargs.pop_front(); }
	ForgettenTempleManuelOpening(Type);
	return true;
}

COMMAND_HANDLER(CGameServerDlg::HandleForgettenTempleEventClose) {
	ForgettenTempleManuelClosed();
	return true;
}