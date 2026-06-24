-- ByNoisee
local NPC = 25259;

if (EVENT == 100) then;
	SelectMsg(UID, 2, -1, 44350, NPC, 4170, 101,4162,-1);
end

if(EVENT == 101)then
	DrakiRiftChange(UID, 2, 4);
	DrakiTowerNpcOut(UID);
end