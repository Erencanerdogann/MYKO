-- ByNoisee
local NPC = 28235
local Ret = 0
local savenum = -1

if (EVENT == 2501) then
	SelectMsg(UID, 3, savenum, 3018, NPC, 90000, 101, 90001, 102, 90002, 103, 90003, 104, 90004, 105, 90005, 106, 90006, 107, 90009, 108, 90010, 109, 90011, 110)
end 


---------------------------------------------------------------
--  1.200 Enemy Blood=Lycoan Pendant
---------------------------------------------------------------
if (EVENT == 101) then 
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1010, 4005, -1)
	end
end
 
if (EVENT == 1010) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 1200)
		GiveItem(UID, 320410013, 1)
		end
	end
end



 ---------------------------------------------------------------
-- 1.200 Enemy Blood=Lupus Pendant
 ---------------------------------------------------------------
if (EVENT == 102) then 
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1020, 4005, -1)
	end
end
 
if (EVENT == 1020) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 1200)
		GiveItem(UID, 320410012, 1)
		end
	end
end




 ---------------------------------------------------------------
-- 1.000 Enemy Blood=Lobo Pendant
 ---------------------------------------------------------------
if (EVENT == 103) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1030, 4005, -1)
	end
end
 
if (EVENT == 1030) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 1000)
		GiveItem(UID, 320410011, 1)
		end
	end
end



  ---------------------------------------------------------------
  -- 1.000 Enemy Blood=Red Chest
  ---------------------------------------------------------------
if (EVENT == 104) then 
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1040, 4005, -1)
	end
end
 
if (EVENT == 1040) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 1000)
		GiveItem(UID, 379154000, 1)
		end
	end
end




---------------------------------------------------------------
  -- 1.500 Enemy Blood=Blue
  ---------------------------------------------------------------
if (EVENT == 105) then  
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1050, 4005, -1)
	end
end
 
if (EVENT == 1050) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 1500)
		GiveItem(UID, 379156000, 1)
		end
	end
end





---------------------------------------------------------------
  -- 2.000 Enemy Blood=Green Chest
  ---------------------------------------------------------------
if (EVENT == 106) then  
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1060, 4005, -1)
	end
end
 
if (EVENT == 1060) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 2000)
		GiveItem(UID, 379155000, 1)
		end
	end
end






---------------------------------------------------------------
  -- 6.000 Enemy Blood=Strengh Belt
---------------------------------------------------------------
if (EVENT == 107) then  
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1070, 4005, -1)
	end
end
 
if (EVENT == 1070) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 6000)
		GiveItem(UID, 379155000, 1)
		end
	end
end





---------------------------------------------------------------
  -- 5.000 Enemy Blood=HP Belt
---------------------------------------------------------------
if (EVENT == 108) then  
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1080, 4005, -1)
	end
end
 
if (EVENT == 1080) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 5000)
		GiveItem(UID, 379155000, 1)
		end
	end
end









---------------------------------------------------------------
  -- 8.000 Enemy Blood=Dexetery Belt
---------------------------------------------------------------
if (EVENT == 109) then  
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1090, 4005, -1)
	end
end
 
if (EVENT == 1090) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 8000)
		GiveItem(UID, 379155000, 1)
		end
	end
end





---------------------------------------------------------------
  -- 9.999 Enemy Blood=MP Belt
---------------------------------------------------------------
if (EVENT == 110) then  
	Enemy_Blood = HowmuchItem(UID, 346250000)
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 18, -1)
	else
		SelectMsg(UID, 2, savenum, 45293, NPC, 4006, 1100, 4005, -1)
	end
end
 
if (EVENT == 1100) then
	Enemy_Blood = HowmuchItem(UID, 346250000)
	Slot = CheckGiveSlot(UID, 1)
	if SlotCheck == false then
	else
	if (Enemy_Blood < 2) then
		SelectMsg(UID, 2, savenum, 45292, NPC, 27, -1)
	else
		RobItem(UID, 346250000, 8000)
		GiveItem(UID, 379155000, 1)
		end
	end
end
