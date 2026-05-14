#include "StateBase.h"

#include "FSMComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "../Movement/SteeringComponent.h"
#include "../BlackBoard/BBKeys.h"
#include "../MACROS/DebugMacro.h"
#include "../StudentPerceptor/StudentPerceptor.h"


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
	RepickTimer = RepickInterval;
	
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
	
	//
	// // Periodic re-evaluation: even mid-walk, change our mind if a better
	// // memory target appeared (e.g., a perceived item).
	// RepickTimer -= DeltaTime;
	// if (RepickTimer <= 0.f)
	// {
	// 	PickNewTarget(Owner);
	// 	RepickTimer = RepickInterval;
	// }

}

void UWanderState::OnExit_Implementation(AActor * Owner)
{
	Steering->OnMoveCompleted.RemoveDynamic(this, &UWanderState::HandleArrived);
}


void UWanderState::HandleArrived(bool bSucceeded)
{
	if (!bSucceeded) UE_LOG(LogTemp,Error,TEXT("Arrived Not Succeded"));

	switch (m_ReasonToMove)
	{
	case EReasonToMove::Loot:
		
		UE_LOG(LogTemp,Warning,TEXT("Looting"));
		
		break;
	case EReasonToMove::Explore:
		
		UE_LOG(LogTemp,Warning,TEXT("Explored"));
		
		break;
	default:
		break;
	}
	
	
	AdvancePatrol();
	PickNewTarget(GetOwnerActor());
}


// ---- Target picking ---------------------------------------------------------

void UWanderState::PickNewTarget(AActor * Owner)
{
	if (!Owner) return;
	
	
	//    if (AActor* Item = Memory->FindBestUnvisitedTarget(Needs.Get()))
	//    {
	//        CurrentDestination = Item->GetActorLocation();
	//        WriteDestinationToBlackboard(CurrentDestination);
	//        return;
	//    }


	// 2. Fallback: the next patrol point on the concentric grid.
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


void ULootState::OnInit()
{
	
}

ULootState::ULootState()
	: UStateBase()
{
}

void ULootState::OnEnter_Implementation(AActor* Owner)
{

	UE_LOG(LogTemp,Warning, TEXT("ULoot State OnEnter "));
	//Grab The Item Set it on The blackBoard 
	//wait 2 seconds while looting 
	//Stop the Rotating ?
	
}
void ULootState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	
}

void ULootState::OnExit_Implementation(AActor * Owner)
{
	//make it rotate again 
}

