#pragma once

#include "CoreMinimal.h"
#include "Items/ItemType.h"
#include "Items/BaseItem.h"


namespace LootSlotsLozanoMiguel
{
	constexpr int32 WeaponMin = 0;
	constexpr int32 WeaponMax = 1;
	constexpr int32 FoodMin   = 2;
	constexpr int32 FoodMax   = 3;
	constexpr int32 Medkit    = 4;

	constexpr int32 Total     = 5;
	// Returns inclusive [Min, Max] range or [-1, -1] for "no slot".
	inline void RangeFor(EItemType T, int32& OutMin, int32& OutMax)
	{
		switch (T)
		{
		case EItemType::Pistol:
		case EItemType::Shotgun: OutMin = WeaponMin; OutMax = WeaponMax; return;
		case EItemType::Food:    OutMin = FoodMin;   OutMax = FoodMax;   return;
		case EItemType::Medkit:  OutMin = Medkit;    OutMax = Medkit;    return;
		default:                 OutMin = -1;        OutMax = -1;        return;
		}
	}
	// Weapon-vs-weapon swap score. Based on the item's Value field, which
	// for weapons is the current ammo count — so a fully-loaded Pistol
	// (Value=8) beats an almost-empty Shotgun (Value=1). Null-safe.
	inline int32 WeaponScore(const ABaseItem* Weapon)
	{
		return Weapon ? Weapon->GetValue() : 0;
	}
}
