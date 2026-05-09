#include "FSMComponent.h"

UFSMComponent::UFSMComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFSMComponent::BeginPlay()
{
	Super::BeginPlay();

	if (InitialState.IsValid())
		TransitionTo(InitialState);
}

void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentState)
		return;

	AActor * Owner = GetOwner();

	for (const FStateTransition & T : Transitions)
	{
		if (T.From == CurrentStateTag && T.Condition(Owner))
		{
			TransitionTo(T.To);
			return;
		}
	}

	CurrentState->OnTick(DeltaTime, Owner);
}

void UFSMComponent::RegisterState(FGameplayTag Tag, UStateBase* State)
{
	States.Add(Tag, State);
}

void UFSMComponent::AddTransition(FGameplayTag From, FGameplayTag To,
                                  TFunction<bool(AActor*)> Condition)
{
	Transitions.Add({ From, To, MoveTemp(Condition) });
}

void UFSMComponent::ForceTransition(FGameplayTag To)
{
	TransitionTo(To);
}

void UFSMComponent::TransitionTo(FGameplayTag To)
{
	TObjectPtr<UStateBase>* Found = States.Find(To);
	if (!Found || !*Found)
		return;

	AActor* Owner = GetOwner();

	if (CurrentState)
		CurrentState->OnExit(Owner);

	CurrentStateTag = To;
	CurrentState    = *Found;
	CurrentState->OnEnter(Owner);
}
