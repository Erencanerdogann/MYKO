-- ==================================================================
-- Bynoisee 
-- Updated: 2026-04-19
-- Knight Online Pvp 1098 & 1534 & v2 Server Files & AntiCheat System
-- ==================================================================
-- Kontrol Edilecek.
-- ==================================================================
local NPC = 29095;

if (EVENT == 100) then
	QuestNum = SearchQuest(UID, NPC);
	if (QuestNum == 0) then
		SelectMsg(UID, 2, -1, 9745, NPC, 10, 0);
	elseif (QuestNum > 1 and  QuestNum < 100) then
		NpcMsg(UID, 9745, NPC)
	else
		EVENT = QuestNum
	end
end


if (EVENT == 1801) then
	SelectMsg(UID, 4, 838, 9745, NPC, 22, 1802, 23, -1);
end


if (EVENT == 1802) then
	SelectMsg(UID, 2, -1, 9745, NPC, 10, -1);
end