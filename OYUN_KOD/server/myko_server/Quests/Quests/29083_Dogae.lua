-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
-- Kontrol Edilecek.
-- ==================================================================
local NPC = 29083;

if (EVENT == 100) then
	SelectMsg(UID, 2, -1, 12370, NPC, 8933, 101, 8934, -1);
end

if (EVENT == 101) then
	SelectMsg(UID, 32, -1, NPC);
end

--22,32,33,34,35,36,37,38,39,40,41 betting