#include "stdafx.h"

#pragma region CUser::MoveProcess(Packet & pkt)
void CUser::MoveProcess(Packet & pkt)
{
	if(!isInGame() || GetMap() == nullptr)
		return;

	if (m_bWarp || isDead()) 
		return;

	uint16 will_x, will_z, will_y;
	int16 speed = 0;
	float real_x, real_z, real_y;
	uint8 echo;
	uint16 fWillX = m_oldwillx, fWillZ = m_oldwillz, fWillY = m_oldwilly;

	pkt >> will_x >> will_z >> will_y >> speed >> echo >> curX1 >> curZ1 >> curY1;
	if (will_y == 0xFFFF) will_y = curY1; // Genie: KO exe Y bilinmiyorsa 0xFFFF gönderir
	
	//printf("%d %d %d : %d %d %d - %d %d\n", will_x, will_z, will_x, curX1, curZ1, curY1, speed, echo);

	/*__Vector3 StartPoint, EndPoint, WayPoint;

	EndPoint.Set(will_x / 10.0f, will_y / 10.0f, will_z / 10.0f);
	StartPoint.Set(curX1 / 10.0f, curY1 / 10.0f, curZ1 / 10.0f);
	WayPoint = EndPoint - StartPoint;

	WayPoint *= speed / 10.0f;

	StartPoint += WayPoint;*/

	

	//Packet aresult(WIZ_SELECT_MSG);
	//aresult << uint16(0) << uint8(7) << uint64(0);
	//aresult << uint32(6) << uint8(11) << uint32(500);
	//
	//Packet bresult(WIZ_EVENT);//DRAW  Ak�n juraid paketi buras�
	//bresult << uint8(TEMPLE_EVENT_FINISH)
	//	<< uint8(2) << uint8(0)
	//	<< uint8(0) << uint8(20) << uint32(0);
	//Send(&aresult);
	//Send(&bresult);


	moveop e = (moveop)echo;
	if (e != moveop::start && e != moveop::move && e != moveop::finish)
		return goDisconnect("moveprocess diff echo", __FUNCTION__);

	/*if (speed == 0 && (e == moveop::start || e == moveop::move))
		return goDisconnect("velocity amount is zero but echo 1 and 3 are not.", __FUNCTION__);*/

	pMove.status = e;

	bool stable = will_x == curX1 && will_z == curZ1 && will_y == curY1;
	if (!isGM() && !stable && ((echo != 0 && speed == 0) || (pMove.oldecho == 0 && echo == 0))) {
		if (pMove.caughttime > UNIXTIME2) pMove.caughtcount++;
		else pMove.caughtcount = 1;

		if (pMove.caughtcount >= 3) {
			std::string dclog = string_format("echo is not 0 but speed is 0 struserid=%s\n", GetName().c_str());
			
			if (pMove.oldecho == 0 && echo == 0)
				dclog = string_format("successively echo 0 came\n", GetName().c_str());
			
			printf("%s\n", dclog.c_str());
			return Home();
		}
		pMove.caughttime = UNIXTIME2 + 1100;
	}
	pMove.oldecho = echo; pMove.oldspeed = speed;

	if (fWillX == 0)
		fWillX = will_x;
	if (fWillY == 0)
		fWillY = will_y;
	if (fWillZ == 0)
		fWillZ = will_z;

	if (m_sSpeed == 0 && echo == 1)
	{
		will_x = (will_x + curX1) / 2;
		will_y = (will_y + curY1) / 2;
		will_z = (will_z + curZ1) / 2;
	}
	else if (speed)
	{
		if (GetDistance(fWillX / 10.0f, fWillZ / 10.0f, will_x / 10.0f, will_z / 10.0f) / speed > 16.0f && GetDistance(fWillX / 10.0f, fWillZ / 10.0f, will_x / 10.0f, will_z / 10.0f) / speed < 36.0f)
		{
			will_x = (will_x + curX1) / 2;
			will_y = (will_y + curY1) / 2;
			will_z = (will_z + curZ1) / 2;
		}
		else if (GetDistance(fWillX / 10.0f, fWillZ / 10.0f, will_x / 10.0f, will_z / 10.0f) / speed >= 36.0f)
		{
			will_x = curX1;
			will_y = curY1;
			will_z = curZ1;
		}
	}

	real_x = will_x / 10.0f; real_z = will_z / 10.0f; real_y = will_y / 10.0f;
	
	if (isSellingMerchant() || isSellingMerchantingPreparing())
		MerchantClose();
	
	m_sSpeed = speed;
	SpeedHackUser();

	m_oldwillx = will_x; m_oldwillz = will_z; m_oldwilly = will_y;

	if (!GetMap()->IsValidPosition(real_x, real_z, real_y))
		return;

	// G22: Y ekseni fly hack tespiti — zone bazli max Y limiti
	if (!isGM() && real_y > 0.0f)
	{
		float maxY = 200.0f; // varsayilan max yukseklik (cogu zone)
		uint8 zoneID = GetZoneID();
		// Bazi zone'lar daha yuksek terrain'e sahip
		if (zoneID == ZONE_DELOS || zoneID == ZONE_RONARK_LAND || zoneID == ZONE_RONARK_LAND_BASE)
			maxY = 350.0f;
		else if (zoneID == ZONE_ARDREAM || zoneID == ZONE_BIFROST
			|| zoneID == ZONE_CLAN_WAR_ARDREAM || zoneID == ZONE_CLAN_WAR_RONARK
			|| zoneID == ZONE_JURAID_MOUNTAIN || zoneID == ZONE_DRAKI_TOWER)
			maxY = 300.0f;
		else if (zoneID == ZONE_KARUS || zoneID == ZONE_ELMORAD
			|| zoneID == ZONE_KARUS2 || zoneID == ZONE_KARUS3
			|| zoneID == ZONE_ELMORAD2 || zoneID == ZONE_ELMORAD3
			|| zoneID == ZONE_KARUS_ESLANT || zoneID == ZONE_ELMORAD_ESLANT
			|| zoneID == ZONE_KARUS_ESLANT2 || zoneID == ZONE_KARUS_ESLANT3
			|| zoneID == ZONE_ELMORAD_ESLANT2 || zoneID == ZONE_ELMORAD_ESLANT3)
			maxY = 250.0f;

		if (!m_bGenieStatus && real_y > maxY)
		{
			LOG_HACK("[FLY_HACK] User=%s Zone=%u Y=%.1f MaxY=%.1f IP=%s", GetName().c_str(), zoneID, real_y, maxY, GetRemoteIP().c_str());
			real_y = GetY(); // eski Y'ye geri dondur
		}
	}

	// Teleport hack detection: reject movement > 300 units from current position
	// m_bCheckWarpZoneChange: zone gecisi sonrasi ilk harekette kontrol atla
	// m_lastZoneChangeTime: zone gecisi sonrasi 5 saniye grace period
	if (!isGM() && !m_bCheckWarpZoneChange && GetX() != 0 && GetZ() != 0
		&& (UNIXTIME2 - m_lastZoneChangeTime) > 2000) {
		float dx = real_x - GetX(), dz = real_z - GetZ();
		float distSq = dx * dx + dz * dz;
		if (distSq > 300.0f * 300.0f) {
			LOG_HACK("Teleport: %s dist=%.0f from(%.0f,%.0f) to(%.0f,%.0f) IP=%s", GetName().c_str(), sqrtf(distSq), GetX(), GetZ(), real_x, real_z, GetRemoteIP().c_str());
			goDisconnect("Teleport hack detected", __FUNCTION__);
			return;
		}
	}

	if (m_oldx != GetX()
		|| m_oldy != GetY()
		|| m_oldz != GetZ())
	{
		m_oldx = GetX();
		m_oldy = GetY();
		m_oldz = GetZ();
	}

	// TODO: Ensure this is checked properly to prevent speedhacking
	SetPosition(real_x, real_y, real_z);
	 
	if (RegisterRegion())
	{
		g_pMain->RegionNpcInfoForMe(this);
		g_pMain->RegionUserInOutForMe(this);
	
	}

	if (m_PettingOn)
	{
		CNpc *pPet = g_pMain->GetPetPtr(GetSocketID(), GetZoneID());
		if (pPet != nullptr) 
		{
			if ((pPet->GetState() == (uint8)NpcState::NPC_STANDING
				|| pPet->GetState() == (uint8)NpcState::NPC_MOVING)
				&& (speed == 0
					|| GetDistanceSqrt(pPet) >= 10))
			{
				float warp_x = pPet->GetX() - GetX(), warp_z = pPet->GetZ() - GetZ();

				// Unable to work out orientation, so we'll just fail (won't be necessary with m_sDirection).
				float	distance = sqrtf(warp_x * warp_x + warp_z * warp_z);
				if (distance == 0.0f)
					return;

				warp_x /= distance; warp_z /= distance;
				warp_x *= 2; warp_z *= 2;
				warp_x += m_oldx; warp_z += m_oldz;

				pPet->SendMoveResult(warp_x, 0, warp_z, distance);
			}
		}
	}

	if (m_bInvisibilityType == (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE)
		CMagicProcess::RemoveStealth(this, InvisibilityType::INVIS_DISPEL_ON_MOVE);

	if (isMining())
		HandleMiningStop();

	if (isFishing())
		HandleFishingStop((Packet)(WIZ_MINING, FishingStop));

	Packet result;

	// Throttle movement broadcasts to max 1 per N ms per player
	// Yogun bolgeler (Moradon gibi) icin broadcast araligini artir
	ULONGLONG broadcastInterval = (ULONGLONG)g_ServerConfig.MoveBroadcastInterval();
	if (GetMap() != nullptr)
	{
		CRegion* pMyRegion = GetRegion();
		if (pMyRegion != nullptr)
		{
			// Bolgedeki kullanici sayisina gore throttle artir
			pMyRegion->m_lockUserArray.lock_shared();
			size_t regionPopulation = pMyRegion->m_RegionUserArray.size();
			pMyRegion->m_lockUserArray.unlock_shared();

			if (regionPopulation > 100)
				broadcastInterval = broadcastInterval * 3;      // 100+ kisi: 300ms
			else if (regionPopulation > 50)
				broadcastInterval = broadcastInterval * 2;      // 50-100 kisi: 200ms
		}
	}

	if (UNIXTIME2 - m_lastMoveBroadcast >= broadcastInterval || speed == 0)
	{
		m_lastMoveBroadcast = UNIXTIME2;

		if (isGM())
		{
			if (m_bAbnormalType != ABNORMAL_INVISIBLE)
			{
				Packet x(WIZ_MOVE);
				x << GetSocketID() << will_x << will_z << will_y << speed << echo;
				SendToRegion(&x, nullptr, GetEventRoom());
			}
		}
		else
		{
			Packet x(WIZ_MOVE);
			x << GetSocketID() << will_x << will_z << will_y << speed << echo;
			SendToRegion(&x, nullptr, GetEventRoom());
		}
	}

	if (m_bCheckWarpZoneChange && speed)
		m_bCheckWarpZoneChange = false;

	GetMap()->CheckEvent(real_x, real_z, this);

	if(isEventUser() && isInTempleEventZone())
		m_event_afkcheck = UNIXTIME;

	EventTrapProcess(real_x, real_z, this);
	OreadsZoneTerrainEvent();
	//UserWallCheatCheckRegion();
	BDWMonumentPointProcess();
}
#pragma endregion 

