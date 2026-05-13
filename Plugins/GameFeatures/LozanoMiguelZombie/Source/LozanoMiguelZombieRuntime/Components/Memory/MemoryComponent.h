// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MemoryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMemoryComponent();
protected:
	virtual void BeginPlay() override;
	
	TArray<AActor*> m_MemoryActors;
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction * ThisTickFunction) override;
	
	
	// virtual void OnRegister() override;
	 //virtual void InitializeComponent() override;
	// virtual void UninitializeComponent() override;
	//
	// virtual void OnUnregister() override;
	//
	// virtual void OnComponentCreated() override;
	// virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//PICK BEST INTEREST 
	
	//ADD TO MEMORY 
	
};
