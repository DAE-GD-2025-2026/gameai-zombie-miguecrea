// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Items/ItemType.h"
#include "../FSM/InterestPoint.h"
#include "../FSM/ReasonToMove.h"
#include "StudentPerceptor.generated.h"

class ASurvivorPawn;
class ABaseZombie;


DECLARE_MULTICAST_DELEGATE(UStaminaLow);
DECLARE_MULTICAST_DELEGATE(UHealthLow);


class UAIPerceptionComponent;
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()
	
	
private:
	
	 
	UAIPerceptionComponent * m_PerceptionComponent;
	ASurvivorPawn * m_SurvivorPawn = nullptr;
public:
	
	UStaminaLow m_StaminaLow;
	UHealthLow m_HealthLow;
	UStudentPerceptor();
	virtual void BeginPlay() override;
	void TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction) override;
	
	
	float m_EnemyDetectionRadius = 190.f;

	// --- Needs (low-stat) edge detection ----------------------------------
	// These are continuous polls in Tick. We fire the delegate exactly once
	// on the falling edge (value crosses below the *Enter threshold).
	// The flag mirrors physical state ("am I currently low?") and re-arms
	// automatically when value climbs above the *Exit threshold — the gap
	// between Enter and Exit is hysteresis to prevent flapping.

	UPROPERTY(EditDefaultsOnly, Category="Needs")
	float StaminaLowEnter = 5.f;
	UPROPERTY(EditDefaultsOnly, Category="Needs")
	float StaminaLowExit  = 7.f;

	UPROPERTY(EditDefaultsOnly, Category="Needs")
	int   HealthLowEnter  = 2;
	UPROPERTY(EditDefaultsOnly, Category="Needs")
	int   HealthLowExit   = 4;

	bool m_bWasStaminaLow = false;
	bool m_bWasHealthLow  = false;

///////
	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION()
	void OnTargetForgotten(AActor* Actor);
	
	void DeferredInit();
	
	FInterestPoint * GetBestInterestPoint();
	
	float GetItemBaseUtility(EItemType type);
	float GetHouseBaseUtility();
	float ApplyContextModifier(float base,const EItemType & ItemType);

	// True iff the survivor's inventory currently contains any weapon
	// (Pistol or Shotgun). Used by ApplyContextModifier to boost weapon
	// utility when the survivor is unarmed.
	bool SurvivorHasWeapon() const;

	// Hard cutoff for distance falloff. Items further than this contribute
	// zero utility — used so the survivor doesn't path across the map for a
	// trivial pickup.
	UPROPERTY(EditDefaultsOnly, Category = "Utility")
	float MaxConsiderDistance = 5000.f;

	FColor PulsingColor1 = FColor::Blue;
	FColor PulsingColor2 = FColor::Yellow;
	FColor * CurrentColor =  &PulsingColor1;
	
	void ChangeColor();
	
	// I want to grab these Now 
	TArray<FInterestPoint> m_WannaPointsInBrain;
	//wont need now but later might come back
	TArray<FInterestPoint> m_SaveForLaterPoints;
	
	TSet<AActor*> m_IgnoredActors; // Actors that I want to not put on memory 
	//those you grabbed 
	

	TArray<ABaseZombie*> GetVisibleZombies ();

	class UBlackboardComponent * GetBlackboard() const;

	void ForgetInterestPoints(const FInterestPoint& InterestPoint);

	// --- Debug: zombie perception audit -----------------------------------
	// Periodic scan of all ABaseZombie actors in the world. Any zombie that
	// lacks an AIPerceptionStimuliSourceComponent (or has one but isn't
	// registered for Sight) is logged — those zombies are invisible to the
	// survivor's perception regardless of distance/FOV.

	UPROPERTY(EditDefaultsOnly, Category="Debug")
	float ZombieAuditInterval = 5.f;

	float m_ZombieAuditTimer = 0.f;

	void LogUnperceivedZombies() const;
	
	void Suicide();
	
	void AddZombiesToMemory();

private:

	class UHealthComponent    * m_Health    = nullptr;
	class UStaminaComponent   * m_Stamina   = nullptr;
	class UInventoryComponent * m_Inventory = nullptr;

	// Event-driven mirror of "which zombies are currently visible". Updated
	// from OnPerceptionUpdated (add on success, remove on failure) and
	// OnTargetForgotten (remove). Weak ptrs so dead zombies prune themselves.
	// GetCurrentlyPerceivedActors is too laggy to rely on per-tick — sight
	// updates fire on the perception system's own clock, not ours.
	TSet<TWeakObjectPtr<ABaseZombie>> m_VisibleZombies;
};
