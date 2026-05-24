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
#include "PurgeZones/PurgeZone.h"


UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* W = GetWorld())
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

	m_Health = Owner->FindComponentByClass<UHealthComponent>();
	m_Stamina = Owner->FindComponentByClass<UStaminaComponent>();
	m_Inventory = Owner->FindComponentByClass<UInventoryComponent>();
	m_PerceptionComponent = Owner->FindComponentByClass<UAIPerceptionComponent>();

	if (!m_Health) UE_LOG(LogTemp, Error, TEXT("[Perceptor] HealthComponent not found."));
	if (!m_Stamina) UE_LOG(LogTemp, Error, TEXT("[Perceptor] StaminaComponent not found."));
	if (!m_Inventory) UE_LOG(LogTemp, Warning,
	                         TEXT("[Perceptor] InventoryComponent not found — weapon-urgency disabled."));

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
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	AddZombiesToMemory();
	UpdatePurgeZoneFlag();

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
			m_bWasStaminaLow = false; 
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

	for (const FInterestPoint& InterestPoint : m_WannaPointsInBrain)
	{
		AActor* A = InterestPoint.Actor.Get();
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
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
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
		InterestPoint.Actor = Actor;
		InterestPoint.m_Visited = false;
		m_WannaPointsInBrain.AddUnique(InterestPoint);
	}
}

void UStudentPerceptor::OnTargetForgotten(AActor* Actor)
{
	// when Stimuli expired age is 5 seconds will forget after 5 seconds 
	UE_LOG(LogTemp, Error, TEXT("Target Forgotten"))
	if (ABaseZombie* Z = Cast<ABaseZombie>(Actor))
	{
		m_VisibleZombies.Remove(Z);
	}
}


FInterestPoint * UStudentPerceptor::GetBestInterestPoint()
{
	FInterestPoint* BestInterestPoint = nullptr;

	if (m_WannaPointsInBrain.IsEmpty())
		return nullptr;

	AActor* OwnerActor = GetOwner();
	const FVector MyLoc = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;

	float BestScore = 0.f;
	for (FInterestPoint & InterestPoint : m_WannaPointsInBrain)
	{
		if (InterestPoint.m_Visited)
			continue;

		AActor * Target = InterestPoint.Actor.Get();
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


FInterestPoint* UStudentPerceptor::GetClosestWeaponInMemory()
{
	// Scan m_WannaPointsInBrain for the closest unvisited weapon (Pistol or
	// Shotgun). Returns nullptr if memory holds no weapons or none are
	// currently valid.
	if (m_WannaPointsInBrain.IsEmpty()) return nullptr;

	AActor* OwnerActor = GetOwner();
	const FVector MyLoc =
		OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;

	FInterestPoint* Closest = nullptr;
	float ClosestDistSq     = TNumericLimits<float>::Max();

	for (FInterestPoint& IP : m_WannaPointsInBrain)
	{
		if (IP.m_Visited) continue;

		AActor* Target = IP.Actor.Get();
		if (!Target) continue;                       // weak ptr expired

		const ABaseItem* Item = Cast<ABaseItem>(Target);
		if (!Item) continue;                         // house / not an item

		const EItemType T = Item->GetItemType();
		if (T != EItemType::Pistol && T != EItemType::Shotgun)
			continue;                                // not a weapon

		// Squared distance — avoids a sqrt per candidate.
		const float DistSq = FVector::DistSquared(MyLoc, Target->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = &IP;
		}
	}

	return Closest;
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

	if (ItemType == EItemType::Medkit && m_Health->GetHealth() < 3)
	{
		score *= 3.f;
	}
	else if (ItemType == EItemType::Food && m_Stamina->GetCurrentStamina() < 3.f)
	{
		score *= 3.f;
	}

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
		if (ABaseZombie* Z = (*It).Get())
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

UBlackboardComponent* UStudentPerceptor::GetBlackboard() const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;
	AAIController* AI = Cast<AAIController>(Pawn->GetController());
	return AI ? AI->GetBlackboardComponent() : nullptr;
}

void UStudentPerceptor::LogUnperceivedZombies() const
{
	UWorld* W = GetWorld();
	if (!W) return;

	int32 TotalZombies = 0;
	int32 BadZombies = 0;


	for (TActorIterator<ABaseZombie> It(W); It; ++It)
	{
		ABaseZombie* Z = *It;
		if (!IsValid(Z)) continue;
		++TotalZombies;

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
		UE_LOG(LogTemp, Warning, TEXT("Suiciding"))
		BB->SetValueAsBool(BBKeys::bShouldSuicide, true);
	}
}

void UStudentPerceptor::UpdatePurgeZoneFlag()
{
	UBlackboardComponent* BB = GetBlackboard();
	if (!BB) return;

	UWorld* W = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!W || !OwnerActor)
	{
		BB->SetValueAsBool(BBKeys::bPurgeZoneNearby, false);
		return;
	}

	const FVector MyLoc = OwnerActor->GetActorLocation();

	// Hysteresis: if we're already flagged as in-danger, require the larger
	// exit-buffer before clearing. If we're not flagged, the smaller
	// enter-buffer triggers the flag. The gap between them is what stops
	// Flee↔Wander bouncing at the boundary.
	const bool bWasInDanger = BB->GetValueAsBool(BBKeys::bPurgeZoneNearby);
	const float ActiveBuffer = bWasInDanger ? PurgeZoneExitBuffer : PurgeZoneEnterBuffer;

	bool bAnyInRange = false;
	for (TActorIterator<APurgeZone> It(W); It; ++It)
	{
		APurgeZone* Z = *It;
		if (!IsValid(Z)) continue;

		// Diameter is protected on APurgeZone — read via reflection.
		float Diameter = 0.f;
		if (const FFloatProperty* DiamProp =
			FindFProperty<FFloatProperty>(Z->GetClass(), TEXT("Diameter")))
		{
			Diameter = DiamProp->GetPropertyValue_InContainer(Z);
		}

		const float DangerRadius = Diameter * 0.5f + ActiveBuffer;
		const float DistSq       = FVector::DistSquared(MyLoc, Z->GetActorLocation());
		if (DistSq <= DangerRadius * DangerRadius)
		{
			bAnyInRange = true;
			break;  // one is enough
		}
	}

	BB->SetValueAsBool(BBKeys::bPurgeZoneNearby, bAnyInRange);
}

void UStudentPerceptor::AddZombiesToMemory()
{
	UWorld* W = GetWorld();
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
}
