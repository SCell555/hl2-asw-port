#ifndef	WEAPONCUSTOM_H
#define	WEAPONCUSTOM_H

#include "basehlcombatweapon.h"
#include "weapon_rpg.h"

class CWeaponCustom : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponCustom, CHLSelectFireMachineGun );

	CWeaponCustom();

	DECLARE_SERVERCLASS();

	//DECLARE_ACTTABLE();
	
	void	Precache( void );
	void	AddViewKick(float easyDampen, float degrees, float seconds);
	void	ShootBullets( bool isPrimary = true, bool usePrimaryAmmo = true );
	void	ShootBulletsRight(bool isPrimary = true, bool usePrimaryAmmo = true);
	void	ShootBulletsLeft(bool isPrimary = true, bool usePrimaryAmmo = true);

	void	ShootProjectile( bool isPrimary, bool usePrimaryAmmo );
	void	ShootProjectileRight(bool isPrimary, bool usePrimaryAmmo);
	void	ShootProjectileLeft(bool isPrimary, bool usePrimaryAmmo);

	void	ShootAR2EnergyBall(bool isPrimary, bool usePrimaryAmmo);
	void	ShootAR2EnergyBallRight(bool isPrimary, bool usePrimaryAmmo);
	void	ShootAR2EnergyBallLeft(bool isPrimary, bool usePrimaryAmmo);

	void	ShootSMGGrenade(bool isPrimary, bool usePrimaryAmmo);
	void	ShootSMGGrenadeRight(bool isPrimary, bool usePrimaryAmmo);
	void	ShootSMGGrenadeLeft(bool isPrimary, bool usePrimaryAmmo);
	//void	ShootFragGrenadeThrow(bool isPrimary, bool usePrimaryAmmo);
	//void	ShootFragGrenadeRoll(bool isPrimary, bool usePrimaryAmmo);
	void	ItemPostFrame( void );
	void	ItemBusyFrame(void);
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );
	void	Drop(const Vector &vecVelocity);
	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	//void	ApplyCustomization(void);

	float	GetPrimaryFireRate( void ) 
	{ 
		if (IsIronsighted())
		{
			return this->GetWpnData().primary.ironsightFireRate;
		}
		else if (m_bInZoom)
		{
			return this->GetWpnData().primary.zoomFireRate;
		}
		else
		{
			return this->GetWpnData().primary.fireRate;
		} 
	}

	float	GetSecondaryFireRate(void)
	{
		if (IsIronsighted())
		{
			return this->GetWpnData().secondary.ironsightFireRate;
		}
		else if (m_bInZoom)
		{
			return this->GetWpnData().secondary.zoomFireRate;
		}
		else
		{
			return this->GetWpnData().secondary.fireRate;
		}
	}

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack2Condition( float flDot, float flDist );
	Activity	GetPrimaryAttackActivity( void );
	Activity	GetLeftGunActivity(void);
	Activity	GetRightGunActivity(void);

	virtual const Vector& GetBulletSpreadPrimary( void )
	{
		static const Vector cone = this->GetWpnData().primary.spread;
		static const Vector ironsightCone = this->GetWpnData().primary.ironsightSpread;
		static const Vector zoomCone = this->GetWpnData().primary.zoomSpread;
		if (IsIronsighted())
		{
			return ironsightCone;
		}
		else if (m_bInZoom)
		{
			return zoomCone;
		}
		else
		{
			return cone;
		}
	}

	virtual const Vector& GetBulletSpreadSecondary( void )
	{
		static const Vector cone = this->GetWpnData().secondary.spread;
		static const Vector ironsightCone = this->GetWpnData().secondary.ironsightSpread;
		static const Vector zoomCone = this->GetWpnData().secondary.zoomSpread;
		if (IsIronsighted())
		{
			return ironsightCone;
		}
		else if (m_bInZoom)
		{
			return zoomCone;
		}
		else
		{
			return cone;
		}
	}
	bool IsPrimaryBullet(void) { return this->GetWpnData().primary.bulletEnabled; }
	bool IsSecondaryBullet(void) { return this->GetWpnData().secondary.bulletEnabled; }

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
protected: //Why did I not put this in? I have no idea...
		CHandle<CMissile>	m_hMissile;
		CHandle<CMissile>	m_hMissile2;
private:
	void	CheckZoomToggle(void);
	void	ToggleZoom(void);

private:
	bool				m_bInZoom;
	bool				bFlip;
};

#define CustomWeaponAdd( num )										\
class CWeaponCustom##num : public CWeaponCustom						\
{																	\
	DECLARE_DATADESC();												\
	public:															\
	DECLARE_CLASS( CWeaponCustom##num, CWeaponCustom );				\
	CWeaponCustom##num() {};										\
	DECLARE_SERVERCLASS();											\
};																	\
IMPLEMENT_SERVERCLASS_ST(CWeaponCustom##num, DT_WeaponCustom##num)	\
END_SEND_TABLE()													\
BEGIN_DATADESC( CWeaponCustom##num )										\
END_DATADESC()														\
LINK_ENTITY_TO_CLASS( weapon_custom##num, CWeaponCustom##num );		\
PRECACHE_WEAPON_REGISTER(weapon_custom##num);

#define CustomWeaponNamedAdd( customname )										\
class CWeaponCustomNamed##customname : public CWeaponCustom						\
{																	\
	DECLARE_DATADESC();												\
	public:															\
	DECLARE_CLASS( CWeaponCustomNamed##customname, CWeaponCustom );				\
	CWeaponCustomNamed##customname() {};										\
	DECLARE_SERVERCLASS();											\
};																	\
IMPLEMENT_SERVERCLASS_ST(CWeaponCustomNamed##customname, DT_WeaponCustomNamed##customname)	\
END_SEND_TABLE()													\
BEGIN_DATADESC( CWeaponCustomNamed##customname )										\
END_DATADESC()														\
LINK_ENTITY_TO_CLASS( weapon_##customname, CWeaponCustomNamed##customname );		\
PRECACHE_WEAPON_REGISTER(weapon_##customname);
#endif	//WEAPONCUSTOM_H