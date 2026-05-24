// Fill out your copyright notice in the Description page of Project Settings.


#include "SteeringComponent.h"
#include "SurvivorAIController.h"
#include "AITypes.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameAI_Zombie/GameManagement/ZombieGameMode.h"
#include "TimerManager.h"
#include "../SpectatorComponent/SpectatorFollowComponent.h"
#include "../MACROS/DebugMacro.h"
#include "Survivor/SurvivorPawn.h"
#include "NavigationPath.h"

USteeringComponent::USteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USteeringComponent::Move(const FVector & ToLocation)
{
	if (!m_AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("Move Failed -> AIController is NULL"));
		return;
	}

	APawn* ControlledPawn = m_AIController->GetPawn();

	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("Move Failed -> AIController has no possessed pawn"));
		return;
	}

	// --------------------------------------------------------------------
	// BASIC LOCATION VALIDATION
	// --------------------------------------------------------------------

	if (ToLocation.ContainsNaN())
	{
		UE_LOG(LogTemp, Error, TEXT("Move Failed -> Destination contains NaN"));
		return;
	}


	// --------------------------------------------------------------------
	// NAVIGATION SYSTEM CHECK
	// --------------------------------------------------------------------

	UNavigationSystemV1 * NavSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Move Failed -> NavigationSystem is NULL"));
		return;
	}


	// --------------------------------------------------------------------
	// TRY TO BUILD A PATH BEFORE MOVING
	// --------------------------------------------------------------------

	UNavigationPath * TestPath =
		NavSystem->FindPathToLocationSynchronously(
			GetWorld(),
			ControlledPawn->GetActorLocation(),
			ToLocation);

	if (!TestPath)
	{
		UE_LOG(LogTemp, Error, TEXT("Move Failed -> Path object is NULL"));
	}

	// --------------------------------------------------------------------
	// MOVEMENT COMPONENT CHECK
	// --------------------------------------------------------------------

	UPawnMovementComponent * MoveComp =
		ControlledPawn->GetMovementComponent();

	if (!MoveComp)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Move Failed -> Pawn has NO movement component"));
	}


	// --------------------------------------------------------------------
	// PATH FOLLOWING COMPONENT CHECK
	// --------------------------------------------------------------------

	if (!m_AIController->GetPathFollowingComponent())
	{
		UE_LOG(LogTemp, Error,
			TEXT("Move Failed -> No PathFollowingComponent"));
	}

	// --------------------------------------------------------------------
	// ACTUAL MOVE REQUEST
	// --------------------------------------------------------------------

	FNavLocation ProjectedLocation;
	// Try to find nearest valid NavMesh position
	bool bProjected =
		NavSystem->ProjectPointToNavigation(
			ToLocation,
			ProjectedLocation,
			FVector(300.f, 300.f, 500.f));

	if (!bProjected)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Move Failed -> Could not project destination to NavMesh"));

		// Optional:
		// fallback to original location OR abort completely
		return;
	}

	// --------------------------------------------------------------------
	// MOVE REQUEST
	// --------------------------------------------------------------------

	FAIMoveRequest MoveReq;

	MoveReq.SetGoalLocation(ProjectedLocation.Location);
	MoveReq.SetAcceptanceRadius(30.f);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(false); // already projected manually
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(true);

	EPathFollowingRequestResult::Type Result =
		m_AIController->MoveTo(MoveReq, &m_NavPath);

	switch (Result)
	{
	case EPathFollowingRequestResult::RequestSuccessful:

		// UE_LOG(LogTemp, Warning,
		// 	TEXT("Move Result -> SUCCESS"));
		break;

	case EPathFollowingRequestResult::AlreadyAtGoal:

		UE_LOG(LogTemp, Warning,
			TEXT("Move Result -> ALREADY AT GOAL"));
		break;

	case EPathFollowingRequestResult::Failed:

		UE_LOG(LogTemp, Error,
			TEXT("Move Result -> FAILED"));

		// Extra useful debugging
		if (m_NavPath.IsValid())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("NavPath exists but move failed"));
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("NavPath INVALID"));
		}

		break;
	}
	
}

void USteeringComponent::SetRotate(bool Rotate)
{
	m_Rotate = Rotate;
}

void USteeringComponent::BeginPlay()
{
	Super::BeginPlay();
	m_PawnOwner = Cast<APawn>(GetOwner());

	if (m_PawnOwner)
	{
		m_PawnOwner->bUseControllerRotationYaw   = false;
		m_PawnOwner->bUseControllerRotationPitch = false;
		m_PawnOwner->bUseControllerRotationRoll  = false;
	}

	GetAI();
}

ASurvivorAIController * USteeringComponent::GetAI()
{
	if (m_AIController) return m_AIController;

	if (!m_PawnOwner) m_PawnOwner = Cast<APawn>(GetOwner());
	if (!m_PawnOwner) return nullptr;

	m_AIController = Cast<ASurvivorAIController>(m_PawnOwner->GetController());
	if (!m_AIController) return nullptr;

	m_AIController->bSetControlRotationFromPawnOrientation = false;
	m_AIController->ClearFocus(EAIFocusPriority::Gameplay); // remove combat override
	m_AIController->ClearFocus(EAIFocusPriority::Default);  // remove baseline
	m_AIController->ClearFocus(EAIFocusPriority::Move);     // remove move-driven
	
	m_AIController->ReceiveMoveCompleted.AddUniqueDynamic(
		this, &USteeringComponent::HandleAIMoveCompleted);

	return m_AIController;
}

void USteeringComponent::HandleAIMoveCompleted(FAIRequestID /*RequestID*/,
                                               EPathFollowingResult::Type Result)
{
	OnMoveCompleted.Broadcast(Result);
}

void USteeringComponent::RenderPath()
{
	ASurvivorAIController * AI = GetAI();
	if (!AI) return;

	UPathFollowingComponent* PFC = AI->GetPathFollowingComponent();
	if (!PFC) return;

	FNavPathSharedPtr LivePath = PFC->GetPath();

	if (LivePath.IsValid())
	{
		const TArray<FNavPathPoint>& Points = LivePath->GetPathPoints();
		float TotalLength = LivePath->GetLength();
		bool bPartial = LivePath->IsPartial();
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			DRAW_VECTOR(GetWorld(), Points[i].Location, Points[i + 1].Location, FColor::Yellow)
			DRAW_CIRCLE(GetWorld(), Points[i].Location, 20.f, FColor::Red, 3.f);
		}
	}
}

void USteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RenderPath();

	if (!m_PawnOwner) return;

	if (m_Rotate)
	{
		m_PawnOwner->AddActorLocalRotation(
			FRotator(0.f, m_ManualRotationSpeed * DeltaTime, 0.f));
	}

}

void USteeringComponent::SetFocus(AActor * ActorToFocus)
{
	m_AIController->SetFocus(ActorToFocus);
}

void USteeringComponent::ClearFocus()
{
	m_AIController->ClearFocus(EAIFocusPriority::Move);
}

void USteeringComponent::StopMoving()
{
	
	if (m_AIController)
	{
		m_AIController->StopMovement();
	}
}


// FVector Vel = m_PawnOwner->GetVelocity();
// if (Vel.SizeSquared() > 100.f)
// {
// 	FRotator Current = m_PawnOwner->GetActorRotation();
// 	FRotator Target = Vel.Rotation();
// 	Target.Pitch = 0.f;
// 	Target.Roll = 0.f;
// 		
// 	FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaTime, m_FaceVelocitySpeed);
// 	m_PawnOwner->SetActorRotation(NewRot);
// }