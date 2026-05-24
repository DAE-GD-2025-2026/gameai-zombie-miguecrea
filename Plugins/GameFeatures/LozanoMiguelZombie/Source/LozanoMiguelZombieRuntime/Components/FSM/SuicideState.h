#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "SuicideState.generated.h"

class USteeringComponent;
class USoundBase;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API USuicideState : public UStateBase
{
	GENERATED_BODY()
public:
	USuicideState();
protected:
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;


	FVector LocationToExplode{};
	void Explode();
	void KillPlayer();
	float m_Radius = 300.f;
	float m_ExplosionRadius = 0.f;

	bool m_UpdateRadius = true;

	float Timer{};

	UPROPERTY()
	TObjectPtr<USoundBase> m_ExplosionSound = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> m_TickBombSound = nullptr;

	TWeakObjectPtr<USteeringComponent> SteeringComponent;
};
