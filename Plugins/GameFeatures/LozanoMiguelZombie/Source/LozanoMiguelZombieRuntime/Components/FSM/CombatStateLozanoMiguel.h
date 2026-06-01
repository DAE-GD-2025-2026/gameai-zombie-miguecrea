#pragma once

#include "CoreMinimal.h"
#include "StateBaseLozanoMiguel.h"
#include "CombatStateLozanoMiguel.generated.h"

class USteeringComponentLozanoMiguel;
class UStudentPerceptorLozanoMiguel;
class UInventoryComponent;
class ABaseItem;
class ABaseZombie;
class USoundBase;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UCombatStateLozanoMiguel : public UStateBaseLozanoMiguel
{
	GENERATED_BODY()

public:
	UCombatStateLozanoMiguel();

protected:
	TWeakObjectPtr<USteeringComponentLozanoMiguel> SteeringComponent;
	TWeakObjectPtr<UStudentPerceptorLozanoMiguel>  Memory;
	TWeakObjectPtr<UInventoryComponent>            Inventory;

	TWeakObjectPtr<ABaseItem>            m_ClosestWeaponInMemory;

	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor* Owner) override;
	bool HandleShooting(AActor* Owner, ABaseZombie* Target, bool IsFacingTarget);
	virtual void OnTick_Implementation(float DeltaTime, AActor* Owner) override;
	virtual void OnExit_Implementation(AActor* Owner) override;

	bool  HasAnyWeapon = false;
	const float m_TimeUntilItIsSafe = 3.f;
	float m_Timer = 0.f;

	bool m_CanShoot = true;

	FTimerHandle m_NoMoreZombiesAroundTimer;
	FTimerHandle m_OhShitSounds;
	FTimerHandle m_EmptyWeapondSound;
	FTimerHandle m_ResetShootingFlag;

	void PlayOhShitSound();
	void PlayEmptyWeaponSound();
	void ResetShootFlag();
	void GoBackToWander();

	bool m_OnDangerSearchingOldWeapon = false;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float FireCooldown = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float FaceTargetSpeed = 40.f;

private:
	float m_FireTimer = 0.f;

	TWeakObjectPtr<ABaseZombie> m_CurrentTarget;

	ABaseZombie* PickClosestZombie(const TArray<ABaseZombie*>& Zombies, AActor* Owner) const;
	TArray<int32> FindWeaponSlots() const;
	bool FaceTarget(float Dt, AActor* Owner, AActor* Threat) const;
	void DrawWeaponTrace(AActor* Owner, ABaseItem* WeaponItem) const;
	void PlayWeaponSound(AActor* Owner, ABaseItem* WeaponItem) const;

	UPROPERTY()
	TObjectPtr<USoundBase> m_PistolSound  = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> m_ShotgunSound = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> m_PanicSound = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> m_EmptyGunSound = nullptr;
};
