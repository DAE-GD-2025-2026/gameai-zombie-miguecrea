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

// Any-state interrupt: fires from whatever state the FSM happens to be in.
// Evaluated before per-state transitions every tick, so a global transition
// to Combat takes precedence over, e.g., a Loot->Wander transition that
// would otherwise fire the same frame.
//
// Self-loops are filtered automatically (the FSM checks CurrentStateName
// against To and skips if they match), so a "to Combat" global transition
// doesn't keep re-firing while we're already in Combat.
USTRUCT()
struct FFSMGlobalTransition
{
	GENERATED_BODY()
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

	// Interrupts that can fire from any current state (e.g. ->Combat when
	// a threat is spotted). Checked before Transitions in TickComponent.
	TArray<FFSMGlobalTransition> GlobalTransitions;

	UPROPERTY(EditAnywhere, Category="FSM|Debug")
	bool bLogTransitions = true;

protected: 
	  void InitializeComponent() override;
	
	virtual void BeginPlay() override;
	

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterState(FName Name, UStateBase * State);

	void AddTransition(const FFSMTransition & Transition);

	// Register an any-state interrupt. Multiple globals may exist; they're
	// evaluated in insertion order each tick, first match wins.
	void AddGlobalTransition(const FFSMGlobalTransition & Transition);

	UFUNCTION(BlueprintCallable, Category="FSM")
	void ForceTransition(FName To);

	UFUNCTION(BlueprintPure, Category="FSM")
	FName GetCurrentStateName() const { return CurrentStateName; }

	UFUNCTION(BlueprintPure, Category="FSM")
	bool IsInState(FName Name) const { return CurrentStateName == Name; }

	TWeakObjectPtr<UBlackboardComponent> Blackboard;
private:
	TObjectPtr<UStateBase>               CurrentState;
	FName                                CurrentStateName;

	UBlackboardComponent * ResolveBlackboard() const;
	bool EvaluateTransition(const FFSMTransition & T, UBlackboardComponent* BB) const;
	void TransitionTo(FName To);


	void DeferredInit();
	void TestFunctionStateMachine();

	// Reactive "use a consumable" handlers. Bound to the perceptor's
	// m_StaminaLow / m_HealthLow non-dynamic multicast delegates in
	// DeferredInit. State-agnostic — fires regardless of whether we're
	// in Wander, Combat, Loot, etc.
	void HandleStaminaLow();
	void HandleHealthLow();

	// Background music: load once, play 2D, manually loop via OnAudioFinished.
	// Tracks the *live* AudioComponent so PIE restarts re-start music cleanly
	// (a process-level bool would stay true between PIE sessions and lock us
	// out — TWeakObjectPtr invalidates when the AC is destroyed).
	static TWeakObjectPtr<class UAudioComponent> s_LiveAmbientMusicAC;

	void StartAmbientMusicOnce();

	UFUNCTION()
	void OnAmbientMusicFinished();

	UPROPERTY()
	TObjectPtr<class USoundBase> m_AmbientMusic = nullptr;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> m_AmbientMusicAC = nullptr;
};
