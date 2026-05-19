#include "FSMComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "StateBase.h"
#include "../Movement/SteeringComponent.h"
#include "../StudentPerceptor/StudentPerceptor.h"
#include "LozanoMiguelZombieRuntime/Components/BlackBoard/BBKeys.h"


TWeakObjectPtr<UAudioComponent> UFSMComponent::s_LiveAmbientMusicAC;

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

	// Background ambient — only the first FSM to BeginPlay wins this race.
	// All subsequent FSMs see the static flag and skip; one shared track.
	StartAmbientMusicOnce();
}

void UFSMComponent::StartAmbientMusicOnce()
{
	
	if (s_LiveAmbientMusicAC.IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FSM] Ambient music already running — skipping."));
		return;
	}

	UWorld* W = GetWorld();
	if (!W)
	{
		UE_LOG(LogTemp, Error, TEXT("[FSM] No world — cannot start ambient music."));
		return;
	}

	static const TCHAR * MusicPath =
		TEXT("/LozanoMiguelZombie/Sounds/shadowsandechoes-inhuman-post-apocalyptic-dark-ambient-248361.shadowsandechoes-inhuman-post-apocalyptic-dark-ambient-248361");

	UE_LOG(LogTemp, Warning, TEXT("[FSM] Loading ambient music: %s"), MusicPath);

	m_AmbientMusic = LoadObject<USoundBase>(nullptr, MusicPath);
	if (!m_AmbientMusic)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM] Failed to load ambient music at '%s'.\n"
			     "  → Open the asset in the Content Browser, right-click → "
			     "Copy Reference, and paste here to compare."), MusicPath);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[FSM] Ambient asset loaded: %s (Duration=%.2fs)"),
		*m_AmbientMusic->GetName(), m_AmbientMusic->GetDuration());


	m_AmbientMusicAC = UGameplayStatics::SpawnSound2D(
		W,
		m_AmbientMusic,
		/*Volume*/ 0.7f,
		/*Pitch*/  1.0f,
		/*Start*/  0.0f,
		/*Attenuation*/ nullptr,
		/*bPersistAcrossLevelTransition*/ false,
		/*bAutoDestroy*/ false);

	if (!m_AmbientMusicAC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM] SpawnSound2D returned null — ambient music not playing."));
		return;
	}

	m_AmbientMusicAC->OnAudioFinished.AddDynamic(this, &UFSMComponent::OnAmbientMusicFinished);
	s_LiveAmbientMusicAC = m_AmbientMusicAC;

	UE_LOG(LogTemp, Warning,
		TEXT("[FSM] Ambient music spawned. IsPlaying=%d, Volume=%.2f"),
		m_AmbientMusicAC->IsPlaying() ? 1 : 0, m_AmbientMusicAC->VolumeMultiplier);
}

void UFSMComponent::OnAmbientMusicFinished()
{
	// Manual loop — restart the same audio component. Cheaper than spawning
	// a new one and keeps the bound delegate alive for the next iteration.
	if (m_AmbientMusicAC && IsValid(m_AmbientMusicAC))
	{
		m_AmbientMusicAC->Play();
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
	UCombatState * Combat = NewObject<UCombatState>(this);
	UFleeState * Flee = NewObject<UFleeState>(this);
	
	//ADD THE FEAR STATE 
	
	RegisterState(Wander->GetStateName(), Wander);
	RegisterState(Loot->GetStateName(),Loot);
	RegisterState(Combat->GetStateName(),Combat);
	RegisterState(Flee->GetStateName(),Flee);
	
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

	// --- Combat interrupt --------------------------------------------------
	// Any-state -> Combat the moment a threat is visible. Evaluated before
	// the per-state transitions above, so a zombie spotted mid-Loot stops
	// Loot immediately.
	FFSMGlobalTransition ToCombat;
	ToCombat.To = Combat->GetStateName();
	ToCombat.Predicate = [](UBlackboardComponent * BB)
	{
		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeys::bThreatNearby);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error, TEXT("Blackboard key bThreatNearby does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeys::bThreatNearby);
	};
	AddGlobalTransition(ToCombat);

	// Return path: Combat -> Wander once the threat clears. This is a
	// regular per-state transition (not a global) so it can't accidentally
	// fire while we're in another state.
	FFSMTransition CombatToWander;
	CombatToWander.From      = Combat->GetStateName();
	CombatToWander.To        = Wander->GetStateName();
	CombatToWander.Predicate = [](UBlackboardComponent * BB)
	{
		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeys::bThreatGone);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error, TEXT("Blackboard key bThreatGone does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeys::bThreatGone);
	};
	AddTransition(CombatToWander);
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
	UBlackboardComponent * BB = Blackboard.Get();

	// 1) Global interrupts — evaluated first so a threat sighting preempts
	//    whatever per-state transition would otherwise fire this frame.
	if (BB)
	{
		for (const FFSMGlobalTransition & G : GlobalTransitions)
		{
			// Skip self-loops so "->Combat" doesn't keep re-firing while
			// we're already in Combat (the predicate stays true until the
			// threat goes away).
			if (G.To == CurrentStateName) continue;
			if (G.Predicate && G.Predicate(BB))
			{
				TransitionTo(G.To);
				return;
			}
		}
	}

	// 2) Per-state transitions.
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

void UFSMComponent::AddGlobalTransition(const FFSMGlobalTransition& Transition)
{
	GlobalTransitions.Add(Transition);
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

	AActor * Owner = GetOwner();

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
