// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "Survivor/SurvivorPawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"
#include "../MACROS/DebugMacro.h"
#include "Items/ItemType.h"
#include "Common/StaminaComponent.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Zombies/BaseZombie.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "../BlackBoard/BBKeys.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "EngineUtils.h" // TActorIterator
#include "Kismet/GameplayStatics.h"




UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld * W = GetWorld())
	{
		W->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UStudentPerceptor::DeferredInit));
	}

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&UStudentPerceptor::ChangeColor,
		0.1f,
		true
	);
	
}

void UStudentPerceptor::DeferredInit()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	m_Health              = Owner->FindComponentByClass<UHealthComponent>();
	m_Stamina             = Owner->FindComponentByClass<UStaminaComponent>();
	m_Inventory           = Owner->FindComponentByClass<UInventoryComponent>();
	m_PerceptionComponent = Owner->FindComponentByClass<UAIPerceptionComponent>();

	if (!m_Health)    UE_LOG(LogTemp, Error, TEXT("[Perceptor] HealthComponent not found."));
	if (!m_Stamina)   UE_LOG(LogTemp, Error, TEXT("[Perceptor] StaminaComponent not found."));
	if (!m_Inventory) UE_LOG(LogTemp, Warning, TEXT("[Perceptor] InventoryComponent not found — weapon-urgency disabled."));

	if (m_PerceptionComponent)
	{
		m_PerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(
			this, &UStudentPerceptor::OnPerceptionUpdated);
		m_PerceptionComponent->OnTargetPerceptionForgotten.AddUniqueDynamic(
			this, &UStudentPerceptor::OnTargetForgotten);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Perceptor] AIPerceptionComponent not found."));
	}
}

void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	
	AddZombiesToMemory();

	if (m_Stamina)
	{
		const float S = m_Stamina->GetCurrentStamina();
		if (!m_bWasStaminaLow && S <= StaminaLowEnter)
		{
			m_bWasStaminaLow = true;
			m_StaminaLow.Broadcast();
		}
		else if (m_bWasStaminaLow && S >= StaminaLowExit)
		{
			m_bWasStaminaLow = false;   // silent re-arm
		}
	}

	// --- Edge-detect low health ---------------------------------------
	if (m_Health)
	{
		const int H = m_Health->GetHealth();
		if (!m_bWasHealthLow && H <= HealthLowEnter)
		{
			m_bWasHealthLow = true;
			m_HealthLow.Broadcast();
		}
		else if (m_bWasHealthLow && H >= HealthLowExit)
		{
			m_bWasHealthLow = false;
		}
	}
	
	for (const FInterestPoint & InterestPoint : m_WannaPointsInBrain)
	{
		AActor * A = InterestPoint.Actor.Get();
		if (A && CurrentColor)
		{
			if (!InterestPoint.m_Visited)
			{
				DRAW_CIRCLE(GetWorld(), A->GetActorLocation(), 30.f, *CurrentColor, 3.f);
			}
			else
			{
				DRAW_CIRCLE(GetWorld(), A->GetActorLocation(), 30.f, FColor::Yellow, 3.f);
			}
		}
	}

	for (const FInterestPoint& InterestPoint : m_SaveForLaterPoints)
	{
		AActor* A = InterestPoint.Actor.Get();
		if (A)
		{
			DRAW_CIRCLE(GetWorld(), A->GetActorLocation(), 30.f, FColor::Turquoise, 3.f);
		}
	}
	
}

void UStudentPerceptor::OnPerceptionUpdated(AActor * Actor, FAIStimulus Stimulus)
{
	
	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
		return;
	const bool bSensed = Stimulus.WasSuccessfullySensed();
	
	if (!bSensed) return;

	if (Cast<AHouse>(Actor) || Cast<ABaseItem>(Actor))
	{
		ABaseItem* Item = Cast<ABaseItem>(Actor);
		if (Item && Item->GetItemType() == EItemType::Garbage) return;

		FInterestPoint InterestPoint;
		InterestPoint.Actor    = Actor;
		InterestPoint.m_Visited = false;
		m_WannaPointsInBrain.AddUnique(InterestPoint);
	}
}

void UStudentPerceptor::OnTargetForgotten(AActor * Actor)
{
	// when Stimuli expired age is 5 seconds will forget after 5 seconds 
	UE_LOG(LogTemp,Error,TEXT("Target Forgotten"))
	if (ABaseZombie * Z = Cast<ABaseZombie>(Actor))
	{
		m_VisibleZombies.Remove(Z);
	}
}


FInterestPoint * UStudentPerceptor::GetBestInterestPoint()
{
	FInterestPoint * BestInterestPoint = nullptr;
	
	if (m_WannaPointsInBrain.IsEmpty())
		return nullptr;

	AActor * OwnerActor = GetOwner();
	const FVector MyLoc = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;

	float BestScore = 0.f;
	for (FInterestPoint & InterestPoint : m_WannaPointsInBrain)
	{
		if (InterestPoint.m_Visited)
			continue;

		AActor* Target = InterestPoint.Actor.Get();
		if (!Target)
			continue;

		float Score = 0.f;

		if (Cast<AHouse>(Target))
		{
			Score = GetHouseBaseUtility();
			 
		}
		else if (ABaseItem * Item = Cast<ABaseItem>(Target))
		{
			const EItemType ItemType = Item->GetItemType();
			Score = GetItemBaseUtility(ItemType);
			Score = ApplyContextModifier(Score, ItemType);
		}

		const float Dist = FVector::Dist(MyLoc, Target->GetActorLocation());
		const float DistFalloff =
			FMath::Clamp(1.f - Dist / MaxConsiderDistance, 0.f, 1.f);

		Score *= DistFalloff;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestInterestPoint = &InterestPoint;
		}
	}

	return BestInterestPoint;
}


float UStudentPerceptor::GetItemBaseUtility(EItemType type)
{
	switch (type)
	{
	case EItemType::Medkit: return 10.f;
	case EItemType::Food: return 8.f;
	case EItemType::Pistol:
	case EItemType::Shotgun: return 6.f;
	default: return 1.f;
	}
}

float UStudentPerceptor::GetHouseBaseUtility()
{
	// Lower than the Medkit base (10) so a visible item at comparable
	// distance always wins. Houses are still picked when no item is in
	// memory or when an item is significantly further away — the distance
	// falloff in GetBestInterestPoint does the rest.
	return 5.f;
}

float UStudentPerceptor::ApplyContextModifier(float base, const EItemType& ItemType)
{
	if (!m_Health || !m_Stamina)
	{
		UE_LOG(LogTemp, Error, TEXT("[Perceptor] Context modifier called before stats cached."));
		return base;
	}

	float score = base;

	// --- Emergency multipliers (3× when the corresponding stat is critical).
	// Threshold of 3 on a 0-10 scale = "below 30%". The non-linear cliff is
	// intentional for now: states need a clear signal to switch behavior.
	if (ItemType == EItemType::Medkit && m_Health->GetHealth() < 3)
	{
		score *= 3.f;
	}
	else if (ItemType == EItemType::Food && m_Stamina->GetCurrentStamina() < 3.f)
	{
		score *= 3.f;
	}
	// --- Weapon urgency: a survivor with NO weapon should grab the first
	// one they see, even over a house. 4× on a base of 6 = 24, which beats
	// the baseline House score of 20.
	else if ((ItemType == EItemType::Pistol || ItemType == EItemType::Shotgun)
	         && !SurvivorHasWeapon())
	{
		score *= 4.f;
	}
	return score;
}

bool UStudentPerceptor::SurvivorHasWeapon() const
{
	if (!m_Inventory) return false;

	for (const ABaseItem* Item : m_Inventory->GetInventory())
	{
		if (!Item) continue;
		const EItemType T = Item->GetItemType();
		if (T == EItemType::Pistol || T == EItemType::Shotgun)
		{
			return true;
		}
	}
	return false;
}

void UStudentPerceptor::ChangeColor()
{
	if (CurrentColor)
	{
		if (*CurrentColor == PulsingColor1)
		{
			CurrentColor = &PulsingColor2;
		}
		else
		{
			CurrentColor = &PulsingColor1;
		}
	}
}


TArray<ABaseZombie*> UStudentPerceptor::GetVisibleZombies()
{
	TArray<ABaseZombie*> Result;
	Result.Reserve(m_VisibleZombies.Num());

	for (auto It = m_VisibleZombies.CreateIterator(); It; ++It)
	{
		if (ABaseZombie * Z = (*It).Get())
		{
			Result.Add(Z);
		}
		else
		{
			It.RemoveCurrent(); // weak ptr expired — clean up the set
		}
	}
	return Result;
}

void UStudentPerceptor::ForgetInterestPoints(const FInterestPoint& InterestPoint)
{
	m_WannaPointsInBrain.Remove(InterestPoint);
}

UBlackboardComponent * UStudentPerceptor::GetBlackboard() const
{
	APawn * Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;
	AAIController * AI = Cast<AAIController>(Pawn->GetController());
	return AI ? AI->GetBlackboardComponent() : nullptr;
}

void UStudentPerceptor::LogUnperceivedZombies() const
{
	UWorld* W = GetWorld();
	if (!W) return;

	int32 TotalZombies = 0;
	int32 BadZombies   = 0;

	// Iterate every ABaseZombie in the world. TActorIterator is the
	// canonical way; UGameplayStatics::GetAllActorsOfClass also works but
	// allocates an array we don't need.
	for (TActorIterator<ABaseZombie> It(W); It; ++It)
	{
		ABaseZombie* Z = *It;
		if (!IsValid(Z)) continue;
		++TotalZombies;

		// Look for a stimuli source component. This is the standard way
		// to make an actor visible to AI perception. Manual registration
		// via UAIPerceptionSystem::RegisterPerceptionStimuliSource also
		// works but is rarer — if you use that elsewhere, this audit will
		// false-positive that zombie.
		UAIPerceptionStimuliSourceComponent* Source =
			Z->FindComponentByClass<UAIPerceptionStimuliSourceComponent>();

		if (!Source)
		{
			++BadZombies;
			UE_LOG(LogTemp, Error,
				TEXT("[Audit] %s has NO AIPerceptionStimuliSourceComponent — "
				     "survivor cannot see this zombie."),
				*Z->GetName());
			continue;
		}

		// bAutoRegister and RegisterAsSourceForSenses are protected members
		// (BlueprintReadOnly but not C++-public), so we can't read them
		// directly. Use reflection to peek at the protected UPROPERTYs.
		const UClass* SourceClass = Source->GetClass();

		// --- bAutoRegister (uint32 bitfield) -----------------------------
		if (const FBoolProperty* AutoProp =
			FindFProperty<FBoolProperty>(SourceClass, TEXT("bAutoRegister")))
		{
			if (!AutoProp->GetPropertyValue_InContainer(Source))
			{
				++BadZombies;
				UE_LOG(LogTemp, Error,
					TEXT("[Audit] %s: bAutoRegister=false. "
					     "Tick 'Auto Register as Source' in the editor."),
					*Z->GetName());
				continue;
			}
		}

		// --- RegisterAsSourceForSenses (TArray<TSubclassOf<UAISense>>) ---
		bool bRegistersSight = false;
		if (const FArrayProperty* ArrProp =
			FindFProperty<FArrayProperty>(SourceClass, TEXT("RegisterAsSourceForSenses")))
		{
			FScriptArrayHelper Helper(ArrProp,
				ArrProp->ContainerPtrToValuePtr<void>(Source));
			for (int32 i = 0; i < Helper.Num(); ++i)
			{
				const UClass* SenseClass =
					*reinterpret_cast<UClass* const*>(Helper.GetRawPtr(i));
				if (SenseClass == UAISense_Sight::StaticClass())
				{
					bRegistersSight = true;
					break;
				}
			}
		}

		if (!bRegistersSight)
		{
			++BadZombies;
			UE_LOG(LogTemp, Error,
				TEXT("[Audit] %s registers as a source, but NOT for AISense_Sight. "
				     "Add AISense_Sight to 'Register as Source for Senses'."),
				*Z->GetName());
			continue;
		}
	}

	if (BadZombies == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Audit] All %d ABaseZombie actors are properly registered for Sight."),
			TotalZombies);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Audit] Zombie audit: %d total, %d NOT perceivable for Sight."),
			TotalZombies, BadZombies);
	}
}

void UStudentPerceptor::Suicide()
{
	if (UBlackboardComponent* BB = GetBlackboard())
	{
		UE_LOG(LogTemp,Warning,TEXT("Suiciding"))
		BB->SetValueAsBool(BBKeys::bShouldSuicide,true);
	}
	
}

void UStudentPerceptor::AddZombiesToMemory()
{
	// Perception fallback: when AIPerception's sight sense misses a nearby
	// zombie (one-frame occlusion, edge-of-cone flutter, slow update tick),
	// manually scan all ABaseZombie actors in the world and add anything
	// within DetectionRadius to the visible set. No occlusion check —
	// strictly Euclidean distance. Think "they're close enough that the
	// survivor would hear/feel them even without a clean line of sight."


	UWorld * W = GetWorld();
	if (!W) return;
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	DRAW_CIRCLE(GetWorld(), GetOwner()->GetActorLocation(), m_EnemyDetectionRadius, FColor::White, 3.f);
	for (const auto& Zombie : m_VisibleZombies)
	{
		if (!Zombie.IsValid()) continue;

		DRAW_CIRCLE(GetWorld(), Zombie->GetActorLocation(), 30.f, FColor::Emerald, 3.f);
	}

	//  if there is a wall 
	const FVector MyLoc = OwnerActor->GetActorLocation();
	float DetectionRadiusSq = m_EnemyDetectionRadius * m_EnemyDetectionRadius;

	bool bAnyZombieInRange = false;

	for (TActorIterator<ABaseZombie> It(W); It; ++It)
	{
		ABaseZombie* Z = *It;
		if (!IsValid(Z)) continue;
		const float DistSq = FVector::DistSquared(MyLoc, Z->GetActorLocation());
		if (DistSq <= DetectionRadiusSq)
		{
			m_VisibleZombies.Add(Z);
			bAnyZombieInRange = true;
			UBlackboardComponent* BB = GetBlackboard();

			if (!BB->GetValueAsBool(BBKeys::bShouldSuicide))
			{
				BB->SetValueAsBool(BBKeys::bThreatNearby, true);
			}
			else
			{
				//Suicide is the end makes no sense to make Any calculations of enemies 
				return;
			}
		}
		else
		{
			m_VisibleZombies.Remove(Z);
		}
	}
	
	
	
//	UE_LOG(LogTemp,Warning,TEXT(" Zombies in range %b"),bAnyZombieInRange)
	
	
	// if (!bAnyZombieInRange) return;
	// UBlackboardComponent * BB = GetBlackboard();
	// if (!BB) return;
	// bool ShouldSuicide = BB->GetValueAsBool(BBKeys::bShouldSuicide);
	// if (ShouldSuicide) return;
	// BB->SetValueAsBool(BBKeys::bThreatNearby, true);

	
}

// TArray<AActor*> UStudentPerceptor::GetSeenActorsInMemory()
// {
// 	TArray<AActor*> Actors;
// 	if (!m_PerceptionComponent) return Actors;
// 	m_PerceptionComponent->GetKnownPerceivedActors(
// 		UAISense_Sight::StaticClass(),
// 		Actors
// 	);
// 	return Actors;
// }
//
// void UStudentPerceptor::ForgetActorsFromMemory(AActor* Actor)
// {
// 	if (!m_PerceptionComponent) return;
// 	m_PerceptionComponent->ForgetActor(Actor);
// }

//FAIStimulus Stimulus
// Perception->ForgetAll();
//Perception->RequestStimuliListenerUpdate();
// const FActorPerceptionInfo* Info = Perception->GetActorInfo(*Actor);
// Perception->HasAnyActiveStimulus(*Actor)
// Perception->GetDominantSense()
// Perception->SetSenseEnabled(UAISense_Sight::StaticClass(),false);
// FActorPerceptionBlueprintInfo Info;

//Stimulus.Type             // FAISenseID — which sense fired (Sight, Hearing, Damage, Touch, Team, Prediction)//Stimulus.StimulusLocation // FVector — world location of the stimulus (where the noise was, where the actor was seen)//Stimulus.ReceiverLocation // FVector — where the perceiver was when it sensed it
//Stimulus.Strength         // float — sense-specific intensity (loudness, damage amount, etc.)
//Stimulus.Age              // float — seconds since this stimulus was registered
//Stimulus.ExpirationAge    // float — when the stimulus will be discarded
//Stimulus.Tag              // FName — optional sense-specific tag (e.g., hearing event tag)
