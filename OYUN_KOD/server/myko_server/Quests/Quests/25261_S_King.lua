-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 25261;

if (EVENT == 100) then;
	SelectMsg(UID, 2, -1, 44358, NPC, 40483, 101);
end

if(EVENT == 101)then
	DrakiRiftChange(UID, 3, 4);
	DrakiTowerNpcOut(UID);
end