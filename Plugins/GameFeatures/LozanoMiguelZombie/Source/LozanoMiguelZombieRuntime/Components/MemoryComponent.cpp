// Fill out your copyright notice in the Description page of Project Settings.


#include "MemoryComponent.h"


// Sets default values for this component's properties
UMemoryComponent::UMemoryComponent()
{

	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void UMemoryComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT(" MemoryComponentInitialized"))
	
}


// Called every frame
void UMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

