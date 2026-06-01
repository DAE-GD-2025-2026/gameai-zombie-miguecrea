#pragma once

#include "CoreMinimal.h"
#include "StateBaseLozanoMiguel.h"
#include "FleeStateLozanoMiguel.generated.h"

class USteeringComponentLozanoMiguel;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UFleeStateLozanoMiguel : public UStateBaseLozanoMiguel
{
	GENERATED_BODY()
public:
	UFleeStateLozanoMiguel();
protected:
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;

	UPROPERTY(EditDefaultsOnly, Category="Flee")
	float ReplanInterval = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category="Flee")
	float FleeDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category="Flee")
	float PurgeZoneBuffer = 200.f;

private:
	TWeakObjectPtr<USteeringComponentLozanoMiguel> Steering;
	float m_ReplanTimer = 0.f;

	class APurgeZone* FindClosestDangerousZone(AActor* Owner) const;
};
