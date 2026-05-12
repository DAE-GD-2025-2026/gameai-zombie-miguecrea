#include "FSMComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "StateBase.h"

UFSMComponent::UFSMComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFSMComponent::BeginPlay()
{
	Super::BeginPlay();


	UWanderState * Wander = NewObject<UWanderState>(this);
	RegisterState(Wander->GetStateName(), Wander);
	
	

	if (InitialState.IsNone())
		InitialState = Wander->GetStateName();

	
	Blackboard = ResolveBlackboard();
	if (!Blackboard.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[FSM:%s] Blackboard not yet available; will retry on first tick."),
			*GetNameSafe(GetOwner()));
	}

	TransitionTo(InitialState);
}

void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                  FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentState)
		return;
	
	AActor* Owner = GetOwner();
	
	UBlackboardComponent * BB = Blackboard.Get();
	if (!BB)
	{
		// Controller may possess after our BeginPlay — retry until found
		BB = ResolveBlackboard();
		Blackboard = BB;
	}
	
	
	for (const FFSMTransition & T : Transitions)
	{
		if (T.From == CurrentStateName && EvaluateTransition(T, BB))
		{
			TransitionTo(T.To);
			return;
		}
	}

	CurrentState->OnTick(DeltaTime, Owner);
}

void UFSMComponent::RegisterState(FName Name, UStateBase * State)
{
	if (!State) return;
	State->Init(this);   // hand the state a back-ref so it can reach siblings
	States.Add(Name, State);
}

void UFSMComponent::AddTransition(const FFSMTransition & Transition)
{
	Transitions.Add(Transition);
}

void UFSMComponent::ForceTransition(FName To)
{
	TransitionTo(To);
}

UBlackboardComponent * UFSMComponent::ResolveBlackboard() const
{
	APawn * Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;
	AAIController * AI = Cast<AAIController>(Pawn->GetController());
	
	return AI ? AI->GetBlackboardComponent() : nullptr;
}

bool UFSMComponent::EvaluateTransition(const FFSMTransition & T,UBlackboardComponent* BB) const
{
	return T.Predicate && T.Predicate(BB);
}

void UFSMComponent::TransitionTo(FName To)
{
	TObjectPtr<UStateBase>* Found = States.Find(To);
	if (!Found || !*Found)
		return;

	AActor* Owner = GetOwner();

	if (CurrentState)
		CurrentState->OnExit(Owner);

	const FName PrevName = CurrentStateName;
	CurrentStateName = To;
	CurrentState     = *Found;
	CurrentState->OnEnter(Owner);

	if (bLogTransitions)
	{
		UE_LOG(LogTemp, Log, TEXT("[FSM:%s] %s -> %s"),
			*GetNameSafe(Owner), *PrevName.ToString(), *To.ToString());
	}
}
