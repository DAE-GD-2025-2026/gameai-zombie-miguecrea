// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"
#include "Survivor/SurvivorPawn.h"

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	m_SurvivorPawn = Cast<ASurvivorPawn>(GetOwner());
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
	//In case I want teetetwtet
	// PrimaryComponentTick.AddPrerequisite(
	// 	Perception,
	// 	Perception->PrimaryComponentTick
	// );
}


void UStudentPerceptor::OnPerceptionUpdated(AActor * Actor,FAIStimulus Stimulus)
{
	// Update the FSM keys 
	bool isValid = false;


	//items , 

	//PurgeZone 

	 //House 
	
	//Zombie 

}
