// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SteeringComponent.generated.h"

class ASurvivorAIController;
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API USteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USteeringComponent();
	void Move(const FVector & ToLocation);
protected:
	virtual void BeginPlay() override;
	void RenderPath();

	
	ASurvivorAIController * m_AIController;
	APawn * m_PawnOwner;
	FNavPathSharedPtr  m_NavPath;
	
	
	UPROPERTY(VisibleAnywhere, Category = "Steering")
	bool m_Rotate  = true;
	
	UPROPERTY(EditAnywhere, Category = "Steering")
	float m_ManualRotationSpeed = 180.f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float m_FaceVelocitySpeed = 6.f;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
