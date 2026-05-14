// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "Survivor/SurvivorPawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	
	m_PerceptionComponent = GetOwner()->GetComponentByClass<UAIPerceptionComponent>();
	
	m_SurvivorPawn =
	  Cast<ASurvivorPawn>(GetOwner());

	m_PerceptionComponent =
		GetOwner()->FindComponentByClass<UAIPerceptionComponent>();

	if (m_PerceptionComponent)
	{
		m_PerceptionComponent->OnTargetPerceptionUpdated
			.AddDynamic(
				this,
				&UStudentPerceptor::OnPerceptionUpdated
			);
		
		m_PerceptionComponent->OnTargetPerceptionForgotten
			.AddDynamic(
				this,
				&UStudentPerceptor::OnTargetForgotten
			);
	}
}

void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	GetSeenActorsInMemory();
	GetActorsOnFOV();
}


void UStudentPerceptor::OnPerceptionUpdated(AActor * Actor, FAIStimulus Stimulus)
{

	if (Stimulus.Type ==
		UAISense::GetSenseID<UAISense_Sight>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			// UE_LOG(LogTemp, Warning,
			// 	TEXT("Entered sight"));
		}
		else
		{
			// UE_LOG(LogTemp, Warning,
			// 	TEXT("Lost sight"));
		}
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
	{
		//maybe needs to Heal 
		//Or needs to seacrh for Meds 
		//search for guns ? 
		
	}
	
	//items , 

	//PurgeZone 

	//House 

	//Zombie 
}

void UStudentPerceptor::OnTargetForgotten(AActor* Actor)
{
}

TArray<AActor*> UStudentPerceptor::GetSeenActorsInMemory()
{
	TArray<AActor*> Actors;
	m_PerceptionComponent->GetKnownPerceivedActors(
		UAISense_Sight::StaticClass(),
		Actors
	);
	return Actors;
}

TArray<AActor*> UStudentPerceptor::GetActorsOnFOV()
{
	TArray<AActor*> SeenActors;
	m_PerceptionComponent->GetCurrentlyPerceivedActors(
		UAISense_Sight::StaticClass(),
		SeenActors
	);
	return SeenActors;
}

void UStudentPerceptor::ForgetActorsFromMemory(AActor * Actor)
{
	m_PerceptionComponent->ForgetActor(Actor);
}


 //ABaseItem * IsItem = Cast<ABaseItem>(Actor);
// UE_LOG(LogTemp,Warning,TEXT("hELLO"))
// //ADD Actor to memory 
//  if (IsItem)
//  {
//  	auto inv = GetOwner()->GetComponentByClass<UInventoryComponent>()->GrabItem(0,IsItem);
// 	 
//  //	UE_LOG(LogTemp, Warning, TEXT("ItEM gRABBED"));
//  }
//



//FAIStimulus Stimulus
// Perception->ForgetAll();
//Perception->RequestStimuliListenerUpdate();
// const FActorPerceptionInfo* Info = Perception->GetActorInfo(*Actor);
// Perception->HasAnyActiveStimulus(*Actor)
// Perception->GetDominantSense()
// Perception->SetSenseEnabled(UAISense_Sight::StaticClass(),false);
// FActorPerceptionBlueprintInfo Info;

//Stimulus.Type             // FAISenseID — which sense fired (Sight, Hearing, Damage, Touch, Team, Prediction)//Stimulus.StimulusLocation // FVector — world location of the stimulus (where the noise was, where the actor was seen)//Stimulus.ReceiverLocation // FVector — where the perceiver was when it sensed it
//Stimulus.Strength         // float — sense-specific intensity (loudness, damage amount, etc.)
//Stimulus.Age              // float — seconds since this stimulus was registered
//Stimulus.ExpirationAge    // float — when the stimulus will be discarded
//Stimulus.Tag              // FName — optional sense-specific tag (e.g., hearing event tag)


