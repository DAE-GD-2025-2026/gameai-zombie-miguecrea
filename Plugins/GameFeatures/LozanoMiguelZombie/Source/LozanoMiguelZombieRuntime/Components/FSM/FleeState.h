#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "FleeState.generated.h"

class USteeringComponent;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UFleeState : public UStateBase
{
	GENERATED_BODY()
public:
	UFleeState();
protected:
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;

	// How often the flee target gets re-issued. MoveTo every frame would
	// stutter the path; this paces the updates.
	UPROPERTY(EditDefaultsOnly, Category="Flee")
	float ReplanInterval = 0.4f;

	// How far away from the danger zone center we ask Steering to run.
	UPROPERTY(EditDefaultsOnly, Category="Flee")
	float FleeDistance = 1500.f;

	// Extra slack added to each zone's radius before we consider it
	// dangerous — matches the perceptor's PurgeZoneBuffer in intent.
	UPROPERTY(EditDefaultsOnly, Category="Flee")
	float PurgeZoneBuffer = 200.f;

private:
	TWeakObjectPtr<USteeringComponent> Steering;
	float m_ReplanTimer = 0.f;

	// Find the closest dangerous APurgeZone (within its danger radius).
	// Returns nullptr if none.
	class APurgeZone* FindClosestDangerousZone(AActor* Owner) const;
};
