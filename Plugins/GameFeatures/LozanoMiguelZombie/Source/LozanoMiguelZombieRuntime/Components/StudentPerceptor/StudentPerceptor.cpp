// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/InventoryComponent.h"

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
	
	
	auto inv = GetOwner()->GetComponentByClass<UInventoryComponent>();
	
	
	//In case I want teetetwtet
	// PrimaryComponentTick.AddPrerequisite(
	// 	Perception,
	// 	Perception->PrimaryComponentTick
	// );
}


void UStudentPerceptor::OnPerceptionUpdated(AActor * Actor,FAIStimulus Stimulus)
{
	
	 ABaseItem * IsItem = Cast<ABaseItem>(Actor);
	
	
	UE_LOG(LogTemp,Warning,TEXT("hELLO"))
	//ADD Actor to memory 
	 if (IsItem)
	 {
	 	auto inv = GetOwner()->GetComponentByClass<UInventoryComponent>()->GrabItem(0,IsItem);
		 
	 //	UE_LOG(LogTemp, Warning, TEXT("ItEM gRABBED"));
	 }
	
	
	
	// GO TO ITEM 
	
	//SET key for the BB to check what is going to do 
	
	
	
	//Stimulus.Type             // FAISenseID — which sense fired (Sight, Hearing, Damage, Touch, Team, Prediction)//Stimulus.StimulusLocation // FVector — world location of the stimulus (where the noise was, where the actor was seen)//Stimulus.ReceiverLocation // FVector — where the perceiver was when it sensed it
//Stimulus.Strength         // float — sense-specific intensity (loudness, damage amount, etc.)
//Stimulus.Age              // float — seconds since this stimulus was registered
//Stimulus.ExpirationAge    // float — when the stimulus will be discarded
//Stimulus.Tag              // FName — optional sense-specific tag (e.g., hearing event tag)
	

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		// saw someone
	}
	// else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	// {
	// 	// heard something — Stimulus.Tag often carries the noise event tag
	// }
	//

	
	
	//items , 

	//PurgeZone 

	 //House 
	
	//Zombie 

}
