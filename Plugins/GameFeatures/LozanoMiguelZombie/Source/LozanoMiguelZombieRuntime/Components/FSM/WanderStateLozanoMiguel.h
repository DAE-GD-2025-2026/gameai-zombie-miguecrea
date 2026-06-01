#pragma once

#include "CoreMinimal.h"
#include "StateBaseLozanoMiguel.h"
#include "InterestPointLozanoMiguel.h"
#include "ReasonToMoveLozanoMiguel.h"
#include "WanderStateLozanoMiguel.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}

class UStudentPerceptorLozanoMiguel;
class USteeringComponentLozanoMiguel;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UWanderStateLozanoMiguel : public UStateBaseLozanoMiguel
{
	GENERATED_BODY()
public:
	UWanderStateLozanoMiguel();

protected:
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	void VisualizeWanderPoints();
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;


	EReasonToMoveLozanoMiguel m_ReasonToMove;

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

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	float RepickInterval = 3.f;

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

	TWeakObjectPtr<UStudentPerceptorLozanoMiguel>  Memory;
	TWeakObjectPtr<USteeringComponentLozanoMiguel> Steering;

	void BuildPatrolGrid();
	void PickNewTarget(AActor* Owner);
	void AdvancePatrol();
	void WriteDestinationToBlackboard(const FVector& Destination) const;
};
