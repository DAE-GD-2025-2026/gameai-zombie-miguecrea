#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "StateBase.generated.h"

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
	

	UFUNCTION()
	void HandleArrived(bool bSucceeded);

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
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;
};
