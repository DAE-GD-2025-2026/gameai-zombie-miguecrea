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
	
	
//	USeekState * SeekState =  NewObject<USeekState>();
	
	//Weak ptr
	Blackboard = ResolveBlackboard();
	
	
	//EVALUATE TARISITON FUNCTION IS WRONG 
	//STATES NEED THE NAME 
	//ED ANOTHER COMPONENT THAT SETS VALUES OF KEYS AND IUT NEEDS TO TICK BEFORE
	
	
	if (Blackboard.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BlackBoard Is Okay"));
		
		const FBlackboard::FKey KeyID = Blackboard->GetKeyID("Perro");

		if (KeyID != FBlackboard::InvalidKey)
		{
			int32 Value = Blackboard->GetValueAsInt("Perro");
			
			UE_LOG(LogTemp, Warning, TEXT("Perro %d"), Value);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Key does not exist"));
		}
		
		
		// FFSMTransition T;
		// T.From = FGame;
		// T.To   = ZombieTags::State_Chase;
		// T.Predicate = [](UBlackboardComponent* BB) {
		// 	return BB->GetValueAsBool(TEXT("bTargetVisible"));
		// };
		// FSM->AddTransition(T);
		//
		// // or a more complex one
		// FFSMTransition T2;
		// T2.From = ZombieTags::State_Chase;
		// T2.To   = ZombieTags::State_Attack;
		// T2.Predicate = [](UBlackboardComponent* BB) {
		// 	return BB->GetValueAsFloat(TEXT("DistanceToTarget")) < 150.f;
		// };
		// FSM->AddTransition(T2);
		//
		
		
		
		if (InitialState.IsValid())
			TransitionTo(InitialState);
	}
	else
	{
		UE_LOG(LogTemp,Error, TEXT("BlackBoard is invalid"));
	}

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
	
	if (BB)
	{
		for (const FFSMTransition & T : Transitions)
		{
			if (T.From == CurrentStateTag && EvaluateTransition(T,BB))
			{
				TransitionTo(T.To);
				return;
			}
		}
	}
	
	CurrentState->OnTick(DeltaTime, Owner);
}

void UFSMComponent::RegisterState(FGameplayTag Tag, UStateBase * State)
{
	States.Add(Tag, State);
}

void UFSMComponent::AddTransition(const FFSMTransition & Transition)
{
	Transitions.Add(Transition);
}

void UFSMComponent::ForceTransition(FGameplayTag To)
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

void UFSMComponent::TransitionTo(FGameplayTag To)
{
	TObjectPtr<UStateBase>* Found = States.Find(To);
	if (!Found || !*Found)
		return;

	AActor* Owner = GetOwner();

	if (CurrentState)
		CurrentState->OnExit(Owner);

	const FGameplayTag PrevTag = CurrentStateTag;
	CurrentStateTag = To;
	CurrentState    = *Found;
	CurrentState->OnEnter(Owner);

	if (bLogTransitions)
	{
		UE_LOG(LogTemp, Log, TEXT("[FSM:%s] %s -> %s"),
			*GetNameSafe(Owner), *PrevTag.ToString(), *To.ToString());
	}
}
