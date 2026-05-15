// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.generated.h"

class ASurvivorAIController;

// Broadcast when the AIController's MoveTo request finishes for any reason.
// bSucceeded is true only for EPathFollowingResult::Success — states that
// care about the difference between success / blocked / aborted can handle
// it themselves; most just want "are we done?"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteeringMoveCompleted,EPathFollowingResult::Type,WhatHappened);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API USteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USteeringComponent();
	void Move(const FVector & ToLocation);

	// Bind in your state's OnEnter, RemoveDynamic in OnExit.
	UPROPERTY(BlueprintAssignable, Category = "Steering")
	FOnSteeringMoveCompleted OnMoveCompleted;

	
	
	void SetRotate(bool Rotate);
protected:
	virtual void BeginPlay() override;
	void RenderPath();

	// Lazy controller resolve. Returns null until the pawn has been
	// possessed by an ASurvivorAIController. Once resolved, performs the
	// one-time controller configuration (focus clears, rotation flags),
	// hooks ReceiveMoveCompleted, and caches the pointer.
	ASurvivorAIController * GetAI();

	// Forwards the AIController's path-following completion to our own
	// OnMoveCompleted delegate. UFUNCTION because AddDynamic requires it.
	UFUNCTION()
	void HandleAIMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	ASurvivorAIController * m_AIController = nullptr;
	APawn * m_PawnOwner = nullptr;
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
	void SetFocus(AActor * ActorToFocus);
	void ClearFocus();
};
