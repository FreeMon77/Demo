#pragma once

#include "Player.h"

typedef std::map<iD, int> MAP_BOWNER_USER;

typedef struct
{
	iD eOwnerID;
	BAZA_OWNER_BONUS eBonus;
	MAP_BOWNER_USER mapKondidats;
	MAP_BOWNER_USER mapOwners;
}BAZA_OWNER_UNIT;

typedef std::map<iD, BAZA_OWNER_UNIT> MAP_BAZA_OWNERS;

typedef struct
{
	Player *pPlayer;
	int nToatalStavka;
}STR_BUSER;

typedef std::list<STR_BUSER>			BUSER_LIST;

typedef struct 
{
	iD eUserID;
	int nVal;
}STR_BOWUSER;

typedef std::list<STR_BOWUSER>			SBUSER_LIST;

class BazaOwnerManager : public ThreadManager
{
public:
	BazaOwnerManager();
	~BazaOwnerManager();
	void Start();
	const MAP_BAZA_OWNERS &GetOwnersMap();
	void SendOneBazaOwnerToUser(Player *pPlayer, iD eBazaID);
	void OnUserSetStavka(Player *pPlayer, const STR_BOWNER_STAVKA *pData);
	void OnBOwnerSetNewAllBonus(Player *pPlayer, const STR_BOWNER_NEW_ALL_BONUS *pData);
	void DB_SaveData(FFDBStorage *pDBStorage);
	void DB_LoadData(FFDBStorage *pDBStorage);
	void OnUserEnterEveryday(Player *pPlayer);
	void AddFishExp(iD eUserID, iD eLocID, int &rExp);
	void AddFishCost(iD eUserID, iD eLocID, int &rCost);
	void GetBonusChances(iD eUserID, iD eLocID, int &rMinVal, int &rMaxVal, BAZA_OWNER_BONUS eBParam);
	void AddRecordGold(iD eUserID, iD eLocID, int &rGold);
	void OnUserWannaBOwnersTop100(Player *pPlayer, iD eBazaID);

protected:
	virtual void ThreadTick();
	void CalcNewBOwners();
	void CalcNewBOwner(iD eBazaID, BAZA_OWNER_UNIT &rBUnit);
	void SendOneBazaOwnerToAllUsers(iD eBazaID);
	int GetUserBazaBonusPlus(iD eUserID, iD eLocID, BAZA_OWNER_BONUS eBParam);
	void OwnerMapToOwnerTop100List(const MAP_BOWNER_USER &rMap, SBUSER_LIST &rList);

protected:
	static bool SortBUsers(const STR_BUSER &rA, const STR_BUSER &rB);
	static bool SortSBUsers(const STR_BOWUSER &rA, const STR_BOWUSER &rB);

protected:
	MAP_BAZA_OWNERS m_mapBOwners;
};

extern BazaOwnerManager *g_pBazaOwnerMan;