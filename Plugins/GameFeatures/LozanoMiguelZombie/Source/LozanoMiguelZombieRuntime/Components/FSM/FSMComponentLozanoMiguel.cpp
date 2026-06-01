#include "FSMComponentLozanoMiguel.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "StateBaseLozanoMiguel.h"
#include "WanderStateLozanoMiguel.h"
#include "LootStateLozanoMiguel.h"
#include "CombatStateLozanoMiguel.h"
#include "FleeStateLozanoMiguel.h"
#include "SuicideStateLozanoMiguel.h"
#include "../Movement/SteeringComponentLozanoMiguel.h"
#include "../StudentPerceptor/StudentPerceptorLozanoMiguel.h"
#include "LozanoMiguelZombieRuntime/Components/BlackBoard/BBKeysLozanoMiguel.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"


TWeakObjectPtr<UAudioComponent> UFSMComponentLozanoMiguel::s_LiveAmbientMusicAC;

UFSMComponentLozanoMiguel::UFSMComponentLozanoMiguel()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UFSMComponentLozanoMiguel::InitializeComponent()
{
	Super::InitializeComponent();

}

void UFSMComponentLozanoMiguel::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[FSM] BeginPlay on %s — scheduling deferred init."),
		*GetNameSafe(GetOwner()));

	if (UWorld * W = GetWorld())
	{
		W->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UFSMComponentLozanoMiguel::DeferredInit));
	}

	StartAmbientMusicOnce();
}

void UFSMComponentLozanoMiguel::StartAmbientMusicOnce()
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
		 0.7f,
		 1.0f,
	  0.0f,
		 nullptr,
		 false,
		 false);

	if (!m_AmbientMusicAC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM] SpawnSound2D returned null — ambient music not playing."));
		return;
	}

	m_AmbientMusicAC->OnAudioFinished.AddDynamic(this, &UFSMComponentLozanoMiguel::OnAmbientMusicFinished);
	s_LiveAmbientMusicAC = m_AmbientMusicAC;

	UE_LOG(LogTemp, Warning,
		TEXT("[FSM] Ambient music spawned. IsPlaying=%d, Volume=%.2f"),
		m_AmbientMusicAC->IsPlaying() ? 1 : 0, m_AmbientMusicAC->VolumeMultiplier);
}

void UFSMComponentLozanoMiguel::OnAmbientMusicFinished()
{
	if (m_AmbientMusicAC && IsValid(m_AmbientMusicAC))
	{
		m_AmbientMusicAC->Play();
	}
}

void UFSMComponentLozanoMiguel::DeferredInit()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

		Blackboard = ResolveBlackboard();

	USteeringComponentLozanoMiguel * Steering = Owner->FindComponentByClass<USteeringComponentLozanoMiguel>();
	if (!Steering)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM:%s] Steering still null after deferred init — check the GameFeature action."),
			*GetNameSafe(Owner));
	}

	// ADD THE FACT THAT MEMORY TICKS BEFORE STATE MACHINE
	UStudentPerceptorLozanoMiguel * memory = Owner->FindComponentByClass<UStudentPerceptorLozanoMiguel>();
	if (memory)
	{
		PrimaryComponentTick.AddPrerequisite(memory, memory->PrimaryComponentTick);
		memory->m_StaminaLow.AddUObject(this, &UFSMComponentLozanoMiguel::HandleStaminaLow);
		memory->m_HealthLow .AddUObject(this, &UFSMComponentLozanoMiguel::HandleHealthLow);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FSM:%s] StudentPerceptor not found — needs delegates unbound."),
			*GetNameSafe(Owner));
	}


	UWanderStateLozanoMiguel * Wander = NewObject<UWanderStateLozanoMiguel>(this);
	ULootStateLozanoMiguel * Loot = NewObject<ULootStateLozanoMiguel>(this);
	UCombatStateLozanoMiguel * Combat = NewObject<UCombatStateLozanoMiguel>(this);
	UFleeStateLozanoMiguel * Flee = NewObject<UFleeStateLozanoMiguel>(this);
	USuicideStateLozanoMiguel * Suicide = NewObject<USuicideStateLozanoMiguel>(this);

	RegisterState(Wander->GetStateName(), Wander);
	RegisterState(Loot->GetStateName(),Loot);
	RegisterState(Combat->GetStateName(),Combat);
	RegisterState(Flee->GetStateName(),Flee);
	RegisterState(Suicide->GetStateName(),Suicide);

	FFSMTransitionLozanoMiguel WanderToLoot;
	WanderToLoot.From = Wander->GetStateName();
	WanderToLoot.To   = Loot->GetStateName();
	WanderToLoot.Predicate = [](UBlackboardComponent * BB)
	{
		const FBlackboard::FKey KeyID =
		BB->GetKeyID(BBKeysLozanoMiguel::bArrivedAtInterestPoint);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error,
				TEXT("Blackboard key bTargetVisible does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeysLozanoMiguel::bArrivedAtInterestPoint);
	};


	FFSMTransitionLozanoMiguel ToLootWander;
	ToLootWander.From = Loot->GetStateName();
	ToLootWander.To   = Wander->GetStateName();
	ToLootWander.Predicate = [](UBlackboardComponent * BB)
	{
		const FBlackboard::FKey KeyID =
		BB->GetKeyID(BBKeysLozanoMiguel::bLootDone);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error,
				TEXT("Blackboard key bTargetVisible does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeysLozanoMiguel::bLootDone);
	};

	AddTransition(WanderToLoot);
	AddTransition(ToLootWander);


	FFSMGlobalTransitionLozanoMiguel ToSuicide;
	ToSuicide.To = Suicide->GetStateName();
	ToSuicide.Predicate = [](UBlackboardComponent* BB)
	{
		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeysLozanoMiguel::bShouldSuicide);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error, TEXT("Blackboard key bShouldSuicide does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeysLozanoMiguel::bShouldSuicide);
	};
	AddGlobalTransition(ToSuicide);

	FFSMGlobalTransitionLozanoMiguel ToFlee;
	ToFlee.To = Flee->GetStateName();
	ToFlee.Predicate = [](UBlackboardComponent* BB)
	{
		// Suicide outranks Flee — if we're queued to die, don't run.
		const FBlackboard::FKey SuicideKey = BB->GetKeyID(BBKeysLozanoMiguel::bShouldSuicide);
		if (SuicideKey != FBlackboard::InvalidKey &&
			BB->GetValueAsBool(BBKeysLozanoMiguel::bShouldSuicide))
		{
			return false;
		}

		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeysLozanoMiguel::bPurgeZoneNearby);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error, TEXT("Blackboard key bPurgeZoneNearby does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeysLozanoMiguel::bPurgeZoneNearby);
	};
	AddGlobalTransition(ToFlee);

	FFSMGlobalTransitionLozanoMiguel ToCombat;
	ToCombat.To = Combat->GetStateName();
	ToCombat.Predicate = [](UBlackboardComponent * BB)
	{
		// Combat is gated by BOTH higher-priority signals.
		const FBlackboard::FKey SuicideKey = BB->GetKeyID(BBKeysLozanoMiguel::bShouldSuicide);
		if (SuicideKey != FBlackboard::InvalidKey &&
			BB->GetValueAsBool(BBKeysLozanoMiguel::bShouldSuicide))
		{
			return false;
		}
		const FBlackboard::FKey FleeKey = BB->GetKeyID(BBKeysLozanoMiguel::bPurgeZoneNearby);
		if (FleeKey != FBlackboard::InvalidKey &&
			BB->GetValueAsBool(BBKeysLozanoMiguel::bPurgeZoneNearby))
		{
			return false;
		}

		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeysLozanoMiguel::bThreatNearby);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error, TEXT("Blackboard key bThreatNearby does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeysLozanoMiguel::bThreatNearby);
	};
	AddGlobalTransition(ToCombat);

	// Flee→Wander when the perceptor reports the zone is no longer close.
	// Per-state (not global) so it only fires while Flee is active.
	FFSMTransitionLozanoMiguel FleeToWander;
	FleeToWander.From      = Flee->GetStateName();
	FleeToWander.To        = Wander->GetStateName();
	FleeToWander.Predicate = [](UBlackboardComponent* BB)
	{
		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeysLozanoMiguel::bPurgeZoneNearby);
		if (KeyID == FBlackboard::InvalidKey) return false;
		// Out of Flee the moment the zone is no longer flagged.
		return !BB->GetValueAsBool(BBKeysLozanoMiguel::bPurgeZoneNearby);
	};
	AddTransition(FleeToWander);

	FFSMTransitionLozanoMiguel CombatToWander;
	CombatToWander.From      = Combat->GetStateName();
	CombatToWander.To        = Wander->GetStateName();
	CombatToWander.Predicate = [](UBlackboardComponent * BB)
	{
		const FBlackboard::FKey KeyID = BB->GetKeyID(BBKeysLozanoMiguel::bThreatGone);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogTemp, Error, TEXT("Blackboard key bThreatGone does not exist"));
			return false;
		}
		return BB->GetValueAsBool(BBKeysLozanoMiguel::bThreatGone);
	};
	AddTransition(CombatToWander);

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

void UFSMComponentLozanoMiguel::TickComponent(float DeltaTime, ELevelTick TickType,
                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentState)
		return;

	AActor * Owner = GetOwner();
	UBlackboardComponent * BB = Blackboard.Get();


	if (BB)
	{
		for (const FFSMGlobalTransitionLozanoMiguel & G : GlobalTransitions)
		{
			if (G.To == CurrentStateName) continue;
			if (G.Predicate && G.Predicate(BB))
			{
				TransitionTo(G.To);
				return;
			}
		}
	}

	// 2) Per-state transitions.
	for (const FFSMTransitionLozanoMiguel & T : Transitions)
	{
		if (T.From == CurrentStateName && EvaluateTransition(T, BB))
		{
			TransitionTo(T.To);
			return;
		}
	}

	CurrentState->OnTick(DeltaTime, Owner);
}

void UFSMComponentLozanoMiguel::RegisterState(FName Name, UStateBaseLozanoMiguel* State)
{
	if (!State) return;
	State->Init(this);
	States.Add(Name, State);
}

void UFSMComponentLozanoMiguel::AddTransition(const FFSMTransitionLozanoMiguel& Transition)
{
	Transitions.Add(Transition);
}

void UFSMComponentLozanoMiguel::AddGlobalTransition(const FFSMGlobalTransitionLozanoMiguel& Transition)
{
	GlobalTransitions.Add(Transition);
}

void UFSMComponentLozanoMiguel::ForceTransition(FName To)
{
	TransitionTo(To);
}

namespace
{
	bool UseFirstItemOfType(UInventoryComponent * Inv, EItemType DesiredType)
	{
		if (!Inv) return false;
		const TArray<ABaseItem*>& Slots = Inv->GetInventory();
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i] && Slots[i]->GetItemType() == DesiredType)
			{
				bool result = Inv->UseItem(i);
				if (Slots[i]->GetValue() <=0)
				{
					//if after using Item values is 0 remove it
					Inv->RemoveItem(i);
				}
				return result;
			}
		}
		return false;
	}
}

void UFSMComponentLozanoMiguel::HandleStaminaLow()
{

	AActor* Owner = GetOwner();
	UInventoryComponent* Inv = Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
	const bool bUsed = UseFirstItemOfType(Inv, EItemType::Food);
	UE_LOG(LogTemp, Warning,
		TEXT("[FSM] StaminaLow received — UseFood %s."),
		bUsed ? TEXT("succeeded") : TEXT("failed (no food in inventory)"));
}

void UFSMComponentLozanoMiguel::HandleHealthLow()
{
	AActor* Owner = GetOwner();
	UInventoryComponent * Inv = Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
	const bool bUsed = UseFirstItemOfType(Inv, EItemType::Medkit);
	UE_LOG(LogTemp, Warning,
		TEXT("[FSM] HealthLow received — UseMedkit %s."),
		bUsed ? TEXT("succeeded") : TEXT("failed (no medkit in inventory)"));
}

UBlackboardComponent * UFSMComponentLozanoMiguel::ResolveBlackboard() const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;
	AAIController * AI = Cast<AAIController>(Pawn->GetController());
	return AI ? AI->GetBlackboardComponent() : nullptr;
}

bool UFSMComponentLozanoMiguel::EvaluateTransition(const FFSMTransitionLozanoMiguel& T, UBlackboardComponent* BB) const
{
	return T.Predicate && T.Predicate(BB);
}

void UFSMComponentLozanoMiguel::TransitionTo(FName To)
{
	TObjectPtr<UStateBaseLozanoMiguel>* Found = States.Find(To);
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
