-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 30337;

if (EVENT == 100) then
	Cast = CastSkill(UID, 610096);
		if (Cast) then
			Cast = CastSkill(UID, 610096);
		else
			SelectMsg(UID, 2, -1, 8970, NPC, 10, -1);
	end	
end