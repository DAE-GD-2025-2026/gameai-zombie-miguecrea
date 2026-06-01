#include "WanderStateLozanoMiguel.h"

#include "FSMComponentLozanoMiguel.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "../Movement/SteeringComponentLozanoMiguel.h"
#include "../BlackBoard/BBKeysLozanoMiguel.h"
#include "../MACROS/DebugMacro.h"
#include "../StudentPerceptor/StudentPerceptorLozanoMiguel.h"
#include "Village/House/House.h"


UWanderStateLozanoMiguel::UWanderStateLozanoMiguel()
	: UStateBaseLozanoMiguel()
{
	DestinationKey = BBKeysLozanoMiguel::CurrentDestination;
}

void UWanderStateLozanoMiguel::OnInit()
{
	Memory   = GetSibling<UStudentPerceptorLozanoMiguel>();
	Steering = GetSibling<USteeringComponentLozanoMiguel>();
	BuildPatrolGrid();
}

void UWanderStateLozanoMiguel::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT(" Wander State Entered"))


	FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bThreatGone,   false);
	FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bThreatNearby, false);
	FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bLootDone,     false);

	Steering->SetRotate(true);
	Steering->OnMoveCompleted.AddDynamic(this, &UWanderStateLozanoMiguel::HandleArrived);
	PickNewTarget(Owner);
}

void UWanderStateLozanoMiguel::VisualizeWanderPoints()
{
	for (int32 RingNumber = 1; RingNumber <= MaxRings; ++RingNumber)
	{
		if (RingNumber == 2  || RingNumber == 3) continue;
		const float Radius = RadiusStep * RingNumber;
		DRAW_CIRCLE(GetWorld(), FVector{}, Radius, FColor::Blue, 3.f);
	}
	for (const auto & Point : PatrolPoints)
	{
		const FColor DrawColor = Point.bVisited ? FColor::Green : FColor::Red;
		DRAW_CIRCLE(GetWorld(), Point.Location, 40.f, DrawColor, 3.f);
	}
}

void UWanderStateLozanoMiguel::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner) return;

	VisualizeWanderPoints();

	RepickTimer += DeltaTime;
	if (RepickTimer >= ChangeMindTime)
	{
		RepickTimer = 0;
		PickNewTarget(Owner);
	}
}

void UWanderStateLozanoMiguel::OnExit_Implementation(AActor* Owner)
{
	m_GoingToPatrolPoint = false;
	Steering->SetRotate(false);
	Steering->OnMoveCompleted.RemoveDynamic(this, &UWanderStateLozanoMiguel::HandleArrived);
}


void UWanderStateLozanoMiguel::HandleArrived(EPathFollowingResult::Type WhatHappened)
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

	AActor* InterestActor = m_BestInterestActor.Get();

	switch (m_ReasonToMove)
	{
	case EReasonToMoveLozanoMiguel::Loot:
		UE_LOG(LogTemp, Warning, TEXT(" Arrived to Loot "));
		FSM->Blackboard.Get()->SetValueAsBool(BBKeysLozanoMiguel::bArrivedAtInterestPoint, true);
		if (InterestActor)
		{
			FSM->Blackboard.Get()->SetValueAsObject(BBKeysLozanoMiguel::bItem, InterestActor);
		}
		break;

	case EReasonToMoveLozanoMiguel::Explore:
		UE_LOG(LogTemp, Warning, TEXT("Explored"));
		AdvancePatrol();
		m_GoingToPatrolPoint = false;
		break;

	case EReasonToMoveLozanoMiguel::VisitHouse:
		UE_LOG(LogTemp, Warning, TEXT("Arrived To House "));
		break;
	default:
		break;
	}

	if (InterestActor && Memory.IsValid())
	{
		Memory->MarkVisited(InterestActor);
	}
}


// ---- Target picking ---------------------------------------------------------

void UWanderStateLozanoMiguel::GoToPatrolPoint()
{
	if (PatrolPoints.IsValidIndex(CurrentPatrolIdx))
	{
		CurrentDestination = PatrolPoints[CurrentPatrolIdx].Location;
		if (Steering.IsValid())
		{
			Steering->Move(CurrentDestination);
			m_ReasonToMove = EReasonToMoveLozanoMiguel::Explore;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Wander] Steering ref is null — cannot Move."));
		}
	}
}

void UWanderStateLozanoMiguel::PickNewTarget(AActor* Owner)
{
	if (!Owner) return;
	if (!Memory.Get()) return;

	m_BestInterestActor.Reset();
	FInterestPointLozanoMiguel* IP = Memory->GetBestInterestPoint();

	if (IP)
	{
		AActor* InterestActor = IP->Actor.Get();
		m_BestInterestActor = InterestActor;

		m_GoingToPatrolPoint = false;
		if (Cast<AHouse>(InterestActor))
		{
			m_ReasonToMove = EReasonToMoveLozanoMiguel::VisitHouse;
		}
		else
		{
			m_ReasonToMove = EReasonToMoveLozanoMiguel::Loot;
		}
		if (InterestActor)
		{
			Steering->Move(InterestActor->GetActorLocation());
		}
	}
	else
	{
		if (!m_GoingToPatrolPoint) GoToPatrolPoint();
		m_GoingToPatrolPoint = true;
	}
}

void UWanderStateLozanoMiguel::AdvancePatrol()
{
	if (!PatrolPoints.IsValidIndex(CurrentPatrolIdx)) return;

	PatrolPoints[CurrentPatrolIdx].bVisited = true;

	if (CurrentPatrolIdx == PatrolPoints.Num() - 1)
	{
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

void UWanderStateLozanoMiguel::WriteDestinationToBlackboard(const FVector& Destination) const
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

void UWanderStateLozanoMiguel::BuildPatrolGrid()
{
	PatrolPoints.Reset();

	constexpr float DegToRad = PI / 180.f;
	int32 PointsThisRing = 4;

	for (int32 Ring = 1; Ring <= MaxRings; ++Ring)
	{
		if (Ring == 2 || Ring == 3) continue;
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
