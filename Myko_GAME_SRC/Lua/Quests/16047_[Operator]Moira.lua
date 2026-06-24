local NPC = 16047;

if (EVENT == 240) then
	SelectMsg(UID, 2, -1, 4031, NPC, 46021, 500, 46022, 286, 46023, 285, 46024, 300, 46025, 910)
end

if EVENT == 280 then
	BLUEBOX  = HowmuchItem(UID, 379156000)
	GREENBOX = HowmuchItem(UID, 379155000)
	BLACKBOX = HowmuchItem(UID, 810636000)
	REDBOX   = HowmuchItem(UID, 379154000)
	if REDBOX > 0 and GREENBOX > 0 and BLUEBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 8942, 285, 4323, 286, 4324, 500)
	elseif BLUEBOX > 0 and REDBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 8942, 285, 4324, 500)
	elseif BLUEBOX > 0 and GREENBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 8942, 285, 4323, 286)
	elseif BLUEBOX > 0 and BLACKBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 8942, 285, 9002, 287)
	elseif GREENBOX > 0 and BLACKBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 4323, 286, 9002, 287)
	elseif GREENBOX > 0 and REDBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 4323, 286, 4324, 500)
	elseif REDBOX > 0 and BLACKBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 9002, 287, 4324, 500)
	elseif BLUEBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 8942, 285, 27, -1)
	elseif GREENBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 4323, 286, 27, -1)
	elseif BLACKBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 9002, 287, 27, -1)
	elseif REDBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 4324, 500, 27, -1)
	else
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	end
end

if EVENT == 285 then
	BLUEBOX = HowmuchItem(UID, 379156000)
	if BLUEBOX > 0 then
		SelectMsg(UID, 4, 11, 4034, NPC, 4006, 289, 27, -1)
	else
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	end
end

if EVENT == 289 then
	BLUEBOX = HowmuchItem(UID, 379156000)
	if BLUEBOX < 1 then
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 429)
			if Check == true then
				Roll = RollDice(UID, 20)
				found = Roll + 409
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 286 then
	GREENBOX = HowmuchItem(UID, 379155000)
	if GREENBOX > 0 then
		SelectMsg(UID, 4, 12, 4033, NPC, 4006, 290, 27, -1)
	else
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	end
end

if EVENT == 290 then
	GREENBOX = HowmuchItem(UID, 379155000)
	if GREENBOX < 1 then
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 450)
			if Check == true then
				Roll = RollDice(UID, 20)
				found = Roll + 430
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 287 then
	BLACKBOX = HowmuchItem(UID, 810636000)
	if BLACKBOX > 0 then
		SelectMsg(UID, 4, 5520, 4033, NPC, 4006, 291, 27, -1)
	else
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	end
end

if EVENT == 291 then
	BLACKBOX = HowmuchItem(UID, 810636000)
	if BLACKBOX < 1 then
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 5540)
			if Check == true then
				Roll = RollDice(UID, 20)
				found = Roll + 5520
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 500 then
	REDBOX = HowmuchItem(UID, 379154000)
	if REDBOX > 0 then
		SelectMsg(UID, 4, 15, 4920, NPC, 4006, 584, 27, -1)
	else
		SelectMsg(UID, 2, -1, 4912, NPC, 10, -1)
	end
end

if EVENT == 584 then
	REDBOX = HowmuchItem(UID, 379154000)
	if REDBOX < 1 then
		SelectMsg(UID, 2, -1, 4912, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 2219)
			if Check == true then
				Roll = RollDice(UID, 19)
				found = Roll + 2200
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 300 then
	ItemA = HowmuchItem(UID, 379106000)
	if ItemA < 1 then
		SelectMsg(UID, 2, 13, 4142, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 13, 4141, NPC, 4006, 301, 27, -1)
	end
end

if EVENT == 301 then
	ItemA = HowmuchItem(UID, 379106000)
	if ItemA < 1 then
		SelectMsg(UID, 2, 13, 4142, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 2060)
			if Check == true then
				Roll = RollDice(UID, 59)
				found = Roll + 2001
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 400 then
	LOTTO = HowmuchItem(UID, 379095000)
	if LOTTO < 1 then
		SelectMsg(UID, 2, 14, 4664, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 14, 4665, NPC, 4006, 401, 27, -1)
	end
end

if EVENT == 401 then
	LOTTO = HowmuchItem(UID, 379095000)
	if LOTTO < 1 then
		SelectMsg(UID, 2, 14, 4664, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 2089)
			if Check == true then
				Roll = RollDice(UID, 19)
				found = Roll + 2070
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 450 then
	PRELOTTO = HowmuchItem(UID, 379166000)
	if PRELOTTO < 1 then
		SelectMsg(UID, 2, 18, 4664, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 18, 4665, NPC, 4006, 451, 27, -1)
	end
end

if EVENT == 451 then
	PRELOTTO = HowmuchItem(UID, 379166000)
	if PRELOTTO < 1 then
		SelectMsg(UID, 2, 18, 4664, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 2119)
			if Check == true then
				Roll = RollDice(UID, 19)
				found = Roll + 2100
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 1000 then
	ShowMap(UID, 450)
end

if EVENT == 700 then
	ABOX   = HowmuchItem(UID, 508194000)
	FBOX   = HowmuchItem(UID, 900823000)
	FADBOX = HowmuchItem(UID, 900824000)
	if ABOX > 0 and FBOX > 0 and FADBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 8805, 701, 8806, 702, 8807, 703)
	elseif ABOX > 0 and FBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 8805, 701, 8806, 702)
	elseif ABOX > 0 and FADBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 8805, 701, 8807, 703)
	elseif FBOX > 0 and FADBOX > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 8806, 702, 8807, 703)
	elseif ABOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 8805, 701, 27, -1)
	elseif FBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 8806, 702, 27, -1)
	elseif FADBOX > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 8807, 703, 27, -1)
	else
		SelectMsg(UID, 2, -1, 9462, NPC, 10, -1)
	end
end

if EVENT == 701 then
	ABOX = HowmuchItem(UID, 508194000)
	if ABOX < 1 then
		SelectMsg(UID, 2, 17, 9462, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 17, 9463, NPC, 4006, 704, 27, -1)
	end
end

if EVENT == 702 then
	FBOX = HowmuchItem(UID, 900823000)
	if FBOX < 1 then
		SelectMsg(UID, 2, -1, 9462, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 6189, 9463, NPC, 4006, 705, 27, -1)
	end
end

if EVENT == 703 then
	FADBOX = HowmuchItem(UID, 900824000)
	if FADBOX < 1 then
		SelectMsg(UID, 2, -1, 9462, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 6213, 9463, NPC, 4006, 706, 27, -1)
	end
end

if EVENT == 704 then
	ABOX = HowmuchItem(UID, 508194000)
	if ABOX < 1 then
		SelectMsg(UID, 2, 17, 9462, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 1523)
			if Check == true then
				Roll = RollDice(UID, 23)
				found = Roll + 1500
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 705 then
	FBOX = HowmuchItem(UID, 900823000)
	if FBOX < 1 then
		SelectMsg(UID, 2, -1, 9462, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 6211)
			if Check == true then
				Roll = RollDice(UID, 22)
				found = Roll + 6189
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 706 then
	FADBOX = HowmuchItem(UID, 900824000)
	if FADBOX < 1 then
		SelectMsg(UID, 2, -1, 9462, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 6215)
			if Check == true then
				Roll = RollDice(UID, 2)
				found = Roll + 6213
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 600 then
	TROPHYMAVI = HowmuchItem(UID, 399113000)
	if TROPHYMAVI < 1 then
		SelectMsg(UID, 2, 16, 8917, NPC, 10, -1)
	else
		SelectMsg(UID, 4, 16, 8968, NPC, 4006, 601, 27, -1)
	end
end

if EVENT == 601 then
	TROPHYMAVI = HowmuchItem(UID, 399113000)
	if TROPHYMAVI < 1 then
		SelectMsg(UID, 2, 16, 8917, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 2397)
			if Check == true then
				Roll = RollDice(UID, 20)
				found = Roll + 2377
				RunRandomExchange(UID, found)
			end
		end
	end
end

if EVENT == 800 then
	SelectMsg(UID, 3, -1, 12267, NPC, 8799, 801, 8800, 802, 8801, 803)
end

if EVENT == 801 then
	BLUE = HowmuchItem(UID, 810411000)
	if BLUE < 1 then
		SelectMsg(UID, 2, -1, 12260, NPC, 10, -1)
	else
		SelectMsg(UID, 2, -1, 12260, NPC, 10, -1)
	end
end

if EVENT == 802 then
	GREEN = HowmuchItem(UID, 810412000)
	if GREEN < 1 then
		SelectMsg(UID, 2, -1, 12261, NPC, 10, -1)
	else
		SelectMsg(UID, 2, -1, 12261, NPC, 10, -1)
	end
end

if EVENT == 803 then
	RED = HowmuchItem(UID, 810413000)
	if RED < 1 then
		SelectMsg(UID, 2, -1, 12262, NPC, 10, -1)
	else
		SelectMsg(UID, 2, -1, 12262, NPC, 10, -1)
	end
end

if EVENT == 2000 then
	SelectMsg(UID, 2, -1, 12391, NPC, 8939, 2001, 8940, 2010)
end

if EVENT == 2010 then
	SelectMsg(UID, 2, -1, 12393, NPC, 8943, 2011, 8941, 2013)
end

if EVENT == 2011 then
	TEMPLELOWCOIN = HowmuchItem(UID, 810445000)
	if TEMPLELOWCOIN < 2 then
		SelectMsg(UID, 2, -1, 12396, NPC, 10, -1)
	else
		SelectMsg(UID, 4, -1, 12393, NPC, 4006, 2012, 27, -1)
	end
end

if EVENT == 2012 then
	TEMPLELOWCOIN = HowmuchItem(UID, 810445000)
	if TEMPLELOWCOIN < 2 then
		SelectMsg(UID, 2, -1, 12396, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			RobItem(UID, 810445000, 2)
			GiveItem(UID, 379154000, 1)
			SelectMsg(UID, 2, -1, 12390, NPC, 10, -1)
		end
	end
end

if EVENT == 2013 then
	TEMPLELOWCOIN = HowmuchItem(UID, 810445000)
	if TEMPLELOWCOIN < 3 then
		SelectMsg(UID, 2, -1, 12397, NPC, 10, -1)
	else
		SelectMsg(UID, 4, -1, 12393, NPC, 4006, 2014, 27, -1)
	end
end

if EVENT == 2014 then
	TEMPLELOWCOIN = HowmuchItem(UID, 810445000)
	if TEMPLELOWCOIN < 3 then
		SelectMsg(UID, 2, -1, 12397, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			RobItem(UID, 810445000, 3)
			GiveItem(UID, 379155000, 1)
			SelectMsg(UID, 2, -1, 12390, NPC, 10, -1)
		end
	end
end

if EVENT == 2001 then
	SelectMsg(UID, 2, -1, 12392, NPC, 8941, 2002, 8942, 2004)
end

if EVENT == 2002 then
	TEMPLECOIN = HowmuchItem(UID, 810446000)
	if TEMPLECOIN < 2 then
		SelectMsg(UID, 2, -1, 12394, NPC, 10, -1)
	else
		SelectMsg(UID, 4, -1, 12392, NPC, 4006, 2003, 27, -1)
	end
end

if EVENT == 2003 then
	TEMPLECOIN = HowmuchItem(UID, 810446000)
	if TEMPLECOIN < 2 then
		SelectMsg(UID, 2, -1, 12394, NPC, 10, -1)
	else
		RobItem(UID, 810446000, 2)
		GiveItem(UID, 379155000, 1)
		SelectMsg(UID, 2, -1, 12390, NPC, 10, -1)
	end
end

if EVENT == 2004 then
	TEMPLECOIN = HowmuchItem(UID, 810446000)
	if TEMPLECOIN < 3 then
		SelectMsg(UID, 2, -1, 12395, NPC, 10, -1)
	else
		SelectMsg(UID, 4, -1, 12392, NPC, 4006, 2005, 27, -1)
	end
end

if EVENT == 2005 then
	TEMPLECOIN = HowmuchItem(UID, 810446000)
	if TEMPLECOIN < 3 then
		SelectMsg(UID, 2, -1, 12395, NPC, 10, -1)
	else
		RobItem(UID, 810446000, 3)
		GiveItem(UID, 379156000, 1)
		SelectMsg(UID, 2, -1, 12390, NPC, 10, -1)
	end
end

if EVENT == 2500 then
	DRAKI    = HowmuchItem(UID, 810596000)
	OLDDRAKI = HowmuchItem(UID, 810678000)
	if DRAKI > 0 and OLDDRAKI > 0 then
		SelectMsg(UID, 3, -1, 4035, NPC, 40501, 2501, 40502, 2503)
	elseif DRAKI > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 40501, 2501, 27, -1)
	elseif OLDDRAKI > 0 then
		SelectMsg(UID, 2, -1, 4035, NPC, 40502, 2503, 27, -1)
	else
		SelectMsg(UID, 2, -1, 44410, NPC, 10, -1)
	end
end

if EVENT == 2501 then
	DRAKI = HowmuchItem(UID, 810596000)
	if DRAKI > 0 then
		SelectMsg(UID, 4, 1553, 44412, NPC, 4006, 2502, 27, -1)
	else
		SelectMsg(UID, 2, -1, 44410, NPC, 10, -1)
	end
end

if EVENT == 2502 then
	DRAKI = HowmuchItem(UID, 810596000)
	if DRAKI < 1 then
		SelectMsg(UID, 2, -1, 44412, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Roll = RollDice(UID, 25)
			found = Roll + 6501
			RunRandomExchange(UID, found)
		else
			SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
		end
	end
end

if EVENT == 2503 then
	OLDDRAKI = HowmuchItem(UID, 810678000)
	if OLDDRAKI > 0 then
		SelectMsg(UID, 4, 6531, 44412, NPC, 4006, 2504, 27, -1)
	else
		SelectMsg(UID, 2, -1, 44410, NPC, 10, -1)
	end
end

if EVENT == 2504 then
	OLDDRAKI = HowmuchItem(UID, 810678000)
	if OLDDRAKI < 1 then
		SelectMsg(UID, 2, -1, 44410, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Roll = RollDice(UID, 37)
			found = Roll + 6531
			RunRandomExchange(UID, found)
		else
			SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
		end
	end
end

-- Gem Exchange: 5 box DOGRUDAN ana menude (EVENT 240, submenu yok). Mevcut 4 kutu calisan
-- onay event'lerine gider (Red 500, Green 286, Blue 285, Abyss 300). Malaysiako (Accessory
-- Box 810347000) -> yeni 910/911, ITEM_EXCHANGE origin araligi 1705-1727.
if EVENT == 910 then
	MALAY = HowmuchItem(UID, 810347000)
	if MALAY < 1 then
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	else
		SelectMsg(UID, 4, -1, 4034, NPC, 4006, 911, 27, -1)
	end
end

if EVENT == 911 then
	MALAY = HowmuchItem(UID, 810347000)
	if MALAY < 1 then
		SelectMsg(UID, 2, -1, 4032, NPC, 10, -1)
	else
		SlotCheck = CheckGiveSlot(UID, 1)
		if SlotCheck == true then
			Check = CheckExchange(UID, 1727)
			if Check == true then
				Roll = RollDice(UID, 22)
				found = Roll + 1705
				RunRandomExchange(UID, found)
			end
		end
	end
end