// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectatorFollowComponent.h"
#include "TimerManager.h"


USpectatorFollowComponent::USpectatorFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void USpectatorFollowComponent::BeginPlay()
{ 
	Super::BeginPlay();
	m_Owner = GetOwner();
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&USpectatorFollowComponent::CheckParent,
		0.1f,
		false
	);
}

void USpectatorFollowComponent::CheckParent()
{
	if (m_Owner)
	{
		//check if it has parent for cleaness
		if (!m_Owner->GetAttachParentActor()) return;
		m_FollowPawn = m_Owner->GetAttachParentActor();
		m_Owner->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));
		m_Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		m_Owner->SetOwner(nullptr);
		m_Detached = true;
	}
}


void USpectatorFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (m_Detached && m_FollowPawn)
	{
		if (m_Owner)
		{
			FVector Current = m_Owner->GetActorLocation();
			FVector TargetLocation = m_FollowPawn->GetActorLocation();
			TargetLocation.Z = Current.Z;
			FVector NewLoc = FMath::VInterpTo(Current, TargetLocation, DeltaTime, /*InterpSpeed=*/5.f);
			m_Owner->SetActorLocation(NewLoc);
		}
	}
}


//FOR TIME LINE 
// Header
// UPROPERTY(EditAnywhere) UCurveFloat* MoveCurve;
// FTimeline MoveTimeline;
//
// // BeginPlay
// if (MoveCurve)
// {
// 	FOnTimelineFloat ProgressFn;
// 	ProgressFn.BindUFunction(this, FName("HandleProgress"));
// 	MoveTimeline.AddInterpFloat(MoveCurve, ProgressFn);
// 	MoveTimeline.SetLooping(false);
// }
//
// // Tick
// MoveTimeline.TickTimeline(DeltaTime);
//
// // Callback
// UFUNCTION()
// void HandleProgress(float Value)
// {
// 	SetActorLocation(FMath::Lerp(StartLoc, EndLoc, Value));
// }
//
// // Start
// void StartMove()
// {
// 	StartLoc = GetActorLocation();
// 	MoveTimeline.PlayFromStart();
// }