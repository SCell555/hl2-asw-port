#include "cbase.h"
#include "playerInventory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE(C_PlayerInventory, DT_Inventory)
RecvPropArray_UniqueElements(entNames, RecvPropString(RECVINFO(entNames[0]))),
RecvPropArray_UniqueElements(classNames, RecvPropString(RECVINFO(classNames[0]))),
RecvPropArray_UniqueElements(modelNames, RecvPropString(RECVINFO(modelNames[0]))),
RecvPropArray_UniqueElements(isOccupied, RecvPropBool(RECVINFO(isOccupied[0]))), 
END_RECV_TABLE()

C_PlayerInventory::C_PlayerInventory()
{
	for (int i = 0; i < 32; ++i)
	{
		Q_memset(entNames[i], 0, sizeof(char) * DT_MAX_STRING_BUFFERSIZE);
		Q_memset(classNames[i], 0, sizeof(char) * DT_MAX_STRING_BUFFERSIZE);
		Q_memset(modelNames[i], 0, sizeof(char) * DT_MAX_STRING_BUFFERSIZE);
		isOccupied.GetForModify(i) = false;
	}
}