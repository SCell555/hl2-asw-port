#include "cbase.h"
#include "playerInventory.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SEND_TABLE_NOBASE( PlayerInventory, DT_Inventory )
SendPropArray_UniqueElements(entNames, SendPropStringT(SENDINFO_NETWORKARRAYELEM(entNames,0))),
SendPropArray_UniqueElements(classNames, SendPropStringT(SENDINFO_NETWORKARRAYELEM(classNames,0))),
SendPropArray_UniqueElements(modelNames, SendPropStringT(SENDINFO_NETWORKARRAYELEM(modelNames,0))),
SendPropArray_UniqueElements(isOccupied, SendPropBool(SENDINFO_NETWORKARRAYELEM(isOccupied,0))),
END_SEND_TABLE()

BEGIN_SIMPLE_DATADESC( PlayerInventory )
DEFINE_AUTO_ARRAY(entNames, FIELD_STRING),
DEFINE_AUTO_ARRAY(classNames, FIELD_STRING),
DEFINE_AUTO_ARRAY(modelNames, FIELD_STRING),
DEFINE_AUTO_ARRAY(isOccupied, FIELD_BOOLEAN),
END_DATADESC()

PlayerInventory::PlayerInventory()
{
	for (int i = 0; i < 32; ++i)
	{
		entNames.GetForModify(i) = NULL_STRING;
		classNames.GetForModify(i) = NULL_STRING;
		modelNames.GetForModify(i) = NULL_STRING;
		isOccupied.GetForModify(i) = false;
	}
}

const short SAVERESTOREINVENTORYMGRVER = 1;

class SaveRestoreInventoryMgr : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "Inventory";
	}

	struct SaveRestStruct
	{
		//size_t sizes[3];
		bool isOcc;
		char entName[DT_MAX_STRING_BUFFERSIZE];
		char className[DT_MAX_STRING_BUFFERSIZE];
		char mdlName[DT_MAX_STRING_BUFFERSIZE];
	};

	void Save(ISave *pSave)
	{
		pSave->StartBlock("Inventory");

		SaveRestStruct st[32];

		CHL2_Player *pl = dynamic_cast<CHL2_Player*>(UTIL_GetLocalPlayer());

		if (!pl)
			return;

		for (int i = 0; i < 32; i++)
		{
			SaveRestStruct &s = st[i];
			
			Q_memcpy(s.entName, pl->m_Inventory.entNames.Get(i).ToCStr(), sizeof(char) * DT_MAX_STRING_BUFFERSIZE);
			Q_memcpy(s.className, pl->m_Inventory.classNames.Get(i).ToCStr(), sizeof(char) * DT_MAX_STRING_BUFFERSIZE);
			Q_memcpy(s.mdlName, pl->m_Inventory.modelNames.Get(i).ToCStr(), sizeof(char) * DT_MAX_STRING_BUFFERSIZE);
			s.isOcc = pl->m_Inventory.isOccupied.Get(i);

			/*const char* entName = pl->m_Inventory.entNames.Get(i).ToCStr();
			const char* className = pl->m_Inventory.classNames.Get(i).ToCStr();
			const char* mdlName = pl->m_Inventory.modelNames.Get(i).ToCStr();
			size_t s1 = (entName && *entName) ? Q_strlen(entName) + 1 : 0;
			size_t s2 = (className && *className) ? Q_strlen(className) +1 : 0;
			size_t s3 = (mdlName && *mdlName) ? Q_strlen(mdlName) + 1 : 0;
			size_t os[] = { s1, s2, s3 };
			Q_memcpy(s.sizes, os, sizeof(size_t) * 3);
			if (s1 - 1)
				Q_memcpy(s.entName, entName, s1);
			else
				Q_memset(s.entName, 0, 64 * sizeof(char));
			if (s2 - 1)
				Q_memcpy(s.className, className, s2);
			else
				Q_memset(s.className, 0, 64 * sizeof(char));
			if (s3 - 1)
				Q_memcpy(s.mdlName, mdlName, s3);
			else
				Q_memset(s.mdlName, 0, MAX_PATH * sizeof(char));
			s.isOcc = pl->m_Inventory.isOccupied.Get(i);*/
		}

		const void* outData = st;

		size_t outSize = sizeof(st);

		pSave->WriteInt(&outSize);

		pSave->WriteData(static_cast<const char*>(outData), outSize);

		pSave->EndBlock();
	}

	void WriteSaveHeaders(ISave *pSave)
	{
		pSave->WriteShort(&SAVERESTOREINVENTORYMGRVER);
	}

	void ReadRestoreHeaders(IRestore *pRestore)
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort(&version);
		// only load if version matches and if we are loading a game, not a transition
		m_fDoLoad = ((version == SAVERESTOREINVENTORYMGRVER) &&
			((MapLoad_LoadGame == gpGlobals->eLoadType) || (MapLoad_NewGame == gpGlobals->eLoadType))
			);
	}

	void Restore(IRestore *pRestore, bool createPlayers)
	{
		if (m_fDoLoad)
		{
			pRestore->StartBlock();
			
			size_t inSize;

			pRestore->ReadInt(&inSize);

			DevWarning("input size:%i\n", inSize);

			char *inData = new char[inSize];

			pRestore->ReadData(inData, inSize, 0);

			SaveRestStruct *st = static_cast<SaveRestStruct*>(static_cast<void*>(inData));

			CHL2_Player *pl = dynamic_cast<CHL2_Player*>(UTIL_GetLocalPlayer());

			if (!pl)
				goto del;

			for (int i = 0; i < 32; i++)
			{
				Color c(8 * i, 8 * i, 8 * i, 255);
				if (developer.GetBool())ConColorMsg(c, "No:%i init\n", i);
				SaveRestStruct &s = st[i];
				auto &inv = pl->m_Inventory;
				/*if (s.sizes[0])
				{
					inv.entNames.GetForModify(i) = new char[s.sizes[0]];
					Q_memcpy(inv.entNames.GetForModify(i), s.entName, s.sizes[0]);
				}
				else
					inv.entNames.GetForModify(i) = NULL;
				if (developer.GetBool())ConColorMsg(c, "No:%i step1\n", i);
				if (s.sizes[1])
				{
					inv.classNames.GetForModify(i) = new char[s.sizes[1]];
					Q_memcpy(inv.classNames.GetForModify(i), s.className, s.sizes[1]);
				}
				else
					inv.classNames.GetForModify(i) = NULL;
				if (developer.GetBool())ConColorMsg(c, "No:%i step2\n", i);
				if (s.sizes[2])
				{
					inv.modelNames.GetForModify(i) = new char[s.sizes[2]];
					Q_memcpy(inv.modelNames.GetForModify(i), s.mdlName, s.sizes[2]);
				}
				else
					inv.modelNames.GetForModify(i) = NULL;
				if (developer.GetBool())ConColorMsg(c, "No:%i step3\n", i);*/
				inv.classNames.GetForModify(i) = !(s.className && *s.className) ? NULL_STRING : !FindPooledString(s.className) ? AllocPooledString(s.className) : FindPooledString(s.className);
				inv.entNames.GetForModify(i) = !(s.entName && *s.entName) ? NULL_STRING : !FindPooledString(s.entName) ? AllocPooledString(s.entName) : FindPooledString(s.entName);
				inv.modelNames.GetForModify(i) = !(s.mdlName && *s.mdlName) ? NULL_STRING : !FindPooledString(s.mdlName) ? AllocPooledString(s.mdlName) : FindPooledString(s.mdlName);
				inv.isOccupied.GetForModify(i) = s.isOcc;
			}
		del:
			delete[] inData;
			if (developer.GetBool())ConColorMsg(Color(0, 255, 0, 255), "deleting...\n");
			pRestore->EndBlock();
		}
	}

private:
	bool m_fDoLoad;
};


SaveRestoreInventoryMgr g_saveRestoreInventoryMgr;

ISaveRestoreBlockHandler* InventorySaverestoreMgr()
{
	return &g_saveRestoreInventoryMgr;
}