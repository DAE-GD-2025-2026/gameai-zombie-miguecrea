#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include"InterestPoint.h"
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





UENUM(BlueprintType)
enum class EReasonToMove : uint8
{
	VisitHouse,
	Loot,
	Explore
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
	

	UFUNCTION()
	void HandleArrived(EPathFollowingResult::Type WhatHappened);
	void GoToPatrolPoint();

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	FVector WorldCenter = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	int32 MaxRings = 10;

	UPROPERTY(EditDefaultsOnly, Category="Wander")
	float RadiusStep = 800.f;

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
	float ScanDuration = 2.5f;

	// Maximum yaw offset from the "facing-away" base rotation, in degrees.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanSweepHalfAngleDeg = 60.f;

	// Angular frequency of the sin sweep, in radians/second. With 3.14
	// (≈π) the survivor completes one full L→R→L→ cycle per ScanDuration=2s.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanSweepFrequency = 3.14f;

	// RInterpTo speed used during the AlignToItem / ScanAlign phases.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float RotationInterpSpeed = 6.f;

	// We consider the survivor "aligned" once yaw error is below this.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float AlignToleranceDeg = 5.f;

	// How long to stand at the item after aligning, before transitioning out.
	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float LootDuration = 0.5f;

private:
	ELootPhase m_Phase           = ELootPhase::AlignToItem;
	FVector    m_ItemLocation    = FVector::ZeroVector;
	FRotator   m_DesiredRotation = FRotator::ZeroRotator;
	FRotator   m_ScanBaseRotation= FRotator::ZeroRotator;
	float      m_ScanElapsed     = 0.f;
	float      m_LootTimer       = 0.f;

	// Loaded once in OnInit from the plugin Content folder. UPROPERTY so the
	// GC doesn't collect it out from under us between Loot entries.
	UPROPERTY()
	TObjectPtr<class USoundBase> m_LootSound = nullptr;

	// Smooth yaw interpolation toward Target. Keeps pitch/roll at zero.
	void TickAlignToward(float Dt, AActor* Owner, const FRotator& Target) const;
	// True when actor yaw is within AlignToleranceDeg of Target's yaw.
	bool IsAlignedTo(AActor* Owner, const FRotator& Target) const;
	bool TryGrabItem(class ABaseItem * Item);

	
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

	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor* Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor* Owner) override;
	virtual void OnExit_Implementation(AActor* Owner) override;

	bool HasAnyWeapon = false;
	const float m_TimeUntilItIsSafe = 5.f;
	float m_Timer = 0.f;

	// --- Tuning -----------------------------------------------------------

	// How often the zigzag side flips, which is also when we re-issue the
	// retreat MoveTo. Lower = more frantic, higher = lazier.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float ZigzagFlipInterval = 0.6f;

	// How far back to retreat per flip (Unreal units).
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float RetreatDistance = 800.f;

	// Lateral offset of the zigzag (Unreal units).
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float ZigzagAmount = 400.f;

	// Seconds between weapon uses.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float FireCooldown = 1.0f;

	// RInterp speed for the face-target rotation. Higher = snappier.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float FaceTargetSpeed = 12.f;

private:
	float m_ZigzagTimer = 0.f;
	float m_FireTimer   = 0.f;
	int32 m_ZigzagSide  = 1; // +1 / -1

	TWeakObjectPtr<class ABaseZombie> m_CurrentTarget;

	// Closest of the visible zombies. Returns nullptr if list is empty.
	class ABaseZombie* PickClosestZombie(
		const TArray<class ABaseZombie*>& Zombies, AActor* Owner) const;

	// Inventory slot containing a weapon, preferring Shotgun. -1 if none.
	int32 FindWeaponSlot() const;

	// Smooth manual yaw rotation toward Threat. Pitch/roll forced to zero.
	void FaceTarget(float Dt, AActor* Owner, AActor* Threat) const;

	// Recompute (away-from-threat + lateral) destination and re-issue Move.
	void IssueRetreatMove(AActor* Owner, AActor* Threat);

	// Mirrors AWeapon::Shoot's line trace from the plugin side so we can
	// actually see the trace. Persists 1 second. Doesn't damage anything —
	// the host's UseItem still does the real shoot/damage.
	void DrawWeaponTrace(AActor* Owner, class ABaseItem* WeaponItem) const;
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

