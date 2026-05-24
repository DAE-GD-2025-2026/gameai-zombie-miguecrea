#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "InterestPoint.h"
#include "ReasonToMove.h"
#include "StateBase.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}

class UFSMComponent;
class USteeringComponent;
class UStudentPerceptor;

UCLASS(Abstract, Blueprintable, EditInlineNew)
class LOZANOMIGUELZOMBIERUNTIME_API UStateBase : public UObject
{
	GENERATED_BODY()
public:
	UStateBase();
	
	void Init(UFSMComponent * InFSM)
	{
		FSM = InFSM;
		OnInit();
	}
	virtual void OnInit() {}

	FName GetStateName() const { return m_StateName; }

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnEnter(AActor* Owner);
	virtual void OnEnter_Implementation(AActor * Owner) {}

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnTick(float DeltaTime, AActor* Owner);
	virtual void OnTick_Implementation(float DeltaTime, AActor* Owner) {}

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnExit(AActor* Owner);
	virtual void OnExit_Implementation(AActor * Owner) {}

protected:
	FName m_StateName;

	// Back-reference to the FSM that registered this state. Weak so we don't
	// extend its lifetime — the FSM owns us, not the other way around.
	TWeakObjectPtr<UFSMComponent> FSM;

	// Defined in StateBase.cpp; needs the full UFSMComponent definition.
	AActor * GetOwnerActor() const;

	// Convenience: find a sibling component on the owning actor. Wrap in a
	// weak ptr in your state if you intend to keep the reference around.
	template<class T>
	T * GetSibling() const
	{
		AActor * O = GetOwnerActor();
		return O ? O->FindComponentByClass<T>() : nullptr;
	}
};






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
	FInterestPoint * m_BestInterest;
	
	FInterestPoint * m_previousInterest;
	bool m_GoingToPatrolPoint = false;
	
	
	bool m_FirstFrame = true;
	

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
	// Default comes from BBKeys::CurrentDestination so it stays in sync with
	// the central registry. Override only if a state needs a custom key.
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
	float  ChangeMindTime      = 0.25f;
	FVector CurrentDestination = FVector::ZeroVector;

	TWeakObjectPtr<UStudentPerceptor>  Memory;
	TWeakObjectPtr<USteeringComponent> Steering;

	void BuildPatrolGrid();
	void PickNewTarget(AActor* Owner);
	void AdvancePatrol();
	void WriteDestinationToBlackboard(const FVector& Destination) const;
};


UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API ULootState : public UStateBase
{
	GENERATED_BODY()
public:
	ULootState();
protected:
	// Mini FSM inside the Loot state. Two flows:
	//   1) 2/3 of entries: AlignToItem -> Looting
	//   2) 1/3 of entries: ScanAlign -> Scanning -> AlignToItem -> Looting
	enum class ELootPhase : uint8
	{
		ScanAlign,   // rotating to face AWAY from the item
		Scanning,    // sweeping yaw left/right while facing away (paranoia check)
		AlignToItem, // rotating to face the item before grabbing
		Looting      // looking at item, loot timer ticking down
	};

	TWeakObjectPtr<USteeringComponent>        Steering;
	TWeakObjectPtr<class UInventoryComponent> Inventory;
	TWeakObjectPtr<class UStudentPerceptor> Memory;

	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;

	void ResumeWandering();

	// --- Tuning -------------------------------------------------------------

	// Probability of doing a paranoia scan instead of looting immediately.
	UPROPERTY(EditDefaultsOnly, Category="Loot", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ScanProbability = 0.5f;

	// How long the back-facing yaw sweep lasts (seconds).
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanDuration = 2.f;

	// Maximum yaw offset from the "facing-away" base rotation, in degrees.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanSweepHalfAngleDeg = 80.f;

	// Angular frequency of the sin sweep, in radians/second. With 3.14
	// (≈π) the survivor completes one full L→R→L→ cycle per ScanDuration=2s.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanSweepFrequency = 3.14f;

	// RInterpTo speed used during the AlignToItem / ScanAlign phases.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float RotationInterpSpeed = 10.f;

	// We consider the survivor "aligned" once yaw error is below this.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float AlignToleranceDeg = 5.f;

	// How long to stand at the item after aligning, before transitioning out.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float LootDuration = 0.3f;

private:
	ELootPhase m_Phase           = ELootPhase::AlignToItem;
	FVector    m_ItemLocation    = FVector::ZeroVector;
	FRotator   m_DesiredRotation = FRotator::ZeroRotator;
	FRotator   m_ScanBaseRotation= FRotator::ZeroRotator;
	float      m_ScanElapsed     = 0.f;
	float      m_LootTimer       = 0.f;
	std::optional<int> m_TargetInventoryIndex;

	// Loaded once in OnInit from the plugin Content folder. UPROPERTY so the
	// GC doesn't collect it out from under us between Loot entries.
	UPROPERTY()
	TObjectPtr<class USoundBase> m_LootSound = nullptr;

	// Smooth yaw interpolation toward Target. Keeps pitch/roll at zero.
	void TickAlignToward(float Dt, AActor* Owner, const FRotator& Target) const;
	// True when actor yaw is within AlignToleranceDeg of Target's yaw.
	bool IsAlignedTo(AActor* Owner, const FRotator& Target) const;
	bool WillIGrabThisItem(class ABaseItem * Item);
	 void GrabItem(int32 Slot);
};


UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UCombatState : public UStateBase
{
	GENERATED_BODY()

public:
	UCombatState();

protected:
	TWeakObjectPtr<USteeringComponent>          SteeringComponent;
	TWeakObjectPtr<UStudentPerceptor>           Memory;
	TWeakObjectPtr<class UInventoryComponent>   Inventory;

	// Weak ref to the actual weapon actor that Memory thinks is closest.
	// TWeakObjectPtr works because ABaseItem is a UObject; FInterestPoint
	// is a USTRUCT and can't be tracked weakly. IsValid() catches both
	// "never set" and "the item got destroyed mid-chase."
	TWeakObjectPtr<class ABaseItem> m_ClosestWeaponInMemory;
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor* Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor* Owner) override;
	virtual void OnExit_Implementation(AActor* Owner) override;

	bool HasAnyWeapon = false;
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
	
	// Seconds between weapon uses.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float FireCooldown = 1.0f;

	// RInterp speed for the face-target rotation. Higher = snappier.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float FaceTargetSpeed = 40.f;

private:
	float m_FireTimer   = 0.f;

	TWeakObjectPtr<class ABaseZombie> m_CurrentTarget;

	class ABaseZombie* PickClosestZombie(
		const TArray<class ABaseZombie*>& Zombies, AActor* Owner) const;

	TArray<int32>FindWeaponSlots() const;

	bool FaceTarget(float Dt, AActor* Owner, AActor* Threat) const;
	void DrawWeaponTrace(AActor* Owner, class ABaseItem* WeaponItem) const;
	void PlayWeaponSound(AActor* Owner, class ABaseItem* WeaponItem) const;
	
	UPROPERTY()
	TObjectPtr<class USoundBase> m_PistolSound  = nullptr;

	UPROPERTY()
	TObjectPtr<class USoundBase> m_ShotgunSound = nullptr;
	
	UPROPERTY()
	TObjectPtr<class USoundBase> m_PanicSound = nullptr;


	UPROPERTY()
	TObjectPtr<class USoundBase> m_EmptyGunSound = nullptr;
};

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
};


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
	// Loaded once in OnInit from the plugin Content folder. UPROPERTY so GC
	// doesn't sweep the asset between Explode calls.
	UPROPERTY()
	TObjectPtr<class USoundBase> m_ExplosionSound = nullptr;

	// Ticking bomb SFX — plays on OnEnter. If the asset is shorter than the
	// 3-second countdown, it'll just stop early; the explosion sound takes
	// over visually.
	UPROPERTY()
	TObjectPtr<class USoundBase> m_TickBombSound = nullptr;
	
	TWeakObjectPtr<USteeringComponent> SteeringComponent;
	
};

