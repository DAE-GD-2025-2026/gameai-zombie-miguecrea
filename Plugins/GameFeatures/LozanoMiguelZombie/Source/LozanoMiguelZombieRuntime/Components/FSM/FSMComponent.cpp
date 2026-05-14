#include "FSMComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "StateBase.h"
#include "../Movement/SteeringComponent.h"

UFSMComponent::UFSMComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UFSMComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
}

void UFSMComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[FSM] BeginPlay on %s — scheduling deferred init."),
		*GetNameSafe(GetOwner()));
	if (UWorld * W = GetWorld())
	{
		W->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UFSMComponent::DeferredInit));
	}
}

void UFSMComponent::DeferredInit()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Sanity-check the sibling we care about — should be present now.
	USteeringComponent * Steering = Owner->FindComponentByClass<USteeringComponent>();
	if (!Steering)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM:%s] Steering still null after deferred init — check the GameFeature action."),
			*GetNameSafe(Owner));
	}

	UWanderState* Wander = NewObject<UWanderState>(this);
	RegisterState(Wander->GetStateName(), Wander);

	if (InitialState.IsNone())
		InitialState = Wander->GetStateName();

	Blackboard = ResolveBlackboard();
	if (!Blackboard.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[FSM:%s] Blackboard not yet available; will retry on first tick."),
			*GetNameSafe(Owner));
	}

	TransitionTo(InitialState);
}

void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentState)
		return;

	AActor * Owner = GetOwner();

	UBlackboardComponent* BB = Blackboard.Get();
	if (!BB)
	{
		// Controller may possess after our BeginPlay — retry until found
		BB = ResolveBlackboard();
		Blackboard = BB;
	}


	for (const FFSMTransition& T : Transitions)
	{
		if (T.From == CurrentStateName && EvaluateTransition(T, BB))
		{
			TransitionTo(T.To);
			return;
		}
	}

	CurrentState->OnTick(DeltaTime, Owner);
}

void UFSMComponent::RegisterState(FName Name, UStateBase* State)
{
	if (!State) return;
	State->Init(this); // hand the state a back-ref so it can reach siblings
	States.Add(Name, State);
}

void UFSMComponent::AddTransition(const FFSMTransition& Transition)
{
	Transitions.Add(Transition);
}

void UFSMComponent::ForceTransition(FName To)
{
	TransitionTo(To);
}

UBlackboardComponent* UFSMComponent::ResolveBlackboard() const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;
	AAIController* AI = Cast<AAIController>(Pawn->GetController());

	return AI ? AI->GetBlackboardComponent() : nullptr;
}

bool UFSMComponent::EvaluateTransition(const FFSMTransition& T, UBlackboardComponent* BB) const
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
	CurrentState = *Found;
	CurrentState->OnEnter(Owner);

	if (bLogTransitions)
	{
		UE_LOG(LogTemp, Log, TEXT("[FSM:%s] %s -> %s"),
		       *GetNameSafe(Owner), *PrevName.ToString(), *To.ToString());
	}
}


// static auto MakeBoolPredicate = [](FName Key)
// {
// 	return [Key](UBlackboardComponent* BB) -> bool
// 	{
// 		if (!BB) return false;
// 		const FBlackboard::FKey ID = BB->GetKeyID(Key);
// 		if (ID == FBlackboard::InvalidKey)
// 		{
// 			UE_LOG(LogTemp, Error, TEXT("Blackboard key %s missing"), *Key.ToString());
// 			return false;
// 		}
// 		return BB->GetValueAsBool(Key);
// 	};
// };
//
// WanderToLoot.Predicate = MakeBoolPredicate(BBKeys::bArrivedAtInterestPoint);
// ToLootWander.Predicate = MakeBoolPredicate(BBKeys::bLootDone);
//
