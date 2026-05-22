#include "StateBase.h"

#include "FSMComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "../Movement/SteeringComponent.h"
#include "../BlackBoard/BBKeys.h"
#include "../MACROS/DebugMacro.h"
#include "../StudentPerceptor/StudentPerceptor.h"
#include "Common/InventoryComponent.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Zombies/BaseZombie.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"


namespace LootSlots
{
	// Weapons (Pistol / Shotgun) — heterogeneous, so swap rules matter here.
	constexpr int32 WeaponMin = 0;
	constexpr int32 WeaponMax = 1;
	// Food — homogeneous, never swap.
	
	constexpr int32 FoodMin   = 2;
	constexpr int32 FoodMax   = 3;
	// Medkit — single slot, homogeneous.
	constexpr int32 Medkit    = 4;

	constexpr int32 Total     = 5;

	// Returns inclusive [Min, Max] range or [-1, -1] for "no slot".
	static void RangeFor(EItemType T, int32& OutMin, int32& OutMax)
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

	// Score used only for weapon-vs-weapon swap decisions. Shotgun > Pistol.
	// Kept local so it can't drift from the perceptor's utility table.
	static float WeaponScore(EItemType T)
	{
		switch (T)
		{
		case EItemType::Shotgun: return 10.f;
		case EItemType::Pistol:  return 6.f;
		default:                 return 0.f;
		}
	}
}


// ============================================================================
// UStateBase
// ============================================================================

UStateBase::UStateBase()
{
	m_StateName = GetClass()->GetFName();
}

AActor * UStateBase::GetOwnerActor() const
{
	return FSM.IsValid() ? FSM->GetOwner() : nullptr;
}


// ============================================================================
// UWanderState
// ============================================================================

UWanderState::UWanderState()
	: UStateBase()
{
	DestinationKey = BBKeys::CurrentDestination;
}

void UWanderState::OnInit()
{
	
	// I COUld have Components that are needed in multiple States IN FSM 
	//otherwise theywill be local to state in case only that State uses it
	Memory   = GetSibling<UStudentPerceptor>();
	Steering = GetSibling<USteeringComponent>();
	BuildPatrolGrid();
}

void UWanderState::OnEnter_Implementation(AActor * Owner)
{
	UE_LOG(LogTemp,Warning,TEXT(" Wander State Entered"))

	// Consume any transition signals that brought us here. bThreatGone was
	// the one-shot pulse from Combat's safe-timer; clear it so it doesn't
	// keep re-firing. bThreatNearby is re-armed — the perceptor's tick poll
	// will immediately set it back to true if a zombie is genuinely visible.
	// bLootDone is consumed similarly.
	
	
	FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone,   false);
	FSM->Blackboard->SetValueAsBool(BBKeys::bThreatNearby, false);
	FSM->Blackboard->SetValueAsBool(BBKeys::bLootDone,     false);

	Steering->SetRotate(true);
	Steering->OnMoveCompleted.AddDynamic(this, &UWanderState::HandleArrived);
	PickNewTarget(Owner);
}

void UWanderState::VisualizeWanderPoints()
{
	for (int32 RingNumber = 1; RingNumber <= MaxRings; ++RingNumber)
	{
		const float Radius = RadiusStep * RingNumber;
		DRAW_CIRCLE(GetWorld(), FVector{}, Radius, FColor::Blue, 3.f);
	}
	for (const auto& Point : PatrolPoints)
	{
		const FColor DrawColor =
			Point.bVisited ? FColor::Green : FColor::Red;

		DRAW_CIRCLE(
			GetWorld(),
			Point.Location,
			40.f,
			DrawColor,
			3.f
		);
	}
}

void UWanderState::OnTick_Implementation(float DeltaTime, AActor * Owner)
{
	if (!Owner) return;

	VisualizeWanderPoints();
	
	// // Periodic re-evaluation: even mid-walk, change our mind if a better
	// // memory target appeared (e.g., a perceived item).
	 RepickTimer += DeltaTime;
	if (RepickTimer >= ChangeMindTime) //1 Second for now 
	{
		RepickTimer = 0;
		PickNewTarget(Owner);
	}

}

void UWanderState::OnExit_Implementation(AActor * Owner)
{
	m_GoingToPatrolPoint = false;
	Steering->SetRotate(false);
	Steering->OnMoveCompleted.RemoveDynamic(this, &UWanderState::HandleArrived);
}


void UWanderState::HandleArrived(EPathFollowingResult::Type WhatHappened)
{
	
	
	switch (WhatHappened)
	{
	case EPathFollowingResult::Success:
		UE_LOG(LogTemp, Warning, TEXT("Move Success"));
		break;

	case EPathFollowingResult::Blocked:
		UE_LOG(LogTemp,Error, TEXT("Move Blocked"));
		break;

	case EPathFollowingResult::OffPath:
		UE_LOG(LogTemp,Error, TEXT("Move OffPath"));
		break;

	case EPathFollowingResult::Aborted:
		UE_LOG(LogTemp, Warning, TEXT("Move Aborted"));
		break;

	case EPathFollowingResult::Invalid:
		UE_LOG(LogTemp, Error, TEXT("Move Invalid"));
		break;

	default:
		UE_LOG(LogTemp, Error, TEXT("Unknown Path Result"));
		break;
	}

	if (WhatHappened != EPathFollowingResult::Type::Success) return;
	
	switch (m_ReasonToMove)
	{
	case EReasonToMove::Loot:
		
		UE_LOG(LogTemp,Warning,TEXT(" Arrived to Loot "));
		
		FSM->Blackboard.Get()->SetValueAsBool(BBKeys::bArrivedAtInterestPoint,true);
		if (m_BestInterest)
		{
			FSM->Blackboard.Get()->SetValueAsObject(BBKeys::bItem,m_BestInterest->Actor.Get());
		}
		
		break;
	case EReasonToMove::Explore:


		UE_LOG(LogTemp, Warning, TEXT("Explored"));
		// if (m_BestInterest)
		// {
		// 	Memory->ForgetInterestPoints(*m_BestInterest);
		// }
		AdvancePatrol();
		m_GoingToPatrolPoint = false;
		break;
		
	case EReasonToMove::VisitHouse:
		UE_LOG(LogTemp,Warning,TEXT("Arrived To House "));
		break;
	default:
		break;
	}
	
	
	if (m_BestInterest)
	{
		m_BestInterest->m_Visited = true;
	}
}


// ---- Target picking ---------------------------------------------------------

 void UWanderState::GoToPatrolPoint()
{
	if (PatrolPoints.IsValidIndex(CurrentPatrolIdx))
	{
		CurrentDestination = PatrolPoints[CurrentPatrolIdx].Location;
		if (Steering.IsValid())
		{
			Steering->Move(CurrentDestination);
			m_ReasonToMove = EReasonToMove::Explore;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Wander] Steering ref is null — cannot Move."));
		}
	}
	
}

void UWanderState::PickNewTarget(AActor * Owner)
{
	if (!Owner) return;
	if (!Memory.Get()) return;
	
	//No Interest Point or all visited 
	
	m_BestInterest = Memory->GetBestInterestPoint();
	if (m_BestInterest)
	{
		m_GoingToPatrolPoint = false;
		//TODO: this can be set from Interest point
		if (Cast<AHouse>(m_BestInterest->Actor.Get()))
		{
			m_ReasonToMove = EReasonToMove::VisitHouse;
		}
		else
		{
			m_ReasonToMove = EReasonToMove::Loot;
		}
		Steering->Move(m_BestInterest->Actor.Get()->GetActorLocation());
	}
	else
	{
		if (!m_GoingToPatrolPoint) GoToPatrolPoint();  
		 	m_GoingToPatrolPoint = true;
	}
	

	
}

void UWanderState::AdvancePatrol()
{
	if (!PatrolPoints.IsValidIndex(CurrentPatrolIdx)) return;

	PatrolPoints[CurrentPatrolIdx].bVisited = true;

	if (CurrentPatrolIdx == PatrolPoints.Num() - 1)
	{
		// End of the list — flip direction and start over with a fresh sweep.
		Algo::Reverse(PatrolPoints);
		for (FPatrolPoint& P : PatrolPoints) P.bVisited = false;
		CurrentPatrolIdx = 0;
		bPatrolReversed  = !bPatrolReversed;
	}
	else
	{
		++CurrentPatrolIdx;
	}
}

void UWanderState::WriteDestinationToBlackboard(const FVector & Destination) const
{
	if (!FSM.IsValid()) return;

	APawn * Pawn = Cast<APawn>(FSM->GetOwner());
	AAIController * AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	UBlackboardComponent * BB = AI ? AI->GetBlackboardComponent() : nullptr;
	if (BB && !DestinationKey.IsNone())
	{
		BB->SetValueAsVector(DestinationKey, Destination);
	}
	else
	{
		UE_LOG(LogTemp,Error, TEXT("DestinationKey is not set"));
	}
}
void UWanderState::BuildPatrolGrid()
{
	PatrolPoints.Reset();

	constexpr float DegToRad = PI / 180.f;
	int32 PointsThisRing = 4;

	for (int32 Ring = 1; Ring <= MaxRings; ++Ring)
	{
		const float Step    = 360.f / PointsThisRing;
		const float Radius  = RadiusStep * Ring;

		for (int32 i = 0; i < PointsThisRing; ++i)
		{
			const float AngleRad = Step * i * DegToRad;
			const FVector Offset(
				FMath::Cos(AngleRad) * Radius,
				FMath::Sin(AngleRad) * Radius,
				0.f);

			FPatrolPoint Pt;
			Pt.Location = WorldCenter + Offset;
			PatrolPoints.Add(Pt);
		}

		PointsThisRing += 2 * Ring;
	}

	CurrentPatrolIdx = 0;
	bPatrolReversed  = false;
}

// ============================================================================
// ULootState
// ============================================================================


ULootState::ULootState()
	: UStateBase()
{
}

void ULootState::OnInit()
{
	Steering  = GetSibling<USteeringComponent>();
	Inventory = GetSibling<UInventoryComponent>();

	
	static const TCHAR * LootSoundPath =
		TEXT("/LozanoMiguelZombie/Sounds/freesound_community-item-equip-6904.freesound_community-item-equip-6904");

	m_LootSound = LoadObject<USoundBase>(nullptr, LootSoundPath);
	if (!m_LootSound)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Loot] Failed to load loot sound at '%s'. Check the plugin "
			     "mount point and the asset's name."), LootSoundPath);
	}
}

void ULootState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("Loot State Entered"));
	if (!Owner) return;

	// Make sure Steering's auto-spin isn't fighting our rotation.
	if (Steering.IsValid()) Steering->SetRotate(false);

	// Consume the "arrived" pulse so Wander doesn't re-enter from a stale flag.
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeys::bArrivedAtInterestPoint, false);
	}

	// Read the item we came here for.
	UObject* ItemObj = nullptr;
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		ItemObj = FSM->Blackboard->GetValueAsObject(BBKeys::bItem);
	}

	ABaseItem * Item = Cast<ABaseItem>(ItemObj);
	if (!Item)
	{
		// Nothing to loot — bail back to Wander on the next tick.
		UE_LOG(LogTemp, Error, TEXT("[Loot] No item on blackboard — resuming wander."));
		ResumeWandering();
		return;
	}
	if (!WillIGrabThisItem(Item))
	{
		//because it is very full
		UE_LOG(LogTemp,Error,TEXT("Wont Grab This Item"))
		ResumeWandering();
		return;
	}
	

	m_ItemLocation = Item->GetActorLocation();
	m_ScanElapsed  = 0.f;
	m_LootTimer    = 0.f;
	
	
	// Roll the paranoia dice. 1-in-3 by default, tunable via ScanProbability.
	const bool bWillScan = FMath::FRand() < ScanProbability;

	if (bWillScan)
	{
		// Phase 1 of the scan path: turn AWAY from the item first.
		const FVector AwayDir = (Owner->GetActorLocation() - m_ItemLocation).GetSafeNormal2D();
		m_DesiredRotation = AwayDir.IsNearlyZero() ? Owner->GetActorRotation() : AwayDir.Rotation();
		m_DesiredRotation.Pitch = 0.f;
		m_DesiredRotation.Roll  = 0.f;
		m_Phase = ELootPhase::ScanAlign;

		UE_LOG(LogTemp, Warning, TEXT("[Loot] Scanning before looting."));
	}
	else
	{
		// Direct path: just rotate to face the item.
		const FVector TowardDir = (m_ItemLocation - Owner->GetActorLocation()).GetSafeNormal2D();
		m_DesiredRotation = TowardDir.IsNearlyZero() ? Owner->GetActorRotation() : TowardDir.Rotation();
		m_DesiredRotation.Pitch = 0.f;
		m_DesiredRotation.Roll  = 0.f;
		m_Phase = ELootPhase::AlignToItem;
	}
}

void ULootState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner) return;

	switch (m_Phase)
	{
	case ELootPhase::ScanAlign:
	{
		// Rotate toward the "facing-away" target.
		TickAlignToward(DeltaTime, Owner, m_DesiredRotation);
		if (IsAlignedTo(Owner, m_DesiredRotation))
		{
			// Anchor the sweep so it oscillates around facing-away, not around
			// wherever the actor happens to be when scanning starts.
			m_ScanBaseRotation = m_DesiredRotation;
			m_ScanElapsed      = 0.f;
			m_Phase            = ELootPhase::Scanning;
		}
		break;
	}

	case ELootPhase::Scanning:
	{
		// sin sweep around the base rotation. Smooth, no jerks at the edges.
		m_ScanElapsed += DeltaTime;
		const float YawOffset =
			FMath::Sin(m_ScanElapsed * ScanSweepFrequency) * ScanSweepHalfAngleDeg;

		FRotator Sweep = m_ScanBaseRotation;
		Sweep.Yaw += YawOffset;
		Owner->SetActorRotation(Sweep);

			
		if (m_ScanElapsed >= ScanDuration)
		{
			// Done checking our six — now turn around and look at the item.
			const FVector TowardDir =
				(m_ItemLocation - Owner->GetActorLocation()).GetSafeNormal2D();
			m_DesiredRotation = TowardDir.IsNearlyZero()
				? Owner->GetActorRotation() : TowardDir.Rotation();
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
			m_Phase     = ELootPhase::Looting;
			if (m_LootSound)
			{
				//Set Item To Null becase we garbbed it 
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
			// Pull the item from the blackboard (Wander set it on arrival).
			ABaseItem* Item = nullptr;
			if (FSM.IsValid() && FSM->Blackboard.IsValid())
			{
				UObject * ItemObj = FSM->Blackboard->GetValueAsObject(BBKeys::bItem);
				Item = Cast<ABaseItem>(ItemObj);
			}
			
			if (m_TargetInventoryIndex.has_value())
			{
				GrabItem(m_TargetInventoryIndex.value());
			}
			
			
			//WillIGrabThisItem(Item);
			//
			ResumeWandering();
		}
		break;
	}
	}
}

void ULootState::OnExit_Implementation(AActor * Owner)
{
	
	m_TargetInventoryIndex.reset();
	m_ScanElapsed = 0.f;
	m_LootTimer   = 0.f;
	m_Phase       = ELootPhase::AlignToItem;
	
	
	FSM->Blackboard->SetValueAsObject(BBKeys::bItem,nullptr);
}

void ULootState::ResumeWandering()
{
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeys::bLootDone, true);
	}
}

void ULootState::TickAlignToward(float Dt, AActor* Owner, const FRotator& Target) const
{
	if (!Owner) return;
	FRotator Cur = Owner->GetActorRotation();
	FRotator New = FMath::RInterpTo(Cur, Target, Dt, RotationInterpSpeed);
	// We only care about yaw here. Keep the pawn level.
	New.Pitch = 0.f;
	New.Roll  = 0.f;
	Owner->SetActorRotation(New);
}

bool ULootState::IsAlignedTo(AActor* Owner, const FRotator& Target) const
{
	if (!Owner) return false;
	// NormalizeAxis collapses the difference to [-180, 180] so that a 359°→1°
	// step shows up as 2°, not 358°.
	const float DeltaYaw = FMath::Abs(FRotator::NormalizeAxis(Owner->GetActorRotation().Yaw - Target.Yaw));
	return DeltaYaw <= AlignToleranceDeg;
}



bool ULootState::WillIGrabThisItem(ABaseItem * Item)
{
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Loot] TryGrabItem: null item."));
		return false;
	}
	if (!Inventory.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Loot] TryGrabItem: no inventory ref."));
		return false;
	}
	
	const EItemType Type = Item->GetItemType();

	int32 RangeMin = -1, RangeMax = -1;
	LootSlots::RangeFor(Type, RangeMin, RangeMax);
	if (RangeMin < 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Loot] No slot policy for item type %d — ignoring."), (int32)Type);
		return false;
	}

	const TArray<ABaseItem*> & inventory = Inventory->GetInventory();
	
	for (int i = 0; i < Inventory->GetInventoryCapacity(); ++i)
	{
		if (inventory[i] == nullptr)
		{
			m_TargetInventoryIndex = i;
			return true; //Grab Anything in any spot 
		}
	}
	
	
	return false;


	// int32 WorstSlot  = -1;
	// float WorstScore = TNumericLimits<float>::Max();
	//
	// for (int32 i = RangeMin; i <= RangeMax; ++i)
	// {
	// 	if (!Slots[i]) continue; // shouldn't happen — we already checked above
	// 	const float ExistingScore = LootSlots::WeaponScore(Slots[i]->GetItemType());
	// 	if (ExistingScore < WorstScore)
	// 	{
	// 		WorstScore = ExistingScore;
	// 		WorstSlot  = i;
	// 	}
	//}
	//
	// if (WorstSlot < 0)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("[Loot] Couldn't identify worst weapon slot."));
	// 	return false;
	// }
	//
	// const float NewScore = LootSlots::WeaponScore(Type);
	// if (NewScore <= WorstScore)
	// {
	// 	UE_LOG(LogTemp, Warning,
	// 		TEXT("[Loot] New weapon (type=%d, score=%.1f) does not beat existing "
	// 		     "(slot=%d, score=%.1f) — skipping swap."),
	// 		(int32)Type, NewScore, WorstSlot, WorstScore);
	// 	return false;
	// }
	//
	// // --- 4) Swap: remove the worst, grab the new one into that slot.
	// const bool bRemoved = Inventory->RemoveItem(WorstSlot);
	// if (!bRemoved)
	// {
	// 	UE_LOG(LogTemp, Error,
	// 		TEXT("[Loot] RemoveItem(slot=%d) failed; abort swap."), WorstSlot);
	// 	return false;
	// }
	// const bool bGrabbed = Inventory->GrabItem(WorstSlot, Item);
	// UE_LOG(LogTemp, Warning,
	// 	TEXT("[Loot] Swapped slot %d: old score %.1f -> new score %.1f (success=%d)."),
	// 	WorstSlot, WorstScore, NewScore, bGrabbed ? 1 : 0);
	// return bGrabbed;
}

void ULootState::GrabItem(int Slot)
{
//	Is this Slot is not empty remove what is in there 
	 if (Inventory->GetInventory()[Slot] != nullptr)
	 {
	 	Inventory->RemoveItem(Slot);
	 }
	UObject * ItemObj = nullptr;
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		ItemObj = FSM->Blackboard->GetValueAsObject(BBKeys::bItem);
	}
	 ABaseItem * Item = Cast<ABaseItem>(ItemObj);
	 Inventory->GrabItem(Slot,Item);
}

UCombatState::UCombatState()
	: UStateBase()
{
}

void UCombatState::OnInit()
{
	SteeringComponent = GetSibling<USteeringComponent>();
	Memory            = GetSibling<UStudentPerceptor>();
	Inventory         = GetSibling<UInventoryComponent>();

	
	static const TCHAR* PistolPath  = TEXT("/LozanoMiguelZombie/Sounds/Pistol.Pistol");
	static const TCHAR* ShotgunPath = TEXT("/LozanoMiguelZombie/Sounds/Shotgun.Shotgun");
	

	m_PistolSound  = LoadObject<USoundBase>(nullptr, PistolPath);
	m_ShotgunSound = LoadObject<USoundBase>(nullptr, ShotgunPath);

	static const TCHAR* PanicPath    = TEXT("/LozanoMiguelZombie/Sounds/OhShitScream.OhShitScream");
	static const TCHAR* EmptyGunPath = TEXT("/LozanoMiguelZombie/Sounds/EmptyGun.EmptyGun");

	m_PanicSound    = LoadObject<USoundBase>(nullptr, PanicPath);
	m_EmptyGunSound = LoadObject<USoundBase>(nullptr, EmptyGunPath);

	if (!m_PistolSound)
		UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load pistol sound at '%s'."), PistolPath);
	if (!m_ShotgunSound)
		UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load shotgun sound at '%s'."), ShotgunPath);
	if (!m_PanicSound)
		UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load panic sound at '%s'."), PanicPath);
	if (!m_EmptyGunSound)
		UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load empty-gun sound at '%s'."), EmptyGunPath);
}

void UCombatState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("UCombat State OnEnter"));

	// Defensive: ensure the "all clear" pulse from a previous combat
	// session isn't still true. Without this, the per-state Combat->Wander
	// transition would fire on the very first tick of THIS new fight.
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone, false);
	}

	if (SteeringComponent.IsValid()) SteeringComponent->StopMoving();
	if (SteeringComponent.IsValid()) SteeringComponent->SetRotate(false);

	m_CurrentTarget.Reset();
	
}

void UCombatState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner || !Memory.IsValid()) return;

	
	//Inevbtory for now only grabbing Items when there is a free Spot and not 
	//not coming back for items whej forgotten 
	//what if it has No weapons 
	//what if I had a buch Around him before 
	//what if it runs out of ammo
	//got to the last locations etc 
	//what if it gets interruped when looting grab stuff quick 
	// if it has Zombies return to last Items in memory 
	// makw sure you dont stay looting what you wont grab 
	//bomb suicide 
	//
	//this is called everyTime Agent Arrives of Move function is called
	//if is moving to a target and then Move gets called Again inmediatly
	//Handle Arrived Fires and would set it to Abort
	
	// if it is looting needs to grab eveything quick 
	//if it has no weapons // two things can happen can go 
	//to previous in memory or explore 
	// it loots even if it wont grab it
	//it can run out of ammo 
	//still dont use Items and search for them 
	//use med kits ,food //return for items
	
	
	
	auto Zombies = Memory->GetVisibleZombies();
	if (Zombies.IsEmpty())
	{
		FTimerManager& TM = GetWorld()->GetTimerManager();
		if (!TM.IsTimerActive(m_NoMoreZombiesAroundTimer))
		{
			TM.SetTimer(
				m_NoMoreZombiesAroundTimer,
				this,
				&UCombatState::GoBackToWander,
				1.f,
				false);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(m_NoMoreZombiesAroundTimer);
	}
	// No More Zombies In Front Of Person 
	ABaseZombie  * Target = PickClosestZombie(Zombies, Owner);
	if (!Target) return;
	m_CurrentTarget = Target;
	bool IsFacingTarget  = FaceTarget(DeltaTime, Owner, Target);
	
	SteeringComponent->Move(Owner->GetActorLocation() + FVector::RightVector * 100.f);
	
	UE_LOG(LogTemp, Warning,
	TEXT("IsFacingTarget: %s | DeltaTime: %f | Target: %s"),
	IsFacingTarget ? TEXT("true") : TEXT("false"),
	DeltaTime,
	Target ? *Target->GetName() : TEXT("NULL"));
	
	if (IsFacingTarget && m_CanShoot)
	{
		const TArray<int32> WeaponSlots = FindWeaponSlots();
		if (WeaponSlots.IsEmpty())
		{
			UE_LOG(LogTemp,Warning, TEXT("[Combat] Weapons are not available."));
			return;
		}
		
		///if Many Enemies Choose Shotgun
		///
		///
		for (int32 Slot : WeaponSlots)
		{
			// CanShoot = false
			ABaseItem * WeaponItem = Inventory->GetInventory()[Slot];
			if (!WeaponItem)
			{
				UE_LOG(LogTemp,Error,TEXT("Not Valid Item"))
				continue;
			}
				bool HasValue = WeaponItem->GetValue() > 0;
			
				m_CanShoot = false;
				if (!HasValue)
				{
					
					UE_LOG(LogTemp,Error,TEXT("NO AMMO"))
					Inventory->RemoveItem(Slot);

					// Immediate dry-click on the trigger-pull frame.
					PlayEmptyWeaponSound();

					// Panic scream 0.3s later, on its own handle so it isn't
					// cancelled by the reset-shoot-flag timer below.
					GetWorld()->GetTimerManager().SetTimer(
						m_OhShitSounds,
						this,
						&UCombatState::PlayOhShitSound,
						0.3f,
						false);

					// Re-arm shooting after 0.9s.
					GetWorld()->GetTimerManager().SetTimer(
						m_ResetShootingFlag,
						this,
						&UCombatState::ResetShootFlag,
						2.f,
						false);
				}
				else
				{
					UE_LOG(LogTemp,Warning,TEXT(" SHOOT"))
					Inventory->UseItem(Slot);
					PlayWeaponSound(Owner, WeaponItem);
					DrawWeaponTrace(Owner, WeaponItem);

					// Re-arm shooting after the standard cooldown.
					GetWorld()->GetTimerManager().SetTimer(
						m_ResetShootingFlag,
						this,
						&UCombatState::ResetShootFlag,
						1.5f,
						false);
					int value  = WeaponItem->GetValue();
					
					UE_LOG(LogTemp,Warning,TEXT(" VALUE IS : %d"),value)
				}
			break;
		}
	}
}



void UCombatState::PlayOhShitSound()
{
	AActor* Owner = GetOwnerActor();
	if (!Owner || !m_PanicSound) return;

	UGameplayStatics::PlaySoundAtLocation(
		Owner, m_PanicSound, Owner->GetActorLocation());
}

void UCombatState::PlayEmptyWeaponSound()
{
	AActor* Owner = GetOwnerActor();
	if (!Owner || !m_EmptyGunSound) return;

	UGameplayStatics::PlaySoundAtLocation(
		Owner, m_EmptyGunSound, Owner->GetActorLocation());
}

void UCombatState::ResetShootFlag()
{
	m_CanShoot = true;
}

void UCombatState::GoBackToWander()
{
	FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone, true);
}

// ---- Helpers ---------------------------------------------------------------

ABaseZombie* UCombatState::PickClosestZombie(
	const TArray<ABaseZombie*>& Zombies, AActor* Owner) const
{
	if (Zombies.IsEmpty() || !Owner) return nullptr;
	const FVector MyLoc = Owner->GetActorLocation();

	ABaseZombie* Closest = nullptr;
	float ClosestDistSq  = TNumericLimits<float>::Max();
	for (ABaseZombie * Z : Zombies)
	{
		if (!Z) continue;
		const float DistSq = FVector::DistSquared(MyLoc, Z->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Z;
		}
	}
	return Closest;
}

TArray<int32> UCombatState::FindWeaponSlots() const
{
	TArray<int32> Slots;

	if (!Inventory.IsValid())
	{
		return Slots;
	}

	const TArray<ABaseItem*>& TotalNumerSlots =
		Inventory->GetInventory();

	for (int32 i = 0; i < TotalNumerSlots.Num(); ++i)
	{
		if (!TotalNumerSlots[i])
		{
			continue;
		}

		const EItemType TypeOfItem =
			TotalNumerSlots[i]->GetItemType();

		if (TypeOfItem == EItemType::Pistol ||
			TypeOfItem == EItemType::Shotgun)
		{
			Slots.Add(i);
		}
	}

	return Slots;
}

bool UCombatState::FaceTarget(float Dt, AActor * Owner, AActor * Threat) const
{
	if (!Owner || !Threat) return false;
	const FVector ToTarget =
		(Threat->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return false;;

	FRotator TargetRot = ToTarget.Rotation();
	TargetRot.Pitch = 0.f;
	TargetRot.Roll  = 0.f;

	const FRotator NewRot = FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, Dt, FaceTargetSpeed);
	Owner->SetActorRotation(NewRot);
	
	const float DeltaYaw = FMath::Abs(FRotator::NormalizeAxis(Owner->GetActorRotation().Yaw - TargetRot.Yaw));
	
	return DeltaYaw <= 1.0f;
	
}

void UCombatState::OnExit_Implementation(AActor* Owner)
{
	m_CurrentTarget.Reset();
	m_Timer       = 0.f;
	m_ZigzagTimer = 0.f;
	m_FireTimer   = 0.f;

}
void UCombatState::IssueRetreatMove(AActor* Owner, AActor* Threat)
{
	if (!Owner || !Threat || !SteeringComponent.IsValid()) return;

	const FVector MyLoc      = Owner->GetActorLocation();
	const FVector ThreatLoc  = Threat->GetActorLocation();

	FVector AwayDir = (MyLoc - ThreatLoc).GetSafeNormal2D();
	if (AwayDir.IsNearlyZero())
	{
		// Stacked on top of the zombie — fall back to current backward.
		AwayDir = -Owner->GetActorForwardVector();
		AwayDir.Z = 0.f;
		AwayDir.Normalize();
	}

	// Perpendicular in world XY = cross with UpVector. Flip sign per zigzag.
	const FVector LateralDir =
		FVector::CrossProduct(AwayDir, FVector::UpVector).GetSafeNormal();

	const FVector Destination =
		MyLoc + AwayDir * RetreatDistance + LateralDir * (ZigzagAmount * m_ZigzagSide);

	SteeringComponent->Move(Destination);
}

void UCombatState::DrawWeaponTrace(AActor* Owner, ABaseItem* WeaponItem) const
{
	if (!Owner || !WeaponItem) return;
	UWorld* W = Owner->GetWorld();
	if (!W) return;

	// Mirrors AWeapon::Shoot: trace from Survivor's location along Direction
	// for 10000uu against ECC_Pawn, ignoring the survivor itself.
	const FVector Start   = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	// Helper lambda so Pistol and Shotgun share the same trace+draw step.
	// Hit lines turn red; misses are yellow. 1-second persist as requested.
	auto TraceAndDraw = [&](const FVector& Direction)
	{
		const FVector End = Start + Direction * 10000.f;

		FHitResult Hit;
		const bool bHit = W->LineTraceSingleByChannel(
			Hit, Start, End, ECC_Pawn, Params);

		const FVector LineEnd  = bHit ? Hit.ImpactPoint : End;
		const FColor  LineCol  = bHit ? FColor::Red    : FColor::Yellow;

		DrawDebugLine(W, Start, LineEnd, LineCol,
			/*bPersistentLines*/ false,
			/*LifeTime*/         1.0f,
			/*DepthPriority*/    0,
			/*Thickness*/        2.f);

		if (bHit)
		{
			DrawDebugSphere(W, Hit.ImpactPoint, 20.f, 8, FColor::Red,
				false, 1.0f, 0, 1.f);
		}
		
	};

	switch (WeaponItem->GetItemType())
	{
	case EItemType::Pistol:
	{
		TraceAndDraw(Forward);
		break;
	}
	case EItemType::Shotgun:
	{
		constexpr int32 ShotsPerAmmo  = 3;
		constexpr float MaxSprayDelta = 10.f;
		for (int32 i = 0; i < ShotsPerAmmo; ++i)
		{
			const float YawDeg = FMath::RandRange(-MaxSprayDelta, MaxSprayDelta);
			const FVector Dir = Forward.ToOrientationRotator().Add(0.f, YawDeg, 0.f).Vector();
			TraceAndDraw(Dir);
		}
		break;
	}
	default:
		// Not a weapon — nothing to draw.
		break;
	}
}

void UCombatState::PlayWeaponSound(AActor* Owner, ABaseItem* WeaponItem) const
{
	if (!Owner || !WeaponItem) return;

	// Pick the sound to fire based on item type. Null sounds are tolerated
	// (logged in OnInit), so a load failure won't crash here.
	USoundBase* SoundToPlay = nullptr;
	switch (WeaponItem->GetItemType())
	{
	case EItemType::Pistol:  SoundToPlay = m_PistolSound;  break;
	case EItemType::Shotgun: SoundToPlay = m_ShotgunSound; break;
	default: return; // not a weapon
	}

	if (!SoundToPlay) return;

	// PlaySoundAtLocation is fire-and-forget — no audio component to manage,
	// no looping concerns. 3D-spatialized so multiple survivors firing in
	// different places sound correct.
	UGameplayStatics::PlaySoundAtLocation(
		Owner,
		SoundToPlay,
		Owner->GetActorLocation(),
		/*VolumeMultiplier*/ 1.0f,
		/*PitchMultiplier*/  1.0f);
}

///////////////////////////////






void UFleeState::OnInit()
{
	
}

UFleeState::UFleeState()
	: UStateBase()
{
	
}

void UFleeState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp,Warning, TEXT("UFlee State OnEnter "));
	
}
void UFleeState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	
	//GO ON ZIG ZAG WHILE looking at the Zombies And Shooting Them 1 by One 
	//Add A grenade 
}

void UFleeState::OnExit_Implementation(AActor * Owner)
{
	//make it rotate again 
}

