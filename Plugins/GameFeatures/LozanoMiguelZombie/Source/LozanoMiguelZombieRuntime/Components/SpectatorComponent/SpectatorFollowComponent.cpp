// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectatorFollowComponent.h"


USpectatorFollowComponent::USpectatorFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void USpectatorFollowComponent::BeginPlay()
{ 
	Super::BeginPlay();
	UE_LOG(LogTemp,Error, TEXT("SpectatorFollowComponent::BeginPlay()"));
}


void USpectatorFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UE_LOG(LogTemp,Error, TEXT("SpectatorFollowComponent::BeginPlay()"));
}

