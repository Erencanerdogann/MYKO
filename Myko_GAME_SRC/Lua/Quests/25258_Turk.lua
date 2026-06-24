-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================

local NPC = 25258;

if (EVENT == 100) then;
	SelectMsg(UID, 2, -1, 44346, NPC, 65, 101, 13, -1);
end

if(EVENT == 101)then
	ZoneChange(UID, 95, 78, 58);
	DrakiRiftChange(UID, 2, 1);
	DrakiTowerNpcOut(UID);
end