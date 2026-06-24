-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 25257;

if (EVENT == 100) then;
	SelectMsg(UID, 2, -1, 12401, NPC, 65, 101);
end

if(EVENT == 101)then
	DrakiRiftChange(UID, 1, 4);
	DrakiTowerNpcOut(UID);
end