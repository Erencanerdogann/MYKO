-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
local NPC = 31720;

if (EVENT == 100) then
	--SelectMsg(UID, 2, -1, 723, NPC, 10, -1);
	SelectMsg(UID, 3, -1, 723, NPC,45545,110,45546,111,45547,112,45548,113,45549,114,45550,115,45551,116);
end

if (EVENT == 110) then--Material of Pumpkin Mask
	ITEM = HowmuchItem(UID, 389120000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, 5000);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389120000,10);
		GiveItem(UID, 508021476,1,1);
    	end
    end
end

if (EVENT == 111) then--Material of Scream Mask
	ITEM = HowmuchItem(UID, 389121000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, -1);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389121000,10);
		GiveItem(UID, 508031477,1,1);
    	end
    end
end

if (EVENT == 112) then--Material of Devil Mask
	ITEM = HowmuchItem(UID, 389122000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, -1);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389122000,10);
		GiveItem(UID, 508041478,1,1);
    	end
    end
end

if (EVENT == 113) then--Material of Wizard Mask
	ITEM = HowmuchItem(UID, 389123000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, -1);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389123000,10);
		GiveItem(UID, 508042479,1,1);
    	end
    end
end

if (EVENT == 114) then--Material of Halloween Cane
	ITEM = HowmuchItem(UID, 389124000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, -1);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389124000,10);
		GiveItem(UID, 191600881,1,1);
    	end
    end
end

if (EVENT == 115) then--Material of Halloween Spear
	ITEM = HowmuchItem(UID, 389125000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, -1);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389125000,10);
		GiveItem(UID, 191600881,1,1);
    	end
    end
end

if (EVENT == 116) then--Material of Halloween Cane2
	ITEM = HowmuchItem(UID, 389126000);
	if (ITEM < 10) then
		SelectMsg(UID, 2, -1, 45164, NPC, 27, -1);
	else
	SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
       
	    else
		RobItem(UID, 389126000,10);
		GiveItem(UID, 191620000,1,1);
    	end
    end
end