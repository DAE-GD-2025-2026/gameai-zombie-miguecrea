#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StateBase.h"
#include "FSMComponent.generated.h"

class UBlackboardComponent;


USTRUCT()
struct FFSMTransition
{
	GENERATED_BODY()
	FName From;
	FName To;
	TFunction<bool(UBlackboardComponent*)> Predicate;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UFSMComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFSMComponent();
	
	UPROPERTY()
	TMap<FName, TObjectPtr<UStateBase>> States;

	UPROPERTY(EditAnywhere, Category="FSM")
	FName InitialState;

	TArray<FFSMTransition> Transitions;

	UPROPERTY(EditAnywhere, Category="FSM|Debug")
	bool bLogTransitions = false;

protected: 
	  void InitializeComponent() override;
	
	virtual void BeginPlay() override;
	

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterState(FName Name, UStateBase * State);

	void AddTransition(const FFSMTransition & Transition);

	UFUNCTION(BlueprintCallable, Category="FSM")
	void ForceTransition(FName To);

	UFUNCTION(BlueprintPure, Category="FSM")
	FName GetCurrentStateName() const { return CurrentStateName; }

	UFUNCTION(BlueprintPure, Category="FSM")
	bool IsInState(FName Name) const { return CurrentStateName == Name; }

private:
	TWeakObjectPtr<UBlackboardComponent> Blackboard;
	TObjectPtr<UStateBase>               CurrentState;
	FName                                CurrentStateName;

	UBlackboardComponent * ResolveBlackboard() const;
	bool EvaluateTransition(const FFSMTransition & T, UBlackboardComponent* BB) const;
	void TransitionTo(FName To);

	// Runs one frame after BeginPlay. GameFeatureAction_AddComponents adds
	// sibling components one by one; by next tick every component in the
	// action's list is present, so this is the first safe moment to wire
	// cross-component references and activate the initial state.
	void DeferredInit();
};
