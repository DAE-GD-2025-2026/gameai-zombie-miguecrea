// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/BaseItem.h"
#include "InventoryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEAI_ZOMBIE_API UInventoryComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	
	UFUNCTION()
	bool GrabItem(int SlotIdx, ABaseItem * Item);
	UFUNCTION()
	bool UseItem(int SlotIdx);
	UFUNCTION()
	bool RemoveItem(int SlotIdx);
	UFUNCTION()
	int GetInventoryCapacity() const {return Items.Num();}
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<ABaseItem*> const & GetInventory() const {return Items;}

	UFUNCTION()
	float GetPickupRange() const {return PickupRange;}
	
protected:
	UPROPERTY()
	TArray<ABaseItem*> Items{};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory")
	float PickupRange{100.0f};	
};
