// Fill out your copyright notice in the Description page of Project Settings.


#include "MemoryComponent.h"
#include "../FSM/FSMComponent.h"

// Sets default values for this component's properties
UMemoryComponent::UMemoryComponent()
{

	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void UMemoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	

}


// Called every frame
void UMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

