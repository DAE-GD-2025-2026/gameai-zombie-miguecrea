#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StateBase.h"
#include "FSMComponent.generated.h"

class UBlackboardComponent;


USTRUCT()
struct FFSMTransition
{
	GENERATED_BODY()
	FGameplayTag From;
	FGameplayTag To;
	// Pure C++ predicate. Not a UPROPERTY — invisible to editor/Blueprint.
	TFunction<bool(UBlackboardComponent*)> Predicate;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UFSMComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFSMComponent();

	UPROPERTY(EditAnywhere, Instanced, Category="FSM")
	TMap<FGameplayTag, TObjectPtr<UStateBase>> States;

	UPROPERTY(EditAnywhere, Category="FSM")
	FGameplayTag InitialState;

	UPROPERTY(EditAnywhere, Category="FSM")
	TArray<FFSMTransition> Transitions;

	UPROPERTY(EditAnywhere, Category="FSM|Debug")
	bool bLogTransitions = false;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterState(FGameplayTag Tag, UStateBase * State);

	void AddTransition(const FFSMTransition & Transition);

	UFUNCTION(BlueprintCallable, Category="FSM")
	void ForceTransition(FGameplayTag To);

	UFUNCTION(BlueprintPure, Category="FSM")
	FGameplayTag GetCurrentStateTag() const { return CurrentStateTag; }

	UFUNCTION(BlueprintPure, Category="FSM")
	bool IsInState(FGameplayTag Tag) const { return CurrentStateTag == Tag; }

private:
	TWeakObjectPtr<UBlackboardComponent> Blackboard;
	TObjectPtr<UStateBase>               CurrentState;
	FGameplayTag                         CurrentStateTag;

	UBlackboardComponent * ResolveBlackboard() const;
	bool EvaluateTransition(const FFSMTransition & T, UBlackboardComponent* BB) const;
	void TransitionTo(FGameplayTag To);
};
