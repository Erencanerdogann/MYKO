#include "stdafx.h"
#include "DBAgent.h"

#pragma region GenderChangeV2(Packet & pkt)
void CUser::GenderChangeV2(Packet & pkt)
{
	Packet result(WIZ_GENDER_CHANGE);

	if (isDead()
		|| isTrading()
		|| isMerchanting()
		|| isStoreOpen()
		|| isSellingMerchant()
		|| isBuyingMerchant()
		|| isMining()
		|| isFishing())
		return;

	int8 opcode, gRace, gFace, Results = 0;
	uint32 bHair;

	pkt >> opcode >> gRace >> gFace >> bHair;

	if (!CheckExistItem(ITEM_GENDER_CHANGE))
		goto fail_return;

	if (gRace == 0 || gFace == 0 || bHair == 0)
		goto fail_return;

	// Validate race matches nation (explicit whitelist)
	{
		bool validRace = false;
		if (GetNation() == 1) // Karus
			validRace = (gRace >= 1 && gRace <= 4) || gRace == 6; // KARUS_BIG..KARUS_WOMAN, KURIAN
		else if (GetNation() == 2) // Elmorad
			validRace = (gRace >= 11 && gRace <= 13); // BABARIAN, ELMORAD_MAN, ELMORAD_WOMAN

		if (!validRace)
			goto fail_return;
	}



	m_bRace = gRace;
	m_bFace = gFace;
	m_nHair = bHair;
	if (GetHealth() < (GetMaxHealth() / 2))
		HpChange(GetMaxHealth());

	SendMyInfo();

	UserInOut(INOUT_OUT);
	SetRegion(GetNewRegionX(), GetNewRegionZ());
	UserInOut(INOUT_WARP);

	g_pMain->UserInOutForMe(this);
	NpcInOutForMe();
	g_pMain->MerchantUserInOutForMe(this);


	ResetWindows();

	InitType4();
	RecastSavedMagic();

	RobItem(ITEM_GENDER_CHANGE);

	result << uint8(1) << m_bRace
		<< m_bFace
		<< m_nHair
		<< GetClass();
	Send(&result);


	return;
fail_return:
	result << Results;
	Send(&result);
}
#pragma endregion


#pragma region Gender Change
bool CUser::GenderChange(uint8 Race /*= 0*/)
{
	if (Race == 0 || Race > 13)
		return false;

	if (!CheckExistItem(ITEM_GENDER_CHANGE))
		return false;

	RobItem(ITEM_GENDER_CHANGE);

	m_bRace = Race;

	if (GetHealth() < (GetMaxHealth() / 2))
		HpChange(GetMaxHealth());

	SendMyInfo();

	UserInOut(INOUT_OUT);
	SetRegion(GetNewRegionX(), GetNewRegionZ());
	UserInOut(INOUT_WARP);

	g_pMain->UserInOutForMe(this);
	NpcInOutForMe();
	g_pMain->MerchantUserInOutForMe(this);

	ResetWindows();

	InitType4();
	RecastSavedMagic();

	return true;
}
#pragma endregion

#pragma region Gender Change Game Master
bool CUser::GenderChangeGM(uint8 Race /*= 0*/)
{
	if (Race == 0 || Race > 13)
		return false;

	m_bRace = Race;

	if (GetHealth() < (GetMaxHealth() / 2))
		HpChange(GetMaxHealth());

	SendMyInfo();

	UserInOut(INOUT_OUT);
	SetRegion(GetNewRegionX(), GetNewRegionZ());
	UserInOut(INOUT_WARP);

	g_pMain->UserInOutForMe(this);
	NpcInOutForMe();
	g_pMain->MerchantUserInOutForMe(this);

	ResetWindows();

	InitType4();
	RecastSavedMagic();

	return true;
}
#pragma endregion

uint8 CUser::JobChange(uint8 type, uint8 NewJob)
{
	/*
	1 success
	2 invalid job
	3 not exists item
	4 take item
	5 error newjob or class
	6 already class
	7 failed
	*/

	/*
	type 1.d�z
	type 2.master a��k ise kapat�r.
	*/

	uint8 bNewClass = 0, bNewRace = 0;

	if (NewJob < 1 || NewJob > 5 || (type != 0 && type != 1))
		return 5; 
	
	if (type == 0 && !CheckExistItem(ITEM_JOB_CHANGE)) 
		return 6;
	
	if (type == 1 && !CheckExistItem(ITEM_JOB_CHANGE2))
		return 6;

	if (isWarrior() && NewJob == 1) return 6;
	if (isRogue() && NewJob == 2) return 6;
	if (isMage() && NewJob == 3) return 6;
	if (isPriest() && NewJob == 4) return 6;
	if (isPortuKurian() && NewJob == 5) return 6;

	for (int i = 0; i < SLOT_MAX; i++) {
		if (m_sItemArray[i].nNum) {
			Packet result(WIZ_CLASS_CHANGE, uint8(ALL_POINT_CHANGE));
			result << uint8(4) << int(0);
			Send(&result);
			return 4;
		}
	}

	if (NewJob == 1) {
		if (isBeginnerRogue() || isBeginnerMage() || isBeginnerPriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = KARUWARRIOR;
				bNewRace = KARUS_BIG;
			}
			else {
				bNewClass = ELMORWARRRIOR;
				bNewRace = GetRace();
			}
		}
		else if (isNoviceRogue() || isNoviceMage() || isNovicePriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = BERSERKER;
				bNewRace = KARUS_BIG;
			}
			else {
				bNewClass = BLADE;
				bNewRace = GetRace();
			}
		}
		else if (isMasteredRogue() || isMasteredMage() || isMasteredPriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				if (type == 1) bNewClass = BERSERKER;
				else bNewClass = GUARDIAN;
				bNewRace = KARUS_BIG;
			}
			else {
				if (type == 1) bNewClass = BLADE;
				else bNewClass = PROTECTOR;
				bNewRace = GetRace();
			}
		}
	}
	else if (NewJob == 2) {
		if (isBeginnerWarrior() || isBeginnerMage() || isBeginnerPriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = KARUROGUE;
				bNewRace = KARUS_MIDDLE;
			}
			else {
				bNewClass = ELMOROGUE;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
		else if (isNoviceWarrior() || isNoviceMage() || isNovicePriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = HUNTER;
				bNewRace = KARUS_MIDDLE;
			}
			else {
				bNewClass = RANGER;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
		else if (isMasteredWarrior() || isMasteredMage() || isMasteredPriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				if (type == 1) bNewClass = HUNTER;
				else bNewClass = PENETRATOR;
				bNewRace = KARUS_MIDDLE;
			}
			else {
				if (type == 1) bNewClass = RANGER;
				else bNewClass = ASSASSIN;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
	}
	else if (NewJob == 3) {
		if (isBeginnerWarrior() || isBeginnerRogue() || isBeginnerPriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = KARUWIZARD;
				if (GetRace() == KARUS_BIG || GetRace() == KARUS_MIDDLE) bNewRace = KARUS_SMALL;
				else bNewRace = GetRace();
			}
			else {
				bNewClass = ELMOWIZARD;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
		else if (isNoviceWarrior() || isNoviceRogue() || isNovicePriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = SORSERER;
				if (GetRace() == KARUS_BIG || GetRace() == KARUS_MIDDLE) bNewRace = KARUS_SMALL;
				else bNewRace = GetRace();
			}
			else {
				bNewClass = MAGE;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
		else if (isMasteredWarrior() || isMasteredRogue() || isMasteredPriest()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				if (type == 1) bNewClass = SORSERER;
				else bNewClass = NECROMANCER;
				if (GetRace() == KARUS_BIG || GetRace() == KARUS_MIDDLE) bNewRace = KARUS_SMALL;
				else bNewRace = GetRace();
			}
			else {
				if (type == 1) bNewClass = MAGE;
				else bNewClass = ENCHANTER;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
	}
	else if (NewJob == 4) {
		if (isBeginnerWarrior() || isBeginnerRogue() || isBeginnerMage()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = KARUPRIEST;
				if (GetRace() == KARUS_BIG || GetRace() == KARUS_SMALL) bNewRace = KARUS_MIDDLE;
				else bNewRace = GetRace();
			}
			else {
				bNewClass = ELMOPRIEST;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
		else if (isNoviceWarrior() || isNoviceRogue() || isNoviceMage()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				bNewClass = SHAMAN;
				if (GetRace() == KARUS_BIG || GetRace() == KARUS_SMALL) bNewRace = KARUS_MIDDLE;
				else bNewRace = GetRace();
			}
			else {
				bNewClass = CLERIC;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
		else if (isMasteredWarrior() || isMasteredRogue() || isMasteredMage()) {
			if (GetNation() == (uint8)Nation::KARUS) {
				if (type == 1) bNewClass = SHAMAN;
				else bNewClass = DARKPRIEST;
				if (GetRace() == KARUS_BIG || GetRace() == KARUS_SMALL) bNewRace = KARUS_MIDDLE;
				else bNewRace = GetRace();
			}
			else {
				if (type == 1) bNewClass = CLERIC;
				else bNewClass = DRUID;
				if (GetRace() == BABARIAN) bNewRace = ELMORAD_MAN;
				else bNewRace = GetRace();
			}
		}
	}
	else return 7;

	if (!bNewClass || !bNewRace) 
		return 5;

	if (type == 0 && !RobItem(ITEM_JOB_CHANGE)) 
		return 6;
	else if (type == 1 && !RobItem(ITEM_JOB_CHANGE2)) 
		return 6;
	/*else return 4;*/

	JobChangeInsertLog(m_sClass, bNewClass, m_bRace, bNewRace);

	m_sClass = bNewClass; m_bRace = bNewRace;

	std::vector<uint16> metclist;
	_getEtcList(metclist, true);
	
	if (type == 0) {
		foreach(itr, metclist)
			SaveEvent(*itr, 4);
	}

	AllPointChange(true);
	AllSkillPointChange(true);

	/*if (GetHealth() < (GetMaxHealth() / 2))
		HpChange(GetMaxHealth());*/

	SendMyInfo();

	UserInOut(INOUT_OUT);
	SetRegion(GetNewRegionX(), GetNewRegionZ());
	UserInOut(INOUT_WARP);

	g_pMain->UserInOutForMe(this);
	NpcInOutForMe();
	g_pMain->MerchantUserInOutForMe(this);

	ResetWindows();

	InitType4();
	RecastSavedMagic();
	return 1;
}

//21.07.2020 JobChangeGM start
uint8 CUser::JobChangeGM(uint8 NewJob /*= 0*/)
{
	uint8 bNewClass = 0, bNewRace = 0;
	uint8 bResult = 0;

	if (NewJob < 1 || NewJob > 4)
		return 5; // Unknown job is selected...

	for (int i = 0; i < SLOT_MAX; i++)
	{
		if (m_sItemArray[i].nNum)
		{
			bResult = 7;
			break;
		}
	}

	if (bResult == 7)
	{
		Packet result(WIZ_CLASS_CHANGE, uint8(ALL_POINT_CHANGE));
		result << uint8(4) << int(0);
		Send(&result);
		return bResult; // While there are items equipped on you.
	}

	// If bResult between 1 and 4 character already selected job...

	// If selected a new job Warrior
	if (NewJob == 1)
	{
		if (GetNation() == (uint8)Nation::KARUS)
		{
			// Beginner Karus Rogue, Magician, Priest
			if (isBeginnerRogue() || isBeginnerMage() || isBeginnerPriest())
			{
				bNewClass = KARUWARRIOR;
				bNewRace = KARUS_BIG;
			}
			// Skilled Karus Rogue, Magician, Priest
			else if (isNoviceRogue() || isNoviceMage() || isNovicePriest())
			{
				bNewClass = BERSERKER;
				bNewRace = KARUS_BIG;
			}
			// Mastered Karus Rogue, Magician, Priest
			else if (isMasteredRogue() || isMasteredMage() || isMasteredPriest())
			{
				bNewClass = GUARDIAN;
				bNewRace = KARUS_BIG;
			}
		}
		else
		{
			// Beginner El Morad Rogue, Magician, Priest
			if (isBeginnerRogue() || isBeginnerMage() || isBeginnerPriest())
			{
				bNewClass = ELMORWARRRIOR;
				bNewRace = GetRace();
			}
			// Skilled El Morad Rogue, Magician, Priest
			else if (isNoviceRogue() || isNoviceMage() || isNovicePriest())
			{
				bNewClass = BLADE;
				bNewRace = GetRace();
			}
			// Mastered El Morad Rogue, Magician, Priest
			else if (isMasteredRogue() || isMasteredMage() || isMasteredPriest())
			{
				bNewClass = PROTECTOR;
				bNewRace = GetRace();
			}
		}

		// Character already Warrior.
		if (bNewClass == 0 || bNewRace == 0)
			bResult = NewJob;
	}

	// If selected a new job Rogue
	if (NewJob == 2)
	{
		if (GetNation() == (uint8)Nation::KARUS)
		{
			// Beginner Karus Warrior, Magician, Priest
			if (isBeginnerWarrior() || isBeginnerMage() || isBeginnerPriest())
			{
				bNewClass = KARUROGUE;
				bNewRace = KARUS_MIDDLE;
			}
			// Skilled Karus Warrior, Magician, Priest
			else if (isNoviceWarrior() || isNoviceMage() || isNovicePriest())
			{
				bNewClass = HUNTER;
				bNewRace = KARUS_MIDDLE;
			}
			// Mastered Karus Warrior, Magician, Priest
			else if (isMasteredWarrior() || isMasteredMage() || isMasteredPriest())
			{
				bNewClass = PENETRATOR;
				bNewRace = KARUS_MIDDLE;
			}
		}
		else
		{
			// Beginner El Morad Warrior, Magician, Priest
			if (isBeginnerWarrior() || isBeginnerMage() || isBeginnerPriest())
			{
				bNewClass = ELMOROGUE;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
			// Skilled El Morad Warrior, Magician, Priest
			else if (isNoviceWarrior() || isNoviceMage() || isNovicePriest())
			{
				bNewClass = RANGER;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
			// Mastered El Morad Warrior, Magician, Priest
			else if (isMasteredWarrior() || isMasteredMage() || isMasteredPriest())
			{
				bNewClass = ASSASSIN;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
		}

		// Character already Rogue.
		if (bNewClass == 0 || bNewRace == 0)
			bResult = NewJob;
	}

	// If selected a new job Magician
	if (NewJob == 3)
	{
		if (GetNation() == (uint8)Nation::KARUS)
		{
			// Beginner Karus Warrior, Rogue, Priest
			if (isBeginnerWarrior() || isBeginnerRogue() || isBeginnerPriest())
			{
				bNewClass = KARUWIZARD;

				if (GetRace() == KARUS_BIG || GetRace() == KARUS_MIDDLE)
					bNewRace = KARUS_SMALL;
				else
					bNewRace = GetRace();
			}
			// Skilled Karus Warrior, Rogue, Priest
			else if (isNoviceWarrior() || isNoviceRogue() || isNovicePriest())
			{
				bNewClass = SORSERER;

				if (GetRace() == KARUS_BIG || GetRace() == KARUS_MIDDLE)
					bNewRace = KARUS_SMALL;
				else
					bNewRace = GetRace();
			}
			// Mastered Karus Warrior, Rogue, Priest
			else if (isMasteredWarrior() || isMasteredRogue() || isMasteredPriest())
			{
				bNewClass = NECROMANCER;

				if (GetRace() == KARUS_BIG || GetRace() == KARUS_MIDDLE)
					bNewRace = KARUS_SMALL;
				else
					bNewRace = GetRace();
			}
		}
		else
		{
			// Beginner El Morad Warrior, Rogue, Priest
			if (isBeginnerWarrior() || isBeginnerRogue() || isBeginnerPriest()) //21.07.2020 2 kez isBeginnerRogue Eklenmi�ti d�zeltildi biri isBeginnerWarrior olarak
			{
				bNewClass = ELMOWIZARD;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
			// Skilled El Morad Warrior, Rogue, Priest
			else if (isNoviceWarrior() || isNoviceRogue() || isNovicePriest())
			{
				bNewClass = MAGE;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
			// Mastered El Morad Warrior, Rogue, Priest
			else if (isMasteredWarrior() || isMasteredRogue() || isMasteredPriest())
			{
				bNewClass = ENCHANTER;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
		}

		// Character already Magician.
		if (bNewClass == 0 || bNewRace == 0)
			bResult = NewJob;
	}

	// If selected a new job Priest
	if (NewJob == 4)
	{
		if (GetNation() == (uint8)Nation::KARUS)
		{
			// Beginner Karus Warrior, Rogue, Magician
			if (isBeginnerWarrior() || isBeginnerRogue() || isBeginnerMage())
			{
				bNewClass = KARUPRIEST;

				if (GetRace() == KARUS_BIG || GetRace() == KARUS_SMALL)
					bNewRace = KARUS_MIDDLE;
				else
					bNewRace = GetRace();
			}
			// Skilled Karus Warrior, Rogue, Magician
			else if (isNoviceWarrior() || isNoviceRogue() || isNoviceMage())
			{
				bNewClass = SHAMAN;

				if (GetRace() == KARUS_BIG || GetRace() == KARUS_SMALL)
					bNewRace = KARUS_MIDDLE;
				else
					bNewRace = GetRace();
			}
			// Mastered Karus Warrior, Rogue, Magician
			else if (isMasteredWarrior() || isMasteredRogue() || isMasteredMage()) //21.07.2020 2 kez isMasteredRogue Eklenmi�ti d�zeltildi biri isMasteredWarrior olarak
			{
				bNewClass = DARKPRIEST;

				if (GetRace() == KARUS_BIG || GetRace() == KARUS_SMALL)
					bNewRace = KARUS_MIDDLE;
				else
					bNewRace = GetRace();
			}
		}
		else
		{
			// Beginner El Morad Warrior, Rogue, Magician
			if (isBeginnerRogue() || isBeginnerWarrior() || isBeginnerMage())
			{
				bNewClass = ELMOPRIEST;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
			// Skilled El Morad Warrior, Rogue, Magician
			else if (isNoviceRogue() || isNoviceWarrior() || isNoviceMage())
			{
				bNewClass = CLERIC;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
			// Mastered El Morad Warrior, Rogue, Magician
			else if (isMasteredRogue() || isMasteredWarrior() || isMasteredMage())
			{
				bNewClass = DRUID;

				if (GetRace() == BABARIAN)
					bNewRace = ELMORAD_MAN;
				else
					bNewRace = GetRace();
			}
		}

		// Character already Priest.
		if (bNewClass == 0 || bNewRace == 0)
			bResult = NewJob;
	}

	if (bResult == 0)
	{
		m_sClass = bNewClass;
		m_bRace = bNewRace;

		AllPointChange(true);
		AllSkillPointChange(true);

		if (GetHealth() < (GetMaxHealth() / 2))
			HpChange(GetMaxHealth());

		SendMyInfo();

		UserInOut(INOUT_OUT);
		SetRegion(GetNewRegionX(), GetNewRegionZ());
		UserInOut(INOUT_WARP);

		g_pMain->UserInOutForMe(this);
		NpcInOutForMe();
		g_pMain->MerchantUserInOutForMe(this);

		ResetWindows();

		InitType4();
		RecastSavedMagic();
	}

	return bResult;
}
//21.07.2020 JobChangeGM end