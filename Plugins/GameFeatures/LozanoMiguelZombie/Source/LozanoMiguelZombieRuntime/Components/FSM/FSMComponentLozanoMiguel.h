#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StateBaseLozanoMiguel.h"
#include "FSMComponentLozanoMiguel.generated.h"

class UBlackboardComponent;


USTRUCT()
struct FFSMTransitionLozanoMiguel
{
	GENERATED_BODY()
	FName From;
	FName To;
	TFunction<bool(UBlackboardComponent*)> Predicate;
};

USTRUCT()
struct FFSMGlobalTransitionLozanoMiguel
{
	GENERATED_BODY()
	FName To;
	TFunction<bool(UBlackboardComponent*)> Predicate;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UFSMComponentLozanoMiguel : public UActorComponent
{
	GENERATED_BODY()
public:
	UFSMComponentLozanoMiguel();

	UPROPERTY()
	TMap<FName, TObjectPtr<UStateBaseLozanoMiguel>> States;

	UPROPERTY(EditAnywhere, Category="FSM")
	FName InitialState;

	TArray<FFSMTransitionLozanoMiguel> Transitions;

	TArray<FFSMGlobalTransitionLozanoMiguel> GlobalTransitions;

	UPROPERTY(EditAnywhere, Category="FSM|Debug")
	bool bLogTransitions = true;

protected:
	void InitializeComponent() override;

	virtual void BeginPlay() override;


public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterState(FName Name, UStateBaseLozanoMiguel * State);

	void AddTransition(const FFSMTransitionLozanoMiguel & Transition);

	void AddGlobalTransition(const FFSMGlobalTransitionLozanoMiguel & Transition);

	UFUNCTION(BlueprintCallable, Category="FSM")
	void ForceTransition(FName To);

	UFUNCTION(BlueprintPure, Category="FSM")
	FName GetCurrentStateName() const { return CurrentStateName; }

	UFUNCTION(BlueprintPure, Category="FSM")
	bool IsInState(FName Name) const { return CurrentStateName == Name; }

	TWeakObjectPtr<UBlackboardComponent> Blackboard;
private:
	TObjectPtr<UStateBaseLozanoMiguel>   CurrentState;
	FName                                CurrentStateName;

	UBlackboardComponent * ResolveBlackboard() const;
	bool EvaluateTransition(const FFSMTransitionLozanoMiguel & T, UBlackboardComponent* BB) const;
	void TransitionTo(FName To);


	void DeferredInit();


	void HandleStaminaLow();
	void HandleHealthLow();

	static TWeakObjectPtr<class UAudioComponent> s_LiveAmbientMusicAC;

	void StartAmbientMusicOnce();

	UFUNCTION()
	void OnAmbientMusicFinished();

	UPROPERTY()
	TObjectPtr<class USoundBase> m_AmbientMusic = nullptr;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> m_AmbientMusicAC = nullptr;
};
