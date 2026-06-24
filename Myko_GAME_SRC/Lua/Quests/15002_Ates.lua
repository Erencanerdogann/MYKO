-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 15002;

if (EVENT == 165) then
	SelectMsg(UID, 2, -1, 4132, NPC, 4073, 169, 4074, -1);
end

if (EVENT == 169) then
	ZoneChange(UID, 48, 133, 118)
end
