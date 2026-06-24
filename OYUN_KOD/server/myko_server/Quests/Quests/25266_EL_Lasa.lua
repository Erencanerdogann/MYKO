-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 25266;

if (EVENT == 100) then;
	SelectMsg(UID, 2, -1, 44398, NPC, 40497, 101);
end

if(EVENT == 101)then
	DrakiOutZone(UID);
end