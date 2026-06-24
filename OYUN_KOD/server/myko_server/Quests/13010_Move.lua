-- ByNoisee
-- Arena Exit → Moradon Town
local NPC = 13010;

if (EVENT == 500) then
	SelectMsg(UID, 2, -1, 4072, NPC, 4071, 501, 4072, -1);
end

if (EVENT == 501) then
	ZoneChange(UID, 21, 308, 346)
end