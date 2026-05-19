#include "stdafx.h"
#include "KnightsManager.h"
#include "KingSystem.h"
#include "DBAgent.h"
#include "FerihaQueque.h"

#pragma region CUser::Disconnectprintfstyle()
std::string CUser::Disconnectprintfstyle() {
	DateTime time; return GetAccountName().empty() ? "" : string_format("%d:%d:%d :: Accountid=%s <> Sockid=%d ::",
		time.GetHour(), time.GetMinute(), time.GetSecond(), GetAccountName().c_str(), GetSocketID());
}
#pragma endregion

#pragma region AdiniFerihaKoydum::BeginSynchronized(int type)
void AdiniFerihaKoydum::BeginSynchronized(int type)
{
	m_conditionlock[type].lock();
	++m_nLockCount[type];
}
#pragma endregion

#pragma region AdiniFerihaKoydum::EndSynchronized(int type)
void AdiniFerihaKoydum::EndSynchronized(int type)
{
	--m_nLockCount[type];
	m_conditionlock[type].unlock();
}
#pragma endregion

#pragma region AdiniFerihaKoydum::Wait(int type,time_t timeout)
uint32 AdiniFerihaKoydum::Wait(int type,time_t timeout)
{
	std::unique_lock<std::mutex> lock(m_conditionlock[type]);
	m_condition[type].wait_for(lock, std::chrono::milliseconds(timeout));
	return 0;
}
#pragma endregion

#pragma region AdiniFerihaKoydum::Wait(int type)
uint32 AdiniFerihaKoydum::Wait(int type)
{
	std::unique_lock<std::mutex> lock(m_conditionlock[type]);
	m_condition[type].wait(lock);
	return 0;
}
#pragma endregion

#pragma region AdiniFerihaKoydum::Signal(int type)
void AdiniFerihaKoydum::Signal(int type) { m_condition[type].notify_one(); }
#pragma endregion

#pragma region AdiniFerihaKoydum::Broadcast(int type)
void AdiniFerihaKoydum::Broadcast(int type) { m_condition[type].notify_all(); }
#pragma endregion

#pragma region AdiniFerihaKoydum::AdiniFerihaKoydum()
AdiniFerihaKoydum::AdiniFerihaKoydum() { Initialize(); }
#pragma endregion

#pragma region AdiniFerihaKoydum::AddRequest(int type, Packet * pkt)
void AdiniFerihaKoydum::Initialize() {
	for (int i = 0; i < (int)dbreqtype::COUNT; i++) {
		_running[i] = true; m_nLockCount[i] = 0;
	}
	mthread[(int)dbreqtype::Database]  = std::thread(&AdiniFerihaKoydum::tDatabase, this);
	mthread[(int)dbreqtype::Database2] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database2);
	mthread[(int)dbreqtype::Database3] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database3);
	mthread[(int)dbreqtype::Database4] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database4);
	mthread[(int)dbreqtype::Database5] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database5);
	mthread[(int)dbreqtype::Database6] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database6);
	mthread[(int)dbreqtype::Database7] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database7);
	mthread[(int)dbreqtype::Database8] = std::thread(&AdiniFerihaKoydum::tDatabaseWorker, this, (int)dbreqtype::Database8);
	mthread[(int)dbreqtype::Logger]    = std::thread(&AdiniFerihaKoydum::tKnightLogger, this);
}
#pragma endregion

#pragma region AdiniFerihaKoydum::AddRequest(int type, Packet * pkt)
void AdiniFerihaKoydum::AddRequest(int type, Packet * pkt)
{
	_lock[type].lock();
	_queue[type].push(pkt);
	_lock[type].unlock();
	Signal(type);
}
#pragma endregion

// CHI-AUDIT: Ortak DB packet isleme fonksiyonu — kod duplikasyonu fix
// Return: true = packet consumed (caller must NOT delete p), false = normal (caller deletes p)
void AdiniFerihaKoydum::ProcessDatabasePacket(Packet& pkt)
{
	int16 uid = pkt.read<int16>();
	CUser* pUser = nullptr;

	if (uid >= 0) { pUser = g_pMain->GetUserPtr(uid); if (!pUser) return; }

	switch (pkt.GetOpcode())
	{
	case WIZ_LOGIN:
		if (pUser) pUser->ReqAccountLogIn(pkt);
		if (pUser) pUser->CaptchaProcess();
		break;
	case WIZ_KICKOUT:
		if (pUser) pUser->ReqKickOutProcess(pkt);
		break;
	case WIZ_SEL_NATION:
		if (pUser) pUser->ReqSelectNation(pkt);
		break;
	case WIZ_ALLCHAR_INFO_REQ:
		if (pUser) pUser->ReqAllCharInfo(pkt);
		break;
	case WIZ_CHANGE_HAIR:
		if (pUser) pUser->ReqChangeHair(pkt);
		break;
	case WIZ_NEW_CHAR:
		if (pUser) pUser->ReqCreateNewChar(pkt);
		break;
	case WIZ_DEL_CHAR:
		if (pUser) pUser->ReqDeleteChar(pkt);
		break;
	case WIZ_SEL_CHAR:
		if (pUser) pUser->ReqSelectCharacter(pkt);
		break;
	case WIZ_CHAT:
		break;
	case WIZ_DATASAVE:
		if (pUser) pUser->ReqSaveCharacter();
		break;
	case WIZ_KNIGHTS_PROCESS:
		if (!pUser) return;
		CKnightsManager::ReqKnightsPacket(pUser, pkt);
		break;
	case WIZ_REMOVE_CURRENT_USER:
		if (pUser) pUser->ReqRemoveCurrentUser(pkt);
		break;
	case WIZ_LOGIN_INFO:
		if (pUser) pUser->ReqSetLogInInfo(pkt);
		break;
	case WIZ_BATTLE_EVENT:
		if (pUser) pUser->BattleEventResult(pkt);
		break;
	case WIZ_SHOPPING_MALL:
		if (pUser) pUser->ReqShoppingMall(pkt);
		break;
	case WIZ_SKILLDATA:
		if (pUser) pUser->ReqSkillDataProcess(pkt);
		break;
	case WIZ_FRIEND_PROCESS:
		if (pUser) pUser->ReqFriendProcess(pkt);
		break;
	case WIZ_NAME_CHANGE:
		if (pUser) pUser->NameChangeSystem(pkt);
		break;
	case WIZ_CAPE:
		if (pUser) pUser->ReqChangeCape(pkt);
		break;
	case WIZ_LOGOUT:
		if (pUser) pUser->ReqUserLogOut();
		break;
	case WIZ_NATION_TRANSFER:
		if (pUser) pUser->ReqNationTransfer(pkt);
		break;
	case WIZ_KING:
		// Why: Timer'dan tetiklenen DB updateleri (KING_ELECTION_UPDATE_STATUS vb)
		// pUser=nullptr ile gelir. Eski erken return DB writelerini drop ediyordu.
		// Alt handlerlar (HandleDatabaseRequest_Election vs) gerekli yerlerde pUser null check yapiyor.
		CKingSystem::HandleDatabaseRequest(pUser, pkt);
		break;
	case WIZ_SIEGE:
		if (!pUser) return;
		g_pMain->HandleSiegeDatabaseRequest(pUser, pkt);
		break;
	case WIZ_ITEM_UPGRADE:
		if (pUser) pUser->ReqItemUpgrade(pkt);
		break;
	case WIZ_VIPWAREHOUSE:
		if (pUser) pUser->ReqVipStorageProcess(pkt);
		break;
	case WIZ_ZONE_CONCURRENT:
		if (pUser) pUser->ReqConcurrentProcess(pkt);
		break;
	case WIZ_PREMIUM:
		if (pUser) g_DBAgent.UpdatePremiumServiceUser(pUser);
		break;
	case WIZ_DB_DAILY_OP:
		if (pUser) pUser->DailyOpProcess(pkt);
		break;
	case WIZ_AUTHORITY_CHANGE:
		g_pMain->ReqHandleAuthority(pkt, pUser);
		break;
	case WIZ_EVENT:
		if (pUser) pUser->ReqHandleEvent(pkt, pUser);
		break;
	case WIZ_DB_SAVE_USER:
		if (pUser) pUser->ReqHandleUserDatabaseRequest(pkt);
		break;
	case WIZ_DB_SAVE:
		g_pMain->ReqHandleDatabasRequest(pkt);
		break;
	case XSafe:
		if (pUser) pUser->ReqHandleXSafe(pkt);
		break;
	case WIZ_EDIT_BOX:
		if (pUser) pUser->ReqPPCard(pkt);
		break;
	}
}

#pragma region AdiniFerihaKoydum::tDatabase()
void AdiniFerihaKoydum::tDatabase()
{
	while (true) {
		Packet* p = nullptr;
		_lock[(int)dbreqtype::Database].lock();
		if (_queue[(int)dbreqtype::Database].size()) {
			p = _queue[(int)dbreqtype::Database].front();
			_queue[(int)dbreqtype::Database].pop();
		}
		_lock[(int)dbreqtype::Database].unlock();

		if (!p) {
			if (!_running[(int)dbreqtype::Database]) break;
			Wait((int)dbreqtype::Database);
			continue;
		}

		ProcessDatabasePacket(*p);
		delete p;
	}
}
#pragma endregion

#pragma region AdiniFerihaKoydum::tDatabaseWorker(int queueType)
void AdiniFerihaKoydum::tDatabaseWorker(int queueType)
{
	while (true) {
		Packet* p = nullptr;
		_lock[queueType].lock();
		if (_queue[queueType].size()) {
			p = _queue[queueType].front();
			_queue[queueType].pop();
		}
		_lock[queueType].unlock();

		if (!p) {
			if (!_running[queueType]) break;
			Wait(queueType);
			continue;
		}

		ProcessDatabasePacket(*p);
		delete p;
	}
}
#pragma endregion

#pragma region AdiniFerihaKoydum::~AdiniFerihaKoydum()
AdiniFerihaKoydum::~AdiniFerihaKoydum()
{
	for (int i = 0; i < (int)dbreqtype::COUNT; i++) {
		_running[i] = false;
		Broadcast(i);

		_lock[i].lock();
		while (!_queue[i].empty()) {
			auto* p = _queue[i].front();
			_queue[i].pop();
			if (p) delete p;
		}
		_lock[i].unlock();
	}
}
#pragma endregion

#pragma region AdiniFerihaKoydum::Shutdown(int type)
void AdiniFerihaKoydum::Shutdown(int type)
{
	_running[type] = false;
	Broadcast(type);

	int dbpoppedCount = 0;

	_lock[type].lock();
	while (_queue[type].size()) {
		auto* p = _queue[type].front();
		_queue[type].pop();
		dbpoppedCount++;
		if (p) delete p;
	}
	_lock[type].unlock();

}
#pragma endregion