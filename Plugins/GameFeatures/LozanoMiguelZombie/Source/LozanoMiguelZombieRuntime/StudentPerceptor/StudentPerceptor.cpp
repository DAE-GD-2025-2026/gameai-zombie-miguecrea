// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();

	//this component get added at runtime 
	
	
	 //m_SurvivorPawn =  Cast<ASurvivorPawn>(this->GetOwner());
	
	UE_LOG(LogTemp,Warning,TEXT("Hello"))
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
}


void UStudentPerceptor::OnPerceptionUpdated(AActor * Actor,FAIStimulus Stimulus)
{
	
	bool isValid = false;


	//items , 

	//PurgeZone 

	 //House 
	
	//Zombie 
	UE_LOG(LogTemp, Warning, TEXT("Saw Something %s"), *Actor->GetName())

}
