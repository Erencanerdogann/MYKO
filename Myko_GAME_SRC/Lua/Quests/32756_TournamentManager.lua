-- ==================================================================
-- Bynoisee MalaysiaKO
-- Updated: 2026-05-28 (S115)
-- Tournament Manager NPC (sSid=32756, Moradon Zone 21 koord 905,870)
-- ==================================================================
-- DB INSERT (migration 113):
--   QUEST_TALK_US 50001-50010 (paragraf metinler)
--   QUEST_MENU_US 95001-95010 (buton metinleri)
--   QUEST_HELPER nIndex=30001 (NPC -> Lua bagi, nEventTriggerIndex=50000)
--
-- Lua C++ bindings (lua_bindings.cpp + User.h):
--   LuaTournamentRegister()        -> 1=basari, 2=zaten, 3=lider-degil, 4=klan-yok
--   LuaTournamentCancelReg()       -> 1=iptal, 0=aktif kayit yok
--   LuaBracketActiveCount()        -> aktif bracket sayisi (max 4)
--   LuaBracketRegisterByIndex(idx) -> idx=1-4, 1=basari/0=hata
--   LuaBracket8v8ListLatest()      -> en son aktif bracket icin chat liste
-- ==================================================================

local NPC = 32756;

-- === GIRIS — NPC tikla, EVENT=50000 (QUEST_HELPER.nEventTriggerIndex) ===
if (EVENT == 50000) then
	-- Klan kontrol — onceki dialog cizgisi: klansiz / uye / lider
	-- DIKKAT 1: isInClan/isClanLeader Lua tarafinda BOOL doner (lua_pushboolean),
	--          'bool == 0' yanlis — 'not X' kullanilmali.
	-- DIKKAT 2: SelectMsg'in 2. parametresi (bFlag) buton SAYISI degil, dialog STILI.
	--          Mevcut Lua'larda 2 yaygin pattern (Patric, Helena). 2 ile gidiyoruz.
	if (not isInClan(UID)) then
		-- Klansiz: bilgi mesaji + cikis butonu (47 = 'Exit' standart text)
		SelectMsg(UID, 2, -1, 50001, NPC, 47, -1);
	elseif (not isClanLeader(UID)) then
		-- Klan uyesi ama lider degil
		SelectMsg(UID, 2, -1, 50002, NPC, 47, -1);
	else
		-- Klan lideri — ana menu (4 secenek + cikis)
		-- Helena'da bFlag=3 ile 4 buton verilmis pattern var (4 ana + 47,-1)
		SelectMsg(UID, 3, -1, 50003, NPC,
			95001, 50001,   -- "Turnuvaya Kayit Ol" -> EVENT 50001
			95002, 50002,   -- "Turnuva Kaydi Iptal" -> EVENT 50002
			95003, 50003,   -- "Bracket Listesi" -> EVENT 50003
			95004, 50004,   -- "8v8 Uye Listem" -> EVENT 50004
			47, -1          -- Cikis (47 = Exit standart text ID)
		);
	end
end

-- === EVENT 50001: Turnuvaya Kayit Ol ===
if (EVENT == 50001) then
	Ret = LuaTournamentRegister(UID);
	if (Ret == 1) then
		NpcMsg(UID, 50007, NPC);  -- "Klanin turnuvaya basariyla kayit edildi!"
	else
		NpcMsg(UID, 50008, NPC);  -- "Islem basarisiz. Detay icin chat ekranina bak."
	end
end

-- === EVENT 50002: Turnuva Kaydi Iptal ===
if (EVENT == 50002) then
	Ret = LuaTournamentCancelReg(UID);
	if (Ret == 1) then
		NpcMsg(UID, 50009, NPC);  -- "Kaydin iptal edildi."
	else
		NpcMsg(UID, 50008, NPC);  -- "Aktif kaydin yok / hata"
	end
end

-- === EVENT 50003: Bracket Listesi ===
if (EVENT == 50003) then
	Cnt = LuaBracketActiveCount(UID);
	if (Cnt == 0) then
		NpcMsg(UID, 50005, NPC);  -- "Su an aktif bracket turnuvasi yok"
	elseif (Cnt == 1) then
		SelectMsg(UID, 2, -1, 50004, NPC,
			95005, 50011,   -- "Bracket #1 e Kayit" -> EVENT 50011
			95009, 50000    -- "Geri" -> ana menu
		);
	elseif (Cnt == 2) then
		SelectMsg(UID, 2, -1, 50004, NPC,
			95005, 50011,
			95006, 50012,
			95009, 50000
		);
	elseif (Cnt == 3) then
		SelectMsg(UID, 3, -1, 50004, NPC,
			95005, 50011,
			95006, 50012,
			95007, 50013,
			95009, 50000
		);
	else
		-- 4+ aktif bracket
		SelectMsg(UID, 3, -1, 50004, NPC,
			95005, 50011,
			95006, 50012,
			95007, 50013,
			95008, 50014,
			95009, 50000
		);
	end
end

-- === EVENT 50004: 8v8 Uye Listem ===
if (EVENT == 50004) then
	Ret = LuaBracket8v8ListLatest(UID);
	-- Chat'e liste yazildi (C++ tarafi), NPC mesaji da goster
	NpcMsg(UID, 50006, NPC);  -- "8v8 listen chat ekranina yazildi"
end

-- === EVENT 50011-50014: Bracket #1-4 kayit ===
if (EVENT == 50011) then
	Ret = LuaBracketRegisterByIndex(UID, 1);
	if (Ret == 1) then NpcMsg(UID, 50007, NPC); else NpcMsg(UID, 50008, NPC); end
end

if (EVENT == 50012) then
	Ret = LuaBracketRegisterByIndex(UID, 2);
	if (Ret == 1) then NpcMsg(UID, 50007, NPC); else NpcMsg(UID, 50008, NPC); end
end

if (EVENT == 50013) then
	Ret = LuaBracketRegisterByIndex(UID, 3);
	if (Ret == 1) then NpcMsg(UID, 50007, NPC); else NpcMsg(UID, 50008, NPC); end
end

if (EVENT == 50014) then
	Ret = LuaBracketRegisterByIndex(UID, 4);
	if (Ret == 1) then NpcMsg(UID, 50007, NPC); else NpcMsg(UID, 50008, NPC); end
end
