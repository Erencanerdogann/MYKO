-- ByNoisee
local NPC = 25264;

if (EVENT == 100) then;
	SelectMsg(UID, 2, -1, 44366, NPC, 3000, 101,13,-1);
end

if(EVENT == 101)then
	DrakiRiftChange(UID, 4, 4);
	DrakiTowerNpcOut(UID);
end