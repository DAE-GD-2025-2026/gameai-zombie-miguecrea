#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "InterestPoint.h"
#include "ReasonToMove.h"
#include "WanderState.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}

class UStudentPerceptor;
class USteeringComponent;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UWanderState : public UStateBase
{
	GENERATED_BODY()
public:
	UWanderState();

protected:
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	void VisualizeWanderPoints();
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;


	EReasonToMove m_ReasonToMove;

	// Weak ref to the AActor we picked as our current interest target.
	// We store the actor (which lives in its own memory) rather than a
	// FInterestPoint* (which points INTO the perceptor's TArray and would
	// dangle the moment that array reallocates on AddUnique).
	// IsValid() returns false if the actor is destroyed mid-chase.
	TWeakObjectPtr<AActor> m_BestInterestActor;

	bool m_GoingToPatrolPoint = false;


	UFUNCTION()
	void HandleArrived(EPathFollowingResult::Type WhatHappened);
	void GoToPatrolPoint();

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	FVector WorldCenter = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	int32 MaxRings = 6;

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	float RadiusStep = 1100.f;

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	float ArrivalDistance = 150.f;

	// Re-evaluate the chosen target every N seconds while wandering.
	UPROPERTY(EditDefaultsOnly, Category="Wander")
	float RepickInterval = 3.f;

	// Name of the blackboard Vector key Wander writes its destination to.
	UPROPERTY(EditDefaultsOnly, Category="Wander")
	FName DestinationKey;

private:
	struct FPatrolPoint
	{
		FVector Location = FVector::ZeroVector;
		bool    bVisited = false;
	};

	TArray<FPatrolPoint> PatrolPoints;
	int32  CurrentPatrolIdx = 0;
	bool   bPatrolReversed  = false;
	float  RepickTimer      = 0.f;
	float  ChangeMindTime   = 0.25f;
	FVector CurrentDestination = FVector::ZeroVector;

	TWeakObjectPtr<UStudentPerceptor>  Memory;
	TWeakObjectPtr<USteeringComponent> Steering;

	void BuildPatrolGrid();
	void PickNewTarget(AActor* Owner);
	void AdvancePatrol();
	void WriteDestinationToBlackboard(const FVector& Destination) const;
};
