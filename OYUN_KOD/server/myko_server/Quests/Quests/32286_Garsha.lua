-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 32286;

if (EVENT == 100) then
	SelectMsg(UID, 3, -1, 906, NPC, 4076, 102, 4154, -1);
end


if (EVENT == 102) then
	ZoneChange(UID, 71, 1367, 1102)
end