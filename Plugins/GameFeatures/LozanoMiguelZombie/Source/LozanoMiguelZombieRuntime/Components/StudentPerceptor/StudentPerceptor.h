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
#include "StudentPerceptor.generated.h"

class ASurvivorPawn;
class ABaseZombie;


class UAIPerceptionComponent;
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()
	
	
private:
	UAIPerceptionComponent * m_PerceptionComponent;

	ASurvivorPawn * m_SurvivorPawn = nullptr;
public:
	UStudentPerceptor();
	virtual void BeginPlay() override;
	void TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction) override;

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
	
	TArray<FInterestPoint> m_UnvisitedInterestPointsInBrain;
	TSet<AActor*> m_IgnoredActors; // Actors that I want to not put on memory 
	//those you grabbed 
	

	TArray<ABaseZombie*> GetVisibleZombies ();
	
	class UBlackboardComponent * GetBlackboard() const;

	void ForgetInterestPoints(const FInterestPoint& InterestPoint);

private:

	class UHealthComponent    * m_Health    = nullptr;
	class UStaminaComponent   * m_Stamina   = nullptr;
	class UInventoryComponent * m_Inventory = nullptr;
};
