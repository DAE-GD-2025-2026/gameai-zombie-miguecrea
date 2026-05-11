// Fill out your copyright notice in the Description page of Project Settings.


#include "SteeringComponent.h"
#include "SurvivorAIController.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameAI_Zombie/GameManagement/ZombieGameMode.h"
#include "TimerManager.h"
#include "../SpectatorComponent/SpectatorFollowComponent.h"
#include "../MACROS/DebugMacro.h"

USteeringComponent::USteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USteeringComponent::BeginPlay()
{
	Super::BeginPlay();
	m_PawnOwner = Cast<APawn>(GetOwner());
	APawn * m_SpectatorPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	
	// if (m_SpectatorPawn)
	// {
	// 	USpectatorFollowComponent * FollowComp =
	//    NewObject<USpectatorFollowComponent>(m_SpectatorPawn);
	// 	if (FollowComp)
	// 	{
	// 		m_SpectatorPawn->AddOwnedComponent(FollowComp);
	// 		FollowComp->RegisterComponent();
	// 	}
	// }

	
	if (m_PawnOwner)
	{
		m_AIController = Cast<ASurvivorAIController>(m_PawnOwner->GetController());
		
		if (m_AIController)
		{
			UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller created"));

			
			
			
			FAIMoveRequest MoveReq;

			FVector TargetLocation{0.f, 7000.f, 35.f};
			MoveReq.SetGoalLocation(TargetLocation);
			MoveReq.SetAcceptanceRadius(120.f);
			MoveReq.SetUsePathfinding(true);
			MoveReq.SetAllowPartialPath(true);
			MoveReq.SetProjectGoalLocation(true);
			MoveReq.SetReachTestIncludesAgentRadius(true);
			MoveReq.SetCanStrafe(true);
			// MoveReq.SetNavigationFilter(MyFilterClass);

			EPathFollowingRequestResult::Type Result = m_AIController->MoveTo(MoveReq, &m_NavPath);

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

				UE_LOG(LogTemp, Error, TEXT("Survivor AI Controller Failed"));
				// Usually no navmesh, no path, or invalid goal
				break;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AI Controller Is Null"));
		}
	}
}

void USteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	
	FNavPathSharedPtr LivePath = m_AIController->GetPathFollowingComponent()->GetPath();
	
	if (LivePath.IsValid())
	{
		const TArray<FNavPathPoint> & Points = LivePath->GetPathPoints();
		float TotalLength = LivePath->GetLength();
		bool bPartial = LivePath->IsPartial();
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
				DRAW_VECTOR(GetWorld(),Points[i].Location,Points[i + 1].Location, FColor::Yellow)
				DRAW_CIRCLE(GetWorld(),Points[i].Location, 20.f,FColor::Red,3.f);
		}
	}
	
	
	if (m_Rotate)
	{
		if (m_PawnOwner)
		{
			m_PawnOwner->AddActorLocalRotation(
				FRotator(0.f, 90.f * DeltaTime, 0.f)
			);
		}
	}
}

 