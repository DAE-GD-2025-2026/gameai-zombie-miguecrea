// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Items/ItemType.h"
#include "StudentPerceptor.generated.h"


class ASurvivorPawn;

USTRUCT(BlueprintType)
struct FInterestPoint
{
	GENERATED_BODY()

	// Weak so this struct doesn't keep destroyed items alive. Callers must
	// check .IsValid() before dereferencing — anything that read raw Actor*
	// before is a potential dangling-pointer crash if the item gets picked
	// up by another survivor or otherwise despawns.
	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY()
	bool m_Visited = false;


	bool operator==(const FInterestPoint& Other) const
	{
		// TWeakObjectPtr::operator== compares the underlying object identity
		// (and the SerialNumber), so AddUnique still de-dupes correctly.
		return Actor == Other.Actor;
	}
};

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
	
	std::optional<FInterestPoint>GetBestInterestPoint();
	
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
	
	TArray<AActor*> GetSeenActorsInMemory(); 
	TArray<AActor*> GetActorsOnFOV();
	void ForgetActorsFromMemory(AActor * Actor);
	
private:

	class UHealthComponent    * m_Health    = nullptr;
	class UStaminaComponent   * m_Stamina   = nullptr;
	class UInventoryComponent * m_Inventory = nullptr;
};
