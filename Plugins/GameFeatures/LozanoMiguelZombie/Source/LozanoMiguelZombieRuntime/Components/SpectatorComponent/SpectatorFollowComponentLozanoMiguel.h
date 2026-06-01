// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectatorFollowComponentLozanoMiguel.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API USpectatorFollowComponentLozanoMiguel : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USpectatorFollowComponentLozanoMiguel();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void CheckParent();

	AActor * m_Owner;
	AActor * m_FollowPawn;
	bool m_Detached = false;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
