#ifndef PLAYER_INV_H
#define PLAYER_INV_H

#ifdef _WIN32
#pragma once
#endif

#ifdef HL2_DLL
#include "networkvar.h"
#else
#include "dt_recv.h"
#endif

//typedef char* str;
using str = char[DT_MAX_STRING_BUFFERSIZE];

#ifdef HL2_DLL
class PlayerInventory
{
public:
	DECLARE_SIMPLE_DATADESC();
	DECLARE_CLASS_NOBASE( PlayerInventory );
	DECLARE_EMBEDDED_NETWORKVAR();

	PlayerInventory();

	CNetworkArray(string_t, entNames, 32);
	CNetworkArray(string_t, classNames, 32);
	CNetworkArray(string_t, modelNames, 32);
	CNetworkArray(bool, isOccupied, 32);
};

EXTERN_SEND_TABLE( DT_Inventory );

#else

class C_PlayerInventory
{
public:
	DECLARE_CLASS_NOBASE( C_PlayerInventory );
	DECLARE_EMBEDDED_NETWORKVAR();

	C_PlayerInventory();

	str entNames[32];
	str classNames[32];
	str modelNames[32];

	//CNetworkArray(str, entNames, 32);
	//CNetworkArray(str, classNames, 32);
	//CNetworkArray(str, modelNames, 32);

	CNetworkArray(bool, isOccupied, 32);
};

EXTERN_RECV_TABLE(DT_Inventory);

#endif
#endif