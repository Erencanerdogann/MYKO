--  ============================================================  --
-- |  NPC 28235 (Esibara) - Enemy Blood -> Trina Exchange         | --
-- |  Client 1098 uyumlu. Giris EVENT = 2501 (QUEST_HELPER 5454). | --
-- |  9 bagimsiz tier:  tier N = N*1000 Enemy Blood  ->  N Trina   | --
-- |    1000 EB -> 1 Trina ... 9000 EB -> 9 Trina                  | --
-- |  Simdilik tum odul yalniz Trina (700002000). Tier'a ozel ek  | --
-- |  odul + menu metinleri sonra elle eklenecek (TXT_* asagida).  | --
--  ============================================================  --

local NPC   = 28235
local BLOOD = 346250000   -- Enemy Blood (countable = 1, stack)
local TRINA = 700002000   -- Trina's Piece (countable = 0 -> her adet ayri slot)

-- ----------------------------------------------------------------
-- Menu metin ID'leri. SelectMsg metinleri CLIENT metin tablosundan
-- render edilir; asagidaki 46001-46019 bu NPC icin yenidir ve client
-- metin tablosuna eklenmelidir ("isimleri sonra duzeltiriz" kismi).
-- Eklenene kadar butonlar bos etiketli ama TIKLANABILIR kalir (logic test edilebilir).
-- 4006 / 4005 / 10 = mevcut standart Yes / No / Close (zaten render olur).
-- TODO client text (English):
--   46001 = "Enemy Blood Exchange" (menu header)
--   46002 = "Are you sure?" (confirm)
--   46003 = "You don't have enough Enemy Blood"
--   46004 = "Not enough inventory space"
--   46005 = "Exchange complete"
--   46011..46019 = tier buttons: "1000 Enemy Blood -> 1 Trina's Piece" ... "9000 Enemy Blood -> 9 Trina's Piece"
-- ----------------------------------------------------------------
local TXT_HEADER    = 46001
local TXT_CONFIRM   = 46002
local TXT_NOTENOUGH = 46003
local TXT_NOSLOT    = 46004
local TXT_SUCCESS   = 46005
local TXT_TIER      = { 46011, 46012, 46013, 46014, 46015, 46016, 46017, 46018, 46019 }
local TXT_YES       = 4006
local TXT_NO        = 4005
local TXT_CLOSE     = 10

-- Giris menusu: 9 tier + Kapat (9 + 1 = 10 buton, MAX_MESSAGE_EVENT = 12)
if (EVENT == 2501) then
	SelectMsg(UID, 2, -1, TXT_HEADER, NPC,
		TXT_TIER[1], 2511, TXT_TIER[2], 2512, TXT_TIER[3], 2513,
		TXT_TIER[4], 2514, TXT_TIER[5], 2515, TXT_TIER[6], 2516,
		TXT_TIER[7], 2517, TXT_TIER[8], 2518, TXT_TIER[9], 2519,
		TXT_CLOSE, -1)
end

-- Onay adimi (2511..2519): blood yeterli mi -> Evet/Hayir
if (EVENT >= 2511 and EVENT <= 2519) then
	local tier = EVENT - 2510
	local need = tier * 1000
	if (HowmuchItem(UID, BLOOD) < need) then
		SelectMsg(UID, 2, -1, TXT_NOTENOUGH, NPC, TXT_CLOSE, -1)
	else
		SelectMsg(UID, 2, -1, TXT_CONFIRM, NPC, TXT_YES, 2520 + tier, TXT_NO, 2501)
	end
end

-- Calistirma (2521..2529): tekrar dogrula, blood'u al, N adet Trina ver
if (EVENT >= 2521 and EVENT <= 2529) then
	local tier = EVENT - 2520
	local need = tier * 1000
	if (HowmuchItem(UID, BLOOD) < need) then
		SelectMsg(UID, 2, -1, TXT_NOTENOUGH, NPC, TXT_CLOSE, -1)
	elseif (CheckGiveSlot(UID, tier) == false) then
		SelectMsg(UID, 2, -1, TXT_NOSLOT, NPC, TXT_CLOSE, -1)
	else
		RobItem(UID, BLOOD, need)
		for i = 1, tier do
			GiveItem(UID, TRINA, 1)
		end
		SelectMsg(UID, 2, -1, TXT_SUCCESS, NPC, TXT_CLOSE, -1)
	end
end
