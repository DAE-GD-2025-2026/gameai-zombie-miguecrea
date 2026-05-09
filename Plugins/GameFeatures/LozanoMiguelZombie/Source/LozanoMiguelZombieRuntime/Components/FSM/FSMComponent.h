#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StateBase.h"
#include "FSMComponent.generated.h"

// Plain C++ struct — TFunction cannot be a UPROPERTY
struct FStateTransition
{
	FGameplayTag From;
	FGameplayTag To;
	TFunction<bool(AActor*)> Condition;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UFSMComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFSMComponent();

	// Assign states directly in the Blueprint details panel
	UPROPERTY(EditAnywhere, Instanced, Category="FSM")
	TMap<FGameplayTag, TObjectPtr<UStateBase>> States;

	UPROPERTY(EditAnywhere, Category="FSM")
	FGameplayTag InitialState;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// Register a state from C++ (alternative to the Blueprint Instanced map)
	void RegisterState(FGameplayTag Tag, UStateBase* State);

	// Wire a transition — the only thing callers need to set up
	void AddTransition(FGameplayTag From, FGameplayTag To, TFunction<bool(AActor*)> Condition);

	// Skip transition evaluation and jump directly to a state
	void ForceTransition(FGameplayTag To);

	UFUNCTION(BlueprintPure, Category="FSM")
	FGameplayTag GetCurrentStateTag() const { return CurrentStateTag; }

	UFUNCTION(BlueprintPure, Category="FSM")
	bool IsInState(FGameplayTag Tag) const { return CurrentStateTag == Tag; }

private:
	TArray<FStateTransition> Transitions;
	TObjectPtr<UStateBase>   CurrentState;
	FGameplayTag             CurrentStateTag;

	void TransitionTo(FGameplayTag To);
};
