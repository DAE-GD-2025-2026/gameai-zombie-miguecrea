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
#include "Survivor/SurvivorPawn.h"

USteeringComponent::USteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USteeringComponent::Move(const FVector & ToLocation)
{
	ASurvivorAIController* AI = GetAI();
	if (!AI)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Steering::Move ignored — AIController not ready yet."));
		return;
	}

	FAIMoveRequest MoveReq;

	MoveReq.SetGoalLocation(ToLocation);
	MoveReq.SetAcceptanceRadius(100.f);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(true);
	// MoveReq.SetNavigationFilter(MyFilterClass);

	EPathFollowingRequestResult::Type Result = AI->MoveTo(MoveReq, &m_NavPath);

	switch (Result)
	{
	case EPathFollowingRequestResult::RequestSuccessful:

		//UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller Succesfull"));
		break;
	case EPathFollowingRequestResult::AlreadyAtGoal:

		UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller AlreadyAtGoal"));
		break;
	case EPathFollowingRequestResult::Failed:

		UE_LOG(LogTemp, Error, TEXT("Survivor AI Controller Failed"));
		break;
	}
	
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

	// Opportunistic warm-up — fine if it fails. GetAI() will retry lazily.
	GetAI();
}

ASurvivorAIController * USteeringComponent::GetAI()
{
	if (m_AIController) return m_AIController;

	if (!m_PawnOwner) m_PawnOwner = Cast<APawn>(GetOwner());
	if (!m_PawnOwner) return nullptr;

	m_AIController = Cast<ASurvivorAIController>(m_PawnOwner->GetController());
	if (!m_AIController) return nullptr;

	// One-time controller config, run the moment the controller is first seen.
	m_AIController->bSetControlRotationFromPawnOrientation = false;
	m_AIController->ClearFocus(EAIFocusPriority::Gameplay); // remove combat override
	m_AIController->ClearFocus(EAIFocusPriority::Default);  // remove baseline
	m_AIController->ClearFocus(EAIFocusPriority::Move);     // remove move-driven

	// Forward MoveTo completion events to our own delegate. AddUniqueDynamic
	// guards against double-binding if GetAI() is ever called twice through
	// some weird code path (it shouldn't, given the m_AIController early-out
	// above, but cheap insurance).
	m_AIController->ReceiveMoveCompleted.AddUniqueDynamic(
		this, &USteeringComponent::HandleAIMoveCompleted);

	return m_AIController;
}

void USteeringComponent::HandleAIMoveCompleted(FAIRequestID /*RequestID*/,
                                               EPathFollowingResult::Type Result)
{
	const bool bSucceeded = (Result == EPathFollowingResult::Success);
	OnMoveCompleted.Broadcast(bSucceeded);
}

void USteeringComponent::RenderPath()
{
	ASurvivorAIController* AI = GetAI();
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
	else
	{
		FVector Vel = m_PawnOwner->GetVelocity();
		if (Vel.SizeSquared() > 100.f)
		{
			FRotator Current = m_PawnOwner->GetActorRotation();
			FRotator Target = Vel.Rotation();
			Target.Pitch = 0.f;
			Target.Roll = 0.f;

			FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaTime, m_FaceVelocitySpeed);
			m_PawnOwner->SetActorRotation(NewRot);
		}
	}
}
