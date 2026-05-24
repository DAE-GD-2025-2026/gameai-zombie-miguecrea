#include "WanderState.h"

#include "FSMComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "../Movement/SteeringComponent.h"
#include "../BlackBoard/BBKeys.h"
#include "../MACROS/DebugMacro.h"
#include "../StudentPerceptor/StudentPerceptor.h"
#include "Village/House/House.h"


UWanderState::UWanderState()
	: UStateBase()
{
	DestinationKey = BBKeys::CurrentDestination;
}

void UWanderState::OnInit()
{

	Memory   = GetSibling<UStudentPerceptor>();
	Steering = GetSibling<USteeringComponent>();
	BuildPatrolGrid();
}

void UWanderState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT(" Wander State Entered"))


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
		const FColor DrawColor = Point.bVisited ? FColor::Green : FColor::Red;
		DRAW_CIRCLE(GetWorld(), Point.Location, 40.f, DrawColor, 3.f);
	}
}

void UWanderState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner) return;

	VisualizeWanderPoints();

	// Periodic re-evaluation: even mid-walk, change our mind if a better
	// memory target appeared (e.g., a perceived item).
	RepickTimer += DeltaTime;
	if (RepickTimer >= ChangeMindTime)
	{
		RepickTimer = 0;
		PickNewTarget(Owner);
	}
}

void UWanderState::OnExit_Implementation(AActor* Owner)
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
		UE_LOG(LogTemp, Warning, TEXT("Move Success")); break;
	case EPathFollowingResult::Blocked:
		UE_LOG(LogTemp, Error,   TEXT("Move Blocked")); break;
	case EPathFollowingResult::OffPath:
		UE_LOG(LogTemp, Error,   TEXT("Move OffPath")); break;
	case EPathFollowingResult::Aborted:
		UE_LOG(LogTemp, Warning, TEXT("Move Aborted")); break;
	case EPathFollowingResult::Invalid:
		UE_LOG(LogTemp, Error,   TEXT("Move Invalid")); break;
	default:
		UE_LOG(LogTemp, Error,   TEXT("Unknown Path Result")); break;
	}

	if (WhatHappened != EPathFollowingResult::Type::Success) return;

	switch (m_ReasonToMove)
	{
	case EReasonToMove::Loot:
		UE_LOG(LogTemp, Warning, TEXT(" Arrived to Loot "));
		FSM->Blackboard.Get()->SetValueAsBool(BBKeys::bArrivedAtInterestPoint, true);
		if (m_BestInterest)
		{
			FSM->Blackboard.Get()->SetValueAsObject(BBKeys::bItem, m_BestInterest->Actor.Get());
		}
		break;

	case EReasonToMove::Explore:
		UE_LOG(LogTemp, Warning, TEXT("Explored"));
		AdvancePatrol();
		m_GoingToPatrolPoint = false;
		break;

	case EReasonToMove::VisitHouse:
		UE_LOG(LogTemp, Warning, TEXT("Arrived To House "));
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

void UWanderState::PickNewTarget(AActor* Owner)
{
	if (!Owner) return;
	if (!Memory.Get()) return;

	m_BestInterest = Memory->GetBestInterestPoint();
	if (m_BestInterest)
	{
		m_GoingToPatrolPoint = false;
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

void UWanderState::WriteDestinationToBlackboard(const FVector& Destination) const
{
	if (!FSM.IsValid()) return;

	APawn* Pawn = Cast<APawn>(FSM->GetOwner());
	AAIController* AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	UBlackboardComponent* BB = AI ? AI->GetBlackboardComponent() : nullptr;
	if (BB && !DestinationKey.IsNone())
	{
		BB->SetValueAsVector(DestinationKey, Destination);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("DestinationKey is not set"));
	}
}

void UWanderState::BuildPatrolGrid()
{
	PatrolPoints.Reset();

	constexpr float DegToRad = PI / 180.f;
	int32 PointsThisRing = 4;

	for (int32 Ring = 1; Ring <= MaxRings; ++Ring)
	{
		const float Step   = 360.f / PointsThisRing;
		const float Radius = RadiusStep * Ring;

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
