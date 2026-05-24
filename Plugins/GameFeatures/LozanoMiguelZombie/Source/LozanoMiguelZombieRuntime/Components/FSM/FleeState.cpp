#include "FleeState.h"

#include "FSMComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "../Movement/SteeringComponent.h"
#include "../BlackBoard/BBKeys.h"

#include "EngineUtils.h"  // TActorIterator
#include "PurgeZones/PurgeZone.h"


UFleeState::UFleeState()
	: UStateBase()
{
}

void UFleeState::OnInit()
{
	Steering = GetSibling<USteeringComponent>();
}

void UFleeState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("UFlee State OnEnter"));


	if (Steering.IsValid()) Steering->StopMoving();
	
	if (Steering.IsValid()) Steering->SetRotate(false);

	m_ReplanTimer = ReplanInterval; 
	
	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone, false);
	}
}

void UFleeState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner || !Steering.IsValid()) return;

	m_ReplanTimer += DeltaTime;
	if (m_ReplanTimer < ReplanInterval) return;
	m_ReplanTimer = 0.f;

	APurgeZone* Zone = FindClosestDangerousZone(Owner);
	if (!Zone)
	{
		return;
	}

	const FVector MyLoc   = Owner->GetActorLocation();
	const FVector ZoneLoc = Zone->GetActorLocation();

	FVector AwayDir = (MyLoc - ZoneLoc).GetSafeNormal2D();
	if (AwayDir.IsNearlyZero())
	{
		// Standing exactly on the zone center — pick any direction.
		AwayDir = Owner->GetActorForwardVector();
		AwayDir.Z = 0.f;
		AwayDir.Normalize();
	}

	const FVector Destination = MyLoc + AwayDir * FleeDistance;
	Steering->Move(Destination);
}

void UFleeState::OnExit_Implementation(AActor* Owner)
{
	m_ReplanTimer = 0.f;
}

APurgeZone* UFleeState::FindClosestDangerousZone(AActor* Owner) const
{
	UWorld* W = Owner ? Owner->GetWorld() : nullptr;
	if (!W) return nullptr;

	const FVector MyLoc = Owner->GetActorLocation();

	APurgeZone* Closest      = nullptr;
	float       ClosestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<APurgeZone> It(W); It; ++It)
	{
		APurgeZone* Z = *It;
		if (!IsValid(Z)) continue;

		// Diameter is protected on APurgeZone — read via reflection so we
		// don't depend on a host accessor.
		float Diameter = 0.f;
		if (const FFloatProperty* DiamProp =
			FindFProperty<FFloatProperty>(Z->GetClass(), TEXT("Diameter")))
		{
			Diameter = DiamProp->GetPropertyValue_InContainer(Z);
		}

		const float DangerRadius = Diameter * 0.5f + PurgeZoneBuffer;
		const float DistSq       = FVector::DistSquared(MyLoc, Z->GetActorLocation());

		if (DistSq <= DangerRadius * DangerRadius && DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Z;
		}
	}

	return Closest;
}
