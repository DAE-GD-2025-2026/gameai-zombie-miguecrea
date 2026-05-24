#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include <optional>
#include "LootState.generated.h"

class USteeringComponent;
class UInventoryComponent;
class UStudentPerceptor;
class USoundBase;
class ABaseItem;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API ULootState : public UStateBase
{
	GENERATED_BODY()
public:
	ULootState();
protected:
	// Mini FSM inside the Loot state. Two flows:
	//   1) 2/3 of entries: AlignToItem -> Looting
	//   2) 1/3 of entries: ScanAlign -> Scanning -> AlignToItem -> Looting
	enum class ELootPhase : uint8
	{
		ScanAlign,
		Scanning,
		AlignToItem,
		Looting
	};

	TWeakObjectPtr<USteeringComponent>     Steering;
	TWeakObjectPtr<UInventoryComponent>    Inventory;
	TWeakObjectPtr<UStudentPerceptor>      Memory;

	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;

	void ResumeWandering();

	// --- Tuning -------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category="Loot", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ScanProbability = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanSweepHalfAngleDeg = 80.f;

	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float ScanSweepFrequency = 3.14f;

	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float RotationInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float AlignToleranceDeg = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="Loot")
	float LootDuration = 0.3f;

private:
	ELootPhase m_Phase           = ELootPhase::AlignToItem;
	FVector    m_ItemLocation    = FVector::ZeroVector;
	FRotator   m_DesiredRotation = FRotator::ZeroRotator;
	FRotator   m_ScanBaseRotation= FRotator::ZeroRotator;
	float      m_ScanElapsed     = 0.f;
	float      m_LootTimer       = 0.f;
	std::optional<int> m_TargetInventoryIndex;

	UPROPERTY()
	TObjectPtr<USoundBase> m_LootSound = nullptr;

	void TickAlignToward(float Dt, AActor* Owner, const FRotator& Target) const;
	bool IsAlignedTo(AActor* Owner, const FRotator& Target) const;
	bool WillIGrabThisItem(ABaseItem* Item);
	void GrabItem(int32 Slot);
};
