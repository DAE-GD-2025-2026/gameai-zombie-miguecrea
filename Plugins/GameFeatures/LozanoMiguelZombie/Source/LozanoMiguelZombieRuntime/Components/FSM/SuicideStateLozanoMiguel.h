#pragma once

#include "CoreMinimal.h"
#include "StateBaseLozanoMiguel.h"
#include "SuicideStateLozanoMiguel.generated.h"

class USteeringComponentLozanoMiguel;
class USoundBase;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API USuicideStateLozanoMiguel : public UStateBaseLozanoMiguel
{
	GENERATED_BODY()
public:
	USuicideStateLozanoMiguel();
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

	TWeakObjectPtr<USteeringComponentLozanoMiguel> SteeringComponent;
};
