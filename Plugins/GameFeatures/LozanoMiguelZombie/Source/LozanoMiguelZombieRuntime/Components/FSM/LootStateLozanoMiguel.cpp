#include "LootStateLozanoMiguel.h"

#include "FSMComponentLozanoMiguel.h"
#include "LootSlotsLozanoMiguel.h"
#include "InterestPointLozanoMiguel.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "../Movement/SteeringComponentLozanoMiguel.h"
#include "../BlackBoard/BBKeysLozanoMiguel.h"
#include "../StudentPerceptor/StudentPerceptorLozanoMiguel.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"


ULootStateLozanoMiguel::ULootStateLozanoMiguel()
	: UStateBaseLozanoMiguel()
{
}

void ULootStateLozanoMiguel::OnInit()
{
	Steering  = GetSibling<USteeringComponentLozanoMiguel>();
	Inventory = GetSibling<UInventoryComponent>();
	Memory    = GetSibling<UStudentPerceptorLozanoMiguel>();

	static const TCHAR* LootSoundPath =
		TEXT("/LozanoMiguelZombie/Sounds/freesound_community-item-equip-6904.freesound_community-item-equip-6904");

	m_LootSound = LoadObject<USoundBase>(nullptr, LootSoundPath);
	if (!m_LootSound)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Loot] Failed to load loot sound at '%s'. Check the plugin "
			     "mount point and the asset's name."), LootSoundPath);
	}
}

void ULootStateLozanoMiguel::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("Loot State Entered"));
	if (!Owner) return;

	if (Steering.IsValid()) Steering->SetRotate(false);

	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bArrivedAtInterestPoint, false);
	}

	UObject* ItemObj = nullptr;
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		ItemObj = FSM->Blackboard->GetValueAsObject(BBKeysLozanoMiguel::bItem);
	}

	ABaseItem* Item = Cast<ABaseItem>(ItemObj);
	if (!Item)
	{
		UE_LOG(LogTemp, Error, TEXT("[Loot] No item on blackboard — resuming wander."));
		ResumeWandering();
		return;
	}
	if (!WillIGrabThisItem(Item))
	{
		FSM->Blackboard->SetValueAsObject(BBKeysLozanoMiguel::bItem, nullptr);
		ResumeWandering();
		return;
	}

	m_ItemLocation = Item->GetActorLocation();
	m_ScanElapsed  = 0.f;
	m_LootTimer    = 0.f;

	const bool bWillScan = FMath::FRand() < ScanProbability;

	if (bWillScan)
	{
		const FVector AwayDir = (Owner->GetActorLocation() - m_ItemLocation).GetSafeNormal2D();
		m_DesiredRotation = AwayDir.IsNearlyZero() ? Owner->GetActorRotation() : AwayDir.Rotation();
		m_DesiredRotation.Pitch = 0.f;
		m_DesiredRotation.Roll  = 0.f;
		m_Phase = ELootPhase::ScanAlign;
		UE_LOG(LogTemp, Warning, TEXT("[Loot] Scanning before looting."));
	}
	else
	{
		const FVector TowardDir = (m_ItemLocation - Owner->GetActorLocation()).GetSafeNormal2D();
		m_DesiredRotation = TowardDir.IsNearlyZero() ? Owner->GetActorRotation() : TowardDir.Rotation();
		m_DesiredRotation.Pitch = 0.f;
		m_DesiredRotation.Roll  = 0.f;
		m_Phase = ELootPhase::AlignToItem;
	}
}

void ULootStateLozanoMiguel::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner) return;

	switch (m_Phase)
	{
	case ELootPhase::ScanAlign:
	{
		TickAlignToward(DeltaTime, Owner, m_DesiredRotation);
		if (IsAlignedTo(Owner, m_DesiredRotation))
		{
			m_ScanBaseRotation = m_DesiredRotation;
			m_ScanElapsed = 0.f;
			m_Phase = ELootPhase::Scanning;
		}
		break;
	}

	case ELootPhase::Scanning:
	{
		m_ScanElapsed += DeltaTime;
		const float YawOffset =
			FMath::Sin(m_ScanElapsed * ScanSweepFrequency) * ScanSweepHalfAngleDeg;

		FRotator Sweep = m_ScanBaseRotation;
		Sweep.Yaw += YawOffset;
		Owner->SetActorRotation(Sweep);

		if (m_ScanElapsed >= ScanDuration)
		{
			const FVector TowardDir =
				(m_ItemLocation - Owner->GetActorLocation()).GetSafeNormal2D();
			m_DesiredRotation = TowardDir.IsNearlyZero()
				? Owner->GetActorRotation()
				: TowardDir.Rotation();
			m_DesiredRotation.Pitch = 0.f;
			m_DesiredRotation.Roll  = 0.f;
			m_Phase = ELootPhase::AlignToItem;
		}
		break;
	}

	case ELootPhase::AlignToItem:
	{
		TickAlignToward(DeltaTime, Owner, m_DesiredRotation);
		if (IsAlignedTo(Owner, m_DesiredRotation))
		{
			m_LootTimer = 0.f;
			m_Phase = ELootPhase::Looting;
			if (m_LootSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					Owner, m_LootSound, Owner->GetActorLocation());
			}
		}
		break;
	}

	case ELootPhase::Looting:
	{
		m_LootTimer += DeltaTime;
		if (m_LootTimer >= LootDuration)
		{
			ABaseItem* Item = nullptr;
			if (FSM.IsValid() && FSM->Blackboard.IsValid())
			{
				UObject* ItemObj = FSM->Blackboard->GetValueAsObject(BBKeysLozanoMiguel::bItem);
				Item = Cast<ABaseItem>(ItemObj);
			}

			if (m_TargetInventoryIndex.has_value())
			{
				GrabItem(m_TargetInventoryIndex.value());
			}
			ResumeWandering();
		}
		break;
	}
	}
}

void ULootStateLozanoMiguel::OnExit_Implementation(AActor* Owner)
{
	m_TargetInventoryIndex.reset();
	m_ScanElapsed = 0.f;
	m_LootTimer   = 0.f;
	m_Phase       = ELootPhase::AlignToItem;

	FSM->Blackboard->SetValueAsObject(BBKeysLozanoMiguel::bItem, nullptr);
}

void ULootStateLozanoMiguel::ResumeWandering()
{
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bLootDone, true);
	}
}

void ULootStateLozanoMiguel::TickAlignToward(float Dt, AActor* Owner, const FRotator& Target) const
{
	if (!Owner) return;
	FRotator Cur = Owner->GetActorRotation();
	FRotator New = FMath::RInterpTo(Cur, Target, Dt, RotationInterpSpeed);
	New.Pitch = 0.f;
	New.Roll  = 0.f;
	Owner->SetActorRotation(New);
}

bool ULootStateLozanoMiguel::IsAlignedTo(AActor* Owner, const FRotator& Target) const
{
	if (!Owner) return false;
	const float DeltaYaw =
		FMath::Abs(FRotator::NormalizeAxis(Owner->GetActorRotation().Yaw - Target.Yaw));
	return DeltaYaw <= AlignToleranceDeg;
}


bool ULootStateLozanoMiguel::WillIGrabThisItem(ABaseItem* Item)
{
	m_TargetInventoryIndex.reset();

	if (!Item)        { UE_LOG(LogTemp, Warning, TEXT("[Loot] WillIGrab: null item."));    return false; }
	if (!Inventory.IsValid()) { UE_LOG(LogTemp, Error, TEXT("[Loot] WillIGrab: no inventory ref.")); return false; }

	const EItemType Type = Item->GetItemType();
	if (Type == EItemType::Garbage) return false;

	const TArray<ABaseItem*>& Slots = Inventory->GetInventory();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i] == nullptr)
		{
			m_TargetInventoryIndex = i;
			return true;
		}
	}

	int32 NumWeapons = 0, NumFood = 0, NumMedkit = 0;
	int32 LowestWeaponSlot  = -1;
	int32 LowestWeaponScore = TNumericLimits<int32>::Max();
	int32 AnyFoodSlot   = -1;
	int32 AnyMedkitSlot = -1;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		ABaseItem * S = Slots[i];
		if (!S) continue;
		const EItemType ST = S->GetItemType();
		switch (ST)
		{
		case EItemType::Pistol:
		case EItemType::Shotgun:
		{
			++NumWeapons;
			const int32 Score = LootSlotsLozanoMiguel::WeaponScore(S);
			if (Score < LowestWeaponScore)
			{
				LowestWeaponScore = Score;
				LowestWeaponSlot  = i;
			}
			break;
		}
		case EItemType::Food:
			++NumFood;
			if (AnyFoodSlot   < 0) AnyFoodSlot   = i;
			break;
		case EItemType::Medkit:
			++NumMedkit;
			if (AnyMedkitSlot < 0) AnyMedkitSlot = i;
			break;
		default: break;
		}
	}

	constexpr int32 IdealWeapons = 3;
	constexpr int32 IdealFood    = 1;
	constexpr int32 IdealMedkit  = 1;

	if (Type == EItemType::Pistol || Type == EItemType::Shotgun)
	{
		if (NumWeapons < IdealWeapons)
		{
			if (NumFood  > IdealFood   && AnyFoodSlot   >= 0) { m_TargetInventoryIndex = AnyFoodSlot;   return true; }
			if (NumMedkit > IdealMedkit && AnyMedkitSlot >= 0) { m_TargetInventoryIndex = AnyMedkitSlot; return true; }
			return false;
		}
		const int32 NewScore = LootSlotsLozanoMiguel::WeaponScore(Item);
		if (LowestWeaponSlot >= 0 && NewScore > LowestWeaponScore)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Loot] Weapon upgrade: slot %d (ammo %d) → new (ammo %d)."),
				LowestWeaponSlot, LowestWeaponScore, NewScore);
			m_TargetInventoryIndex = LowestWeaponSlot;
			return true;
		}
		return false;
	}

	if (Type == EItemType::Food)
	{
		if (NumFood >= IdealFood) return false;
		if (NumWeapons > IdealWeapons && LowestWeaponSlot >= 0) { m_TargetInventoryIndex = LowestWeaponSlot; return true; }
		if (NumMedkit  > IdealMedkit  && AnyMedkitSlot   >= 0) { m_TargetInventoryIndex = AnyMedkitSlot;   return true; }
		return false;
	}

	if (Type == EItemType::Medkit)
	{
		if (NumMedkit >= IdealMedkit) return false;
		if (NumWeapons > IdealWeapons && LowestWeaponSlot >= 0) { m_TargetInventoryIndex = LowestWeaponSlot; return true; }
		if (NumFood    > IdealFood    && AnyFoodSlot     >= 0) { m_TargetInventoryIndex = AnyFoodSlot;     return true; }
		return false;
	}

	return false;
}

void ULootStateLozanoMiguel::GrabItem(int Slot)
{
	UObject* ItemObj = nullptr;
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		ItemObj = FSM->Blackboard->GetValueAsObject(BBKeysLozanoMiguel::bItem);
	}
	ABaseItem* Item = Cast<ABaseItem>(ItemObj);
	if (!Item) return;

	if (Inventory->GetInventory()[Slot] != nullptr)
	{
		Inventory->RemoveItem(Slot);
	}

	Inventory->GrabItem(Slot, Item);

	FSM->Blackboard->SetValueAsObject(BBKeysLozanoMiguel::bItem, nullptr);

	if (Memory.IsValid())
	{
		FInterestPointLozanoMiguel Probe;
		Probe.Actor = Item;
		Memory->m_WannaPointsInBrain.Remove(Probe);
	}
}
