#pragma once

#include "CoreMinimal.h"
#include "StateBaseLozanoMiguel.h"
#include <optional>
#include "LootStateLozanoMiguel.generated.h"

class USteeringComponentLozanoMiguel;
class UInventoryComponent;
class UStudentPerceptorLozanoMiguel;
class USoundBase;
class ABaseItem;

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API ULootStateLozanoMiguel : public UStateBaseLozanoMiguel
{
	GENERATED_BODY()
public:
	ULootStateLozanoMiguel();
protected:
	enum class ELootPhase : uint8
	{
		ScanAlign,
		Scanning,
		AlignToItem,
		Looting
	};

	TWeakObjectPtr<USteeringComponentLozanoMiguel> Steering;
	TWeakObjectPtr<UInventoryComponent>            Inventory;
	TWeakObjectPtr<UStudentPerceptorLozanoMiguel>  Memory;

	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;

	void ResumeWandering();

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
