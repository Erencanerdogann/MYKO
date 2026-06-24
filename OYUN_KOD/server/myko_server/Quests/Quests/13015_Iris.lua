-- ==================================================================
-- Author:		TheThyke
-- Create date: 04.12.2017
-- Update date: 06.06.2020
-- İsmimi Belgelerden Silseniz Bile Beyninizde Her Daim Olacağım.
-- ==================================================================
local NPC = 13015;

if (EVENT == 165) then
	SelectMsg(UID, 2, -1, 4133, NPC, 4075, 190, 4076, -1); 
end

if (EVENT == 190) then
STATUS = CheckMonsterChallengeTime(UID);
	if (STATUS == 1) then
		EVENT = 191
	else
		SelectMsg(UID, 2, -1, 4138, NPC, 10, -1);
	end
end

if EVENT == 191 then 
Count = CheckMonsterChallengeUserCount(UID);
	if (Count < 80) then
	ItemA = HowmuchItem(UID, 900000000); 
	if (ItemA > 100000) then
		GoldLose(UID, 100000);
		ZoneChange(UID, 55, 150, 150);
	else
		SelectMsg(UID, 2, -1, 4136, NPC, 10, -1);
	end
	else
		SelectMsg(UID, 2, -1, 4137, NPC, 10, -1);
	end
end