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

	//this component get added at runtime 
	
	
	m_SurvivorPawn = Cast<ASurvivorPawn>(GetOwner());
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
	
	
	//In case I want 
	// PrimaryComponentTick.AddPrerequisite(
	// 	Perception,
	// 	Perception->PrimaryComponentTick
	// );
}


void UStudentPerceptor::OnPerceptionUpdated(AActor * Actor,FAIStimulus Stimulus)
{
	
	bool isValid = false;


	//items , 

	//PurgeZone 

	 //House 
	
	//Zombie 

}
