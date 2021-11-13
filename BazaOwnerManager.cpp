#include "stdafx.h"
#include "BazaOwnerManager.h"
#include "FServerDlg.h"

BazaOwnerManager *g_pBazaOwnerMan;

#ifdef _DEBUG2
	#undef BOWNER_WEEK_DAY
	#undef BOWNER_HOUR
	#undef BOWNER_MINUTE
	#undef BOWNER_STAVKA_TIME

	#define BOWNER_WEEK_DAY					2
	#define BOWNER_HOUR						(22 - 3)
	#define BOWNER_MINUTE					13
	#define BOWNER_STAVKA_TIME				TM_ONE_MINUTE * 10
#endif

BazaOwnerManager::BazaOwnerManager()
{
	BAZA_OWNER_UNIT unit;
	unit.eOwnerID = UNDEFINED_ID;
	unit.eBonus = BOB_NONE;

	for (iD eID = FIRST_ID; ; eID++)
	{
		if (::GetBazaParam(eID, BP_ID) == NONE_PARAM)
			break;

		if (::GetBazaParam(eID, BP_OWN_STAVKA_TYPE) == NONE_PARAM)
			continue;

		m_mapBOwners[eID] = unit;
	}
}

BazaOwnerManager::~BazaOwnerManager()
{

}

void BazaOwnerManager::Start()
{
#ifdef _DEBUG2
	FFTime dwTime = ::GetServerTime();

	DATE_TIME strDTime;
	dwTime.GetDateTime(strDTime);

	CServerDlg::WriteString(L"Baza Owner manager started with %d.%d.%d %d:%d(%d)", strDTime.nDayOfMonth, strDTime.nMonth, strDTime.nYear, strDTime.nHour, strDTime.nMinute, strDTime.nDayOfWeek);
#else
	CServerDlg::WriteString(L"Baza Owner manager started with %d bazes", m_mapBOwners.size());
#endif

	ThreadManager::SetSleepTime_Min(1);
	ThreadManager::StartThread();
}

const MAP_BAZA_OWNERS & BazaOwnerManager::GetOwnersMap()
{
	return m_mapBOwners;
}

void BazaOwnerManager::SendOneBazaOwnerToUser(Player *pPlayer, iD eBazaID)
{
	if (pPlayer == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	MAP_BAZA_OWNERS::const_iterator ii = m_mapBOwners.find(eBazaID);
	if (ii == m_mapBOwners.end())
	{
		ASSERT_FALSE;
		return;
	}

	STR_BAZA_OWNER data;
	data.eBazaID = eBazaID;
	data.unit.eOwnerID = ii->second.eOwnerID;
	data.unit.eBonus = ii->second.eBonus;
	data.unit.strOwnerName[0] = 0;

	if (data.unit.eOwnerID != UNDEFINED_ID)
	{
		Player *pOwner = pPlayer->GetPlayerPTRByID(data.unit.eOwnerID);
		if (pOwner == NULL)
		{
			ASSERT_FALSE;
		}
		else
		{
			pOwner->GetNick(data.unit.strOwnerName);
		}
	}

	MAP_BOWNER_USER::const_iterator ik = ii->second.mapKondidats.find(pPlayer->GetID());
	if (ik == ii->second.mapKondidats.end())
		data.unit.nMyTotalStavka = 0;
	else
		data.unit.nMyTotalStavka = ik->second;

	pPlayer->SendMessage(SC_MSG_ONE_BAZA_OWNER, &data);
}

void BazaOwnerManager::OnUserSetStavka(Player *pPlayer, const STR_BOWNER_STAVKA *pData)
{
	if (pPlayer == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	if (pData == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	MAP_BAZA_OWNERS::iterator ib = m_mapBOwners.find(pData->eBazaID);
	if (ib == m_mapBOwners.end())
	{
		ASSERT_FALSE;
		return;
	}

	iD eUserID = pPlayer->GetID();
	if (ib->second.eOwnerID == eUserID)
	{
		ASSERT_FALSE;
		return;
	}

	ItemManager *pItemMan;
	pPlayer->GetItemMan(&pItemMan);

	if (pItemMan == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	switch(pItemMan->OnDoBOwnerStavka(pPlayer->m_Data.baseInfo, pData->eBazaID, pData->nStavka))
	{
	case BSR_OK:
		{
			MAP_BOWNER_USER::iterator ik = ib->second.mapKondidats.find(eUserID);
			if (ik == ib->second.mapKondidats.end())
			{
				ib->second.mapKondidats[eUserID] = pData->nStavka;
			}
			else
			{
				ik->second += pData->nStavka;
			}
		}
		break;

	default:
		ASSERT_FALSE;
		break;
	}

	SendOneBazaOwnerToUser(pPlayer, pData->eBazaID);
	pPlayer->OnUserBaseInfoChanged();
}

void BazaOwnerManager::OnBOwnerSetNewAllBonus(Player *pPlayer, const STR_BOWNER_NEW_ALL_BONUS *pData)
{
	if (pPlayer == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	if (pData == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	MAP_BAZA_OWNERS::iterator ib = m_mapBOwners.find(pData->eBazaID);
	if (ib == m_mapBOwners.end())
	{
		ASSERT_FALSE;
		return;
	}

	iD eUserID = pPlayer->GetID();
	if (ib->second.eOwnerID != eUserID)
	{
		ASSERT_FALSE;
		return;
	}

	switch(pData->eBonus)
	{
	case BOB_FISH_COST_PLUS:
	case BOB_FISH_EXP_PLUS:
	case BOB_LOOT_PLUS:
	case BOB_KLAD_PLUS:
		break;

	default:
		ASSERT_FALSE;
		return;
	}

	if (ib->second.eBonus == pData->eBonus)
	{
		ASSERT_FALSE;
		return;
	}

	ib->second.eBonus = pData->eBonus;
	SendOneBazaOwnerToAllUsers(ib->first);
}

void BazaOwnerManager::DB_SaveData(FFDBStorage *pDBStorage)
{
	if (pDBStorage == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	pDBStorage->m_mapBOwnerBaza.clear();
	pDBStorage->m_mapBOwnerStavka.clear();
	pDBStorage->m_mapBOwner.clear();

	for (MAP_BAZA_OWNERS::const_iterator ii = m_mapBOwners.begin();
		ii != m_mapBOwners.end(); ++ii)
	{
		STR_BOWNER_BAZA bazaData;
		bazaData.eOwnerID = ii->second.eOwnerID;
		bazaData.data.eBazaID = ii->first;
		bazaData.data.eBonus = ii->second.eBonus;

		pDBStorage->UpdateOneBOwnerBaza(bazaData);

		STR_BOWNER_BAZA_STAVKA stavkaData;
		stavkaData.data.eBazaID = ii->first;

		for (MAP_BOWNER_USER::const_iterator ik = ii->second.mapKondidats.begin();
			ik != ii->second.mapKondidats.end(); ++ik)
		{
			stavkaData.eUserID = ik->first;
			stavkaData.data.nStavka = ik->second;

			pDBStorage->UpdateOneBOwnerBazaStavka(stavkaData);
		}

		FF_BOWNER bownerData;
		bownerData.eBazaID = ii->first;

		for (MAP_BOWNER_USER::const_iterator ib = ii->second.mapOwners.begin();
			ib != ii->second.mapOwners.end(); ++ib)
		{
			bownerData.eUserID = ib->first;
			bownerData.nTotalVladenij = ib->second;

			pDBStorage->UpdateOneBOwner(bownerData);
		}
	}
}

void BazaOwnerManager::DB_LoadData(FFDBStorage *pDBStorage)
{
	if (pDBStorage == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	for (FMAP_BOWNER_BAZA::const_iterator ii = pDBStorage->m_mapBOwnerBaza.begin();
		ii != pDBStorage->m_mapBOwnerBaza.end(); ++ii)
	{
		MAP_BAZA_OWNERS::iterator ib = m_mapBOwners.find(ii->first);
		if (ib == m_mapBOwners.end())
		{
			ASSERT_FALSE;
			continue;
		}

		ib->second.eBonus = ii->second.data.eBonus;
		ib->second.eOwnerID = ii->second.eOwnerID;

#ifdef _DEBUG
		if (ii->first == 16)
		{
			ib->second.eOwnerID = 15;
			ib->second.eBonus = BOB_KLAD_PLUS;
		}
#endif

	}

	for (FMAP_BOWNER_BAZA_STAVKA::const_iterator ii = pDBStorage->m_mapBOwnerStavka.begin();
		ii != pDBStorage->m_mapBOwnerStavka.end(); ++ii)
	{
		MAP_BAZA_OWNERS::iterator ib = m_mapBOwners.find(ii->second.data.eBazaID);
		if (ib == m_mapBOwners.end())
		{
			ASSERT_FALSE;
			continue;
		}

		ib->second.mapKondidats[ii->second.eUserID] = ii->second.data.nStavka;
	}

	for (FMAP_BOWNER::const_iterator ii = pDBStorage->m_mapBOwner.begin();
		ii != pDBStorage->m_mapBOwner.end(); ++ii)
	{
		MAP_BAZA_OWNERS::iterator ib = m_mapBOwners.find(ii->second.eBazaID);
		if (ib == m_mapBOwners.end())
		{
			ASSERT_FALSE;
			continue;
		}

		ib->second.mapOwners[ii->second.eUserID] = ii->second.nTotalVladenij;
	}

#ifdef _DEBUG2
	MAP_BAZA_OWNERS::iterator it = m_mapBOwners.find(28);
	it->second.mapKondidats[1] = 150;
	it->second.mapKondidats[10] = 100;

	it = m_mapBOwners.find(16);
	it->second.mapKondidats[15] = 200;
#endif
}

void BazaOwnerManager::OnUserEnterEveryday(Player *pPlayer)
{
	if (pPlayer == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	STR_BOWNER_ENTER data;
	ItemManager *pItemMan = NULL;

	iD eUserID = pPlayer->GetID();

	for (MAP_BAZA_OWNERS::const_iterator ii = m_mapBOwners.begin();
		ii != m_mapBOwners.end(); ++ii)
	{
		if (ii->second.eOwnerID != eUserID)
			continue;

		if (pItemMan == NULL)
		{
			pPlayer->GetItemMan(&pItemMan);
			pPlayer->GetNick(data.strOwnerName);
		}

		iD eBazaID = ii->first;
		data.eBazaID = eBazaID;

		pPlayer->SendMesToAllUsersExceptMe(CS_MSG_BOWNER_ENTER, &data);

		if (pItemMan == NULL)
		{
			ASSERT_FALSE;
			continue;
		}

		int nVal = (int)::GetBazaParam(eBazaID, BP_OWN_EV_BONUS_ITEM_TYPE);
		if (nVal != ITEM_TYPE_NONE)
		{
			ITEM_TYPE eIType = (ITEM_TYPE)nVal;
			iD  eIID = (iD)::GetBazaParam(eBazaID, BP_OWN_EV_BONUS_ITEM_ID);
			nVal = (int)::GetBazaParam(eBazaID, BP_OWN_EV_BONUS_ITEM_COUNT);

			pItemMan->AddItem(eIType, eIID, nVal);
		}

		struct
		{
			BAZA_PARAM eBParam;
			int *pVal;
		}arrData[]=
		{
			{BP_OWN_EV_BONUS_4ESHUIKI,		&pPlayer->m_Data.baseInfo.n4eshuiki},
			{BP_OWN_EV_BONUS_LIKES,			&pPlayer->m_Data.baseInfo.nLikesCNT},
			{BP_OWN_EV_BONUS_ZHETONS,		&pPlayer->m_Data.baseInfo.nZhetonCNT},
			{BP_OWN_EV_BONUS_GOLD,			&pPlayer->m_Data.baseInfo.gold},
			{BP_OWN_EV_BONUS_FISHKI,		&pPlayer->m_Data.baseInfo.nFishkaCNT},
			{BP_OWN_EV_BONUS_STAT_SCORES,	NULL},
		};

		BAZA_PARAM eBParam;
		for (int cc = 0; cc < ARRAYSIZE(arrData); cc++)
		{
			eBParam = arrData[cc].eBParam;
			nVal = (int)::GetBazaParam(eBazaID, eBParam);
			if (nVal == NONE_PARAM || nVal < 1)
				continue;

			switch(eBParam)
			{
			case BP_OWN_EV_BONUS_STAT_SCORES:
				pPlayer->m_Data.baseInfo.AddAddStatScores(nVal);
				break;

			default:
				arrData[cc].pVal[0] += nVal;
				break;
			}
		}
	}
}

void BazaOwnerManager::ThreadTick()
{
	LockCS();

	FFTime dwCurTime = ::GetServerTime();

	SYSTEMTIME sysCurTime;
	dwCurTime.GetSysDateTime(sysCurTime);

	int nCurTT = sysCurTime.wHour * 60 + sysCurTime.wMinute;
	int nBowTT = BOWNER_HOUR * 60 + BOWNER_MINUTE;

	if (sysCurTime.wDayOfWeek != BOWNER_WEEK_DAY ||
		nCurTT < nBowTT)
	{
		UnlockCS();
		return;
	}

	SYSTEMTIME sysPrevTime;
	g_Jack.dwLastBOwnerEndTime.GetSysDateTime(sysPrevTime);

	if (sysPrevTime.wYear == sysCurTime.wYear &&
		sysPrevTime.wMonth == sysCurTime.wMonth &&
		sysPrevTime.wDay == sysCurTime.wDay)
	{
		UnlockCS();
		return;
	}

	g_Jack.dwLastBOwnerEndTime = dwCurTime;
	CalcNewBOwners();
	UnlockCS();
}

void BazaOwnerManager::CalcNewBOwners()
{
	for (MAP_BAZA_OWNERS::iterator ib = m_mapBOwners.begin();
		ib != m_mapBOwners.end(); ++ib)
	{
		CalcNewBOwner(ib->first, ib->second);
		SendOneBazaOwnerToAllUsers(ib->first);
	}
}

bool BazaOwnerManager::SortBUsers(const STR_BUSER &rA, const STR_BUSER &rB)
{
	return rA.nToatalStavka > rB.nToatalStavka;
}

bool BazaOwnerManager::SortSBUsers(const STR_BOWUSER &rA, const STR_BOWUSER &rB)
{
	return rA.nVal > rB.nVal;
}

void BazaOwnerManager::CalcNewBOwner(iD eBazaID, BAZA_OWNER_UNIT &rBUnit)
{
	BUSER_LIST uList;
	STR_BUSER bUser;

	for (MAP_BOWNER_USER::const_iterator ik = rBUnit.mapKondidats.begin();
		ik != rBUnit.mapKondidats.end(); ++ik)
	{
		bUser.pPlayer = Player::GetPlayerPTRByID(ik->first);
		if (bUser.pPlayer == NULL)
		{
			ASSERT_FALSE;
			continue;
		}

		bUser.nToatalStavka = ik->second;
		uList.push_back(bUser);
	}

	rBUnit.mapKondidats.clear();

	if (uList.size() < 1)
	{
		rBUnit.eOwnerID = UNDEFINED_ID;
		rBUnit.eBonus = BOB_NONE;
		return;
	}

	uList.sort(BazaOwnerManager::SortBUsers);
	rBUnit.eBonus = BOB_FISH_COST_PLUS;

	int nPlace = 1;
	for (BUSER_LIST::iterator ib = uList.begin();
		ib != uList.end(); ++ib, nPlace++)
	{
		if (nPlace == 1)
		{
			rBUnit.eOwnerID = ib->pPlayer->GetID();
			MAP_BOWNER_USER::iterator bo = rBUnit.mapOwners.find(rBUnit.eOwnerID);
			if (bo == rBUnit.mapOwners.end())
				rBUnit.mapOwners[rBUnit.eOwnerID] = 1;
			else
				bo->second++;
		}

		ib->pPlayer->OnTakeBOwnerPlace(eBazaID, nPlace, ib->nToatalStavka);
	}

	SBUSER_LIST arrOwnersList;
	OwnerMapToOwnerTop100List(rBUnit.mapOwners, arrOwnersList);

	rBUnit.mapOwners.clear();

	for (SBUSER_LIST::const_iterator is = arrOwnersList.begin();
		is != arrOwnersList.end(); ++is)
	{
		rBUnit.mapOwners[is->eUserID] = is->nVal;
	}
}

void BazaOwnerManager::SendOneBazaOwnerToAllUsers(iD eBazaID)
{
	for (UID_LIST::const_iterator ii = g_OnlineUsers.begin(); 
		ii != g_OnlineUsers.end(); ++ii)
	{
		Player *pPlayer = Player::GetPlayerPTRByID(*ii);
		if (pPlayer == NULL)
		{
			ASSERT_FALSE;
			continue;
		}

		SendOneBazaOwnerToUser(pPlayer, eBazaID);
	}
}

int BazaOwnerManager::GetUserBazaBonusPlus(iD eUserID, iD eLocID, BAZA_OWNER_BONUS eBParam)
{
	int nRet = 0;

	BAZA_PARAM eSelfBParam;
	int nStdVal;
	switch (eBParam)
	{
	case BOB_FISH_COST_PLUS:
		eSelfBParam = BP_OWN_BONUS_FISH_COST;
		nStdVal = BOWNER_FISH_COST_PLUS;
		break;

	case BOB_FISH_EXP_PLUS:
		eSelfBParam = BP_OWN_BONUS_FISH_EXP;
		nStdVal = BOWNER_FISH_EXP_PLUS;
		break;

	case BOB_LOOT_PLUS:
		eSelfBParam = BP_OWN_BONUS_LOOT_PLUS;
		nStdVal = BOWNER_LOOT_CHANCE;
		break;

	case BOB_KLAD_PLUS:
		eSelfBParam = BP_OWN_BONUS_KLAD_PLUS;
		nStdVal = BOWNER_KLAD_CHANCE;
		break;

	default:
		ASSERT_FALSE;
		return nRet;
	}

	int nVal = ::GetLocParam(eLocID, LP_BAZA_ID);
	if (nVal == NONE_PARAM)
	{
		ASSERT_FALSE;
		return nRet;
	}

	MAP_BAZA_OWNERS::const_iterator ii = m_mapBOwners.find(nVal);
	if (ii == m_mapBOwners.end())
		return nRet;

	iD eBazaID = ii->first;
	if (ii->second.eOwnerID == eUserID)
	{
		int nAddVal = (int)::GetBazaParam(eBazaID, eSelfBParam);
		if (nAddVal != NONE_PARAM && nAddVal > 0)
			nRet += nAddVal;
	}

	if (ii->second.eBonus == eBParam)
		nRet += nStdVal;

	return nRet;
}

void BazaOwnerManager::OwnerMapToOwnerTop100List(const MAP_BOWNER_USER &rMap, SBUSER_LIST &rList)
{
	rList.clear();

	STR_BOWUSER sUser;
	for (MAP_BOWNER_USER::const_iterator io = rMap.begin();
		io != rMap.end(); ++io)
	{
		sUser.eUserID = io->first;
		sUser.nVal = io->second;

		rList.push_back(sUser);
	}

	rList.sort(BazaOwnerManager::SortSBUsers);
	if (rList.size() > BOWNER_TOP100_COUNT)
		rList.resize(BOWNER_TOP100_COUNT);
}

void BazaOwnerManager::AddFishExp(iD eUserID, iD eLocID, int &rExp)
{
	int nVal = GetUserBazaBonusPlus(eUserID, eLocID, BOB_FISH_EXP_PLUS);
	if (nVal < 1)
		return;

	::AddPercents(rExp, nVal);
}

void BazaOwnerManager::AddFishCost(iD eUserID, iD eLocID, int &rCost)
{
	int nVal = GetUserBazaBonusPlus(eUserID, eLocID, BOB_FISH_COST_PLUS);
	if (nVal < 1)
		return;

	::AddPercents(rCost, nVal);
}

void BazaOwnerManager::GetBonusChances(iD eUserID, iD eLocID, int &rMinVal, int &rMaxVal, BAZA_OWNER_BONUS eBParam)
{
	int nVal = GetUserBazaBonusPlus(eUserID, eLocID, eBParam);
	if (nVal < 1)
		return;

	rMinVal = ::DecNumPercents(rMinVal, nVal);
	rMaxVal = ::DecNumPercents(rMaxVal, nVal);

	Max(rMinVal, 1);
	Max(rMaxVal, 1);
}

void BazaOwnerManager::AddRecordGold(iD eUserID, iD eLocID, int &rGold)
{
	int nVal = ::GetLocParam(eLocID, LP_BAZA_ID);
	if (nVal == NONE_PARAM)
	{
		ASSERT_FALSE;
		return;
	}

	MAP_BAZA_OWNERS::const_iterator ii = m_mapBOwners.find(nVal);
	if (ii == m_mapBOwners.end())
		return;

	if (ii->second.eOwnerID != eUserID)
		return;

	int nAddVal = (int)::GetBazaParam(ii->first, BP_OWN_BONUS_RECORD_GOLD_PLUS);
	if (nAddVal != NONE_PARAM && nAddVal > 0)
		::AddPercents(rGold, nAddVal);
}

void BazaOwnerManager::OnUserWannaBOwnersTop100(Player *pPlayer, iD eBazaID)
{
	if (pPlayer == NULL)
	{
		ASSERT_FALSE;
		return;
	}

	SBUSER_LIST top100OwnersList;
	switch(eBazaID)
	{
	case UNDEFINED_ID:
		{
			MAP_BOWNER_USER mapAllOwners;
			for (MAP_BAZA_OWNERS::const_iterator ib = m_mapBOwners.begin();
				ib != m_mapBOwners.end(); ++ib)
			{
				for (MAP_BOWNER_USER::const_iterator iu = ib->second.mapOwners.begin();
					iu != ib->second.mapOwners.end(); ++iu)
				{
					MAP_BOWNER_USER::iterator uu = mapAllOwners.find(iu->first);
					if (uu == mapAllOwners.end())
						mapAllOwners[iu->first] = iu->second;
					else
						uu->second += iu->second;
				}
			}

			OwnerMapToOwnerTop100List(mapAllOwners, top100OwnersList);
		}
		break;

	default:
		{
			MAP_BAZA_OWNERS::const_iterator ib = m_mapBOwners.find(eBazaID);
			if (ib == m_mapBOwners.end())
			{
				ASSERT_FALSE;
				return;
			}

			OwnerMapToOwnerTop100List(ib->second.mapOwners, top100OwnersList);
		}
		break;
	}

	STR_ONE_BOWNER data;
	int nPlace = 1;
	for (SBUSER_LIST::const_iterator ii = top100OwnersList.begin();
		ii != top100OwnersList.end(); ++ii)
	{
		Player *pP = pPlayer->GetPlayerPTRByID(ii->eUserID);
		if (pP == NULL)
		{
			ASSERT_FALSE;
			continue;
		}

		data.nPlace = nPlace++;
		data.eUserID = ii->eUserID;
		data.nLevel = ::GetUserLevel(pP->m_Data.baseInfo);
		data.nTotalVladenij = ii->nVal;
		pP->GetNick(data.strNick);
		pP->GetTeamName(data.strTeam);

		pPlayer->SendMessage(SC_MSG_ONE_BOWNER, &data);
	}

	pPlayer->SendMessage(SC_MSG_BOWNER_LIST_END);
}
