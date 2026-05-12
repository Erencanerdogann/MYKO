#include "stdafx.h"
#include "DBAgent.h"

void CUser::GiveBalance(int32 cashcount, int32 tlcount, short ownerid)
{
	if (!cashcount && !tlcount)
		return;

	if (cashcount < -1000000 || cashcount > 1000000) return;
	if (tlcount < -1000000 || tlcount > 1000000) return;

	Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateAccountKnightCash));
	Save << cashcount << tlcount << ownerid;
	g_pMain->AddDatabaseRequest(Save, this);
}