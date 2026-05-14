#include "FSMComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "StateBase.h"
#include "../Movement/SteeringComponent.h"
#include "../StudentPerceptor/StudentPerceptor.h"
#include "LozanoMiguelZombieRuntime/Components/BlackBoard/BBKeys.h"

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
	
		Blackboard = ResolveBlackboard();
	
	// Sanity-check the sibling we care about — should be present now.
	USteeringComponent * Steering = Owner->FindComponentByClass<USteeringComponent>();
	if (!Steering)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM:%s] Steering still null after deferred init — check the GameFeature action."),
			*GetNameSafe(Owner));
	}

	// ADD THE FACT THAT MEMORY TICKS BEFORE STATE MACHINE 
	UStudentPerceptor * memory = Owner->FindComponentByClass<UStudentPerceptor>();
	PrimaryComponentTick.AddPrerequisite(
	memory,
	memory->PrimaryComponentTick
     );
	
	
	UWanderState * Wander = NewObject<UWanderState>(this);
	ULootState * Loot = NewObject<ULootState>(this);
	
	RegisterState(Wander->GetStateName(), Wander);
	RegisterState(Loot->GetStateName(),Loot);
	
	FFSMTransition WanderToLoot;
	WanderToLoot.From = Wander->GetStateName();
	WanderToLoot.To   = Loot->GetStateName();
	WanderToLoot.Predicate = [](UBlackboardComponent * BB) 
	{
		const FBlackboard::FKey KeyID =
		BB->GetKeyID(BBKeys::bArrivedAtInterestPoint);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error,
				TEXT("Blackboard key bTargetVisible does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeys::bArrivedAtInterestPoint);
	};
	
	
	FFSMTransition ToLootWander;
	ToLootWander.From = Loot->GetStateName();
	ToLootWander.To   = Wander->GetStateName();
	ToLootWander.Predicate = [](UBlackboardComponent * BB) 
	{
		const FBlackboard::FKey KeyID =
		BB->GetKeyID(BBKeys::bLootDone);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error,
				TEXT("Blackboard key bTargetVisible does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeys::bLootDone);
	};
	
	
	
	//if there is more than one way of transitioning From one state to Another first one would
	//have higher priority
	AddTransition(WanderToLoot);  // 
	AddTransition(ToLootWander);  // 
	
	
	
	//FTimerHandle TestHandle;
	
	// GetWorld()->GetTimerManager().SetTimer(
	// TestHandle,
	// this,
	// &UFSMComponent::TestFunctionStateMachine,
	// 5.0f,
	// false
//);
	
	
	
	
	
	
	
	
	
	
	////////////////////////////////
	
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

void UFSMComponent::TestFunctionStateMachine()
{
	
	
	// UE_LOG(LogTemp,Warning,TEXT("TestFunctionStateMachine"));
	// if (Blackboard.Get())
	// {
	// Blackboard->SetValueAsBool(TEXT("bTargetVisible"),true);
	// }
	// else
	// {
	// 	UE_LOG(LogTemp,Error,TEXT("Blackboard Is Null"));
	// }
}

void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentState)
		return;

	AActor * Owner = GetOwner();
	
	for (const FFSMTransition & T : Transitions)
	{
		// if Evaluates to true switches to State
		if (T.From == CurrentStateName && EvaluateTransition(T,Blackboard.Get()))
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

UBlackboardComponent * UFSMComponent::ResolveBlackboard() const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;
	AAIController * AI = Cast<AAIController>(Pawn->GetController());
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
		UE_LOG(LogTemp, Log, TEXT("[FSM:%s] %s -> %s"),*GetNameSafe(Owner), *PrevName.ToString(), *To.ToString());
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
