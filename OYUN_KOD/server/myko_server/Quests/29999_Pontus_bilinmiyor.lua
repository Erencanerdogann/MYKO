-- ByNoisee
local NPC = 29999;

-----------------------------------// 1.Ana Menü	//-----------------------------------
if (EVENT == 100) then	
	SelectMsg(UID, 2, -1, 4131, NPC,45432, 200, 45217, 300, 45433, 400, 45441, 500, 45230, 550, 45231, 570, 45435, 600, 45434, 660, 45442, 680, 2002, 101); 
end
-----------------------------------// 2.Ana Menü	//-----------------------------------
if (EVENT == 101) then  
	SelectMsg(UID, 2, -1, 4131, NPC,45443, 700, 45444, 720, 45445, 750, 4344, 760, 45446, 780, 45437, 800, 45448, 820, 45451, 840, 45486, 860, 2002, 102);   
end
-----------------------------------// 3.Ana Menü	//-----------------------------------
if (EVENT == 102) then  
	SelectMsg(UID, 2, -1, 4131, NPC, 45487, 890, 8975, 1300, 8976, 1310, 4479, 1200, 45528, 1900, 45531, 1910, 45540, 1930, 45542, 2810, 45216, 2799, 2003, 101);   
end

if (EVENT == 5000) then -- Pus açar
	ShowMap(UID, 450);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 200) then -- Change ID Menu
	SelectMsg(UID, 3, -1, 4131, NPC, 45438, 201, 45439, 202, 45209, 203, 4296, 100);
end

if (EVENT == 201) then -- NCS
	NCS = HowmuchItem(UID, 800032000);
	if (NCS < 1) then
		SelectMsg(UID, 2, -1, 4454, NPC, 18, 5000);
	else
		SendNameChange(UID);
	end
end

if (EVENT == 202) then -- Clan NCS
	CLANNCS = HowmuchItem(UID, 800086000);
	if (CLANNCS < 1) then
		SelectMsg(UID, 2, -1, 4670, NPC, 18, 5000);
	else
		Check = isClanLeader(UID)
		if (Check) then 
			SendClanNameChange(UID);
		else
			SelectMsg(UID, 2, -1, 4671, NPC, 10, -1);
		end
	end
end

if (EVENT == 203) then -- Tag Name Change
	TCS = HowmuchItem(UID, 800099000);
	if (TCS < 1) then
		SelectMsg(UID, 2, -1, 4454, NPC, 18, 5000);
	else
		SendTagNameChangePanel(UID);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 300) then  ----------- KNIGHT CASH MENU
	SelectMsg(UID, 3, -1, 45238, NPC, 45218, 301, 45219, 302, 45220, 303, 45222, 304, 45224, 305, 45226, 306, 4296, 100);
end

if (EVENT == 301) then  -- 100 KNIGHT CASH
	ITEMA = HowmuchItem(UID, 700082000);
	if (ITEMA == 0) then
		SelectMsg(UID, 2, -1, 45239, NPC, 18, 5000); --44302
	else
		SelectMsg(UID, 2, -1, 91001, NPC, 40326, -1);
		RobItem(UID,700082000,1);
		GiveBalance(UID, 100, 0); --bu şekilde bakiye verir

		
	end
end

if (EVENT == 302) then  -- 350 KNIGHT CASH
	ITEMA = HowmuchItem(UID, 700083000);
	if (ITEMA == 0) then
		SelectMsg(UID, 2, -1, 45240, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 91001, NPC, 40326, -1);
		RobItem(UID,700083000,1);
		GiveBalance(UID, 350, 0);
	end
end

if (EVENT == 303) then  -- 700 KNIGHT CASH
	ITEMA = HowmuchItem(UID, 700084000);
	if (ITEMA == 0) then
		SelectMsg(UID, 2, -1, 45241, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 91001, NPC, 40326, -1);
		RobItem(UID,700084000,1);
		GiveBalance(UID, 700, 0);
	end
end

if (EVENT == 304) then  -- 1200 KNIGHT CASH
	ITEMA = HowmuchItem(UID, 700089000);
	if (ITEMA == 0) then
		SelectMsg(UID, 2, -1, 45242, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 91001, NPC, 40326, -1);
		RobItem(UID,700089000,1);
		GiveBalance(UID, 1200, 0);
	end
end

if (EVENT == 305) then -- 2100 KNIGHT CASH
	ITEMA = HowmuchItem(UID, 700079000);
	if (ITEMA == 0) then
		SelectMsg(UID, 2, -1, 45243, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 91001, NPC, 40326, -1);
		RobItem(UID,700079000,1);
		GiveBalance(UID, 2100, 0);
	end
end


if (EVENT == 306) then  -- 10000 KNIGHT CASH
	ITEMA = HowmuchItem(UID, 700088000);
	if (ITEMA == 0) then
		SelectMsg(UID, 2, -1, 45244, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 91001, NPC, 40326, -1);
		RobItem(UID,700088000,1);
		GiveBalance(UID, 10000, 0);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 400) then
	SelectMsg(UID, 3, -1, 45238, NPC, 8574, 1950, 7803, 900, 8808, 2750, 10005, 2760, 45241, 480, 4296, 100);
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 1950) then -- Platinum Premium
	SelectMsg(UID, 2, -1, 45286, NPC, 45545, 2900, 45546, 2902, 45547, 2904, 45548, 2906);
end

if (EVENT == 2900) then -- [Platinum Premium 1 Day]
	DCPREM = HowmuchItem(UID, 800880961);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
	else
		EVENT = 2901
	end
end

if (EVENT == 2901) then
	DCPREM = HowmuchItem(UID, 800880961);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 800880961, 1);
		GivePremium(UID, 7, 1);
		end	
	end
end

if (EVENT == 2902) then -- [Platinum Premium 3 Days]
	DCPREM = HowmuchItem(UID, 800880950);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
	else
		EVENT = 2903
	end
end

if (EVENT == 2903) then
	DCPREM = HowmuchItem(UID, 800880950);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 800880950, 1);
		GivePremium(UID, 7, 3);
		end	
	end
end

if (EVENT == 2904) then -- [Platinum Premium 7 Days]
	DCPREM = HowmuchItem(UID, 800880951);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
	else
		EVENT = 2905
	end
end

if (EVENT == 2905) then
	DCPREM = HowmuchItem(UID, 800880951);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 800880951, 1);
		GivePremium(UID, 7, 7);
		end	
	end
end

if (EVENT == 2906) then -- [Platinum Premium 30 Days]
	DCPREM = HowmuchItem(UID, 800880754);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
	else
		EVENT = 2907
	end
end

if (EVENT == 2907) then
	DCPREM = HowmuchItem(UID, 800880754);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45276, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 10)
     if SlotCheck == false then
    else   
		RobItem(UID, 800880754, 1);
	GiveItem(UID, 800013000, 1); -- 1500 HP+ Scroll(L)
	GiveItem(UID, 800010000, 1); -- 300 Defense+ Scroll(L)
	GiveItem(UID, 800014000, 1); -- Scroll of Attack
	GiveItem(UID, 800015000, 1); -- Speed-Up Potion
	GiveItem(UID, 800050000, 5); -- Mount Scroll - %50 EXP
	GiveItem(UID, 700002000, 1); -- Trina's Piece
	GiveItem(UID, 800087000, 1); -- Spirit of Merchant
	GiveItem(UID, 700111000, 1); -- Voucher of Offline Merchant
	GiveItem(UID, 389320000, 1); -- Premium Potion HP
	GiveItem(UID, 389350000, 1); -- Premium Potion MP
		GivePremium(UID, 7, 30);
		end	
	end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 900) then -- Gold Premium
	SelectMsg(UID, 2, -1, 45287, NPC, 45549, 2908, 45550, 2910, 45551, 2912, 45552, 2914);
end

if (EVENT == 2908) then -- [Gold Premium 1 Day]
	DCPREM = HowmuchItem(UID, 300080958);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
	else
		EVENT = 2909
	end
end

if (EVENT == 2909) then
	DCPREM = HowmuchItem(UID, 300080958);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 300080958, 1);
		GivePremium(UID, 5, 1);
		end	
	end
end

if (EVENT == 2910) then -- [Gold Premium 3 Days]
	DCPREM = HowmuchItem(UID, 300080959);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
	else
		EVENT = 2911
	end
end

if (EVENT == 2911) then
	DCPREM = HowmuchItem(UID, 300080959);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 300080959, 1);
		GivePremium(UID, 5, 3);
		end	
	end
end

if (EVENT == 2912) then -- [Gold Premium 7 Days]
	DCPREM = HowmuchItem(UID, 300080960);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
	else
		EVENT = 2913
	end
end

if (EVENT == 2913) then
	DCPREM = HowmuchItem(UID, 300080960);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 300080960, 1);
		GivePremium(UID, 5, 7);
		end	
	end
end

if (EVENT == 2914) then -- [Gold Premium 30 Days]
	DCPREM = HowmuchItem(UID, 300080930);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
	else
		EVENT = 2915
	end
end

if (EVENT == 2915) then
	DCPREM = HowmuchItem(UID, 300080930);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45290, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 5)
     if SlotCheck == false then
    else   
		RobItem(UID, 300080930, 1);
		GiveItem(UID, 800013000, 1); -- 1500 HP+ Scroll(L)
		GiveItem(UID, 800010000, 1); -- 300 Defense+ Scroll(L)
		GiveItem(UID, 800014000, 1); -- Scroll of Attack
		GiveItem(UID, 800015000, 1); -- Speed-Up Potion
		GiveItem(UID, 800050000, 1); -- Mount Scroll - %50 EXP
		GivePremium(UID, 5, 30);
		end	
	end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 2750) then -- Silver Premium
	SelectMsg(UID, 2, -1, 45288, NPC, 45553, 2916, 45554, 2918, 45555 , 2920, 45556, 2922);
end

if (EVENT == 2916) then -- [Silver Premium 1 Day]
	DCPREM = HowmuchItem(UID, 810936943);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
	else
		EVENT = 2917
	end
end

if (EVENT == 2917) then
	DCPREM = HowmuchItem(UID, 810936943);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 810936943, 1);
		GivePremium(UID, 4, 1);
		end	
	end
end

if (EVENT == 2918) then -- [Silver Premium 3 Days]
	DCPREM = HowmuchItem(UID, 810936944);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
	else
		EVENT = 2919
	end
end

if (EVENT == 2919) then
	DCPREM = HowmuchItem(UID, 810936944);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 810936944, 1);
		GivePremium(UID, 4, 3);
		end	
	end
end

if (EVENT == 2920) then -- [Silver Premium 7 Days]
	DCPREM = HowmuchItem(UID, 810936957);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
	else
		EVENT = 2921
	end
end

if (EVENT == 2921) then
	DCPREM = HowmuchItem(UID, 810936957);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 810936957, 1);
		GivePremium(UID, 4, 7);
		end	
	end
end

if (EVENT == 2922) then -- [Silver Premium 30 Days]
	DCPREM = HowmuchItem(UID, 810936742);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
	else
		EVENT = 2923
	end
end

if (EVENT == 2923) then
	DCPREM = HowmuchItem(UID, 810936742);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45284, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 5)
     if SlotCheck == false then
    else   
		RobItem(UID, 810936742, 1);
		GiveItem(UID, 800013000, 1); -- 1500 HP+ Scroll(L)
		GiveItem(UID, 800010000, 1); -- 300 Defense+ Scroll(L)
		GiveItem(UID, 800014000, 1); -- Scroll of Attack
		GiveItem(UID, 800015000, 1); -- Speed-Up Potion
		GiveItem(UID, 800050000, 1); -- Mount Scroll - %50 EXP
		GivePremium(UID, 4, 30);
		end	
	end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 2760) then -- Bronze Premium
	SelectMsg(UID, 2, -1, 45289, NPC, 45557, 2924, 45558, 2926, 45559, 2928, 45560, 2930);
end

if (EVENT == 2924) then -- [Bronze Premium 1 Day]
	DCPREM = HowmuchItem(UID, 814042940);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
	else
		EVENT = 2925
	end
end

if (EVENT == 2925) then
	DCPREM = HowmuchItem(UID, 814042940);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 814042940, 1);
		GivePremium(UID, 3, 1);
		end	
	end
end

if (EVENT == 2926) then -- [Bronze Premium 3 Days]
	DCPREM = HowmuchItem(UID, 814042941);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
	else
		EVENT = 2927
	end
end

if (EVENT == 2927) then
	DCPREM = HowmuchItem(UID, 814042941);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 814042941, 1);
		GivePremium(UID, 3, 3);
		end	
	end
end

if (EVENT == 2928) then -- [Bronze Premium 7 Days]
	DCPREM = HowmuchItem(UID, 814042942);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
	else
		EVENT = 2929
	end
end

if (EVENT == 2929) then
	DCPREM = HowmuchItem(UID, 814042942);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
    else   
		RobItem(UID, 814042942, 1);
		GivePremium(UID, 3, 7);
		end	
	end
end

if (EVENT == 2930) then -- [Bronze Premium 30 Days]
	DCPREM = HowmuchItem(UID, 814042737);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
	else
		EVENT = 2931
	end
end

if (EVENT == 2931) then
	DCPREM = HowmuchItem(UID, 814042737);
	if (DCPREM < 1 or DCPREM == 0) then
		SelectMsg(UID, 2, -1, 45285, NPC, 18, 5000);
		else
		SlotCheck = CheckGiveSlot(UID, 5)
     if SlotCheck == false then
    else   
		RobItem(UID, 814042737, 1);
		GiveItem(UID, 800013000, 1); -- 1500 HP+ Scroll(L)
		GiveItem(UID, 800010000, 1); -- 300 Defense+ Scroll(L)
		GiveItem(UID, 800014000, 1); -- Scroll of Attack
		GiveItem(UID, 800015000, 1); -- Speed-Up Potion
		GiveItem(UID, 800050000, 1); -- Mount Scroll - %50 EXP
		GivePremium(UID, 3, 30);
		end	
	end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 480) then -- Clan Premium
    CheckLeader = isClanLeader(UID) 
    if (CheckLeader == false) then
        SelectMsg(UID, 2, -1, 45291, NPC, 10, -1);
    else
    CLANPREM = HowmuchItem(UID, 399300914);
    if (CLANPREM < 1) then
        SelectMsg(UID, 2, -1, 45245, NPC, 18, 5000);
    else
        RobItem(UID, 399300914, 1);
        GiveClanPremium(UID,2,30)
		SelectMsg(UID, 2, -1, 45034, NPC, 10, -1);
		end
    end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 500) then -- Genie Voucher
	SelectMsg(UID, 2, -1, 45238, NPC, 45468, 501, 45469, 503, 45470, 505, 45471, 507, 45472, 509, 4296, 100);
end

if (EVENT == 501) then -- Genie Voucher ( 2 Hours ) 
	EXPPREM = HowmuchItem(UID, 700092000);
	if (EXPPREM < 1) then
		SelectMsg(UID, 2, -1, 45246, NPC, 18, 5000);
	else
		GenieExchange(UID, 700092000, 2);
	end
end

if (EVENT == 503) then -- Genie Voucher ( 1 Day ) 
	EXPPREM = HowmuchItem(UID, 700093000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45246, NPC, 18, 5000);
	else
		GenieExchange(UID, 700093000, 24);
	end
end

if (EVENT == 505) then -- Genie Voucher ( 3 Days ) 
	EXPPREM = HowmuchItem(UID, 700094000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45246, NPC, 18, 5000);
	else
    GenieExchange(UID, 700094000, 72);
	end
end

if (EVENT == 507) then -- Genie Voucher ( 7 Days ) 
	EXPPREM = HowmuchItem(UID, 700095000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45246, NPC, 18, 5000);
	else
    GenieExchange(UID, 700095000, 168);
	end
end

if (EVENT == 509) then -- Genie Voucher ( 15 Days ) 
	EXPPREM = HowmuchItem(UID, 700091000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45246, NPC, 18, 5000);
	else
    GenieExchange(UID, 700091000, 360);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 550) then -- Voucher of Automatic Loot ( 15 Days ) 
	LOOTING = HowmuchItem(UID, 750680000);
	if (LOOTING < 1 or LOOTING == 0) then
		SelectMsg(UID, 2, -1, 45247, NPC, 18, 5000);
	else
    EVENT = 551
	end
end

if (EVENT == 551) then
	LOOTING = HowmuchItem(UID, 750680000);
	if (LOOTING < 1 or LOOTING == 0) then
		SelectMsg(UID, 2, -1, 45247, NPC, 18, 5000);
	else
	RobItem(UID, 750680000, 1);
	GiveItem(UID, 850680000, 1, 15);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 570) then -- Voucher of Genie (15 Days) + Auto Loot (15 Days)
	EXPPREM = HowmuchItem(UID, 700191000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45248, NPC, 18, 5000);
	else
    EVENT = 571
	end
end

if (EVENT == 571) then
	EXPPREM = HowmuchItem(UID, 700191000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45248, NPC, 18, 5000);
	else
	RobItem(UID, 700191000, 1);
	GiveItem(UID, 700091000, 1);
	GiveItem(UID, 750680000, 1);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 600) then
	SelectMsg(UID, 3, -1, 4901, NPC, 4285, 601, 4286, 602, 4287, 1150, 4420, 611, 4421, 616, 4589, 621, 4588, 626, 45502, 1600, 4504, 631, 4296, 100);
end

if (EVENT == 601) then
	SelectMsg(UID, 11, savenum, 4432, NPC);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 602) then -- Valkyrie Armor
	ITEMARMOR = HowmuchItem(UID, 800180000);
	if (ITEMARMOR > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 603, 4289, 604, 4290, 605, 4291, 606);
	else
		SelectMsg(UID, 2, -1, 4921, NPC, 18, 5000);
	end
end

if (EVENT == 603) then
	Check = isRoomForItem(UID, 508011441);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800180000, 1)
		GiveItem(UID, 508011441, 1, 30)
	end
end

if (EVENT == 604) then
	Check = isRoomForItem(UID, 508011442);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800180000, 1)
		GiveItem(UID, 508011442, 1, 30)
	end
end

if (EVENT == 605) then
	Check = isRoomForItem(UID, 508011443);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800180000, 1)
		GiveItem(UID, 508011443, 1, 30)
	end
end

if (EVENT == 606) then
	Check = isRoomForItem(UID, 508011444);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800180000, 1)
		GiveItem(UID, 508011444, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1150) then -- Valkyrie Helmet
	ITEMHELMET = HowmuchItem(UID, 800170000);
	if (ITEMHELMET > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 607, 4293, 608, 4294, 609, 4295, 610);
	else
		SelectMsg(UID, 2, -1, 4911, NPC, 18, 5000);
	end
end

if (EVENT == 607) then
	Check = isRoomForItem(UID, 508013318);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800170000, 1)
		GiveItem(UID, 508013318, 1, 30)
	end
end

if (EVENT == 608) then
	Check = isRoomForItem(UID, 508013319);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800170000, 1)
		GiveItem(UID, 508013319, 1, 30)
	end
end

if (EVENT == 609) then
	Check = isRoomForItem(UID, 508013320);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800170000, 1)
		GiveItem(UID, 508013320, 1, 30)
	end
end

if (EVENT == 610) then
	Check = isRoomForItem(UID, 508013321);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800170000, 1)
		GiveItem(UID, 508013321, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 611) then -- Gryphon Armor
	ITEMGRYPA = HowmuchItem(UID, 800240000);
	if (ITEMGRYPA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 612, 4289, 613, 4290, 614, 4291, 615);
	else
		SelectMsg(UID, 2, -1, 6488, NPC, 18, 5000);
	end
end

if (EVENT == 612) then
	Check = isRoomForItem(UID, 508471453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800240000, 1)
		GiveItem(UID, 508471453, 1, 30)
	end
end

if (EVENT == 613) then
	Check = isRoomForItem(UID, 508471454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800240000, 1)
		GiveItem(UID, 508471454, 1, 30)
	end
end

if (EVENT == 614) then
	Check = isRoomForItem(UID, 508471455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800240000, 1)
		GiveItem(UID, 508471455, 1, 30)
	end
end

if (EVENT == 615) then
	Check = isRoomForItem(UID, 508471456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800240000, 1)
		GiveItem(UID, 508471456, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 616) then -- Gryphon Helmet
	ITEMGRYPH = HowmuchItem(UID, 800230000);
	if (ITEMGRYPH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 617, 4293, 618, 4294, 619, 4295, 620);
	else
		SelectMsg(UID, 2, -1, 6497, NPC, 18, 5000);
	end
end

if (EVENT == 617) then
	Check = isRoomForItem(UID, 508473453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800230000, 1)
		GiveItem(UID, 508473453, 1, 30)
	end
end

if (EVENT == 618) then
	Check = isRoomForItem(UID, 508473454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800230000, 1)
		GiveItem(UID, 508473454, 1, 30)
	end
end

if (EVENT == 619) then
	Check = isRoomForItem(UID, 508473455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800230000, 1)
		GiveItem(UID, 508473455, 1, 30)
	end
end

if (EVENT == 620) then
	Check = isRoomForItem(UID, 508473456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800230000, 1)
		GiveItem(UID, 508473456, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 621) then -- Bahamut Armor
	ITEMBHMTA = HowmuchItem(UID, 800270000);
	if (ITEMBHMTA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 622, 4289, 623, 4290, 624, 4291, 625);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 622) then
	Check = isRoomForItem(UID, 508051466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800270000, 1)
		GiveItem(UID, 508051466, 1, 30)
	end
end

if (EVENT == 623) then
	Check = isRoomForItem(UID, 508051467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800270000, 1)
		GiveItem(UID, 508051467, 1, 30)
	end
end

if (EVENT == 624) then
	Check = isRoomForItem(UID, 508051468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800270000, 1)
		GiveItem(UID, 508051468, 1, 30)
	end
end

if (EVENT == 625) then
	Check = isRoomForItem(UID, 508051469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800270000, 1)
		GiveItem(UID, 508051469, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 626) then -- Bahamut Helmet
	ITEMBHMTH = HowmuchItem(UID, 800260000);
	if (ITEMBHMTH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 627, 4293, 628, 4294, 629, 4295, 630);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 1000);
	end
end

if (EVENT == 627) then
	Check = isRoomForItem(UID, 508053466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800260000, 1)
		GiveItem(UID, 508053466, 1, 30)
	end
end

if (EVENT == 628) then
	Check = isRoomForItem(UID, 508053467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800260000, 1)
		GiveItem(UID, 508053467, 1, 30)
	end
end

if (EVENT == 629) then
	Check = isRoomForItem(UID, 508053468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800260000, 1)
		GiveItem(UID, 508053468, 1, 30)
	end
end

if (EVENT == 630) then
	Check = isRoomForItem(UID, 508053469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800260000, 1)
		GiveItem(UID, 508053469, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 631) then -- Pathos Attack
	ITEMPTHS = HowmuchItem(UID, 800250000);
	if (ITEMPTHS > 0) then
		SelectMsg(UID, 3, -1, 748, NPC, 4509, 632, 4510, 633);
	else
		SelectMsg(UID, 2, -1, 749, NPC, 18, 1000);
	end
end

if (EVENT == 632) then
	SelectMsg(UID, 3, -1, 750, NPC, 4505, 634, 4506, 635, 4507, 636, 4508, 637);
end

if (EVENT == 634) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID, 502573462, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 635) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID, 503573463, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 636) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID , 504573464, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 637) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID, 505573465, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 633) then -- Pathos Defans
	SelectMsg(UID, 3, -1, 751, NPC, 4514, 638, 4515, 639, 4516, 640, 4517, 641);
end

if (EVENT == 638) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID, 511573471, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 639) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID, 512573472, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 640) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID , 513573473, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 641) then
	Check = isRoomForItem(UID, 800250000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 800250000, 1)
		GiveItem(UID, 514573474, 1, 30)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 660) then -- Starter Pack
	VIPPackage = HowmuchItem(UID, 810035000);
	if (VIPPackage < 1 or VIPPackage == 0) then
		SelectMsg(UID, 2, -1, 45249, NPC, 18, 5000);
	else
    EVENT = 661
	end
end

if (EVENT == 661) then
	VIPPackage = HowmuchItem(UID, 810035000);
	if (VIPPackage < 1 or VIPPackage == 0) then
		SelectMsg(UID, 2, -1, 45249, NPC, 18, 5000);
	else
SlotCheck = CheckGiveSlot(UID, 22)
     if SlotCheck == false then
    else   
   	RobItem(UID, 810035000, 1);
	GiveItem(UID, 800880754, 1); -- Platinum Premium
	GiveItem(UID, 700191000, 1); -- Genie + Auto Loot
	GiveItem(UID, 814663000, 1); -- Special Tattoo
	GiveItem(UID, 800180000, 1); -- Valkyrie Armor
	GiveItem(UID, 800170000, 1); -- Valkyrie Helmet
	GiveItem(UID, 353300000, 1); -- Gold Star
	GiveItem(UID, 800440000, 1); -- Magic Bag
	GiveItem(UID, 800440000, 1); -- Magic Bag
	GiveItem(UID, 700111000, 1); -- Offline Merchant
	GiveItem(UID, 800605000, 1); -- Voucher of Infinite Arrow
	GiveItem(UID, 389390000, 1); -- Premium Potion HP
	GiveItem(UID, 389400000, 1); -- Premium Potion MP
	GiveItem(UID, 800050000, 5); -- Mount Scroll - %50 EXP
	GiveItem(UID, 800008000, 1); -- Lion Scroll (L)
	GiveItem(UID, 800015000, 1); -- Speed-Up Potion
	GiveItem(UID, 800061000, 1); -- Weapon Enchant Scroll
	GiveItem(UID, 800062000, 1); -- Armor Enchant Scroll
	GiveItem(UID, 700001000, 1); -- Redistribution Item
	GiveItem(UID, 700089000, 1); -- 1200 Knight Cash
	GiveItem(UID, 800074000, 1); -- NP increase item
	GiveItem(UID, 800442000, 1); -- VIP Key
	GiveItem(UID, 810227000, 1); -- Genie Hammer
	end
end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 680) then -- Voucher of TL
	SelectMsg(UID, 2, -1, 45238, NPC, 45473, 681, 45474, 683, 45475, 685, 45476, 687, 45477, 689, 45478, 691);
end

if (EVENT == 681) then -- 1 TL Voucher
	EXPPREM = HowmuchItem(UID, 346600000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45250, NPC, 18, 5000);
	else
    EVENT = 682
	end
end

if (EVENT == 682) then
	EXPPREM = HowmuchItem(UID, 346600000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45250, NPC, 18, 5000);
	else
	RobItem(UID, 346600000, 1);
	GiveBalance(UID, 0, 1);
	end
end

if (EVENT == 683) then -- 5 TL Voucher
	EXPPREM = HowmuchItem(UID, 346700000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45251, NPC, 18, 5000);
	else
    EVENT = 684
	end
end

if (EVENT == 684) then
	EXPPREM = HowmuchItem(UID, 346700000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45251, NPC, 18, 5000);
	else
	RobItem(UID, 346700000, 1);
	GiveBalance(UID, 0, 5);
	end
end

if (EVENT == 685) then -- 10 TL Voucher
	EXPPREM = HowmuchItem(UID, 346800000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45252, NPC, 18, 5000);
	else
    EVENT = 686
	end
end

if (EVENT == 686) then
	EXPPREM = HowmuchItem(UID, 346800000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45252, NPC, 18, 5000);
	else
	RobItem(UID, 346800000, 1);
	GiveBalance(UID, 0, 10);
	end
end

if (EVENT == 687) then -- 20 TL Voucher
	EXPPREM = HowmuchItem(UID, 346900000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45253, NPC, 18, 5000);
	else
    EVENT = 688
	end
end

if (EVENT == 688) then
	EXPPREM = HowmuchItem(UID, 346900000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45253, NPC, 18, 5000);
	else
	RobItem(UID, 346900000, 1);
	GiveBalance(UID, 0, 20);
	end
end

if (EVENT == 689) then -- 50 TL Voucher
	EXPPREM = HowmuchItem(UID, 347000000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45254, NPC, 18, 5000);
	else
    EVENT = 690
	end
end

if (EVENT == 690) then
	EXPPREM = HowmuchItem(UID, 347000000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45254, NPC, 18, 5000);
	else
	RobItem(UID, 347000000, 1);
	GiveBalance(UID, 0, 50);
	end
end

if (EVENT == 691) then -- 100 TL Voucher
	EXPPREM = HowmuchItem(UID, 347100000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45255, NPC, 18, 5000);
	else
    EVENT = 692
	end
end

if (EVENT == 692) then
	EXPPREM = HowmuchItem(UID, 347100000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45255, NPC, 18, 5000);
	else
	RobItem(UID, 347100000, 1);
	GiveBalance(UID, 0, 100);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 700) then -- +EXP Scroll
	SelectMsg(UID, 2, -1, 45238, NPC, 45479, 701, 45480, 703, 45481, 705);
end


if (EVENT == 701) then -- +EXP Scroll ( 1M )
	EXPPREM = HowmuchItem(UID, 347500000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45256, NPC, 18, 5000);
	else
    EVENT = 702
	end
end

if (EVENT == 702) then
	EXPPREM = HowmuchItem(UID, 347500000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45256, NPC, 18, 5000);
	else
	RobItem(UID, 347500000, 1);
	ExpChange(UID,1000000, true);
	end
end

if (EVENT == 703) then -- +EXP Scroll ( 3M )
	EXPPREM = HowmuchItem(UID, 347600000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45257, NPC, 18, 5000);
	else
    EVENT = 704
	end
end

if (EVENT == 704) then
	EXPPREM = HowmuchItem(UID, 347600000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45257, NPC, 18, 5000);
	else
	RobItem(UID, 347600000, 1);
	ExpChange(UID,3000000, true);
	end
end

if (EVENT == 705) then -- +EXP Scroll ( 5M )
	EXPPREM = HowmuchItem(UID, 347700000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45258, NPC, 18, 5000);
	else
    EVENT = 706
	end
end

if (EVENT == 706) then
	EXPPREM = HowmuchItem(UID, 347700000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45258, NPC, 18, 5000);
	else
	RobItem(UID, 347700000, 1);
	ExpChange(UID,5000000, true);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 720) then -- +NP Scroll
	SelectMsg(UID, 2, -1, 45238, NPC, 45482, 721, 45483, 723, 45484, 725);
end


if (EVENT == 721) then -- +NP Scroll ( 1M )
	EXPPREM = HowmuchItem(UID, 347200000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45259, NPC, 18, 5000);
	else
    EVENT = 722
	end
end

if (EVENT == 722) then
	EXPPREM = HowmuchItem(UID, 347200000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45259, NPC, 18, 5000);
	else
	RobItem(UID, 347200000, 1);
	GiveLoyalty(UID, 1000);
	end
end

if (EVENT == 723) then -- +NP Scroll ( 3M )
	EXPPREM = HowmuchItem(UID, 347300000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45260, NPC, 18, 5000);
	else
    EVENT = 724
	end
end

if (EVENT == 724) then
	EXPPREM = HowmuchItem(UID, 347300000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45260, NPC, 18, 5000);
	else
	RobItem(UID, 347300000, 1);
	GiveLoyalty(UID, 3000);
	end
end

if (EVENT == 725) then -- +NP Scroll ( 5M )
	EXPPREM = HowmuchItem(UID, 347400000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45261, NPC, 18, 5000);
	else
    EVENT = 726
	end
end

if (EVENT == 726) then
	EXPPREM = HowmuchItem(UID, 347400000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45261, NPC, 18, 5000);
	else
	RobItem(UID, 347400000, 1);
	GiveLoyalty(UID, 5000);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 750) then -- Death Knight Skulls
	SelectMsg(UID, 2, -1, 45238, NPC, 45485, 751, 45497, 1500, 45498, 1502, 45499 , 1504, 45500, 1506, 45501, 1508);
end

if (EVENT == 751) then -- [ +0 ] Death Knight Skulls Emblem
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 100000000 or EXPPREM2 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
    EVENT = 752
		end
end

if (EVENT == 752) then
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 100000000 or EXPPREM2 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
	RobItem(UID, 346250000, 1000);
	GoldLose(UID, 100000000);
	GiveItem(UID, 814050748, 1, 30);
	end
end

if (EVENT == 1500) then -- [ +1 ] Death Knight Skulls Emblem
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814050748);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 200000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
    EVENT = 1501
	end
end

if (EVENT == 1501) then
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814050748);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 200000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
	RobItem(UID, 346250000, 1000);
	RobItem(UID, 814050748, 1);
	GoldLose(UID, 200000000);
	GiveItem(UID, 814051749, 1, 30);
	end
end

if (EVENT == 1502) then -- [ +2 ] Death Knight Skulls Emblem
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814051749);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 300000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
    EVENT = 1503
	end
end

if (EVENT == 1503) then
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814051749);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 300000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
	RobItem(UID, 346250000, 1000);
	RobItem(UID, 814051749, 1);
	GoldLose(UID, 300000000);
	GiveItem(UID, 814052750, 1, 30);
	end
end

if (EVENT == 1504) then -- [ +3 ] Death Knight Skulls Emblem
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814052750);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 400000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
    EVENT = 1505
	end
end

if (EVENT == 1505) then
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814052750);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 400000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
	RobItem(UID, 346250000, 1000);
	RobItem(UID, 814052750, 1);
	GoldLose(UID, 400000000);
	GiveItem(UID, 814053751, 1, 30);
	end
end

if (EVENT == 1506) then -- [ +4 ] Death Knight Skulls Emblem
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814053751);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 500000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
    EVENT = 1507
	end
end

if (EVENT == 1507) then
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814053751);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 500000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
	RobItem(UID, 346250000, 1000);
	RobItem(UID, 814053751, 1);
	GoldLose(UID, 500000000);
	GiveItem(UID, 814054752, 1, 30);
	end
end

if (EVENT == 1508) then -- [ +5 ] Death Knight Skulls Emblem
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814054752);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 600000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
    EVENT = 1509
	end
end

if (EVENT == 1509) then
	EXPPREM1 = HowmuchItem(UID, 346250000);
	EXPPREM2 = HowmuchItem(UID, 900000000);
	EXPPREM3 = HowmuchItem(UID, 814054752);
	if (EXPPREM1 < 1000 or EXPPREM1 == 0 or EXPPREM2 < 600000000 or EXPPREM2 == 0 or EXPPREM3 < 1 or EXPPREM3 == 0) then
		SelectMsg(UID, 2, -1, 45262, NPC, 18, 5000);
	else
	RobItem(UID, 346250000, 1000);
	RobItem(UID, 814054752, 1);
	GoldLose(UID, 600000000);
	GiveItem(UID, 814055753, 1, 30);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 760) then -- Extra Inventory
	EXPPREM = HowmuchItem(UID, 800440000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45263, NPC, 18, 5000);
	else
    EVENT = 762
	end
end

if (EVENT == 762) then
	EXPPREM = HowmuchItem(UID, 800440000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45263, NPC, 18, 5000);
	else
	RobItem(UID, 800440000, 1);
	GiveItem(UID, 700011001, 1, 30);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 780) then -- Voucher of Offline Merchant
	EXPPREM = HowmuchItem(UID, 700111000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45264, NPC, 18, 5000);
	else
    EVENT = 781
	end
end

if (EVENT == 781) then
	EXPPREM = HowmuchItem(UID, 700111000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45264, NPC, 18, 5000);
	else
	RobItem(UID, 700111000, 1);
	GiveItem(UID, 924041913, 1, 30);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 800) then -- Voucher of Merchant's Eye
	EXPPREM = HowmuchItem(UID, 810163000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45265, NPC, 18, 5000);
	else
    EVENT = 801
	end
end

if (EVENT == 801) then
	EXPPREM = HowmuchItem(UID, 810163000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45265, NPC, 18, 5000);
	else
	RobItem(UID, 810163000, 1);
	GiveItem(UID, 810168000, 1, 30);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 820) then -- Voucher of Infinite Arrow (15 Days)
	EXPPREM = HowmuchItem(UID, 800605000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45266, NPC, 18, 5000);
	else
    EVENT = 821
	end
end

if (EVENT == 821) then
	EXPPREM = HowmuchItem(UID, 800605000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45266, NPC, 18, 5000);
	else
	RobItem(UID, 800605000, 1);
	GiveItem(UID, 800606000, 1, 15);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 840) then -- Voucher of Infinite Cure (15 Days)
	EXPPREM = HowmuchItem(UID, 346390000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45267, NPC, 18, 5000);
	else
    EVENT = 841
	end
end

if (EVENT == 841) then
	EXPPREM = HowmuchItem(UID, 346390000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45267, NPC, 18, 5000);
	else
	RobItem(UID, 346390000, 1);
	GiveItem(UID, 346391000, 1, 15);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 860) then -- Job Change Menü
	SelectMsg(UID, 3, -1, 4131, NPC, 45426, 870, 45440, 880, 4296, 100);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 870) then  -- Job Change (Master)
JOBCHANGEITEM = HowmuchItem(UID, 700112000);
	if (JOBCHANGEITEM > 0) then
	Class = CheckClass(UID);
	if (Class == 1 or Class == 5 or Class == 6) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45429, 872, 45430, 873, 45431, 874);
	elseif (Class == 2 or Class == 7 or Class == 8) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45428, 871, 45430, 873, 45431, 874);
	elseif (Class == 3 or Class == 9 or Class == 10) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45428, 871, 45429, 872, 45431, 874);
	elseif (Class == 4 or Class == 11 or Class == 12) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45428, 871, 45429, 872, 45430, 873);
	end
    else
        SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
    end
end

if (EVENT == 871) then
JOBCHANGEITEM = HowmuchItem(UID, 700112000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		check = JobChange(UID,0,1);
		print(check);
	end
end

if (EVENT == 872) then
    JOBCHANGEITEM = HowmuchItem(UID, 700112000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,0,2);
	end
end

if (EVENT == 873) then
    JOBCHANGEITEM = HowmuchItem(UID, 700112000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,0,3);
	end
end

if (EVENT == 874) then
    JOBCHANGEITEM = HowmuchItem(UID, 700112000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,0,4);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 880) then  -- Job Change (NO Master)
JOBCHANGEITEM = HowmuchItem(UID, 700113000);
	if (JOBCHANGEITEM > 0) then
	Class = CheckClass(UID);
	if (Class == 1 or Class == 5 or Class == 6) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45429, 882, 45430, 883, 45431, 884);
	elseif (Class == 2 or Class == 7 or Class == 8) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45428, 881, 45430, 883, 45431, 884);
	elseif (Class == 3 or Class == 9 or Class == 10) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45428, 881, 45429, 882, 45431, 884);
	elseif (Class == 4 or Class == 11 or Class == 12) then
		SelectMsg(UID, 2, -1, 45268, NPC, 45428, 881, 45429, 882, 45430, 883);
	end
    else
        SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
    end
end

if (EVENT == 881) then
JOBCHANGEITEM = HowmuchItem(UID, 700113000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,1,1);
	end
end

if (EVENT == 882) then
    JOBCHANGEITEM = HowmuchItem(UID, 700113000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,1,2);
	end
end

if (EVENT == 883) then
    JOBCHANGEITEM = HowmuchItem(UID, 700113000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,1,3);
	end
end

if (EVENT == 884) then
    JOBCHANGEITEM = HowmuchItem(UID, 700113000);
	if (JOBCHANGEITEM < 1) then
		SelectMsg(UID, 2, -1, 45055, NPC, 18, 5000);
	else
		JobChange(UID,1,4);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 890) then  -- Exchange Stars
	SelectMsg(UID, 3, -1, 45238, NPC, 45488, 891, 45489, 893, 45490, 895);
end

if (EVENT == 891) then -- Exchange [ Bronze Star ]
	EXPPREM = HowmuchItem(UID, 353500000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45269, NPC, 18, 5000);
	else
    EVENT = 892
	end
end

if (EVENT == 892) then
	EXPPREM = HowmuchItem(UID, 353500000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45269, NPC, 18, 5000);
	else
		SlotCheck = CheckGiveSlot(UID, 3)
     if SlotCheck == false then
	else
	RobItem(UID, 353500000, 1);
	GiveItem(UID, 930520722, 1, 15);
		end
	end
end

if (EVENT == 893) then -- Exchange [ Silver Star ]
	EXPPREM = HowmuchItem(UID, 353400000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45270, NPC, 18, 5000);
	else
    EVENT = 894
	end
end

if (EVENT == 894) then
	EXPPREM = HowmuchItem(UID, 353400000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45270, NPC, 18, 5000);
	else
		SlotCheck = CheckGiveSlot(UID, 3)
     if SlotCheck == false then
	else
	RobItem(UID, 353400000, 1);
	GiveItem(UID, 930530723, 1, 15);
		end
	end
end

if (EVENT == 895) then -- Exchange [ Gold Star ]
	EXPPREM = HowmuchItem(UID, 353300000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45271, NPC, 18, 5000);
	else
    EVENT = 896
		end
	end


if (EVENT == 896) then
	EXPPREM = HowmuchItem(UID, 353300000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45271, NPC, 18, 5000);
	else
		SlotCheck = CheckGiveSlot(UID, 3)
     if SlotCheck == false then
	else
	RobItem(UID, 353300000, 1);
	GiveItem(UID, 930540724, 1, 15);
		end
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1200) then
	REDIST = HowmuchItem(UID, 700001000);
	if (REDIST > 0) then
		SelectMsg(UID, 2, -1, 4456, NPC, 4189, 1201, 4190, 1202);
	else
		SelectMsg(UID, 2, -1, 4455, NPC, 18, 5000);
	end
end

if (EVENT == 1201) then
	REDIST = HowmuchItem(UID, 700001000);
	if (REDIST < 1) then 
		SelectMsg(UID, 2, -1, 4455, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 4456, NPC, 3000, 1203, 3005, -1);
	end
end

if (EVENT == 1202) then
	REDIST = HowmuchItem(UID, 700001000);
	if (REDIST < 1) then 
		SelectMsg(UID, 2, -1, 4455, NPC, 18, 5000);
	else
		SelectMsg(UID, 2, -1, 4456, NPC, 3000, 1204, 3005, -1);
	end
end


if (EVENT == 1203) then
	REDIST = HowmuchItem(UID, 700001000);
	if (REDIST < 1) then 
		SelectMsg(UID, 2, -1, 4455, NPC, 18, 5000);
	else
		ResetSkillPoints(UID);
	end
end

if (EVENT == 1204) then
	REDIST = HowmuchItem(UID, 700001000);
	if (REDIST < 1) then 
		SelectMsg(UID, 2, -1, 4455, NPC, 18, 5000);
	else
		ResetStatPoints(UID);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if EVENT == 1300 then
    SelectMsg(UID, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

end
if EVENT == 1310 then
    SelectMsg(UID, 53, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1600) then -- Exchange Daily [Valkyrie - Bahamut - Gryphon - Pathos]
	SelectMsg(UID, 3, -1, 4901, NPC, 45503, 1601, 45504 , 1655, 45505 , 1720, 45524, 1800, 2003, 600);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1601) then -- Exchange [Daily Valkyrie Armor & Helmet]
	SelectMsg(UID, 3, -1, 4901, NPC, 45506, 1602, 45507, 1607, 45508, 1620, 45509, 1630, 45510, 1640, 45511, 1650);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1602) then -- Exchange Valkyrie Armor ( 1 Day )
	ITEMARMOR = HowmuchItem(UID, 510910000);
	if (ITEMARMOR > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1603, 4289, 1604, 4290, 1605, 4291, 1606);
	else
		SelectMsg(UID, 2, -1, 4921, NPC, 18, 5000);
	end
end

if (EVENT == 1603) then
	Check = isRoomForItem(UID, 508011441);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510910000, 1)
		GiveItem(UID, 508011441, 1, 1)
	end
end

if (EVENT == 1604) then
	Check = isRoomForItem(UID, 508011442);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510910000, 1)
		GiveItem(UID, 508011442, 1, 1)
	end
end

if (EVENT == 1605) then
	Check = isRoomForItem(UID, 508011443);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510910000, 1)
		GiveItem(UID, 508011443, 1, 1)
	end
end

if (EVENT == 1606) then
	Check = isRoomForItem(UID, 508011444);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510910000, 1)
		GiveItem(UID, 508011444, 1, 1)
	end
end

if (EVENT == 1607) then -- Exchange Valkyrie Helmet ( 1 Day )
	ITEMHELMET = HowmuchItem(UID, 510810000);
	if (ITEMHELMET > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1608, 4293, 1609, 4294, 1610, 4295, 1611);
	else
		SelectMsg(UID, 2, -1, 4911, NPC, 18, 5000);
	end
end

if (EVENT == 1608) then
	Check = isRoomForItem(UID, 508013318);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510810000, 1)
		GiveItem(UID, 508013318, 1, 1)
	end
end

if (EVENT == 1609) then
	Check = isRoomForItem(UID, 508013319);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510810000, 1)
		GiveItem(UID, 508013319, 1, 1)
	end
end

if (EVENT == 1610) then
	Check = isRoomForItem(UID, 508013320);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510810000, 1)
		GiveItem(UID, 508013320, 1, 1)
	end
end

if (EVENT == 1611) then
	Check = isRoomForItem(UID, 508013321);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510810000, 1)
		GiveItem(UID, 508013321, 1, 1)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1620) then -- Exchange Valkyrie Armor ( 3 Days )
	ITEMARMOR = HowmuchItem(UID, 510920000);
	if (ITEMARMOR > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1621, 4289, 1622, 4290, 1623, 4291, 1624);
	else
		SelectMsg(UID, 2, -1, 4921, NPC, 18, 5000);
	end
end

if (EVENT == 1621) then
	Check = isRoomForItem(UID, 508011441);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510920000, 1)
		GiveItem(UID, 508011441, 1, 3)
	end
end

if (EVENT == 1622) then
	Check = isRoomForItem(UID, 508011442);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510920000, 1)
		GiveItem(UID, 508011442, 1, 3)
	end
end

if (EVENT == 1623) then
	Check = isRoomForItem(UID, 508011443);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510920000, 1)
		GiveItem(UID, 508011443, 1, 3)
	end
end

if (EVENT == 1624) then
	Check = isRoomForItem(UID, 508011444);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510920000, 1)
		GiveItem(UID, 508011444, 1, 3)
	end
end

if (EVENT == 1630) then -- Exchange Valkyrie Helmet ( 3 Days )
	ITEMHELMET = HowmuchItem(UID, 510820000);
	if (ITEMHELMET > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1631, 4293, 1632, 4294, 1633, 4295, 1634);
	else
		SelectMsg(UID, 2, -1, 4911, NPC, 18, 5000);
	end
end

if (EVENT == 1631) then
	Check = isRoomForItem(UID, 508013318);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510820000, 1)
		GiveItem(UID, 508013318, 1, 3)
	end
end

if (EVENT == 1632) then
	Check = isRoomForItem(UID, 508013319);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510820000, 1)
		GiveItem(UID, 508013319, 1, 3)
	end
end

if (EVENT == 1633) then
	Check = isRoomForItem(UID, 508013320);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510820000, 1)
		GiveItem(UID, 508013320, 1, 3)
	end
end

if (EVENT == 1634) then
	Check = isRoomForItem(UID, 508013321);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510820000, 1)
		GiveItem(UID, 508013321, 1, 3)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1640) then -- Exchange Valkyrie Armor ( 7 Days )
	ITEMARMOR = HowmuchItem(UID, 510930000);
	if (ITEMARMOR > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1641, 4289, 1642, 4290, 1643, 4291, 1644);
	else
		SelectMsg(UID, 2, -1, 4921, NPC, 18, 5000);
	end
end

if (EVENT == 1641) then
	Check = isRoomForItem(UID, 508011441);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510930000, 1)
		GiveItem(UID, 508011441, 1, 7)
	end
end

if (EVENT == 1642) then
	Check = isRoomForItem(UID, 508011442);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510930000, 1)
		GiveItem(UID, 508011442, 1, 7)
	end
end

if (EVENT == 1643) then
	Check = isRoomForItem(UID, 508011443);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510930000, 1)
		GiveItem(UID, 508011443, 1, 7)
	end
end

if (EVENT == 1644) then
	Check = isRoomForItem(UID, 508011444);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510930000, 1)
		GiveItem(UID, 508011444, 1, 7)
	end
end

if (EVENT == 1650) then -- Exchange Valkyrie Helmet ( 7 Days )
	ITEMHELMET = HowmuchItem(UID, 510830000);
	if (ITEMHELMET > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1651, 4293, 1652, 4294, 1653, 4295, 1654);
	else
		SelectMsg(UID, 2, -1, 4911, NPC, 18, 5000);
	end
end

if (EVENT == 1651) then
	Check = isRoomForItem(UID, 508013318);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510830000, 1)
		GiveItem(UID, 508013318, 1, 7)
	end
end

if (EVENT == 1652) then
	Check = isRoomForItem(UID, 508013319);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510830000, 1)
		GiveItem(UID, 508013319, 1, 7)
	end
end

if (EVENT == 1653) then
	Check = isRoomForItem(UID, 508013320);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510830000, 1)
		GiveItem(UID, 508013320, 1, 7)
	end
end

if (EVENT == 1654) then
	Check = isRoomForItem(UID, 508013321);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510830000, 1)
		GiveItem(UID, 508013321, 1, 7)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1655) then -- Exchange [Daily Bahamut Armor & Helmet]
	SelectMsg(UID, 3, -1, 4901, NPC, 45512, 1660, 45513, 1670, 45514, 1680, 45515, 1690, 45516, 1700, 45517, 1710);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1660) then -- Exchange Bahamut Armor ( 1 Day )
	ITEMBHMTA = HowmuchItem(UID, 510940000);
	if (ITEMBHMTA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1661, 4289, 1662, 4290, 1663, 4291, 1664);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 1661) then
	Check = isRoomForItem(UID, 508051466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510940000, 1)
		GiveItem(UID, 508051466, 1, 1)
	end
end

if (EVENT == 1662) then
	Check = isRoomForItem(UID, 508051467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510940000, 1)
		GiveItem(UID, 508051467, 1, 1)
	end
end

if (EVENT == 1663) then
	Check = isRoomForItem(UID, 508051468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510940000, 1)
		GiveItem(UID, 508051468, 1, 1)
	end
end

if (EVENT == 1664) then
	Check = isRoomForItem(UID, 508051469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510940000, 1)
		GiveItem(UID, 508051469, 1, 1)
	end
end

if (EVENT == 1670) then -- Exchange Bahamut Helmet ( 1 Day )
	ITEMBHMTH = HowmuchItem(UID, 510840000);
	if (ITEMBHMTH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1671, 4293, 1672, 4294, 1673, 4295, 1674);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 1671) then
	Check = isRoomForItem(UID, 508053466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510840000, 1)
		GiveItem(UID, 508053466, 1, 1)
	end
end

if (EVENT == 1672) then
	Check = isRoomForItem(UID, 508053467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510840000, 1)
		GiveItem(UID, 508053467, 1, 1)
	end
end

if (EVENT == 1673) then
	Check = isRoomForItem(UID, 508053468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510840000, 1)
		GiveItem(UID, 508053468, 1, 1)
	end
end

if (EVENT == 1674) then
	Check = isRoomForItem(UID, 508053469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510840000, 1)
		GiveItem(UID, 508053469, 1, 1)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1680) then -- Exchange Bahamut Armor ( 3 Days )
	ITEMBHMTA = HowmuchItem(UID, 510950000);
	if (ITEMBHMTA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1681, 4289, 1682, 4290, 1683, 4291, 1684);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 1681) then
	Check = isRoomForItem(UID, 508051466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510950000, 1)
		GiveItem(UID, 508051466, 1, 3)
	end
end

if (EVENT == 1682) then
	Check = isRoomForItem(UID, 508051467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510950000, 1)
		GiveItem(UID, 508051467, 1, 3)
	end
end

if (EVENT == 1683) then
	Check = isRoomForItem(UID, 508051468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510950000, 1)
		GiveItem(UID, 508051468, 1, 3)
	end
end

if (EVENT == 1684) then
	Check = isRoomForItem(UID, 508051469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510950000, 1)
		GiveItem(UID, 508051469, 1, 3)
	end
end

if (EVENT == 1690) then -- Exchange Bahamut Helmet ( 3 Days )
	ITEMBHMTH = HowmuchItem(UID, 510850000);
	if (ITEMBHMTH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1691, 4293, 1692, 4294, 1693, 4295, 1694);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 1691) then
	Check = isRoomForItem(UID, 508053466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510850000, 1)
		GiveItem(UID, 508053466, 1, 3)
	end
end

if (EVENT == 1692) then
	Check = isRoomForItem(UID, 508053467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510850000, 1)
		GiveItem(UID, 508053467, 1, 3)
	end
end

if (EVENT == 1693) then
	Check = isRoomForItem(UID, 508053468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510850000, 1)
		GiveItem(UID, 508053468, 1, 3)
	end
end

if (EVENT == 1694) then
	Check = isRoomForItem(UID, 508053469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510850000, 1)
		GiveItem(UID, 508053469, 1, 3)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1700) then -- Exchange Bahamut Armor ( 7 Days )
	ITEMBHMTA = HowmuchItem(UID, 510960000);
	if (ITEMBHMTA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1701, 4289, 1702, 4290, 1703, 4291, 1704);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 1701) then
	Check = isRoomForItem(UID, 508051466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510960000, 1)
		GiveItem(UID, 508051466, 1, 7)
	end
end

if (EVENT == 1702) then
	Check = isRoomForItem(UID, 508051467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510960000, 1)
		GiveItem(UID, 508051467, 1, 7)
	end
end

if (EVENT == 1703) then
	Check = isRoomForItem(UID, 508051468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510960000, 1)
		GiveItem(UID, 508051468, 1, 7)
	end
end

if (EVENT == 1704) then
	Check = isRoomForItem(UID, 508051469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510960000, 1)
		GiveItem(UID, 508051469, 1, 7)
	end
end

if (EVENT == 1710) then -- Exchange Bahamut Helmet ( 7 Days )
	ITEMBHMTH = HowmuchItem(UID, 510860000);
	if (ITEMBHMTH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1711, 4293, 1712, 4294, 1713, 4295, 1714);
	else
		SelectMsg(UID, 2, -1, 1126, NPC, 18, 5000);
	end
end

if (EVENT == 1711) then
	Check = isRoomForItem(UID, 508053466);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510860000, 1)
		GiveItem(UID, 508053466, 1, 7)
	end
end

if (EVENT == 1712) then
	Check = isRoomForItem(UID, 508053467);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510860000, 1)
		GiveItem(UID, 508053467, 1, 7)
	end
end

if (EVENT == 1713) then
	Check = isRoomForItem(UID, 508053468);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510860000, 1)
		GiveItem(UID, 508053468, 1, 7)
	end
end

if (EVENT == 1714) then
	Check = isRoomForItem(UID, 508053469);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510860000, 1)
		GiveItem(UID, 508053469, 1, 7)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1720) then -- Exchange [Daily Gryphon's Armor & Helmet]
	SelectMsg(UID, 3, -1, 4901, NPC, 45518, 1730, 45519, 1740, 45520, 1750, 45521, 1760, 45522, 1770, 45523, 1780);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1730) then -- Exchange Gryphon's Armor ( 1 Day )
	ITEMGRYPA = HowmuchItem(UID, 510970000);
	if (ITEMGRYPA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1731, 4289, 1732, 4290, 1733, 4291, 1734);
	else
		SelectMsg(UID, 2, -1, 6488, NPC, 18, 5000);
	end
end

if (EVENT == 1731) then
	Check = isRoomForItem(UID, 508471453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510970000, 1)
		GiveItem(UID, 508471453, 1, 1)
	end
end

if (EVENT == 1732) then
	Check = isRoomForItem(UID, 508471454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510970000, 1)
		GiveItem(UID, 508471454, 1, 1)
	end
end

if (EVENT == 1733) then
	Check = isRoomForItem(UID, 508471455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510970000, 1)
		GiveItem(UID, 508471455, 1, 1)
	end
end

if (EVENT == 1734) then
	Check = isRoomForItem(UID, 508471456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510970000, 1)
		GiveItem(UID, 508471456, 1, 1)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1740) then -- Exchange Gryphon's Helmet ( 1 Day )
	ITEMGRYPH = HowmuchItem(UID, 510870000);
	if (ITEMGRYPH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1741, 4293, 1742, 4294, 1743, 4295, 1744);
	else
		SelectMsg(UID, 2, -1, 6497, NPC, 18, 5000);
	end
end

if (EVENT == 1741) then
	Check = isRoomForItem(UID, 508473453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510870000, 1)
		GiveItem(UID, 508473453, 1, 1)
	end
end

if (EVENT == 1742) then
	Check = isRoomForItem(UID, 508473454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510870000, 1)
		GiveItem(UID, 508473454, 1, 1)
	end
end

if (EVENT == 1743) then
	Check = isRoomForItem(UID, 508473455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510870000, 1)
		GiveItem(UID, 508473455, 1, 1)
	end
end

if (EVENT == 1744) then
	Check = isRoomForItem(UID, 508473456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510870000, 1)
		GiveItem(UID, 508473456, 1, 1)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1750) then -- Exchange Gryphon's Armor ( 3 Days )
	ITEMGRYPA = HowmuchItem(UID, 510980000);
	if (ITEMGRYPA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1751, 4289, 1752, 4290, 1753, 4291, 1754);
	else
		SelectMsg(UID, 2, -1, 6488, NPC, 18, 5000);
	end
end

if (EVENT == 1751) then
	Check = isRoomForItem(UID, 508471453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510980000, 1)
		GiveItem(UID, 508471453, 1, 3)
	end
end

if (EVENT == 1752) then
	Check = isRoomForItem(UID, 508471454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510980000, 1)
		GiveItem(UID, 508471454, 1, 3)
	end
end

if (EVENT == 1753) then
	Check = isRoomForItem(UID, 508471455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510980000, 1)
		GiveItem(UID, 508471455, 1, 3)
	end
end

if (EVENT == 1754) then
	Check = isRoomForItem(UID, 508471456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510980000, 1)
		GiveItem(UID, 508471456, 1, 3)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1760) then -- Exchange Gryphon's Helmet ( 3 Days )
	ITEMGRYPH = HowmuchItem(UID, 510880000);
	if (ITEMGRYPH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1761, 4289, 1762, 4290, 1763, 4291, 1764);
	else
		SelectMsg(UID, 2, -1, 6497, NPC, 18, 5000);
	end
end

if (EVENT == 1761) then
	Check = isRoomForItem(UID, 508473453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510880000, 1)
		GiveItem(UID, 508473453, 1, 3)
	end
end

if (EVENT == 1762) then
	Check = isRoomForItem(UID, 508473454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510880000, 1)
		GiveItem(UID, 508473454, 1, 3)
	end
end

if (EVENT == 1763) then
	Check = isRoomForItem(UID, 508473455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510880000, 1)
		GiveItem(UID, 508473455, 1, 3)
	end
end

if (EVENT == 1764) then
	Check = isRoomForItem(UID, 508473456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510880000, 1)
		GiveItem(UID, 508473456, 1, 3)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1770) then -- Exchange Gryphon's Armor ( 7 Days )
	ITEMGRYPA = HowmuchItem(UID, 510990000);
	if (ITEMGRYPA > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4288, 1771, 4289, 1772, 4290, 1773, 4291, 1774);
	else
		SelectMsg(UID, 2, -1, 6488, NPC, 18, 5000);
	end
end

if (EVENT == 1771) then
	Check = isRoomForItem(UID, 508471453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510990000, 1)
		GiveItem(UID, 508471453, 1, 7)
	end
end

if (EVENT == 1772) then
	Check = isRoomForItem(UID, 508471454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510990000, 1)
		GiveItem(UID, 508471454, 1, 7)
	end
end

if (EVENT == 1773) then
	Check = isRoomForItem(UID, 508471455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510990000, 1)
		GiveItem(UID, 508471455, 1, 7)
	end
end

if (EVENT == 1774) then
	Check = isRoomForItem(UID, 508471456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510990000, 1)
		GiveItem(UID, 508471456, 1, 7)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1780) then -- Exchange Gryphon's Helmet ( 7 Days )
	ITEMGRYPH = HowmuchItem(UID, 510890000);
	if (ITEMGRYPH > 0) then
		SelectMsg(UID, 3, -1, 4902, NPC, 4292, 1781, 4289, 1782, 4290, 1783, 4291, 1784);
	else
		SelectMsg(UID, 2, -1, 6497, NPC, 18, 5000);
	end
end

if (EVENT == 1781) then
	Check = isRoomForItem(UID, 508473453);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510890000, 1)
		GiveItem(UID, 508473453, 1, 7)
	end
end

if (EVENT == 1782) then
	Check = isRoomForItem(UID, 508473454);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510890000, 1)
		GiveItem(UID, 508473454, 1, 7)
	end
end

if (EVENT == 1783) then
	Check = isRoomForItem(UID, 508473455);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510890000, 1)
		GiveItem(UID, 508473455, 1, 7)
	end
end

if (EVENT == 1784) then
	Check = isRoomForItem(UID, 508473456);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 510890000, 1)
		GiveItem(UID, 508473456, 1, 7)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1800) then -- Exchange [Daily Pathos' Glove]
	SelectMsg(UID, 3, -1, 4901, NPC, 45525, 1810, 45526, 1830, 45527, 1850);
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1810) then -- [Exchange] Certificate of Pathos' Glove ( 1 Day )
	ITEMPTHS = HowmuchItem(UID, 521100000);
	if (ITEMPTHS > 0) then
		SelectMsg(UID, 3, -1, 748, NPC, 4509, 1811, 4510, 1816);
	else
		SelectMsg(UID, 2, -1, 749, NPC, 18, 5000);
	end
end

if (EVENT == 1811) then -- Attack Aurora
	SelectMsg(UID, 3, -1, 750, NPC, 4505, 1812, 4506, 1813, 4507, 1814, 4508, 1815);
end

if (EVENT == 1812) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID, 502573462, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1813) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID, 503573463, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1814) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID , 504573464, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1815) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID, 505573465, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1816) then -- Defense Aurora
	SelectMsg(UID, 3, -1, 751, NPC, 4514, 1817, 4515, 1818, 4516, 1819, 4517, 1820);
end

if (EVENT == 1817) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID, 511573471, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1818) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID, 512573472, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1819) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID , 513573473, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1820) then
	Check = isRoomForItem(UID, 521100000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521100000, 1)
		GiveItem(UID, 514573474, 1, 1)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1830) then -- [Exchange] Certificate of Pathos' Glove ( 3 Days )
	ITEMPTHS = HowmuchItem(UID, 521200000);
	if (ITEMPTHS > 0) then
		SelectMsg(UID, 3, -1, 748, NPC, 4509, 1831, 4510, 1836);
	else
		SelectMsg(UID, 2, -1, 749, NPC, 18, 5000);
	end
end

if (EVENT == 1831) then -- Attack Aurora
	SelectMsg(UID, 3, -1, 750, NPC, 4505, 1832, 4506, 1833, 4507, 1834, 4508, 1835);
end

if (EVENT == 1832) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID, 502573462, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1833) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID, 503573463, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1834) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID , 504573464, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1835) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID, 505573465, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1836) then -- Defense Aurora
	SelectMsg(UID, 3, -1, 751, NPC, 4514, 1837, 4515, 1838, 4516, 1839, 4517, 1840);
end

if (EVENT == 1837) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID, 511573471, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1838) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID, 512573472, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1839) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID , 513573473, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1840) then
	Check = isRoomForItem(UID, 521200000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521200000, 1)
		GiveItem(UID, 514573474, 1, 3)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1850) then -- [Exchange] Certificate of Pathos' Glove ( 7 Days )
	ITEMPTHS = HowmuchItem(UID, 521300000);
	if (ITEMPTHS > 0) then
		SelectMsg(UID, 3, -1, 748, NPC, 4509, 1851, 4510, 1856);
	else
		SelectMsg(UID, 2, -1, 749, NPC, 18, 5000);
	end
end

if (EVENT == 1851) then -- Attack Aurora
	SelectMsg(UID, 3, -1, 750, NPC, 4505, 1852, 4506, 1853, 4507, 1854, 4508, 1855);
end

if (EVENT == 1852) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID, 502573462, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1853) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID, 503573463, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1854) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID , 504573464, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1855) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID, 505573465, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1856) then -- Defense Aurora
	SelectMsg(UID, 3, -1, 751, NPC, 4514, 1857, 4515, 1858, 4516, 1859, 4517, 1860);
end

if (EVENT == 1857) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID, 511573471, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1858) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID, 512573472, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1859) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID , 513573473, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

if (EVENT == 1860) then
	Check = isRoomForItem(UID, 521300000);
	if (Check == -1) then
		SelectMsg(UID, 2, -1, 832, NPC, 27, 168);
	else
		RobItem(UID, 521300000, 1)
		GiveItem(UID, 514573474, 1, 7)
		SelectMsg(UID, 2, -1, 752, NPC, 27, 168);
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1900) then -- Exchange [HP & MP Maestro Voucher]
	SelectMsg(UID, 3, -1, 4901, NPC, 45529, 1901, 45530, 1903);
end

if (EVENT == 1901) then -- [Exchange] HP Maestro Voucher ( 30 Days )
	EXPPREM = HowmuchItem(UID, 810115000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45273, NPC, 18, 5000);
	else
    EVENT = 1902
	end
end

if (EVENT == 1902) then
	EXPPREM = HowmuchItem(UID, 810115000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45273, NPC, 18, 5000);
	else
	RobItem(UID, 810115000, 1);
	GiveItem(UID, 810117000, 1, 30)
	end
end

if (EVENT == 1903) then -- [Exchange] MP Maestro Voucher ( 30 Days )
	EXPPREM = HowmuchItem(UID, 810116000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45274, NPC, 18, 5000);
	else
    EVENT = 1904
	end
end

if (EVENT == 1904) then
	EXPPREM = HowmuchItem(UID, 810116000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45274, NPC, 18, 5000);
	else
	RobItem(UID, 810116000, 1);
	GiveItem(UID, 810118000, 1, 30)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1910) then -- Exchange [Soccer Uniform Certificate]
	SelectMsg(UID, 3, -1, 4901, NPC, 45532, 1911, 45533, 1913, 45534, 1915, 45535, 1917);
end

if (EVENT == 1911) then -- [Exchange] Soccer Uniform ( Besiktas )
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
    EVENT = 1912
	end
end

if (EVENT == 1912) then
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
	RobItem(UID, 800420000, 1);
	GiveItem(UID, 929001609, 1, 1)
	end
end

if (EVENT == 1913) then -- [Exchange] Soccer Uniform ( Galatasaray )
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
    EVENT = 1914
	end
end

if (EVENT == 1914) then
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
	RobItem(UID, 800420000, 1);
	GiveItem(UID, 927001607, 1, 1)
	end
end

if (EVENT == 1915) then -- [Exchange] Soccer Uniform ( Fenerbahce )
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
    EVENT = 1916
	end
end

if (EVENT == 1916) then
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
	RobItem(UID, 800420000, 1);
	GiveItem(UID, 933001634, 1, 1)
	end
end

if (EVENT == 1917) then -- [Exchange] Soccer Uniform ( Trabzonspor )
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
    EVENT = 1918
	end
end

if (EVENT == 1918) then
	EXPPREM = HowmuchItem(UID, 800420000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45275, NPC, 18, 5000);
	else
	RobItem(UID, 800420000, 1);
	GiveItem(UID, 928001608, 1, 1)
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1930) then -- [ 100 Million ] Exp Gift
    Level = CheckLevel(UID);
    expcount = GetExp(UID);
    SlotCheck = CheckGiveSlot(UID, 1);
    if (Level < 65) then
        SelectMsg(UID, 2, -1, 45280, NPC, 18, 5000);
    elseif (expcount < 100000000) then
        SelectMsg(UID, 2, -1, 45280, NPC, 18, 5000);
    elseif SlotCheck == false then
    else
        ExpChange(UID, -100000000);
        GiveItem(UID, 810366000, 1); 
    end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------

if (EVENT == 1935) then -- Exchange [ New EXP Jar ]
	EXPPREM = HowmuchItem(UID, 810366000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45281, NPC, 18, 5000);
	else
    EVENT = 1936
	end
end

if (EVENT == 1936) then
	EXPPREM = HowmuchItem(UID, 810366000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45281, NPC, 18, 5000);
	else
		SlotCheck = CheckGiveSlot(UID, 1)
     if SlotCheck == false then
	else
		Check = CheckExchange(UID, 12219)
			if  Check == true then   
				Roll = RollDice(UID, 19) 
				found = Roll + 12200
				RunRandomExchange(UID, found);
			end
		end
	end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 2700) then -- Exchange [Perks Point]
	SelectMsg(UID, 3, -1, 4131, NPC, 45543, 2710, 45544, 2715, 4296, 102);
end

if (EVENT == 2710) then -- [Exchange] EXP Jar + Silver Bar ( 1 Point )
	EXPPREM = HowmuchItem(UID, 810366000);
	EXPPREM1 = HowmuchItem(UID, 379067000);
	if (EXPPREM < 5 or EXPPREM == 0 or EXPPREM1 < 3 or EXPPREM1 == 0) then
		SelectMsg(UID, 2, -1, 45282, NPC, 18, 5000);
	else
    EVENT = 2711
	end
end

if (EVENT == 2711) then
	EXPPREM = HowmuchItem(UID, 810366000);
	EXPPREM1 = HowmuchItem(UID, 379067000);
	if (EXPPREM < 5 or EXPPREM == 0 or EXPPREM1 < 3 or EXPPREM1 == 0) then
		SelectMsg(UID, 2, -1, 45282, NPC, 18, 5000);
	else
	RobItem(UID, 810366000, 5);
	RobItem(UID, 379067000, 1);
	RobItem(UID, 379067000, 1);
	RobItem(UID, 379067000, 1);
	PerkUseItem(UID, 0, 0, 1)
	end
end

if (EVENT == 2715) then -- [Exchange] EXP Jar + Knight Cash ( 1 Point )
	EXPPREM = HowmuchItem(UID, 810366000);
	EXPPREM1 = HowmuchItem(UID, 700088000);
	if (EXPPREM < 5 or EXPPREM == 0 or EXPPREM1 < 1 or EXPPREM1 == 0) then
		SelectMsg(UID, 2, -1, 45283, NPC, 18, 5000);
	else
    EVENT = 2716
	end
end

if (EVENT == 2716) then
	EXPPREM = HowmuchItem(UID, 810366000);
	EXPPREM1 = HowmuchItem(UID, 700088000);
	if (EXPPREM < 5 or EXPPREM == 0 or EXPPREM1 < 1 or EXPPREM1 == 0) then
		SelectMsg(UID, 2, -1, 45283, NPC, 18, 5000);
	else
	RobItem(UID, 810366000, 5);
	RobItem(UID, 700088000, 1);
	PerkUseItem(UID, 0, 0, 1)
	end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 2799) then -- Exchange [Tattoo]
	SelectMsg(UID, 3, -1, 4901, NPC, 40587, 2800, 40588, 2802, 40589, 2804, 40590, 2806, 40809, 2808);
end

if (EVENT == 2800) then -- [Exchange] Solar Tattoo
	EXPPREM = HowmuchItem(UID, 810926000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45292, NPC, 18, 5000);
	else
    EVENT = 2801
	end
end

if (EVENT == 2801) then
	EXPPREM = HowmuchItem(UID, 810926000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45292, NPC, 18, 5000);
	else
	RobItem(UID, 810926000, 1);
	GiveItem(UID, 810431970, 1, 30)
	end
end

if (EVENT == 2802) then -- [Exchange] Lunar Tattoo
	EXPPREM = HowmuchItem(UID, 810927000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45293, NPC, 18, 5000);
	else
    EVENT = 2803
	end
end

if (EVENT == 2803) then
	EXPPREM = HowmuchItem(UID, 810927000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45293, NPC, 18, 5000);
	else
	RobItem(UID, 810927000, 1);
	GiveItem(UID, 810432971, 1, 30)
	end
end

if (EVENT == 2804) then -- [Exchange] Stella Tattoo
	EXPPREM = HowmuchItem(UID, 810928000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45294, NPC, 18, 5000);
	else
    EVENT = 2805
	end
end

if (EVENT == 2805) then
	EXPPREM = HowmuchItem(UID, 810928000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45294, NPC, 18, 5000);
	else
	RobItem(UID, 810928000, 1);
	GiveItem(UID, 810433972, 1, 30)
	end
end

if (EVENT == 2806) then -- [Exchange] Nimbus Tattoo
	EXPPREM = HowmuchItem(UID, 810929000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45295, NPC, 18, 5000);
	else
    EVENT = 2807
	end
end

if (EVENT == 2807) then
	EXPPREM = HowmuchItem(UID, 810929000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45295, NPC, 18, 5000);
	else
	RobItem(UID, 810929000, 1);
	GiveItem(UID, 810434973, 1, 30)
	end
end

if (EVENT == 2808) then -- [Exchange] Special Tattoo
	EXPPREM = HowmuchItem(UID, 814663000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45296, NPC, 18, 5000);
	else
    EVENT = 2809
	end
end

if (EVENT == 2809) then
	EXPPREM = HowmuchItem(UID, 814663000);
	if (EXPPREM < 1 or EXPPREM == 0) then
		SelectMsg(UID, 2, -1, 45296, NPC, 18, 5000);
	else
	RobItem(UID, 814663000, 1);
	GiveItem(UID, 814664796, 1, 7)
	end
end
----------------------------------------------------------------------------------------------------------------------------------------------------------
if (EVENT == 2810) then -- [ 100 Million ] Perk Point Gift
    expcount = GetExp(UID);
	loseexpcount = 100000000
	if (expcount >= loseexpcount) then
        ExpChange(UID, -loseexpcount);
        PerkUseItem(UID, 0, 0, 1);
    end
end