--  ============================================================  --
-- |  NPC 28235 (Esibara) - Enemy Blood -> Trina Exchange         | --
-- |  Client 1098. Giris EVENT = 2501 (QUEST_HELPER 5454).        | --
-- |  9 bagimsiz tier: tier N = N*1000 Enemy Blood -> N Trina.     | --
-- |    1000 EB -> 1 Trina ... 9000 EB -> 9 Trina (sadece Trina).  | --
-- |  Metin id'leri MalaysiaKO\Data icinde mevcut:                 | --
-- |    header/mesaj -> Quest_Talk_us.tbl (45238/45292/45293/856)  | --
-- |    buton label  -> Quest_Menu_us.tbl (46011-46019, 4006/4005/10) | --
-- |  ONEMLI: SelectMsg header'i Quest_TALK'tan, butonlar          | --
-- |  Quest_MENU'den okunur. Header'a Menu id'si verilirse menu    | --
-- |  acilmaz (semih'in 46001-46005'i Menu'deydi -> o yuzden eski  | --
-- |  surum acilmiyordu).                                          | --
--  ============================================================  --

local NPC   = 28235
local BLOOD = 346250000   -- Enemy Blood (countable = 1, stack)
local TRINA = 700002000   -- Trina's Piece (countable = 0 -> her adet ayri slot)

-- Header / dialog metni = Quest_Talk_us.tbl
local TXT_HEADER    = 45238   -- "Please select item to exchange."
local TXT_CONFIRM   = 45292   -- "Would you like to exchange the Enemy Blood?"
local TXT_NOTENOUGH = 45293   -- "You don't have the [Enemy Blood] item."
local TXT_NOSLOT    = 856     -- "Your inventory is full."
-- Buton metni = Quest_Menu_us.tbl
local TXT_TIER  = { 46011, 46012, 46013, 46014, 46015, 46016, 46017, 46018, 46019 }
local TXT_YES   = 4006   -- "Exchange"
local TXT_NO    = 4005   -- "Cancel"
local TXT_CLOSE = 10     -- "Confirm"

local function ShowMenu()
	SelectMsg(UID, 2, -1, TXT_HEADER, NPC,
		TXT_TIER[1], 2511, TXT_TIER[2], 2512, TXT_TIER[3], 2513,
		TXT_TIER[4], 2514, TXT_TIER[5], 2515, TXT_TIER[6], 2516,
		TXT_TIER[7], 2517, TXT_TIER[8], 2518, TXT_TIER[9], 2519,
		TXT_CLOSE, -1)
end

-- Giris menusu (9 tier + Kapat)
if (EVENT == 2501) then
	ShowMenu()
end

-- Onay adimi (2511..2519): blood yeterli mi -> Exchange/Cancel
if (EVENT >= 2511 and EVENT <= 2519) then
	local tier = EVENT - 2510
	local need = tier * 1000
	if (HowmuchItem(UID, BLOOD) < need) then
		SelectMsg(UID, 2, -1, TXT_NOTENOUGH, NPC, TXT_CLOSE, -1)
	else
		SelectMsg(UID, 2, -1, TXT_CONFIRM, NPC, TXT_YES, 2520 + tier, TXT_NO, 2501)
	end
end

-- Calistirma (2521..2529): tekrar dogrula, blood'u al, N adet Trina ver, menuyu tekrar goster
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
		ShowMenu()
	end
end
