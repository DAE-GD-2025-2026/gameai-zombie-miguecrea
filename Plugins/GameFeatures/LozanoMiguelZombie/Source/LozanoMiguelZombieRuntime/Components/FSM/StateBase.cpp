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
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"


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
	
	FSM->Blackboard->SetValueAsBool(BBKeys::bLootDone,false);
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
	Steering->SetRotate(false);
	Steering->OnMoveCompleted.RemoveDynamic(this, &UWanderState::HandleArrived);
}


void UWanderState::HandleArrived(EPathFollowingResult::Type WhatHappened)
{
	//this is called everyTime Agent Arrives of Move function is called
	//if is moving to a target and then Move gets called Again inmediatly
	//Handle Arrived Fires and would set it to Abort
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
		if (m_BestInterest)FSM->Blackboard.Get()->SetValueAsObject(BBKeys::bItem,m_BestInterest->Actor.Get());
		
		break;
	case EReasonToMove::Explore:
		UE_LOG(LogTemp,Warning,TEXT("Explored"));
		AdvancePatrol();
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
	if (!Memory->GetBestInterestPoint())
	{
		GoToPatrolPoint();
		return;
	}
	
	m_BestInterest = Memory->GetBestInterestPoint();

	if (m_BestInterest)
	{
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

	// Load the loot SFX once from the plugin Content folder.
	// Path format: /<PluginMountPoint>/<Folder>/<AssetName>.<AssetName>
	// The plugin mount point is the plugin's name — "LozanoMiguelZombie" —
	// taken from the .uplugin's FriendlyName / module name. The doubled
	// asset name at the end is the standard "PackagePath.ObjectName" form.
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

	ABaseItem* Item = Cast<ABaseItem>(ItemObj);
	if (!Item)
	{
		// Nothing to loot — bail back to Wander on the next tick.
		UE_LOG(LogTemp, Error, TEXT("[Loot] No item on blackboard — resuming wander."));
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
			// TODO: actually grab the item once Inventory wiring is decided.
			if (Inventory.IsValid())
			{
			     UObject* ItemObj = FSM->Blackboard->GetValueAsObject(BBKeys::bItem);
			     Inventory->GrabItem(0, Cast<ABaseItem>(ItemObj));
			 }
			ResumeWandering();
		}
		break;
	}
	}
}

void ULootState::OnExit_Implementation(AActor * Owner)
{
	m_ScanElapsed = 0.f;
	m_LootTimer   = 0.f;
	m_Phase       = ELootPhase::AlignToItem;
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

void UCombatState::OnInit()
{
	
}


UCombatState::UCombatState()
	: UStateBase()
{
}

void UCombatState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp,Warning, TEXT("UCombat State OnEnter "));
	
}
void UCombatState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	
	//GO ON ZIG ZAG WHILE looking at the Zombies And Shooting Them 1 by One 
	//Add A grenade 
}

void UCombatState::OnExit_Implementation(AActor * Owner)
{
	//make it rotate again 
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

