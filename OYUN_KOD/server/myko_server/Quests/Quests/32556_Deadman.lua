-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 32556;

if (EVENT == 100) then
	NATION = CheckNation(UID);
	if (NATION == 2) then
		GiveItem(UID, 900071000, 1);
		SelectMsg(UID, 2, -1, 9846, NPC, 10, -1);
	else
	    GiveItem(UID, 900071000, 1);
		SelectMsg(UID, 2, -1, 1028, NPC, 10, -1);
	end
end