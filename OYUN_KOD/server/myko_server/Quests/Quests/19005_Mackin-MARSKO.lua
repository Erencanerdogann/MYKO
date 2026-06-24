local NPC = 19005;
local Ret = 0;


if (EVENT == 100) then
 SelectMsg(UID, 3, 974, 8075, NPC, 8265, 102);
end

--Rebirth System start
if (EVENT == 102) then
SelectMsg(UID, 3, 974, 11705, NPC, 8258 , 210);
end

if (EVENT == 210) then
	Loyalty = CheckLoyalty(UID,10000);
	MonthlyLoyalty = CheckLoyaltyMonthly(UID,10000);
	COIN = HowmuchItem(UID, 900000000);
	ITEM2 = HowmuchItem(UID, 900579000);
	GetRebLevel = GetRebirthLevel(UID);
	Slot = CheckGiveSlot(UID, 1);
	LEVEL = GetLevel(UID);
	EXP = GetExpPercent(UID);
	if Slot == false then
	SelectMsg(UID,2,-1,8900,NPC,10,1000)
	else
    if (LEVEL == 83 and EXP < 100) then
		SelectMsg(UID, 2, -1, 11694, NPC, 10, -1);
	elseif (LEVEL == 83 and GetRebLevel > 15) then
		SelectMsg(UID, 2, -1, 7116, NPC, 10, 151);
	elseif(LEVEL == 83 and EXP == 100 and ITEM2 > 0) then
		SelectMsg(UID, 48, -1, -1, NPC);			
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 0 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 1 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 2 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);		
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 3 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 4 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 5 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 6 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 7 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 8 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 9 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 10 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);		
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 11 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 12 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 13 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);	
	elseif(Loyalty >= 10000 and MonthlyLoyalty >= 10000 and GetRebLevel == 14 and COIN >= 500000000) then
		GoldLose(UID, 500000000);
        RobLoyalty(UID, 10000);
		RobLoyaltyMonthly(UID, 10000);
		GiveItem(UID, 900579000, 1)
		SelectMsg(UID, 48, -1, -1, NPC);			
	else
		SelectMsg(UID, 2, -1, 11698, NPC, 10, -1);
	end
end
end
--Rebirth System end
