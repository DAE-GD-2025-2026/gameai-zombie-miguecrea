// Fill out your copyright notice in the Description page of Project Settings.


#include "SteeringComponent.h"
#include "SurvivorAIController.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpectatorPawn.h"

USteeringComponent::USteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}
void USteeringComponent::BeginPlay()
{
	Super::BeginPlay();
	APawn * PawnOwner = Cast<APawn>(GetOwner());
	
	
	AGameModeBase* GM = GetWorld()->GetAuthGameMode();

	if (GM)
	{
		UE_LOG(LogTemp,Error,TEXT("GameMode Is Okay "));
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("GameMode "));
	}
	
	////////////////////////////////
	UFloatingPawnMovement * Floating =
	Cast<UFloatingPawnMovement>(
		PawnOwner->GetComponentByClass(UFloatingPawnMovement::StaticClass())
	);
	Floating->MaxSpeed = Floating->GetMaxSpeed() * 0.1f;
	if (PawnOwner)
	{
		m_AIController = Cast<ASurvivorAIController>(PawnOwner->GetController());
		if (m_AIController)
		{
			UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller created"));
			
			FAIMoveRequest MoveReq;
			
			FVector TargetLocation {200.f,4000.f,35.f};
			MoveReq.SetGoalLocation(TargetLocation);
			MoveReq.SetAcceptanceRadius(120.f);
			MoveReq.SetUsePathfinding(true);
			MoveReq.SetAllowPartialPath(true);
			MoveReq.SetProjectGoalLocation(true);
			MoveReq.SetReachTestIncludesAgentRadius(true);
			MoveReq.SetCanStrafe(true);
			// MoveReq.SetNavigationFilter(MyFilterClass);

			FNavPathSharedPtr NavPath;  //Alt Enter 
			EPathFollowingRequestResult::Type Result = m_AIController->MoveTo(MoveReq, &NavPath);

			 switch (Result)
			 {
			 case EPathFollowingRequestResult::RequestSuccessful:
			 	
			 	UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller Moved"));
			 	// Path computed, agent is now moving
			 	break;
			 case EPathFollowingRequestResult::AlreadyAtGoal:
			 	
			 	UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller AlreadyAtGoal"));
			 	
			 	// No movement needed
			 	break;
			 case EPathFollowingRequestResult::Failed:
			 	
			 	UE_LOG(LogTemp,Error, TEXT("Survivor AI Controller Failed"));
			 	
			 	// Usually no navmesh, no path, or invalid goal
			 	break;
			 }
			
	
		}
		else
		{
			UE_LOG(LogTemp,Error, TEXT("AI Controller Is Null"));
		}
	}
	
	
	
}
void USteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

