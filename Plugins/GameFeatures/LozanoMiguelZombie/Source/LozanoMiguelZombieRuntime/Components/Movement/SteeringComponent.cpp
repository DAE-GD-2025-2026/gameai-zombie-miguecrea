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

void USteeringComponent::Move(const FVector & ToLocation)
{
	FAIMoveRequest MoveReq;

	MoveReq.SetGoalLocation(ToLocation);
	MoveReq.SetAcceptanceRadius(150.f);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(true);
	// MoveReq.SetNavigationFilter(MyFilterClass);

	EPathFollowingRequestResult::Type Result = m_AIController->MoveTo(MoveReq,&m_NavPath);

	switch (Result)
	{
	case EPathFollowingRequestResult::RequestSuccessful:

		UE_LOG(LogTemp, Warning, TEXT("Survivor AI Controller Succesfull"));
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
		m_PawnOwner->bUseControllerRotationYaw = false;
		m_PawnOwner->bUseControllerRotationPitch = false;
		m_PawnOwner->bUseControllerRotationRoll = false;

		m_AIController = Cast<ASurvivorAIController>(m_PawnOwner->GetController());
		
		if (m_AIController)
		{
			m_AIController->bSetControlRotationFromPawnOrientation = false;
			m_AIController->ClearFocus(EAIFocusPriority::Gameplay); // remove combat override
			m_AIController->ClearFocus(EAIFocusPriority::Default); // remove baseline
			m_AIController->ClearFocus(EAIFocusPriority::Move); // remove move-driven
			//Move();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AI Controller Is Null"));
		}
	}
}

void USteeringComponent::RenderPath()
{
	FNavPathSharedPtr LivePath = m_AIController->GetPathFollowingComponent()->GetPath();

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
